module rom_presence_test_top (
    input clk, reset, mark, io_write, sna_load,
    input [1:0] write_bank, read_bank,
    input [7:0] write_slot, select_slot,
    output [8:0] upper_page
);
wire [255:0] present;
wire [22:0] mem_addr;

rom_presence_map presence (
    .clk(clk), .mark(mark), .write_bank(write_bank), .write_slot(write_slot),
    .read_bank(read_bank), .present(present)
);

Amstrad_MMU mmu (
    .CLK(clk), .reset(reset), .ram64k(1'b0), .romen_n(1'b0), .rom_map(present),
    .io_WR(io_write), .D(select_slot), .A(16'hC000), .sna_load(sna_load),
    .sna_ram_config(8'd0), .sna_rom_select(select_slot), .ram_A(mem_addr),
    .snap_rammap(), .snap_rampage(), .snap_rom_select_shadow()
);
assign upper_page = mem_addr[22:14];
endmodule
