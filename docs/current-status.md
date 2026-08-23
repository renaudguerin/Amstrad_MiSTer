# Current implementation status

This is the handoff for the next development and hardware-test session. Hardware facts below
describe `codex/exploratory-gx4000-plus-plan` as of 2026-08-19. Since then `accc-review-and-fixes`
has absorbed the ACCC v1.10 faithfulness review and corrections B1-B13, review-debt repayment,
the randomized equivalence-soak harness, F9 closure (`t12a`/`t12b`), and the per-type engine
split (wrapper `rtl/CRTC.v` + two engines, renamed from `rtl/UM6845R.v`) with no behaviour
change (soak golden-hash pinned). The whole-branch independent review is recorded in
`accuracy/accc-review-and-fixes-independent-review.md`; its documentation findings are fixed on
this branch. The detailed behavioral rules remain in
`accuracy/`; the long-term ordering remains in `implementation-roadmap.md`.

## How hardware testing fits the loop

SHAKER is **not** part of the automated loop. The automated loop is the Verilator suite
(`make -C sim`) plus GitHub Actions synthesis. SHAKER sessions are manual, user-run, and
happen only at significant milestones, against a named target list recorded before the
session. A green simulation gate is never evidence of hardware accuracy; a manual session
never gates a commit.

## Hardware-test milestone

`4c78603` is the newest synthesized milestone and the first to carry F8; it has not been
hardware-tested yet. `5ddddef` is the newest hardware-*tested* milestone, covering the
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
predates F8 and cannot produce evidence for it. Everything merged after `4c78603` (soak
harness, F9 vectors, the per-type split and rename) provably changed no CRTC behaviour — the
soak reproduces golden hash `0x5b5004ff70148443` minted from the unsplit core, and a ~45.5M-sample
lockstep differential comparison against the pre-split core found no divergence — so the
current-tip CI build is equally valid for that session.

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
  leading hypothesis is that Module A leans on behaviour that is still unimplemented — F7 RFD,
  F10 interlace parity, and F13's hardware-blocked half-character F6 seam — rather
  than on the counter internals already fixed. F8 (type-1 C5) is now implemented, so it is no
  longer a candidate explanation; F6 Stage 1's presence/type/skew approximation landed
  2026-08-23.

Do not infer hardware accuracy from the green counter-level simulation gate alone.

The build is suitable for classic CPC regression testing. It contains the F1, F2, F3, and
main F5 CRTC accuracy work. Plus support is not bootable yet: the `Plus model` menu and
tested leaf modules are foundations only, and the production cartridge client remains tied
off. Selecting a Plus model is therefore not a meaningful hardware test at this milestone.

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
- The current local gate reports 93 required CRTC passes, zero expected failures, no
  unexpected passes, and no failures (verified 2026-08-23, Verilator 5.050). The randomized
  equivalence soak reproduces golden hash `0xf5f8ae01ffdf928d`, re-minted for the sampled-field
  expansion (review issue 4 remediation; previously `0x326ea81358e7d88f` from F6 Stage 1).
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

The next independent classic checkpoint is F7 RFD including the B6 disarm path and A1
VSYNC-corner fix; F13 waits for hardware and does not block it. F10 remains the last,
separately fixture-gated project.

## Completed Plus foundations

- A separate default-off `Plus model` selector decodes GX4000, 6128+, and 464+ capabilities
  without reinterpreting the classic model field or selecting Plus hardware.
- The ASIC lock/unlock state machine is implemented and exhaustively unit-tested as a leaf.
- An atomic 512 KiB cartridge memory service is implemented and tested for clear, load,
  commit, abort/detach/reset, invalid addresses, and CPU reads.
- `sdram.v` now has a held cartridge request/acknowledge client with tested byte lanes,
  addressing, arbitration, back-to-back transfers, classic main/tape writes, and refresh
  fairness. The top-level ties it inactive until P0 integration.
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
  windows are bridged to the service with CPU WAIT insertion and an open-bus-FF watchdog.
  The CPR stream is live on ioctl index 8 (OSD "F8,CPR"), and a P0 boot integration bench
  runs parser + service + real SDRAM end to end, including reset-mid-load cleanup.
- Dandanator uploads are bounded below the Plus cartridge reservation: bank 3
  `0x000000..0x07ffff` remains Dandanator, while `0x080000..0x0fffff` is reserved for Plus.
