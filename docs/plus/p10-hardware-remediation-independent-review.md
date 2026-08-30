# P10 hardware-remediation independent review

**Reviewer:** Claude Opus 5, xhigh, through the guarded `ask-claude` bridge
**Date:** 2026-08-30
**Reviewed base:** `d621230`
**Reviewed state:** uncommitted `plus/p10-hardware-remediation` worktree
**Mode:** read-only
**Verdict:** NOT CLEAR

The report below is preserved verbatim. Later remediation status belongs after
the report rather than editing the reviewer's words.

---

# Independent review — `plus/p10-hardware-remediation` uncommitted diff vs `d621230`

**Mode:** read-only. No edits, commits, branch changes, or delegation. Gates were **not** re-run (plan mode); the only non-writing command I executed was a standalone `verilator --lint-only` on the new module.

**→ Verdict: NOT CLEAR.** I found no live functional defect in the changed RTL — the four remediations are correct and source-backed. Three integrity defects block, all small edits.

---

## Verified defects, severity-ordered

**1 → MEDIUM-HIGH · `rtl/plus/asic_video.v:350-353` now asserts the opposite of `:398`.**
The block comment says *"there is no C9 term in the type-3/4 condition, unlike type 0's C4=C9=C0=0 (§20.3.1)"*. The diff adds exactly that C9 term (`raster_n == 5'd0`). **The code is right**: ACCC §20.3.4's narrative is "When the counters C4, C9 and C0 change to 0" (`docs/accuracy/extract/pdf2md/accc-v1.10-paged.md:7844`), and `docs/accuracy/compendium-03-display-regs.md:365` states "type 3/4 match type 0's two-stage behavior". The comment is what's wrong, and left as-is it will get the SSCR fix reverted by a future session. Also worth noting: only v1.10 extracts exist in-repo; the ranked authority (v1.11 PDF) was not consulted for this rule change.

**2 → MEDIUM · `rtl/plus/asic_video.v:664-667` is now a divergent duplicate of the same rule.**
`frame_origin` still lacks the C9 term while its comment claims it is *"the same edge the video pointers reload from R12/R13 (C4=C9=C0, §20.3.4 p.243)"*. The two sites matched before this diff. With R9=7, `frame_origin` fires 8× per frame, so `frame16_cnt`/`frame16_toggle` → status bit `s2_bit3` (`:658`) advances ~8× too fast. The behaviour is pre-existing; the false claim is newly created.

**3 → MEDIUM · `rtl/plus/plus_fdc_decode.v` is outside the lint gate and fails it.**
`sim/plus/Makefile:361-395` gives every other `rtl/plus/` leaf a `--top-module … -Wall` entry (`asic_unlock`, `asic_regs`, `asic_video`, `asic_sprites`, `asic_dma`, `plus_model_select`, `plus_cartridge_memory`, `plus_cpr_parser`, `plus_mmu`). The new module has none. Verified directly:

```
%Warning-UNUSEDSIGNAL: rtl/plus/plus_fdc_decode.v:13:16:
  Bits of signal are not used: 'addr'[15:11,6:5,3:0]
%Error: Exiting due to 1 warning(s)
```

Green `make -C sim lint` is therefore not evidence for this module. Needs a `lint_off UNUSEDSIGNAL` pragma plus a lint top.

**4 → MEDIUM · `sim/plus/p10_boot_test_top.v:412-413` still re-declares the FDC decode**, with the *old* A9/A4 equations. It now disagrees with production in classic mode. `docs/plus/hardware-checkpoint-findings.md:196` ticks `[x] Use one shared production/test module` — three copies existed and one was removed. Prior finding 2 is only partially closed.

**5 → MEDIUM · `rtl/plus/asic_sprites.v:444-445` is redundant and `:439-443` misdescribes it.**
`if (ACC_EN && FQ_REQ && …) fq_acc <= 1'b1;` requires `FQ_REQ` high, so it cannot "sample the launch edge" as the comment claims — the launch edge is covered by `:496`. And `FQ_REQ` is only ever set with `walk_act==1` (`:490-497`), so the tail `else if (fq_acc_hit)` at `:515-516` already covers every cycle `:444` can fire, including the `do_pop` cycle. Dead logic with a contradicting comment: the exact class prior finding 8 called out.

