# B8-1 Production-T80 Executed-Instruction Validation

**Completed 2026-09-08 for B8-1 production CPU write phases.**

This document records the bounded executed-instruction validation harness for B8-1.
It exercises real Z80 instruction sequences through the production `T80pa.vhd` core
synthesized to Verilog via GHDL, verifying CRTC write qualification between character
decisions against the French CRTC Compendium (ACCC v1.11).

## 1. Documentary Rules & Instruction Timing Windows

- **Primary Reference**: French ACCC v1.11 §13.7.1 p.126 (English p.124).
  `OUT(C), reg8` executes its I/O write on the 3rd microsecond; `OUTI` executes its
  I/O write on the 5th microsecond.
- **Timing Grid & Reference Edge**:
  - System clock: 64 MHz master clock; 4 MHz CPU clock (1 T-state = 16 ticks, 1 µs = 64 ticks = 4 T-states).
  - Opcode fetch reference edge: start of the instruction's first opcode fetch cycle,
    identified by `!M1_n && !MREQ_n && !RD_n && A == criticalPC`. Qualifying on `!MREQ_n && !RD_n`
    ensures the sample point does not alias with memory refresh addresses.
- **Documentary Windows vs Simulation Observations**:
  - `OUT (C), C`: §13.7.1 p.126 specifies I/O during the 3rd microsecond, defining the
    window `[128, 192)` master ticks from opcode fetch start. In the simulation harness,
    the write is observed at 152 master ticks (2.375 µs).
  - `OUTI`: §13.7.1 p.126 specifies I/O during the 5th microsecond, defining the
    window `[256, 320)` master ticks from opcode fetch start. In the simulation harness,
    the write is observed at 264 master ticks (4.125 µs).
  - The load-bearing assertion in each test is the documentary window (`[128, 192)` or
    `[256, 320)`). The measured tick counts (152 and 264 ticks) are simulation observations
    reported by the harness; they are not asserted as independently sourced documentary
    constants or claimed hardware-exact values.

## 2. First-Store System Edge Capture & Margin

The harness verifies CRTC register capture relative to CPU enables and character decisions:
- The CPU launches the I/O write cycle on the system clock edge where `CEN_p` is active.
- On the immediate next system clock edge (`tick+1`), the CRTC register file captures the
  new register value.
- A positive separation margin is verified between this first store and the subsequent
  character clock decision (`CLKEN`), and the observed tick margin is reported by the harness.

## 3. Architecture & Test Harnesses

1. **Native VHDL vs Verilog Trace Comparison** (`sim/t80_trace_tb.vhd`, `sim/t80_trace_test.cpp`):
   - Compares native GHDL simulation against Verilated `T80pa.v` netlist under clean enables
     (`CEN_p=1, CEN_n=1, WAIT_n=1`).
   - Bounded defined-field trace: 139 cycles compared across 8 specific bus signals:
     `A`, `DO`, `M1_n`, `MREQ_n`, `IORQ_n`, `RD_n`, `WR_n`, and `HALT_n`. Internal CPU state
     and non-traced pins (`RFSH_n`, `BUSAK_n`, `REG`) are not included in this comparison.
   - Only uninitialized startup `IntCycleD_n` (`'X'` on `IORQ_n` during cycles 6..8 before M1 T3
     reload `"11"`) and idle-bus `DO` are masked; all defined values on the 8 traced signals
     must match strictly.
   - OUTI stimulus distinguishes memory data from register C: uses `C = 0x43` with `ROM[0x20] = 0x42`,
     asserting port `0xBD43` and `DO = 0x42` (confirming `DO` reflects memory contents rather than `C`).
   - Enforces consecutive `CYC=` labels and requires completion through `HALT` (cycle 137) with
     at least 2 post-HALT cycles.
2. **Executed-Instruction Integration Harness** (`sim/crtc_t80_top.sv`, `sim/crtc_t80_test.cpp`):
   - Connects translated `T80pa.v` to real `ga40010` (clock divider, `phi_en`, `cclk_en`,
     and `READY` wait-states) and `CRTC.v`.
   - RAM model is a 256-byte test RAM holding test machine code and data.
   - OUTI integration case sets `C = 0x02` with `RAM[0x40] = 0x01`, asserting port `0xBD02`
     (B decremented `0xBE -> 0xBD`) and `DO = 0x01`, materially confirming data originates
     from RAM rather than `C`.

## 4. Executed Regressions & Discriminator Results

All 4 test cases run independently with identical assertions across current and pre-B8 engines:

| Test Case | Reference | Current Engine (`crtc-t80-test`) | Pre-B8 Engine (`crtc-t80-old-test`) |
|---|---|---|---|
| 1. Type 1 `OUT (C), C` R5 RFD | French ACCC §11.6 pp.89–92, §13.7.1 p.126 | `vma_flag=1`, `parity_flag=1`, `ma=0x1234` (PASS) | `vma_flag` remains 0 (FAIL) |
| 2. Type 1 `OUTI` R5 RFD | French ACCC §11.6 pp.89–92, §13.7.1 p.126 | `vma_flag=1`, `parity_flag=1`, `ma=0x1234` (PASS) | `vma_flag` remains 0 (FAIL) |
| 3. Type 0 `OUT (C), C` R0 Widen | French ACCC §13.7.2 pp.126–128 | `row=4 (R4+1)`, `line=3`, `c0=2`, `in_adj=1` (PASS) | `row` remains 3 (FAIL) |
| 4. Type 1 `OUT (C), C` R0 Widen | French ACCC §13.6.2 p.124, §13.7.1.2 p.126 | `row=3`, `line=3`, `c0=16`, `pending=1` (PASS) | `pending` remains 0 (FAIL) |

