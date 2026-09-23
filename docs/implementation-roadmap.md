# Amstrad MiSTer accuracy and Plus implementation roadmap

This is the execution plan for improving the existing CPC core and, separately, adding
Amstrad Plus/GX4000 support. The detailed evidence remains in `accuracy/` and
`plus/references/`; this document records dependency order, integration boundaries, and
acceptance gates for a fresh implementation session. It is not an exhaustive validation
register: include validation here when it gates a named milestone or selected task. Keep
other residuals in [current status](current-status.md), linked evidence reports and
[review debt](review-debt.md), without automatically promoting them into the execution queue.

Do not combine the two work streams merely because both concern video. Classic accuracy
changes refine `rtl/CRTC.v` and its per-type engines while retaining the netlist Gate Array. Plus support adds a
parallel ASIC path with its own type-3 CRTC, memory mapping, palette, sprites, and DMA. They
can share simulation infrastructure, but neither stream should wait for the other or be
merged into the same behavioral PR.

## 1. Current baseline

Use [current-status.md](current-status.md) for the current integration/artifact handoff and
[the status history](archive/current-status-history-2026-09-21.md) for dated build identities.
Section 8 below is the active queue. Sections 4–5 retain implemented checkpoint contracts and
open acceptance criteria; they are not instructions to rebuild the completed stacks.

- **Classic:** F1–F20 are implemented within their recorded deterministic-model scope.
  The D1/D6 parity repair and F14 type-1 additional-line correction have 232 required classic vectors with no expected
  failures, and the canonical soak is `0xe99ab434a5e1cdb3`. Named hardware questions remain.
- **Plus:** P-2 through P9 and the P10 compatibility repairs are integrated. Hardware confirms
  6128 Plus BASIC boot, Pang/Plotting/arn5diag input, Copter 271 logo colours and title-flash
  repair, and the Burnin' Rubber/CRTC3-demo right-edge sprite repairs. Sonic GX, Copter gameplay
  scrolling, Navy Seals left-edge flicker and the remaining title matrix are still open.
  Left-edge sprite corruption overall is much improved, perhaps fixed; do not promote that
  tentative result to closure.
- **Shared:** B8-1 through B8-7 repairs and B6's video boundary/rendering follow-up are
  integrated. Their production-boundary and hardware residuals are listed in section 8.
  B2 device capture is repeatable. B4 Phase 1 CSL/SSM was device-verified on Module A for both
  classic CRTC types and Gate 1 (Module B); the experimental Phase 2 recorder was retired.
- **Snapshots and reference capture:** B18 classic SNA saving is integrated and device-tested
  on 6128, as a development tool requiring an SSH pull. Capture/round-trip fixture acceptance
  is covered by the reviewed production-T80 fixture within its documented limits. The
  [AmSpirit helper](../scripts/amspirit/README.md) and
  [Copter pilot](investigations/hardware-runs/amspirit-oracle-pilot-2026-09-13.md) already exist.
- **Build/review policy:** use the selected gate (`sim/select_tests.py`), the synthesis routing
  in [ci-testing-policy.md](ci-testing-policy.md), and the actual open rows in
  [review-debt.md](review-debt.md). Source, simulation, synthesis, review and device acceptance
  are distinct evidence; none silently closes another.

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

- A timing/state repair starts with a focused failing vector derived independently from the
  documented rule or reproduced production defect. Keep Classic and Plus behavior separate.
- Before READY or integration, the last code editor runs `python3 sim/select_tests.py --run`
  once and records the command and final `select_tests:` line. Add `--slow` only when the
  change targets a slow fixture's protected boundary. New benches need a `sim/TESTS.md` row.
- Trust a matching completed gate report or green exact-SHA CI; rerun only after later code
  edits, a failure, or missing/mismatched evidence. Do not run baseline suites at task start.
- Use focused checks during implementation and `git diff --check` before committing.
  Pure documentation changes need link/consistency checks, not simulation or independent review.
- Never weaken assertions to make a change pass. A screenshot is supporting evidence, not
  a substitute for a cycle-, byte-, address- or state-specific regression.

### Gate B: synthesis and integration

- Integration pushes are classified by CI; source changes that Quartus compiles receive
  full-effort synthesis. Preserve the exact source SHA, RBF/hash, fitter and timing evidence.
