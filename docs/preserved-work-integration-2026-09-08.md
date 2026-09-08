# Preserved FDC and test consolidation integration

Accepted source: `5fcf223a66085e0db67f1410f0b87d2d3565f053`, based on
integration `f0ed9b6121afc5078185171546d5ea5f723c376c`. Separate no-ff merges
retain FDC source `c1a8ff9` (`bf9a731`) and test-consolidation source `c12c264`
(`92a5e24`); `5fcf223` corrects only P1 fixture commentary.

The FDC test holds nRD for three CE pulses, checks stable data and subsequent
byte consumption against the synthetic EDSK payload. It covers bytes 0 and 1,
not full-sector result-phase/ST1 or classic AMSDOS acceptance. The duplicate
model-select test is removed; B6 retains explicit RAM-size checks alongside
the model/menu integration checks, and standalone decoder lint remains.

Fresh Gemini review of the combined code/test diff at `92a5e24` returned
ACCEPT / CLEAR in guarded run `20260908T042930Z-3512-3f74` (exit 0, complete
handoff). Its inherited P1 timing-ownership assertion was incorrect: source
inspection shows `p1_mobo_bench` ties VRAM data to zero. The parent required
the comment correction at `5fcf223` and corrected the accompanying audit
prose during integration. That fixture proves neither physical memory return
timing nor the complete address-to-pixel path. No RTL behavior changed.

Source-owner gates passed: FDC 7/7, B6 menu, full simulation, full lint and
soak `0x6e8258198d6e6137`. The integration checkout independently passed full
simulation, full lint and that exact soak before committing. Changed-path
synthesis classification is false. This test/documentation-only change reuses
the existing `dba49d5` RBF with its original source identity; CI evidence is
reported against the resulting integration SHA.

Integration `5c201a8d081e646cfca32c9294ad38359fb4ca09` passed simulation/lint,
synthesis policy and the required gate in
[CI run 34187708364](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/34187708364);
Quartus was skipped. The task worktree was removed after this check, with its
branch retained and unique evidence copied into the integration checkout.

The B3 capture branch remains pending coordination with SDRAM/P10 changes.
Both stashes and private recovery files are retained. The missing temporary
CPU candidate has no staged recovery diff or additional commit in its retained
worktree metadata; historical payload results do not establish a runnable
candidate or phase-verified result bytes.
