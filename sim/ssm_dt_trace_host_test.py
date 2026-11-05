#!/usr/bin/env python3
"""Decode the asset-free RTL packet and reject stale or misleading envelopes."""
from pathlib import Path
import importlib.util
import struct
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location(
    "ssm_dt_trace", ROOT / "scripts/hardware-loop/ssm_dt_trace.py")
decoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decoder)


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def opaque(data, index):
    a, b = struct.unpack_from("<QQ", data, 16 + 16 * index)
    return (a >> 16) | ((b >> 48) << 48), (b >> 16) & 0xFFFFFFFF


def put_opaque(data, index, value, tick):
    a, b = struct.unpack_from("<QQ", data, 16 + 16 * index)
    a = ((value & ((1 << 48) - 1)) << 16) | (a & 0xFFFF)
    b = ((value >> 48) << 48) | (tick << 16) | (b & 0xFFFF)
    struct.pack_into("<QQ", data, 16 + 16 * index, a, b)


def slot(data, index):
    lo, tick = opaque(data, 2 + 2 * index)
    hi, hi_tick = opaque(data, 3 + 2 * index)
    require(tick == hi_tick, "fixture slot timestamps differ")
    return lo | (hi << 64), tick


def put_slot(data, index, value, tick):
    put_opaque(data, 2 + 2 * index, value & ((1 << 64) - 1), tick)
    put_opaque(data, 3 + 2 * index, value >> 64, tick)


def replace_field(value, bit, width, replacement):
    mask = ((1 << width) - 1) << bit
    return (value & ~mask) | (replacement << bit)


def rejects(data, reason):
    try:
        decoder.decode(data)
    except ValueError as error:
        require(reason in str(error), f"wrong rejection: {error}")
    else:
        raise AssertionError(f"accepted invalid {reason} envelope")


def audit_regressions(directory):
    failures = []

    def ordinary_packets():
        for name, written in [("transport-interleaved.bin", 18),
                              ("transport-postflush.bin", 20)]:
            data = (directory / name).read_bytes()
            result = decoder.decode(data)
            require(result["header"]["written"] == written and
                    result["source_qualified_complete_bracket"],
                    "ordinary records must not invalidate the complete DT packet")
        data = bytearray((directory / "transport-postflush.bin").read_bytes())
        header = decoder.ring.parse_header(data)
        # Do not solve post-flush acceptance by ignoring later records.
        last = header.written - 1
        sequence = struct.unpack_from("<H", data, 16 + 16 * last + 8)[0]
        struct.pack_into("<H", data, 16 + 16 * last + 8, sequence ^ 1)
        rejects(data, "sequence")
        data = bytearray((directory / "transport-interleaved.bin").read_bytes())
        status_index = next(i for i in range(18)
                            if struct.unpack_from("<H", data, 16 + 16 * i)[0] == 0xFF2F)
        struct.pack_into("<H", data, 16 + 16 * status_index + 8, status_index ^ 1)
        rejects(data, "sequence")

    def invalid_closing_tail():
        result = decoder.decode((directory / "transport-invalid-closing-tail.bin").read_bytes())
        require(result["status_error"] == 10 and not result["source_qualified_complete_bracket"],
                "honest closing-tail partial must decode without claiming a complete bracket")
        require(result["summary"]["slot_valid_mask"] & 7 == 7,
                "wrong closing tail must retain the preceding valid M1 snapshots")

    for name, check in [("ordinary-interleave-and-postflush", ordinary_packets),
                        ("invalid-closing-tail-partial", invalid_closing_tail)]:
        try:
            check()
        except (ValueError, AssertionError) as error:
            failures.append(name)
            print(f"SSM_DT_AUDIT FAIL {name}: {error}", flush=True)
        else:
            print(f"SSM_DT_AUDIT PASS {name}", flush=True)
    require(not failures, "decoder audit regressions: " + ", ".join(failures))


def main():
    require(len(sys.argv) == 2, "expected the RTL-generated packet path")
    packet = Path(sys.argv[1]).read_bytes()
    result = decoder.decode(packet)
    require(result["schema"] == "DTR1/version1", "DT schema identity")
    require(result["header"]["written"] == 17 and result["header"]["dropped"] == 0,
            "two ordinary markers plus fourteen payloads/status")
    require(result["status_error"] == 0 and result["source_qualified_complete_bracket"],
            "qualified complete bracket status")
    require(result["all_six_slots_present"] and result["summary"]["slot_valid_mask"] == 63,
            "all six snapshot slots are valid")
    require([x["PC"] for x in result["samples"][:3]] == ["69F1", "69FC", "6A07"],
            "ordered physical M1 cut labels")
    require([x["R7"] for x in result["samples"][:3]] == [30, 10, 14],
            "M1 cuts retain pre-OUT register ownership")
    require([x["actual_C4"] for x in result["actual_edge_owners"]] == [10, 14, 30],
            "actual captured edge rows")
    require([x["active_black_masterticks"] for x in result["actual_edge_owners"]] == [5, 7, 0],
            "transport waveform's per-owner active-black measurements")

    # A missing row14 edge shifts row30 into ordinal2; it is not a missing
    # fixed row14 slot. Preserve the source-qualified M1 bracket and sequence.
    missing = bytearray(packet)
    value, tick = slot(missing, 5)
    put_slot(missing, 4, value, tick)
    put_slot(missing, 5, 0, 0)
    summary, tick = slot(missing, 6)
    for bit, width, replacement in [(0, 6, 31), (6, 4, 2), (11, 4, 2),
                                    (34, 18, 0), (52, 18, 0),
                                    (70, 3, 3), (73, 3, 1)]:
        summary = replace_field(summary, bit, width, replacement)
    put_slot(missing, 6, summary, tick)
    ordinal = decoder.decode(missing)
    require(ordinal["source_qualified_complete_bracket"] and not ordinal["all_six_slots_present"],
            "missing raw edge must not invalidate the qualified M1 bracket")
    require([x["actual_C4"] for x in ordinal["actual_edge_owners"]] == [10, 30, None],
            "decoder invented a fixed row identity for an ordinal slot")

    # Parent's failing-first host regression: change only the opaque record's
    # sequence while retaining payload, tick, code, header and committed status.
    stale = bytearray(packet)
    header = decoder.ring.parse_header(packet)
    index = next(i for i in range(max(0, header.written - header.entries), header.written)
                 if struct.unpack_from("<H", packet, 16 + 16 * (i % header.entries))[0] == 0xFF20)
    struct.pack_into("<H", stale, 16 + 16 * (index % header.entries) + 8, (index ^ 1) & 0xFFFF)
    rejects(stale, "sequence")

    wrong_schema = bytearray(packet)
    status, tick = opaque(wrong_schema, 16)
    put_opaque(wrong_schema, 16, (status & 0xFFFFFFFF) | (0x53545231 << 32), tick)
    rejects(wrong_schema, "DTR1")
    # Complete packets still require exact timestamp/elapsed agreement.
    mismatch = bytearray(packet)
    summary, tick = slot(mismatch, 6)
    summary = replace_field(summary, 79, 24, result["summary"]["elapsed_master_ticks"] + 1)
    put_slot(mismatch, 6, summary, tick)
    rejects(mismatch, "elapsed")
    audit_regressions(Path(sys.argv[1]).parent)
    print("SSM_DT_HOST PASS schema/status/count, pre-OUT cuts, ordinal missing edge, sequences, partial closing tail and complete elapsed")


if __name__ == "__main__":
    main()
