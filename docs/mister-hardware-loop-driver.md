# MiSTer capture drivers

Two entry points share one transport, MGL generation, hash pinning and capture
retrieval. [driver.py](../scripts/hardware-loop/driver.py) runs a single
hand-written JSON case and is the right tool for an ad-hoc probe.
[csl_runner.py](../scripts/hardware-loop/csl_runner.py) runs a Logon System CSL
script and is the normal way to drive a SHAKER walk; see
[the CSL runner](#the-csl-runner) below.

## The JSON case driver

[driver.py](../scripts/hardware-loop/driver.py) loads an existing device RBF and
DSK/CPR through MGL, optionally sends a bounded MBC input sequence, then requests
three native PNG screenshots serially. Each download must decode completely
before the next screenshot request. The September 12 device run established
French-ROM SHAKER 2.7 boot and B (9) navigation on `5c16b17`; see the
[device record](b2-device-capture-2026-09-12.md) for repeated-load evidence and
configuration-observation limits.

## Local use

Requires Python 3.10+, SSH and SCP. Live capture and tests also need Pillow.
If Pillow is absent, install it in a task-local environment:

```sh
python3 -m venv docs/references/b2-python
. docs/references/b2-python/bin/activate
python3 -m pip install Pillow
```

The dry-run does not import Pillow or contact the network:

```sh
python3 scripts/hardware-loop/driver.py scripts/hardware-loop/example_case.json \
  --dry-run --out-dir docs/references/b2-dry-run
python3 -m unittest discover -s scripts/hardware-loop -v
```

Dry-run writes an MGL and JSON manifest containing the effective case, target
(if supplied), ordered plan, and explicit `hardware_contacted: false`. Use a new
output directory for every run; an existing manifest is never overwritten.

## Case and device prerequisites

Copy the [example case](../scripts/hardware-loop/example_case.json) and replace
its paths and input, or start from the device-tested
[French SHAKER 2.7 B (9) case](../scripts/hardware-loop/shaker27-b9-fr.json).
The latter pins this session’s exact RBF/media bytes and assumes the saved
classic 6128 / CRTC 1 / Full configuration. It does not apply those settings.
The generic example is a template: its Enter key does **not** select a known
SHAKER test. Record the model, CRTC, Full/Live/Off setting, output mode, scaler
and CRT settings in `declared_settings`. These settings are recorded, not applied
or verified by the driver. Confirm the selected test and model from the captured
footer. A mounted DSK does not automatically type BASIC's RUN command.

- Explicit SSH target and optional port; existing batch SSH authentication and
  host-key trust. The driver does not configure accounts or install device tools.
- Installed Main supporting `load_core` and named `screenshot` commands, writable
  `/dev/MiSTer_cmd` FIFO, active root `/media/fat`, and **active** PNG screenshots.
  `--ack-main-cmd` acknowledges this whole contract. On-disk INI content is saved
  with its sections intact; it does not establish the active configuration.
- Remote `timeout` supporting `-s KILL`, `sha256sum`, `scp`, and normal BusyBox
  shell utilities. The driver uses [legacy SCP mode](https://man.openbsd.org/scp#O)
  so an SFTP subsystem is unnecessary. Missing command tools fail explicitly.
- Absolute existing RBF/media paths under `/media/fat`, without parent traversal.
  MGL supports DSK S0/S1 and CPR F8 for this core. Ambiguous RBF prefix matches
  are rejected: use a distinct build filename without matching variants.
- If input is supplied, an executable MBC and writable `/dev/uinput`. See the
  [optional pinned ARM cross-build recipe](mister-mbc-cross-build.md).

```sh
python3 scripts/hardware-loop/driver.py my-case.json \
  --target root@mister.local --port 22 --ack-main-cmd \
  --out-dir docs/references/b2-run-001
```

Optional `expected_sha256` contains `rbf` and/or `media` hashes. A mismatch
stops before MGL upload, core load, or input; the failure manifest retains the
actual hashes. Omit pins for discovery runs. Pins check on-disk bytes before
load; keep one device operator and do not replace files during a run.

Input accepts lowercase ASCII letters and digits, MBC navigation keys `UDLROEHFM`, `:XX` taps,
balanced `{XX`/`}XX` presses/releases, and at most 30 `!s` waits. Whitespace,
Unicode, unbalanced holds and MBC's unbounded `!m` wait are rejected. Each
invocation uses 40 ms key timing and 1000 ms before/after discovery waits. Set
`timeouts.cmd_timeout` long enough for the sequence; host and remote timeouts
bound execution independently. Remote timers round up to whole seconds.
An interrupted sequence gets a best-effort release attempt and an uncertainty
record; a new MBC process cannot prove release of an earlier uinput device.
Inspect/reset the target before retrying an interrupted case.

## Evidence and limits

The manifest records actual on-disk SHA-256 values for RBF, media and Main,
partial identity on failure, command/transfer output, failures, cleanup results,
and each decoded capture's dimensions and SHA-256. Local MGLs and images remain;
the unique remote MGL is removed after use, while remote screenshots remain.
A failed load command stops before input/capture. Main does not acknowledge that
a dispatched load succeeded: successful capture means the transport and image
roundtrip completed, not that the requested core/model/test was confirmed.
The on-disk Main hash does not prove which binary is currently running.

Three captures follow **one** load/input sequence. Wait for each run to exit and finish cleanup before starting another; do not
use successful image transfer or an interim manifest as process completion.
Inspect them for tearing,
alternation and state differences; rerun the case into another output directory
to test repeatability across loads. No pixel equality or hardware pass is inferred.
CSL, SSM and exact event capture remain separate gates in the
[hardware-loop plan](mister-hardware-loop-plan.md).

Pinned contracts: Main
[input dispatch and FIFO creation](https://github.com/MiSTer-devel/Main_MiSTer/blob/f8dc68e3dcf4694f5593e6552aea56cd852982af/input.cpp),
[MGL prefix resolution](https://github.com/MiSTer-devel/Main_MiSTer/blob/f8dc68e3dcf4694f5593e6552aea56cd852982af/support/arcade/mra_loader.cpp#L1224),
[named screenshot paths](https://github.com/MiSTer-devel/Main_MiSTer/blob/f8dc68e3dcf4694f5593e6552aea56cd852982af/file_io.cpp#L928),
[asynchronous scaler capture](https://github.com/MiSTer-devel/Main_MiSTer/blob/f8dc68e3dcf4694f5593e6552aea56cd852982af/scaler.cpp#L539),
and [MBC parser/device lifecycle](https://github.com/pocomane/MiSTer_Batch_Control/blob/3873450d413c30e6b0339e6b3dbf2373e0e5a74a/mbc.c#L553).

## French SHAKER 2.7 launch

The device’s French ROM uses AZERTY key positions. `CAT` is
`:2E:10:14:1C`; `RUN"SHAKE27B` plus Enter is
`:13:16:31:04:1F:23:10:25:12{2A:03:08}2A:30:1C`. Unshifted Linux KEY_3
produces the opening quote; Shift+2/7 produces the filename digits. BASIC accepts
the omitted closing quote. After the module loads, SHAKER reads physical menu
keys: `:0A` selects B (9) without Shift. The case records this session’s temporary `/tmp/mbc-b2-20260912` path.
After cleanup that executable is absent: copy the retained pinned binary to an
unused temporary device path and pass `--mbc-path <that-path>` to reuse the case.
The saved case waits 15 seconds for
disk load, then 15 seconds for the numeric screen to finish. These are host
delays, not emulated frame counts.

Run three times sequentially into distinct output directories to check fresh
loads. Inspect the CRTC footer and actual numeric test in the PNGs. Native
Main screenshots exclude the OSD; a saved CFG or a case declaration alone
does not visually confirm the active filter mode. Record that distinction.


## The CSL runner

[csl_runner.py](../scripts/hardware-loop/csl_runner.py) executes a CSL v1.4
script from the Logon System bundle (`docs/references/Shaker_CSL/`, untracked)
against the same device contract as the JSON driver. It is phase 0 of
[the CSL/SSM plan](csl-ssm-implementation-plan.md): the host half, with no RTL
change. The optional phase 1 detector is described under [SSM markers](#ssm-markers).

```sh
# Offline: parse, validate, plan. Contacts nothing, imports no Pillow.
python3 scripts/hardware-loop/csl_runner.py \
  docs/references/Shaker_CSL/MODULE_B/SHAKE26B-1.CSL \
  --rbf-path /media/fat/_Computer/Amstrad_20260911_5c16b17.rbf \
  --disk-dir /media/fat/games/Amstrad/dsk --layout fr --dry-run \
  --out-dir docs/references/csl-dry-run

# On the device, stopping partway and capturing at chosen script lines.
python3 scripts/hardware-loop/csl_runner.py \
  docs/references/Shaker_CSL/MODULE_B/SHAKE26B-1.CSL \
  --rbf-path /media/fat/_Computer/Amstrad_20260911_5c16b17.rbf \
  --disk-dir /media/fat/games/Amstrad/dsk --layout fr \
  --target root@mister --ack-main-cmd --mbc-path /tmp/mbc \
  --stop-at 120 --screenshot-at 110 --screenshot-at 118 \
  --out-dir docs/references/csl-b1-run1
```

`--no-follow-loads` keeps the run inside one file; by default `csl_load` chains
are followed, depth-limited and cycle-checked.

### What the target can and cannot honour

| Command | On this target |
|---|---|
| `csl_version` | recorded; an unknown version is a manifest warning, not a stop. The bundled scripts declare 1.0 while using v1.4 forms |
| `reset` / `reset hard` | fresh core load, equal to power-on |
| `reset soft` | rejected: no distinct soft-reset path exists from the host |
| `crtc_select 0` | status bit 2 set (`crtc_type = ~status[2]` in `Amstrad.sv`) |
| `crtc_select 1`, `1A`, `1B` | status bit 2 clear. 1A and 1B are one model here; which one the script asked for is recorded |
| `crtc_select 2/3/4` | rejected: not implemented in the core |
| `crtc_select` mid-session | a no-op when the effective bit is unchanged, rejected otherwise: a live change needs the OSD |
| `cpc_model 0/1/2` | status bits 5:4 before the load. Plus models are rejected until Plus CSL is in scope |
| `disk_insert [A\|B] 'x.dsk'` | MGL `S0`/`S1` mount, resolved under `--disk-dir` |
| `disk_dir` | rejected: it names a host directory. Use `--disk-dir` |
| `key_delay` | drives `MBC_KEY_WAIT` (see below) |
| `key_output` | translated to Linux keycodes for the ROM layout and sent through MBC `raw_seq` |
| `key_from_file` | rejected: inline the text with `key_output` |
| `keyboard_write` | rejected: the core has no matrix injection port |
| `wait` | host sleep of the emulated microseconds, bounded by `--max-wait` |
| `wait_vsyncoffon`, `wait_driveonoff`, `wait_ssm0000` | rejected in phase 0; they need core observability |
| `screenshot_name` | names the next capture |
| `screenshot_dir` | rejected: captures land under `--out-dir` |
| `screenshot` | Main `screenshot <name>.png`, polled to a complete decode. The `vsync` option is rejected because Main captures the scaler output asynchronously |
| `snapshot*`, `tape_*`, `rom_*`, `gate_array`, `memory_exp` | rejected |
| `csl_load` | followed recursively, depth-limited, cycle-checked |

A rejected command stops the script and is reported with the six fields CSL
requires: script, line, instruction, reason, script version, supported version.
The manifest and `last-run.log` are written even then, holding the partial
trace up to the stop.

### Power-on folding

A core load applies the CFG and mounts the media in one step, so both sides of
a `reset` feed it. The SHAKER scripts put `crtc_select` before the reset and
`disk_insert` after it, separated by a boot `wait`; the runner binds both to
that power-on and records the reordering as an approximation. The window ends
at the first command needing a running machine, which is why a `crtc_select`
arriving mid-session is still judged as a live change.

### Configuration is applied, not observed

The runner reads the 16-byte `config/Amstrad.CFG`, changes only the status bits
the script names, confirms the write by SHA-256, and restores the original
bytes at the end. It refuses to run if that file is absent: set the base
configuration once through the OSD. Native PNGs exclude the OSD, so the
manifest records "applied by CFG" and never "visually confirmed" — the same
limit the [device record](b2-device-capture-2026-09-12.md) established.

### Keyboard translation

Character to keystroke goes through three layers, in
[cpc_keys.py](../scripts/hardware-loop/cpc_keys.py): the ROM layout decides
which CPC key position prints a character, `CPC_KEY_TO_LINUX` decides which
Linux keycode reaches that position through Main and `rtl/hid.sv`, and the
sequence builder holds SHIFT across a run of shifted keys rather than tapping
it per key.

Fifteen entries of that table are confirmed on hardware: the runner's French
translation of `RUN"SHAKE27B` reproduces the device sequence in the
[device record](b2-device-capture-2026-09-12.md) byte for byte, and a test pins
it. Two layouts ship, `uk` and `fr`; pass the one matching the ROM actually
installed, because the layout is what the machine's ROM does, not the host.

CSL says an emulator may silently skip a character it cannot send. This runner
refuses instead: a skipped key leaves SHAKER on a different screen and labels a
capture with the wrong test.

Under the French ROM every digit needs SHIFT, and SHAKER reads its menu keys
straight off the CPC matrix, so SHIFT is visible to it while the key is down.
That is recorded per run.

### Timing

`wait` is emulated microseconds. This core runs 1:1 from the PLL, so a wait is
a host sleep whose only error is host latency. The scripts pad their waits well
above the screen times in their own comments.

MBC sleeps `MBC_KEY_WAIT` before every press and before every release
([mbc.c](https://github.com/pocomane/MiSTer_Batch_Control/blob/3873450d413c30e6b0339e6b3dbf2373e0e5a74a/mbc.c#L410),
`inter_key_wait`), so one knob is simultaneously the CSL key-press delay and
the CSL inter-key delay. `key_delay 70000 70000` therefore maps exactly. A
script asking for two different values gets `max(press, gap)` and an
approximation row. The third `key_delay` parameter, the delay after a carriage
return, splits the MBC invocation and becomes a host sleep. `\(KOF)` cannot be
honoured at all, because MBC always sleeps before each event.

Each MBC invocation also costs `2 x MBC_SEQUENCE_WAIT` (2 s) of uinput
settling, outside the script's timing model. That is recorded, not compensated
for: inventing a subtraction would be a silent approximation.

### Outputs

Each run directory gets `manifest.json` (effective settings, device hashes, the
ordered trace with host timestamps, deduplicated approximations, the rejection
with its six fields, capture names and SHA-256s, the SSH command log, and the
cleanup result including CFG restoration) and `last-run.log`, the human-readable
ordered trace the standard asks an emulator to keep. An existing manifest is
never overwritten.

### Captures

The bundled SHAKER scripts contain no `screenshot` instruction: their captures are
meant to come from the SSM code SHAKER emits for each test screen, which the core
now detects. Two paths exist.

`--screenshot-at [SCRIPT:]LINE` requests a capture after a chosen script line
without editing the author's files, named
`MISTER_<crtc>_<script>_<line>_<n>.png`. It needs no RTL support and is the
right tool for a one-off look at a particular screen.

`--ssm` turns on the detector and captures on the markers SHAKER itself emits,
naming each from its code so captures line up with the portal's
code-to-image table. This is the path that makes captures comparable with the
SHAKERLAND references.

## SSM markers

`rtl/ssm_marker.v` watches the Z80 opcode-fetch stream for the two consecutive
undefined-ED instructions SSM v1.1 defines, `#ED #LL #ED #HH`, and publishes
each one to a ring in the DDR3 window described below; allocation is still a device gate. Those instructions are
two NOPs on real hardware, so a SHAKER disc runs identically on a CPC, on an
emulator and here.

The detector is off by default. `--ssm` sets OSD status bit 37 through the
same CFG mechanism as the CRTC bit, and restores it with the rest of the
configuration at the end of the run. The runner then polls the ring over the
existing SSH transport with BusyBox `dd if=/dev/mem`; no Main patch and no
device daemon is involved. `status_set` and `info_req` were both checked and
neither reaches user space.

`#0000` and every `#FFxx` are reserved by the standard; **everything else is a
screenshot request**, which is how SHAKER works — it assigns one code per test
screen and builds markers at run time by patching a template. The workbook contains
712 reference code rows; it is not an observed emission count. `#FFFE`
is only the variant that takes its name from CSL instead of from the code.

| Marker | What the runner does |
|---|---|
| any non-reserved code | captures, named `MISTER_<crtc>_<HHLL>.png` |
| `#0000` | releases a pending `wait_ssm0000`, bounded by `--max-wait` |
| `#FFFE` | captures, named `screenshot_name` if one is pending, else `MISTER_<crtc>_FFFE.png` |
| `#FFFF` | recorded as an approximation: this runner makes no snapshots |
| other `#FFxx` | recorded with its raster position, acted on by nothing |

Each record carries the marker's code, a VSYNC-edge count, line, horizontal
position, field and core clock tick. Format 1 samples the pre-conversion timing
at 16 MHz; `hpos` wraps every 256 dots (16 us), so it is not a unique pixel
coordinate. The 32-bit tick wraps after 67.108864 s. Keep deltas within a verified
session and account for wrap.

**Captures remain approximate.** Main reads the scaler asynchronously, and neither
the marker stamp nor a successfully decoded PNG establishes the image's age. Stable
content before a marker does not guarantee unchanged content until the host request.
The intended exact path is capture at the opcode; see the revised
[phase 2 contract](csl-ssm-implementation-plan.md#phase-2-exact-frame-capture-separate-gate).

SHAKER uses distinct ordinary codes for paired states. The runner compares adjacent
screenshot markers within four VSYNC periods and marks **both** captures of a close
pair, because neither of the two is the trustworthy one. Absence of that heuristic
warning does not prove phase fidelity: proximity measures neither phase dwell nor
host latency, and every SSM capture is labelled `capture_alignment: approximate`.
A pending `screenshot_name` is kept for a later `#FFFE` and never consumed by an
ordinary code.

### Startup, read errors and service gaps

The observer publishes its header with `written = 0` when enabled. After the controlled
boot wait, the runner requires that state before program input; a first nonzero header
or a read completing after the startup deadline is a failure. The manifest records
the observed boundary. **It is
a startup boundary for one controlled run, not a session identity**: an old empty
header is indistinguishable from a fresh one, and a program that emitted a marker
before the host looked would already have advanced the count. General attach,
concurrent reload and persistence across FPGA loads are unsupported.

A read that fails is never reported as "no marker". `ssm_ring.py` distinguishes a
transport failure (`dd` error, empty output, bad base64), a truncated read, an
unsupported format and a not-yet-initialised header; only the last is tolerated, and
only within a bounded startup budget. Each poll reads the header, the ring and the
header again, and returns only records whose slots the second header proves were not
reused during the read; the margin allows for the record the core writes before it
commits the count. Anything outside it is counted as lost and reported.

After startup is established, the runner drains the ring at every command boundary and once more at the end of the
run, so a marker emitted while MBC or a PNG retrieval blocked the thread is still
collected. CSL waits and `--max-wait` are monotonic wall deadlines, because the polls
and SSH round trips inside them advance the machine too. `wait_ssm0000` consumes one
unconsumed `#0000` from the current run in ring order, including one that arrived
before the wait was entered; CSL v1.4 does not settle that case, and the alternative
reading is recorded in the [plan](csl-ssm-implementation-plan.md) rather than
presented as the standard's rule.

Still open: the DDR interval itself, and the fact that host polling is not an exact
core wait under either reading.

### Verify the DDR3 allocation before enabling writes

`DDR_BASE` defaults to byte address `0x30000000`; `DDRAM_ADDR` is a 64-bit word
index. Verify the reserved interval against the framework/Main allocations and the
target Linux memory map before enabling the observer. A matching magic value alone
establishes neither ownership nor current-session freshness.

`--ssm-base` changes only the host reader. Changing the FPGA writer requires updating
its parameter and building a matching RBF. Missing magic means the detector is
disabled, the core has not finished loading, or the base is wrong for this framework
build; diagnose that before considering an address change. Do not try arbitrary writer
bases. The full device acceptance remains open.

## The experimental capture recorder

`rtl/ssm_sample_recorder.v` is a prototype that appends native RGB24 samples and
their aligned sync tuple into rotating DDR3 windows, so a host can rebuild the
picture as it stood at the marker's opcode rather than whenever Main got around to
grabbing the scaler. It is **compile-time off**: `Amstrad.sv` instantiates it only
when `SSM_SAMPLE_RECORDER` is defined, which it is not, so the ordinary build is
exactly the marker build. Turning it on needs a separately reviewed 17 MiB DDR3
allocation that has not been obtained.

Its region layout, publication order and loss reporting are written down in
[the capture ABI](ssm-capture-abi.md); `scripts/hardware-loop/ssm_capture.py` reads
and decodes it, including offline from a region dump:

```bash
python3 scripts/hardware-loop/ssm_capture.py decode --region dump.bin \
    --capture 0 --profile cpc-native-progressive --out capture.ppm --meta capture.json
```

For live retrieval over SSH from a running device directly into an ordered `.raw`
sample trace, decoded `.ppm` image, and `.json` metadata:

```bash
python3 scripts/hardware-loop/ssm_capture.py live --target root@mister \
    --capture 0 --profile cpc-native-progressive --out-dir /tmp/captures --out-prefix mister_0
```

`decode_live_capture` retrieves the capture record, confirms validity, and brackets
payload reads with pre- and post-read record commit identity checks (`record_commit_identity`
on upper 32 bytes of the record), rejecting captures where slot reuse or mutation occurred
during retrieval. The ordered samples are written to `<out-prefix>.raw`, image surface to
`<out-prefix>.ppm`, and metadata/invalidation diagnostics to `<out-prefix>.json`.

Offline decoding reports `lost` for reused windows and `pending` for unsealed ones.
The live operation instead fails a read if its record or windows are unavailable,
unsealed or change during retrieval; it cannot label those bytes a coherent capture.
A successfully read trace with gaps, loss, missing sync, unsupported geometry or
insufficient history stays incomplete, retaining its raw trace and metadata. PPMs are
cropped previews, including partial surfaces; JSON preserves their sync-relative origin.
Native profiles require Raw CRT, HQ2x and alternate pixel rate off, and remain declared
assumptions rather than device measurements. The HPS atomicity/ordering contract still
requires device validation before these read brackets can establish hardware coherence.
