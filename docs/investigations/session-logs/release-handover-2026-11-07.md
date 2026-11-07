# Release preparation and open-work handover · 7 November 2026

The release scope is the accumulated classic CRTC/GA and Plus accuracy fixes,
including the hardware-backed CRTC3 compatible-interrupt correction after its
final review and gates. V1 monitor response and optional RTL simplifications
stay outside this release. Preserve their worktrees: a release does not finish
or supersede those tasks. Do not reset them onto the release source.

The [current status](../../current-status.md) owns the latest release artifact
and gate result. The [SHAKER ledger](../../shaker-results.csv) owns page grades.
The existing 1,384-image archive is historical `fdf348c` evidence, not a complete
fresh capture of this release. Refreshing a coherent current-build archive for
Longshot remains a separate milestone task.

## Worktrees to retain

Paths below were checked with `git worktree list`; use Git to rediscover them
if the host layout changes. All paths share the same repository object store.

| Track | Worktree | Branch and checkpoint | Position |
|---|---|---|---|
| V1 standalone design | `/Users/renaudg/.codex/worktrees/75c8/Amstrad_MiSTer` | `codex/general/v1-crt-response-20261106`, `b291826` | Clean; standalone READY, frozen base `1e95fa0`; no production timing/native acceptance |
| V1 production integration | `/Users/renaudg/.codex/worktrees/c1e6/Amstrad_MiSTer` | `codex/v1-production-integration-20261106`, functional checkpoint `e48614c` | Preserved; enabled fit fails timing, excluded from release |
| RTL audit | `/Users/renaudg/.codex/worktrees/5839/Amstrad_MiSTer` | `codex/accuracy/b26-terminal-sampling-20261106`, `425be96` | Clean; scoped audit/consolidation READY on frozen `1e95fa0`, not integrated |
| CRTC3 candidate / release preparation | `/Users/renaudg/.codex/worktrees/c1e6/Amstrad_MiSTer/local/overnight-20261103/resumed-20261104/c3-br-private-hardware-20261107/worktree` | Candidate lineage `codex/c3-br-private-hardware-20261107`, physical build `ae8bc6c`; release branch/gates in current status | Contains the non-V1 release source and test-contract work |
| CRTC1 E→S observer | `/Users/renaudg/.codex/worktrees/c1e6/Amstrad_MiSTer/local/overnight-20261103/resumed-20261104/c1-es-passive-observer-20261107/worktree` | `codex/c1-es-passive-observer-20261107`, base `ef0c109` | Dirty, provisional passive instrumentation; source backup retained; no core fix or hardware run |
| Older status-2 investigation | `/Users/renaudg/.codex/worktrees/c1e6/Amstrad_MiSTer/local/worktrees/plus-status2-adjustment-20261106` | `codex/plus-status2-adjustment-20261106`, `4e45417` | Preserved investigation; accepted status fixes are already in the candidate lineage |

The main checkout `/Users/renaudg/code/Amstrad_MiSTer` remains on `master`
(`6103619` when inventoried). Release preparation must select the validated
non-V1 candidate explicitly; the V1 integration checkout is not that source.
One integrator owns shared docs and source adoption at a time.

## V1: next work is timing, then production acceptance

The standalone model has 18 frozen streams / 5,432,672 comparison clocks,
47 selected passes, unchanged CRTC soak and independent Opus review. Production
integration fixes the RGB/metadata alignment and retains a full-board consumer
test. Its 260-master-clock relative lag is native-sampled; physical tuple
transport has 261 registers. FIELD is checked at the output consumer and
FRAME_FIELD separately at the motherboard export. Final ASCAL coherence and
classic nonzero response remain open.

The shared-arithmetic receiver at `e48614c` matches public outputs, canonical
state and valid history for 199 cases / 23,425,376 clocks. Invalid history
payload need not match. It passes 48 selected benches, unchanged soak
`0x974e5c048eb7f877`, and fresh Opus functional review. It changes neither the
qualitative preset nor presentation latency.

Full Quartus run **42323729673** nevertheless fails setup at **−28.450 ns**,
TNS **−33,453.313 ns**, with minimum hold **+0.248 ns**. The worst reported path
is HPS `status[34]` to receiver `absolute_phase[30]`, data delay **43.423 ns**.
Resources: 26,765 ALMs (64%), 32,653 registers, 104 RAM blocks and 35 DSPs.
Selected CI passes; production-T80 is cancelled, not a pass. No failed-timing
V1 bitstream has been installed.

Resume from the actual TimeQuest path, without a weaker clock constraint.
The next multi-edge proposal must preserve strict-prior publication and epoch
cancellation, and prove the tight finalization/HS spacing explicitly. Enabled
timing, RAM mapping, full-top/ASCAL and exact-build native acceptance are still
required. Do not copy the older motherboard wholesale over the release fixes.
See the [integration report](../video-boundary/v1-production-integration-2026-11-06.md)
and [receiver proof](../video-boundary/v1-rtl-candidate-2026-11-06.md).

## RTL simplifications: preserve model intent and frozen constraints

The separate `425be96` audit classifies 32 policies and proposes one C0
pending-step consolidation. Its frozen-base evidence includes 65,536 four-state
cases, 253 CRTC passes, unchanged soak, Opus review and exact-SHA CI. Before
adoption, reconcile it against the newer C0 fixes and rerun the relevant proof
and selection. Do not overwrite the newer model-confidence inventory.

