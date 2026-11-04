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

Pause at approximately **60% weekly Codex usage** until Renaud authorizes
more. Initial live OAuth reading at 22:24 UTC: 21% weekly used, reset
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

The full C-1 walk on the combined artifact completed80captures: E/P
readable, E numeric60/80residual remains, 02D3now0240expected in this
run. 66PNGs are byte-identical to old C-1f. Repeat full history on the
classic-only build8dde1e7, hosted run41742364942, still in progress.

Next: finish classic-only artifact checks and its repeat C-1 history; use the ASIC candidate only for diagnostic
DSC4/Amazing counterfactual captures; finish D3 comparison and preserve a
host-paced overlay. Resolve the ASIC acceptance gap before any verified
publication or integration claim.
