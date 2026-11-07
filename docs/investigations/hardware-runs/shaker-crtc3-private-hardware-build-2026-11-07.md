# CRTC3 private hardware experiment

Branch `codex/c3-br-private-hardware-20261107` starts fromef0c109, preserving
all acceptedcb3 functional owners except the compatible IRQ hypothesis in
asic_ga_timing.v. It contains no V1 receiver or private V1 build macro. All110
other RTL files and the top level match the frozen pre-V1 source.

The ASIC is executable-token-identical to reviewed privateb1a8c264; only stale
comments/private paths are corrected. The behavior includes delayed normal line
counting, prompt normal-wrap delivery, held source-qualified ACK/DMA retention
and the reviewed epoch-qualified VS-reset-line token. These are B26 model
choices; physical phase, PRI-write window and MRER aperture remain unresolved.

The original cart passes48/48 and18/18 source fields with complete CPU/HID/
SDRAM qualification. All six V5 photograph endpoints/counts and all68 DMA
photograph records match. The retained focused target passes118+26 assertions.
A one-shot neighbour aggregate remains179/180: its minimum-line MRER-release
setup assumes zero when a delayed count remains due. No expectation was changed.

The exact selection command `python3 sim/select_tests.py --run` is RED on
d02/r02 raw-Gate-Array equality. The old stimulus aliases the delayed character
sampler; it is not valid physical evidence for the new count placement, and no
assertion has been waived, removed or changed to obtain green. GNU make stops
queued targets after that failure. Historical PRI/snapshot disagreements remain
separate; this run does not claim those tests passed or even completed.

This branch is NOT READY for integration or release. Fresh Opus review permits
a labelled private fit/device experiment after the original-caller/V5/DMA
checks above. Full Quartus timing/resource verification is still required before
loading any produced bitstream. The manual build can produce a useful artifact
while the overall workflow correctly remains red on the recorded software gate.

Targeted device acceptance uses the original standalone Module B cartridge,
MRER and VS-threshold diagnostics and scoped CRTC3 preservation. It is not a
fresh SHAKER milestone baseline. Real6128Plus/C4CPC photographs are pending;
the32-line threshold differs between current MiSTer and AmSpirit.

Proof and source identities are retained under c3-br-private-hardware-20261107/
in the coordinator checkout's private session root. See the
[full model and review evidence](shaker-crtc3-br-delayed-count-2026-11-06.md).
