# D7 R2 alternation: Full acquisition stays at an endpoint

The current Full filter fails a bounded alternating-R2 discriminator while the CRTC produces the requested raw sync. Static R2 positions are correct. This identifies a filter phase defect separately from the open CRTC comparator questions; it does not specify a complete monitor model or close original D7 hardware acceptance.

## Rule and expected bound

French ACCC v1.11 §14.4 p135, read directly in the rendered page and position-aware French text, gives LEFT16 Pixel-M2 for R2 increasing by one. For positions alternating between lines, the CTM tries to synchronize between them. The test therefore requires a settled mean strictly inside the two independently controlled static endpoints. It does not assert an exact midpoint, response time, jitter or copied emulator coefficient.

## Native and peer controls

Direct D7 CRTC1/Full/FR first-two-page controls on accepted `5261414` complete18 commands, four PNGs and zero failures. Both marker/later pairs (`0383`/`0384`) are RGB-identical. All four images and cached original-CPC A1/A2 photographs were inspected. Fresh isolated CPCSyntaxError v0.12.0 with packaged UK ROMs and `--no-chain --no-errata` completes six images/two v3snapshots and101,008 register trace entries; its pairs are also identical. It shows an entry transient and about8/16 image pixels of settled left shift, where native output remains flat. CRT geometry is not assigned a numeric verdict.

Actual peer writes at PC54C8/54CD alternate R2=46/47, then46/48, on even/odd C9 at character-rounded C0=2. R3 stays6. The matching monitor renderer filters phase error across lines (`src/core/monitor_renderer.cpp`, around250–301); that SHAKER-trained implementation is a useful hypothesis, not the hardware authority.

## Production-motherboard discriminator

Five generated asset-free public-CPU callers run the production T80/READY, current CRTC1, GA and Full path with R3=6. They program static R2=46/47/48 or adjacent-line46/47 and46/48. Actual loop writes occur once per4096 master clocks at C0=13, before either target HSYNC. This qualifies ordinary R2 writes; it does not replay original D7's C0=2 phase.

| R2 sequence | Public visible entry from selected Full HS, M2 | Result |
|---|---:|---|
| Static46 |267|Endpoint control|
| Static47 |251|LEFT16 control passes|
| Static48 |235|LEFT32 control passes|
| Alternating46/47 |267 for all192 entries|Fails strict251<mean<267|
| Alternating46/48 |267 for all192 entries|Fails strict235<mean<267|

Raw HS rises at the requested46/47 or46/48 positions, with unchanged384-master-clock width. Inter-rise intervals alternate4032/4160 or3968/4224 clocks. Selected Full HS remains at C0=48 every4096 clocks and Full HB opens at60. Static47/48 correctly move selected HS to49/50 and HB to61/62. The measured visible entry is a public RGB ink edge inside the actual Full view; pre-edge CRTC observations and post-edge output samples are distinguished explicitly.

Two complete312-line frames have matching CE4/raw-HS/raw-VS input hashes and an identical snapshot of every stored filter variable/output at their boundaries, in all five cases. This qualifies the repeating response instead of assuming a settling deadline. The single run completes33,476,850 clocks and exits1 as expected: `QUALIFIED_RED`, both alternating bounds fail, all setup/static/recurrence controls pass. No phase/NOP fit, hidden-state force, setup retry or production candidate was used. Generated RAM supplies content; no accepted-fetch provenance claim is made beyond the observed public ink edge.

The filter's two-line estimator correctly retains period256 CE4 clocks. Its first-HS-of-VSYNC alignment anchors phase at one endpoint; no between-line phase averaging follows. The failure belongs to acquisition phase, not numeric R2 decoding or the guest's raw sync generation. The ordinary width4 RGB correction in trial `4e68533` leaves this width6 case unchanged.

## Receipt and next step

