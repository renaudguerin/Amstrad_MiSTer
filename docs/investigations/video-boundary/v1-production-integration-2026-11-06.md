# V1 CRT response: private production integration

Production video remains unchanged. The reviewed standalone receiver is READY
onb291826; the subsequent private motherboard/output integration is **not READY**.
Fresh Opus review found a one-native-dot RGB/metadata alignment defect, and its
focused failing-before proof and repair now pass; fresh Opus supplement
reports conditionalGO/no blockers for private integration. No monitor parameters
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
cannot be attributed to that common stage. The new combined consumer observation failed inherited source before the
smallest tuple/field transport alignment; the repair is qualified below. The frozen receiver/preset stay unchanged.

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
The immutable inherited patch is16a6e59f…; repaired aggregate40d6cc1a… has
fresh Opus conditionalGO for private integration, not shipping or hardware
acceptance.

[Standalone design](v1-qualitative-monitor-design-2026-11-06.md),
[receiver candidate](https://github.com/renaudguerin/Amstrad_MiSTer/blob/b2918263037a2c6fa9f5c9c339af81bcef414d08/docs/investigations/video-boundary/v1-rtl-candidate-2026-11-06.md). The latter is currently
on the standalone branch and must be imported with its module.

## Alignment repair and follow-up review

The new consumer check first fails inherited source at3933325 (RGB matches,
FIELD differs), then passes after four continuous motherboard timing stages
and stagedVBL admission. Aggregate patch40d6cc1a… keeps the frozen receiver
and preset unchanged. Six ordinary owner/width cases pass5888 combined samples
each; full-boundary tuple/field, crop and lifecycle checks pass.

Primed Plus widths4/5/6 match frozen static offsets−15/−7/0 over32768 samples
each. This is Plus-only HS/HBL/RGB evidence; classic nonzero response, vertical
edge excitation and displacement transitions are not covered by that window.
Full-frame boundary checks cover the six tags by composition.260 is the
native-sampled relative lag for the qualified production source phases, not
an assertion of260 physical registers or every arbitrary asynchronous edge.
The timing path has261 registers. Opus requests retaining the combined consumer
regression at import and keeping strict lint/enabled-fit/native gates open.


## Owned source import

The active branch imports the reviewed production aggregate unchanged, including
the four timing stages. The retained full-board regression scores655104 native
samples per owner for RGB/metadata/FIELD at the actual output consumer, and
655104 separate FRAME_FIELD samples at the motherboard export. Each owner
exercises all four acquisition bits and both independent field transitions.
The test is a zero-offset transport check, not acquisition/priming or nonzero
classic-response proof; it makes no final ASCAL coherence claim.

Fresh Gemini test/plumbing review is reconciled. Strict disabled/enabled
motherboard lint passes with receiver-only BLKSEQ/PROCASSINIT controls and an
explicit lint parameter. Final `python3 sim/select_tests.py --run` passes48
benches; receipt and source hashes are in the private owned-integration packet,
review-triage-20261106/FINAL-RECEIPT.json. Enabled fullfit, RAM inference,
physical timing, full-top/ASCAL and native original-caller acceptance remain.


## Enabled full fit: timing blocker

The reviewed shared-arithmetic receiver is preserved at `e48614c` on
`codex/v1-production-integration-20261106`. It passes199 cases /23,425,376
comparison clocks,48 selected benches and unchanged soak974e5c048eb7f877.
Fresh Opus review permits a private timing measurement. Equivalence covers
public outputs, canonical state and valid history; dead invalid payload can
change. The preset, presentation latency and both physical records remain.

Full Quartus17.0.2 run42323729673 fails setup at **−28.450ns**, TNS
−33,453.313ns; minimum hold is+0.248ns. The worst reported path is HPS
status[34] to receiver absolute_phase[30],43.423ns data delay. Resources are
26,765/41,910ALMs (64%),32,653 registers,703,161 memory bits,104 RAM blocks,
35 DSPs. This improves on the earlier −39.346ns/28,304ALM/39DSP fit but
still cannot qualify a hardware build. Selected CI passes; production-T80
is cancelled and must not be reported as passing on this SHA.

No failed-timing V1 bitstream is installed. V1 is excluded from the release.
Resume from this actual TimeQuest path, without weakening clock constraints.
A multi-edge implementation must additionally prove the tight finalization/HS
spacing and preserve strict-prior selection and epoch cancellation; a copied
pipeline must not change the original edge's publication contract. Enabled
fit/RAM mapping, full-top/ASCAL and exact-build native acceptance remain.

Reports and source/proof receipts are in the private session root:
`v1-timing-shared-tail-20261107/fit-42323729673/` and its parent packet.
The earlier rise_tick→offset_delay failure remains in
`v1-owned-integration-20261106/failed-enabled-fit-42311949060/`.
Preserve the standalone75c8 and production-integrationc1e6 worktrees; neither
is the non-V1 release source. See the [handover](../session-logs/release-handover-2026-11-07.md).
