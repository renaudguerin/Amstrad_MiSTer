# Reviewed text from static CPC reference photos

Use `local/shaker-reference-text/INDEX.json` and the matching CRTC/test/page
JSON or TXT before reopening a reviewed static text photograph. This private
cache is a reading aid; the original image remains authoritative. The first
five-page pilot covers C0 A4/B6/C5 and C1 A4/B6, with explicit partial coverage.
Source-image SHA256 binds each record. Keep source expected values separate
from MiSTer observations and from program-version assumptions.

Only `verified_fields` are accepted. `unknown_fields` and raw OCR remain
unverified, even when an OCR engine reports high confidence. If a material
ambiguity is outside verified coverage, read the image and add only the fields
actually checked. Never infer missing digits, apply PPI masking to the literal
transcription, or infer geometry/animation from the text. A source hash change
means a new reference. No CRTC/GA passing grade follows automatically from a
transcription.

One local Tesseract pass produced TXT/TSV drafts for all five photos; perspective
and CRT texture caused severe hex errors and almost empty B6 drafts. Root
visually reviewed the accepted numeric fields. C0 A4 now records all eight
lower pairs, preventing the existing six VS-bit mismatches from disappearing
behind the unrelated upper-PPI allowance. C5's parity sequence is verified;
its raw PPI pairs are not. C1 A4 lower pairs remain outside this pilot.

The pilot, raw drafts and execution/source receipts are in
`local/overnight-20261103/resumed-20261104/shaker-reference-text-pilot-20261106/`.
An initial local Vision request failed before returning text; its empty output
and error remain preserved. No source photo was edited. The cache and source
photos are private reference inputs and are excluded from distributed MiSTer
screenshot archives. Add text-only pages in bounded batches, verifying their
numeric fields once; graphics and dynamic effects still need image evidence.

Batch ownership, model/OCR routing and acceptance are tracked in [B27](backlog.md#b27-reviewed-text-cache-for-static-real-cpc-shaker-photographs).
