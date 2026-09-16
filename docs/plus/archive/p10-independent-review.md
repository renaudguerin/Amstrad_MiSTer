# Plus P10 independent review

**Reviewer:** Claude Opus 5, xhigh, through the guarded `ask-claude` bridge
**Date:** 2026-08-29
**Reviewed range:** `3233837..796ae04`
**Integration tip inspected:** `d621230`
**Mode:** read-only
**Verdict:** CLEAR WITH BLOCKING FINDINGS; keep review debt open

The report below is preserved verbatim. Remediation and parent verification are
recorded after it rather than editing the reviewer's words.

---

# Independent review — Plus P10a–P10f, P10h (`3233837..796ae04`, tip `d621230`)

Read-only. No writes, no branch changes, no delegation. Simulation and full-effort synthesis were **not** re-run locally; I verified instead that CI run `33260557663` at the tip passed both jobs.

## Overall verdict

**→ CLEAR WITH BLOCKING FINDINGS.** The RTL changes are mostly sound and CF-1/CF-2/CF-3 are correct against the primary digest. But one claimed fix (P10h) is only half-implemented, and the *test evidence* behind CF-2, CF-3, CF-4 and CF-5 does not exist in the form the checkpoint document claims it does — three of those exit boxes are ticked against tests that either test a copy of the logic or do not cover the named behaviour at all.

**→ The single most consequential problem is not a bug, it is the evidence.** `docs/plus/hardware-checkpoint-findings.md` now shows `[x]` beside six exit criteria that the cited files do not satisfy. A fresh session reading that file will believe the FDC path, the DMA/PPI WAIT and the production SNA path are simulation-verified. None of the three is.

**→ On the two hardware symptoms:** P10f is a credible mechanism for RoboCop 2 sprites and I expect it to improve them. P10c is a credible partial mechanism for the BASIC/System `Drive A: read fail`. Neither is proven, and the *first* P10a gate — an exact-tip full-effort timing-clean RBF — was never met, so the "half of cartridges load" split remains unexplained and un-narrowed.

## Per-area verdicts

| Area | Verdict |
|---|---|
| P10a T80 harness / WAIT | **Partial.** Wrapper WAIT semantics faithfully mirror `T80pa.vhd`; the *core* is substituted and the interrupt-ack path knowingly diverges. Timing gate unmet. |
| P10b PPI Port C (CF-1) | **CLEAR.** Correct and correctly tested. |
| P10c FDC/tape/reset (CF-2/CF-3) | **RTL CLEAR, tests void.** |
| P10e DMA arbitration (CF-4) | **RTL mostly correct, one documented rule missing, zero test coverage of the integration signal.** |
| P10h SNA parser (CF-5) | **Half-fixed. Confirmed remaining defect.** |
| P10f sprite write-through (CG-3) | **Mechanism sound, one unsourced constant, test does not reproduce production stimulus.** |

## Findings, severity-ordered

### 1 — HIGH · verified logic defect · CPC+ snapshot RMR2 and ASIC-unlock restore is still dead

`rtl/plus/plus_sna_parser.v:47` · `Amstrad.sv:1143`

