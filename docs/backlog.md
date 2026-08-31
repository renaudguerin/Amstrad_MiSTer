# Architecture and methodology backlog

Opened 2026-08-31 after a session that reviewed the project's methodology rather than any
single finding. `docs/implementation-roadmap.md` orders the *accuracy and Plus feature* work.
This file holds the cross-cutting items that sit underneath it: the things that decide whether
feature work is visible, verifiable, and maintainable at all.

Read this before picking up roadmap section 8. Several roadmap items are blocked on B1 in a way
that is not obvious from the roadmap itself.

## Why this file exists

By 2026-08-31 the project had accumulated roughly 400 commits, 183 classic vectors, about
twenty Plus simulation benches and about twenty independent-review documents, and produced no
measurable change in the SHAKER Module A entries under observation and no change in any of the
four reported Plus title symptoms. The methodology review concluded the bottleneck is not
model strength or code quality but **observability**: nothing in the automated loop can fail
for a reason the authors did not already know, and the one loop that could (hardware) is
manual, rare, and was being judged against the wrong baseline.

The items below are ordered by expected information gained per unit of effort, not by
interest.

---

## B1. crt_filter owns the sync and blanking geometry, and discards the CRTC's

**Priority: highest. Everything sync-related is downstream of this.**

`rtl/crt_filter.v` sits at the end of the video path and is currently hardwired on
(`Amstrad.sv`, `.sync_filter(1)`). It does three things that matter:

- It **regenerates HSYNC** as a fixed-width pulse at a fixed offset (asserted at
  `hSyncCount == 2*4`, cleared at `6*4`), re-aligned to a line length it measures from the two
  lines following VSYNC.
- It **regenerates VSYNC** with a two-line delay and a length limit.
- It derives **HBLANK and VBLANK from hardcoded constants** (`BEGIN_HBORDER = 49`,
  `END_HBORDER = 241`, `BEGIN_VBORDER`, `END_VBORDER`) counted from its own regenerated HSYNC,
  not from the CRTC or the Gate Array.

It also actively suppresses HSYNC pulses that arrive sooner than expected (the `hsync_mask` /
`line_time >= 190` block, commented "too frequent HSYNCs (S&KOH)") — precisely the signal a
CRTC-trickery demo produces on purpose.

**Consequence.** Longshot's description of the R2.JIT technique is that writing R2 exactly at
`C0 == R2` *delays the start of the HSYNC black zone*. That effect is HSYNC edge position plus
blanking geometry. A filter that re-synthesizes both from constants cannot pass it through.
The R2.JIT and RFD implementations can be perfectly correct and DSC4 will still look identical
to upstream, because upstream carries the same filter. The same argument covers SHAKER Module A
entries (T), (Y), (TAB) and (R).

**Scope limit, stated honestly.** VRAM addressing (`ma_sel` / `ra_sel`), DE-gated data, border
and ink, screen mode, and the whole Plus sprite and palette path bypass `crt_filter`. This
hypothesis explains the sync-geometry family only. It does not explain the Plus title
symptoms.

**Provenance.** `crt_filter` is core-specific: it is referenced only by `files.qip` and
`rtl/Amstrad_motherboard.v`, and originates from Sorgelig's 2018 CoreAmstrad work. It has
nothing to do with MiSTer's Scandoubler Fx / CRT 25%/50% post-processing, which lives in `sys/`
and operates after this stage. Removing or redesigning it does not affect those.

**Steps.**

1. **Done, merged 2026-08-31 (`accuracy/sync-filter-toggle`)**: `sync_filter` is now an OSD
   option, "Sync filter, On/Off" on the Audio & Video page, backed by `status[35]`. The
   default is On, preserving previous behaviour bit-for-bit. Awaiting hardware test.
