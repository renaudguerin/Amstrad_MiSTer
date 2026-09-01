# ACCC Round 2 author-response consequence audit — 2026-09-01

This is the bounded RTL/vector audit requested by
`accc-author-response-round2-2026-08-31.md`. It evaluates the published v1.11
extracts, the dated author answers, current RTL, and directed vectors. It is
source/model evidence only; no result here is hardware confirmation.

## §16.4.1.2 — type-0 R0/VSYNC

The English v1.11 §16.4.1.2 p.168 says C0 must reach 2 on the line preceding
C4=R7 before natural VSYNC is considered. The author confirmed that the two
following English-only R0-write paragraphs are also normative and were intended
for French; their current published anchor remains English v1.11 p.169.

The existing engine froze only steady R0=0. With steady R0=1 it generated a
line boundary after C0=1 and fired natural VSYNC without any preceding-line
qualification. Failure-first vector `t02l` isolated that discrepancy: all 185
pre-existing classic vectors passed, while the new vector observed VSYNC high
after the preceding line visited only C0=0,1.

The engine now retains whether C0 actually reached 2 on the current and
preceding lines. The natural row-entry comparison and the interlace half-line
consumer use that history, and a failed qualification consumes the comparison
as blocked without producing a pulse. This deliberately does not reduce the
rule to `R0>=2`: a change from R0>2 to R0=0 or 1 at C0=0 of the target line
retains the qualifying preceding-line history, preserving the two confirmed
dynamic cases. `t02l` is now a required failure-first pass. Native review then
identified that this vector alone did not prove comparison consumption or the
two timing-sensitive dynamic interactions. `t02m` pins the qualified R0=0
freeze, `t02n` pins the qualified R0=1 two-character pulse, and `t02o` widens
an ineligible target line before attempting a later equal-R7 write, so removal
of blocked-comparison consumption would fail.

## §11.6 — type-1 RFD VMA source

No mismatch was predicted. The type-1 engine distinguishes the parity-gated
VMA-prime save path from the R1>R0 bare-C9 disarm, and disables the VMA source
on the documented C9=R9 trigger route. `t13b`, `t13c`, `t13d`, and `t13n` pin
the consequential parity, unreachable-R1, final-line, and source-disable
interactions. No new vector or RTL change was warranted.

## §14.1 — HSYNC terminal wording

No mismatch was predicted. Both engines use C3l=R3l as the HSYNC end
comparison. `t33a`-`t33c` additionally pin terminal collision, overflow,
controlled restart phase, and lifecycle behavior. The author answer corrects
the French word “begins”; it does not require a model change. Hardware phase
confirmation remains separate.

## §19.5.3 — type-1 frame-origin ParityC9

No mismatch was predicted. The type-1 frame-origin route assigns ParityC9 from
the newly toggled ParityFrame, and `t32a` begins from deliberately unequal
parities so it distinguishes that assignment from the stale toggle-both model.
This remains local model evidence, not hardware evidence.

## Verification

- Failure-first: 185 pre-existing classic vectors passed; `t02l` alone failed
  on the unmodified §16.4.1.2 behavior.
- Final `make -C sim test`: 189 classic vectors and every integrated suite pass.
- Final `make -C sim lint`: passes with pre-existing warnings only.
- Fixed-seed soak: `0x8a2c2290bcef06a7`, reproduced twice across 2,845,088
  CLKEN samples per run.
- Native Sol review initially found the missing dynamic/consumption evidence;
  after `t02m`-`t02o` were added, its focused re-review was clear.
- Cross-provider review remains infrastructure-blocked, not passed. Guarded
  Gemini runs `20260901T035507Z-12974-14676`,
  `20260901T035639Z-16648-3561`, and coordinator relay
  `20260901T035828Z-20484-32033` produced no review because the managed
  headless permission layer denied `unsandboxed` or `read_file`. All three
  cleaned up and left the checkout unchanged. No permission bypass was used;
  shared `review-debt.md` reconciliation belongs to the integration owner.
  The separate Agents Roster repair task was queued as
  `client-new-thread:46b86d50-d08b-47b4-b79c-65b4f9fb6450`.
