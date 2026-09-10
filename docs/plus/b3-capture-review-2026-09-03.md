Reviewer: Gemini 3.8 Flash high, independent of Muse Spark implementation.
Date: 2026-09-03. Review run: `20260903T072746Z-88334-b69c`.
Reviewed base: `d3aabbca4f4bee8e7b1d928f1831b647e7e3685b`.

# Independent Review Report: Muse B3 Frame Capture & P10j Comments

**Verdict**: **CLEAR** (with 1 non-blocking test-tightening recommendation on symlink probe assertion)

---

## 1. Exact Base & Review Scope

- **Exact Base Commit**: [`d3aabbc`](file:///Users/renaudg/code/Amstrad_MiSTer-plus) (`test: preserve shared FDC first-divergence diagnostics`)
- **Active Branch**: `plus/b3-capture-recovery` (no other active writer)
- **Scoped Diff**:
  - [`sim/plus/p10_boot_test.cpp`](../../sim/plus/p10_boot_test.cpp): +1095 / −27 lines (static-CRTC CPR generator, video taps & framing, FNV-1a64 streaming hasher, atomic `ExclusiveFileBuf`, CLI parser/validation, 5 focused B3 tests).
  - [`sim/plus/p10_boot_test_top.v`](../../sim/plus/p10_boot_test_top.v): +2 lines (`output dbg_cpr_load_abort`, `assign dbg_cpr_load_abort = cart_load_abort;`).
  - [`rtl/plus/plus_sprite_ram.v`](../../rtl/plus/plus_sprite_ram.v): +16 lines, **COMMENT-ONLY** (lines 19–34: P10j same-port collision unreachability invariant & M10K synthesis boundary).
  - [`rtl/plus/asic_regs.v`](../../rtl/plus/asic_regs.v): +8 lines, **COMMENT-ONLY** (lines 197–205: P10j `eff_addr` reset-domain ownership note).
  - [`docs/plus/b3-capture-2026-09-03.md`](../../docs/plus/b3-capture-2026-09-03.md): dated record of B3 capture implementation, evidence, and bounds.

---

## 2. Gate Verification & Suite Execution

1. **Simulation Suite (`make -C sim`)**:
   - Re-run verified in [`.coord-inputs/b3-parent-sim.log`](../../.coord-inputs/b3-parent-sim.log).
   - All suites passed: `crt_filter_blank_tests`, `asic_video_tests`, `asic_sprites_tests`, `asic_dma_tests`, `plus_p8_tests`, `p10_boot_tests` (including the 5 B3 tests), `b7-dark-silicon-audit` (all 9 active Plus mutations changed, all 9 classic-in-plus unchanged, all 4 classic controls changed), and `u765_tb`.
   - Result: **0 failures** (`Summary: 6 passed, 0 failed` on final harness; exit 0).
   - Existing FDC XFAIL (`fdc-payload-poll` byte 0 mismatch) and XPASS guard remain intact; `SYNC_FILTER` parameter default in [`p10_boot_test_top.v:14`](../../sim/plus/p10_boot_test_top.v#L14) remains `2'd2` (Off).
2. **Lint Suite (`make -C sim lint`)**:
   - Re-run executed directly: passes cleanly with exit code 0 (`b3_make_lint.log: EXIT:0`).
   - Synthesis branch of `plus_sprite_ram` verified against `altsyncram_lint_stub.v`.

---

## 3. Runtime CPR Abort & Malformed Failure Propagation

- **Hardware Tap Wiring**:
  [`p10_boot_test_top.v:744`](../../sim/plus/p10_boot_test_top.v#L744) assigns `dbg_cpr_load_abort = cart_load_abort`. This wire is driven by [`plus_cpr_parser.v`](../../rtl/plus/plus_cpr_parser.v) on any syntax/RIFF error or chunk boundary violation.
- **Harness Latching**:
  In [`p10_boot_test.cpp`](../../sim/plus/p10_boot_test.cpp#L394), `Harness::raw_tick()` samples `dut.dbg_cpr_load_abort` into `cpr_load_abort_seen`.
- **Download & Apply Rejection**:
  `Harness::download()` checks `cpr_load_abort_seen` during packet delivery and apply transitions, immediately throwing `TestFailure("CPR parser aborted during download")` or `TestFailure("CPR parser aborted while applying download")`.
  `wait_for_cpr_apply()` independently checks `cpr_load_abort_seen`, `cpr_load_error_seen`, and `dut.dbg_cpr_load_error`.
- **Pre-execution Failure Boundary**:
  In `run_capture_cli()` ([`p10_boot_test.cpp:1892-1903`](../../sim/plus/p10_boot_test.cpp#L1892-L1903)), `h.download(image)` and `wait_for_cpr_apply(h)` execute **before** `output_buffer.open(options.output_path)`. A malformed or truncated CPR aborts the run before the output file descriptor is even requested or created on disk.

---

## 4. Finite Limits, Metadata Truthfulness, & Tap Integrity

- **Finite Limits**:
  - Frame count: strictly bounded to `1 <= n <= 16` (`kCaptureMaxFrames = 16`). Enforced both in CLI parsing (`parse_capture_frame_count`) and in harness (`capture_video_frames`).
  - Timeouts: `kCaptureWaitLimit = 16,000,000` ticks for resets, apply, and VSYNC phase acquisition; `kCaptureFrameTickLimit = 16,000,000` ticks per frame. Overruns throw named `TestFailure` exceptions.
  - CPR file input: bounded to `32 MiB` (`kCaptureMaxCprBytes = 0x02000000`); empty CPRs (`size <= 0`) are refused.
  - Serialized output: bounded to `2 GiB` (`kCaptureMaxOutputBytes = 2 * 1024 * 1024 * 1024`). `CaptureWriter::write()` checks remaining quota before writing and throws if exceeded.
- **Truthful Metadata**:
  The emitted stream header truthfully records:
  - Version: `version=1`, `clock_hz=64000000`, `delimiter=selected_vsync rising edge (0-to-1)`.
  - CPU & surrogate limits:
    `# cpu_surrogate=t80pa_wrapped_reduced_tv80 (fixture only; the production VHDL T80 is not compiled under Verilator, so JR/DJNZ/JP-cc/interrupt software is out of scope)`.
  - Clocking mode: `# production_clocking=0 (fixture_divider)`.
  - Sync filter selection: `# filter_selector=2 (0=Full,1=Live_blank,2=Off)`.
  - Simulator identity: simulator executable path, file size, and FNV-1a64 hash (or explicit `--build-id`).
- **Raw vs Selected vs Shared Payload**:
  - Header explicitly declares:
    `# raw_* = motherboard hs_sel/vs_sel/de_sel source taps; selected_* = production output tuple`
    `# RGB/MA/RA/VRAM are shared and filter-dependent taps, not pre/post-filter taps`
  - Verified against motherboard wiring: `raw_hsync/vsync/de` are pre-filter sources; `selected_hsync/vsync/hblank/vblank` are the post-filter tuple; `rgb`, `ma`, `ra`, and `vram_*` reflect `crtc_shift`-shaped video memory and are shared. No false claim of a pre/post filter RGB split is made.

---

## 5. File Output Safety Analysis

- **Input/Output Aliasing & Collisions**:
  [`paths_identical()`](../../sim/plus/p10_boot_test.cpp#L1846) evaluates both `weakly_canonical` path equivalence and, when both files exist, `std::filesystem::equivalent` (device + inode matching). An output path pointing to the input CPR is rejected before simulation.
- **Pre-existing File & Symlink Pre-check**:
  [`reject_existing_capture_output()`](../../sim/plus/p10_boot_test.cpp#L1869) inspects `std::filesystem::exists(output_path)`. A pre-existing file or a symlink pointing to an existing file is refused prior to simulation.
- **Atomic Creation & Symlink Refusal (`ExclusiveFileBuf`)**:
  [`ExclusiveFileBuf::open()`](../../sim/plus/p10_boot_test.cpp#L212) uses `O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0666`.
  - `O_CREAT | O_EXCL` guarantees atomic creation: files appearing between pre-check and open fail with `EEXIST` rather than being overwritten.
  - `O_NOFOLLOW` refuses to follow symlinks. On Linux, following a symlink fails with `ELOOP`. On macOS, `O_CREAT | O_EXCL` against an existing directory entry (even a dangling symlink) returns `EEXIST`.
  - `run_capture_cli()` uses `std::filesystem::is_symlink(std::filesystem::symlink_status(...))` to explicitly identify and report symlinks across all platforms: `"capture output path is a symlink; refusing to follow it"`.
- **Descriptor Ownership & Partial Writes**:
  - `ExclusiveFileBuf` owns `fd_`. Destructor safely calls `close()`, which flushes (`sync()`) and sets `fd_ = -1`.
  - Short writes are tracked in a retry loop inside `flush_buffer()`. If `::write()` fails or returns `<= 0`, `flush_buffer()` returns `false`, `xsputn()` halts, `std::ostream` sets badbit/failbit, and `require_stream_ok()` throws `TestFailure`.
  - In-flight failures abort via exception. Partial output files are neither unlinked nor reported as valid; subsequent runs refuse to overwrite them, preventing false success.

---

## 6. Test Value & Boundary Negative Cases

- **Negative CLI Tests**:
  `test_b3_capture_cli_validation()` validates invalid frame counts (`0`, `17`, `two`), missing trio components, duplicate flags, combined `--help`, unknown arguments, same-path collision, existing output, symlink-to-existing output, and dangling symlink refusal.
- **Malformed & Truncated CPR Tests**:
  - `test_b3_malformed_cpr_rejected`: mutates RIFF magic byte from `'R'` to `'X'`; asserts parser aborts during download.
  - `test_b3_truncated_cpr_rejected`: provides an 8-byte valid RIFF prefix without body chunks; asserts failure to reach commit/valid state.
- **Repeatability & Steady-State Equality**:
  - `test_b3_frame_capture_smoke`: boots a static CRTC CPR, waits for CRTC programming (R9=7), discards 2 warmup frames, and proves consecutive steady-state frames 0 and 1 match in sample count, raw sync edges, selected sync edges, active video ticks, and nonzero FNV-1a64 hash.
  - `test_b3_repeat_capture_stable`: executes two independent boots from scratch and asserts exact bit-for-bit serialization equality (`stream.str() == first_serialized`).
  - No synthetic golden hash oracle is committed; hashes verify self-equality and serialization determinism only.

---

## 7. P10j Debt Disposition & Production Source Chain Derivation

**Disposition**: **CLEARED** (documentation debt resolved by comments in `plus_sprite_ram.v` and `asic_regs.v`).

The claimed production invariant—that same-port read-during-write on `plus_sprite_ram` Port A is physically unreachable—was independently verified against the production RTL sources:

1. **SNA Drain Lifecycle**:
   In [`rtl/plus/plus_sna_parser.v:54-55`](../../rtl/plus/plus_sna_parser.v#L54-L55):
   ```verilog
   assign busy = sna_download || cpc_plus_byte_wr || !fifo_empty || asic_sna_wr;
   ```
   `busy` remains asserted throughout download, byte strobe registration, FIFO drainage, and the active write tail (`asic_sna_wr`).
2. **Apply Barrier**:
   In [`Amstrad.sv:636-640`](../../Amstrad.sv#L636-L640):
   ```verilog
   if(sna_finish_pending && !romdl_wait && !boot_wr && !sna_rle_count && !plus_sna_busy) begin
       sna_finish_pending <= 1'b0;
       sna_apply_cnt <= 3'd5;
   end
   ```
   `sna_finish_pending` cannot clear until `!plus_sna_busy`, guaranteeing every SNA write to sprite RAM has retired before `sna_apply_cnt` begins.
3. **CPU Reset Hold**:
   In [`Amstrad.sv:726-727`](../../Amstrad.sv#L726-L727):
   ```verilog
   wire reset_base = ... sna_download | sna_finish_pending | (old_sna_download_reset & ~sna_download) | (sna_apply_cnt > 3'd2);
   ```
   Top-level `reset` holds `motherboard.reset` throughout download, `sna_finish_pending`, and early apply.
4. **Inactive Production CPU Controls**:
   In [`rtl/T80/T80pa.vhd:148-152`](../../rtl/T80/T80pa.vhd#L148-L152):
   ```vhdl
   if RESET_n = '0' then
       WR_n   <= '1';
       RD_n   <= '1';
       IORQ_n <= '1';
       MREQ_n <= '1';
   ```
   While reset is asserted, `RD_n` and `MREQ_n` are forced high ('1').
   In [`rtl/Amstrad_motherboard.v:146`](../../rtl/Amstrad_motherboard.v#L146):
   `assign mem_rd = ~(RD_n | MREQ_n);` $\rightarrow$ forced low (`0`).
   In [`rtl/plus/asic_regs.v:581`](../../rtl/plus/asic_regs.v#L581):
   `wire spr_host_rd = asic_cs && mem_rd && (wsel == 2'b00);` $\rightarrow$ forced low (`0`).
   Therefore, a CPU read cannot coincide with an SNA write.
5. **Decoupled ASIC Reset Domain**:
   `asic_regs` and `plus_sprite_ram` receive `plus_asic_reset` ([`Amstrad.sv:737`](../../Amstrad.sv#L737)), which pulses only at the leading edge of `sna_download` and is **not** held during the drain. Thus, `eff_cs` and `spr_host_wr` remain active to populate sprite RAM while the CPU is held idle.
6. **M10K Silicon vs Behavioral RAM Semantics**:
   - CPU vs CPU same-port write/read is excluded by Z80 bus cycles (`RD_n` and `WR_n` are never simultaneously active).
   - Port B (video fetch) is strictly read-only (`wren_b = 1'b0`).
   - Mixed-port read-during-write collision returns `OLD_DATA` on both behavioral Verilog (`mem` non-blocking read/write) and Quartus M10K silicon (`read_during_write_mode_mixed_ports = "OLD_DATA"`).
   - The comments in [`plus_sprite_ram.v:19-34`](../../rtl/plus/plus_sprite_ram.v#L19-L34) and [`asic_regs.v:197-205`](../../rtl/plus/asic_regs.v#L197-L205) accurately document this invariant and distinguish the elaboration-only lint stub from M10K hardware behavior.

---

## 8. Actionable Findings & Symbols

### Finding B3-1 (Low / Test Quality, Non-blocking)
- **Symbol**: [`sim/plus/p10_boot_test.cpp:2047-2051`](../../sim/plus/p10_boot_test.cpp#L2047-L2051) (`test_b3_capture_cli_validation`)
- **Detail**: In the dangling symlink test, the assertion:
  ```cpp
  require(refused_dangling &&
              (dangling.error_number() == ELOOP ||
               dangling.error_number() == EEXIST),
          "exclusive open followed a dangling symlink instead of refusing it");
  ```
  accepts both `ELOOP` (Linux) and `EEXIST` (macOS), which correctly verifies that `open()` refused. However, the test does not directly inspect the filesystem to assert that the symlink target was not created.
- **Recommendation**:
  For strict non-clobbering verification, add:
  ```cpp
  require(!std::filesystem::exists("b3_cli_no_such_target.tmp"),
          "dangling symlink target was created on disk");
  require(std::filesystem::is_symlink(std::filesystem::symlink_status(dangling_link)),
          "dangling symlink entry was clobbered");
  ```
  This proves on the filesystem itself that the target was untouched and the link entry was preserved, rather than relying solely on the returned `errno`.

---

## 9. Remaining Hardware, Full-T80, & CI Boundaries

1. **Reduced TV80 Surrogate vs Production VHDL T80pa**:
   Verilator tests run against `tv80_to_t80pa_wrapper` using reduced TV80 opcodes (`LD BC, nn`, `LD A, n`, `OUT (C), A`, `JP`, `HALT`). Software requiring `JR JumpE`, conditional `JP cc`, `DJNZ`, `LDIR`, interrupts, or sub-T-state bus timings remains unsupported and must not be used as test programs in this harness.
2. **Synthetic Frame Self-Equality vs Hardware Oracle**:
   Steady-state frame capture hashes prove internal simulation consistency and raster repeatability. They do not constitute an oracle against real Amstrad Plus hardware, CRTC3/ASIC chip anomalies, or reference photographs.
3. **CI Synthesis & Full-Core Validation**:
   Verification of full top-level integration (including SDRAM controller timings, HPS I/O, audio DACs, OSD mixing, and Quartus M10K block allocation) belongs to the Quartus synthesis build on the integration branch (`accc-review-and-fixes`).
