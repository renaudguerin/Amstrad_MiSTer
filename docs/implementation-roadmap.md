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

- **September 12 hardware, `5c16b17`:** BASIC boot is confirmed fixed on
  6128 Plus; left-edge sprite corruption is much improved, perhaps fixed.
  Pang/Plotting fire always pressed and Copter 271's logo remain failing.
  Full versus Raw pixels has shown no visible difference in Amazing Demo,
  DSC4 or SHAKER A (T). See the [report](hardware-evidence-2026-09-12.md);
  B1/B6 visual and P10 subsystem acceptance remain open.

- D3's bounded first-visible-row sprite prefetch and D4's Plus PPI mode-word
  readback are integrated. Their required regressions retain sprite row-colour
  checks and DMA/PPI direction behavior. Post-BSR control reads and real-title
  flicker/input acceptance remain open; see the
  [repair contracts](hardware-diagnosis-2026-09-10.md#d3-repair-contract).

- D5's production `/EXP` input repair is integrated with unchanged MMU
  decoder polarity. The opt-in production-T80 gate requires both unchanged
  BASIC CPRs to emit `Ready` on 6128 Plus and 464 Plus, retaining ROM7,
  direct-page and GX4000 controls. The user confirms physical BASIC boot on
  6128 Plus with `5c16b17`; other model/cartridge combinations and subsequent
  Plus disk-based testing remain separate acceptance steps. See
  [D5 validation](plus/d5-basic-boot-input-2026-09-11.md).

- B8-1 production-phase R5/R0 event retention and B8-6 Plus RGB/metadata
  alignment are integrated as separate changes. Their real-GA/scripted-write
  and production-colour/gamma fixtures run in the default gate. B8-2 selected
  FIELD ownership is integrated with a motherboard/consumer fixture, and four
  optional executed-T80 cases pass. Full vendor video processing, full
  motherboard CPU execution and named hardware retests remain separate.
  See the dated [accuracy](accuracy/b8-1-cpu-write-timing-2026-09-08.md) and
  [Plus](plus/b8-6-colour-boundary-2026-09-08.md) evidence.

- `master` is the primary integration branch (merged from `accc-review-and-fixes`).
- Review/correction work lands on `master`; stream branches (`accuracy/*`, `plus/*`) cut from it
  only after shared dependencies land there. Tasks use ad-hoc worktrees; general tasks use
  `general/*` and host-required prefixes may wrap the scope (for example `codex/accuracy/*`).
  See [the task workflow](task-workflow.md); directory names do not encode stream ownership.
- The current development state contains the accuracy/reference documents, the F1-F3 and main
  F5 corrections, deterministic-complete F12/F4/F8/F9, the Verilator CRTC/Plus gates plus the
  randomized equivalence soak (`make -C sim soak`, golden hash `0xb1cb70da95c2e44f`), the
  production-wired bounded CPR parser/service/MMU path, R12/R13 reload vectors
  (`t20a`-`t20i`), the per-type engine
  split (wrapper `rtl/CRTC.v` + `rtl/crtc_type0_engine.v`/`rtl/crtc_type1_engine.v`, renamed
  from `rtl/UM6845R.v`), F7's type-1 R5-route RFD with A1/A2, and GitHub Actions synthesis.
  Platform-level references covering the ASIC, Gate Array, MMU, PPI, PSG, FDC, and file formats
  are inventoried in [`docs/references/README.md`](references/README.md).
- Latest synthesized integration source `5c16b17` passed full-effort Quartus
  17.0.2, simulation and the required gate in run `34570572190`; setup
  +0.320 ns, hold +0.247 ns, zero TNS. The delivered RBF and hash are recorded
  in [current status](current-status.md). `013c7e5` adds documentation only.
  Earlier B8 artifacts retain their own source identity as comparison baselines.
- `sim/` currently reports **225** required classic CRTC passes with no expected failures
  plus 45 production-GA/scripted-write cases (B8-1, Verilator 5.052); the soak
  reproduces golden hash `0xb1cb70da95c2e44f` after the D1/D6 parity repairs.
  The [D1/D6 branch evidence](accuracy/d1-d6-parity-repair-2026-09-11.md)
  records Gemini high source-review clearance and pending hardware acceptance.
  Four optional executed production-T80 cases and the native/translated bus
  trace pass; full motherboard execution remains open. See the
  [bounded CPU evidence](accuracy/b8-production-t80-2026-09-08.md).
  The Plus leaf, MMU, SDRAM, and boot-integration suites are green.
  Do not start another timing-sensitive finding until its focused failing vector exists.
- P-2 model plumbing, the P-1 cartridge memory/SDRAM contract, and P0 parser/MMU/top-level
  wiring are implemented. Simulation proves atomic publication and cartridge reads through
  the production-sized clear/load path. BASIC boot is hardware-confirmed on
  6128 Plus / `5c16b17`; broader Plus/GX4000 cartridge acceptance remains open.
- ACCC v1.11 French is the primary written Compendium baseline
  (`docs/references/ACCC1.11-FR.pdf`, user-owned and untracked); v1.11 English is a working
  translation. The bilingual ledger is `accuracy/accc-1.11-fr-en-differences.md`. Historical
  v1.10/v1.9 reports remain provenance, not the current oracle.
- The v1.10 documentation rebaseline and the deterministic F12/F4/F8 milestones are complete;
  F9 closure is merged into this branch (`t12a`/`t12b`: exact-C0==R0 write → C4=39/C9=8 and
  its windowed companion → C4=38/C9=8, ACCC p.82). F13's ACCC-model half-character DE
  phase is implemented; SHAKER/DE-pin hardware validation remains open. F20's CRTC-1
  R2.JIT start phase and fixed display-reactivation edge are implemented through the integrated
  CRTC+GA path; DSC4 and SHAKER `(TAB)` remain hardware gates. F7 RFD and the
  implemented F10/F14/F15/F16 interlace scope are complete
  at the deterministic-model level. F17/F18 are also implemented; production-CPU
  timing and named hardware residuals remain separate acceptance work.

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
| **C4: border decision — deterministic complete; hardware pending** | F6/F13 half-character correction implemented | `t31a` pins the half-character phase; `t10` retains type and skew controls | Visual R1>R0 discriminator and affected demos |
| **C5A: type-0 adjustment arbitration — deterministic complete; hardware pending** | F12 and IA-4, test first | `t16a`-`t16z` prove C0=0 same-edge comparison, C0=1/R5=0 entry including exact R0=1 rollover consumption, R5 acceptance/rejection around C0=2, R4/R9 live-write windows including exact-R0 at both bus phases, the exact-R0 R9-to-R5 split, R0=0/1 default adjustment, active-adjustment R0=0 freeze, completion reset, retained-state lifecycle, and the French v1.11 p.106 R4-equality history condition | Focused SHAKER or hardware traces verify uncertain sub-character MA/DE/VSYNC timing and the transient R4 restore case without changing the fixed counter expectations |
| **C5B: equality/overflow foundation — deterministic complete; hardware pending** | F4 implemented after F12 | `t07` and `t08` pass, including the tightened RLAL regression vectors; no shortcut term is retained to hide a latch bug | SHAKER overflow/rupture tests and Batman Forever, The Demo, and Yao demo sweep |
| **C6: type-1 adjustment — deterministic complete; hardware pending** | F8 only, after F4 | `t11` proves independent C5 counting, continuing C4/C9, RA sequence, and the R5=0 mid-adjustment behavior | SHAKER adjustment vectors; Q17 hardware sweep at R7=38/39 for R4=36/R9=7/R5=16 |
| **C7: type-0 R9 race — deterministic complete; hardware pending** | Revised F9 only, after F12/F4/F8 have stabilized the counter structure | `t12` reproduces both documented exact-cycle results using the v1.10 comparison target; it must not preserve the v1.9 rationale as an oracle | Contrived timing test; hardware trace if simulation and SHAKER disagree |
| **C8: type-1 RFD — deterministic complete; CPU/hardware validation pending** | F7/F17 and B8-1 production-phase event retention implemented | `t13` and the real-GA scripted-write fixture cover trigger timing and source-state controls; two executed T80 R5 recipes also pass; complete frame-level RFD and hardware validation remain open | SHAKER RFD tests and a CRTC-1 RFD demo sweep |
| **C9: interlace — deterministic complete; hardware pending** | F10/F14/F15/F16 implemented; retain documented residuals | Reviewed additional-line, odd-R9, and post-exit fixtures derived from the cited ACCC tables; all prior regressions stay green | SHAKER interlace suite and hardware comparison for both CRTC types |
| **C10: readable register matrix — deterministic complete** | F18 readback validation implemented; physical LPSTB capture is a separate optional feature | `t01` pins the supported readback matrix; R16/R17 have no live capture source | See the optional light-pen/light-gun feature below |

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

F10 and its F14/F15/F16 follow-ups are implemented within the recorded scope.
The [implementation notes](accuracy/f10-implementation-notes.md) retain the
source derivations and remaining cases; they are not a request to restart the
completed fixture stack. New interlace work needs a specific source or hardware
disagreement and a failing vector, with type-0 and type-1 behavior kept separate.

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
sprite, DMA, CRTC3, cartridge-timing, and recovery failures. The exact
`ce1d2da` retest on September 9 confirms many persist; CRTC3's right-edge leak
appears fixed and Navy Seals black screen was not reproduced. See
[latest results](hardware-evidence-2026-09-09.md). Further targeted validation
remains open; no Dandanator was involved in the reported Navy Seals incident. Source/test review of the triage seam is CLEAR (Muse read-only 2026-09-03
at `a98590a`; record `docs/plus/plus-review-2026-09-03.md` §6) with separate Dandanator ownership validation,
CRTC3-leak, System-CPR/real-T80/full-top, exact-tip Quartus, and TV80/model limits retained
as validation; B3 foundation (§8 queue) is likewise review-CLEAR
(`docs/plus/b3-frame-harness-review-2026-09-03.md`); P10j stays OPEN LOW on doc comments
only. The detailed evidence and checkboxes live in
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

### Optional feature: light pen and light gun (LOW priority, opened 2026-09-01)

Not required for accuracy or for any current finding. Recorded because the hardware picture
turned out to be more interesting than "unimplemented register", and because MiSTer's existing
mouse support makes it reachable without any adapter.

**Current state.** The CRTC light-pen address registers R16/R17 read as zero on both paths and
nothing latches them. On the classic side that is a recorded decision (F11f, and the §21.2
digest footnote: "This core lacks LPSTB capture and returns 0 for R16/R17"). On the Plus side
it is an unowned gap discovered by synthesis as a stuck-at-GND register; see
`docs/b7-synthesis-inference-audit.md`. The minimum action, independent of this feature, is to
record the Plus-side decision in `rtl/plus/asic_video.v` and in the findings table.

**Three CPC light-pen designs, and they do not share an input.**

- **Amstrad LP-1** connects to the **joystick port**, with the light sensor read on
  Keyboard Row 9 Bit 1. It never touches the CRTC. LP-1 support therefore needs no R16/R17
  work at all: it is a keyboard-matrix bit plus timing.
- **Dk'tronics Lightpen** uses the CRTC `/LPEN` strobe on **expansion port pin 47** (CRTC pin
  3, per the §22 digest). This is the design R16/R17 exist to serve.
