# AmSpirit as troubleshooting oracle — design

Written 2026-09-13. Stream: **general** (shared host tooling; no RTL).
Status: **reviewed; Track G pilot and helper implemented.** Helper:
[`scripts/amspirit/`](../../../scripts/amspirit/README.md). Pilot:
[Copter 271 title](amspirit-oracle-pilot-2026-09-13.md), accepted: the checkpoint SNA
resumes in AmSpirit and, after loading the cartridge first, on the MiSTer.

Goal: use the AmSpirit emulator — LUA scripting plus HTTP API — as a
reference oracle when debugging core issues, alongside (but decoupled from)
the MiSTer capture path owned by the
[mister-capture skill](../../../.agents/skills/mister-capture/SKILL.md) and the
[hardware-loop driver](mister-hardware-loop-driver.md).

Non-goal: turning AmSpirit into a correctness verdict. The authority ranking
stands — real hardware and Logon System photographs outrank any emulator, and
the stock upstream core is only a regression baseline. AmSpirit answers "what
does this AmSpirit build show here?", never "is our core correct?".
Emulator agreement is supporting diagnostic evidence only. A snapshot-seeded
run additionally shares the emulator's initial-state assumptions, so any
consequential conclusion must still be confirmed through a cold-boot
reproduction or independent hardware/reference evidence. The French ACCC keeps
its documentary role for rule claims.

## 1. Two tracks, different priority

**Track G — generic troubleshooting (priority).** Booting Plus cartridges and
other titles to a defect point, screenshotting, and interrogating machine
state (registers, CRTC, RAM, BASIC) while debugging the core in simulation or
on device. Uses no CSL/SSM.

**Track S — Shaker (background).** CSL/SSM walks on both sides already exist
(Phase 0 runner + Phase 1 ring, see
[csl-ssm-implementation-plan.md](../ssm-csl/csl-ssm-implementation-plan.md)). Since
`shaker.logonsystem.eu/tests` already hosts most AmSpirit Shaker screenshots
and we hold no guaranteed up-to-date local copy, bulk AmSpirit-side SHAKER
capture is out of scope. AmSpirit's Track S role is spot checks plus
SSM-semantics reference (e.g. how Longshot's engine treats ordinary per-test
codes).

## 2. Verified AmSpirit capabilities (live, 2026-09-13)

Probed against the local instance (lite 1.15.1, `127.0.0.1:6128`).
Upstream sources:
`amspirit-releases/docs/lite/scripting.md`,
`amspirit-releases/docs/lite/web_api.md`, and the machine-readable
`GET /api/doc` + `GET /api/doc/<name>`.

### 2.1 Screenshots

`GET /api/screenshot?crop=1&full=1&live=0` returns a PNG with no script, file,
or disk round-trip (verified: 768×542 frame retrieved). Response headers give
beam position and cut geometry (`X-Beam-*`, `X-Crop-*`), in the same
coordinate space as `GET /api/beam` and `POST /api/raster_bp`. Parameters:
`crop` (visible area vs full 1024×350 buffer), `live` (in-progress vs settled
frame — use settled for reference), `full` (plain vs composite debug view).

Reference captures pin: monitor preset `Off` (raw pixels, no CRT shader),
`screen_type` colour, settled frame (`live=0`), plain image (`full=1`; `full=0`
is a composite debug view, not a reference). Pipeline position must be recorded
per capture: AmSpirit-with-shader-off is approximately a **pre-shader raw
frame**. On the MiSTer side the driver documents asynchronous scaler capture;
a pre-scaler mode, if wanted, needs its Main command/build prerequisite
identified first — until then it is optional, not assumed. Crop geometry
(which rectangle) and pipeline position (which stage) are separate manifest
fields; neither implies the other.

### 2.2 State and memory

- `GET /api/state`: Z80 (full file incl. shadows, IFF, IM), GA (mode, border,
  16 inks + RGB24), CRTC (R0–R13, selected reg, rasterline, vsync), PSG, FDC,
  plus `emu` (fps, frame counter, model, CRTC type, autotype status,
  timelapse, `ram_apply_seq`).
- Singles: `/api/z80|ga|crtc|psg|fdc|keymatrix|memmap|beam`.
- `/api/ram`: raw, per-16K-bank, or `view=cpu` (what the Z80 sees through
  paging — prefer this for banked RAM).
- `/api/history` (last 20 instructions), `/api/codemap`,
  `/api/basic_state|basic_listing|basic_export` (BASIC oracle: pointers,
  statement addresses, detokenized source).
- No internal CRTC counters anywhere: Lua `cpc.getCRTC` is **nil**, and
  `/api/crtc` exposes only registers + rasterline + vsync. Internal-counter
  questions still go to the ACCC and our own sim.

### 2.3 Control

