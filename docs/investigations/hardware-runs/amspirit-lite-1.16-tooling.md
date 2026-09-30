# AmSpirit Lite 1.16 diagnostic tooling

Documentation checked 2026-09-30 against upstream
[Web API](https://raw.githubusercontent.com/AMSpiriT-Emulator/amspirit-releases/refs/heads/main/docs/lite/web_api.md),
[scripting](https://raw.githubusercontent.com/AMSpiriT-Emulator/amspirit-releases/refs/heads/main/docs/lite/scripting.md), and
[BASIC injection/export](https://raw.githubusercontent.com/AMSpiriT-Emulator/amspirit-releases/refs/heads/main/docs/lite/basic_injection_export.md).
Read-only local checks on the same date found Lite 1.16.0/core 2491682:
`/api/ping` includes ticks and `applied_cmd_seq`; `/api/ga` includes blanking and
palette fields; `/api/crtc` currently returns only `regs` and `selected_reg`.
Command, Lua and capture workflows below are documentation-derived and were not
re-probed against the live machine. Record the actual instance version with
`amspirit.py identity`. The
[helper guide](../../../scripts/amspirit/README.md#api-behaviour-that-shapes-the-helper)
lists which operations still need direct API calls. Emulator observations retain the
[oracle evidence limits](amspirit-oracle-design-2026-09-13.md).

## Order commands before reading state

A successful response accepts a queued command. Save its `cmd_seq`, then poll
`GET /api/ping` until `emu.applied_cmd_seq >= cmd_seq`, with a host deadline and a
short polling interval. This confirms application and an emulation iteration afterwards;
for `/api/step`, the instruction has executed. Resume, run-to and BASIC stepping may
still be running: also wait for the intended stop/event. Acknowledgement alone does not
prove media loaded successfully; check effective state and the application log.
RAM/exec retain their separate `seq` / `emu.ram_apply_seq` contract as well.

For a reproducible media probe, acknowledge config, acknowledge media, acknowledge
the CPR hard reset if needed, then read effective settings and choose the frame origin.
Never reset after loading an SNA. `/api/doc` and its improved HTML rendering/API test
page help discover the running instance's capabilities.

## Time code at breakpoint stops

`emu.ticks`, `/api/z80` and `cpc.getZ80().ticks` count NOP units (1 µs). Subtract
readings within one epoch; hard reset, SNA load and rewind restart the count, soft reset
does not. Continue pacing sessions with `emu.frames` and bounded host waits.

Subscribe to `GET /api/events?topics=z80_bp` before resuming with
`POST /api/config {"paused":false}`. Each `z80_bp` event contains the PC, registers
and ticks at the stop, avoiding a separate HTTP read to timestamp it. Use `/api/state`
for additional beam/device state while stopped. Resuming or stepping executes the
instruction at the stopped PC even if a breakpoint is there; a later return to that PC
hits again. No clear/step/re-arm workaround is needed. Breakpoint sets replace the
previous set and resolve CPU addresses against paging when applied; record the mapping
or use explicit bank locations for banked code.

Ticks at software stops measure CPU-visible timing. They do not themselves expose raw
IRQ assertion, nor establish sub-µs ASIC phase.

## Compare one SNA across classic CRTCs

Set `crtc_type` through `/api/config`, wait for acknowledgement, then upload with:

```sh
curl -fsS -X POST 'http://127.0.0.1:6128/api/media?name=checkpoint.sna&crtc=keep' \
  -H 'Content-Type: application/octet-stream' --data-binary @checkpoint.sna
```

Wait for this response's `cmd_seq`, then record `emu.crtc_type` and other effective
settings. Default `crtc=sna` follows the v3 header. `keep` preserves the pre-load CRTC
only within classic types 0/1/2/4 or within Plus type 3; crossing that boundary uses the
snapshot's type and logs a warning. For our classic core, compare types 0/1. This avoids
resetting away the checkpoint, but still needs cold-boot or hardware confirmation for
consequential conclusions.

The helper performs these acknowledgements for `load checkpoint.sna --crtc keep`.
For a case, set `media.crtc` to `"keep"` and choose the desired `config.crtc_type`;
SNA reset defaults to false. Inspect `media_effect` and
`settings.effective_after_media` in the manifest for overrides and mismatches.

## Inspect CRTC and Gate Array in Lua

`cpc.getCRTC()` now mirrors `/api/crtc`, with `regs[0..13]`, `selected_reg`, ticks
and any extra signals published by the loaded core. Inspect actual keys before relying
on internal counters or sync signals. `cpc.getGateArray()` mirrors `/api/ga`, including
blanking, colours and core-specific signals; older `border`, `inks` and `inks_rgb`
aliases remain. `ink_idx` and `ink_rgb` are keyed 0–15, while legacy `inks` arrays
are 1–16. Use explicit numeric loops for zero-based arrays; `ipairs()`/`#` omit index 0.

## Capture history before the fault

Set `/api/config` before reproduction, for example:

```json
{"rw_enabled":true,"rw_interval":1,"rw_slots":300,"history_size":2000}
```

At 50 Hz this retains about six seconds at one snapshot per frame. Ranges:
`rw_interval` 1–500 frames, `rw_slots` 10–2000, `history_size` 1–100000 instructions
(default 20, not persisted). Changing rewind settings clears rewind history; resizing
instruction history clears both histories. Configure first, then reproduce.
Pause and acknowledge, check `emu.rw_active` / `rw_steps_back`, then acknowledge each
`POST /api/rewind/back` before inspecting state, RAM, history and screenshots. Rewind
changes the tick epoch. 1.16 allocates each snapshot's actual size per the release notes;
upstream API examples still estimate ~1.6 MB per snapshot, so treat that as a rough
budget rather than a fixed allocation guarantee and keep capacity proportionate.

## Diagnose disk input and mechanics

`/api/fdc` (also `state.fdc`) adds motor inertia `spin_us` (0–400000) and per-drive
`connected`, `disk`, `track`, `steps`, `step_us`, `dir`, `changes`. Diff cumulative
`steps` to count head movement and `changes` to distinguish disk replacement even
when `disk` remains true. They support mechanical/load diagnosis, not an ASIC oracle.
Autotype now pauses during disk access; wait for `emu.autotyping` to clear rather than
assuming a fixed typing duration. Keep a host deadline that accommodates disk activity.

## CSL chords and marker checkpoints

`key_output` supports simultaneous groups, e.g. CSL `key_output '{\(SHI)1}'` for
SHIFT+1. Use `\({)` / `\(})` for literal braces. In Lua, use long-bracket strings
(e.g. `key_output([[{\(SHI)1}]])`) so backslashes survive. This is CSL/Lua syntax;
do not assume the helper's HTTP `keys` command interprets it as a chord.

`wait_ssm("0405")` matches bytes `ED 05 ED 04`: the string is hexadecimal with
the second operand first. Numeric `0x0405` works too. It enables SSM automatically;
`wait_ssm0000()` remains the alias for code 0000. Ordinary pairs have a documented
legacy screenshot+snapshot fallback as well as waking a matching wait. Reserved codes
FFFF/FFFE save snapshot/screenshot, FFFD starts a timing window, FFFC logs elapsed
NOPs. Configure output directories/names before marker capture and use bounded host
supervision of the script. SSM still runs through scripts, not a dedicated HTTP endpoint;
matcher parity with our SHAKER runner remains unverified.

## Small BASIC reproductions without a disk image

POST numbered source as `text/plain` to `/api/basic?run=1` (`--data-binary @probe.bas`)
to inject and autotype RUN; optional `reset=1` hard-resets first. Inspect
`/api/basic_state`, `/api/basic_listing`, and `/api/basic_export?verbose=1` to locate
statements and archive source. These are existing capabilities worth using for small
reproducers, not additions claimed for 1.16. Injection replaces program/variable state;
tokenization does not establish valid runtime semantics. Exporting source cannot prove
execution passed: use a RAM result, breakpoint or visible output as the checkpoint.
