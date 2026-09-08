# B8-6 Plus colour boundary

Task branch `codex/plus/b8-6-colour-alignment` was accepted at `023d020`
from exact requested base `d46609d066aafb6b182fd6fa504a91719500cd91`.
**Refreshed READY for integration** onto exact Accuracy destination
`35ae03b0dc8b8708bf963dcddf89c4c0e5c0e451`. Refresh reconciles shared manifests
and status documentation only; Plus RTL/test behavior is unchanged. Both fixtures
remain in the default test and lint gates. The identical waiver commit `f12f998`
is already present as `d52152a` and is not duplicated.

The original Plus source passed the user-selected Verilator 5.052 local gate
and scoped independent reviews below. The coordinator owns refreshed merged
gates, whose expected CRTC soak is now Accuracy's `0x6e8258198d6e6137`.
The older hash below records pre-refresh evidence, not a new expected value.
Plus has not been integrated, synthesized, or tested on hardware.

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
- Final `make -C sim` and `make -C sim lint` pass on Verilator 5.052 after the
  separate build-only compatibility commit `f12f998f8107b7a07871944149ac22889b0ad0c8`.
  The exact-base strict motherboard lint previously exited 2 on five existing
  `SIMILARNAME` warnings. `sim/plus/legacy-similar-names.vlt` waives only those
  five file/message pairs; the Makefile loads it before sources in the two
  affected strict recipes. No RTL was renamed, no blanket category suppression
  or `-Wno-fatal` was added, and all existing warning policies are otherwise
  unchanged. One-off controls still exit 1 for an unlisted similar name and
  for width truncation on a listed name.
- The CI pin remains 5.050. `-Wfuture-SIMILARNAME` provides the documented
  older-version handling of the rule name; this was source/documentation
  reviewed, not executed on 5.050. See the official
  [warning classification](https://verilator.org/guide/latest/warnings.html#cmdoption-arg-SIMILARNAME)
  and [future-message option](https://verilator.org/guide/latest/exe_verilator.html#cmdoption-Wfuture-message).
- Two temporary 5.050 build attempts failed on Apple/upstream Flex signature and
  header mismatches. Logs are preserved; provisioning stopped when the user
  selected the narrow 5.052 gate adjustment. No local 5.050 fixture run, CI run,
  synthesis result or hardware closure is claimed. New timed-fixture and waiver
  compatibility with the unchanged CI pin remains a CI acceptance residual.

## Independent review

Anthropic `claude-opus-5`, effort `high`, reviewed the complete range
`d46609d066aafb6b182fd6fa504a91719500cd91..51b61e3557c44480eb2f935f7d05a779acc557f7`.
Guarded run `20260908T010716Z-66508-26b4` exited 0 with clean handoff; the
bridge reaper confirmed no remaining live run. Verdict: **CLEAR on correctness**,
with no defects in the seven-file diff. Review covered the pipeline algebra,
line-by-line extraction fidelity, classic controls, test oracle, and top/QIP wiring.

The reviewer inspected sources and retained logs. Its `verilator --version`,
focused test, and mechanical extraction-diff commands were automatically denied
by its sandbox. This is independent source/log review, not independent execution.
The reviewer identified the new `--binary`/`--timing` fixture's compatibility
with the repository's 5.050 pin as a pre-merge risk. The user's subsequent gate
instruction accepts the updated 5.052 local gate; unexecuted 5.050 compatibility
remains explicitly pending CI evidence.
The full output and process status are retained in the ignored evidence directory
as `video-opus-review.log` and `video-opus-review-process.json`.

The optional coverage gap is explicit: VSYNC alignment transitions are checked
on native CE, while frame-selected routes check changing HSYNC/HBLANK/VBLANK
with steady cadence. Training crosses VSYNC, but those transitions are outside
the frame-selected scoreboard window. Full mixer/freezer behavior remains outside
this patch. The historical B8 audit retains its original source anchors.

The separate two-file gate delta, committed as `f12f998`, received a
supplementary Google `gemini-3.8-flash-high` review (high effort route), run
`20260908T012217Z-93148-1064`: **CLEAR**, exit 0, clean handoff, no remaining
live bridge. It inspected source, official documentation and parent execution
logs, without independent execution. Treat this as a lower-capability,
lower-confidence review than the Opus RTL review, limited to waiver scope,
command ordering, warning policy and documented older-version handling.
It is not an Opus-equivalent RTL certification or proof of a 5.050 run.
Its wording about all warnings remaining fatal is qualified here: the actual
change preserves the existing severity/suppression policy, which already had
exceptions. The one-off controls substantiate the narrower claim above.
The raw review and process status are retained as `video-gemini-gate-review.log`
and `video-gemini-gate-review-process.json`. This review was already running
when the user selected Muse for subsequent reviews.

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
