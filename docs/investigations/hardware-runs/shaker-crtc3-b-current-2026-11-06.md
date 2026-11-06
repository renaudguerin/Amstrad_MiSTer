# Current CRTC3 Module B numeric checks — 6 November 2026

On `fdf348c`, B6/page019D passes all six original program checks:
27/14/26/27/1E/1E. Root viewed the complete page. Fresh CPCSyntaxError
v0.12.0, matching source22de765, gives the same six GOOD results through
its original full Module B history. This one page earns a current passing
program-self-check judgment; it does not confer whole-module acceptance.

The same native walk exposes explicit numeric WRONGs on BR/page01F0
(SHAKER KILLER) and01F1 (interrupt delay after R2 bit5 reset). On01F0,
five rows returnCC where the program expectsC4. On01F1, three instruction
groups contain0D03 where the program expects0503; do not transpose this
asD003/D503. The fresh peer returns the programmed expectations on both.
The original Type3 script runs both pages, and the byte-verified callers
supply the disputed expectations. Both pages are therefore broken against
the current program. No matched physical Plus photograph or particular
CPU/ASIC cause is inferred, and no timing fix is selected.

MiSTer uses the pinned French6128 Plus cartridge, original2.7 disk and
Full0 on every202SSM record. All121captures have verified SHA256/dimensions
and CFG restoration passes. Forty-three images are768×546; retain their
native height. Root viewed all seven contact sheets and the three numeric
pages at full size. CPCSE uses its isolated packaged English Burning Rubber
cartridge, `--model cpc6128plus --crtc3 --no-chain --no-errata`; no firmware
or phase equivalence is claimed. The other peer pages are not adjudicated.

Private evidence: `local/overnight-20261103/resumed-20261104/`
`milestone-release-20261106/{captures/crtc3-B-001,inspection/crtc3-B}/`
and `c3-br-cpcse-20261106/`. Pinned RBF SHA256 is
`17466675860ee115418e712c55250a866c1b2f45bc9a796fd7f314a86d1cbf0d`.
These upstream computed values are separate from physical CRT curvature;
a native image alone does not validate the final display.

## Source qualification and next discriminator

The five C4 records begin at4BE6/4BFB/4C00/4C5A/4C60 in the original
26683-byte payload (origin3900, SHA256c34e2fcc273ab427baf9aeb84bcfa1fe6144565c0be6dee2241a8d99f4f2dc88).
Root independently verifies all five record bytes and the48-entry table's
4CC5terminator. Internal instruction cycles alone do not classify them:
SBC HL,HL has an internal tail but expectsCC. The 01F1 third group patches
EX(SP),HL on both sides of EI; its stored0503 delays14DF/14DE/14D6 become
printed14F7/14F6/14F3 after group-specific offsets.

French ACCC §27.7.1 p288 gives an independent R52 ordering discriminator:
a qualified raster ACK's bit5clear wholly before a natural31→32increment
leaves32; wholly after it leaves0. Without intervening VS resynchronization,
PRI or DMA, the next raster request follows20or52lines respectively.
The observed0D03/0503 gap is consistent with this0800µs difference but
cannot locate the ACK or select its cause.

French §27.7.2 pp288–291 describes relative HS/CPU sampling phases; it
provides no universal Type3 immunity or classic-only restriction on the
author's tables. CPCSE's Type3 disables its late-HSYNC holdover yet passes
both pages, so that holdover cannot be a necessary explanation. Its residual
instruction-state timing is a useful comparator rather than a silicon oracle.

Existing IRQ/ASIC tests cover other timing and provenance cases. A cheap
natural-counter paired ACK trial can qualify the independent rule without
forcing hidden counters or selecting a same-clock priority. A BR-specific
failing proof still needs the original caller, production READY/WAIT, both
interrupt return PCs, actual request/delivery/ACK and R52 history. The source
report alone does not establish that the CPU accepts all48opcodes at the
same boundary or that one ASIC pin is precisely1µs early.

Private qualified audit and parent byte receipts:
`resumed-20261104/c3-br-judgment-20261106/`.
