# Hardware Test Findings & Fix Plan (Amstrad Plus / GX4000)

**Date**: 2026-08-28  
**Status**: IN PROGRESS  
**Target Branch**: `plus/hardware-checkpoint`  
**Reference Images**:
- Active area grey / border blue (`Burnin' Rubber`): `media_1787942285549.jpg`
- `Drive A: read fail` (`Basic 6128 Fr.cpr` / `Basic Plus Fr.cpr`): `media_1787942290314.jpg`

---

## 1. Finding HF-1: Floppy Disk Controller (FDC) Port Decode Ignores $A_7=1$

### Symptom
When booting cartridges that include AMSDOS (e.g. `Basic 6128 Fr.cpr`, `Basic Plus Fr.cpr`, System Cartridge v4 with AMSDOS in page 3), the firmware initializes the system, displays the copyright banner (`Amstrad Microcomputer (f4)`), pauses during drive initialization, and errors out with:
```
Drive A: read fail
Retry, Ignore or Cancel?
```

### Root Cause Analysis
In [`Amstrad.sv`](file:///Users/renaudg/code/Amstrad_MiSTer-plus/Amstrad.sv#L809-L824):
```verilog
wire [3:0] fdc_sel = {cpu_addr[10], cpu_addr[8], cpu_addr[7], cpu_addr[0]};
wire [7:0] fdc_dout = (u765_sel & io_rd) ? u765_dout : 8'hFF;

reg motor = 0;
always @(posedge clk_sys) begin
    reg old_wr;
    old_wr <= io_wr;
    if(~old_wr && io_wr && !fdc_sel[3:1]) begin
        motor <= cpu_dout[0];
    end
end

wire [7:0] u765_dout;
wire       u765_sel = (fdc_sel[3:1] == 'b010) & ~status[17];
```
- `fdc_sel[3:1]` bundles `{cpu_addr[10], cpu_addr[8], cpu_addr[7]}` and expects `cpu_addr[7] == 0`.
- On real Amstrad CPC / 6128+ / DDI-1 hardware, address line $A_7$ is **not** decoded:
  - FDC Floppy Motor: `!A[10] & !A[8]` (e.g. `&FA7E`, `&FADD`, `&FAFE`).
  - FDC Status/Data: `!A[10] & A[8]` (e.g. `&FB7E`, `&FBDF`, `&FB7F`, `&FBFF`), with $A_0$ selecting Status ($A_0=0$) vs Data ($A_0=1$).
- Standard AMSDOS (v0.5) explicitly uses `&FADD` for floppy motor control and `&FBDF` for FDC command/data transfers. Because $A_7=1$ in these port addresses, `fdc_sel[3:1]` evaluated to `3'b001` (motor) and `3'b011` (FDC), causing the core to ignore motor writes and fail all FDC communication.

### Implementation Tasks
- [ ] In `Amstrad.sv`, redefine FDC and Motor selection to ignore `cpu_addr[7]`:
  ```verilog
  wire fdc_motor_sel = !cpu_addr[10] & !cpu_addr[8];
  wire u765_sel = !cpu_addr[10] & cpu_addr[8] & ~status[17];
  ```
- [ ] Update `motor` latch to trigger on `fdc_motor_sel & io_wr`.
- [ ] Connect `a0` of `u765` directly to `cpu_addr[0]`.

---

## 2. Finding HF-2: ASIC 12-Bit Palette Disconnected from `asic_video.v`

### Symptom
When booting Plus titles that program the ASIC 12-bit palette in `&6400..&643F` (e.g. `Burnin' Rubber`, `Pang`, `Klax`), the game unlocks the ASIC and programs palette entry 15 to `&0EEE` (white/grey), but the active video area and border render using default legacy Gate Array colors instead of the 12-bit ASIC palette.

### Root Cause Analysis
In [`rtl/plus/asic_video.v`](file:///Users/renaudg/code/Amstrad_MiSTer-plus/rtl/plus/asic_video.v#L861-L888):
```verilog
wire eff_de   = de_hold & ~(SSCR[7] & de_first_char);
wire [4:0] hw_sel  = eff_de ? INKR_I[pen_delayed*5 +: 5] : BORDER_I;
wire [11:0] rgb_mux = legacy_colour(hw_sel);
```
- `asic_video.v` translates pen indices using `legacy_colour(hw_sel)` from the fixed 27-color ROM table against legacy `INKR_I` / `BORDER_I` inputs.
- It never samples the 12-bit RGB palette stored in [`rtl/plus/asic_regs.v`](file:///Users/renaudg/code/Amstrad_MiSTer-plus/rtl/plus/asic_regs.v#L164) (`pal[0..31]`).
- In [`rtl/Amstrad_motherboard.v`](file:///Users/renaudg/code/Amstrad_MiSTer-plus/rtl/Amstrad_motherboard.v#L445), `pal_raddr` and `pal_rdata` are left unconnected.

### Implementation Tasks
- [ ] In `asic_regs.v`, expose the full 32×12-bit palette array or provide a dual-ported read interface for video rendering:
  - Pen $p \in [0..15]$ -> `pal[p]` (12-bit `{G,R,B}` or `{R,G,B}`).
  - Border -> `pal[16]`.
  - Sprites -> `pal[16 + sprite_color]`.
- [ ] In `asic_video.v`, multiplex between the 12-bit palette for active display (`eff_de ? pal[pen_delayed] : pal[16]`) and sprite overlay `SPR_RGB`.
- [ ] Verify both legacy Gate Array shadow updates (which update `pal[0..16]` via `legacy_colour_gbr`) and direct ASIC page writes (`&6400..&643F`) display accurately.

---

## 3. Finding HF-3: MMU 128K / 64K Bank Configuration Decoupled from Plus Model

### Symptom
On physical hardware, if the classic CPC OSD `Model` setting (`status[5:4]`) is set to `CPC 464`, selecting `Plus model = 6128+` still forces `ram64k = 1` in `Amstrad_MMU` and selects SDRAM bank 2 instead of bank 0 (128KB).

### Root Cause Analysis
In [`Amstrad.sv`](file:///Users/renaudg/code/Amstrad_MiSTer-plus/Amstrad.sv#L1292):
```verilog
.ram64k(model != 2'd0),
```
Where `model` is derived only from `valid_model(status[5:4])`. In Plus mode, `plus_model_select` already produces `plus_ram_128k` (1 for `6128+`, 0 for `GX4000` / `464+`), but `Amstrad.sv` does not pass `!plus_ram_128k` to `ram64k` or update the SDRAM memory bank.

### Implementation Tasks
- [ ] In `Amstrad.sv`, wire `.ram64k(plus_mode ? !plus_ram_128k : (model != 2'd0))`.
- [ ] In `Amstrad.sv`, set SDRAM bank to `(plus_mode ? (plus_ram_128k ? 2'b00 : 2'b10) : model)`.

---

## 4. Verification & Testing Protocol
1. **Unit & Integration Regression**:
   - `make -C sim`: Verify all 172 classic CRTC vectors and all Plus test suites remain green.
   - `make -C sim lint`: Check for zero lint errors or implicit nets.
2. **Motherboard Bench Integration**:
   - Extend `p1_mobo_bench_test.cpp` to verify FDC read/write at `&FADD`/`&FBDF` and 12-bit palette output on top-level RGB pins.
3. **Independent Review**:
   - Outsource implementation to Claude (via `claude`) and request cross-provider review from Codex (via `codex`).
