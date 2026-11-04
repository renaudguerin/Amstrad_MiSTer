# Unattended continuation — 3 November 2026

## Scope and limits

Branch `codex/overnight-validation-20261103`, starting at `6103619` (same RTL
as manually tested `5fba168`). Task checkout:
`/Users/renaudg/.codex/worktrees/c1e6/Amstrad_MiSTer`.

Requested work: incorporate the manual demo retest, migrate AmSpirit tooling
to Lite 1.17.1, audit/install the author's `local/CSLSSM/` package, assess
existing SHAKER evidence and rerun needs, and cross-check open ACCC author
questions against locally available emulator source. Continue local accuracy
work where a deterministic failing test and a source-backed repair are possible.

**MiSTer has been released by Renaud's explicit all clear.** The temporary
hold was respected; four existing manual PNGs were copied before it and no
test was launched during it. Subsequent runs require a named RBF and one operator.
See the [manual acceptance update](../hardware-runs/manual-demo-retest-2026-11-03.md).
Renaud subsequently authorized branch pushes and GitHub-hosted builds. The
local Quartus VM is on the MacBook Air, while this session runs on the Mac
mini. Renaud later reported the local Quartus VM up and available for
subsequent builds. Recheck reachability when needed; keep the already
running hosted build to completion without duplication. No VM migration
or provisioning was requested. Merge to master and
release remain outside this run's request.

Wrap up cleanly at approximately **80% weekly Codex usage**, as authorized when
Renaud went to sleep. Initial live OAuth reading at 22:24 UTC: 21% weekly used, reset
5 November 19:34 UTC; short-window usage unavailable. Three banked resets
exist but redemption is not authorized. The stale dashboard reading is not
the active telemetry. Gemini pool at 22:25 UTC: about 19.4% weekly used;
Claude/GPT pool on Antigravity about 87.5% weekly used. Keep work units bounded.
Automatic continuation after app exit, host sleep or quota exhaustion is not
verified; no scheduler tool was exposed at task start.

## Work ownership

- Gemini migration: `scripts/amspirit/`, its helper documentation and new
  Lite 1.17 guide. Sole AmSpirit operator until the migration smoke check ends.
- Gemini package audit: new dated package report only, static comparison of
  supplied assets, prior corpus and private fixes. Installation belongs to parent.
- Gemini source audit: new dated author-question source report only.
- Sol high classic display: `rtl/CRTC.v`, `sim/sim_main.cpp` and the E/P report;
  focused fail-before/pass-after proof. R5-release/02D3 is outside this repair.
- Sol high ASIC diagnosis: new diagnosis report and ignored scratch only;
  no device operation, no RTL edits during initial diagnosis.
- Parent owns integration, current-status, author-question reconciliation,
  installation, independent review and final acceptance.

The Gemini runs retain output and lifecycle evidence under
`/tmp/agents-roster-runs/20261103T222602Z-8352-461f/` (migration),
`20261103T222603Z-8384-efbe/` (package), and
`20261103T222604Z-8416-1c64/` (source). Preserve useful logs in ignored local
evidence before temporary-run pruning.

## Local reference provisioning

The author package was copied from the main checkout into task-local
`local/CSLSSM/`; this preserves both supplied originals. Both ignored ACCC
PDFs were copied into `docs/specs/`. A fresh pdf-inspector invocation could
not install its dependency in the restricted global Python directory; reuse
the main checkout's saved inspector report/Markdown and render relevant pages
with `pdftoppm`. Never commit the PDFs, ROMs, disks or screenshots.

## Completion / continuation

Accepted on the task branch:

- `237c8e5`: manual retest and device-hold record. MiSTer later released.
- `bcfcd5e`: AmSpirit 1.17 migration, 21 protocol tests and live Plus
  title/joystick/downloaded-snapshot smoke; retain direct HTTP.
- `980703e`: emulator-source cross-check of author questions. Q21, Q22 and
  the RFD lifetime question stay open; software agreement is not a hardware
  oracle. Parent corrected two overclaims in the provisional source audit.
- `8dde1e7`: classic stuck-adjustment display/status origin repair.
  Failure-first vector, 239 CRTC passes, historical negative fixture,
  soak `0x743ca9b795e30eaf`, 43 selected benches, final fresh Opus clearance.

`bedb1c5` contains a provisional ASIC write qualifier. Its register-storage
windows and original controls passed, but independent review identified
missing counter seams. Three new Plus seams fail while classic controls
pass. This candidate is **diagnostic, not accepted**. No ASIC counter/pixel
phase is changed merely to make those seams green. Evidence and boundaries
are in the [ASIC report](../hardware-runs/asic-demo-diagnosis-2026-11-03.md).

