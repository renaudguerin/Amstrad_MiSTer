# B21 post-integration cartridge timing acceptance

**Bounded no-input regression acceptance passed, 2026-09-23.** No new boot,
title or sampled attract regression was observed across the seven B21 titles.
This closes B21's post-integration sampling gate, not general Plus accuracy,
continuous flicker, physical-output timing or interactive gameplay acceptance.

## Build, device and settings

Task base: integrated master `57a90bc86b1ff05d2eacf18cd303957152d7bee4`.
The exact full-effort RBF is `Amstrad_20260923_cb60ff8.rbf`, SHA-256
`29f69fc4ab58c7072f922661da1b1bec9c1df80bc1e245d0ab73d5b22d2d7310`,
loaded from `/media/fat/_Computer/` on `root@mister`. Local artifact and device
hashes agree. `git diff --name-only cb60ff8 57a90bc` contains only four Markdown
files: there is no compiled-source difference. This is acceptance of that RBF
as representative of the integrated source, not a test of a new `57a90bc` RBF.
The [Eerie record](eerie-forest-pending-classic-2026-09-23.md) pins its successful
full synthesis and timing provenance.

The coordinator released the device; preflight confirmed `MENU` and the expected
original CFG. Every new run used unchanged original CPR media through MGL F8,
without keys or joystick input. Local `local/test_media/cartridges/` hashes
match the pinned device originals. The driver checked RBF/media hashes before
loading each case.

Applied CFG: `00004000040000000000000000000000`, SHA-256
`13ef32c7f1acfd5b5c9a1df3aa8b270b6378b00e0f5692fb05e10a350bc35747`.
Bits 34:33=2 request 6128+, bits 36:35=0 request Full sync, bit 6=0 requests
Original CPU timings (`no_wait` off). Other bits were preserved. These are
on-disk settings applied before core load; native PNGs do not show the live OSD.

## Sampling and comparison

Each new title received one load and three serial 768×273 native PNG captures.
The table gives configured boot delay and delay **between completed captures**;
SSH, capture and transfer time make these approximate checkpoints, not exact
emulated timestamps. With no input, the driver's `settle_delay` is not used.
All 18 new PNGs and the three reused Copter PNGs were visually inspected.

| Title | Boot / inter-capture delay | Observation and comparison |
|---|---|---|
| Sonic | 18 s / 8 s | Coherent emblem, wings, clouds and sea; progresses into Green Hill Zone with ring counts 07 then 18 and changed player/scene positions. Matches the prior repaired title/attract sequence. |
| Copter 271 | 45 s / 4 s, reused | Same-RBF Eerie regression run: intact title logo, helicopter and title text advance. No new top-row palette corruption in these samples. |
| Burnin' Rubber | 10 s / 3 s | Intact title and start/options text, changing flame detail. Matches the earlier title samples. |
| Pang | 12 s / 3 s | Portrait, balloons, character and intro text progress as before. Compressed/overlapping text is visible in both old and new captures; this is not a claim that the intro is fully correct. |
| Plotting | 10 s / 3 s | Three identical credits frames, matching the earlier credits scene. Static-screen agreement establishes no later progression. |
| Navy Seals | 12 s / 3 s | Title/eagle and fire prompt, then rainbow high-score table. Matches the earlier sequence. No gameplay sprite-line flicker claim. |
| CRTC3 demo | 10 s / 3 s | Black intro checkpoint, then magenta logo, then surrounding blocks and coloured gradient. The earlier run shows the same progression, including the black first capture. |

The comparison set is Build B `64702ac` from the
[B20-7 device acceptance](../sonic/b20-7-dma-pause-acceptance-2026-09-22.md).
Its actual PNGs were consulted rather than treating the summary's shorthand
(e.g. CRTC3 “clean gradient logo”) as a description of every captured frame.
Unaligned animation phases differ, so this is visual sequence comparison,
not pixel equivalence or a timing-rate measurement. That baseline already has
the READY-only cartridge fix; this pass detects later integration regressions,
not the isolated causal effect of enabling `cart_granted`.

Copter evidence is reused from the exact `cb60ff8` run in the
[Eerie acceptance](eerie-forest-pending-classic-2026-09-23.md), with matching
RBF, media and applied CFG hashes. It was not reloaded for this task.
No new AmSpirit run was needed: there was no newly divergent sampled scene for
an emulator comparison to discriminate. Original-hardware correctness of
Pang's text remains unadjudicated.

## Evidence retention and restoration

Private evidence is retained in this task checkout under
`docs/screenshots/b21-cart-timing-2026-09-23/` (gitignored): six run directories,
`copter-reused/`, copied `baseline-64702ac/` evidence, case JSON, MGLs, manifests,
`capture.py`, `capture.log`, original/applied/read-back CFG and `restoration.json`.
The run manifests retain their original creation paths under
`docs/references/b21-cart-timing-2026-09-23/`; files were moved after completion
because `docs/references` resolves to `docs/specs`. Use the retained directory
above when locating images. Only documentation is committed.

