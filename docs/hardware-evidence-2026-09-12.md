# Hardware retest reported 2026-09-12

This records the user's direct hardware observations. It supersedes earlier
verdicts only for the symptoms named below; source review, simulation and other
title/subsystem acceptance remain separate.

## Build and configuration

The user confirms **`5c16b17`**, delivered as
`Amstrad_20260911_5c16b17.rbf`, and **6128 Plus** for the Plus tests.
The delivered local artifact's SHA-256 is
`8b3b5bed518165040f8e578c83f061891fa64d58ce3b52fb07d5765765506468`;
the device copy was not independently hashed in this report. Its full-fit
Quartus build passed with setup +0.320 ns, hold +0.247 ns and zero TNS; see
the [artifact record](b6-video-boundary-review-2026-09-11.md#published-integration-and-full-fit-artifact).
The output connection, Classic model/CRTC selection, media versions/hashes,
reset sequence and other video settings are unrecorded. The B6 comparison
explicitly used **Full** and **Raw pixels**. No Raw CRT result, new screenshot
or video was supplied with this report.

## Reported results

| Scope / symptom | User observation | Current verdict |
|---|---|---|
| Plus BASIC boot | "Basic boot confirmed fixed in Plus." | **Confirmed fixed on 6128 Plus hardware.** This closes the reported boot symptom, not all model/cartridge combinations or disk I/O acceptance. |
| Plus sprites at the left edge | "some lines corrupt in sprites on left edge of the screen" is "much improved, perhaps even fixed" | **Much improved; possible fix, not confirmed.** The report does not name the title or prove D3 causality. |
| Pang / Plotting input | "Fire always pressed in Pang/Plotting" is **NOT fixed** | **Still failing.** D4's scoped PPI readback repair does not close this input symptom. |
| Copter 271 logo | **NOT fixed** | **Still failing.** No new mechanism is established. |
| B6: Amazing Demo | No visible difference between Full and Raw pixels so far | **No visible improvement demonstrated by this comparison.** Existing corruption remains open. |
| B6: DSC4 | No visible difference between Full and Raw pixels so far | **No visible improvement demonstrated by this comparison.** Existing failure remains open. |
| B6: SHAKER module A (T) | No visible difference between Full and Raw pixels so far | **No visible improvement demonstrated by this comparison.** This is not a verdict on the numeric B (9) or C (4) parity retests. |

No visible difference does not establish that the applied modes or their
internal signals are identical. The next capture should retain the selected
mode and exact configuration, and the B6 fixtures should establish where a
deliberately discriminating trace differs in pixels while acquisition stays
unchanged. Do not infer an RTL cause or a successful visual repair from this
report alone.

## Next work and ownership

1. **B2 autonomous capture:** the user reports MiSTer online at `root@mister`.
   Use one task as the sole device operator. Complete real boot/menu input for
   a stable SHAKER case, collect decoded captures and repeat across independent
   loads with exact RBF/media and observed configuration. Online status is
   user-reported until SSH/device execution succeeds.
2. **B6 remaining validation:** complete CPU-driven malformed-raster and
   combined Plus scroll/opaque-sprite cases; investigate the production
   mixer's Verilator RGB elaboration gap. Preserve the distinction between
   source behavior, simulated final output and physical HDMI/CRT acceptance.
3. **Review closure:** verify actual outstanding review coverage, remediate
   actionable findings, and require fresh cross-provider review for new
   non-trivial implementation. Existing source-review CLEAR verdicts do not
   close these hardware residuals.

Pang/Plotting input and Copter 271 need production traces before further
title-driven RTL repairs. BASIC boot no longer blocks the reported Plus
setup; disk access still needs its own test. D1/D6 SHAKER B (9)/C (4), Raw CRT
and complete B1/P10 hardware acceptance remain open.
