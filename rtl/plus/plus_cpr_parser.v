// Amstrad Plus CPR (RIFF) cartridge image parser.
//
// Parses a sequential ioctl stream carrying a .cpr image, validating the RIFF
// container and block chunks ("cb00".."cb31"), forwarding page data to
// plus_cartridge_memory, and pulsing control lines (load_begin, load_commit,
// load_abort).  A cbNN chunk declaring more than one 16 KiB page is malformed
// and aborts the load (fail closed); see docs/plus/architecture.md,
// "CPR parser policy (P0)".

module plus_cpr_parser
(
	input             clk,
	input             reset,

	input             cpr_download,
	input             ioctl_wr,
	input      [24:0] ioctl_addr,
	input      [7:0]  ioctl_dout,
	output            ioctl_wait,

	output reg        load_begin,
	output reg        load_commit,
	output reg        load_abort,
	output reg        load_valid,
	output reg [5:0]  load_page,
	output reg [14:0] load_offset,
	output reg [7:0]  load_data,
	input             load_ready,
	input             load_error
);

localparam [3:0] STATE_IDLE             = 4'd0;
localparam [3:0] STATE_HEADER_RIFF      = 4'd1;
localparam [3:0] STATE_HEADER_RIFF_LEN  = 4'd2;
localparam [3:0] STATE_HEADER_FORM_TYPE = 4'd3;
localparam [3:0] STATE_CHUNK_ID         = 4'd4;
localparam [3:0] STATE_CHUNK_LEN        = 4'd5;
localparam [3:0] STATE_CHUNK_DATA       = 4'd6;
localparam [3:0] STATE_CHUNK_PAD        = 4'd7;
localparam [3:0] STATE_DONE             = 4'd8;
localparam [3:0] STATE_ERROR            = 4'd9;

reg [3:0]  state;
reg        cpr_download_d;

reg [24:0] expected_addr;
reg [24:0] file_pos;
reg [23:0] riff_len;
reg [24:0] riff_limit;

reg [1:0]  sub_idx;
reg [23:0] chunk_id_prev;
reg [23:0] chunk_len_prev;
reg [31:0] chunk_len;
reg [31:0] chunk_bytes_read;
reg [5:0]  current_page;
reg        is_block;
reg        has_block;
reg        chunk_pad;

assign ioctl_wait = load_valid || (cpr_download && !cpr_download_d);

// Chunk ID evaluation helpers
wire [7:0] id0 = chunk_id_prev[7:0];
wire [7:0] id1 = chunk_id_prev[15:8];
wire [7:0] id2 = chunk_id_prev[23:16];
wire [7:0] id3 = ioctl_dout;

wire is_cb_prefix = ((id0 == 8'h63) || (id0 == 8'h43)) && ((id1 == 8'h62) || (id1 == 8'h42)); // "cb" or "CB"

