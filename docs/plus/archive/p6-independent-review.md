# Independent Cross-Provider Review: Phase P6 (Screen Split & Hardware Soft Scroll)

**Date:** 2026-08-26  
**Commit under review:** `3405d0c` on branch `plus/p6-split-scroll` (diffing against `plus/p5-crtc3-bus`)  
**Scope:** Screen Split (`&6801` SPLT, `&6802`/`&6803` SSA), Soft Scroll (`&6804` SSCR), Motherboard integration, Unit/Integration tests (`t08a`–`t08h`, `m10`), and Two Streams discipline.

---

## Executive Summary & Final Verdict

### **Verdict: CLEAR**

The Phase P6 implementation cleanly and faithfully realizes the Amstrad Plus ASIC screen split (`SPLT`/`SSA`) and soft scroll (`SSCR`) facilities per the official Arnold V specification (`_Arnold V_ Specification - Issue 1.5`), `docs/plus/references/asic-reference.md` §8, and CRTC Compendium (ACCC) v1.10 §20.5. All timing and sequencing requirements, counter tap decoupling, border masking, sprite priority preservation, motherboard wiring, and regression tests are implemented with high precision.

---

## Detailed Technical Review

### 1. Screen Split (&6801 SPLT, &6802/&6803 SSA)
- **Match Condition (`rtl/plus/asic_video.v:353`):**
  - `wire split_match = (SPLT != 8'd0) && ({charline[4:0], raster[2:0]} == SPLT);`
  - Correctly evaluates `{VC4..0, RC2..0} == SPLT` with `SPLT != 0` gating (SPLT=0 disables split). Correctly models the 8-bit wrap behavior where VC wraps at 32 rows (256 scanlines).
- **Latch Event (`rtl/plus/asic_video.v:354, 362-365`):**
  - `wire split_latch_event = CLKEN && !in_adj && split_match && (hcc == R1_h_displayed);`
  - Fires at `HCC == R1` on the programmed line and latches `vma_latch <= SSA`.
- **VMA & `vma_latch` Sequencing & Row Advance (`rtl/plus/asic_video.v:361-388`):**
  - At subsequent line starts (`hcc_last`), `vma` reloads from `vma_latch` (`SSA`).
  - At the end of that character row (`raster == R9 && hcc == R1`), `row_latch_event` advances `vma_latch <= vma` (which is `SSA + R1`), so subsequent rows naturally advance from the new SSA base.
  - When a split falls on a character row boundary (`raster == R9`), `split_latch_event` takes precedence over `row_latch_event` in `vma_latch` update, cleanly retargeting the next row.
- **Frame-Start Reload Precedence (`rtl/plus/asic_video.v:372-375`):**
  - `if (!adj_n && (charline_n == 7'd0))` has highest priority at `hcc_last`, ensuring `vma` and `vma_latch` reload from `{R12, R13}` on line 0 of the frame regardless of split state.
- **14-bit Overscan Carry Across 16K Boundaries (`rtl/plus/asic_video.v:344, 386`):**
  - `vma` and `vma_latch` are full 14-bit registers (`[13:0]`); `vma <= vma + 14'd1` maintains linear 14-bit counting without 10-bit or 12-bit truncation per ACCC §20.5.

### 2. Soft Scroll (&6804 SSCR)
- **Vertical Scanline Offset (`rtl/plus/asic_video.v:566`):**
  - `assign RA = {raster[4:3], (raster[2:0] + SSCR[6:4]) & 3'd7};`
  - Adds `SSCR[6:4]` modulo 8 to `raster[2:0]` while preserving `raster[4:3]`.
  - The internal counter `raster` remains unaffected, ensuring CRTC3 row completions, adjustment triggers, PRI, and split comparisons are undisturbed.
- **Horizontal Pixel Shift Delay (`rtl/plus/asic_video.v:833-848`):**
  - Implements a 15-tap shift register `pen_delay[0:14]` clocked on `PIXEN` (16 MHz dot clock).
  - `wire [3:0] pen_delayed = (SSCR[3:0] == 4'd0) ? pen_nib : pen_delay[SSCR[3:0] - 4'd1];` provides exact 0–15 mode-2 pixel (0–15 dot) delays.