Root read the driver, waveform receipt and state observation, then verified all46 source and30 artifact hashes. The private pack is `local/overnight-20261103/resumed-20261104/before-the-storm-r3/d7-phase-proof/`; artifact manifest SHA256 is `34c9cf2c43afd402e1e8665a2ee13aaf3feb0d02c965cd37ff3704a5fdf2cc9a`. `HANDOFF.md` records exact commands and limits. The frozen core is f95 filter/75b85 motherboard, before the later initializer-syntax CI repair.

The source-only plan proposes observing phase separately from the existing period/width counter and scheduling a coherent output phase from a measured pair. It deliberately preserves width classification, VRAM/RGB histories, no-HS fallback and vertical line ownership. French p135 supports the interior bound, but does not uniquely select a two-sample average, recovery or transient policy. One private phase-only candidate is now authorized with predeclared reversed-position controls; no production implementation or passing candidate is claimed. Any candidate must first preserve static positions, pulse widths/period, vertical line accounting and snapshot/output-policy controls; exact physical settling remains a separate acceptance question.


## Private phase candidate and acquisition coherence

A separate signed small-phase, two-observation prototype preserves the original period/width/classification path and schedules the next horizontal output phase. Its first continuity check exposed a shared1312-clock pre-window VS reanchor, not a candidate-only event: passive paired observation confirms identical old/new edges and all327 I/O-prefix rows. Validation now permits only that strongly qualified zero-phase legacy event before recording, with no exception inside the measurement window.

The measured means then pass in both directions, but keeping VBLANK at its old master-clock phase clips eight complete acquisition windows. That independently detected integration defect is corrected by using the actual horizontal blank counter for VBLANK's horizontal boundaries, while retaining the original vertical line counter and exact VSYNC timing. This changes a private integration assumption; no existing retained oracle is relaxed.

The final private filter `32308784a5d9fef3c699b8b885518ce29c8b03855a84e2bb1a56fef4ae7e4ca3` passes all seven ordinary CRTC1 patterns: static267/251/235M2; both orders46/47 give259 and both orders46/48 give251. All3,815 complete candidate and reference windows retain3072clocks. All70 vertical-blank transitions pair in polarity and absolute raw-line identity; horizontal timestamp shifts are retained, while VS remains master-tick exact. Complete functional state/input recurrence passes7/7; unchanged retained R3 and blank owners pass12/9. Root verifies51 source and60 artifact bindings. Exact source, one-run receipts and prior failures are frozen under `d7-phase-proof/candidate/coherent-blank/`.

Opus5.5 high review `20261105T110200Z-33065-b41c` conditionally permits a labelled experimental trial, with no source blocker within this cohort. Before transport counts as trial evidence it requires strict lint/final selection, both slow Plus B6 boundary/dynamic controls, full Quartus timing/latch checks and explicit common Type0/ASIC scope. The prototype removes per-line variance as well as moving the mean; French p135 also describes oscillation, so the passing mean bound is incomplete physical evidence. Band steps, waves, odd frames, loss/recovery and combined R2/R3 remain unproved. No tracked transport or phase RBF exists. The full report is frozen under `d7-phase-proof/phase-review/`.

## Full guest parameter audit

A fresh CPCSyntaxError run advances all ten D7 snapshot states and records48,779 R2/R3 writes. The reconstructed VRAM headers, verified self-modifying instruction bytes and stable trace groups establish five `(low R2, high R2, frame R2)` triples, repeated for A/B:

| Pair | Low | High | Outside-band frame |
|---|---:|---:|---:|
| A1/B1 |46|47|46|
| A2/B2 |46|48|46|
| A3/B3 |46|48|47|
| A4/B4 |46|49|48|
| A5/B5 |46|50|48|

The guest loads HL at54C1: low is immediate54C3/H and high54C2/L. OUT(C),H at54C6 and OUT(C),L at54CB produce trace PCs54C8/54CD; frame R2 is immediate54DC before the54DF trace point. Band RMR54BC changes from8E (Mode2) to8D (Mode1). Only three automatic BMP filenames remain and their apparent page labels are unqualified; they are not ten accepted screenshot/page comparisons. All ten snapshot headers were reconstructed and inspected independently. `d7-acceptance/ALL10-PARAMETERS.json` and `ALL10-TRACE-GROUPS.json` preserve this binding.

