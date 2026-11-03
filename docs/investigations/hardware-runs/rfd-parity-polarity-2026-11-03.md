# CRTC 1 RFD save parity polarity — 2026-11-03

## Position

With RFD parity management armed, **even ParityC9 now suppresses the VMA' save
(case 1) and odd ParityC9 permits it (case 2)**. The previous gate had the
opposite polarity. The change is one term in `rtl/crtc_type1_engine.v`
(`row_addr_save`). On the MiSTer it fixes DSC4, clears most of Amazing's lower
corruption, and makes SHAKER 2.7 module B COPY match the Logon System
photographs where it previously did not.

## Rule and the source tension

French ACCC v1.11 §11.6.2 pp.90-91 gives an operational recipe. An IVM ON/OFF
(`OUT R8,3` then `OUT R8,0`, odd R9, on an even C9) fixes **even** parity.
ON/OFF **before** an RFD makes every frame case 1: VMA' is not saved and
R12/R13 stays the line source. ON/OFF **after** the RFD gives case 2 from the
next frame, once the origin toggle has made the even parity odd. Both sentences
need even parity to mean case 1.

§11.6.1 p.90 says case 1 fails "à cause de la parité impaire". The D6 repair of
2026-09-11 implemented that sentence literally against the ParityC9 bit and
treated the §11.6.2 recipe as secondary prose. This change reads "impaire" in
§11.6.1 as the frame numbering ("premier frame"), not the ParityC9 bit value.
That reading is chosen because every higher-ranked source below agrees with it.
It remains an interpretation of the text, not an author clarification; the
French author-question draft can carry it.

The prior [DSC4 evidence](dsc4-video-2026-10-29.md) had already shown that the
same one-line counterfactual makes both §11.6.2 recipes pass in a private
production-T80 replay, and that CPCSyntaxError (suppress on even frame parity)
and AmSpirit (coherent DSC4) agree with it. It stopped short of a production
change because no hardware discriminator had been run. This unit ran it.

## Hardware evidence

Device `root@mister`, classic 6128, Plus off, CRTC 1, Full sync filter,
French ROM. CFG `Amstrad.CFG.full`, SHA-256 `2e585b4c…93d8e4`, left active.

| RBF | SHA-256 |
|---|---|
| Baseline `Amstrad_20261029_46f8ab8.rbf` (current master RTL) | `e448afff…c3ff553` |
| Change `Amstrad_20261103_eba4dc8.rbf` (hosted full build, run 41588560104) | `5b79396a…79eff3` |

The `eba4dc8` RTL is identical to the branch head; only simulation sources and
documentation changed afterwards.

| Target | Baseline 46f8ab8 | Change eba4dc8 |
|---|---|---|
| DSC4 (`dsc4-alt.dsk`, `RUN"-DSC4`, 52 s) | fragmented, rolling image | coherent otter image with its intended line wobble, as in AmSpirit |
| Amazing (`amazing2021_rev2.dsk`, 30 s + 8 s steps, 4 frames) | lower region corrupted in 3 of 4 | clean in 3 of 4; one capture still shows the dense lower pattern |
| SHAKER 2.7 B COPY (`01D4`–`01D9`) vs Logon photos | A, B, C2, D mismatch, inverted | A, B, C1, D, E match; C2 differs |

COPY test C performs no ON/OFF, so its parity alternates between frames and the
two photographs C1/C2 record the two phases. Both native captures (`01D6`,
`01D7`) landed on the C1 phase. This is consistent with the rule but does not
prove C2; a capture series inside test C would. BCTRL (`01DA`–`01DC`) is an
interactive scanner whose captured C0 position differs from the photographs,
so it is not used as a discriminator.

Private evidence, ignored, in the main clone:
`local/investigations/demo-recheck-46f8ab8-2026-11-03/` (baseline captures,
COPY CSL excerpt `csl/RFDB.CSL`, downloaded photographs, comparison grids) and
`local/investigations/rfd-polarity-eba4dc8-2026-11-03/` (change captures,
grids). `local/rbf/rfd-eba4dc8/` holds the RBF and its build reports.

## Simulation

The six vectors that encoded the old polarity were re-derived on paper from
§11.6.2 and the fixture geometry. `t13b`, `t13f`, `t13g` and `t13n` now enter
case 1 at even parity after two whole frames from reset; `t13b` then expects
odd parity to select case 2. D6 repeats RFD then ON/OFF each frame: the even
remainder of the frame keeps reloading R12/R13 (`0x1000`) on every row, and
the following odd frame without ON/OFF saves VMA' = base + R1 = `0x1028`, which
row 1 consumes.

- `make -C sim crtc-test`: the six fail on the old gate (229/235) and all 235
  pass with the change.
- `python3 sim/select_tests.py --run`: `select_tests: PASS 3 benches:
  crtc-test, crtc-cpu-phase-test, ga40010-test`.
- Soak re-minted to `0x3e1301080dd3b3ef` (intended behaviour change).

Cross-provider review (Sol 6.1 medium, run `20261103T092722Z-48860-0572`):
CLEAR. It checked the paper derivations, ParityC9 versus ParityFrame for the
gate, and remaining consumers. Its two notes (soak row, a §19.8.1→§19.8.2
citation for CRTC 1) are applied.

## Open residuals

- Amazing still shows the lower pattern in one of four captures. Characterise
  which scene, then compare against AmSpirit before assuming the RFD rule.
- COPY C2 phase not captured; take several captures within test C.
- SHAKER D (9) "RFD round 2" (`0393`–`039B`) was run on both RBFs after
  integration (`RFDD.CSL`, captures in the same private folders). Against the
  photographs: G and H (ON/OFF after RFD) now match, where the baseline lost the
  header and showed only two rows. D, E and F (ON/OFF before RFD) mismatch on
  both builds: the photographs repeat "THIS LINE REPEATED" on every row under an
  intact header; the change blanks the header and shows only three rows, the
  baseline shows one repeated row then case-2 rows. I mismatches on both. A1–C1
  (no ON/OFF) are stroboscopic and identical on both builds. The mirrored
  header loss suggests a second RFD lifecycle defect (for example how long
  parity management stays armed); an authentic-code replay is in progress.
- Even-R9 and IVM-active RFD cases rest on the ParityC9 reading only.
