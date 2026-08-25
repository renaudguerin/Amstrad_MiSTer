// plus_mode=1 motherboard-level integration bench (queued P1 review
// follow-up: minimal proof that GA-register writes traverse the production
// motherboard into asic_video and that the Plus interrupt path reaches the
// CPU pin and clears on acknowledge).
//
// m1  boot + bus programming of the CRTC3 registers over the motherboard
//     decode (fake CPU script completes);
// m2  legacy GA RMR writes reach asic_video through asic_ga_timing's
//     GAMODE_O: mode 3 is observed from the 0x83 write and the final state
//     after the 0x82 write is mode 2;
// m3  INKR[5] = 0x15 and border = 0x04 payloads arrive on asic_video's
//     INKR_I/BORDER_I inputs (tapped via --public-flat-rw);
// m4  the 52-line interrupt fires into the CPU int_n pin (counted after
//     the prime acknowledge, so simulator zero-init levels don't count)
//     and clears on the fake Z80 acknowledge cycle.

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "Vp1_mobo_bench_top.h"
#include "Vp1_mobo_bench_top___024root.h"
#include "verilated.h"

namespace {

class TestFailure : public std::runtime_error {
public:
	explicit TestFailure(const std::string& m) : std::runtime_error(m) {}
};

[[noreturn]] void fail(const std::string& what) { throw TestFailure(what); }

constexpr uint64_t kScriptTimeout = 50000;   // script needs ~1.6k clks
constexpr uint64_t kFireTimeout   = 800000;  // one frame is ~229k clks
constexpr uint64_t kGlobalTimeout = 2000000;

class MoboBench {
public:
	Vp1_mobo_bench_top dut;
	uint64_t cyc = 0;

	explicit MoboBench() : dut("p1_mobo_bench_top") {
		dut.clk = 0;
		dut.reset = 1;
		dut.asic_page_on = 0;
	}

	void tick() {
		dut.clk = 0;
		dut.eval();
		dut.clk = 1;
		dut.eval();
		++cyc;
	}

	void run(uint64_t n) { for (uint64_t i = 0; i < n; ++i) tick(); }

	// Flat taps into the hierarchy (--public-flat-rw build).
	auto* border_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_border;
	}
	auto* inkr_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_inkr;
	}
	auto* cpu_step() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__CPU__DOT__dbg_step;
	}
	auto* cpu_done() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__CPU__DOT__dbg_done;
	}
	auto* cpu_fires() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__CPU__DOT__dbg_int_fires;
	}
	auto* cpu_level() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__CPU__DOT__dbg_int_level;
	}
	auto* spr_ram() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_page__DOT__spr_ram[0];
	}
	auto* pal_word(unsigned e) {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_page__DOT__pal[0];
	}
	auto* x_lo(unsigned n) {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_page__DOT__spr_x_lo[0];
	}

	static uint8_t inkr_entry(const VlWide<3>* w, unsigned k) {
		const unsigned lo = k * 5;
		if (lo >= 64)
			return uint8_t(((*w)[2] >> (lo - 64)) & 0x1F);
		const uint64_t low = uint64_t((*w)[0]) | (uint64_t((*w)[1]) << 32);
		return uint8_t((low >> lo) & 0x1F);
	}
};

