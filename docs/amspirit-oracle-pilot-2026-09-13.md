# AmSpirit oracle pilot: Copter 271 title (2026-09-13)

Pilot for [the AmSpirit oracle design](amspirit-oracle-design-2026-09-13.md) §5, Track G.
It validates the AmSpirit side of the pipeline and the manifest fields. It says nothing
about core correctness.

Evidence (ignored, main checkout): `docs/references/amspirit-pilot-copter271-2026-09-13/`
- `shell-pilot/`: the hand-run shell commands (checkpoint, resume and fire tests).
- `helper-run-001/`: the same recipe through `scripts/amspirit/amspirit.py run`, with
  `manifest.json`.
- `prepilot_20260913.sna` and `pre-pilot.png`: the user's AmSpirit state before the pilot.
  It was reloaded afterwards and reproduced the pre-pilot screen exactly.

## Setup

| Field | Value |
|---|---|
| AmSpirit | lite 1.15.1 (SDL frontend, `--web-server`), core id 2491682 |
| Config | `cpc_model` 4 (6128+), `crtc_type` 3, `ram_kb` 128, `rom_lang` FR |
| Render | monitor 0 `Off`, `screen_type` 0; all CRT shader parameters neutral |
| Media | `Copter 271.cpr`, SHA-256 `4b75c62c…2a9c1d` (same file as the MiSTer case) |
| Case | `scripts/amspirit/cases/copter271_title.json` |
| Capture | `crop=1 full=1 live=0`, 768×542, crop rectangle 155,55 768×271 |

Requested and applied settings matched; the manifest lists no mismatches. `screen_type` 0
is assumed to be colour: the captures are colour, but the API does not name the value.

## Observation points

Frame offsets count from `emu.frames` read right after load plus hard reset (a few frames
of latency). They are AmSpirit metadata and do not identify MiSTer state.

- About +250: Loriciel logo.
- From +500: Copter 271 title with logo and sky palettes as in the earlier AmSpirit reference
  under `docs/defects/copter271-2026-09-13/`; attract loop of meteors, helicopters and
  credits after that.
- Joystick fire (10 frames on matrix row 9) on the title opens the options menu within
  200 frames. Helper run `helper-run-001`: fire released at +1023 (held 10 frames from
  about +1013), options at +1225, checkpoint at +1225. The helper now records each step's
  start (`at`) and end (`done_at`) separately.

## Acceptance against design §5

- **Stable Plus-cartridge checkpoint:** yes. The title is reached with no input, and the
  options menu is a deterministic input-driven checkpoint.
- **Manifest fields:** a single helper pass recorded version, settings, media and case
  hashes, frame origin, event offsets, checkpoint screenshot, state files and SNA chunk list.
- **Snapshot resumes and responds to input, AmSpirit side:** yes. The checkpoint SNA
  reloaded into AmSpirit continued the credit sequence from the saved point, and fire then
  opened the options menu.
- **Snapshot resumes on the MiSTer:** yes, with the cartridge loaded first. Device RBF
  `Amstrad_20260913_05cb9fd.rbf` (SHA-256 `a81d89cc…9659af`; the only later RTL commit,
  `a0778b6`, resets PSG R7 and does not touch the SNA path), `Amstrad.CFG` Plus model
  6128+ (byte 4 `0x04`), restored to the original hash afterwards. The credits-screen SNA
  is v3, model 4, CRTC 3, 128 KiB, chunks `CPC+` 2,296 bytes and `SPRT` 1,626,514 bytes;
  our core ignores `SPRT`.
  - SNA alone (MGL F6): the machine hangs on vertical bars. The snapshot holds RAM and
    registers, not the cartridge ROM the game runs from.
  - MGL loading the CPR (F8, delay 1 s) then the SNA (F6, delay 5 s): the Loriciel boot
    screen at 4 s, then the "Coding" credits page the snapshot was saved on at 11 s, followed
    by the same credits sequence as AmSpirit. A cartridge boot without the snapshot shows the
    title's meteor and helicopter phases for about 45 s before any credits, so this is the
    snapshot resuming, not the attract loop restarting.
  - Input after the load on the MiSTer is untested: Copter 271 reacts to joystick fire,
    and MBC injects keyboard codes only, which never reach matrix row 9.
  Evidence: `mister/` in the evidence directory (MGL, captures, CFG before and after).
- **Configuration mismatches recorded:** none on the AmSpirit side. A joint run must still
  record the MiSTer's Plus model and sync filter, since the classic CRTC numbering does not
  map onto AmSpirit type 3.

## Findings that changed the tooling

These are recorded in the design (§2) and the [helper README](../scripts/amspirit/README.md):
loading a CPR does not reset the machine; a crashed machine freezes the settled screenshot;
`snapshot()` silently writes nothing until its directory exists; `keytype` does not reach
the joystick; Plus ASIC state is only available through the SNA `CPC+` chunk.