**6 → MEDIUM · `rtl/plus/asic_sprites.v:439` "ACC_EN is normally a one-cycle CPU-port pulse" is false.**
`rtl/plus/asic_regs.v:591` — `spr_acc_en = asic_cs && (mem_rd || mem_wr) && (wsel == 2'b00)` — is a **level held for the whole Z80 memory cycle**. `s16` drives it for one clock (`sim/plus/asic_sprites_test.cpp:1072-1076`), so the production shape (multi-clock level, repeated poison-and-re-demand of that sprite's fetches) is still unmodelled. Prior finding 7 is not closed.

**7 → LOW-MEDIUM · the MRER/RMR2 rule change carries no citation.**
`rtl/plus/plus_mmu.v:191-206`, `rtl/plus/asic_ga_timing.v:51-54,239`, and the **rewritten assertion** at `sim/plus/plus_mmu_test.cpp:288-300` change a documented rule with bare prose. The rule is correct and sourced — `docs/plus/references/asic-reference.md:54-55` ("While locked, RMR2 writes are interpreted as plain MRER (bit 5 ignored on old CPC)") and `:73`, plus `docs/references/Amstrad CPC Gate-Array.md:163-175` (RMR summary, bit 5 "not used") — but nothing at the implementation point says so, and a previously passing assertion was replaced without the citation that justifies replacing it.

**8 → LOW · residual lock-state leak.** `ga40010` (`rtl/Amstrad_motherboard.v:692-737`) still latches unlocked `%101` as MRER in Plus mode. The ownership-critical paths are clean — `mode` (`:677`) and `INT_n` (`:165`) mux to the ASIC, and `Amstrad_MMU.romen_n` is forced (`:747`) — but `romen` (`:155`) still carries the stale classic decode to `CPC_Dandanator.nRomEn` (`Amstrad.sv:1375`).

**9 → LOW · `rtl/Amstrad_motherboard.v:747` also removes all onboard/expansion ROM service in Plus mode.** `Amstrad_MMU`'s `ROMbank`/`rom_map` path (`rtl/Amstrad_MMU.v:80-81`) becomes unreachable when `plus_mode`. MF2 and Dandanator are unaffected — they override the address at `Amstrad.sv:696`. Consistent with `plus_exp_n = 1'b1` (`Amstrad.sv:1112`), but the user-visible consequence (loaded expansion ROMs silently do nothing in Plus mode) is recorded nowhere.

**10 → LOW · `rtl/plus/asic_video.v:368` duplicates `:592` verbatim** — the same SSCR formula in two places that must stay in sync.

**11 → LOW · unrecorded SSCR model conflict.** `asic-reference.md:359` says "added to the low 3 bits"; `:365` says "RA output = raster count + scroll value (≈ ANDed with &1F)". The code implements the former. They differ when R9 > 7, and neither the code nor the checkpoint names the conflict.

**12 → LOW · `sim/plus/p0_boot_test_top.v:101-103` indentation broken** by the edit (over-indented ports and closing paren).

**13 → INFO · no bench elaborates `Amstrad.sv`.** `sna_parser_reset` (`:730-736`), the `plus_asic_unlocked` wire, and the production `plus_fdc_decode` instance (`:853-861`) are unverified by `make -C sim`. Only Quartus would catch a wiring error there, and the branch is uncommitted so CI has not run.

---

## What I verified as correct

**→ FDC decode reproduces upstream exactly.** `git show master:Amstrad.sv` gives `fdc_sel = {A10,A8,A7,A0}`, motor on `!fdc_sel[3:1]`, u765 on `== 3'b010`. `plus_fdc_decode.v:25-26` is bit-identical, and matches `docs/references/I_O port allocation.md:73-88` (A10=0 and A7=0 select the FDC; A8/A0 pick the mode; A9 and A4-A1 ignored). Dropping the A9 qualification is safe because real PlayCity ports all have A7=1. The Plus path is unchanged from HEAD, `status[17]` gates u765 only as upstream, and `sim/plus/plus_p8_test.cpp:302-350` pins all 24 cases.

**→ SNA retention closes prior finding 1 at the leaf.** `sna_parser_reset` excludes the whole SNA lifecycle; `reset` drops at `sna_apply_cnt==2` and `sna_load` fires at `==1` (`Amstrad.sv:632-638, 726-742`), so `plus_mmu.v:189-196` and `asic_unlock.v:53` consume live shadows. `plus_sna_parser.v:71-78` clears the shadow on the next download's first clock, so a later classic SNA cannot inherit it. The new vector at `plus_p8_test.cpp:225-239` genuinely fails on HEAD.

**→ MRER/RMR2 split is right.** `ctrl_en` (`asic_ga_timing.v:239`) suppresses mode, ROM enables and IRQ-clear for unlocked `%101`; `plus_mmu.v:202-206` mirrors locked `%101` into `lromen`/`hromen`. `r04` proves the GA half.

**→ SSCR row advance is right.** `ra_eff == R9` at `HCC==R1` matches `asic-reference.md:367-368`. `t08i` fails on HEAD for **both** halves of the change — the old per-line reload would clobber the latch at raster 6.

**→ Classic non-regression holds.** `plus_unlocked = plus_mode & unlocked` (`plus_mmu.v:152`) and `plus_mode ? 1'b1 : romen_n` make both motherboard changes no-ops in classic mode. The FDC change is a deliberate classic behaviour change *back to* upstream.

**→ Port coverage is complete.** Every `asic_ga_timing` / `plus_mmu` / `Amstrad_motherboard` instantiation was updated (`asic_ga_diff_top:138`, `p1_video_test_top:85`, `p1_mobo_bench_top:51`, `p0_boot_test_top:101-102`, `p10_boot_test_top:256,328`, `Amstrad.sv:1143,1258`). `asic_pri_test` uses `asic_ga_timing` as Verilator top, so the new input defaults to 0 (locked).

**→ Documentation is honest.** Six over-claimed `[x]` boxes correctly reverted, CF-6/CF-7/CF-8 added, P10a downgraded, P10d/P10g named as unstarted. I found no false evidence claim other than item 4.

---

## Residual hardware uncertainty (correctly left open)

Burnin' Rubber / Pang / Panza causality for CF-6; RoboCop for CG-3; Navy Seals and World of Sports for CF-8; `The Demo` and BASIC/System `read fail` for CF-7 (needs a real u765 + DSK transaction); the `blank_cnt` hole shape; the DMA `+1/+2` contention extension; the exact-tip full-effort timing gate.

---

## Prior blocking findings

| # | Prior finding | Status |
|---|---|---|
| 1 | SNA RMR2/unlock restore dead | **Closed** at leaf; production `Amstrad.sv` seam untested |
| 2 | FDC tests test a copy | **Partial**; `p10_boot_test_top:412` copy remains and now diverges; no `u765` bench |
| 3 | `dma_ppi_wait` zero coverage | **Open**, correctly reopened |
| 4 | Tape gating untested | **Open**, reopened; misleading test name corrected |
| 5 | DMA LOAD `+1/+2` missing | **Open**, reopened |
| 6 | `blank_cnt` 8→2 unsourced + stale comment | **Open**; `asic_sprites.v:241-244` still false, `:255` still `4'd2` |
| 7 | `s15` does not drive ACC_EN | **Open**; `s16` drives a pulse, not the production level |
| 8 | `fq_acc` write-only dead code | **Closed** functionally; new redundant statement + wrong comment introduced |
| 9 | Timing-clean baseline never established | **Open**, correctly stated |
| 10 | Harness fidelity | **Partial**; P10a downgraded in docs, but `dbg_wait_n` (`:470`) misnomer and both weak assertions unfixed |
| 11 | SNA FIFO no full detection | **Open**; `sna_ioctl_wait` still never read by `plus_p8_test.cpp` |
| 12 | `asic_regs` mux priority | **Open**, untouched (`asic_regs.v:593-595`) |
| 13 | `tape_in = 1'b1` unsourced | **Open**, reopened (`Amstrad_motherboard.v:786`) |
| 14 | DMA fetch cycles per channel | **Open**, untouched |

---

## Verdict

**NOT CLEAR.** Findings 1, 3 and 4 block: a comment asserting the opposite of the rule it documents, a new production module excluded from the lint gate that fails it as written, and an `[x]` exit box that over-claims decoder unification while a divergent third copy remains. None is a live functional bug; all three are small edits. Keep the `docs/review-debt.md` row open.

**Also found:** I could not write the report to `/Users/renaudg/.claude/plans/` — the directory does not exist and creating it is not a read-only action. The report above is the complete deliverable.

---

## Parent remediation status — 2026-08-30

The verbatim verdict above remains the review record. The uncommitted worktree
now contains the following focused remediation:

- Blocker 1: the video-pointer comment now records the selected ACCC v1.11
  section 20.3.4 opening-sentence reading and its same-page C9 wording conflict.
  Pointer reload and the status-2 timer share one named frame-origin condition.
  `t07f` uses R9=7 and proves the timer toggles once per 16 frames; the pre-fix
  condition fails that vector.
- Blocker 3: `plus_fdc_decode` has an input-scoped unused-bit waiver and its own
  `-Wall` lint target.
- Blocker 4: the P10 harness instantiates the shared decoder instead of carrying
  copied equations. Its absent menu-disable input is intentionally tied to FDC
  enabled; production and the P8 truth-table test retain the real setting.
- Findings 5-6: the redundant sprite assignment is gone, the comments describe
  `ACC_EN` as a complete-Z80-cycle level, and `s16` now sustains that level across
  launch/in-flight clocks before a delayed stale ACK, then proves re-demand and
  rendering of the post-write nibble.
- Finding 7: implementation and MMU-vector comments cite the repository Plus
  ASIC reference sections 1-2 and the classic Gate Array MRER bit table.
- Findings 8-9 and 11: the residual Dandanator ROM-enable seam, Plus-mode masking
  of classic expansion-ROM service, and the SSCR R9>7 source conflict are now
  explicit architecture/hardware work rather than silently implied behavior.
- Findings 10 and 12: the SSCR effective-RA expression is shared and the P0 port
  indentation is repaired.
- Finding 13 remains true: no local bench elaborates the full `Amstrad.sv` top.
  Quartus synthesis is still required before a hardware build is accepted.

Post-fix parent gates:

- `make -C sim`: PASS — 175 classic vectors and every Plus bench, including 57
  video, 16 sprite, 8 GA differential/directed, P8, and P10.
- `make -C sim lint`: PASS, including standalone `plus_fdc_decode -Wall`.
- `make -C sim soak SOAK_EXPECT=0x48146d2b681268ab`: PASS, exact hash match.
- `git diff --check`: PASS.

Review debt remains open until an independent remediation pass confirms the
blockers are closed. Hardware/title causality and the earlier production-level
coverage gaps remain outside this local remediation.

## Post-remediation final scan and CF-5 production repair — 2026-08-30

A later read-only production-path scan found three HIGH CF-5 defects beyond the
reviewed shadow-retention fix:

- `asic_regs` used the ordinary machine reset and therefore discarded CPC+
  payload writes throughout the SNA download;
- the registered write strobe consumed live `ioctl_dout`, could remain asserted
  across non-accepted cycles, and the end-of-chunk branch suppressed the final
  payload strobe;
- snapshot apply could start before the parser FIFO, current `asic_sna_wr`, and
  the final registered top-level strobe had drained.

The current uncommitted remediation gives the parser its own lifecycle, pulses
`plus_asic_reset` for one clock at SNA start, captures accepted byte data with a
one-cycle strobe, accepts the registered tail after `sna_download` falls,
exports `busy`, and makes delayed apply wait for the complete write pipeline.
The P10 harness connects the new motherboard reset input to `sys_reset` rather
than an undriven implicit net.

`p8_04` is a production-shaped seam test: it instantiates the production parser,
`asic_regs`, and `plus_mmu`, honors `ioctl_wait`, forces the final registered
payload after download falls, waits for drain, and verifies sprite, palette,
control, DMA, RMR2/unlock, apply, and subsequent-reset behavior. It does not
elaborate `Amstrad.sv` itself, so the exact top-level ioctl decoder, reset/strobe
expression, PPI/PSG restore, and model application remain Quartus/hardware
boundaries.

Parent verification after the final changes:

- complete `make -C sim`: PASS, including 175 classic vectors and all Plus
  benches;
- focused rebuilt P8 and P10 benches: PASS;
- complete `make -C sim lint`: PASS, with the new decoder target and corrected
  P10 ASIC-reset connection;
- `make -C sim soak SOAK_EXPECT=0x48146d2b681268ab`: exact match;
- `git diff --check`: PASS.

This section records work performed after the verbatim Claude review. Review
debt therefore remains open until the expanded current delta receives a fresh
independent remediation pass.

### Final Luna re-scan: rapid snapshot restart

The final read-only Luna scan found one additional medium defect: a new
`sna_download` edge cleared retained shadows but left 1-3 old FIFO entries and
an already-presented write able to replay into the reset image. The parser now
clears its pointers and output strobe on restart, and restart has explicit
priority over the normal dequeue block whose conditions still reflect pre-edge
pointers. The new `p8_04` restart sequence queues old writes, drops download for
one clock, restarts before drain, and asserts that no prior write tail leaks.
It failed before the dequeue-priority correction and passes afterward.

A later requested Opus-high surgical re-review was stopped when provider quota
entered paid extra-credit usage. It returned no review text and therefore does
not alter the verbatim verdict or clear this debt.