The parser's internal reset is `reset || !sna_download`, which zeroes `asic_sna_rmr2` (`:53`) and `asic_sna_unlock` (`:54`). `sna_download` (`Amstrad.sv:232`) falls at end of file; `sna_load = (sna_apply_cnt == 3'd1)` (`Amstrad.sv:270`) fires roughly six clocks later via `sna_finish_pending`. `plus_mmu` latches `sna_rmr2`/`sna_unlock` only on `sna_load` (`rtl/plus/plus_mmu.v:172-175`).

**Failure:** load any CPC+ snapshot whose `CPC+` chunk sets RMR2 to page the ASIC in. Sprite pixels, palette and ASIC registers now restore correctly (that part of P10h works), but `rmr2_pos`, `rmr2_page` and `asic_page_on` all restore as 0 and the ASIC stays locked. The snapshot resumes with the wrong memory map.

P10h made this *newly reachable* — before the fix nothing parsed at all — so it is fair to call it in scope. The CF-5 exit box "verify … model application" would have caught it; that test does not exist.

### 2 — HIGH · test defect · The CF-2/CF-3 "production" FDC tests test a copy of the logic

`sim/plus/plus_p8_test_top.v:50-51` · `sim/plus/p10_boot_test_top.v:409-419`

Both testbenches **re-declare** the `Amstrad.sv` expressions rather than instantiating them. `u765` is instantiated in no bench in the repository. The 24 assertions in `test_p10c_fdc_motor_tape_gating` and the `dbg_motor` assertion in `p10_boot_test.cpp:352` all pass unchanged if `Amstrad.sv` is reverted to the pre-P10c code. This is tautological coverage, and it is the exact failure mode `CLAUDE.md` forbids.

Three checkpoint boxes rest on it and are false as written: the model matrix, the AMSDOS-alias production bench, and `active FDC command → CPR load/apply → first FDC access` (which has no test anywhere).

### 3 — HIGH · missing test · `dma_ppi_wait` and `cpu_psg_addr` have zero coverage

`rtl/Amstrad_motherboard.v:757` (`dma_ppi_wait`), `:759-764` (`cpu_psg_addr`), `:775-776` (PPI `we`/`oe` gating), `:200` (`wait_n`)

The only new DMA test, `d11`, runs against `asic_dma.v` alone (`sim/plus/Makefile:177`) and drives `cpu_psg_addr` as a testbench input. Nothing exercises the Z80 WAIT assertion, the PPI access gating, the AY-address tracker, or the CPU-versus-DMA concurrency the fix exists for. `p10_boot_test_top.v` instantiates the motherboard and could have, but does not.

CF-4's exit box claims "a production motherboard test with DMA active while the CPU scans the keyboard, based on the Arnold 5 DMA/keyboard diagnostic (verified in `sim/plus/asic_dma_test.cpp`)". That test is not in that file, and that file cannot contain it.

I traced the WAIT path by hand and believe it is functionally correct: `io_rd`/`io_wr` are levels held across wait states, `T80pa` re-evaluates `CEN_pol <= not WAIT_n` at every `TState=2`, `i8255` re-edges on `~old_we & we`, and `dma_load_owner` drops for a full 1 µs between channels so the worst stall stays inside the documented 8 µs. No interrupt-acknowledge false trigger: `RD_n` is held high during int-ack in both the real and substituted CPU. **That is inference from reading, not verification.**

### 4 — MEDIUM · missing test · Tape gating has no test at all

`rtl/Amstrad_motherboard.v:780`, `:794-795`

`test_p10c_fdc_motor_tape_gating` has "tape" in its name and in its PASS string, but contains no tape assertion. The only tape-adjacent checks in the file are the pre-existing `model_has_tape` decode assertions (`plus_p8_test.cpp:237-247`), which predate P10. The CF-2 box claiming "GX4000/6128+ tape cases" verified is false.

### 5 — MEDIUM · verified spec gap · The DMA LOAD contention extension is not implemented

`rtl/plus/asic_dma.v:250-350` versus `docs/plus/references/asic-reference.md:440-442`

The digest, citing the same `ARNOLD §2.6` the commit message cites, says: *"LOAD = ≥ 8 cycles; **+1 cycle if the CPU is simultaneously accessing the 8255, +2 if the CPU access is itself a PSG register write**"*. The implementation is a fixed 8 with no contention extension. The 8-cycle base and the "up to 8 µs" wait bound are correct (CCLK is 1 MHz, confirmed via `plus_cclk_en_p`), so this is an incomplete implementation of a cited rule, not a wrong one — but the checkpoint records the section as "verified".

### 6 — MEDIUM · unsourced behaviour change · `blank_cnt` reduced 8 → 2 with no derivation

`rtl/plus/asic_sprites.v:255`

`blank_cnt` decrements one per `PIXEN`, the 16 MHz dot enable (`:79`). `docs/plus/references/asic-reference.md:204-206` says the accessed sprite is removed *"for the duration of the access (~1 byte width per 1 µs access)"* — 1 µs at the PIXEN rate is ~16 dots. The old value 8 was already short; 2 is ~125 ns. No test pins either value (`s11` asserts nothing about the tail; its comment at `:815-820` explicitly declines to). The change is invisible to the suite in both directions.

The comment above it, `asic_sprites.v:241-244`, still says *"staged banks are invalidated in the fetch block below so the display re-reads the fresh image"* — that block was deleted in this diff.

### 7 — MEDIUM · test fidelity · `s15` does not drive `ACC_EN`

`sim/plus/asic_sprites_test.cpp:997-1035`

In production every CPU sprite-pixel write asserts **both** `spr_wr_en` and `spr_acc_en` (`rtl/plus/asic_regs.v:591,593`). `s15` drives only `spr_wr_en` and leaves `ACC_EN = 0`, so the interaction between write-through and the retriggerable blanking hole — the thing that decides whether RoboCop's sprites are visible during an animation burst — is never exercised. The test would pass with `blank_cnt` at 8, at 2, or removed.

The bench also updates its model RAM synchronously in `wr()` (`:117`), so the fetch path never returns pre-write data; the coherence hazard the write-through replaces is not modelled.

### 8 — MEDIUM · dead code + stale comment · `fq_acc` is now write-only

`rtl/plus/asic_sprites.v:324`, `:326-333`, `:333`

The diff removed the only `fq_acc <= 1'b1`. The signal is now permanently 0, so `fq_stale` (`:333`) reduces to the row-tag test, and the eleven-line comment at `:326-333` describes a guard that no longer exists — including the specific hazard *"the port serves grants through CPU WRITE cycles too, and a registered read can return the pre-write byte, so the access must poison the request for its whole life"*.

