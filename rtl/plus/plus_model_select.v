// Amstrad Plus model selection and static capability decode.
//
// plus_model encoding:
//   2'b00: classic CPC mode (Plus disabled)
//   2'b01: GX4000
//   2'b10: CPC 6128 Plus
//   2'b11: CPC 464 Plus
//
// A reset high-cartridge-page output is intentionally not decoded here.
// GX4000 always selects page 1, but the 464 Plus and 6128 Plus select page 1
// or page 3 according to the external /EXP line. That dynamic input belongs
// in the later cartridge/MMU integration rather than this static decoder.

module plus_model_select
(
	input      [1:0] plus_model,
	output reg       plus_mode,
	output reg       ram_128k,
	output reg       has_fdc,
	output reg       has_tape
);

always @(*) begin
	plus_mode = 1'b0;
	ram_128k   = 1'b0;
	has_fdc    = 1'b0;
	has_tape   = 1'b0;

	case (plus_model)
		2'b01: begin // GX4000: 64K, no FDC, no tape
			plus_mode = 1'b1;
		end

		2'b10: begin // CPC 6128 Plus: 128K, FDC, no tape
			plus_mode = 1'b1;
			ram_128k   = 1'b1;
			has_fdc    = 1'b1;
		end

		2'b11: begin // CPC 464 Plus: 64K, no FDC, tape
			plus_mode = 1'b1;
			has_tape   = 1'b1;
		end

		default: begin // Off, and unknown simulation values: fail closed
		end
	endcase
end

endmodule
