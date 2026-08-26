# Current implementation status

This is the handoff for the next development and hardware-test session. Hardware observations
below remain dated 2026-08-19; current simulation and synthesis evidence is newer. The
`accc-review-and-fixes` branch now contains the ACCC review/corrections, per-type classic CRTC
split, F6 Stage 1 full-character approximation, sampled-field soak expansion, production Plus
P0 cartridge wiring, the simulation-only P1 CRTC3 foundation, the implemented F7/A1/A2
classic work, and the F14/F15 classic closures (2026-08-26, see the newest milestone below). The earlier whole-branch reviews are recorded in
`accuracy/accc-review-and-fixes-independent-review.md` (pass 1),
`accuracy/accc-review-and-fixes-independent-review-pass2.md` (pass 2), and
`accuracy/accc-review-and-fixes-independent-review-pass2-fixes-verification.md` (pass 3,
which accepted all pass-2 remediations); the 2026-08-23 review of the F7/A1/A2 and Plus
follow-up deltas is `accuracy/f7-plus-followups-independent-review.md`, and both
review-debt rows are cleared. The §13.7.1.2 R0-widening trigger took two cross-provider
passes recorded in `accuracy/f7-r0-widening-independent-review.md` — pass 1 returned NOT
CLEAR on two blocking findings, pass 2 cleared the remediation on 2026-08-24 — and the
branch is merged, so no review-debt row is outstanding. The detailed behavioral rules remain in
`accuracy/`; the long-term ordering remains in `implementation-roadmap.md`.

## How hardware testing fits the loop

SHAKER is **not** part of the automated loop. The automated loop is the Verilator suite
(`make -C sim`) plus GitHub Actions synthesis. SHAKER sessions are manual, user-run, and
happen only at significant milestones, against a named target list recorded before the
session. A green simulation gate is never evidence of hardware accuracy; a manual session
never gates a commit.

## Hardware-test milestone

`27cb993` is the newest successfully synthesized code milestone (GitHub Actions run
`32970234749`, 2026-08-26, workflow_dispatch, Quartus 17.0.2, full effort): the merge of
`accuracy/f14-f15-interlace` -- the F14 additional interlace line on both classic types and
the F15 type-0 odd-R9 IVM counting with its VSYNC delay correction, fixtures `t27a`-`t27d`,
`t28a`-`t28c`, `t29a`-`t29e`, independently reviewed by Codex with all four findings
remediated (`accuracy/f14-f15-independent-review.md`) -- on top of the Plus P4 mobo-bench tip.
**It has not been hardware-tested.**

- Simulation: 169 required CRTC passes / 0 failed, all Plus leaf/integration suites green;
  lint clean. Soak re-minted `0x85b3f8e847430495` for F14 and F15 (full chain in AGENTS.md);
  even-R9 and non-IVM behavior bit-identical, t21-t24 untouched.
- Logic utilization 28,533 / 41,910 ALMs (68 %); 37,685 registers; 685,217 / 5,662,720
  block-memory bits (12 %); 100 / 553 RAM blocks; 34 / 112 DSP blocks; 145 / 314 pins.
  Versus the previous milestone `c762d36` (29,971 ALMs, 37,442 registers) the classic
  CRTC delta alone enters the fit: -1,438 ALMs (fitter re-placement around the new
  frame-end intercept logic), +243 registers, memory/DSP unchanged.
- Worst-case setup slack +0.635 ns, hold +0.151 ns -- positive; setup improved +0.375 ns
  versus `c762d36`. No regression signal.
- RBF retained as `output_files/hardware-milestones/Amstrad_20260826_27cb993.rbf`
  (SHA-256 `2a94701c30c922f94e3f3bb8050f0e2a5bfa437d90930688330c7ae3d053e79a`). Use a
  current-tip RBF for the next SHAKER session; interlace-aware tests (SHAKER 22C/3
  toggles, Module A `(O)`) are the ones this change should move.

The P4 work merged three times (`0dabcb8`, `85b0eaa`, then `143a213`
on 2026-08-26 carrying review-pass 3-5 remediations with the thread
closed CLEAR — see the cleared review-debt row for the full record).
Exact synthesis ran green at the engine tip `e3dd848`/`69e5d91`
(run `32902476483`: 71 % ALMs / +0.436 ns setup), at merge tips
`b533a93`/`dcbc6ad` (run `32914211795`: 72 % ALMs, +0.260 ns setup),
and a fresh dispatch re-confirmed at the docs tip `abfcadc`: GitHub
Actions run `32918709419` (workflow_dispatch, Quartus 17.0.2) passed
simulation, policy, exact synthesis and the required gate — 29,971 /
41,910 ALMs (72 %); 37,442 registers; worst-case setup slack +0.260 ns,
hold +0.241 ns; RBF artifact `Amstrad_20260826_abfcadc.rbf`. Identical
fitter figures to `dcbc6ad`: the fq_row/fq_acc validation flops and
their compare mux fit inside existing slack; no regression signal.

The classic stream's `accuracy/d1-followups` branch then merged onto
this tip as `c762d36` (2026-08-26; t25 type-0 adjustment addressing
pins, t26 section 17.5 R1=0 deadline pins, the Q10/Q11/Q12/Q19
re-adjudication with finding candidates F14/F15, and the A4/N-6
housekeeping; reviewed CLEAR pre-merge by ask-claude review at Opus 5
high with gates and bite-tests independently reproduced, seven
non-blocking findings remediated at `6c7c905`). Dispatched CI run
`32921230624` (2026-08-26, Quartus 17.0.2) passed simulation, policy,
exact synthesis and the required gate with fitter figures identical to
`abfcadc` (29,971 / 41,910 ALMs, 37,442 registers, +0.260 ns setup) —
the branch's RTL delta is a comment change, so the fit is neutral; RBF
artifact `Amstrad_20260826_c762d36.rbf` (SHA-256
`981fd9dcd7f81ed5d1c9f53447b722381b13169f086a23adcf414e55f40ec858`).
The branch's own intermediate milestone (comment-only RTL, sim/docs
delta) is run `32917100161` at `d9abf35`: 16,208 / 41,910 ALMs,
21,112 registers, +0.623/+0.231 ns. Soak unchanged at
`0x63d9de100ac9f6f2` across the whole session; nothing hardware-tested.

`b533a93` was the first merge's synthesized milestone: GitHub Actions run `32905405634`
(2026-08-25, Quartus 17.0.2, workflow_dispatch at the merge tip) passed
simulation, policy, exact synthesis and the required gate. Fitter:
29,893 / 41,910 ALMs (71 %); 37,506 registers; 685,217 block-memory bits
(12 %); 34 / 112 DSP blocks; worst-case setup slack +0.436 ns, hold
+0.179 ns — positive; no regression signal. Versus `5d6d342` (16,198
ALMs / 39 %) the growth is the sprite engine's flop-based staging arrays
and datapath entering the fit for the first time; memory bits are
unchanged by design. RBF retained as artifact of run `32905405634`
(`Amstrad_20260825_b533a93.rbf`). Not hardware-tested. The push-run
`32905376888` is identically green. Historical paragraph:

NOTE superseded — see above.



`5d6d342` is the newest successfully synthesized code milestone (GitHub Actions run
`32852900420`, 2026-08-25, Quartus 17.0.2): the merge of `plus/p2-asic-regs`
into this branch - P1 review follow-ups, the calibrated p1_video bench, the P2 ASIC
register page (asic_regs + motherboard integration + 4-bit RGB widening) and P3
interrupts (PRI merged into asic_ga_timing, DCSR fields, IVR vector supply), on top
of the F11h/t24 classic work. Both streams' independent reviews are complete and
remediated (`docs/plus/p2p3-independent-review.md`, plus the F10/F11h records).
**It has not been hardware-tested.**

- Logic utilization 16,198 / 41,910 ALMs (39 %); 21,030 registers; 701,601 /
  5,662,720 block-memory bits (12 %); 34 / 112 DSP blocks.
- Worst-case setup slack +0.665 ns, hold +0.250 ns - positive; no regression signal.
- RBF retained as `output_files/hardware-milestones/Amstrad_20260825_5d6d342.rbf`
  (SHA-256 `94b426a49f612895ff287072409badd452cddfa685f3bd7c27f130d3a5e75af5`).

## Hardware-test milestone

`df6c0f7` is the newest successfully synthesized code milestone of the classic stream
(GitHub Actions run `32845921357`, 2026-08-25, Quartus 17.0.2): the merge of
`accuracy/f11h-and-ivm-vsync-coverage` (F11h same-edge R12/R13 closure, t24 type-1 IVM VSYNC
positions + MID-VSYNC, checkout v7 bump; reviewed and remediated) on top of the F10 merge
`e78e0ab`. Recorded retroactively: the original recording commit (`b806502`) landed empty,
so this section was re-inserted from its message and the session record. **It has not been
hardware-tested.**

- Logic utilization 15,745 / 41,910 ALMs (38 %); 20,651 registers; 685,217 / 5,662,720
  block-memory bits (12 %); 100 / 553 RAM blocks; 34 / 112 DSP blocks. Versus the previous
  classic-stream milestone `e78e0ab` (15,718 ALMs, 20,717 registers) the delta is the classic
  CRTC changes alone (no Plus file moved); memory and DSP are unchanged.
