# Before the Storm: alternating R3 investigation

The user reports alternating-frame horizontal wobble on CRTC1 in an earlier build. The author confirms alternating R3 values. Media is`btf.dsk`, launched with`RUN"-BTF`; the affected landscape scroller is approximately373–403 seconds after RUN (the supplied video enters RUN near2s and shows the effect at6:15–6:45). The video frames have not been independently inspected: retrieval was throttled.

## Consecutive native acceptance on c0939d8

Fresh coverage now distinguishes the shared classic correction from ASIC:

| CRTC | Current complete frames / eligible pairs | Middle-band steps | Status |
|---:|---:|---|---|
| 0 | 72 /63 | 31 holds,32 left2 | No reversal in the sampled current run. |
| 1 | 192 /176 | 88 holds,88 left2 | Before/after native acceptance below. |
| 3 | 192 /176 | 88 holds,43 left10,45 right6 | Reversing wobble still present; ASIC proof required. |

All three use c0939d8, the same original disk/command and requested Full
filter. Root views all72 CRTC0 and192 CRTC3 complete frames in their
contact sheets. The same predecessor reader, successor-header geometry
guard and preselected registration window are retained. CRTC0 has five
explicit capture rejections/gaps;67 complete frames qualify and63 links
survive the guard. CRTC3 has184 eligible frames and no explicit rejection.
No pre-fix CRTC0 or CRTC3 comparison is claimed.

CRTC0's fourth burst stopped on a full MiSTer `/tmp`; its partial files are
preserved and entirely excluded. Before continuing, root verifies397
original files across16 owned remote burst directories against their local
SHA256 copies, then removes those temporary remote directories, retaining
239,379,385bytes locally. CRTC3's eight bursts subsequently complete.
This capture-storage interruption is not a demo failure.

A fresh isolated CPCSyntaxError0 run also reaches the scene and logs276
original writer08A0 R3 updates,138 each of4/5, strictly alternating. This
qualifies the guest mechanism on its CRTC0 path without measuring native
bus phase. A separate CPCSyntaxError3 run reaches the same scene. Its
packaged English Plus cartridge differs from MiSTer's pinned French
`6128_FR.cpr` (`ab241580…3360cf`); peer output is supporting evidence only.

Private roots are `btf-crtc0-native-20261105-002/`,
`btf-crtc3-native-20261105/`, `btf-cpcse-crtc0-001/` and
`btf-cpcse-crtc3-001/` under `resumed-20261104/`. Per-machine
`PARENT-ACCEPTANCE.json` records counts, hashes and bounds; the CRTC0 root
also holds `REMOTE-CLEANUP-VERIFIED.json`. No physical HDMI/CRT or full-demo
acceptance follows. The new ASIC proof must fail before any RTL repair.

The reported reversing wobble now has a bounded native before/after result.
Both runs normal-boot the original disk/CSL, CRTC1, French layout and requested
Full filter. Applied CFG bytes match (SHA256
`2e585b4c85e2387cfb9c25028a061c6ba2aa3749f82453ffd895b5aaa393d8e4`);
the runtime applied-mode latch remains unobserved. Source c0939d8 uses RBF
`cb7fe0bd06c1d282413e501fd6017a34dba11ae62f4483e388a630c7ca1d883e`;
pre-R3 reference e24cb50 uses `c65d7e60ac4ff95263e2bf425da248c31b6c838bfadb90f2e653df533605cc46`.

A private read-only ARM reader acquires the scaler input buffers without
changing Main or the RBF. Root checks actual `ascal.vhd`: writer rotation
occurs at input VS rise, while the **new bank's** header is written on VS
fall before its pixels. Thus a fresh header is not a completion marker.
The corrected reader copies the distinct preceding-counter bank at the next
fresh event. It requires stable19–22ms PAL cadence, poll/detection delays
at most1ms each, copy at most12ms and unchanged post-copy header. Root
checks the Full-filter route; its ordinary VS spans counter1→3, about two
64µs lines. The14ms maximum detection/copy age leaves an ordinary-frame margin.
These conditions do not qualify arbitrary sync or a physical monitor.

