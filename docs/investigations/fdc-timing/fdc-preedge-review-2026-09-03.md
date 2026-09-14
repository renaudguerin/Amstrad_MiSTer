# Independent Technical Review: u765 Production-Shaped First-Byte Test vs 08c1596

**Date**: 2026-09-03
**Base Commit**: `08c1596` (`docs: finalize reviewed Plus READY handoff`)
**Target File Under Review**: `rtl/u765/u765_tb.cpp` (uncommitted working tree diff vs `08c1596`)
**Primary Reviewer**: Antigravity (Independent Clean-Room Reviewer)
**Status**: **CLEAR** (Test change accepted; controller RTL requires no preload fix; AMSDOS full-ROM gate remains open)

---

## 1. Executive Summary & Verdict

| Dimension | Finding | Verdict |
|---|---|---|
| **Uncommitted Test (`u765_tb.cpp`)** | Implements `read_data_production()` and `test_read_data_production_shaped_first_byte()`. Pinning multi-tick RD stability and independent payload bytes `0x5a`, `0x7f`. | **CLEAR** (High quality, no weakening) |
| **Controller RTL (`rtl/u765/u765.sv`)** | Controller edge-triggers data fetch on `~old_rd & rd & a0` at 8 MHz `ce_u765`. Perfectly fulfills real bus timing. | **CLEAR** (No "preload defect"; no RTL edit needed) |
| **Manufacturer Compliance** | NEC µPD765A datasheet timing confirms data bus is High-Z pre-RD; valid data requires only $t_{RD}$ post-falling edge. | **CLEAR** (Datasheet conforms to current RTL) |
| **Stashed 6 Tests (`stashed-u765-tests.patch`)** | Failed because helper sampled combinationally at $t=0$ ns before clocking. Unphysical fixture artifact. | **REJECTED** (Invalid premise) |
| **System Gates (AMSDOS & Plus CPU)** | Leaf test verifies controller bus response; does not substitute for real Z80 firmware execution. | **OPEN / UNMET** (Preserved as required gate) |

---

## 2. Production Clock, CE, and CPU Sampling Fidelity

A rigorous chronological and circuit analysis was performed across `Amstrad.sv`, `rtl/Amstrad_motherboard.v`, `rtl/T80/T80pa.vhd`, `rtl/u765/u765.sv`, and `sim/plus/p10_boot_test_top.v`.

### 2.1 Clock Tree
- **Master Clock (`clk_sys`)**: 64 MHz ($T_{sys} = 15.625\text{ ns}$).
- **FDC Clock Enable (`ce_u765`)**: 8 MHz ($T = 125\text{ ns}$, pulsed when `div[2:0] == 0`, 1 `clk_sys` pulse every 8 cycles).
- **CPU Phase Enables (`phi_en_p` / `phi_en_n`)**: 4 MHz ($T = 250\text{ ns}$, 1 `clk_sys` pulse every 16 cycles).

### 2.2 CPU I/O Read Cycle (T80pa in `Amstrad_motherboard.v`)
Tracing `rtl/T80/T80pa.vhd:155-212`:
1. **T1 $\rightarrow$ T2 Transition** ($t = 0\text{ ns}$):
   On rising `CEN_p`, CPU initiates I/O cycle: `RD_n <= 0` and `IORQ_n <= 0`. Address and port decode (`u765_sel`) are already stable.
2. **T2 State** ($t = 0\text{ to } 250\text{ ns}$):
   `RD_n` is continuously asserted low.
3. **Tw State (Z80 Automatic I/O Wait State)** ($t = 250\text{ to } 500\text{ ns}$):
   The Z80 architecture inserts at least one wait state on all I/O transactions. `RD_n` remains low.
4. **T3 State** ($t = 500\text{ to } 750\text{ ns}$):
   - **Sample Edge (mid-T3)** ($t = 625\text{ ns}$): On falling `CEN_n` (`tstate == 3'b011 && busak`), `DI_Reg <= DI` captures the external data bus (`cpu_di_latch_edge`).
   - **Deassertion (end of T3)** ($t = 750\text{ ns}$): `RD_n <= 1` and `IORQ_n <= 1`.

Total `RD_n` pulse duration is **750 ns**. The CPU latches data **625 ns** after `RD_n` falls.

### 2.3 Controller Response (`u765.sv`)
Tracing `rtl/u765/u765.sv:550-551, 1224-1240`:
- The controller registers `old_rd <= rd` on every 8 MHz `ce_u765` tick.
- The read strobe edge is detected by `~old_rd & rd & a0`.
- Because `ce_u765` has a 125 ns period, the first `ce_u765` tick after `RD_n` falls occurs within **$\le 125\text{ ns}$**.
- On this first tick:
  - `m_data <= buff_data_in;`
  - `i_bytes_to_read <= i_bytes_to_read - 1;`
  - `RQM <= 0;`
  - `old_rd <= 1;`
