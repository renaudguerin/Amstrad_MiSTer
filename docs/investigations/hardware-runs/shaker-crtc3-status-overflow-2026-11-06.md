# CRTC3 status and C4-overflow failures

Current `fdf348c` fails the original program checks on DU/03C0 and E/0406.
Fresh AmSpirit Lite 1.17.1 completes both pages with every check clear. These
are confirmed failures against the current program, separate from a matched
physical-Plus photograph or a selected RTL mechanism.

| Check | MiSTer fdf348c | CPCSyntaxError v0.12.0 | AmSpirit1.17.1 |
|---|---|---|---|
| DU status1 bit5, zero VSYNC-width nibble | WRONG | OK | OK |
| DU status2 bit0, final captured sample | WRONG at4DFF | Same failure | OK |
| DU status2 bit4, adjustment-context capture | WRONG at0000 | Same failure | OK |
| E0406 ordinary R4/R9/R0 variants | Several failures | Checks clear | Checks clear |
| E0406 final ROM-select variants | Multiple failures | B0D/B1D fail | All clear |

MiSTer and AmSpirit use the same French Plus cartridge and current 2.7 disk:
CPR SHA256 `ab241580c9b6a9fa9aeae94ca6847ea70dca386d305e38fc2ac03fce603360cf`,
DSK SHA256 `6b9ff106bb8dd02e984f69acd39d0b26d7318854345d23643b12d6748ed0eb61`.
AmSpirit reports frontendLite 1.17.1/core 2491682, 6128+, CRTC3, 128 KB, FR;
monitor Off. CPCSE uses matching source 22de765 and its packaged English Plus
cartridge, with no chaining or built-in errata. Root viewed the full target
images and all other spot-capture images; their other checks are not regraded.

## Source boundaries

Re-extracted current D/E payloads and original Type3 scripts explicitly enable
these pages. DU rejects types below3; E distinguishes Type4 expectations in
its final rows. No Type4 photograph or result is substituted for Type3.

DU's printed offsets describe software sampling, not raw counter registers.
S1 bit5's FFFF+15 comes from a line-loop countdown. The status2 collector
reconstructs its frame map from624 samples at 32 strided phases. S2 bit4 first
checks a normal frame, then programs R4=25hex/R5=8 and expects a different
predicate in that adjustment-context capture. French§21.3.4.2 p249 instead
lists bit4 as always 1. The capture/interpretation and this apparent table
conflict need qualification before changing the constant; CF-9's bit3timer
question is a different issue.

French§21.3.4.1 p248 describes S1 bit5's fifteen-high-line band forR3h=0;
its outside-band value needs careful interpretation. The current test/RTL
assumes high outside VS, unlike the original guest check. AmSpirit and CPCSE
support the guest result but do not identify a silicon sampling aperture.

E0406 reads PPIportB bit0 after its register sequence as an overflow witness;
it does not read C4 directly. French§12.5 p103's final simultaneous ROM-select/
R4 race is explicitly excluded by the current `asic_video` implementation.
That interface lacks the full B bit5 distinction needed for the last cases.
The additional ordinary-row failures remain separate from that known gap.

## AmSpirit acquisition qualification

The first marker-only DU adapter stopped on the first page: its early Space
could not replace the author's long waits. That attempt is retained without
a final-page verdict. A fresh direct-U run preserves the authored 10/10/10/15/
45-second waits. The final45-second screenshot still says PLEASEWAIT, so the
already-running measurement was resumed for 1,000 frames without another guest
input or state edit. The later complete 03C0 page prints every check OK.
E follows the original module order through 0406 and clears every row.

Private evidence is under
`local/overnight-20261103/resumed-20261104/`:
`milestone-release-20261106/{captures,inspection}/`,
`c3-status-overflow-cpcse-20261106/`,
`c3-status-overflow-amspirit-20261106/`, and the exact-byte/French-source
`c3-status-overflow-source-readiness-20261106/` brief. Original failures,
incomplete captures, snapshots and the completed continuations remain intact.