I chased that hazard and it is **currently unreachable**: `eff_cs` is level (`asic_regs.v:196`), `mem_wr` spans many 64 MHz clocks, so `spr_ram` is updated on the first edge of the write window while `ACC_EN` stays high for the rest — `fq_acc_hit` (`:335`) catches the one stale sample. The `sna_wr` path *is* a single-cycle pulse and would be exposed, but the sprite engine is held in reset throughout `sna_download`. Classify as latent, not live. It should still not ship as dead logic with a comment that contradicts it.

### 9 — MEDIUM · unmet gate · P10a's timing-clean baseline was never established

`docs/plus/hardware-checkpoint-findings.md:196-200` · `.github/workflows/build.yml:235-266`

P10a's first requirement was a full-effort exact-tip build with non-negative constrained-domain setup/hold slack and zero TNS, with the artifact identity recorded. `build.yml` parses the compile log only for `Warning (10236)` and for cosmetic stage timings; **it never inspects slack or TNS**. A green synthesis leg therefore carries no timing information. The last recorded fit (`7c46b8d` smoke) was **-0.796 ns setup, -10.576 ns TNS**. `docs/current-status.md:1123` presents P10a as delivered on the harness alone, with the timing half silently dropped — while the checkpoint says not to interpret further cartridge results until that gate passes.

This matters for the review question directly: a timing-invalid RBF remains the best available explanation for the title-dependent split, and nothing in this range narrows it.

### 10 — MEDIUM · harness fidelity · What `p10_boot_test` can and cannot prove

`sim/plus/tv80/t80pa.v:1-4`, `:122-128` · `sim/plus/p10_boot_test_top.v:344` · `sim/plus/p10_boot_test.cpp:270`

Legitimate approach — Verilator cannot compile the VHDL T80 — and the wrapper's WAIT handling genuinely matches `rtl/T80/T80pa.vhd:169-174`. Four limitations should be recorded rather than left implicit:

