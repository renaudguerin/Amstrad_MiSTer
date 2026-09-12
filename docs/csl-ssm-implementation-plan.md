# CSL/SSM implementation plan

Written 2026-09-12 for backlog item B4. Stream: **general** (shared tooling plus one
passive RTL observer). This plan follows the research in
[mister-hardware-loop-plan.md](mister-hardware-loop-plan.md) and the working B2 harness
recorded in [b2-device-capture-2026-09-12.md](b2-device-capture-2026-09-12.md). It is an
implementation brief for a fresh session; each phase is separately mergeable and has its
own gate.

## Goal

Run the Logon System SHAKER test walks from the published CSL scripts on the real MiSTer,
and label every capture with the SSM code that SHAKER itself emits, so captures map
one-to-one onto the SHAKERLAND hardware photographs and AMSpiriT reference images.
B2 today covers one hand-built case (module B, test 9). The target is every SHAKER
module and test that our core can run, driven by the author's scripts, without
per-test authoring.

Non-goals: a CSL interpreter inside the FPGA, CRTC types 2/3/4, tape and snapshot
commands, and full CSL conformance claims. Unsupported commands are rejected with a
reason; nothing is silently approximated.

## The two standards in one paragraph each

**CSL v1.4** (`docs/references/Shaker_CSL/CSL-STANDARD-EN.pdf`, bundle at
`https://shaker.logonsystem.eu/Shaker_CSL.zip`) is a host-side script: one command per
line, `;` comments. Commands used by the SHAKER scripts: `csl_version`, `crtc_select`,
`reset`, `wait <µs>`, `disk_insert`, `key_delay <press> <between> [<after CR>]`,
`key_output '<text>'` with `\(RET)`-style special keys and `{...}` chords, `csl_load`.
Also defined: `wait_vsyncoffon`, `wait_driveonoff`, `wait_ssm0000`, `screenshot_name`,
`screenshot [vsync]`, `snapshot*`, `keyboard_write`, `cpc_model`, `gate_array`,
`memory_exp`, `rom_*`, `tape_*`. `wait` is in *emulated* microseconds. On error the
runner must report script name, line number, instruction, reason, script version and
supported version, and keep a log of the last run.

**SSM v1.1** (`docs/references/Shaker_CSL/SSM-STANDARD-EN.pdf`): the Z80 executes two
consecutive undefined-ED instructions `ED LL ED HH`; the code is `HH*256+LL`, recognised
after the HH byte is fetched. Any non-ED byte between the pairs resets recognition
(`ED 3F 00 ED 3E ED 3D` yields `3D3E`). LL and HH are each within the allowed ranges
`00-3F`, `7F-9F`, `A4-A7`, `AC-AF`, `B4-B7`, `BC-BF`, `C0-FD`. Reserved: `0000`
releases `wait_ssm0000`; `FFFE` (`ED FE ED FF`) = named screenshot; `FFFF` = snapshot;
`FFFD`/`FFFC` = Sikoview logging; other `FFxx` reserved. Suggested image name:
`<Emulator>_<CRTC>_<HHLL>.<ext>`. Undefined ED opcodes are two NOPs on real hardware,
so SHAKER runs identically on hardware, emulator and our core. SHAKER 2.5+ emits the
markers; an Excel sheet on the portal maps codes to SHAKERLAND images.

## Why the FPGA changes the picture

On an emulator, "emulated microseconds" need a virtual clock. Our core runs 1:1 real
time from the PLL, so a CSL `wait` is a host sleep whose only error is host latency
(SSH round trip, MBC start-up, Main polling), tens of milliseconds. SHAKER scripts pad
their waits well above the measured screen times recorded in their comments, and the
screens are static. Timing is therefore not the blocker. The precision problem is
confined to the **capture instant**, which is exactly what the SSM marker fixes.

CSL belongs on the host: keyboard, media, reset and configuration are Main/HPS jobs.
SSM belongs in RTL: a four-byte matcher on the M1 fetch stream, passive and cheap.

## Phase 0: CSL runner on the host (no RTL)

