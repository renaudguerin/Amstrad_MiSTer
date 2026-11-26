// MiSTer disables d<n> items on clear bits and hides H<n> items on set bits.
// Keep the machine capability mask separate from the HPS transport.

module plus_menu_capability_mask
(
	input  en270p,
	input  plus_mode,
	input  plus_has_fdc,
	input  plus_has_tape,
	input  sna_unavailable,
	output [15:0] status_menumask
);

assign status_menumask = {9'b0, sna_unavailable,
	(!plus_mode || plus_has_tape),
	(!plus_mode || plus_has_fdc),
	!plus_mode,
	plus_mode,
	en270p,
	1'b0};

endmodule
