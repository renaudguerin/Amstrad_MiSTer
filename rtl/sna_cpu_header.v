//============================================================================
//  SNA header capture for the Z80 / PPI / PSG / RAM-config / memory-size
//  fields (B18 slice 2).
//
//  Amstrad.sv used to decode these header offsets inline. They moved here so
//  the B18 save/reload round-trip fixture drives the SAME decoder from a real
//  byte stream instead of poking pre-decoded values at the owners: an offset
//  typo in Amstrad.sv is neither linted nor simulated, so it would only show
//  up on hardware. The memory-chunk and CPC+ chunk parser stays in Amstrad.sv;
//  it is outside this slice.
//
//  Reference: docs/references/Snapshot (.SNA) file format.md
//    v1 header  11        Z80 F            -> cpu_dir[15:8]
//               12        Z80 A            -> cpu_dir[7:0]
//               13        Z80 C            -> cpu_dir[87:80]
//               14        Z80 B            -> cpu_dir[95:88]
//               15        Z80 E            -> cpu_dir[103:96]
//               16        Z80 D            -> cpu_dir[111:104]
//               17        Z80 L            -> cpu_dir[119:112]
//               18        Z80 H            -> cpu_dir[127:120]
//               19        Z80 R            -> cpu_dir[47:40]
//               1a        Z80 I            -> cpu_dir[39:32]
//               1b        IFF0 (note 2)    -> cpu_dir[210] (bit 0 only)
//               1c        IFF1 (note 2)    -> cpu_dir[211] (bit 0 only)
//               1d-1e     IX low/high      -> cpu_dir[135:128]/[143:136]
//               1f-20     IY low/high      -> cpu_dir[199:192]/[207:200]
//               21-22     SP low/high      -> cpu_dir[55:48]/[63:56]
//               23-24     PC low/high      -> cpu_dir[71:64]/[79:72]
//               25        IM (note 3)      -> cpu_dir[209:208] (bits 1:0 only)
//               26        F' (note 4)      -> cpu_dir[31:24]
//               27        A' (note 4)      -> cpu_dir[23:16]
//               28        C'               -> cpu_dir[151:144]
//               29        B'               -> cpu_dir[159:152]
//               2a        E'               -> cpu_dir[167:160]
//               2b        D'               -> cpu_dir[175:168]
//               2c        L'               -> cpu_dir[183:176]
//               2d        H'               -> cpu_dir[191:184]
//               41        RAM config       -> ram_config (note 13)
//               56        PPI port A       -> ppi_a (note 6: inputs)
//               57        PPI port B       -> ppi_b (note 7: inputs)
//               58        PPI port C       -> ppi_c (note 8: outputs)
//               59        PPI control      -> ppi_control (note 9)
//               5a        PSG select       -> psg_addr (bits 3:0; note 17)
//               5b-6a     PSG regs 0-15    -> psg_regs[k*8 +: 8], k = addr-5b
//               6b-6c     memory size KB   -> mem_size (little-endian; note 18)
//    v2 header  6d        CPC type         -> model (see decision logic below)
//
//  The cpu_dir bit layout is the T80pa DIR vector documented in
//  rtl/T80/T80pa.vhd (REG/DIR comment: IFF2, IFF1, IM, IY, HL', DE', BC', IX,
//  HL, DE, BC, PC, SP, R, I, F', A', F, A, from high bits to low); T80.vhd
//  consumes DIR(211) as IFF2, DIR(210) as IFF1 and DIR(209:208) as the
//  interrupt mode. The slice positions below are copied verbatim from the
//  inline code this module replaces.
//
//  Model decision (copied verbatim from the inline code):
//    * at 6c (size high byte): size > 64K forces model 0 (6128 map); a 64K
//      file whose PC is 0x0038 selects model 2 (464). The 0x0038 test is the
//      old v1 heuristic: a v1 file carries no CPC-type byte, and a 64K v1
//      snapshot of a machine sitting in the firmware ROM looks like PC=0x38.
//    * at 6d (CPC type): size > 64K forces model 0; otherwise type 0 -> 2
//      (464), 1 -> 1 (664), 2/4/5/6 -> 0 (6128 map, incl. Plus/GX4000
//      snapshots); any other type value leaves the model unchanged.
//  Amstrad.sv ORs a registered "chunk forces 128K" flag over `model`: a MEM1
//  or CPC+ chunk header completing later in the file forces the 128K map.
//  That flag lives in Amstrad.sv next to the chunk parser so `model' keeps a
//  single procedural driver per signal; see the instantiation comment there.
//
//  Reset behaviour (copied verbatim from the inline code, quirks included):
//  cpu_dir, psg_regs, mem_size (=64), model (=menu_model) and ppi_control
//  (=0x9b) clear on each new download. ram_config, ppi_a/b/c and psg_addr are
//  NOT cleared: offset 0x41 is always present in a real file, and the PPI/PSG
//  owners apply the freshly decoded bytes at sna_load either way.
//============================================================================

