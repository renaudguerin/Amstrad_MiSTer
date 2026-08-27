# Phase P8 (Platform Polish & SNA v3 CPC+ Chunk Support) Independent Cross-Provider Review

**Branch**: `plus/p8-polish-sna`  
**Initial Reviewed Tip**: commit `227a098`  
**Reviewer Role**: Independent Cross-Provider Reviewer (Claude / Codex / Gemini protocol)  
**Final Verdict**: **CLEAR**

---

## Executive Summary

Phase P8 implements the platform polish and SNA v3 snapshot loading features for the Amstrad Plus / GX4000 ASIC work stream:
1. **ASIC PPI 8255 Quirks (`rtl/i8255.v`, `rtl/Amstrad_motherboard.v`)**:
   - Port B is input-only when `plus_mode = 1` (reads return `ipb`, writes ignored).
   - Port C is output-only when `plus_mode = 1` (reads return output latch `opc_r`).
   - Control word rewrites (`idata[7] == 1`): when `plus_mode = 1`, output latches `opa_r, opb_r, opc_r` are preserved without clearing to 0.
   - Classic mode (`plus_mode = 0`) remains unaffected and clears latches as expected.
2. **ADC Paddle Registers (`rtl/plus/asic_regs.v`)**:
   - Read decode for `&6808-&680F` returns default values `{3F, 3F, 3F, 3F, 3F, 00, 3F, 00}`.
   - Open-bus neutrality is preserved over remaining unmapped regions.
3. **Grayscale / Monochrome Luma Weighting (`Amstrad.sv`)**:
   - Plus mode luma weighting formula: `px_plus = R*59 + G*177 + B*20` (`G:R:B = 9:3:1`).
   - Mapped to OSD monitor color, green, amber, cyan, and monochrome display modes.
4. **SNA v3 "CPC+" Chunk Parser & Loader (`rtl/plus/plus_sna_parser.v`, `rtl/plus/asic_regs.v`, `rtl/plus/plus_mmu.v`, `rtl/plus/asic_unlock.v`, `Amstrad.sv`)**:
   - Implements full 2296-byte chunk unpacking:
     - `0x000-0x7FF`: Sprite RAM nibbles unpacked to `&4000-&4FFF`.
     - `0x800-0x87F`: 16 Sprite Attributes to `&6000-&607F`.
     - `0x880-0x8BF`: 32 Palette entries to `&6400-&643F`.
     - `0x8C0-0x8C5`: Control registers (PRI, SPLT, SSA, SSCR, IVR) to `&6800-&6805`.
     - `0x8D0-0x8DF`: Sound DMA channel registers to `&6C00-&6C0F`.
     - `0x8F5`: Gate Array RMR2 register restored into `plus_mmu`.
     - `0x8F6`: ASIC lock status restored into `asic_unlock`.
5. **Unit & Integration Tests**:
   - `sim/plus/plus_p8_test.cpp`: PPI 8255 quirks, SNA v3 chunk parser unpacking, and model capability decodes.
   - `sim/plus/asic_regs_test.cpp`: `a11` (ADC paddles) and `a12` (direct SNA loading).

---

## Verification Summary

- `make -C sim`: All classic CRTC tests, Plus unit testbenches (including `plus_p8_tests` and `asic_regs_tests`), and motherboard integration tests pass.
- `make -C sim lint`: Verilator lint clean across classic and Plus hierarchies.
- `make -C sim soak`: Golden soak hash `0x85b3f8e847430495` bit-identical.
- **Review Verdict**: **CLEAR**.
