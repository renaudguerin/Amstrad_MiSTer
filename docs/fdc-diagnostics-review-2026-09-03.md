Reviewer: Gemini 3.8 Flash high, independent of Muse Spark implementation.
Date: 2026-09-03. Review run: `20260903T070656Z-77333-e2a2`.

# Final Review: Bounded FDC Diagnostics & Recovery Report

**Reviewed Base**: [`22ad7660b1ed53b91a5c04628f45473dd6a7d230`](file:///Users/renaudg/code/Amstrad_MiSTer)  
**Reviewed Files**:
- [`sim/plus/p10_boot_test.cpp`](../sim/plus/p10_boot_test.cpp) (struct fields, print-only DIAG block, XFAIL printout)
- [`sim/plus/p10_boot_test_top.v`](../sim/plus/p10_boot_test_top.v) (passive observation taps: `dbg_cpu_di_m_data`, `dbg_cpu_di_buff_wait`, `dbg_cpu_di_bytes_left`)
- [`docs/fdc-recovery-2026-09-03.md`](../docs/fdc-recovery-2026-09-03.md) (recovery report)
- Logs: [`/private/tmp/amstrad-fdc-final-sim.log`](file:///private/tmp/amstrad-fdc-final-sim.log) and [`/private/tmp/amstrad-fdc-final-lint.log`](file:///private/tmp/amstrad-fdc-final-lint.log)

---

### Verdict: **CLEAR**

All actionable findings from the initial review have been resolved faithfully and accurately without introducing behavior changes, regressions, or weakening test assertions.

---

### Verification Checklist & Findings Resolution

1. **Statistical Explanation (Finding 1 Resolved)**:
   - [`docs/fdc-recovery-2026-09-03.md:62-70`](../docs/fdc-recovery-2026-09-03.md#L62-L70): The flawed extrapolation claiming deviations were uniformly `0x00` matching disk zeroes has been removed. The text accurately states that sector `0x41` contains only 26 zero bytes total, confirming that 124 matches cannot be zero-matching artifacts. It accurately attributes the initial 8 zero deviations to reads executing prior to `u765` completing sector search (`COMMAND_RW_DATA_EXEC3`, state 9).

2. **Result-Slot Interpretation & Label (Finding 2 Resolved)**:
   - [`sim/plus/p10_boot_test.cpp:874-876,925-932`](../sim/plus/p10_boot_test.cpp#L874-L932): The misleading `st1_overrun` label has been eliminated. The console output now prints raw result slots as `DIAG result-slots (raw, phase-unverified): 6f/20/20/20/20/20/61`.
   - [`docs/fdc-recovery-2026-09-03.md:71-78`](../docs/fdc-recovery-2026-09-03.md#L71-L78): Explicitly identifies the 7 bytes as unconsumed sector payload bytes 474–480 (`"o     a"`) read in state 13 before reaching `COMMAND_READ_RESULTS` (state 42). Overrun status is correctly documented as **UNKNOWN**, and no status bits are asserted against them.

3. **Command Exit Provenance (Residual 3 Resolved)**:
   - [`docs/fdc-recovery-2026-09-03.md:39-44,134-141`](../docs/fdc-recovery-2026-09-03.md#L39-L141): Pipelined scratch echoes (`| tail ...; echo "EXIT:$?"`) are explicitly rejected as valid exit evidence.
   - Verified direct parent rerun logs: [`/private/tmp/amstrad-fdc-final-sim.log`](file:///private/tmp/amstrad-fdc-final-sim.log) (all 192 CRTC, 67 `asic_video`, 17 `asic_sprites`, 14 `asic_dma`, P8/P10 boot, B7 dark silicon audit tests passed; exit 0) and [`/private/tmp/amstrad-fdc-final-lint.log`](file:///private/tmp/amstrad-fdc-final-lint.log) (clean lint; exit 0). No duplicate runs were initiated.

4. **Poll Histogram Accuracy**:
   - [`sim/plus/p10_boot_test.cpp:878-897`](../sim/plus/p10_boot_test.cpp#L878-L897) & [`docs/fdc-recovery-2026-09-03.md:55-61`](../docs/fdc-recovery-2026-09-03.md#L55-L61): Correctly accounts for all 1047 latches (519 data reads = 512 payload + 7 results; 519 × 2 + 9 command status reads). The single block with >1 poll (`max_polls_per_block=10`) reflects the initial command phase. All subsequent 518 data reads see exactly 1 preceding status poll, proving that poll loops never spun.

5. **Assertion Strength & RTL Taps**:
   - [`sim/plus/p10_boot_test.cpp:991-994`](../sim/plus/p10_boot_test.cpp#L991-L994): Assertions (`first_payload_mismatch != 512` and `first_payload_mismatch == 0`) remain byte-identical; XPASS still fails if full payload is consumed, and any shape change trips a failure.
   - [`sim/plus/p10_boot_test_top.v:124-126,619-621,636-638`](../sim/plus/p10_boot_test_top.v#L124-L638): Observation taps remain strictly passive registers sampled on `cpu_di_latch_edge` with zero feedback into DUT nets.

---

### Residual Boundaries

1. **Classic AMSDOS Full Command/Data Regression**:
   - **Boundary**: **Required and Unmet**. The P10 harness issues the 9 AMSDOS command bytes and consumes bytes via production decode, but executes a synthetic unrolled straight-line sequence rather than an AMSDOS ROM firmware boot. AMSDOS polling behavior cannot be claimed from this harness.
2. **Plus Payload Boundary**:
   - **Boundary**: Decode, motor enable, and SD sector request logic pass on the Plus path. Data payload transfer integrity and result-phase sequencing across the Plus bus remain open under the retained XFAIL signature.
3. **Production T80 vs. TV80 Surrogate Boundary**:
   - **Boundary**: The absence of polling loops is strictly a surrogate CPU defect ([`sim/plus/tv80/tv80_core.v:369`](../sim/plus/tv80/tv80_core.v#L369) leaves `jump_e` unconnected, so `JR` never jumps; [`sim/plus/tv80/tv80_mcode.v:865-879`](../sim/plus/tv80/tv80_mcode.v#L865-L879) forces unconditional `Jump_r`, so `JP cc` never falls through). The production `T80.vhd` core correctly executes both conditional branches; no controller RTL change is justified by TV80 surrogate limitations.