- The **core** is TV80, not T80. The wrapper comment at `:122-128` documents a deliberate divergence: real `T80pa` stages `IORQ_n` through `IntCycleD_n` and relies on T80's 5-T-state interrupt M1; the substitute asserts `IORQ_n` from M1/T1. **Interrupt-acknowledge and vector-supply timing observed here does not transfer** — which is precisely the P10g/Panza question.
- `no_wait(1'b1)` at `:344` disables normal memory wait states, so CG-2's "every cartridge byte incurs an SDRAM WAIT round trip" cannot be measured on this harness.
- No `u765`, no tape, `fdc_dout` hard-tied `8'hFF` (`:307`), `plus_mmu.sna_load` tied 0.
- `dbg_wait_n` (`:467`) is `~plus_cart_stall`, not the CPU's `wait_n`. `dbg_cpu_waitn` is the real one; the misleading name is worth fixing.

Two weak assertions in the test itself: `p10_boot_test.cpp:270` checks `dbg_pc == 0x0000` at the first M1, but `dbg_pc` is reset to 0 and only updates on the clock edge — it passes on the reset value. And the program reads `0xC000` after selecting cartridge page 3 (`:208`) but **never asserts it returns `0x42`**, so "cartridge page selection" is exercised and unverified.

### 11 — LOW · unguarded overflow · SNA write FIFO has no full detection

`rtl/plus/plus_sna_parser.v:42-44`

8 entries, threshold `fifo_count >= 4'd4`, 2 pushes per sprite byte, 1 pop per clock. That leaves ~4 bytes of slack after `ioctl_wait` asserts, and `cpc_plus_byte_wr` is itself a registered version of `ioctl_wr` (`Amstrad.sv:558`), consuming one. Almost certainly safe given `hps_io` honours `ioctl_wait` within a byte — the pre-existing RLE path at `Amstrad.sv:298` relies on the same immediacy. But overflow silently clobbers unread entries, and `sna_ioctl_wait` was plumbed out to `plus_p8_test_top.v:39` and is **never read by the C++ test**. The CF-5 backpressure claim is untested.

### 12 — LOW · mux priority inconsistency

`rtl/plus/asic_regs.v:594-595`

`spr_wr_en` qualifies the SNA term with `sna_addr[13:12] == 2'b00`, but `spr_wr_addr`/`spr_wr_data` select on bare `sna_wr`. A simultaneous `sna_wr` outside sprite space and a CPU sprite-pixel write would route the CPU's enable with the SNA's address/data. Not reachable today (the CPU is in reset during `sna_download`), but it diverges from `eff_addr`/`eff_data` at `:197-198` for no reason.

### 13 — LOW · unsourced constant

`rtl/Amstrad_motherboard.v:780` — `tape_in` forced to `1'b1` when the model has no tape. The CF-2 box says "pin the intended open-bus/read result"; the choice of 1 over 0 is not sourced and not tested.

### 14 — LOW · pre-existing, now more visible

`rtl/plus/asic_dma.v:202-241` always spends three fetch cycles regardless of how many channels are active; `asic-reference.md:438-439` specifies one fetch cycle *per active channel*. Untouched by this diff but the ST_DEAD rework sits right beside it.

### 15 — INFO · benign side effect of CF-1

With Port C always driven in Plus mode, `opc_r` resets to `8'h00` (`rtl/i8255.v:87`), so `portC[3:0] = 0` selects **keyboard row 0** from reset until firmware writes Port C, where previously mode `0x9B` clamped it to `0xF` (row 15, none). Transient and arguably closer to hardware. Noting it so it is not later mistaken for a regression.

## Missing tests, in the order I would write them

1. **`plus_mmu` RMR2/unlock survive to `sna_load`** — the failing vector for finding 1. Write it before touching the parser.
2. **`dma_ppi_wait` at motherboard level** — CPU `IN A,(&F4xx)` concurrent with a DMA LOAD: WAIT asserted, PPI `we`/`oe` suppressed, access completes after release, AY selected register restored. This is CF-4's actual exit criterion.
3. **Real FDC gating** — instantiate `u765` (or at minimum the `Amstrad.sv` decode itself, not a copy) in a bench. Delete the duplicated expressions in `plus_p8_test_top.v:50-51` and `p10_boot_test_top.v:409-419` in the same commit; leaving them is worse than having no test.
4. **Tape gating** — `tape_motor`/`tape_out`/`ipb[7]` across GX4000 / 6128+ / 464+ / classic.
5. **`s15` with `ACC_EN` driven**, plus a case where the write burst overlaps the display window, so the blanking model is pinned to *something*.
6. **SNA FIFO backpressure** — assert `sna_ioctl_wait` and confirm no entry is lost across a 2048-byte sprite region at maximum input rate.
7. **Boot-test hardening** — assert the `0xC000` readback is `0x42`; make the PC assertion non-tautological.

