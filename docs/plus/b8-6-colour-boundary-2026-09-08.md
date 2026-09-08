# B8-6 Plus colour boundary

Task branch `codex/plus/b8-6-colour-alignment`, exact requested base
`d46609d066aafb6b182fd6fa504a91719500cd91`. This is a simulation candidate;
independent review and the aggregate lint gate remain pending. Nothing has
been integrated, synthesized, or tested on hardware.

## Contract and change

A motherboard pixel consists of native RGB and HSync/VSync/HBlank/VBlank.
`color_mix` captures all four metadata bits and classic RGB on `ce_pix`.
`video_mixer` consumes this boundary through `gamma_corr` (with the freezer
passing metadata combinationally when freeze is off). Plus previously bypassed
the capture register, pairing the current pixel with the previous metadata.
This is a source-derived integration contract, not a new ACCC/CRTC rule.

The behavior-preserving extraction `f08a3ca5e4732314e14fa6f0e3e27b6ff5b9a6bb`
moves the existing colour conversion, pixel-enable acquisition/selection, and
machine colour mux into `rtl/amstrad_video_color.sv`. `Amstrad.sv` uses this
production module; `files.qip` includes it. The timing fix then captures the
converted Plus RGB in a single 24-bit register on the same `ce_pix` as metadata.
The classic DAC implementation is unchanged. Monitor conversion, selected CE,
progress overlay, CRTC windows, and sprite coordinates retain their behavior.

## Discriminating regression

`sim/plus/video_color_test.sv` instantiates the actual production module and
real `color_mix`/`gamma_corr`. There is no copied production conversion table or
mock gamma path. Its independent integer oracle uses nibble expansion by 17,
luma `floor((59R + 177G + 20B)/256)`, and amber green
`Y - floor((59R + 177G + 20B)/1024)`. A three-tuple scoreboard tracks the
conversion register and gamma's two rising-CE stages from input samples.
It checks RGB and metadata every system clock, including disabled-clock holds.

Coverage comprises native `ce_16`, HQ2x-selected `ce_pix_fs`, and status[30]-selected
`ce_pix_fs`; all four video modes; all eight monitor settings; classic and Plus;
gamma bypass and an inverting LUT. The actual frame-mode acquisition logic is
trained across VSYNC boundaries. Expected steady CE periods are 4 system clocks
for native/mode 2, 8 for mode 1, and 16 for modes 0/3. Classic uses an unchanged
production-DAC/gamma instance as its bit-for-bit control. Input colours change
both on and between CE samples; native-route checks include VSYNC transitions.

Before the timing repair, the assertion exited nonzero with 166,123 Plus
mismatches and zero classic, metadata, or cadence-control failures. After the
repair, all 182,272 checked cycles pass. The earlier retained diagnostic's
successful defect reproduction is not counted as an acceptance test.

Local logs live in ignored `docs/references/b8-audit-2026-09-08/`:
`video-acceptance-before.log` records the failure on unchanged extracted behavior;
`video-acceptance-after.log` records the repaired boundary. Both ACCC v1.11 PDFs
were provisioned and remain ignored; no new CRTC rule was inferred from them.

## Validation status

- Behavior-preserving extraction: full `make -C sim` passed; CRTC soak retained
  `0x2263c9fc44af4ee7` over 2,845,088 samples. This soak covers CRTC state, not RGB.
- Timing repair: focused fixture and full `make -C sim` passed; the existing
  FDC payload XFAIL remains. Final CRTC soak also retains the recorded hash.
- Raw `make -C sim lint` on installed Verilator 5.052 exits 2 at the existing
  `motherboard-lint` target on five `SIMILARNAME` warnings. The exact base
  Makefile replay against unchanged motherboard dependencies produces the same
  failure. The repository CI installer pins Verilator 5.050. No lint assertions
  or production dependencies were weakened to hide the tool-version issue.
- Fresh cross-provider review must cover the complete base-to-final diff,
  including extraction, top-level wiring, enabled register, and test oracle.

## Residual acceptance

The real conversion/CE/gamma boundary is exercised. The full vendor mixer,
scandoubler/HQ2x algorithm, freeze mode, full T80-driven motherboard/top-level
simulation, Quartus timing/fit, and named title/hardware retests are not covered.
In particular, choosing the HQ2x input pixel cadence does not certify the HQ2x
pipeline. The prior diagnostic encountered vendor `hq2x.sv` Verilator
incompatibilities; this patch does not modify that code or claim to resolve them.

FIELD ownership, palette-write events, snapshots, shared FDC/CPU work, and the
Accuracy peer's CRTC bus-event contract are outside this patch. The existing
P10 payload XFAIL and classic AMSDOS requirement remain intact. Simulation
alignment does not establish that a particular title or photographed symptom
is fixed.
