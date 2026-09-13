# AmSpirit oracle helper

`amspirit.py` drives a running AmSpirit lite instance over its HTTP API (default
`http://127.0.0.1:6128`, override with `--url` or `AMSPIRIT_URL`). It loads media, paces
input on `emu.frames`, captures settled screenshots, dumps machine state and saves SNA
snapshots. It never touches the MiSTer; joint runs share only a case identifier and media
hash with [the hardware-loop driver](../hardware-loop/).

What AmSpirit evidence means, and what it cannot establish, is set in
[the design](../../docs/amspirit-oracle-design-2026-09-13.md). In short: it shows what this
AmSpirit build does, never whether the core is correct.

Python 3 standard library only. AmSpirit must run on the same host for `snapshot` and `run`,
because the SNA is copied from AmSpirit's local script directory.

## Commands

```sh
python3 scripts/amspirit/amspirit.py identity                 # version, config, render
python3 scripts/amspirit/amspirit.py load cart.cpr            # load + hard reset, prints frame origin
python3 scripts/amspirit/amspirit.py load --no-reset x.sna    # snapshots must not be reset
python3 scripts/amspirit/amspirit.py wait 500                 # frames, with host deadline
python3 scripts/amspirit/amspirit.py joy fire --frames 10     # joystick 0 via keyboard matrix row 9
python3 scripts/amspirit/amspirit.py keys 'RUN"DISC'          # autotype, waits until typed
python3 scripts/amspirit/amspirit.py pause                    # or resume
python3 scripts/amspirit/amspirit.py shot out.png             # settled plain frame + beam/crop headers
python3 scripts/amspirit/amspirit.py state out-dir/           # ping/config/render/state/memmap/beam/history/keymatrix
python3 scripts/amspirit/amspirit.py snapshot name out-dir/   # SNA + parsed header/chunk list
python3 scripts/amspirit/amspirit.py eval 'return fs.root()'
python3 scripts/amspirit/amspirit.py run scripts/amspirit/cases/copter271_title.json --out-dir $S/run-001
```

`run` applies the case's `config` and `render` settings, records requested versus applied
values, loads the media (hard reset unless `media.hard_reset` is false), executes `steps`
(`wait_frames`, `joystick`, `keys`, `screenshot`), then pauses and captures a checkpoint:
screenshot, state files and SNA, all describing one instant. `manifest.json` records
AmSpirit version, settings, media and case hashes, the frame origin, every event with its
start and end frame offsets (`at`, `done_at`), and the SNA chunk list. It refuses an
existing manifest. On success it restores the pause state it found (or stays on the
checkpoint with `leave_paused`); on failure it leaves the emulator paused for post-mortem.
An eval that times out keeps running inside AmSpirit, so later evals are refused until it
ends. Case `media.path` is resolved
against the main checkout, where the ignored cartridges live (`--media-root` overrides).

Keep decisive runs under `docs/references/<topic>-<date>/` in the main checkout
(ignored, never committed); scratchpad directories vanish with the session.

## API behaviour that shapes the helper

Verified live on lite 1.15.1.

- **Loading a CPR does not reset.** Loading into a running machine left it executing garbage
  with no VSYNC. The helper hard-resets after a load unless told otherwise.
- **The settled screenshot goes stale without VSYNC.** `live=0` returns the last completed
  frame, so a crashed machine yields identical PNGs forever. Identical hashes across
  captures of an animated screen mean "check the machine", not "stable screen".
- **`snapshot()` fails silently if its directory is missing.** `fs.root()`
  (`~/.config/amspirit-lite/scripts`) does not exist on a fresh install; any `fs.write`
  creates it. Snapshots, screenshots and state dumps all work while paused.
- **`emu.frames` stops while paused**, at a breakpoint, or when the core stalls. Every wait
  checks the pause flag and a host deadline instead of polling forever.
- **`/api/keytype " "` is the keyboard, not the joystick.** Plus titles usually want fire;
  `keyboard_write` of matrix row 9 (active low, fire = `0x10`) works and is what `joy` does.
- **No ASIC state over HTTP.** `/api/state` has Z80, GA, CRTC registers, PSG and FDC only.
  Plus ASIC registers (PRI, DCSR, sprites, palette) exist only in the SNA `CPC+` chunk.
- **SNA from AmSpirit is v3 with `CPC+` and `SPRT` chunks.** Our core's apply path does not
  restore `SPRT`; the manifest's chunk list is what a MiSTer handoff must record.