module sna_cpu_header
(
	input          clk,
	input          sna_download,

	// Header byte stream: `wr` strobes one byte of the first 256 bytes of
	// the snapshot file, `addr` is its file offset. Amstrad.sv gates `wr`
	// to (sna_download && ioctl_wr && ioctl_addr < 0x100) and passes the
	// low address byte, exactly the old inline condition.
	input          wr,
	input    [7:0] addr,
	input    [7:0] data,

	// OSD model menu selection, sampled at download start.
	input    [1:0] menu_model,

	output [211:0] cpu_dir,
	output   [7:0] ram_config,
	output   [7:0] ppi_a,
	output   [7:0] ppi_b,
	output   [7:0] ppi_c,
	output   [7:0] ppi_control,
	output   [3:0] psg_addr,
	output [127:0] psg_regs,
	output  [15:0] mem_size,
	output   [1:0] model
);

	reg [211:0] cpu_dir_r;
	reg   [7:0] ram_config_r;
	reg   [7:0] ppi_a_r;
	reg   [7:0] ppi_b_r;
	reg   [7:0] ppi_c_r;
	reg   [7:0] ppi_control_r;
	reg   [3:0] psg_addr_r;
	reg [127:0] psg_regs_r;
	reg  [15:0] mem_size_r;
	reg   [1:0] model_r;

	reg         sna_download_d;

	assign cpu_dir     = cpu_dir_r;
	assign ram_config  = ram_config_r;
	assign ppi_a       = ppi_a_r;
	assign ppi_b       = ppi_b_r;
	assign ppi_c       = ppi_c_r;
	assign ppi_control = ppi_control_r;
	assign psg_addr    = psg_addr_r;
	assign psg_regs    = psg_regs_r;
	assign mem_size    = mem_size_r;
	assign model       = model_r;

	always @(posedge clk) begin
		sna_download_d <= sna_download;

		// Download-start edge. The inline code detected the start as
		// `~old_download & ioctl_download & sna_download` where old_download
		// is a one-cycle delayed ioctl_download. This module sees only
		// sna_download (= ioctl_download && index==6), so it uses the rising
		// edge of sna_download instead. The two coincide: a SNA download
		// always starts from ioctl_download low (MiSTer drops the strobe
		// between downloads), so on the first SNA cycle both delayed regs
		// are 0 and both edges fire on that same cycle.
		//
		// If a header write ever landed on that same edge cycle, the inline
		// code evaluated the reset statement AFTER the decode statement, so
		// the reset won; the if/else below gives the reset the same
		// priority. In practice the first data byte follows the download
		// strobe, so the collision does not occur.
		if (sna_download && !sna_download_d) begin
			cpu_dir_r     <= 212'd0;
			psg_regs_r    <= 128'd0;
			mem_size_r    <= 16'd64;
			model_r       <= menu_model;
			ppi_control_r <= 8'h9b;
		end
		else if (wr) begin
			case (addr)
				8'h11: cpu_dir_r[15:8]    <= data;          // F
				8'h12: cpu_dir_r[7:0]     <= data;          // A
				8'h13: cpu_dir_r[87:80]   <= data;          // C
				8'h14: cpu_dir_r[95:88]   <= data;          // B
				8'h15: cpu_dir_r[103:96]  <= data;          // E
				8'h16: cpu_dir_r[111:104] <= data;          // D
				8'h17: cpu_dir_r[119:112] <= data;          // L
				8'h18: cpu_dir_r[127:120] <= data;          // H
				8'h19: cpu_dir_r[47:40]   <= data;          // R
				8'h1a: cpu_dir_r[39:32]   <= data;          // I
				8'h1b: cpu_dir_r[210]     <= data[0];       // IFF1
				8'h1c: cpu_dir_r[211]     <= data[0];       // IFF2
				8'h1d: cpu_dir_r[135:128] <= data;          // IX low
				8'h1e: cpu_dir_r[143:136] <= data;          // IX high
				8'h1f: cpu_dir_r[199:192] <= data;          // IY low
				8'h20: cpu_dir_r[207:200] <= data;          // IY high
				8'h21: cpu_dir_r[55:48]   <= data;          // SP low
				8'h22: cpu_dir_r[63:56]   <= data;          // SP high
				8'h23: cpu_dir_r[71:64]   <= data;          // PC low
				8'h24: cpu_dir_r[79:72]   <= data;          // PC high
				8'h25: cpu_dir_r[209:208] <= data[1:0];     // IM
				8'h26: cpu_dir_r[31:24]   <= data;          // F'
				8'h27: cpu_dir_r[23:16]   <= data;          // A'
				8'h28: cpu_dir_r[151:144] <= data;          // C'
				8'h29: cpu_dir_r[159:152] <= data;          // B'
				8'h2a: cpu_dir_r[167:160] <= data;          // E'
				8'h2b: cpu_dir_r[175:168] <= data;          // D'
				8'h2c: cpu_dir_r[183:176] <= data;          // L'
				8'h2d: cpu_dir_r[191:184] <= data;          // H'
				8'h41: ram_config_r       <= data;
				8'h56: ppi_a_r            <= data;
				8'h57: ppi_b_r            <= data;
				8'h58: ppi_c_r            <= data;
				8'h59: ppi_control_r      <= data;
				8'h5a: psg_addr_r         <= data[3:0];
				8'h6b: mem_size_r[7:0]    <= data;
				8'h6c: begin
					mem_size_r[15:8] <= data;
					if ({data, mem_size_r[7:0]} > 16'd64) model_r <= 2'd0;
					else if (cpu_dir_r[79:64] == 16'h0038) model_r <= 2'd2;
				end
				8'h6d: begin
					if (mem_size_r > 16'd64) model_r <= 2'd0;
					else begin
						case (data)
							8'd0: model_r <= 2'd2; // CPC464
							8'd1: model_r <= 2'd1; // CPC664
							8'd2, 8'd4, 8'd5, 8'd6: model_r <= 2'd0; // 6128/464+/6128+/GX snapshots need the 128K map.
							default: ; // any other type value leaves the model unchanged
						endcase
					end
				end
				default: ;
			endcase

			if (addr >= 8'h5b && addr <= 8'h6a)
				psg_regs_r[((addr - 8'h5b) * 8) +: 8] <= data;
		end
	end

	initial begin
		cpu_dir_r      = 212'd0;
		ram_config_r   = 8'd0;
		ppi_a_r        = 8'd0;
		ppi_b_r        = 8'd0;
		ppi_c_r        = 8'd0;
		ppi_control_r  = 8'h9b;
		psg_addr_r     = 4'd0;
		psg_regs_r     = 128'd0;
		mem_size_r     = 16'd64;
		model_r        = 2'd0;
		sna_download_d = 1'b0;
	end

endmodule
