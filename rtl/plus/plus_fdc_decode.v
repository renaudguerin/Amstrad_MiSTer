// Shared classic/Plus FDC port decoder.
//
// Classic CPC decoding follows the partial I/O map in
// docs/references/I_O port allocation.md: A10, A8 and A7 select the FDC;
// A9 and A4-A1 are deliberately ignored. The caller uses A0 only for reads;
// either selected CPC write alias is mapped to the uPD765 data register.
//
// The Plus 6128 firmware also uses the A7=1 aliases &FADD and &FBDF.  Its
// board-level decode retains the A9 qualification and the A4 qualification
// needed to keep PlayCity and the Kempston mouse out of the uPD765 path.
module plus_fdc_decode
(
	/* verilator lint_off UNUSEDSIGNAL */
	input  [15:0] addr,
	/* verilator lint_on UNUSEDSIGNAL */
	input         plus_mode,
	input         plus_has_fdc,
	input         fdc_disabled,
	output        motor_sel,
	output        u765_sel
);

	wire fdc_present = !plus_mode | plus_has_fdc;

	// Classic CPC: A10=0, A8 selects motor/uPD765, A7=0.  A9 and A4-A1
	// are not decoded (the uPD765's A0 input is wired separately).
	wire classic_motor_sel = !addr[10] & !addr[8] & !addr[7];
	wire classic_u765_sel  = !addr[10] &  addr[8] & !addr[7];

	// Plus 6128: preserve the firmware aliases &FA7E/&FADD and
	// &FB7E/&FB7F/&FBDF while isolating &F8xx/&F9xx and the Kempston mouse.
	wire plus_motor_sel = !addr[10] & addr[9] & !addr[8];
	wire plus_u765_sel  = !addr[10] & addr[9] & addr[8] & addr[4];

	assign motor_sel = fdc_present & (plus_mode ? plus_motor_sel : classic_motor_sel);
	assign u765_sel  = fdc_present & ~fdc_disabled & (plus_mode ? plus_u765_sel : classic_u765_sel);

endmodule