- Worst-case setup slack +0.662 ns, hold +0.228 ns — positive; no regression signal.

## Hardware-test milestone

`3d7a178` is the newest successfully synthesized code milestone (GitHub Actions run
`32810340518`, 2026-08-25, Quartus 17.0.2): the Plus branch `plus/p2-asic-regs` with
the P1 review follow-ups, the calibrated p1_video bench in the gate, the P2 ASIC
register page (`asic_regs` + motherboard integration + 4-bit RGB widening), and P3
interrupts (PRI merged into asic_ga_timing, DCSR fields, IVR vector supply on
acknowledge). **It has not been hardware-tested.**

- Logic utilization 15,868 / 41,910 ALMs (38 %); 20,551 registers; 145 / 314 pins
  (46 %); 689,313 / 5,662,720 block-memory bits (12 %); 34 / 112 DSP blocks. Versus
  `e78e0ab` (15,718 ALMs, 20,717 registers) the delta is `asic_regs` plus the PRI
  merger entering the fit; memory +4,096 bits (sprite RAM); DSP unchanged.
- Worst-case setup slack +0.657 ns, hold +0.243 ns — positive; no regression signal.
- RBF retained as `output_files/hardware-milestones/Amstrad_20260825_3d7a178.rbf`
  (SHA-256 `1d47554fca5d082170855c637c224c67d85cc854da91c18a958c61f47a291505`).

The prior milestone paragraph is kept below for bisection history.

`e78e0ab` was the newest synthesized code milestone before that (GitHub Actions run
`32789356344`, 2026-08-25, Quartus 17.0.2): the merge of `accuracy/f10-fixtures` (F10
interlace parity machinery, both types, reviewed and remediated) on top of the Plus P1
motherboard integration (merge `4cab4ec`). **It has not been hardware-tested.**

- Logic utilization 15,718 / 41,910 ALMs (38 %); 20,717 registers; 685,217 / 5,662,720
  block-memory bits (12 %); 100 / 553 RAM blocks; 34 / 112 DSP blocks. Versus the
  pre-both-streams `de30faf` (15,378 ALMs, 20,168 registers) the delta covers the Plus P1
  motherboard integration and the F10 machinery together; memory and DSP are unchanged.
- Worst-case setup slack +0.734 ns, hold +0.246 ns — positive; no regression signal.
- Every synthesis is now a clean compile and `reports/quartus-cache.txt` records
  `build_mode=clean` (queue item D2, closed 2026-08-24: the Quartus database cache was
  removed after the investigation showed it saved zero time; evidence in
  `docs/ci-testing-policy.md` and the roadmap).
- Queue item D2 closed 2026-08-24: the Quartus database cache was removed after the
  investigation showed it saved zero time (no design partitions → `--flow compile` never
  reuses the restored databases; evidence in `docs/ci-testing-policy.md` and the roadmap).
  Every synthesis is now a clean compile.
- The synthesizable delta over the previously synthesized `f6f09f5` is the CRTC change alone:
  the Plus P1 pixel path lives in `rtl/plus/asic_video.v`, which no QIP compiles, so it is
  simulation-only and contributes nothing to this bitstream. **When `asic_video` is wired into
  the motherboard it must be added to `files.qip`**, or no CI run will ever tell you whether it
  fits or meets timing. `5ddddef` remains the
newest hardware-*tested* milestone, covering the
deterministic-complete F12/F4 counter work and the CPR parser. `1a1233f` is the previous one; GitHub Actions run
`31661330994` passed the complete Verilator gate, Quartus 17.0.2 compilation, fitter,
TimeQuest, RBF packaging, and artifact upload for it, and it carries the independently
reviewed C0>=2 F12 arbitration slice on top of the earlier Dandanator/SDRAM milestone.

The retained CI builds have been downloaded locally under the ignored
`output_files/hardware-milestones/` directory:

| Milestone | Commit | RBF SHA-256 |
|---|---|---|
| F2 status readback | `9c16729` | `40992b8e41ead9a9441734aedd171bd0e0942412fbadb089d62e26e4d3e8ba0c` |
| F3 complete, before F5 | `9956d83` | `9634938072dec7a9c82676a3a5b7192ef6927df50e77d164b84963d0d6c554d2` |
| F5 plus tied-off SDRAM foundation | `4ffa853` | `67efe7c7d07f49b31edb6dfca0c19ccf99237bd62df5eefdb0c780a86c95f0a9` |
| Final milestone with Dandanator isolation | `ba5b629` | `7ede21c7449868764f576c114f1697ffd5e6ce4a9b98a38679861d2d52dd3249` |
| F12 C0>=2 arbitration | `1a1233f` | `fd9705732ae20cb45f1807d4c980b893e974392c3b8f48bdb69ff57794f93319` |
| F12 complete | `365c132` | `f44e16cc8c815a5d34c4a807feadbb54a25d358175fb4d542dfbcfddbd20f231` |
| F4 complete plus CPR parser | `5ddddef` | `e1ba1728435f33fc4fe8e1886b0b7b4021f12a4dff861767440b9e2b60a65ff6` |
| F8 type-1 C5 counter | `4c78603` | `8caa9a9f4db825e6fe0d375554a6810e053ec4839409944139e18b29c8bc8e0b` |

The `1a1233f` fitter used 14,947 / 41,910 ALMs (36%), 685,217 block-memory bits (12%),
and 3 / 6 PLLs. Worst setup and hold slacks were positive at +0.541 ns and +0.192 ns.
The later `365c132` F12-complete build also passed synthesis, with worst setup and hold
slacks of +0.472 ns and +0.253 ns respectively; it has not yet been hardware-tested.
GitHub Actions run `32251491936` synthesized the F4-complete/CPR-parser state at `5ddddef`.
Its fitter used 14,899 / 41,910 ALMs (36%), 685,217 block-memory bits (12%), and 3 / 6
PLLs. Worst setup and hold slacks were +0.606 ns and +0.254 ns. TimeQuest still reports
the repository's existing unconstrained external I/O paths, so these positive internal
slacks are not full timing closure. This RBF is retained under
`output_files/hardware-milestones/f4-plus-cpr-5ddddef/` and is the one hardware-tested on
2026-08-19. The intermediate `365c132` build remains untested and is kept only as a bisection
point.

GitHub Actions run `32289023249` then synthesized `4c78603`, the first build containing F8 on
top of that F12/F4 and CPR-parser state. Its fitter used 14,947 / 41,910 ALMs (36%), 685,217
block-memory bits (12%), and 3 / 6 PLLs, with worst setup and hold slacks of +0.516 ns and
+0.246 ns. The artifact is retained under
`output_files/hardware-milestones/Amstrad-build-17-1/Amstrad_20260819_4c78603.rbf`. It has not
been hardware-tested, and it is the build the next SHAKER session should use: `5ddddef`
predates F8 and cannot produce evidence for it. The later per-type split and rename were
behaviour-preserving within the directed, soak, and frozen differential projections, but F6
Stage 1 intentionally changed classic DE behaviour and re-minted the soak. F7 RFD, the A1
VSYNC correction, and the A2 reload caveat then re-minted it again with unchanged
seed/schedule/projection; the F10 work re-minted it four more times (fixture-stage field
expansion, one intended behavior mint per type, and the review remediation) and the
current canonical hash is `0xa9e5026de83d287c` (full chain in AGENTS.md). Use a current-tip RBF for F6/F7 work; retain `4c78603` as the clean F8-era
bisection milestone.

Hardware testing on 2026-08-19 covered two milestones and returned the same result for
both. `1a1233f` showed no regression against the stock core and no CRTC-0 compatibility
improvement in the SHAKER Module A tests that were run. `5ddddef`, which adds the
deterministic-complete F12/F4 counter work and the CPR parser, was then tested and also
showed no regression and no Module A progress.

Treat that as a signal about coverage, not only about correctness. The completed F12/F4
counter arbitration work moved nothing that the attempted Module A subtests measure, so
further counter-internal work is unlikely to change those tests. Both sessions tested only a
few subtests and did not record them individually, so no failure can currently be mapped to a
named finding.

Before the next classic RTL change, close that data gap:

- Record every Module A subtest by name and result, for both supported CRTC selections, with
  the stock core tested side by side in the same session.
- Confirm that SHAKER's own CRTC identification agrees with the OSD CRTC selection. If it
  does not, every Module A comparison so far is uninterpretable and that is the first bug to
  chase.
- Map each persistent difference to an implemented finding or to a named gap. The current
  leading hypothesis is that Module A leans on behaviour that is still unimplemented —
  F10 interlace parity and F13's hardware-blocked half-character F6 seam — rather
  than on the counter internals already fixed. F8 (type-1 C5) and F7's type-1 R5-route RFD
  are now implemented, so they are no longer candidate explanations; F6 Stage 1's
  presence/type/skew approximation landed 2026-08-23.

Do not infer hardware accuracy from the green counter-level simulation gate alone.

The build is suitable for classic CPC regression testing. It contains the F1, F2, F3, and
main F5 CRTC accuracy work. Plus P0 is now production-wired: `.cpr` parsing, atomic cartridge
publication, SDRAM service, MMU windows, and CPU WAIT are connected and covered by an
integrated production-sized simulation. A real `.cpr` boot on MiSTer hardware is still
unverified, and Plus video remains the uninstantiated P1 foundation; selecting a Plus model is
therefore a manual P0 checkpoint, not evidence that Plus support is complete.

