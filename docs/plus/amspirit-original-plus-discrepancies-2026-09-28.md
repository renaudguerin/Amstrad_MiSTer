# Two original-Plus / AmSpirit interrupt discrepancies

28 September 2026 · Reproduction report for the AmSpirit authors

Tested emulator: **AmSpirit Lite 1.15.1, core 2491682**, 6128 Plus model,
CRTC3, 128 KB; unfiltered rendering (monitor Off for the DMA runs).
Reference: a user's original 6128 Plus, photographed after running the probes.
These are two observed discrepancies; whether they share a cause is unknown.
No conclusion about newer AmSpirit builds is implied.

## 1. Untaken RET NC: first raster reference marker occurs earlier

**Repro:** load [V5 PA7 cartridge](../../output_files/amspirit-plus-repro-2026-09-28/plus-hw-probes-v5-pa7.cpr). It starts on
screen30. Press/release a key or joystick fire four times to reach
**34 PA7R RET NC**, allowing three seconds to settle. IRQ/FRAME must read04.
The four yellow right edges, top to bottom, are programmed-raster reference A,
compatible interrupt A, reference B, compatible B. The two passes use opposite
instruction phase. Carry is set, so RET NC is **not taken**.

Compare each edge with its own adjacent ruler (ticks are eight mode-2 dots),
not camera coordinates across bands. Hardware values are photographic readings,
approximately ±2 dots per edge; emulator values come from native screenshots.

| Measurement (mode-2 dots) | Original Plus | AmSpirit |
|---|---:|---:|
| Reference A right edge, relative to ruler zero | 68 | 36 |
| Compatible A right edge | 196 | 196 |
| Reference B right edge | 52 | 52 |
| Compatible B right edge | 180 | 180 |
| A / B separations | 128 / 128 | 160 / 128 |
| Mean separation | **128** | **144** |

The difference is specifically the **first programmed-raster reference edge**,
32 dots earlier in AmSpirit. It is not simply a uniform displacement of the
compatible IRQ. Both systems count four interrupts per frame. The on-screen
“RTL MEAN112” is an old static prediction label, not a measured result.

Controls agree in mean: screen30 NOP W8=128 dots; screen32 NOP W12=192;
screen33 LD A,(HL)=128 (144/112 pair); screen35 INC HL=128 (128/128 pair).
Screenshots: [original Plus](../../output_files/amspirit-plus-repro-2026-09-28/evidence/ret-nc-original-plus.png),
[AmSpirit](../../output_files/amspirit-plus-repro-2026-09-28/evidence/ret-nc-amspirit.png).
These software observations do not isolate CPU sampling from ASIC request
phase. A sampling-edge correction resolved this case in our FPGA CPU model;
that is a debugging lead, not a diagnosis of AmSpirit internals.

## 2. Pending DMA: missing DMA-only IRQ and different source order

**Repro:** load [DMA overlap cartridge](../../output_files/amspirit-plus-repro-2026-09-28/pa7-dma-overlap-start00.cpr), wait about
four seconds for DONE, then press/release any key or fire to advance through
four pages: W8/NOP automatic, W8/LD A,(HL) automatic, W8/NOP manual, W12/NOP
automatic. Each page runs 17 rows × eight repetitions. Both systems finish
with REP08, DIFF00 and FL00 throughout.

DMA0 is armed with one INT|STOP request while interrupts are disabled, well
before the observation window. PRI=0 enables the compatible source; the
DMA-only control uses PRI255. IM2 vector04 identifies DMA0 and06 identifies
raster. The handler records DCSR before explicitly clearing DMA. The cartridge
uses A13=1 interruptible code and vector addressing.

| Case | Original Plus | AmSpirit |
|---|---|---|
| DMA ONLY | One DMA IRQ; PRE=C0, POST=00 | No IRQ; PRE=POST=C0 |
| DMA FAR EARLY | DMA then raster | Raster then DMA |
| DMA LATE | Raster then DMA | Raster then DMA |
| DMA sweep | Order changes near the boundary (below) | Raster then DMA throughout |

Original Plus sweep order is DMA→raster for −6…−2, then raster→DMA for
−1…+1 on pages1/3/4. Page2 switches one step earlier: DMA→raster for −6…−3,
raster→DMA for −2…+1. Every sweep row has two interrupts.
No-DMA marker transitions agree between systems: 0, −1, 0, 0 respectively.
Offsets are microseconds from the cartridge's calibrated base.

On hardware, raster-first DCSR=C0; DMA DCSR=00 automatic or40 manual;
raster after DMA reads80. AmSpirit's raster-first handler reads40; DMA reads00
or40 respectively. AmSpirit POST is F0 automatic or B0 manual for these
raster→DMA rows, versus hardware00. Values here are hexadecimal.

The DMA-only control is the simplest starting point: the request remains
pending but is not delivered in the observation window. The control failures
prevent interpreting AmSpirit's sweep as evidence for a particular internal
pending/delivery mechanism.

Paired captures (hardware / emulator):
[page1](../../output_files/amspirit-plus-repro-2026-09-28/evidence/overlap-original-plus-page1.jpeg) / [page1](../../output_files/amspirit-plus-repro-2026-09-28/evidence/overlap-amspirit-page1.png),
[page2](../../output_files/amspirit-plus-repro-2026-09-28/evidence/overlap-original-plus-page2.jpeg) / [page2](../../output_files/amspirit-plus-repro-2026-09-28/evidence/overlap-amspirit-page2.png),
[page3](../../output_files/amspirit-plus-repro-2026-09-28/evidence/overlap-original-plus-page3.jpeg) / [page3](../../output_files/amspirit-plus-repro-2026-09-28/evidence/overlap-amspirit-page3.png),
[page4](../../output_files/amspirit-plus-repro-2026-09-28/evidence/overlap-original-plus-page4.jpeg) / [page4](../../output_files/amspirit-plus-repro-2026-09-28/evidence/overlap-amspirit-page4.png).

## Cartridge identities

These are the existing tested binaries; no rebuild was made for this report.
V5 starts on screen30; the overlap cartridge starts on page1 and contains all
four pages. The recorded AmSpirit V5 captures used equivalent start-screen
variants to select each screen directly.

- V5 PA7 SHA-256: `70efbbbcbf6bcc994461c4052ca9cbd4f06545f6d12f429fb858161a424bf352`
- DMA overlap SHA-256: `0f499bc6c50f935b0f9a455ec953db44a97a6a2db5da57aa215914c80c7c7106`
