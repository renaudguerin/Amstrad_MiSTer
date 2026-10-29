# FIELD export qualification for ASCAL

## Position

Short classic CRTC R8=3→0 pulses can change raw FIELD without creating an
interlaced displayed frame. The core previously wired that signal directly to
MiSTer's `VGA_F1`. Production ASCAL treats every input FIELD edge as interlace
acquisition (`sys/ascal.vhd`, `i_interlace`/`i_intercnt`), retaining the decision
for three frames. Repeated pulses therefore keep a progressive source in the
interlaced framebuffer path.

This integration defect is reproduced in simulation and its repair passes the
bounded SCOPE0/SCOPE1 native-capture check on MiSTer: the contaminated alternate
plane disappears while each program's live-plane output is preserved exactly.
It does not settle the separate RFD save policy on an original CRTC1 CPC.

## Repair boundary

The classic CRTC adds a separate `FRAME_FIELD` export. It combines the existing
field flop with an interlace mask latched only at the same canonical frame
origin that updates that flop. A mid-frame R8 pulse therefore cannot leak to
this export. The motherboard routes it to `VGA_F1` in classic mode, retaining
the previous Plus FIELD path. The raw FIELD equation, observation tap,
interlace/scandoubler history and SSM marker input retain their meanings.
No CRTC parity/address rule, pixel, sync or blanking calculation changes.
The shared MiSTer ASCAL source is unchanged.

Sampling at VSYNC alone was tested and rejected because it reverses genuine
field order. Review also caught the same defect in a first-active-pixel
variant: production's visible top border can start before the CRTC origin.
The new border-inclusive fixture reproduces that ordering failure before the
origin-based correction. The regression compares consumer field ordering and
header geometry against the raw genuine-interlace reference, rather than
merely checking that both field values occur.

## Deterministic evidence

`sim/video_field_test_top.sv` instantiates the real CRTC and production output
chain. Ordinary register writes generate progressive frames, repeated short
R8 pulses, held interlace, and return to progressive. Each runs both with
display-only blanking and an adversarial visible top border whose blank ends
before the CRTC origin. This compact border input exercises the production
ordering hazard; it does not substitute for the complete GA/filter path.
The C++ driver records
the final CE-qualified HS/VS/FIELD/DE pins. GHDL replays these into the real
`sys/ascal.vhd`; assertions inspect its public Avalon framebuffer headers.
The consumer test also checks independent progressive, alternating-field and
pulse-acquisition/recovery controls.

Focused command:

```sh
make -C sim ascal-field-test GHDL=/opt/homebrew/bin/ghdl
```

Before the fix, the short-pulse case fails with settled header interlace bits
all `1`, despite progressive frame identity. Both the initial full-sized
raster and the compact permanent fixture reproduce this. After the fix,
progressive and short-pulse headers stay progressive, genuine interlace keeps
both fields in the same order as the raw reference, and recovery ends with
three progressive headers, in both border conditions. The final origin-based
correction, including exact genuine-interlace header geometry equality, ends
with `ascal_field_test: PASS` in `origin-green.log`.

The combined source includes master `bb0052a`. Its final selected gate,
`python3 sim/select_tests.py --run`, passes all 43 selected benches; the exact
`select_tests: PASS 43 benches: ...` line is preserved in
`origin-selected-gate.log`. The pre-existing FDC payload-poll expected failure
remains an XFAIL. No CRTC parity/address assertion was changed or weakened.

The VHDL fixture observes the input framebuffer writer with its output clock
stopped. It proves the real consumer's classification and field ordering; it
does not emulate a complete HDMI image, memory readback or output weave.
The separate full-build and device acceptance are recorded below.

Private red/green logs are under `.roster-scratch/v3-field-fix/`; the pulse
failure is `compact-red.log`, the rejected VS-only ordering failure is
`vsync-sample-order.log`, and the rejected first-active-pixel ordering failure
is `border-order-red.log`. Supplied media and copied title code remain private.

The bench runs in the existing GHDL-equipped `production-t80-test` CI job.
The normal selected gate remains usable without a local GHDL installation;
the test index classifies this additional consumer check as slow.

## Hosted build and bounded MiSTer acceptance

Session A accepted hosted full build
[40959680151](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/40959680151)
at exact source `46f8ab861cbe9cdd8a7ed23600dfb27fc2bc4c1b`. All required CI jobs
passed. Quartus 17.0.2 Build 602 used clean full synthesis and High Performance
Effort; minimum setup/hold slack is 0.508/0.210 ns, all TNS zero. ALM use is
24,575/41,910 (59%). Delivered `Amstrad_20261029_46f8ab8.rbf` has SHA-256
`e448afff055950bf1f3ace4bfc8613f08b3f4104e2fc9d19364787448c3ff553`, verified
against both the local and device copies.

A cold-booted each unchanged SCOPE program independently, requesting Full,
classic 6128/CRTC1, Plus off and normal CPU. Captures were requested at 10, 13
and 16 seconds after command completion. The applied Full latch was not
separately read for these non-SSM programs; manifests preserve the CFG and
actual command/capture times.

| Program | Native frame | Repeat stability | RGB differences from own old clean odd plane | Strong colour pixels |
|---|---|---|---|---|
| SCOPE0 | 768×273 | All three PNGs byte-identical | 0 | 0 |
| SCOPE1 | 768×273 | All three PNGs byte-identical | 0 | 0 |

The fixed native frames need no crop, row removal or resizing to compare with
their own old rows 1,3,…545. The contaminated alternate plane and doubled
height are absent. SCOPE0 retains C01→C02 and B00→B01 rearm progression;
SCOPE1 retains A02 and B00 repetition. These are predeclared preservation
controls, not new original-CPC rule expectations.

Same-program AmSpirit differences remain: per-channel >128 thresholding over
the common 768×271 normalized area gives 607/601 differing pixels for SCOPE0/1.
Cross-program counts remain 0/52. They are comparison metrics, not accuracy
scores; full-RGB preservation of each program's own old live plane shows that
this FIELD repair does not resolve those separate output/policy differences.
No emulator boot or model run was repeated for this hardware check.

Six native PNGs, manifests, metrics, labelled comparisons and build reports
remain private under
`local/investigations/classic-video-v2-2026-10-29-b157/scope-field-46f8ab8/`
and sibling `field-build/`. A restored the original CFG and verified MENU;
no device operation remains in flight. Native captures do not prove physical
HDMI waveform, analog fidelity or buffer freshness. Genuine-interlace order
and recovery are verified by the production-ASCAL simulation above.