Independent Opus review accepts this with a completeness limit: header
geometry lags measured frame geometry. Retain frame i only when frame i+1
is accepted consecutive with valid identical geometry; discard the last frame
and every frame before a gap. A static24-frame trial is full-RGB-exact to
Main at768×273. Moving captures are separately inspected for seams/stale rows.
The initial prototype's150 attempts all exceeded its5ms bound and supplied
zero accepted frames; its wrong completion premise and logs are preserved.

Current c0939d8 yields eight24-frame bursts across about21seconds of the
landscape (approximately372–393seconds after RUN),192images in total.
The successor-header guard retains184frames and176consecutive motion pairs.
All192images are inspected through eight contact sheets. Fixed registration
bounds chosen before capture—x32–735, y98–121, shifts−24…24—have unique
minima at **0 in88 pairs and −2 in88 pairs**, with no positive reversal.
The upper mountain and foreground bands retain distinct parallax speeds.
Copies take8.745–11.774ms; accepted periods19.533–20.462ms and maximum
poll gap0.592ms remain within the reader's bounds.

The same reader/CSL on e24cb50 captures four24-frame bursts. Eight explicit
copy/poll rejections and gaps are retained; only82eligible motion pairs enter
the comparison. The same middle band gives **−10 in20 pairs, 0 in41 and
+6 in21**. Its upper band also reverses, while the foreground combines into
larger left steps. Root inspects all96reference images. The earlier build
therefore reproduces a reversing displacement; the current build removes
it in the sampled native acquisition stream. This agrees with the separate
same-input production-T80 width4/right8 proof below. The source tuples differ
beyond R3; no cross-run equality of animated frames or hidden phases is claimed.
Physical HDMI/CRT output and the whole demo remain outside this result.

Evidence: `local/overnight-20261103/resumed-20261104/btf-native-frameburst-feasibility/`,
including `NATIVE-BTF-ACCEPTANCE.json`, both normal-boot manifests, all raw
RGB/header/timing records, registration scores, source/binary identities,
`v3-opus-review/` and a20-frame native before/after GIF at5×slow speed.
No public capture infrastructure, RTL edit or new gate follows from this trial.
Earlier asynchronous-still limits below describe those earlier runs.

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

A private candidate is therefore constrained to preserve today's5/6 output byte-exact, retain their existing coarse8-pixel approximation, and add the missing extra byte delay for4. Exact4/5 LEFT8 comes from the primary source; current5/6 are explicit regression anchors. Both transition histories, return6, reset/mode history and direction need proof, followed by fresh strong review before production adoption. Physical fractional placement and analogue settling remain outside this bounded correction. The original proof predates tracked transport; the current candidate status is below.

Private media, exact scripts, configurations, native manifests, reference images, original peer traces/snapshots, parent corrections and the filter probe are preserved under`local/overnight-20261103/resumed-20261104/before-the-storm-r3/`. The reviewed GA/MID control build is complete; the R3 candidate needs its own gated experimental build. No title-correctness verdict is claimed.

## Reviewed GA/MID build control

The same original CRTC1/Full/FR script on reviewed5261414 completes38 commands,13 PNGs and zero failures. Root inspected the contact sheet of all13 and full burst03: the landscape, trees and scrolling scene are present; the final after checkpoint is black. These snapshots have different guest phases from the e24/AmSpirit runs and cannot qualify alternating-frame wobble or full-demo completion. This build predates the private width-compensation candidate. Its exact receipts are in`g1-native-build/device-c1-btf/` and`BTF-CONTROL.json`; no device/API job remains active after this control.

## Pre-GA extension held after review

The private two-byte history passes12 exact LEFT8 relations,15 whole-capture width5/6 anchors, rapid first-following-frame controls on CRTC1, CRTC0 static controls, and nonzero reset/held-CAS/raw-path checks. Root verified all86 source/input hashes. Claude Opus5.5 high review`20261105T073915Z-2502-35b9` finds no implementation defect within its bitmap-delay scope, but identifies a material edge cost: delaying data inside fixed DISPEN doubles the lost final word at width4 and leaves border edges fixed. Root independently identified the corresponding timed border/palette-edge counterexample. The primary source describes displacement of the rendered line, so green interior bitmap checks are insufficient.

