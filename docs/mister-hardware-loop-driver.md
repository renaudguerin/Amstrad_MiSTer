# B2 MiSTer capture driver

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
