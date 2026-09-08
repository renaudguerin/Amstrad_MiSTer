// Tape download write queue: narrow production seam for CDT download bytes.
//
// Shared by Amstrad.sv and the physical-DQ SDRAM fixture so the producer/
// address-ownership contract executes in both places (B8-7). Fixed contract:
//
//  - tape_wr is held until the producer synchronously samples tape_wr_ack
//    (sdram.v acknowledges a write at STATE_READ, one slot phase before the
//    next IDLE arbitration, so the cleared request is what the next grant
//    sees — the same lifetime as the cartridge client);
//  - the accepted address AND payload latch together at admission in sdram.v,
//    so a next payload can never corrupt a queued or re-admitted byte;
//  - while a byte is outstanding (held, not acking) tape_wait stalls the
//    sender so the next strobe cannot overwrite the queued byte: sys/sys_top.v
//    freezes rack/io_ack under ioctl_wait and sys/hps_io.sv emits each
//    strobe+address-advance once with no retry of a busy ioctl_wr, so the
//    sender honors backpressure and a strobe ignored while busy would be lost.
//    In the ACK cycle wait releases and a simultaneous ioctl strobe is
//    accepted as the next byte (last assignment wins, matching the historical
//    simultaneous ack+payload phase);
//  - the drain owns its pending address: the accepted address latches into a
//    separate pending register at accept time and the owned mux drains that
//    pending tuple unchanged, even if Fn[2] clear resets the logical
//    last-address metadata first. Reset dominates a simultaneous strobe and
//    clears the held request, both address registers, and the payload, so it
//    cannot accept a new byte. A write already admitted by SDRAM may finish.
//  - tape_pending holds the playback reset until the drain completes, even if
//    tape_download falls first. Tape reads are untouched by this helper.

module tape_write_queue
(
	input             clk,
	input             reset,          // machine reset: dominates, clears all
	input             clear,          // reset | Fn[2]: clears logical metadata
	input             tape_download,
	input             ioctl_wr,
	input       [7:0] ioctl_dout,
	input      [22:0] ioctl_addr,
	input             tape_wr_ack,    // synchronous acknowledge from sdram.v
	input      [22:0] tape_play_addr, // parked playback address (0 during download)
	output reg        tape_wr,        // held write request to sdram.v
	output reg  [7:0] tape_din,       // pending payload (drains unchanged)
	output reg [22:0] tape_queued_addr, // logical last accepted download address
	output     [22:0] tape_addr,      // owned address to sdram.v
	output            tape_wait,      // ioctl backpressure
	output            tape_pending    // outstanding write (held request)
);

	reg [22:0] pending_addr; // accepted drain address, immune to clear

	assign tape_addr = tape_wr ? pending_addr :
	                   (tape_download ? tape_queued_addr : tape_play_addr);
	assign tape_wait = tape_download & tape_wr & ~tape_wr_ack;
	assign tape_pending = tape_wr;

	initial begin
		tape_wr = 1'b0;
		tape_din = 8'h00;
		tape_queued_addr = 23'd0;
		pending_addr = 23'd0;
	end

	always @(posedge clk) begin
		if (reset) begin
			tape_wr <= 1'b0;
			tape_din <= 8'h00;
			tape_queued_addr <= 23'd0;
			pending_addr <= 23'd0;
		end
		else begin
			if (tape_wr_ack)
				tape_wr <= 1'b0;
			// Accept only when idle or when the previous byte drains this
			// same cycle. A strobe while busy (wait asserted) is ignored;
			// the sender stalls on wait, so no retry is expected.
			if (tape_download && ioctl_wr && (!tape_wr || tape_wr_ack)) begin
				tape_wr <= 1'b1;
				tape_din <= ioctl_dout;
				pending_addr <= ioctl_addr;
				tape_queued_addr <= ioctl_addr;
			end
			// Clear (Fn[2]) affects only the logical metadata; the pending
			// tuple drains unchanged. Ordered after accept so a same-cycle
			// clear still leaves the drain intact while metadata reads 0.
			if (clear)
				tape_queued_addr <= 23'd0;
		end
	end

endmodule
