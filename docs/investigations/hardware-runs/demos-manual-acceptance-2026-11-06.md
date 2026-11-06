# Manual demo acceptance, 6 November 2026

Renaud reports manual MiSTer testing of **Amazing, DSC4 and Before the Storm
(BTF), all working on CRTC0, CRTC1 and CRTC3**. This accepts the reported
credit corruption, missing effects and alternating horizontal-scroller wobble
on all three supported CRTCs.

The tested RBF filename/SHA was not supplied, so this report is not attributed
to an exact build. The latest separately verified hardware artifact remains
`5c1ab57`; its source-bound native samples and the earlier before/after captures
remain the reproducible evidence. No additional demo rerun is required merely
to repeat this manual acceptance. Future behavior changes still need targeted
preservation checks.

This accepts the reported demo defects. It does not judge every SHAKER page,
validate the proposed CRT-response model, or establish original-chip timing for
remaining inferred CRTC rules. MiSTer is released for continued work.

Related evidence: [classic display](classic-ga-display-edge-2026-11-05.md),
[ASIC position](asic-r3-rendered-position-2026-11-05.md),
[BTF](before-the-storm-r3-2026-11-05.md).
