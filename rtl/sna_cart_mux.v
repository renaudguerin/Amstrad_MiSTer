//============================================================================
//  SDRAM cartridge-port owner mux for the SNA save stream (B18 slice 4c).
//
//  rtl/sdram.v has one held-request port (cart_*). plus_cartridge_memory owns
//  it by default; sna_save_stream borrows it to read RAM during a save. Each
//  client holds its request until its acknowledge.
//
//  Handover waits for the owner to drain. The save stream drops its request
//  at once on abort, and a read the controller already accepted still
//  acknowledges later (sdram.v arbitrates at q == STATE_IDLE and acknowledges
//  at STATE_READ of the same slot). Handing the port over at that moment would
//  deliver the stale acknowledge to the new owner as completion of its own
//  request. So once the owner's request is low while the other client waits,
//  the mux yields: it stops forwarding the owner's requests and hands over
//  only after two clkref rising edges, which puts at least one whole SDRAM
//  slot, and any acknowledge still owed, behind the handover.
//
//  Yielding also bounds the wait. The stream re-requests one clock after the
//  arbitration that follows each acknowledge, so without the forwarding mask
//  the owner's request would never stay low for two edges and the other
//  client would wait for the whole save.
//
//  Reference: docs/b18-sna-save.md, slice 4c constraints.
//============================================================================

module sna_cart_mux
(
	input             clk,
	input             clkref,        // ce_ref, the SDRAM slot reference

	// Client A: plus_cartridge_memory (default owner)
	input             a_req,
	input             a_wr,
	input       [1:0] a_bank,
	input      [22:0] a_addr,
	input       [7:0] a_din,
	output            a_ack,
	output            a_grant,

	// Client B: sna_save_stream (read-only)
	input             b_req,
	input       [1:0] b_bank,
	input      [22:0] b_addr,
	output            b_ack,

	// SDRAM cart_* port
	output            cart_req,
	output            cart_wr,
	output      [1:0] cart_bank,
	output     [22:0] cart_addr,
	output      [7:0] cart_din,
	input             cart_ack,
	input             cart_grant
);

	reg       owner_b;
	reg       yielding;
	reg [1:0] drain;             // clkref rising edges since yielding began
	reg       clkref_d;

	initial begin
		owner_b  = 1'b0;
		yielding = 1'b0;
		drain    = 2'd0;
		clkref_d = 1'b0;
	end

	wire owner_req = owner_b ? b_req : a_req;
	wire other_req = owner_b ? a_req : b_req;

	always @(posedge clk) begin
		clkref_d <= clkref;

		if (!yielding) begin
			drain <= 2'd0;
			if (!owner_req && other_req) yielding <= 1'b1;
		end else if (drain == 2'd2) begin
			owner_b  <= ~owner_b;
			yielding <= 1'b0;
			drain    <= 2'd0;
		end else if (clkref && !clkref_d) begin
			drain <= drain + 2'd1;
		end
	end

	assign cart_req  = owner_req && !yielding;
	assign cart_wr   = owner_b ? 1'b0   : a_wr;
	assign cart_bank = owner_b ? b_bank : a_bank;
	assign cart_addr = owner_b ? b_addr : a_addr;
	assign cart_din  = owner_b ? 8'd0   : a_din;

	assign a_ack = !owner_b && cart_ack;
	assign a_grant = !owner_b && cart_grant;
	assign b_ack =  owner_b && cart_ack;

endmodule