2. **Run the experiment. DONE 2026-09-01, hardware.** Result: with the filter off, normal
   software displays correctly, but software using HSYNC tricks (SHAKER, DSC4) is **garbled** —
   not merely wide or off-centre as predicted, but unusable. Turning the filter off also fixed
   none of the Plus title defects, which is consistent with the scope limit stated above.

   **What that tells us.** The unfiltered path cannot hold a lock when line geometry varies,
   which is exactly the condition the CRTC-trickery software creates. That is not an argument
   for keeping `crt_filter` as it is; it is evidence that the module conflates two separate
   jobs, and that only one of them is wanted:

   - **(a) Making sync safe for the scaler.** Regenerating a stable HSYNC/VSYNC so ASCAL and
     the display can lock even when the CRTC emits irregular lines. This is genuinely needed,
     and removing it is what produced the garbling.
   - **(b) Deriving blanking from fixed constants.** `BEGIN_HBORDER`, `END_HBORDER`,
     `BEGIN_VBORDER`, `END_VBORDER` counted off the regenerated HSYNC. This is what destroys
     the R2.JIT black-zone position, and nothing requires it.

   **Proposed third mode: keep (a), replace (b).** Regenerate sync for scaler lock as today,
   but derive HBLANK/VBLANK from the live Gate Array and CRTC signals instead of constants. The
   black zone would then move with the CRTC while the scaler stays locked. This is the design
   to build and test next, and it is a different experiment from the on/off toggle already
   shipped.

   Open question that experiment must answer: whether the visible effect the SHAKER entries and
   DSC4 test lives in the blanking geometry (recoverable this way) or in the sync edges
   themselves (not recoverable while sync is regenerated). If the latter, the honest conclusion
   is that these tests cannot be judged on a scaler-driven display at all, and they need the
   analog output path or a pre-filter capture tap.
3. **Fix the fallback.** With the filter off, blanking currently falls back to
   `hblank = hs_sel` — the raw CRTC HSYNC — so the visible window is much wider than normal and
   likely off-centre. That is acceptable for the experiment and not acceptable as a shipped
   path. Design a real unfiltered blanking derivation from the Gate Array's own outputs
   (`ga40010` exposes `HSYNC_O`, `VSYNC_O`, `VBLANK`, and the ASIC path exposes the
   equivalents).
4. **Decide crt_filter's future.** Options, in rough order of preference: keep it as an opt-in
   compatibility mode for software that relies on a stable monitor-like sync; narrow it so it
   only rescues genuinely absent sync rather than rewriting present sync; or remove it. The
   deciding evidence is step 2 plus a survey of which titles actually break without it.

---

## B2. Hardware-in-the-loop screenshot harness

**Priority: high. Cheapest route to a real oracle.**

This is the only loop that can use Logon System's reference photographs as the oracle, and it
tests the real signal path including the scaler. Every piece already exists in the MiSTer
ecosystem; what is missing is glue.

