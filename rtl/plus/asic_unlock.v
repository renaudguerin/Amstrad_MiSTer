// Amstrad Plus ASIC lock/unlock sequence detector.
//
// The write input is already decoded: each write_strobe represents one write
// to any CRTC register-select port in the &BCxx range. RESET_N is the active-
// low hard reset.

module asic_unlock
(
	input            clk,
	input            RESET_N,
	input            write_strobe,
	input      [7:0] write_data,
	output reg       unlocked,

	input            sna_load,
	input            sna_unlock,
	// B8-5 slice A: SNA CPC+ 0x8F7 next-expected-unlock-byte state.
	// 0 waits nonzero, 1 waits zero, 2..E wait FF..8A, F waits CD, 10h
	// waits the terminal EE (which never alters the lock decision).
	input      [4:0] sna_seq_state
);

reg       matching_sequence;
reg       previous_nonzero;
reg [3:0] sequence_index;

function [7:0] sequence_byte;
	input [3:0] index;
	begin
		case (index)
			4'd0:  sequence_byte = 8'hFF;
			4'd1:  sequence_byte = 8'h77;
			4'd2:  sequence_byte = 8'hB3;
			4'd3:  sequence_byte = 8'h51;
			4'd4:  sequence_byte = 8'hA8;
			4'd5:  sequence_byte = 8'hD4;
			4'd6:  sequence_byte = 8'h62;
			4'd7:  sequence_byte = 8'h39;
			4'd8:  sequence_byte = 8'h9C;
			4'd9:  sequence_byte = 8'h46;
			4'd10: sequence_byte = 8'h2B;
			4'd11: sequence_byte = 8'h15;
			4'd12: sequence_byte = 8'h8A;
			default: sequence_byte = 8'h00;
		endcase
	end
endfunction

always @(posedge clk or negedge RESET_N) begin
	if (!RESET_N) begin
		unlocked          <= 1'b0;
		matching_sequence <= 1'b0;
		previous_nonzero  <= 1'b0;
		sequence_index    <= 4'd0;
	end
	else if (sna_load) begin
		// 8F6 restores the lock independently; 8F7 restores the next
		// expected sequence state (SNA format note 10). Mid-sequence
		// states resume with the previous byte's nonzero status so a
		// later mismatch re-arms exactly as if the prefix had been
		// written live: only the state-2 prefix (after the sync zero)
		// resumes with previous_nonzero clear. The terminal EE state
		// (10h) is the ordinary post-CD unsynchronised state with a
		// pending nonzero: the EE byte itself never touches the lock,
		// and no CD is replayed.
		unlocked <= sna_unlock;
		case (sna_seq_state)
		5'd0: begin
			matching_sequence <= 1'b0;
			previous_nonzero  <= 1'b0;
			sequence_index    <= 4'd0;
		end
		5'd1: begin
			matching_sequence <= 1'b0;
			previous_nonzero  <= 1'b1;
			sequence_index    <= 4'd0;
		end
		5'd15: begin
			matching_sequence <= 1'b1;
			previous_nonzero  <= 1'b1;
			sequence_index    <= 4'd13;
		end
		5'd16: begin
			matching_sequence <= 1'b0;
			previous_nonzero  <= 1'b1;
			sequence_index    <= 4'd0;
		end
		default: begin
			if (sna_seq_state >= 5'd2 && sna_seq_state <= 5'd14) begin
				matching_sequence <= 1'b1;
				previous_nonzero  <= (sna_seq_state == 5'd2) ? 1'b0 : 1'b1;
				sequence_index    <= sna_seq_state[3:0] - 4'd2;
			end
			else begin
				matching_sequence <= 1'b0;
				previous_nonzero  <= 1'b0;
				sequence_index    <= 4'd0;
			end
		end
		endcase
	end
	else if (write_strobe) begin
		previous_nonzero <= (write_data != 8'h00);

		if (!matching_sequence) begin
			// Any adjacent nonzero, zero pair starts (or restarts) matching.
			if (previous_nonzero && (write_data == 8'h00)) begin
				matching_sequence <= 1'b1;
				sequence_index    <= 4'd0;
			end
		end
		else if (sequence_index < 4'd13) begin
			if (write_data == sequence_byte(sequence_index)) begin
				sequence_index <= sequence_index + 4'd1;
			end
			else begin
				// A mismatch cannot unlock the ASIC. It may itself finish a
				// fresh nonzero, zero sync pair.
				matching_sequence <= previous_nonzero &&
				                     (write_data == 8'h00);
				sequence_index <= 4'd0;
			end
		end
		else begin
			// The state byte takes effect immediately; the published trailing
			// EE byte is not part of the lock/unlock decision.
			unlocked <= (write_data == 8'hCD);
			matching_sequence <= previous_nonzero &&
			                     (write_data == 8'h00);
			sequence_index <= 4'd0;
		end
	end
end

endmodule
