# Unattended continuation handoff — 2026-09-03

Main source is unchanged from `08c1596`. This receipt records isolated local work;
none of the branches below has been merged, pushed, synthesized, or hardware-validated.
B8 remains excluded. The user's instruction is to stop after one failed Claude retry;
that condition has now occurred (session limit, reported reset 18:00 Europe/London).

## Committed candidates

| Branch / worktree | Tip | Evidence and remaining boundary |
| --- | --- | --- |
| `codex/testing-policy-audit` — `/private/tmp/amstrad-test-policy-20260903` | `c12c264` | Prior `c04991e` consolidates duplicate model-selection coverage while retaining RAM checks and strict leaf lint; `8168ed0` records the audit/review. Latest commit only clarifies fixture comments and evidence. Full parent simulation and lint exit 0. The audit is sampled, not whole-repository certification. |
| `codex/fdc-preedge-triage` — `/private/tmp/amstrad-fdc-preedge-20260903` | `c1a8ff9` | Seven u765 leaf tests pass, including held-read stability and one-byte advancement across the first two payload bytes. Gemini 3.8 Flash high independent CLEAR; full parent simulation and lint exit 0. No controller RTL change. |
| `plus/b3-capture-recovery` — `/Users/renaudg/code/Amstrad_MiSTer-plus` | `bb77075` | Existing independently reviewed B3 capture and P10j comment candidate remains READY, with exact-tip simulation/lint passing. See the existing Plus handoff. |

The audit's U1 found no demonstrated copied-logic drift: the fixture's phase adaptation
already existed at its introduction. Its comments now describe a functional model, without
claiming absolute bus-phase equivalence. U3's alleged non-ROM-index routing difference was
disproved by the old outer loader gate; no behavior fix was warranted.

The six original stashed instantaneous pre-edge FDC tests remain unaccepted and unapplied.
They all fail, but that does not establish a controller preload defect: readiness does not
require data before the documented read-strobe interval. The accepted test holds the read
strobe and checks stable data plus exactly one advancement. Full-sector/result-phase,
CPU-boundary, and classic AMSDOS firmware acceptance remain separate requirements.

## CPU candidate: preserved, not accepted

`codex/fdc-cpu-boundary` at `/private/tmp/amstrad-fdc-cpu-20260903`, base `08c1596`,
contains uncommitted Muse/Opus simulation-only changes. Production T80 and u765 RTL are
unchanged. Parent results:

- Focused branch fixture: 29 PASS, 2 DJNZ XFAIL, exit 0; lint exit 0.
- Full `make -C sim`: exit 2 because the existing P10 XFAIL unexpectedly passes all
  512 EDSK payload bytes after polling starts working. Raw result slots are
  `40/80/0/0/0/41/2`; they are still phase-unverified.
- Next: verify all 7 results against command semantics and actual result phase, integrity
  over all 519 latches, and the ST1 overrun bit; replace the obsolete XFAIL with strict
  acceptance and rerun gates. Do not weaken assertions or infer hardware/AMSDOS closure.
- Independent review debt is recorded under the user's Opus implementation exception.

Recovery evidence is in that worktree's `docs/fdc-cpu-boundary-2026-09-03.md`,
`.coord-inputs/parent-opus-*.log`, and `.coord-inputs/recovery-20260903/`
(tracked patch plus copies of new files). The full-suite failure remains a gate.

`codex/b10-rom-wrapper-seam` at `/private/tmp/amstrad-b10-wrapper-20260903` has
**no code changes**. Its brief is preserved under `.coord-inputs/opus-b10-brief.txt`:
production-coupled invalid-chunk boot address/bank retention and loader priority, with only
a narrow behavior-preserving extraction if necessary. This remains future work.

## References and bridge recovery

The retrieved NEC manufacturer-page image is preserved locally at
`docs/references/NEC-uPD765A-processor-read-timing.png`, with provenance in the adjacent
`.provenance.md` file; both remain untracked. SHA-256:
`e4c1219fb6da5de29b6b46eaf4c35779d9472ac5ff258280d34e0031077e6513`.
It is one reproduced page, not a complete verified PDF. Private source PDFs remain untouched.

Agents Roster task **Fix OpenCode bridge lifecycle visibility**
(`01a06609-078c-7d02-8340-35e4fe5bd7cc`) owns repairs. Its backlog is
`/Users/renaudg/.codex/worktrees/2ce7/agents-roster/docs/2026-09-03-bridge-repair-backlog.md`.
It has recorded launch-PID visibility, safe ownership/reaping, external-directory refusal
with misleading success, scratch routing, silent rate limits, Gemini response timeouts,
and Claude headless command denials. The Gemini export denial is separately reproduced
under consent/preflight: prior OpenCode-specific consent did not authorize the changed
provider; the user then directly authorized Gemini in the repair task. That grant was
accepted. No permission bypass was used. Implementation/review safety gates still apply.

Amstrad Muse workers hit rate limits; Gemini writers timed out while one Gemini review
succeeded. Opus produced the CPU candidate but both continuation workers then hit the
session limit. The single retry `20260903T125537Z-65040-7a2a` also ended 1,
cleanup clean. No Amstrad provider worker from this continuation remains running.
Do not globally reap: the repair task and unrelated processes have separate ownership.

Integration still requires the stream workflow's explicit finish authorization. Hardware
residuals remain as recorded: AmazingDemo named symptom appears fixed and Burnin Rubber's
right sprite symptom fixed on the named live-blanking build; DSC4/SHAKER still fail with
uncaptured current shape. ACCC v1.11 is unchanged. Stash
`0fe18a4513a47e4f21e0f504f002673a853388c3` remains intact.
