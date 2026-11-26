// SPDX-License-Identifier: GPL-2.0-or-later
module rom_presence_map (
    input clk,
    input mark,
    input [1:0] write_bank,
    input [7:0] write_slot,
    input [1:0] read_bank,
    output [255:0] present
);

wire [255:0] slots [0:2];
genvar bank;
generate
    for (bank = 0; bank < 3; bank = bank + 1) begin : model
        localparam [1:0] BANK = bank;
        reg [255:0] loaded = 0;
        always @(posedge clk) begin
            if (mark && write_bank == BANK) loaded[write_slot] <= 1'b1;
        end
        assign slots[bank] = loaded;
    end
endgenerate

assign present = read_bank == 2'd3 ? 256'd0 : slots[read_bank];

endmodule
