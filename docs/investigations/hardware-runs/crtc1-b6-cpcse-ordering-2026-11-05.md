# CRTC1 B6: CPCSyntaxError ordering evidence — 5 November 2026

Original SHAKER B6 page `019D` on CPCSyntaxError v0.12.0, source commit
`22de765ef44da46c55217e4dd7caa385d69a6045`, returns all six values
`13/14/26/26/1E/1F`, matching the physical photograph. Native MiSTer production
`e24cb50` returns `13/14/26/26/1F/1F`: the fifth row remains the discrepancy.
This is a peer behavioural success and character-ordering clue, not a physical
measurement of the CRTC's internal comparison aperture.

In the matching CPCSyntaxError source, the block-OUT scheduler advances hardware
to instruction effect offset 16 T-states before `writePort`; the advance is
relative to hardware already advanced in that instruction, not an unconditional
extra 16 T-states. The R7 write comparison uses the current vertical counter.
Its trace records the fifth-µs write at C0/C4/C9 `0/33/0`, so R7=32 does not
match; the fourth is at `63/32/7`, where it does match. The emulator trace is
not a physical hardware strobe observation and this ordering does not uniquely
prove a silicon mechanism. The [reference report and trace](../../../local/overnight-20261103/resumed-20261104/cpcse-019d-current/REPORT.md)
retain the execution and identities; its definitive physical-aperture inference
is not established by that trace.

A separate [frozen counterfactual](../../../local/overnight-20261103/resumed-20261104/r7-jit-current-cohort/counterfactual/REPORT.md)
rejects a common next-CLKEN R7 consumer: for the qualified OUT(C) optical case,
the required first-black edge is 35 master ticks before that proposed consumer.
With the existing transport, the proposal predicts pixels `22/6` for OUT(C)/OUTI
against the required `9/5`. B6's character ordering alone is compatible with
such deferral, but the optical constraint rules out the common model. No new
R7 timing rule or numeric delay is selected; the remaining question is which
comparison visibility ordering can satisfy both observations.


Additional read-only peer-source retrieval checks current CPCEC `c025aab`, Caprice32 `a0d78f0` and konCePCja `f15cdcd`. Root verifies commits and direct R7/row-update anchors. CPCEC compares current crtc_count_r4 (cpcec.c343–345); konCePCja compares current vcc with an equality-edge latch (src/hw/crtc.cpp120–127,273–277). Caprice identifies its controller as Type0 and has no executed crtc_type selector; its cap32.cpp679–688 C0>=2 gate is not CRTC1 evidence. No B6 run is performed in those peers. The helper’s universal scheduling/physical-/WR/quantization conclusions are rejected as unsupported, with the original report and parent assessment preserved in `r7-peer-ordering-sources/`. These source anchors do not resolve the physical question or justify changing the author draft.

A focused Astra-medium source audit finds no demonstrated OUTI-specific productionT80 defect: nominal OUTI4/5/3/4 and OUT(C)4/4/4 use the same wrapper I/O/WAIT path. Motherboard CRTC acceptance uses rawIORQ/WR levels, independently of READY and the GA register latch. Existing optical OUTI READY rises atstrobe+12 and WR/IORQ releases at+40; neither supplies an onset replacement within the frozen requiredRGB+20 budget. This rejects a simple later-acceptance repair, not the entire unmeasured physical ordering class. Root verifies the local source/evidence hashes; indexed official Zilog instruction totals were accessible, but no timing diagram was rendered, so pin-edge fidelity is not certified. No CPU patch or new numeric aperture follows. `outi-instruction-phase-audit/REPORT.md` and PARENT-ASSESSMENT.md retain the evidence and source limits.


## Current-source optical replay — 6 November

The earlier frozen optical cohort compiled source `90f3b18`, rather than the current
RTL. A fresh private replay now binds all 29 compiled production inputs to the
`fdf348c` functional source at documentation checkpoint `45853b9`. All 12 original
fixture, guest and oracle files are unchanged. One compilation and one execution
per original guest reproduce first-black ordinals 8/4 against required 9/5.
The analyzer exits 0 for input/observation qualification and explicitly records
`QUALIFIED_MISMATCH`; that exit is not a behaviour pass.

Every one of the 57 recorded raw columns is byte-identical to the frozen run
(8,705 OUT(C) rows and8,849 OUTI rows). The original word, production T80/READY,
PPI calibration, mode receipts, complete 16 KiB VRAM and independent bitplane
checks remain qualified. The parent read the report/final logs and verified
29 source bindings, 12 protected inputs and 82 artifact hashes without rerunning
the gate. The [current-cohort report](../../../local/overnight-20261103/resumed-20261104/r7-jit-fdf-cohort/REPORT.md)
and `PARENT-ACCEPTANCE.json` retain commands and limits. This is a raw-GA,
custom-reset-ROM diagnostic, not firmware/native boot or a new execution of
B6's original fourth/fifth caller. Downstream motherboard RGB is outside its
observed plane.

A focused read-only source audit confirms that immediate R7 writes inspect
current C4, whereas ordinary row entry uses the entering row. Forwarding the
entering row on the same edge cannot by itself explain the synthetic B6 fifth
write, which arrives seven master clocks before the carry. Unconditionally
using the future row would introduce an unsupported aperture.

## Pulse protection limits

A distinct hypothesis would expose a write-induced onset immediately, then
withdraw only that onset if equality disappears before the same held write
ends. Existing vectors do not distinguish this proposed ownership rule from
an irreversible onset: their active-pulse rearming case starts the pulse before
the held write. This observation identifies a missing discriminator, not a
hardware rule or a fix. It also does not repair the independent one-dot optical
residual.

French ACCC 1.11 §16.3 p169 states that an active VSYNC is protected from interruption
when the C4/R7 comparison changes; equality changes can lift the second
protection while the first remains. §16.4.2 p171 describes immediate CRTC1 onset
and counting the partial line. Neither passage establishes an exception for a
pulse started during an unfinished write. No provisional-pulse withdrawal,
numeric window or RTL change is selected. An original-CRTC pin trace resolving
a brief onset during the fifth write, or an author clarification of that held-
write case, would discriminate this hypothesis. A later PPI sample and the
photographed B6 duration alone cannot distinguish no onset from a short pulse.
