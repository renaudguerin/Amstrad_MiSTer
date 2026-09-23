# B20-2 production acknowledge boundary

Investigation base: `bca3f4f4a7d145952fb273ab06ce885d781c7ce6`, including the
locally integrated Eerie and Switchblade work. This is a Plus-only investigation.

## Question and authority

S29, *Plus Vectored Interrupt Bug*, captured revision 115513, pp.3–4 describes
an instruction/address-dependent board effect: logic around LK106/IC116 reshapes
IORQ, the ASIC sees two acknowledges, the first clears the raster request, and
the second supplies DMA0/offset 4 with no source pending. The source distinguishes
susceptible memory instructions at A13=0 from A13=1 and single-byte/HALT controls.
Its example vector table and auto-clear wording contain errors already recorded
in [the source findings](scrapes-interrupt-findings-2026-09-22.md); they are not
used as an executable oracle.

The previously integrated [diagnostics](b20-ack-discriminator-2026-09-22.md)
already show the downstream discrepancy under synthetic two-ack stimulus:
`0x06` then `0x00`, versus the reported `0x06` then `0x04`. Their standalone
T80 matrix does not connect the ASIC's request/clear/vector path to the CPU.
The present discriminator addresses that integration gap, not original-hardware
validation.

## Source-checked model boundary

`rtl/Amstrad_motherboard.v` connects the T80's IORQ output directly to `iorq`
through inversion. Both `asic_ga_timing.intack` and `asic_regs.intack` receive
`plus_mode & ~M1_n & iorq`. There is no A13-conditioned reshaping between those
outputs and either ASIC input. The CPU's clock enables and WAIT path can stretch
execution; they do not constitute the missing LK106/IC116 logic.

`asic_regs.v` samples the selected source on the first master-clock edge of an
acknowledge and holds it while the acknowledge remains asserted. The GA clears
the raster request and records raster provenance on that acknowledge. The
vector has priority in the CPU input mux in both `Amstrad.sv` and the reused
P10/D5 fixture. Consequently, clearing the request during one continuous
acknowledge must not be confused with a second acknowledge or justify changing
the vector mid-cycle.

The first structural divergence from the source account is therefore the
board's acknowledge formation, upstream of the empty-source fallback. This
identifies a missing modeled mechanism; it does not establish its exact timing
or justify synthesizing an A13-only compatibility condition.

## Evidence boundary

No MiSTer, AmSpirit, synthesis or original CPC Plus measurement is part of this
investigation. The source-linked schematic and logic-analyser image were not
retrievable through the web tool on 2026-09-23 (CPCWiki page HTTP 403; image
requests inaccessible). No timing rule is inferred from unseen diagrams.

A behavioral fix remains conditional on a source-derived failing vector for the
reachable scenario. Arbitrary idle-acknowledge semantics are unconstrained.

## Discriminator and retention

`make -C sim/plus b20-bus-diag` runs a synthetic CPR through the D5-prepared
production T80 and motherboard/ASIC hierarchy with production clock enables
and WAIT handling. It does not inject an acknowledge pulse or a vector byte.
The program unlocks/maps the ASIC, sets IVR=0 and PRI=24, establishes IM2 with
I=3, and executes LDIR at low/high A13 origins or the low-A13 HALT control.
Vector-table entries distinguish raster (`0x0306 -> 0x0600`), DMA0
(`0x0304 -> 0x0400`) and DMA2/empty (`0x0300 -> 0x0500`); the raster handler
reads DCSR and writes a completion marker.

The diagnostic is retained as an opt-in slow bench because it crosses the
production CPU, clock/WAIT, motherboard, ASIC interrupt and CPU-data-mux
boundaries. The standalone T80 and synthetic-ack benches cannot prove that
interaction. It is a model discriminator with explicitly recorded source
mismatch, not an assertion that original hardware must emit one acknowledge.

## Observed result and validation

The final diagnostic passed these three bounded cells:

| Execution context | Address at acknowledge | Acknowledge clocks | Vector from clock 2 | Handler / DCSR |
| --- | --- | --- | --- | --- |
| LDIR at `0x0109`, A13=0 | `0x0109` | 24 | `0x06`, stable | `0x0600` / `0x80` |
| LDIR at `0x2109`, A13=1 | `0x2109` | 24 | `0x06`, stable | `0x0600` / `0x80` |
| HALT at `0x0100`, A13=0 | `0x0101` | 56 | `0x06`, stable | `0x0600` / `0x80` |

Each cell observed exactly one pulse on the shared acknowledge net. The initial
master-clock observation has vector `0x00`; after the first ASIC register latch
edge, clock 2 is unconditionally checked as `0x06`, and every later clock must
match. This is explicit registered latency, not stability from the instant the
CPU outputs change. Fetching the distinct raster handler proves the CPU's
consumed vector selected that table entry.

Both LDIR cells require pre-acknowledge read/write activity and opcode-fetch
addresses for ED/B0, plus the acknowledge address at the repeating LDIR. The
HALT control requires its fetch and held-PC evidence. The actual GA request
(`plus_ga_int_n`, read-only hierarchy tap) is asserted at acknowledge entry,
clear at clock 2, and stays clear through the handler. No DMA interrupt-set
event is observed, and the handler reads zero DMA flags/enables. DCSR bit 7
is therefore attribution evidence alongside the directly observed request
clear, not a substitute for it.

These executions do **not** reproduce S29's second acknowledge. The downstream
empty-vector mismatch remains as documented by the synthetic diagnostic; no
source-derived failing production scenario establishes an RTL repair. A13 and
instruction class alone do not specify the missing board timing. The next
useful input is the original schematic/logic-analyser capture, with CPU and
ASIC-side IORQ, M1, WAIT, A13, interrupt request and vector sampling aligned.
The three cells do not establish global unreachability or idle-ack semantics.

Final commands:

- `make -C sim/plus b20-bus-diag` — all three cells pass with the source/model
  distinction reported explicitly.
- `python3 sim/select_tests.py --run` — `select_tests: PASS 40 benches`.
- `git diff --check` — clean.

Gemini authored the test; independent Astra review identified and then closed
four evidence defects: delayed-vector tolerance, interrupted-instruction
proof, actual request-clear observation, and overbroad reachability claims.
The parent inspected the final diff and accepted the focused/gate results from
bridge run `20260923T060807Z-38501-5dbc`; no simulation was repeated by review.

**READY as a test/evidence slice. No RTL changed.** B20-2's physical behavior
and compatibility correction remain open. No synthesis or device acceptance
is claimed, and integration belongs to the coordinator after its current run.