The `finally` cleanup restored the original CFG, downloaded it again and compared
all bytes. SHA-256: `2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`.
It then loaded `/media/fat/menu.rbf`, read back `MENU`, and released the device.
All six driver manifests report success. No RTL, media or simulation changed;
`git diff --check` is the documentation gate, with no simulation or independent
code review required for this documentation-only change.

Still captures cannot exclude intermittent flicker, establish exact CPU/raster
phase or certify gameplay, audio, other Plus models, all sync modes or full-demo
completion. If a later title regression appears, retain B21's discriminator:
compare a controlled build with `cart_granted` tied low at the `plus_mmu`
instance, then locate the newly exposed timing defect instead of restoring the
old stall as a permanent workaround.

## Pinned media and capture hashes

Capture columns are ordered 1–3. Copter rows identify reused evidence.

| Title | Original CPR SHA-256 |
|---|---|
| sonic | `4cb31c7f1769989a029fe1bd7ffb04670d5d24570dc56b0417fac662fed887ae` |
| copter-reused | `4b75c62cbd660206ef30ff8cbf9c4b1281282a423b5eccc1ff8444589f2a9c1d` |
| burnin_rubber | `08c81e4aeca95abaecc3c196519e2aeb63d2f805b6e18c055a7a3cf49e51a1df` |
| pang | `1bd132aa5871581db4fbc379d3ad9bfe162f0e098fb6f0e48f71bccfabc0912b` |
| plotting | `d9e24dcdf199bdba270723cb835b006eba014576e010e8621d65a756a850d85e` |
| navy_seals | `a933276f9f580ad81cb3d04af7de1a0ebdbb3944e4fa980137010bb4cd0a0695` |
| crtc3 | `3d3f5e01c291e97c9284d10e27f12c0fda3e9530933f0919c607f354e38a7ad6` |

| Title / capture | PNG SHA-256 |
|---|---|
| sonic / 1 | `fce5a4f44a887de38f1ec02e94bfac562d10aedf968eaee88bae080375d89c0b` |
| sonic / 2 | `50657058da09ed90d303d430f1d551e122929c9fb7f2c49139c18b570f32cc7b` |
| sonic / 3 | `c2a9912eed52c4c2e3001fda7d7a37bb9a8604ade9823f6f6bcd346fc25b36b1` |
| copter-reused / 1 | `e163919f0d2f534503b3115aeb11bdbc2272990d9dc2a579dbacaa7d7c4a4a11` |
| copter-reused / 2 | `a8934d8e5486de9e7ae1d3aae410bb44d5c496a5f46566f0ceaba29df8f66d9b` |
| copter-reused / 3 | `1eb53d32b5e08e4b05fe93f2dab36efbef84682344c47bfc33e0001ef17dcebd` |
| burnin_rubber / 1 | `3289dddf4e3930d2b07eee29ef322160c0cf771de64348a04ed06af4032b538f` |
| burnin_rubber / 2 | `893b0b6b36af6692ca0e11717c3af868c5001f9320383644368e45afd2555cce` |
| burnin_rubber / 3 | `b692817a6d54b6a9a7d68552db342c162a8191e6a496fd8e5a5258fd6a3186b4` |
| pang / 1 | `ea929c69841e6ae454573a5d96096ea0a560779d6fc4e11d3398333e05926841` |
| pang / 2 | `1d04f4382a2557b19e17236842f14ff2d22bd89f0dd8cbb6a40945ece552a7e7` |
| pang / 3 | `305418ad5b9fb50e819ff229c64bb08e1092308ffd69f9d866aed9918e481c26` |
| plotting / 1 | `34ef34287f897e234509065fb2a55b84e16c4babc67e454a1729bade0eb9cc91` |
| plotting / 2 | `34ef34287f897e234509065fb2a55b84e16c4babc67e454a1729bade0eb9cc91` |
| plotting / 3 | `34ef34287f897e234509065fb2a55b84e16c4babc67e454a1729bade0eb9cc91` |
| navy_seals / 1 | `720813666cb16c7b316c3abc92bec5fb9f6961d1fd0adf7948e2c4fc3145af88` |
| navy_seals / 2 | `720813666cb16c7b316c3abc92bec5fb9f6961d1fd0adf7948e2c4fc3145af88` |
| navy_seals / 3 | `b1c4cf3d1061bc53c3f7089377c59e45ecb1296c6a44da33e336219d8564d29f` |
| crtc3 / 1 | `386c70a5e6243a33f73b8fbdea04993ca691ac5fd9bab6c79a6fc5ed78ba2bf6` |
| crtc3 / 2 | `320bd9b2ac54dc147e7e5fe68012d99a39fd209f2b812f4748c2e1dd570d3f3f` |
| crtc3 / 3 | `bc60a47f048ab2a4a833c0c2516951e43a57a8b6cfe10552a905d84247df71d4` |