**Engine-Isolated Negative Control**:
`crtc-t80-old-test` pairs historical pre-B8 rule engines (`rtl/crtc_type0_engine.v` and
`rtl/crtc_type1_engine.v` at commit `d46609d066aafb6b182fd6fa504a91719500cd91`) with the
current `rtl/CRTC.v` wrapper. This setup serves as an engine-isolated negative control:
it isolates the engine qualification logic from wrapper changes. The pre-B8 engines evaluated
qualification only at `CLKEN`, by which time the register file had already captured the new
value, missing the qualification earned at write launch.

## 5. Build Targets & Execution

```sh
# Clean build and run with explicit GHDL path override:
make -C sim t80-trace-test crtc-t80-test crtc-t80-old-test \
  OBJ_DIR=/tmp/b8-t80-clean-20260908 \
  GHDL=/tmp/amstrad-tools-20260908/ghdl-llvm-6.0.0-macos15-aarch64/bin/ghdl
```

- Requirements: GHDL with LLVM or GCC backend (verified with GHDL 6.0.0 LLVM on macOS aarch64;
  mcode backend does not support executable target `-o`). Default `GHDL ?= ghdl`.
- `crtc-t80-test`: all 4 cases PASS (exit code 0).
- `crtc-t80-old-test`: executes unchanged test against pre-B8 engines, confirms all 4 named B8 side-effect failures, reports PASS on correct discrimination.

## 6. Scope & Non-Claims

- **Type 1 R0 Scope**: Case 4 verifies only pending qualification at the next character decision
  (`pending=1`); it does not test subsequent cancellation or full RFD completion across frames.
- **Harness Scope**: Native CPU comparison uses clean enables (`CEN_p=CEN_n=WAIT_n=1`). The
  `T80pa.vhd` `CEN`/`WAIT` path logic is exercised in the translated CPU harness against the
  real GA, but is not crosschecked against native VHDL under dynamic wait states (known P3 limitation).
- **RAM Scope**: 256-byte test RAM, not full motherboard dynamic RAM arbitration.
- **Integration Scope**: Confined to specific `OUT (C), C` and `OUTI` write recipes. Hardware
  testing, full top-level integration, and general software title certification remain open.

## 7. Accepted gates and review

The final candidate is based on integration tip
`f9a9835468e769ca7dec9ce44e9a284a9314af0e`. Production RTL is unchanged.
The final refresh changed only documentation; the preceding refresh added host tooling
and documentation without changing these simulation sources.

- Parent `make -C sim`: PASS on the final simulation changes, including the existing
  TV80/FDC XFAIL. Aggregate lint: PASS. Soak: PASS at `0x6e8258198d6e6137`,
  seed `0xaccc5eed20260822`, 2,845,088 samples. Lint/soak were retained across the final
  optional-test refinements and documentation-only refresh; no production RTL changed.
- Fresh GHDL/Verilator generation and optional targets: PASS. Current engines pass all
  four cases; the same assertions fail all four named B8 side effects with historical
  engines. Native comparison covers 139 samples of the eight listed bus fields,
  subject to the explicit startup/idle-data exclusions above.
- Parent mutation checks reject a trace truncated before HALT and a defined IORQ
  mismatch inside the startup window, both with exit status 1. The final matched-ROM
  revision retains these rejections.
- Printed margins 54/6 are observed ticks until CLKEN becomes asserted. The consuming
  decision edge follows one tick later (55/7 ticks). Only positive separation is
  asserted; these measured counts are not hardware-derived expectations.

Review is **SCOPED CLEAR**, combining the full-candidate Opus 5/high source review
`20260908T044213Z-37834-7f8e` with Astra/medium review of the final Gemini-authored
remediation. Opus accepted the B8 side-effect/discriminator substance and identified
the unsupported exact timing derivation; the final revision removes it and the Astra
closure found no material remaining issue. Astra's configured route was verified as
`gpt-6-astra`, effort `medium`, task `/root/t80_review`.

Opus could not access scratch artifacts/PDFs in its sandbox and ran no gates. Astra's
closure inspected the final patch, sources and accepted optional-run log without
rerunning gates. Parent execution and bilingual/visual source checks are separate
evidence. The attempted Opus closure `20260908T050430Z-66965-4b06` hit its session
limit and produced no verdict; the coordinator authorized the narrow Astra fallback.

Raw build, negative-control, mutation and review evidence is retained locally under
ignored `docs/references/b8-t80-2026-09-08/`. Generated netlists, PDFs and tool binaries
are not committed. The coordinator owns integration, push and any hardware follow-up.
