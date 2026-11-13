#!/usr/bin/env python3
"""Replay CE-qualified video through the production Avalon scaler.

Legacy traces contain `hs vs field de` per enabled pixel. Pixel pairs contain
`hs vs f1 de ce r g b` per 64 MHz master clock, including CE=0 rows. The
external reference supplies geometry, chronological field ownership and RGB.
"""
import argparse
import json
import hashlib
import difflib
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from collections import deque

ROOT = Path(__file__).resolve().parent.parent
HEADER = re.compile(r"^HEADER [0-9A-FXUZ]+ \d+ ([01UX]) ([01UX]) (\d+)$", re.M)


def make_trace(path, kind):
    with path.open("w") as out:
        for frame in range(12):
            for y in range(30):
                for x in range(128):
                    field = (frame % 2 if kind == "interlace" else
                             int(kind == "pulse" and 4 <= frame < 8 and y == 10 and 40 <= x < 48))
                    out.write(f"{int(100 <= x < 110)} {int(y < 2)} {field} "
                              f"{int(4 <= y < 24 and x < 80)}\n")


# Pixel mode observes the production writer. Expected image bytes originate
# exclusively in the paired external reference's CE-qualified DE events.
PIXEL_BASE = 0x20000000
MASTER_PS = 15625


def log_lines(output):
    if isinstance(output, Path):
        with output.open() as stream:
            yield from stream
    else:
        yield from output.splitlines()


def log_match(output, pattern):
    for line in log_lines(output):
        match = re.search(pattern, line)
        if match:
            return match
    raise AssertionError(f"missing log record {pattern}")