Root declines production adoption of this candidate. Its source, proof, reviewer report and explicit parent decision remain frozen; that rejected implementation was never adopted in the tracked checkout. The review's inference that all classic HSYNC changes occur only on1MHz edges is not adopted: current CRTC has master-clock phaseful/JIT paths. Count16 remains a qualified waveform bin, not universal R3/JIT identity.

The next private experiment keeps the old VRAM path byte-identical and moves the extra eight-pixel correction after classic GA rendering. It must first reproduce a visible border/palette-edge failure, then prove that public motherboard RGB shifts with data and DE edges, preserving old width5/6 output and the common legacy approximation. A small rendered-pixel history is under investigation; the reviewed successor below remains an experimental implementation, without a general monitor-model or title-fix claim. New lifecycle proof and fresh independent review are required.

## Rendered-border counterexample

A new asset-free public-CPU caller selects BORDER and repeatedly writes hardware colours11/12. The R3=4/5 ROMs differ only in that immediate. After three natural frame origins, the predetermined row26/raster0 two-line window is wholly raw-DE0, visible Full HB/VB, and outside GA forceblank. Each case has12 accepted BORDER latch events, qualified before RGB comparison; these are latch events, not necessarily distinct OUT instructions. The accepted bus and visibility ledgers match across the same current/pre-GA coherent tuples.

Both current source and the rejected byte candidate fail the independent eight-Pixel-M2 relation at48/752 samples, while their zero-shift RGB relation has zero mismatches. There are six visible colour transitions; the complete8,192-clock public/internal GA RGB captures are identical between current and pre-GA for each width. The test observes public motherboard RGB ports. This supplies a deterministic failing-first proof for moving the additional correction after rendering, without fitting an absolute publication delay.

An offline comparison initially treated shared GA.D values as palette input even when no palette latch was active; the preserved correction qualifies that comparison by actual acceptance. Guest, phase, RGB oracle, sources and simulation captures did not change or rerun. Root verified all52 source/input hashes; pack manifest SHA256`6b74e2c1d58bc53ba6bd8ad1f0be86202f56f2912f12bfbe0a0cd1c7e5824415`. Evidence and exact commands are in`post-ga-rgb-proof/HANDOFF.md` under the same private BTF directory.

The post-rendering candidate was first authorized in a private copy. Its new cache must discard old rendered pixels and reject inherited/partial pulse classifications after reset, SNA or output-policy changes. The proposed sidecar owns a newly observed complete accepted pulse; it does not reset existing filter counters or infer physical R3/JIT identity. The completed proof and review now permit provisional transport on the experimental branch, as bounded below.


## Post-GA candidate and experimental transport

The successor leaves the old VRAM, SHIFT and acquisition path byte-identical and stores eight rendered six-bit RGB pixels after the classic GA. Only a qualified ordinary width-4 pulse in classic Full mode selects the additional delay. Widths 5/6 retain their existing output; raw and Plus routes bypass it. A separate pulse owner invalidates the new history on reset, snapshot loading and output-policy changes. It admits a classification only after a newly observed complete accepted pulse, without resetting the existing filter.

The frozen public-output proof changes the BORDER counterexample from 48/752 incorrect samples to zero. Both classic types have zero mismatches over the 1,520-sample complete visible window, including DE entry/exit; six rapid first-following-frame relations also pass. Sixteen old VRAM/raw-GA/input streams and ten public width-5/6 anchors are exact. Real save-capture snapshots exercise 4→4 rearming and 4→6 bypass through the supported loader. That loader starts download with old HSYNC high but applies the snapshot with HSYNC low: active-HSYNC-at-apply coverage is not claimed. Private lifecycle assertions that mirror the implementation are consistency checks, not independent hardware evidence.