- **CPC Plus Aux socket** combines the expansion port's LPEN signal with the joystick fire
  lines. Amstrad added it specifically so a light gun could work on the **GX4000**, which has
  no expansion port. The Trojan Light Phazer used it.

That last point is the interesting one: on the Plus this is a first-class Amstrad connector,
not an expansion-bus hack, so **the Plus Aux socket is the more defensible starting point** if
this is ever implemented. Several Plus cartridges support a light gun.

**No hardware adapter is required.** A light pen physically asserts LPEN when the raster passes
beneath it. Emulators synthesise that from a pointing device's screen coordinates, and the same
approach works here: take MiSTer's existing mouse (or a light-gun device), convert its screen
position to a raster position, and assert the LPEN strobe at that moment so the CRTC latches MA
into R16/R17. A real CRT and a real photodiode would also work but are not needed to make the
feature real.

**Prior art is thin but exists.** Caprice Forever lists Lightpen among its supported
peripherals, and Caprice32 under RetroArch exposes an "Amstrad Lightgun" device type. No
light-pen support is documented for WinAPE or Arnold. So there is something to compare against,
but this is not a widely-copied solved feature.

**Sequencing.** Do the decision-recording now, as part of ordinary hygiene. Treat the feature
itself as optional and independent: it is not blocked by anything and blocks nothing.

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

