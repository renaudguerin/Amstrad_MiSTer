# B20 acknowledge-vector discriminators (test-only, 2026-09-22)

Two slow-tier diagnostics. No RTL, `Amstrad.sv`, or T80 change; no PPR/DMA
scheduling change (other tasks own those in independent worktrees).
Both benches exit 0 on current-model behavior, so the default (fast) gate
stays green; the A1 source mismatch is reported as an explicit
EXPECTED-MISMATCH line, not a pass.

## Authority split

Physical claims are cited to S29 (Plus Vectored Interrupt Bug, CPCWiki) and
are never asserted by these benches: two acknowledges seen by the ASIC with
the second defaulting to DMA0/offset 4 (S29 pp.3-4); A13=0 required in the
interrupted instruction/address context (S29 p.3), not as a generic
address-during-acknowledge condition; memory read/write instructions
affected while single-byte instructions including HALT are fine (S29 p.3). CPU-model
assertions are code-based: single IORQ_n fall per accepted interrupt via the
TState-1 shift register with Auto_Wait hold and T3 reset to "11"
(`rtl/T80/T80pa.vhd:181-189`, `rtl/T80/T80.vhd:1338-1341,1352`); M1_n/IORQ_n
rise on the same TState-2 CEN edge (`T80pa.vhd:159-161`,
`T80.vhd:1287-1289`); vector latched at that edge while the acknowledge is
valid (`T80.vhd:503-506`); core WAIT_n tied high (`T80pa.vhd:125`) so
external WAIT only stretches via CEN_pol (`T80pa.vhd:109,172-177`); intack
has no address term (`rtl/Amstrad_motherboard.v:479,683`).

## A: GA/register two-ack discriminator (`make -C sim/plus b20-ack-diag`)

Fixture `sim/plus/b20_ack_diag_test.cpp` reuses `plus_p8_test_top.v` with
testbench-driven `ga_m1_n`/`ga_iorq_n`. That stimulus is synthetic bus
wiggle, not production CPU timing and not the S29 LK106/IC116/A13 board
mechanism -- it pins register-file behavior under acknowledge shapes, not
whether hardware presents them. Setup goes through the production SNA
controller path; no HSYNC stimulus is applied afterwards, so the DMA engine
cannot raise `dma_int_set` concurrently with the acknowledge-edge clear
(`asic_regs.v:430`, comment/code note at `:416`).

Observed 2026-09-22 (all vectors stable across every sampled clock):

- A1, two genuinely distinct acknowledges (full M1+IORQ release between),
  raster restored via SNA B4 with IVR=0x00, no DMA: first `0x06` 8/8, DCSR
  bit 7 0->1, 20-clock idle window quiet; second `0x00` 8/8, DCSR bit 7
  1->0, DMA flags 0. Source-derived expectation for this exact post-raster
  scenario is DMA0/offset 4 (`0x04`, S29 pp.3-4). Verdict: EXPECTED-MISMATCH,
  exit 0 while it persists; any move (first != `0x06`, instability, second
  != `0x00`) fails loudly.
- A2, double pulse in one M1 window (M1 held, IORQ low-high-low), DMA0
  restored with IVR[0]=0: first `0x04` 4/4, gap quiet 3/3, second `0x00`
  4/4; DMA0 auto-cleared on the first edge, DCSR bit 7 stayed 0. All
  required-pass. A joint raster+DMA0 restore is deliberately not constructed:
  the B4 attribution rule credits DMA-explained pending flags to the DMA
  path, so that combination needs live video/DMA stimulus (out of scope).
- Idle probe (informational, no assertion): an isolated acknowledge with
  nothing ever pending gives `0x00` 4/4. S29's DMA0 claim is scoped to the
  post-raster window, so no comparison is made; arbitrary idle semantics
  stay unconstrained and no global fallback change is required or made.

## B: production-T80 matrix (`make -C sim/plus b20-ack-matrix`)

Fixture `sim/plus/b20_ack_matrix_test.cpp` drives the GHDL-synthesized
`sim/obj_dir/t80/T80pa.v` netlist directly (as `sim/t80_trace_test.cpp`
does; CEN tied high, plain CLK). Not the TV80 substitute. INT_n, WAIT_n and
the vector byte (`0x06`, IM2 table `0x0306` -> handler `0x0C00`) are
synthetic harness stimulus; request/clear provenance below is harness
provenance (INT released 2 clocks after the acknowledge rise, mimicking the
GA clear), not hardware evidence. No invented hardware timing is asserted.

Observed 2026-09-22 (one intack rise and one IORQ-fall-while-M1 per accepted
interrupt in every non-race cell; handler entered every time; handler
offsets below are relative to the actual INT assert tick):

