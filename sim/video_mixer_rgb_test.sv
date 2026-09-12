`timescale 1ns/1ps

// Exercise production video_mixer and all its real dependencies. The scaler
// remains bypassed: this test owns RGB width/scope, freeze and CE staging only.
module mixer_rgb_case #(parameter GAMMA = 0, HALF_DEPTH = 0)
(output reg done = 0, output integer errors = 0);
    localparam INPUT_BITS = HALF_DEPTH ? 4 : 8;
    reg clk = 0, ce = 0, freeze = 0;
    reg [7:0] red = 0, green = 0, blue = 0;
    reg gamma_clk = 0, gamma_en = 0, gamma_wr = 0;
    reg [9:0] gamma_addr = 0;
    reg [7:0] gamma_value = 0;
    wire [21:0] gamma_bus;
    assign gamma_bus[20:0] = {gamma_clk,gamma_en,gamma_wr,gamma_addr,gamma_value};
    wire [7:0] r,g,b;
    wire out_ce;
    video_mixer #(.LINE_LENGTH(64), .GAMMA(GAMMA), .HALF_DEPTH(HALF_DEPTH)) dut (
        .CLK_VIDEO(clk), .CE_PIXEL(out_ce), .ce_pix(ce),
        .scandoubler(1'b0), .hq2x(1'b0), .gamma_bus(gamma_bus),
        .R(red[INPUT_BITS-1:0]),
        .G(green[INPUT_BITS-1:0]),
        .B(blue[INPUT_BITS-1:0]),
        .HSync(1'b0), .VSync(1'b0), .HBlank(1'b0), .VBlank(1'b0),
        .HDMI_FREEZE(freeze), .freeze_sync(),
        .VGA_R(r), .VGA_G(g), .VGA_B(b), .VGA_VS(), .VGA_HS(), .VGA_DE()
    );
    task tick;
        #5; clk = 1; #5; clk = 0;
    endtask
    task check_rgb(input [23:0] expected, input string label);
        if ({r,g,b} !== expected) begin
            $display("FAIL GAMMA=%0d HALF_DEPTH=%0d %s RGB=%06x expected=%06x",
                     GAMMA, HALF_DEPTH, label, {r,g,b}, expected);
            errors = errors + 1;
        end
    endtask
    function automatic [23:0] expanded(input [23:0] rgb);
        expanded = HALF_DEPTH ? {rgb[19:16],rgb[19:16],rgb[11:8],rgb[11:8],rgb[3:0],rgb[3:0]} : rgb;
    endfunction
    // Distinct channel maps detect both width loss and channel misalignment.
    function automatic [23:0] corrected(input [23:0] rgb);
        corrected = {rgb[23:16] ^ 8'h96, rgb[15:8] ^ 8'h3c, rgb[7:0] ^ 8'ha5};
    endfunction
    reg [23:0] previous_sample = 0;
    reg [23:0] last_output = 0;
    reg [23:0] gamma_pending = 0;
    bit primed = 0;
    task pixel(input [23:0] rgb, input string label);
        reg [23:0] sample, expected;
        {red,green,blue} = rgb;
        ce = 0;
        // At least four clocks for gamma's three-channel lookup, and enough
        // time for the two-flop freeze synchronizer and mixer input register.
        repeat (6) begin
            tick();
            if (primed) check_rgb(last_output, {label," CE-low hold"});
        end
        sample = freeze ? 24'd0 : expanded(rgb);
        expected = GAMMA ? gamma_pending : sample;
        gamma_pending = gamma_en ? corrected(previous_sample) : previous_sample;
        ce = 1; tick();
        if (primed) check_rgb(last_output, {label," CE edge holds output"});
        if (out_ce !== 1'b1) begin errors++; $display("FAIL missing CE_PIXEL"); end
        ce = 0; tick();
        // video_mixer captures RGB on the clock after CE_PIXEL is raised.
        // At that capture the mixer RGB register still holds the pre-edge
        // gamma output: gamma therefore contributes two sampled-pixel stages.
        check_rgb(expected, label);
        if (out_ce !== 1'b0) begin errors++; $display("FAIL CE_PIXEL did not fall"); end
        last_output = expected;
        previous_sample = sample;
        primed = 1;
    endtask
    initial begin
        // Program the actual gamma RAM, through its real write interface.
        for (integer channel = 0; channel < 3; channel++) begin
            for (integer value = 0; value < 256; value++) begin
                gamma_addr = 10'(channel * 256 + value);
                gamma_value = 8'(value) ^ (channel == 0 ? 8'h96 : channel == 1 ? 8'h3c : 8'ha5);
                gamma_wr = 1; #1; gamma_clk = 1; #1; gamma_clk = 0;
            end
        end
        gamma_wr = 0;
        pixel(24'h000000, "warmup");
        pixel(24'ha63de9, "distinct channels");
        pixel(24'hf18254, "successive pixel");
        pixel(24'h804020, "high bits without bit zero");
        freeze = 1;
        pixel(24'hffffff, "freeze entry");
        pixel(24'h4d2b97, "freeze settled black");
        freeze = 0;
        pixel(24'h5a6c93, "freeze release");
        pixel(24'h17e2b4, "released colour");
        gamma_en = 1;
        pixel(24'hd42873, "enabled gamma preceding pixel");
        pixel(24'h7ba15e, "enabled gamma distinct channels");
        pixel(24'h000000, "gamma flush");
        $display("mixer RGB GAMMA=%0d HALF_DEPTH=%0d: %0d errors", GAMMA, HALF_DEPTH, errors);
        done = 1;
    end
endmodule

module video_mixer_rgb_test;
    wire [3:0] done;
    wire integer e00,e01,e10,e11;
    mixer_rgb_case #(.GAMMA(0),.HALF_DEPTH(0)) c00(done[0],e00);
    mixer_rgb_case #(.GAMMA(0),.HALF_DEPTH(1)) c01(done[1],e01);
    mixer_rgb_case #(.GAMMA(1),.HALF_DEPTH(0)) c10(done[2],e10);
    mixer_rgb_case #(.GAMMA(1),.HALF_DEPTH(1)) c11(done[3],e11);
    initial begin
        wait (&done);
        if ((e00+e01+e10+e11) != 0) $fatal(1,"production mixer RGB regression: %0d errors",e00+e01+e10+e11);
        $display("PASS production mixer RGB: all GAMMA/HALF_DEPTH combinations, freeze, CE latency and gamma LUT");
        $finish;
    end
endmodule
