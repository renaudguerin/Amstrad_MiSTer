// P1 pixel-phase validation (docs/plus/architecture.md §4 P1 exit item,
// closing the t05h unverified-assumption note against the production
// cadence).
//
// Drives sim/plus/p1_video_test_top.v — asic_ga_timing + asic_video plus a
// verbatim copy of the motherboard VRAM fetch and word-assembly blocks —
// with a one-line frame (R4=0, R5=0, R9=0), R1=R6=40 displayed characters of
// a 64-character line in screen mode 2. The fake VRAM returns words that
// encode their own even address, so the displayed PEN stream must decode
// back to exactly the expected MA sequence:
//
//   p1a  even byte of character i on dots 0-7 of its slot, odd byte on
//        dots 8-15 (t05h assumption, production cadence);
//   p1b  border flag low exactly across the R1 displayed slots, high in
//        every border cell up to C0=R0;
//   p1c  MA streams base..base+63 through the fetch path (pointer rules
//        already pinned by t03*, here proven end to end).

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <array>

#include "Vp1_video_test_top.h"
#include "verilated.h"

namespace {

class TestFailure : public std::runtime_error {
public:
	explicit TestFailure(const std::string& m) : std::runtime_error(m) {}
};

[[noreturn]] void fail(const std::string& what) { throw TestFailure(what); }

constexpr unsigned kDotsPerChar = 16;
constexpr unsigned kHChars = 64;        // R0 = 63
constexpr unsigned kDisplayed = 40;     // R1 = R6 = 40
constexpr uint16_t kBase = 0x0028;      // R12/R13 video base

class Bench {
public:
	Vp1_video_test_top dut;
	uint64_t cyc = 0;
	unsigned cen_phase = 0;

	// stimulus state
	bool reset_n = true;
	bool vid_enable = false, vid_ncs = true, vid_rnw = true, vid_rs = false;
	uint8_t vid_di = 0;
	uint16_t ga_addr = 0xFFFF;
	uint8_t ga_data = 0xFF;
	bool ga_iorq_n = true, ga_m1_n = true;
	bool fast = false;

	explicit Bench() : dut("p1_video_test_top") {}

	// Byte tag; must mirror pat() in p1_video_test_top.v.
	static uint8_t pat(uint16_t a) {
		uint8_t v = uint8_t(a & 0x7F);
		v ^= uint8_t((((a >> 8) & 0x3) << 6) | 0x16);
		v ^= uint8_t(0x80 | (((a >> 10) & 0x1F) << 2) | 0x01);
		return v;
	}

	static uint16_t vram_addr_for_ma(uint16_t ma) {
		// crtc_vram_addr = {MA[13:12], RA[2:0]=000, MA[9:0]}
		return uint16_t((((ma >> 12) & 0x3) << 13) | (ma & 0x03FF));
	}

	static uint16_t vram_word_for_ma(uint16_t ma) {
		uint16_t a = vram_addr_for_ma(ma);
		uint16_t even = uint16_t(a & 0x7FFE);
		return uint16_t((pat(even | 1) << 8) | pat(even));
	}

	void apply() {
		dut.clk = 0;
		dut.cen_16 = (cen_phase == 0);
		dut.fast = fast;
		dut.RESET_N = reset_n;
		dut.vid_enable = vid_enable;
		dut.vid_nCS = vid_ncs;
		dut.vid_R_nW = vid_rnw;
		dut.vid_RS = vid_rs;
		dut.vid_di = vid_di;
		dut.ga_addr = ga_addr;
		dut.ga_data = ga_data;
		dut.ga_iorq_n = ga_iorq_n;
		dut.ga_m1_n = ga_m1_n;
	}

	void tick() {
		apply();
		dut.eval();
		dut.clk = 1;
		dut.eval();
		++cyc;
		cen_phase = (cen_phase + 1) % 4;
	}

	void run(unsigned n) { for (unsigned i = 0; i < n; ++i) tick(); }

	void vid_write(uint8_t index, uint8_t value) {
		const bool dbg = getenv("P1VID_DEBUG") != nullptr;
		vid_rs = false; vid_rnw = false; vid_ncs = false; vid_enable = true;
		vid_di = uint8_t(index & 0x1F);
		run(2);
		if (dbg) std::printf("[wr] idx-phase done: rs=%d en=%d di=%02X\n",
		                     (int)vid_rs, (int)vid_enable, vid_di);
		vid_rs = true;
		vid_di = value;
		run(2);
		if (dbg) std::printf("[wr] val-phase done: rs=%d di=%02X pin=%02X r1=%02X rnw=%d ncs=%d en=%d\n",
		                     (int)vid_rs, vid_di, (unsigned)dut.vid_di,
		                     dut.dbg_r1, (int)vid_rnw, (int)vid_ncs,
		                     (int)vid_enable);
		vid_enable = false; vid_ncs = true; vid_rnw = true; vid_rs = false;
		run(2);
	}