These actual patterns expose a coverage gap: the existing private proof alternates indefinitely relative to the first endpoint. It does not exercise a band with a different frame reference. The final two patterns fit the prototype's absolute±8CE4 phase cohort around frame R2=48, but their consecutive-edge deltas reach12/16CE4, outside its cadence policy. The separate source-derived band discriminator below confirms that policy gap. An interior frame reference can itself satisfy the interior-mean bound, so that bound alone cannot qualify these cases.


## Qualified guest band discriminator

All ten snapshot payloads execute through the production T80 and public snapshot loader with frozen323 and a simultaneous old-filter shadow. Only the snapshot PC is changed to54AE, the original CALL A241 synchronization entry; every other header/payload byte is unchanged. This is an edited-entry source discriminator, not an authentic full boot. Unchanged snapshot replay fails qualification because the classic loader restores registers without the CRTC beam counters. An initial five-frame observation window also fails nonvacuity before the guest finishes synchronization. Both failures remain preserved.

The final predeclared window starts only after one complete source-qualified guest band/frame cycle and the next natural frame origin, with a64-frame safety cap. All ten start measurement after12naturalframes. Each then executes three312-line frames with exactly100 alternating low/high writes per frame atC0=2, C4=10+floor(i/8), C9=i%8, and one frame restore atC0=3/C4=22/C9=4. Raw inputs recur across frames and match between A/B modes.

Observed candidate band-minus-frame acquisition positions are−32/−64/0/0/0 master clocks for the five triples in both modes; the old filter remains0 throughout. The centered third pattern retains its phase epoch. The last two instead invalidate it at the first wide interval every frame:268/272CE4, although absolute phase+4/+8 remains within the declared cohort. There are three such rejections in each three-frame run, in both modes. A zero relative displacement alone cannot distinguish a centered result from this rejection, and no exact physical gain is asserted.

Independent controls preserve8,180 complete3072-clock DE spans in each candidate/reference path,60 paired VB transitions at identical natural lines/timestamps, exact VS timing and continuous legacy state. Root reads the driver analysis and verifies all168 artifact and48 frozen-source bindings. The private `band-discriminator/HANDOFF.md` is SHA256 `a05ca97ff46d6361c4628173ad27a7e7cd9146ae9bd4d932948da74c92103ccd`; its artifact manifest is `22a809f1c311e0f374bbc465cf93f87fa93b97291cd99f15aff71c937aec8862`.

A separate private correction is authorized to make the declared phase cohort and one-line cadence check internally coherent, with explicit arithmetic widths and unchanged guest/blank/vertical controls. No tracked adoption follows from this model-consistency repair. Physical oscillation, transients, recovery, combined R2/R3, Type0/Plus and the strong review's named acceptance controls remain separate gates.


## Marker-qualified software pictures

A fresh isolated v0.12.0 normal-boot run waits for each guest marker0383–038C before explicit capture/snapshot and the next Space. It exits0 with20BMPs and10v3snapshots. Each automatic marker image is RGB-identical to its explicit partner. Root verifies all ten snapshot instruction/parameter bindings against the table above and inspects the ten unique images alongside all ten original-CPC photographs. The wider-pattern diagonal/beat families are visible in the peer as well as the photographs; this does not assign a numeric CRT-geometry, gain or settling verdict.

The earlier fixed-delay run advanced without waiting for the requested marker picture. Its ten parameter snapshots remain useful, but its three automatic-image names are not ten page comparisons. The marker-gated workflow eliminates that limitation without an emulator change. Exact script/argv, logs, page/image hashes, snapshots and comparison contacts are under `d7-acceptance/cpcse-all10-ssm-qualified/`. The executable remains SHA256 `f4254653b11676055eedd89dcdae6c6aab381835344d69780f56fcbff218a577`, matching source tagv0.12.0; packaged UK ROMs and `--no-chain --no-errata` are explicit. These are fresh peer observations, not a refreshed MiSTer/photo compliance tally.
