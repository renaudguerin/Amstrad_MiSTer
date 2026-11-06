# CPCSyntaxError testing and source comparison

Use CPCSyntaxError for fast, instrumentable SHAKER experiments. Establish which
observables match the original-CPC photographs before using its implementation
to explain a MiSTer failure. Its source provides hypotheses; hardware and the
French ACCC remain the rule authorities. AmSpirit remains our closest software
reference. Use it when CPCSyntaxError fails
the relevant hardware comparison or its timing model cannot answer the question.

## Installed tool and source

The user-supplied macOS universal v0.12.0 package is ignored at
`local/CPCSyntaxError-macos-universal-v0.12.0/` in the main checkout. The current
validation worktree also has a private copy for headless batches. The package
keeps settings inside the app: an isolated copy avoids sharing those files with
the open GUI. Neither the package, its firmware nor captured media belongs in Git.

- Executable: `CPCSyntaxError.app/Contents/MacOS/cpcse` inside the package.
- Client: `cpcse-ctl` beside the app.
- Firmware: `CPCSyntaxError.app/Contents/Resources/roms`.
- Matching source: `~/code/CPCSyntaxError`, tag `v0.12.0`, commit
  `22de765ef44da46c55217e4dd7caa385d69a6045`.
- Executable SHA-256:
  `f4254653b11676055eedd89dcdae6c6aab381835344d69780f56fcbff218a577`.

Read the matching source's `README.md` (CSL/SSM and headless sections) and
`docs/external-debugging.md` for additional operations. Pin a new binary and
source revision together before attributing new output to the inspected code.

## Headless CSL batches

The bundled executable runs CSL without opening a window. This route was
exercised on macOS with the supplied v0.12.0 binary; it does not need the command
API or a replacement client library. From a checkout with the private inputs:

```sh
CPCSE_DIR="$PWD/local/CPCSyntaxError-macos-universal-v0.12.0"
"$CPCSE_DIR/CPCSyntaxError.app/Contents/MacOS/cpcse" \
  --csl "local/test_media/shaker/CSL_27/MODULE C/SHAKER-C-1.CSL" \
  --out local/investigations/cpcse-c1-unique-run \
  --disk-dir local/test_media/shaker \
  --roms "$CPCSE_DIR/CPCSyntaxError.app/Contents/Resources/roms" \
  --model cpc6128 --crtc 1 --no-chain --no-errata
```

Use a fresh output directory. `--no-chain` limits execution to the named script.
`--no-errata` runs the supplied script without CPCSyntaxError's built-in A-U wait
extension. The default extension may be useful, but record it rather than
silently calling the input identical. Keep the author's canonical scripts intact.
The MiSTer French B-1 key override is specific to its keyboard translation; do
not automatically apply it to a CPCSyntaxError run with different firmware.

For Plus, use `--model cpc6128plus --crtc 3`. The player boots its configured
system cartridge into BASIC; record the cartridge named in the log. The first
pilot used the packaged English Burning Rubber cartridge, whereas MiSTer used
its pinned French cartridge. That difference is part of the evidence setup.

SSM captures are named `CPCSE_<crtc>_<HHLL>.bmp`; retain the originals and log.
Convert copies to PNG for inspection if needed. The completion count establishes
execution coverage, not that every captured page matches hardware. Check page
parameters and state before comparing pictures, especially dynamic tests.
For marker-producing SHAKER pages, use `wait_ssm 0xHHLL` before taking a
snapshot or advancing. The D7 fixed-delay pilot reached ten parameter states
but retained only three automatic marker images; waiting for each marker
produced all ten correctly bound image pairs. Extra fixed waits do not establish
that the requested marker picture has completed.

CSL supports snapshots and explicit snapshot versions 1–3. Verify the saved
version before interpreting its CRTC-counter fields. Standard snapshots still
omit some hidden latch/phase history: restarting an initializer from a later
snapshot is not equivalent to an original boot and can modify the title's RAM.

## Built-in source traces

Before rebuilding for instrumentation, inspect the pinned source for existing
`CPCSE_TRACE_*` environment switches. The supplied v0.12 binary's register trace
was exercised with `CPCSE_TRACE_REG=7 CPCSE_TRACE_REG_MAX=8000`; it logs payload,
C0/C4/C9 and PC to stderr without changing the captured02F8 BMP. Register lists
such as `4,7,9` and a starting CRTC-frame filter are supported by `crtc.cpp`.
These are source-defined diagnostic controls, not a stable external API.

The logged PC is the CPU PC at I/O dispatch and can be after the opcode; account
for this when comparing a production-T80 instruction-start trace. A snapshot
that omits counters may retain a wrong phase indefinitely if the restored loop
bypasses its one-time synchronizer. Repeated output alone does not validate it.
For transfer into the current classic FPGA loader, even a CPCSyntaxError v3
snapshot does not preserve beam phase: `rtl/CRTC.v` takes only `SNA_ADDR` and
`SNA_REGS` on restore, without counter inputs. The D7 guest loop can therefore
resume on the wrong raster. Qualify its actual I/O coordinates; an explicitly
labelled return through the guest synchronizer is a separate input experiment,
not an unchanged snapshot replay.