Parent verified the 52-entry source/input manifest, SHA256 `9328ec4ccbf1c9b407eb993a5342ed3d36cd9fa5cfa1c71b759744c6dd4430f0`. Opus 5.5 high review `20261105T084322Z-9675-575b` finds no blocker to an experimental source/build/device trial. It does not clear the general monitor model: adjacent-line 4/5 alternation may produce an eight-pixel comb where the physical monitor settles between positions; R2 relocation can leave a stale class during relocking. Nonordinary/JIT pulse bins and fractional 5/6 placement remain outside the correction.

The exact reviewed source has been transported on `codex/classic-ga-boundary-native-20261105`, with a compact retained public-output test, unchanged legacy controls, focused B6/output-path checks and the final selected gate. Opus transport/test review `20261105T093332Z-18934-c979` finds no blockers for the bounded trial. The retained 12-caller proof and final 46-bench selection pass. Parent applied its two small bookkeeping suggestions: SNA dependency coverage and clearer loader-reset result wording, then reran the focused owner and final selection. No RTL, fixture, guest or oracle changed. A clean full build and targeted native acceptance remain pending. No master adoption or readiness claim follows from the private results. The complete proof and reviewer assessment are under `post-ga-rgb-proof/candidate/` and `strong-post-rgb-review/` in the private BTF directory.

## D6 acceptance control

A fresh direct D6 normal-boot CRTC1/Full/FR run on `5261414` completes 18 commands, four captures and zero failures. Markers `0381`/`0382` identify its mode-1/mode-0 scrolling pages; each also has a later capture. Root inspected all four against cached original-CPC photographs `D6_0381_A.webp` and `D6_0382_B.webp`. The marker and later captures differ substantially in horizontal position and wrapping. These asynchronous snapshots do not establish adjacent-line pulse behavior, a matched photo phase or a new regression. Fresh CPCSyntaxError guest-write tracing now qualifies the guest sequence, with parent corrections below. Private receipts are in `d6-acceptance/`.


Fresh isolated CPCSyntaxError completes six images, two v3 snapshots and 367,058 R2/R3 trace entries. Parent inspected every contact-sheet image and verified original trace/header/code facts. The useful result is a two-source-line routine: for example, R3=4 at C0=51 and R3=5 at58 on even C9, followed by an odd line without those writes. Another routine changes R2=52 on the even line and restores51 on the odd line. That does not exclude alternating effective pulse widths; a register-only trace cannot measure them. Logged C4 spans0..38. The provider's single-line/C4=30..38 interpretation and several incorrect C0 values are rejected; the corrected per-PC summary and parent assessment are retained beside the original report. Its monitor-source pointer is useful: `src/core/monitor_renderer.cpp` filters phase error across lines and explicitly discusses D7, but that SHAKER-trained peer renderer is not a physical oracle. No rerun or RTL rule is selected from the rejected interpretation.


A separate targeted first-two-page D7 control on the same accepted build completes 18 commands, four PNGs and zero failures. Root inspected all four and the cached physical `D7_0383_A1.webp`/`D7_0384_A2.webp` photographs. Both native marker/later pairs are RGB-identical, providing repeatable R2-oscillation regression controls for the candidate. No numeric CRT-geometry or general settling verdict is assigned. The script, manifests, images and exact comparison are preserved in `d7-acceptance/` under the private BTF directory.


## Separate D7 filter discriminator

Root also runs the same first two D7 pages in the isolated CPCSyntaxError v0.12.0 copy, with packaged UK ROMs and `--no-chain --no-errata`. The bounded run exits0 with six BMPs, two v3 snapshots and101,008 R2/R3 trace entries. All six images were inspected; both marker/later pairs are identical. Actual writes at PC54C8/54CD alternate R2=46/47 on adjacent even/odd C9 lines for A1, then46/48 for A2, at the peer's character-rounded C0=2. R3 remains6 during both cases.

The native captures have a constant left boundary, apart from a one-pixel content difference. The peer images show an entry transient and then about8/16 image pixels of left displacement respectively. Their horizontal dimensions are both768 pixels; the peer images have twice as many vertical image rows. The raw threshold-based edge observations are retained in `d7-acceptance/OBSERVED-LEFT-EDGES.json`; they are measurements of emulator output, not physical-monitor coefficients. The visible peer settling and the cached CRT photographs motivate a source discriminator against French §14.4 p135's between-position synchronization. A private production-motherboard discriminator is being qualified before choosing a timing change. No copied AFC coefficients, exact settling law or production implementation is selected.

