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

Our Full path regenerates acquisition sync from the raw HSYNC start and applies`crtc_shift` to accepted VRAM fetches; Raw pixels keeps the acquisition tuple but disables that byte compensation. A bounded read-only filter probe warms two312-line frames at R3l6 then alternates4/5/4/5, across all16 master-clock offsets relative to CE_4. All64 recorded frames have identical settled SHIFT=1 and unchanged regenerated HSYNC/HBLANK for both widths. The probe drives manual raw pulses, so it does not yet qualify actual CRTC/CPU phase or BTF's output. A production motherboard discriminator is in preparation before selecting any fix. No filter RTL has changed.

Private media, exact scripts, configurations, native manifests, reference images, original peer traces/snapshots, parent corrections and the filter probe are preserved under`local/overnight-20261103/resumed-20261104/before-the-storm-r3/`. A corrected, reviewed full RBF is building before the next native comparison. No title-correctness verdict is claimed.
