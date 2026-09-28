# Original Plus DMA overlap observations — 28 September 2026

## Conclusion

The user supplied four photographs identified as original Plus results. All
four match the predeclared **H1 compatible request survives DMA ACK** class.
The pre-repair production model and MiSTer RBF1346f39 instead lose the raster interrupt
at two anchor-relative sweep positions. This is a confirmed software-visible
hardware mismatch in the current DMA/compatible overlap handling.

The photographs do not timestamp internal request creation or electrical ACK
edges. H1 includes later creation retained during ACK; the result does not
uniquely establish an early pending latch or prescribe the repair. The generated
`retain` model demonstrates one mechanism with the observed result, not a
silicon implementation. H0, H0rv and the tested H2/H4 alternatives are rejected
within the predeclared model family.

## Provenance

The four photographs were supplied together by the user on 2026-09-28. Their
page titles identify the four companion configurations. The delivered start-00
CPR SHA-256 is `0f499bc6c50f935b0f9a455ec953db44a97a6a2db5da57aa215914c80c7c7106`;
photographs cannot independently verify the loaded cartridge hash. The machine
is identified by the user as original Plus; no additional unit identity or
measurement of electrical signals is inferred.

Original image bytes are preserved locally under ignored
`output_files/pa7-dma-overlap/original-plus/`. The source is the user's Photos
library on `/Volumes/2TB Hub`. This tracked transcription and image hashes
retain the finding independently of that library's paths. Images are not
committed. A second reader independently confirmed all anchors, orders,
statuses, counts and repeat flags without ambiguous cells.

| Page | Original filename | SHA-256 |
|---|---|---|
| 1 | `7EFF8B28-3148-43A9-A1DE-7D72655D8125_1_102_o.jpeg` | `e8e3e9d9ca0781a2cbff203a821ee04ff29a5b74031af5927282369db41504a9` |
| 2 | `CF7D73E1-6D6D-4053-A91A-35835AC208B2_1_102_o.jpeg` | `9969661e32a65da4ea3d34d5d78e64d70719580699d4fe593447261d7c023e3b` |
| 3 | `38888854-562A-4003-89EC-FD1EC9CC9877_1_102_o.jpeg` | `e571a55b560fcdb0b37d16b4f7ba7def27cf5140b85955e50f8d13b8bf1e8d8d` |
| 4 | `FBBB5701-0797-46BE-B547-0226B8E42222_1_102_o.jpeg` | `06ebbc6777fe51907d3809fb6193ca4c6ddc703ec4d16024c9b814a94a2a4bec` |

## Full observation transcription by row class

All four pages show DONE. Every row has REP=08, DIFF=00 and FL=00.
D denotes vector04 (DMA0), R vector06 (raster). A triple is vector/status/marker;
all values are hexadecimal except signed microsecond offsets. On page3 only,
replace every DMA status00 below with40. That substitution applies to controls
and sweep rows. Every unused record is FF/FF/F.

| Rows | N | First triple | Second triple | PRE | POST |
|---|---|---|---|---|---|
| DMA FAR EARLY, all pages | 02 | 04/00/0 | 06/80/1 | C0 | 80 |
| DMA LATE, all pages | 02 | 06/C0/0 | 04/00/0 | C0 | 00 |
| DMA ONLY, all pages | 01 | 04/00/0 | unused | C0 | 00 |
| NO DMA −3 through +2 | 01 | 06/80/M (see below) | unused | 80 | 80 |
| DMA sweep, D then R rows below | 02 | 04/00/0 | 06/80/0 | C0 | 80 |
| DMA sweep, R then D rows below | 02 | 06/C0/0 | 04/00/0 | C0 | 00 |

No row has a third interrupt. No-DMA marker M is1 below s* and0 at or above s*.
These tables therefore specify every displayed observation on all 68 rows.

| Page | Configuration | s* | DMA sweep D then R | DMA sweep R then D |
|---|---|---|---|---|
| 1 | W8 NOP AUTO | 0 | −6 through −2 | −1 through +1 |
| 2 | W8 LD A,(HL) AUTO | −1 | −6 through −3 | −2 through +1 |
| 3 | W8 NOP MANUAL | 0 | −6 through −2 | −1 through +1 |
| 4 | W12 NOP AUTO | 0 | −6 through −2 | −1 through +1 |

## Comparison and limits

The anchors equal the production and AmSpirit anchors. Far-early, late and
DMA-only controls all meet the experiment's contract. Unlike AmSpirit, real
hardware delivers the DMA-only interrupt and DMA precedes raster in the
early control. The near-early rows retain raster too, so these observations
do not support a DMA ACK counter reset that would postpone raster beyond the
bounded window.

| Result relative to s* | s*−4 | s*−3 | s*−2 | s*−1 |
|---|---|---|---|---|
| Original Plus, all four pages | D,R | D,R | D,R | R,D |
| Production model and MiSTer (H0) | D,R | D only | D only | R,D |
| Generated late model (H2/H4) | D,R | D,R | D only | R,D |
| Generated retain model (H1) | D,R | D,R | D,R | R,D |

The mismatching absolute rows are −3 and −2 on pages1/3/4, and −4 and −3
on page2. There are eight differing rows overall; each is stable over eight
repetitions. The original page1 matches every recorded field of the generated
`retain` page0 result. Only that counterfactual page was run; matching source
order across all hardware pages is not a claim of unrun counterfactual tests.

Automatic DMA handler status00, manual status40, raster-first statusC0,
and raster-after-DMA status80 agree with the probe's status predictions in
these cases. This narrows the earlier DCSR uncertainty for this experiment;
it does not settle readback under other DMA modes or event combinations.

## Next implementation acceptance

The [repair acceptance record](pa7-dma-overlap-repair.md) now tracks the
implementation and its regression, review, gate and device evidence against
the requirements below.

Use these photographs and transcription as the authority for a failing
production-T80 regression: preserve both sources across the disputed DMA ACK
window, their order, status and no-DMA marker anchors. Read the shared pending,
vector-provenance and counter-clear logic before choosing a repair. Preserve
V5 timing, programmed raster behavior, DMA automatic/manual clearing and
classic CPC behavior. Obtain fresh cross-provider review and the repository
selection gate for that implementation. This evidence update changes no code
or RTL and runs no tests; the existing probe validation remains applicable.

See [probe design, predictions and model evidence](pa7-dma-overlap-probe.md).