	void ga_write(uint8_t value) {
		// Fast-path window: with fast=1 an IORQ-low cycle latches when the
		// ring hits S[2]&S[3]; holding ten character clocks guarantees it.
		fast = true;
		ga_m1_n = true;
		ga_iorq_n = false;
		ga_addr = 0x7F10;
		ga_data = value;
		run(40);
		fast = false;
		ga_iorq_n = true;
		run(4);
	}

	void power_on_reset() {
		reset_n = false;
		run(16);
		reset_n = true;
		run(16);
	}
};

void configure(Bench& b) {
	b.power_on_reset();
	b.ga_write(0x82);            // legacy GA port: screen mode 2
	b.vid_write(0, 63);          // R0: 64 cells per line
	b.vid_write(1, kDisplayed);  // R1
	if (getenv("P1VID_DEBUG"))
		std::printf("[after R1] R1=%02X addr=%02X\n", b.dut.dbg_r1, b.dut.dbg_addr_sel);
	b.vid_write(2, 50);          // R2
	b.vid_write(3, 0x22);        // R3 hsync=2 vsync=2
	b.vid_write(4, 0);           // R4: one row per frame
	b.vid_write(5, 0);           // R5: no adjustment
	b.vid_write(6, kDisplayed);  // R6
	b.vid_write(7, 6);           // R7: never matches C4=0 -> quiet
	b.vid_write(9, 0);           // R9: one scanline per row
	b.vid_write(12, 0);          // R12
	b.vid_write(13, kBase & 0xFF); // R13
	b.run(8);
}

void trace_raw(Bench& b) {
	// Log signals across two character windows starting at a row base.
	bool prev_clken = false;
	uint64_t guard = 0;
	for (;;) {
		const bool pre = b.dut.dbg_cclk_en_n != 0;
		b.tick();
		if (!pre && (b.dut.dbg_cclk_en_n != 0) && b.dut.dbg_ma == kBase) break;
		if (++guard > (1u << 22)) fail("trace: no line start");
	}
	unsigned dots = 0;
	while (dots < 34) {
		const bool dot = (b.cen_phase == 0);
		b.tick();
		if (dot || b.dut.dbg_cclk_en_n || b.dut.dbg_cclk_en_p ||
		    b.dut.dbg_ras_n == 0 || b.dut.dbg_cas_n == 0) {
			std::printf("[raw] d=%02u ma=%04X p=%d n=%d vd=%02X ras=%d cas=%d cpu=%d ep=%d en=%d de=%d pen=%02X\n",
			            dots, b.dut.dbg_ma, (int)b.cen_phase,
			            (int)(b.dut.dbg_cclk_en_n != 0), b.dut.dbg_vram_d,
			            b.dut.dbg_ras_n, b.dut.dbg_cas_n, b.dut.dbg_cpu_n,
			            (int)(b.dut.dbg_cclk_en_p != 0),
			            (int)(b.dut.dbg_cclk_en_n != 0),
			            (int)b.dut.dbg_de, b.dut.dbg_pen);
			if (dot) ++dots;
			if (++guard > (1u << 23)) fail("trace: stall");
		}
	}
}

bool p1_pixel_stream(Bench& b) {
	configure(b);
	if (getenv("P1VID_TRACE")) trace_raw(b);

	if (getenv("P1VID_DEBUG"))
		std::printf("[regs] R0=%02X R1=%02X addr=%02X\n",
		            b.dut.dbg_r0, b.dut.dbg_r1, b.dut.dbg_addr_sel);
	// Wait for a CLKEN pulse whose post-edge MA is the row base: that edge
	// just reloaded VMA for character 0 of the (single-line) frame.
	uint64_t guard = 0;
	for (;;) {
		const bool prev_clken_pre = b.dut.dbg_cclk_en_n != 0;
		b.tick();
		const bool clken_now = b.dut.dbg_cclk_en_n != 0;
		if (!prev_clken_pre && clken_now && b.dut.dbg_ma == kBase) break;
		if (++guard > (1u << 22)) fail("p1: line start never found");
	}

	// Collect one full line: kHChars slots x 16 dots. A dot sample is taken
	// after each clk edge on which cen_16 was high (asic_video registers
	// PEN/RGB on PIXEN edges). The first sample after the CLKEN edge is
	// dot 0 of the new character.
	std::array<std::array<uint8_t, kDotsPerChar>, kHChars> pens{};
	std::array<std::array<bool, kDotsPerChar>, kHChars> borders{};
	// Classic-oracle samples: ga40010's VIDEO_BUF latched at its two
	// buffer windows, snapshotted per character slot at each CLKEN edge.
	std::array<std::array<uint8_t, 2>, kHChars> classic_bytes{};
	uint8_t c_even = 0, c_odd = 0;
	bool prev_clken_s = false;
	for (unsigned ch = 0; ch < kHChars; ++ch) {
		for (unsigned d = 0; d < kDotsPerChar; ++d) {
			bool sampled = false;
			while (!sampled) {
				const bool dot_tick = (b.cen_phase == 0);
				const bool pre_clken = b.dut.dbg_cclk_en_n != 0;
				b.tick();
				if (dot_tick && b.dut.dbg_c_s == 0xE0) c_even = b.dut.dbg_c_vbuf;
				if (dot_tick && b.dut.dbg_c_s == 0x03) c_odd  = b.dut.dbg_c_vbuf;
				const bool clken_now = b.dut.dbg_cclk_en_n != 0;
				if (!pre_clken && clken_now && ch > 0) {
					classic_bytes[ch - 1][0] = c_even;
					classic_bytes[ch - 1][1] = c_odd;
				}
				prev_clken_s = clken_now;
				if (dot_tick) {
					pens[ch][d]    = b.dut.dbg_pen & 0x0F;
					borders[ch][d] = (b.dut.dbg_pen >> 4) != 0;
					sampled = true;
				}
				if (++guard > (1u << 23)) fail("p1: stream stalled");
			}
		}
	}

	// Mode 2: one bit per dot, MSB first per byte half.
	const bool debug = getenv("P1VID_DEBUG") != nullptr;
	for (unsigned ch = 0; ch < kHChars; ++ch) {
		uint8_t even = 0, odd = 0;
		for (unsigned d = 0; d < 8; ++d) {
			even = uint8_t((even << 1) | (pens[ch][d] & 1));
			odd  = uint8_t((odd  << 1) | (pens[ch][8 + d] & 1));
		}
		if (debug && ch < 8)
			std::printf("[dbg] slot %u even=%02X odd=%02X b0=%d bL=%d cl=%02X/%02X\n",
			            ch, even, odd, (int)borders[ch][0], (int)borders[ch][15],
			            classic_bytes[ch][0], classic_bytes[ch][1]);
		if (debug && ch < 8)
			std::printf("[dbg2] hcc=%02X r1=%02X hde=%d vde=%d de=%d\n",
			            b.dut.dbg_hcc, b.dut.dbg_r1,
			            (int)b.dut.dbg_hde, (int)b.dut.dbg_vde, (int)b.dut.dbg_de);
		if (ch < kDisplayed) {
			for (unsigned d = 0; d < kDotsPerChar; ++d)
				if (borders[ch][d])
					fail("p1b: border asserted inside displayed slot " + std::to_string(ch));
			if (even != classic_bytes[ch][0] || odd != classic_bytes[ch][1])
				fail("p1a: char " + std::to_string(ch) + " displayed " +
				     std::to_string(even) + "/" + std::to_string(odd) +
				     ", classic oracle shows " +
				     std::to_string(classic_bytes[ch][0]) + "/" +
				     std::to_string(classic_bytes[ch][1]));
		} else {
			for (unsigned d = 0; d < kDotsPerChar; ++d)
				if (!borders[ch][d])
					fail("p1b: border missing in border slot " + std::to_string(ch));
		}
	}

	return true;
}

} // namespace

