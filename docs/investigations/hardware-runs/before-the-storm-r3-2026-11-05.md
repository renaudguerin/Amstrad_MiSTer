# Before the Storm: alternating R3 investigation

The user reports alternating-frame horizontal wobble on CRTC1 in an earlier build. The author confirms alternating R3 values. Media is`btf.dsk`, launched with`RUN"-BTF`; the affected landscape scroller is approximately373–403 seconds after RUN (the supplied video enters RUN near2s and shows the effect at6:15–6:45). The video frames have not been independently inspected: retrieval was throttled.

## Original caller and references

Disk SHA256 is`f01bc41d8ea9dc8dbacc8b78d7f331399d4a856437bd01f0558924feb1ba703e`. Normal-boot native CRTC1/Full/FR on accepted reference`e24cb50` completes38 commands,13 PNGs and zero runner failures. The RBF SHA256 is`c65d7e60ac4ff95263e2bf425da248c31b6c838bfadb90f2e653df533605cc46`. All images were inspected; they bracket the rainbow landscape, trees and parallax scene. These asynchronous stills are not consecutive physical frames and cannot establish or reject the reported every-other-frame wobble.

A matching original AmSpirit1.17.1/CRTC1/6128/FR run reaches the same scene and preserves13 reference images. Its eight burst callbacks are about19,968µs apart, whereas the native screenshot requests are farther apart. Their guest/frame phases are not matched. No PC/RAM/hidden-counter intervention was used. Effective post-reset`freq_screen` reads1; that setting is the documented PAL/NTSC enumeration, not an acceleration control. AmSpirit is paused after the scene with script and breakpoints cleared.

Fresh isolated CPCSyntaxError v0.12.0 (binary SHA256`f4254653b11676055eedd89dcdae6c6aab381835344d69780f56fcbff218a577`, source`22de765ef44da46c55217e4dd7caa385d69a6045`) also reaches the landscape under packaged UK firmware, `--no-chain --no-errata`. The bounded run exits0 with13 BMPs and two v3 snapshots. Root inspected the contact sheet and full burst03.

## Guest R3 mechanism

The loaded original code at085E computes`4 + (L & 1)` and stores it at0E36. The routine beginning087F writes R12/R13 and then R3 through`OUT (C),A` at089E; the peer trace logs the post-fetch PC08A0. Root verified these opcode bytes in the uncompressed v3 snapshot. The trace contains780 writes from this routine:390×04 and390×05, strictly alternating.

The peer's internal frame counter is a CRTC restart counter, not a display-frame ID. Of779 adjacent trace gaps,778 are2 and one is110; one early write hasC4=20 and the remaining779 haveC4=32. This evidence qualifies guest intent, not exact physical cadence, edge timing or monitor response. The provider's claims of universal delta2/C4=32, large lower scrolling text and a proven native wobble cause are rejected in the parent assessment. The useful result is the original register values, code and writer identity.

## Filter discriminator

French ACCC v1.11 §14.4 pp134–135, checked in the position-aware text and both rendered pages, specifies an eight-Pixel-M2 left shift when ordinary R3l changes4→5. The recommended4/5 pair avoids the less precise5/6 displacement. This is distinct from R3.JIT mode-switch controls already passing in the new GA candidate.

Our Full path regenerates acquisition sync from the raw HSYNC start and applies`crtc_shift` to accepted VRAM fetches; Raw pixels keeps the acquisition tuple but disables that byte compensation. A bounded read-only filter probe warms two312-line frames at R3l6 then alternates4/5/4/5, across all16 master-clock offsets relative to CE_4. All64 recorded frames have identical settled SHIFT=1 and unchanged regenerated HSYNC/HBLANK for both widths. The probe drives manual raw pulses, so it does not yet qualify actual CRTC/CPU phase or BTF's output. The current production-T80/motherboard discriminator now confirms the same defect after three natural frame origins. Raw HS starts at the same clock and lasts256/320clocks for4/5, but accepted interior fetches, Full acquisition and all RGB samples remain identical. The real falling classifier consumes count15→16 for4,19→20 for5; hs4 stays0 and both select the one-byte delay. The only setup repair prevented an actual tick-0 backend write while external reset was asserted; the ledger proves the original ROM jump survives and first CPU fetch occurs at1042. No release/counter/phase fit was made.

An otherwise identical width6 control has raw width384clocks and the same acquisition/address/input stream. It selects native bytes, and RGB is exactly left8 of both4 and5. There is no hidden whole-character acquisition offset: the current selector has only delay8/native positions. A threshold-only correction cannot represent all three widths. The French5/6 fractional displacement is not promoted into an invented exact15/7 digital oracle.