- Plus P1 counter/timing foundation is implemented on `plus/p1-crtc3-foundation`:
  `rtl/plus/asic_video.v` carries the type-3 register file, C0/C9/C4 counters with the
  type-3 R9-forced-reset and R4-overflow rules (ACCC §10.3.4/§12.5), R5 vertical
  adjustment that freezes C4 at R4 (§11.2.6/§11.3.3), the two-stage video pointer with
  the C4=0 ∧ C0=0 reload condition (§20.3.4), DE with line-start-only R6 semantics
  (§18.2.4) and SKEW-DISPTMG (§19.2), and HSYNC/VSYNC generation including the bounded
  R3=0 widths and the §15.3.1/§15.3.2 infinite-HSYNC relation. 26 deterministic vectors
  (t01a-t04g) cover them; every rule cites its ACCC section at the point of
  implementation. `t03c` also pins the simultaneous C0=R1=R0 row-end save/reload so MA
  advances to the captured row base rather than restoring stale VMA'. Interlace is
  stored-but-inert; status registers/read map are P5.
- Still open in P1 before the milestone is complete: the basic locked-ASIC pixel path
  (needs the legacy-colour table sourced from [KT] or hardware measurement), and the
  CPU/WAIT timing-contract decision that lands with the first motherboard instantiation
  of `asic_video` (architecture §5 Risk 1). `files.qip` is untouched until that
  instantiation commit. The locked-ASIC title boot-point check remains the manual P0/P1
  hardware checkpoint described above.
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

The next Plus milestone work is the P1 remainder per `docs/plus/architecture.md` §4/§7:
pixel path + clocking-contract decision, then P2. The P0 hardware checkpoint is manual:
with a Plus model selected, load a real `.cpr` (e.g. the local untracked `crtc3_v2fix.cpr`
fixture) and confirm the firmware/game reaches its first screen; classic mode must be
re-checked side by side in the same session. Do not start Plus video by extending
`ga40010`; the planned path is the parallel behavioral `asic_video` module now under
construction.

Plus P0 wiring is merged onto `accc-review-and-fixes` (merge `daf1d6f`) and has a green
GitHub Actions build (simulation + synthesis) on the merged tip. Fitter: 15,295 / 41,910
ALMs (36%), 685,217 block-memory bits (12%), 3 / 6 PLLs; worst setup slack +0.342 ns,
worst hold slack +0.244 ns (TimeQuest still reports the repo's unconstrained external I/O
paths, so internal slacks are not full closure). Versus the pre-P0 build (`4c78603`:
14,947 ALMs, +0.516/+0.246 ns), the ~350-ALM growth and small setup-slack shift match the
added cartridge decode/bridge logic; no regression signal. It has not been hardware-tested.

## Build and tooling state

- `.github/workflows/build.yml` runs local-style Verilator tests/lint before a pinned Quartus
  17.0.2 synthesis job and uploads the RBF plus fitter/timing reports.
- GitHub currently warns that `actions/checkout@v4` targets deprecated Node 20 and is being
  forced onto Node 24. It does not fail the build; update the action in a separate CI-only
  maintenance commit rather than mixing it into an RTL milestone.
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
     `t20h` now exist locally, so a divergence here maps straight to code).
   The first two passes produced only an aggregate impression and are not actionable.
   Confirm SHAKER's own CRTC identification agrees with the OSD selection before comparing.
3. Map each persistent SHAKER difference to an implemented finding or a named gap; add a
   deterministic regression before repairing any newly understood behavior.
4. Classic: F6 Stage 2/2b is complete per `accuracy/f6-decision-gate.md`; F13 records the
   half-character CRTC-side phase mismatch and is BLOCKED-PENDING-HARDWARE-EVIDENCE.
   Next independent work: F7 RFD (including B6 disarm and the A1 VSYNC-corner fix).
   F10 stays fixture-gated.
5. Plus: P0 is merged (CPR parser -> cartridge service -> SDRAM -> `plus_mmu` windows ->
   Z80, ioctl index 8). Next Plus steps: the manual hardware checkpoint named above (real
   `.cpr` boot with a Plus model selected, classic re-checked side by side in the same
   session), then P1 CRTC3 counter/timing foundation per `docs/plus/architecture.md` §4.
   Whole-branch review of `plus/p0-parser-wiring` is pending in `docs/review-debt.md`.
6. Update this file when either stream reaches its next hardware-testable checkpoint.