New `scripts/hardware-loop/csl_runner.py`, reusing `SSHTransport`, MGL generation,
hash pinning and named-screenshot retrieval from [driver.py](../scripts/hardware-loop/driver.py).
The JSON case format stays for ad-hoc cases; CSL becomes the normal way to drive a
SHAKER walk.

Command semantics on this target:

| Command | Implementation | Notes |
|---|---|---|
| `csl_version` | record; warn if unknown | bundled scripts say 1.0 while using v1.4 forms |
| `reset` / `reset hard` | fresh core load through MGL | equals power-on |
| `reset soft` | reject | no distinct soft reset path from the host |
| `crtc_select 0` | CFG status bit 2 = 1 | `crtc_type = ~status[2]` in `Amstrad.sv` |
| `crtc_select 1`, `1A`, `1B` | CFG status bit 2 = 0 | 1A/1B are one type here; record which the script asked for |
| `crtc_select 2/3/4` | reject | not implemented in the core |
| `crtc_select` after the first key | accept as no-op if the effective bit is unchanged, else reject | a live change needs the OSD |
| `cpc_model 0/1/2` | CFG status bits 5:4 before load | Plus models: reject until Plus CSL is in scope |
| `disk_insert [A] 'x.dsk'` | MGL `S0` mount; drive B `S1` | path resolved under `disk_dir` or the case's media directory |
| `key_delay` | record; MBC timing is fixed | see approximation below |
| `key_output` | CPC characters and `\(XX)` names translated to Linux keycodes for the active ROM layout, sent through MBC `raw_seq` | chords `{..}` become MBC hold/release; `\(KOF)` drops the inter-key wait |
| `key_from_file` | same as `key_output` | |
| `keyboard_write` | reject | no matrix injection port in the core |
| `wait` | host sleep, value in µs | logged as approximate |
| `wait_vsyncoffon`, `wait_driveonoff` | reject in Phase 0 | need core observability |
| `wait_ssm0000` | reject in Phase 0, implemented in Phase 1 | |
| `screenshot_name`, `screenshot_dir` | set the next Main screenshot name and the local output directory | |
| `screenshot` | Main `screenshot <name>.png`, wait for a complete decode | `vsync` option rejected: Main captures asynchronously |
| `snapshot*`, `tape_*`, `rom_*`, `gate_array`, `memory_exp` | reject | |
| `csl_load` | recursive run, depth-limited, cycle-checked | |

CFG editing follows the B2 method: back up the 16-byte `config/Amstrad.CFG`, change only
the bits the script names, verify by hash, restore at the end. Configuration is applied,
not merely declared, but the native PNG still omits the OSD, so the manifest records
"applied by CFG" and never "visually confirmed".

Keyboard: the CSL layer is character-level; the mapping from character to Linux keycode
depends on the ROM's layout. Start from the AZERTY table that B2 established for the
French ROM and add a UK table. Keep the table in one module with a corpus test that
every character used by the 25 bundled scripts is mappable in both layouts.

MBC timing approximation: MBC presses each key for 40 ms with fixed spacing and only
offers whole-second `!s` waits; `key_delay 70000 70000 400000` cannot be honoured
exactly. Check MBC's options for a configurable delay before accepting this; if none,
record the effective timing per key in the manifest as an approximation. SHAKER input
is single characters at a menu, so this is expected to be harmless, but it must be
recorded rather than assumed.

Runner outputs: per-run manifest (effective settings, hashes, ordered command log with
host timestamps, approximations, rejections with the six error fields the standard
lists, screenshot names and SHA-256), the retained Main log, and a `last-run.log`.

Corpus: SHAKER 2.6 (`shaker26.dsk`) with the bundled `SHAKE26*` scripts. The bundle has
no 2.7 scripts; try 2.6 scripts against `shaker27.dsk` once, keep 2.6 if menus differ.

Tests (host, `python3 -m unittest discover -s scripts/hardware-loop`): parser over all
25 bundled scripts with no rejection other than the documented ones; keycode coverage;
chord and `\(KOF)` translation; rejection reporting fields; `csl_load` cycle detection;
dry-run producing MGL and manifest without network. Deterministic and offline.

