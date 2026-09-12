# CSL/SSM design review, 2026-09-12

## Verdict and disposition

Fable's second review returned **CHANGES REQUIRED, converged; no architecture
blocker**. The parent accepted that convergence for the bounded phase 0/1 repairs and a
default-off phase 2 prototype. Opus produced the draft; Gemini continued the repairs
and the parent integrated the remaining fixes. Design convergence is distinct from
implementation and hardware acceptance; current source verification is recorded below.

The first Fable 5.1 high review returned **CHANGES REQUIRED** on Codex's uncommitted amendments
to [the implementation plan](csl-ssm-implementation-plan.md), based on source
`8731452`. The parent accepts the criticism that three continuously mirrored
surfaces plus a cloning engine and host-release mailbox prescribe too much
machinery before the device measurements exist. That architecture is withdrawn
as the selected implementation.

The revised direction is to prototype one native write stream into bounded,
rotating sample windows, with host reconstruction and explicit retention/loss.
It is approved for an experimental implementation, not as a complete exact-capture
solution. Fable's second pass accepts the corrections to sync-addressed windows,
4-bit pass stamps, unsupported minimum marker spacing and generation-only validation.

The next bounded device run needs the DDR allocation check and small source fixes
for startup, read failures and applicable Avalon reset/disable behavior. Broader
capture conformance work is separated from that diagnostic run. Repository-required
implementation review and synthesis gates remain in force.

## Findings retained or narrowed

- **Capture timing:** freezing at the first marker cannot preserve later samples.
  For a fixed progressive raster visited once per pass, freezing at pass end can
  preserve multiple earlier cuts from current/previous buffers. The old wording was
  wrong; the entire class of pass-buffer optimizations is not inherently invalid.
- **Observation boundary:** retain converted RGB24 at native cadence, before gamma
  and scaling. Keep 32-bit sample packing and the 64 MB/s nominal single-stream
  estimate. The old event stamp observes the same selected sync stream one colour
  register earlier; it has a narrower coordinate and a stage offset, not an unrelated
  timebase. Its 8-bit horizontal position still cannot locate a full native line.
- **Startup and read errors:** stale DDR can replay old events after a second load,
  and the broad read-error catch can turn an empty/failed read into “no marker.”
  Initialize a zero-count header and report actual transport/data errors. A controlled
  startup handshake must not silently assume an old zero header is a fresh load.
- **Host servicing:** drain at command boundaries and completion, record latency and
  use monotonic deadlines. Do not require an ingestion thread before measuring a
  bounded SHAKER walk. Naming and pair-warning fixes remain valid, but scripts that
  do not exercise those features need not wait for full conformance work.
- **Counter/drop handling:** short-interval tick deltas need modular arithmetic.
  Exhaustive 24-bit VSYNC-wrap coverage is not a first-run gate. Final-drop publication
  and saturation are real bookkeeping limits; the bounded gate must report losses
  honestly without turning them into a general queue framework.
- **Malformed output:** moved but periodic VSYNC can be represented relative to the
  observed sync. Missing sync or insufficient retained history needs an explicit
  incomplete/unsupported result. Host normalization must not erase the displacement
  under investigation.

## Proposals not accepted as established facts

### A short pass stamp does not preserve overwritten history

Fable proposed sync-relative windows with a 4-bit pass stamp per sample and host
selection of the newest stamp before the cut. Consider one address that contains
green in the preceding window, is written red before a marker, then blue after it
in the current window because sync resets the address traversal. Its red value is
gone. A stamp on the remaining blue value cannot recover it, whether the stamp
changes or stays the same. A cut within one pass also needs finer ordering than
a pass number. Sixteen stamps wrap rapidly and cannot be compared over a proposed
5–10 second retention interval without additional generation context.

The prototype should therefore append samples in time order rather than overwrite
sync-relative addresses within a window. Sync and field placement belong in the
host decoder. This removes that loss mechanism; it does not by itself prove that
the chosen finite prehistory contains every pixel needed for a persistent image.

### Paired markers need not be a pass apart by definition

