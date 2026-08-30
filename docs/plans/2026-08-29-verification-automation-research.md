# Raster simulation and MiSTer hardware-in-the-loop research brief

## Short answer

Both proposed loops are technically plausible. Neither is a small extension of
the current tests.

- Full Z80/software-to-frame simulation is possible and resembles established
  system-level simulation and emulator differential testing, but it requires a
  production-faithful CPU/bus/memory/video harness, deterministic media, and a
  visual oracle tolerant of analogue and capture differences.
- MiSTer hardware-in-the-loop is likely the more authoritative route, but no
  existing path in this repository currently injects arbitrary inputs from
  Linux and captures the core's final FPGA video as a shared framebuffer. The
  MiSTer framebuffer interfaces found in `sys/` are not yet evidence of such a
  capture path.

## Current foundations and gaps

The repository already has useful pieces:

- deterministic CRTC and Plus leaf benches;
- a real-T80-shaped P10 boot harness using a Verilog TV80 core under the T80pa
  wrapper contract;
- production motherboard, cartridge parser/MMU/SDRAM paths in simulation;
- GA/ASIC pixel tests and PNG-capable rendering experiments;
- synthesis and real-hardware milestone procedures.

It does not yet have one harness containing the full `Amstrad.sv`/OSD/media
integration, real u765+DSK behavior, production input devices, final scaler/video
capture, and a known-working real title as a control. The P10 harness must not be
called a real-title oracle until a known-working CPR reaches stable execution in
it.

## Track A — full software-to-raster simulation

Research and prototype in this order:

1. Harden the P10 harness with a known-working CPR and observable milestones:
   reset vector, ASIC unlock, stable PC range, interrupts, CRTC/ASIC writes, and
   expected cartridge-page ownership.
2. Add production FDC and deterministic DSK responses before using BASIC/System
   cartridges as an oracle.
3. Connect the production pixel path to a frame collector with exact dot/line/
   frame metadata. Store raw digital RGB and sync first; PNG is a presentation
   artifact.
4. Build small software fixtures before whole demos: RMR2 relocation, PRI timing,
   SSCR wrap, sprite write/fetch collision, and R2.JIT.
5. Add title-level smoke tests only after one working title is a positive control.

Screenshot comparison should be a higher verification tier, not the source of
unit-test expected values. Prefer region masks, exact digital traces, feature
measurements, or perceptual thresholds with a documented reason. SHAKER
photographs remain the hardware oracle where the ACCC and current model disagree.

## Track B — MiSTer hardware-in-the-loop

Investigate three capture/control routes:

1. A purpose-built HPS↔FPGA test bridge that injects keyboard/joystick/media
   actions and exposes final digital video or selected internal probes.
2. Existing MiSTer framework facilities for scripted input and framebuffer or
   direct-video capture. Verify directionality: a Linux framebuffer overlay is
   not automatically an FPGA-output capture buffer.
3. External HDMI capture plus network-controlled MiSTer input as the least
   invasive hardware oracle. It may reach useful automation sooner than an HDL
   framebuffer bridge.

For each route, measure determinism, timestamp alignment, reset/reload control,
capture fidelity, required MiSTer framework changes, and whether a stock core can
serve as a regression comparator.

## SHAKER CSL/SSM automation

Treat CSL/SSM as a protocol-research subproject:

- locate authoritative protocol documentation and existing implementations;
- enumerate required commands, timing, media, input, and screenshot operations;
- determine which operations belong on HPS, in FPGA debug logic, or in an
  external controller;
- implement a minimal handshake and one named SHAKER test before attempting the
  full suite;
- record every test by module/key and compare against Logon System references,
  never only against the upstream core.

## Decision gate

Choose the first investment after short spikes:

| Route | Authority | Engineering cost | Earliest useful result |
|---|---|---:|---|
| Directed RTL/software fixture + raw raster | Model only | Medium | Fast |
| Full-title Verilator frame | Model only | High | Medium |
| Network input + external HDMI capture | Real hardware | Medium | Medium |
| HPS/FPGA framebuffer/probe bridge | Real hardware | High | Slow |
| Full CSL/SSM automation | Real hardware + protocol | Very high | Slow |

The recommended first breakthrough is a directed production-path raster fixture
paired with external hardware capture of the same fixture. It closes one loop
without requiring a complete emulator or a complete SHAKER controller.
