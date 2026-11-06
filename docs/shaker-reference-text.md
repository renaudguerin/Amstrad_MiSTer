# Reviewed text from static CPC reference photos

Use `local/shaker-reference-text/INDEX.json` and the matching CRTC/test/page
JSON or TXT before reopening a reviewed static text photograph. This private
cache is a reading aid; the original image remains authoritative. Seventeen pages
have reviewed result fields: the five-page C0 A4/B6/C5 and C1 A4/B6 pilot,
eight C0/C1 AI pages, three CRTC3 DU photographs and one C1 S photograph, with explicit partial coverage.
The complete private cache is also preserved in the main clone’s ignored
`local/shaker-reference-text/`, which newly provisioned task worktrees share
through their normal `local` link. Keep accepted fields and the raw batch
outputs when preserving private evidence before worktree cleanup.

Source-image SHA256 binds each record. Keep source expected values separate
from MiSTer observations and from program-version assumptions.

Only `verified_fields` are accepted. `unknown_fields` and raw OCR remain
unverified, even when an OCR engine reports high confidence. If a material
ambiguity is outside verified coverage, read the image and add only the fields
actually checked. Never infer missing digits, apply PPI masking to the literal
transcription, or infer geometry/animation from the text. A source hash change
means a new reference. No CRTC/GA passing grade follows automatically from a
transcription.

The pilot records all eight C0 A4 lower pairs. Independent Gemini vision
review and root crop checks also accept C5’s18 raw PPI pairs and C1 A4’s six
visible lower pairs. These are literal physical-photo transcriptions; core
comparisons apply the documented unrelated upper-PPI allowance separately.
The [native A4 glyph correction](investigations/hardware-runs/shaker-crtc0-a4-native-glyph-correction-2026-11-06.md)
resolves an earlier E/F reading error in the MiSTer image: the baseline had
one photographed-field failure, corrected on493cdea. The physical reference
values remain unchanged.

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

## CRTC3 status-reference recovery

The canonical physicalDU03C0photo is present in the main clone’s ignored
reference tree. Root,Astra and blindGemini read STATUS2bit0OK andbit4OK; only
those two fields are accepted in `CRTC3-DU_03C0_D.json`. Earlier no-photo
claims were an evidence-discovery error. Use `rg --files --no-ignore` for
ignored reference/media trees. Available26/27DU predicates are byte-identical;
that bounded binding does not identify the photo’s exact disk binary.

## Partial CRTC3 register-table cache

Two additional DU03BD/03BE photographs have15 verified literal fields after
blind Astra-medium and root image adjudication. Enlarging these moiré-heavy
photos did not make cheap-model readings reliable: Haiku retained unknowns;
Gemini supplied conflicting valid-hex digits. Those drafts remain separate.
Only the JSON `verified_fields` are accepted. R4/R5 and other uncertain
readings remain unknown; neither complete page receives a core grade. The
private packet is `vision-reference-batch-du-br-20261106/` beneath the session
evidence root. No further broad retry is required for these unresolved fields.

## Exact native screenshot text

For source-bound native MiSTer text panels, use the private reusable reader at
`local/shaker-native-text/read_native_text.py` and its README. It matches the
pinned ROM font exactly using fixed source-string anchors and an exact palette.
Nine frozen fixtures reproduce all codepoints and RGB panels; an independent
parent review found and qualified an unknown-cell validation RED, repaired
before the final92-check gate. Explicit unknown/off-palette cells prevent
acceptance, even if a diagnostic placeholder happens to match the raw image.

CLI exit0 only means output was written. Require `roundtrip_accepted`, and keep
font/program provenance separate from a caller-declared source hash. This reads
literal native glyphs and supplies no physical-photo or whole-page core grade.
The tool and private font are excluded from release screenshot archives.

## CRTC1 AI0090 partial matrix

The dense physical matrix now has316 of328 literal cells adjudicated, plus
eight footer fields. Twelve E/F cells remain unresolved. The source-bound
native decoder matches those316 PPI-bit0 samples and seven footer comparisons
(length66 and six samples), using the existing upper-PPI allowance separately
from literal transcription. The page stays unknown. Prior Gemini's five E
readings at the start of upper row9 were corrected to F by blind Astra and
root crop inspection. Raw outputs and partial comparison receipts remain in
`c1-ai-0090-photo-20261106/` and `c1-ai-0090-native-20261106/` beneath the
private session root. No new hardware run or matrix-wide pass is implied.

## S/02D3 physical result correction

The physical CRTC1 S word is4E40 after two blind vision reads and root glyph
adjudication. The former4E30 reading is superseded; it does not establish a
program-version conflict. The source-hash-bound cache record preserves that
correction and the exact-binary uncertainty separately. See the
[current S report](investigations/hardware-runs/shaker-s-current-2026-11-06.md).
