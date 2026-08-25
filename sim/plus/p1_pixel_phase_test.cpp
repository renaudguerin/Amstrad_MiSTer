// P1 pixel-phase validation (docs/plus/architecture.md §4 P1 exit item,
// closing the t05h unverified-assumption note against the production
// cadence).
//
// Drives sim/plus/p1_video_test_top.v — asic_ga_timing + asic_video plus a
// verbatim copy of the motherboard VRAM fetch and word-assembly blocks, and
// a classic CRTC type-0 + ga40010 oracle slice fed the same program — with
// a one-line frame (R4=0, R5=0, R9=0), R1=R6=40 displayed characters of a
// 64-character line in screen mode 2. The fake VRAM returns words that
// encode their own even address, so the displayed PEN stream can be decoded
// end to end:
//
//   p1a  every line streams its words in order — the CRTC address is
//        word-granular, so the pair fetched for ma=X legitimately spans
//        two character slots (labels 0,0,2,2,...) — with dots 0-7 always
//        carrying the even byte and dots 8-15 the odd byte (the t05h
//        assumption: first pixel of the even byte on dot 0, one-dot
//        registered presentation), plus a bounded budget for the mixed
//        word presented across the MA reload at each line start;
//   p1b  border flag clear on all 16 dots of display-interior slots and
//        set in border-interior slots. Interiors only: the PEN flag uses
//        de_hold (DE captured one slot earlier), so the colour-class
//        switch at region boundaries is skewed by up to one character —
//        that skew is the documented GA pipeline latency question that
//        stays open with the motherboard-integration timing contract;
//   p1c  classic-oracle provenance: under the same program and fake VRAM,
//        the reference netlist slice's VIDEO_BUF holds only bytes whose
//        tags encode addresses inside the same line window, covering
//        every word. Byte ORDER through the Plus assembly is already
//        pinned end to end by p1a; the netlist buffer stage's own
//        latency semantics are GADIFF lockstep territory.
//
// Timing model (derived from the RTL and verified against pix_cnt): a
// character slot is the 16-dot window between CLKEN action edges; the
// sample on the boundary edge itself shows the previous slot's last
// pixel, and the next 16 PIXEN samples are dots 0..15. The bench anchors
// on post-edge pix_cnt==0 rather than a CLKEN level probe: the ring may
// already show its next state when the tick returns, which puts a level
// probe on the wrong one of the two adjacent edges.
//
// The video base sits at kBase=0x0080 because the tag pattern pat() is
// only injective within a 128-byte-aligned window; a 64-word line then
// never crosses an aliasing boundary.

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
// Video base at a 128-byte-aligned window: the fake VRAM tag pattern
// pat() is only injective within such a window (its mid/high address
// terms alias across lower boundaries otherwise), and a 64-word line
// then stays inside one window. Documented in the header below.
constexpr uint16_t kBase = 0x0080;

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

	// Byte tag; must mirror pat() in p1_video_test_top.v:
	//   {1'b0,a[6:0]} ^ {a[9:8],6'b010110} ^ {2'b01,a[14:10],1'b1}
	// (the last term is 0x40-based: bit6 set, bit7 clear).
	static uint8_t pat(uint16_t a) {
		uint8_t v = uint8_t(a & 0x7F);
		v ^= uint8_t((((a >> 8) & 0x3) << 6) | 0x16);
		v ^= uint8_t(0x40 | ((((a >> 10) & 0x1F) << 1) | 0x01));
		return v;
	}

	static uint16_t vram_addr_for_ma(uint16_t ma) {
		// crtc_vram_addr = {MA[13:12], RA[2:0]=000, MA[9:0]}
		return uint16_t((((ma >> 12) & 0x3) << 13) | (ma & 0x03FF));
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

	bool clken() const { return dut.dbg_cclk_en_n != 0; }
	bool de() const { return dut.dbg_de != 0; }

	void vid_write(uint8_t index, uint8_t value) {
		vid_rs = false; vid_rnw = false; vid_ncs = false; vid_enable = true;
		vid_di = uint8_t(index & 0x1F);
		run(2);
		vid_rs = true;
		vid_di = value;
		run(2);
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
	b.vid_write(2, 50);          // R2
	b.vid_write(3, 0x22);        // R3 hsync=2 vsync=2
	b.vid_write(4, 0);           // R4: one row per frame
	b.vid_write(5, 0);           // R5: no adjustment
	b.vid_write(6, kDisplayed);  // R6
	b.vid_write(7, 6);           // R7: never matches C4=0 -> quiet
	b.vid_write(9, 0);           // R9: one scanline per row
	b.vid_write(12, (kBase >> 8) & 0x3F); // R12
	b.vid_write(13, kBase & 0xFF);        // R13
	b.run(8);
}

void trace_raw(Bench& b) {
	// Debug aid: log signals around a row-base line start.
	uint64_t guard = 0;
	for (;;) {
		const bool pre = b.clken();
		b.tick();
		if (!pre && b.clken() && b.dut.dbg_ma == kBase) break;
		if (++guard > (1u << 22)) fail("trace: no line start");
	}
	unsigned dots = 0;
	while (dots < 34) {
		const bool dot = (b.cen_phase == 0);
		b.tick();
		if (dot || b.clken()) {
			std::printf("[raw] d=%02u ma=%04X p=%d n=%d ep=%d vd=%02X vbs=%d vw=%04X de=%d pen=%02X va=%04X pc=%u ve=%02X vo=%02X\n",
			            dots, b.dut.dbg_ma, (int)b.cen_phase,
			            (int)(b.dut.dbg_cclk_en_n != 0), (int)b.dut.dbg_ep,
			            b.dut.dbg_vram_d, (int)b.dut.dbg_vbs,
			            b.dut.dbg_vidword, (int)b.dut.dbg_de, b.dut.dbg_pen,
			            b.dut.dbg_vaddr, b.dut.dbg_ra,
			            b.dut.dbg_pixcnt, b.dut.dbg_veven, b.dut.dbg_vodd);
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

	// Classic-oracle capture: every VIDEO_BUF value presented at either
	// buffer window while the classic MA runs inside the line's address
	// window. The reference buffer stage has its own pipeline lag, so
	// values are recorded globally (tag-provenance model) rather than
	// paired to the simultaneously-live MA.
	std::array<uint32_t, 256> classic_hits{};
	auto classic_capture = [&]() {
		const uint16_t cma = b.dut.dbg_c_ma;
		if (cma < kBase || cma >= kBase + kHChars) return;
		if (b.dut.dbg_c_s == 0xE0 || b.dut.dbg_c_s == 0x03)
			++classic_hits[b.dut.dbg_c_vbuf];
	};

	uint64_t guard = 0;

	// Locate a slot boundary: the cen_16 edge whose post-edge pix_cnt
	// reads 0 is the CLKEN action edge (sequential logic consumed CLKEN
	// and reset the dot counter on it). Reading pix_cnt instead of a
	// CLKEN level avoids a probe subtlety: by the time the tick returns,
	// the ring may already show its NEXT state, so a CLKEN-level probe
	// lands on the ring-arrival edge one cen_16 before the action edge.
	// Collection is self-calibrating afterwards, so any boundary works.
	uint64_t guard2 = 0;
	for (;;) {
		const bool dot_tick = (b.cen_phase == 0);
		b.tick();
		classic_capture();
		if (dot_tick && b.dut.dbg_pixcnt == 0) break;
		if (++guard2 > (1u << 23)) fail("p1: no slot boundary after reset");
	}

	// Collect three lines of slots x kDotsPerChar dots. The anchor edge
	// itself shows the previous slot's last pixel; the next 16 PIXEN
	// samples are exactly one slot's dots 0..15 (pix_cnt resets on CLKEN).
	constexpr unsigned kSlotCount = 3 * kHChars;
	std::array<std::array<uint8_t, kDotsPerChar>, kSlotCount> pens{};
	std::array<std::array<bool, kDotsPerChar>, kSlotCount> borders{};
	for (unsigned ch = 0; ch < kSlotCount; ++ch) {
		for (unsigned d = 0; d < kDotsPerChar; ++d) {
			for (;;) {
				const bool dot_tick = (b.cen_phase == 0);
				b.tick();
				classic_capture();
				if (++guard > (1u << 24)) fail("p1: stream stalled");
				if (!dot_tick) continue;
				pens[ch][d]    = b.dut.dbg_pen & 0x0F;
				borders[ch][d] = (b.dut.dbg_pen >> 4) != 0;
				if (getenv("P1VID_DEBUG") && ch < 2)
					std::printf("[col] ch=%u d=%u pc=%u n=%d pen=%02X\n",
					            ch, d, b.dut.dbg_pixcnt,
					            (int)b.clken(), b.dut.dbg_pen);
				break;
			}
		}
	}

	// Identify every slot by content: reconstruct the mode-2 pair from
	// dots 0-7 / 8-15 and match it against the pat()-encoded row address.
	// Success requires BOTH halves in their documented positions, so this
	// closes the t05h phase claim (even byte on dots 0-7, MSB first,
	// one-dot registered presentation) together with the pointer stream.
	std::array<int, kSlotCount> slot_x{};
	for (unsigned ch = 0; ch < kSlotCount; ++ch) {
		uint8_t even = 0, odd = 0;
		for (unsigned d = 0; d < 8; ++d) {
			even = uint8_t((even << 1) | (pens[ch][d] & 1));
			odd  = uint8_t((odd  << 1) | (pens[ch][8 + d] & 1));
		}
		slot_x[ch] = -1;
		for (unsigned X = 0; X < kHChars; ++X) {
			const uint16_t ma = uint16_t(kBase + X);
			const uint16_t va = Bench::vram_addr_for_ma(ma);
			const uint16_t ea = uint16_t(va & 0x7FFE);
			if (Bench::pat(ea) == even && Bench::pat(uint16_t(ea | 1)) == odd) {
				slot_x[ch] = int(X);
				break;
			}
		}
	}

	// The CRTC address is WORD-granular: the word fetched for ma=X also
	// covers ma=X+1 (its CAS halves are the byte pair at the aligned
	// address), so identification labels come back as 0,0,2,2,... Each
	// word therefore legitimately spans two character slots; assert the
	// full repeating pattern rather than per-slot uniqueness.
	// Split the collection into complete lines: a line starts where the
	// label wraps to 0. Every line must stream 0,0,2,2,...,62,62.
	// Candidate line starts: the first of a double-zero label pair that
	// continues with a double-two (the wrap transient can forge a single
	// stray zero, so require the full opening signature). Score every
	// candidate over its 64 slots and keep up to three non-overlapping
	// best lines.
	std::array<unsigned, 4> line_starts{};
	unsigned nlines = 0;
	for (unsigned ch = 0; ch + kHChars <= kSlotCount && nlines < 4u; ++ch) {
		if (!(slot_x[ch] == 0 && slot_x[ch + 1] == 0 &&
		      slot_x[ch + 2] == 2 && slot_x[ch + 3] == 2))
			continue;
		unsigned score = 0;
		for (unsigned i = 0; i < kHChars; ++i)
			if (slot_x[ch + i] == int(i & ~1u)) ++score;
		bool better = true;
		for (unsigned L = 0; L < nlines; ++L)
			if (line_starts[L] == ch ||
			    (ch > line_starts[L] && ch < line_starts[L] + kHChars))
				better = false; // overlaps a kept line; keep the earlier
		if (!better) continue;
		if (score < kHChars - 4) continue; // per-line budget below
		line_starts[nlines++] = ch;
	}
	if (nlines < 2)
		fail("p1a: fewer than two clean lines found in " +
		     std::to_string(kSlotCount) + " slots");
	for (unsigned L = 0; L + 1 < nlines; ++L) {
		const unsigned base = line_starts[L];
		const unsigned len = line_starts[L + 1] - base;
		if (len != kHChars)
			fail("p1a: line length " + std::to_string(len) +
			     " slots, expected " + std::to_string(kHChars));
		// Tolerate a bounded number of boundary anomalies: the slot
		// straddling the MA reload can present a mixed/transient word (one
		// VIDBUF half from the old address, one from the new), which may
		// decode as unknown or as an unrelated label. Real pointer breaks
		// shift EVERY subsequent label and cannot stay inside the budget.
		unsigned unknown = 0, misplaced = 0;
		for (unsigned i = 0; i < kHChars; ++i) {
			const int got = slot_x[base + i];
			if (got < 0) { ++unknown; continue; }
			if (got != int(i & ~1u)) ++misplaced;
		}
		if (unknown > 2)
			fail("p1a: " + std::to_string(unknown) + " unidentified slots in one line");
		if (misplaced > 1)
			fail("p1a: pointer stream broken (" + std::to_string(misplaced) +
			     " misplaced slots in one line)");
	}

	// Flag classification, interior-only so the documented one-character
	// colour-class skew at region boundaries (GA pipeline latency, t05h
	// note) cannot flip a verdict. Indices are slot positions in the line:
	//   slots 1..R1-2      : display interior -> flag clear on all 16 dots
	//   slots R1+2..R0-2   : border interior  -> flag set on all 16 dots
	for (unsigned L = 0; L + 1 < nlines; ++L) {
		const unsigned base = line_starts[L];
		for (unsigned i = 1; i + 2 < kDisplayed; ++i)
			for (unsigned d = 0; d < kDotsPerChar; ++d)
				if (borders[base + i][d])
					fail("p1b: border flag asserted in display-interior slot " +
					     std::to_string(i));
		for (unsigned i = kDisplayed + 2; i + 2 < kHChars; ++i)
			for (unsigned d = 0; d < kDotsPerChar; ++d)
				if (!borders[base + i][d])
					fail("p1b: border flag missing in border-interior slot " +
					     std::to_string(i));
	}

	// Classic oracle: content provenance. Byte ORDER through the Plus
	// assembly is already pinned end to end by p1a (dots 0-7 carry the
	// even half through the production VIDBUF windows); the reference
	// netlist's own buffer-stage latency is GADIFF territory. What this
	// bench adds here: under the same program and fake VRAM, the classic
	// slice's VIDEO_BUF must hold bytes from the SAME addressed window
	// (both tag patterns must appear per word), i.e. both pipelines
	// consume the same memory locations.
	// Provenance assertions: every captured byte must be one of the two
	// tag bytes of some word in the line's window, and every word's tag
	// pair must have been observed at least once across the run.
	const uint16_t win_lo = kBase, win_hi = uint16_t(kBase + kHChars);
	unsigned covered = 0;
	for (uint32_t v = 0; v < 256; ++v) {
		if (classic_hits[v] == 0) continue;
		bool allowed = false;
		for (uint16_t a = win_lo; a < win_hi && !allowed; ++a)
			allowed = (v == Bench::pat(a));
		if (!allowed)
			fail("p1c: classic VIDEO_BUF held byte " + std::to_string(v) +
			     ", which encodes no address in the line window");
	}
	for (uint16_t w = win_lo; w < win_hi; w += 2) {
		if (classic_hits[Bench::pat(w)] > 0 &&
		    classic_hits[Bench::pat(uint16_t(w | 1))] > 0)
			++covered;
	}
	if (covered < kHChars / 2 - 1)
		fail("p1c: classic oracle covered only " + std::to_string(covered) +
		     "/" + std::to_string(kHChars / 2) + " words");

	return true;
}

} // namespace

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
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
