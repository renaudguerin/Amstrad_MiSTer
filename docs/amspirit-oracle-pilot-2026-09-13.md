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
  200 frames. Helper run: fire at +1023, options at +1225, checkpoint at +1225.

## Acceptance against design §5

- **Stable Plus-cartridge checkpoint:** yes. The title is reached with no input, and the
  options menu is a deterministic input-driven checkpoint.
- **Manifest fields:** a single helper pass recorded version, settings, media and case
  hashes, frame origin, event offsets, checkpoint screenshot, state files and SNA chunk list.
- **Snapshot resumes and responds to input, AmSpirit side:** yes. The checkpoint SNA
  reloaded into AmSpirit continued the credit sequence from the saved point, and fire then
  opened the options menu.
- **Snapshot resumes on the MiSTer:** open. It needs a device load of the checkpoint SNA
  (F6) on a 6128 Plus configuration. The SNA is v3, model 4, CRTC 3, 128 KiB, chunks
  `CPC+` 2,296 bytes and `SPRT` 1,626,514 bytes; `SPRT` is not applied by our core.
- **Configuration mismatches recorded:** none on the AmSpirit side. A joint run must still
  record the MiSTer's Plus model and sync filter, since the classic CRTC numbering does not
  map onto AmSpirit type 3.

## Findings that changed the tooling

These are recorded in the design (§2) and the [helper README](../scripts/amspirit/README.md):
loading a CPR does not reset the machine; a crashed machine freezes the settled screenshot;
`snapshot()` silently writes nothing until its directory exists; `keytype` does not reach
the joystick; Plus ASIC state is only available through the SNA `CPC+` chunk.
