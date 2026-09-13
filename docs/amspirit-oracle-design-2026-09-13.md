# AmSpirit as troubleshooting oracle — design

Written 2026-09-13. Stream: **general** (shared host tooling; no RTL).
Status: **draft for review** (Astra-high review pending, §8).

Goal: use the AmSpirit emulator — LUA scripting plus HTTP API — as a
reference oracle when debugging core issues, alongside (but decoupled from)
the MiSTer capture path owned by the
[mister-capture skill](../.agents/skills/mister-capture/SKILL.md) and the
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
[csl-ssm-implementation-plan.md](csl-ssm-implementation-plan.md)). Since
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

MiSTer-side snapshot *saving* is under development on a parallel branch (not
ready; this design neither depends on it nor touches it). If that lands, the
reverse direction needs its own acceptance — it is not covered by this one.

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
3. SNA save-to-load round-trip details (snapshot naming/dir retrieval via
   `fs.root()` vs local CLI paths), plus per-case snapshot compatibility and
   configuration-matching evidence (§§3–4).
4. Main pre-scaler capture prerequisite (command/build) if wanted (§2.1).
5. Pilot defect selection (needs a Plus-cartridge issue with a reachable
   screen) and durable evidence locations (§5).

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

No RTL, no helper, no pilot run in this pass — implementation belongs to a new thread.
