// Legacy expansion devices keep their loaded image across ordinary CPC
// resets, like physical hardware. Their bus ownership is nevertheless a
// machine-level capability: a previously loaded classic cartridge must not
// claim memory while a Plus/GX4000 cartridge is running.

module plus_legacy_cart_gate
(
	input  clk,
	input  plus_mode,
	input  dandanator_download,
	input  dandanator_detach,
	input  dandanator_nce,
	output dandanator_loaded,
	output dandanator_active
);

	reg loaded;
	reg old_download;
	reg old_detach;

	initial begin
		loaded = 1'b0;
		old_download = 1'b0;
		old_detach = 1'b0;
	end

	always @(posedge clk) begin
		old_download <= dandanator_download;
		old_detach <= dandanator_detach;

		if(old_download & ~dandanator_download) loaded <= 1'b1;
		if(~old_detach & dandanator_detach) loaded <= 1'b0;
	end

	assign dandanator_loaded = loaded;
	assign dandanator_active = ~plus_mode & ~dandanator_nce &
	                            loaded;

endmodule