The authorized hosted build is run
[41739354589](https://github.com/renaudguerin/Amstrad_MiSTer/actions/runs/41739354589),
full effort at exact `bedb1c5e20e1d720da4b75f5f735c987776e2835`. Selected
simulation/lint and production T80 pass; full synthesis/timing and required gate passed. Artifact identity is
`Amstrad_20261103_bedb1c5.rbf`, SHA-256
`02ed2ae66f6648d72c86cbaa9bbbe393d36b0d5656ab05e17cb32931b17321c7`.
The build operator owns the watcher and artifact download; the parent is
the sole MiSTer operator. Keep the run to completion rather than discarding
its build work after the review finding.

Package installation and existing evidence are described in the
[package audit](../hardware-runs/shaker-package-audit-2026-11-03.md) and
[device result](../hardware-runs/shaker-package-device-2026-11-03.md). A/B/C
old captures remain valid for their build/history; full D-1 now completes,
but its D3 bulk input outruns native capture. A paced 44-page D3 rerun
completes on the new disk; visual comparison is in progress. Updated E-1
retains warning/blank residuals.

Latest live Codex OAuth reading, 00:05 UTC November4: **26% weekly used**, reset
5 November 19:34 UTC, primary window unavailable. The 60% user limit
remains in force. Automatic restart after ending this chat is unverified.

The full C-1 walk on the combined artifact completed 80captures: E/P
readable, E numeric60/80residual remains, 02D3now4E40expected in this
run. 66PNGs are byte-identical to old C-1f. Repeat full history on the
classic-only build8dde1e7, hosted run 41742364942, still in progress.

Next: finish classic-only artifact checks and its repeat C-1 history; use the ASIC candidate only for diagnostic
DSC4/Amazing counterfactual captures; finish D3 comparison and preserve a
host-paced overlay. Resolve the ASIC acceptance gap before any verified
publication or integration claim.

## November4 continuation and strategy

Renaud requested an efficiency assessment. The recommended working order
is now SHAKER-led functional diagnosis, with demos as milestone checks.
Monitor-response appearance and filter acquisition have their own evidence
and acceptance; neither a photograph mismatch nor a successful native PNG
is automatically a CRTC verdict. CPCSyntaxError's headless CSL/API/source
make it useful for instrumentation, with AmSpirit and original hardware
retained for independent checks. Installing a built macOS version would
be useful but does not block other work; no binary was found in the local
source tree during this check. Its default CSL runner patches an A-U wait;
record `--no-errata` versus default instead of assuming identical scripts.

The live AmSpirit corruption Renaud observed came from private restart
controls, not the clean boot reference. The initializer replay changed
left-hand screen RAM and zeroed sampled colons; its trace is rejected as
a rendering oracle. A fresh original-disk classic boot now shows all
colons/names correctly and is left paused for inspection. Evidence is in
`classic/credit-replay/visible-control/fresh-credits-t23/`.

The ASIC bus qualifier has now failed an original-Plus SHAKER comparison:
BI/R3-JIT's six narrow-band cases become224pixel blocks, while the two
large-band controls stay unchanged. Old widths16/32/48 and the expected
large controls agree with original-Plus photographs and fresh AmSpirit.
The qualifier and its uncalibrated fixture have been removed from the
working source, with all code/patches retained under
`asic/rejected-bus-candidate/`; a code checkpoint follows the final gate.
The current mode-latch unit is independent, uses French52–53 and actual
GA→video tests, and keeps raw sync/blank/IRQ/clocks unchanged.

Hardware ownership transferred from parent to classic_build: corrected
D3 entry-wait trial timed out before 0314, proving that first SSM arrives
on page advance rather than stable initial entry. The next host overlay
uses explicit screenshots before keys with SSM autocapture off. The
classic-only C-1 repeat is running; parent must not touch MiSTer until
that operator returns ownership. AmSpirit API ownership has returned to
parent; leave the clean credits reference visible while working offline.


## Current checkpoint — 4 November, 01:00 UTC

The prior build/capture ownership notes above are historical. Both earlier hosted
builds completed; the classic-only 8dde1e7 repeat has 80 captures, readable E/P,
P 26 and S 4E40. E 0060/0080 remains open. Device ownership is back with the parent;
no emulator API work is active, and AmSpirit remains paused on clean credits.

The rejected ASIC bus qualifier is now reverted in 939ee6f, backed by actual
production-T80 BI counter traces and original-Plus photographs. The independent
mode-only repair is 3113323f24bb47dd18d51489d6c53a25793f158c, pushed. Opus accepted
its RTL; the requested test/comment follow-ups pass44 selected benches, with no
code edits after that gate. The whole-character mode/blank alignment residual is
named separately. Hosted full build 41750348807 is the only synthesis in progress;
the build operator owns its watcher and artifacts, with no device ownership.

CPCSyntaxError v0.12.0 is installed/pinned and now in the toolbelt guide/skill.
Fresh headless BI3/C1/AE-AR3 batches yielded 95 captures. These are execution
counts, not pass counts. The hardware-matching E 0080 result has narrowed an
[experimental positive-R5 C5 restart](../hardware-runs/shaker-e-0080-positive-r5-reentry-2026-11-04.md);
Q20's zero continuity remains protected. A bounded private trace is next.

D3's marker is emitted on advance, so removing the entry key was not a valid
capture fix. The manual before-key overlay avoids that marker race but still
loses bulk inputs. A three-page 200 ms probe repeats keys (35→3C→3E instead of
35→3A→3B); the 100 ms probe is in progress. Canonical author scripts are unchanged.
No full 44-page rerun is accepted until header alignment is demonstrated.

Live Codex OAuth at 00:53 UTC: 29% weekly used, reset 5 November 19:34 UTC; primary
window unavailable. User's 60% pause threshold remains in force; no banked reset
has been redeemed. No master integration, release or PR is requested.


## Checkpoint — 4 November, 01:12 UTC

CPCSyntaxError toolbelt guide/skill and initial source discriminator are committed
asdd247d4. Fresh DT through 02FA adds13 complete captures; its02F8/02FA both show
the photographed second black band and match the earlier partial attempt exactly.
The broader older event-paced T script times out naturally at 0300; that is not
assigned as an emulator or author defect. Raw-GA-versus-acquisition tracing remains
necessary, as explained in the DT boundary report.

The E proof now directly observes C5 and obtains0080 via a private C5-only
positive-R5 re-entry restart, keeping all other seven values and 515zero controls.
No production change was made. An authorized bounded Opus faithfulness review is
running at`/tmp/agents-roster-runs/20261104T011108Z-64964-7921/` (sessionfc46133d).
It must finish before any implementation decision or turn-end cleanup.

D3's100ms full walk has22correct headers, then an extra key at the R3=4 group
shifts the rest. The replacement single-key overlay retains70ms presses and
sends each space in its own MBC invocation; it is active, with parent sole
MiSTer ownership. No canonical author edits and no accepted44page claim.

Hosted mode 3113323 synthesis is active after exact-SHA44bench/lint and
production-T80 CI pass. No duplicate VM build was started. The build worker
owns only artifacts, and the DT worker owns only private headless CPCSE/replay
experiments; neither may operate MiSTer or AmSpirit. AmSpirit's clean original
credits remain paused. No new code after the accepted44gate.


## Checkpoint — 4 November, 01:45 UTC

The mode-only 3113323 full build is verified and on MiSTer. Hardware runs complete:
BI25commands/8captures/zero failures; AE/AR21/7/0 on both new and baseline;
DSC416/3/0 and Amazing13/2/0. BI pulse widths are preserved and its one-us mode
is fixed; AE's readable09 and AR's stripe family match original Plus. DSC4 and
Amazing CRTC3 remain broken. No demo-wide, master or release acceptance is claimed.

One focused Astra-medium leaf resolved the D3 source question; persisted model
and effort matched the request.35/04 has no intrinsic timeout: Space is level-
sensitive across handlers. The separate-key walk still shifts at R3=4. Stop full
blind delay sweeps; the next discriminator is an accepted-key boundary trace.
Gemini4Argon high is additionally authorized if Astra does not help; it was not
needed for this question.

The DT replay has two raw GA black intervals and one acquired interval. Classic
SNA counters were not restored, so repeated settled cycles establish the model's
boundary, not exact continuation of the captured hardware phase. Parent corrected
metadata: per-marker SSM records do contain applied Full0 at 02F8/02FA despite the
null top-level summary.

E's Opus review completed NEED-DISCRIMINATOR and is preserved. A4AD's deliberate
1us/frame drift prevents the saved logs excluding an extra acquisition frame.
A precise C5 French follow-up is drafted, not sent. Production C5 is unchanged.

Original AmSpirit boots captured1800 before file load, then1803 after genuine
file load but before initialization. The first replay needs absent ROM/FDC;
the second matches all 128KiB of RAM and first 1891 CPU registers, with real short
HSYNC. Colon data reaches correct pens and is suppressed by raw FORCE_BLANK.
The worker is checking R2 write/JIT classification; no timing fix has started.
AmSpirit is paused on clean credits: PC2084, tick29224275, all ten colon bytes
match12/31, no breakpoints. Parent owns its API.

The other Sol-high worker owns the natural Plus C000→C802 trace. Genuine v3 CPU,
RAM and counters load correctly, with no trampoline or patches. Initial frame
wraps succeed; the saved later AmSpirit chain is 2253frames after the first, so
it is not the same invocation. A terminal-character R0 write is now the earlier
candidate; its type3 documentary expectation must be checked before a defect is
claimed. Both workers own ignored scratch and reports only.

No bridge or hardware job is active. MiSTer is on3113323, Amazing's final black
screen. Live Codex OAuth01:38 UTC reports31% weekly used, reset November5 19:34 UTC,
primary unknown. The user's60% pause threshold remains; banked resets are untouched.
Production code remains3113323; subsequent changes are documentation. No master
integration, release or PR is authorized.


## Checkpoint — 4 November, 02:30 UTC

Production code remains3113323. The mode-only RBF has the bounded BI/AE/AR
hardware acceptance above; DSC4/Amazing type3 remain broken. A4 adds an
independent reference constraint: original Plus and fresh CPCSE reject HCC3D,
while MiSTer accepts it. A genuine CPCSE Plus pre-A4-menu v3 SNA is ready for
a production-motherboard/T80 replay, with real key4 injection, not a synthesized
CPU phase. The Sol-high ASIC worker owns that private baseline unit.

The private >= counter experiment scores4/4 against the correctly ordered first
E2B/E54 reference, but retains its documented overflow-test conflict and later
phase residual. The character-clock gate is rejected: it loses DSC4's actual
A13-low OUT(#FF),A register select and consequently writes R9 instead of R0.
All eight BI width controls still pass, illustrating why BI alone is insufficient.
Both experiments are frozen and no production repair is accepted from them.

An AmSpirit control proves nonempty breakpoint replacement resumes execution,
as does an empty replacement. An unconditional resume after acknowledgment can
skip an already-hit first target. The ordered first-loop probe corrects the old
E54 ordinal; tooling guidance now records the safe stop check. AmSpirit is paused
on a clean original classic credit boot, PC2084/tick29164371, ten correct colon
bytes, no breakpoints. No API or hardware job is active.

Argon-high completed a focused classic timing source pass. French143–144 explicitly
anchors the black interval to displayed character R2−1; parent rendered/read143.
Sol-high is preparing a private actual-T80 unique-byte NJIT/JIT vector. No RTL
edit precedes a failing cross-module proof. A separate Gemini3.8-high guarded
worker owns only ignored CPCSE E instrumentation, to observe the actual C5 route
behind its0080 rather than infer it from static source. Its run is
20261104T022818Z-80045-c04a; the parent retains ownership until exit.

Live Codex OAuth02:25 UTC reports33% weekly used, reset November5 19:34 UTC;
primary unavailable, three banked resets untouched. Continue bounded units and
pause near the user's60% weekly threshold. No master integration, release or PR
has been requested.


## Overnight authorization update — 4 November

Renaud is away/asleep and raised the clean-wrap threshold from60% to approximately
80% weekly Codex usage. Continue bounded authorized units, preserve active work,
and wrap up without abruptly killing builds or workers. Do not redeem banked
resets. Previous checkpoint thresholds remain historical readings, not the active
limit. Author questions remain unsent; no integration/release request was added.


## DT correction and classic relative proof — 4 November, 02:45 UTC

Fresh unmodified CPCSE built-in R7 tracing disproves the DT replay's phase
assumption. Actual G writes occur at C0/C4/C9=12/0/0,27/10/0,42/20/0; replay
had61/6/7,12/17/0,27/27/0. Original JP69E3 bypasses CALLA241 after its first
iteration, so missing classic counters do not recover with repeated frames.
The apparent local acquisition explanation is withdrawn in the report, backlog
and current status. Full/Raw CRT device runs each finish32commands/14captures/
zero failures; applied modes0/2 are confirmed at both target markers. Raw CRT
crops the picture and does not isolate raw band count. Runtime Full is restored
by a four-command cleanup reset; MiSTer is on classic BASIC,3113323.

The private classic actual-T80 vector is a valid candidate fail-first proof:
matched ordinary/JIT writes, unique six bytes and24unblankedRGBslots pass;
relative blank onset is5clocks versus nominal12, outside conservative±4.
Trailing edges match; raw and RGB widths remain separate, no physical T2/T3
claim. Argon-high read-only review is active at
20261104T024051Z-84726-b36f, with no test rerun requested.

A4's original menu restore exposed a separate concrete first-fetch problem:
T80pa's IntCycleD_n has no reset value, so the initial memory fetch also asserts
IORQ, allowing PPI-B to corrupt CA toC0 at4195. The Sol-high worker owns a focused
fail-first B18 test and minimal reset repair if proved, followed by authentic
menu continuation. No CRTC/ASIC timing change is authorized from that symptom.
General gate and review wait until the provisional diff is ready.


## Checkpoint — 4 November, 03:05 UTC

CPU reset repair is committed as9de75d3 after Gemini3.8-high ACCEPT and the final
single44bench selected gate. Strengthened native trace/freeze/14IRQ checks also
pass; focused B18 cold+6restore/host checks passed after the RTL edit. No code
edits followed. The original A4 menu now resumes faithfully and real key4 reaches
its caller: topresults[0,0,0,0,0,1,1], reproducing3DwrongOK against originalPlusKO.
OUTI has not been reached within the bounded replay; no full A4 pass is claimed.

Argon accepts the classic relative5-versus12clock fail-first proof. Sol-high is
adding source-derived absolute unique-byte visibility/reactivation checks and
one private phase variant; root flagged the measured3clock RGB-release lag so
raw and visible widths cannot be conflated. No classic timing edit is in production.
The other Sol-high worker owns only a private captured-write/character-consume
ASIC hypothesis, checked against authentic A4, ordered DSC4 R0 and BI width/alias
controls. It must preserve A13-low transfers; no production change is accepted.

Fresh AmSpirit canonical C1 history reaches E02C9 and visibly prints0080 with
all seven numeric controls, on1.17.1/core2491682,6128/CRTC1/FR/PAL/monitorOff.
It is paused there, with no active script or breakpoints. No C5 counter was
exposed; streamed classic SNA remainsv2. The instrumented CPCSE build's identical
E BMP and trace prove its earlier zero-R5 release, not positive active re-entry.
Raw worker prose had a32line/512us conversion error; parent corrected units while
preserving the original trace. No C5 production rule was changed.

MiSTer remains on3113323 in runtime Full/classic BASIC after the DT comparison.
No API/device/build job is active. All three guarded bridge runs completed0;
provider logs are preserved under the corresponding ignored proof directories.
Codex live02:46 UTC is34% weekly used; active user ceiling is80%, banked resets
untouched. No master integration, release, PR or author message is authorized.

## Checkpoint — 4 November, 04:05 UTC

CPU9de75d3 exact-SHA CI41764626693 is green, including production-T80 and the
required gate. A narrower classic proposal now passes eight source byte windows,
CRTC239, real-T80 IRQ8 and one final44-bench selection; intended soak is
0xdc253576aa4029a0. Fresh Opus review20261104T040025Z-1839-e8fa is active, with
production frozen. It delays sampled GA blank plus qualified JIT onset; it does
not delay the raw falling edge. Global blank includes vertical edges, whose
absolute RGB latency is not independently accepted. Type0's old-mode extra-M2
failure remains explicit. No RBF exists for this proposal yet.

The private broad ASIC capture/consume mechanism passes genuine A4 firstOUTI01,
but a smaller entered-terminal R0 decision bit now passes all the same bounded
A4 and DSC4 counter observables without changing register admission. The worker
is preparing a retained real-T80/motherboard/PPI test before production edits.
Source status bits require literal C0=R0, so the production proposal must keep
those comparisons separate from the remembered line-end event.

AmSpirit currently holds the unchanged synthetic OUT3D program paused at HALT,
PC804B, with no breakpoints or script active. It independently falsified the
unsupported post-return zero observer assertion; the critical-stop one-character
difference remains documented. MiSTer remains3113323, Full/classic BASIC. Live
Codex usage at03:55 was36% weekly; ceiling80%, resets untouched.

Optional local-VM preflight could not establish a trusted SSH host key, and the
repository reports no registered GitHub runners. No host key or account setting
was changed. Hosted synthesis remains available for the next reviewed milestone;
do not stall the run or duplicate builds to investigate this optional route.
