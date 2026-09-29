# Diagnostic and safety integration, 2026-09-29

The original `4a44394` title regression was reported clean by the user. The
follow-up work separates numeric CPU/interrupt evidence from monitor acquisition
and preserves that acceptance without extending it to new builds.

## Delivered diagnostics

- **PA3:** `output_files/pa3-followup/cartridge/pa3-followup-start00.cpr`, also
  packaged as `output_files/pa3-followup/pa3-followup-morning.zip`. Five labelled
  terminal-split controls retain the late baseline and R5=0 comparison. All five
  passed production-T80 timing checks and AmSpirit cold-boot/navigation checks.
  Source `0dc566a`, integration `bde8492`, exact CI run `36504360145` passed
  simulation/lint, production-T80 and the required gate; synthesis was unnecessary.
  [Original-Plus instructions and evidence](../plus/pa3-followup-cartridge.md).
- **Classic B25:** `output_files/classic-b25/classic-b25.dsk`. From AMSDOS,
  `RUN"B25"`; photograph all nine groups and eight trials after normal video
  returns. Real HALT interrupts synchronize each measurement; an SNA does not
  establish the phase. Production T80/GA/CRTC0/1 passed 432 captures, and actual
  AmSpirit AMSDOS boots and reruns passed for both CRTC types. Source `4c665c2`,
  integration `d0fdb0f`, exact CI run `36504696881` passed all required jobs;
  synthesis was unnecessary. [Measurement contract](b25-diagnostic.md).

The default PA3 CPR SHA-256 is
`060a7f252d38679892b6b16cfaaf905d5d8881be6ab4930f97eaaadfc00d7543`.
The B25 DSK SHA-256 is
`90111197e72fdf424ff3b4fdb2311da7b08d5dbf19d77a6b580c31678a3b4db5`.
Both are preserved in the main checkout. Original-machine acceptance remains
outstanding. B25's INC pad0 differs between production simulation and AmSpirit;
AmSpirit CRTC1 also differs on ADD pad1. Neither model is promoted to a hardware
oracle by those differences.

## RTL and host safety changes

- **B18, integration `fe42298`:** loaded Dandanator state prevents snapshot
  admission even when chip select is idle. Fail-before/pass-after coverage
  composes the actual production expression with the cartridge gate and capture
  FSM. Forty-two selected benches passed; Opus review was CLEAR.
  [Evidence](b18-save-admission-2026-09-29.md).
- **B1/B22, integration `11e9819`:** passive SSM format-2 records expose
  requested/applied mode and filter history at marker-fetch completion. The
  reader retains format-1 support with unknown observation. The trace separates
  startup period contamination from phase misalignment; acquisition policy is
  unchanged. Selected tests, focused B6 checks and unchanged soak passed;
  fresh Opus review was CLEAR. [ABI and evidence](video-boundary/b22-acquisition-trace-2026-09-29.md).
- **CSL, integration `b0ef51f`:** uncertain CFG writes trigger restoration
  from a preserved original; cleanup failure fails the run; unknown live CRTC
  selection and unsupported live disk insertion are rejected. Sixty-seven tests
  passed with six absent private-corpus skips; Opus review was CLEAR.
  [Failure-injection evidence](ssm-csl/csl-runner-safety-2026-09-29.md).

B18 exact CI run `36505022364` passed every required job, including full-effort
Quartus 17.0.2 synthesis. Retained artifact `Amstrad-build-299-1-full` includes
fitter/STA reports and worst paths. Setup/hold minima are +0.516/+0.241 ns
across seven clocks with zero TNS; utilization is 24,650 ALMs (59%), 28,407
registers, 102 RAM blocks and 35 DSP blocks. The delivered main-checkout RBF is
`output_files/Amstrad_20260929_fe42298.rbf`, SHA-256
`0ec7dc164acc91fa4d828a8fabca695dedcf62e0b8a4836f23c2ec0f10fd8897`;
the downloaded and copied files hash identically. Reports and Actions logs are
retained in `output_files/b18-integration-fe42298/`.

Sync run `36506927028` passed its six selected benches but failed motherboard
lint: moving `syncs` to module scope exposed two existing blocking assignments
to `BLKSEQ`. A narrow annotation retains their intentional same-edge ordering;
`make -C sim lint` passes after that comment-only repair. The corrected combined
head includes CSL. Its successor `36507312981` exposed a separate clean-build
dependency: selecting only `crt-filter-blank-test` did not create its nested
output directory. Adding `mkdir -p $(CRTF_OBJ_DIR)` makes the target standalone;
an isolated empty `/tmp` output tree passed all nine focused cases. The older
failed runs are not claimed as successful integration evidence. The full synthesis leg of `36507312981` subsequently passed on `cec641c`.
Artifact `Amstrad-build-301-1-full` is retained under
`output_files/sync-integration-cec641c/`, including fitter/STA summaries and
worst-path reports. Setup/hold minima are +0.810/+0.242 ns across seven clocks
with zero TNS; utilization is 24,558 ALMs (59%), 28,390 registers, 102 RAM blocks
and 35 DSP blocks, using clean full-effort Quartus 17.0.2.
The delivered `output_files/Amstrad_20260929_cec641c.rbf` has SHA-256
`40f7d66bfcfda37ce9f5053ff3e3af833713bdb4065fe4bcde9c311a3cb4dd2c`,
verified against the downloaded copy. The aggregate run remains failed because
of the test-directory defect; its synthesis result and production-T80 pass are
separate evidence. The Makefile/docs repair changes no synthesized input
(classifier false), so final CI can reuse this bitstream without relabelling it.
Final repair CI remains pending. New-RBF device checks, real SSH-disconnect recovery,
unobserved key mappings and original-monitor fidelity remain separate acceptance
work. Numeric SHAKER runs use Full once; a small targeted display corpus covers
mode differences without tripling every numeric/title test.
