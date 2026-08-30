// P4 production seam fixture: real asic_regs sprite page services directly
// connected to the real asic_sprites row-fetch port.  The C++ bench drives the
// CPU ASIC-page bus and CRTC taps so the registered shared-port handshake and
// stale-row emission gate are exercised together.
module p4_sprites_regs_test_top (
	input         clk,
	input         reset,

	// CRTC/video timing taps into the production sprite engine.
	input  [6:0]  line,
	input  [4:0]  row,
	input         pixen,
	input         clken,
	input         hwrap,

	// CPU memory bus into the enabled ASIC page.
	input         asic_cs,
	input         mem_wr,
	input         mem_rd,
	input  [13:0] A,
	input   [7:0] D_in,
	output  [7:0] D_out,

	// Direct probes of the connected row-fetch handshake.
	output        fq_req,
	output [10:0] fq_addr,
	output        fq_ack,
	output  [7:0] fq_data,

	// Direct probes of sprite emission.
	output        spr_en,
	output [11:0] spr_rgb,
	output  [3:0] spr_idx,
	output [15:0] spr_win,

	// Attribute probes make page-write programming observable without
	// bypassing the register file.
	output [159:0] spr_x_view,
	output [143:0] spr_y_view,
	output  [63:0] spr_mag_view,
	output [179:0] spr_pal_view,
	output  [9:0] spr0_x,
	output  [8:0] spr0_y,
	output  [3:0] spr0_mag,
	output [11:0] spr0_pal3
);

	wire [4:0]  pal_raddr = 5'd0;
	/* verilator lint_off UNUSEDSIGNAL */
	wire [11:0] pal_rdata;
	wire [7:0]  pri;
	wire [7:0]  splt;
	wire [7:0]  sscr;
	wire [7:0]  ivr;
	wire [7:0]  ssa_hi;
	wire [7:0]  ssa_lo;
	wire [7:0]  dcsr;
	wire        intack_raster = 1'b0;
	wire        intack = 1'b0;
	wire        int_pending = 1'b0;
	wire [2:0]  dma_int_set = 3'd0;
	wire [2:0]  dcsr_ena_clr = 3'd0;
	wire [7:0]  vec_byte;
	wire        vec_valid;
	wire [7:0]  sar0_lo;
	wire [7:0]  sar0_hi;
	wire [7:0]  ppr0;
	wire        sar0_wr;
	wire [7:0]  sar1_lo;
	wire [7:0]  sar1_hi;
	wire [7:0]  ppr1;
	wire        sar1_wr;
	wire [7:0]  sar2_lo;
	wire [7:0]  sar2_hi;
	wire [7:0]  ppr2;
	wire        sar2_wr;
	wire [2:0]  dcsr_ena_out;
	wire        dma_int_req;
	/* verilator lint_on UNUSEDSIGNAL */
	wire        sna_wr = 1'b0;
	wire [13:0] sna_addr = 14'd0;
	wire [7:0]  sna_data = 8'd0;

	wire        spr_acc_en;
	wire [3:0]  spr_acc_idx;
	wire        spr_wr_en;
	wire [11:0] spr_wr_addr;
	wire [3:0]  spr_wr_data;
	wire [10:0] sprq_addr;
	wire        sprq_req;
	wire [7:0]  sprq_data;
	wire        sprq_ack;

	// Legacy palette inputs are deliberately held at their reset values.  The
	// fixture only programs sprite colours 1..15 through the ASIC page.
	wire [4:0]  leg_border = 5'd0;
	wire [79:0] leg_inkr = 80'd0;

	asic_regs regs (
		.clk(clk),
		.reset(reset),
		.asic_cs(asic_cs),
		.mem_wr(mem_wr),
		.mem_rd(mem_rd),
		.A(A),
		.D_in(D_in),
		.D_out(D_out),
		.leg_border(leg_border),
		.leg_inkr(leg_inkr),
		.pal_raddr(pal_raddr),
		.pal_rdata(pal_rdata),
		.pri(pri),
		.splt(splt),
		.sscr(sscr),
		.ivr(ivr),
		.ssa_hi(ssa_hi),
		.ssa_lo(ssa_lo),
		.dcsr(dcsr),
		.intack_raster(intack_raster),
		.intack(intack),
		.int_pending(int_pending),
		.vec_byte(vec_byte),
		.vec_valid(vec_valid),
		.dma_int_set(dma_int_set),
		.sprq_req(sprq_req),
		.sprq_addr(sprq_addr),
		.sprq_data(sprq_data),
		.sprq_ack(sprq_ack),
		.spr_acc_en(spr_acc_en),
		.spr_acc_idx(spr_acc_idx),
		.spr_wr_en(spr_wr_en),
		.spr_wr_addr(spr_wr_addr),
		.spr_wr_data(spr_wr_data),
		.spr_x_view(spr_x_view),
		.spr_y_view(spr_y_view),
		.spr_mag_view(spr_mag_view),
		.spr_pal_view(spr_pal_view),
		.sar0_lo(sar0_lo),
		.sar0_hi(sar0_hi),
		.ppr0(ppr0),
		.sar0_wr(sar0_wr),
		.sar1_lo(sar1_lo),
		.sar1_hi(sar1_hi),
		.ppr1(ppr1),
		.sar1_wr(sar1_wr),
		.sar2_lo(sar2_lo),
		.sar2_hi(sar2_hi),
		.ppr2(ppr2),
		.sar2_wr(sar2_wr),
		.dcsr_ena_out(dcsr_ena_out),
		.dcsr_ena_clr(dcsr_ena_clr),
		.dma_int_req(dma_int_req),
		.sna_wr(sna_wr),
		.sna_addr(sna_addr),
		.sna_data(sna_data)
	);

	asic_sprites sprites (
		.CLOCK(clk),
		.PIXEN(pixen),
		.CLKEN(clken),
		.HWRAP(hwrap),
		.nRESET(!reset),
		.LINE(line),
		.ROW(row),
		.SPR_X(spr_x_view),
		.SPR_Y(spr_y_view),
		.SPR_MAG(spr_mag_view),
		.SPR_PAL(spr_pal_view),
		.ACC_EN(spr_acc_en),
		.ACC_IDX(spr_acc_idx),
		.spr_wr_en(spr_wr_en),
		.spr_wr_addr(spr_wr_addr),
		.spr_wr_data(spr_wr_data),
		.FQ_REQ(sprq_req),
		.FQ_ADDR(sprq_addr),
		.FQ_DATA(sprq_data),
		.FQ_ACK(sprq_ack),
		.SPR_EN(spr_en),
		.SPR_RGB(spr_rgb),
		.SPR_IDX(spr_idx),
		.SPR_WIN(spr_win)
	);

	assign fq_req  = sprq_req;
	assign fq_addr = sprq_addr;
	assign fq_ack  = sprq_ack;
	assign fq_data = sprq_data;
	assign spr0_x = spr_x_view[9:0];
	assign spr0_y = spr_y_view[8:0];
	assign spr0_mag = spr_mag_view[3:0];
	assign spr0_pal3 = spr_pal_view[35:24];

endmodule
