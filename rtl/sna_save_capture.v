//============================================================================
//  SNA snapshot freeze controller and header latch (B18 slice 4a).
//
//  Reference: docs/b18-sna-save.md, "Freeze point", "Header fields and their
//  sources (classic)".
//============================================================================

module sna_save_capture
(
	input          clk,
	input          reset,          // core reset: cancels everything, hold released
	input          save_req,       // one-clock request pulse (OSD action in 4b)
	input          admit,          // 1 when outside reset, download and snapshot apply
	input          rampage_ok,     // MMU RAMpage == 3 (no expansion page mapped)
	input          release_req,    // one-clock pulse from the stream (4b) or the test
	input          insn_start,     // T80pa INSN_START
	input          halt_n,         // T80pa halt_n
	input  [211:0] cpu_reg,        // T80pa REG
	input  [8*135-1:0] hw_hdr,     // sna_hw_header output, offsets 0x2E..0xB4
	output reg     hold,           // gates both T80pa enables (motherboard save_hold)
	output reg     captured,       // one-clock pulse when header is latched
	output reg     refused,        // one-clock pulse: request refused (rampage_ok low or !admit)
	output reg     cancelled,      // one-clock pulse: pending request cancelled
	output         busy,           // armed or holding
	output reg [2047:0] header     // latched 256-byte SNA v3 header, byte o at [o*8 +: 8]
);

	localparam [1:0] S_IDLE  = 2'd0;
	localparam [1:0] S_ARMED = 2'd1;
	localparam [1:0] S_HELD  = 2'd2;

	reg [1:0] state;
	reg       insn_start_d;

	wire insn_start_rising = insn_start && !insn_start_d;

	assign busy = (state != S_IDLE);

	wire [15:0] pc_raw = cpu_reg[79:64];
	wire [15:0] pc_adj = halt_n ? pc_raw : (pc_raw - 16'd1);
	wire  [7:0] r_raw  = cpu_reg[47:40];
	wire  [7:0] r_adj  = halt_n ? r_raw : {r_raw[7], (r_raw[6:0] - 7'd1)};

	reg [2047:0] assembled_header;
	integer i;

	always @(*) begin
		assembled_header = {2048{1'b0}};

		// 0x00-0x07: ASCII "MV - SNA"
		assembled_header[8'h00*8 +: 8] = 8'h4D; // 'M'
		assembled_header[8'h01*8 +: 8] = 8'h56; // 'V'
		assembled_header[8'h02*8 +: 8] = 8'h20; // ' '
		assembled_header[8'h03*8 +: 8] = 8'h2D; // '-'
		assembled_header[8'h04*8 +: 8] = 8'h20; // ' '
		assembled_header[8'h05*8 +: 8] = 8'h53; // 'S'
		assembled_header[8'h06*8 +: 8] = 8'h4E; // 'N'
		assembled_header[8'h07*8 +: 8] = 8'h41; // 'A'

		// 0x10: snapshot version 3
		assembled_header[8'h10*8 +: 8] = 8'h03;

		// 0x11-0x2D: Z80 CPU registers (inverse of rtl/sna_cpu_header.v decode)
		assembled_header[8'h11*8 +: 8] = cpu_reg[15:8];    // F
		assembled_header[8'h12*8 +: 8] = cpu_reg[7:0];     // A
		assembled_header[8'h13*8 +: 8] = cpu_reg[87:80];   // C
		assembled_header[8'h14*8 +: 8] = cpu_reg[95:88];   // B
		assembled_header[8'h15*8 +: 8] = cpu_reg[103:96];  // E
		assembled_header[8'h16*8 +: 8] = cpu_reg[111:104]; // D
		assembled_header[8'h17*8 +: 8] = cpu_reg[119:112]; // L
		assembled_header[8'h18*8 +: 8] = cpu_reg[127:120]; // H
		assembled_header[8'h19*8 +: 8] = r_adj;            // R (HALT adjusted)
		assembled_header[8'h1A*8 +: 8] = cpu_reg[39:32];   // I
		assembled_header[8'h1B*8 +: 8] = {7'b0000000, cpu_reg[210]};     // IFF1
		assembled_header[8'h1C*8 +: 8] = {7'b0000000, cpu_reg[211]};     // IFF2
		assembled_header[8'h1D*8 +: 8] = cpu_reg[135:128]; // IX low
		assembled_header[8'h1E*8 +: 8] = cpu_reg[143:136]; // IX high
		assembled_header[8'h1F*8 +: 8] = cpu_reg[199:192]; // IY low
		assembled_header[8'h20*8 +: 8] = cpu_reg[207:200]; // IY high
		assembled_header[8'h21*8 +: 8] = cpu_reg[55:48];   // SP low
		assembled_header[8'h22*8 +: 8] = cpu_reg[63:56];   // SP high
		assembled_header[8'h23*8 +: 8] = pc_adj[7:0];      // PC low (HALT adjusted)
		assembled_header[8'h24*8 +: 8] = pc_adj[15:8];     // PC high (HALT adjusted)
		assembled_header[8'h25*8 +: 8] = {6'b000000, cpu_reg[209:208]}; // IM
		assembled_header[8'h26*8 +: 8] = cpu_reg[31:24];   // F'
		assembled_header[8'h27*8 +: 8] = cpu_reg[23:16];   // A'
		assembled_header[8'h28*8 +: 8] = cpu_reg[151:144]; // C'
		assembled_header[8'h29*8 +: 8] = cpu_reg[159:152]; // B'
		assembled_header[8'h2A*8 +: 8] = cpu_reg[167:160]; // E'
		assembled_header[8'h2B*8 +: 8] = cpu_reg[175:168]; // D'
		assembled_header[8'h2C*8 +: 8] = cpu_reg[183:176]; // L'
		assembled_header[8'h2D*8 +: 8] = cpu_reg[191:184]; // H'

		// 0x2E-0xB4: classic hardware state from sna_hw_header (135 bytes)
		for (i = 0; i < 135; i = i + 1) begin
			assembled_header[(32'h2E + i)*8 +: 8] = hw_hdr[i*8 +: 8];
		end
	end

	always @(posedge clk) begin
		insn_start_d <= insn_start;

		if (reset) begin
			state        <= S_IDLE;
			hold         <= 1'b0;
			captured     <= 1'b0;
			refused      <= 1'b0;
			cancelled    <= 1'b0;
			header       <= {2048{1'b0}};
			insn_start_d <= 1'b0;
		end else begin
			captured  <= 1'b0;
			refused   <= 1'b0;
			cancelled <= 1'b0;

			case (state)
				S_IDLE: begin
					if (save_req) begin
						if (!admit || !rampage_ok) begin
							refused <= 1'b1;
						end else begin
							state   <= S_ARMED;
						end
					end
				end

				S_ARMED: begin
					if (!admit || save_req) begin
						cancelled <= 1'b1;
						state     <= S_IDLE;
					end else if (insn_start_rising && !rampage_ok) begin
						// The program paged in expansion RAM between request and
						// boundary; the 128K dump would no longer match the header.
						refused <= 1'b1;
						state   <= S_IDLE;
					end else if (insn_start_rising) begin
						hold     <= 1'b1;
						captured <= 1'b1;
						header   <= assembled_header;
						state    <= S_HELD;
					end
				end

				S_HELD: begin
					if (release_req) begin
						hold  <= 1'b0;
						state <= S_IDLE;
					end
				end

				default: begin
					state <= S_IDLE;
					hold  <= 1'b0;
				end
			endcase
		end
	end

	initial begin
		state        = S_IDLE;
		hold         = 1'b0;
		captured     = 1'b0;
		refused      = 1'b0;
		cancelled    = 1'b0;
		header       = {2048{1'b0}};
		insn_start_d = 1'b0;
	end

endmodule
