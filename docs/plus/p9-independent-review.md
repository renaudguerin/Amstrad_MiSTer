# Phase P9 (Cartridge Boot, CPR Parser Tolerances, and MMU ROM Gating) Independent Cross-Provider Review

**Branch**: `plus/p9-cartridge-boot`  
**Base**: `accc-review-and-fixes` (commit `005d412`)  
**Reviewer Role**: Independent Cross-Provider Reviewer (Claude / Codex / Gemini protocol)  
**Final Verdict**: **CLEAR**

---

## Executive Summary

Phase P9 resolves the cartridge booting failure observed during hardware testing and adds robust CPR format compatibility and MMU Gate Array ROM enable tracking:

1. **CPR Parser Case-Insensitivity (`rtl/plus/plus_cpr_parser.v`)**:
   - Accepts both `AMS!` (uppercase standard in real hardware dumps and toolings like `crtc3_v2fix.cpr`) and `Ams!` form-types.
   - Accepts uppercase `CB` as well as lowercase `cb` chunk ID prefixes (`CB00`..`CB31`, `cb00`..`cb31`).
2. **Top-Level CPR Auto-Reset Sequencing (`Amstrad.sv`)**:
   - Separates base reset (`reset_base`) from system reset (`reset`).
   - Feeds `reset_base` to `plus_cpr_parser` and `plus_cartridge_memory` to prevent loader self-abortion during active downloads.
   - Holds system `reset` asserted during `cpr_download`.
   - On download end, waits for `cart_service_busy` to drop (atomic commit), then issues an extended `cpr_apply_cnt` reset pulse so the Z80 automatically restarts from `0x0000` into cartridge page 0.
3. **Plus MMU ROM-Enable Decoding & Bank Relocation (`rtl/plus/plus_mmu.v`)**:
   - Tracks Gate Array MRER register writes (`!A[15] && A[14] && D[7:5] == 3'b100`) for Lower ROM (`D[2]`) and Upper ROM (`D[3]`) disable/enable flags.
   - Correctly maps Lower ROM when relocated by RMR2 to positions `01` (`&4000`) and `10` (`&8000`) independent of classic Gate Array decode restrictions.
4. **Comprehensive Test Suite & Verification**:
   - `sim/plus/plus_cpr_parser_test.cpp`: Adds test vectors validating `AMS!`, `Ams!`, `ams!`, and `CB00` chunks.
   - `sim/plus/plus_mmu_test.cpp`: Validates MRER lower/upper ROM enable/disable gating and relocated low ROM reads.
   - `sim/plus/p0_boot_test.cpp`: Validates CPR download with `AMS!` / `Ams!`, auto-reset pulse, and cartridge boot readback at `&0000`.

---

## Verification Summary

- `make -C sim`: All classic CRTC tests, Plus unit testbenches, and integration tests PASS.
- `make -C sim lint`: Verilator lint 100% clean across all modules.
- `make -C sim soak`: Golden soak hash `0x48146d2b681268ab` bit-identical.
- **Review Verdict**: **CLEAR**.