Acceptance gate: run `MODULE_B/SHAKE26B-1.CSL` on the device from the top through test 9
(module menu, tests 1, Z, 1, 9), stopping at a script line chosen for the run. Every
screenshot is named by the script, complete, and repeatable across two runs. The
manifest lists each approximation and rejection. This replaces the single B2 case as
the standard capture path.

## Phase 1: SSM detector, event ring and OSD toggle (RTL)

**Detector.** New `rtl/ssm_marker.v`, header citing the SSM standard v1.1 and the
SHAKER portal (not CRTC behaviour, so no ACCC attribution is required, but keep the
Logon System credit). Input is the M1 opcode-fetch byte stream. The motherboard already
samples exactly that byte for Plus open-bus behaviour at
[Amstrad_motherboard.v:238](../rtl/Amstrad_motherboard.v:238)
(`~M1_n & ~MREQ_n & ~RD_n`, byte from `cpu_data_bus`); reuse the same condition and
edge so wait states and T80pa clock enables are handled identically. One sample per
fetch: derive a fetch strobe from the falling edge of that condition, not a level.

State machine over consecutive M1 bytes: `ED` then allowed `LL` then `ED` then allowed
`HH` emits `ssm_valid` with `ssm_code = {HH, LL}`. Any other byte resets to idle, and a
resetting byte that is itself `ED` restarts at state 1 (the spec's `ED 3F 00 ED 3E ED 3D`
example). Bytes outside the allowed ranges after an `ED` are real instructions
(`ED 4B` is `LD BC,(nn)`) and reset. Undefined ED opcodes execute in T80 as two M1
fetches like hardware (`Prefix` path in `rtl/T80/T80.vhd`), so both bytes of each pair
appear on the fetch stream. An interrupt taken between the two instructions inserts
non-ED fetches and drops the marker; that is the spec's behaviour and AMSpiriT's too.

**Event record.** On `ssm_valid`, latch `{code[15:0], frame[23:0], line[9:0],
hcc[7:0], field, seq[7:0]}` where `frame` counts VSYNC rising edges since core load,
`line` and `hcc` come from the CRTC wrapper's vertical/horizontal position, and `seq`
is a wrapping event counter. Keep the last record and the count as core registers for
simulation visibility.

**Transport to the host.** Use DDR3, which this core does not touch today
(`DDRAM_*` tied to 0 at [Amstrad.sv:29](../Amstrad.sv:29)). Write a 16-byte header
(magic, format version, write index, overflow flag) plus a 64-entry ring of 16-byte
records at the core-reserved DDR3 base. Verify the exact base and word addressing
against the framework (`sys/sys_top.v` DDRAM port, `sys/ddr_svc.sv`) and Main's
`shmem` code before choosing the address; the MiSTer convention is the core window at
HPS physical `0x30000000`. The host reads it with BusyBox `dd if=/dev/mem` over the
existing SSH transport, so no Main patch and no device daemon are needed. Reject the
alternatives already checked: `status_set` only updates the saved CFG,
`info_req` only shows OSD text, and neither reaches user space.

Overflow: the ring reports overflow in the header rather than losing the fact silently.

**OSD toggle.** `P2O[37],SSM markers,Off,On;` (bit 37 is free; bits 37-41, 45-60 and 63
are unused). Off, the default, holds the detector in reset and gates all DDR3 writes.
On, the detector is passive: it never affects CPU, CRTC or video timing. The toggle
exists to avoid DDR3 traffic and to exclude the rare program that executes undefined
ED sequences by accident, not because the detector can disturb the machine.

**Runner integration.** `wait_ssm0000` polls the ring for a `0000` record newer than
the last consumed sequence number. A `FFFE` record triggers Main's screenshot named
`MISTER_<crtc>_<HHLL>.png` (or the pending `screenshot_name`), and the manifest stores
the record's frame/line and the host timestamps of detection and capture. This capture
lands about one frame or more after the marker and is labelled approximate.
`wait_vsyncoffon` and `wait_driveonoff` can be added the same way if a later script
needs them; the SHAKER scripts do not.

**Simulation vectors** (`make -C sim`, new target in the executing fixture):

