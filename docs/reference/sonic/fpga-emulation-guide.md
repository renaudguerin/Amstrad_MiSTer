# FPGA Emulation Guide & Core Audit Checklist

This guide provides a structured verification checklist and technical audit mapping for developers and LLM agents working on FPGA Amstrad cores (specifically the MiSTer Amstrad core in `rtl/plus/`).

It translates the hardware mechanisms and timing assumptions documented in Sonic GX into concrete Verilog audit points and testbench vector scenarios.

---

## RTL Audit Checklist: MiSTer Core vs. Sonic GX Requirements

### 1. Mid-Frame Dynamic `SPLT` & `SSA` Reprogramming
* **Hardware Requirement**: Sonic GX triggers a second screen split on the same field by rewriting ASIC registers `SPLT` (`&6801`) and `SSA` (`&6802/&6803`) mid-frame from inside a `DMA2` interrupt handler.
* **Relevant Core File**: [`rtl/plus/asic_video.v`](file:///Users/renaudg/.gemini/antigravity/worktrees/Amstrad_MiSTer/parse_sonic_technical_docs/rtl/plus/asic_video.v)
* **Code Verification**:
  ```verilog
  // P6: Screen split comparison ({SPLT7..0} == {VC4..0, RC2..0}, asic-reference §8).
  // When matched and SPLT != 0, capture SSA into vma_latch at HCC == R1.
  wire split_match = (SPLT != 8'd0) && ({charline[4:0], raster[2:0]} == SPLT);
  wire split_latch_event = CLKEN && !in_adj && split_match && (hcc == R1_h_displayed);
  ```
* **Audit Assessment**:
  * The comparison `{charline[4:0], raster[2:0]} == SPLT` is **fully combinatorial** and does not latch a "split already occurred" lockout bit.
  * When `DMA2` updates `SPLT` to a later scanline (e.g. line 255) and rewrites `SSA`, the comparator correctly fires a second time on the same frame.
  * At `hcc == R1_h_displayed`, `vma_latch` captures the new `SSA`. At `hcc_last`, `vma` updates to the new `vma_latch`.
  * **Pass**: The MiSTer core architecture inherently supports dynamic mid-frame multi-splitting.

---

### 2. Simultaneous Interrupt Priority Arbitration
* **Hardware Requirement**: When multiple interrupt conditions coincide on the same scanline, the hardware priority order must strictly follow:
  $$\text{PRI} \longrightarrow \text{DMA2} \longrightarrow \text{DMA1} \longrightarrow \text{DMA0}$$
* **Relevant Core File**: [`rtl/plus/asic_regs.v`](file:///Users/renaudg/.gemini/antigravity/worktrees/Amstrad_MiSTer/parse_sonic_technical_docs/rtl/plus/asic_regs.v)
* **Code Verification**:
  ```verilog
  // Vector byte: (IVR & &F8) | source; source = %110 (raster) while a raster interrupt pends
  if (intack && !intack_d) begin
      if (int_pending)
          ack_src <= 3'b110;  // PRI (raster interrupt) -> Highest priority
      else if (dcsr_flags[2])
          ack_src <= 3'b000;  // DMA2 -> Second priority
      else if (dcsr_flags[1])
          ack_src <= 3'b010;  // DMA1 -> Third priority
      else if (dcsr_flags[0])
          ack_src <= 3'b100;  // DMA0 -> Lowest priority
      else
          ack_src <= 3'b000;
  end
  ```
* **Audit Assessment**:
  * **Exact Match**: The priority encoder in `asic_regs.v` implements the exact priority order discovered on real Plus hardware by Arnaud Storq.

---

### 3. DMA Interrupt Generation Outside Visible Screen
* **Hardware Requirement**: DMA interrupts (`DMA0`, `DMA1`, `DMA2`) must be capable of asserting `/INT` outside the active CRTC display area (i.e. in borders, blanking, and overscan).
* **Relevant Core Files**:
  * [`rtl/plus/asic_dma.v`](file:///Users/renaudg/.gemini/antigravity/worktrees/Amstrad_MiSTer/parse_sonic_technical_docs/rtl/plus/asic_dma.v)
  * [`rtl/plus/asic_regs.v`](file:///Users/renaudg/.gemini/antigravity/worktrees/Amstrad_MiSTer/parse_sonic_technical_docs/rtl/plus/asic_regs.v)
* **Audit Assessment**:
  * `asic_dma.v` channel execution is clocked by audio clock pulses and prescalers, operating completely independently of CRTC `DISPTMG` or `HSYNC`/`VSYNC` gating.
  * When a DMA channel executes an `INT` instruction, it sets `dma_int_set[2:0]`, which asserts `dcsr_flags` and triggers `dma_int_req = |dcsr_flags`.
  * **Pass**: DMA interrupts fire reliably during vertical and horizontal blanking intervals.

---

### 4. Sprite RAM Write Contention & Video Serialization
* **Hardware Quirk**: Real Plus silicon exhibits a bus contention spike resulting in white pixel artifacts (`Pen 15 / 4'hF`) if the Z80 writes to an ASIC sprite address while the video beam is fetching that sprite.
* **Relevant Core File**: [`rtl/plus/plus_sprite_ram.v`](file:///Users/renaudg/.gemini/antigravity/worktrees/Amstrad_MiSTer/parse_sonic_technical_docs/rtl/plus/plus_sprite_ram.v)
* **Code Verification**:
  ```verilog
  //  Mixed-port read-during-write semantics:
  //    Read-first / old-data on write collisions.
  //    Untouched locations initialize to zero (defined-zero FPGA model assumption).
  ```
* **Audit Assessment**:
  * The FPGA block RAM (Altera Cyclone V M10K) returns `OLD_DATA` during mixed-port collisions.
  * **Emulation Fidelity Note**: Because the FPGA core does not emulate the white artifact, well-behaved games like Sonic GX render cleanly. If software fails to schedule sprite updates outside the beam window, it will run without glitches on MiSTer but will glitch on real hardware.

---

### 5. Soft Scroll Register (`SSCR`, `&6804`) Mechanics
* **Hardware Requirement**:
  * `SSCR[3:0]`: Horizontal pixel delay (0..7 Mode 0 half-pixels).
  * `SSCR[6:4]`: Vertical scanline delay (0..7 scanlines).
  * `SSCR[7]`: Border mask (suppresses first character column).
* **Relevant Core File**: [`rtl/plus/asic_video.v`](file:///Users/renaudg/.gemini/antigravity/worktrees/Amstrad_MiSTer/parse_sonic_technical_docs/rtl/plus/asic_video.v)
* **Code Verification**:
  * Line 527:
    ```verilog
    wire [4:0] ra_eff = {raster[4:3], (raster[2:0] + SSCR[6:4]) & 3'd7};
    ```
  * Line 1184:
    ```verilog
    wire [3:0] pen_delayed = (SSCR[3:0] == 4'd0) ? pen_nib : pen_delay[SSCR[3:0] - 4'd1];
    ```
  * Line 1200:
    ```verilog
    wire eff_de = de_hold & ~(SSCR[7] & de_first_char);
    ```
* **Audit Assessment**:
  * Vertical scroll wraps cleanly within the character cell without altering the CRTC character line counter `charline`.
  * Horizontal shift register delays pixel output by 0..7 clock cycles without shifting the display enable window.
  * **Pass**: Full conformity with Amstrad Plus ASIC specification §8.

---

## Capture Analysis: Baseline Observations

Two MiSTer hardware captures exist in this directory:
1. `20260913_140450-Sonic the Hedgehog (UK) (64K) (2025) [Original].png` (Title / Intro)
2. `20260913_140452-Sonic the Hedgehog (UK) (64K) (2025) [Original].png` (Zone Title Card: "ACT 1", "START")

### In-Game Emulation Test Points
When evaluating Sonic GX gameplay recordings or regression captures on MiSTer:

1. **Background Sky Gradient**:
   * Inspect the upper border and sky area.
   * Verify that the PRI interrupt updates Pen 0 every scanline without vertical jitter, color smearing, or missing raster bars.
2. **Screen Split Seams (Lines 119 and 255)**:
   * Inspect scanlines 119 and 255 during rapid vertical descent (e.g. falling through Green Hill Zone tunnels).
   * Confirm there is no 1-scanline black line, garbage address blip, or horizontal jitter at the seam where `SPLT` reloads `SSA`.
3. **Ring Multiplexing Grid (5×8 Array)**:
   * Navigate to a field of rings.
   * Verify all 5 columns across all 8 rows render simultaneously.
   * Confirm that rings do not flicker or disappear when Sonic or enemies occupy the same horizontal scanlines.
4. **Compound Character Sprites**:
   * Observe Sonic when running at maximum speed.
   * Confirm that Sprite 0 (left half) and Sprite 1 (right half) remain perfectly locked horizontally and vertically with zero 1-frame tearing between halves.
5. **Audio DMA Synchronization**:
   * Verify that music and sound effects play without popping, buffer underruns, or pitch shifts when rapid 8-way scrolling and sprite multiplexing are active.
