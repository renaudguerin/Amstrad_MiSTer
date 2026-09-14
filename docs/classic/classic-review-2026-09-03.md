# Classic-accuracy independent review — 2026-09-03

Read-only review by Muse of committed `a98590a33345f2c529fcd6d30676235d4193bfd7`.
Method: `git show` at the pinned base only (active FDC work could interfere).
No edits, no delegation, no gates executed (concurrent FDC writer; gate values
below are reported, not reproduced). French ACCC v1.11 controls; author
correspondence and prior source-equipped reviews are documentary/model
evidence, never hardware proof.

Scope: B9 classic slice (`825ecef` archive moves + index READMEs; `84e6969`
dead-scaffold trim — classic delta only), Round 2 consequence + B1 hybrid
blanking (`1655188`, feature tip `eb45555`), OSD sync-filter toggle
(`74882c7`, merge `0e3a248`). Excluded with no verdict: prior IA-1/2/3/4/6 and
B10 rows (already cleared, no re-verdict); B3, B6, all Plus-only rows (P10j,
B7, hardware-defect triage); the FDC writer; the B8 architecture audit.

## 1. B9 review-record archive + first test-suite cleanup — classic slice passes; bookkeeping remainder owned separately

Classic delta: deleted unused `expect_xfail_*` wrappers (`sim/sim_main.cpp`
~-110 lines), reworded stale fixture-first comments (t07/t08, t22, t24b,
t21/t27/t28/t29 registrations), `docs/accuracy/testbench-spec.md` rewrite
(v1.11 reference, 192-vector registry), `sim/README.md` count.

Checked at base: zero `expect_xfail_*` definitions or callers remain; generic
`known_divergence` (`:669`), flag and XFAIL/XPASS runner (`:8246-8258`)
retained. Registry still carries t21/t22 entries as required passes; sampled
comments honestly describe required-pass state. `testbench-spec.md` matches the
in-source harness (192 vectors, VCD on failure). Archive indexes
(`docs/accuracy/archive/README.md:1-28` via `825ecef`) link to live
`current-status.md`/`review-debt.md`; active rule sources, the dated Round 2
response (`accc-author-response-round2-2026-08-31.md`), consequence audit, and
FDC records remain unmoved (spot-checked).

At review time the row stayed open for the excluded Plus P8 dedup slice
(`sim/plus/plus_p8_test.cpp`, `sim/plus/Makefile` in `84e6969 --stat`) and a
full keep/move + relative-link audit (this review sampled only). Both are
closed elsewhere: the Plus pass at the same base clears the P8 dedup, and the
2026-09-03 mechanical reconciliation verifies every archived file exists, no
non-archive reference to a moved record remains, and the keep/move boundary
holds. No classic rework indicated.

## 2. Round 2 consequence + B1 hybrid blanking — CLEAR

One coherent slice (`e2ac61b..eb45555`: `rtl/CRTC.v`,
`rtl/crtc_type0_engine.v`, `rtl/crtc_type1_engine.v`, `rtl/crt_filter.v`,
`rtl/Amstrad_motherboard.v`, `sim/sim_main.cpp` t02l-t02r/t08j group,
`sim/crt_filter_blank_test.cpp` 6→9 tests, consequence audit +
`b1-hybrid-live-blanking.md` + `q20-r5-zero-independent-review.md`).

- Q20 stuck-R5 row reset: author 2026-08-31 confirms the §11.3.2 R5=0 reading
  (`accc-author-response-round2-2026-08-31.md` Q20/N2). RTL
  `crtc_type1_engine.v:363-370` resets only `row_next`, not frame/adjustment
  end; rollover-effective R5 preserves same-edge 0→positive exit; `:470`
  drives row-0 R12/R13 reload and `:549` selects actual `row_next` for VSYNC at
  that boundary. Failure-first `t08j` plus R7=0/R7=R4+1 discriminators;
  ParityC9-on-reset explicitly left as source/hardware gap — preserved, not
  inferred.
- Type-0 C0=2 qualification: English §16.4.1.2 p.168 plus author-normative
  p.169 paragraphs. `crtc_type0_engine.v:604-625` latches `seen/preceding`
  with live `hcc>=2` reconstruction after snapshot/type-switch clear;
  `:682-689` gates `vsync_fire_seam` and exports `vsync_line_blocked`;
  `CRTC.v:649,720` qualifies only type-0 natural fire and consumes blocked
  comparisons via `vsync_allow`. Type-1 field route stays neutral (`t02p`).
  `t02l/m/n` pin steady-R0=1 block, R0=0 freeze, R0=1 two-char pulse; `t02q/r`
  pin snapshot/live-switch reconstruction. `t02o`
  (`sim_main.cpp:1049-1077`, registered `:7679` as "model
  inference/hardware discriminator") pins `vsync_allow` consumption —
  correctly labelled inference, not author-confirmed. No blanket hardware
  claim; the consequence audit states the boundary explicitly.
- B1 Live blanking: `crt_filter.v:65-79,106-115` implements minimum-window
  phase anchor, exact-expiry reacquisition (`~ext || count==0`),
  healthy-cadence-only `hsz` learning, stuck-high/no-VSYNC watchdog promotion
  (`if(|hsz)`). Nine blanking tests pin Full geometry, R2.JIT +3-pixel phase,
  long-pulse override, non-restart, exact-expiry, missing-sync fallback,
  stuck-high fallback, masked-retrigger composition (`:383-425` selector pins
  Full/Live/Off tuples), matching the `b1-hybrid-live-blanking.md` contract.
  The +7-CE Full-vs-Live offset is labelled a model measurement;
  SHAKER/DSC4/Amazing-Demo A/B results retained as validation residuals, with
  the regenerated-sync-vs-physical-edge limit stated honestly.

No functional or test-integrity defect; citations carry French/English pages;
residuals stay residuals. Gate reproduction (192 classic + 9 blanking, lint,
soak `0x2263c9fc44af4ee7`) is reported, not re-executed — the integration
owner confirms on the next touching commit.

## 3. OSD sync-filter toggle — CLEAR

Two-bit selector `status[36:35]` (`Amstrad.sv:78,1305`): 0 Full, 1 Live
blanking, 2 Off. Default `00` = Full, bit-for-bit the old filtered path
(`Amstrad_motherboard.v:731-746`; mode 0 the full filtered tuple, mode 2 the
old raw tuple `hsync_ga/vsync_ga/hs_sel/vblank_ga`; VRAM-shift
`(sync_filter!=2)&crtc_shift` preserves the old `1'b0`=raw meaning). Bit 36
has no other consumer (`status[34:33]` Plus model, `status[32]` Dandanator
only); no `sys/` collision found in the touched files. Plus benches retargeted
to explicit `2'd2` raw, preserving pre-filter bench meaning. Production tuple
pinned by `test_production_mode_selector`
(`crt_filter_blank_test.cpp:383-409`). No ACCC oracle involved; no weakening.
Actual OSD rendering remains UI/hardware validation, not closure.
Live-blanking geometry itself belongs to the B1 row, not this wiring change.

## Retained residuals (validation, not review debt)

`t02o` model inference, ParityC9-on-reset gap, +7-CE model measurement,
SHAKER/DSC4/Amazing-Demo hardware A/B, OSD rendering. A CLEAR verdict is not
hardware closure. Published ACCC v1.11 unchanged; the author message is dated
clarification only.
