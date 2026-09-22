module b16_load_model_test_top (
 input clk,
 input [63:0] status,
 input cpr_image_valid,
 input fn_toggle, cpr_apply, sna_download, sna_header_wr,
 input [7:0] sna_addr, sna_data,
 input drain_busy,
 output [1:0] plus_model,
 output [63:0] status_in,
 output status_set,
 output sna_load, sna_hold, owner_reset_hold
);
 wire [2:0] apply_cnt;
 plus_sna_apply apply_sequence (
  .clk(clk), .sna_download(sna_download), .romdl_wait(drain_busy),
  .boot_wr(1'b0), .sna_rle_count(8'd0), .plus_sna_busy(1'b0),
  .finish_pending(), .apply_cnt(apply_cnt), .sna_load(sna_load),
  .sna_hold(sna_hold), .owner_reset_hold(owner_reset_hold)
 );
 plus_load_model selection (
  .clk(clk), .status(status), .fn_toggle(fn_toggle), .cpr_apply(cpr_apply), .cpr_image_valid(cpr_image_valid),
  .sna_download(sna_download), .sna_header_wr(sna_header_wr),
  .sna_addr(sna_addr), .sna_data(sna_data), .sna_prepare(apply_cnt == 3'd5),
  .plus_model(plus_model), .status_in(status_in), .status_set(status_set)
 );
endmodule
