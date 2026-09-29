# SHAKER B9 MID FRAME execution diagnostic

This optional private-corpus diagnostic executes the authentic page-B caller at
`8E5A` on production T80pa (GHDL translation), GA40010 clock/WAIT logic and CRTC
type 1. It includes the caller's setup and three update-delay blocks before the
four MID FRAME entry points `9266`, `926B`, `9271`, `9277`. It stops on the fourth
`932B` return fetch, before menu re-entry. No production RTL is modified.

```sh
make -C sim/diagnostics/shaker_b9_mid diagnostic PAYLOAD=/absolute/path/SHAKE27B.BIN
```

`PAYLOAD` defaults to the local September 23 archive. The input is the
header-stripped 26,683-byte payload, SHA-256
`c34e2fcc273ab427baf9aeb84bcfa1fe6144565c0be6dee2241a8d99f4f2dc88`.
The script rejects a missing or mismatched input before building. No binary or
disassembly belongs in this directory. GHDL, Verilator, make, a C++17 compiler
and `shasum` are required. On the current Mac, the system compiler needs its SDK
C++ headers supplied explicitly:

```sh
CPATH="$(xcrun --show-sdk-path)/usr/include/c++/v1" make -C sim/diagnostics/shaker_b9_mid diagnostic
```

Build products and logs stay in `.roster-scratch/b9-mid/`. The run is bounded by
400 million master clock ticks after release; missing cases, count completions,
R8 acceptance/stage events, R6 operand pairs or software arithmetic fail the run.
This is a diagnostic, not a hardware expected-value vector.

## Fixture contract

The 64 KiB RAM starts zeroed. The complete private payload is loaded at `3900`.
Only reset bootstrap `LD SP,4065; JP 8E5A` and detected type byte `(BF03)=1`
are injected. The authentic dispatcher at `9D81..9D93` sets SP=406D, saves
four register pairs, pushes the call target, and enters the caller by RET with
SP=4065. This avoids placing the stack in the screen clear at C000..FFFF.
Saved caller registers are not restored because the diagnostic stops before
menu re-entry. T80 reset establishes the remaining CPU state; no snapshot CPU
registers are restored. Initial CRTC register values reuse the earlier scratch replay's fixture defaults,
not a newly captured snapshot: decimal R0=63, R1=40, R2=46, R4=38, R5=0,
R6=25, R7=0, R8=0, R9=7; hexadecimal R3=8E and start address=1234.
The authentic setup reprograms the registers before measuring.
The fixture waits through row 2 to row 0 before releasing CPU reset. Authentic
setup, self-modifying operands, synchronization, delays and rendering helpers
then execute. CPU interrupts remain inactive; PPI F5xx returns raw CRTC VSYNC
in bit 0 with upper bits high, and PPI writes are ignored. RAM is flat with no
ROM banking. GA WAIT matches the classic, normal-speed motherboard path with
Plus and no-wait options disabled. These limits prevent claiming equivalence
to a complete menu-launched hardware run or arbitrary prior parity history.

## Observation and arithmetic contract

All times are master clock ticks: **64 ticks/µs, 4096 ticks/64-µs line**.
`*_PRE` is the settled combinational state before the next rising edge, labelled
with the number of already completed ticks. `*_POST` is after that edge and has
timestamp PRE+1. R8 acceptance is sampled when the real register-write decode
is active with data 3 and the old R8 differs; stage pulses, `line_new`, CLKEN,
C0/C9/C4 and parity are sampled before the consuming edge. M1 fetch traces are
bus observations after an edge, not speculative CPU PC values.

The independent arithmetic expectations come from the payload instructions:

- First loop `92CD..92D4` increments HL once per iteration and exits through
  `JP NC`. At `92D7`, raw HL is captured before four doublings. The saved
  product at `9316` is later increased by `0470`, formatted into `9053..9056`.
  Thus MID = `(16 * first_count + 0470) mod 65536`.
- The intervening delay at `92EC` receives DE =
  `(5228 - (16 * first_count + 0464)) mod 65536`.
- Second loop `92FE..9305` uses `JR NC`. Raw HL at `9307` is doubled four times
  and increased by `5270`, then formatted into `902D..9030`.
  Thus total = `(16 * second_count + 5270) mod 65536`.

The two different branch forms are executed, not normalized by the fixture.
Counts must match observed loop iterations, formatter outputs must match the
arithmetic, and the four self-modified R6 pairs must be 32/32, 7F/32, 32/7F,
7F/7F. Numeric output values are observations only. The disputed reference
**4E40/4F40 remains unresolved**, regardless of this replay's output.

The first fixture attempt used the T80 reset SP=FFFF and timed out at tick
401278026 without reaching MID. The authentic setup clears C000..FFFF,
overwrites that stack, and loses its return path. The explicit dispatcher-derived
SP bootstrap fixes this fixture defect; it is unrelated to CRTC behavior.

PPI ports A/C, keyboard and PSG are absent; other I/O reads return FF.
A path depending on these, ROM routines or an interrupt-driven HALT is outside
this contract and may time out rather than produce a numeric discrepancy.
Progress reports the last observed M1 address; returning to the reset stub
fails immediately. Each count loop explicitly loads HL=0 before entry.
The raw count is checked against observed INC HL fetches. T80 REG is not a
universally coherent architectural snapshot at M1: formatter DE reads were
transitional and are omitted. Completed destination buffers are the evidence;
the delay-argument and count samples separately pass their arithmetic checks.
