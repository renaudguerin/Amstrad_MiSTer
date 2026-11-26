# Roadmap

This fork has two main tracks:

1. Improve classic CPC CRTC types 0 and 1, especially timing behaviour exposed by real hardware and SHAKER tests.
2. Complete Plus and GX4000 compatibility, including ASIC video, audio DMA, cartridge and snapshot behaviour.

Add two further classic hardware profiles in stages:

- **CRTC2 (Motorola MC6845).** Use the classic Gate Array path with type-2
  counter, sync and readback behaviour. Verify progressive timing first, then
  vertical adjustment, interlace and mid-frame register writes.
- **CRTC4 (Amstrad 40226).** Add the cost-reduced classic CPC video profile.
  Reuse shared ASIC timing where the chips agree and verify 40226-specific
  video and bus timing with SHAKER and original hardware.

Cross-cutting work includes reproducible simulation, build and timing checks, and checking important fixes on hardware. Features move into releases after their relevant checks; [known issues](known-issues.md) describes current user-visible limits.