Two additional C1 algebraic reductions have private full-wrapper evidence:
633,696 ticks / 1,901,088 observations and negative controls. They are not
production changes or independently accepted final patches. The proposed
one-bit reduction remains blocked on producer-bus and lifecycle constraints;
unrestricted synthetic double writes distinguish the states. Do not remove
that state merely because ordinary instruction spacing looks sufficient.

Readable event names and timing intent matter more than operator count. Keep
sourced quirks distinct from inferred guards, retain `MODEL LIMIT (B26)` labels,
and seek original-hardware discriminators when complete models disagree.
See [B26](../../backlog.md#b26-consolidate-inferred-rtl-exceptions-before-upstream-submission)
and the [current inventory](../rtl-model-confidence-2026-11-06.md).

## Six remaining confirmed SHAKER pages

The maintained tally is **377 passing / 6 broken / 1,004 unknown / 3 stale**,
1,390 unique pages. It includes qualified historical comparisons and two new
C3 BR passes on `ae8bc6c`; it is not a fresh baseline or a compliance percentage.
These residuals concern digital results; V1 models monitor response.

| CRTC | Page | Current defect | Next evidence or task |
|---|---|---|---|
| 0 | E6 `0409` | Cases 27/28 wrong; 26/28 assertions across the E6 pages pass | Resolve R4 rematch; use the original forced-entry stimulus, not a fitted exception |
| 0 | B6 `019D` | Two late-R7 rows give `1F` versus `1E` | Hardware/author discrimination of the late-write ordering; current CPCSE matches these physical fields |
| 1 | B6 `019D` | One late-R7 row differs | Apply the paired R7=32/33 discriminator to CRTC1 separately; no shared C0 rule is established |
| 1 | A4 `0037` | First OUTI scalar bit differs | Continue qualified original-caller bus/timing analysis; the old broad PPI classification does not excuse this bit |
| 1 | AI `008D` | Short-line VS samples differ from physical `1111` | Resolve width/counting rule; AmSpirit matches, CPCSE fails; raw-PPI samples cannot be fixed by the sync filter |
| 1 | S `02D3` | Current E→S history gives `0640` versus `4E40` | Observe E's final accepted R5 writes and outgoing adjustment state; no clear-C5 rule is justified |

Each page's current evidence and source links are in [SHAKER status](../../shaker-status.md).
The demo defects are closed by the user's manual DSC4/Amazing/BTF acceptance
on CRTC0/1/3; final-candidate preservation remains a release validation task.

### C1 S diagnostic checkpoint

The isolated observer implements a default-off, seven-slot `EST1`/version-2
trace of actual CLOCK-level R5 acceptance, the preceding/coincident and next
effective line end, next-edge post-NBA state, cleanup and S entry. It uses
original executed-byte/operand guards; no machine behaviour is intentionally
changed. This is not a fix for S.

After the worker interruption, all nine modified/new source files were copied
to `source-checkpoint/`; `UNCOMMITTED.patch`, `SOURCE-CHECKPOINT.json`, the
private decoder and `RECOVERY-FOCUSED.log` are preserved in the session packet.
The recovered current focused target passes:
`make -C <observer-worktree>/sim ssm-es-trace-test`.
The worker reported before-collector RED, held-episode/strict-next mutant REDs,
old S/DT passes and macro identity, but a final complete receipt did not survive.
Rebind missing evidence to the current source before adoption; do not treat
the report alone as its final selected gate or cross-provider review.

Next: complete default-off/old-profile identity and lint receipts, independent
review and selected gate, then an enabled diagnostic fit and original E→S
capture. A good-phase `4E40` result is a control, not proof that instrumentation
fixed the core. Require the bad active-adjustment trajectory before attributing
its cause. The immutable plan and source anchors remain in the session packet.

## Evidence locations and resume discipline

Private session root:
`local/overnight-20261103/resumed-20261104/` in the V1 integration checkout.

- V1: `v1-timing-shared-tail-20261107/`, including `fit-42323729673/` reports,
  source/miter/negative-control receipts and completed functional Opus review.
- C1 reductions: `b26-c1-terminal-astra-20261106/`,
  `b26-c1-terminal-equivalence-20261106/`, and
  `b26-c1-production-bus-contract-20261106/`.
- C1 observer: `c1-es-passive-probe-plan-20261107/` and
  `c1-es-passive-observer-20261107/`.
- C3: `c3-fast-contract-repair-20261107/` and
  `c3-br-private-hardware-20261107/`; physical/native comparison is
  `local/real-plus-results-20261107/NATIVE-AE8-QUALIFICATION.{md,json}`.
- AmSpirit: the focused French VS32 report, own CPR/ASM and screenshots are in
  `local/amspirit-vs-threshold-bugreport-20261107.zip`. Nothing has been sent.

Keep firmware, original SHAKER binaries and user-owned ACCC PDFs untracked.
Inspect actual Git state and `ACTIVE-JOBS.json` on resume; a saved worker PID
or a stale running receipt does not prove a worker survived. Refresh quota
before costly work, use Astra sparingly, and preserve partial diffs before
replacing interrupted workers. No public release has been authorized merely
by preparing this draft.
