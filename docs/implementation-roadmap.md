# Amstrad MiSTer accuracy and Plus implementation roadmap

This is the execution plan for improving the existing CPC core and, separately, adding
Amstrad Plus/GX4000 support. The detailed evidence remains in `accuracy/` and
`plus/references/`; this document records dependency order, integration boundaries, and
acceptance gates for a fresh implementation session.

Do not combine the two work streams merely because both concern video. Classic accuracy
changes refine `rtl/CRTC.v` and its per-type engines while retaining the netlist Gate Array. Plus support adds a
parallel ASIC path with its own type-3 CRTC, memory mapping, palette, sprites, and DMA. They
can share simulation infrastructure, but neither stream should wait for the other or be
merged into the same behavioral PR.

## 1. Current baseline

- `master` is the upstream baseline.
- Review/correction work lands on `accc-review-and-fixes` (cut from
  `codex/exploratory-gx4000-plus-plan`); stream branches (`accuracy/*`, `plus/*`) cut from it
  only after shared dependencies land there (currently `accuracy/a3-f6-stage1` and
  `plus/p0-parser-wiring`, which rebase onto this branch's post-review fixes).
- The current development state contains the accuracy/reference documents, the F1-F3 and main
  F5 corrections, deterministic-complete F12/F4/F8/F9, the Verilator CRTC/Plus gates plus the
  randomized equivalence soak (`make -C sim soak`, golden hash `0xd6bc1649ff2058a1`), the
  production-wired bounded CPR parser/service/MMU path, R12/R13 reload vectors
  (`t20a`-`t20i`), the per-type engine
  split (wrapper `rtl/CRTC.v` + `rtl/crtc_type0_engine.v`/`rtl/crtc_type1_engine.v`, renamed
  from `rtl/UM6845R.v`), F7's type-1 R5-route RFD with A1/A2, and GitHub Actions synthesis.
  Platform-level references covering the ASIC, Gate Array, MMU, PPI, PSG, FDC, and file formats
  are inventoried in [`docs/references/README.md`](references/README.md).
- GitHub Actions has completed simulation, Quartus 17.0.2 compilation, fitter, TimeQuest,
  RBF packaging, and artifact upload through the pass-2 fix tip `f6f09f5` (run
  `32645547100`). New top-level/file-list commits still require their own run.
- `sim/` currently reports **183** required classic CRTC passes with no expected failures
  (verified 2026-08-31, Verilator 5.050); the soak reproduces golden hash
  `0xd6bc1649ff2058a1`. The Plus leaf, MMU, SDRAM, and boot-integration suites are green.
  Do not start another timing-sensitive finding until its focused failing vector exists.
- P-2 model plumbing, the P-1 cartridge memory/SDRAM contract, and P0 parser/MMU/top-level
  wiring are implemented. Simulation proves atomic publication and cartridge reads through
  the production-sized clear/load path; a real Plus/GX4000 hardware boot remains unverified.
- ACCC v1.11 French is the primary written Compendium baseline
  (`docs/references/ACCC1.11-FR.pdf`, user-owned and untracked); v1.11 English is a working
  translation. The bilingual ledger is `accuracy/accc-1.11-fr-en-differences.md`. Historical
  v1.10/v1.9 reports remain provenance, not the current oracle.
- The v1.10 documentation rebaseline and the deterministic F12/F4/F8 milestones are complete;
  F9 closure is merged into this branch (`t12a`/`t12b`: exact-C0==R0 write → C4=39/C9=8 and
  its windowed companion → C4=38/C9=8, ACCC p.82). F13's ACCC-model half-character DE
  phase is implemented; SHAKER/DE-pin hardware validation remains open. F20's CRTC-1
  R2.JIT start phase and fixed display-reactivation edge are implemented through the integrated
  CRTC+GA path; DSC4 and SHAKER `(TAB)` remain hardware gates. F7 RFD is complete
  in full; the next independent classic checkpoint is
  F10 (fixtures first).

The current branch is a useful staging branch, not a requirement to publish one large PR.
The commits may be rearranged into the small sequences below before publication.
See `current-status.md` for the exact handoff and real-hardware checklist.

## 2. Integration rules

1. Keep classic CPC mode bit-identical when implementing Plus plumbing. With the new Plus
   selector off, existing CPC model bits, CRTC selection, ROM loading, video, memory, disk,
   tape, and snapshot behavior must retain their old paths.
2. Keep Plus code out of `rtl/CRTC.v` and its engines. The Plus ASIC's type-3 CRTC belongs in the new
   behavioral Plus path described in `plus/architecture.md`.
3. Give every behavioral change one focused commit and one deterministic regression test.
   A later commit may refactor only after the behavior commit is independently green.
4. Add every new synthesizable Verilog/SystemVerilog source to `files.qip` in the same
   commit that first instantiates it. Also add it to the relevant simulation file list.
   A simulation-only pass is not evidence that Quartus can see a module.
5. Preserve neutral bus behavior. Existing CPU read responders return `8'hFF` when not
   selected and are combined by bitwise AND. New Plus responders must follow that contract;
   open-bus behavior is an explicit selected response, not an accidental undriven signal.
6. Do not mix a new memory client, a new arbiter policy, and a file parser in one commit.
   Establish and test the memory interface first, then connect the CPR stream.
7. No acceptance test may silently depend on an unavailable ROM, cartridge, disk, or
   hardware setup. Unit tests generate their own stimuli. Manual assets and hardware are
   named separately in the checkpoint record.

## 3. Verification ladder

Every checkpoint advances through the cheapest applicable gates in order.

### Gate A: deterministic local checks

- `make -C sim` must run the full CRTC suite non-interactively and return nonzero on an
  unexpected failure. Existing known divergences may start as named `xfail(Fn)` cases;
  implementing Fn changes only that case to a required pass.
- Plus modules get similarly deterministic, self-contained benches. Tests should assert
  exact cycle, decoded address, selected page, returned byte, interrupt edge, or pixel/pen
  result. A screenshot is not a unit-test oracle.
- Run HDL lint where available and `git diff --check` on every commit.
- A failure reports the test/vector, time, expected value, and actual value and produces a
  bounded VCD trace when timing is relevant.

### Gate B: synthesis and integration

- The GitHub Actions Quartus 17.0 build must produce the RBF, fitter summary, and TimeQuest
  report. First run this once on the unmodified integration baseline; otherwise a toolchain
  failure can be mistaken for an RTL regression.
- Synthesize every checkpoint that changes top-level wiring, clocks, memory arbitration,
  RGB width, or `files.qip`. Small CRTC-only commits may share one synthesis run only after
  their individual simulation gates pass, but keep their commits independently bisectable.
- CI automatically recognizes known project and integration paths. Manually dispatch the exact
  checkpoint for semantic clock/memory/RGB risks or internal-RTL milestones which a path match
  cannot identify; `docs/ci-testing-policy.md` is the durable routing policy.
- Record fitter utilization and the worst timing result at each Plus milestone. A sudden
  change is a regression signal even if Quartus returns success.
- The local UTM/Quartus setup in `docs/building.md` is a second route to the same gate, not
  a different acceptance standard.

### Gate C: real software and hardware

- **SHAKER is not part of the automated loop.** The automated verification loop is Gate A
  (Verilator) plus Gate B (CI synthesis). SHAKER sessions are manual, user-run, and happen
  only at significant milestones — never per-commit. A checkpoint names its suggested SHAKER
  targets in advance so each manual session is milestone-targeted and results are recorded
  per entry.
- Run the named SHAKER entries for the changed behavior before calling a timing fix
  complete.
- On MiSTer, first verify classic boot, a known-good disk `cat`, video, keyboard/joystick,
  and reset. Then run the finding- or milestone-specific titles.
- Capture the RBF commit, MiSTer version, selected model/CRTC, test image hash or filename,
  and observed result. For visual comparisons, retain a real-hardware or trusted-emulator
  reference and describe the expected raster feature. SHAKER results are judged against the
  Logon System reference photographs (`shaker.logonsystem.eu`); the stock core is only a
  regression baseline.
- A hardware-only success never replaces Gate A. It promotes a deterministic implementation
  after simulation and synthesis have passed.

## 4. Classic CPC accuracy checkpoints

Classic work is intentionally serial because most findings touch the same state machine in
`rtl/CRTC.v`. The required order below supersedes the older priority table in
`accuracy/audit-findings.md` where dependencies differ.

| Checkpoint | Work, in order | Deterministic exit | Hardware/software exit |
|---|---|---|---|
| **C0: establish the harness — deterministic complete; hardware pending** | Implement the Verilator harness; run the complete register table; verify the already-present F1 fix rather than rewriting it | `t01` passes for types 0 and 1, baseline protection tests pass, and all unimplemented findings are explicit named xfails | One baseline RBF builds remotely; classic CPC boot/disk smoke test passes |
| **C1: status readback — deterministic complete; hardware pending** | F2 only | `t06` proves bit 5 changes only at the required C0=R0 sample and excludes the dynamic R6=0 border case; `t01` remains green | SHAKER/type-detection status test |
| **C2: VSYNC write timing — deterministic complete; hardware pending** | F3 only | `t02` covers type-0 blocked writes at C0=0/1, type-0 extended duration, and unchanged type-1 partial-line duration; `t03` protects re-entrancy | SHAKER VSYNC tests plus Onescreen Colonies and PHX regression |
| **C3: R0 stall — deterministic complete; hardware pending** | F5 only | `t09` proves type-0 freeze, the single deferred C4 increment, R2-dependent HSYNC, clean resume, and unchanged type-1 one-character lines | SHAKER R0 tests; monitor sync and GA interrupt behavior remain stable |
| **C4: border decision** | F6 only if its approximation is explicitly accepted | `t10` distinguishes type 0 and type 1 and proves skew placement | Visual R1>R0 discriminator and affected demos |
| **C5A: type-0 adjustment arbitration — deterministic complete; hardware pending** | F12 and IA-4, test first | `t16a`-`t16z` prove C0=0 same-edge comparison, C0=1/R5=0 entry including exact R0=1 rollover consumption, R5 acceptance/rejection around C0=2, R4/R9 live-write windows including exact-R0 at both bus phases, the exact-R0 R9-to-R5 split, R0=0/1 default adjustment, active-adjustment R0=0 freeze, completion reset, retained-state lifecycle, and the French v1.11 p.106 R4-equality history condition | Focused SHAKER or hardware traces verify uncertain sub-character MA/DE/VSYNC timing and the transient R4 restore case without changing the fixed counter expectations |
| **C5B: equality/overflow foundation** | F4 only, after F12 establishes the corrected state seam | `t07` and `t08` pass, including the tightened RLAL regression vectors; no shortcut term is retained to hide a latch bug | SHAKER overflow/rupture tests and Batman Forever, The Demo, and Yao demo sweep |
| **C6: type-1 adjustment — deterministic complete; hardware pending** | F8 only, after F4 | `t11` proves independent C5 counting, continuing C4/C9, RA sequence, and the R5=0 mid-adjustment behavior | SHAKER adjustment vectors; Q17 hardware sweep at R7=38/39 for R4=36/R9=7/R5=16 |
| **C7: type-0 R9 race — deterministic complete; hardware pending** | Revised F9 only, after F12/F4/F8 have stabilized the counter structure | `t12` reproduces both documented exact-cycle results using the v1.10 comparison target; it must not preserve the v1.9 rationale as an oracle | Contrived timing test; hardware trace if simulation and SHAKER disagree |
| **C8: type-1 RFD** | F7, then F17's C9=R9 source-state correction, after F4/F8/F9 | `t13` proves trigger timing, frame parity, VMA reload, never-armed behavior, and the p.88 C9=R9 source-state disable; re-derive current `t13d` before RTL | SHAKER RFD tests and a CRTC-1 RFD demo sweep |
| **C9: interlace** | Implemented F10 scope, then fixture-first F14/F15/F16 | Reviewed additional-line, odd-R9, and post-exit fixtures derived from the cited ACCC tables; all prior regressions stay green | SHAKER interlace suite and hardware comparison for both CRTC types |
| **C10: light pen interface decision** | F18, independent of the counter stack | If supported, a captured MA value reads through R16/R17 on both types; otherwise the unsupported LPSTB path is explicitly documented | Expansion-port/light-pen hardware test if the interface is implemented |

### Suggested SHAKER targets per checkpoint

Manual-session target lists drawn from `accuracy/shaker/shaker-accc-crossref.md` (its
citations are unverified until each cited page is confirmed before acting on a result).
Module/key names are SHAKER 2.6 menu entries.

- C1 F2 → B `(S) CRTC 1 : BE00 CHECK`.
- C2 F3 / C3 F5 → A `(I) VSYNC CONDITIONS`; A entries `(4)` and `(U)` for the R0-timing
  edges; B `(P) ANALYZER / FORCED STAB CRTC 0 R0=0`.
- C4 F6 (if accepted) → A `(O) R1 STORIES`.
- C5A/C5B F12/F4 → A `(U) R4 & R9 CHECKING`, A `(P) R6 STORIES`, E `(3) CRTC 0 C4/C9
  COUNTER LOGIC BUG`.
- C6 F8 → build `4c78603` or later: E `(2) CRTC 1 VMA TRT ... ADJ LINE`, E `(1) R5 STORIES
  2ND ROUND`, B `(RETURN) R5 STORIES`, D `(E) CRTC 1 : OFS UPD IN ADD MANAGEMENT`.
- C7 F9 → E `(3)` (same entry covers the C0==R0 comparator switch).
- C8 F7/F17 → C `(1) CRTC 1 : RFD & PARITY STORY`, D `(9) CRTC 1 : RFD ROUND 2`. (B `(O)
  CRTC 1-A OR 1-B?` is the chip-variant discriminator — informative only; the variant is
  deliberately not modeled.)
- C9 F10 → interlace suite: B `(1) INTERLACE C4/C9 COUNTERS`, B `(9) INTERLACE VM`,
  C `(1)`–`(5)` parity entries, plus the SHAKER 22C/3 parity truth tables (ACCC pp.210-211;
  p.212 is §19.5.4 CRTC 2) as fixture sources.
- Plus P1/P5 (CRTC3 foundation, bus quirks) → run the classic entries above on the CRTC3
  setting where applicable, plus D `(U) CRTC 3/4 : STATUS` once status paths exist.
- Any session touching R12/R13 reload → A `(5)`/`(6)`/`(7)` R13 UPDATE IN n USEC SCREENS
  (mechanism vectors `t20a`-`t20h` already exist locally).

### F6 decision gate

The full options analysis, evidence, staged plan, and revert conditions live in
`accuracy/f6-decision-gate.md`. Stage 1 landed a full-character type-0 DE gap plus
SKEW-DISPTMG handling (`accuracy/a3-f6-stage1`, t10a-t10e). Stage 2 rendered a 16-mode-2-px
(1 µs) seam. Stage 2b's visual reading of ACCC pp.186/195 establishes that the documented
0.5 µs belongs to a sub-character CRTC DE pulse; test/production CRTC clock phase matches
and both GA buffer paths agree. F13 is implemented in the CRTC wrapper with `t31a` pinning
the no-skew half-phases; SHAKER Module A `(O)` plus a DE-pin capture remain required hardware
validation. SKEW-DISPTMG 1/2 retains the p.195 rounded full-character displacement.

### F20 R2.JIT hardware gate

ACCC v1.11 §14.6.1 p.141 is pinned through the production CRTC+GA timing path:
type-0/type-1 dynamic `OUT (C),r8` equality starts blanking four/three Mode-2
pixels after the normal start while the type-specific display-reactivation edge
stays fixed, shortening the raw pulse by four/three pixels. The deterministic fixture
is complete; the next acceptance layer is DSC4 plus SHAKER `(TAB)` on real CRTC-1
hardware. Keep RFD×IVM, active-pulse R2 updates, and instruction-form distinctions
as separately named residuals rather than attributing a remaining DSC4 failure
to R2.JIT without a first-divergence trace.

### F10 scope gate

F10 is not a normal one-commit finding. Before RTL work, turn the SHAKER 22C/3 tables into
reviewed fixtures and re-check only the PDF pages flagged by the audit. Keep type-0 and
type-1 implementations separate, with a passing fixture-only commit before each behavioral
commit. F10 is allowed to remain a later project after C8 ships.

## 5. Plus/GX4000 checkpoints

Plus development has its own internal stack. It may begin once the shared build and
simulation conventions are stable; it does not depend on completing classic C1-C9.

### P-2: model selection before Plus behavior

**Status:** deterministic implementation complete; hardware remains deliberately
unselected.

Add a separate OSD `Plus model` field with `Off`, `GX4000`, `6128+`, and `464+` values. Do
not extend or reinterpret the existing classic CPC `Model` field. Decode the new value once
into `plus_mode` and static model capabilities (RAM size, FDC, and tape), then plumb those
signals without changing behavior while `Plus model = Off`. Cartridge reset-page selection
is deliberately not a static capability: GX4000 fixes the high window to page 1, whereas the
464+ and 6128+ select page 1 or page 3 from the external `/EXP` state. P0 must define and
test that dynamic input before implementing reset mapping.

Exit requires an exhaustive decode test, reset/default-off test, and a classic-mode
integration trace showing unchanged selected paths. Reserve non-overlapping status bits and
document them beside the existing status map.

### P-1: cartridge SDRAM contract before CPR parsing

**Status:** deterministic implementation and Quartus integration complete; production
top-level connection landed at P0 on `plus/p0-parser-wiring`.

Before accepting `.cpr`, write and test one cartridge-memory ownership design. It must fix:

- the 512 KiB SDRAM region and collision check against every existing client;
- one canonical conversion from `{physical_page[4:0], offset[13:0]}` to SDRAM address;
- ownership of request/acknowledge signals and arbitration between ioctl loader writes and
  CPU cartridge reads;
- priorities during download, reset behavior, bounds rejection, which layer owns short-page
  zero fill, and behavior if the CPU requests a cartridge byte while a load is active; and
- where address arbitration joins the existing RAM/tape/disk SDRAM schedule, including the
  synthesis-visible module/file-list boundary.

Use a small memory service or a clearly owned extension of `sdram.v`; do not duplicate page
address arithmetic in the parser, MMU, and motherboard. Unit tests must interleave loader
writes and CPU reads over page 0, page 31, the last byte, invalid page/address values, and
reset. Only after this interface passes may P0 connect RIFF/CPR parsing.

### P0-P9 functional milestones

The corrected functional phases and exact exits live in `plus/architecture.md` §4. In
summary: cartridge boot is followed by the CRTC3 counter/timing and basic pixel foundation;
only then come the ASIC page/palette, PRI, and sprites. Exact readback and I/O traps may land
later because they do not provide timing signals consumed by PRI/sprites. Split/scroll, DMA,
and platform quirks (PPI emulation differences, ADC paddle defaults, and SNA v3 `CPC+` snapshot
support per `docs/references/`) remain later milestones.

At the ASIC-page milestone, use an exhaustive decode table and explicit read-response
contract:

- selected readable sources participate in the core's `8'hFF`-neutral wired-AND CPU bus;
- unmapped and write-only reads return the modeled Plus open-bus byte;
- sprite pixel writes retain only the low nibble, X/Y high bytes apply their documented
  masks/sign extension on read, mirrored registers follow the chosen documented rule, and
  DCSR read/write ranges differ as specified;
- ASIC-page writes do not write through to underlying main RAM; and
- any deliberately emulated external-expansion dual-write bug is isolated behind a named
  condition and test, not produced accidentally by incomplete decoding.

Each milestone ends with a classic-mode regression, a Quartus build, and the phase-specific
diagnostic/title named in the architecture. A title reaching a screen is useful smoke
evidence, but it does not replace the decoder, counter, interrupt, or compositor assertions.

### P10: post-implementation compatibility closure

**Status:** OPEN after the 2026-08-30 round-three simulation repairs. P0-P9 are implemented
and simulation-verified, but the two broad hardware samples still expose title, input, FDC,
sprite, DMA, CRTC3, cartridge-timing, and recovery failures pending an exact new build and
hardware retest. The detailed evidence and checkboxes live in
`plus/hardware-checkpoint-findings.md`; the newest repair-to-retest mapping is
`plus/hardware-test-round3-2026-08-30.md`.

P10 is an acceptance/repair stack, not one RTL commit. Keep its sub-milestones independently
reviewable and in this order:

Build 168's full fitter report makes resource closure a concrete Plus task rather than a
general optimization wish: the combined core uses 37,728 / 41,910 ALMs (90%), with
`asic_regs` accounting for about 16,666 ALMs and `asic_sprites` another 4,184. The dominant
candidate is the 4Kx4 sprite-pixel array currently implemented as logic/registers instead of
M10K memory. An exact upstream utilization baseline is not required before addressing this
measured local cost.

| Sub-milestone | Scope | Deterministic exit |
|---|---|---|
| **P10a: evidence baseline + production boot harness** | Exact-tip full-effort build; real T80/top-level CPR reset-vector execution and bounded trace | Dispatch `local-build.yml` with `effort=full` when the Quartus VM is online, otherwise hosted `build.yml`; constrained internal domains have non-negative setup/hold slack and zero TNS; named RBF/hash and external-path caveat recorded; tiny fixture reaches a pinned PC/page state; BASIC/Panza traces expose first divergence rather than only a screen result |
| **P10b: Plus PPI Port C physical output** | Make Port C pins always output in Plus mode while keeping classic direction behavior | Physical-pin vectors for `0x9B`/`0x92`; PPI -> PSG register 14 -> HID row test; Arnold 5 control-write trace establishes whether CF-1 is its cause before the 6128+/464+ retest |
| **P10c: model capabilities + FDC reset** | Enforce FDC/tape presence; reset u765 and motor on the defined CPR/system event; test AMSDOS aliases | Accuracy tip `683fcaf` closes the demonstrated production-timed EDSK READ DATA late-ACK/reset-reload alias in simulation. Hardware exit remains an exact build/media/config capture plus reset during active READ DATA. Retain no-ACK epoch/tag, two-drive overlap, sector-search reset, WRITE DATA `buff_wr`, automatic-EOT C/R, and BASIC with a recorded known-good DSK as named validation residuals |
| **P10d: cartridge execution timing** | Replace per-byte serial SDRAM WAIT only when a real-CPU/title trace proves incompatible pacing | The production harness pins a sustained 4,096-tick cartridge window and 11-tick maximum stall; a valid ordinary-RAM/title comparison, no load/clear or classic regression, and an exact full fit remain required before redesign |
| **P10e: DMA/PPI/PSG arbitration** | Implement the missing CPU WAIT and state preservation/restoration contract | The production motherboard fixture now pins physical PSG classification, bounded 8/9/10-CCLK LOADs, late upgrade, one accepted CPU strobe, and a preserved pre-owner AY R14 read. Exact full-fit timing plus Arnold 5, Plotting, and sample-pitch hardware retests remain required |
| **P10f: dynamic sprite writes** | Close RoboCop's first traced divergence; replace undocumented staging behavior only where evidence requires | Game-derived burst-write/delayed-ACK vector, all-16 dynamic overlap coverage, documented per-access blanking, exact full-fit build |
| **P10g: Panza first divergence** | Close one traced MMU/CRTC3/PRI/video behavior at a time | Each fix has a primary-source or hardware-derived vector; no self-derived expectation from current RTL |
| **P10h: production CPC+ SNA** | Correct parser reset sequencing and consecutive-byte/nibble handling | `Amstrad.sv` snapshot integration test covers model, PPI/PSG, ASIC registers, palette, and sprite data |
| **P10i: hardware matrix** | Repeat the Plus checklist with exact environment metadata | Individual items promoted to hardware-confirmed only with commit, full-fit RBF hash, model/media configuration, and recorded result |
| **P10j: Plus resource and timing closure — synthesis complete, hardware pending** | Sprite pixels now use one explicit Cyclone V true-dual-port M10K per even/odd bank; the renderer did not need redesign because the memory conversion restored sufficient margin | Focused fixtures preserve CPU read/write, SNA write, video-fetch, access-blanking, and mixed-port collision semantics; full simulation/lint and the soak pass; exact feature build `c047a7d` uses 2 M10Ks / 16,384 bits with no soft mirror, total use is 22,057 ALMs (53%), setup/hold are +0.323/+0.251 ns with zero TNS, and CI fails closed on timing violations. Final integration build, hardware matrix, and the exact-tip review-debt row remain |

Do not combine P10b/P10c's confirmed defects with P10f/P10g's evidence-gated ASIC changes.
Clock, WAIT, memory, RGB, and top-level arbitration commits require exact full-effort synthesis.
The unresolved sprite `+3` mirror, sprite coordinate formula, PRI offset, lowered-R0, R3-low-
zero collision, and pixel-phase questions remain named assumptions until a focused source or
hardware discriminator settles each one.

## 6. Commit and PR structure

No PR needs to be created merely to follow this plan. Keep local commits in the same shape
so they can later be pushed as small stacked PRs.

### Classic stack

Recommended stack: `C0 harness` -> `C1 F2` -> `C2 F3` -> `C3 F5` -> optional `C4 F6` ->
`C5A F12` -> `C5B F4` -> `C6 F8` -> `C7 F9` -> `C8 F7`. F10 is a separate stack. The
existing F1 commit can remain distinct; C0 adds its deterministic verification.

- One finding per commit and normally one finding per PR.
- A small PR is roughly one behavior change, its focused vectors, and a short documentation
  update. Avoid a line-count rule; semantic separability matters more.
- Harness scaffolding may be one medium PR, but its initial test-data commits should remain
  reviewable and must not change DUT behavior.
- Merge the classic stack in order. Rebase later branches after each merge so a regression
  can be attributed to one finding without resolving repeated `CRTC.v` conflicts.

### Plus stack

Recommended stack: `P-2 model field` -> `P-1 cartridge memory contract/service` -> `P0 CPR +
boot` -> `P1 CRTC3 foundation` -> `P2 ASIC page/palette` -> `P3 PRI` -> `P4 sprites` -> `P5
CRTC3 bus quirks` -> `P6 split/scroll` -> `P7 DMA` -> `P8 polish` -> `P9 cartridge
tolerances` -> `P10 compatibility closure`.

Current Plus position (2026-08-31): P0-P9 are implemented and simulation-verified, and
HF-1/HF-2/HF-3 have landed. They are not collectively hardware-confirmed. P10 now includes
shared FDC reset/alias hardening, a real-module input fixture, a pinned cartridge WAIT
baseline, a real u765 READ DATA/EDSK reset-reload seam, source-backed CRTC3 R8=3 timing,
inactive-DMA-slot correction, bounded SNA tail headroom, stronger all-16/real-register
sprite discriminators, ACCC-backed CRTC3 R8=1
midpoint/additional-line timing, and a full-motherboard DMA/PPI/PSG concurrency seam. The
u765, CRTC3, and concurrency seams are simulation-verified only. Exact-tip full-effort
synthesis, title traces, hardware retest, no-ACK epoch/tag, two-drive overlap, sector-search
reset, WRITE DATA `buff_wr`, automatic-EOT C/R, odd-R5 CRTC3 behavior,
cartridge-versus-RAM pacing, top-level SNA recovery, and undocumented sprite/coordinate
behavior remain open. P10j has converted the register-backed sprite-pixel array to exactly
two M10Ks while preserving its multi-client access semantics. Exact feature fit `c047a7d`
uses 22,057 ALMs (53%), so no sprite-renderer redesign is justified by the resource report;
setup/hold are positive with zero TNS and a named RBF is packaged. Final integration
synthesis, exact-tip independent review of the two Quartus-compatibility parameter fixes,
and hardware retest remain. No separate upstream utilization build was required.

- P-2 is independently mergeable because default-off behavior is invariant.
- P-1 may be independently mergeable if the cartridge service is unselected in classic
  mode. P0 stacks on it; do not combine the parser with initial arbitration.
- P1-P9 stack internally because they share the Plus ASIC interfaces. P10 starts from their
  integrated result but keeps each confirmed repair or evidence-gated behavior separate.
  Plus work must not stack on unfinished classic findings unless it needs a shared harness
  commit already destined for merge.
- A Plus PR may contain several commits only when they form one vertical milestone: module
  logic, unit tests, integration wiring plus `files.qip`, and documentation. Keep each commit
  buildable where practical and the final PR deterministically testable.
- RGB widening, SDRAM arbitration, clock/WAIT changes, and PPI changes deserve their own
  focused integration review even when they are part of a milestone.

## 7. Bisect and rollback rules

1. Every commit must compile its local simulation targets. Never leave a red intermediate
   commit in a stack that will be used for `git bisect`.
2. Behavior changes and broad refactors do not share a commit. Land a behavior-preserving
   rename/extraction first or after the verified change.
3. Do not squash different findings or Plus milestones together. A fix discovered during a
   later milestone gets a `fixup!` commit until review, then is folded only into that same
   milestone's owning commit.
4. Optional approximations, hardware bugs, and compatibility quirks use named parameters or
   isolated signals and focused tests. Their commits must be safely revertible.
5. Keep `plus_mode = 0` as a stable bisection oracle. For every Plus commit, save a short
   classic regression result; if classic behavior changes, treat it as an integration bug.
6. Tag or record the last commit passing each C/P checkpoint. Retain failing VCDs and Quartus
   reports outside Git or as CI artifacts, identified by commit.
7. When a hardware regression is found, first bisect with the same RBF deployment procedure
   and configuration. After identifying the commit, add a deterministic regression vector
   before repairing it.

## 8. Immediate execution queue

1. Test the synthesized current milestone on real MiSTer hardware using
   `current-status.md`; record classic CPC and F2/F3/F5/F8 results per entry.
2. **D1 source/model audit COMPLETE; hardware follow-up remains.** The section-complete
   French/English sweep and visual calibration are recorded in
   `accuracy/accc-1.11-fr-en-differences.md`. The high-confidence digest corrections are
   applied, and the six behavioral candidates have moved through
   `accuracy/accc-bilingual-implementation-todos.md`, with a paper-derived directed vector
   before every justified RTL change. IA-5 deliberately closes with a hardware discriminator:
   the current integrated CRTC/GA harness has no independent U.S.-ROM phase oracle and a
   synthetic vector would only restate the existing model. Source PDFs remain user-owned and
   ignored; the reproducible v1.11 extraction snapshot is versioned, while unselected
   generated intermediates stay ignored.
   **Historical digest verification already banked, do not redo:** the earlier English-v1.10
   pass retired each extraction flag it could settle and corrected section/page anchors against
   the real table of contents; it then swept `docs/`, `rtl/`, and `sim/` for references to
   the corrected sections. RTL and vector citations count — `t13e`/`t13j`/`t13l` cited §13.5
   p.121 (CRTC 3/4) for a type-1 rule that belongs to §13.3 p.113. Expect this to surface
   rule claims we implemented from a misread digest; each one is a finding, not a typo, and
   gets a vector before any RTL moves. The PDF is authority rank 2 and outranks the digests,
   so where they disagree the digest is wrong. Never commit the PDF.

   **Already banked, do not redo** (2026-08-24): chapter 13's section map is verified —
   §13.3 CRTC 1 p.113, §13.4 CRTC 2 p.117, §13.5 CRTC 3/4 p.121, §13.6 R0 UPDATE p.122-123
   (§13.6.1 CRTC 0/2 chronogram, §13.6.2 CRTC 1 chronogram, both p.122; §13.6.3 CRTC 3/4
   p.123), §13.7 SPECIAL CASES p.124 (§13.7.1 CRTC 1 → §13.7.1.1 R0 UPDATE: OUTI,
   §13.7.1.2 R0 UPDATE: OUT(C),R8; §13.7.2 CRTC 0), §13.8 OFFSET p.126. Chapter 14's map is
   verified too, including §14.5 ABSENCE OF HSYNC p.141. Digest-01 §8.5's chronogram
   annotation is confirmed verbatim, so that half of its flag is retired. The §13.5-for-§13.3
   citation error in `t13e`/`t13j`/`t13l` and the §13.7.1.1-for-§13.6.2 chronogram mislabel
   are both already corrected. **Status (2026-08-24): COMPLETE** (cross-reviewed: GPT
   reviewer-cross pass, then Opus adjudication of two evidence disputes). Every remaining
   flag was re-verified against the PDF renders (pdf-inspector Markdown primary per
   `accuracy/extract/README.md`); most retired as confirmed, four genuine digest errors
   corrected (p.81 period-8 adjustment addressing; §17.5 R1=0 deadline — type 0/1/2 accept
   through C0=0, type 3/4 first too-late C0=3f; p.183 example R1=40/&28; the pp.221-224 IVM
   tables re-adjudicated as R9=6-even and corroborating §19.8.1, with the p.219 gate-token
   polarity tracked as author question Q19 — subsequently resolved 2026-08-25 under F15),
   anchors fixed against the real TOC, and the separate stale-reference sweep applied ten
   further docs fixes with rtl//sim/ citations clean.
3. **D2: find out why the Quartus database cache saves nothing — CLOSED 2026-08-24.** Measured
   across two runs on `accc-review-and-fixes`: run `32657783842` restored the cache
   (`Cache restored successfully`, `build_mode=incremental_db`) and its synthesis job took
   **12.2 min**; run `32652569271` missed the cache (`build_mode=clean`) and took **12.2 min**.
   Identical: without design partitions `--flow compile` never reused the restored databases.
   The cache was removed; all synthesis runs are clean compiles. Evidence in
   `docs/ci-testing-policy.md` and `docs/current-status.md`.
4. Classic stream: F13's CRTC-side half-character phase is implemented from the
   render-verified ACCC rule and is pending SHAKER/DE-pin validation. F20's CRTC-1
   R2.JIT phase is implemented and waits for DSC4/SHAKER `(TAB)` hardware validation.
   F7's planned routes
   are implemented, but the
   Q4 recheck opens F17 for the C9=R9 source-state result and requires `t13d` re-derivation.
   F10's implemented scope is complete; F14/F15/F16 are the fixture-first follow-ups. F18 is
   an independent light-pen interface decision. Q17 remains hardware-gated.
5. Plus stream: P0-P9 and HF-1/HF-2/HF-3 are implemented and simulation-verified, but the
   2026-08-29 and 2026-08-30 hardware samples keep P10 open. Build an exact-tip full-effort
   timing-clean RBF, repeat the recorded matrix, and capture title-level first divergences.
   Keep input/DMA concurrency, cartridge timing redesign, and undocumented sprite/video
   behavior evidence-gated. P10j's sprite-pixel M10K conversion and timing-clean exact
   feature fit are complete; next produce the combined integration RBF, close its narrow
   exact-tip review row, and run the recorded hardware matrix. Do not combine Plus work with
   the classic stream. See
   `plus/hardware-checkpoint-findings.md` and `plus/hardware-test-round2-2026-08-30.md`.
6. F6/F13 proceeds per `accuracy/f6-decision-gate.md`: Stage 2 measured the old 1 µs input;
   Stage 2b assigned the documented 0.5 µs to the CRTC DE phase; the wrapper correction and
   exact `t31a` vector are now implemented. Hardware remains the authority and can reopen it.
7. Common dependencies for both streams (harness helpers, shared docs) land on
   `accc-review-and-fixes`; the running stream branches (`accuracy/a3-f6-stage1`,
   `plus/p0-parser-wiring`) rebase onto it rather than stacking.
