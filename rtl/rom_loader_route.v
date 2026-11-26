// Production ROM loader destination and bank decoder.
// Decodes ioctl_index, ioctl_addr, and page into destination model bank,
// high address bits, validity, and second-write promotion.

module rom_loader_route
(
	input        [7:0] ioctl_index,
	input       [24:14] ioctl_addr,
	input        [8:0] page,
	input        [1:0] selected_bank,
	input        [4:0] selected_slot,
	output             rom_active,
	output             addr_valid,
	output       [1:0] initial_bank,
	output       [8:0] dest_a_hi,
	output             promote_bank0_to_bank1
);

	wire numbered_slot = ioctl_index == 8'd9;
	assign rom_active = (ioctl_index[4:0] < 5'd4) || (ioctl_index == 8'd7) || numbered_slot;

	// Second-write promotion: expansion ROMs loaded to bank 0 are promoted
	// to bank 1 on the second write cycle if index[7:6] == 1 or index[5:0] != 0.
	assign promote_bank0_to_bank1 = !numbered_slot &&
		((ioctl_index[7:6] == 2'b01) || (ioctl_index[5:0] != 6'd0));

	reg       valid;
	reg [1:0] bank;
	reg [8:0] a_hi;
	wire [4:0] upper_slot = selected_slot - 5'd1;

	always @(*) begin
		if (numbered_slot) begin
			// One raw 16 KiB image, isolated from the adjacent slots and banks.
			valid = ioctl_addr == 0 && selected_bank < 2'd3 && selected_slot <= 5'd16;
			bank = selected_bank;
			a_hi = selected_slot == 0 ? 9'd0 : {1'b1, 3'd0, upper_slot};
		end else if (ioctl_index != 8'd0) begin
			valid = 1'b1;
			bank  = (ioctl_index == 8'd7) ? 2'd2 : {1'b0, &ioctl_index[7:6]};
			a_hi  = {page[8], page[7:0] + ioctl_addr[21:14]};
		end
		else begin
			case (ioctl_addr[24:14])
				11'd0, 11'd4: begin a_hi = 9'h000; valid = 1'b1; end // OS (6128, 664)
				11'd1, 11'd5: begin a_hi = 9'h100; valid = 1'b1; end // BASIC (6128, 664)
				11'd2, 11'd6: begin a_hi = 9'h107; valid = 1'b1; end // AMSDOS (6128, 664)
				11'd3, 11'd7: begin a_hi = 9'h0ff; valid = 1'b1; end // MF2 (6128, 664)
				11'd8:        begin a_hi = 9'h000; valid = 1'b1; end // CPC464 OS
				11'd9:        begin a_hi = 9'h100; valid = 1'b1; end // CPC464 BASIC
				default:      begin a_hi = 9'h000; valid = 1'b0; end
			endcase

			case (ioctl_addr[24:14])
				11'd0, 11'd1, 11'd2, 11'd3: bank = 2'd0; // CPC6128
				11'd4, 11'd5, 11'd6, 11'd7: bank = 2'd1; // CPC664
				11'd8, 11'd9:               bank = 2'd2; // CPC464
				default:                    bank = 2'd0;
			endcase
		end
	end

	assign addr_valid   = valid;
	assign initial_bank = bank;
	assign dest_a_hi    = a_hi;

endmodule
