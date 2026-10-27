# SHAKER CRTC 1 divergences: decoded test sequences (2026-10-27)

This is groundwork for the four [hardware-confirmed](shaker-hw-photo-compare-2026-10-27.md)
CRTC 1 divergences that are still open: `019D`, `03AE`/`03B4` and `008D`. `01F4` is fixed in
[crtc1-infinite-hsync-01f4](crtc1-infinite-hsync-01f4-2026-10-27.md).

## Method

Each test was read from the bytes of the SHAKER 2.7 disk (`shaker27.dsk`).

- Extraction used `scripts/hardware-loop/shaker_ssm_inventory.py` (`read_tracks`,
  `extract`, `amsdos_body`), and disassembly used `z80dasm`.
- Page ids are computed at runtime, so each test was found through its module's dispatch
  table and its on-screen strings.
- Timing uses the CPC NOP table: `OUT (C),r` takes 4 µs with I/O at about +3, `OUTI` takes
  5 µs with I/O at the 5th µs, and `IN A,(C)` takes 4 µs.
- All modules share a sync routine (`9F52` in B, `A241` in D, `A487` in A). It locks onto
  the VSYNC rising edge in 1 µs steps, one step per frame, and returns about one frame
  later.
- Default registers (from the module tables) are R0..R9 = `3F 28 2E 8E 26 00 19 1E 00 07`
  and R12/R13 = `3000`, so VSYNC starts on line 240.

What is verified and what is inferred:

- **Verified from the bytes:** the instruction streams and register values below.
- **Inferred, about ±1 µs:** absolute C0 positions, from the sync arithmetic. No trace on
  a machine has confirmed them.

## `019D`: B6/A "OUTI ON R7 LAST CHANCE 5TH uSec ON C0=0" (`SHAKE27B.BIN`)

The test entry is `8B09` and the measurement is in `sub_8D4C`.

**Setup.** The test writes only R3=6 (5 on type 3 and above). R0 is left at `3F`.

**Measurement:**

1. Select R7, then sync.
2. Delay so that the `OUTI` I/O writes **R7=`0x20`** exactly 21504 µs after the VSYNC
   sample. That is one 312-line frame plus 24 lines, at the C0 phase of the VSYNC start.
3. Read PPI port B 6 µs later. The result is `0x1E | bit 0`.

**Expected values.** The 5TH variant expects `#1E` (no VSYNC) on every type. The 4TH
variant (`DE=05EE`, 1 µs earlier) expects `#1F` on types 1 and 2 and `#1E` on the others.
Real CRTC 1 reads `#1E` for the 5TH variant and the core reads `#1F`.

**Open question.** The reading against the rule is not settled. With R7=30 and R9=7,
"24 lines later" is line 264 (C4=33, C9=0). R7 is then set to 32. C4=32 began on line 256,
which is not the current row, so a plain "R7=C4 write triggers immediately" rule does not
apply here.

The core reading VSYNC high means its VSYNC is active at that sample. That may be the old
R7=30 pulse, which starts at line 240 and lasts R3h=0 (16 lines), so it ends at line 256.
Or it may be a new pulse.

**Pin-bench result (2026-10-27, probe not committed).** The type-1 CRTC alone follows the
expected rule: R7 is 32, the frame number of row 32.

- An R7=32 write during the last character of C4=32 (C9=7, C0=63) raises VSYNC at once. That
  is the `#1F` the 4TH variant expects.
- The same write at C0=0 of C4=33 does not raise VSYNC. That is the `#1E` the 5TH variant
  expects.

So the CRTC rule is right, and the `019D` divergence comes from how the integrated machine
places the `OUTI` write relative to C0. In the core, the 5TH-µs `OUTI` write takes effect one
character early, inside C4=32.

**Supporting evidence:**

