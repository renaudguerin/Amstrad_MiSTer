# Real 6128 Plus: compatible interrupt comparison

The corrected CRTC3 candidate matches all measured fields on the original Plus
captures for SHAKER BR, both MRER pages and the VS-threshold probe. The same
cartridges on the preceding MiSTer implementation reproduce the BR and MRER
mismatches. This supports the candidate's observable behaviour; it does not
establish a unique internal counter circuit or complete its regression gates.

## Original hardware and callers

Renaud booted the cartridges through C4CPC on a real CPC6128 Plus and supplied
five RetroTINK 4K captures. The visible SHAKER titles are **SHAKER KILLER** and
**R52 BIT 5 RESET**; `01F0` and `01F1` are their automation identifiers, not
additional screens that the cartridge must display. All five supplied screens
are usable. No further capture is required for these comparisons.

The MRER cartridge measures the interrupted PC over 64 phase choices, with four
independently synchronized repetitions per phase, for R3 low nibbles 8 and 12.
One phase step is one NOP, nominally 1 microsecond at 4 MHz. This is a software
phase index, without an independent calibration of the internal HS sampling
aperture. The threshold cartridge counts accepted interrupts over 64 complete
PB0 periods after settling; it repeats each geometry three times. Neither probe
contains the expected result in its measurement path.

## Targeted MiSTer before and after

| Measured result | Preceding model | Corrected candidate | Real Plus |
|---|---:|---:|---|
| BR `01F0`, actual return-PC bytes | 43/48 match | 48/48 match | Five C4 results; other 43 CC |
| BR `01F1`, measured timing records | 15/18 match | 18/18 match | All 18 records transcribed |
| MRER, R3=08h, absolute PC words | 63/64 match | 64/64 match | 0CCD through P=0D; 0D0D from P=0E |
| MRER, R3=0Ch, absolute PC words | 63/64 match | 64/64 match | 0CD1 through P=11; 0D11 from P=12 |
| VS threshold, all three geometries and repetitions | 9/9 match | 9/9 match | 31/32/33 lines: 0/0/64 interrupts |

The five BR byte differences are `ED A1`, `2B`, `FD 2B`, `ED 57` and `ED 5F`:
the preceding model prints CC where the physical Plus and candidate print C4.
The MRER differences occur at P=0D and P=11 respectively. All 128 MRER
repeat-difference bytes are zero before and after. Threshold repeat-difference
fields are also zero, with no timeout result. Its LP poll-loop measurements are
retained separately; differing LP values are not an interrupt-count failure.

These are cold cartridge runs with normal WAIT and original navigation, not a
new full SHAKER baseline. Standard-font BR decoding round-trips all 2,000 cells
of each screen. MRER/threshold numeric decoding uses their own source font and
an unambiguous hexadecimal alphabet. It does not force photographed expected
values onto ambiguous glyphs.

## Exact experimental build and limits

The after RBF is `Amstrad_20261107_ae8bc6c.rbf`, source
`ae8bc6c6059cbe38990272a9d311d155bb36a41d`, 3,167,520 bytes, SHA256
`c0e3b335df53272c9d19e5e3c0cc3fc0185c3fa7f972696929c0a91f8d50aa15`.
It is on `codex/c3-br-private-hardware-20261107`, without V1. Its full Quartus
17.0.2 fit closes all seven setup/hold domains, minimum slack 0.215/0.242 ns.
The production-T80 CI job passes. The overall workflow remains red because
existing fast tests assume the preceding raw IRQ phase. Those failures are
preserved and need justified replacement coverage before READY; hardware
matches do not waive them.

The before RBF is the passive `0c9f6c8` observer over the accepted functional
`cb3a1a6` model, SHA256
`6d43ff06a0494b0ed2b7fc2d8ca66a1fb93edd3925dcb51a2750a82c7c417806`.
All three after capture runs finish successfully and restore the original
MiSTer CFG. The original SHAKER body and firmware remain private.
The manifests request Plus/CRTC3, Full filtering and normal WAIT. Runtime mode
latches are not independently read back by these native PNGs; complete source
titles/results and the restored CFG are the recorded configuration evidence.

The candidate retains the bounded distinction between ordinary delayed counting
and VS rephase, labelled `MODEL LIMIT (B26)` at the implementation. A PRI write
inside that interval, minimum-line overlaps and snapshot history remain distinct
questions. The measurements select between the tested complete programs; they
do not by themselves prove the electrical clock or eliminate competing models.

Private source evidence is in `local/real-plus-results-20261107/`: unchanged
original PNGs, `PROVENANCE.json`, `TRANSCRIPTION.json`, and
`NATIVE-AE8-QUALIFICATION.{md,json}`. Caller sources/captures live in
`local/real-plus-{probes,shaker-b,vs-threshold}-20261106/`. The build and review
receipts are under
`local/overnight-20261103/resumed-20261104/c3-br-private-hardware-20261107/`.

## AmSpirit threshold discrepancy

Two independent cold runs of AmSpirit Lite 1.17.1, configured as 6128 Plus,
CRTC3, 128 KiB, French ROM and monitor response off, give **0/64/64** interrupts
for 31/32/33 lines, each repeated three times. Real Plus and both MiSTer builds
give **0/0/64**. This is a focused disagreement at 32 lines; no general claim
about all interrupt timing follows from it.

A French report and a self-contained reproduction package are prepared in
`local/amspirit-vs-threshold-bugreport-20261107/` and its sibling ZIP. The own-code
CPR, `vsthreshold.asm`, `probe.inc`, symbols, portable rebuild script and both
screenshots are included. The rebuilt CPR is byte-identical. No firmware ROM,
SHAKER disk or original Module B cartridge is required. No report has been sent.
