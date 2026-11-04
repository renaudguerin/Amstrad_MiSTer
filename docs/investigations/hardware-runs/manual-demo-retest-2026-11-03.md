# Manual demo retest on 5fba168 — 3 November 2026

## Acceptance position

Renaud's manual testing supersedes the earlier blanket DSC4/Amazing acceptance.
The CRTC1 main-effect repairs stand within the tested scope; the complete demos
are not clean across all checkpoints or CRTC types. The ASIC has a separate
counter/address engine in `rtl/plus/asic_video.v`; classic engine changes do not
automatically change CRTC3 behavior. Whether an analogous change is appropriate
requires a type-3 rule and a failing discriminator. Renaud subsequently
confirmed that DSC4 correctly detects CRTC3 on the device; treat detection
as verified and investigate the later execution/display path.

| Demo / checkpoint after RUN | CRTC0 | CRTC1 | CRTC3 / ASIC |
|---|---|---|---|
| DSC4 credits, about 23 s | Colons missing after GFX / ORIGINAL GFX / MUSIC / CODE | Same missing colons | Word endings more severely clipped |
| DSC4 image, about 43 s | Not separately reported | Single corrupt line at the very bottom | Reported OK |
| DSC4 main effects, about 50 s | Mostly fixed, remaining small issues | Reported fine | Only a top band visible, remainder black |
| Amazing main effects, about 30 s | Reported fine | Reported fine | Still completely broken |

Launch commands: `RUN"-DSC4` and `RUN"AMAZING`. The report names build
`5fba168`; no new bitstream was loaded for this record. Do not attribute a
model or filter beyond the user's report from a screenshot filename alone.

## Preserved evidence

Copied from `/media/fat/screenshots/Amstrad/` without changing the device:

- `20261103_205511-screen.png`: DSC4 credits on classic CRTC0/1.
- `20261103_205848-screen.png`: DSC4 bottom-line residual on CRTC1.
- `20261103_210557-6128_FR.png`: DSC4 credit clipping on CRTC3.
- `20261103_210632-6128_FR.png`: top-strip/black DSC4 effect; exact time is
  not independently established from the PNG.

Local copies are ignored under `local/overnight-20261103/` in the task
checkout. AmSpirit 1.17.1 is the requested emulator comparison; it is diagnostic
evidence, not original-CPC proof.

## Device ownership

Renaud's temporary exclusive-use hold ended with his explicit all clear during
this run. Automated checks may now use MiSTer; keep a single device operator
and pin the RBF/media/settings for each new run. The copied manual screenshots
above predate the automated continuation.

## Next discriminators

Separate credit clipping from frame/address failures. Inspect the DSC4/AMAZING
type-detection and register-write paths under AmSpirit, then replay the actual
type-3 path against the ASIC engine. Port a classic rule only if the French
ACCC's type-3 behavior or stronger hardware evidence supports it. Preserve the
classic 43-second residual as its own checkpoint.

## Controlled continuation

The pinned `5fba168` baseline was reproduced after device release with the
original DSC4 and Amazing disks on explicit 6128+ / CRTC3 / French system
cartridge settings. DSC4 again shows clipped credits, black main capture
and a later top strip; Amazing again shows a broken header/text region and
then black. Cases, manifests and exact hashes are under
`local/overnight-20261103/asic/{dsc4-device-5fba168,amazing-device-5fba168}/`.
Screenshot retrieval adds host latency: do not equate successive capture
timestamps with exact emulated 23/30/50-second checkpoints.

Fresh AmSpirit Lite 1.17.1 classic 6128/CRTC1 and 6128+/CRTC3 cold boots
show all four credit colons. The classic 42.5/43-second otter captures have
a clean bottom row; Plus main captures are coherent too. Saved requested
and effective config, render state, CPU history and SNA remain under
`local/overnight-20261103/{classic/dsc4-crtc1-coldboot,asic/dsc4-plus-coldboot}/`.
They use the original disk hash
`ec9e0d51b10a7455848bd9211793c585c46456759486057b12a491039923b8e4`.
These are reference-emulator observations, not independent silicon timing
measurements. The snapshot's tick epoch restarts after load; any later
restored trace must not be presented as uninterrupted cold-boot timing.