- French ACCC v1.11 §13.3 p.114 and §13.7.1.1 p.125: on CRTC 1, an `OUTI` write takes effect
  one µs later than an `OUT (C),r8` write ("R0 is actually modified 6 µs after the start of
  OUTI").
- SHAKER module A test (4): the real CRTC 1 photograph reads `OUTI ON C0vs=#3c:01` (I/O on the
  5th NOP), where the core reads `00` (see `docs/classic/accc-author-feedback.md` §4).
- `docs/classic/audit-findings.md` already names the residual: "the current bus interface
  cannot distinguish `OUTI` from an otherwise identical write".

**Next step.** In the production-T80 bench (`sim/crtc_t80_test.cpp`), compare the CRTC data
latch edge for `OUTI` against `OUT (C),r`. The question is whether CRTC 1 latches at the end of
the I/O cycle, which for `OUTI` falls in the next µs once GA wait states are counted, rather
than at the start. Then write a failing type-1 vector from the `019D` sequence: sync on VSYNC,
`OUTI` R7=`0x20` at +21504 µs, PPI read 6 µs later, expect bit 0 low.

## `03AE` / `03B4`: DE/A and DE/C "VMA=R12/R13 UPD WHEN C4=0" (`SHAKE27D.BIN`)

There is one shared body at `49BB`. The entries are `4966` (A, `03AE`), `4989` (B) and
`4994` (C, `03B4`), and each patches operands in the body.

**Verdict.** The verdict is visual. The screen is drawn twice:

- at base `0x3000`, where row 1 reads "…YOUR EMULATOR IS WRONG";
- at base `0x3190`, where row 1 reads "…IS OK".

Real CRTC 1 shows the second, so display row 1 must start from the R12/R13 value that was
written during row 0.

The timeline below is in µs from frame line 0. It is synced to the start of VSYNC, plus 72
lines.

| µs | Line | A (`03AE`) | C (`03B4`) |
|---|---|---|---|
| 11 | 0 | R4=0 | R4=0 |
| 23, 29 | 0 | R9=0, R9=0 | R9=0, R9=0 |
| 77 | 1 | R9=6 | R9=6 |
| 92 / 104 | 1 | R12/R13=`3190` | R12/R13=`3000` (no-op) |
| 151 / 163 | 2 | R12/R13=`3190` | R12/R13=`3190` |
| 241 | 3 | R4=`0x26` | R4=`0x26` |
| 583–609 | 9 | R9=7, R12/R13=`3000` | same |

Line 0 ends a one-line frame (R4=R9=0), so line 1 starts a new frame at C4=0, C9=0. Row 0
then runs lines 1–7 (C9 0..6). In variant C the real update lands on line 2 (C9=1). The
expected value therefore depends on how CRTC 1 takes R12/R13 into account on every line
while C4=0. The type-1 engine already has a row-0 reload path (`e1_row0_reload`).

**Pin-bench result (2026-10-27, probe not committed).** I replayed the table above on the
type-1 CRTC alone. Writes land at the listed µs from a C4=C9=C0=0 frame start, with R12/R13
starting at `3000`.

- Variant A takes `3190` on line 2 (C9=1). Variant C takes it on line 3 (C9=2).
- Both start row 1 at `0x3190 + 40`, which is the **OK** copy that real CRTC 1 shows.

The CRTC rule, as modelled, already matches hardware. The MiSTer **WRONG** therefore comes
from the integrated machine. Either the CPU write phase lands differently relative to C0, or
the test's VSYNC lock aligns to a different line than assumed.

**Next step.** Replay the module D entry in the production-T80 bench, or take a MiSTer SNA on
the `03AE` page, to get the real C4/C9/C0 at each write. Compare that with the table above.

## `008D`: AI/A "LOST VSYNC ON R0=0/1" (`SHAKE27A.BIN`)

There is one shared body at `6400`, reached through `6491(a)`. Each block writes R0 and
stores four PPI port B reads (S1..S4). Times below are relative to the nominal VSYNC start
(line 240, C0=0).

| rel µs | Write or read |
|---|---|
| −63 | **R0=a** (0 or 1); line 239 is C4=29, C9=7=R9, C0≈1 |
| −58 | S1 |
| −45 | R0=`0x2D` (`0x2F` when a=3) |
| −40 | S2 ("EVT R0>2 SAME LINE") |
| −22 | R2=`0x1C` (`0x1E`) |
| −1 / −2 | S3 ("R0>2 SAME LINE ON C0=R0") |
| +3 / +2 | S4 ("R0>2 ON C4=30") |

**Expected values.** Real CRTC 1 reads VSYNC high on S1..S4 in both R0=0 blocks. The core
reads it low on S2 and S3.

**Reading so far.** Writing R0=0 (or 1) near C0=0 ends line 239 at once, so C4 becomes
30=R7 and VSYNC starts about 60 µs early. The R0=0 regime that follows makes each character
a new line.

- **Core:** a line-counted VSYNC width of R3h=8 then expires within about 8 µs, before S2.
- **Hardware:** VSYNC survives to S4.

Either CRTC 1 does not count VSYNC-width lines on these 1 µs lines, or the count differs.
This is a hypothesis; the ACCC rule for it has not been located yet.

**Rule search (2026-10-27).** French §16.4.2 (p.170, CRTC 1) and §13.3 say only that R0=0
causes no counter problem on CRTC 1. Neither says what advances the VSYNC-width counter C3h on
1 µs lines.

If C3h counts line ends, as the core does, VSYNC ends about 8 µs after the R0=0 write, before
S2. Hardware keeps it high through S3, about 60 µs later.

One hypothesis fits all four rows: on CRTC 1, C3h advances at HSYNC rather than at the C0=R0
line end. While R0 (0, then `0x2D`) is below R2=`0x2E`, no HSYNC occurs at all; the first one
comes only after R2 is set to `0x1C`.

This is not a source rule. It would change type-1 VSYNC width wherever R2 > R0, so it needs a
discriminating hardware vector or an author answer before any RTL change. It is filed as author
question 22.