For a first MiSTer pass:

1. Keep `Plus model = Off`. Boot each classic model that you normally use and test both
   supported CRTC selections.
2. Confirm BASIC boot, keyboard/joystick, reset, video stability, tape if available, and
   `cat` plus a program load from a known-good disk.
3. Exercise SHAKER's status/readback and dynamic VSYNC tests for F2/F3.
4. Exercise its type-0 R0=0/stall tests for F5. Watch sync stability and Gate Array
   interrupt behavior as well as the diagnostic result.
5. Load a normal 512 KiB Dandanator image. The build rejects bytes at `0x080000` and
   above so malformed uploads cannot overwrite the reserved Plus cartridge region.
6. Record the RBF commit, MiSTer version, classic model, CRTC type, diagnostic/image name,
   and observed result. If a regression appears, compare the F2, pre-F5, and final milestone
   RBFs retained from CI before narrowing it further.

## Completed classic accuracy work

- A deterministic Verilator harness and GitHub simulation gate now cover the CRTC at pin
  level. Unexpected passes of named divergences fail the suite.
- F1 register readback is protected across R0-R31 for both supported CRTC types.
- F2 implements type-1 status bit 5 sampling at the required C0=R0 point and excludes the
  dynamic R6=0 false case.
- F3 implements model-specific live R7/VSYNC timing, including blocked type-0 early writes,
  partial-line duration, interlace boundaries, live type changes, and snapshot loading.
- F5 implements the type-0 R0=0 freeze, R2-dependent HSYNC behavior, live entry and
  recovery, MA/RA behavior, type switching, odd-field VSYNC freeze, and the single C4
  increment when C9 already equals R9 at freeze entry.
- ACCC v1.10 F12 counter arbitration is deterministic-complete for the documented entry
  paths. `t16a`-`t16s` cover R5 arbitration at C0=2, the R4/R9 last-line write windows,
  same-edge C0=0 writes, the C0=1/R5=0 equality-break route, exact-C0=R0 switching,
  `R0=0` and `R0=1` default adjustment, active-adjustment freeze, exact short-line latch
  consumption, bus phases, completion, and retained-state lifecycle. Exact sub-character MA/DE/VSYNC behavior still requires hardware or SHAKER
  traces; the deterministic assertions use C4/C9/RA and adjustment state.
- F4 removes the non-equality C9/C4 zero-limit shortcuts for both CRTC types and preserves
  type-0's live-versus-latched Last Line/RLAL behavior. Live R9 writes now also feed the
  independent VMA capture comparison, while active type-0 adjustment reuses C9 against
  R5 rather than R9.
- F8 gives the CRTC 1 type-1 vertical adjustment its own 5-bit C5 counter. C9 now keeps
  cycling 0..R9 during adjustment so the row address keeps cycling; C4 increments at each
  C9==R9 wrap; C5 counts the adjustment lines and the adjustment ends when C5+1 equals R5
  by equality, so R5=0 never ends it — the documented ACCC 11.3.2 hardware bug, reproduced
  deliberately. The comparison is widened to six bits so C5=31 (32) cannot alias R5=0.
  The two former F8 expected-failure cases (`t08f`, `t08g`) are now required passes.
- The current local gate reports 152 required CRTC passes, zero expected failures, no
  unexpected passes, and no failures (verified 2026-08-25, Verilator 5.050). The randomized
  equivalence soak reproduces golden hash `0x63d9de100ac9f6f2` (chain in AGENTS.md). The
  §13.7.1.2 trigger leaves the hash unchanged because random traffic does not reach that
  window; per review finding F-9 the soak is measurably insensitive to this region, so the
  directed vectors — not the soak — carry the behavioral proof here.
  The Plus leaf and SDRAM integration suites are also green.
- The core is split into a shared-state wrapper (`rtl/CRTC.v`) plus two per-type rule engines
  (`rtl/crtc_type0_engine.v`, `rtl/crtc_type1_engine.v`); live `CRTC_TYPE` round-trips stay
  pinned by t02j/t06d/t09f/t16l, and bit-identity with the pre-split core is pinned by the
  reproduced lockstep differential run.
- F9 closure is merged into this branch: the documented `t12` worked-example pair — R9 write
  at exact C0==R0 → C4=39/C9=8, and its windowed companion in C0∈[2,R0−1] → C4=38/C9=8
  (ACCC p.82) — is encoded as `t12a`/`t12b` (`aea80b5`, merged via `d5cab8f`).
- F6 Stage 1 implements the presence/type/skew discriminator but uses a full-character DE
  gap (t10a-t10e). Stage 2 measured a 16-mode-2-px (1 µs) seam. Stage 2b visual evidence
  from ACCC pp.186/195 assigns the documented 0.5 µs to a sub-character CRTC DE pulse;
  test/production phase matches and both GA paths agree. Formal finding F13 blocks a
  correction pending SHAKER Module A `(O)` and, if possible, a DE-pin capture.
- F7 RFD is implemented for the type-1 R5 route (`t13a`-`t13d`): same-edge `R5 0→nonzero`
  arming at C0=R0, VMA-from-R12/R13 on every row, parity-gated VMA' saves with odd-R9
  frame-parity alternation, successful-save disarm, and the B6 R1>R0 bare-C9 disarm.
  A1 closes the adjustment-ending VSYNC corner (`t08m`, corrected `t08g`) and A2 implements
  the §11.2.4 exact-C0==R0 caveat pair (`t08n`/`t08o`). The §13.7.1.2 R0-widening trigger —
  the second route — landed 2026-08-23 (`t13e`-`t13m` after cross-provider review): a strictly
  widening R0 write on the C0==R0 edge of the frame's last line defers that line end (wrapper
  `hcc_end`), and a last-line condition cancelled by R9/R4 rewrites before the extended end
  arms the same two flags there. The review's blocking findings are remediated in-branch:
  the vestigial display-end guard is removed (`t13m` pins DE blanking at `C0==R1` on the
  extended line) and both off-last-line precondition halves now have load-bearing vectors
  (`t13j` retimed, `t13l` added). RFD#10's "1-B" variant and the scope notes in
  `audit-findings.md` remain as documented there.

- F10 interlace parity machinery is implemented for the unblocked scope on
  `accuracy/f10-fixtures` (2026-08-24): type-1 two-stage R8-toggle parity update and
  §19.8.2 counting (`t21a`-`t21p`, the 16 pp.210-211 panels), type-0 split C9/C9.VMA with
  the asymmetric entry/exit limit tests and §19.5.2 parity rules (`t22a`-`t22o`, the
  pp.221-224 tables). All 31 vectors are required passes; the old stepping/halving
  approximation is removed and non-IVM behavior is bit-identical. Odd-R9 alternation
  (Q19), the additional interlace line (Q10), and the odd-C4 VSYNC-imbalance correction
  (Q12) remain deliberately unimplemented; residuals are in
  `accuracy/f10-implementation-notes.md`. The stack was independently reviewed 2026-08-25
  (`accuracy/f10-independent-review.md`): NOT CLEAR on two blockings, both fixed with new
  vectors (`t23a`-`t23c`, `t22p`-`t22s`, RA column in `t22`); review-debt row cleared.
  All three remaining non-gated classic items named for this session are done (2026-08-25):
  the F11h closure, the t24 IVM VSYNC fixture family, and the CI-only `actions/checkout`
  bump — see the three bullets below.

