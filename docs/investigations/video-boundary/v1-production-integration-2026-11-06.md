# V1 CRT response: private production integration

Production video remains unchanged. The reviewed standalone receiver is READY
onb291826; the subsequent private motherboard/output integration is **not READY**.
Fresh Opus review found a one-native-dot RGB/metadata alignment defect, and its
focused failing-before proof and fix are in preparation. No monitor parameters
or qualitative response gains are being tuned.

## Private integration and current defect

The Sol-high candidate admits a fourth profile only with
`V1_CRT_RESPONSE_PRIVATE`; ordinary builds retain the existing three choices
and reserved3=Full. It wires actual shaped GA tips, explicit filter source
absence, post-compositor RGB/raw blank, complete filtered timing and both
field tags into the byte-identical standalone receiver. Native pixel cadence
is distinct from Raw CRT crop/geometry policy. There is no producer feedback.

Focused checks found a real adaptive-cadence RED and a same-edge owner-colour
RED before their local corrections. Legacy/disabled controls and zero-response
RGB preservation then passed. Those checks did **not** establish common RGB
and metadata latency at the final consumer: RGB required260master clocks
relative to Raw, while timing and fields required256. Opus correctly identified
that both routes already share the colour converter, so the extra4clocks
cannot be attributed to that common stage. A combined consumer observation
must fail on the inherited patch before adding the smallest tuple/field
transport alignment. The frozen receiver/preset stay unchanged.

Opus also narrows the stale-owner guarantee: the mask limits stale colour at
the observed converter boundary; an earlier mixer/gamma sample may retain one
dot. No physical owner-transition guarantee follows. A primed real-tip-phase
control with nonzero displacement must cross the actual compositor/colour
chain, beyond standalone receiver coverage. Forward net declarations should
move before their new Verilog continuous uses.

## Build and acceptance boundaries

Private dependency closure covers all motherboard source lists and B18/SSM.
Default/enabled static elaboration succeeds, while strict motherboard lint
fails59style warnings in the unchanged receiver:46BLKSEQ and13PROCASSINIT.
A receiver-only lint policy remains to be adopted; no broad waiver was added.
Standalone tests/index and their cited design doc must accompany production
adoption.

An ordinary disabled Quartus fit cannot qualify this receiver because its
outputs optimize away. The explicit enabled profile needs full timing/resource
verification before calling its final source stable. Complete emu/OSD and
ASCAL field transport, retained post-processing and original-caller/native
monitor acceptance remain separate gates. Discrete simulation is not proof
of FPGA RAM latency, physical timing or CRT accuracy.

Private evidence under
`local/overnight-20261103/resumed-20261104/`:
`v1-production-boundary-proof-20261106/` retains immutable original sources,
RED/GREEN/legacy receipts, build closure and complete Opus review;
`v1-production-alignment-fix-20261106/` owns the new counterexample and fix.
The current inherited aggregate patch is16a6e59f…; Opus conditionalGO permits
prepared integration work, not shipping or a hardware acceptance claim.

[Standalone design](v1-qualitative-monitor-design-2026-11-06.md),
[receiver candidate](https://github.com/renaudguerin/Amstrad_MiSTer/blob/b2918263037a2c6fa9f5c9c339af81bcef414d08/docs/investigations/video-boundary/v1-rtl-candidate-2026-11-06.md). The latter is currently
on the standalone branch and must be imported with its module.
