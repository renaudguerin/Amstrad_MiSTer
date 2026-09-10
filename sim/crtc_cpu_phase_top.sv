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
 output [7:0] seq, c0,
 output [6:0] row,
 output [4:0] line, r5,
 output pending, in_adj, vde, vde_r,
 output ce16, phi_p, cclk_n, write_active, arm, vma_flag, parity_flag,
 output [13:0] ma,
 output [7:0] bus_data
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
 wire cn, cp, hs, vs, de;
 assign seq=ga.S;
 assign ce16=ce_16;
 assign cclk_n=cn;
 assign write_active=selected_write;
 assign bus_data=selected_data;
 assign c0=crtc.hcc;
 assign row=crtc.row;
 assign line=crtc.line;
 assign r5=crtc.R5_v_total_adj;
 assign pending=crtc.crtc_type1_engine.rfd_r0_pending;
 assign in_adj=crtc.in_adj;
 assign vde=crtc.vde;
 assign vde_r=crtc.vde_r;
 assign arm=crtc.crtc_type1_engine.rfd_arm;
 assign vma_flag=crtc.crtc_type1_engine.rfd_vma_flag;
 assign parity_flag=crtc.crtc_type1_engine.rfd_parity_flag;
 ga40010 ga(
   .clk(clk), .cen_16(ce_16), .fast(1'b0), .RESET_N(~reset),
   .A(2'b10), .D(selected_data), .MREQ_N(1'b1), .M1_N(1'b1),
   .RD_N(1'b1), .IORQ_N(~selected_write),
   .HSYNC_I(hs), .VSYNC_I(vs), .DISPEN(de),
   .CCLK_EN_P(cp), .CCLK_EN_N(cn), .PHI_EN_P(phi_p),
   .PHI_EN_N(), .PHI_N(), .CCLK(), .RAS_N(), .CAS_N(),
   .CASAD_N(), .READY(), .CPU_N(), .MWE_N(), .E244_N(), .ROMEN_N(),
   .RAMRD_N(), .ROM(), .MODE(), .HSYNC_O(), .VSYNC_O(), .SYNC_N(),
   .INT_N(), .VBLANK(), .BLUE_OE_N(), .BLUE(), .GREEN_OE_N(), .GREEN(),
   .RED_OE_N(), .RED(),
   .SNA_LOAD(1'b0), .SNA_INKSEL(5'd0), .SNA_PALETTE(136'd0), .SNA_CONFIG(8'd0)
 );
 CRTC crtc(
   .CLOCK(clk), .CLKEN(cn), .nCLKEN(cp), .nRESET(~reset), .CRTC_TYPE(crtc_type),
   .ENABLE(selected_write), .nCS(1'b0), .R_nW(1'b0), .RS(bus_rs),
   .DI(selected_data), .DO(),
   .SNA_LOAD(sna_load), .SNA_ADDR(sna_addr), .SNA_REGS(sna_regs),
   .VSYNC(vs), .HSYNC(hs), .DE(de), .FIELD(), .CURSOR(), .MA(ma), .RA()
 );
endmodule
