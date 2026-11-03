# CRTC 1 RFD parity management is frame-local — 2026-11-03

## Position

Follow-up to the [same-day polarity fix](rfd-parity-polarity-2026-11-03.md).
Three changes in `rtl/crtc_type1_engine.v` make RFD parity management
frame-local and keyed to ParityFrame:

1. `rfd_parity_flag` clears at the frame origin and on a successful VMA' save.
2. The RFD save gate reads ParityFrame instead of ParityC9.
3. The R0-widening R4 variant arms only on an even ParityFrame; the R9 variant
   still arms on every frame.

On the MiSTer this fixes the last RFD-related mismatches against the Logon
photographs, removes Amazing's flashing lower corruption, and fixes SHAKER D
`03AE`/`03B4` after the module's own history. Each term was shown necessary by
an authentic-code replay.

## Rules

- French ACCC v1.11 §11.6.1 p.90: the RFD save test uses "la parité courante du
  frame". With even R9, ParityC9 toggles at every row end (§19.5.3 p.209), so it
  is not the frame parity.
- §11.6.2 p.91: once an RFD activates parity management, it "ne peut plus être
  désactivé tant que le frame n'est pas terminé". So management ends with the
  frame.
- §13.7.1.2 p.126: on the R9 variant, R12/R13 are taken and the line repeats
  one frame in two. On the R4 variant, R12/R13 are taken one frame in two.

**Retiring on a successful save is an inference.** The p.91 wording speaks of
the frame end. The photographed G/H pages (IVM ON/OFF after the RFD) need the
save to stay effective after a later ON/OFF in the same frame. Two models fit
equally: retire on save (CPCSyntaxError's model, implemented), or latch the
parity when the RFD arms. No current evidence distinguishes them. The
cross-provider review flagged this as P2; it needs a real-CPC sequence that
separates the two models.

## Diagnosis by replay

The SHAKER 2.6 module D binary ran at test 9 ("CRTC 1 RFD ROUND 2") on
production T80, GA and CRTC for six frames per page. On the previous RTL it
reproduces every MiSTer capture exactly. Removing any one of the three terms
breaks at least one page:

- Without the origin clear, D/E/F show the repeat on every row, header included.
- With the ParityC9 gate, the even-R9 override rows let the save through and
  D/E/F show only two or three repeats.
- Without retire-on-save, G/H show "2ND LINE ALONE" five times.
- Without the R4 even-frame gate, I shows the rupture instead of a single line.

Clearing `rfd_vma_flag` at the frame origin changes nothing here and is not
done. The harness, logs, engine variants and decode notes are private:
`local/investigations/rfd-lifetime-07bac19-2026-11-03/d9-replay/` in the main
clone (`build.sh`, `d9_test.cpp`, `rows.py`; the Mac build needs `CPATH` as in
`sim/diagnostics/shaker_b9_mid/README.md`).

The polarity report wrongly stated that G and H matched after the first fix.
Their photographs show each line once followed by blank rows; that build
showed "2ND LINE ALONE" five times.

## Hardware results

Device `root@mister`, classic 6128, Plus off, CRTC 1, Full sync, French ROM,
CFG `Amstrad.CFG.full` (SHA-256 `2e585b4c…93d8e4`) left active.
RBF `Amstrad_20261103_07bac19.rbf`, SHA-256 `40c465a6…5f6296`, hosted full
build run 41614911897 (setup slack +0.593 ns). Its RTL equals the branch head.

| Target | Master `918237f` | This change |
|---|---|---|
| DSC4 | coherent | coherent |
| Amazing, 10 captures 4 s apart | lower region corrupted in 5 of 10 | clean in 10 of 10 |
| SHAKER D9 A1–I vs photographs | D, E, F, G, H, I mismatch | all match; A1/C1 show their flashing phases |
| SHAKER B COPY A–E vs photographs | A, B, C1, D, E match; C2 phase not seen | all match; C1 and C2 phases both captured |
| SHAKER D9 then E (`03AE`, `03B4`) | "YOUR EMULATOR IS WRONG" | "YOUR EMULATOR IS OK" |

The D9→E run reproduces the historical full-walk `03AE`/`03B4` WRONG result
from D9's leftover state alone, without the D1 `0305` stall that stops the
scripted full walk. Private captures and grids:
`local/investigations/rfd-lifetime-07bac19-2026-11-03/`, scripts under
`local/investigations/demo-recheck-46f8ab8-2026-11-03/csl/`.

## Simulation

- `t13b`, `t13f`, `t13g` now expect parity management cleared at the frame
  origin. New `t13o` (R4 variant on an odd frame does not arm), `t13p` (even R9:
  ParityC9 odd, ParityFrame even, save still suppressed) and `t13q` (a
  successful save retires management, so a later ON/OFF does not re-suppress).
  All six fail on the previous engine; 238/238 pass.
- `python3 sim/select_tests.py --run`: `select_tests: PASS 3 benches:
  crtc-test, crtc-cpu-phase-test, ga40010-test`. `make -C sim lint` passes.
- Soak re-minted to `0x1d57d12f7c83810a`.

Cross-provider review (Sol 6.1 medium, run `20261103T102910Z-69992-5a84`):
one P2, the retire-on-save inference above. It found the same-edge arm
priority, the R0-extension parity timing, the IVM-active gate and the
persistent `rfd_vma_flag` consistent.

## Open

- Retire-on-save versus latch-at-arm (above).
- The R4 variant's other frame ("C4 n'est pas remis à 0") follows from the
  ordinary C4 != R4 path and is not separately pinned on hardware.
- A full SHAKER D walk still stops at D1 `0305` with the scripted input.