This is a distinct R2/acquisition question. The current R3 correction is scoped to a qualified width-4 pulse and bypasses ordinary width6; its steady4/5 proof does not close D7.


## Strict-lint syntax repair

Trial commit `4e68533` passes hosted production-T80 CI, including the retained owner within the existing job limit, but its simulation job rejects eight `PROCASSINIT` warnings in strict motherboard lint. The initializer style had been noted during review; it is a CI blocker here. Its running full synthesis is allowed to finish, but a bitstream from this SHA is not accepted for device use.

Only the eight new one-bit declaration zeros are moved into an explicit `initial` block. No RGB history initializer, clocked equation, port, stimulus or oracle changes. Motherboard SHA256 changes from75b85 to `44fd0c206eff3573eb571c5102f875cf55a458c969876019b1061d7a2bf9765a`; filterf95 and39 protected sources remain unchanged. Strict motherboard lint, full corresponding CI lint, the12-caller owner and the final46-bench selection all pass after this edit. Root verifies41 source bindings and confirms the preserved before-file equals GitHEAD.

Fresh narrow Opus review `20261105T100532Z-25288-b91d` finds no semantic blocker: the same eight zeros use the repository's explicit initialization syntax. It reads the actual one-hunk before/after diff and clean lint log, accepting gate/hash receipts without rerunning them. Its scope does not reopen or close the known monitor-model limits. Exact red, source substitution guard, commands, final gate line and review are preserved in `ci-initializers/`. A corrected exact-SHA full build and native checks remain required.


## Corrected full build and first native controls

Commit `6cc52c3` has all-green exact-SHA CI/full synthesis in run42008725418, artifact13407526543/build388. Quartus17.0.2.602 is clean full, with setup0.477/hold0.248ns across seven clocks each and zero TNS; no actual critical or implicit-net diagnostics. Its pre-existing invalid fast-I/O/fitter assignment warnings remain; their ID counts match the later f4 trial. Resource summary is24,381ALMs/29,028registers/701,596RAMbits/102RAMblocks/35DSP. Diagnostic S/DT macros are off in the exact source. `Amstrad_20261105_6cc52c3.rbf` is3,124,180bytes, SHA256 `666930cf70ce029901dbbbacfe0fef37f834dce70e227201713674a66bf632cf`, copied and read back identically on MiSTer.

Targeted D6 and first-two-page D7 each complete18commands/fourPNGs/zero failures. Root inspects every capture. All four D7 images are RGB-identical to526; this correction does not change the separate R2 phase defect. D6 captures still span different scroller phases and do not establish per-line settling or a regression. New receipts are beside their baseline runs in `d6-acceptance/` and `d7-acceptance/`.

The static BASIC Mode2/R3=&84/&85/&86/&84 calibration completes on both `5261414` and `6cc52c3`: each has24commands/fourPNGs/zero failures, all eight inspected. The640-pixel white paper spans x63–702 for every old case. On the candidate, width4 and return-to4 span x71–710, while widths5/6 retain x63–702. This independently verifies ordinary4→5 LEFT8, the complete paper width and a stable return. Constant paper does not measure interior5/6 byte position or adjacent-line settling. `native-width-calibration/RESULT.json` binds the observations and captures.

The first calibration attempt stopped in the host translator before BASIC input; it supplies no pixel result. Direct inspection of the pinned French system ROM table at0x1EEF proves physical UK-M produces comma and UK-dot produces colon. The bounded host repair retains a full OUT-command/modifier-boundary test: red before the map fix, green after;231 hardware-loop tests pass with12 skips, followed by the final46-bench selection. Root independently reviews the Gemini-authored diff against those ROM bytes and verifies the final source hashes and provider gate receipt. No firmware bytes are committed. Exact failure, ROM-table evidence and review/gate receipts remain under `native-width-calibration/keymap-repair/`.