The author says two captures preserve two flashing states. Neither that statement
nor the inventory establishes a minimum gap. Different states can occur within
one traversal, and SHAKER can alter its sync. Retain same-window and cross-window
counterexamples, then measure the actual gaps. Do not size the recorder from a
claimed construction rule that has not been verified in the program.

### Generation checking needs a publication state and a retention budget

Generation-before-reuse is useful, but descriptor/data/descriptor equality alone
can accept partial data if both descriptor reads occur after reuse starts and before
writing finishes. Only sealed windows may be read as valid: invalidate/change the
generation before reuse, commit readiness after all payload writes, then validate
both readiness and generation around the host copy.

A finite hold time can replace a host-release mailbox for an initial diagnostic
recorder, provided expiration/exhaustion is detected. Four or six windows and a
5–10 second hold are not established sizes. Without host release, a copied window
still remains pinned until expiry. Capacity depends on the unique windows pinned
by every marker during that interval, the required prehistory and continuing writes.

### A zero header and a read-margin check have bounded guarantees

Publishing `written=0` on enable is the useful first fix. Requiring the host to see
zero can miss initialization if a program emits immediately; an old empty header
can also look fresh before an asynchronous load finishes. The bounded BASIC/CSL
startup can deliberately observe the zero state before input and retain its boot
wait. General reload/concurrency guarantees remain a separate contract.

A torn ring read does not always require 64 new markers during that read: a reader
already near overrun can have its oldest needed slot reused by one new event. For
the sparse bounded walk, a conservative double-header/read-margin check and loss
reporting can be enough; account for a slot write preceding its header commit.

### Keep CSL wait semantics explicit

Fable suggested accepting a sync marker associated with the preceding command rather
than insisting on a fresh event at wait entry, but could not read the CSL extract.
The source says the wait lasts until the sequence is executed. Do not silently rename
a buffered-event approximation as exact CSL behavior. This does not block the bundled
scripts, which use no `wait_ssm0000`; settle the contract before claiming that feature.

## Implementation verification

**Source review CLEAR; local acceptance gates passed.** Code is uncommitted on
`general/b4-csl-ssm` over `8731452`; `SSM_SAMPLE_RECORDER` remains undefined by default.
The prototype uses the shared production recorder subsystem, exclusive HH-fetch cuts,
protected rolling prehistory, sealed generations and commit-identity read brackets.
Configuration changes reject transition-edge and subsequent input until a fresh epoch.
The live CLI exports raw samples, JSON metadata and a labelled cropped PPM; unsupported
cadence, missing history or loss cannot become a complete capture.

Parent-executed acceptance gates:

- Host suite: 183/183. Added late-zero, contradictory-cadence and ABI predecessor-bound
  cases failed on the prior source before their guards were repaired.
- Recorder: 21/21. The same-edge configuration-change test first failed with sample
  count 9 instead of 8, then passed with ingestion quiesced while old work drains.
- Shared composition: 5/5, including a selected stalled beat surviving a one-clock
  disable and both headers restarting at zero, plus the exact `ce/4` pressured HH cut.
  The latter fails against the saved broken recorder with generation 5 instead of 2;
  the other four composition cases still pass that control.
- `make -C sim lint`: exit 0, with non-fatal unused-signal/framework warnings.
- Soak: `0xb1cb70da95c2e44f`, unchanged across 2,845,088 character samples.
- Full `make -C sim`: exit 0 against the final implementation. Its composition target
  ran four cases before the fifth, pressure-regression case was added; the final
  composition target was then rebuilt and passed all five separately. No implementation
  changed during the full-suite run.

Opus run `20260912T152527Z-59275-6227` exited normally but could not execute its gates
through its permission layer. Its draft and the first Sol findings were preserved.
Claude quota was depleted, so no Claude retry was made. Gemini implementation runs
`20260912T162043Z-81226-bf30` and `20260912T170238Z-6347-94ab` exited normally; their
self-reported clearance was superseded by independently reproduced residual failures.
Neither a completed provider call nor its claimed gate result was used as acceptance.
Sol's final frozen-source review returned **CLEAR**, independently reproducing the
pressure, configuration-transition and host-coherence probes. Gemini review run
`20260912T173536Z-26982-c83d` independently reviewed the parent corrections and final
regressions, returning **CLEAR** with a normal exit and no interruption. All 20 source
hashes in `final-source-hashes.sha256` matched the reviewed files after both reviews.

