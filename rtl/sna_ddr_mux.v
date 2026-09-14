//============================================================================
//  Two-master DDR3 Avalon-style bus arbiter and multiplexer (B18 slice 4b).
//
//  Master A: SSM marker (default owner).
//  Master B: SNA snapshot save stream (claims bus via b_request/b_grant).
//
//  Ownership changes only when the current owner has no write outstanding:
//  - B is granted when b_request is asserted and A is not holding a_we.
//  - While B owns the bus, A sees a_busy = 1 to hold any pending write stalled.
//  - B releases by dropping b_request when b_we is deasserted.
//  - Ownership returns to A on the next clock.
//
//  Reference: docs/b18-sna-save.md, "Transport", "SSM coexistence".
//============================================================================

module sna_ddr_mux
(
	input             clk,
	input             reset,

	// Master A: SSM marker (default owner)
	input      [28:0] a_addr,
	input      [63:0] a_din,
	input       [7:0] a_be,
	input       [7:0] a_burstcnt,
	input             a_we,
	output            a_busy,

	// Master B: SNA save stream
	input             b_request,
	output reg        b_grant,
	input      [28:0] b_addr,
	input      [63:0] b_din,
	input       [7:0] b_be,
	input       [7:0] b_burstcnt,
	input             b_we,
	output            b_busy,

	// Slave DDR3 port
	output     [28:0] ddram_addr,
	output     [63:0] ddram_din,
	output      [7:0] ddram_be,
	output      [7:0] ddram_burstcnt,
	output            ddram_we,
	input             ddram_busy
);

	always @(posedge clk) begin
		// Reset never switches the slave away from a write B still holds:
		// the stream keeps a stalled write through its own reset until the
		// slave accepts it, and the mux must keep presenting it until then.
		if (!b_grant) begin
			// Master A currently owns the bus. Grant B only when B requests
			// and A is not holding an active write request (a_we is low).
			if (!reset && b_request && !a_we) begin
				b_grant <= 1'b1;
			end
		end else begin
			// Master B currently owns the bus. Release grant when B drops
			// request (or reset) and is not holding a write (b_we is low).
			if ((reset || !b_request) && !b_we) begin
				b_grant <= 1'b0;
			end
		end
	end

	initial begin
		b_grant = 1'b0;
	end

	// While B owns the bus, A sees busy=1 so its stalled write stays pending.
	// B sees the real busy when granted, or 1 when not granted.
	assign a_busy = b_grant ? 1'b1 : ddram_busy;
	assign b_busy = b_grant ? ddram_busy : 1'b1;

	// Output multiplexer
	assign ddram_addr     = b_grant ? b_addr     : a_addr;
	assign ddram_din      = b_grant ? b_din      : a_din;
	assign ddram_be       = b_grant ? b_be       : a_be;
	assign ddram_burstcnt = b_grant ? b_burstcnt : a_burstcnt;
	assign ddram_we       = b_grant ? b_we       : a_we;

endmodule
