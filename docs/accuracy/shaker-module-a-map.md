# SHAKER Module A: test map and coverage gaps

Working document, opened 2026-08-19 after two hardware sessions (`1a1233f` and `5ddddef`)
returned no Module A improvement over the stock core. Its purpose is to turn "no progress"
into a per-subtest statement, and to decide what to implement next on evidence rather than on
the order of the audit table.

## Confidence warning

The subtest inventory below came from a secondary research pass over public sources, not from
running SHAKER or from the ACCC text. The researcher itself reported that Module A is **not
publicly documented at subtest granularity**, then supplied a subtest table anyway. Those two
statements are in tension, so treat every subtest name and description here as a hypothesis
to be checked against the running program, not as fact.

The first hardware session that reads the module's own on-screen list replaces this section
with observed names. Do not cite this file as a source until that has happened.

## What is solid

- SHAKER's current release is v2.6, organised as modules A through E, launched from BASIC as
  `RUN"SHAKE26A"` and so on.
- **Results are read by visual comparison against reference photographs of real hardware**,
  published by Logon System on the Shakerland portal (`shaker.logonsystem.eu`), covering CRTC
  types 0 through 4. There is no numeric score and no published pass/fail table.
- The ACCC explains the behaviour each test probes but does not state the expected picture
  per subtest.

### Consequence for our test protocol

This invalidates how the first two hardware sessions were run. Comparing our core against the
stock core answers "did I change anything", not "is it correct": if both cores are wrong in
the same way, the pictures match and the session reports no progress, which is exactly what
was observed. From now on the reference photograph for the selected CRTC type is the oracle,
and the stock core is only a regression baseline.

## Claimed Module A coverage, mapped to our findings

Unverified names, grouped by what they probe. The right-hand column is the honest coverage
statement.

| Claimed subtests | Behaviour probed | Our coverage |
|---|---|---|
| A5, A6, A7 — R13 update in 4/2/1 µs screens | R12/R13 start-address latch timing at three character widths | **Gap.** Only `F11h`, filed as minor with no action and still marked for a PDF re-read of ACCC §20.3.2 p.242. Nothing implemented or tested. |
| A1 — VRAM update versus CRTC/GA fetch | CPU write versus fetch phase within the 1 MHz cycle | **Gap.** Sub-character phase; not representable at the current character-granular CRTC-to-Gate-Array interface. |
| A8, A9, AE, AR — Gate Array pixelisation, ink, mode change, mode-to-HSYNC delay | Gate Array timing relative to CRTC edges | **Out of CRTC scope by design** (`F11i`). These live in the netlist-derived `GA40010`, which is gate-accurate and deliberately untouched. Our contribution is only correct HSYNC edges. |
| A2 — skew on R0 rupture | R8 SKEW-DISPTMG interacting with a mid-line R0 change | Partial. Skew path is `F11g` and believed correct; the R0-rupture interaction is untested. |
| AO — R1 stories | R1 > R0 and mid-line R1 writes | **Deferred by decision.** This is `F6`, the type-0 spurious border byte, parked because the documented half-character byte cannot be expressed through the character-granular interface. |
| AP — R6 stories | R6 beyond R4, overscan counter overflow | Partial, via `F4` equality-only rollover and `F12`. |
| AU — R4 and R9 checkings | Equality comparators, row increment and rollover trickery | **Implemented**, and this is precisely the F4/F12 work just hardware-tested. |
| AI — VSYNC conditions | R3/R4/R7 sync entry, clearing, and lock-up | **Implemented**, via `F3` and `F11b`. |
| AT, AY — R2 and R3 updates during HSYNC | Latched comparator versus live comparison during an active pulse | Believed correct via `F11a`, but only the width semantics were reasoned about; the during-pulse write cases have no vectors. |
| A3 — vertical rupture | Mid-frame R4/R9 changes | Partial, via `F12`/`F4`. |
| A4 — R0 update timing | When a new R0 takes effect | Partial, via `F5`/`F12`. |
| ACOPY — CRTC 2 offset | Type-2-specific behaviour | **Out of scope.** The project targets types 0 and 1. |

## Why the flat hardware result is consistent with this map

The two hardware sessions changed AU-class and AI-class behaviour, which is a narrow slice of
Module A. The subtests that emulator authors historically found hardest — the R13 start-address
update timing of A5 through A7 — are untouched by everything implemented so far, and the
Gate-Array-timing and sub-character subtests are either out of scope or blocked on the
interface granularity decision. A flat result is therefore the expected outcome, not evidence
that the F4/F12 work is wrong.

It is equally possible that the few subtests actually attempted were ones we never touched.
Without recorded names this cannot be distinguished, which is the reason for the protocol
change above.

## Next actions

1. Run Module A and record the module's own subtest list verbatim, with results for both
   supported CRTC selections. Replace the confidence warning and the table above with observed
   names.
2. Pull the matching Shakerland reference photographs for the selected CRTC type and judge
   against those, keeping the stock core only as a regression baseline.
3. Re-read ACCC §20.3.2 p.242 and decide whether R12/R13 update timing becomes a real finding.
   If the A5–A7 grouping survives verification, it is the strongest candidate for the next
   classic implementation, ahead of the audit's nominal next item.
4. Whatever is chosen, add its deterministic vector to the Verilator gate before changing RTL.
