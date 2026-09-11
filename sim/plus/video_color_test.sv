// B8-6 integration oracle: a selected pixel and all four metadata bits must
// cross color_mix's one-ce_pix boundary together, then gamma_corr's two
// rising-ce stages. This is a production pipeline contract, not a CRTC rule.
module video_color_test;
reg clk=0;
always #5 clk=~clk;
reg ce16=0, hq=0, fs=0, plus=1;
reg [1:0] mode=0;
reg [2:0] mix=0;
reg [3:0] r=0,g=0,b=0,meta=0;
// B6 raw vertical-blank tag. It must ride the same ce_pix boundary as the
// pixel beside it on every route, and must leave the metadata tuple alone.
reg tag=0, mask_drive=0;
wire ce;
wire [23:0] rgb;
wire [3:0] converted_meta;
amstrad_video_color dut(.CLK_VIDEO(clk),.ce_16(ce16),.hq2x(hq),
    .pixel_vblank(tag),
    .pixel_rate_select(fs),.plus_mode(plus),.mode(mode),.mix(mix),
    .r4(r),.g4(g),.b4(b),.hs(meta[3]),.vs(meta[2]),.hbl(meta[1]),.vbl(meta[0]),
    .ce_pix(ce),.R_out(rgb[23:16]),.G_out(rgb[15:8]),.B_out(rgb[7:0]),
    .HSync(converted_meta[3]),.VSync(converted_meta[2]),
    .HBlank(converted_meta[1]),.VBlank(converted_meta[0]));
wire [23:0] classic_rgb;
wire [3:0] classic_meta;
// The unchanged production DAC is the bit-for-bit classic control, including
// undocumented menu values 6/7 and ignored high nibble bits. No copied table.
color_mix classic(.clk_vid(clk),.ce_pix(ce),.mix(mix),
    .R_in(r[1:0]),.G_in(g[1:0]),.B_in(b[1:0]),
    .HSync_in(meta[3]),.VSync_in(meta[2]),.HBlank_in(meta[1]),.VBlank_in(meta[0]),
    .R_out(classic_rgb[23:16]),.G_out(classic_rgb[15:8]),.B_out(classic_rgb[7:0]),
    .HSync_out(classic_meta[3]),.VSync_out(classic_meta[2]),
    .HBlank_out(classic_meta[1]),.VBlank_out(classic_meta[0]));
reg gamma_en=0, gamma_wr=0;
reg [9:0] gamma_addr=0;
reg [7:0] gamma_value=0;
wire [23:0] out_rgb, classic_out_rgb;
wire [3:0] out_meta, classic_out_meta;
gamma_corr gamma_dut(.clk_sys(clk),.clk_vid(clk),.ce_pix(ce),
    .gamma_en(gamma_en),.gamma_wr(gamma_wr),.gamma_wr_addr(gamma_addr),.gamma_value(gamma_value),
    .RGB_in(rgb),.HSync(converted_meta[3]),.VSync(converted_meta[2]),
    .HBlank(converted_meta[1]),.VBlank(converted_meta[0]),.RGB_out(out_rgb),
    .HSync_out(out_meta[3]),.VSync_out(out_meta[2]),.HBlank_out(out_meta[1]),.VBlank_out(out_meta[0]));
gamma_corr gamma_classic(.clk_sys(clk),.clk_vid(clk),.ce_pix(ce),
    .gamma_en(gamma_en),.gamma_wr(gamma_wr),.gamma_wr_addr(gamma_addr),.gamma_value(gamma_value),
    .RGB_in(classic_rgb),.HSync(classic_meta[3]),.VSync(classic_meta[2]),
    .HBlank(classic_meta[1]),.VBlank(classic_meta[0]),.RGB_out(classic_out_rgb),
    .HSync_out(classic_out_meta[3]),.VSync_out(classic_out_meta[2]),
    .HBlank_out(classic_out_meta[1]),.VBlank_out(classic_out_meta[0]));