**Active September 12 priorities:** (1) bring the B2 capture loop through real
device acceptance at `root@mister`, with one device operator and real SHAKER
navigation; (2) finish B6's malformed-raster, Plus scroll/sprite and final-mixer
RGB simulation gaps; (3) close actual source-review gaps and obtain fresh
cross-provider review of new non-trivial changes. These tasks prepare reviewed,
tested READY branches; integration/push is a separate step. The
[latest hardware report](hardware-evidence-2026-09-12.md) guides the work:
6128 Plus BASIC boot is fixed, sprite improvement is tentative, input/Copter
failures persist, and Full/Raw pixels has not demonstrated visible improvement.

The items below are retained validation work outside that bounded task pair.

The September 8 [B8 architecture review](b8-architecture-methodology-review-2026-09-08.md)
sets the current repair order. B8-1 R5/R0 write-event retention and B8-6 Plus
RGB/metadata alignment, plus B8-2 selected FIELD ownership, are integrated;
B8-4 retained video-word coherence is also integrated. Do not restart those
implementations.
Their successful simulation and Quartus artifact do not close hardware symptoms.
See [current status](current-status.md) for accepted source and artifact identities.

1. **Plus validation:** B8-2 FIELD ownership and B8-3 accepted palette-write
   events are integrated separately from `55151a0` and `807f081`. Full ASCAL,
   executed titles and hardware retests remain open; the scripted regressions
   do not establish Copter causality.
