// Production GA/CRTC bus-phase regression; no copied timing/rule logic.
// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND).
// Setup uses SNA/reset; measured writes use a scripted CEN_p/NBA bus launch.
module crtc_cpu_phase_top(
 input clk, reset, sna_load, crtc_type, bus_rs,
 input [4:0] sna_addr,
 input [7:0] write_data,
 input [143:0] sna_regs,
 input launch, release_bus, direct_write,
 input [7:0] launch_s,
 input video_enable,
 input [7:0] video_data,
 output [7:0] seq, c0,
 output [6:0] row,
 output [4:0] line, r5,
 output [3:0] c3,
 output raw_hsync,
 output pending, in_adj, vde, vde_r, de_out,
 output ce16, phi_p, cclk_p, cclk_n, write_active, arm, vma_flag, parity_flag,
 output [13:0] ma,
 output [7:0] bus_data,
 output [5:0] dac_rgb
);
 reg [2:0] div=0;
 reg ce_16=0;
 always @(posedge clk) begin
   div <= div + 1'b1;
   ce_16 <= (div[1:0] == 0); // identical to Amstrad.sv
 end
 reg iorq_n=1, wr_n=1;
 reg [7:0] data=0;
 always @(posedge clk) begin
   if(reset || release_bus) begin iorq_n<=1; wr_n<=1; end
   else if(launch && phi_p && seq==launch_s) begin
     // Same launch edge as T80pa's TState=1 I/O branch. No CPU execution.
     iorq_n<=0; wr_n<=0; data<=write_data;
   end
 end
 wire [7:0] selected_data = direct_write ? write_data : data;
 wire selected_write = direct_write || !(iorq_n | wr_n);
 wire cn, cp, hs, vs, de, de_sample_next;
 wire hs_restart;
 wire red, green, blue, red_oe_n, green_oe_n, blue_oe_n;
 assign dac_rgb = {red, red_oe_n, green, green_oe_n, blue, blue_oe_n};
 assign seq=ga.S;
 assign ce16=ce_16;
 assign cclk_p=cp;
 assign cclk_n=cn;
 assign write_active=selected_write;
 assign bus_data=selected_data;
 assign c0=crtc.hcc;
 assign c3=crtc.hsc;
 assign raw_hsync=hs;
 assign row=crtc.row;
 assign line=crtc.line;
 assign r5=crtc.R5_v_total_adj;
 assign pending=crtc.crtc_type1_engine.rfd_r0_pending;
 assign in_adj=crtc.in_adj;
 assign de_out=de;
 assign vde=crtc.vde;
 assign vde_r=crtc.vde_r;
 assign arm=crtc.crtc_type1_engine.rfd_arm;
 assign vma_flag=crtc.crtc_type1_engine.rfd_vma_flag;
 assign parity_flag=crtc.crtc_type1_engine.rfd_parity_flag;
 ga40010 ga(
   .clk(clk), .cen_16(ce_16), .fast(1'b0), .RESET_N(~reset),
   .A(2'b10), .D(video_enable ? video_data : selected_data),
   .MREQ_N(1'b1), .M1_N(1'b1),
   .RD_N(1'b1), .IORQ_N(~selected_write),
   .HSYNC_I(hs), .HSYNC_RESTART_I(hs_restart), .VSYNC_I(vs), .DISPEN(de), .DISPEN_SAMPLE_NEXT(de_sample_next),
   .CCLK_EN_P(cp), .CCLK_EN_N(cn), .PHI_EN_P(phi_p),
   .PHI_EN_N(), .PHI_N(), .CCLK(), .RAS_N(), .CAS_N(),
   .CASAD_N(), .READY(), .CPU_N(), .MWE_N(), .E244_N(), .ROMEN_N(),
   .RAMRD_N(), .ROM(), .MODE(), .HSYNC_O(), .VSYNC_O(), .SYNC_N(),
   .INT_N(), .VBLANK(), .BLUE_OE_N(blue_oe_n), .BLUE(blue),
   .GREEN_OE_N(green_oe_n), .GREEN(green), .RED_OE_N(red_oe_n), .RED(red),
   .SNA_LOAD(video_enable && sna_load), .SNA_INKSEL(5'd0),
   .SNA_PALETTE({8'h1f, 112'd0, 8'h06, 8'h0b}), .SNA_CONFIG(8'h02)
 );
 CRTC crtc(
   .CLOCK(clk), .CLKEN(cn), .nCLKEN(cp), .nRESET(~reset), .CRTC_TYPE(crtc_type),
   .ENABLE(selected_write), .nCS(1'b0), .R_nW(1'b0), .RS(bus_rs),
   .DI(selected_data), .DO(),
   .SNA_LOAD(sna_load), .SNA_ADDR(sna_addr), .SNA_REGS(sna_regs),
   .VSYNC(vs), .HSYNC(hs), .HSYNC_RESTART(hs_restart), .DE(de), .DE_SAMPLE_NEXT(de_sample_next), .FIELD(), .CURSOR(), .MA(ma), .RA()
 );
endmodule