**Screen capture.** MiSTer has a built-in screenshot feature (Win+PrtScr for the scaled output,
Win+Shift+PrtScr at the core's native resolution, written to `/media/fat/screenshots/`). For
scripting, `alanswx/Screenshot_MiSTer` reads the ASCAL scaler's framebuffer directly from
`/dev/mem` above the 512 MB boundary — Linux on the MiSTer is configured to use only the low
512 MB, and ASCAL's buffers sit above it. The buffer carries a header describing the image
format followed by raw RGB, with a frame counter at `0x20000005` and, under triple buffering,
further frames at `0x2080_0000` and `0x2100_0000`. The triple-buffering caveats in that
project's README must be handled or the capture will tear.

**Input injection.** The Remote Input Server Daemon runs a TCP server and synthesizes
keystrokes through Linux `uinput`, so the FPGA receives them exactly as it would USB input.
That solves keyboard scripting without any RTL change. MiSTer Companion is an SSH management
GUI and is not the tool for this.

**Shape of the work.** An SSH-driven script that loads the core, mounts a DSK/SNA/CPR, plays a
recorded input sequence with delays, captures the ASCAL buffer at a named moment, and stores it
under a test name. Expect to need: the capture utility built for the MiSTer's ARM target, the
input daemon installed and started, and a host-side driver script. Treat "compile something on
the ARM side" as in scope.

**Comparison strategy — the unsolved part.** Reference material at
`https://shaker.logonsystem.eu/tests` exists as CRT photographs of real hardware *and* as
emulator screenshots, multiplied across up to five CRTC types. Photographs will not survive
pixel-exact comparison, and visual-LLM comparison is expensive and unreliable for exactly this
class of small geometric difference. Two practical positions:

- Use **our own captures as the regression baseline** (does this build differ from the last
  build, and where), which needs no reference at all and is immediately useful for bisecting.
- Use **emulator screenshots** rather than photographs for direct comparison, accepting the
  emulator as a rank-2.5 oracle. Reaching Amspirit's accuracy level is an acceptable target.

Decide this before building the comparison half; the capture half is useful either way.

---

## B3. Whole-core Verilator frame harness

**Priority: medium. Portable, and the right debugger for whatever B2 flags.**

Runs on a laptop with no MiSTer attached, and gives cycle-level visibility and bisectability
that hardware capture cannot.

**Feasibility.** Core clock is 16 MHz; one frame is about 320k clocks. Verilator on an
M-series Mac runs a design of this size at a few MHz, so roughly 0.1–0.3 s per frame — a
hundred frames in under a minute. This is ordinary practice in FPGA retro development.

**Most of it already exists.** `sim/plus/p10_boot_test_top.v` already instantiates the
production motherboard with a real T80 and the SDRAM model. What it lacks is real software and
a framebuffer dump; it currently executes hand-written stub programs of a few dozen bytes.

**Design notes.**

- **Tap video both before and after `crt_filter`.** Given B1, the pre-filter tap is where the
  truth lives, and having both makes the filter's effect directly visible as a diff.
- **Input**: SNA loads full Z80, RAM and CRTC state, so it skips booting and typing entirely,
  and it freezes the exact frame of interest. CPR auto-boots and needs nothing. A scripted
  keyboard-matrix driver is the last resort.
- **Known limitation**: for SHAKER specifically, SNA is a poor fit. Each test needs a snapshot
  taken after its menu key, and some tests advance with further keypresses, so covering a module
  means dozens of hand-made snapshots. That is more manual work than scripted input on
  hardware. **For SHAKER, prefer B2 or B4; use this harness for titles and demos**, where one
  SNA or CPR per case is sufficient.

---

## B4. CSL and SSM support

**Priority: medium, but the payoff is an oracle, so raise it if B2 stalls on comparison.**

Longshot's two testing standards (`https://shaker.logonsystem.eu/ssmcsl`):

- **CSL** (CPC Scripting Language) is an ASCII script controlling the *emulator*: insert a
  disk, reset, select the CRTC type, send keystrokes, wait, capture a named screenshot. That
  vocabulary maps almost one-to-one onto the B2 driver script. Implementing it is mostly a
  matter of adopting his format instead of inventing one, and it makes our harness able to
  consume scripts he already publishes.
- **SSM** (ScreenShot Management) is a set of Z80 `ED`-prefixed instruction sequences that the
  *running program* executes to request a screenshot tagged with a 16-bit identifier. This
  requires RTL: detect the sequence in the CPU path and raise a flag the HPS can poll. More
  work, but it produces captures at exactly the moment the test program intends, which is the
  only way to compare against Longshot's reference set without guessing at timing.

**First move is a conversation, not code.** Ask Longshot whether the reference image sets
behind `shaker.logonsystem.eu/tests` are available in a machine-comparable form, and whether
SSM's opcode sequences are documented well enough to implement. A yes turns B2's comparison
problem from unsolved into solved, and would justify the RTL work immediately.

---

## B5. ASIC documentation-gap map

**Priority: high. Cheap, and it converts "we don't know what we don't know" into a list.**

The Plus symptoms are stubborn because the ASIC is far less documented than the CRTC, and we
have no explicit statement of where the documentation runs out.

**Method — invert the problem.** Enumerate every ASIC register and behaviour named in
`docs/references/ArnoldV15.txt` and the CPCWiki ASIC pages already in `docs/references/`. For
each row record: which RTL module owns it, which simulation test exercises it, and which
source documents it. The rows with no owner, no test, or no source *are* the gaps, made
explicit and prioritizable.

**Note on sources.** Arnold 5 is the Plus's development codename, so `ArnoldV15.txt` is
Amstrad's own documentation and outranks any emulator. Amspirit is the most accurate emulator
but is closed-source. Open alternatives (MAME's CPC driver, Caprice32) are weaker on the Plus
than the material already held. Emulator source is not a promising avenue here; the ArnoldV15
document, the CPCWiki ASIC pages, and Longshot himself are the real references.

---

## B6. Plus and classic are structurally entangled

**Priority: high. This is the architect-pass item.**

Both machines are always instantiated and always clocked; only their outputs are muxed.
Concretely, the classic `CRTC` and `asic_video` **both receive every CRTC register write in
both modes** — each carries `.ENABLE(io_rd | io_wr)` with no `plus_mode` gate
(`rtl/Amstrad_motherboard.v`, the `CRTC crtc` and `asic_video asic_vid` instantiations). The
same pattern repeats for `ga40010` versus `asic_ga_timing`.

This is benign for outputs *today*, because the muxes are correct as far as a fast reading
shows. It is not benign as a structure: it is exactly the shape in which a classic code path
silently overrides a Plus fix (which has happened in this project before), and it spends a
large share of the 53% ALM utilization on a machine that is not running.

**Do not split the core in two.** MiSTer convention is one core per machine *family* with a
model selector — the ZX Spectrum core covers 48k/128k/+2/+3/Pentagon, Minimig covers multiple
Amiga configurations, the Atari core switches ST/STE. Splitting would be against convention and
would double the maintenance surface.

**The menu problem has a standard solution.** The OSD usability complaints — "Model" and "Plus
model" both live and only one meaningful, "Load CPR" offered when Plus is off, impossible
combinations reachable — are solved by MiSTer's conditional-option mechanism, not by splitting
cores. `CONF_STR` entries prefixed `d<n>` are hidden when bit *n* of `status_menumask` is clear;
this core already uses it once (`d1P1OR,Vertical Crop,...`) and currently wires only two mask
bits (`.status_menumask({en270p,1'b0})`). Extending the mask to gate the Plus options on Plus
mode, and the classic model options on classic mode, is a contained change.

**The menu problem is wider than greying out.** Conditional visibility via `status_menumask`
solves impossible combinations, but two further asks stand:

- **Group items into presets.** Selecting a machine should carry its sensible companion
  settings rather than leaving the user to assemble a valid machine from orthogonal switches.
- **Remove granularity where it does not earn its place.** Not every option needs to be
  independently settable; some exist only because it was easier to add a switch than to decide.

Both are design work, not mechanism work, and belong in the same pass as the mask wiring.

**Two upstream oddities worth resolving in that pass**, both inherited rather than introduced
here:

- `"F7,E??,Load CPC464 ROM;"` exists because `boot.rom` ends with OS464 + BASIC464 and the only
  way to change that half is to rebuild the whole 160 KB blob. The separate slot is a targeted
  workaround for exactly the pain B10 describes, added upstream rather than in this fork.
- `"R[32],Reset & Detach Cartridge;"` drives `plus_cartridge_memory`'s `detach`, so it *is*
  about Plus CPR cartridges, and it additionally clears the Dandanator EEPROM-loaded flag. The
  label does not say either. It should be Plus-conditional and more clearly named.

**Architect brief (for a strong model — this is the pass to run before B2/B3):**

1. **Sync and blanking ownership.** Given B1, who should own HBLANK/VBLANK, and what is the
   right unfiltered path? Should `crt_filter` survive at all?
2. **Classic/Plus separation.** Is the always-both-live mux structure defensible, or should
   register writes and clock enables be gated by `plus_mode`? What is the resource and risk
   trade-off, and what is the migration path that keeps the classic soak hash stable?
3. **ASIC feature ownership.** Consume B5's map and identify features with no RTL owner.
4. **Menu model.** Specify the `status_menumask` gating so impossible combinations become
   unreachable.

---

## B7. Two cheap audits

**Priority: high. Both are small and both target a class of defect already observed.**

Motivated by the P10j sprite-RAM incident, where a large memory was inferred as thousands of
flip-flops instead of M10K block RAM and was only caught because ALM utilization approached
90%. That is a "knows Verilog, does not know synthesis" failure, and reviewers focused on
finding *mistakes* rather than reconsidering the *approach* will not catch its siblings.

**Audit 1 — synthesis inference sweep. DONE 2026-08-31, see
`docs/b7-synthesis-inference-audit.md`.** Result: no second sprite-RAM-class defect. All 28
"uninferred RAM" instances are correctly too small for block RAM; every real memory inferred.
One follow-up: `asic_video` R16/R17 (CRTC3 light pen) are stuck at GND because nothing writes
them, and this is an unowned gap — F18 covers the classic CRTC readback only and is closed. Original scope follows. Read the Quartus fitter and Analysis & Synthesis
reports for the current build and check, for every memory-shaped structure in the design,
whether it inferred as block RAM or as registers. Also read the removed/stuck-register report:
anything optimized away as unreachable is either dead code or a wiring bug.

**Audit 2 — dark silicon test. DONE 2026-09-01 on `plus/b7-dark-silicon-audit`, see
`docs/plus/b7-dark-silicon-audit.md`.** Result, reproduced independently by the parent rather
than accepted from the delegated report: **the classic-path-leaks-into-Plus hypothesis is
ruled out.** Mutating `CRTC`, `crtc_type0_engine`, `crtc_type1_engine` or `ga40010` in Plus
mode leaves the Plus signature bit-identical, while the same mutations in classic mode do move
it, so the null result is meaningful rather than vacuous. All nine Plus modules shift the
signature when corrupted, so there is no dead Plus code either. Two recorded limits: the two
classic engines are not independently proven, and the fixture does not reach CRTC-type-divergent
behaviour. Original scope follows. For each Plus module, deliberately corrupt it in simulation
and assert that a Plus-mode output changes. Anything that stays green is not in the active
path. Do the mirror test for classic modules in classic mode. This directly answers "are we
running everything we built, and is a classic path overriding a Plus path".

---

## B8. Full independent architecture audit

**Priority: medium, deliberately deferred.**

The 2026-08-31 review that produced this backlog was a fast pass, not an audit: it read the
motherboard's wiring, the video path, and the simulation layout, and stopped there. A proper
audit of the whole design has not been done by any model.

Deferred by decision pending a stronger reviewer (Fable, awaiting a rumoured new version, and
costing usage credits). Run B6 and B7 first — they are cheaper and will narrow what the audit
needs to look at.

---

## B9. Test-suite bloat and review-document archive

**Priority: high. Cheap, and it reduces the cost of everything else.**

`sim/sim_main.cpp` is 8088 lines carrying 71 tests, alongside roughly twenty separate Plus
benches. `docs/accuracy/` and `docs/plus/` hold roughly twenty independent-review documents,
including multi-pass cross-provider reviews of corrections to *prose*.

**Standing rule, now recorded in `CLAUDE.md`:** a test earns its place only if it could have
failed for a reason the author did not already know. A vector derived from an ACCC rule that
was just implemented, asserting that same rule, is documentation with a `make` target.

**Actions.**

1. Sweep the existing suites against that rule and delete what only restates its own
   implementation. Keep everything that pins a *cross-module* interaction, since the shared
   state across `CRTC.v` and the two engines is what the suite genuinely protects.
2. Stop running independent review passes on documentation-only changes.
3. Move superseded review records into `docs/accuracy/archive/` and `docs/plus/archive/`,
   leaving the currently-load-bearing ones in place. `docs/current-status.md` should link the
   archive rather than narrate its contents.

---

## B13. Stale `rom_map` survives every reset (suspected)

**Priority: high. Concrete lead with a reproduction.**

**Symptom, observed on hardware 2026-09-01.** In Plus mode, running a cartridge that had
previously misbehaved left the machine in a state where a subsequently loaded, known-good
cartridge (Navy Seals) produced a **black screen with working music**. Loading a CPR performs a
reset, and that did not clear it. Only reloading the core entirely did.

**Suspect.** `Amstrad.sv` declares `reg [255:0] rom_map = '0;`. The only other assignment is
`rom_map[boot_a[21:14]] <= 1;` — the array is **set-only and no reset clears it**. The `'0`
initialiser applies at FPGA configuration, which is precisely the boundary the user found
themselves needing to cross. Mapped ROM pages therefore accumulate across cartridge and
expansion loads for the lifetime of the configuration.

That matches the symptom shape well: music playing means the CPU is executing, so this is a
paging or ROM-presence fault rather than a dead machine, and "survives reset, dies on
reconfigure" is the exact signature of state with an initialiser but no reset.

**Not yet proven.** No test reproduces it, and `rom_map` may not be the only such state; the
same audit should look for other registers with a configuration-time initialiser and no reset
path.

**First moves.** Reproduce in simulation by mapping pages, resetting, and asserting the map is
clear. Then decide what the correct behaviour is: a full clear on cold reset is the obvious
candidate, but expansion ROMs loaded before boot are presumably meant to persist across a soft
reset, so the two reset tiers need separating rather than collapsing.

---

## B10. ROM slot model and keyboard layout

**Priority: medium.**

`boot.rom` as a single concatenated blob is this core's choice, not a MiSTer-wide requirement —
MiSTer's convention is only that the HPS pushes a file over `ioctl` with an index. This core
already routes several indices separately (`ioctl_index < 4`, `== 7` for the CPC464 ROM, `== 8`
for CPR), so per-slot loading partly exists.

**Concrete example of why the current model is awkward.** The "Load CPC464 ROM" menu entry
exists only because the generic expansion-ROM loader hardcodes which banks it fills.
`Amstrad.sv` writes a loaded expansion ROM into bank 0 and then promotes it to bank 1
(`if(rom_download && ... && !boot_bank) boot_bank <= 1;`), so expansion ROMs reach the 6128 and
664 banks and never bank 2, the 464. The separate menu entry forces `boot_bank <= 2'd2` via
`ioctl_index == 7` and is the only way to write that bank. Upstream added a dedicated slot
rather than generalise the loop, which is the same pain this item exists to fix, one bank at a
time. A proper per-bank model would make that entry unnecessary.

The Amstrad world thinks in lower/upper ROM banks, and hardware expansions such as the M4 let
each bank be set independently. Today, changing the keyboard layout between English, French and
Spanish requires rebuilding the concatenated blob, which is hostile.

**Target:** an OSD keyboard-layout selector backed by per-bank ROM selection, keeping the
existing `boot.rom` path working as the default so nothing breaks for current users. Sequence
this after B6, since the menu model is the same conversation.

---

## B11. Sub-character CRTC granularity

**Priority: medium. Back in scope by decision, 2026-08-31.**

**Plain statement of the limit.** The CRTC engines make their decisions on character phases.
Real hardware resolves events *inside* a character: a Z80 write can land partway through a
fetch and the outcome depends on exactly where. SHAKER Module A entry (1) UPDATE VRAM VS CRTC
(79 tests) and part of entry (4) test precisely that, and cannot pass at the current
granularity however correct everything else becomes.

**Correction to the earlier framing.** The CRTC is not purely character-granular today: it
already resolves half-characters, using both `CLKEN` and `nCLKEN` with explicit
`de_second_half`, `hsync_char_phase` and `r2_jit_pending` state. The remaining gap is finer
than that — resolving the Z80's position within the character, which the (4) entry probes as
`OUT (C),r` third microsecond versus `OUTI` fifth microsecond.

**Compatibility with GA40010 — this is the good news.** `ga40010` is netlist-derived (from a
decapped chip; see the history at `https://github.com/codedchip/AMSGateArray#other-peoples-work`,
meaning it was reconstructed from the physical gate layout rather than written from a behavioural
description, and is therefore the most trustworthy module in the project). It already runs on the
16 MHz clock and generates CCLK, RAS and CAS, so it is *already* sub-character and needs no
change. It is in fact the phase reference the rework would use. **The work is entirely
CRTC-side**: give the engines awareness of the finer phase the Gate Array already provides,
rather than only the character enables.

This was parked early in the project, and the user has since asked for it back in scope. Treat
it as a genuine structural change: it touches state shared across `rtl/CRTC.v` and both engines,
so it needs its own failing vectors first and a careful soak-hash story.

---

## B12. Merge to `master` for an accurate README

**Priority: high. Cheap, and it is currently a reputational risk.**

`master` still presents the upstream core. A casual visitor who finds the fork sees no
statement of its aims and no sign of the work on `accc-review-and-fixes`.

The original plan — clean separated PRs upstream, accuracy and Plus split cleanly — is no longer
realistic after several hundred commits. Merge `accc-review-and-fixes` into `master`, or at
minimum land a README on `master` describing the fork's aims, its two work streams, and its
current state. See `docs/strategy-notes.local.md` (untracked) for the upstream-relationship
question that sits behind this.
