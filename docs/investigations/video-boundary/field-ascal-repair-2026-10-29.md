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

`amstrad_video_output` exports one field identity per displayed frame: capture
the aligned converted FIELD at the first enabled active pixel after VSYNC,
then hold it until the next frame. Capture happens before the mixer/gamma
pipeline so the identity settles before final-output DE. The existing raw
FIELD observation tap, interlace/scandoubler history, motherboard signal and
SSM marker input retain their previous meanings. No CRTC rule, pixel, sync or
blanking calculation changes. The shared MiSTer ASCAL source is unchanged.

Sampling at VSYNC alone was tested and rejected: the CRTC changes genuine
interlace identity later in the blank interval, so that version reversed the
field ordering relative to the previous raw export. The regression compares
the actual consumer headers against a raw genuine-interlace reference, rather
than merely checking that both field values occur.

## Deterministic evidence

`sim/video_field_test_top.sv` instantiates the real CRTC and production output
chain. Ordinary register writes generate progressive frames, repeated short
R8 pulses, held interlace, and return to progressive. The C++ driver records
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
three progressive headers. Final result: `ascal_field_test: PASS`.

The VHDL fixture observes the input framebuffer writer with its output clock
stopped. It proves the real consumer's classification and field ordering; it
does not emulate a complete HDMI image, memory readback or output weave. The
required next acceptance step is session A's cold-boot MiSTer rerun of the
unchanged private SCOPE0/SCOPE1 disk on the fixed RBF, compared with the saved
baseline and AmSpirit captures. Full synthesis/timing and hardware results
must be recorded separately from this local test.

Private red/green logs are under `.roster-scratch/v3-field-fix/`; the pulse
failure is `compact-red.log`, the rejected VS-only ordering failure is
`vsync-sample-order.log`, and the final CE-qualified result is
`ce-qualified-green.log`. Supplied media and copied title code remain private.

The bench runs in the existing GHDL-equipped `production-t80-test` CI job.
The normal selected gate remains usable without a local GHDL installation;
the test index classifies this additional consumer check as slow.