`POST /api/config` (model, CRTC type 0–4, pause,
reset), `/api/media` (SNA/DSK/HFE/IPF/**CPR**/CRO/BIN body upload; `.cdt`
unsupported), `/api/keytype`+`/api/keypress` (non-trivial pacing: SHIFT and
repeat chars cost extra frames — pace on `emu.frames` with a host
elapsed-time deadline and explicit failure reporting, never bare wall-clock
`sleep` or unbounded frame polling, since counters stop while paused, at a
breakpoint, or after failure), `/api/basic` inject,
`/api/ram`+`/api/exec` (poll `ram_apply_seq` before trusting readback), Z80 /
BASIC / raster breakpoints, single-step, run-to, `POST /api/tl_back`,
`/api/disk` create/save, `/api/quit`.

### 2.4 Scripting and console

CSL + raw Lua 5.4 on a frame-by-frame coroutine (`wait*` family). Live
enumeration showed the engine exposes more than `scripting.md` documents
(`gate_array`, `memory_exp`, `rom_config`, `key_from_file`,
`snapshot_version`; `cpc.getAudio/getFDC/getBasicState/getKeyMatrix/
injectBasic/exportBasic`). `/api/eval` runs one Lua chunk on a persistent
state with `seq` polling; `/api/script` runs a file with `print()` capture
(both verified live, including the single-wait-slot refusal when a script is
already running). Network scripts are sandboxed (no `io`/`package`/`debug`/
`dofile`, pruned `os`, `fs.*` jailed under the config dir) — verified live;
fine for our uses, keep `--lua-full-stdlib` off.

### 2.4a Behaviour found by the pilot

- SNA saving has no HTTP endpoint: Lua `snapshot_dir(fs.root())`,
  `snapshot_name(name)`, `snapshot()`. It writes nothing, without error, until
  the directory exists (any `fs.write` creates it). It works while paused.
- `POST /api/media` with a CPR does not reset the machine; a load into a running
  program crashed it. Follow a CPR load with `do_hard_reset`; never reset after
  loading an SNA.
- A machine without VSYNC keeps returning the same `live=0` frame, so identical
  screenshots of an animated screen are a crash signal.
- `/api/keytype` drives the keyboard only. Joystick input goes through
  `keyboard_write` on matrix row 9 (active low, fire `0x10`).
- `/api/state` carries no Plus ASIC state; PRI, DCSR, sprites and the ASIC
  palette are only reachable through the SNA `CPC+` chunk.

### 2.5 SSM scope (Track S only)

SSM exists in the script engine (`wait_ssm0000()`, `#0000/#FFFF/#FFFE/
#FFFD/#FFFC`) and CLI flags (`--ssm`, `--ssm-both`); it has **no HTTP
surface** (absent from the full endpoint list). Marker-driven automation must
go through a posted script. Open semantic question, do not assume: per docs,
only `#FFFE` screenshots and `#FFFF` snapshots — ordinary per-test codes
appear to produce nothing, unlike our runner, which captures on every
non-reserved code. Resolve against Longshot/docs before mirroring a SHAKER
walk.

## 3. Snapshot handoff (proven to load; state transfer per-case)

`docs/defects/arn5diag/snapshot_20260913_171245_OS_PLUS_FR.sna` (AmSpirit
`MV - SNA`, 1.6 MB) **loads in our MiSTer core** — that much is demonstrated.
Review-established contents (Astra-high, read-only parse): SNA **v3**, model
byte 4, CRTC byte 3, 128 KiB base RAM, a 2,296-byte `CPC+` chunk and a
1,528,210-byte `SPRT` chunk. Our `Amstrad.sv` apply path handles `MEM0`/`MEM1`
and `CPC+` plus selected v3 counters; it does **not** restore `SPRT`. So
"loads" must not be read as "complete emulator-state transfer": the working
direction **AmSpirit → MiSTer** holds as a way to seed a deep-game state
without replaying input, but each case needs its own validation — the tested
RBF/configuration, observed continuation after load, SNA version and chunks
present, ignored state, and cartridge/ROM prerequisites all recorded. A
snapshot that resumes and plays on is evidence; a snapshot that merely loads
is not.

A cartridge title's SNA does not carry the cartridge ROM. On the MiSTer, load
the CPR first and the SNA a few seconds later (one MGL with two `file`
entries); the SNA alone hangs the machine. For the pilot build, Plus model had to be set in the CFG beforehand. B16 now
selects 6128+ for a valid CPR loaded from Off and the matching Plus model for
SNA v3 headers 4/5/6; its Main/OSD and device acceptance remain open. Continue
to record the explicit effective model for controlled comparisons.

MiSTer-side classic snapshot saving (B18) is now integrated as a development tool
requiring an SSH pull. A 6128 save resumed in this core and AmSpirit on September 14;
see [the B18 design](../../b18-sna-save.md). That result does not establish Plus
save support or complete state equivalence. Automated capture/round-trip acceptance
remains open and is separate from this AmSpirit-to-MiSTer pilot.

