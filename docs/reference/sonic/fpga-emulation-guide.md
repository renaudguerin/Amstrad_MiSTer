# Sonic GX FPGA Emulation Guide & Defect Audit

This guide provides a structured verification checklist, technical audit mapping, and defect inventory for developers and LLM agents working on FPGA Amstrad cores (specifically the MiSTer Amstrad core in `rtl/plus/`).

It translates the hardware mechanisms and timing assumptions documented in Sonic GX into concrete Verilog audit points, identifies active timing hazards, and records CPR reverse-engineering findings.

---

## 1. CPR Cartridge Binary Reverse-Engineering (Ground Truth)

Disassembly of the production CPR cartridge (`docs/plus/cartridges/Sonic the Hedgehog (UK) (64K) (2025) [Original].cpr`) establishes the baseline hardware configuration:

### Boot Sequence & CRTC Register Initialization (`cb00` @ `0x007F`)
On cold boot, the engine programs CRTC registers 0 through 9:
* **`R0 = 63`**: Horizontal total = 64 characters (standard 64 µs line).
* **`R1 = 48`**: Horizontal displayed = 48 characters (overscan width = 192 pixels in Mode 0).
* **`R2 = 50`**: Horizontal sync position = character 50.
* **`R3 = 140` (`&8C`)**: VSYNC width = 8 lines; HSYNC width = 12 characters.
* **`R4 = 38`**: Vertical total = 39 character rows ($39 \times 8 = 312$ scanlines, standard 50 Hz PAL).
* **`R5 = 0`**: Vertical total adjust = 0.
* **`R6 = 32`** (set dynamically in `cb30`): Vertical displayed = 32 character rows (256 scanlines).
* **`R7 = 34`**: Vertical sync position = character row 34 (scanline 272).
* **`R8 = 0`**: Interlace mode = 0 (progressive, no interlace).
* **`R9 = 7`**: Max raster = 7 (8 scanlines per character row).

### Interrupt Configuration & DMA Auto-Clear Mode (`cb30` @ `0x3928`, `cb18` @ `0x2780`)
* **`IM 2`**: Interrupt Mode 2 initialized with `I = &B8`. Vector table resides in RAM at `&B800`.
* **`IVR = &00` (`&6805`)**: Sonic explicitly executes `XOR A; LD (&6805), A`:
  * Sets vector table base to `&00` (Vector offsets: `&00`=DMA2, `&02`=DMA1, `&04`=DMA0, `&06`=PRI/Raster).
  * **Bit 0 = 0 (DMA Auto-Clear Mode Enabled)**: Hardware automatically clears the highest-priority pending DMA flag in `DCSR` upon interrupt acknowledge. The engine does *not* write to `&6C0F` to clear DMA interrupts manually.

### PRI Sky Raster Handler (`cb16` @ `&A000` / `cb18`)
* Background sky rasters cycle Pen 0 using the Z80 alternate register set (`EXX`).
* The handler executes `INC C; LD A, C; LD (&6800), A`, re-arming the PRI comparator for the **very next scanline** on every single line of the sky.

---

## 2. RTL Audit Checklist & Critical Hardware Timing Hazards

### Hazard 1: Coincident Interrupt Priority vs Acknowledge Clearing (B19 Residual)
* **Hardware Requirement**: When PRI and DMA coincide, priority must be `PRI > DMA2 > DMA1 > DMA0`. Pending interrupts of lower priority must remain asserted for subsequent service.
* **Relevant Core Files**: [`rtl/plus/asic_ga_timing.v`](../../rtl/plus/asic_ga_timing.v), [`rtl/plus/asic_regs.v`](../../rtl/plus/asic_regs.v)
* **Code Verification**:
  * In `asic_regs.v` (lines 627–636), `ack_src` priority arbitration correctly prioritizes `PRI` over `DMA2..0`.
  * In `asic_ga_timing.v` (lines 656–658):
    ```verilog
    if (int_reset) INT_N <= 1'b1;
    else if (raster_fire) INT_N <= 1'b0;
    ```
  * During any interrupt acknowledge cycle (`intack`), `irqack_rst` asserts `int_reset = 1` for ~1 µs (4 Z80 T-states).
