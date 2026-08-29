// Top wrapper module for Phase P8 unit testbench.
module plus_p8_test_top (
	input clk,
	input reset,

	// i8255 ports
	input        ppi_cs,
	input        ppi_we,
	input        ppi_oe,
	input  [1:0] ppi_addr,
	input  [7:0] ppi_idata,
	output [7:0] ppi_odata,
	input  [7:0] ppi_ipa,
	output [7:0] ppi_opa,
	input  [7:0] ppi_ipb,
	output [7:0] ppi_opb,
	input  [7:0] ppi_ipc,
	output [7:0] ppi_opc,
	input        ppi_plus_mode,

	// plus_sna_parser ports
	input        sna_download,
	input        cpc_plus_chunk_start,
	input        cpc_plus_byte_wr,
	input  [7:0] cpc_plus_byte_data,
	output       sna_ioctl_wait,
	output       asic_sna_wr,
	output [13:0] asic_sna_addr,
	output [7:0] asic_sna_data,
	output       asic_sna_active,
	output [7:0] asic_sna_rmr2,
	output       asic_sna_unlock,

	// plus_model_select ports
	input  [1:0] model_plus_model,
	output       model_plus_mode,
	output       model_ram_128k,
	output       model_has_fdc,
	output       model_has_tape,

	// FDC decode test signals
	input  [15:0] fdc_test_addr,
	input         fdc_test_status17,
	input         fdc_test_plus_mode,
	input         fdc_test_has_fdc,
	output        fdc_motor_sel,
	output        u765_sel
);

	assign fdc_motor_sel = !fdc_test_addr[10] & fdc_test_addr[9] & !fdc_test_addr[8] & (!fdc_test_plus_mode | fdc_test_has_fdc);
	assign u765_sel      = !fdc_test_addr[10] & fdc_test_addr[9] & fdc_test_addr[8] & fdc_test_addr[4] & ~fdc_test_status17 & (!fdc_test_plus_mode | fdc_test_has_fdc);

	i8255 ppi
	(
		.reset(reset),
		.clk_sys(clk),
		.cs(ppi_cs),
		.we(ppi_we),
		.oe(ppi_oe),
		.addr(ppi_addr),
		.idata(ppi_idata),
		.odata(ppi_odata),
		.ipa(ppi_ipa),
		.opa(ppi_opa),
		.ipb(ppi_ipb),
		.opb(ppi_opb),
		.ipc(ppi_ipc),
		.opc(ppi_opc),
		.plus_mode(ppi_plus_mode),
		.sna_load(1'b0),
		.sna_opa(8'd0),
		.sna_opb(8'd0),
		.sna_opc(8'd0),
		.sna_control(8'd0)
	);

	plus_sna_parser sna_parser
	(
		.clk(clk),
		.reset(reset),
		.sna_download(sna_download),
		.cpc_plus_chunk_start(cpc_plus_chunk_start),
		.cpc_plus_byte_wr(cpc_plus_byte_wr),
		.cpc_plus_byte_data(cpc_plus_byte_data),
		.ioctl_wait(sna_ioctl_wait),
		.asic_sna_wr(asic_sna_wr),
		.asic_sna_addr(asic_sna_addr),
		.asic_sna_data(asic_sna_data),
		.asic_sna_active(asic_sna_active),
		.asic_sna_rmr2(asic_sna_rmr2),
		.asic_sna_unlock(asic_sna_unlock)
	);

	plus_model_select model_select
	(
		.plus_model(model_plus_model),
		.plus_mode(model_plus_mode),
		.ram_128k(model_ram_128k),
		.has_fdc(model_has_fdc),
		.has_tape(model_has_tape)
	);

endmodule