- Stream branches normally stay on simulation. Only dispatch a pre-merge/milestone build
  when explicitly requested: prefer `local-build.yml` with `effort=full` if `quartus-vm` is
  online, otherwise hosted `build.yml`. Do not duplicate the integration push workflow.
- Clock, WAIT, memory arbitration, RGB and top-level wiring changes require full-fit evidence;
  a local leaf test cannot establish their synthesis/timing behavior. Follow
  [ci-testing-policy.md](ci-testing-policy.md) for classification and artifact reuse.
- A cancelled CI run may have been superseded. Find its successor before diagnosing or retrying.

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

These implemented checkpoints retain their dependency order and hardware targets for
regression planning. New classic work starts from a specific discrepancy; coordinate changes
to the wrapper's shared state across both per-type engines.

| Checkpoint | Implemented scope | Deterministic exit | Hardware/software exit |
|---|---|---|---|
| **C0: establish the harness — deterministic complete; hardware pending** | Verilator harness, complete register table and F1 protection implemented | `t01` passes for types 0 and 1, baseline protection tests pass, and no expected failures remain | One baseline RBF builds remotely; classic CPC boot/disk smoke test passes |
| **C1: status readback — deterministic complete; hardware pending** | F2 only | `t06` proves bit 5 changes only at the required C0=R0 sample and excludes the dynamic R6=0 border case; `t01` remains green | SHAKER/type-detection status test |
| **C2: VSYNC write timing — deterministic complete; hardware pending** | F3 only | `t02` covers type-0 blocked writes at C0=0/1, type-0 extended duration, and unchanged type-1 partial-line duration; `t03` protects re-entrancy | SHAKER VSYNC tests plus Onescreen Colonies and PHX regression |
| **C3: R0 stall — deterministic complete; hardware pending** | F5 only | `t09` proves type-0 freeze, the single deferred C4 increment, R2-dependent HSYNC, clean resume, and unchanged type-1 one-character lines | SHAKER R0 tests; monitor sync and GA interrupt behavior remain stable |
| **C4: border decision — deterministic complete; hardware pending** | F6/F13 half-character correction implemented | `t31a` pins the half-character phase; `t10` retains type and skew controls | Visual R1>R0 discriminator and affected demos |
| **C5A: type-0 adjustment arbitration — deterministic complete; hardware pending** | F12 and IA-4 implemented | `t16a`-`t16z` prove C0=0 same-edge comparison, C0=1/R5=0 entry including exact R0=1 rollover consumption, R5 acceptance/rejection around C0=2, R4/R9 live-write windows including exact-R0 at both bus phases, the exact-R0 R9-to-R5 split, R0=0/1 default adjustment, active-adjustment R0=0 freeze, completion reset, retained-state lifecycle, and the French v1.11 p.106 R4-equality history condition | Focused SHAKER or hardware traces verify uncertain sub-character MA/DE/VSYNC timing and the transient R4 restore case without changing the fixed counter expectations |
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
  C `(1)`–`(5)` parity entries, plus the SHAKER 22C/3 parity truth tables (ACCC pp.211-212;
  p.213 is §19.5.4 CRTC 2) as fixture sources.
- Plus P1/P5 (CRTC3 foundation, bus quirks) → run the classic entries above on the CRTC3
  setting where applicable, plus D `(U) CRTC 3/4 : STATUS` once status paths exist.
- Any session touching R12/R13 reload → A `(5)`/`(6)`/`(7)` R13 UPDATE IN n USEC SCREENS
  (mechanism vectors `t20a`-`t20h` already exist locally).

### F6 decision gate

The full options analysis, evidence, staged plan, and revert conditions live in
`accuracy/f6-decision-gate.md`. Stage 1 landed a full-character type-0 DE gap plus
SKEW-DISPTMG handling (`accuracy/a3-f6-stage1`, t10a-t10e). Stage 2 rendered a 16-mode-2-px
(1 µs) seam. Stage 2b's visual reading of ACCC pp.187/196 establishes that the documented
0.5 µs belongs to a sub-character CRTC DE pulse; test/production CRTC clock phase matches
and both GA buffer paths agree. F13 is implemented in the CRTC wrapper with `t31a` pinning
the no-skew half-phases; SHAKER Module A `(O)` plus a DE-pin capture remain required hardware
validation. SKEW-DISPTMG 1/2 retains the p.196 rounded full-character displacement.

