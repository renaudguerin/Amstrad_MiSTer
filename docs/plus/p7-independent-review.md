# Phase P7 (3-Channel DMA Sound Engine) Independent Cross-Provider Review

**Branch**: `plus/p7-dma-sound`  
**Initial Reviewed Tip**: commit `7b316d4`  
**Reviewer Role**: Independent Cross-Provider Reviewer (Claude / Codex / Gemini protocol)  
**Final Verdict**: **CLEAR** (after remediations)

---

## Executive Summary

Phase P7 implements the Amstrad Plus / GX4000 3-Channel DMA Sound Engine (`rtl/plus/asic_dma.v`), address and control register storage (`&6C00-&6C0F`) in `rtl/plus/asic_regs.v`, bus multiplexing for VRAM fetch and PSG audio in `rtl/Amstrad_motherboard.v`, unit tests `sim/plus/asic_dma_test.cpp` (d01–d10), and motherboard integration test `m11` in `sim/plus/p1_mobo_bench_test.cpp`.

Initial review identified 3 blocking issues. All 3 have been fully remediated and verified.

---

## Review Findings and Remediations

### Finding 1 (BLOCKING): Stale SAR Register Sampling on CPU Writes
- **Initial Problem**: `sar0/1/2_wr` pulsed during CPU write cycles, but `sar_cur[0..2]` sampled `sar0/1/2_hi/lo` registers on the same clock edge that they were being written to, causing `sar_cur` to sample stale pre-edge flop data on 8-bit sequential writes.
- **Remediation**: In `rtl/plus/asic_regs.v`, combinationally resolved `next_sar0/1/2_lo/hi` wires drive the outputs so that `sar_cur` immediately captures the updated byte value when `sar_wr` strobes. Added unit test `d10` in `sim/plus/asic_dma_test.cpp` and asserted `sar_cur` tracking in mobo bench `m11`.

### Finding 2 (BLOCKING): DMA Interrupts Disconnected from CPU `/INT` Pin
- **Initial Problem**: DMA `INT` instructions set `dcsr_flags`, but `asic_regs` had no interrupt line wired to `Amstrad_motherboard.v`'s `plus_int_n`, so the CPU was never interrupted by DMA.
- **Remediation**: Added `dma_int_req = |dcsr_flags` to `asic_regs.v`, routed to `Amstrad_motherboard.v` to merge into `plus_int_n = plus_ga_int_n & ~plus_dma_int_req`, while preserving raster interrupt pending priority in `asic_regs.v`.

### Finding 3 (BLOCKING): DCSR Bits 6 and 4 Swapped on Read and W1C
- **Initial Problem**: DCSR bit 6 is Channel 0 INT flag and bit 4 is Channel 2 INT flag per specification, but `asic_regs.v` mapped `dcsr_flags[2]` to bit 6 and `dcsr_flags[0]` to bit 4 on read and write-1-to-clear.
- **Remediation**: Updated DCSR read mux to `{dcsr_stat | intack_raster, dcsr_flags[0], dcsr_flags[1], dcsr_flags[2], 1'b0, dcsr_ena}` and added `dcsr_w1c_mask = {D_in[4], D_in[5], D_in[6]}` for W1C writes. Verified in unit test `a05` in `sim/plus/asic_regs_test.cpp`.

---

## Verification Summary

- `make -C sim`: 10/10 `asic_dma` unit tests pass, `asic_regs` tests pass, motherboard bench `m11` passes.
- `make -C sim lint`: Verilator lint clean across classic and Plus hierarchies.
- `make -C sim soak`: Golden soak hash `0x85b3f8e847430495` bit-identical.