## 4. Corrections and contracts adopted while writing

- No "NTSC Amstrad" framing: the emulator's `freq_screen` label is not
  hardware fact and is ignored by this design.
- CRTC numbering matches, but that is **not** configuration equivalence: the
  emulator offers types 0–4 while the classic core selects 0/1 and Plus is a
  separate implementation (the §3 snapshot itself declares type 3).
  Supported contract: classic comparisons on 0/1, Plus configuration
  identified separately, other types explicitly unsupported. Every joint run
  records requested/applied model, CRTC, RAM expansion, ROM/cartridge
  identities and palette/monitor settings on **both** sides; intentional
  mismatches are classified, not silently carried.
- Comparison manifest records pipeline position on both sides, with the
  pre-scaler prerequisite from §2.1.

## 5. Proposal

A thin standalone helper, `scripts/amspirit/` (new; does not touch
`scripts/hardware-loop/`): load media → send input paced on `emu.frames`
(with host deadline, §2.3) → screenshot / dump state / save SNA. Usable as a
bare oracle with **no MiSTer involvement**. The helper earns its place after
the pilot: the pilot runs as shell commands first, and repeated
polling/capture/manifest operations are extracted into the helper once
demonstrated. Only when a joint run happens, both sides share a
case/run/checkpoint identifier plus join labels (media hash, input script,
snapshot hash); frame counts are side-specific metadata with a defined
origin, never a cross-system state identity. No lockstep coupling, no merged
tool, no RTL.

Per-run AmSpirit identity is recorded every time: build/version, effective
configuration (§4), input recipe, artifact hashes. Evidence follows the
capture skill's dated-document convention (`docs/references/<topic>-<date>/`,
gitignored, never committed): name the private artifact location and copy
decisive files somewhere that survives worktree deletion. No
artifact-management service.

Joint-run comparison keeps the existing discipline: shapes/colours not
pixels, normalize dimensions, record variant/version/timing-policy/geometry/
colour-settings, never auto-align away a sync displacement.

Pilot acceptance (lightweight, hobby-grade): one stable Plus-cartridge
checkpoint; a single load/input pass is enough to validate the manifest
fields — repeat only if something looks off, since any regression here is
cheap to reverse or bisect. Record the screenshot/state observation point,
check the snapshot at least resumes and responds to input (§3), and write
down configuration mismatches rather than gating on zero. The point is not
to fool yourself, not to certify the pipeline.
`arn5diag` is excluded (worked in another thread without AmSpirit; retry with
it only if that stalls), so the pilot defect is still to be chosen.

## 6. Open questions

1. AmSpirit ordinary-code capture behaviour (§2.5) — Track S only, nonblocking for Track G.
2. AmSpirit SSM matcher byte set vs our 177-value permissive set — Track S only, nonblocking for Track G.
3. Per-case snapshot compatibility and configuration-matching evidence on the
   MiSTer (§§3–4). The AmSpirit save/load round trip is settled (§2.4a): the
   helper saves under `fs.root()` and copies the file from the local host.
4. Main pre-scaler capture prerequisite (command/build) if wanted (§2.1).
5. Settled for the pilot: Copter 271 title, evidence under
   `docs/references/amspirit-pilot-copter271-2026-09-13/` in the main checkout.

## 7. Review debt note

This is a docs-only change; per project rule it needs no independent review
cycle of its own — except that this document's whole purpose is to be
reviewed once, now, before implementation in a new thread.

## 8. Review record

Astra-high review, run `20260913T170942Z-55786-8463` (read-only; static repo
checks only, live API not re-verified; log under `/tmp/agents-roster-runs/`).
Verdict: **CHANGES-REQUIRED** — 2 blockers, 5 advisories. All were doc-level
and are incorporated above:

- §3 qualified: load ≠ state transfer (`SPRT` unrestored), per-case validation required.
- §4 is now a supported-configuration contract, not a numbering blanket.
- Authority language de-absolutized (intro); ACCC role and cold-boot rule stated.
- Per-run identity + dated evidence retention added (§5).
- Pilot acceptance criteria added (§5); frame counts demoted to side-specific metadata.
  Parent override 2026-09-13: the repeat-twice / zero-mismatch gate was
  deliberately softened after review. Hobby economics — reversibility is
  cheap, ceremony is not. The remaining advisories stand as written.
- Pre-scaler mode made prerequisite-gated; crop-vs-pipeline distinguished (§§2.1, 4, 6).
- Bounded polling (frames + host deadline) specified; shell-first, helper-after (§§2.3, 5).

The design pass itself contained no RTL, helper or pilot run; those followed in
the implementation task recorded in the pilot document.
