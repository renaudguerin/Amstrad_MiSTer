# DSC4 and Amazing CRTC1: horizontal lines also occur in AmSpirit

A fresh normal-boot AmSpirit run reproduces long, thin green horizontal
lines during DSC4's main effect. The symptom is therefore shared with the
software reference; it is not evidence by itself of a MiSTer-only defect.
Physical-CPC occurrence and matched-phase equivalence remain unverified.

## Controlled reference run

AmSpirit Lite1.17.1/core2491682 uses CPC6128, CRTC1,128K, French ROM and
monitor Off. Original disk SHA256 is
`ec9e0d51b10a7455848bd9211793c585c46456759486057b12a491039923b8e4`.
The command is `RUN"-DSC4`. There is no snapshot input, RAM/PC/counter
patch, breakpoint or guest-code instrumentation.

After boot and command completion, nine paused settled captures are taken
at requested offsets2500..3300 frames, spaced100 frames apart. These are
approximately50..66seconds after autotyping completes, not phase-matched
video timestamps. Actual frame/tick readings and screenshot metadata are
recorded per capture. The instance is paused after the last capture;
scripts and breakpoints are empty.

Root views all nine images in a contact sheet and the3100/3200 images at
full size. The main picture and subsequent animated effect are visible.
The3100 image has an unambiguous green line across the black area on both
sides of the picture, at doubled-image rows402..407. Later images also
contain long green horizontal segments. The acquisition is768×542,
representing AmSpirit's271-row viewport with each native row doubled.

The [earlier native control](classic-ga-native-2026-11-05.md) already shows
the symptom on e24cb50 as well as the experimental GA implementation.
The fresh reference comparison supplies the missing software-reference
presence check. Different animation phases do not establish equal
locations, durations, rates or pixels, and agreement between emulators
does not establish physical hardware correctness. No CRTC/GA/filter
change follows from this observation.

This is separate from the static image's
[extra bottom row](dsc4-bottom-row-crop-2026-11-04.md): that row also exists
in AmSpirit's uncropped buffer but is outside its default viewport.

## Evidence and execution limit

Artifacts are under ignored
`local/overnight-20261103/resumed-20261104/dsc4-c1-lines-reference-002/`:
normal-boot script/log, input/config/render, launch origin, all nine
screenshots and their timestamps, final state/snapshot, cleanup, contact
sheet and `PARENT-ACCEPTANCE.json`. The receipt records hashes and a
diagnostic count of green pixels at the outer edges; that count is not
a physical timing oracle.

## Amazing: the later coloured bands also occur in the reference

A separate fresh normal boot uses the same emulator/configuration and
`RUN"AMAZING`, disk SHA256
`4a631dabab9d8b9a916620a0e0b78f634daeae7736aee3c06b6c0e9979f1e8c9`.
Six settled captures at requested offsets1500..2000 frames show the
scrolling logo, lower text and subsequent thin coloured bands across the
text region. Root views all six in a contact sheet and1700/1900 at full
size. These bands resemble the family present in the
[native preservation images](shaker-b9-current-reference-2026-11-05.md).
They are therefore also a shared software-reference observable, not by
themselves a MiSTer-specific failure. Exact phase, rate and physical-CPC
behavior remain unverified; this is not full-demo sign-off.

The six images, config/launch/timestamps, script/log, final state/snapshot
and paused/empty-script/empty-breakpoint cleanup are preserved in sibling
`amazing-c1-lines-reference-001/`. Both reference runs finish successfully.

The first attempt stopped before boot/launch because `freq_screen` is
reported by1.17 but is not a writable config field. Its script/log and
cleanup remain in sibling `dsc4-c1-lines-reference/`. The completed run
uses the supported default real-time mode. The failed setup supplies no
demo result. No full-demo completion or broad baseline is claimed.
