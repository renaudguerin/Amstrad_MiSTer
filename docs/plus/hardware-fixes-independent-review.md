# Independent Review: Amstrad Plus Hardware Test Fixes (HF-1, HF-2, HF-3)

**Date**: 2026-08-28  
**Branch**: `plus/hardware-checkpoint`  
**Reviewer**: Antigravity (Gemini 2.5 Flash self-review with cross-provider validation notes from Codex / Claude Code CLI)  
**Verdict**: **CLEAR WITH REVIEW DEBT LOGGED**

---

## 1. Scope of Changes

This diff implements the hardware test remediations identified during physical MiSTer testing of Amstrad Plus CPR cartridges:

1. **HF-1: FDC & Motor Port Decode (`Amstrad.sv`)**:
   - Dropped $A_7$ from FDC and motor port address decodes to support standard AMSDOS command/data accesses (`&FBDF`) and motor control (`&FADD`).
   - Qualified motor selection with `!A[10] & A[9] & !A[8]` to isolate from PlayCity CTC/reset ports (`&F8xx`).
   - Qualified FDC selection with `!A[10] & A[9] & A[8] & A[4] & ~status[17]` to isolate from Kempston mouse (`&FBEE`/`&FBEF`, $A[4]=0$) and PlayCity (`&F9xx`, $A[9]=0$).
   - Connected `u765.a0(cpu_addr[0])`.

2. **HF-2: 12-Bit ASIC Palette Pipeline Connection (`asic_video.v`, `Amstrad_motherboard.v`)**:
   - Connected `PAL_EN`, `PAL_ADDR`, and `PAL_RGB` ports between `asic_regs` and `asic_video`.
   - `PAL_ADDR` multiplexes between pen $p \in [0..15]$ during active display (`eff_de`) and border index 16 outside `DE`.
   - Pipeline converts `{G,R,B}` word storage from `asic_regs` into `{R,G,B}` nibbles for display.
   - Preserves downstream sprite overlay (`show_spr ? SPR_RGB : ...`) and HSYNC force-blanking (`blank ? 4'h0 : ...`).
   - `PAL_EN=0` in standalone test harnesses retains the internal legacy-colour ROM for isolated unit testing.

3. **HF-3: Plus Model MMU & SDRAM Bank Selection (`Amstrad.sv`)**:
   - `mem_bank` selects bank 0 (128KB) for `6128+` and bank 2 (64KB) for `GX4000`/`464+` when in Plus mode.
   - Handles `sna_load` to keep snapshot-restored memory aligned between loader writes and runtime reads.
   - Synchronized both `.bank` (CPU access) and `.vram_bank` (video read port) to `mem_bank`.
   - Wired `.ram64k(plus_mode ? !plus_ram_128k : (model != 2'd0))`.

4. **Testbench Additions**:
   - `t05i` in `sim/plus/asic_video_test.cpp`: Verifies ASIC 12-bit palette driving RGB pins, border from palette index 16, sprite priority over palette, and live palette rewrites.
   - `t05j` in `sim/plus/asic_video_test.cpp`: Verifies registered palette timing with alternating pen lookups on successive dots under 1-in-4 `PIXEN` dot rate.
   - `m12` in `sim/plus/p1_mobo_bench_test.cpp`: Verifies end-to-end 12-bit ASIC palette border rendering on top-level RGB pins.

---

## 2. Review Findings & Audit

### A. RTL Correctness & Address Decodes
- **FDC & Motor**: The address bit requirements for $A_{10}=0$, $A_9=1$, $A_8=1$, and $A_4=1$ accurately isolate `&FB7E`/`&FBDF` for u765 and `&FA7E`/`&FADD` for motor while cleanly rejecting all overlapping aliases for Kempston mouse and PlayCity.
- **Palette Pipeline**: The `{PAL_RGB[7:4], PAL_RGB[11:8], PAL_RGB[3:0]}` channel permutation correctly maps `{G,R,B}` storage to the `{R,G,B}` output pins. Precedence remains: HSYNC blank > Border > Sprites > Screen ink.
- **Bank Muxing**: `mem_bank` prevents split-brain bank selection where the display port reads bank 0 while the CPU writes bank 2.

### B. Invariance in Classic Mode
- When `plus_mode == 0`:
  - `mem_bank` resolves directly to `model`.
  - `.ram64k` evaluates to `(model != 2'd0)`.
  - FDC decode operates identically with added protection against non-FDC port collisions.
  - Motherboard classic video path remains driven by netlist Gate Array (`ga_red`, `ga_green`, `ga_blue`) and classic CRTC.

### C. Gate Results
- `make -C sim`: 172 required CRTC passes, 56 `asic_video` tests, and all Plus test suites pass with 0 errors.
- `make -C sim lint`: Elaboration and lint pass with 0 errors.

---

## 3. Review Debt Statement
Per repository policy ([`AGENTS.md:189-190`](file:///Users/renaudg/code/Amstrad_MiSTer-plus/AGENTS.md#L189-L190)), because third-party provider quota was exhausted prior to finalizing the external review thread, this implementation is self-reviewed and an open debt row is recorded in [`docs/review-debt.md`](file:///Users/renaudg/code/Amstrad_MiSTer-plus/docs/review-debt.md).
