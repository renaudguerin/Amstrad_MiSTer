# Plus ASIC: where the RTL departs from, or chooses between, written sources

This ledger lists every Plus ASIC rule where the RTL follows something other than a single
uncontested written source. That covers a source conflict, a derived rule, an emulator-derived
rule and a model assumption. Each row names the hardware mechanism the RTL implements, the
evidence for it, how confident we are, and the discriminator that would settle it.

Classic CRTC (types 0/1) divergences from the ACCC are recorded per finding in
[audit-findings.md](../classic/audit-findings.md), where the ACCC is the working oracle.

**Policy.** A row describes one hardware mechanism that explains every observation. It never
names a title-specific workaround. Titles and emulators are evidence, not rules. When a probe
result contradicts a row, the mechanism changes, the row is revised, and titles are rechecked.

Confidence, strongest first:

1. **hardware**: original Plus/GX4000 photographs confirm it.
2. **title+emulator**: a real title needs it and at least one emulator agrees; no hardware test.
3. **derived**: read from written sources with an inference the sources do not state.
4. **assumption**: a named model choice with no source.

The probe screen numbers refer to [the multi-test probe cartridge](../../scripts/diagnostics/README.md#multi-test-probe-cartridge-plus_hw_probespy).

| Rule (owner) | Written sources say | RTL does | Evidence | Confidence | Discriminator |
|---|---|---|---|---|---|
| Ordinary PRI request phase (`asic_ga_timing.v`) | Revised Arnold §2.4: monitor-HSYNC trailing edge, clamped at start+6 µs. Kevin Thacker (KT): ~10 µs, width-independent | 1 µs after raw HSYNC start, every width, while HSYNC is active | Original Plus 2026-09-27: flat-plane markers at widths 3/6/11 end ~136/~135/~139 dots in, ~78 without padding. The width rule is absent; RTL predicts 139/139/139 and 75. AmSpirit agrees; Eerie Forest needs ≤ start+4 µs ([evidence](references/eerie-pri-trigger-counterfactual-2026-09-27.md#original-hardware-2026-09-27)) | **hardware** for width independence and the 1 µs CPU slot. Sub-µs position is invisible to software | Settled. A raw INT trace would add sub-µs detail |
| PRI at HSYNC width 1 (same) | Not addressed | No request: HSYNC ends as the 1 µs point arrives | Consequence of the row above only. AmSpirit does request | derived | Probe 01 (A1), against 02–03 |
| PRI written to the current line during HSYNC (same, `pri_value_change`) | Not addressed | Changing PRI to the current line while raw HSYNC is active requests at once. Same-value writes and writes after HSYNC ends do not | FF2 palette chain (MiSTer-accepted 2026-09-27); CPCEC `cpcec.c:2106–2114` | title+emulator | Probe 04 (B): write phase C0 45–63. RTL marks 45–59, AmSpirit also 60 |
| PRI line compare width (`asic_ga_timing.v`) | Revised Arnold: nine-bit `{0,PRI}` against `{VC5..0,RC2..0}`. CPCWiki ASIC page and Quasar: PRI=n also fires at n+256 | Nine-bit, no alias | Original Plus alias probe 2026-09-27: 32 IRQs in all four cases. Copter 271; AmSpirit; CPCEC | **hardware** | Settled |
| HSYNC crossing into the PRI line (same, `pri_line_entry`) | Revised Arnold §2.4: overlap can trigger at line entry | Line-entry request, plus the ordinary one at +1 µs on the same line. When +1 µs lands exactly on the next line's entry, both compare the new line | CRTC3 demo Buddha/spheres (MiSTer). AmSpirit gives two requests at R2=63 where the RTL gives one | derived | Probes 05–09 (C1–C5); C5 splits RTL and AmSpirit |
| SPLT near the 312-line wrap (`asic_video.v`) | Eight-bit compare, so SPLT=55 also matches line 311 and is pathological (revised Arnold); KT says 56 | Eight-bit compare, but a line-311 capture has no visible effect: the frame-origin reload wins | AmSpirit shows SPLT=55 turning the whole frame into the split bank | derived, and **against both sources** | Probes 10–13 (D1–D4) |
| SSCR vertical offset with R9>7 (`asic_video.v`) | §2.5 adds to the low three RA bits; the revised summary reads as a wider addition | Low-three-bit addition. With R9=11 and offset 5 the row-end compare never matches, so every row repeats | AmSpirit shows a third pattern | derived (claimed only for R9=7, pinned by `t08i`) | Probes 14–15 (E1–E2) |
| Sprite attribute write mirrors (`asic_regs.v`) | Revised Arnold: +3, +4, +5, +7 write magnification. KT: +4..+7 | +4..+7 write magnification; +3 is Y high only | AmSpirit agrees with RTL | derived | Probe 16 (F) |
| Sprite X at the left edge (`asic_sprites.v`) | Arnold §2.1 prints X −64..+639 | X field is a 10-bit coordinate; at x4 the first visible dot is X=−63 | Geometry derivation; AmSpirit agrees | derived | Probe 17 (G1) |
| SSCR[7] extended border over sprites (`asic_video.v`) | §2.5: D7 extends the border; §2.1: border beats sprites | Sprites are hidden in the masked 16 dots | Eerie Forest green bar gone on MiSTer; CPCEC, CPCSyntaxError, konCePCja, Caprice32, AmSpirit agree | title+emulator | Probe 18 (G2) |
| Sprite access blanking duration (`asic_sprites.v`) | "About one byte / 1 µs" | Two-dot tail (`blank_cnt=2`) | None | assumption | Needs sub-µs capture, not a photo probe |
| Classic request pending across a PRI 0→nonzero switch (`asic_ga_timing.v`) | Arnold §2.4 selects PRI instead of CPC interrupts; KT: CPC requests inactive while raster interrupts are active | A pending classic request is retained but masked, and delivered again when PRI returns to 0 | AmSpirit 1.15.1; Eerie Forest pending-classic investigation (`asic_pri_test.cpp` pr08) | title+emulator | No probe yet |
| Pending clear on PRI writes (not implemented) | Not addressed | Not imported. CPCEC clears a pending raster request on some PRI writes (Eerie Forest comment) | Eerie PRI writes never see a pending request in any tested timeline | decision | Revisit only with a title that needs it |

## Recording a probe result

Add the photograph path (ignored media, main checkout `local/`), the machine model and the
observed outcome to the row. Then do one of three things:

- Promote the confidence.
- Open an RTL finding with a failing vector derived from the photograph.
- Revise the mechanism.

Update the owner's comment and [asic-reference.md](references/asic-reference.md) in the same
change.
