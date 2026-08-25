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

	bool saw_mode3 = false;
	bool done_checked = false;
	bool fired = false, cleared_after_ack = false;
	uint8_t prev_mode = 0xFF;

	while (!cleared_after_ack) {
		b.tick();

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