## Debt disposition

**→ Keep the `docs/review-debt.md` row open.** Its scope list is accurate and its six review headings are the right ones. This review is a cross-provider read of the diff, but it did not run the gates and it found enough unproven claims that a second pass after remediation is warranted.

**→ Correct `docs/plus/hardware-checkpoint-findings.md` before anything else.** Six `[x]` boxes should revert to `[ ]` with the reason recorded: CF-2 model matrix (tape), CF-2 open-bus/motor pinning, CF-3 active-command coverage, CF-3 AMSDOS-alias production bench, CF-4 production concurrency test, CF-5 `Amstrad.sv` integration test. CG-3's box should be narrowed to what `s15` actually does. This is documentation-only and needs no simulation gate.

**→ Correct `docs/current-status.md:1123`** to state that P10a delivered the harness only and that the timing-clean baseline — the gate the checkpoint says must precede any further cartridge interpretation — is still open. Add the slack/TNS check to `build.yml` or stop describing green synthesis as evidence of timing.

**→ P10d and P10g were not attempted** and the status entry does not say so. Worth one line.

## Verified defect versus hardware-only uncertainty

**Verified from source, no hardware needed:** findings 1, 2, 3, 4, 7, 8 (as dead code), 9, 11, 12, and the stale comments in 6. Finding 5 is verified against the checked-in digest, which is the ranked authority below real hardware.

**Hardware-dependent, correctly left as model assumptions:** the exact blanking-hole shape (finding 6 — but the *change* still needs a derivation, and the digest's ~1 µs is the current best evidence); whether write-through or the real ASIC's undocumented sprite-RAM port is closer to truth; whether the `+1/+2` contention extension is observable; the sprite `+3` Y-high-versus-magnification and `X>>3` conflicts the checkpoint already names.

**Neither — it is a gate nobody ran:** finding 9. The timing question is answerable today with one dispatch and a slack check.

---

## Parent triage and remediation status

Triage began from the reviewed tip without weakening tests:

- Finding 1 is confirmed and a failing retention vector was added before the
  parser/top-level reset fix.
- Findings 2–4 are confirmed as evidence gaps. Decoder correction is separate
  from a future real-u765 transaction bench.
- Findings 7–8 align with the real-hardware RoboCop result and an independent
  source investigation; a delayed-ack sprite collision vector is required.
- Finding 9 remains an exact-tip synthesis/timing gate, not a simulation claim.
- Findings 5–6 and 10–15 remain open unless a later section explicitly closes
  them with a focused test or sourced scope decision.

The review-debt row must remain open until the remediation range receives a
fresh independent review and the parent reruns the required gates.

### 2026-08-30 provisional remediation checkpoint

The uncommitted `plus/p10-hardware-remediation` worktree now contains:

- the SNA RMR2/unlock retention vector and dedicated parser reset lifecycle;
- a shared model-specific FDC decoder which restores the classic A10/A8/A7
  partial decode and retains the 6128+ firmware aliases;
- one authoritative ASIC lock state across `plus_mmu` and `asic_ga_timing`,
  plus classic onboard-ROM isolation in Plus mode;
- a delayed pre-write sprite ACK collision vector and whole-request access
  poisoning; and
- an SSCR vertical-wrap vector and effective-RA VMA row-base advance.

Parent gates pass: `make -C sim`, `make -C sim lint`, and
`make -C sim soak SOAK_EXPECT=0x48146d2b681268ab`. This does not clear the
review: the real-u765/DSK transaction, production `Amstrad.sv` SNA application,
DMA/PPI concurrency, exact-tip Quartus timing, and hardware retest remain open.