| Cell | Rises | IORQ falls | Ack clocks | WAIT in ack | First A | Handler |
|---|---|---|---|---|---|---|
| NOP @ 0x0100, WAIT stretch | 1 | 1 | 4 | 2 | 0x0103 | +42, assert@120 rise@127 release@129 |
| NOP @ 0x2100 | 1 | 1 | 3 | 0 | 0x2103 | +41, assert@120 rise@127 release@129 |
| HALT @ 0x0100 | 1 | 1 | 3 | 0 | 0x0101 | +41, assert@120 rise@127 release@129 |
| LDIR @ 0x0100 | 1 | 1 | 3 | 0 | 0x0109 | +56, exec@159 data@175=0x0800 assert@183 rise@205 release@207 |
| LDIR @ 0x2100 | 1 | 1 | 3 | 0 | 0x2109 | +56, exec@159 data@175=0x0800 assert@183 rise@205 release@207 |
| OUT (C),C @ 0x0100, I/O WAIT x3 | 1 | 1 | 3 | 0 | 0x0105 | +62, assert@120 rise@148 release@150, 2 I/O cycles stretched, 6 WAIT lows |

LDIR cells assert INT 8 ticks after the first observed LDIR data access
(source-range read at 0x0800; opcode fetch at origin+9 seen at +159), so the
acknowledge genuinely interrupts LDIR execution -- first_A is the LDIR
opcode address itself (0x0109/0x2109). The bench fails if no LDIR data
access precedes the acknowledge, so the cells cannot silently revert to
setup-code interrupts. NOP/HALT stay fixed-tick controls at assert@120.
The WAIT stretch lengthened the acknowledge (4 vs 3 clocks) without a second
rise. A[15:0] during intack tracks the interrupted code region (A13 follows
the origin in every pair) and is recorded as I2 evidence with no assertion.
Race sweep (NOP @ 0x0100, INT withdrawn K clocks after assert at 120):
K=1 -> no ack (withdraw@121 release@121 ack@none, 68 fetch marks);
K=2 -> no ack (withdraw@122 release@122 ack@none, 68 fetch marks);
K=3 -> single ack (withdraw@123 release@123 ack@127, 0 fetch marks);
K=4 -> single ack (withdraw@124 release@124 ack@127, 0 fetch marks).
Zero double acknowledges; no empty-acknowledge claim is made (DI is
hard-wired to 0x06 during every intack, so vector provenance after a
withdrawal is untested -- the harness has no pending-source model).
INSN_START marks opcode fetches (an ack replaces the next fetch), so zero
marks before an ack is the normal shape, not a finding.

Separate S29 expectations (not asserted): in the interrupted
instruction/address context, A13=1 origins and the HALT cell should be
immune while memory-op A13=0 origins are susceptible, and a second hardware
acknowledge should vector DMA0. This bench cannot confirm or deny any of
that; it only executes the code-based single-acknowledge half. A safe RTL
correction still needs a hardware bus capture (interrupted PC/opcode, /M1,
/IORQ, /WAIT, INT, vector).

## Index and runs

`sim/TESTS.md` rows (both tier `slow`): `sim/plus b20-ack-diag` covering
`rtl/plus/asic_regs.v` `rtl/plus/asic_ga_timing.v`; `sim/plus b20-ack-matrix`
covering `rtl/T80/*` `rtl/Amstrad_motherboard.v`. Focused runs above pass;
`python3 sim/select_tests.py --run` is the READY gate with the two slow
targets run explicitly alongside it. `B20_TRACE=1` on the matrix binary
writes a per-tick signal log to `B20_TRACE_DIR` (optional, defaults to the
current directory; an unopenable path fails loudly) -- debug aid only, off
by default.


## Validation and disposition

Final focused runs: `make -C sim/plus b20-ack-diag` and
`make -C sim/plus b20-ack-matrix` passed with the documented expected
source mismatch. Trace-file creation and the unopenable-path failure were
also checked. The final `python3 sim/select_tests.py --run` reported
`select_tests: PASS 40 benches`; the two slow diagnostics were run explicitly.
The parent verified the report in bridge run `20260922T070721Z-63060-d35c`.
A fresh independent Astra review of the Muse implementation closed the
instruction-trigger, netlist-dependency, trace-path, and evidence-claim findings.

READY as a test/evidence slice only. B20-2/B20-3 hardware behavior remains
open: the synthetic second acknowledge returns offset 0 against S29's offset 4,
and the tested production CPU sequences have a single acknowledge edge.
Neither result specifies the missing board-level pulse shaping well enough
for a compatibility fix. No synthesis, device validation, or Sonic-causality
claim is made.