- **Border Masking (`SSCR[7]`, `rtl/plus/asic_video.v:850-861`):**
  - `de_first_char` tracks the first 16 dots (character 0) of active display (`(!de_hold && DE) || (hcc_last && DE)`).
  - `wire eff_de = de_hold & ~(SSCR[7] & de_first_char);`
  - Masks decoded screen ink to `BORDER_I` during the first 16 dots of active display when `SSCR[7] == 1`.
- **Sprite Invariance (`rtl/plus/asic_video.v:871, 882-888`):**
  - `wire show_spr = de_hold & SPR_EN;` uses `de_hold` rather than `eff_de`.
  - Sprites display over the `SSCR[7]`-masked border area inside `de_hold`, while natural border outside `de_hold` beats sprites and `blank` (HSYNC) blanks all, matching `../references/asic-reference.md` §5 priority rules.

### 3. Motherboard Wiring & Integration
- **`rtl/Amstrad_motherboard.v:372-374, 397-400, 425-426`:**
  - Wires `asic_splt` [7:0], `asic_ssa_hi` [7:0], `asic_ssa_lo` [7:0], `asic_sscr` [7:0] between `asic_regs` (`asic_page`) and `asic_video` (`asic_vid`).
  - `.SSA({asic_ssa_hi[5:0], asic_ssa_lo[7:0]})` correctly maps the 14-bit address space.
  - `asic_regs.v` decodes `&6801` (SPLT), `&6802` (SSA_HI), `&6803` (SSA_LO), `&6804` (SSCR) under `r_raster` (`A[13:8] == 6'h28`), with POR=0 on SPLT and SSCR.

### 4. Unit and Integration Tests
- **`sim/plus/asic_video_test.cpp` (`t08a`–`t08h`):**
  - `t08a`: Screen split capture at HCC==R1 on SPLT line, line reload from SSA, subsequent row advance from SSA base, frame-start restoration.
  - `t08b`: SPLT=0 turns off split screen facility.
  - `t08c`: Multiple splits per frame via mid-frame reprogramming.
  - `t08d`: Split on row boundary (`raster == R9`) overriding normal row capture.
  - `t08e`: `SSCR[6:4]` vertical scanline offset added to `RA[2:0]` with internal `ROW` counter untouched.
  - `t08f`: `SSCR[3:0]` horizontal pixel shift delay (0 and 4 dot shifts).
  - `t08g`: `SSCR[7]` border mask on dots 0..15 with sprite pixel visibility over masked border.
  - `t08h`: 14-bit VMA overscan carry across 10-bit and 12-bit boundaries.
- **`sim/plus/p1_mobo_bench_test.cpp` (`m10`):**
  - Z80 bus test via `t80pa_bench_cpu.v` writes SPLT=0x05, SSA_HI=0x24, SSA_LO=0x00, SSCR=0x34.
  - Confirms register writes reach ASIC page and `plus_ra` reflects the SSCR vertical scanline offset.

### 5. Two Streams Discipline
- Classic CPC mode (`plus_mode = 0`) remains strictly untouched and independent in `rtl/CRTC.v`, `rtl/crtc_type0_engine.v`, and `rtl/crtc_type1_engine.v`.
- The golden soak hash (`0x85b3f8e847430495`) remains intact and undisturbed.

---

## Observations (Non-Blocking)
1. **VC truncation in split comparison (`asic_video.v:353`):** Using `{charline[4:0], raster[2:0]}` compares the lower 5 bits of `charline`, faithfully reproducing the documented 8-bit wrap at line 256 on a 312-line frame.
2. **SSA power-on reset state:** `ssa_hi_r` and `ssa_lo_r` retain POR=N (uninitialized / retention), while `splt_r` and `sscr_r` reset to 0, directly following Arnold V spec §3.
3. **RA modulo 8 wrapping:** Modulo 8 addition on `RA[2:0]` matches hardware behavior for standard character heights (`R9 >= 7`).

---

## Conclusion
Phase P6 meets all architectural and accuracy gates. Ready for integration merge.
