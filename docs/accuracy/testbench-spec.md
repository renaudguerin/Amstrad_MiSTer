# Verilator CRTC Testbench — Specification

Goal: a **macOS-native, no-Quartus** simulation harness for the classic CRTC core
(`rtl/CRTC.v` + its two per-type engine files) that encodes the
Compendium rules (see the digests in this directory) as executable assertions, so that any
future agent or human can verify CRTC changes in seconds instead of a 30-minute synthesis +
hardware test. This is the verification backbone for findings F3–F10 and F12 in
`audit-findings.md`.

Primary reference: *The Amstrad CPC CRTC Compendium* v1.11 (Longshot / Logon System).
Technical information sourced from the "Amstrad CPC CRTC Compendium" by Longshot
(CC BY-NC-ND).

## Why Verilator

- Runs natively on Apple Silicon (`brew install verilator`), compiles Verilog to C++.
- The CRTC core (`rtl/CRTC.v` + engines) is pure synchronous logic on a single clock — ideal DUT. No vendor
  primitives, no PLL, no sys/ dependencies.
- Note: the RTL carries `/* verilator lint_off WIDTH */` pragmas — the author
  anticipated this. Expect only minor lint noise.

## Architecture

```
sim/
  Makefile                    # aggregate build/test/lint/soak entry points
  sim_main.cpp                # CRTC harness, helpers, 192-vector registry, soak
  crt_filter_blank_test.*     # focused filter/blanking seam fixture
  plus/                       # Plus, motherboard, memory, DMA and CPR benches
rtl/u765/
  Makefile + u765_tb.cpp      # real controller/EDSK transaction fixture
```

No separate `crtc_script` interpreter, text-vector directory or golden-trace directory exists.
The implemented suite keeps each directed vector in `sim_main.cpp`, where the timing setup,
independently derived expectation and registration are reviewed together. A failing classic
vector writes a named VCD under `sim/obj_dir/`; passing traces are removed.

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

### Vector format

Vectors are named C++ functions registered in the table at the end of `sim_main.cpp`. They use
shared helpers for register writes, exact C0/line positioning, half-character phases, bounded
walks, output assertions and internal counter assertions. Keep expectations on the vector and
citation side of that boundary: do not read expected values from the DUT or add a parser that
merely restates newly implemented RTL. A new timing-sensitive behavior starts as a failing
required vector (or a narrowly registered XFAIL when the repository deliberately lands the
fixture first), and the behavior change removes that XFAIL in the same coherent slice.

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
| t08 type-1 adjustment: §28.1.1 identification fixture plus C5/C4/C9 sequencing; `t08j` pins the author-confirmed Q20 behavior where R5=0 keeps C5 and adjustment active but C4 still resets on C4=R4, including the row-0 R12/R13 reload; `t08p`/`t08q` pin VSYNC to actual `row_next=0`; A1 makes the adjustment-ending final-row+1 VSYNC comparison silent; A2 pins the §11.2.4 exact-C0==R0 caveat pair (R4>0 suppresses the C4=1 R12/R13 reload, R9 does not) | ACCC v1.11 §11.3.2 p.85 + 2026-08-31 author response Q20; digest-01 §11.2.4 + digest-02 §§16.1/16.4.2 + digest-03 §28.1.1 | F4/F8/A1/A2/Q20 |
| t09 R0=0 freeze (type 0): counters halt, no HSYNC unless R2=0, resume clean; C9=R9 entry consumes exactly one C4 increment — implemented | digest-01 §8.1 | F5/F12 boundary |
| t10a-t10e + t31a R1>R0: type 0 no-skew DE is high for C0=R0's first half and low for its second 0.5 µs; type 1 none; SKEW-DISPTMG 1/2 rounds the deferred event to a full delayed character and mode 3 suppresses output — implemented, hardware validation pending | digest-03 §17.6.2/§19.2.3/§19.2.4 | F6/F13 |
| t32a type-1 frame-origin ParityC9 realignment from a deliberately unequal pre-origin state — implemented, hardware validation pending | ACCC v1.11 French §19.5.3 p.209 | IA-2/BL-038 |
| t33a-t33c type-0 live R3 terminal collision: C3l overflow, controlled earliest restart phase, and lifecycle controls — implemented, hardware validation pending | ACCC v1.11 French §§15.3.2-15.3.3 pp.150-151 | IA-1/BL-025 |
| t34a type-0 R6=0 first-frame-line conflict: high/low half-character polarity, reachable live R1, and R6 0→nonzero cancellation control — implemented, hardware validation pending | ACCC v1.11 French §18.3.2 p.191 | IA-3/BL-036 |
| t35a type-0 R0=1 true-last-line widening: safe C0=0 control versus unsafe C0=1 continuation into C4=R4+1/C9=R9..31 additional management — implemented, hardware validation pending | ACCC v1.11 French §13.7.2 pp.126-127 | IA-6/BL-018-BL-020 |
| t02l-t02r type-0 natural VSYNC requires C0=2 qualification; steady R0=1 blocks C4=R7, while qualified exact-C0=0 writes to R0=0/1 retain the documented frozen/two-character outcomes; `t02o` treats blocked-comparison consumption as an inferred hardware discriminator, and `t02p`-`t02r` pin type-1 isolation plus snapshot/live-type lifecycle reconstruction even when the clear lands after C0=2 — implemented, hardware validation pending | ACCC v1.11 English §16.4.1.2 pp.168-169 and §§16.4.2/19.3.2.1; author confirmation applies to the short-line block and dynamic R0 outcomes, not `vsync_allow` consumption | Round-2 consequence audit and lifecycle review |
| integrated GA R2.JIT controls: production-phased `OUT (C),r8` makes R2 equal current C0; type-0/type-1 visible/raw starts move +4/+3 Mode-2 pixels while raw width shortens by 4/3 and the type-specific display-reactivation edge stays fixed; same-value intent control stays normal but lands after HSYNC rise — implemented, hardware validation pending | ACCC v1.11 §9.3.4.1 pp.53-54, §9.3.4.3 p.57, §14.6.1 p.141 | F20 |
| t11 type 1 adjustment: C9 keeps cycling (RA!), C4 increments, and R5=0 does not end adjustment; the ordinary C4=R4 reset remains live per Q20 | digest-01 §4 + 2026-08-31 author response Q20 | F8 |
| t12a/t12b documented R4=38/R9=7 worked example pair (type 0): exact-C0==R0 R9 write leaves C4=39,C9=8; windowed write (C0∈[2,R0−1]) leaves C4=38,C9=8 — both encoded from ACCC §11.2.2 p.82 ex.3 — implemented | digest-01 §3.1/§4.2 | F9/F12 |
| t13a-t13d RFD — implemented: away-from-R0 never-triggered control, R5 0→1 at C0==R0, same-cycle VMA reload/adjustment entry, frame-parity VMA' alternation, successful-save disarm, and R1>R0 bare-C9 disarm | digest-01 §4.5/§5 | F7/B6 |
| t14 VMA reload: type 0 only at C4=C9=C0=0; type 1 every line of C4=0 row | digest-03 §17.4/§20.3 | F11h + regression |
| t15 R12/R13 overscan-bit carry into MA[13:12] | digest-03 §20.5 | regression |
| t16a-t16y type-0 last-line/adjustment arbitration — implemented: same-edge C0=0 R4 comparison; C0=1 R4/R9 equality breaks with R5=0, including exact R0=1 rollover consumption; R5 accepted through C0==2 and rejected after it with the accepted current-line target retained; R9 updates at C0==2..R0-1; R4 updates switch C9/R9 to C9/R5 through exact-R0; exact-R0 R9 increments C4 and C9; active adjustment reuses C9 against R5 even when R9 differs, including R5=0 overflow and zero-entry extension; R0=0/1 default adjustment; R0=0 during active adjustment freezes C4/C9; completion and retained-state lifecycle | digest-01 §3.1/§4.2/§7.1/§8.1 | F12 deterministic counter milestone and F5/F9 boundaries |

