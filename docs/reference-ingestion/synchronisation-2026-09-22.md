# Synchronisation source versus the video pipeline

Checked against base `a33d93e` on 2026-09-22. This is a source/integration comparison,
not a new Classic CRTC rule or hardware acceptance result. Source S30 in the
[inventory](scrapes-2026-09-22.md) is the five-page captured CPCWiki
*Synchronising with the CRTC and display* (revision 33234).

## What the source adds

- **PDF pp.1–2:** polling PPI B0 detects CRTC VSYNC with loop-dependent latency.
  The example assumes VSYNC was inactive on entry. Its cycle counts are software
  example units; do not translate them directly into FPGA master-clock counts.
- **pp.2–3:** HSYNC-derived interrupts can refine synchronization after VSYNC;
  the example uses HALT. The prose claiming constant request-to-handler latency
  is too broad without an instruction/interrupt-enable/wait-state qualification.
  Its “3 interrupt sources” list actually enumerates four Plus sources; the
  `im 1` example comment says mode 0. Neither typo is an implementation rule.
- **pp.4–5:** PRI reprogramming and DMA regular cadence are useful programming
  patterns. Calling the PRI target a visible-area line does not establish a
  hardware display-enable gate. Plus range/phase disputes remain in the separate
  [interrupt findings](../plus/references/scrapes-interrupt-findings-2026-09-22.md).

## Verified production boundaries

| Boundary | Current source evidence | Consequence |
|---|---|---|
| CPU observes raw VSYNC | `rtl/Amstrad_motherboard.v`: `vs_sel` at 448; `ppi_ipb` at 1151, connected at 1167 | PPI polling observes selected Classic/Plus CRTC VSYNC, not filtered monitor VSYNC. |
| Filter input | Motherboard `crt_filter` instance at 1023 takes `hs_sel`, `vs_sel`, `phi_en_n` | Raw selected CRTC timing enters the filter. The broad comment in `crt_filter.v` calling HSYNC_I a GA/ASIC force-blank event must not be mistaken for a GA-shaped monitor-sync input. |
| Filter output | `rtl/crt_filter.v`: `hsyncfilt`, `syncgen`, `blankgen` | Close HSYNCs can be masked; absent sync can be synthesized; sync and fixed-border blanking are regenerated. These are core output policies, absent from the programming article. |
| Full / Raw pixels | Motherboard `crt_filter_output_select` at 1006 and mode contract at 979 | Both use the Full acquisition timing tuple. Raw pixels changes byte/vertical-mask policy; it is not raw output synchronization. |
| Raw CRT | Same selector: raw monitor sync = `hsync_ga/vsync_ga`, horizontal blank = `hs_sel`, vertical blank = `vblank_ga` | Raw CRT is an explicit geometry-changing diagnostic policy, not an unmodified real-monitor oracle. |
| Pixel dependency | Motherboard accepted-fetch block near 900: Full samples `crtc_shift`; raw modes keep native byte order | A screenshot before the final selector is not automatically filter-independent. |

This corroborates the distinction already recorded in
[B6](../investigations/video-boundary/b6-video-boundary.md) and backlog B1/B6.
It does not justify removing `crt_filter` or changing Classic CRTC timing.
For a software-synchronization failure, capture raw CRTC HS/VS, CPU-visible B0,
interrupt request/acknowledge, selected output HS/VS/DE and pixels together.
Record the filter mode. A stable output screenshot alone cannot identify whether
software synchronization, raw timing, byte phase or scaler acquisition caused it.

Classic rule changes still require the French ACCC/hardware hierarchy. No ACCC
rule was changed or newly adjudicated in this ingestion.