- F11h is closed by implementation (2026-08-25, this branch): the p.242 render shows the
  second CRTC-1 chronogram catching an R12 write that lands on the row-0 line-boundary edge
  itself (OFFSET=#30xx from C0=0) where the paired CRTC-0 chronogram keeps the old offset —
  so the §20.3.2 reload samples the post-edge register file. `t20j` (fixture commit XFAIL,
  behavior commit required pass) pins the catch at a mid-row-0 boundary and the frame
  origin; `t20k` pins the type-0 miss. Soak re-minted `0x801a59096c192d26` (chain in
  AGENTS.md). Unpinned residuals are recorded in the F11h entry of `audit-findings.md`.
  Still open from the list above: the t24 IVM VSYNC-gap fixture and the CI-only
  `actions/checkout` bump.

- t24 is closed (2026-08-25, this branch): the p.208 table (with the §19.8.2 p.225
  alternation, which needs an odd C4 count — the fixture uses R4=6) pins the type-1 IVM
  VSYNC start at the first line of C4=R7 on both frame parities, with no delay correction —
  the documented permanent 1-line gap for odd R7 (`t24a`, required pass) and the no-gap
  contrast for even R7 (`t24b`, fixture XFAIL `e0f5b6a`, behavior commit required). The fix:
  `vsync_line_fire` uses the IVM-aware row-structure test, and during type-1 IVM the legacy
  field=1 MID-VSYNC arm no longer hijacks fire or count tick. Soak re-minted
  `0xd620fce8b1c05b25`. The type-0 IVM VSYNC rule (the §19.5.2 delay) stays Q19-gated and
  the f10-implementation-notes residuals are updated accordingly. The CI-only
  `actions/checkout` bump landed as `4e776f1` (v4 → v7, standalone).

- Independent review of the F11h+t24 work (Claude Opus 5 xhigh via the ask-claude bridge,
  fresh session; record `accuracy/f11h-t24-independent-review.md`, review-debt row
  `accuracy/f11h-and-ivm-vsync-coverage`) returned NOT CLEAR on one blocking finding,
  **B-1**: the t24 fix had silently removed the type-1 IVM MID-VSYNC (p.208 schedules it on
  the ParityFrame-even frame) and no vector sampled the half-line phase. Remediated on this
  branch: the wrapper now keys the type-1 IVM VSYNC on ParityFrame directly — even-parity
  frames start and end the pulse at the half-line tick via a seam-latched fire decision
  (`e1_vsync_line_fire` is hcc-independent, so consuming it mid-line needed the latch),
  odd-parity frames keep the seam start/end — pinned by `t24c` (fixture XFAIL `acbc51a`,
  behavior commit required). The reviewer's sandbox could not execute gates or bite-tests;
  the gates and all three of the reviewer's bite-tests were reproduced by the parent
  session against the remediated tip: (a) forcing `vsync_type1_ivm` to 0 (legacy field mux)
  fails exactly `t24b`+`t24c`; (b) reverting the engine row-end test to plain `C9==R9`
  fails exactly `t24a`+`t24b`+`t24c`; (c) deleting the `e1_row0_reload` override fails
  exactly `t20j`; each restore re-greens the suite. Soak re-minted `0x63d9de100ac9f6f2`.
  Non-blockings: N-3/N-4/N-5 fixed; N-1 recorded as a named residual comment at the gate
  (raw R8 mode vs latched engine IVM, 1-2 character window at toggles); N-2 folded into
  action item A4; N-7 covered by synthesis-on-merge. Review-debt row cleared 2026-08-25.

- D1 follow-up, p.81 type-0 adjustment addressing (2026-08-25, this branch): the D1
  correction's RTL premise did not survive verification. The p.81 LINE column is the
  C9-driven segment of the *composed* VRAM address (§20.2 p.241 takes bits 13:11 from
  C9[2:0]; `Amstrad_motherboard.v` forms `{MA[13:12], RA[2:0], MA[9:0]}`), not the CRTC
  module's raw MA port — that port carries the video pointer, which must scan per character
  because the p.83 prose's memorized value is line start + R1. The wrapper already produced
  the documented behavior (C9 counts to R5 at the seam limit, RA=C9 feeds the composition,
  save/restore gives the pointer steps), so Item A of the session brief landed as
  **required-pass pins, not a fix**: `t25a` (period-8 segment cycle, wrap at C9=8, DE-off
  caveat pinned), `t25b` (constant pointer between crossings, single +R1 step at the C9==R9
  crossing, within-line scan), `t25c` (exit resets C4/C9 and reloads R12/R13). Bite-tested
  (mutation: `type0_c0_adjust_line_max`, the C0=0 seam limit, retargeted to R9): fails
  exactly the t25 family plus the nine existing t16/t08k adjustment guards; a broader
  mutation that also retargets the live in-adjustment limit adds t12a to the failure set. Soak unchanged at `0x63d9de100ac9f6f2` (no behavior change —
  the brief's expected re-mint does not apply). Recorded NOT-PINNED boundary: the tables
  normalize PTR-VRAM to 0 at adjustment entry, so the absolute entry pointer value
  (last-row base vs base+R1, i.e. whether the entry line's own capture applies) is not
  source-adjudicated; this core keeps the plain-rule entry capture and t25 asserts only
  source-supported deltas. Full adjudication in `compendium-01-counters.md` §4.1.

- D1 follow-ups session, 2026-08-26 (branch `accuracy/d1-followups`, base `6030b4c`):
  - **Item A** (p.81 adjustment addressing): landed as required-pass pins, not a fix —
    see the dedicated bullet above. Soak unchanged; the brief's expected re-mint does not
    apply.
  - **Item B** (§17.5 R1=0 write deadline, p.185): the RTL already matches the documented
    deadline via the seam-time `hcc_next==R1` check and the R1 write-hit term (`hcc==DI`);
    `t26a` (type 0) / `t26b` (type 1) pin it as required passes, bite-tested (disabling the
    write-hit term fails exactly t26a+t26b). Derived and pinned beyond the chronograms: a
    too-late write still updates the register, so the live `C0=R1` comparison targets 0 and
    the too-late line displays past the old R1's end; R1=0 is honored from the next line.
    Digest §17.5 updated. Soak unchanged.
  - **Item C** (Q19/Q10/Q12 re-adjudication, fresh renders pp.198-199/205-206/216/219-220/
    223-224): Q10 RESOLVED — the additional interlace line is generated at the end of the
    ParityFrame-even frame (type 1 gate ParityFrame even; type 0 gate ParityR6 odd with the
    R6>R4 freeze) and duration-counted in the following odd frame; labelling is
    ParityFrame-relative. Q11 RESOLVED (p.205 states even explicitly). Q12 RESOLVED (the
    source does not assert self-correction; the no-persistent-state reading is recorded as
    inferred). Q19 main token + (a) RESOLVED (`R9.0=0` is a typo for `R9.0=1`; the
    three-phase comparison form is pinned by p.220 and already implemented); Q19(b) STILL
    OPEN, sharpened — the pp.223-224 exit tables imply a frozen-C9.VMA line-end test after a
    non-matching R8=0 write, while this core resumes a live plain `C9==R9` test on
    post-write lines (unpinned divergence, documented in the question). Actionable rules
    opened as finding candidates **F14** (additional interlace line, both types) and
    **F15** (type-0 odd-R9 IVM counting incl. the §19.5.2 VSYNC delay correction) —
    fixtures before any RTL.
  - **Item D**: A4 closed (the `expect_known_*` helpers renamed `expect_xfail_*` with the
    house rule in the comment; review-debt row done); N-6 closed (`actions/upload-artifact`
    v4 → v7, standalone; first run resolves it and uploads green).
  - **CI evidence**: dispatched run `32917100161` (2026-08-26, Quartus 17.0.2) fully green
    — simulation, policy, synthesis, required gate; the v7 artifact upload verified online.
    Fitter: 16,208 / 41,910 ALMs (39 %); 21,112 registers; 701,601 / 5,662,720
    block-memory bits (12 %); 34 / 112 DSP blocks. Worst-case setup slack +0.623 ns, hold
    +0.231 ns — positive; versus the previous milestone `5d6d342` (16,198 ALMs, 21,030
    registers) the delta is tool noise on a comment-only RTL change. RBF retained as
    `output_files/hardware-milestones/Amstrad_20260826_d9abf35.rbf` (SHA-256
    `26f415d4d9d2723c98168539cbec91fd421b7b79da05c12a602f0f9a89dde259`). Not
    hardware-tested.
  - **Soak**: unchanged at `0x63d9de100ac9f6f2` throughout the session — no classic
    behavior change landed (pins and docs only), so no re-mint is due.
  - **Review status**: end-of-session cross-provider review of the branch diff happens
    before merging; outcome recorded in the merge/branch notes.

D1 is complete (2026-08-24): every remaining ⚠ VERIFY flag in the three digests was
re-verified against the PDF (pdf-inspector Markdown primary, figures judged from rendered
pages). Outcomes: most flags retired as confirmed; four genuine digest errors corrected —
p.81 type-0 adjustment addressing is period-8 through 8 distinct addresses (never propagated
to RTL/vectors), §17.5's R1=0 deadline boundary was inverted-ish and is now derived (type
0/1/2 accept writes through C0=0, type 3/4 close two characters earlier), p.183's worked
example uses R1=40/&28 not 64, and the pp.221-224 IVM tables were re-adjudicated after an
initial misread: they use R9=6 (even, per p.220) and corroborate the §19.8.1 pseudocode; the
surviving source-internal conflict is only the p.219 gate token `If R9.0=0` against its own
odd-R9 gloss (author question Q19, narrowed). The re-verification was itself cross-reviewed:
a GPT reviewer-cross pass returned five blockings, two of which contained evidence errors
that an Opus adjudication settled against the renders (both disputes sided with the
re-verification's corrected readings); all other blockings are remediated in this diff. Page
anchors were corrected against the real TOC (§13.2.x, pp.210-211, p.247, §21.4), and the
separate stale-reference sweep over docs/ found ten more fixes; rtl/ and sim/ citations were
all clean. F10 has since been implemented, reviewed, and merged (see the completed-work
section); F13 waits for hardware.

## Completed Plus foundations

- A separate default-off `Plus model` selector decodes GX4000, 6128+, and 464+ capabilities
  without reinterpreting the classic model field or selecting Plus hardware.
- The ASIC lock/unlock state machine is implemented and exhaustively unit-tested as a leaf.
- An atomic 512 KiB cartridge memory service is implemented and tested for clear, load,
  commit, abort/detach/reset, invalid addresses, and CPU reads.
- `sdram.v` now has a held cartridge request/acknowledge client with tested byte lanes,
  addressing, arbitration, back-to-back transfers, classic main/tape writes, and refresh
  fairness. P0 production wiring connects it to the cartridge service when Plus mode owns a
  cartridge window; classic mode leaves it inactive.
- A real service-to-real-SDRAM simulation proves exact clear/load transaction counts,
  publication, and CPU readback without duplicate held requests.
- A bounded, streaming RIFF/CPR parser now validates the `RIFF`/`AMS!` envelope, accepts
  ordered `cbNN` cartridge-bank chunks, handles RIFF padding, streams payload bytes into
  the atomic cartridge service, and fails closed on malformed or aborted downloads.
  Oversized `cbNN` chunks abort instead of truncating (A5 decision,
  `docs/plus/architecture.md` "CPR parser policy (P0)").
- P0 wiring is complete on `plus/p0-parser-wiring`: `plus_mmu` implements the Plus
  cartridge windows (high window from the ROM-select port incl. the GX4000 page-1 rule
  and the /EXP-dependent value-0 rule; low window position/page from unlock-gated RMR2;
  ASIC-page-enable captured but unbacked until P2). `/EXP` is a defined dynamic input,
  tied high at the top level for P0 (= no expansion connected). The cartridge memory
  service is production-connected to the reserved SDRAM port, and Z80 reads in cartridge
  windows are bridged to the service with CPU WAIT insertion. The watchdog pauses while the
  cartridge service is clearing/loading, and retains fail-open behavior only for a quiescent
  backend that does not respond.
  The CPR stream is live on ioctl index 8 (OSD "F8,CPR"), and a P0 boot integration bench
  runs parser + service + real SDRAM end to end, including reset-mid-load cleanup.
- Dandanator uploads are bounded below the Plus cartridge reservation: bank 3
  `0x000000..0x07ffff` remains Dandanator, while `0x080000..0x0fffff` is reserved for Plus.
- Plus P1 counter/timing foundation is implemented on `plus/p1-crtc3-foundation`:
  `rtl/plus/asic_video.v` carries the type-3 register file, C0/C9/C4 counters with the
  type-3 R9-forced-reset and R4-overflow rules (ACCC §10.3.4/§12.5), R5 vertical
  adjustment that freezes C4 at R4 (§11.2.6/§11.3.3), the two-stage video pointer with
  the C4=0 ∧ C0=0 reload condition (§20.3.4), DE with line-start-only R6 semantics
  (§18.2.4) and SKEW-DISPTMG (§19.2), and HSYNC/VSYNC generation including bounded
  R3=0 widths and the live §15.3 end/start collision. 28 deterministic vectors
  (t01a-t04i) cover them, including the p.151 live-R2 chronogram; every sourced rule cites
  its ACCC section at the point of implementation. `t03c` also pins the simultaneous C0=R1=R0
  row-end save/reload so MA
  advances to the captured row base rather than restoring stale VMA'. Interlace is
  stored-but-inert; the status registers/read map are implemented by P5 below.
  Follow-up vector `t04i` makes the R3l=0 end/start-collision exception explicit: the
  current model keeps the documented 16-character pulse bounded, but that boundary is an
  unverified model assumption pending a direct rule, Logon observation, or hardware capture.
- Still open in P1 before the milestone is complete: the CPU/WAIT
  timing-contract decision that lands with the first motherboard instantiation
  of `asic_video` (architecture §5 Risk 1), plus the intra-character pixel
  phase validation against the ga40010 cadence named below. `files.qip` is
  untouched until that instantiation commit. The locked-ASIC title boot-point
  check remains the manual P0/P1 hardware checkpoint described above.
- P1 remainder, locked-ASIC pixel path, is implemented on
  `plus/p1-pixel-path` as a leaf extension of `asic_video`: a pen pipeline
  decoding two video bytes per CRTC character (MA is word-addressed;
  ga40010 latches VIDEO_BUF twice per character) at the documented
  mode-dependent rates, border substitution outside DE, HSYNC forced blank,
  screen mode latched on HSYNC assertion, and a registered 32-entry
  legacy-colour ROM translating hardware colour numbers to 4-bit-per-channel
  RGB. Sources: [KT] palette table (web.archive.org capture 20230923001014),
  cross-checked entry by entry during extraction against this repo's own
  ga40010 DAC equations — all 32 agree; byte/pixel layouts from the Grimware
  Gate Array page (already cited by color_mix.sv), corroborated by the
  netlist cidx taps ({r1,r5,r3,r7}/{r3,r7}/{r7}). Vectors t05a-t05h pin the
  ROM sweep, all four mode layouts, border/sync blanking, the
  after-next-HSYNC mode latch, and the per-character byte-latch phase. Explicit unverified P1 model assumption
  (t04i discipline): the first pixel of a character's even byte is presented
  on dot 0 with one-dot registered output latency; the real GA's pipeline
  latencies relative to its load/DISPEN cadence (Plus INKR effects ~1/4
  character late, 40010's one-pixel mode-2 early start) are deferred to the
  motherboard-integration differential check. Interlace stays stored-but-
  inert; the status registers/read map are implemented by P5 below.
- CI evidence for the pre-rebase P1 foundation branch (run `32632492492`, original tip
  `0be8a60`):
  simulation and synthesis both green. Fitter: 15,295 / 41,910 ALMs (36%), 685,217
  block-memory bits (12%), 3 / 6 PLLs; worst setup slack +0.342 ns — numerically
  identical to the P0 merged-tip build, and `asic_video` appears nowhere in the fit
  report because nothing instantiates it yet. No regression signal; the first
  meaningful synthesis delta arrives with the P1-remainder motherboard integration.
- ASIC register page backing, palette, interrupts, sprites, split/scroll,
  and DMA are not implemented. FDC/tape presence gating for GX4000/464+ is also still
  inert (P8 polish scope).

The next Plus milestone work is the P1 motherboard-integration commit per
`docs/plus/architecture.md` §4/§5/§7: instantiate `asic_video` (deciding the
CPU/WAIT timing contract, Risk 1: replicate ga40010 timing behaviorally vs
keep it as clock generator), wire the video word and legacy GA-config inputs,
add `files.qip`, record fitter utilisation, and validate the intra-character
pixel phase assumption against the production cadence; then P2. The P0
hardware checkpoint is manual:
with a Plus model selected, load a real `.cpr` (e.g. the local untracked `crtc3_v2fix.cpr`
fixture) and confirm the firmware/game reaches its first screen; classic mode must be
re-checked side by side in the same session. Do not start Plus video by extending
`ga40010`; the planned path is the parallel behavioral `asic_video` module.

### P1 motherboard integration — landed on `plus/p1-motherboard-integration` (2026-08-24)

Risk 1 was decided in favour of option (a): `rtl/plus/asic_ga_timing.v`
reproduces the ga40010 timing contract behaviourally (sequencer, CCLK/PHI/
READY/RAS/CAS/CPU_N, CAS refresh masking, monitor sync shaping, 52-line
interrupt counter, legacy GA register file). Cycle-exact equivalence with the
synthesised ga40010 composition is pinned by
`sim/plus/asic_ga_timing_diff_tests`, which compiles the reference with
`-UVERILATOR` (its simulation-only shadow domain double-drives the sync
outputs under plain Verilator) and drives both with identical randomised
bus/reset/sync traffic; register payloads ga40010 does not export are pinned
by directed vectors r01-r03. Deliberate deltas are documented in the module
header (no SNA preload — no Plus snapshots; defined INKR power-up values,
named unverified assumption).

`Amstrad_motherboard.v` instantiates both Plus subsystems unconditionally and
muxes at the consumption points (house style); classic mode is untouched
(soak re-verified). ga40010 stays the ROM-enable source in both modes because
its register decode watches the same bus; plus_mmu overlays cartridge
windows. The motherboard assembles VIDEOD on the reference VIDEO_BUF latch
phases (e0 → even byte, 03 → odd byte). RGB reaches the existing 2-bit+OE
path through a temporary lossless adapter for the legacy {0,6,15} levels;
true 4-bit widening is P2's first commit. `files.qip` gained both new files.

Open P1 follow-up, tracked here so it is not lost: `sim/plus/p1_video_tests`
(`make -C sim/plus p1-video-bench`) hosts an integration bench with a classic
CRTC+ga40010 oracle slice intended to close the t05h pixel-phase note by
requiring the Plus PEN stream to match the classic pipeline byte-for-byte.
Its stimulus/sampling calibration is unfinished (the first run showed a
slot-grid/border-sampling mismatch that behaved like a bench-side phase
error, not a production defect), so it builds but sits outside the default
gate until calibrated; the t05h caveat therefore remains open.

**Update 2026-08-25: calibrated and moved into the default gate** (commit
`cee64cd`). Three bench-side defects explained every earlier symptom — the
RTL was correct throughout: a wrong C++ mirror of the fake-VRAM tag pattern,
a CLKEN level probe landing one cen_16 edge early, and tag-pattern aliasing
across 128-byte boundaries (video base moved to &0080). Final coverage: p1a
pins the word-granular pointer stream with dots 0-7 = even byte / dots 8-15
= odd byte through the production VIDBUF assembly (t05h assumption closed at
integration level), tolerating the mixed word across each line-start MA
reload; p1b pins border-flag interiors only — the PEN flag's de_hold capture
skews the colour-class switch at region boundaries by up to one character,
which is the documented GA pipeline latency question and stays deliberately
open with the motherboard timing contract; p1c checks classic VIDEO_BUF
provenance over the same window (byte order is pinned by p1a; netlist buffer
latency is GADIFF territory). Bite-tested against an assembly-order swap.

An Opus-5-high independent review of this P1 delta returned NOT CLEAR on
2026-08-24 with two BLOCKING findings, both confirmed real and fixed in the
same pass: the asic_ga_timing bus pins were wired to uppercase implicit nets
(`MREQ_N` etc.) that synthesis tied to constants — dead GA-register decode,
stuck irqack, no Plus interrupts — and `plus_vidword`'s reset arm had
inverted polarity (active-high `reset`). The review also corrected the
VIDBUF comment (byte order is assumed pending p1_video calibration, not
validated) and flagged that the soak scopes only to `rtl/CRTC.v`, so
'classic untouched' claims must cite the mux inspection, not the soak.
Post-fix CI (run `32777625616`, both blocking fixes in) is fully green:
fitter 15,716 / 41,910 ALMs (37 %), 19,966 registers, worst setup +0.519 ns,
hold +0.252 ns. The +187-ALM delta versus the pruned 15,529 build confirms
the reviewer's constant-bus inference; the earlier paragraph's figures are
superseded by these.

Queued from the review: a Verilator lint pass over Amstrad_motherboard.v
(would have caught finding 1; needs stub modules for YM2149/hid), an explicit
decision on the dead plus_phi_en_* wires vs driving T80pa/crt_filter from
the ASIC enables in Plus mode, re-measuring the fitter delta after these
fixes (previous numbers were taken on the constant-bus-pruned build),
directed U204-restart and randomised-fast lockstep coverage, implementing or
re-documenting the INKR power-up constants so r03 pins RTL rather than
Verilator zero-init, and a minimal plus_mode=1 motherboard bench before P2.

**All queued items implemented 2026-08-25 on `plus/p2-asic-regs`** (commits
`5730b66`, `ea69d68`, `e9f2bca`; CI evidence below):

- `make -C sim/plus motherboard-lint` (in the default lint chain) elaborates
  the whole motherboard hierarchy under `--language 1364-2001 -UVERILATOR`
  with full-port-list stubs for T80pa (VHDL), ga40010/YM2149/hid
  (SystemVerilog; the `.do(` pin name rules out default-SV mode). IMPLICIT
  and UNDRIVEN stay fatal — finding 1's exact bug class. Waived classes are
  triaged and documented in `sim/plus/Makefile`.
- The plus_phi_en_* decision: T80pa, crt_filter CE and the expansion phi
  pins now take ASIC enables under plus_mode via explicit ownership muxes.
  Cycle-neutral today (GADIFF-proven equivalence); makes asic_ga_timing the
  Plus owner so deliberate deltas land everywhere at once later.
- Differential bench: d04 drives an intack bus state across reset (U204's
  reset term — previously uncovered); d01 randomises the no-wait input
  inside lockstep traffic. r03 now proves INKR/ink-select power-up clears
  are explicit RTL resets in `asic_ga_timing.v` (bite-tested), not simulator
  zero-init; real ASIC power-up contents remain a named assumption.
- Motherboard bench m1-m4 (`make -C sim` runs it): plus_mode=1 boot with a
  scripted fake Z80; GA RMR/INKR/border writes reach asic_video through the
  production muxes; the 52-line interrupt fires into the CPU pin and clears
  on acknowledge. Uses --public-flat-rw taps; ga40010/YM2149/hid join as
  stubs (same language constraint).

The gate after these changes reports 256 PASS lines (147 CRTC vectors, all
Plus leaf/integration suites including p1_video and mobo benches); lint green
including the new hierarchy pass; soak unchanged at `0xa9e5026de83d287c`.

CI synthesis of the instantiation is green on the dispatched exact build
(run `32771020608` — simulation, policy, Quartus
17.0.2 compile/fitter/TimeQuest all pass; NOTE fitter figures below were
measured before the two blocking fixes landed and must be re-recorded).
Fitter: 15,529 / 41,910 ALMs
(37 %), 20,483 registers, 145 / 314 pins (46 %), 685,217 block-memory bits
(12 %), 34 / 112 DSP blocks; worst-case setup slack +0.410 ns, hold
+0.246 ns. Versus the pre-integration milestone `de30faf` (15,378 ALMs,
20,168 registers, +0.581/+0.246 ns) the ~150-ALM / ~300-register growth is
the two Plus subsystems entering the fit for the first time (`asic_video`
was previously uninstantiated); the setup-slack shift stays comfortably
positive — no regression signal. Three integration defects were caught by
this CI loop and fixed en route: `plus_vidword` wire-vs-reg (Verilator
tolerated it, Quartus did not), a double drive of `plus_gamode` from both
`MODE` and `GAMODE_O` aliases, and two stale lint/policy expectations
(`-UVERILATOR` on the wrapper lint line; `asic_video.v` now legitimately on
the synthesized manifest).

Plus P0 wiring is merged onto `accc-review-and-fixes` (merge `daf1d6f`) and has a green
GitHub Actions build (simulation + synthesis) on the merged tip. Fitter: 15,295 / 41,910
ALMs (36%), 685,217 block-memory bits (12%), 3 / 6 PLLs; worst setup slack +0.342 ns,
worst hold slack +0.244 ns (TimeQuest still reports the repo's unconstrained external I/O
paths, so internal slacks are not full closure). Versus the pre-P0 build (`4c78603`:
14,947 ALMs, +0.516/+0.246 ns), the ~350-ALM growth and small setup-slack shift match the
added cartridge decode/bridge logic; no regression signal. It has not been hardware-tested.

### P2 ASIC register page — landed on `plus/p2-asic-regs` (2026-08-25)

`rtl/plus/asic_regs.v` backs the &4000-&7FFF page per `asic-reference.md`
§2-§6: the 4K×4 sprite pixel RAM with its low-nibble mask, sprite X/Y/mag
storage with the documented read rules (&FF for all-ones high bytes) and
+4..+7 read mirrors, the 32×12 palette in the documented {G,R,B} word
layout with split-byte writes and a free-running video port, legacy
PENR/INKR translation into entries 0-16 through the [KT] table, PRI/SPLT/
SSA/SSCR/IVR and DMA SAR/PPR byte storage for later phases, DCSR readable
across &6C00-&6C0F but writable only at &6C0F, and wired-AND-neutral open
bus over every unmapped/write-only region. Seven exhaustive vector groups
(a01-a07) run in the gate.

Integration: `Amstrad_motherboard` instantiates it (chip-select from the new
`plus_aspage_on`, legacy GA shadow straight from `asic_ga_timing`); `Amstrad.sv`
captures `plus_mmu`'s RMR2 page-enable and suppresses main-memory read AND
write cycles across the whole window while it is on (no read/write-through,
reference §2, cartridge-owned-cycle pattern), with an answering page read
taking priority on the CPU data bus. The motherboard bench gained cycle-type
awareness (I/O vs memory, like a real Z80's pin behaviour) and m5: scripted
page writes land in sprite RAM, sprite registers and palette with correct
masks/layout; unused-region writes are ignored. Byte order through the
production VIDBUF assembly — the t05h caveat — was closed by the calibrated
p1_video bench (p1a).

RGB widening (P2's second focused commit): motherboard red/green/blue ports
are now 4-bit. Plus mode carries ASIC palette nibbles natively to a new
expansion stage before the video mixer; classic mode keeps the netlist
{level, OE_N} pair unchanged in the low two bits feeding color_mix exactly
as before, so classic video output is bit-identical by construction. The P1
lvl4_to_ga lossy adapter is gone.

Open P2 items: the phase exit "static Plus palettes display correctly
(Burnin' Rubber title)" needs the manual hardware checkpoint (this RBF plus
a real .cpr). The magnification write-mirror on offset +3 remains the ⚠
ASIC-REF §4 conflict note (+3 stores Y-high here pending hardware
verification). ADC/DMA behaviour stays unmapped-rule until their phases.
No-write-through into real SDRAM is enforced by the suppression terms and
verified by construction/mux inspection; no bench drives the full top-level
memory path yet.

The branch was merged into `accc-review-and-fixes` on 2026-08-25 at merge
commit `0dabcb8`; see the open review-debt row for the outstanding second
pass over the post-review fix delta and the skipped-vector residual.

### P4 sprites — engine implemented on `plus/p4-sprites` (2026-08-25)

`rtl/plus/asic_sprites.v` implements the [KT] coordinate model literally
(asic-reference §5): vertical compare on `{LINE, ROW&7}` (not gated by R6),
horizontal window on a free-running 10-bit dot counter cleared at the CRTC
character wrap — so the documented "R0>64 repeats horizontally" falls out of
the scale wrapping at exactly 64 characters — magnification codes
01/10/11, transparency at nibble zero, lowest-index priority, colour c ->
palette entry 16+c. Rows stage in DUAL banks: at each line seam a
row-tag-matched inactive bank is promoted (zero-latency swap) while the
background walker speculatively fills it with predicted row+1; mismatches
(Y/mag rewrites, first frame) fall back to urgent refill. X rewrites cut
the live window via a shadow; CPU pixel-data accesses blank that sprite only
and flush its staged banks through `asic_regs`' new access indicator.
`asic_video` gained HWRAP plus the final border > sprite > screen mux under
HSYNC force-blank; the motherboard instantiates the engine; `files.qip`
carries it with instantiation.

Vectors: s01-s10 PASS (disabled codes, placement/transparency, Y-formula
masking incl. the ROW&7 pin, x/y/quad magnification bounds and row
duplication, priority chain + 16-stack, palette mapping/order, X extremes +
wrap-through + negative alias, R0>64 repeat). asic_video t06a-c pin the
precedence mux; asic_regs a09/a10 pin the fetch-port handshake/preemption
and access-indicator decode.

**VECTOR STATUS (all 14 green, 2026-08-25):** s01-s14 all pass with zero
skips; the runner counts skips separately and exits 65 if any vector is
skipped, so gates cannot silently accept a disabled vector.

Two independent review passes shaped this phase. Pass 1 (Codex/GPT-5.6
Sol high, NOT CLEAR) found: suppressed port completions stranded sreq
bits forever (fixed — the handshake always completes; only the payload
write is conditional, scoped to words whose sprite matches ACC_IDX);
the disabled-sprite block jump advanced walk[7:4], crossing bank+sprite
and starving odd sprites; and an out-of-window s11 assertion.

Pass 2 (same route, also NOT CLEAR on the first remediation delta)
found the deeper truth and closed the phase: (a) the pass-1 walk-jump fix
had overcorrected — preserving walk[7] trapped the walker inside one
bank half whenever sprite 15 was disabled, which is the common case;
the skip now advances the {bank,sprite} block number with carry across
the half boundary. (b) The "exit 65" skip accounting existed only in
prose; it is now real code. (c) The long-standing "post-flush cross-seam
refill incompleteness" residual was never an RTL defect at all: s11 had
been configuring its target sprite with mag code 0x5 = X1/Y1, whose
window is ONE character, so the char5 recovery assertions sampled where
the sprite correctly never appears. With mag 0xA (X2/Y2) and correct
per-char expectations, plus the s12 cut bound moved to dot 6 (the
rewrite lands after dot 5 is sampled), every vector passes honestly.

One measured behaviour is pinned as a documented model choice rather
than an S5 rule: a pixel-data access flush landing mid-walker-lap leaves
the accessed sprite invisible for up to roughly four further characters,
because the single continuous fetch server finishes its current sweep
(including speculative work) before revisiting the sprite's active
block; recovery is complete and byte-correct by the same source row's
window on the next display line (s11 pins exactly this). Reference §5
fixes only THAT-sprite-only scope and image integrity, not hole shape.
Future optimisation if hardware ever needs it: urgent-first scheduling
of active-bank misses ahead of the speculative sweep.

The review thread ran five passes total and closed CLEAR. Pass 3
confirmed the walker block-carry sound but exposed a pre-existing race:
an issue on the seam edge captured pre-edge bank state while seam
maintenance retagged it, so a delayed ACK could land row-N data into a
bank retagged N+1. Fixed by completion-time validation: each request
carries its source row and is accepted only while the target bank still
holds that tag. Pass 4 cleared that mechanism but found its own pair: a
pixel-data WRITE during an in-flight fetch to the same sprite returned
the pre-write byte after ACC_EN dropped (the port serves grants through
CPU writes), fixed by poisoning the request for its whole life via
fq_acc; plus an s11 oracle bug (source row under Y2 is (12-8)>>1 = 2,
not 4) and an s12 early-arm coverage gap. Pass 5 reviewed exactly that
delta and returned CLEAR with no findings, clearing the review-debt
row.

mobo bench m8 (2026-08-26, merged `2a221e1`) closed the phase's last
build item: the first end-to-end sprite vector through the production
chain. The scripted fake CPU programs sprite 0 over the ASIC page
(X=0x166 via the x_hi byte, Y=16, MAG x1/x1), a bench-CPU auto-fill
phase writes the whole 16x16 image with the low-nibble mask exercised
on every write, and pal[21]/pal[26] carry distinct payloads. The scan
derives expectations on paper — [KT] compare formulas, reference S5/S6,
the engine's {G,R,B}->{R,G,B} emission swap, and asic_video's
registered RGB output lagging the engine plane by one dot — and
requires exactly three 16-dot SPR_EN windows on each compare line
16..31 per frame (and nowhere else) with alternating palette payloads
on the top-level RGB pins. Two review passes over the scan harness
ended CLEAR. The finding en route was fixture-side, not RTL: rows 1-15
unwritten meant transparent pixels, and the engine had been correct all
along.

Still open from this phase: the INKR-effects ~1/2-us-late GA pipeline
question noted in P1 remains deferred; P4 hardware checks land with the
next board milestone.

The synthesis-cost audit's Plus-track finding was probed 2026-08-26 and
resolved as accepted cost: the decode-split walker increment
(`c09534c`, review CLEAR, cycle-exact) did not lift the fitter cliff —
fit stayed at 15:40 in the post-cliff band — falsifying the carry-chain
hypothesis. The cost tracks the walk half bit gaining any next-state
logic; the pipelining alternative is rejected on vector-pinned-latency
risk. Full record in `docs/plans/2026-08-26-synthesis-cost-audit.md`
(remediation-outcome section).

Milestone CI (workflow_dispatch run `32892544906`, Quartus 17.0.2,
commit `e3dd848`): simulation, policy, exact synthesis all green.
Fitter: 29,893 / 41,910 ALMs (71 %); 37,506 registers; 685,217
block-memory bits (12 %); 34 / 112 DSP blocks; worst-case setup slack
+0.436 ns, hold +0.179 ns — positive; no regression signal. Versus the
P2/P3 merge milestone (`5d6d342`: 16,198 ALMs / 39 %) the ~13,700-ALM
growth is the sprite staging arrays (dual 8-byte banks x 16 sprites with
request/delivery bitmaps) plus the engine datapath landing in fabric for
the first time; memory bits unchanged (staging is flop-based by design).
RBF retained as artifact `Amstrad-build-104-1`
(`Amstrad_20260825_e3dd848.rbf`). Not hardware-tested.

### P5 CRTC-3 bus semantics — implemented on `plus/p5-crtc3-bus` (2026-08-26)

Two code commits close the deterministic P5 scope. `3891213` implements the
ACCC v1.10 §21.2.3 modulo-8 map
`{R16,R17,STATUS1,STATUS2,R12,R13,R14,R15}`, full-byte R12 storage/readback,
stored R14/R15, and the §21.3.4 live status groups. `8523136` connects the
Plus CRTC output to the CPU wired-AND path only under `plus_mode`, keeps the
live CPU byte on CRTC DI during IN cycles, and propagates read transactions to
the unlock/RMR2 observers. The MMU converts held Z80 I/O levels to one strobe
per transaction before those one-shot side effects.

Vectors t07a-t07g cover the map, storage widths, both `&BE`/`&BF` read ports,
horizontal/vertical status boundaries, VMA preview and the 16-frame timer.
MMU tests hold read/write cycles for four system clocks and prove a read byte
inside the unlock stream plus an IN-carried RMR2 payload. Motherboard m9 proves
the production mux and CRTC data/select plus GA traps; m7 proves classic mode
retains its existing type-0 `&BE`/`&BF` behavior. Full simulation and lint pass;
classic soak remains `0x85b3f8e847430495`. Hardware SHAKER/CRTC3 evidence and
the shared Plus title checkpoint remain pending.

### P3 interrupts — implemented on `plus/p2-asic-regs` (2026-08-25)

The programmable raster interrupt lives inside `asic_ga_timing`, where the
classic 52-line counter it modulates lives: with PRI=0 the new block is
inert and the GADIFF lockstep equivalence is untouched; with PRI!=0 the
counter keeps running but its assertion is suppressed, and an interrupt
fires at the trailing edge of the shaped monitor HSYNC when
{VC5..VC0,RC2..RC0}=={0,PRI}. The comparison's bit-8 don't-care produces
the documented n / n+256 aliasing. Vertical adjustment gates firing. A
raster fire pokes counter bit 5 (as an acknowledge would), so a later
re-enabled CPC-compatible interrupt cannot occur within 32 lines.
Clearing is shared with the classic path: CPU acknowledge or MRER bit 4.

DCSR became field-wise: bit 7 is a merger-driven read-only level ("last
INT ack was raster"), bits 6:4 are write-1-to-clear DMA flag storage
(set-paths arrive with P7's INT instruction), bits 2:0 are plain R/W
enables. IVR/vector supply: on every INT acknowledge the ASIC drives
(IVR & &F8) | source; with DMA absent the source field is raster (%110)
while a raster interrupt pends, else 0 — the no-pending behaviour is
unspecified on hardware (named assumption). The motherboard detects the
acknowledge cycle and Amstrad.sv gives the vector top priority on the
CPU data mux. The A13 vectored-interrupt bug stays deliberately not
emulated (architecture §5.4 decision).

Vectors: `pr01`–`pr04` (exact 52-line cadence at PRI=0; suppression plus
aliased fires at identical intra-line offsets; adjustment gate; MRER
clearing a pending PRI interrupt), `a08` (DCSR bit 7 mirrors the merger
level), mobo bench `m6` (ack-cycle vector byte 0xDE after a scripted
IVR write). P3's remaining exit item is title-level stability (Pang,
RoboCop 2) at the next hardware checkpoint.

Open scope note: the monitor-trailing-edge trigger uses this model's
fixed four-character shaping microsequence, so [ARNOLD-REV]'s "clamp at
HSYNC_start+6µs" is covered by construction here; [KT]'s conflicting
"~10µs" measurement stays recorded as ⚠ ASIC-REF §7.

Independent review (two passes, 2026-08-25, record in
`docs/plus/p2p3-independent-review.md`): Claude Opus 5 xhigh on
invariance/PRI/seams returned five blockers — all real, headline being
undeclared top-level wires that corrupted every ASIC-page read while
Quartus warning 10236 sat inside green synthesis, and an intack-polarity/
sampling pair that inverted DCSR bit 7 and collapsed the vector source.
GPT-5.6 Sol high on asic_regs conformance returned two blockers: a reset-
dominance regression in the page-write branch and unobservable w1c flags
(now settable via the new dma_int_set lines ahead of P7). Everything is
remediated at the tip with new vectors pr05, strengthened m6/m7 and
extended a02-a06; both passes' residual items are recorded in the review
document.

Tooling lesson from this branch's CI runs: CI's Verilator is **5.020** while
local is 5.050 — three deltas bit us and were fixed version-portably:
unknown `-Wno-<name>` flags and unknown lint metacomments are hard errors on
5.020 (keep waivers out of both; fix sources instead), function-call
bit-selects are SystemVerilog-only under 1364-2001, an else-wrapped indexed
part-select write tripped a V3Gate internal error, and mixed blocked/
nonblocked assignment is fatal. Write new RTL/benches against the older
front end.

## Build and tooling state

- `.github/workflows/build.yml` runs local-style Verilator tests/lint before a pinned Quartus
  17.0.2 synthesis job and uploads the RBF plus fitter/timing reports.
- Since 2026-08-26 that synthesis job has two effort tiers: routine default/integration-branch
  pushes compile at smoke fitter effort (`scripts/ci/apply-quartus-effort.sh` appends
  FAST FIT / physical-synthesis-off overrides; a log guard fails the leg on any
  `Ignored assignment:`), while PRs, tags, and manual dispatches keep full effort and are the
  only hardware-build evidence. Simulation and synthesis run in parallel behind one gate.
  Trigger rules, measured figures, and the dispatch `both` benchmark mode are documented in
  `docs/ci-testing-policy.md`; the cost audit that motivated it (the P4 pass-2 fitter cliff)
  is `plans/2026-08-26-synthesis-cost-audit.md`.
- `actions/checkout` was bumped v4 → v7 (2026-08-25, standalone CI-only commit `4e776f1`)
  to clear the per-run Node 20 deprecation warning; the first CI run on this branch is the
  online check that v7 resolves. `actions/upload-artifact@v4` also sits on the deprecated
  runtime (review N-6) and is the next standalone CI-only bump.
- `ansible/` provisions the Debian 13 arm64 UTM guest (reached as `quartus-vm.local` over
  mDNS), restores the build user's supplementary groups, mounts Rosetta, registers amd64
  binfmt, and validates
  a real amd64 binary. It also creates a private installer staging directory and provides a
  read-only checksum preflight for the exact Altera 17.0.2 payloads. Quartus itself still
  requires a human download and interactive EULA step; then run `ansible/post-install.yml`
  and `ansible/validate.yml -e quartus_required=true`.
- ACCC v1.10 is now the primary documentation baseline. The checked-in digests and
  `accuracy/accc-1.10-differences.md` capture its rules and the edition delta; consult the
  full PDF only when a page is specifically flagged for re-extraction.
- The local `docs/ACCC1.10-EN.pdf` is user-owned source material and must remain outside
  commits. If v1.9 is retained locally for edition-delta or historical-citation checks, it
  must likewise stay untracked.
- The local example cartridge `docs/plus/references/cartridges/crtc3_v2fix.cpr` is likewise
  deliberately untracked. Use it as a real RIFF/CPR parsing fixture when P0 starts; do not
  make it a build dependency or redistribute it from this repository.
- The complete deterministic F12/F4 counter milestone and CPR parser have a synthesized CI
  build at `5ddddef`, which was hardware-tested on 2026-08-19 as described above. The F8 work
  on top of it has a synthesized, not yet hardware-tested, CI build at `4c78603`.
- Independent review: the six per-commit `review-debt.md` rows were repaid on 2026-08-22 by
  same-model independent review under the 2026-08-22 locked decision (cross-provider review
  unavailable on this harness); findings became action items A1-A5. The whole-diff review of
  this branch plus the type-split branch then ran on 2026-08-23
  (`accuracy/accc-review-and-fixes-independent-review.md`): split RTL accepted as sound; its
  blocking findings (broken GA40010 co-sim manifest, stale handoff/F6 premises) and
  non-blocking ones (soak claim bounds, sweep leftovers) are addressed on this branch. New
  work here still deliberately takes no per-commit debt rows — stream branches cut from this
  tip rebase onto these fixes.

## Next-session order

1. Quartus VM post-install is a future-session task and must only run after explicit user
   authorization. Until then, keep synthesis on GitHub Actions. When authorized, run
   the four-command post-install sequence in `ansible/README.md` from the `ansible/`
   directory: check, apply, repeat the check, then validate with
   `quartus_required=true`.
2. Re-run SHAKER on `4c78603` with Plus model disabled, recording every subtest by name and
   result for both CRTC selections, alongside the stock core — but judged against the Logon
   System reference photographs, not against the stock core (`shaker-module-a-map.md`; the
   stock core is a regression baseline only). Suggested target list for this session, drawn
   from `accuracy/shaker/shaker-accc-crossref.md` (confirm each cited page before acting on
   a result):
   - Module E `(3)` (CRTC 0 C4/C9 counter logic — the F12/F4 work),
   - Module E `(2)` and `(1)`, Module B `(RETURN) R5 STORIES`, Module D `(E)` (F8 type-1
     C5 adjustment),
   - Module A `(U)` and `(P)` (counter/border edges moved by F12/F4),
   - Module A `(5)`/`(6)`/`(7)` R13 UPDATE IN n USEC SCREENS (mechanism vectors `t20a`-
     `t20h` now exist locally, so a divergence here maps straight to code),
   - Module C `(1)` and Module D `(9)` (RFD — both trigger routes are now implemented,
     `t13a`-`t13m`; these are also the tests that discriminate author question 18, the
     end-state vs write-event reading of the R0-widening cancellation).
   The first two passes produced only an aggregate impression and are not actionable.
   Confirm SHAKER's own CRTC identification agrees with the OSD selection before comparing.
3. Map each persistent SHAKER difference to an implemented finding or a named gap; add a
   deterministic regression before repairing any newly understood behavior.
4. Classic: F6 Stage 2/2b is complete per `accuracy/f6-decision-gate.md`; F13 records the
   half-character CRTC-side phase mismatch and is BLOCKED-PENDING-HARDWARE-EVIDENCE.
   F7 RFD (R5 route, B6 disarm, A1, A2) is implemented and independently reviewed
   (`accuracy/f7-plus-followups-independent-review.md`), and the §13.7.1.2 R0-widening
   trigger is implemented with its blocking review findings remediated
   (`accuracy/f7-r0-widening-independent-review.md`, vectors `t13e`-`t13m`). That branch
   passed its pass-2 cross-provider re-review on 2026-08-24 and is merged at `27078f4`, so
   F7 is complete in full. **D1 completed 2026-08-24** (digest re-verification + stale-
   reference sweep; outcomes summarized above and in the digests' 2026-08-24 notes; new
   author question Q19). **F10 is implemented, reviewed, and merged** (2026-08-25; the
   fixture-gating PDF re-checks — pp.210-211 truth tables render-verified; the pp.221-224
   IVM tables corroborating the pseudocode for the tested even R9 — fed fixtures first, then
   per-type behavior commits). Remaining F10 work is Q-gated: odd-R9 parity-alternation
   expectations wait on Q19, the additional interlace line on Q10, and the odd-C4
   VSYNC-imbalance correction on Q12.
 5. Plus: P0, both P1 milestones, the P1 motherboard integration with its review
    follow-ups, the calibrated p1_video bench, the P2 ASIC register page, and P3
    interrupts (PRI/DCSR/IVR) are done on `plus/p2-asic-regs`, synthesized green at
    `3d7a178`. Next Plus steps: the manual hardware checkpoint (real `.cpr` boot,
    a static-palette title for P2's exit, a raster-split title for P3's; classic
    re-checked side by side). P4 sprites are functionally complete on
    `plus/p4-sprites` (engine, s01-s14, t06a-c/a09/a10, mobo bench m8
    end-to-end sprite vector; all review threads closed CLEAR) — remaining
    items are the deferred INKR pipeline question and the shared hardware
    checkpoint above. P5 CRTC-3 bus semantics is complete in simulation on
    `plus/p5-crtc3-bus` (t07a-t07g, MMU held-cycle traps, motherboard m9,
    classic m7). Next implementation order: resolve the INKR-effects latency,
    reconcile VIDBUF/vram sampling, then decide whether to enter optional P6
    split/scroll before the shared hardware checkpoint.
6. Update this file when either stream reaches its next hardware-testable checkpoint.