def file_hash(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def pixel_rows(path):
    with path.open() as stream:
        for number, line in enumerate(stream, 1):
            try:
                row = tuple(map(int, line.split()))
            except ValueError as error:
                raise AssertionError(f"{path}:{number}: expected decimal pixels") from error
            if len(row) != 8 or any(v not in (0, 1) for v in row[:5]) or any(
                    not 0 <= v <= 255 for v in row[5:]):
                raise AssertionError(f"{path}:{number}: expected hs vs f1 de ce r g b")
            yield row


def reference_fields(path):
    fields = []
    current = None
    old_vs = old_de = 0
    count = 0
    disabled = 0
    pending_origin = False
    de_field_latch = None
    for cycle, (hs, vs, field, de, ce, red, green, blue) in enumerate(pixel_rows(path)):
        count = cycle + 1
        disabled += not ce
        if not ce:
            continue
        old_origin = pending_origin
        if vs and not old_vs:
            pending_origin = True
            if current is not None:
                current["end"] = cycle
                if current["rows"]:
                    fields.append(current)
            current = {"start": cycle, "rows": [], "field": None,
                       "placement_field": None}
        if current is not None and de:
            # sys/ascal.vhd:1211-1248 consumes the SOF ticket with the
            # PREVIOUS DE latch, while a coincident DE rise schedules its
            # replacement. Derive that ordering from the external pins.
            if old_origin:
                current["placement_field"] = de_field_latch
                pending_origin = False
            if not old_de:
                current["rows"].append(bytearray())
            current["rows"][-1].extend((red, green, blue))
        if de and not old_de:
            de_field_latch = field
            if current is not None:
                current["field"] = field
        old_vs, old_de = vs, de
    assert disabled, "pixel trace must retain CE=0 master clocks"
    assert len(fields) >= 2, "pixel reference needs two complete fields"
    return fields, count


def prepare_pixels(actual, reference, work):
    fields, count = reference_fields(reference)
    actual_count = sum(1 for _ in pixel_rows(actual))
    assert actual_count == count, "paired pixel traces have different master-clock lengths"
    repetition = None
    if len(fields) < 8:
        # Repeat an externally delimited alternating cycle, preserving every
        # master-clock sample and its CE gaps. One following field flushes the
        # final completed field's header. No memory result selects the cycle.
        first, second = fields[-2:]
        assert {first["field"], second["field"]} == {0, 1}, "reference cycle lacks both fields"
        start, middle, end = first["start"], second["start"], second["end"]
        for source, name in ((actual, "actual"), (reference, "reference")):
            cycle_path = work / f"{name}-cycle.txt"
            tail_path = work / f"{name}-tail.txt"
            with source.open() as stream, cycle_path.open("w") as cycle_out, tail_path.open("w") as tail:
                for index, line in enumerate(stream):
                    if start <= index < end:
                        cycle_out.write(line)
                    if start <= index < middle:
                        tail.write(line)
            target = work / f"{name}-repeated.txt"
            with target.open("wb") as output:
                for _ in range(5):
                    with cycle_path.open("rb") as stream:
                        shutil.copyfileobj(stream, output)
                with tail_path.open("rb") as stream:
                    shutil.copyfileobj(stream, output)
            if name == "actual":
                actual = target
            else:
                reference = target
        repetition = {"source_start": start, "source_end": end, "cycles": 5,
                      "following_field_end": middle}
        fields, count = reference_fields(reference)
    selected = fields[-2:]
    assert {f["field"] for f in selected} == {0, 1}, "settled reference lacks both field parities"
    assert {f["placement_field"] for f in selected} == {0, 1}, "reference lacks both DE-latch row planes"
    for field in selected:
        widths = {len(row) // 3 for row in field["rows"]}
        assert len(widths) == 1 and next(iter(widths)) >= 86, "need uniform rows including pixel85"
        assert len({bytes(row) for row in field["rows"]}) > 1, "reference lacks vertical variation"
        colors = {pixel for row in field["rows"]
                  for pixel in zip(row[::3], row[1::3], row[2::3])}
        assert len(colors) > 1, "reference lacks nonuniform RGB samples"
    return actual, fields, repetition


def check_pixel_memory(output, fields):
    start_match = log_match(output, r"^INPUT_START_FS (\d+)$")
    assert start_match, "missing input clock origin"
    start_fs = int(start_match[1])
    selected = {f["start"]: f for f in fields[-2:]}
    scored = set()
    memory = {}
    beats = 0
    burst_base = None
    headers = 0
    compared = 0
    geometry_passes = 0
    pixel_errors = []
    for line in log_lines(output):
        if not line.startswith("BEAT "):
            continue
        _, stamp, address, burstcount, beat, enables, data = line.split()
        stamp, address, burstcount, beat = map(int, (stamp, address, burstcount, beat))
        assert burstcount == 16 and enables == "FFFF", "invalid RGB24 burst shape/byteenable"
        if beat == 0:
            burst_base = address
        assert address == burst_base and beat == beats % 16, "Avalon address/index changed within burst"
        # Unknown padding is permitted. Unknown scored header or pixel bytes
        # are rejected below when the independent reference names their use.
        assert len(data) == 128, "expected one full 128-bit Avalon beat"
        for lane in range(16):
            text = data[120 - 8 * lane:128 - 8 * lane]
            known = value = 0
            for bit in text:
                known = (known << 1) | (bit in "01")
                value = (value << 1) | (bit == "1")
            memory[16 * (address + beat) + lane] = value, known
        beats += 1
        if beat != 15 or address != PIXEL_BASE // 16:
            continue
        headers += 1
        # Header is emitted after VS falls. Bind to the external field just
        # completed before that VS, independently of header FIELD or pixels.
        completed = [f for f in fields if start_fs + f["end"] * MASTER_PS * 1000 <= stamp * 1000000]
        if not completed:
            continue
        expected = completed[-1]
        if expected["start"] not in selected or expected["start"] in scored:
            continue
        raw = []
        for offset in range(16):
            value, known = memory.get(PIXEL_BASE + offset, (0, 0))
            # Only the three frame-counter bits lack an external oracle.
            # sys/ascal.vhd leaves i_count uninitialized in simulation.
            mask = 31 if offset == 5 else 255
            assert known & mask == mask, f"unknown scored header byte {offset}"
            raw.append(value)
        header = bytes(raw)
        word = lambda at: int.from_bytes(header[at:at + 2], "big")
        width = len(expected["rows"][0]) // 3
        height = len(expected["rows"]) * 2
        stride = 256 * ((3 * width + 255) // 256)
        attrs = word(4)
        assert header[:2] == b"\x01\x01" and word(2) == 256, "wrong RGB24 header format/offset"
        assert attrs & 31 == 1 | expected["field"] << 1, "header field/interlace/downscale mismatch"
        assert (word(6), word(8), word(10)) == (width, height, stride), (
            f"external DE geometry mismatch {(word(6), word(8), word(10))} vs {(width, height, stride)}")
        assert word(12) >= width and word(14) >= height, "output bounds downscale input"
        geometry_passes += 1
        # sys/ascal.vhd:1232 uses the prior DE latch for row placement.
        # The header independently describes the completed field's latch.
        for local_row, rgb in enumerate(expected["rows"]):
            row = 2 * local_row + (1 - expected["placement_field"])
            base = PIXEL_BASE + 256 + row * stride
            for offset, value in enumerate(rgb):
                got, known = memory.get(base + offset, (0, 0))
                if known != 255:
                    got = None
                compared += 1
                if got != value and len(pixel_errors) < 8:
                    pixel_errors.append((expected["field"], local_row, offset // 3,
                                         "RGB"[offset % 3], value, got))
        scored.add(expected["start"])
    assert beats and beats % 16 == 0, "missing/incomplete accepted bursts"
    assert len(scored) == 2, f"expected two chronologically completed fields, scored {len(scored)}"
    assert not pixel_errors, (f"pixel mismatch after {geometry_passes} geometry/header passes: "
                              f"{pixel_errors}")
    return {"fields": 2, "pixel_bytes": compared, "accepted_beats": beats,
            "headers": headers, "cross_beat_pixel": 5, "cross_burst_pixel": 85,
            "prior_de_latch_memory_rows": {"0": "odd", "1": "even"},
            "scored_fields": [{"field": f["field"], "prior_de_latch": f["placement_field"],
                               "start": f["start"], "end": f["end"],
                               "rows": len(f["rows"]), "width": len(f["rows"][0]) // 3,
                               "expected_rgb_sha256": hashlib.sha256(b"".join(f["rows"])).hexdigest()}
                              for f in fields[-2:]],
            "clock_period_ps": MASTER_PS, "beat_time_unit": "ns",
            "unscored_header_bits": "frame-counter attribute bits7..5"}



def synthetic_reader_trace(path):
    """Stable interlaced RGB ruler, specified independently of scaler memory."""
    with path.open("w") as output:
        for frame in range(12):
            field = frame % 2
            for y in range(14):
                for x in range(128):
                    row = 2 * (y - 4) + field
                    rgb = ((17*x + 31*row + 3) % 256,
                           (43*x + 7*row + 19) % 256,
                           (11*x + 53*row + 101) % 256)
                    for phase in range(4):
                        output.write(f"{int(104 <= x < 112)} {int(y < 2)} {field} "
                                     f"{int(4 <= y < 12 and x < 96)} {int(phase == 0)} "
                                     + " ".join(map(str, rgb)) + "\n")


def reader_image(fields):
    """Weave complete external fields using the documented prior-DE latch."""
    selected = fields[-2:]
    heights = {len(field["rows"]) for field in selected}
    widths = {len(row)//3 for field in selected for row in field["rows"]}
    assert len(heights) == len(widths) == 1, "reader reference has inconsistent geometry"
    width, height = next(iter(widths)), 2*next(iter(heights))
    image = [None] * height
    for field in selected:
        for y, row in enumerate(field["rows"]):
            image[2*y + 1-field["placement_field"]] = row
    assert all(row is not None and len(row) == width*3 for row in image), "missing external row plane"
    return width, height, image


def check_reader_output(output, fields, scale):
    """Stream complete output frames against the independently woven image."""
    start = int(log_match(output, r"^READER_START_NS (\d+)$")[1])
    input_start = int(log_match(output, r"^INPUT_START_FS (\d+)$")[1])
    assert all(input_start + f["end"] * MASTER_PS * 1000 < start * 1000000
               for f in fields[-2:]), "reader started before complete external fields"
    width, height, image = reader_image(fields)
    output_width, output_height = width*scale, height*scale
    frames = deque(maxlen=2)
    frame_count = compared = 0
    errors = []
    x, y = 0, -1
    old_vs = old_de = 0
    reads = returns = stalls = 0
    synchronized = False
    active_seen = False
    for line in log_lines(output):
        returns += line.startswith("RETURN ")
        stalls += line.startswith("READ_STALL ")
        if line.startswith("READ "):
            _, stamp, address, count = line.split()
            assert int(stamp) >= start and int(count) == 16
            reads += 1
        if not line.startswith("OUT "):
            continue
        _, stamp, hs, vs, de, border, rgb = line.split()
        if not synchronized:
            if vs != "1":
                continue
            synchronized = True
        assert all(value in ("0", "1") for value in (hs, vs, de, border)), "unknown settled output timing"
        vs, de = int(vs), int(de) and not int(border)
        if old_de and not de and x != output_width and len(errors) < 8:
            errors.append(("row width", y, x, output_width))
        if vs and not old_vs:
            if active_seen:
                if y+1 != output_height and len(errors) < 8:
                    errors.append(("frame height", y+1, output_height))
                frames.append((compared, errors))
                frame_count += 1
            x, y, compared, errors = 0, -1, 0, []
        if de:
            active_seen = True
            if not old_de:
                x = 0
                y += 1
            if x < output_width and y < output_height:
                expected = image[y//scale][3*(x//scale):3*(x//scale)+3]
                actual = bytes.fromhex(rgb) if re.fullmatch(r"[0-9A-F]{6}", rgb) else None
                if actual != expected and len(errors) < 8:
                    errors.append((x, y, expected.hex(), rgb))
            elif len(errors) < 8:
                errors.append(("pixel outside image", x, y))
            x += 1
            compared += 1
        old_vs, old_de = vs, de
    assert reads > 0 and stalls > 0, "reader did not exercise Avalon acceptance/backpressure"
    assert returns == reads*16, "incomplete Avalon read responses"
    assert frame_count >= 3, "reader did not emit three complete acquisition frames"
    for pixels, errors in frames:
        assert not errors, f"scaled output RGB/geometry mismatch {errors}"
        assert pixels == output_width*output_height, "incomplete scored output frame"
    return {"scale": scale, "input_width": width, "input_height": height,
            "output_width": output_width, "output_height": output_height,
            "scored_frames": 2, "compared_pixels": sum(frame[0] for frame in frames),
            "accepted_read_bursts": reads, "returned_beats": returns, "stalled_read_clocks": stalls,
            "image_sha256": hashlib.sha256(b"".join(image)).hexdigest(),
            "acquisition": "frozen after external complete fields and writer drain", "reader_start_ns": start}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", type=Path, action="append", default=[],
                        help="production output trace to replay (repeatable)")
    parser.add_argument("--interlace-trace", type=Path, action="append", default=[])
    parser.add_argument("--recovery-trace", type=Path, action="append", default=[])
    parser.add_argument("--interlace-reference", type=Path, action="append", default=[],
                        help="raw genuine-interlace trace whose field ordering must survive")
    parser.add_argument("--pixel-trace", type=Path, action="append", default=[],
                        help="eight-column master-clock actual input (repeatable)")
    parser.add_argument("--pixel-reference", type=Path, action="append", default=[],
                        help="paired independent expected input, in argument order")
    parser.add_argument("--pixel-log-dir", type=Path,
                        help="retain accepted Avalon beat logs and pixel receipts")
    parser.add_argument("--synthetic-reader", action="store_true",
                        help="optional synthetic nearest-reader proof at 1x and 2x")
    parser.add_argument("--reader-trace", type=Path, action="append", default=[],
                        help="optional actual master-clock input for scaled reader proof")
    parser.add_argument("--reader-reference", type=Path, action="append", default=[],
                        help="paired independent input reference for reader proof")
    parser.add_argument("--reader-scale", type=int, choices=(1, 2), action="append", default=[],
                        help="output scale paired with each --reader-trace")
    parser.add_argument("--reader-log-dir", type=Path,
                        help="retain writer/reader logs, source adapter and receipts")
    parser.add_argument("--expect", choices=("progressive", "interlace"), default="progressive")
    parser.add_argument("--ghdl", default=os.environ.get("GHDL", "ghdl"))
    args = parser.parse_args()
    if len(args.pixel_trace) != len(args.pixel_reference):
        parser.error("each --pixel-trace requires its paired --pixel-reference")
    if not len(args.reader_trace) == len(args.reader_reference) == len(args.reader_scale):
        parser.error("each --reader-trace requires --reader-reference and --reader-scale")
    ghdl = shutil.which(args.ghdl)
    if not ghdl:
        raise SystemExit("ghdl is required")
    with tempfile.TemporaryDirectory(prefix="ascal-field-") as td:
        work = Path(td)
        def command(*cmd):
            subprocess.run(cmd, cwd=work, check=True)
        for source in (ROOT / "sys/ascal.vhd", ROOT / "sim/ascal_field_test.vhd"):
            command(ghdl, "-a", "--std=08", str(source))
        command(ghdl, "-e", "--std=08", "ascal_field_test")

        def run(trace):
            result = subprocess.run([ghdl, "-r", "--std=08", "ascal_field_test",
                                     f"-gtrace_path={trace.resolve()}", "--assert-level=error"],
                                    cwd=work, text=True, capture_output=True, check=True)
            headers = HEADER.findall(result.stdout)
            if len(headers) < 7:
                raise AssertionError(f"Expected at least seven headers, got {len(headers)}")
            return headers[3:]

        for kind in ("progressive", "interlace", "pulse"):
            trace = work / f"{kind}.txt"
            make_trace(trace, kind)
            headers = run(trace)
            flags = {x[0] for x in headers}
            fields = {x[1] for x in headers}
            if kind == "progressive":
                assert flags == {"0"}, f"progressive control: {headers}"
            elif kind == "interlace":
                assert flags == {"1"} and fields == {"0", "1"}, f"interlace control: {headers}"
            else:
                # This is an input to the scaler, not a claim that transient
                # field pulses represent true interlace. It establishes the
                # integration hazard addressed by the producer fix.
                assert flags == {"0", "1"} and all(x[0] == "1" for x in headers[2:6]) and all(x[0] == "0" for x in headers[-2:]), f"pulse acquisition/recovery control: {headers}"
            print(f"{kind} control: PASS ({len(headers)} headers)", flush=True)
        traces = [(p, args.expect) for p in args.trace]
        traces += [(p, "interlace") for p in args.interlace_trace]
        traces += [(p, "recovery") for p in args.recovery_trace]
        if args.interlace_reference and len(args.interlace_reference) != len(args.interlace_trace):
            raise AssertionError("Each genuine-interlace trace needs its own raw reference")
        references = dict(zip(args.interlace_trace, args.interlace_reference))
        for trace, expect in traces:
            headers = run(trace)
            flags, fields = {x[0] for x in headers}, {x[1] for x in headers}
            if expect == "progressive":
                assert flags == {"0"}, f"producer trace acquired interlace: {headers}"
            elif expect == "recovery":
                assert "1" in flags and all(x[0] == "0" for x in headers[-3:]), \
                    f"producer trace did not recover progressive frames: {headers}"
            else:
                assert flags == {"1"} and fields == {"0", "1"}, f"producer trace lost interlace fields: {headers}"
                if trace in references:
                    reference = run(references[trace])
                    assert headers == reference, \
                        f"genuine field ordering/geometry changed: {headers} vs raw {reference}"
            print(f"producer {trace.name} {expect}: PASS ({len(headers)} headers)", flush=True)
        if args.synthetic_reader or args.reader_trace:
            # sys/ascal.vhd Scalaire sREAD forces off_v=0 in normal-buffer
            # mode, and sWAIT enters sSHIFT only for o_off(0)>0. Therefore
            # o_pshift is unconsumed here. Keep that precondition executable
            # while avoiding GHDL's natural-range failure on the idle load.
            source = (ROOT / "sys/ascal.vhd").read_text()
            old = "o_pshift<=o_off(0) -1;"
            new = ('ASSERT o_fb_ena=\'0\' AND o_off(0)=0 '
                   'REPORT "reader phase adapter outside normal-buffer zero-offset scope" SEVERITY failure; '
                   'o_pshift<=0;')
            assert source.count(old) == 1, "ASCAL reader phase load changed"
            compiled = source.replace(old, new)
            overlay = work / "ascal-reader.vhd"
            overlay.write_text(compiled)
            patch = "".join(difflib.unified_diff(source.splitlines(True), compiled.splitlines(True),
                                               fromfile="sys/ascal.vhd", tofile="ascal-reader.vhd"))
            if args.reader_log_dir:
                args.reader_log_dir.mkdir(parents=True, exist_ok=True)
                (args.reader_log_dir / "ascal-reader-phase.patch").write_text(patch)
                (args.reader_log_dir / "ascal-reader-phase.json").write_text(json.dumps({
                    "original_sha256": hashlib.sha256(source.encode()).hexdigest(),
                    "compiled_sha256": hashlib.sha256(compiled.encode()).hexdigest(),
                    "scope": "normal-buffer zero offset, sSHIFT unreachable, o_pshift unconsumed",
                    "source_argument": "sREAD forces off_v=0 when o_fb_ena=0; sWAIT enters sSHIFT only if o_off(0)>0",
                    "runtime_precondition": "o_fb_ena=0 and o_off(0)=0 on every sWAIT phase load"}, indent=2)+"\n")
            command(ghdl, "-a", "--std=08", str(overlay))
            command(ghdl, "-a", "--std=08", str(ROOT / "sim/ascal_field_test.vhd"))
            command(ghdl, "-e", "--std=08", "ascal_field_test")
            jobs = []
            if args.synthetic_reader:
                trace = work / "synthetic-reader.txt"
                synthetic_reader_trace(trace)
                fields, _ = reference_fields(trace)
                jobs += [(f"reader-{scale}", trace, trace, trace, fields, None, scale, 64, 16)
                         for scale in (1, 2)]
            for index, (actual, reference, scale) in enumerate(zip(
                    args.reader_trace, args.reader_reference, args.reader_scale)):
                replay_work = work / f"reader-{index}"
                replay_work.mkdir()
                replay, fields, repetition = prepare_pixels(actual, reference, replay_work)
                jobs.append((f"reader-real-{index}-{scale}x", actual, reference, replay,
                             fields, repetition, scale, 192, 54))
            for name, actual, reference, replay, fields, repetition, scale, hblank, vblank in jobs:
                width, height, _ = reader_image(fields)
                assert width*scale <= 2048 and height*scale+vblank <= 4095, "reader output exceeds scaler bounds"
                assert width*scale+hblank <= 4095, "reader horizontal total exceeds uint12"
                log_dir = args.reader_log_dir or work
                log_dir.mkdir(parents=True, exist_ok=True)
                log_path = log_dir / f"{name}.log"
                run_command = [ghdl, "-r", "--std=08", "ascal_field_test",
                               f"-gtrace_path={replay.resolve()}", "-gpixel_mode=true",
                               "-greader_mode=true", f"-greader_scale={scale}",
                               f"-greader_width={width}", f"-greader_height={height}",
                               f"-greader_hblank={hblank}", f"-greader_vblank={vblank}",
                               "--assert-level=error", "--ieee-asserts=disable"]
                print(f"reader starting {name} input={width}x{height} scale={scale}", flush=True)
                with log_path.open("w") as output:
                    result = subprocess.run(run_command, cwd=work, stdout=output, stderr=subprocess.STDOUT)
                if result.returncode:
                    with log_path.open("rb") as output:
                        output.seek(max(0, log_path.stat().st_size-4000))
                        raise AssertionError(output.read().decode(errors="replace"))
                writer = check_pixel_memory(log_path, fields)
                if args.reader_log_dir:
                    (log_dir / f"{name}-writer.json").write_text(json.dumps(writer, indent=2)+"\n")
                print(f"{name} writer: PASS", flush=True)
                receipt = {"writer": writer, "reader": check_reader_output(log_path, fields, scale),
                           "actual": str(actual.resolve()), "reference": str(reference.resolve()),
                           "actual_sha256": file_hash(actual), "reference_sha256": file_hash(reference),
                           "repeated_cycle": repetition, "command": run_command,
                           "output_htotal": width*scale+hblank, "output_vtotal": height*scale+vblank,
                           "source_sha256": {str(source.relative_to(ROOT)): file_hash(source) for source in
                               (ROOT / "sys/ascal.vhd", ROOT / "sim/ascal_field_test.vhd",
                                ROOT / "sim/run_ascal_field_test.py")}, "log_sha256": file_hash(log_path)}
                if args.reader_log_dir:
                    (log_dir / f"{name}.json").write_text(json.dumps(receipt, indent=2)+"\n")
                print(f"{name}: PASS {json.dumps(receipt, sort_keys=True)}", flush=True)
        adapter = None
        if args.pixel_trace:
            # GHDL checks an otherwise inactive output counter's natural
            # range at every running OCLK. Four-bit wrap compatibility
            # adapter for that sole idle-reader assignment. The input writer
            # and all other range checks retain their production source.
            source = (ROOT / "sys/ascal.vhd").read_text()
            old = "o_pshift<=o_off(0) -1;"
            new = "o_pshift<=(o_off(0)+15) mod 16;"
            assert source.count(old) == 1, "ASCAL idle-reader adapter source changed"
            compiled = source.replace(old, new)
            overlay = work / "ascal-pixel.vhd"
            overlay.write_text(compiled)
            patch = "".join(difflib.unified_diff(source.splitlines(True), compiled.splitlines(True),
                                               fromfile="sys/ascal.vhd", tofile="ascal-pixel.vhd"))
            adapter = {"original_sha256": hashlib.sha256(source.encode()).hexdigest(),
                       "compiled_sha256": hashlib.sha256(compiled.encode()).hexdigest(),
                       "patch_sha256": hashlib.sha256(patch.encode()).hexdigest(),
                       "scope": "idle output counter four-bit wrap; run=0 o_ce=0; no Avalon reads"}
            if args.pixel_log_dir:
                args.pixel_log_dir.mkdir(parents=True, exist_ok=True)
                (args.pixel_log_dir / "ascal-idle-reader.patch").write_text(patch)
                (args.pixel_log_dir / "ascal-adapter.json").write_text(json.dumps(adapter, indent=2)+"\n")
            command(ghdl, "-a", "--std=08", str(overlay))
            command(ghdl, "-a", "--std=08", str(ROOT / "sim/ascal_field_test.vhd"))
            command(ghdl, "-e", "--std=08", "ascal_field_test")
            print(f"pixel compiler adapter: {json.dumps(adapter, sort_keys=True)}", flush=True)
        for index, (actual, reference) in enumerate(zip(args.pixel_trace, args.pixel_reference)):
            replay, fields, repetition = prepare_pixels(actual, reference, work)
            result = subprocess.run([ghdl, "-r", "--std=08", "ascal_field_test",
                                     f"-gtrace_path={replay.resolve()}", "-gpixel_mode=true",
                                     "--assert-level=error", "--ieee-asserts=disable"], cwd=work, text=True,
                                    capture_output=True)
            if args.pixel_log_dir:
                args.pixel_log_dir.mkdir(parents=True, exist_ok=True)
                (args.pixel_log_dir / f"pixel-{index}.avalon.log").write_text(result.stdout)
            if result.returncode:
                raise AssertionError(result.stdout[-4000:] + result.stderr)
            receipt = check_pixel_memory(result.stdout, fields)
            receipt.update(actual=str(actual.resolve()), reference=str(reference.resolve()),
                           actual_sha256=file_hash(actual), reference_sha256=file_hash(reference),
                           repeated_cycle=repetition, compiler_adapter=adapter)
            if args.pixel_log_dir:
                (args.pixel_log_dir / f"pixel-{index}.json").write_text(json.dumps(receipt, indent=2)+"\n")
            print(f"pixel {actual.name}: PASS {json.dumps(receipt, sort_keys=True)}", flush=True)
    print("ascal_field_test: PASS")


if __name__ == "__main__":
    main()
