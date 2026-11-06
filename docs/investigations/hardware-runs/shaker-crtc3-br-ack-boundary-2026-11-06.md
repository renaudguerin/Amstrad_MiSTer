# CRTC3 BR CPI: qualified mismatch, ACK cause unresolved

The original resident BR caller preserves twelve preceding passing rows, then
stores CC for CPI where its table and the canonical physical CRTC3 photo show
C4. This reproduces a software-visible defect, but does not identify a justified
IRQ timing change. The native BR01F0/01F1 failures remain in the page tally.

## Original caller and result

One production-T80 run executes the original Module B setup, dispatcher, slot
copies, synchronization and IRQ handlers. The declared resident prelude calls
409B, 9D6D and 9D79 before entering the original table at 4A1F. It retains the
twelve preceding rows: CPI's five-byte slot is ED A1 03 46 00, including bytes
left by earlier tests. A fresh isolated CPI instruction would lose that history.

Rows 1–12 produce the expected CC. Row 13 retires CC to 4BEB against expected
C4 at 4BEA. Actual IRQ return PCs are 4A9B and CCED. CPU reads, writes, stack
frames, copies, mappings and full-RAM replay qualify transport. The analyzer's
numeric RED is accepted under the parent preregistration; a timeout or alternate
handler path would not have qualified. This is a resident-boundary proof, not
a reconstruction of firmware, menu or earlier-module history.

The source tuple uses ASIC-video SHA256
`ad41ed3810050bf8cc88e880e9a24b33f262732e42f065c13bb96ad8d8ee3b4c`,
unchanged across 5c1ab57 and 493cdea. The STATUS2 bit0 candidate changes no IRQ
logic. Evidence under the private session root
`local/overnight-20261103/resumed-20261104/`:

- `c3-br-original-proof-readiness-20261106/runtime/`: frozen inputs, one build,
  one execution, analysis and parent-bound artifacts.
- `c3-br-parent-source-20261106/`: original disassembly and first CPI ACK window.
- `c3-br-ack-source-readiness-20261106/`: source constraint matrix, primary
  schematic/manufacturer receipts and their hashes.
- `c3-physical-reference-discovery-20261106/`: physical-photo binding and reviewed
  CPI C4 field. This does not grade every BR row or assert an exact photo binary.

## What the first ACK establishes

| Event | Master tick | Current-model consequence |
|---|---:|---|
| Compatible INT delivered | 55354314 | Remains pending under DI |
| CPI interrupt recognition | 55485250 | PC/address 4A9B; R52 = 31 |
| Raw CPU ACK starts | 55485290 | READY low; ASIC-side IORQ masked |
| Natural HSYNC counter event | 55485322 | R52 advances to 32 on the following tick |
| READY exposes shaped ACK | 55485326 | Counter clears to 0 on the following tick |
| Raw ACK ends | 55485346 | Address changes from PC to I:R refresh |

The motherboard's IC116 equation matches the original Plus CPU schematic:
`ASIC_IORQ_n = CPU_IORQ_n OR (!READY AND !A13)` in normal operation.
No schematic-supported M1/ACK exception was found. Bypassing that circuit just
for ACK would therefore require new evidence.

Frozen T80/T80pa source keeps PC on the address bus throughout raw ACK. I:R
appears when M1/IORQ release, consistent with the visually checked Zilog Z80 CPU
User Manual UM008011-0816, Figure 9, printed page 13. PC 4A9B supplies A13=0;
changing I cannot expose an earlier ASIC ACK in this digital model. Native
startup I is not asserted known, but it has no direct channel into this ACK.

INT was already delivered 130,936 master ticks before recognition. A small
local change to delivery delay cannot move this acceptance. A global change
could alter earlier synchronization history; that is a different hypothesis
requiring its own evidence.

CPCSyntaxError explicitly clears R52 before its CPU step according to prior
instruction slack, citing the BR/CPI photographs. It does not model the
IC116-shaped ACK pins. Its passing result supplies a peer policy, rather than
independent evidence for the physical ACK edge or a chosen delay.

## Independent CPU contract evidence

Five [qualified NMOS die-netlist runs](z80-die-cpi-wait-ack-2026-11-06.md)
now support two narrow contracts independently of T80: selected CPI internal-T2
WAIT is ignored while operand-T2 WAIT adds one clock, and PC stays actively
driven throughout raw ACK including one external wait. I:R appears at ACK
release. This narrows CPU-policy uncertainty without proving the original Plus
variant, complete BR history or ASIC counter semantics. No timing fix is selected.

## Remaining discriminator

One original-CPC CPI bus capture, reproducing C4 with the original antecedents,
could separate CPU ACK/WAIT timing, READY/HSYNC ordering and ASIC internal clear
semantics. Useful signals are CPUCLK, INT, M1, raw IORQ, ASIC-side NIORQ,
READY/WAIT, A13 and HSYNC, with instruction and synchronization anchors plus
the resulting next IRQ/outcome. Existing simulation explains its own ordering;
another I seed or arbitrary numerical phase scan adds no independent rule.
No production fix is selected by this investigation.
