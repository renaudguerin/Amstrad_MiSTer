// Plus SNA snapshot drain/apply sequencing (B8-5 slice A).
//
// Shares the production Amstrad.sv download -> finish_pending -> apply-count
// transaction with every fixture, so the C++ side never copies the countdown:
// after the download falls, the controller waits for the ordinary snapshot
// writes (!romdl_wait), the SDRAM retire (!boot_wr), the RLE tail
// (!sna_rle_count) and the CPC+ parser FIFO (!plus_sna_busy), then counts
// down 5 -> 0 with a single sna_load pulse at count 1.
//
// Owner reset is held (download / drain / count > 2) and the registered top
// reset therefore releases one cycle before the apply pulse, so the T80
// DIRSet load is never held through reset (T80 RESET has priority over
// DIRSet while DIRSet loads independently of CEN). sna_hold instead gates
// both T80 CEN enables through the load cycle, holding CPU execution until
// the owners settle. A new download rising edge aborts a pending older
// apply (finish/countdown clear) so a stale sna_load can never fire into
// the new image's drain.

module plus_sna_apply
(
	input            clk,
	input            sna_download,
	input            romdl_wait,
	input            boot_wr,
	input      [7:0] sna_rle_count,
	input            plus_sna_busy,

	output reg       finish_pending,
	output reg [2:0] apply_cnt,

	output wire      sna_load,          // single-cycle apply pulse (cnt == 1)
	output wire      sna_hold,          // CPU execution hold through load
	output wire      owner_reset_hold   // combinational owner reset term
);

	assign sna_load         = (apply_cnt == 3'd1) && !sna_download;
	assign sna_hold         = sna_download | old_sna_download |
	                          finish_pending | (apply_cnt != 3'd0);
	assign owner_reset_hold = sna_download | old_sna_download |
	                          finish_pending | (apply_cnt > 3'd2);

	reg old_sna_download;

	always @(posedge clk) begin
		old_sna_download <= sna_download;
		if (sna_download && !old_sna_download) begin
			// New restore aborts any pending older apply.
			finish_pending <= 1'b0;
			apply_cnt <= 3'd0;
		end
		else if (old_sna_download & ~sna_download)
			finish_pending <= 1'b1;
		else if (finish_pending && !romdl_wait && !boot_wr &&
		         (sna_rle_count == 8'd0) && !plus_sna_busy) begin
			finish_pending <= 1'b0;
			apply_cnt <= 3'd5;
		end
		else if (apply_cnt != 3'd0)
			apply_cnt <= apply_cnt - 1'd1;
	end

	initial begin
		finish_pending   = 1'b0;
		apply_cnt        = 3'd0;
		old_sna_download = 1'b0;
	end

endmodule
