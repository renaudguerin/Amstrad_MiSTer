# Current status

Current tip, 7 November 2026. The [release/open-work handover](investigations/session-logs/release-handover-2026-11-07.md)
records exact worktree paths, proof limits and resume steps. Keep the separate
V1 and RTL-audit worktrees open. [Prior detail](archive/current-status-pre-release-2026-11-07.md)
is archived; replace superseded state here rather than appending milestones.

## Release prepared

- [Draft v2026.11.07](https://github.com/renaudguerin/Amstrad_MiSTer/releases/tag/untagged-ee9c07a27e573000d3bb) targets tested `354388d` on
  `codex/release-20261107` (core/test checkpoint `10e905c`). V1 and optional audit
  reductions are excluded. No publication or master integration is performed.
- Full [CI42327881289](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/42327881289)
  passes, including production-T80. Local 47-bench selection and independent
  test-contract review pass. Full Quartus setup/hold minima 0.671/0.241 ns over
  seven clocks, zero TNS. [Acceptance](investigations/hardware-runs/release-accuracy-milestone-2026-11-07.md).
- `Amstrad_20261107.rbf`, 3,124,560 bytes, SHA256
  `78e063ef130bff543b5b81d6b541725cfc7e00a80be86ddc2a8623a22664073b`; reports and checksums attached and uploaded bytes verified.
  MiSTer now runs that tested354 build, last left in BTF; CFG is restored.
- Six final CRTC3 native checks / 12 PNGs pass: BR48+18, MRER128, VS9counts all
  match physical Plus; DSC4/Amazing/BTF sampled scenes are preserved. User
  manual CRTC0/1/3 acceptance remains separate from these asynchronous captures.

The accumulated fixes include classic R0/vertical-adjustment/display sequencing,
GA/ASIC horizontal positioning and Plus status/overflow/compatible IRQ handling.
The [roadmap](implementation-roadmap.md), [backlog](backlog.md) and
[model-confidence inventory](investigations/rtl-model-confidence-2026-11-06.md)
retain broader scope, inferred limits and source authorities.

## SHAKER

Counts are per unique page, carrying qualified historical comparisons. The two
C3 BR pages freshly match the real Plus on `354388d`.
This is not a fresh baseline or a full-compliance percentage.

| CRTC | Passing | Broken | Unknown | Stale | Total |
|---:|---:|---:|---:|---:|---:|
| 0 | 7 | 2 | 477 | 1 | 487 |
| 1 | 364 | 4 | 111 | 1 | 480 |
| 3 | 6 | 0 | 416 | 1 | 423 |
| All | 377 | 6 | 1004 | 3 | 1390 |

| CRTC | Broken page | Current result / next task |
|---:|---|---|
| 0 | E6 `0409` | Cases27/28 wrong; resolve R4 rematch |
| 0 | B6 `019D` | Two late-R7 rows1F versus1E; hardware/author ordering discriminator |
| 1 | B6 `019D` | One late-R7 mismatch; assess CRTC1 separately |
| 1 | A4 `0037` | First OUTI scalar bit differs; continue original-caller bus/timing analysis |
| 1 | AI `008D` | Short-line VS samples differ from physical1111; width/counting rule unresolved |
| 1 | S `02D3` | E→S history0640 versus4E40; provisional passive observer, no core fix |

[Per-module tally and source links](shaker-status.md), [ledger](shaker-results.csv).
D3 content, D6/D7 phase/settling, DI's unreliable CRTC1 sample and partial AI/A4
comparisons remain explicitly unknown; missing CRT curvature is a V1 question.
The1,384-image fdf348c corpus and six capture gaps are historical. A coherent
current-build archive for Longshot still needs its milestone refresh.
[Archive](investigations/hardware-runs/shaker-milestone-archive-2026-11-06.md).

## Work kept outside the release

- **V1 standalone:**75c8, `codex/general/v1-crt-response-20261106`, b291826,
  READY on its frozen base. **Integration:**c1e6,
  `codex/v1-production-integration-20261106`, implementatione48614c.
  It passes199 cases/23,425,376 comparison clocks,48 selected benches and
  unchanged soak. Enabled full fit42323729673 fails setup at−28.450ns
  (status[34]→absolute_phase[30],43.423ns), minimumhold+0.248ns.
  Selected CI passes; production-T80 is cancelled. Timing/RAM/full-top/ASCAL/
  native acceptance remains. No failed-timing V1 bitstream is installed.
  [Integration report](investigations/video-boundary/v1-production-integration-2026-11-06.md).
- **RTL audit:**5839, `codex/accuracy/b26-terminal-sampling-20261106`,425be96,
  READY on frozen1e95fa0;32 classified policies and one C0 consolidation.
  Reconcile with current fixes before adoption. Two C1 algebraic reductions
  retain private bounded proof; state-bit removal still needs bus/lifecycle
  closure. Preserve timing intent/readability and B26 labels.
- **S observer:** isolated `codex/c1-es-passive-observer-20261107`, baseef0c109.
  Dirty source and untracked tests are backed up; recovered focused proof
  passes. Final identity/lint/review/selected-gate/build/native receipts remain.
  It is diagnostic instrumentation, not a fix. Details are in the handover.

Wider Plus DMA/save/reset/title and classic lifetime/phase gates remain in the
[roadmap](implementation-roadmap.md), [Plus divergences](plus/source-divergences.md)
and [review debt](review-debt.md). Do not infer general closure from a targeted pass.

## Tools and operation

AmSpirit1.17.1 and CPCSyntaxError are available peers; physical hardware wins.
CPCSyntaxError v0.14.2 is the sole installed package in the main checkout;
headless `--shot` and minimal CSL smoke tests pass. v0.12.0 captures remain
historical evidence. See [testing and source comparison](cpcsyntaxerror-testing.md).
The recent accuracy leads are queued in [B28](backlog.md#b28-triage-cpcsyntaxerror-v0130v0142-accuracy-changes-against-open-work);
[B29](backlog.md#b29-sweep-the-entire-cpcsyntaxerror-release-note-history)
holds the full historical release-note sweep.
The focused AmSpirit VS32 French report/own CPR/source ZIP is prepared privately and has not been sent. Read the [static-photo text cache](shaker-reference-text.md)
before reopening covered images. The hardware loop applies CFG settings with
requested-versus-observed limits; one operator owns MiSTer. Check
`local/overnight-20261103/resumed-20261104/ACTIVE-JOBS.json` and real Git/process
state on resume. Refresh quota; Astra is reserved for prepared hard questions.
