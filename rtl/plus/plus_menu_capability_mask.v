// MiSTer hides a CONF_STR item when its d<n> prefix addresses a clear bit in
// status_menumask. Keep the machine-capability mapping in a small production
// module so the model decoder and menu contract can be tested without the HPS
// wrapper or a generated build_id.v.

module plus_menu_capability_mask
(
	input  en270p,
	input  plus_mode,
	input  plus_has_fdc,
	input  plus_has_tape,
	output [15:0] status_menumask
);

assign status_menumask = {10'b0,
	(!plus_mode || plus_has_tape),
	(!plus_mode || plus_has_fdc),
	!plus_mode,
	plus_mode,
	en270p,
	1'b0};

endmodule
