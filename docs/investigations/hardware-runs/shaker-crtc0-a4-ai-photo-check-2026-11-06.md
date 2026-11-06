# CRTC0 A4 and AI: current photo comparison

Two full-resolution comparisons qualify one additional passing page and one
additional broken page on exact `fdf348c`. This examines previously unknown
coverage; it does not establish a regression from an earlier build. No RTL or
timing change is selected.

| Page | Original-CPC photograph | Current native result | Judgment |
|---|---|---|---|
| A4 `0037` | First/second OUTI scalar `01/00` | `00/00` | Broken, with program/photo binding caveat |
| AI `008D` | All sixteen displayed VS bits zero, PPI `5E` | All sixteen zero, PPI `FE` | Passing under the documented upper-PPI-input allowance |

The AI difference is exactly `A0` in the unrelated upper input bits; none of
the sixteen VS bit-zero observations differs. All four parameter blocks and
their text are readable. This is distinct from the known CRTC1 AI mismatch.
The A4 scalar is already a low-bit result, so upper PPI input differences cannot
explain it. [A4 original photo](https://shaker.logonsystem.eu/images/cpc/CPC/A4_CRTC0_A.webp),
[AI original photo](https://shaker.logonsystem.eu/images/cpc/CPC/AI_CRTC0_A.webp).

## Useful current CPCSyntaxError discriminator

Two fresh isolated CPCSyntaxError v0.12.0 runs use normal boot and direct key 4,
once on each locally available 2.6/2.7 disk. Both complete `0037` and print
`01/00` for CRTC0. Their readable complete pages are RGB-identical. Both include
the extra C9 sweep row absent from the physical photograph. The matching
executable/source pins are unchanged from the [toolbelt](../../cpcsyntaxerror-testing.md).
Default packaged UK firmware, no errata and no chaining are explicit; these
are peer results, not firmware equivalence to native French boot.

All four images are viewed. The 2.6 marker image and 2.7 later image are readable;
the other acquisition-phase images are black and retained without declaring an
execution failure. On CRTC1 the corresponding qualified version pair remains
`00/00`, so this peer helps distinguish the type0 path rather than supplying a
universal successful OUTI model.

An additional instrumentation-only original 2.7 pair records the first R0 OUTI
at source PC `7192`, C0=0/C4=1/C9=1, on both peer types. Immediate R0 restoration
at `7194` sees C0=3 on type0 and 67 on type1. Its UMC-specific counter unwrap
explains that source difference; the character scheduler does not measure an
FPGA or physical sampling aperture. Trace PC is the I/O-dispatch PC after the
opcode, not a production-T80 M1 address.

French ACCC §13.6.1/§13.6.2 p124 is checked in the rendered figure: the specified
OUTI starting at 3C wraps to 0 on type0, while type1 continues to 40. This
conditional distinction must not be replaced by a shared guessed delay. The
original FPGA type0 caller still needs its own qualified PPI/M1/CCLK ledger;
the existing type1 ledger alone cannot locate the type0 operation in that figure.
No E-fall-only register policy or numeric delay follows from the pictures.

Astra's focused read-only audit supplies a stronger phase anchor for the
existing CRTC1 receipts. The last failed and successful drifting IN are 19969
microseconds apart, with the branch taking the original success path. French
§7.2 p39 defines C0vs from the fourth IN microsecond. Corresponding second-opcode
M1 points remove the first-fetch wait mismatch: IN's ED→78 interval is80 clocks,
OUTI's ED→A3 is64. The two target-minus-IN intervals are exactly25151/25150
microseconds; subtract3 and reduce modulo64 to obtain3C/3B. Both CRTC1
continuations to40 agree with §13.6.2. The parent reads the French §4.4.4
pp27–28 alignment explanation and checks the arithmetic. This operational
calibration does not identify a physical data aperture.

Apply the same guards to the original CRTC0 caller before modifying behaviour.
If its first call qualifies as3C yet continues40, the French type0 assertion is
failing independently of the photograph. If it starts in another bin, trace
synchronizer/CPU alignment first. Preserve both original calls and immediate
restoration. The focused report is `c0-a4-aperture-judgment-20261106/` under the
private evidence root. RTL remains on hold pending that diagnostic.

The photograph's exact program identity remains unresolved. The available
2.6/2.7 pair eliminates a simple difference between those local disks for this
peer, without identifying an older reference binary. The existing unsent author
question is updated with this distinction.

## Evidence

The current native frame comes from the canonical CRTC0 A milestone walk;
its SSM record confirms applied Full mode. The RBF hash remains
`17466675860ee115418e712c55250a866c1b2f45bc9a796fd7f314a86d1cbf0d`.
The maintained ledger records these two judgments independently of the other
current but unadjudicated captures.

Private evidence beneath `local/overnight-20261103/resumed-20261104/`:

- `milestone-release-20261106/inspection/crtc0-A/focused-photo-0037-008d/INPUTS.json`:
  original/native hashes and paths; parent views both full-resolution pairs.
- `c0-a4-version-pair-20261106/`: frozen executable/source/media/script/ROM
  identities, two completed runs, all four images and parent RGB comparison.
- `a4-cpcse-type-pair-trace-20261106/`: original type0/1 instrumented runs;
  no emulator-source modification, phase fitting or guest patch.

Original photographs and firmware remain private and are excluded from the
milestone archive. No simulation gate is required for these documentation and
visual-judgment changes.