Local evidence is retained under ignored `docs/references/csl-ssm-opus-implementation-2026-09-12/`,
`csl-ssm-gemini-finish-2026-09-12/` and `csl-ssm-gemini-final-repairs-2026-09-12/`
under that same references directory, including source hashes, failures before fixes,
mutation controls, gates and provider lifecycle records.

No synthesis, CI dispatch, commit/push, device run or physical-memory write was done.
DDR allocation, atomic/ordered HPS visibility, throughput, timing closure, calibrated
geometry and real SHAKER/photo acceptance remain open. Earlier broad CSL folding,
CFG restoration, keymap and production T80pa review/evidence debt is not discharged by
this SSM-focused implementation review.

## Evidence and limits

### Second review and implementation disposition

Run `20260912T151717Z-56014-1f34`, Fable 5.1 high, finished normally with return
code 0, clean cleanup, no hard timeout and no interruption. The reaper confirmed
no live bridge. The submitted plan, diff, in-checkout SSM/CSL extracts, complete
review and lifecycle metadata are retained under ignored
`docs/references/csl-ssm-fable-review2-2026-09-12/`.

The parent accepts append-only windows, finite retention with forced-oldest expiry,
separate loss reporting, a sealed publication state, explicit experimental defaults,
and the three immediate phase-1 fixes. One-shot consumption is selected for sync wait
because the CSL wording permits it and the asynchronous host can arrive late; it is
documented as an interpretation rather than author-confirmed semantics.

Several proposed details need correction and are not copied literally:

- A registered hit two clocks after fetch completion is not the exact HH cut even if
  the delay is less than one native dot. Carry the fetch-boundary sample count through
  recognition, excluding the coincident sample; test the window-boundary case too.
- Two previous-window indices plus two 32-bit generations exceed one 64-bit word.
  Use a separately versioned, sufficiently sized capture ABI and keep the ordinary
  format-1 ring compatible. Explicit aligned fields are preferable to dense packing.
- FIFO loss counts do not locate gaps. Preserve logical position and invalidate the
  affected window rather than silently compressing time.
- Port acceptance orders requests in the writer but does not by itself prove HPS
  visibility. Retain the device ordering gate. A short enable epoch also does not
  distinguish reconfigured cores; controlled startup remains a bounded guarantee.
- Do not add host writes to `/dev/mem` as a startup shortcut. That would require a
  proven inactive writer/allocation and an additional device operation outside this
  local implementation. Initialize descriptors and use the stated controlled handshake.
- Eight 2-MiB windows and a 4.19-second hold are experimental defaults. Forced expiry
  makes shortage detectable, not impossible; neither the simple capture-count formula
  nor 65.536 ms of prehistory establishes complete SHAKER coverage.

With those corrections, the parent authorizes the requested Opus implementation and
will obtain a fresh Sol review of its resulting diff. Device allocation, synthesis,
throughput, reload behavior and photographic acceptance remain separate gates.

### First review provenance

Invocation: guarded `ask-claude`, model `claude-fable-5-1`, effort `high`, run
`20260912T145731Z-50722-9d76`. It finished normally with return code 0, clean cleanup,
no hard timeout and no interruption. The reaper confirmed no live bridge remained.

The submitted plan, input diff, complete review and lifecycle metadata are retained
under ignored `docs/references/csl-ssm-fable-review-2026-09-12/`. The reviewer reported
permission limits reading the parent's temporary CSL extract, the workbook and the
complete HEAD plan. The parent had inspected the standards and workbook in the first
review; those checks are not independent Fable verification.

This is design feedback and parent disposition, not a full code review, a new
simulation pass, synthesis acceptance or a device result. The implementation
[review-debt rows](review-debt.md) remain open.