* **Failure Mechanism `[HYPOTHESIS - PENDING VERIFICATION]`**:
  * If a `raster_fire` occurs while a DMA interrupt acknowledge cycle is in progress, `int_reset` dominates and clears `INT_N`.
  * Because Sonic re-arms PRI for line $N+1$ from inside the line $N$ handler, **dropping a single raster interrupt stalls the entire sky gradient for the rest of the frame**.
  * Coincident DMA/PRI events occur sporadically, causing the sky gradient to freeze at different lines frame-to-frame, producing extreme 50 Hz flickering.

---

### Hazard 2: Mid-Frame `SPLT` & `SSA` Latch Timing Deadline (`hcc == R1`)
* **Hardware Requirement**: Sonic splits the screen into 3 windows across dual 16 KB VRAM buffers (`&0000` and `&C000`). The third segment is reprogrammed mid-frame inside a DMA2 ISR by rewriting `SPLT` (e.g. line 255) and `SSA` on the fly.
* **Relevant Core File**: [`rtl/plus/asic_video.v`](../../rtl/plus/asic_video.v)
* **Code Verification**:
  ```verilog
  wire split_match = (SPLT != 8'd0) && ({charline[4:0], raster[2:0]} == SPLT);
  wire split_latch_event = CLKEN && !in_adj && split_match && (hcc == R1_h_displayed);
  ```
* **Failure Mechanism `[HYPOTHESIS - PENDING VERIFICATION]`**:
  * `split_latch_event` captures `SSA` strictly at `hcc == R1_h_displayed` (character 48).
  * **Timing Race**: If DMA2 interrupt latency (delayed by long instructions or alternate ISRs) causes the Z80 to write `SPLT` or `SSA` after character 48 of the split line, the split latch event is missed for that line.
  * **8-bit Wrap Anomaly**: In a 312-line frame ($R4=38, R9=7$), scanlines 256–311 wrap modulo 256 to 0–55. Writing `SPLT = 255` (`(32 * 8) - 1`) sits right on the wrap boundary. If not cleared during vertical retrace, it re-triggers at line 55 (near line 311).

---

### Hazard 3: DMA Auto-Clear Mode (`IVR[0] = 0`) Desynchronization
* **Hardware Requirement**: With `IVR[0] = 0`, the ASIC must automatically clear the acknowledged DMA channel's flag in `DCSR`.
* **Relevant Core File**: [`rtl/plus/asic_regs.v`](../../rtl/plus/asic_regs.v)
* **Code Verification**:
  ```verilog
  wire auto_clr_dma = intack && !intack_d && !ivr_r[0] && !int_pending;
  wire [2:0] auto_clr_mask = (dcsr_flags[2]) ? 3'b100 :
                             (dcsr_flags[1]) ? 3'b010 :
                             (dcsr_flags[0]) ? 3'b001 : 3'b000;
  ```
* **Failure Mechanism `[HYPOTHESIS - PENDING VERIFICATION]`**:
  * If `int_pending` is high when `intack` begins, `auto_clr_dma` is inhibited (correctly reserving the ack for raster).
  * However, if DMA interrupts trigger back-to-back while raster status is resolving, jitter in `int_pending` or missed ack pulses can leave DMA channel flags stuck high, resulting in repeated spurious ISR entries or audio desync.

---