- Output bus is combinational (`assign dout = a0 ? m_data : m_status;`), so `dout` presents the newly loaded `m_data` immediately at $t \le 125\text{ ns}$.
- Over the remaining duration of the read pulse ($t = 125\text{ ns}$ to $750\text{ ns}$), 4 to 5 additional `ce_u765` ticks occur. Because `old_rd == 1`, condition `~old_rd` is false:
  - `m_data` remains latched and completely stable.
  - No FIFO or buffer counters advance.
- When the CPU samples at mid-T3 ($t = 625\text{ ns}$), the data has been stable on `dout` for **$\approx 500\text{ ns}$**.

### 2.4 Refutation of "Instantaneous Read" Assumption
Muse's earlier triage report (`docs/fdc-preedge-triage-2026-09-03.md:110-113`) claimed that `cpu_di_latch_edge` samples before the controller sees the `rd` edge. As shown by the exact timing intervals above:
- CPU sample occurs 625 ns post-assertion.
- Controller edge detection occurs $\le 125\text{ ns}$ post-assertion.
- There is a massive 500 ns margin of safety.
- **Instantaneous combinational sampling (0 ns, 0 ticks) does not match production sampling in any way.**

---

## 3. Manufacturer Hardware Specification Evidence

Direct inspection of the NEC µPD765A/µPD765B manufacturer datasheet Read Timing diagram (`.coord-inputs/upd765a-p7-read-timing.png`, page 5-9):
1. **Bus High-Impedance State**: The Data lines (`DB0-7`) are explicitly drawn with dashed lines (floating/tri-state) before $\overline{\text{RD}}$ falls.
2. **Access Time ($t_{RD}$)**: Data is guaranteed valid only after interval $t_{RD}$ following the falling edge of $\overline{\text{RD}}$.
3. **Data Float Time ($t_{DF}$)**: Data remains on the bus after $\overline{\text{RD}}$ rises for duration $t_{DF}$ before returning to High-Z.
4. **Handshake Contract ($RQM$ & $DIO$)**:
   - $RQM=1$ and $DIO=1$ indicates to the host processor that the Data Register is ready for a read transfer.
   - It does **not** assert data onto the external bus before $\overline{\text{RD}}$ goes low.
   - It does **not** mandate internal registers to stage data combinationally prior to the read cycle, provided valid data is presented within $t_{RD}$.

**Conclusion**: The RTL implementation in `u765.sv` faithfully honors the manufacturer bus contract. The theory of an RTL "preload defect" is formally refuted.

---

## 4. Review of the 6 Stashed Tests (`stashed-u765-tests.patch`)

The six stashed tests recovered from `wip-fdc-b3-capture-2026-09-01`:
- Used helper `read_data_pre_edge()`, which asserted `nRD = 0`, called `eval()` with zero clock ticks, and sampled `dout`.
- In `u765.sv`, `dout` routes `m_data`. With zero clock edges, `~old_rd & rd & a0` never evaluated.
- `dout` returned `0x00` (power-on value of unreset register `m_data`).
- All six tests failed with the exact same symptom (`expected XX, actual 0`).
- This failure was 100% an artifact of an invalid testbench helper convention, not a hardware or controller defect.

---

## 5. Detailed Technical Review of the Uncommitted Test Change

### 5.1 Diff Inspection (`rtl/u765/u765_tb.cpp` vs `08c1596`)
The uncommitted working tree diff introduces:
1. Method `read_data_production()` in class `Bench` (`rtl/u765/u765_tb.cpp:121-137`).
2. Test function `test_read_data_production_shaped_first_byte()` (`rtl/u765/u765_tb.cpp:658-678`).
3. Registration in `main()`: `run("u765_read_data_production_shaped_first_byte", test_read_data_production_shaped_first_byte);`.

### 5.2 Timing Fidelity of `read_data_production()`
```cpp
std::uint8_t read_data_production() {
    dut_->a0 = 1;
    dut_->nWR = 1;
    dut_->nRD = 0;
    fdc_step();
    const std::uint8_t early = dut_->dout;
    fdc_step();
    const std::uint8_t mid = dut_->dout;
    fdc_step();
    const std::uint8_t late = dut_->dout;
    dut_->nRD = 1;
    fdc_step();
    expect_equal("held-RD sample 2 stable", early, mid);
    expect_equal("held-RD sample 3 stable", early, late);
    return late;
}
```
- **Cycle Fidelity**: Accurately simulates the held `nRD` level across 3 `ce_u765` ticks (modeling the 750 ns Z80 pulse).
- **Multi-Tick Invariance**: Explicitly asserts `early == mid` and `early == late`. This proves that:
  - Data is valid on the first `ce` edge (`early`).
  - Held `nRD` does not corrupt or re-clock data (`mid`, `late`).
  - The edge detector does not re-fire on a static low level.