// Arithmetic specification: native nibbles expand by 17; monitor luma is
// floor((59 R + 177 G + 20 B)/256). Amber green is Y-floor(sum/1024).
// This independent integer oracle checks the real production converter.
function automatic [23:0] native_pixel(input integer rr,gg,bb,option_n);
    integer red,green,blue,sum,y;
    begin
        red=rr*17; green=gg*17; blue=bb*17;
        sum=59*red+177*green+20*blue; y=sum/256;
        case(option_n)
            2: begin red=0; green=y; blue=0; end
            3: begin red=y; green=y-sum/1024; blue=0; end
            4: begin red=0; green=y; blue=y; end
            5: begin red=y; green=y; blue=y; end
            default: begin end
        endcase
        native_pixel={8'(red),8'(green),8'(blue)};
    end
endfunction
reg checking=0;
integer errors=0, classic_errors=0, metadata_errors=0, cadence_errors=0;
integer checks=0, count=0, ce_count=0, last_ce=-1;
integer expected_period=4;
reg [23:0] p0=0,p1=0,p2=0;
reg [3:0] m0=0,m1=0,m2=0;
reg t0=0,t1=0,t2=0;
// Masked black is defined at the gamma input, so an enabled transfer function
// still applies to it: this table inverts, hence all-ones downstream.
wire [23:0] masked_gamma = gamma_en ? 24'hffffff : 24'd0;
reg pre_ce;
task automatic require_equal(input bit condition,input string what);
    if(!condition) begin
        if(errors<8) $display("FAIL %s plus=%0d hq=%0d fs=%0d mode=%0d mix=%0d gamma=%0d cycle=%0d rgb=%h expected=%h meta=%h expected_meta=%h",
            what,plus,hq,fs,mode,mix,gamma_en,count,out_rgb,p2,out_meta,m2);
        errors++;
        if(!plus) classic_errors++;
        if(what=="metadata latency/hold") metadata_errors++;
        if(what=="pixel-enable cadence") cadence_errors++;
    end
endtask
// Compute expected tuples from inputs before the clock, never from DUT RGB.
always @(posedge clk) begin
    pre_ce=ce;
    if(checking) begin
        count++;
        if(pre_ce) begin
            if(last_ce>=0) require_equal(count-last_ce==expected_period,"pixel-enable cadence");
            last_ce=count; ce_count++;
            p2=gamma_en ? ~p1 : p1; m2=m1;
            p1=p0; m1=m0;
            p0=native_pixel(int'(r),int'(g),int'(b),int'(mix)); m0=meta;
            t2=t1; t1=t0; t0=tag;
        end
        #1;
        if(ce_count>4) begin
            checks++;
            require_equal(converted_meta==m0 && out_meta==m2,"metadata latency/hold");
            if(plus) begin
                require_equal(rgb==(t0?24'd0:p0),"Plus conversion latency/hold");
                require_equal(out_rgb==(t2?masked_gamma:p2),"Plus gamma pixel identity");
            end else begin
                require_equal(rgb==(t0?24'd0:classic_rgb) && converted_meta==classic_meta,"classic converter equivalence");
                require_equal(out_rgb==(t2?masked_gamma:classic_out_rgb) && out_meta==classic_out_meta,"classic gamma equivalence");
            end
        end
    end
end
integer tick_n=0;
task automatic tick(input bit frame_sync);
    @(negedge clk);
    ce16=(tick_n%4==0);
    // Changes during disabled clocks catch accidental ungated sampling too.
    r=4'(tick_n*7+tick_n/4+1); g=4'(tick_n*3+tick_n/7+2); b=4'(tick_n*11+tick_n/13+5);
    // Period 5 shares no factor with any enabled-pixel period (4/8/16), so a
    // tag that lands one enabled edge early or late cannot stay hidden.
    tag=mask_drive & 1'(tick_n/5);
    meta={1'(tick_n/17), (frame_sync | (checking && !hq && !fs && tick_n%131<17)),
        1'(tick_n/23),1'(tick_n/37)};
    tick_n++;
endtask
initial begin
    // Inverting every channel makes enabled-gamma checks distinguish bypass.
    for(integer i=0;i<768;i++) begin
        @(negedge clk); gamma_wr=1; gamma_addr=10'(i); gamma_value=8'(255-(i%256));
    end
    @(negedge clk); gamma_wr=0;
    for(integer route=0;route<3;route++) begin
        hq=(route==1); fs=(route==2);
        for(integer md=0;md<4;md++) begin
            mode=2'(md);
            // Acquire highest frame mode through the actual production state
            // machine. Two frame boundaries transfer mode_next to mode_fs.
            repeat(3) begin
                repeat(256) tick(0);
                repeat(16) tick(1);
            end
            expected_period=(route==0) ? 4 : ((md==2) ? 4 : ((md==1) ? 8 : 16));
            for(integer machine=0;machine<2;machine++) begin
                plus=1'(machine);
                for(integer option_n=0;option_n<8;option_n++) begin
                    mix=3'(option_n);
                    // One colour option per machine/route/mode carries the
                    // B6 mask; the other seven keep the unmasked contract as
                    // the compatibility control.
                    mask_drive=(option_n==0);
                    for(integer gamma_mode=0;gamma_mode<2;gamma_mode++) begin
                        gamma_en=1'(gamma_mode);
                        count=0;ce_count=0;last_ce=-1;checking=1;
                        repeat(512) tick(0);
                        @(negedge clk); checking=0;
                    end
                end
            end
            $display("checked route=%0d mode=%0d period=%0d",route,md,expected_period);
        end
    end
    $display("B8-6 checks=%0d errors=%0d",checks,errors);
    $display("classic_errors=%0d metadata_errors=%0d cadence_errors=%0d",classic_errors,metadata_errors,cadence_errors);
    if(errors!=0) $fatal(1,"video colour boundary contract violated");
    $finish;
end
endmodule
