//============================================================================
//
//  Single-owner DDR3 write arbiter for the SSM observers (backlog B4).
//
//  The SSM event ring and the experimental phase 2 sample recorder both write
//  to the core's DDR3 port. Passivity does not remove memory traffic, so one
//  module has to own the port and order every request on it. Each client keeps
//  its ordinary Avalon-style write interface and sees `busy` asserted whenever
//  it is not the selected client; that is exactly waitrequest, so no client
//  needs to know an arbiter exists.
//
//  Client 0 has priority. It is the event ring, which writes a handful of
//  words per SHAKER screen; client 1 is the continuous sample stream. Once a
//  client's request has been presented to a stalled slave the selection is
//  locked until the slave accepts it, so a request is never abandoned
//  mid-transaction. There is no reset input for that reason: resetting the
//  lock could switch the multiplexer under a live request.
//
//============================================================================

module ssm_ddr_arb
(
	input             clk,

	input      [28:0] c0_addr,
	input      [63:0] c0_din,
	input       [7:0] c0_be,
	input       [7:0] c0_burstcnt,
	input             c0_we,
	output            c0_busy,

	input      [28:0] c1_addr,
	input      [63:0] c1_din,
	input       [7:0] c1_be,
	input       [7:0] c1_burstcnt,
	input             c1_we,
	output            c1_busy,

	output     [28:0] ddram_addr,
	output     [63:0] ddram_din,
	output      [7:0] ddram_be,
	output      [7:0] ddram_burstcnt,
	output            ddram_we,
	input             ddram_busy
);

reg lock = 1'b0;
reg sel  = 1'b0;

wire any_req = c0_we | c1_we;
wire pick    = c0_we ? 1'b0 : 1'b1;   // client 0 first
wire cur     = lock ? sel : pick;

assign ddram_we       = lock ? (sel ? c1_we : c0_we) : (c0_we ? c0_we : c1_we);
assign ddram_addr     = cur ? c1_addr     : c0_addr;
assign ddram_din      = cur ? c1_din      : c0_din;
assign ddram_be       = cur ? c1_be       : c0_be;
assign ddram_burstcnt = cur ? c1_burstcnt : c0_burstcnt;

// When unlocked, client 0 has priority and is only stalled by the slave.
// Client 1 is stalled by the slave or by client 0's request.
// Once locked across a slave stall, the active client holds the port.
assign c0_busy = ddram_busy | (lock ? (sel != 1'b0) : 1'b0);
assign c1_busy = ddram_busy | (lock ? (sel != 1'b1) : c0_we);

always @(posedge clk) begin
	if (lock) begin
		if (!ddram_busy) lock <= 1'b0;
	end
	else if (any_req & ddram_busy) begin
		lock <= 1'b1;
		sel  <= pick;
	end
end

endmodule
