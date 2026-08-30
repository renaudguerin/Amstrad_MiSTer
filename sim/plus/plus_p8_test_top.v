// Top wrapper module for Phase P8 unit and the production parser / asic_regs /
// MMU SNA restore seam. Amstrad.sv itself remains a Quartus-only integration.
module plus_p8_test_top (
	input clk,
	input reset,

	// i8255 ports
	input        ppi_cs,
	input        ppi_we,
	input        ppi_oe,
	input  [1:0] ppi_addr,
	input  [7:0] ppi_idata,
	output [7:0] ppi_odata,
	input  [7:0] ppi_ipa,
	output [7:0] ppi_opa,
	input  [7:0] ppi_ipb,
	output [7:0] ppi_opb,
	input  [7:0] ppi_ipc,
	output [7:0] ppi_opc,
	input        ppi_plus_mode,

	// plus_sna_parser ports
	input        sna_download,
	input        cpc_plus_chunk_start,
	input        cpc_plus_byte_wr,
	input  [7:0] cpc_plus_byte_data,
	output       sna_ioctl_wait,
	output       sna_busy,
	output       asic_sna_wr,
	output [13:0] asic_sna_addr,
	output [7:0] asic_sna_data,
	output       asic_sna_active,
	output [7:0] asic_sna_rmr2,
	output       asic_sna_unlock,

	// plus_model_select ports
	input  [1:0] model_plus_model,
	output       model_plus_mode,
	output       model_ram_128k,
	output       model_has_fdc,
	output       model_has_tape,

	// FDC decode test signals
	input  [15:0] fdc_test_addr,
	input         fdc_test_status17,
	input         fdc_test_plus_mode,
	input         fdc_test_has_fdc,
	output        fdc_motor_sel,
	output        u765_sel,

	// Integrated SNA lifecycle and ASIC-register reset-seam signals
	input         seam_machine_reset,
	input         seam_plus_asic_reset,
	input         seam_sna_load,
	input         seam_plus_mode,

	// CPU-side read port into asic_regs for test verification
	input         aregs_cs,
	input         aregs_mem_rd,
	input         aregs_mem_wr,
	input  [13:0] aregs_addr,
	input   [7:0] aregs_din,
	output  [7:0] aregs_dout,

	// Direct probed views from asic_regs
	input   [4:0] aregs_pal_raddr,
	output [11:0] aregs_pal_rdata,
	output  [7:0] aregs_pri,
	output  [7:0] aregs_splt,
	output  [7:0] aregs_sscr,
	output  [7:0] aregs_ivr,
	output  [7:0] aregs_ssa_hi,
	output  [7:0] aregs_ssa_lo,
	output  [7:0] aregs_dcsr,
	output  [7:0] aregs_sar0_lo,
	output  [7:0] aregs_sar0_hi,
	output  [7:0] aregs_ppr0,
	output  [7:0] aregs_sar1_lo,
	output  [7:0] aregs_sar1_hi,
	output  [7:0] aregs_ppr1,
	output  [7:0] aregs_sar2_lo,
	output  [7:0] aregs_sar2_hi,
	output  [7:0] aregs_ppr2,
	output [159:0] aregs_spr_x_view,
	output [143:0] aregs_spr_y_view,
	output  [63:0] aregs_spr_mag_view,
	output [179:0] aregs_spr_pal_view,

	// Probed views from plus_mmu & asic_unlock
	output        mmu_asic_page_on,
	output        mmu_asic_unlocked
);

	plus_fdc_decode fdc_decode
	(
		.addr(fdc_test_addr),
		.plus_mode(fdc_test_plus_mode),
		.plus_has_fdc(fdc_test_has_fdc),
		.fdc_disabled(fdc_test_status17),
		.motor_sel(fdc_motor_sel),
		.u765_sel(u765_sel)
	);

	i8255 ppi
	(
		.reset(reset),
		.clk_sys(clk),
		.cs(ppi_cs),
		.we(ppi_we),
		.oe(ppi_oe),
		.addr(ppi_addr),
		.idata(ppi_idata),
		.odata(ppi_odata),
		.ipa(ppi_ipa),
		.opa(ppi_opa),
		.ipb(ppi_ipb),
		.opb(ppi_opb),
		.ipc(ppi_ipc),
		.opc(ppi_opc),
		.plus_mode(ppi_plus_mode),
		.sna_load(1'b0),
		.sna_opa(8'd0),
		.sna_opb(8'd0),
		.sna_opc(8'd0),
		.sna_control(8'd0)
	);

	plus_sna_parser sna_parser
	(
		.clk(clk),
		.reset(reset),
		.sna_download(sna_download),
		.cpc_plus_chunk_start(cpc_plus_chunk_start),
		.cpc_plus_byte_wr(cpc_plus_byte_wr),
		.cpc_plus_byte_data(cpc_plus_byte_data),
		.ioctl_wait(sna_ioctl_wait),
		.busy(sna_busy),
		.asic_sna_wr(asic_sna_wr),
		.asic_sna_addr(asic_sna_addr),
		.asic_sna_data(asic_sna_data),
		.asic_sna_active(asic_sna_active),
		.asic_sna_rmr2(asic_sna_rmr2),
		.asic_sna_unlock(asic_sna_unlock)
	);

	plus_model_select model_select
	(
		.plus_model(model_plus_model),
		.plus_mode(model_plus_mode),
		.ram_128k(model_ram_128k),
		.has_fdc(model_has_fdc),
		.has_tape(model_has_tape)
	);

	asic_regs aregs
	(
		.clk(clk),
		.reset(seam_plus_asic_reset),

		.asic_cs(aregs_cs),
		.mem_wr(aregs_mem_wr),
		.mem_rd(aregs_mem_rd),
		.A(aregs_addr),
		.D_in(aregs_din),
		.D_out(aregs_dout),

		.leg_border(5'd16),
		.leg_inkr(80'd0),

		.pal_raddr(aregs_pal_raddr),
		.pal_rdata(aregs_pal_rdata),

		.pri(aregs_pri),
		.splt(aregs_splt),
		.sscr(aregs_sscr),
		.ivr(aregs_ivr),
		.ssa_hi(aregs_ssa_hi),
		.ssa_lo(aregs_ssa_lo),
		.dcsr(aregs_dcsr),

		.intack_raster(1'b0),
		.intack(1'b0),
		.int_pending(1'b0),
		.vec_byte(),
		.vec_valid(),

		.dma_int_set(3'd0),

		.sprq_req(1'b0),
		.sprq_addr(11'd0),
		.sprq_data(),
		.sprq_ack(),

		.spr_acc_en(),
		.spr_acc_idx(),
		.spr_wr_en(),
		.spr_wr_addr(),
		.spr_wr_data(),

		.spr_x_view(aregs_spr_x_view),
		.spr_y_view(aregs_spr_y_view),
		.spr_mag_view(aregs_spr_mag_view),
		.spr_pal_view(aregs_spr_pal_view),

		.sar0_lo(aregs_sar0_lo), .sar0_hi(aregs_sar0_hi), .ppr0(aregs_ppr0), .sar0_wr(),
		.sar1_lo(aregs_sar1_lo), .sar1_hi(aregs_sar1_hi), .ppr1(aregs_ppr1), .sar1_wr(),
		.sar2_lo(aregs_sar2_lo), .sar2_hi(aregs_sar2_hi), .ppr2(aregs_ppr2), .sar2_wr(),
		.dcsr_ena_out(),
		.dcsr_ena_clr(3'd0),
		.dma_int_req(),

		.sna_wr(asic_sna_wr),
		.sna_addr(asic_sna_addr),
		.sna_data(asic_sna_data)
	);

	plus_mmu mmu
	(
		.clk(clk),
		.reset(seam_machine_reset),
		.plus_mode(seam_plus_mode),
		.gx4000(1'b0),
		.io_rd(1'b0),
		.io_wr(1'b0),
		.mem_rd(1'b0),
		.A(16'd0),
		.D(8'd0),
		.rom_en(1'b1),
		.exp_n(1'b1),
		.cart_valid(),
		.cart_page(),
		.cart_offset(),
		.cart_ready(1'b0),
		.cart_data(8'd0),
		.cart_busy(1'b0),
		.cart_own(),
		.cart_stall(),
		.cart_dout(),
		.asic_page_on(mmu_asic_page_on),
		.asic_unlocked(mmu_asic_unlocked),
		.sna_load(seam_sna_load),
		.sna_rmr2(asic_sna_rmr2),
		.sna_unlock(asic_sna_unlock)
	);

endmodule