A private candidate is therefore constrained to preserve today's5/6 output byte-exact, retain their existing coarse8-pixel approximation, and add the missing extra byte delay for4. Exact4/5 LEFT8 comes from the primary source; current5/6 are explicit regression anchors. Both transition histories, return6, reset/mode history and direction need proof, followed by fresh strong review before production adoption. Physical fractional placement and analogue settling remain outside this bounded correction. No tracked filter RTL has changed.

Private media, exact scripts, configurations, native manifests, reference images, original peer traces/snapshots, parent corrections and the filter probe are preserved under`local/overnight-20261103/resumed-20261104/before-the-storm-r3/`. A corrected, reviewed full RBF is building before the next native comparison. No title-correctness verdict is claimed.

## Reviewed GA/MID build control

The same original CRTC1/Full/FR script on reviewed5261414 completes38 commands,13 PNGs and zero failures. Root inspected the contact sheet of all13 and full burst03: the landscape, trees and scrolling scene are present; the final after checkpoint is black. These snapshots have different guest phases from the e24/AmSpirit runs and cannot qualify alternating-frame wobble or full-demo completion. This build predates the private width-compensation candidate. Its exact receipts are in`g1-native-build/device-c1-btf/` and`BTF-CONTROL.json`; no device/API job remains active after this control.

## Pre-GA extension held after review

The private two-byte history passes12 exact LEFT8 relations,15 whole-capture width5/6 anchors, rapid first-following-frame controls on CRTC1, CRTC0 static controls, and nonzero reset/held-CAS/raw-path checks. Root verified all86 source/input hashes. Claude Opus5.5 high review`20261105T073915Z-2502-35b9` finds no implementation defect within its bitmap-delay scope, but identifies a material edge cost: delaying data inside fixed DISPEN doubles the lost final word at width4 and leaves border edges fixed. Root independently identified the corresponding timed border/palette-edge counterexample. The primary source describes displacement of the rendered line, so green interior bitmap checks are insufficient.

Root declines production adoption of this candidate. Its source, proof, reviewer report and explicit parent decision remain frozen; no filter or motherboard RTL has been changed in the tracked checkout. The review's inference that all classic HSYNC changes occur only on1MHz edges is not adopted: current CRTC has master-clock phaseful/JIT paths. Count16 remains a qualified waveform bin, not universal R3/JIT identity.

The next private experiment keeps the old VRAM path byte-identical and moves the extra eight-pixel correction after classic GA rendering. It must first reproduce a visible border/palette-edge failure, then prove that public motherboard RGB shifts with data and DE edges, preserving old width5/6 output and the common legacy approximation. A small rendered-pixel history is under investigation; there is no selected implementation, production adoption, all-phase monitor model or title-fix claim. New lifecycle proof and fresh independent review are required.

## Rendered-border counterexample

A new asset-free public-CPU caller selects BORDER and repeatedly writes hardware colours11/12. The R3=4/5 ROMs differ only in that immediate. After three natural frame origins, the predetermined row26/raster0 two-line window is wholly raw-DE0, visible Full HB/VB, and outside GA forceblank. Each case has12 accepted BORDER latch events, qualified before RGB comparison; these are latch events, not necessarily distinct OUT instructions. The accepted bus and visibility ledgers match across the same current/pre-GA coherent tuples.

Both current source and the rejected byte candidate fail the independent eight-Pixel-M2 relation at48/752 samples, while their zero-shift RGB relation has zero mismatches. There are six visible colour transitions; the complete8,192-clock public/internal GA RGB captures are identical between current and pre-GA for each width. The test observes public motherboard RGB ports. This supplies a deterministic failing-first proof for moving the additional correction after rendering, without fitting an absolute publication delay.

An offline comparison initially treated shared GA.D values as palette input even when no palette latch was active; the preserved correction qualifies that comparison by actual acceptance. Guest, phase, RGB oracle, sources and simulation captures did not change or rerun. Root verified all52 source/input hashes; pack manifest SHA256`6b74e2c1d58bc53ba6bd8ad1f0be86202f56f2912f12bfbe0a0cd1c7e5824415`. Evidence and exact commands are in`post-ga-rgb-proof/HANDOFF.md` under the same private BTF directory.

The post-rendering candidate is authorized only in a private copy. Its new cache must discard old rendered pixels and reject inherited/partial pulse classifications after reset, SNA or output-policy changes. The proposed sidecar owns a newly observed complete accepted pulse; it does not reset existing filter counters or infer physical R3/JIT identity. Production RTL remains unchanged pending new proof and review.