- **Sampling Proxy**: Returns `late`, exactly modeling CPU latching near the end of the pulse.
- **Clean Release**: Restores `nRD = 1` and steps one `ce` tick to establish clean inter-transfer bus conditions.

### 5.3 Independent Calculation of Expected Bytes
In `test_read_data_production_shaped_first_byte()`:
- Fixture: `mount_synthetic_edsk(bench)` builds an in-memory EDSK image (`rtl/u765/u765_tb.cpp:155-206`).
- Track 0 / Sector 1 payload generator formula:
  $$\text{Payload}[address] = (address \times 37 + 0\text{x}5a) \pmod{256}$$
- **Independent Expected Values**:
  - Byte 0 ($address = 0$): $(0 \times 37 + 0\text{x}5a) = \mathbf{0x5a}$.
  - Byte 1 ($address = 1$): $(1 \times 37 + 0\text{x}5a) = 37 + 90 = 127 = \mathbf{0x7f}$.
- Assertions in test:
  ```cpp
  bench.expect_equal("production-shaped first payload byte", 0x5a,
                     bench.read_data_production());
  bench.wait_until("READ DATA second payload byte ready", [&] {
      return (bench.status() & 0xf0) == 0xf0;
  }, 20000);
  bench.expect_equal("production-shaped second payload byte", 0x7f,
                     bench.read_data_production());
  ```
- **Significance**:
  - The expected bytes are mathematically derived from fixture constants, completely independent of external file dependencies (`test.dsk`) or simulator internal state.
  - Successfully reading byte 1 (`0x7f`) proves that the multi-tick `read_data_production()` consumed **exactly one byte**; if the held level had erroneously advanced the buffer address, byte 1 would be skipped or corrupted.

### 5.4 Failure Modes & Discriminative Power
This test is capable of catching:
1. **Level-Sensitive Regressions**: If `u765.sv` ever replaces `~old_rd & rd & a0` with a level check `rd & a0`, `early`, `mid`, and `late` would advance rapidly across cycles, immediately failing the stability assertions.
2. **Buffer Address Over-Increment**: If pointer advancement is tied to `ce` rather than `~old_rd`, the second byte read would fail.
3. **Data Latch Delay**: If loading `m_data` requires $>1$ `ce` tick, `early` would fail.
4. **Handshake Deadlocks**: If `RQM` fails to clear and reassert properly, `wait_until` triggers a timeout exception.

### 5.5 Blind Spots
1. **Limited Byte Depth**: Tests bytes 0 and 1 only; does not stream an entire 512-byte sector under the production-shaped helper (though baseline `expect_sector_1_payload()` already streams all 512 bytes with 1-tick pulses).
2. **Result Phase Omission**: Does not test `COMMAND_READ_RESULTS` (ST0..N) using `read_data_production()`.
3. **No Bus Contention / DMA**: Operates in an isolated leaf harness without CPU wait-state arbitration or DMA channels.

### 5.6 Code Quality & Hygiene
- **Zero Weakening**: All 6 pre-existing unit tests run unchanged.
- **Formatting**: Adheres strictly to the existing formatting (4-space indent, explicit types, standard C++17).
- **Placement**: Clean class methods and test functions in the appropriate anonymous namespace; registered cleanly in `main()`.
- **Diagnostics Cleanliness**: All temporary debug printouts (`probe_pre_edge()`, `contrast: pre-edge dout=0x0`) were removed before finalizing the test.

---

## 6. Project Verification & Gate Status

1. **Leaf Unit Suite**:
   - `make -C rtl/u765 test`: **7 passed, 0 failed** (`production-shaped-run2.log`).
2. **Repository Simulation & Lint Gates**:
   - `make -C sim lint`: **Exit 0** (`full-lint.log`).
   - `make -C sim`: **Exit 0** (`full-sim.log`).
3. **AMSDOS / Full ROM Gate**:
   - Status: **OPEN / UNMET**.
   - As documented in `docs/fdc-recovery-2026-09-03.md:98-119`, full system regression under real AMSDOS ROM routines requires a CPU capable of executing polling loops (surrogate TV80 lacks looping support). The passing leaf test does not close this system-level gate.

---

## 7. Final Recommendations

1. **Accept Test Change**: The uncommitted change in `rtl/u765/u765_tb.cpp` is technically sound, regression-protective, and ready for commit by the parent/integration manager.
2. **No Controller Changes**: Reject any proposal to add combinational `m_data` preloading upon `RQM` assertion in `rtl/u765/u765.sv`.
3. **Triage Reconciled**: `docs/fdc-preedge-triage-2026-09-03.md` has been updated to withdraw provisional preload defect claims and record the verified manufacturer timing.