2. **Shared validation:** B8-4 coherence and B8-7 tape write lifetime are
   integrated from `7a58f88` and `e7d73ba`. Real-CDT playback remains open.
   These are future validation items, not new tasks for the current run.
3. **Classic validation:** four production-T80 OUT(C)/OUTI cases through the
   real GA/CRTC boundary are integrated from `84f3106`, alongside the 45-case
   scripted fixture. Native dynamic-wait comparison, complete frame-level RFD
   and full motherboard execution remain bounded follow-ups; do not infer
   production instruction behavior from the TV80 substitute.
4. **Plus B8-5:** snapshot apply across DMA, selected video/GA, MMU and palette
   owners is implemented from `f2300be`, with twelve focused cases, full gates
   and fresh independent review. CPU execution remains held through apply after
   storage drain. See the [restore contract](plus/b8-5-snapshot-apply-2026-09-08.md)
   for unrepresented address/phase history and first-frame limitations. Hardware
   validation remains open; prefer DSK/CPR for early automation.
5. **Preserved work:** B3 capture `bb77075` and P10j notes `3db81d0` are
   integrated after the SDRAM repair; retain their bounded synthetic/CPU limits.
   FDC held-read test
   `c1a8ff9` and test consolidation `c12c264` are integrated through refreshed
   `5fcf223`, including the P1 timing-ownership correction; see
   [integration evidence](preserved-work-integration-2026-09-08.md).
   The [September 3 handoff](session-continuation-2026-09-03.md)
   records provenance, not proof that its temporary worktree paths still exist.
   Preserve stashes/private recovery files; do not auto-apply them. Full-sector
   result-phase and classic AMSDOS acceptance remain open.
6. **B2/B4 hardware automation:** follow the existing
   [hardware-loop plan](mister-hardware-loop-plan.md). The host driver and ARM
   MBC cross-build are prepared at `7e39204`; see the
   [driver guide](mister-hardware-loop-driver.md). A bounded CSL subset is
   still separate work.
   With `root@mister` reported online, verify SSH/device operation and require
   three repeatable stable-screen captures
   with build/media/model/CRTC/filter identity. Exact SSM event-to-image capture
   remains a later gate.
7. **Hardware retests:** use the delivered SHA-labelled RBF for DSC4/SHAKER,
   IA-5/Q17 and the named Plus title matrix. Retain the
   [September 9 results and capture index](hardware-evidence-2026-09-09.md) as regression
   evidence, not blanket closure. New title-driven RTL work starts from an
   actionable capture or another independently reproduced production defect.

F10/F14/F15/F16/F17/F18 and P0–P9 are implemented within their documented scope.
P10 compatibility, production-boundary validation and hardware acceptance remain
open. B10 locale work still needs provenance-backed ROM assets and a firmware/
keyboard policy; light-pen/light-gun input remains optional. Completed D1 source
verification and D2 Quartus-cache investigation are not new execution tasks.
