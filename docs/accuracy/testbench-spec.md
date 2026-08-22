# Verilator CRTC Testbench — Specification

Goal: a **macOS-native, no-Quartus** simulation harness for `rtl/UM6845R.v` that encodes the
Compendium rules (see the digests in this directory) as executable assertions, so that any
future agent or human can verify CRTC changes in seconds instead of a 30-minute synthesis +
hardware test. This is the verification backbone for findings F3–F10 and F12 in
`audit-findings.md`.

Primary reference: *The Amstrad CPC CRTC Compendium* v1.10 (Longshot / Logon System).
Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).

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

## Test inventory

Implemented groups are marked below. The executable suite is authoritative when this table
lags a sub-vector name.

| Test | Source rule | Protects/verifies |
|---|---|---|
| t01 register readback table, both types (R0-R31 sweep) — implemented | digest-03 §21.2/§28.1.9 | F1, F11c, F11d |
| t02 VSYNC mid-line R7 write, duration per type; blocked at C0<2 (type 0) — implemented | digest-02 §19 | F3 |
| t03 VSYNC re-entrancy: R7=0/R4=0 lock; R7=0,R4=1,R9=7,R3h=0 infinite bypass — implemented | digest-02 §18 | F11b (regression guard) |
| t04 R3l rewrite mid-HSYNC: overflow-to-16; type 1 R3l=0 cancels | digest-02 §4 | F11a (regression guard) |
| t05 R3l=0 static → no HSYNC (both types) | digest-02 §5 | F11a |
| t06 status bit 5 latch at C0=R0; not set by R6=0-while-C4>0 (type 1) — implemented | digest-03 §21.3.3 | F2 |
| t07a-t07m equality overflow and live VMA capture — implemented: R9<C9 counts C9 through 31/wrap; outside adjustment R4<C4 counts C4 through 127/wrap; a late live R9 match also feeds VMA' capture at C0=R1 | digest-01 §3/§7/§17.1 | F4, while preserving F12 arbitration |
| t08 §28.1.1 ID: R4=36,R9=7,R5=16, R7 sweep → VSYNC ceases >37 (t0) / >39 (t1); establish adjustment/overflow prerequisites explicitly | digest-03 §28.1.1 + digest-01 §4.2/§7.1 | F4/F12 discriminator, not shortcut-only proof |
| t09 R0=0 freeze (type 0): counters halt, no HSYNC unless R2=0, resume clean; C9=R9 entry consumes exactly one C4 increment — implemented | digest-01 §8.1 | F5/F12 boundary |
| t10 R1>R0: type 0 one border char at C0=R0, type 1 none | digest-03 §17.6.2 | F6 |
| t11 type 1 adjustment: C9 keeps cycling (RA!), C4 increments, R5=0 doesn't end | digest-01 §4 | F8 |
| t12 R9 write at exact C0==R0 (type 0): preserve documented C4=39,C9=8 result as the R9-to-R5 comparator-switch race, not a generic old/new-R9 sample; encode the companion windowed-write case beside it (R9 written in C0∈[2,R0−1] → C4=38,C9=8, ACCC p.82 ex.3) | digest-01 §3.1/§4.2 | F9/F12 |
| t13 RFD: R5 0→1 at C0==R0, frame-parity VMA' alternation | digest-01 §5 | F7 |
| t14 VMA reload: type 0 only at C4=C9=C0=0; type 1 every line of C4=0 row | digest-03 §17.4/§20.3 | F11h + regression |
| t15 R12/R13 overscan-bit carry into MA[13:12] | digest-03 §20.5 | regression |
| t16a-t16y type-0 last-line/adjustment arbitration — implemented: same-edge C0=0 R4 comparison; C0=1 R4/R9 equality breaks with R5=0, including exact R0=1 rollover consumption; R5 accepted through C0==2 and rejected after it with the accepted current-line target retained; R9 updates at C0==2..R0-1; R4 updates switch C9/R9 to C9/R5 through exact-R0; exact-R0 R9 increments C4 and C9; active adjustment reuses C9 against R5 even when R9 differs, including R5=0 overflow and zero-entry extension; R0=0/1 default adjustment; R0=0 during active adjustment freezes C4/C9; completion and retained-state lifecycle | digest-01 §3.1/§4.2/§7.1/§8.1 | F12 deterministic counter milestone and F5/F9 boundaries |

The implemented t01/t02/t03/t06/t07/t09 and t16 groups are required passes. The t08 group
retains only the two named type-1 F8 adjustment-identification divergences as expected
failures. These vectors fix the v1.10 counter and adjustment-state expectations while
deliberately avoiding unsupported sub-character MA/DE/VSYNC claims. If later hardware
evidence introduces a true pin-level uncertainty, keep any expected failure narrow; never
wrap setup assertions in a whole-test expected failure.

### Planned additions from the 2026-08-22 review (docs only — no RTL/test changes yet)

Recorded per `findings-review.md` Part C; each becomes a deterministic vector derived from
the cited ACCC rule when implemented:

- **t12 companion vector** (B4): windowed R9 write inside `C0∈[2,R0−1]` leaves C4=38,C9=8
  (ACCC p.82 example 3), beside the exact-`C0==R0` 39/8 case.
- **F8 corner vectors** (B5): an R9 write at exactly C0==R0 entering adjustment must NOT
  cancel the type-1 VMA-from-R12/R13-while-C4==1 reload; an R4(>0) rewrite at C0==R0 must
  cancel it (ACCC §11.2.4 note, p.84). The R4 side is the untested corner in
  `docs/review-debt.md`.
- **F7 design note** (B6): the RFD disarm path must cover R1>R0 — with `C0==R1` unreachable,
  the bare `C9==R9` match alone disarms the VMA-source flag (ACCC p.87).

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
