# FIELD export qualification for ASCAL

## Position

Short classic CRTC R8=3→0 pulses can change raw FIELD without creating an
interlaced displayed frame. The core previously wired that signal directly to
MiSTer's `VGA_F1`. Production ASCAL treats every input FIELD edge as interlace
acquisition (`sys/ascal.vhd`, `i_interlace`/`i_intercnt`), retaining the decision
for three frames. Repeated pulses therefore keep a progressive source in the
interlaced framebuffer path.

This integration defect is reproduced in simulation. It is a candidate cause
of the contaminated alternate rows in the SCOPE0/SCOPE1 MiSTer captures, not
yet a hardware-confirmed explanation. It does not settle the separate RFD save
policy question on an original CRTC1 CPC.

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
does not emulate a complete HDMI image, memory readback or output weave. The
required next acceptance step is session A's cold-boot MiSTer rerun of the
unchanged private SCOPE0/SCOPE1 disk on the fixed RBF, compared with the saved
baseline and AmSpirit captures. Full synthesis/timing and hardware results
must be recorded separately from this local test.

Private red/green logs are under `.roster-scratch/v3-field-fix/`; the pulse
failure is `compact-red.log`, the rejected VS-only ordering failure is
`vsync-sample-order.log`, and the rejected first-active-pixel ordering failure
is `border-order-red.log`. Supplied media and copied title code remain private.

The bench runs in the existing GHDL-equipped `production-t80-test` CI job.
The normal selected gate remains usable without a local GHDL installation;
the test index classifies this additional consumer check as slow.
