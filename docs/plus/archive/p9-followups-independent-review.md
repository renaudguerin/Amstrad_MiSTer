# Phase P9 Follow-ups (Review Items N3, N4, N5, OSD Grouping, Hardware Checklist) Independent Cross-Provider Review

**Branch**: `plus/p9-followups`  
**Reviewer Role**: Independent Cross-Provider Reviewers (Claude Code CLI / Opus & Codex CLI / GPT-5)  
**Date**: 2026-08-28  
**Final Verdict**: **CLEAR**

---

## Executive Summary

This review covers the implementation of review follow-up items N3, N4, and N5 from `docs/accuracy/archive/ox-alpha-items-opus-review.md`, OSD menu grouping of CPR cartridge loading, and the creation of the hardware testing checklist:

1. **N3: CPR Parser Upper-Bound RIFF Length Tests (`sim/plus/plus_cpr_parser_test.cpp`)**:
   - `riff_len_valid` bounds check in `plus_cpr_parser.v` rejects `full_riff_len > 32'h01FFFFF7`.
   - Upper boundary test vectors added: `0x01FFFFF8` asserts abort (bounds overflow guard), while `0x01FFFFF7` is verified to be accepted at the header stage.
   - **Verdict**: CLEAR. Arithmetic bounds verified.

2. **N4: Tightened Zero-Length Chunk Handling (`rtl/plus/plus_cpr_parser.v`, `sim/plus/plus_cpr_parser_test.cpp`)**:
   - `has_block` flag setting moved from `STATE_CHUNK_ID` to `STATE_CHUNK_DATA` when actual block data bytes are forwarded (`is_block && (chunk_bytes_read < 32'd16384)`).
   - Prevents zero-length `cb00` chunks from falsely committing an empty cartridge on download completion; isolated zero-length chunk correctly triggers `load_abort`.
   - Tested both standalone zero-length `cb00` (aborts cleanly) and zero-length `cb00` followed by valid non-empty `cb01` (commits page 1).
   - **Verdict**: CLEAR. Fail-closed parsing semantics maintained.

3. **N5: CI Compile-Log Guard for Implicit Net Warnings (`.github/workflows/build.yml`, `.github/workflows/local-build.yml`, `docs/ci-testing-policy.md`)**:
   - Post-compile step in both CI workflows checks `quartus-build.log` for Quartus Warning 10236 (`grep -n -E "(Implicit Net warning|Warning \(10236\))"`), failing synthesis immediately if undeclared wires are detected.
   - Verified that nominal builds emit zero matches for Warning 10236.
   - **Verdict**: CLEAR. Closes the regression hole that caused the previous classic-video black screen.

4. **OSD Menu Grouping (`Amstrad.sv`)**:
   - Grouped `F8,CPR,Load Plus cartridge;` and `OK,Tape sound` directly with DSK and CDT media loading entries in `CONF_STR`.
   - Verified that `ioctl_index` is preserved and status bits are unaffected.
   - **Verdict**: CLEAR.

5. **Hardware Test Checklist (`docs/plus/hardware-test-checklist.md`, `docs/current-status.md`)**:
   - Exhaustive physical testing checklist created for the upcoming MiSTer hardware test session covering:
     - Real `.cpr` titles (System Cartridge, Burnin' Rubber, RoboCop 2, Pang, Navy Seals, Klax, Switchblade, etc.).
     - Demos & diagnostics (`crtc3_v2fix.cpr`, PhX, Batman Forever, SHAKER).
     - Subsystem verification matrix (MMU RMR2 relocation, ASIC video/split/scroll, sprites, 3-channel DMA sound, PPI/ADC quirks, classic non-interference).
   - Codex review observations incorporated: refined unexpanded `/EXP=1` description and clarified MRER RAM pass-through pass criteria.
   - **Verdict**: CLEAR.

---

## Verification Summary

| Check | Result |
|---|---|
| `make -C sim` | **Pass** (All classic CRTC tests, Plus unit benches, and P0 boot integration pass clean) |
| `make -C sim lint` | **Pass** (Verilator lint 100% clean across classic and Plus hierarchies) |
| `make -C sim soak` | **Pass** (Golden soak hash `0x48146d2b681268ab` bit-identical) |
| Review Verdict (Claude Opus) | **CLEAR** |
| Review Verdict (Codex GPT-5) | **CLEAR** |