### Hazard 4: `SSCR` Soft-Scroll Vertical Offset vs `SPLT` Comparator Seam
* **Hardware Requirement**: Soft scroll fine delay (`SSCR[6:4]`) shifts the displayed raster within character cells.
* **Relevant Core File**: [`rtl/plus/asic_video.v`](../../rtl/plus/asic_video.v)
* **Code Verification**:
  * Display raster counter: `ra_eff = {raster[4:3], (raster[2:0] + SSCR[6:4]) & 3'd7}`.
  * Split comparator: `split_match = (SPLT != 8'd0) && ({charline[4:0], raster[2:0]} == SPLT)`.
* **Observation**: `split_match` tests raw `raster[2:0]`, whereas pixel generation uses `ra_eff`. When vertical soft scroll is non-zero, the visual split seam diverges from the internal character row boundary by 0–7 scanlines (Gap G5 in `asic-documentation-gap-map.md`).

---

## 3. Defect Inventory & Capture Diagnosis

| Capture | Observed Defect | Diagnosed Mechanism `[STATUS]` |
|---|---|---|
| `docs/screenshots/testing_0909/20260909_020052-...png` | Sky gradient has solid horizontal color bands; extreme 50 Hz frame flickering. | **Hazard 1 (B19 Residual)**: `raster_fire` now latched during acknowledge in `asic_ga_timing.v` (build `88262b9`). Hardware test **inconclusive**: screen visual behavior may have improved, but display remains too severely broken by video/split timing defects (Hazard 2) to definitively assess. `[INCONCLUSIVE - PENDING SPLIT FIX]` |
| `docs/reference/sonic/20260913_140450-...png` | Level select menu text is doubled/garbled ("GREEN HILL ZONE" overlapping "BRIDGE ZONE"). | **Hazard 2 / Buffer Alternation**: Sonic alternates display between VRAM Buffer 0 (`&0000`) and Buffer 1 (`&C000`) using mid-frame splits. A missed or jittered `SPLT` latch alternates buffer rendering at 50 Hz, appearing superimposed in progressive captures. `[HYPOTHESIS - PENDING VERIFICATION]` |
| `docs/reference/sonic/20260913_140452-...png` | Upper menu lines doubled; lower lines ("ACT 1", "START") resolve cleanly in white. | **Hazard 2 / Split Seam Transition**: Upper segment failed to reload SSA properly; lower segment latched SSA after split line recovered. `[HYPOTHESIS - PENDING VERIFICATION]` |
| In-game (GHZ rings) | Ring matrix flickering or disappearing under enemy/Sonic load. | **DMA1 Prescaler Timing**: DMA1 interrupt repositioning of sprites 10–15 misses the vertical scanline band budget. `[UNVERIFIED - AWAITING TEST]` |
| In-game (Sonic sprint) | Tearing between left (Sprite 0) and right (Sprite 1) compound halves. | **Shadow Buffer Flush Boundary**: Sprite coordinate register writes extending past vertical blanking into line 0. `[UNVERIFIED - AWAITING TEST]` |

---

## 4. Verification Methodology & Recommended Next Steps

### Step 1: Fix Foundational Low-Hanging Fruit (Hazard 1: Interrupt Race)
Before touching complex video split logic, resolve the known B19 interrupt race in `rtl/plus/asic_ga_timing.v`:
* Ensure a `raster_fire` occurring during `irqack_rst` is latched rather than discarded by `int_reset`.
* Verify that clearing raster interrupts on acknowledge only applies to the raster request that initiated that acknowledge.

### Step 2: Capture Oracle Golden Trace with AmSpirit
Run `scripts/amspirit/amspirit.py` against `docs/plus/cartridges/Sonic the Hedgehog (UK) (64K) (2025) [Original].cpr`:
* Record exact DMA channel fire scanlines and ISR execution durations.
* Extract register trace during level select menu to confirm exact cycle timing of `SPLT`, `SSA`, and `SSCR` writes.

### Step 3: Device Retest & Defect Narrowing
* Retest build on MiSTer hardware using `mister-capture`.
* Determine whether fixing Hazard 1 stabilizes the sky and eliminates the 50 Hz menu flicker before attempting changes to `asic_video.v` split logic.
