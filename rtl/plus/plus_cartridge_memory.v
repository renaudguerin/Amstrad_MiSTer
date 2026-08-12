// Amstrad Plus cartridge image memory service.
//
// Cartridge bytes occupy bank 3 addresses 0x080000 through 0x0fffff.  A new
// load invalidates the published image before clearing that region, so the CPU
// can never observe a partly replaced image.  CLEAR_BYTES is exposed only to
// make the clear sequence practical in deterministic simulations; production
// builds use the complete 512 KiB region.
//
// cold_reset is deliberately a machine reset, not a power-on reset: it cancels
// transient loader/CPU activity but preserves image_valid and SDRAM contents.
// detach and load_abort invalidate the image without scrubbing SDRAM.
// load_ready is a one-cycle completion pulse for both accepted writes and
// consumed address rejections.  A rejection produces no backend request and
// makes load_error sticky until load_begin, load_abort, detach, or cold_reset;
// an errored session cannot be committed.
// Simultaneous controls have explicit dominance:
// detach > load_abort > cold_reset > load_begin > load_commit.  Every asserted
// control is consumed even when a higher-priority control wins, so a held
// lower-priority input cannot take effect on a later cycle.

module plus_cartridge_memory #(
	parameter [19:0] CLEAR_BYTES = 20'd524288
)
(
	input             clk,
	input             cold_reset,
	input             detach,

	input             load_begin,
	input             load_commit,
	input             load_abort,
	input             load_valid,
	input      [5:0]  load_page,
	input      [14:0] load_offset,
	input      [7:0]  load_data,
	output reg        load_ready,
	output reg        load_error,

	input             cpu_valid,
	input      [4:0]  cpu_page,
	input      [13:0] cpu_offset,
	output reg        cpu_ready,
	output reg [7:0]  cpu_data,

	output reg        image_valid,
	output            busy,

	output reg        mem_req,
	output reg        mem_write,
	output reg [1:0]  mem_bank,
	output reg [22:0] mem_addr,
	output reg [7:0]  mem_wdata,
	input             mem_ack,
	input      [7:0]  mem_rdata
);

localparam [22:0] CARTRIDGE_BASE = 23'h080000;
localparam [19:0] CLEAR_LAST_EXT = CLEAR_BYTES - 20'd1;

localparam [1:0] REQUEST_CLEAR = 2'd0;
localparam [1:0] REQUEST_LOAD  = 2'd1;
localparam [1:0] REQUEST_CPU   = 2'd2;

reg        load_active;
reg        clear_active;
reg [18:0] clear_index;
reg        commit_pending;

reg [1:0] request_kind;
reg       discard_request;

reg load_consumed;
reg cpu_consumed;
reg begin_consumed;
reg commit_consumed;
reg abort_consumed;
reg detach_consumed;

wire detach_event = detach && !detach_consumed;
wire abort_event  = load_abort && !abort_consumed && !detach;
wire begin_event  = load_begin && !begin_consumed && !cold_reset &&
	                  !load_abort && !detach;
wire commit_event = load_commit && !commit_consumed && !load_begin &&
	                  !cold_reset && !load_abort && !detach;

assign busy = load_active;

// Quartus maps these synthesizable initial values to FPGA power-up state.
// cold_reset intentionally does not restore image_valid to this value.
initial begin
	load_ready      = 1'b0;
	load_error      = 1'b0;
	cpu_ready       = 1'b0;
	cpu_data        = 8'hFF;
	image_valid     = 1'b0;
	mem_req         = 1'b0;
	mem_write       = 1'b0;
	mem_bank        = 2'b11;
	mem_addr        = CARTRIDGE_BASE;
	mem_wdata       = 8'h00;
	load_active     = 1'b0;
	clear_active    = 1'b0;
	clear_index     = 19'd0;
	commit_pending  = 1'b0;
	request_kind    = REQUEST_CLEAR;
	discard_request = 1'b0;
	load_consumed   = 1'b0;
	cpu_consumed    = 1'b0;
	begin_consumed  = 1'b0;
	commit_consumed = 1'b0;
	abort_consumed  = 1'b0;
	detach_consumed = 1'b0;
end

// synthesis translate_off
initial begin
	if ((CLEAR_BYTES < 20'd1) || (CLEAR_BYTES > 20'd524288)) begin
		$display("ERROR: CLEAR_BYTES must be in the range 1..524288");
		$finish;
	end
end
// synthesis translate_on

