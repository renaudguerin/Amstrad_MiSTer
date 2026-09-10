# Plus P10 hardware round three preparation — 2026-08-30

This note records the source-backed work prepared after hardware build 165 showed no obvious
Plus improvement and build 166 proved functionally equivalent for Plus. Neither build is a
hardware validation of this branch. The branch must still receive an exact full-effort build,
a named RBF hash, and a repeat of the affected hardware cases before any title symptom is
promoted beyond a retest candidate.

## Source-backed CRTC3 R8=1 correction

ACCC v1.11 sections 19.6.4 and 19.7.3 provide the expectations used by `t04n`. With CRTC3
R8=1, the outgoing even-parity field has one additional line after R5 with C4 held at R4,
C9=0, VMA reloaded from the terminal ordinary row, RA=0, and DE low; its VSYNC starts at the
midpoint. The alternating field has no additional line and starts VSYNC at the seam.

The unchanged implementation failed the initial discriminator with two seam starts and no
midpoint start. Once the frame-line rule was added, the stronger state discriminator also
failed unchanged RTL because the expected held C4=R4 line was absent. The corrected model
passes one seam start and one midpoint start across two fields, the additional-line state,
and the negative control proving that the odd field restarts directly. This is source-backed
simulation evidence. It does not close Pang, the CRTC3 demo corruption/crash, odd-R5
recurrence, or any hardware title.

Independent review also found the R4=0 alias where the added-line entry has C4=C9=0 but is
not a frame origin, and then caught an insufficient first guard at R9=0, where the forced
C9=0 also satisfies the row-end comparison on added-line exit. The final rule uses
`frame_restart` as the pointer/timer origin for both R8=1 and R8=3. The R4=0 entry control
first distinguished captured VMA' from an R12/R13 reload; the R9=0 exit control first failed
with MA=`0x103` instead of reloaded MA=`0x101`, then passed at the corrected real origin.
The final review then found R9=0/R5-nonzero could re-enter adjustment on that added-line exit;
its source-derived seven-line control first failed with ADJ=1, then passed after adjustment
entry was excluded while the added line is active.

## Production motherboard DMA/PPI/PSG concurrency

Two fixtures now cover the seam at different resolutions. The focused fixture drives the
real DMA, PPI, PSG, and motherboard arbitration equations and checks all three DMA channels.
The full fixture elaborates `Amstrad_motherboard` with production `asic_regs`, GA timing,
video, sprites, DMA, CRTC, i8255, YM2149, and HID. A PHI-aligned Verilog CPU substitute makes
accepted I/O strobes and WAIT duration observable under Verilator; it is not a replacement
for exact T80pa/Quartus timing evidence.

The full fixture first exposed the production PSG path directly: `LOAD duration mismatch:
observed 9 CCLK, source-derived expected 10 (ppi=1, psg=1)`. Later discriminators exposed a
duplicate accepted PPI strobe when DMA ownership arrived after CPU acceptance, and an AY R14
keyboard read returning `0xFF` instead of the pre-owner `0xDF` sentinel. Guarded review then
found that applying that latch to every Plus PPI read froze live Port B VSYNC/tape data even
without DMA; the production fixture first failed `uncontended Plus Port-B read did not follow
live tape input`. The production fix:

- keeps ordinary DMA LOAD execution at eight CCLKs, adds one cycle for a colliding PPI
  transaction, and a second for a physical PSG write;
- recognizes Port A under PC7:6=`10`, full Port C writes, and PC7/PC6 BSR transitions into
  that PSG state;
- permits the bounded ninth-cycle PPI-to-PSG upgrade without an eleventh cycle;
- prevents a CPU transaction accepted before DMA ownership from being re-waited or replayed;
- latches an accepted pre-owner PPI/AY read result only after DMA temporarily replaces the
  live PSG bus, leaving uncontended Port B VSYNC/tape reads live through retirement; and
- gates the new classification and arbitration with `plus_mode`, preserving the classic
  path structurally.

