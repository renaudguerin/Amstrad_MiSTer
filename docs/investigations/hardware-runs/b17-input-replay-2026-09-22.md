# B17 bounded keyboard replay — 2026-09-22

B17 now has a small timed uinput replay slice. Physical input recording is deferred:
the live Main grabs input devices, invalidating the original passive-reader premise.
This is host tooling only; no RTL, synthesis, CSL runner or existing case changes.

## Device and configuration

- Task base: `a33d93e967d71afb30143222cf6c23ca4f03f3f5`.
- Fresh SSH preflight: MENU, Logitech K400 Plus (`046d:404d`, event0), Main virtual
  input, Python 3 and `/dev/uinput`; no physical joystick. AmSpirit ping responded
  (lite 1.15.1/core 2491682), but was not mutated or used as hardware proof.
- Main `/media/fat/MiSTer` SHA-256:
  `9f6e5a237c36be6404ab4823d804821491db4bf125827f84aca2a1ca31f0a8a6`.
- Linux `6.18.38-MiSTer`, ARM32; native `input_event` is 16 bytes.
- Existing hardware-tested RBF:
  `/media/fat/_Computer/Amstrad_20260914_88262b9.rbf`, SHA-256
  `bad7d36995c5f480c9328aae1b7f0174881214998786b5610b18bcd11e7b1076`.
  Neither failed-timing `60a63e4` nor untested `37ccfc3` was loaded.
- Sonic CPR: `/media/fat/games/Amstrad/cpr/Sonic the Hedgehog (UK) (64K) (2025) [Original].cpr`,
  SHA-256 `4cb31c7f1769989a029fe1bd7ffb04670d5d24570dc56b0417fac662fed887ae`.
- Original `Amstrad.CFG` bytes: `00004000000000000000000000000000`.
  For this test only, byte 4 changed to `04` (Plus model 6128+); all other bytes
  preserved. This is the requested persisted configuration, not an independently
  measured live OSD/video mode. No monitor geometry or CRTC accuracy claim follows.
- Temporary private mapping `Amstrad_input_0000_b017_v3.map`, 32 little-endian uint32s:
  `106,105,108,103,29,56,57`, followed by 25 zeroes. Right/left/down/up/fire1/2/3.
  It did not exist before this run; no physical-controller mapping was modified.

## Practical Sonic input path

[The schedule](../../../scripts/hardware-loop/cases/sonic-start.json) selects Main
keyboard joystick 1 using F18, holds left Ctrl/fire1 for 200 ms, releases it, then
selects normal keyboard mode with F20. The helper uses its own `0000:b017` identity.
See [usage and setup](../../../scripts/hardware-loop/INPUT-REPLAY.md).

The first title-screen replay reached Green Hill Zone / Act 1, followed by the
severely broken gameplay display. A controlled rerun loaded the same MGL, waited
18 seconds, captured the title, ran the schedule (1 second discovery + 2 seconds
schedule), waited 2 seconds and captured Green Hill Zone / Act 1. Input timing is
wall-clock relative to the helper; MGL load and screenshot commands are asynchronous.

Boot timing is material: at 10 seconds the capture still showed “Condense Team
Presents”, and an early press did not start the game. At 30 seconds the cartridge
was already in attract-mode gameplay; fire returned it to the title. Do not use a
blind 30-second wait, or label an arbitrary gameplay capture as an input checkpoint.
The 18-second recipe is a tested navigation point, not frame-deterministic replay.
A no-input capture at 23 seconds was black (a transition); it does not establish a
stable matched-time negative control or distinguish exact gameplay states.

## Recording and release evidence

A prototype recorder selected only the new replay keyboard's evdev node, did not
grab it, and watched for 700 ms. An independent thread submitted a press after
200 ms and released it 150 ms later. The reader received zero transitions while
Main owned the input grab. Upstream Main source also initializes `grabbed = 1` and
calls `EVIOCGRAB` during enumeration. The prototype was removed; the final helper
neither records nor reads physical keyboard events. B17 is not complete.

The release smoke opened the helper's evdev node only for `EVIOCGKEY` state queries,
which still work under Main's grab. It emitted F24, verified its kernel key-down
bit, delivered SIGHUP to the replay process, and verified the same key bit cleared
in `play()` cleanup before destroying that virtual device:

```
PASS ARM32 event size=16: SIGHUP after observed keydown releases same-device kernel key state
```

This proves kernel input cleanup for that catchable interruption; it does not
measure Main's FPGA-side release latency or guarantee SIGKILL cleanup.

## Evidence and restoration

Local, ignored evidence is under `docs/screenshots/b17-input-replay-2026-09-22/`
in this task checkout: before/after screenshots, the MGL, exact original and test
CFGs, temporary mapping bytes, fire schedule and the release smoke script. Device
screenshots `b17-*.png` and scratch files under `/tmp/b17/` remain as evidence.
No cartridge bytes are committed.

Restoration was verified before explicitly releasing device ownership to the coordinator:
MENU, temporary private map removed, original CFG hash `2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`,
and only the original K400 Plus and Main virtual input enumerated. No further device
mutations are owned by this task.

## Verification

The boundary test interface was written and run before implementation (initial
failure: missing new module). The final focused suite covers absolute timing under
write latency, interruption after a possibly delivered press, uinput setup failure,
continued cleanup after one release fails, and malformed schedules. No simulation
or FPGA behavior changed. `python3 sim/select_tests.py --run` reported
`select_tests: no simulation needed`.

Fresh independent Opus review (`claude-opus-5`, high), run
`20260922T024642Z-978-9f31`, found no blocking issues in the final replay code,
tests and event schedule; it did not rerun the suite. Optional nits concerned a
second signal interrupting cleanup, traceback-style interruption exit, and the
two writes per event. The existing cleanup and explicit interruption limitations
are sufficient for this bounded tool; no new framework was added.

An earlier Gemini review (`20260922T024143Z-98596-447f`) covered the discarded
recorder draft. Its recorder findings are superseded by removing recording; its
proposed immutable-buffer ioctl failure was contradicted by the real-device run
which passed that ioctl. End-of-schedule held keys are intentionally supported and
released in `finally`, rather than rejected as that review suggested.