wire id2_is_0 = (id2 == 8'h30);
wire id2_is_1 = (id2 == 8'h31);
wire id2_is_2 = (id2 == 8'h32);
wire id2_is_3 = (id2 == 8'h33);

wire id3_is_digit = (id3 >= 8'h30) && (id3 <= 8'h39);
wire id3_is_0_or_1 = (id3 == 8'h30) || (id3 == 8'h31);

wire cb_valid_0_9   = id2_is_0 && id3_is_digit;
wire cb_valid_10_19 = id2_is_1 && id3_is_digit;
wire cb_valid_20_29 = id2_is_2 && id3_is_digit;
wire cb_valid_30_31 = id2_is_3 && id3_is_0_or_1;

wire cb_valid = cb_valid_0_9 || cb_valid_10_19 || cb_valid_20_29 || cb_valid_30_31;

wire [5:0] decoded_page =
	cb_valid_0_9   ? {2'b00, id3[3:0] - 4'd0} :
	cb_valid_10_19 ? (6'd10 + {2'b00, id3[3:0] - 4'd0}) :
	cb_valid_20_29 ? (6'd20 + {2'b00, id3[3:0] - 4'd0}) :
	cb_valid_30_31 ? (6'd30 + {5'd0, id3[0]}) : 6'd0;

// Chunk length & extent calculations
wire [31:0] full_chunk_len = {ioctl_dout, chunk_len_prev};
wire        full_chunk_pad = full_chunk_len[0];
wire [32:0] chunk_total_span = {1'b0, full_chunk_len} + {32'd0, full_chunk_pad};
wire [32:0] file_pos_after_hdr = {8'd0, file_pos} + 33'd1;
wire [32:0] file_pos_after_chunk = file_pos_after_hdr + chunk_total_span;
wire [32:0] riff_limit_ext = {8'd0, riff_limit};

wire chunk_extent_exceeds_riff = (file_pos_after_chunk > riff_limit_ext);

// RIFF length evaluation
wire [31:0] full_riff_len = {ioctl_dout, riff_len};
wire        riff_len_valid = (full_riff_len >= 32'd4) && (full_riff_len <= 32'h01FFFFF7);

initial begin
	state            = STATE_IDLE;
	cpr_download_d   = 1'b0;
	expected_addr    = 25'd0;
	file_pos         = 25'd0;
	riff_len         = 24'd0;
	riff_limit       = 25'd0;
	sub_idx          = 2'd0;
	chunk_id_prev    = 24'd0;
	chunk_len_prev   = 24'd0;
	chunk_len        = 32'd0;
	chunk_bytes_read = 32'd0;
	current_page     = 6'd0;
	is_block         = 1'b0;
	has_block        = 1'b0;
	chunk_pad        = 1'b0;
	load_begin       = 1'b0;
	load_commit      = 1'b0;
	load_abort       = 1'b0;
	load_valid       = 1'b0;
	load_page        = 6'd0;
	load_offset      = 15'd0;
	load_data        = 8'h00;
end

always @(posedge clk) begin
	if (reset) begin
		state            <= STATE_IDLE;
		cpr_download_d   <= 1'b0;
		expected_addr    <= 25'd0;
		file_pos         <= 25'd0;
		riff_len         <= 24'd0;
		riff_limit       <= 25'd0;
		sub_idx          <= 2'd0;
		chunk_id_prev    <= 24'd0;
		chunk_len_prev   <= 24'd0;
		chunk_len        <= 32'd0;
		chunk_bytes_read <= 32'd0;
		current_page     <= 6'd0;
		is_block         <= 1'b0;
		has_block        <= 1'b0;
		chunk_pad        <= 1'b0;
		load_begin       <= 1'b0;
		load_commit      <= 1'b0;
		load_abort       <= 1'b0;
		load_valid       <= 1'b0;
		load_page        <= 6'd0;
		load_offset      <= 15'd0;
		load_data        <= 8'h00;
	end
	else begin
		cpr_download_d <= cpr_download;
		load_begin     <= 1'b0;
		load_commit    <= 1'b0;
		load_abort     <= 1'b0;

		if (load_ready) begin
			load_valid <= 1'b0;
		end

		if (!cpr_download_d && cpr_download) begin
			state            <= STATE_HEADER_RIFF;
			expected_addr    <= 25'd0;
			file_pos         <= 25'd0;
			riff_len         <= 24'd0;
			riff_limit       <= 25'd0;
			sub_idx          <= 2'd0;
			chunk_id_prev    <= 24'd0;
			chunk_len_prev   <= 24'd0;
			chunk_len        <= 32'd0;
			chunk_bytes_read <= 32'd0;
			current_page     <= 6'd0;
			is_block         <= 1'b0;
			has_block        <= 1'b0;
			chunk_pad        <= 1'b0;
			load_begin       <= 1'b1;
			load_valid       <= 1'b0;
		end
		else if (cpr_download_d && !cpr_download) begin
			load_valid <= 1'b0;
			if (state == STATE_DONE && has_block && !load_valid && !load_error) begin
				load_commit <= 1'b1;
			end
			else if (state != STATE_ERROR) begin
				load_abort <= 1'b1;
			end
			state <= STATE_IDLE;
		end
		else if (load_error && state != STATE_IDLE && state != STATE_ERROR) begin
			load_abort <= 1'b1;
			load_valid <= 1'b0;
			state      <= STATE_ERROR;
		end
		else if (cpr_download) begin
			if (load_valid) begin
				// Hold stable outputs while waiting for load_ready
			end
			else if (ioctl_wr) begin
				if (state == STATE_ERROR) begin
					// Ignore stream bytes until cpr_download falls
				end
				else if (ioctl_addr != expected_addr) begin
					load_abort <= 1'b1;
					state      <= STATE_ERROR;
				end
				else begin
					expected_addr <= expected_addr + 25'd1;
					file_pos      <= file_pos + 25'd1;

					case (state)
						STATE_HEADER_RIFF: begin
							case (sub_idx)
								2'd0: begin
									if (ioctl_dout != 8'h52) begin // 'R'
										load_abort <= 1'b1;
										state      <= STATE_ERROR;
									end
									else sub_idx <= 2'd1;
								end
								2'd1: begin
									if (ioctl_dout != 8'h49) begin // 'I'
										load_abort <= 1'b1;
										state      <= STATE_ERROR;
									end
									else sub_idx <= 2'd2;
								end
								2'd2: begin
									if (ioctl_dout != 8'h46) begin // 'F'
										load_abort <= 1'b1;
										state      <= STATE_ERROR;
									end
									else sub_idx <= 2'd3;
								end
								2'd3: begin
									if (ioctl_dout != 8'h46) begin // 'F'
										load_abort <= 1'b1;
										state      <= STATE_ERROR;
									end
									else begin
										sub_idx <= 2'd0;
										state   <= STATE_HEADER_RIFF_LEN;
									end
								end
							endcase
						end

						STATE_HEADER_RIFF_LEN: begin
							case (sub_idx)
								2'd0: begin
									riff_len[7:0] <= ioctl_dout;
									sub_idx       <= 2'd1;
								end
								2'd1: begin
									riff_len[15:8] <= ioctl_dout;
									sub_idx        <= 2'd2;
								end
								2'd2: begin
									riff_len[23:16] <= ioctl_dout;
									sub_idx         <= 2'd3;
								end
								2'd3: begin
									if (!riff_len_valid) begin
										load_abort <= 1'b1;
										state      <= STATE_ERROR;
									end
									else begin
										riff_limit  <= full_riff_len[24:0] + 25'd8;
										sub_idx     <= 2'd0;
										state       <= STATE_HEADER_FORM_TYPE;
									end
								end
							endcase
						end

						STATE_HEADER_FORM_TYPE: begin
							case (sub_idx)
								2'd0: begin
									if (ioctl_dout != 8'h41 && ioctl_dout != 8'h61) begin // 'A' or 'a'
										load_abort <= 1'b1;
										state      <= STATE_ERROR;
									end
									else sub_idx <= 2'd1;
								end
								2'd1: begin
									if (ioctl_dout != 8'h4D && ioctl_dout != 8'h6D) begin // 'M' or 'm'
										load_abort <= 1'b1;
										state      <= STATE_ERROR;
									end
									else sub_idx <= 2'd2;
								end
								2'd2: begin
									if (ioctl_dout != 8'h53 && ioctl_dout != 8'h73) begin // 'S' or 's'
										load_abort <= 1'b1;
										state      <= STATE_ERROR;
									end
									else sub_idx <= 2'd3;
								end
								2'd3: begin
									if (ioctl_dout != 8'h21) begin // '!'
										load_abort <= 1'b1;
										state      <= STATE_ERROR;
									end
									else begin
										sub_idx <= 2'd0;
										if (file_pos + 25'd1 == riff_limit) begin
											state <= STATE_DONE;
										end
										else if ({8'd0, file_pos} + 33'd9 > {8'd0, riff_limit}) begin
											load_abort <= 1'b1;
											state      <= STATE_ERROR;
										end
										else begin
											state <= STATE_CHUNK_ID;
										end
									end
								end
							endcase
						end

						STATE_CHUNK_ID: begin
							case (sub_idx)
								2'd0: begin
									chunk_id_prev[7:0] <= ioctl_dout;
									sub_idx            <= 2'd1;
								end
								2'd1: begin
									chunk_id_prev[15:8] <= ioctl_dout;
									sub_idx             <= 2'd2;
								end
								2'd2: begin
									chunk_id_prev[23:16] <= ioctl_dout;
									sub_idx              <= 2'd3;
								end
								2'd3: begin
									sub_idx <= 2'd0;
									if (is_cb_prefix) begin
										if (!cb_valid) begin
											load_abort <= 1'b1;
											state      <= STATE_ERROR;
										end
										else begin
											current_page <= decoded_page;
											is_block     <= 1'b1;
											has_block    <= 1'b1;
											state        <= STATE_CHUNK_LEN;
										end
									end
									else begin
										is_block <= 1'b0;
										state    <= STATE_CHUNK_LEN;
									end
								end
							endcase
						end

						STATE_CHUNK_LEN: begin
							case (sub_idx)
								2'd0: begin
									chunk_len_prev[7:0] <= ioctl_dout;
									sub_idx             <= 2'd1;
								end
								2'd1: begin
									chunk_len_prev[15:8] <= ioctl_dout;
									sub_idx              <= 2'd2;
								end
								2'd2: begin
									chunk_len_prev[23:16] <= ioctl_dout;
									sub_idx               <= 2'd3;
								end
								2'd3: begin
									sub_idx <= 2'd0;
									if (chunk_extent_exceeds_riff) begin
										load_abort <= 1'b1;
										state      <= STATE_ERROR;
									end
									else if (is_block && (full_chunk_len > 32'd16384)) begin
										// A cbNN page is exactly 16 KiB; a longer
										// declared block cannot be valid cartridge
										// data. Abort instead of silently dropping
										// the excess (A5a, review cd47d7d).
										load_abort <= 1'b1;
										state      <= STATE_ERROR;
									end
									else begin
										chunk_len        <= full_chunk_len;
										chunk_pad        <= full_chunk_pad;
										chunk_bytes_read <= 32'd0;
										if (full_chunk_len == 32'd0) begin
											if (file_pos + 25'd1 == riff_limit) begin
												state <= STATE_DONE;
											end
											else if ({8'd0, file_pos} + 33'd9 > {8'd0, riff_limit}) begin
												load_abort <= 1'b1;
												state      <= STATE_ERROR;
											end
											else begin
												state <= STATE_CHUNK_ID;
											end
										end
										else begin
											state <= STATE_CHUNK_DATA;
										end
									end
								end
							endcase
						end

						STATE_CHUNK_DATA: begin
							if (is_block && (chunk_bytes_read < 32'd16384)) begin
								load_valid  <= 1'b1;
								load_page   <= current_page;
								load_offset <= chunk_bytes_read[14:0];
								load_data   <= ioctl_dout;
							end

							chunk_bytes_read <= chunk_bytes_read + 32'd1;

							if (chunk_bytes_read + 32'd1 == chunk_len) begin
								if (chunk_pad) begin
									state <= STATE_CHUNK_PAD;
								end
								else if (file_pos + 25'd1 == riff_limit) begin
									state <= STATE_DONE;
								end
								else if ({8'd0, file_pos} + 33'd9 > {8'd0, riff_limit}) begin
									load_abort <= 1'b1;
									state      <= STATE_ERROR;
								end
								else begin
									state   <= STATE_CHUNK_ID;
									sub_idx <= 2'd0;
								end
							end
						end

						STATE_CHUNK_PAD: begin
							if (file_pos + 25'd1 == riff_limit) begin
								state <= STATE_DONE;
							end
							else if ({8'd0, file_pos} + 33'd9 > {8'd0, riff_limit}) begin
								load_abort <= 1'b1;
								state      <= STATE_ERROR;
							end
							else begin
								state   <= STATE_CHUNK_ID;
								sub_idx <= 2'd0;
							end
						end

						STATE_DONE: begin
							load_abort <= 1'b1;
							state      <= STATE_ERROR;
						end

						STATE_ERROR: begin
							// Ignore stream bytes until cpr_download falls
						end

						default: begin
							load_abort <= 1'b1;
							state      <= STATE_ERROR;
						end
					endcase
				end
			end
		end
	end
end

endmodule
