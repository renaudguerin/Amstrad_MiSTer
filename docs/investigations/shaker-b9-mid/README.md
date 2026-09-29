# SHAKER B9 MID FRAME replay — 2026-09-29

This diagnostic executes the private SHAKER 2.7 page-B caller on the production
T80pa, GA40010 and type-1 CRTC. It investigates the outstanding MID FRAME SIZE
rows without changing production RTL. The original-machine reference glyph
remains disputed (`4E40` / `4F40`); a successful replay is not hardware acceptance.
The established C0=3F update-delay residual remains separate unless evidence
connects the mechanisms. See the [prior discriminator record](../hardware-runs/shaker-b9-page-b-discriminator-2026-09-22.md).

## Independent software checks

The private binary's instruction sequence supplies arithmetic expectations;
these are software checks, not inferred CRTC rules:

- The loop at `92CD` increments HL once per iteration and exits via the conditional
  jump at `92D4`. Four subsequent doublings produce `16 × count1`, saved into the
  immediate operand at `9316`. The addition at `931B` adds `0470`;
  the formatter call at `931F` writes four hexadecimal characters at `9053`.
- The intermediate delay argument at `92EC` is
  `5228 - (16 × count1 + 0464)`, modulo 65536.
- The loop at `92FE` increments HL once per iteration and exits via the conditional
  branch at `9305`. Formatting at `9312` uses `16 × count2 + 5270`, modulo 65536,
  and writes four hexadecimal characters at `902D`.
- Four authentic entry points (`9266`, `926B`, `9271`, `9277`) select R6 pairs
  `32/32`, `7F/32`, `32/7F`, `7F/7F`. The fixture checks the self-modified R6
  operands as well as both loop counts and completed formatter buffers.

No expected MID numeric constant is copied from simulation into an assertion.
The completion and arithmetic checks protect instruction/register/memory trace
correlation; they cannot establish the original machine's frame timing.

## Observation on unchanged production RTL

Base `9ddbf74b35f4f5214f979d5039eb2961b536d121`; GHDL production T80pa translation
and Verilator 5.052. The authentic caller at `8E5A` executes its setup and three
update-delay blocks before these four MID cases. The fixture explicitly supplies
SP=`4065`, derived from the real dispatcher's `9D81..9D93` push/return sequence;
reset-SP is not a valid entry contract because setup clears screen RAM and
would overwrite that stack. See the [fixture contract and commands](../../../sim/diagnostics/shaker_b9_mid/README.md).

| R6 pair (hex) | First / second loop HL | MID buffer | EVEN+ODD buffer | PF before R8 write |
|---|---|---|---|---|
| 32 / 32 | 049D / 049F | 4E40 | 9C60 | 1 |
| 7F / 32 | 049D / 049F | 4E40 | 9C60 | 0 |
| 32 / 7F | 049D / 049F | 4E40 | 9C60 | 0 |
| 7F / 7F | 049D / 049F | 4E40 | 9C60 | 0 |

Each first loop executes 1181 iterations; each second loop executes 1183.
The intervening delay is `03F4`. All four R8=3 writes are accepted at
**C0=`1A`, C9=0, C4=0**. The first case's opcode fetch is tick 325965370;
bus write decode is visible at tick 325965522, and the write is consumed at
325965523. Stage A consumes its pulse at 325965578 (C0 `1A`→`1B`), stage B
at 325965642 (`1B`→`1C`). Both pre-edge pulses have `line_new=0`. The other
three cases have the same relative phase. Tick scale is 64/µs and 4096/line;
pre-edge timestamps count already-completed ticks, post-edge timestamps add one.

**This MID replay does not encounter the C0=3F stage-A/line-end collision.**
It narrows the investigation without explaining or closing the disputed MID
reference. It also shows the same numeric result with both recorded entering
frame-parity values; this is not an exhaustive parity-history test.

For each case the first raw VSYNC rise after the write is 1,282,048 ticks
(20,032 µs, 313 lines) after the previous rise. The following raw rise interval
is 1,279,936 ticks (19,999 µs). The latter must not be substituted for the
calibrated EVEN+ODD value `9C60` (40,032): the displayed values come from the
executed software calculation, not a universal conversion of the last raw
interval. No CRTC rule is inferred from these observations.

## Remaining discriminators

1. Obtain an unambiguous original type-1 CPC photograph or result-buffer read
   for all four MID rows and the EVEN+ODD totals. Preserve the reference
   `4F40` residual until that direct evidence settles the `4E40`/`4F40` glyph.
2. If original hardware confirms `4F40` with the same binary, capture the R8
   bus phase and both count-loop results using this instruction map. With
   unchanged first-loop arithmetic, `4F40` would imply count `04AD` (1197),
   16 iterations above this replay; this is a software-derived discriminator,
   not a prediction of hardware timing. Also verify the second-loop count and
   entry/setup history rather than assuming a common cause with C0=3F.
3. Keep the separate odd/even-R9 boundary discriminator in the prior page-B
   record for the established C0=3F residual. The mid-line observation here
   does not adjudicate that boundary ordering.

There was no MiSTer/AmSpirit access, synthesis, hardware acceptance, production
RTL edit or fail-before CRTC repair vector. Menu/firmware initialization,
interrupts, ROM banking and complete PPI behavior remain outside this fixture;
its full page-B caller coverage does not remove those limits.

## Reproduction and acceptance evidence

The checked-in [replay trace](replay.trace) contains the final current-source run's
instruction, accepted-write, pre/post-stage, VSYNC, loop and buffer observations.
The payload and disassembly remain private and untracked. Build and full gate
logs remain under `.roster-scratch/b9-mid/` in the task checkout.

Final selected gate (no production RTL changed):

```sh
CPATH="$(xcrun --show-sdk-path)/usr/include/c++/v1" python3 sim/select_tests.py --run --slow
# select_tests: PASS 1 benches: diagnostic
python3 sim/select_tests.py --check
# select_tests: index OK (60 rows) on the task's original base
```

Cross-provider review used Opus 5.5 at medium effort, runs
`20260929T043635Z-28373-a98f` and `20260929T044120Z-31464-7413`.
The initial review confirmed the acceptance edge/stage instrumentation but could
not access the symlinked private corpus. The follow-up used local private copies
and verified the entry, dispatcher-derived stack, arithmetic and trace claims.
Its remaining required condition was rebuilding and rerunning the final trace
changes: the selected gate above satisfies it, with `FORMAT_FETCH`,
`last_M1` progress fields and all four unchanged numeric results in the saved
trace. No code was edited after that gate. The optional instruction-address
wording correction was applied to this document. Saved review outputs and
completion metadata are `.roster-scratch/b9-mid/reviews/opus-{initial,followup}*`.

The earlier fixture timeout was corrected before acceptance: reset SP was
incompatible with SHAKER's screen clear, so the bootstrap now supplies the
source-derived dispatcher SP. This was a fixture repair, not a CRTC finding.
The first failure's short log was reconstructed from tool output and is explicitly
labelled as such; only the current-source successful trace is acceptance evidence.
