# P10j primitive/model contract review

Reviewer: Gemini 3.8 Flash high, independent of Muse Spark implementation.
Date: 2026-09-03. Review run: `20260903T072746Z-88334-b69c`.
Reviewed base: `d3aabbca4f4bee8e7b1d928f1831b647e7e3685b`.

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

The coordinator ran `make -C sim` and `make -C sim lint` on the complete reviewed Plus candidate; both process exits were 0. The two production-file changes are comments only. Hardware and primitive fit validation remain separate evidence tiers.