The new CRTC0/1 DSC4 controls each complete19commands/fourPNGs/zero failures, all eight inspected. Credit colons and handwritten names are clear, and sampled main-effect views show the image. All four CRTC1 PNGs and three of four CRTC0 PNGs are RGB-identical to526; the remaining CRTC0 bottom-labelled image is phase-unmatched. CRTC1 Amazing completes13commands/twoPNGs/zero failures; both images are RGB-identical to526, including the later coloured-line pattern. These asynchronous views do not close the reported bottom-line timing or full-demo coverage. `TARGETED-RGB-COMPARISON.json` binds individual comparisons under `r3-native-build-corrected/`.


CRTC0 Amazing also completes13commands/twoPNGs/zero failures; both are RGB-identical to526. The Plus DSC4/Amazing controls complete16/13commands and3/2PNGs, respectively, with zero failures; all five are inspected. Plus DSC4 credits and the first image frame are RGB-identical to275af8a. Its later image and the two Amazing views differ in animation phase, with the effects visible. These are bounded preservation controls for the shared build, not a new full Plus baseline.


Before the Storm's new normal-boot CRTC1/Full/FR control completes38commands,13PNGs and zero failures through the reported landscape scroller. All13 images are inspected: the scene is visible and progresses through its palette/landscape/text changes, then exits. Asynchronous native stills do not settle the reported alternating-frame wobble. `NATIVE-TARGETED-RECEIPT.json` binds all30 new DSC4/Amazing/BTF images in seven machine/title groups; D6/D7 and static calibration retain separate manifests. No full-demo or general monitor acceptance is inferred.

## Actual resident BTF caller on paired production-T80 sources

A private original-snapshot experiment now executes the unmodified landscape effect through the production T80 and supported SNA byte loader on coherent5261414 and42befa3 source tuples. Natural frame origins and four authentic alternating writes qualify the resume before capture; no PC/RAM/counter edits are used beyond the loader. Four consecutive complete Full frames per source are768×273. Guest writes,4,001 complete raw HS pulses,1,092 DE rows and an exact25,559,040-byte all-master-clock raw-GA/acquisition/phase ledger are byte-identical. Fifteen non-public CE16 fields also match. Root reads preparation/driver/DAC analysis and handoff, inspects all eight frame views and verifies115 artifact plus62 coherent compiled-source bindings and original input copies.

The original instruction-start PC089E writes4/5 at natural-origin ordinals1/3/5/7/9/11, sometimes during rawHS. Every complete measured pulse is256 or320master clocks; pulse width is measured rather than inferred from R3-at-rise. Four filtered frames span79.872ms. Visible width5 frames0/3 retain simultaneous old public RGB exactly. Width4 frames1/2 are exactly the simultaneous old frame shifted right8 M2 pixels, with zero mismatches in207,480 within-row comparisons each; zero-shift differs7,508/7,514pixels. This observes the intended correction reaching the actual title caller. It compares paired same-time outputs, not different animated frames.

The SNA does not preserve every hidden CPU/CRTC/divider/acquisition latch, and the fixture has no FDC: this is a qualified resident-effect resume, not a normalboot phase proof. Coherent sources differ beyond R3; equal complete internal ledgers delimit the observed public change. No physical settling, fractional placement, native alternating-frame wobble or full-demo signoff follows. The initial terminal write-count bookkeeping failure and unused reference end_gen observation adaptation are preserved; neither changes guest, waveform oracle or physical claims. Evidence: `btf-snapshot-caller/HANDOFF.md`, RESULT.json, SOURCE-INPUTS.json, exact raw ledgers and complete correctly mapped GA-DAC images under the private BTF directory.

The same normalboot CRTC1/Full/FR BTF control on42befa3 now completes38commands/13PNGs/zero failures; all13 are inspected. The reported landscape scene progresses through sky/trees/text and onward to its next message. These native stills preserve scene execution but remain asynchronous and cannot establish alternating-frame wobble. The first attempt stopped before any capture because its360s wait exceeded the mistakenly supplied120s runner bound; its failure manifest is preserved. The unchanged script succeeds with the explicitly bounded420s wait in a fresh output directory `relock-native-controls/c1-btf-r2/`. No guest/media/code change or timing fit follows.
