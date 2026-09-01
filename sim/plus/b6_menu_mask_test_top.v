module b6_menu_mask_test_top
(
	input  [1:0]  plus_model,
	input         en270p,
	output [15:0] status_menumask
);

wire plus_mode;
wire plus_has_fdc;
wire plus_has_tape;

plus_model_select model_decode
(
	.plus_model(plus_model),
	.plus_mode(plus_mode),
	.ram_128k(),
	.has_fdc(plus_has_fdc),
	.has_tape(plus_has_tape)
);

plus_menu_capability_mask menu_decode
(
	.en270p(en270p),
	.plus_mode(plus_mode),
	.plus_has_fdc(plus_has_fdc),
	.plus_has_tape(plus_has_tape),
	.status_menumask(status_menumask)
);

endmodule
