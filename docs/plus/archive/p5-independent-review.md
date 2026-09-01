# Independent review — P5 CRTC-3 bus semantics (`plus/p5-crtc3-bus`)

Reviewer: Claude Opus 5 high (`ask-claude review --effort high`, fresh session).
Date: 2026-08-26.
Range reviewed: `27cb993..8523136` (P5 code commits `3891213` and `8523136` plus uncommitted documentation diff).

---

## Initial verdict

**NOT CLEAR — two blocking findings (B1, B2), five non-blocking findings (N1–N5).**

All findings were verified and remediated in the follow-up commit series before merge.

---

## Findings and Remediation

### Blocking findings

#### B1. STATUS 1 bit 4 off by one, plus an unsourced 0⇒16 substitution (`rtl/plus/asic_video.v:600-602`)
- **Review finding:** ACCC v1.10 §21.3.4.1 p.248 specifies **bit 4 = 0 at `C0=R2+R3`**. The initial code computed `R2 + (R3l==0 ? 16 : R3l) - 1` (an off-by-one `-1` and an unsourced 0⇒16 substitution that applies to pulse width rather than this comparator). Furthermore, the vector `t07b` encoded the initial implementation (`0xEE` at C0=6, `0xFF` at C0=7) instead of the paper-derived values (`0xFE` at C0=6, `0xEF` at C0=7).
- **Remediation:** In `rtl/plus/asic_video.v`, replaced with the literal 9-bit sum `status1_hsync_end = {1'b0, R2_h_sync_pos} + {5'd0, R3_h_sync_width}; wire s1_bit4 = ~(hcc == status1_hsync_end);`. In `sim/plus/asic_video_test.cpp`, updated `t07b` to assert the exact ACCC-derived values: `0xFE` at C0=6 (before R2+R3) and `0xEF` at C0=7 (where C0=R0 sets bit 0 and C0=R2+R3 clears bit 4).

#### B2. IN-performs-write trap wrote T80's stale internal `DO`, not the open-bus instruction byte (`rtl/Amstrad_motherboard.v`, `rtl/plus/plus_mmu.v`)
- **Review finding:** Per `docs/plus/references/asic-reference.md` §4 and [KT] Ports, reading a write-only I/O port returns the last byte of the instruction performing the read (the opcode fetch left on the open bus), and on Plus hardware this open-bus byte is written into the addressed register. T80's internal `DO` datapath holds stale values during `IN` cycles, which could lead to spurious mode changes, unlock progression, or RMR2 repaging.
- **Remediation:** 
  1. In `rtl/Amstrad_motherboard.v`, added `io_bus_byte` register latched from the CPU data bus during instruction opcode fetch cycles (`~M1_n & ~MREQ_n & ~RD_n`).
  2. Routed `plus_io_data = io_rd ? io_bus_byte : D` to `CRTC.DI`, `plus_mmu.D`, and `asic_ga.D` during Plus I/O operations.
  3. Exported `io_bus_byte` to `Amstrad.sv` top-level wiring.
  4. Updated testbench infrastructure (`sim/plus/p1_mobo_bench_top.v` and `sim/plus/t80pa_bench_cpu.v`) with explicit M1 opcode-fetch cycles before each write-side-effect `IN` transaction to seed `io_bus_byte` accurately.

---

### Non-blocking findings

- **N1. Pre-existing unlock bug resolved by `io_access_start` qualifier:** Confirmed that the `io_access_start` one-shot qualification in `plus_mmu` resolved the held-level issue where `sequence_index` advanced multiple times per cycle.
- **N2. `s2_bit0/1/2` literal counter compare during vertical adjustment:** ACCC specifies literal counter comparisons (`charline == R4`, `raster == R9`, `hcc == R0`), so these remain live during vertical adjustment. Clarified and documented as such in `rtl/plus/asic_video.v`.
- **N3. STATUS 1 bit 5 with R3h=0 sourced from [KT]:** ACCC documents value 1 over 15 lines but leaves the 16th unspecified; [KT]'s final-VSYNC-line rule supplies the 16th-line active-low assertion. Documented explicitly in `rtl/plus/asic_video.v` and `sim/plus/asic_video_test.cpp:t07d`.
- **N4. Light-pen wording correction:** Corrected durable wording in `rtl/plus/asic_video.v` and `docs/plus/references/asic-reference.md` to state "no light-pen strobe source is emulated" (since LPSTB/LPEN exists on expansion port pin 47).
- **N5. Review debt and documentation tracking:** Updated `docs/review-debt.md`, `docs/current-status.md`, and `docs/implementation-roadmap.md` with the full review record and remediation outcomes.

---

## Post-Remediation Gate Verification

All gates pass cleanly post-remediation:
1. `make -C sim`:
   - All 45 `asic_video` foundation tests (`t01a`–`t07g`) pass.
   - All Plus CPR parser, cartridge memory, MMU, P0 boot, GA diff, pixel phase, motherboard bench (`m1`–`m9`), register page, PRI, and sprite engine tests pass.
2. `make -C sim lint`:
   - Verilator lint passes clean with zero errors across all core modules and test fixtures.
3. `make -C sim soak`:
   - Golden soak hash matches `0x85b3f8e847430495` bit-identically.

---

## Residual risks and testing gaps

1. **Read sample point in held I/O window:** CRTC `DO` is combinational off live counters; no vector varies the CPU latching phase within a held I/O cycle that straddles a character boundary.
2. **Motherboard status reading depth:** `m9` proves read muxing, dual read ports, and basic status registers through the production bus; exhaustive intra-line status sweeps are verified at unit level (`t07a`–`t07g`).
