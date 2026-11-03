# AmSpirit oracle helper

Agent workflow: [amspirit-test skill](../../.agents/skills/amspirit-test/SKILL.md).

`amspirit.py` drives a running AmSpirit lite instance over its HTTP API (default
`http://127.0.0.1:6128`, override with `--url` or `AMSPIRIT_URL`). It loads media, paces
input on `emu.frames`, captures settled screenshots, dumps machine state and saves SNA
snapshots. It never touches the MiSTer; joint runs share only a case identifier and media
hash with [the hardware-loop driver](../hardware-loop/).

What AmSpirit evidence means, and what it cannot establish, is set in
[the design](../../docs/investigations/hardware-runs/amspirit-oracle-design-2026-09-13.md). In short: it shows what this
AmSpirit build does, never whether the core is correct.

Python 3 standard library only. In 1.17+, snapshots are streamed directly
over HTTP (`GET /api/snapshot`), enabling remote as well as local capture workflows.

## Starting a debugging session

Start AmSpirit lite with its HTTP server enabled (`--web-server`; current tooling guidance
covers 1.17), then run `python3 scripts/amspirit/amspirit.py identity`. Use `--url` before the
subcommand for a different address. The helper connects to an existing instance; it does
not start the emulator. For joint device work, separately check SSH access to `root@mister`.
B17 and Sonic GX device work require that device connection; use AmSpirit where available.

When the user requests an AmSpirit test, use the existing instance even if it is running a
different program; that alone is not a reason to ask before loading the requested CPR or SNA.
Loading media and applying case settings replaces the current machine state and may change
configuration and rendering settings. Do not manually preserve and restore the prior state or
settings just for cleanup; leave the test configuration in place and record it with the evidence.
The helper's pause-state handling is described below.

The [Copter pilot](../../docs/investigations/hardware-runs/amspirit-oracle-pilot-2026-09-13.md)
proved an automated AmSpirit checkpoint and its continuation on MiSTer after loading the
CPR first. It did not establish an unattended end-to-end comparison loop. To build one for
a new defect, reuse the two drivers with a common case/media identity, define the checkpoint
and expected visual difference, validate snapshot continuation (or input replay), then repeat
capture after a candidate fix. MiSTer joystick injection remains a B17 prerequisite for
cases needing it. Frame counters on the two systems do not identify the same instant, and
scaler-versus-emulator images need deliberate geometry/colour interpretation rather than a
blind pixel-equality verdict. Confirm consequential timing conclusions against hardware or
the documented rule; emulator agreement alone is not correctness.

## Commands

```sh
python3 scripts/amspirit/amspirit.py identity                 # version, config, render
python3 scripts/amspirit/amspirit.py load cart.cpr            # load + hard reset, prints frame origin
python3 scripts/amspirit/amspirit.py load --no-reset x.sna    # snapshots must not be reset
python3 scripts/amspirit/amspirit.py load x.sna --crtc keep  # retain current CRTC within classic/Plus boundary
python3 scripts/amspirit/amspirit.py wait 500                 # frames, with host deadline
python3 scripts/amspirit/amspirit.py joy fire --frames 10     # joystick 0 via native POST /api/joystick
python3 scripts/amspirit/amspirit.py keys 'RUN"DISC'          # autotype, waits until typed
python3 scripts/amspirit/amspirit.py pause                    # or resume (via POST /api/run)
python3 scripts/amspirit/amspirit.py shot out.png             # settled plain frame + beam/crop headers
python3 scripts/amspirit/amspirit.py state out-dir/           # ping/config/render/state/memmap/beam/history/keymatrix
python3 scripts/amspirit/amspirit.py snapshot name out-dir/   # streamed SNA (GET /api/snapshot) + parsed header/chunk list
python3 scripts/amspirit/amspirit.py eval 'return fs.root()'
python3 scripts/amspirit/amspirit.py run scripts/amspirit/cases/copter271_title.json --out-dir $S/run-001
```

