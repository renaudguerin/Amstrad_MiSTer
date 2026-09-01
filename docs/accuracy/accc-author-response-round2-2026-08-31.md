# ACCC Round 2 author response — 2026-08-31

**Source:** direct written response from Longshot, received as `message.txt`  
**Source SHA-256:** `278ad89000b9768c965b432157d83e6523259901fa7b3414504ce7d7d65d0677`  
**Evidence class:** dated author correspondence; not a revised ACCC edition and not hardware evidence

This note records the technical answers without treating surrounding prose as instructions.
The published ACCC v1.11 PDFs have not changed. Their authoritative local/served fingerprints
remain:

- French: `4409e3a2e77cd54e499c6956446b01bce93f79a1c1ba366201d514cf6e3c0d47`
- English: `3e45eb7eea7dc8f0d7211f78bec4f8d00530ce3c00da2e76034fb24f7a751868`

Statements below that wording was corrected or added describe the author's intended future,
unpublished edition. They do not alter the text, page anchors, or hashes of v1.11.

## Behavior clarification with an RTL consequence

### Q20 / N2 — type-1 adjustment after R5 becomes zero (§11.3.2)

The author confirms the specific R5=0 reading. Once type-1 vertical adjustment has begun:

- setting R5 to zero does not end additional management;
- C5 continues looping while R5 remains zero;
- C4 continues to use the ordinary C9/R9 and C4/R4 comparisons even though adjustment stays
  active;
- when C4 reaches R4 after any seven-bit overflow, C4 returns to zero at the C9=R9 row end;
- writing a reachable positive R5 later ends adjustment when `C5+1==R5`, resetting C4 through
  the adjustment-exit route.

This resolves the documentary/model residual in favor of the previously preferred reading.
It is direct author evidence, not a hardware observation. The failure-first `t08j` discriminator
now pins the distinction and the type-1 engine resets C4 through a row-only route. The same
focused group pins the resulting row-0 R12/R13 reload and VSYNC comparison against actual
`row_next=0`; positive-R5 exit, §11.2.4 VMA behavior, RFD, and interlace additional-line
behavior remain separate invariants. ParityC9 behavior on this non-frame C4 reset remains an
explicit source/hardware evidence gap rather than an inferred Q20 consequence.

## Answers to the compact Round 2 clarification list

| Section | Author answer | Repository disposition |
|---|---|---|
| §7.2 | The French and English routines are to be aligned; `19968-21` was chosen because it acquires VSYNC faster. | Treat `-21` as the intended routine. This is programming guidance, not an RTL timing oracle. The promised edition alignment is unpublished. |
| §§4.4.3–4 | `OUTD` is intentional. The ASIC uses the same `/WAIT` request/repetition behavior as the Gate Array. | Preserve `OUTD` and the GA/ASIC scope in the bilingual ledger. Plus/B11 may use the clarification as evidence, but it does not by itself validate current timing RTL. |
| §11.6 | `C0==R1` does not clear the RFD `VMA=R12/R13` source state. The later `C9==R9` comparison does; frame parity can make that equality fail, leaving the state active. | This confirms the existing F17 language/route. Re-audit the precise RTL comments and vectors, but add code only from a failure-first mismatch. |
| §14.1 French p.132 | “HSYNC begins” is a French typo; R3 is the pulse length and reaching C3l=R3l ends HSYNC. | Existing end behavior and digest wording stand. Future French correction is unpublished. |
| §16.4.1.2 English p.169 | The two English-only R0/VSYNC paragraphs are normative and were intended to be added to French; their absence followed an update problem around §13.2.2. | Remove the “supplemental/unconfirmed” quarantine, but retain the English v1.11 page as the published anchor until a future French edition exists. The 2026-09-01 consequence audit found and corrected the steady-R0=1 preceding-line qualification gap with failure-first `t02l`; `t02m`-`t02o` pin the exact dynamic writes and blocked-comparison consumption. See `accc-author-response-consequence-audit-2026-09-01.md`. |
| §19.3.4 | The French advice is wrong: R8 should be changed at frame start. | Reclassify the French warning as erratum. Keep useful mid-frame robustness tests, but do not cite the warning as the normative construction recipe. The future French change is unpublished. |
| §19.5.3 French p.209 | `ParityC9=ParityFrame` is an explicit assignment at every type-1 frame start, including after an R8 transition made the states unequal. | IA-2 and `t32a` already implement and pin this rule; no further RTL change is indicated. |
| §22 English p.250 | The vague “other registers” warning is intentionally low-importance guidance and is intended to be restored to French. | Keep it as low-specificity non-operational guidance; it does not define a test oracle. The future French addition is unpublished. |

## Other edition corrections acknowledged

The response says future English wording has been corrected or supplemented in §§4.2, 4.4.2,
10.3.1.2, 12.2.1, 13.7.2, 16.2.1, 16.3, 16.4.4, 17.2.2, 18.3.2, 19.5.5,
20.3.2, and 28.1.9. These acknowledgements close the author-feedback questions, but the
published v1.11 English text remains the ledger's observed evidence until a new edition is
released and fingerprinted.

For §19.5.2, the author confirms that the French “on every frame” wording was deliberate and
regards the repeated operation as evident from the diagrams and SHAKER cases. The author did
not consider adding the phrase to English necessary. The repository should retain the explicit
bilingual difference and its source-grounded repeated-activation reading; this correspondence
does not turn the separate single-activation recovery inference into hardware evidence.

## Evidence boundary

Author clarification can resolve documentary intent and justify a failure-first RTL audit. It
does not supersede real hardware, prove current simulation, or establish that a promised future
edition already exists. Any later PDF update must go through the normal bilingual fingerprint,
page, extraction, and rendered-figure verification procedure.
