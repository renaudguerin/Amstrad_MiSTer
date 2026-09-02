# Partial hardware retest reported 2026-09-02

The user reports testing `Amstrad_20260901_84e6969.rbf` with **Live blanking**
selected. These are user-run hardware observations, recorded against that build;
the report does not establish which individual change caused either improvement.

## Results

| Existing screenshot defect | User-reported result | Scope |
|---|---|---|
| `amazingdemo_sync_live_blanking` | **Appears fixed** with the latest Live blanking changes. | This Amazing Demo symptom only; retain the user's tentative wording. |
| `burnin_rubber_sprite_on_the_right_should_be_hidden` | **Fixed** in this retest. | The unwanted right-edge sprite only, not all Burnin' Rubber behavior or sprite geometry. |
| DSC4 and SHAKER | **Still failing**; the user recalls that the failure shapes may have changed. | No current screenshots or per-entry comparisons are available. |
| Other reported defects | **TBD**. | No new pass or failure is reported for them. |

These observations supersede the earlier negative results for these two named
symptoms at this build and setting. B1 and P10 remain open. In particular, Pulpo,
the CRTC3 demo's separate right-edge sprite leak, other Plus image
defects, FDC/AMSDOS reads, and reload recovery have no new verdict here.

No new screenshots, exact test time, CRTC/model selection, MiSTer version,
media hashes, output configuration, or reset/load sequence accompanied the
report. The screenshot names identify the existing defects, not newly supplied
captures. A Full/Off comparison was not reported.

## Artifact identity

- Named build commit: `84e69694fa42a24201630daa5b3ca01c1f6673c0`.
- Retained local artifact:
  [`Amstrad_20260901_84e6969.rbf`](../output_files/Amstrad-local-build-12-1-full/Amstrad_20260901_84e6969.rbf).
- Local SHA-256, independently read on 2026-09-02:
  `5787f6b8ed05ee8b9ad56506aa38b63be7c0e5b33de0c7a6c1860758d52f5cbf`.
  This identifies the retained file; the device copy was not independently hashed.
- Integration checkout at recording:
  `be08bf3ac93357a8c71a443283140bce43a52523`. Its changes since the named
  build affect workflow skills and CI-policy documentation only.

## Session scope authorized 2026-09-02

The user authorized shared FDC recovery, Plus work, reviews and local gates.
Accuracy RTL work is deferred: DSC4/SHAKER still fail, but the possibly changed
failure shapes need current captures or equivalent timing traces before choosing
a repair. Portable capture infrastructure and review can progress meanwhile.
The coordinator uses Gemini Flash 3.7 for bounded implementation and Opus 5
for its reviews; complex or critical implementation goes to Opus 5 alone, with
review debt recorded. A Gemini bridge failure permits Luna Max fallback. A
Claude failure gets one retry, then work stops for user instructions. Bridge
failures also trigger a separate Agents Roster repair task.

## Proposed next session — not started

The queue below was proposed before launch authorization; the scope above
controls this session. Hardware tests remain user-run.

1. **Prepare ownership and recover the shared FDC investigation first.** Stash
   `0fe18a4513a47e4f21e0f504f002673a853388c3`
   (`wip-fdc-b3-capture-2026-09-01`) remains intact. It contains
   `rtl/u765/u765_tb.cpp`, `sim/plus/p10_boot_test.cpp`, and
   `sim/plus/p10_boot_test_top.v`. Recover selected, explicitly owned work after
   branch preparation; keep the stash until recovery is committed and verified.
   The u765 pre-edge data-staging discriminator is unfinished work, not an
   accepted controller fix. Reproduce it and establish the bus requirement
   before changing RTL. Require a **classic AMSDOS regression as well as the
   Plus/System CPR disk path**: this peripheral is shared classic/Plus work.
   Preserve the production-VHDL-T80 versus Verilator-TV80 evidence boundary.
2. **Accuracy: finish the B1 hardware discrimination.** Retest Pulpo, DSC4 and
   SHAKER Module A `(T)`, `(Y)`, `(TAB)` and `(R)` on the named RBF, with CRTC
   selection and Full/Live comparison recorded per case. Keep Amazing Demo as
   an apparent pass. For a remaining failure, distinguish raw timing, selected
   blanking and scaler output before proposing another timing change. IA-5 and
   other author-response hardware discriminators remain separate open work.
3. **Plus: recover B3 capture, then use it on unresolved symptoms.** Complete
   runtime CPR ingestion and a bounded, self-describing frame capture with
   repeatable hashes. Keep raw/selected timing distinct from the shared,
   filter-dependent pixel payload; this harness is not an independent hardware
   image oracle. Use the symptom-specific discriminators in
   [the triage record](plus/hardware-defect-triage-2026-09-01.md) for the CRTC3
   sprite leak, Sonic, Copter 271, Dick Tracy and CRTC3 corruption. Retain the
   Burnin' Rubber right-edge result as a regression target. B13's
   Dandanator-to-Plus/Navy Seals load sequence still needs a hardware retest.
4. **Review and integrate only after the bounded work is ready.** Resolve
   relevant existing review debt and obtain fresh review of non-trivial changes;
   run the required simulation gates. Each stream stops at a committed READY
   handoff unless integration is separately authorized. Use serialized
   integration and an exact, verified artifact for subsequent hardware tests.

The coordinator owns shared status/build files and the FDC recovery boundary.
The Plus task may own B3's P10 harness only after any overlapping FDC edits have
been handed off; do not run two writers against those files. Accuracy owns its
CRTC work. Refresh ownership, branch ancestry and writability at actual launch.
Both fixed stream worktrees were clean and their tips already integrated when
this report was recorded. The main checkout also contains untracked private
reference files and screenshots, which must be preserved during preparation.

Published ACCC v1.11 remains unchanged. The author's message is dated
clarification and future-correction guidance; no corrected complete PDF exists.