### F20 R2.JIT hardware gate

ACCC v1.11 §14.7.1 p.142 is pinned through the production CRTC+GA timing path:
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

The implemented Plus stack is independent of classic C1-C9. Retain these contracts when
repairing a demonstrated compatibility defect; section 8 selects the next work.

### P-2: model selection before Plus behavior

**Status:** integrated. Plus hardware results are recorded in section 1 and current status;
B16 load-time model selection is implemented; normal SNA Main echo is device-observed.
Visual OSD and functional restore acceptance remain.

The separate OSD `Plus model` field is implemented with `Off`, `GX4000`, `6128+`, and `464+`
values. Keep the classic CPC `Model` field separate and preserve the common decode into
`plus_mode` and model capabilities (RAM size, FDC and tape). Cartridge reset-page selection
is deliberately not a static capability: GX4000 fixes the high window to page 1, whereas the
464+ and 6128+ select page 1 or page 3 from the external `/EXP` state. The integrated D5
repair supplies that dynamic input;
its broader model/disk acceptance remains separate from the confirmed 6128 Plus boot.

Preserve the decode/reset/default-off regressions and classic-mode isolation. B16 changes
selection on media load explicitly; it must preserve unrelated status bits and publish the
effective selection back to the OSD.

### P-1: cartridge SDRAM contract before CPR parsing

**Status:** deterministic implementation and Quartus integration complete; production
top-level connection landed at P0 on `plus/p0-parser-wiring`.

The integrated cartridge-memory ownership contract covers:

- the 512 KiB SDRAM region and collision check against every existing client;
- one canonical conversion from `{physical_page[4:0], offset[13:0]}` to SDRAM address;
- ownership of request/acknowledge signals and arbitration between ioctl loader writes and
  CPU cartridge reads;
- priorities during download, reset behavior, bounds rejection, which layer owns short-page
  zero fill, and behavior if the CPU requests a cartridge byte while a load is active; and
- where address arbitration joins the existing RAM/tape/disk SDRAM schedule, including the
  synthesis-visible module/file-list boundary.

Preserve the memory service's ownership and address conversion rather than duplicating them
in the parser, MMU and motherboard. Its regressions cover interleaved loader/CPU accesses,
boundary pages, invalid addresses and reset; P0 already connects RIFF/CPR parsing.

### P0-P9 functional milestones

These phases are implemented within their recorded scope. Their contracts and exact exits
live in `plus/architecture.md` §4. Cartridge boot and CRTC3 timing/pixels underpin the ASIC
page, palette, PRI and sprites; split/scroll, DMA and platform quirks build on those owners.
Preserve that dependency structure when diagnosing a compatibility defect.

Preserve the ASIC-page decode and read-response contract:

- selected readable sources participate in the core's `8'hFF`-neutral wired-AND CPU bus;
- unmapped and write-only reads return the modeled Plus open-bus byte;
- sprite pixel writes retain only the low nibble, X/Y high bytes apply their documented
  masks/sign extension on read, mirrored registers follow the chosen documented rule, and
  DCSR read/write ranges differ as specified;
- ASIC-page writes do not write through to underlying main RAM; and
- any deliberately emulated external-expansion dual-write bug is isolated behind a named
  condition and test, not produced accidentally by incomplete decoding.

Changes use the selected gate and integration synthesis policy, with the relevant
diagnostic/title acceptance kept explicit. A title reaching a screen is useful smoke
evidence, but it does not replace the decoder, counter, interrupt, or compositor assertions.

### P10: post-implementation compatibility closure

**Status:** compatibility and hardware acceptance remain open after the integrated repairs.
Use [current status](current-status.md) and the dated hardware reports for title verdicts.
The September 9 report predates the September 12–14 boot, input and Copter repairs; do not
use it as the latest result. The Navy Seals black screen was not reproduced and Dandanator
was not part of the reported sequence.

P10 is an acceptance/repair stack, not one RTL commit. The rows below name protected
boundaries and residuals; implemented source changes must not be restarted. The former
sprite-memory resource problem is resolved in synthesis by the two-M10K implementation;
no renderer redesign is justified by the older 90%-ALM report.

| Sub-milestone | Scope | Deterministic exit |
|---|---|---|
| **P10a: evidence baseline + production boot harness** | Exact-tip full-effort build; real T80/top-level CPR reset-vector execution and bounded trace | Use the integration build, or an explicitly requested pre-merge full-effort build under the CI policy; constrained internal domains have non-negative setup/hold slack and zero TNS; named RBF/hash and external-path caveat recorded; tiny fixture reaches a pinned PC/page state; BASIC/Panza traces expose first divergence rather than only a screen result |
| **P10b: Plus PPI Port C physical output — implemented** | Preserve Plus physical outputs and classic direction behavior | Physical-pin and PPI/PSG/HID fixtures exist; arn5diag/Pang/Plotting input is hardware-confirmed fixed by the PSG R7 reset repair. D4 post-BSR control readback remains separate acceptance |
| **P10c: model capabilities + FDC reset** | Preserve implemented FDC/tape capability, reset and AMSDOS-alias behavior | Accuracy tip `683fcaf` closes the demonstrated production-timed EDSK READ DATA late-ACK/reset-reload alias in simulation. Hardware exit remains an exact build/media/config capture plus reset during active READ DATA. Retain no-ACK epoch/tag, two-drive overlap, sector-search reset, WRITE DATA `buff_wr`, automatic-EOT C/R, and BASIC with a recorded known-good DSK as named validation residuals |
| **P10d: cartridge execution timing** | Replace per-byte serial SDRAM WAIT only when a real-CPU/title trace proves incompatible pacing | The production harness pins a sustained 4,096-tick cartridge window and 11-tick maximum stall; a valid ordinary-RAM/title comparison, no load/clear or classic regression, and an exact full fit remain required before redesign |
| **P10e: DMA/PPI/PSG arbitration** | Preserve the implemented CPU WAIT and state preservation/restoration contract | The production motherboard fixture now pins physical PSG classification, bounded 8/9/10-CCLK LOADs, late upgrade, one accepted CPU strobe, and a preserved pre-owner AY R14 read. Exact full-fit timing and sample-pitch hardware retests remain required; keyboard and joystick fire issues for Arnold 5 (`arn5diag`), Plotting, and Pang are confirmed fixed on hardware in build `a0778b6` (PSG R7 reset to 0x00) |
| **P10f: dynamic sprite writes** | Close RoboCop's first traced divergence; replace undocumented staging behavior only where evidence requires | Game-derived burst-write/delayed-ACK vector, all-16 dynamic overlap coverage, documented per-access blanking, exact full-fit build |
| **P10g: Panza first divergence** | Close one traced MMU/CRTC3/PRI/video behavior at a time | Each fix has a primary-source or hardware-derived vector; no self-derived expectation from current RTL |
| **P10h: production CPC+ SNA** | Preserve integrated parser/apply repairs; add B16 model selection separately | `Amstrad.sv` snapshot integration test covers model, PPI/PSG, ASIC registers, palette, and sprite data |
| **P10i: hardware matrix** | Repeat the Plus checklist with exact environment metadata | Individual items promoted to hardware-confirmed only with commit, full-fit RBF hash, model/media configuration, and recorded result |
| **P10j: Plus resource and timing closure — synthesis complete, hardware pending** | Sprite pixels now use one explicit Cyclone V true-dual-port M10K per even/odd bank; the renderer did not need redesign because the memory conversion restored sufficient margin | Focused fixtures preserve CPU read/write, SNA write, video-fetch, access-blanking, and mixed-port collision semantics; full simulation/lint and the soak pass; exact feature build `c047a7d` uses 2 M10Ks / 16,384 bits with no soft mirror, total use is 22,057 ALMs (53%), setup/hold are +0.323/+0.251 ns with zero TNS, and CI fails closed on timing violations. Later integration synthesis is recorded in current status; the hardware matrix remains open, and review obligations are tracked only in review-debt.md |

Do not combine P10b/P10c's confirmed defects with P10f/P10g's evidence-gated ASIC changes.
Clock, WAIT, memory, RGB, and top-level arbitration commits require exact full-effort synthesis.
The unresolved sprite `+3` mirror, sprite coordinate formula, PRI offset, lowered-R0, R3-low-
zero collision, and pixel-phase questions remain named assumptions until a focused source or
hardware discriminator settles each one.

**P10f/P10g title-defect discriminators.** These capture fields describe the evidence needed
for each failure family, not an assertion that all four defects remain open. Copter 271's
logo and title flash are hardware-confirmed fixed. For remaining or newly reproduced
failures, a screenshot alone is not an oracle for an undocumented timing rule; locate the
first divergent pixel, line or fetch.

| Screenshot family | Capture at the first bad pixel or line |
|---|---|
| Sonic: large horizontal discontinuities, repeated or relocated scene bands | First bad scan line with CRTC MA/RA, R0/R1/R4/R5/R6/R9, SPLT/SSA/SSCR, video fetch address/data, and whether the ASIC is locked |
| Copter 271: top logo rows in the wrong colour | Winning plane and sprite index for the first wrong pixel, then its source row, palette entry/value, CPU sprite/palette access, and dot phase |
| CRTC3 demo: narrow displaced or streaked fragments | First corrupt line and first divergent fetch with MA/RA, CRTC counters/registers, SSCR/SPLT/SSA, SDRAM address/data, and displayed pixel phase |
| Dick Tracy: top rows horizontally displaced | At the first displaced row: HSYNC/DE, CRTC C0/C4/C9 and registers, SSCR/SPLT/SSA, MA reload, and the first two video fetch addresses |

**Open Plus evidence boundaries** (full traces in the
[2026-09-01 triage record](plus/archive/hardware-defect-triage-2026-09-01.md)):

- *System CPR disk read.* The production-shaped `p10_boot_test_top` with the real u765 and
  `rtl/u765/test.dsk` reaches the first payload byte, where the TV80 surrogate stores `&00`
  instead of `&21`; every bus stage agrees on that edge. It stays `XFAIL fdc-payload-poll`
  (a full 512-byte match is an XPASS). It is not evidence for changing u765 media, sector or
  status RTL. Closure needs the failing System CPR with a known-good AMSDOS disk plus a
  real-T80-capable trace or a hardware capture at the first MSR/data-read transition.
- *OUT(C),r versus OUTD.* The ASIC sees no opcode class, so no opcode-specific ASIC patch is
  justified. The TV80 surrogate has no block-I/O decode, so the synthetic R2.JIT
  discriminator cannot show OUTD bus cadence. Closure needs a real-T80 bus trace or a
  hardware capture of both instructions under the same CRTC phase.

### Optional feature: light pen and light gun (LOW priority, opened 2026-09-01)

Not required for accuracy or for any current finding. Recorded because the hardware picture
turned out to be more interesting than "unimplemented register", and because MiSTer's existing
mouse support makes it reachable without any adapter.

**Current state.** The CRTC light-pen address registers R16/R17 read as zero on both paths and
nothing latches them. On the classic side that is a recorded decision (F11f, and the §21.2
digest footnote: "This core lacks LPSTB capture and returns 0 for R16/R17"). On the Plus side
it is an unowned gap discovered by synthesis as a stuck-at-GND register; see
`docs/investigations/archive/b7-synthesis-inference-audit.md`. The minimum action, independent of this feature, is to
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

### Classic and Plus changes

The original C0–C9 and P-2–P9 implementation stacks are integrated. New work is one bounded
finding or coherent fixture per branch, based on current `master` (or an explicit dependency).

- Keep classic CRTC changes separate from Plus ASIC changes. General/shared work owns
  peripherals and harnesses without combining unrelated behavior streams.
- Keep behavior changes separate from broad refactors; each meaningful commit should remain
  buildable and bisectable. A new finding gets a failing deterministic vector first.
- Register new production modules in `files.qip` and relevant simulation manifests.
- Independent tasks may overlap textually in shared docs; coordinate changes to the same
  loader, counter, bus handshake or memory interface before implementation.
- Coordinated tasks stop at committed, reviewed/gated READY. Integration is serial and needs
  explicit finish authorization; starting a task does not authorize pushing or synthesis.

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

### Integrated implementations and follow-up validation

1. **B16 — implemented; partial device acceptance (Plus).** A CPR loaded with Plus Off selects 6128+;
   an already selected Plus model is preserved. SNA header values 4/5/6 select the recorded
   Plus model before CPU resume, with the OSD status updated. Preserve existing classic-header
   behavior in this slice; whether a classic SNA should switch Plus Off is a separate policy
   decision. Publication/reset ordering is pinned by the reviewed selected-gate implementation;
   Device acceptance is recorded separately below. The 2026-09-22 device run
   found the CPR entry disabled with Plus Off (`d2F8`), which blocked the path; with the
   entry ungated, RBF `4027f5e` boots a CPR from Plus Off and a header-4 SNA selects 6128+.
   [Readback on `cb60ff8`](investigations/hardware-runs/b16-sna-model-readback-2026-09-23.md)
   confirms SNA types 4/5/6 from Off and normal Main echo. Visual OSD labels,
   functional cartridge-backed restores for types 5/6, and explicit Plus model
   preservation during CPR loading remain open. See
   [B16](backlog.md#b16-cpr-and-sna-loads-select-a-plus-model).
2. **B18 — classic capture/round-trip fixture implemented (general).** Acceptance 2 and 3
   from [the design](b18-sna-save.md#acceptance) share one reviewed fixture: real production-T80/GA/PSG/HID and
   clock/bank composition, independently derived saved-header expectations, loader-consumed
   state and RAM comparison, DDR stalls and publication overlap. Reuse the already extracted
   `sna_cpu_header` and existing save/apply paths. Saving itself is integrated; Plus save and
   direct-to-SD transport are outside this slice. B16 owns load-time model policy; coordinate
   any future shared decoder/apply interface change. Both branches integrated without changing
   those shared interfaces. The fixture models external byte memory, fixes FDC/video data
   to zero and covers uncompressed classic restores; see the design for exact limits.

### Independent Plus accuracy work

[B20: interrupt and DMA findings](backlog.md#b20-independent-plus-asic-interrupt-and-dma-accuracy-findings)
is a separate queue from title debugging. Continue from the live PPR repair and
the source-specific empty-vector/double-ack discriminators below. A Sonic reproduction is not an entry gate. Conflicting PRI-phase and
DCSR claims require source/hardware discrimination before RTL changes. Coordinate
shared interrupt/DMA interfaces and device use with the active Sonic task.

B20-1 live PPR writes are implemented and reviewed; original-hardware phase
validation remains open. B20-2/3 now have synthetic and production-T80
[acknowledge discriminators](plus/references/b20-ack-discriminator-2026-09-22.md),
with physical double-ack reachability unresolved. Sonic has a reviewed
[85-tick rearm deadline trace](investigations/sonic/rearm-boundary-2026-09-22.md);
CPU/cartridge-memory latency is the next discriminator before another DMA change.

### Device-dependent work

**Prerequisite:** establish MiSTer reachability before B17 or Sonic GX device work; use a
reachable AmSpirit instance where possible. This is a task-start check, not an assumption
that a previously online device remains available. Keep one device operator.

- **Sonic GX (Plus):** DMA terminal-PAUSE rule (**B20-7**) is **hardware-accepted on `64702ac`**
  ([device acceptance record](investigations/sonic/b20-7-dma-pause-acceptance-2026-09-22.md)).
  With cartridge memory wait latency running at the true hardware READY rate (`03f4724`,
  [cart-wait record](investigations/sonic/cart-wait-2026-09-22.md)), the terminal-PAUSE rule
  restores frame-locked 312-line list recurrence and 8-line handler cadence matching AmSpirit.
  On physical MiSTer hardware, the Sonic title screen renders with 100% coherence (no displaced
  bands or copper raster tears); no-input control reaches Green Hill Zone attract playfield;
  sustained fire input transitions cleanly through the Act 1 title card into live player
  gameplay. Zero regressions observed across Copter 271, Burnin' Rubber, Pang, Plotting,
  Navy Seals, and the CRTC3 demo.
  Copter 271 gameplay scrolling remains separate; its title flash is hardware-confirmed fixed.
- **B17 (general):** bounded timed keyboard replay and Sonic fire through Main keyboard
  joystick mode are device-tested. Physical recording remains deferred because Main grabs
  evdev inputs; physical-controller replay and frame-deterministic checkpoints remain open.
  See [B17](backlog.md#b17-record-user-input-on-the-mister-and-replay-it-as-a-capture-script).
- **Classic retests:** repaired `95e6f56` matches all B (9)/type 1 page-A
  numeric rows. Page B retains C0=3F and MID FRAME SIZE discrepancies. The ACCC does not
  order the C0=3F stage-A/line-end collision; the candidate ordering and the hardware and
  author discriminators needed before an RTL change are in the
  [page-B discriminator brief](investigations/hardware-runs/shaker-b9-page-b-discriminator-2026-09-22.md).
  MID FRAME SIZE has no ACCC rule, and its reference glyph (`4E40` or `4F40`) is still
  ambiguous. Next step: a production-T80 replay of the `&9266..&932B` routine that observes
  the R8=3 write phase and the two VSYNC-count loops. It would show whether the same
  collision applies. Type 0
  is inconclusive after capture timeout. C (4)/type 1 states A–E retain structural
  capture/reference differences. CFG/MENU restoration and cleanup are verified.
  See the [repaired-build evidence](investigations/hardware-runs/shaker-repaired-95e6f56-2026-09-22.md).
  Other open targets remain DSC4 and F13/F20; IA-5 simultaneous raw HSYNC/VSYNC and GA interrupt capture; Q17's R7=38/39
  discriminator. IA-2/3/6, Amazing Demo corruption and production instruction timing retain
  their separate residuals. Do not infer a new classic RTL repair from an unclassified image.
- **Eerie Forest (Plus):** same-line sprite-row retargeting (`566e0c7`) is
  reviewed, gated and candidate-hardware-verified: the three horizontal reveal
  leaks disappear, with Sonic GX title/attract regression samples passing.
  The separate left-edge screen sliver remains unresolved. Preserve documented
  PRI timing; the next useful discriminator needs actual bus-event timing,
  not emulator instruction-completion coordinates. See the
  [graphics investigation](investigations/hardware-runs/eerie-forest-graphics-2026-09-23.md).
- **Plus acceptance:** the remaining title/model/disk/reset matrix in
  [current status](current-status.md) and [the checklist](plus/hardware-test-checklist.md).
  Preserve tentative sprite improvement and inconclusive Sonic results as such.

### Existing foundations and remaining validation

- B2 device capture and B6 boundary/rendering are integrated. Physical HDMI/CRT acceptance,
  Full/Raw visual discrimination and a CPU-generated stuck-high-sync recipe remain open.
  Native MiSTer screenshots omit the OSD and cannot prove the active mode.
- B4 Phase 1 CSL/SSM is device-verified for Module A on both types and Gate 1 (Module B).
  The experimental Phase 2 recorder is retired; do not revive its fit/DDR/throughput queue.
  CSL v1.5/SSM v1.2 source support adds arbitrary-code `wait_ssm` while preserving the 2.6
  scripts. Its bounded device check can reuse an existing Phase 1 SSM-capable RBF because
  the standards refresh changes no hardware logic. Production-T80
  fetch-provider review/evidence remains an explicit review-debt item.
- B8-1 through B8-7 are integrated. Remaining work includes native dynamic-WAIT equivalence,
  complete frame-level RFD/full motherboard CPU execution, full video-consumer validation,
  real-CDT playback/HPS cadence, snapshot first-frame limits and named hardware retests.
  Scripted/TV80 fixtures cannot establish production instruction behavior.
- The AmSpirit helper and Copter pilot are implemented. Joint runs need matching media/model
  configuration and an identified checkpoint on each side; frame counts are not a shared
  state identity. Cartridge snapshots need their CPR first, and ignored SNA state must be
  recorded. See [the automation guide](../scripts/amspirit/README.md) and
  [oracle design](investigations/hardware-runs/amspirit-oracle-design-2026-09-13.md).
- Close actual [review debt](review-debt.md) separately from implementation and hardware
  acceptance. B18 still has slice 3–4c review obligations; a new fixture alone does not clear them.
- B10 locale support needs provenance-backed ROM assets and firmware/keyboard policy.
  B14 host workflow smoke checks and optional light-pen/light-gun work remain lower priority.
  B18 direct-to-SD saving needs a Main_MiSTer transport decision.