- On the executing TV80 fixture with real u765 and EDSK
  (`sim/plus/p10_boot_test_top.v`): a small Z80 program in RAM executing
  `ED FE ED FF` (expect `FFFE`), `ED 00 ED 00` (expect `0000`), the spec example
  `ED 3F 00 ED 3E ED 3D` (expect exactly one event, `3D3E`), `ED 4B nn nn` (no event),
  and `ED 10 ED 10` (allowed range boundary, expect `1010`). Expected codes come from
  the SSM document on paper, cited at the assertion.
- Interrupt split: enable interrupts with a pending INT so it lands between the two
  pairs; expect no event, then the same sequence with `DI`; expect one event.
- T80pa parity: the production CPU is T80pa, not TV80. Add the fetch-strobe check to
  `sim/crtc_t80_test.cpp` or the `t80_trace` bench so the detector is proven on both
  fetch timings, including a wait-stated fetch.
- Toggle: same program with the enable low; expect zero events and zero DDR3 writes.

Gates: `make -C sim` green, synthesis through the normal integration CI (no manual
dispatch), and a `docs/review-debt.md` row if no cross-provider review is available.

Acceptance: `SHAKE26B-1.CSL` through test 9 with markers on. The ring holds one record
per SHAKER screen in script order, each `FFFE` produced a named capture, `wait_ssm0000`
released where the script used it, and the code list agrees with the portal's
code-to-image mapping for those tests.

## Phase 2: exact frame capture (separate gate, after the author answers)

On `FFFE` the core itself writes one native frame into a second DDR3 region: the
production output chain's RGB/DE after `crt_filter`, at the native 16 MHz pixel rate,
one line per raster line, both fields tagged. The host converts it to PNG and compares
it with Main's scaler capture of the same event.

Frame semantics are not settled by the SSM text. It says the screenshot is taken
"immediately after reading the HH byte", mid-frame, but what an immediate screenshot
contains depends on the emulator's rendering model: per-scanline renderers hold the
current frame's top and the previous frame's bottom, per-frame renderers hold the last
completed frame. CSL's `screenshot vsync` option exists for this reason; SSM `FFFE` has
no such option. For static SHAKER screens the choice is invisible; for the scrolling
tests it is not. Ask the author before building this phase:

> For SSM #FFFE, which image is intended: the framebuffer as rendered at the
> instruction (partial frame), the last completed frame, or the next completed frame?
> Does SHAKER emit the marker at a known raster position (for example right after a
> VSYNC wait) so the distinction is moot? How does AMSpiriT implement it?

Until answered, the default is "the next complete frame starting at the first VSYNC
rising edge after the marker", with the marker's own frame/line recorded so the other
readings can be reconstructed from consecutive frames if needed.

This phase touches the B6 video boundary; read
[b6-video-boundary.md](b6-video-boundary.md) first and extend its Verilator fixture
rather than adding a second output path.

## Reference comparison

Naming follows the SSM suggestion with `MISTER` as the emulator name. The portal's test
API and `results.js` routing (see the hardware-loop plan) give the code-to-image
association; cache only the images for the tests being run under ignored
`docs/references/`. Hardware photographs remain the authority; AMSpiriT images are a
convenient pixel-diff partner, not an oracle.

## Open questions

- Frame semantics for `FFFE` (above). Ask the author.
- Whether AMSpiriT scripts exist for SHAKER 2.7 or only 2.6.
- MBC per-key delay configurability (affects the `key_delay` approximation).
- Exact DDR3 core window base and word addressing in this framework version.
- Our UM6845R model: 1A or 1B? SHAKER tests `O` and `E` in module B answer this on the
  device and the answer belongs in `docs/accuracy/`.

## Files expected to change

Phase 0: `scripts/hardware-loop/csl_runner.py`, `scripts/hardware-loop/cpc_keys.py`,
`scripts/hardware-loop/test_csl_runner.py`, `docs/mister-hardware-loop-driver.md`.
Phase 1: `rtl/ssm_marker.v`, `rtl/Amstrad_motherboard.v` (fetch strobe export),
`Amstrad.sv` (CONF_STR bit 37, DDR3 writer, event record), `Amstrad.qsf`/`files.qip`,
`sim/plus/*` executing fixture, `sim/Makefile`, `docs/review-debt.md`.
Phase 2: B6 output chain, `docs/b6-video-boundary.md`.