bool p1_de_probe(Bench& b) {
	b.power_on_reset();
	b.ga_write(0x82);
	b.vid_write(0, 63);
	const char* var = getenv("P1VID_VAR");
	if (var && var[0]=='A') {           // exact original config
		b.vid_write(1, kDisplayed);
		b.vid_write(2, 50);
		b.vid_write(3, 0x22);
		b.vid_write(4, 0); b.vid_write(5, 0);
		b.vid_write(6, kDisplayed);
		b.vid_write(7, 6);
		b.vid_write(9, 0);
		b.vid_write(12, 0); b.vid_write(13, kBase & 0xFF);
	} else if (var && var[0]=='B') {    // R1=R6=40 only
		b.vid_write(1, kDisplayed);
		b.vid_write(6, kDisplayed);
	} else {                            // R1=R6=60 baseline
		b.vid_write(1, 60);
		b.vid_write(6, 60);
	}
	b.vid_write(4, 0); b.vid_write(5, 0); b.vid_write(9, 0); b.vid_write(7, 6);
	b.run(100);
	unsigned highs = 0;
	for (unsigned i = 0; i < 20000; ++i) {
		const bool dot = (b.cen_phase == 0);
		b.tick();
		if (dot && b.dut.dbg_de) ++highs;
	}
	std::printf("[de-probe] de-high dot samples = %u / %u\n", highs, 5000);
	return true;
}

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	if (getenv("P1VID_DEPROBE")) {
		Bench b2;
		p1_de_probe(b2);
		return 0;
	}
	try {
		Bench b;
		if (!p1_pixel_stream(b)) return 1;
	} catch (const TestFailure& e) {
		std::printf("FAIL: %s\n", e.what());
		return 1;
	}
	std::printf("PASS: p1 pixel phase / byte order / pointer stream\n");
	return 0;
}
