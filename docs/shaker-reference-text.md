# Reviewed text from static CPC reference photos

Use `local/shaker-reference-text/INDEX.json` and the matching CRTC/test/page
JSON or TXT before reopening a reviewed static text photograph. This private
cache is a reading aid; the original image remains authoritative. Twelve pages
have reviewed numeric coverage: the five-page C0 A4/B6/C5
and C1 A4/B6 pilot, plus seven C0/C1 AI pages, with explicit partial coverage.
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
lower pairs, preventing the existing seven VS-bit mismatches from disappearing
behind the unrelated upper-PPI allowance. Independent Gemini vision review and root crop checks accept C5’s18 raw
PPI pairs and C1 A4’s six visible lower pairs. One C0 A4 E/F correction
raises its lower VS mismatch count from six to seven; page grading is unchanged.

The pilot, raw drafts and execution/source receipts are in
`local/overnight-20261103/resumed-20261104/shaker-reference-text-pilot-20261106/`.
An initial local Vision request failed before returning text; its empty output
and error remain preserved. No source photo was edited. The cache and source
photos are private reference inputs and are excluded from distributed MiSTer
screenshot archives. Add text-only pages in bounded batches, verifying their
numeric fields once; graphics and dynamic effects still need image evidence.

Batch ownership, model/OCR routing and acceptance are tracked in [B27](backlog.md#b27-reviewed-text-cache-for-static-real-cpc-shaker-photographs).


## Native Apple OCR follow-up

Both native APIs now process all five photographs successfully using small
Swift programs: Vision `VNRecognizeTextRequest` with URL decoding/CPU mode,
and VisionKit `ImageAnalyzer` with its Live Text transcript. The initial generic
error is not attributed to one cause: handler, execution context and CPU mode
changed together. No Photos library, TCC setting or system configuration was
changed. The [Apple ImageAnalyzer documentation](https://developer.apple.com/documentation/visionkit/imageanalyzer)
provides the native API used by the second tool.

Their raw results still miss substantial photographed CPC text and confuse
hex glyphs, so neither output is accepted without image review. Source scripts,
per-photo results, stderr and execution receipts are in
`local/overnight-20261103/resumed-20261104/apple-ocr-native-20261106/`.
The independent Gemini pilot checked88 existing fields, agreed87 and corrected
one after root adjudication, then added30 visually checked fields. This supports
local OCR as a draft plus direct vision correction; confidence scores or an OCR
success status cannot establish correctness.


## Vision-only batch policy

Use two different cheap/free vision models for bounded static-reference batches.
The first ten-photo batch covers C0/C1 AI008D–0091. Gemini3.8 and Claude
Haiku completed blind source-only passes; Crouton hit its free-pool quota before
reading images and was replaced after termination. Root parsed20 JSON files and
verified all20 source/PNG hashes. Seven pages have128 identical result tokens;
the dense0090 grids and C0/0091 differ in completeness. Root direct-image
adjudication resolves their numeric uncertainties and accepts
128 result tokens across those seven pages; prose/parameter labels remain
unverified. The other three pages remain drafts.
Private outputs and comparison are in
`local/shaker-reference-text/batch-ai-20261106/`. Keep each pass separate until
comparison; local OCR is optional draft material rather than a prerequisite.

Hints describe field grammar, never expected answers: an explicitly hexadecimal
result allows0–9/A–F, so a visually supportedS→5 orO→0 normalization may be
recorded with the raw reading. E/F,B/8,D/0 and other valid alternatives remain
uncertain unless the image resolves them. Never globally replace letters,
borrow a neighboring row's value or supply the core's expected passing result.
Retain disagreements and source positions for image adjudication; spot-check
agreements before broad batch acceptance.
