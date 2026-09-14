# Preserved FDC, test consolidation and B3 integration

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

B3 source `bb77075` and its comment-only P10j predecessor `3db81d0` are
integrated after SDRAM merge `22202b8`. Both P10 insertion conflicts retained
the complete independent test bodies and every test call. Fresh Gemini review
`20260908T053839Z-2528-42c3` returned CLEAR, exit 0 with a complete handoff,
on merge compatibility and current boundaries; the original full B3 review
remains the implementation evidence. It confirmed the B8-4 and FDC test bodies
are unchanged and the capture CLI keeps exclusive output creation.

The tool uses a reduced-TV80 fixture, `production_clocking=0`, a static CPR
program and bounded frame self-equality. It does not certify arbitrary titles,
production-T80 software behavior, hardware images or a filter-independent RGB
path. The existing FDC XFAIL remains a separate unresolved CPU/data boundary.
Private B3 evidence, including its intentional dangling symlink, is preserved
and verified under ignored `docs/references/b3-preserved-evidence-20260908/`.
Both stashes and private recovery files are retained. The missing temporary
CPU candidate has no staged recovery diff or additional commit in its retained
worktree metadata; historical payload results do not establish a runnable
candidate or phase-verified result bytes.

The merged B3 checkout passes full simulation, aggregate lint and soak
`0x6e8258198d6e6137` (2,845,088 samples). A real one-frame CLI invocation
emitted 1,277,952 samples and 62,685,249 serialized bytes with sample hash
`0x547351db3fd52fa7`; capture-file SHA256 is
`ab79cd0420e1c4fefa957272469fbcbe050a004050e7061475bc6176bf705682`.
The input CPR and existing output remain byte-identical after three real CLI
refusals: existing output, output aliasing input, and 17 requested frames.
Logs, refusal results and the captured frame are preserved locally under
`docs/references/b3-integration-review-20260908/`. This is synthetic consistency
and file-preservation evidence, not a hardware golden image.
