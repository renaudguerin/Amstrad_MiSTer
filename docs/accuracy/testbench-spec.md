# Verilator CRTC Testbench — Specification

Goal: a **macOS-native, no-Quartus** simulation harness for `rtl/UM6845R.v` that encodes the
Compendium rules (see the digests in this directory) as executable assertions, so that any
future agent or human can verify CRTC changes in seconds instead of a 30-minute synthesis +
hardware test. This is the verification backbone for findings F3–F10 in `audit-findings.md`.

## Why Verilator

- Runs natively on Apple Silicon (`brew install verilator`), compiles Verilog to C++.
- `UM6845R.v` is self-contained, pure synchronous, single clock — ideal DUT. No vendor
  primitives, no PLL, no sys/ dependencies.
- The one blocker: `UM6845R.v` uses `/* verilator lint_off WIDTH */` already — the author
  anticipated this. Expect only minor lint noise.

## Architecture

```
sim/
  Makefile              # verilator --cc UM6845R.v --exe sim_main.cpp; make -C obj_dir
  sim_main.cpp          # harness: clocking, register I/O helpers, trace hooks
  crtc_script.h/.cpp    # tiny "test program" interpreter (see below)
  tests/
    t01_readback.txt    # one file per test case
    t02_vsync_midline.txt
    ...
  golden/               # optional captured signal traces for diffing
```

### Clocking model

The DUT wants `CLOCK` (system clock) with `CLKEN` pulsed once per character (1 MHz grid) and
`nCLKEN` on the opposite phase — mirror how `Amstrad_motherboard.v` drives it (CCLK enables from
the GA at 1MHz out of a 16MHz grid). The harness should tick `CLOCK` 16× per character with
`CLKEN` on tick 0 and `nCLKEN` on tick 8. Keep a cycle counter in **character units** (= µs)
as the primary timebase; all Compendium rules are expressed in it.

### Register I/O helper

Emulate the Z80 OUT timing at the granularity that matters: a helper
`out_bc(addr_sel, value, at_c0_phase)` that asserts `ENABLE/nCS/R_nW/RS/DI` for the correct
number of CLOCK ticks aligned to a requested C0 phase. The Compendium's dynamic-update rules
are all "write lands at C0==X" — the harness must be able to place a write on an exact
character boundary, and (for OUTI-vs-OUT(C),r8 rules, digest-01 §8.5) one tick later. Model
only the CRTC-visible strobe timing, not full Z80 instruction execution.

### Test script format

Plain-text micro-language, one directive per line, so tests are reviewable data rather than
C++ (future agents can add cases without recompiling knowledge):

```
# t02: CRTC0 mid-line R7 write extends VSYNC by R0-C0vs (digest-02 §19, ACCC §16.4.1)
type 0
init R0=63 R1=40 R2=46 R3=0x8E R4=38 R5=0 R6=25 R7=30 R9=7
run_lines 100                 # settle into a stable frame
at_line 200 c0 20 write R7 <current_C4>   # symbolic: harness resolves current C4
expect vsync_start within 1 char
expect vsync_len 16*64 + (63-20) chars    # arithmetic on R0/C0
```

Keep the interpreter dumb: `type`, `init`, `run_lines`, `run_chars`, `at_line/at_c0 ... write`,
`expect <signal> <edge|level|duration>`, `expect_reg`, `read` (for readback tests),
`dump_vcd on|off`. Symbolic values (`<current_C4>`) and simple arithmetic are worth the ~100
lines of parser they cost.

### Assertions / observables

Expose and check, per character cycle: `HSYNC`, `VSYNC`, `DE`, `MA`, `RA`, `FIELD`, `DO`
(readback), plus internal state via Verilator's public signals if needed (`hcc`, `line`, `row`,
`in_adj` — mark them `/* verilator public */` in a comment-only patch or use hierarchical
references; prefer observing outputs where a rule allows it, internals only where necessary).

Failure output must state: test file, line, character-cycle timestamp, expected vs actual —
and auto-dump a `.vcd` for the failing window (open with GTKWave/Surfer on macOS).

## Test inventory (initial — map 1:1 to audit findings)

| Test | Source rule | Protects/verifies |
|---|---|---|
| t01 register readback table, both types (R0-R31 sweep) | digest-03 §21.2/§28.1.9 | F1, F11c, F11d |
| t02 VSYNC mid-line R7 write, duration per type; blocked at C0<2 (type 0) | digest-02 §19 | F3 |
| t03 VSYNC re-entrancy: R7=0/R4=0 lock; R7=0,R4=1,R9=7,R3h=0 infinite bypass | digest-02 §18 | F11b (regression guard) |
| t04 R3l rewrite mid-HSYNC: overflow-to-16; type 1 R3l=0 cancels | digest-02 §4 | F11a (regression guard) |
| t05 R3l=0 static → no HSYNC (both types) | digest-02 §5 | F11a |
| t06 status bit 5 latch at C0=R0; not set by R6=0-while-C4>0 (type 1) | digest-03 §21.3.3 | F2 |
| t07 C9 overflow: R9 rewritten < C9 → counts to 31 and wraps | digest-01 §3 | F4 |
| t08 §28.1.1 ID: R4=36,R9=7,R5=16, R7 sweep → VSYNC ceases >37 (t0) / >39 (t1) | digest-03 §28.1.1 | F4 |
| t09 R0=0 freeze (type 0): counters halt, no HSYNC unless R2=0, resume clean | digest-01 §8.1 | F5 |
| t10 R1>R0: type 0 one border char at C0=R0, type 1 none | digest-03 §17.6.2 | F6 |
| t11 type 1 adjustment: C9 keeps cycling (RA!), C4 increments, R5=0 doesn't end | digest-01 §4 | F8 |
| t12 R9 write at exact C0==R0 (type 0): digest-01 §3.1 Ex.1/Ex.2 vectors | digest-01 §3.1 | F9 |
| t13 RFD: R5 0→1 at C0==R0, frame-parity VMA' alternation | digest-01 §5 | F7 |
| t14 VMA reload: type 0 only at C4=C9=C0=0; type 1 every line of C4=0 row | digest-03 §17.4/§20.3 | F11h + regression |
| t15 R12/R13 overscan-bit carry into MA[13:12] | digest-03 §20.5 | regression |

Write t01–t06 first (they pass against **current** RTL except where a finding is being fixed —
each fix PR flips its test from xfail to pass). t07+ document current divergences: mark them
`xfail` with the finding ID so the suite is green from day one and tightens as fixes land.

## Non-goals

- No Gate Array co-simulation (GA40010 is netlist-based and slow to simulate; CRTC rules are
  testable at the CRTC's pins). GA interaction rules (C-SYNC state machine, R52) are out of
  scope — they're already gate-accurate in hardware.
- No Z80 instruction-level modeling; the `at_c0 write` abstraction plus a ±1-tick offset knob
  covers the OUT/OUTI distinction the Compendium cares about.
- No cycle-exact pixel pipeline — DE/MA/RA at character granularity is the contract.

## Definition of done (for the implementing agent)

1. `make -C sim` runs t01–t06 green on current master (with documented xfails), < 30 s total.
2. A failing assertion produces a VCD + human-readable diff.
3. `docs/accuracy/audit-findings.md` verification levels V3 references become real: each
   finding's fix prompt names its test file.
4. CI-friendly: exits nonzero on failure; no interactive steps. (Optionally add to the GitHub
   Actions workflow from `docs/building.md` as a pre-synthesis gate.)