The focused DMA unit passes 14 cases. The focused concurrency fixture passes base, PPI, PSG,
late-PPI, late-PSG, and physical-register classifications. The full motherboard fixture
passes 24 LOADs, 80 PPI transactions, a maximum observed wait of five CCLKs, and 103 CPU
operations, with exactly one accepted strobe per transaction, none accepted under DMA
ownership, the overlapped `0xDF` R14 read preserved, a live uncontended Port B input, and the
Port A/PC7-BSR/PC6-BSR physical classifications observed through production. The late
PPI-to-PSG upgrade is directly toggled on all three DMA channels. These results make Arnold 5 keyboard,
Plotting held Fire, and DMA sample pitch direct hardware retests; they do not prove those
symptoms closed.

## Review and simulation gates

Fresh native Sol and guarded Claude reviews covered the non-trivial CRTC3 and production
DMA/PPI/PSG changes. Review was load-bearing: it found the initial missing additional line,
the R4=0 and R9=0 false frame origins, the R9=0/R5-nonzero adjustment re-entry, and the
over-broad PPI read latch that froze uncontended live Port B. Each finding received its own
focused failing discriminator before the minimal production fix. The final native reviews
returned CLEAR. The guarded final CRTC micro-review independently reverted only the
added-line adjustment guard, reproduced `ADJ: expected 0, actual 1`, restored the guard, and
returned CLEAR after 67/67 focused tests and a full-suite run. The guarded DMA review also
accepted the post-owner-only read latch and live Port B control. No independent-review debt
remains for this round-three delta.

The definitive post-commit gates passed on tip `221745b`: `make -C sim`,
`make -C sim lint`, `make -C sim soak SOAK_EXPECT=0x32d468e81eac63c9`, and
`git diff --check`. Verilator reports only the repository's known waived or non-fatal
warnings. The classic soak reproduced `0x32d468e81eac63c9` exactly.

## Accuracy/FDC peer reconciliation

Accuracy tip `683fcaf3afab672c9bec85c43066292eb9f6bf75` adds a real production-timed u765
READ DATA/EDSK seam. Its tracked case uses track 0, head 0, R=`&41`, N=2, payload `&200`, and
resets while sector LBA1 is outstanding. Unmodified RTL failed `reset retains cancelled
request until host ACK: expected 1, actual 0`. The accuracy-owned fix retains the cancelled
request and ownership until the old ACK rises and falls, gates buffer writes during
reset/cancel, then reloads metadata and verifies all 512 bytes. It closes that demonstrated
late-ACK/reset-reload alias in simulation only; it is not BASIC hardware closure and does not
indicate an `Amstrad.sv` or Plus FDC-decoder change.

## Hardware-only residuals and next gate

- Record the exact full-effort synthesis SHA, RBF hash, model, media, ROM, and configuration.
  This branch changes motherboard WAIT/PPI timing and therefore requires exact synthesis
  before hardware testing.
- Repeat Arnold 5 keyboard, Plotting held Fire, CRTC3 sample pitch, Pang, the CRTC3 demo,
  Copter 271, Burnin' Rubber, Switchblade, BASIC/System reads, and reset/reload recovery.
- For FDC, reset during active READ DATA. Retain no-ACK epoch/tag, two-drive overlap,
  sector-search reset, WRITE DATA `buff_wr`, and automatic-EOT C/R as named validation
  residuals.
- Do not redesign cartridge WAIT/cache pacing without a valid ordinary-RAM/title comparison.
- Confirm on hardware whether a Port A write while configured as input still counts as the
  ASIC's conservative PSG-write collision class.
- Do not add undocumented sprite fetch, blanking, colour-row, or coordinate rules without a
  captured first divergence or primary source.
- Full `Amstrad.sv` CPC+ SNA/reset/reload coverage and a deterministic stuck-core recovery
  discriminator remain below the required top-level boundary.
- CRTC3 C4 free-run wrap pointer-origin behavior in R8=1/3 remains a pre-existing,
  self-correcting source-model question without a captured title discriminator.
