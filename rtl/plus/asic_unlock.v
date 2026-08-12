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
	output reg       unlocked
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