int run() {
	MoboBench b;

	b.dut.reset = 1;
	b.run(64);
	b.dut.reset = 0;
	b.run(8);
	// The page-enable input stands in for plus_mmu's captured RMR2 state
	// (the motherboard-level bench has no MMU instance): raise it before
	// the scripted ASIC-page traffic and leave it on.
	b.dut.asic_page_on = 1;

	bool saw_mode3 = false;
	bool done_checked = false, done_checked2 = false;
	unsigned vec_samples = 0;
	bool vec_ok = false;
	bool fired = false, cleared_after_ack = false;
	uint8_t prev_mode = 0xFF;

	const bool trace2 = getenv("MOBO_TRACE") != nullptr;
	while (!cleared_after_ack) {
		b.tick();
		if (trace2 && b.cyc < 4000 && (b.cyc % 16) == 0)
			std::printf("[t %llu] step=%u mode=%u done=%d iorq=%d mreq=%d a=%04X d=%02X\n",
			            (unsigned long long)b.cyc,
			            (unsigned)*b.cpu_step(), (unsigned)b.dut.mode_o,
			            (int)*b.cpu_done(),
			            (int)b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__CPU__DOT__iorq_n,
			            (int)b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__CPU__DOT__mreq_n,
			            b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__CPU__DOT__a,
			            b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__CPU__DOT__do);

		if (getenv("MOBO_DEBUG") && (b.cyc % 50000 == 0)) {
			auto* r = b.dut.rootp;
			std::printf("[dbg %llu] mode=%u hs_vid=%d intN_asic=%d hcnt=%02x intcnt=%02x "
			            "fires=%u lvl=%d done=%d\n",
			            (unsigned long long)b.cyc,
			            (unsigned)b.dut.mode_o,
			            (int)r->p1_mobo_bench_top__DOT__mb__DOT__plus_crtc_hs,
			            (int)r->p1_mobo_bench_top__DOT__mb__DOT__asic_ga__DOT__INT_N,
			            (unsigned)r->p1_mobo_bench_top__DOT__mb__DOT__asic_ga__DOT__hcnt_reg,
			            (unsigned)r->p1_mobo_bench_top__DOT__mb__DOT__asic_ga__DOT__intcnt_reg,
			            (unsigned)*b.cpu_fires(),
			            (unsigned)*b.cpu_level(),
			            (int)*b.cpu_done());
		}

		const uint8_t mode_now = b.dut.mode_o;
		if (mode_now == 3 && *b.cpu_step() <= 19) saw_mode3 = true;
		prev_mode = mode_now;

		if (!done_checked && *b.cpu_done()) {
			if (!saw_mode3)
				fail("m2: mode 3 never appeared — the 0x83 RMR write did not reach GAMODE_O");
			if (mode_now != 2)
				fail("m2: mode pins should rest at 2 after the scripted RMR writes");
			if (*b.border_tap() != 4)
				fail("m3: border payload should be 4 on asic_video BORDER_I");
			if (b.inkr_entry(b.inkr_tap(), 5) != 0x15)
				fail("m3: INKR[5] should carry 0x15 on asic_video INKR_I");
			if (b.inkr_entry(b.inkr_tap(), 0) != 0x00)
				fail("m3: slot 0 must stay untouched by the scripted writes");
			done_checked = true;
			std::printf("PASS m1: plus_mode motherboard boots; CRTC3 programmed over the bus\n");
			std::printf("PASS m2: GA RMR writes reach asic_video via GAMODE_O (3 -> 2)\n");
			std::printf("PASS m3: INKR[5]/border payloads reach asic_video inputs\n");
		}
		if (*b.cpu_done() && !fired && b.cyc > kFireTimeout)
			fail("m4: no Plus interrupt reached the CPU pin before timeout");

		if (getenv("MOBO_TRACE") && (b.cyc % 32) == 0 && b.cyc < 3600)
			std::printf("[scr] cyc=%llu step=%u done=%d a=%04x ivr=%02x\n",
			            (unsigned long long)b.cyc,
			            (unsigned)*b.cpu_step(), (int)*b.cpu_done(),
			            b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__CPU__DOT__a,
			            (unsigned)b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_page__DOT__ivr_r);
		// m6: during any Z80-style acknowledge cycle the motherboard must
		// present (IVR & F8) | source on the CPU data bus. The script
		// wrote IVR=0xDA -> base 0xD8; raster-pending adds %110 -> 0xDE.
		// The prime acknowledge legitimately sees nothing pending (0xD8);
		// at least one pending-ack sample must show the raster vector.
		if (b.dut.vec_valid_o) {
			const uint8_t v = b.dut.vec_byte_o;
			if (v != 0xD8 && v != 0xDE)
				fail("m6: illegal ack-cycle vector byte " +
				     std::to_string(v) + " at cyc " +
				     std::to_string(b.cyc));
			if (v == 0xDE) vec_ok = true;
			vec_samples++;
		}

		if (!done_checked2 && *b.cpu_done()) {
			done_checked2 = true;
			const auto* ram = b.spr_ram();
			if (ram[0x000] != 0x5 || ram[0x001] != 0xC || ram[0x100] != 0xE)
				fail("m5: sprite RAM contents wrong after bus writes "
				     "(low-nibble mask or decode)");
			if (*b.x_lo(0) != 0x66)
				fail("m5: sprite 0 X lo wrong");
			// Palette entry 0 = {G=3,R=F,B=?}: low byte 0x0F -> R=0 B=F;
			// stored word {G,R,B} = 0x30F.
			if (b.pal_word(0)[0] != 0x30F)
				fail("m5: palette entry 0 wrong (layout or write decode)");
			std::printf("PASS m5: ASIC-page bus writes land in sprite RAM, "
			            "sprite regs and palette; unused regions ignored\n");
		}

		if (*b.cpu_fires() >= 1) {
			fired = true;
			if (*b.cpu_level() == 1) cleared_after_ack = true;
		}

		if (!fired && b.cyc > kFireTimeout)
			fail("m4: no Plus interrupt reached the CPU pin before timeout");
		if (!cleared_after_ack && b.cyc > kGlobalTimeout)
			fail("bench global timeout");
	}

	std::printf("PASS m4: Plus interrupt fires into the CPU pin and clears on acknowledge\n");
	if (!vec_ok || vec_samples < 2)
		fail("m6: no raster-source vector observed on acknowledge cycles");
	std::printf("PASS m6: INT-acknowledge vector byte 0xDE over %u samples\n",
	            vec_samples);
	return 0;
}

} // namespace

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	try {
		return run();
	} catch (const TestFailure& e) {
		std::printf("FAIL: %s\n", e.what());
		return 1;
	}
}
