module b6_menu_mask_test_top
(
	input  [1:0]  plus_model,
	input         en270p,
	input         clk, reset_apply,
	input  [1:0]  crtc_request,
	input         sna_download_request,
	output        sna_download, sna_hold, sna_load,
	output  [1:0] crtc_type,
	output        ram_128k,
	output [15:0] status_menumask
);

wire plus_mode;
wire plus_has_fdc;
wire plus_has_tape;

plus_model_select model_decode
(
	.plus_model(plus_model),
	.plus_mode(plus_mode),
	.ram_128k(ram_128k),
	.has_fdc(plus_has_fdc),
	.has_tape(plus_has_tape)
);

wire sna_unavailable;

classic_crtc_select crtc_selector
(
    .clk(clk),
    .reset_apply(reset_apply),
    .requested(crtc_request),
    .plus_mode(plus_mode),
    .sna_download_request(sna_download_request),
    .sna_hold(sna_hold),
    .crtc_type(crtc_type),
    .sna_unavailable(sna_unavailable),
    .sna_download(sna_download)
);

wire unused_finish_pending, unused_owner_reset_hold;
wire [2:0] unused_apply_cnt;

plus_sna_apply snapshot_apply
(
    .clk(clk),
    .sna_download(sna_download),
    .romdl_wait(1'b0),
    .boot_wr(1'b0),
    .sna_rle_count(8'd0),
    .plus_sna_busy(1'b0),
    .finish_pending(unused_finish_pending),
    .apply_cnt(unused_apply_cnt),
    .sna_load(sna_load),
    .sna_hold(sna_hold),
    .owner_reset_hold(unused_owner_reset_hold)
);

plus_menu_capability_mask menu_decode
(
	.en270p(en270p),
	.plus_mode(plus_mode),
	.plus_has_fdc(plus_has_fdc),
	.plus_has_tape(plus_has_tape),
	.sna_unavailable(sna_unavailable),
	.status_menumask(status_menumask)
);

endmodule
