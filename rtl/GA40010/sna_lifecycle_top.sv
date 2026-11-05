// Public-pin GA snapshot normalization probe; no production control seam.
module sna_lifecycle_top(input clk,reset_n,raw_hs,sna_load,display,
 input[1:0]sna_mode,input[7:0]ram_byte,input[135:0]palette,
 output[1:0]requested,active,
 output[5:0]rgb,output[4:0]hcnt,
 output ce16,load,mode_event,prefetch,buffered_display,buffer_valid);
 reg[1:0]div=0;
 reg ce16_r=0;
 always @(posedge clk)begin div<=div+1'b1;ce16_r<=div==0;end
 assign ce16=ce16_r;
 wire red,red_oe,green,green_oe,blue,blue_oe;
 assign rgb={red,red_oe,green,green_oe,blue,blue_oe};
 assign active=ga.video_sync.mode_is_0?0:ga.video_sync.mode_is_1?1:ga.video_sync.mode_is_2?2:3;
 assign load=ga.video_sync.load;
 assign prefetch=ga.vidbuf_clk_en;
 assign buffered_display=ga.DISPEN_BUF;
 assign buffer_valid=ga.VIDEO_BUF_VALID;
 assign mode_event=ga.mode_sync_en;
 ga40010 ga(
 .CCLK(),
 .CCLK_EN_P(),
 .CCLK_EN_N(),
 .PHI_N(),
 .PHI_EN_N(),
 .PHI_EN_P(),
 .RAS_N(),
 .CAS_N(),
 .READY(),
 .CASAD_N(),
 .CPU_N(),
 .MWE_N(),
 .E244_N(),
 .ROMEN_N(),
 .RAMRD_N(),
 .ROM(),
 .MODE(requested),
 .HSYNC_O(),
 .VSYNC_O(),
 .SYNC_N(),
 .INT_N(),
 .VBLANK(),
 .BLUE_OE_N(blue_oe),
 .BLUE(blue),
 .GREEN_OE_N(green_oe),
 .GREEN(green),
 .RED_OE_N(red_oe),
 .RED(red),
 .SNAP_INKSEL(),
 .SNAP_BORDER(),
 .SNAP_INKR(),
 .SNAP_HROMEN(),
 .SNAP_LROMEN(),
 .SNAP_MODE(),
 .SNAP_INTCNT(),
 .SNAP_HCNT(hcnt),
 .clk(clk),
 .cen_16(ce16),
 .clk_16(div[1]),
 .fast(1'b0),
 .RESET_N(reset_n),
 .A(2'd0),
 .D(ram_byte),
 .MREQ_N(1'b1),
 .M1_N(1'b1),
 .RD_N(1'b1),
 .IORQ_N(1'b1),
 .HSYNC_I(raw_hs),
 .HSYNC_RESTART_I(1'b0),
 .VSYNC_I(1'b0),
 .DISPEN(display),
 .SNA_LOAD(sna_load),
 .SNA_INKSEL(5'd0),
 .SNA_PALETTE(palette),
 .SNA_CONFIG({6'b100011,sna_mode})
 );
endmodule
