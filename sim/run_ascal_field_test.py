#!/usr/bin/env python3
"""Replay CE-qualified video through the production Avalon scaler.

A trace has one `hs vs field de` integer tuple per enabled input pixel. The
optional external trace must contain enough complete frames for acquisition.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", type=Path, action="append", default=[],
                        help="production output trace to replay (repeatable)")
    parser.add_argument("--interlace-trace", type=Path, action="append", default=[])
    parser.add_argument("--recovery-trace", type=Path, action="append", default=[])
    parser.add_argument("--interlace-reference", type=Path, action="append", default=[],
                        help="raw genuine-interlace trace whose field ordering must survive")
    parser.add_argument("--expect", choices=("progressive", "interlace"), default="progressive")
    parser.add_argument("--ghdl", default=os.environ.get("GHDL", "ghdl"))
    args = parser.parse_args()
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
    print("ascal_field_test: PASS")


if __name__ == "__main__":
    main()