always @(posedge clk) begin
	// Completion signals are single-cycle pulses.  A valid held high is
	// consumed only once and must be released before another request.
	load_ready <= 1'b0;
	cpu_ready  <= 1'b0;

	if (!load_valid)
		load_consumed <= 1'b0;
	if (!cpu_valid)
		cpu_consumed <= 1'b0;
	// Consume every asserted control, including controls hidden by a
	// simultaneous higher-priority operation.  Deassertion rearms it.
	begin_consumed  <= load_begin;
	commit_consumed <= load_commit;
	abort_consumed  <= load_abort;
	detach_consumed <= detach;

	// Control operations outrank an acknowledgement in the same cycle.  The
	// physical request is nevertheless held until acknowledged; its logical
	// completion is discarded after the control operation invalidates it.
	if (detach_event) begin
		image_valid     <= 1'b0;
		load_active     <= 1'b0;
		clear_active    <= 1'b0;
		clear_index     <= 19'd0;
		commit_pending  <= 1'b0;
		load_error      <= 1'b0;
		load_consumed   <= load_valid;
		cpu_consumed    <= cpu_valid;
		if (mem_req && !mem_ack)
			discard_request <= 1'b1;
		else begin
			mem_req <= 1'b0;
			discard_request <= 1'b0;
		end
	end
	else if (abort_event) begin
		image_valid    <= 1'b0;
		load_active    <= 1'b0;
		clear_active   <= 1'b0;
		clear_index    <= 19'd0;
		commit_pending <= 1'b0;
		load_error     <= 1'b0;
		load_consumed  <= load_valid;
		cpu_consumed   <= cpu_valid;
		if (mem_req && !mem_ack)
			discard_request <= 1'b1;
		else begin
			mem_req <= 1'b0;
			discard_request <= 1'b0;
		end
	end
	else if (cold_reset) begin
		load_active     <= 1'b0;
		clear_active    <= 1'b0;
		clear_index     <= 19'd0;
		commit_pending  <= 1'b0;
		load_error      <= 1'b0;
		load_consumed   <= load_valid;
		cpu_consumed    <= cpu_valid;
		if (mem_req && !mem_ack)
			discard_request <= 1'b1;
		else begin
			mem_req <= 1'b0;
			discard_request <= 1'b0;
		end
	end
	else if (begin_event) begin
		image_valid    <= 1'b0;
		load_active    <= 1'b1;
		clear_active   <= 1'b1;
		clear_index    <= 19'd0;
		commit_pending <= 1'b0;
		load_error     <= 1'b0;
		load_consumed  <= load_valid;
		cpu_consumed   <= cpu_valid;
		if (mem_req && !mem_ack)
			discard_request <= 1'b1;
		else begin
			mem_req <= 1'b0;
			discard_request <= 1'b0;
		end
	end
	else begin
		if (commit_event) begin
			if (load_active)
				commit_pending <= 1'b1;
		end

		if (mem_req) begin
			if (mem_ack) begin
				mem_req <= 1'b0;
				if (discard_request) begin
					discard_request <= 1'b0;
				end
				else begin
					case (request_kind)
						REQUEST_CLEAR: begin
							if (clear_index == CLEAR_LAST_EXT[18:0]) begin
								clear_active <= 1'b0;
							end
							else begin
								clear_index <= clear_index + 19'd1;
								mem_req   <= 1'b1;
								mem_write <= 1'b1;
								mem_bank  <= 2'b11;
								mem_addr  <= CARTRIDGE_BASE +
								             {4'b0000, clear_index} + 23'd1;
								mem_wdata <= 8'h00;
							end
						end

						REQUEST_LOAD: begin
							load_ready <= 1'b1;
						end

						REQUEST_CPU: begin
							cpu_ready <= 1'b1;
							cpu_data  <= mem_rdata;
						end

						default: begin
						end
					endcase
				end
			end
		end
		else if (load_active) begin
			if (clear_active) begin
				request_kind <= REQUEST_CLEAR;
				mem_req      <= 1'b1;
				mem_write    <= 1'b1;
				mem_bank     <= 2'b11;
				mem_addr     <= CARTRIDGE_BASE +
				                {4'b0000, clear_index};
				mem_wdata    <= 8'h00;
			end
			else if (load_valid && !load_consumed) begin
				load_consumed <= 1'b1;
				// Reject before truncating either address field.  The error is
				// sticky for this load and makes commit fail closed.
				if (load_page[5] || load_offset[14]) begin
					load_ready <= 1'b1;
					load_error <= 1'b1;
				end
				else begin
					request_kind <= REQUEST_LOAD;
					mem_req      <= 1'b1;
					mem_write    <= 1'b1;
					mem_bank     <= 2'b11;
					mem_addr     <= {4'b0001, load_page[4:0],
					                 load_offset[13:0]};
					mem_wdata    <= load_data;
				end
			end
			else if (commit_pending && !load_error) begin
				image_valid    <= 1'b1;
				load_active    <= 1'b0;
				commit_pending <= 1'b0;
			end
		end
		else if (cpu_valid && !cpu_consumed) begin
			cpu_consumed <= 1'b1;
			if (image_valid) begin
				request_kind <= REQUEST_CPU;
				mem_req      <= 1'b1;
				mem_write    <= 1'b0;
				mem_bank     <= 2'b11;
				mem_addr     <= {4'b0001, cpu_page, cpu_offset};
				mem_wdata    <= 8'h00;
			end
			else begin
				cpu_ready <= 1'b1;
				cpu_data  <= 8'hFF;
			end
		end
	end
end

endmodule