All implemented groups are required passes. The classic suite currently has **192 required
passes**, zero expected failures, zero unexpected passes, and zero failures. It has had no
expected failures since
the F8 commit (`c9f4a4e`): the former type-1 adjustment-identification xfails
(`t08f`/`t08g`) became required passes. These vectors fix
the counter and adjustment-state expectations carried forward into v1.11 while
deliberately avoiding unsupported sub-character MA/DE/VSYNC claims. If later hardware
evidence introduces a true pin-level uncertainty, keep any expected failure narrow; never
wrap setup assertions in a whole-test expected failure.

### Follow-ups from the 2026-08-22 review

Recorded per `findings-review.md` Part C; each becomes a deterministic vector derived from
the cited ACCC rule when implemented:

- ~~t12 companion vector (B4)~~ Done: `t12a`/`t12b` encode the documented pair.
- ~~**F8 corner vectors** (B5)~~ Done: `t08n`/`t08o` pin both §11.2.4 p.84 directions;
  exact-edge R4>0 suppresses the type-1 C4=1 reload and exact-edge R9 retains it.
- ~~**F7 design note** (B6)~~ Done in `t13c`: with `C0==R1` unreachable because R1>R0,
  the bare `C9==R9` match alone disarms the VMA-source flag (ACCC p.87).
- ~~t20 companion vector~~ (review action item A3) Done 2026-08-23: `t20i` pins the
  live-entry R0=0 freeze (R0 written to 0 on a wrap edge), including the documented
  "first C0==0 reloads VMA" behaviour that the cold-reset `t20g` cannot reach.

## Non-goals

- No Gate Array co-simulation *in this suite* (CRTC rules are testable at the CRTC's pins).
  This is a scoping choice for the CRTC vectors, not a statement of infeasibility: a separate
  CRTC+GA co-simulation harness already exists in-repo (`rtl/GA40010/ga40010_test.v` +
  `Makefile`, renders frames to PNG; `ga40010.sv` has a Verilator build path). The GA is a
  schematic-derived SystemVerilog recreation, not a black-box netlist, and simulates fine.
  Use that harness when a rule's observable lives downstream of the CRTC (e.g. F6 seam
  width). GA interaction rules (C-SYNC state machine, R52) remain out of scope here.
- No Z80 instruction-level modeling in the classic pin suite; its timed bus helpers place
  writes at explicit character or half-character phases. The separate integrated GA fixture
  owns production-phased `OUT (C),r8`/OUTI distinctions.
- No cycle-exact pixel pipeline — DE/MA/RA at character granularity is the contract.

## Definition of done (for the implementing agent)

1. `make -C sim` runs the full suite non-interactively and exits zero (current classic state:
   192 required passes, no expected failures), with Plus and peripheral benches included by
   the aggregate target.
2. A failing assertion produces a VCD + human-readable diff.
3. `docs/accuracy/audit-findings.md` verification levels V3 reference a named vector or
   focused fixture that actually exercises the stated boundary.
4. CI-friendly: exits nonzero on failure; no interactive steps. (Optionally add to the GitHub
   Actions workflow from `docs/building.md` as a pre-synthesis gate.)