## Headless firmware/input qualification

In v0.12.0, `csl.cpp:1069` passes a hardcoded UK layout to `key_output`.
Own diagnostic runs with explicitly loaded French ROMs therefore typed
`run2...` instead of the intended quote/digits. Package-default UK ROMs fixed
that typed input, but those two bounded runs still did not reach the guest
by their saved checkpoints. Neither set supplies guest counter/PPI evidence.
Pin actual ROM bytes and require code/guest-write guards before interpreting
RAM; a zero result buffer does not prove a test returned zero.

A subsequent bounded UK run uses the explicit `RUN"R7I0.BIN"` filename and a
10-second post-input wait. It loads all 623 guest bytes, reaches the expected
R7 writes and terminal HALT, and records initial PPI bit0=1 with normal pulse
fall. These combined loader changes qualify that recipe; they do not isolate
extension fallback versus motor delay as the earlier cause. Its target trace
is C0=0/C4=10/C9=1 at the character-rounded dispatch, while AmSpirit’s guarded
version samples0. This peer difference does not decide a physical aperture.
Keep the
verified canonical capture batches separate from these failed new loader
qualifications. [R7 qualification](investigations/hardware-runs/crtc1-r7-optical-address-qualification-2026-11-04.md).

Fresh original B6/019D is a positive example: v0.12.0 matches all six physical
results, including fifth/fourth R7 deadlines1E/1F where MiSTer remains1F/1F.
Its source advances hardware to the instruction-effect offset before dispatch,
then compares current C4. This supplies a carry-ordering clue, not a measured
sub-character aperture. [Qualified B6 result and limits](investigations/hardware-runs/crtc1-b6-cpcse-ordering-2026-11-05.md).

Fresh calibration now also distinguishes useful and unsuitable peer cases:
C0 C5/029E and C1 B9/01C8 match the photographed parity/delay groups and
provide useful source hypotheses. C0 E6 matches only9/28 printed checks,
and C1 AI/008D fails the short-line VSYNC samples that AmSpirit matches.
Use AmSpirit for those cases; a peer source algorithm cannot override the
physical photograph. [C5](investigations/hardware-runs/shaker-crtc0-c5-parity-2026-11-05.md),
[B9](investigations/hardware-runs/shaker-b9-current-reference-2026-11-05.md),
[AI008D](investigations/hardware-runs/shaker-crtc1-008d-current-2026-11-05.md).

Current CRTC0 A4 and B6 provide further useful successful peer paths. Available
2.6/2.7 direct A4 runs both give the photographed01/00, while native gives00/00;
its type1 peer instead gives00/00. One original CRTC0 B6 run matches all six
physical values27/14/26/27/1E/1E, versus native13/14/26/26/1F/1F. Preserve the
type distinction and qualify original CPU phase before changing write timing.
[A4 and AI](investigations/hardware-runs/shaker-crtc0-a4-ai-photo-check-2026-11-06.md),
[B6](investigations/hardware-runs/shaker-crtc0-b6-current-2026-11-06.md).

## Interactive inspection

The open GUI can be driven by its bundled `cpcse-ctl` client once its command
API is enabled under Settings/Debug → External debugging. It uses newline JSON
over TCP, **not HTTP**. Its default port 6128 conflicts with AmSpirit's HTTP
server; configure a distinct port such as 7000 when using both.

```sh
"$CPCSE_DIR/cpcse-ctl" --port 7000 status
"$CPCSE_DIR/cpcse-ctl" --port 7000 disasm 0x4000 16
"$CPCSE_DIR/cpcse-ctl" --port 7000 read 0xC000 32
```

The source also documents conditional breakpoints, watchpoints, stepping and
screenshots. These interactive operations have not yet been exercised in this
toolbelt validation; the verified route is headless CSL. Choose an existing
configured GUI for live inspection, rather than assuming a listener is running.

## Hardware-backed source investigation

1. Pin the disk, script/history, supported CRTC/revision, firmware and binary.
   Use fresh v0.12.0 captures; older portal CPCSyntaxError images are not results
   for this build.
2. Compare the relevant result with original hardware. Separate counter/status
   values and intended pixels from monitor-dependent shear, scaling and capture
   phase. Keep unmatched cases for AmSpirit checks.
3. Where CPCSyntaxError matches and MiSTer fails, locate the responsible state
   transition or caller in the pinned source. Trace an observable first
   divergence rather than copying a whole rule engine.
4. Check the hypothesis against the French ACCC and turn it into a focused,
   independently expected failing RTL vector. Then fix, run the selected gate,
   review and compare a synthesized artifact with SHAKER. Demos are milestone
   integration checks.

Known limits: its I/O-effect scheduler rounds advancement to character-sized
steps, so it cannot establish every physical sub-character bus boundary. Its
monitor renderer is partly fitted to SHAKER photographs. Some source rules
diverge from author-confirmed behavior; a local success does not validate the
entire model. See the [source audit](investigations/hardware-runs/emulator-source-author-question-audit-2026-11-03.md)
and [fresh pilot](investigations/hardware-runs/cpcsyntaxerror-v012-pilot-2026-11-04.md).
