//============================================================================
//  Amstrad CPC 6128
//  Copyright (C) 2018-2019 Sorgelig
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 2 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program; if not, write to the Free Software Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//============================================================================

module emu
(
	`include "sys/emu_ports.vh"
);

assign ADC_BUS  = 'Z;
assign USER_OUT = '1;
assign {UART_RTS, UART_TXD, UART_DTR} = 0;
assign {SD_SCK, SD_MOSI, SD_CS} = 'Z;
assign DDRAM_RD = 0;   // the SSM event ring and the SNA save write only; see below

assign LED_USER  = mf2_en | ioctl_download | tape_led | tape_adc_act;
assign LED_DISK  = 0;
assign LED_POWER = 0;
assign BUTTONS   = 0;
assign VGA_SCALER= 0;
assign VGA_DISABLE = 0;
assign HDMI_FREEZE = 0;
assign HDMI_BLACKOUT = 0;
assign HDMI_BOB_DEINT = 0;

// Status Bit Map:
// 0         1         2         3          4         5         6
// 01234567890123456789012345678901 23456789012345678901234567890123
// 0123456789ABCDEFGHIJKLMNOPQRSTUV 0123456789ABCDEFGHIJKLMNOPQRSTUV
// XXX X XXXXXXXXXXXXXXXXXXXXXXXXX  XXXXXXX   XXX

`include "build_id.v"
localparam CONF_STR = {
	"Amstrad;;",
	"d4S0,DSK,Mount A:;",
	"d4S1,DSK,Mount B:;",
	"d5F4,CDT,Load tape;",
	"F8,CPR,Load Plus cartridge;",
	"d5OK,Tape sound,Disabled,Enabled;",
	"-;",
	"FC0,ROM,Load Main ROM;",
	"FC3,E??,Load expansion;",
	"-;",
	"F5,ROM,Load Dandanator ROM;",
	"F6,SNA,Load snapshot;",
	"T[38],Save snapshot;",
	"F7,E??,Load CPC464 ROM;",
	"-;",
	"O[62:61],SNAC,Off,Player 1,Player 2;",
	"-;",
	"OI,Joysticks swap,No,Yes;",
	"-;",
	
	"P1,Audio & Video;",
	"P1-;",
	"P1OPQ,Aspect ratio,Original,Full Screen,[ARC1],[ARC2];",
	"P1O9A,Scandoubler Fx,None,HQ2x,CRT 25%,CRT 50%;",
	"P1-;",
	"d1P1OR,Vertical Crop,No,Yes;",
	"P1OST,Scale,Normal,V-Integer,Narrower HV-Integer,Wider HV-Integer;",
	"P1OU,Pixel Clock,16MHz,Adaptive;",
	"P1-;",
	"d3P1O2,CRTC,Type 1,Type 0;",
	"P1O[36:35],Sync filter,Full,Raw pixels,Raw CRT;",
	"P1OBD,Display,Color(GA),Color(ASIC),Green,Amber,Cyan,White;",
	"P1-;",
	"P1O78,Stereo mix,none,25%,50%,100%;",
	"P1OO,Playcity,Disabled,Enabled;",

	"P2,Hardware;",
	"P2-;",
	"P2OJ,Mouse,Enabled,Disabled;",
	"P2OL,AMX Mouse,Disabled,Joystick 1;",
	"P2OM,Right Shift,Backslash,Shift;",
	"P2ON,Keypad,Numbers,Symbols;",
	"P2-;",
	"P2OEF,Multiface 2,Enabled,Hidden,Disabled;",
	"P2O6,CPU timings,Original,Fast;",
	"d4P2OGH,FDC,Original,Fast,Disabled;",
	"P2-;",
	"P2oAC,Distributor,Amstrad,Orion,Schneider,Awa,Solavox,Saisho,Triumph,Isp;",
	"d3P2O[5:4],Model,CPC 6128,CPC 664,CPC 464;",
	"P2O[34:33],Plus model,Off,GX4000,6128+,464+;",
	"P2OV,Tape progressbar,Off,On;",
	"P2O[37],SSM markers,Off,On;",

	"-;",
	"R0,Reset & apply model;",
	"R[32],Reset & Detach Dandanator;",
	"J,Fire 1,Fire 2,Fire 3;",
	"I,Snapshot saved,Snapshot refused,Snapshot cancelled;",
	"V,v",`BUILD_ID
};

//////////////////////////////////////////////////////////////////////////

wire clk_sys;
wire locked;
wire [15:0] status_menumask;
wire st_right_shift_mod = status[22];
wire st_keypad_mod = status[23];
wire st_progressbar = status[31];

pll pll
(
	.refclk(CLK_50M),
	.outclk_0(clk_sys),
	.locked(locked)
);

reg ce_ref, ce_u765;
reg ce_16;
always @(posedge clk_sys) begin
	reg [2:0] div = 0;

	div     <= div + 1'd1;

	ce_ref  <= !div;
	ce_u765 <= !div[2:0]; //8 MHz
	ce_16   <= !div[1:0]; //16 MHz
end

//////////////////////////////////////////////////////////////////////////

wire [31:0] sd_lba;
wire  [1:0] sd_rd;
wire  [1:0] sd_wr;
wire  [1:0] sd_ack;
wire  [8:0] sd_buff_addr;
wire  [7:0] sd_buff_dout;
wire  [7:0] sd_buff_din;
wire        sd_buff_wr;
wire  [1:0] img_mounted;
wire [63:0] img_size;
wire        img_readonly;

wire        ioctl_wr;
wire [24:0] ioctl_addr;
wire  [7:0] ioctl_dout;
wire        ioctl_download;
wire  [7:0] ioctl_index;
wire [31:0] ioctl_file_ext;
wire        ioctl_wait;

// OSD info popup for the SNA save (index into the CONF_STR "I," entry),
// driven beside the save logic below the SSM marker.
reg         save_info_req = 1'b0;
reg   [7:0] save_info = 8'd0;

wire [10:0] ps2_key;
wire [24:0] ps2_mouse;

wire  [1:0] buttons;
wire  [6:0] joy1_usb;
wire  [6:0] joy2_usb;
wire [63:0] status;

wire        forced_scandoubler;
wire [21:0] gamma_bus;

wire  [1:0] snac_player = status[62:61];
wire [15:0] joydb_1;
wire [15:0] joydb_2;
wire        joydb_1ena;
wire        joydb_2ena;

joydb joydb
(
	.USER_IN(USER_IN),
	.snac_player(snac_player),
	.joystick1(joydb_1),
	.joystick2(joydb_2),
	.joystick1_en(joydb_1ena),
	.joystick2_en(joydb_2ena)
);

wire [6:0] joy1_db9 = OSD_STATUS ? 7'd0 : {joydb_1[10], joydb_1[6], joydb_1[4], joydb_1[3:0]};
wire [6:0] joy2_db9 = OSD_STATUS ? 7'd0 : {joydb_2[10], joydb_2[6], joydb_2[4], joydb_2[3:0]};
wire [6:0] joy1     = joydb_1ena ? joy1_db9 : joy1_usb;
wire [6:0] joy2     = joydb_2ena ? joy2_db9 : joydb_1ena ? joy1_usb : joy2_usb;

wire [63:0] load_status_in;
wire load_status_set;
wire [1:0] plus_model;

hps_io #(.CONF_STR(CONF_STR), .VDNUM(2)) hps_io
(
	.clk_sys(clk_sys),
	.HPS_BUS(HPS_BUS),

	.img_mounted(img_mounted),
	.img_size(img_size),
	.img_readonly(img_readonly),
	.sd_lba('{sd_lba,sd_lba}),
	.sd_rd(sd_rd),
	.sd_wr(sd_wr),
	.sd_ack(sd_ack),
	.sd_buff_addr(sd_buff_addr),
	.sd_buff_dout(sd_buff_dout),
	.sd_buff_din('{sd_buff_din,sd_buff_din}),
	.sd_buff_wr(sd_buff_wr),

	.ps2_key(ps2_key),
	.ps2_mouse(ps2_mouse),

	.joystick_0(joy1_usb),
	.joystick_1(joy2_usb),

	.buttons(buttons),
	.status(status),
	.status_in({64'd0,load_status_in}),
	.status_set(load_status_set),
	.status_menumask(status_menumask),

	.forced_scandoubler(forced_scandoubler),
	.gamma_bus(gamma_bus),

	.ioctl_wr(ioctl_wr),
	.ioctl_addr(ioctl_addr),
	.ioctl_dout(ioctl_dout),
	.ioctl_download(ioctl_download),
	.ioctl_index(ioctl_index),
	.ioctl_file_ext(ioctl_file_ext),
	.ioctl_wait(ioctl_wait),

	.info_req(save_info_req),
	.info(save_info)
);

wire        rom_download = ioctl_download && rom_route_active;
wire        tape_download = ioctl_download && (ioctl_index == 4);
wire        dan_download = ioctl_download && (ioctl_index == 5);
wire        dan_write_accepted;
wire        sna_download = ioctl_download && (ioctl_index == 6);
wire        cpr_download = ioctl_download && (ioctl_index == 8);
wire        cpr_ioctl_wait;

dandanator_loader_bounds dandanator_loader_bounds
(
	.dan_download(dan_download),
	.ioctl_wr(ioctl_wr),
	.ioctl_addr(ioctl_addr),
	.write_accepted(dan_write_accepted)
);
wire [24:0] sna_mem_addr = ioctl_addr - 25'h100;
wire [15:0] sna_std_mem_size = (sna_mem_size > 16'd128) ? 16'd128 : sna_mem_size;
wire [24:0] sna_chunk_start = 25'h100 + {sna_std_mem_size[14:0], 10'd0};

wire [211:0] sna_cpu_dir;
// The selected video/GA header fields (10, 2E, 2F-3F, 40, 42, 43-54, 55,
// A9-B4) are decoded by rtl/plus/plus_sna_header.v instead of inline here, so
// the P8 fixture drives the same decoder from a real byte stream (B8-5).
wire  [4:0] sna_crtc_addr;
wire [143:0] sna_crtc_regs;
wire  [4:0] sna_ga_inksel;
wire [135:0] sna_ga_palette;
wire  [7:0] sna_ga_config;
// The Z80/PPI/PSG/RAM-config/memory-size header fields (11-2D, 41, 56-59,
// 5A-6A, 6B-6D) are decoded by rtl/sna_cpu_header.v instead of inline here,
// so the B18 round-trip fixture drives the same decoder from a real byte
// stream (B18 slice 2).
wire  [7:0] sna_ram_config;
wire  [7:0] sna_rom_select;
wire  [7:0] sna_ppi_a;
wire  [7:0] sna_ppi_b;
wire  [7:0] sna_ppi_c;
wire  [7:0] sna_ppi_control;
wire  [3:0] sna_psg_addr;
wire [127:0] sna_psg_regs;
wire  [15:0] sna_mem_size;
// sna_model single-driver design (B18 slice 2): the header-decoded model
// lives in sna_cpu_header (its only procedural driver is that module's
// always block), while the MEM1/CPC+ chunk parser below owns the registered
// sna_chunk_force_128k flag. sna_model is their combination, so every
// consumer (boot_bank, mem_bank, model) sees one value with no multi-driver.
wire  [1:0] sna_hdr_model;
reg           sna_chunk_force_128k = 1'b0;
wire  [1:0] sna_model;
assign sna_model = sna_chunk_force_128k ? 2'd0 : sna_hdr_model;
wire  [2:0] sna_apply_cnt;
wire        sna_finish_pending;
wire        sna_load;
wire        sna_hold;
wire        sna_owner_reset_hold;
reg         old_sna_download_reset = 1'b0;
reg   [2:0] cpr_apply_cnt = 3'd0;
reg         cpr_finish_pending = 1'b0;
reg         old_cpr_download = 1'b0;
reg         old_cpr_download_reset = 1'b0;

wire        sna_hsync;
wire        sna_crtc_vs;
wire        sna_crtc_adj;
wire  [7:0] sna_crtc_hcc;
wire  [6:0] sna_crtc_line;
wire  [4:0] sna_crtc_raster;
wire  [4:0] sna_crtc_vta;
wire  [3:0] sna_crtc_hsw;
wire  [3:0] sna_crtc_vsw;
wire  [1:0] sna_ga_vsdelay;
wire  [5:0] sna_ga_intcnt;
wire        sna_int_pending;

// Selected video / Gate-Array header decode, shared with sim/plus's P8
// fixture. `wr` is exactly the old inline condition: a header byte of the
// snapshot currently downloading.
plus_sna_header sna_header
(
	.clk(clk_sys),
	.sna_download(sna_download),
	.wr(sna_download && ioctl_wr && (ioctl_addr < 25'h100)),
	.addr(ioctl_addr[7:0]),
	.data(ioctl_dout),

	.version(),
	.v3(),

	.ga_inksel(sna_ga_inksel),
	.ga_palette(sna_ga_palette),
	.ga_config(sna_ga_config),
	.crtc_addr(sna_crtc_addr),
	.crtc_regs(sna_crtc_regs),
	.rom_select(sna_rom_select),

	.crtc_hcc(sna_crtc_hcc),
	.crtc_line(sna_crtc_line),
	.crtc_raster(sna_crtc_raster),
	.crtc_vta(sna_crtc_vta),
	.crtc_hsw(sna_crtc_hsw),
	.crtc_vsw(sna_crtc_vsw),
	.crtc_vs(sna_crtc_vs),
	.crtc_hs(sna_hsync),
	.crtc_adj(sna_crtc_adj),
	.ga_vsdelay(sna_ga_vsdelay),
	.ga_intcnt(sna_ga_intcnt),
	.int_pending(sna_int_pending)
);

// Declared before sna_cpu_header below: a port connection to a
// not-yet-declared name becomes a 1-bit implicit net in Quartus (Warning
// 10236, which CI rejects) instead of a forward reference.
function automatic [1:0] valid_model(input [1:0] requested);
	begin
		valid_model = (requested == 2'd3) ? 2'd0 : requested;
	end
endfunction

wire [1:0] menu_model = valid_model(status[5:4]);

// Z80/PPI/PSG/RAM-config/memory-size header decode, shared with the B18
// save/reload fixture. `wr`/`addr`/`data` are exactly the old inline
// condition: a header byte of the snapshot currently downloading.
//
// Timing argument for sna_model: the header bytes (offsets < 0x100) always
// arrive before any chunk header (which starts at sna_chunk_start >= 0x100),
// and the chunk parser only sets sna_chunk_force_128k, never clears it, while
// sna_hdr_model never changes after the header phase (a chunk-phase write has
// addr >= 0x100, so the module's `wr` is low). The combination therefore
// settles exactly as the old single register did: header-decoded value until
// the first MEM1/CPC+ chunk header completes, 2'd0 from the next cycle on.
// The download-start clear of the flag and the module's reset to menu_model
// fire on the same edge cycle (see the module header comment), so the first
// post-start value is menu_model, as before. Clocked consumers (boot_bank,
// model) sample the combination one cycle after each source update, matching
// the old nonblocking-update visibility.
sna_cpu_header sna_cpu_header
(
	.clk(clk_sys),
	.sna_download(sna_download),
	.wr(sna_download && ioctl_wr && (ioctl_addr < 25'h100)),
	.addr(ioctl_addr[7:0]),
	.data(ioctl_dout),
	.menu_model(menu_model),

	.cpu_dir(sna_cpu_dir),
	.ram_config(sna_ram_config),
	.ppi_a(sna_ppi_a),
	.ppi_b(sna_ppi_b),
	.ppi_c(sna_ppi_c),
	.ppi_control(sna_ppi_control),
	.psg_addr(sna_psg_addr),
	.psg_regs(sna_psg_regs),
	.mem_size(sna_mem_size),
	.model(sna_hdr_model)
);

wire [11:0] plus_sna_loop_cnt0;
wire [11:0] plus_sna_loop_cnt1;
wire [11:0] plus_sna_loop_cnt2;
wire [15:0] plus_sna_loop_addr0;
wire [15:0] plus_sna_loop_addr1;
wire [15:0] plus_sna_loop_addr2;
wire [11:0] plus_sna_pause_cnt0;
wire [11:0] plus_sna_pause_cnt1;
wire [11:0] plus_sna_pause_cnt2;
wire  [7:0] plus_sna_pause_presc0;
wire  [7:0] plus_sna_pause_presc1;
wire  [7:0] plus_sna_pause_presc2;
wire  [4:0] plus_sna_seq_state;

wire        sna_mem_wr = sna_download && ioctl_wr && (ioctl_addr >= 25'h100) &&
                         (ioctl_addr < sna_chunk_start) && ({9'd0, sna_mem_addr[16:10]} < sna_std_mem_size);
reg  [31:0] sna_chunk_name = 32'd0;
reg  [31:0] sna_chunk_len = 32'd0;
reg  [31:0] sna_chunk_rem = 32'd0;
reg   [2:0] sna_chunk_hdr = 3'd0;
reg         sna_chunk_data = 1'b0;
reg         sna_chunk_mem = 1'b0;
reg         sna_chunk_rle = 1'b0;
reg         sna_chunk_finish = 1'b0;
reg   [3:0] sna_chunk_bank = 4'd0;
reg  [15:0] sna_chunk_out = 16'd0;
reg   [1:0] sna_rle_state = 2'd0;
reg   [7:0] sna_rle_count = 8'd0;
reg   [7:0] sna_rle_value = 8'd0;
reg         sna_chunk_cpc_plus = 1'b0;
reg         sna_cpc_plus_start = 1'b0;
reg         sna_cpc_plus_wr    = 1'b0;
reg   [7:0] sna_cpc_plus_data  = 8'd0;

wire        plus_sna_wr;
wire [13:0] plus_sna_addr;
wire  [7:0] plus_sna_data;
wire        plus_sna_active;
wire  [7:0] plus_sna_rmr2;
wire        plus_sna_unlock;
wire        plus_sna_ioctl_wait;
wire        plus_sna_busy;

assign ioctl_wait = romdl_wait | (sna_download && ((|sna_rle_count && (sna_rle_state == 2'd0)) || plus_sna_ioctl_wait)) | cpr_ioctl_wait | tape_queue_wait;

// A 8MB bank is split to 2 halves
// Fist 4 MB is OS ROM + RAM pages + MF2 ROM
// Second 4 MB is max. 256 pages of HI rom

reg         boot_wr = 0;
reg  [22:0] boot_a;
reg   [1:0] boot_bank;
reg   [7:0] boot_dout;

reg [255:0] rom_map = '0;

// B10-3: page/combo have a single sequential driver (the download
// always block below, the only page <= / combo <= writer); rom_loader_route
// reads page combinationally and never writes it.
reg   [8:0] page = 0;
reg         combo = 0;

wire        rom_route_active;
wire        rom_route_valid;
wire  [1:0] rom_route_initial_bank;
wire  [8:0] rom_route_dest_a_hi;
wire        rom_route_promote;

rom_loader_route rom_loader_route
(
	.ioctl_index(ioctl_index),
	.ioctl_addr(ioctl_addr[24:14]),
	.page(page),
	.rom_active(rom_route_active),
	.addr_valid(rom_route_valid),
	.initial_bank(rom_route_initial_bank),
	.dest_a_hi(rom_route_dest_a_hi),
	.promote_bank0_to_bank1(rom_route_promote)
);

reg         romdl_wait = 0;
always @(posedge clk_sys) begin
	reg       old_download;

	sna_cpc_plus_start <= 1'b0;
	sna_cpc_plus_wr    <= 1'b0;

	if(!romdl_wait && sna_rle_count && (sna_rle_state == 2'd0) && sna_chunk_mem && (sna_chunk_bank < 4'd2)) begin
		romdl_wait <= 1;
		boot_dout <= sna_rle_value;
		boot_bank <= sna_model;
		boot_a[22:14] <= 9'd8 + {5'd0, sna_chunk_bank[1:0], sna_chunk_out[15:14]};
		boot_a[13:0] <= sna_chunk_out[13:0];
		sna_chunk_out <= sna_chunk_out + 1'd1;
		sna_rle_count <= sna_rle_count - 1'd1;
		if((sna_rle_count == 8'd1) && sna_chunk_finish) begin
			sna_chunk_data <= 1'b0;
			sna_chunk_hdr <= 3'd0;
			sna_chunk_name <= 32'd0;
			sna_chunk_len <= 32'd0;
			sna_chunk_mem <= 1'b0;
			sna_chunk_rle <= 1'b0;
			sna_chunk_finish <= 1'b0;
			sna_rle_state <= 2'd0;
		end
	end
	else if((rom_download && ioctl_wr) || dan_write_accepted || sna_mem_wr) begin
		romdl_wait <= 1;
		boot_dout <= ioctl_dout;

		boot_a[13:0] <= ioctl_addr[13:0];

		if (sna_mem_wr) begin
			boot_bank <= sna_model;
			boot_a[22:14] <= 9'd8 + {6'd0, sna_mem_addr[16:14]};
			boot_a[13:0] <= sna_mem_addr[13:0];
		end
		else if (dan_write_accepted) begin
			boot_bank <= 2'b11;
			boot_a[22:14] <= ioctl_addr[22:14];
		end 
		else if (rom_download) begin
			if (rom_route_valid) begin
				boot_bank     <= rom_route_initial_bank;
				boot_a[22:14] <= rom_route_dest_a_hi;
			end
			else romdl_wait <= 0;
		end
	end

	if(ce_ref) begin
		boot_wr <= romdl_wait;
		if(boot_wr & romdl_wait) begin
			boot_wr <= 0;
			// load expansion ROM into both banks if manually loaded or boot name is boot.eXX
			if(rom_download && rom_route_promote && !boot_bank) boot_bank <= 1;
			else begin
				{boot_wr, romdl_wait} <= 0;
				if(boot_a[22]) rom_map[boot_a[21:14]] <= 1;
				if(combo && &boot_a[13:0]) begin
					combo <= 0;
					page  <= 9'h1FF;
				end
			end
		end
	end

	old_download <= ioctl_download;
	if(~old_download & ioctl_download & rom_download) begin
		if(ioctl_index) begin
			page <= 9'h1EE; // some unused page for malformed file extension
			combo <= 0;
			if(ioctl_file_ext[15:8] >= "0" && ioctl_file_ext[15:8] <= "9") page[7:4] <= ioctl_file_ext[11:8];
			if(ioctl_file_ext[15:8] >= "A" && ioctl_file_ext[15:8] <= "F") page[7:4] <= ioctl_file_ext[11:8]+4'd9;
			if(ioctl_file_ext[7:0]  >= "0" && ioctl_file_ext[7:0]  <= "9") page[3:0] <= ioctl_file_ext[3:0];
			if(ioctl_file_ext[7:0]  >= "A" && ioctl_file_ext[7:0]  <= "F") page[3:0] <= ioctl_file_ext[3:0] +4'd9;
			if(ioctl_file_ext[15:0] == "ZZ") page <= 0;
			if(ioctl_file_ext[15:0] == "Z0") begin page <= 0; combo <= 1; end
		end
	end
	if(~old_download & ioctl_download & sna_download) begin
		sna_chunk_force_128k <= 1'b0;
		sna_chunk_name <= 32'd0;
		sna_chunk_len <= 32'd0;
		sna_chunk_rem <= 32'd0;
		sna_chunk_hdr <= 3'd0;
		sna_chunk_data <= 1'b0;
		sna_chunk_mem <= 1'b0;
		sna_chunk_rle <= 1'b0;
		sna_chunk_finish <= 1'b0;
		sna_chunk_bank <= 4'd0;
		sna_chunk_out <= 16'd0;
		sna_rle_state <= 2'd0;
		sna_rle_count <= 8'd0;
		sna_rle_value <= 8'd0;
		sna_chunk_cpc_plus <= 1'b0;
		sna_cpc_plus_start <= 1'b0;
		sna_cpc_plus_wr    <= 1'b0;
		sna_cpc_plus_data  <= 8'd0;
	end
	if(sna_download && ioctl_wr && !romdl_wait && (!sna_rle_count || (sna_rle_state == 2'd2)) && (ioctl_addr >= sna_chunk_start)) begin
		if(!sna_chunk_data) begin
			case(sna_chunk_hdr)
				3'd0: sna_chunk_name[31:24] <= ioctl_dout;
				3'd1: sna_chunk_name[23:16] <= ioctl_dout;
				3'd2: sna_chunk_name[15:8]  <= ioctl_dout;
				3'd3: sna_chunk_name[7:0]   <= ioctl_dout;
				3'd4: sna_chunk_len[7:0]    <= ioctl_dout;
				3'd5: sna_chunk_len[15:8]   <= ioctl_dout;
				3'd6: sna_chunk_len[23:16]  <= ioctl_dout;
				3'd7: begin
					reg [31:0] next_name;
					reg [31:0] next_len;
					next_name = sna_chunk_name;
					next_len = {ioctl_dout, sna_chunk_len[23:0]};
					sna_chunk_len[31:24] <= ioctl_dout;
					sna_chunk_rem <= {ioctl_dout, sna_chunk_len[23:0]};
					sna_chunk_data <= |next_len;
					sna_chunk_mem <= (next_name[31:24] == "M") && (next_name[23:16] == "E") &&
					                 (next_name[15:8] == "M") && (next_name[7:0] >= "0") &&
					                 (next_name[7:0] <= "1");
					sna_chunk_cpc_plus <= (next_name[31:24] == "C") && (next_name[23:16] == "P") &&
					                      (next_name[15:8] == "C") && (next_name[7:0] == "+");
					sna_chunk_rle <= (next_len != 32'd65536);
					sna_chunk_finish <= 1'b0;
					sna_chunk_bank <= next_name[3:0];
					if((next_name[31:24] == "M") && (next_name[23:16] == "E") &&
					   (next_name[15:8] == "M") && (next_name[7:0] == "1")) sna_chunk_force_128k <= 1'b1;
					if((next_name[31:24] == "C") && (next_name[23:16] == "P") &&
					   (next_name[15:8] == "C") && (next_name[7:0] == "+")) begin
						sna_cpc_plus_start <= 1'b1;
						sna_chunk_force_128k <= 1'b1;
					end
					sna_chunk_out <= 16'd0;
					sna_rle_state <= 2'd0;
					sna_rle_count <= 8'd0;
				end
			endcase
			sna_chunk_hdr <= sna_chunk_hdr + 1'd1;
		end
		else begin
			reg [31:0] next_rem;
			next_rem = sna_chunk_rem - 1'd1;
			sna_chunk_rem <= next_rem;
			if(sna_chunk_cpc_plus) begin
				sna_cpc_plus_wr   <= 1'b1;
				sna_cpc_plus_data <= ioctl_dout;
			end
			if(sna_chunk_mem && (sna_chunk_bank < 4'd2)) begin
				if(!sna_chunk_rle) begin
					romdl_wait <= 1;
					boot_dout <= ioctl_dout;
					boot_bank <= sna_model;
					boot_a[22:14] <= 9'd8 + {5'd0, sna_chunk_bank[1:0], sna_chunk_out[15:14]};
					boot_a[13:0] <= sna_chunk_out[13:0];
					sna_chunk_out <= sna_chunk_out + 1'd1;
				end
				else begin
					case(sna_rle_state)
						2'd0: begin
							if(ioctl_dout == 8'he5) sna_rle_state <= 2'd1;
							else begin
								romdl_wait <= 1;
								boot_dout <= ioctl_dout;
								boot_bank <= sna_model;
								boot_a[22:14] <= 9'd8 + {5'd0, sna_chunk_bank[1:0], sna_chunk_out[15:14]};
								boot_a[13:0] <= sna_chunk_out[13:0];
								sna_chunk_out <= sna_chunk_out + 1'd1;
							end
						end
						2'd1: begin
							if(ioctl_dout == 8'd0) begin
								romdl_wait <= 1;
								boot_dout <= 8'he5;
								boot_bank <= sna_model;
								boot_a[22:14] <= 9'd8 + {5'd0, sna_chunk_bank[1:0], sna_chunk_out[15:14]};
								boot_a[13:0] <= sna_chunk_out[13:0];
								sna_chunk_out <= sna_chunk_out + 1'd1;
								sna_rle_state <= 2'd0;
							end
							else begin
								sna_rle_count <= ioctl_dout;
								sna_rle_state <= 2'd2;
							end
						end
						2'd2: begin
							sna_rle_value <= ioctl_dout;
							romdl_wait <= 1;
							boot_dout <= ioctl_dout;
							boot_bank <= sna_model;
							boot_a[22:14] <= 9'd8 + {5'd0, sna_chunk_bank[1:0], sna_chunk_out[15:14]};
							boot_a[13:0] <= sna_chunk_out[13:0];
							sna_chunk_out <= sna_chunk_out + 1'd1;
							sna_rle_count <= sna_rle_count - 1'd1;
							sna_rle_state <= 2'd0;
						end
					endcase
				end
			end
			if(next_rem == 32'd0 && sna_chunk_rle && sna_chunk_mem && (sna_chunk_bank < 4'd2) &&
			   (sna_rle_state == 2'd2) && (sna_rle_count > 8'd1)) begin
				sna_chunk_finish <= 1'b1;
			end
			else if(next_rem == 32'd0) begin
				sna_chunk_data <= 1'b0;
				sna_chunk_hdr <= 3'd0;
				sna_chunk_name <= 32'd0;
				sna_chunk_len <= 32'd0;
				sna_chunk_mem <= 1'b0;
				sna_chunk_rle <= 1'b0;
				sna_chunk_cpc_plus <= 1'b0;
				sna_chunk_finish <= 1'b0;
				sna_rle_state <= 2'd0;
			end
		end
	end
	old_cpr_download <= cpr_download;
	if(old_cpr_download & ~cpr_download) cpr_finish_pending <= 1'b1;
	if(cpr_finish_pending && !cart_service_busy) begin
		cpr_finish_pending <= 1'b0;
		cpr_apply_cnt <= 3'd7;
	end
	else if(cpr_apply_cnt) cpr_apply_cnt <= cpr_apply_cnt - 1'd1;
end

plus_sna_apply sna_apply
(
	.clk(clk_sys),
	.sna_download(sna_download),
	.romdl_wait(romdl_wait),
	.boot_wr(boot_wr),
	.sna_rle_count(sna_rle_count),
	.plus_sna_busy(plus_sna_busy),
	.finish_pending(sna_finish_pending),
	.apply_cnt(sna_apply_cnt),
	.sna_load(sna_load),
	.sna_hold(sna_hold),
	.owner_reset_hold(sna_owner_reset_hold)
);


// Model selection precedes owner reset release and sna_load. Do not wait for
// Main's asynchronous status echo: the local model feeds every Plus consumer.
plus_load_model load_model
(
 .clk(clk_sys),
 .status(status),
 .fn_toggle(Fn[1]),
 .cpr_apply(cpr_finish_pending && !cart_service_busy),
 .cpr_image_valid(cart_image_valid),
 .sna_download(sna_download),
 .sna_header_wr(sna_download && ioctl_wr && (ioctl_addr < 25'h100)),
 .sna_addr(ioctl_addr[7:0]),
 .sna_data(ioctl_dout),
 .sna_prepare(sna_apply_cnt == 3'd5),
 .plus_model(plus_model),
 .status_in(load_status_in),
 .status_set(load_status_set)
);


//////////////////////////////////////////////////////////////////////////

wire        mem_wr;
wire        mem_rd;
wire [22:0] ram_a;
wire  [7:0] ram_dout;

// Plus cartridge memory service -> SDRAM held-request port. Declared ahead
// of both the sdram instance and the service instance below.
wire        cart_mem_req, cart_mem_write, cart_mem_ack, cart_mem_grant;
wire  [1:0] cart_mem_bank;
wire [22:0] cart_mem_addr;
wire  [7:0] cart_mem_wdata, cart_mem_rdata;
wire        cart_image_valid;
wire        cart_service_busy;

// B18 SNA save. The save stream shares the SDRAM cartridge port with the
// cartridge memory service through sna_cart_mux (sdram_cart_*), and reads
// machine state from the motherboard and u765 observation ports. Declared
// here, ahead of the sdram, u765 and motherboard instances; the save logic
// itself sits below the SSM marker.
wire        save_cart_req, save_cart_ack;
wire  [1:0] save_cart_bank;
wire [22:0] save_cart_addr;
wire        sdram_cart_req, sdram_cart_wr, sdram_cart_ack, sdram_cart_grant;
wire  [1:0] sdram_cart_bank;
wire [22:0] sdram_cart_addr;
wire  [7:0] sdram_cart_din;

wire  [4:0] snap_ga_inksel, snap_ga_border;
wire [79:0] snap_ga_inkr;
wire        snap_ga_hromen, snap_ga_lromen;
wire  [1:0] snap_ga_mode;
wire  [5:0] snap_ga_intcnt;
wire  [4:0] snap_ga_hcnt;
wire        snap_ga_int_n;
wire  [4:0] snap_crtc_addr;
wire [127:0] snap_crtc_regs;
wire  [7:0] snap_crtc_hcc;
wire  [6:0] snap_crtc_row;
wire  [4:0] snap_crtc_line, snap_crtc_c5;
wire        snap_crtc_in_adj;
wire  [3:0] snap_crtc_hsc;
wire        snap_crtc_hsync, snap_crtc_vsync_r;
wire  [3:0] snap_crtc_vsw_elapsed;
wire  [2:0] snap_mmu_rammap;
wire  [4:0] snap_mmu_rampage;
wire  [7:0] snap_mmu_rom_select_shadow;
wire  [7:0] snap_ppi_porta_in, snap_ppi_portb_in, snap_ppi_opc_r, snap_ppi_mode;
wire  [7:0] snap_psg_addr;
wire [127:0] snap_psg_regs;
wire  [7:0] snap_printer_data;
wire  [7:0] snap_pcn_a, snap_pcn_b;
wire [211:0] cpu_reg;
wire        cpu_insn_start, cpu_halt_n;
wire        save_hold;

wire [15:0] vram_dout;
wire [14:0] vram_addr;

// Plus model decode outputs are declared before the SDRAM bank select (HF-3).
// plus_model comes from load_model, including a pending selection until Main
// echoes the OSD status request.
wire       plus_mode;
wire       plus_ram_128k;
wire       plus_has_fdc;
wire       plus_has_tape;

// RAM bank for the running machine (HF-3). In Plus mode all Plus models
// (6128+, GX4000, 464+) use bank 0 where Plus RAM and SNA snapshot data
// reside; .ram64k on the motherboard instance configures whether the 128K
// banking expansion is active. In classic mode the bank follows `model` with
// the `sna_load` bridge during snapshot restore edges.
wire [1:0] mem_bank = plus_mode ? 2'b00 : (sna_load ? sna_model : model);

sdram sdram
(
	.*,

	.init(~locked),
	.clk(clk_sys),
	.clkref(ce_ref),

	.oe  (reset ? 1'b0      : mem_rd & ~mf2_ram_en & ~plus_cart_own & ~plus_aspage_sel),
	.we  (reset ? boot_wr   : mem_wr & ~mf2_ram_en & ~mf2_rom_en & ~plus_aspage_sel),
	.addr(reset ? boot_a    : mf2_rom_en ? {9'h0ff, cpu_addr[13:0]} : dan_ena ? {4'd0, dan_bank, cpu_addr[13:0]} : ram_a),
	.bank(reset ? boot_bank : dan_ena ? 2'b11 : mem_bank),
	.din (reset ? boot_dout : cpu_dout),
	.dout(ram_dout),
	// Cartridge memory service (P-1 contract, production-connected at P0).
	// The service owns all cartridge region policy; the controller sees a
	// generic held request on bank 3. The SNA save borrows the same port
	// through sna_cart_mux; cart_dout is shared, the acknowledge is not.
	.cart_req(sdram_cart_req),
	.cart_wr(sdram_cart_wr),
	.cart_bank(sdram_cart_bank),
	.cart_addr(sdram_cart_addr),
	.cart_din(sdram_cart_din),
	.cart_dout(cart_mem_rdata),
	.cart_ack(sdram_cart_ack),
	.cart_grant(sdram_cart_grant),
	.vram_bank(mem_bank),
	.vram_addr({2'b10,vram_addr,1'b0}),
	.vram_dout(vram_dout),

	.tape_addr(tape_sdram_addr),
	.tape_din(tape_din),
	.tape_dout(tape_dout),
	.tape_wr(tape_wr),
	.tape_wr_ack(tape_wr_ack),
	.tape_rd(tape_data_req ^ tape_data_ack),
	.tape_rd_ack(tape_data_ack)
);

reg [1:0] model = 2'd0;
reg reset;

wire reset_base = RESET | status[0] | status[32] | buttons[1] | rom_download | key_reset | dan_download |
                  sna_owner_reset_hold;

// The SNA parser owns its download lifecycle.  In particular it must remain
// live while sna_download is asserted and retain its CPC+ RMR2/unlock shadows
// through sna_finish_pending/sna_apply_cnt until the delayed sna_load pulse.
// Other reset/download sources still discard an incomplete or retained SNA.
wire sna_parser_reset = RESET | status[0] | status[32] | buttons[1] | rom_download | key_reset | dan_download |
                        cpr_download | cpr_finish_pending | (old_cpr_download_reset & ~cpr_download) |
                        (cpr_apply_cnt != 3'd0);

wire plus_asic_reset = sna_parser_reset | (sna_download & ~old_sna_download_reset);

always @(posedge clk_sys) begin
	if(sna_load) model <= sna_model;
	else if(reset) model <= menu_model;
	old_sna_download_reset <= sna_download;
	old_cpr_download_reset <= cpr_download;
	reset <= reset_base | cpr_download | cpr_finish_pending |
	         (old_cpr_download_reset & ~cpr_download) | (cpr_apply_cnt != 3'd0);
end

////////////////////// CDT playback ///////////////////////////////

// B8-7: the download write queue (held request, queued payload/address,
// ioctl backpressure, drain-held address ownership) lives in
// rtl/tape_write_queue.v so the physical-DQ fixture executes the same seam.
wire [22:0] tape_last_addr;
wire  [7:0] tape_din;
wire        tape_wr;
wire        tape_wr_ack;
wire [22:0] tape_sdram_addr;
wire        tape_queue_wait;
wire        tape_queue_pending;
wire        tape_read;
wire        tape_running;
wire        tape_data_req;
wire        tape_data_ack;
reg         tape_reset;
wire  [7:0] tape_dout;
reg  [22:0] tape_play_addr;
wire        tape_motor;

tape_write_queue tape_queue
(
	.clk(clk_sys),
	.reset(reset),
	.clear(reset | Fn[2]),
	.tape_download(tape_download),
	.ioctl_wr(ioctl_wr),
	.ioctl_dout(ioctl_dout),
	.ioctl_addr(ioctl_addr[22:0]),
	.tape_wr_ack(tape_wr_ack),
	.tape_play_addr(tape_play_addr),
	.tape_wr(tape_wr),
	.tape_din(tape_din),
	.tape_queued_addr(tape_last_addr),
	.tape_addr(tape_sdram_addr),
	.tape_wait(tape_queue_wait),
	.tape_pending(tape_queue_pending)
);

always @(posedge clk_sys) begin
	reg old_tape_ack;
	reg old_dan_download;

	old_tape_ack <= tape_data_ack;

	if (reset | Fn[2]) begin
		tape_play_addr <= 0;
		tape_reset <= 1;
	end
	else begin
		tape_reset <= 0;
		// B8-7: keep download address ownership and the player reset until
		// the pending write drains, even if the download ends first.
		if (tape_download | tape_queue_pending) begin
			tape_play_addr <= 0;
			tape_reset <= 1;
		end
		else if ((old_tape_ack ^ tape_data_ack) && (tape_play_addr < tape_last_addr)) begin
			tape_play_addr <= tape_play_addr + 1'd1;
		end
	end
end

tzxplayer #(
	.NORMAL_PILOT_LEN(2000),
	.NORMAL_SYNC1_LEN(855),
	.NORMAL_SYNC2_LEN(855),
	.NORMAL_ZERO_LEN(855),
	.NORMAL_ONE_LEN(1710),
	.HEADER_PILOT_PULSES(4095),
	.NORMAL_PILOT_PULSES(4095)
)
tzxplayer (
	.clk(clk_sys),
	.ce(1),
	.restart_tape(tape_reset),
	.host_tap_in(tape_dout),
	.tzx_req(tape_data_req),
	.tzx_ack(tape_data_ack),
	.cass_read(tape_read),
	.cass_motor(tape_motor),
	.cass_running(tape_running)
);

wire progress_pix;

progressbar progressbar(
	.clk(clk_sys),
	.ce_pix(ce_16),
	.hblank(hbl),
	.vblank(vbl),
	.enable(tape_running & st_progressbar),
	.current(tape_play_addr),
	.max(tape_last_addr),
	.pix(progress_pix)
);

wire tape_ready = tape_last_addr && (tape_play_addr <= tape_last_addr);
wire tape_led = act_cnt[24] ? act_cnt[23:16] > act_cnt[7:0] : act_cnt[23:16] <= act_cnt[7:0];

reg [24:0] act_cnt;
always @(posedge clk_sys) if((tape_ready & tape_motor) || ~act_cnt[24] || act_cnt[23:0]) act_cnt <= act_cnt + 1'd1;

//////////////////////////////////////////////////////////////////////////

// Classic and Plus FDC port selections are intentionally kept in one shared
// decoder so the production path and its focused test use the same equations.
// NFDC/NMOTOR are ASIC-generated on Plus: their I/O qualification sees
// IC116-shaped IORQ. Expansion peripherals retain raw io_rd/io_wr below.
// This follows board wiring; A13=0 FDC alias timing is not independently measured.
wire fdc_io_rd = rd & asic_iorq;
wire fdc_io_wr = wr & asic_iorq;
wire fdc_motor_sel;
wire [7:0] fdc_dout = (u765_sel & fdc_io_rd) ? u765_dout : 8'hFF;

reg motor = 0;
always @(posedge clk_sys) begin
	reg old_wr;
	
	old_wr <= fdc_io_wr;
	if (reset) begin
		motor <= 1'b0;
	end else if(~old_wr && fdc_io_wr && fdc_motor_sel) begin
		motor <= cpu_dout[0];
	end
end

wire [7:0] u765_dout;
wire       u765_sel;

plus_fdc_decode fdc_decode
(
	.addr(cpu_addr),
	.plus_mode(plus_mode),
	.plus_has_fdc(plus_has_fdc),
	.fdc_disabled(status[17]),
	.motor_sel(fdc_motor_sel),
	.u765_sel(u765_sel)
);

reg  [1:0] u765_ready = 0;
always @(posedge clk_sys) if(img_mounted[0]) u765_ready[0] <= |img_size;
always @(posedge clk_sys) if(img_mounted[1]) u765_ready[1] <= |img_size;

u765 u765
(
	.reset(reset),

	.clk_sys(clk_sys),
	.ce(ce_u765),
	
	.fast(status[16]),

	// CPC I/O map: A0 selects status/data on reads, but both A0 write
	// aliases address the uPD765 data register.
	.a0(cpu_addr[0] | (u765_sel & fdc_io_wr)),
	.ready(u765_ready),
	.motor({motor,motor}),
	.available(2'b11),
	.nRD(~(u765_sel & fdc_io_rd)),
	.nWR(~(u765_sel & fdc_io_wr)),
	.din(cpu_dout),
	.dout(u765_dout),

	.img_mounted(img_mounted),
	.img_size(img_size[31:0]),
	.img_wp(img_readonly),
	.sd_lba(sd_lba),
	.sd_rd(sd_rd),
	.sd_wr(sd_wr),
	.sd_ack(|sd_ack),
	.sd_buff_addr(sd_buff_addr),
	.sd_buff_dout(sd_buff_dout),
	.sd_buff_din(sd_buff_din),
	.sd_buff_wr(sd_buff_wr),
	.snap_pcn_a(snap_pcn_a),
	.snap_pcn_b(snap_pcn_b)
);

/////////////////////////////////////////////////////////////////////////
///////////////////////////// Multiface Two /////////////////////////////
/////////////////////////////////////////////////////////////////////////

wire  [7:0] mf2_dout = (mf2_ram_en & mem_rd) ? mf2_ram_out : 8'hFF;

reg         mf2_nmi = 0;
reg         mf2_en = 0;
reg         mf2_hidden = 0;
reg   [7:0] mf2_ram[8192];
wire        mf2_ram_en = mf2_en & cpu_addr[15:13] == 3'b001;
wire        mf2_rom_en = mf2_en & cpu_addr[15:13] == 3'b000;
reg   [4:0] mf2_pen_index;
reg   [3:0] mf2_crtc_register;
wire [12:0] mf2_store_addr;
reg  [12:0] mf2_ram_a;
reg         mf2_ram_we;
reg   [7:0] mf2_ram_in, mf2_ram_out;

always_comb begin
	casex({ cpu_addr[15:8], cpu_dout[7:6] })
		{ 8'h7f, 2'b00 }: mf2_store_addr = 13'h1fcf;  // pen index
		{ 8'h7f, 2'b01 }: mf2_store_addr = mf2_pen_index[4] ? 13'h1fdf : { 9'h1f9, mf2_pen_index[3:0] }; // border/pen color
		{ 8'h7f, 2'b10 }: mf2_store_addr = 13'h1fef; // screen mode
		{ 8'h7f, 2'b11 }: mf2_store_addr = 13'h1fff; // banking
		{ 8'hbc, 2'bXX }: mf2_store_addr = 13'h1cff; // CRTC register select
		{ 8'hbd, 2'bXX }: mf2_store_addr = { 9'h1db, mf2_crtc_register[3:0] }; // CRTC register value
		{ 8'hf7, 2'bXX }: mf2_store_addr = 13'h17ff; //8255
		{ 8'hdf, 2'bXX }: mf2_store_addr = 13'h1aac; //upper rom
		default: mf2_store_addr = 0;
	endcase
end

always @(posedge clk_sys) begin
	if (mf2_ram_we) begin
		mf2_ram[mf2_ram_a] <= mf2_ram_in;
		mf2_ram_out <= mf2_ram_in;
	end
	else mf2_ram_out <= mf2_ram[mf2_ram_a];
end

always @(posedge clk_sys) begin
	reg old_key_nmi, old_m1, old_io_wr;

	old_key_nmi <= key_nmi;
	old_m1 <= m1;
	old_io_wr <= io_wr;

	if (reset) begin
		mf2_en <= 0;
		mf2_hidden <= |status[15:14];
		mf2_nmi <= 0;
	end

	if(~old_key_nmi & key_nmi & ~mf2_en & ~status[15]) mf2_nmi <= 1;
	if (mf2_nmi & ~old_m1 & m1 & (cpu_addr == 'h66)) begin
		mf2_en <= 1;
		mf2_hidden <= 0;
		mf2_nmi <= 0;
	end
	if (mf2_en & ~old_m1 & m1 & cpu_addr == 'h65) begin
		mf2_hidden <= 1;
	end

	if (~old_io_wr & io_wr & cpu_addr[15:2] == 14'b11111110111010) begin //fee8/feea
		mf2_en <= ~cpu_addr[1] & ~mf2_hidden & ~status[15];
	end else if (~old_io_wr & io_wr & |mf2_store_addr[12:0]) begin //store hw register in MF2 RAM
		if (cpu_addr[15:8] == 8'h7f & cpu_dout[7:6] == 2'b00) mf2_pen_index <= cpu_dout[4:0];
		if (cpu_addr[15:8] == 8'hbc) mf2_crtc_register <= cpu_dout[3:0];
		mf2_ram_a <= mf2_store_addr;
		mf2_ram_in <= cpu_dout;
		mf2_ram_we <= 1;
	end else if (mem_wr & mf2_ram_en) begin //normal MF2 RAM write
		mf2_ram_a <= ram_a[12:0];
		mf2_ram_in <= cpu_dout;
		mf2_ram_we <= 1;
	end else begin //MF2 RAM read
		mf2_ram_a <= ram_a[12:0];
		mf2_ram_we <=0;
	end

end

//////////////////////////////////////////////////////////////////////

wire        playcity_ena = status[24];
wire  [7:0] playcity_dout;
wire  [7:0] playcity_audio_l, playcity_audio_r;
wire        playcity_int_n, playcity_nmi;

playcity playcity
(
	.clock(clk_sys),
	.reset(reset),
	.ena(playcity_ena),
	.phi_n(phi_n),
	.phi_en(phi_en_n),
	.addr(cpu_addr),
	.din(cpu_dout),
	.dout(playcity_dout),
	.cpu_di(cpu_din),
	.m1_n(~m1),
	.iorq_n(~iorq),
	.rd_n(~rd),
	.wr_n(~wr),
	.int_n(playcity_int_n),
	.nmi(playcity_nmi),
	.cursor(cursor),
	.audio_l(playcity_audio_l),
	.audio_r(playcity_audio_r)
);

//////////////////////////////////////////////////////////////////////

wire mouse_rd = io_rd & ~status[19];

wire [7:0] kmouse_dout;
kempston_mouse kmouse
(
	.clk_sys(clk_sys),
	.reset(reset),
	.ps2_mouse(ps2_mouse),
	.addr({cpu_addr[0], ~cpu_addr[4] & ~cpu_addr[10] & mouse_rd, cpu_addr[8]}),
	.dout(kmouse_dout)
);

wire [7:0] smouse_dout;
symbiface_mouse smouse
(
	.clk_sys(clk_sys),
	.reset(reset),
	.ps2_mouse(ps2_mouse),
	.sel((cpu_addr == 16'hFD10) & mouse_rd),
	.dout(smouse_dout)
);

wire [7:0] mmouse_dout;
multiplay_mouse mmouse
(
	.clk_sys(clk_sys),
	.reset(reset),
	.ps2_mouse(ps2_mouse),
	.sel((cpu_addr[15:4] == 12'hF99) & ~cpu_addr[3] & mouse_rd),
	.addr(cpu_addr[2:0]),
	.dout(mmouse_dout)
);

wire [6:0] amouse_dout;
amx_mouse amx_mouse
(
	.clk_sys(clk_sys),
	.reset(reset),
	.ps2_mouse(ps2_mouse),
	.sel(joy1_sel),
	.dout(amouse_dout)
);

/////////////////////////////////////////////////////////////////////////

wire [15:0] cpu_addr;
wire  [7:0] cpu_dout;
wire        phi_n, phi_en_p, phi_en_n;
wire        m1, key_nmi, key_reset;
wire        ssm_m1_fetch;
wire  [7:0] ssm_bus_data;
wire        rd, wr, iorq, asic_iorq;
wire        mreq;
wire        field;
wire        cursor;
wire  [9:0] Fn;
wire        tape_rec;
wire  [1:0] mode;
wire        joy1_sel;

wire        plus_cart_valid, plus_cart_ready, plus_cart_granted;
wire  [4:0] plus_cart_page;
wire [13:0] plus_cart_offset;
wire  [7:0] plus_cart_data;
wire        plus_cart_own, plus_cart_stall;
wire  [7:0] plus_cart_dout;
wire  [7:0] plus_io_bus_byte;
wire        plus_asic_unlocked;

// Cartridge-owned reads bypass the wired-AND entirely: the SDRAM main port
// is not asked for those cycles (see the sdram oe term below), so ram_dout
// holds stale bytes that must not participate. Classic mode never owns a
// cycle, so the mux reduces to the historical chain.
wire  [7:0] cpu_din_bus = ram_dout & mf2_dout & fdc_dout & kmouse_dout & smouse_dout & mmouse_dout & playcity_dout;
wire  [7:0] cpu_din = plus_vec_valid ? plus_vec_byte :
                      plus_asic_rd   ? plus_asic_dout :
                      plus_cart_own  ? plus_cart_dout : cpu_din_bus;
wire NMI = playcity_nmi | mf2_nmi;
wire        IRQ = ~playcity_int_n;

wire io_rd = rd & iorq;
wire io_wr = wr & iorq;
wire romen;
wire ready;

plus_model_select plus_model_decoder
(
	.plus_model(plus_model),
	.plus_mode(plus_mode),
	.ram_128k(plus_ram_128k),
	.has_fdc(plus_has_fdc),
	.has_tape(plus_has_tape)
);

// B6 capability visibility: classic machines retain both media controls;
// selected Plus models narrow them to the devices they actually provide.
plus_menu_capability_mask menu_capability_mask
(
	.en270p(en270p),
	.plus_mode(plus_mode),
	.plus_has_fdc(plus_has_fdc),
	.plus_has_tape(plus_has_tape),
	.status_menumask(status_menumask)
);

//////////////////// Plus cartridge path (P0) ///////////////////////////

wire plus_gx4000 = (plus_model == 2'b01);

// CPC Plus BASIC configuration: /EXP low selects BASIC at logical ROM 0;
// ROM 7 remains disc firmware. High requests the ROM-0 disc auto-boot route.
// GX4000 ignores /EXP in the MMU. Keep its live decoder polarity unchanged.
// Source rationale and production-T80 boot evidence: docs/plus/architecture.md D5.
wire plus_exp_n = 1'b0;

plus_mmu plus_mmu
(
	.clk(clk_sys),
	.reset(reset),
	.plus_mode(plus_mode),
	.gx4000(plus_gx4000),
	.io_rd(rd & asic_iorq),
	.io_wr(wr & asic_iorq),
	.mem_rd(mem_rd),
	.A(cpu_addr),
	.D((rd & asic_iorq) ? plus_io_bus_byte : cpu_dout),
	.rom_en(romen),
	.exp_n(plus_exp_n),

	.cart_valid(plus_cart_valid),
	.cart_page(plus_cart_page),
	.cart_offset(plus_cart_offset),
	.cart_ready(plus_cart_ready),
	.cart_granted(plus_cart_granted),
	.cart_data(plus_cart_data),
	.cart_busy(cart_service_busy),

	.cart_own(plus_cart_own),
	.cart_stall(plus_cart_stall),
	.cart_dout(plus_cart_dout),

	// Captured since P0; consumed when the ASIC register page gains its
	// backing at P2.
	.asic_page_on(plus_aspage_on),
	.asic_unlocked(plus_asic_unlocked),

	.sna_load(sna_load),
	.sna_rmr2(plus_sna_rmr2),
	.sna_unlock(plus_sna_unlock),
	.sna_ga_config(sna_ga_config),
	.sna_romsel(sna_rom_select),
	.sna_seq_state(plus_sna_seq_state)
);

plus_sna_parser plus_sna_parser
(
	.clk(clk_sys),
	.reset(sna_parser_reset),
	.sna_download(sna_download),
	.cpc_plus_chunk_start(sna_cpc_plus_start),
	.cpc_plus_byte_wr(sna_cpc_plus_wr),
	.cpc_plus_byte_data(sna_cpc_plus_data),
	.ioctl_wait(plus_sna_ioctl_wait),
	.busy(plus_sna_busy),
	.asic_sna_wr(plus_sna_wr),
	.asic_sna_addr(plus_sna_addr),
	.asic_sna_data(plus_sna_data),
	.asic_sna_active(plus_sna_active),
	.asic_sna_rmr2(plus_sna_rmr2),
	.asic_sna_unlock(plus_sna_unlock),
	.asic_sna_loop_cnt0(plus_sna_loop_cnt0),
	.asic_sna_loop_cnt1(plus_sna_loop_cnt1),
	.asic_sna_loop_cnt2(plus_sna_loop_cnt2),
	.asic_sna_loop_addr0(plus_sna_loop_addr0),
	.asic_sna_loop_addr1(plus_sna_loop_addr1),
	.asic_sna_loop_addr2(plus_sna_loop_addr2),
	.asic_sna_pause_cnt0(plus_sna_pause_cnt0),
	.asic_sna_pause_cnt1(plus_sna_pause_cnt1),
	.asic_sna_pause_cnt2(plus_sna_pause_cnt2),
	.asic_sna_pause_presc0(plus_sna_pause_presc0),
	.asic_sna_pause_presc1(plus_sna_pause_presc1),
	.asic_sna_pause_presc2(plus_sna_pause_presc2),
	.asic_sna_seq_state(plus_sna_seq_state)
);

wire [7:0] plus_vec_byte;
wire       plus_vec_valid;
wire plus_aspage_on;
wire [7:0] plus_asic_dout;
wire       plus_asic_rd;
// The whole &4000-&7FFF window while the page is enabled: reads are
// answered by the motherboard's asic_regs instance and BOTH directions
// must be suppressed against main memory (no read/write-through,
// reference §2). Suppression follows the cartridge-owned-cycle pattern.
wire plus_aspage_sel = plus_mode & plus_aspage_on &
                       (mem_rd | mem_wr) & (cpu_addr[15:14] == 2'b01);

// CPR loader stream (P0): the parser validates the RIFF envelope and cbNN
// chunks and streams page bytes into the cartridge memory service. Its
// ioctl_wait output joins the download throttle above so the HPS paces the
// byte stream while writes are outstanding.
wire        cart_load_begin;
wire        cart_load_commit;
wire        cart_load_abort;
wire        cart_load_valid;
wire [5:0]  cart_load_page;
wire [14:0] cart_load_offset;
wire [7:0]  cart_load_data;
wire        cart_load_ready;
wire        cart_load_error;

plus_cpr_parser cpr_parser
(
	.clk(clk_sys),
	.reset(reset_base),

	.cpr_download(cpr_download),
	.ioctl_wr(ioctl_wr),
	.ioctl_addr(ioctl_addr),
	.ioctl_dout(ioctl_dout),
	.ioctl_wait(cpr_ioctl_wait),

	.load_begin(cart_load_begin),
	.load_commit(cart_load_commit),
	.load_abort(cart_load_abort),
	.load_valid(cart_load_valid),
	.load_page(cart_load_page),
	.load_offset(cart_load_offset),
	.load_data(cart_load_data),
	.load_ready(cart_load_ready),
	.load_error(cart_load_error)
);

plus_cartridge_memory cartridge_memory
(
	.clk(clk_sys),
	.cold_reset(reset_base),
	// B6: Reset & Detach Dandanator (R[32]) resets the machine and detaches
	// the Dandanator only.  The Plus image is replaced atomically by CPR
	// loads and keeps its module-level detach API for standalone users
	// without exposing it here.
	.detach(1'b0),

	.load_begin(cart_load_begin),
	.load_commit(cart_load_commit),
	.load_abort(cart_load_abort),
	.load_valid(cart_load_valid),
	.load_page(cart_load_page),
	.load_offset(cart_load_offset),
	.load_data(cart_load_data),
	.load_ready(cart_load_ready),
	.load_error(cart_load_error),

	.cpu_valid(plus_cart_valid),
	.cpu_page({1'b0, plus_cart_page}),
	.cpu_offset(plus_cart_offset),
	.cpu_ready(plus_cart_ready),
	.cpu_granted(plus_cart_granted),
	.cpu_data(plus_cart_data),

	.image_valid(cart_image_valid),
	.busy(cart_service_busy),

	.mem_req(cart_mem_req),
	.mem_write(cart_mem_write),
	.mem_bank(cart_mem_bank),
	.mem_addr(cart_mem_addr),
	.mem_wdata(cart_mem_wdata),
	.mem_ack(cart_mem_ack),
	.mem_grant(cart_mem_grant),
	.mem_rdata(cart_mem_rdata)
);

Amstrad_motherboard motherboard
(
	.reset(reset),
	.clk(clk_sys),
	.ce_16(ce_16),

	// Reserved until the Plus subsystems are integrated. Keeping these
	// explicit prevents the classic CPC model path from changing at P-2.
	.plus_mode(plus_mode),
	.plus_unlocked(plus_asic_unlocked),
	.plus_ram_128k(plus_ram_128k),
	.plus_has_fdc(plus_has_fdc),
	.plus_has_tape(plus_has_tape),

	// Plus cartridge-window reads hold the Z80 in WAIT while the cartridge
	// memory service fetches from SDRAM. Constant 0 in classic mode.
	.plus_mem_wait(plus_cart_stall),
	.plus_aspage_on(plus_aspage_on),
	.plus_asic_dout(plus_asic_dout),
	.plus_asic_rd(plus_asic_rd),
	.plus_vec_byte(plus_vec_byte),
	.plus_vec_valid(plus_vec_valid),

	.right_shift_mod(st_right_shift_mod),
	.keypad_mod(st_keypad_mod),
	.ps2_key(ps2_key),
	.joy1_sel(joy1_sel),
	.Fn(Fn),

	.no_wait(status[6] & ~tape_motor),
	.ppi_jumpers({1'b1, ~status[44:42]}),
	.crtc_type(~status[2]),
	// 0 Full, 1 Raw pixels, 2 Raw CRT (3 reserved, normalised to Full).
	// Raw pixels keeps the whole Full acquisition tuple and changes only the
	// pixels; Raw CRT is the user-approved exception that sends raw geometry
	// to the single core video stream, so HDMI acquisition can become
	// unusable there.  The motherboard commits a changed selection at a safe
	// byte phase and reports it back as raw_crt.  See
	// docs/investigations/video-boundary/b6-video-boundary.md and
	// docs/backlog.md B1.
	.sync_filter(status[36:35]),
	.raw_crt(raw_crt),
	.pixel_vblank(pixel_vblank),

	.sna_load(sna_load),
	.sna_cpu_dir(sna_cpu_dir),
	.sna_crtc_addr(sna_crtc_addr),
	.sna_crtc_regs(sna_crtc_regs),
	.sna_ga_inksel(sna_ga_inksel),
	.sna_ga_palette(sna_ga_palette),
	.sna_ga_config(sna_ga_config),
	.sna_ram_config(sna_ram_config),
	.sna_rom_select(sna_rom_select),
	.sna_ppi_a(sna_ppi_a),
	.sna_ppi_b(sna_ppi_b),
	.sna_ppi_c(sna_ppi_c),
	.sna_ppi_control(sna_ppi_control),
	.sna_psg_addr(sna_psg_addr),
	.sna_psg_regs(sna_psg_regs),

	.sna_hold(sna_hold),
	.save_hold(save_hold),
	.sna_hsync(sna_hsync),
	.sna_dma_loop_cnt0(plus_sna_loop_cnt0),
	.sna_dma_loop_cnt1(plus_sna_loop_cnt1),
	.sna_dma_loop_cnt2(plus_sna_loop_cnt2),
	.sna_dma_loop_addr0(plus_sna_loop_addr0),
	.sna_dma_loop_addr1(plus_sna_loop_addr1),
	.sna_dma_loop_addr2(plus_sna_loop_addr2),
	.sna_dma_pause_cnt0(plus_sna_pause_cnt0),
	.sna_dma_pause_cnt1(plus_sna_pause_cnt1),
	.sna_dma_pause_cnt2(plus_sna_pause_cnt2),
	.sna_dma_pause_presc0(plus_sna_pause_presc0),
	.sna_dma_pause_presc1(plus_sna_pause_presc1),
	.sna_dma_pause_presc2(plus_sna_pause_presc2),

	.sna_crtc_hcc(sna_crtc_hcc),
	.sna_crtc_line(sna_crtc_line),
	.sna_crtc_raster(sna_crtc_raster),
	.sna_crtc_vta(sna_crtc_vta),
	.sna_crtc_hsw(sna_crtc_hsw),
	.sna_crtc_vsw(sna_crtc_vsw),
	.sna_crtc_vs(sna_crtc_vs),
	.sna_crtc_adj(sna_crtc_adj),
	.sna_ga_vsdelay(sna_ga_vsdelay),
	.sna_ga_intcnt(sna_ga_intcnt),
	.sna_int_pending(sna_int_pending),
	.sna_plus_chunk(plus_sna_active),

	.plus_sna_wr(plus_sna_wr),
	.plus_sna_addr(plus_sna_addr),
	.plus_sna_data(plus_sna_data),
	.plus_asic_reset(plus_asic_reset),

	.joy1((status[21] ? amouse_dout : 7'd0) | (status[18] ? joy2 : joy1)),
	.joy2(status[18] ? joy1 : joy2),

	.tape_in(tape_play),
	.tape_out(tape_rec),
	.tape_motor(tape_motor),

	.audio_l(audio_l),
	.audio_r(audio_r),

	.mode(mode),

	.hblank(hbl),
	.vblank(vbl),
	.hsync(hs),
	.vsync(vs),
	.red(r4),
	.green(g4),
	.blue(b4),
	.field(VGA_F1),

	.vram_din(vram_dout),
	.vram_addr(vram_addr),

	.rom_map(rom_map),
	.ram64k(plus_mode ? !plus_ram_128k : (model != 2'd0)),
	.mem_rd(mem_rd),
	.mem_wr(mem_wr),
	.mem_addr(ram_a),
	.romen(romen),

	.ssm_m1_fetch(ssm_m1_fetch),
	.ssm_bus_data(ssm_bus_data),

	.phi_n(phi_n),
	.phi_en_n(phi_en_n),
	.phi_en_p(phi_en_p),
	.cpu_addr(cpu_addr),
	.cpu_dout(cpu_dout),
	.cpu_din(cpu_din),
	.iorq(iorq),
	.asic_iorq(asic_iorq),
	.mreq(mreq),
	.rd(rd),
	.wr(wr),
	.m1(m1),
	.io_bus_byte(plus_io_bus_byte),
	.ga_ready(ready),
	.nmi(NMI),
	.irq(IRQ),
	.cursor(cursor),

	.key_nmi(key_nmi),
	.key_reset(key_reset),

	.snap_ga_inksel(snap_ga_inksel),
	.snap_ga_border(snap_ga_border),
	.snap_ga_inkr(snap_ga_inkr),
	.snap_ga_hromen(snap_ga_hromen),
	.snap_ga_lromen(snap_ga_lromen),
	.snap_ga_mode(snap_ga_mode),
	.snap_ga_intcnt(snap_ga_intcnt),
	.snap_ga_hcnt(snap_ga_hcnt),
	.snap_ga_int_n(snap_ga_int_n),

	.snap_crtc_addr(snap_crtc_addr),
	.snap_crtc_regs(snap_crtc_regs),
	.snap_crtc_hcc(snap_crtc_hcc),
	.snap_crtc_row(snap_crtc_row),
	.snap_crtc_line(snap_crtc_line),
	.snap_crtc_c5(snap_crtc_c5),
	.snap_crtc_in_adj(snap_crtc_in_adj),
	.snap_crtc_hsc(snap_crtc_hsc),
	.snap_crtc_hsync(snap_crtc_hsync),
	.snap_crtc_vsync_r(snap_crtc_vsync_r),
	.snap_crtc_vsw_elapsed(snap_crtc_vsw_elapsed),

	.snap_mmu_rammap(snap_mmu_rammap),
	.snap_mmu_rampage(snap_mmu_rampage),
	.snap_mmu_rom_select_shadow(snap_mmu_rom_select_shadow),

	.snap_ppi_porta_in(snap_ppi_porta_in),
	.snap_ppi_portb_in(snap_ppi_portb_in),
	.snap_ppi_opc_r(snap_ppi_opc_r),
	.snap_ppi_mode(snap_ppi_mode),

	.snap_psg_addr(snap_psg_addr),
	.snap_psg_regs(snap_psg_regs),

	.snap_printer_data(snap_printer_data),

	.cpu_reg(cpu_reg),
	.cpu_insn_start(cpu_insn_start),
	.cpu_halt_n(cpu_halt_n)
);

/////////////////////////////////Dandanator/////////////////////
wire [4:0] dan_bank;
wire dan_romdis;
wire dan_ramdis;
//wire [7:0] dan_databus;
wire dan_eeprom_nce;
wire dan_eeprom_nwr;
//wire dan_nnmi;
wire dan_eeprom_loaded;
wire dan_ena;

plus_legacy_cart_gate legacy_cart_gate
(
	.clk(clk_sys),
	.plus_mode(plus_mode),
	.dandanator_download(dan_download),
	.dandanator_detach(status[32]),
	.dandanator_nce(dan_eeprom_nce),
	.dandanator_loaded(dan_eeprom_loaded),
	.dandanator_active(dan_ena)
);

CPC_Dandanator dandanator(
    .clk(clk_sys),
    .nRst(~reset),
    .ceP(phi_en_p),
    .ceN(phi_en_n),
    
    .Button(1'b1),
    .Button2(1'b1),
    
    .nRomEn(~romen),
    .nM1(~m1),
    .nMreq(~mreq),
    .nWr(~wr),
    .nRd(~rd),
    .Rdy(ready),
    .A15(cpu_addr[15]),
    .A14(cpu_addr[14]),
    .A13(cpu_addr[13]),
    .DataBusIn(cpu_din),
    //.DataBusOut(dan_databus),
    .DataBusOut(),
    .EXP(),
    
    //.nNMI(dan_nnmi),
	.nNMI(),
    .Romdis(dan_romdis),
    .Ramdis(dan_ramdis),
    .nEp_Ce(dan_eeprom_nce),
    .nEp_Wr(dan_eeprom_nwr),
    .Ep_A18_14(dan_bank),
    
    .CHG_Txd(1'b1),
    .CHG_Rxd(1'b1)
);
//////////////////////////////////////////////////////////////////////

assign CLK_VIDEO = clk_sys;

wire [3:0] b4, g4, r4;
wire hs, vs, hbl, vbl;
// One applied motherboard mode controls the complete production output chain.
wire raw_crt, pixel_vblank, en270p;
// B4 phase 2 observation tap. Outputs only; with no recorder compiled in they
// are unconnected and synthesis removes them, so the default build is exactly
// the phase 1 build.
wire obs_ce_pix, obs_hs, obs_vs, obs_hbl, obs_vbl, obs_field, obs_native_cadence;
wire [7:0] obs_r, obs_g, obs_b;
wire [7:0] obs_applied_config;
amstrad_video_output video_output (
    .CLK_VIDEO(CLK_VIDEO), .ce_16(ce_16),
    .raw_crt(raw_crt), .pixel_vblank(pixel_vblank), .plus_mode(plus_mode),
    .mode(mode), .scale(status[10:9]), .ar(status[26:25]),
    .integer_scale(status[29:28]), .mix(status[13:11]),
    .r4(r4), .g4(g4), .b4(b4), .hs(hs), .vs(vs), .hbl(hbl), .vbl(vbl),
    .pixel_rate_select(status[30]), .forced_scandoubler(forced_scandoubler),
    .field_in(VGA_F1), .vcrop_en(status[27]),
    .HDMI_WIDTH(HDMI_WIDTH), .HDMI_HEIGHT(HDMI_HEIGHT),
    .HDMI_FREEZE(HDMI_FREEZE), .progress_pix(progress_pix), .gamma_bus(gamma_bus),
    .CE_PIXEL(CE_PIXEL), .VGA_R(VGA_R), .VGA_G(VGA_G), .VGA_B(VGA_B),
    .VGA_HS(VGA_HS), .VGA_VS(VGA_VS), .VGA_DE(VGA_DE), .VGA_SL(VGA_SL),
    .VIDEO_ARX(VIDEO_ARX), .VIDEO_ARY(VIDEO_ARY), .en270p(en270p),
    .obs_ce_pix(obs_ce_pix), .obs_r(obs_r), .obs_g(obs_g), .obs_b(obs_b),
    .obs_hs(obs_hs), .obs_vs(obs_vs), .obs_hbl(obs_hbl), .obs_vbl(obs_vbl),
    .obs_field(obs_field), .obs_native_cadence(obs_native_cadence),
    .obs_applied_config(obs_applied_config)
);

//////////////////////////////////////////////////////////////////////
// SSM marker detector (backlog B4, phase 1 of the CSL/SSM plan).
//
// Passive: it watches the opcode-fetch stream and the output-side sync, and
// drives nothing the machine can see. Off by default, in which case it is
// held in reset and issues no DDR3 traffic. The raster stamp uses the native
// 16 MHz output timebase, the same one the capture comparison will use.

wire ssm_enable = status[37];

wire [28:0] ssm_ddr_addr;
wire [63:0] ssm_ddr_din;
wire  [7:0] ssm_ddr_be, ssm_ddr_burstcnt;
wire        ssm_ddr_we, ssm_ddr_busy;

ssm_marker ssm
(
	.clk(clk_sys),
	.reset(reset),
	.enable(ssm_enable),

	.m1_fetch(ssm_m1_fetch),
	.bus_data(ssm_bus_data),

	.ce_pix(ce_16),
	.hsync(hs),
	.vsync(vs),
	.field(VGA_F1),

	.last_code(),
	.event_count(),
	.dropped_count(),
	.event_stb(),

	.ddram_addr(ssm_ddr_addr),
	.ddram_din(ssm_ddr_din),
	.ddram_be(ssm_ddr_be),
	.ddram_burstcnt(ssm_ddr_burstcnt),
	.ddram_we(ssm_ddr_we),
	.ddram_busy(ssm_ddr_busy)
);

assign DDRAM_CLK = clk_sys;

//////////////////////////////////////////////////////////////////////
// SNA save (backlog B18, docs/b18-sna-save.md).
//
// OSD "Save snapshot" arms a request. The Z80 freezes at the next
// instruction boundary, the same point a hardware WAIT would hold it, while
// the Gate Array, CRTC and video keep running. The header is latched on that
// clock, RAM is read through the SDRAM cartridge port into the DDR3 slot at
// 0x3E000000, and the CPU is released. scripts/hardware-loop/sna_pull.py
// copies the file out over SSH; there is no route to SD from here.

reg old_save_osd = 1'b0;
always @(posedge clk_sys) old_save_osd <= status[38];
wire save_req = status[38] & ~old_save_osd;

// Classic machine only, outside reset, downloads and snapshot apply. Plus
// mode, the Dandanator and the Multiface II hold memory or mapping state the
// file cannot carry, and an outstanding cartridge request would leave the
// SDRAM port busy. Losing admission while armed cancels the request.
wire save_admit = !reset && !ioctl_download && !sna_hold && !sna_load && !plus_mode &&
                  !dan_ena && !mf2_en && !cart_mem_req;

// Once the CPU is held, only a reset or a snapshot load stops the save: a
// load rewrites the RAM being dumped. Every other download either resets the
// core or leaves RAM alone.
wire save_abort = reset | sna_download;

wire [8*135-1:0] save_hw_hdr;
wire [2047:0]    save_header;
wire             save_captured, save_refused, save_cancelled, save_done;

sna_hw_header save_hw_header
(
	.ga_inksel(snap_ga_inksel),
	.ga_border(snap_ga_border),
	.ga_inkr(snap_ga_inkr),
	.ga_hromen(snap_ga_hromen),
	.ga_lromen(snap_ga_lromen),
	.ga_mode(snap_ga_mode),
	.ga_intcnt(snap_ga_intcnt),
	.ga_hcnt(snap_ga_hcnt),
	.ga_int_n(snap_ga_int_n),

	.mmu_rammap(snap_mmu_rammap),
	.mmu_rom_select_shadow(snap_mmu_rom_select_shadow),

	.crtc_addr(snap_crtc_addr),
	.crtc_regs(snap_crtc_regs),
	.crtc_hcc(snap_crtc_hcc),
	.crtc_row(snap_crtc_row),
	.crtc_line(snap_crtc_line),
	.crtc_c5(snap_crtc_c5),
	.crtc_in_adj(snap_crtc_in_adj),
	.crtc_hsc(snap_crtc_hsc),
	.crtc_hsync(snap_crtc_hsync),
	.crtc_vsync_r(snap_crtc_vsync_r),
	.crtc_vsw_elapsed(snap_crtc_vsw_elapsed),

	.ppi_porta_in(snap_ppi_porta_in),
	.ppi_portb_in(snap_ppi_portb_in),
	.ppi_opc_r(snap_ppi_opc_r),
	.ppi_mode(snap_ppi_mode),

	.psg_addr(snap_psg_addr),
	.psg_regs(snap_psg_regs),

	.printer_data(snap_printer_data),
	.model(model),
	.crtc_type(~status[2]),        // the value fed to the motherboard's CRTC_TYPE
	.fdc_motor(motor),
	.fdc_pcn_a(snap_pcn_a),
	.fdc_pcn_b(snap_pcn_b),

	.hdr(save_hw_hdr)
);

sna_save_capture save_capture
(
	.clk(clk_sys),
	.reset(save_abort),
	.save_req(save_req),
	.admit(save_admit),
	.rampage_ok(snap_mmu_rampage == 5'd3),
	.release_req(save_done),
	.insn_start(cpu_insn_start),
	.halt_n(cpu_halt_n),
	.cpu_reg(cpu_reg),
	.hw_hdr(save_hw_hdr),
	.hold(save_hold),
	.captured(save_captured),
	.refused(save_refused),
	.cancelled(save_cancelled),
	.busy(),
	.header(save_header)
);

wire        save_ddr_request, save_ddr_grant, save_ddr_we, save_ddr_busy;
wire [28:0] save_ddr_addr;
wire [63:0] save_ddr_din;
wire  [7:0] save_ddr_be, save_ddr_burstcnt;

// Admission excludes Plus mode and snapshot apply, so mem_bank is the
// running classic model's bank; the stream latches it on start.
sna_save_stream save_stream
(
	.clk(clk_sys),
	.reset(save_abort),
	.start(save_captured),
	.ram128(model == 2'd0),
	.bank(mem_bank),
	.header(save_header),
	.clkref(ce_ref),

	.cart_req(save_cart_req),
	.cart_bank(save_cart_bank),
	.cart_addr(save_cart_addr),
	.cart_dout(cart_mem_rdata),
	.cart_ack(save_cart_ack),

	.ddr_grant(save_ddr_grant),
	.ddr_request(save_ddr_request),
	.ddram_addr(save_ddr_addr),
	.ddram_din(save_ddr_din),
	.ddram_be(save_ddr_be),
	.ddram_burstcnt(save_ddr_burstcnt),
	.ddram_we(save_ddr_we),
	.ddram_busy(save_ddr_busy),

	.done(save_done),
	.active()
);

sna_cart_mux save_cart_mux
(
	.clk(clk_sys),
	.clkref(ce_ref),

	.a_req(cart_mem_req),
	.a_wr(cart_mem_write),
	.a_bank(cart_mem_bank),
	.a_addr(cart_mem_addr),
	.a_din(cart_mem_wdata),
	.a_ack(cart_mem_ack),
	.a_grant(cart_mem_grant),

	.b_req(save_cart_req),
	.b_bank(save_cart_bank),
	.b_addr(save_cart_addr),
	.b_ack(save_cart_ack),

	.cart_req(sdram_cart_req),
	.cart_wr(sdram_cart_wr),
	.cart_bank(sdram_cart_bank),
	.cart_addr(sdram_cart_addr),
	.cart_din(sdram_cart_din),
	.cart_ack(sdram_cart_ack),
	.cart_grant(sdram_cart_grant)
);

// The SSM marker keeps the DDR3 port by default; the save takes it only
// between SSM writes, and the marker sees busy while the save owns it.
sna_ddr_mux save_ddr_mux
(
	.clk(clk_sys),
	.reset(save_abort),

	.a_addr(ssm_ddr_addr),
	.a_din(ssm_ddr_din),
	.a_be(ssm_ddr_be),
	.a_burstcnt(ssm_ddr_burstcnt),
	.a_we(ssm_ddr_we),
	.a_busy(ssm_ddr_busy),

	.b_request(save_ddr_request),
	.b_grant(save_ddr_grant),
	.b_addr(save_ddr_addr),
	.b_din(save_ddr_din),
	.b_be(save_ddr_be),
	.b_burstcnt(save_ddr_burstcnt),
	.b_we(save_ddr_we),
	.b_busy(save_ddr_busy),

	.ddram_addr(DDRAM_ADDR),
	.ddram_din(DDRAM_DIN),
	.ddram_be(DDRAM_BE),
	.ddram_burstcnt(DDRAM_BURSTCNT),
	.ddram_we(DDRAM_WE),
	.ddram_busy(DDRAM_BUSY)
);

// OSD feedback, 1-based into the CONF_STR "I," entry: saved, refused,
// cancelled. hps_io latches `info` on the rising edge of `info_req`.
always @(posedge clk_sys) begin
	save_info_req <= 1'b0;
	if (save_done) begin
		save_info     <= 8'd1;
		save_info_req <= 1'b1;
	end
	else if (save_refused) begin
		save_info     <= 8'd2;
		save_info_req <= 1'b1;
	end
	else if (save_cancelled) begin
		save_info     <= 8'd3;
		save_info_req <= 1'b1;
	end
end

//////////////////////////////////////////////////////////////////////

wire [7:0] audio_l, audio_r;

wire [8:0] audio_sys_l = audio_l + {tape_rec, 1'b0, tape_play & status[20], 3'd0};
wire [8:0] audio_sys_r = audio_r + {tape_rec, 1'b0, tape_play & status[20], 3'd0};

assign AUDIO_S   = 0;
assign AUDIO_MIX = status[8:7];
assign AUDIO_L   = {audio_sys_l + (playcity_ena ? playcity_audio_l : audio_sys_l), 7'd0};
assign AUDIO_R   = {audio_sys_r + (playcity_ena ? playcity_audio_r : audio_sys_r), 7'd0};

//////////////////////////////////////////////////////////////////////

wire tape_play = tape_ready ? tape_read : tape_adc;

wire tape_adc, tape_adc_act;
ltc2308_tape ltc2308_tape
(
	.clk(CLK_50M),
	.ADC_BUS(ADC_BUS),
	.dout(tape_adc),
	.active(tape_adc_act)
);

endmodule
