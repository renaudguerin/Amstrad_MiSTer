# CRTC0 A4: native glyph correction and repaired lower result

**The eight photographed lower PPI pairs now match on493cdea.** The earlier
claim of seven lower native failures was a transcription error: root read
`FEFF` as `FFFF` in the8ba384e captures. Both original images remain unchanged.
The baseline has exactly one lower mismatch, row4; the candidate corrects it.

| Lower row | Native8ba384e | Native493cdea | Physical reference |
|---:|---|---|---|
|1|FEFF|FEFF|5E5F|
|2|FEFF|FEFF|5E5F|
|3|FEFF|FEFF|5E5F|
|4|FEFF|FFFF|5F5F|
|5|FEFF|FEFF|5E5F|
|6|FEFF|FEFF|5E5F|
|7|FEFF|FEFF|5E5F|
|8|FEFF|FEFF|5E5F|

Only the documented unrelated upper-PPI A0 difference is allowed in each
byte; no VS bit is masked. Two blind native-vision passes, Gemini3.8high and
Haiku5.5low, independently return both complete arrays. Haiku retains E/F
uncertainty. Root resolves it by enlarged direct viewing and exact native
8×8cell identity: baseline row1..8 all share the same E glyph; candidate row4
uses the identical F glyph already present elsewhere in the result. The entire
page differs at only three pixels, bounding box(411,189)..(414,191), all in
that E→F. Marker and delayed candidate grabs are byte-identical.

The upper repaired01/00 pair remains visible. All five AI marker pages are
byte-identical to8ba384e; their existing accepted/unknown grades carry forward.
The capture uses exact493cdea, RBF SHA256
f370e07b6813b4a0b06fa055b1e316518a53bdbc5bb218ae9cfb0f23d8dcfc35,
3,135,416bytes. Full CI/production-T80 and Quartus17.0.2 full fit pass,
seven setup/hold domains close with minima0.475/0.247ns and zeroTNS.
Full-mode telemetry and CFG receipts are retained with the targeted run.

A4 moves **broken→unknown**, not passing: the extra current-program C9
sweep has no original-hardware reference. This removes the last confirmed
photographed-field failure without treating partial validation as a whole-page
pass. The repaired row has C9=R9/C4<>R4 and is consistent with the independently
proved French§13.7.2.1 p127 row advance. That source proof and the R6 consumer
proofs do not depend on the mistaken seven-row transcription. Off-seam R4/R7,
late admission and parity alternatives remain separate model limits.

Historical reports/private frozen packets stating eightFFFF or seven lower
mismatches are superseded by this correction, not silently rewritten. The
original A4 resident diagnostic also differs from native in two second-byte
PB0 results, rows6/8; retain that seeded-loader versus firmware/history boundary.
Its upper33guard/01/00 acceptance and dynamic register binding remain valid.

Private evidence under the active session root:
`c0-a4-native-glyph-audit-20261106/` holds both blind reads, source hashes,
cell hashes and parent acceptance; `c0-r6-native-20261106/` holds exact-SHA
artifact/timing, original captures, comparison and targeted runner receipts.
