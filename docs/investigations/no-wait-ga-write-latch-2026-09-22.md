# Gate Array writes dropped under no_wait: a TV80 artifact (2026-09-22)

Branch `plus/ga-fast-write-latch`. This records why a suspected Gate Array defect turned out to
be a bench-CPU timing difference, what was measured, and what changed.

## Outcome

- **No RTL defect.** The production CPU (T80pa) with the unmodified Gate Array lands every
  register write under no_wait. `ga40010.sv`, `asic_ga_timing.v` and the motherboard are
  unchanged.
- **Root cause:** the TV80 bench CPU used by the P10/B6/B7 fixtures skipped the Z80's automatic
  I/O wait state. Its IORQ window was 6 GA states instead of 10, short enough to miss the
  Gate Array's latch states.
- **Fix:** `sim/plus/tv80/tv80_core.v` now takes one automatic wait state on I/O cycles when
  `IOWait` is set. The parameter already existed but was never used. Its I/O windows now
  match T80pa exactly.
- **Consequences** (see the last section): the `production_wait` fixture switch added by
  `plus/sonic-cpu-cart-latency` is gone, and one slow bench (`b6-dynamic`, type-1 CRTC) now
  fails. That failure is recorded as backlog B22.

## What fast mode (no_wait) is

OSD option `CPU timings: Original / Fast` (`Amstrad.sv`, `P2O6`, `status[6]`), wired as
`no_wait = status[6] & ~tape_motor`: it switches off automatically while the tape motor runs.

On a real CPC the Gate Array holds the Z80's WAIT line (READY) so every memory and I/O cycle
lines up with the GA's fixed 1 µs bus slot. The CPU loses cycles to that alignment. The core
feeds the T80 `wait_n = (ready | (IORQ_n & MREQ_n) | no_wait) & ...`, so no_wait ignores READY
and the CPU runs unstalled at the full 4 MHz. No real CPC behaves like this. The same signal
drives the `fast` input of both `ga40010` and `asic_ga_timing`.

## The Gate Array logic involved

| Line | Origin |
|---|---|
| `reg_sel = S[0] & S[7] & ~IORQ_N & ~A[15] & A[14] & M1_N` | Decap-derived GA, commit `489f340` (2020-07-02, sorgelig), following "40010 simplified" V03 by Gerald |
| `reg_latch = (S[0] & S[7]) \| (fast & ~E244_N)` | Commit `31634e3` (2020-07-24, skywalky, "Re-add fast CPU mode"); a MiSTer addition, not in the chip |
| `asic_ga_timing.v` register latch | Plus-stream copy, pinned in lockstep by `asic_ga_timing_diff_tests` |

In the reference schematic (`rtl/GA40010/40010-simplified_V03.pdf`, sheet 1, Registers block)
the register file's only inputs are RESET, IORQ, M1, A15, A14, S0, S7 and D[7:0]. The GA has no
WR pin. The 16-state ring is FF, FE, FC, F8, F0, E0, C0, 80, 00, 01, 03, 07, 0F, 1F, 3F, 7F.
S2&S3 (the E244 window) holds for 7 consecutive states, 0F to FC, leaving a 9-state gap. The
fast term therefore needs an IORQ window of at least 10 states to be guaranteed a latch.

On the classic board the GA's D pins share the VRAM bus. The CPU byte reaches them through the
74LS244 only during E244 (`ga_din = e244_n ? vram_d : D`), which is why the fast term cannot
simply be widened on the classic path.

## Measurements

The harness was a scratch copy of `sim/crtc_t80_top.sv`: real CPU, real classic `ga40010` with
`fast=1`, `cpu_wait_n` tied high, and `MODE` exposed. The CPU loop was
`LD BC,&7F00 / LD A,&82 / OUT (C),A / LD A,&83 / OUT (C),A / LD A,0 / JR`. Each iteration takes
57 T-states, so successive iterations cover all four CPU-to-ring phases. Each IORQ window with
M1 high was logged, and the GA mode checked afterwards.

| CPU | IORQ window | Windows observed (GA states) | Dropped writes |
|---|---|---|---|
| Production T80pa (GHDL netlist) | 40 clks = 10 states | FE..03, 1F..C0, E0..3F, 01..FC | 0 / 400 |
| TV80, before fix (`master` `03f4724`) | 24 clks = 6 states | FE..C0, 01..3F, 1F..FC, E0..03 | 12 / 50 (every E0..03 window) |
| TV80, after fix | 40 clks = 10 states | identical to T80pa | 0 / 50 |

The TV80 E0..03 window is exactly the one reported from `test_p10b_video_coherence_pixel`
(`OUT (C),A` to &7F00, D=&82, mode stayed 0). A real Z80 holds IORQ from the rising edge of T2
through the falling edge of T3, with one automatic TW, for 2.5 T-states. T80 inserts that wait
(`Auto_Wait_t1 <= Auto_Wait or IORQ_i`, `rtl/T80/T80.vhd`), and the local TV80 port did not.

The harness was not committed. Its only failure mode would be someone removing the I/O wait
from TV80, and `test_p10b_video_coherence_pixel`, now back under no_wait, already fails in that
case. B23 proposes a general TV80-vs-T80pa parity bench instead.

To reproduce, copy `sim/crtc_t80_top.sv` and make three edits: `cpu_wait_n = 1'b1`,
`.fast(1'b1)`, and `.MODE(ga_mode)` as an output. For T80pa, build with the `CRTC_T80_RTL`
list from `sim/Makefile` and `make -C sim t80-netlist`. For TV80, lower-case the CPU port names
and build the `sim/plus/tv80/*.v` files with `--language 1364-2001`.

## An approach that was reverted

A first attempt widened the fast latch in both GA modules to the whole IORQ window except the
two video-fetch clocks, and widened the classic `ga_din` mux to match. It was correct at module
level, but it added permanent MiSTer-only logic to the decap-derived GA and to the classic data
mux. That guarded against a CPU the core never ships, and it left TV80's I/O timing wrong for
every other I/O-phase-sensitive block (CRTC, PPI, ASIC registers). It was never merged or pushed;
the branch holding it was deleted.

## Consequences of the TV80 fix

- `sim/plus/p10_boot_test_top.v`: the `production_wait` input is removed. The fixture is back
  to a fixed `no_wait=1`, and `test_p10b_video_coherence_pixel` passes under it again.
  `sim/plus/prepare_d5_boot.py` now rewrites `.no_wait(1'b1)` instead.
- Every TV80 I/O cycle is one T-state longer, so fixture programs shift in time.
  `run/p10_boot_tests`, `ssm-marker-test`, `b6-video-boundary`, `b6-plus-layers`,
  `b7-dark-silicon-audit` and `d5-cart-timing` pass.
- `b6-dynamic` fails for machine 1 (type-1 CRTC) only: its short-HSYNC stage records no shifted
  fetches. A likely cause, not yet verified, is history-dependent state in `crt_filter`
  (sticky `hs4`). This is backlog **B22**; do not re-tune the program to hide it.
