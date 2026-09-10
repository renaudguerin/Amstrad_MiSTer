// Local-lint port/parameter stub for the Quartus altsyncram primitive.
// This checks the synthesis branch of plus_sprite_ram.v without pretending to
// model the vendor memory. Functional tests use the behavioral RAM branch.
module altsyncram #(
	parameter numwords_a = 0,
	parameter widthad_a = 0,
	parameter width_a = 0,
	parameter numwords_b = 0,
	parameter widthad_b = 0,
	parameter width_b = 0,
	parameter address_reg_b = "",
	parameter clock_enable_input_a = "",
	parameter clock_enable_input_b = "",
	parameter clock_enable_output_a = "",
	parameter clock_enable_output_b = "",
	parameter indata_reg_b = "",
	parameter intended_device_family = "",
	parameter lpm_type = "",
	parameter operation_mode = "",
	parameter outdata_aclr_a = "",
	parameter outdata_aclr_b = "",
	parameter outdata_reg_a = "",
	parameter outdata_reg_b = "",
	parameter power_up_uninitialized = "",
	parameter ram_block_type = "",
	parameter read_during_write_mode_mixed_ports = "",
	parameter read_during_write_mode_port_a = "",
	parameter read_during_write_mode_port_b = "",
	parameter width_byteena_a = 0,
	parameter width_byteena_b = 0,
	parameter wrcontrol_wraddress_reg_b = ""
) (
	input             clock0,
	input             clock1,
	input      [10:0] address_a,
	input      [10:0] address_b,
	input       [3:0] data_a,
	input       [3:0] data_b,
	input             wren_a,
	input             wren_b,
	input             rden_a,
	input             rden_b,
	output      [3:0] q_a,
	output      [3:0] q_b,
	input             aclr0,
	input             aclr1,
	input             addressstall_a,
	input             addressstall_b,
	input             byteena_a,
	input             byteena_b,
	input             clocken0,
	input             clocken1,
	input             clocken2,
	input             clocken3,
	output            eccstatus
);
	assign q_a = 4'd0;
	assign q_b = 4'd0;
	assign eccstatus = 1'b0;
	wire unused = &{1'b0, clock0, clock1, address_a, address_b, data_a,
		data_b, wren_a, wren_b, rden_a, rden_b, aclr0, aclr1,
		addressstall_a, addressstall_b, byteena_a, byteena_b,
		clocken0, clocken1, clocken2, clocken3, 1'b0};
endmodule
