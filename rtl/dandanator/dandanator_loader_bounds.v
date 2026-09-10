// Keep Dandanator images within their reserved 512 KiB SDRAM window.
module dandanator_loader_bounds
(
	input             dan_download,
	input             ioctl_wr,
	input      [24:0] ioctl_addr,
	output            write_accepted
);

	assign write_accepted = dan_download && ioctl_wr &&
	                        (ioctl_addr < 25'h080000);

endmodule