`run` applies the case's `config` and `render` settings, records requested versus applied
values, loads the media (hard reset for other media, no reset for SNA by default), executes `steps`
(`wait_frames`, `joystick`, `keys`, `screenshot`), then pauses and captures a checkpoint:
screenshot, state files and SNA, all describing one instant. The requested config/render
settings remain applied after the run. `manifest.json` records
AmSpirit version/frontend, settings, media and case hashes, the frame origin, every event with its
start and end frame offsets (`at`, `done_at`), and the SNA chunk list. It refuses an
existing manifest. On success it restores the pause state it found (or stays on the
checkpoint with `leave_paused`); on failure it leaves the emulator paused for post-mortem.
An eval that times out keeps running inside AmSpirit, so later evals are refused until it
ends. Case `media.path` is resolved
against the main checkout, where the ignored cartridges live (`--media-root` overrides).

Keep decisive runs under `docs/references/<topic>-<date>/` in the main checkout
(ignored, never committed); scratchpad directories vanish with the session.

## API behaviour that shapes the helper

For the documented 1.17 capabilities, migration details, and direct API recipes, read the
[Lite 1.17 tooling guide](../../docs/investigations/hardware-runs/amspirit-lite-1.17-tooling.md).
Historical 1.16 details remain in the [Lite 1.16 tooling guide](../../docs/investigations/hardware-runs/amspirit-lite-1.16-tooling.md).

Helper settings, pause, autotype, media and run calls require the 1.16/1.17 command acknowledgement contract:
they wait with a host deadline for `emu.applied_cmd_seq >= cmd_seq` before dependent
reads. In 1.17, command responses return `{"cmd_seq": N}` (without `"ok"`), and apply-time
failures recorded in `emu.failed_cmds` are detected and raised as errors.
Execution control (pause, resume, reset) uses `POST /api/run`.
Lua eval retains its separate `seq` / `done` completion contract.
For SNA comparisons, `load --crtc keep` or case `media.crtc: "keep"` retains the current
CRTC within the classic/Plus boundary; default `sna` follows the snapshot header.
SNA loads default to no reset. The manifest records effective settings after loading,
because an SNA can override the case's requested configuration.
`media_effect` reports the requested policy, effective CRTC and any `keep` mismatch;
`settings.effective_after_media` contains the live model/CRTC, tick/frame readings,
config/render and requested-setting mismatches. A Plus/classic boundary mismatch is
recorded rather than treated as a successful preservation. `--reset` or case
`media.hard_reset: true` is rejected for an SNA before applying machine changes.
`state` saves the fields returned by the existing endpoints (including FDC within
`state.json`), but does not subscribe to breakpoint events or configure history/rewind
automatically. Case `config` can pass the new settings through to `/api/config`.

- **Loading a CPR does not reset.** Loading into a running machine left it executing garbage
  with no VSYNC. The helper hard-resets via `POST /api/run {"action":"reset","kind":"hard"}`
  after a load unless told otherwise.
- **The settled screenshot goes stale without VSYNC.** `live=false` returns the last completed
  frame, so a crashed machine yields identical PNGs forever. Identical hashes across
  captures of an animated screen mean "check the machine", not "stable screen".
- **Snapshots are streamed over HTTP in 1.17.** `GET /api/snapshot` streams `.sna` bytes directly
  in the response body, removing the prior requirement for local filesystem co-location and Lua
  `fs.root()` scripting. Snapshots, screenshots and state dumps all work while paused.
- **`emu.frames` stops while paused**, at a breakpoint, or when the core stalls. Every wait
  checks the pause flag and a host deadline instead of polling forever.
- **Joystick port 0 is driven natively via `POST /api/joystick`.** The helper maps `fire` to `fire1`,
  sets the hold `frames` budget, and waits for completion.
- **The pilot exposed no Plus ASIC registers over HTTP.** It used the SNA `CPC+`
  chunk for PRI, DCSR, sprites and palette. In 1.16/1.17, inspect the instance's published
  GA/CRTC fields before assuming a signal is available; their extra fields depend on
  the loaded core. The SNA remains the fallback for unavailable ASIC state.
- **SNA from AmSpirit is v3 with `CPC+` and `SPRT` chunks.** Our core's apply path does not
  restore `SPRT`; the manifest's chunk list is what a MiSTer handoff must record.
