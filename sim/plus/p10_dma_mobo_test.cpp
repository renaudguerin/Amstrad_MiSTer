// Full production-motherboard DMA/PPI concurrency regression (P10 round 3).
//
// The timing assertions below are derived from
// docs/plus/references/asic-reference.md, §9, "Timing & bus interaction":
// a DMA LOAD occupies at least eight CCLK cycles, extends by one cycle when a
// CPU 8255 access overlaps it, and extends by two cycles for an overlapping
// PSG-register write.  The CPU is allowed to wait for at most the ordinary
// eight-cycle ownership window; PPI strobes must never be accepted while the
// production dma_load_owner is asserted.  This test is deliberately
// production-shaped: it drives the real Amstrad_motherboard through its
// PHI-aligned T80pa replacement and observes the real page decoder, timing,
// DMA, PPI, and wait/strobe equations.

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "Vp10_dma_mobo_test_top.h"
#include "verilated.h"

namespace {

class TestFailure : public std::runtime_error {
public:
	explicit TestFailure(const std::string& message) : std::runtime_error(message) {}
};

[[noreturn]] void fail(const std::string& message) {
	throw TestFailure(message);
}

struct Sample {
	uint64_t cycle = 0;
	uint64_t cclk = 0;
	bool cclk_p = false;
	bool phi_p = false;
	bool phi_n = false;
	bool owner_pre = false;
	bool owner_post = false;
	bool busy_pre = false;
	bool busy_post = false;
	bool wait_n = false;
	bool dma_wait = false;
	bool ppi_access = false;
	bool psg_write = false;
	bool cpu_rd = false;
	bool ppi_we = false;
	bool ppi_oe = false;
	uint16_t address = 0;
	uint8_t data = 0xFF;
};

struct LoadWindow {
	uint64_t start_cclk = 0;
	uint64_t end_cclk = 0;
	unsigned duration = 0;
	unsigned owner_cclks = 0;
	bool ppi_overlap = false;
	bool psg_overlap = false;
};

class Bench {
public:
	Vp10_dma_mobo_test_top dut;
	uint64_t cycle = 0;
	uint64_t cclk_count = 0;
	std::vector<LoadWindow> loads;
	std::vector<Sample> trace;
	bool load_open = false;
	LoadWindow load;
	bool io_open = false;
	unsigned io_wait_cclks = 0;
	unsigned io_transactions = 0;
	unsigned io_waited_transactions = 0;
	unsigned io_max_wait_cclks = 0;
	unsigned io_accept_edges = 0;
	bool io_started_under_owner = false;
	bool io_owner_seen = false;
	bool io_is_read = false;
	uint16_t io_address = 0;
	bool ppi_strobe_prev = false;
	bool saw_owner_on_io = false;
	bool saw_accept_on_owner = false;
	bool saw_wait_without_owner = false;
	bool saw_preowner_read_overlap = false;

	Bench() : dut("p10_dma_mobo_test_top") {
		dut.clk = 0;
		dut.reset = 1;
		dut.plus_aspage_on_i = 0;
		dut.vram_din_i = 0x0700; // LOAD R7, #00, held on the DMA VRAM bus
		dut.ps2_key_i = 0;
	}

	Sample tick(bool keep_trace = true) {
		dut.clk = 0;
		dut.eval();
		Sample s;
		s.cycle = cycle;
		s.cclk = cclk_count;
		s.cclk_p = dut.cclk_en_p_o;
		s.phi_p = dut.phi_en_p_o;
		s.phi_n = dut.phi_en_n_o;
		s.owner_pre = dut.dma_load_owner_o;
		s.busy_pre = dut.dma_load_busy_o;
		s.wait_n = dut.cpu_wait_n_o;
		s.dma_wait = dut.dma_ppi_wait_o;
		s.ppi_access = dut.cpu_ppi_access_o;
		s.psg_write = dut.cpu_psg_write_o;
		s.cpu_rd = dut.cpu_rd_o;
		s.ppi_we = dut.ppi_we_o;
		s.ppi_oe = dut.ppi_oe_o;
		s.address = dut.cpu_addr_o;
		s.data = dut.cpu_di_o;
		if (s.cclk_p)
			++cclk_count;

		dut.clk = 1;
		dut.eval();
		++cycle;
		s.owner_post = dut.dma_load_owner_o;
		s.busy_post = dut.dma_load_busy_o;
		if (keep_trace)
			trace.push_back(s);

		observe(s);
		return s;
	}

	void observe(const Sample& s) {
		const bool strobe = s.ppi_we || s.ppi_oe;
		const bool accept_edge = strobe && !ppi_strobe_prev;
		if (accept_edge && s.owner_pre)
			saw_accept_on_owner = true;
		if (s.dma_wait && !s.ppi_access)
			fail("production dma_ppi_wait asserted without a PPI access");
		if (s.dma_wait && s.wait_n)
			fail("CPU wait_n stayed high during production dma_ppi_wait");
		if (s.ppi_access && s.owner_pre)
			saw_owner_on_io = true;
		if (s.ppi_access && !s.wait_n && !s.owner_pre)
			saw_wait_without_owner = true;

		// Count one wait unit per CCLK phase, not raw 64 MHz clocks.  The
		// fake CPU only retires a cycle on PHI_EN_N and keeps its bus stable
		// until wait_n is released.
		if (s.ppi_access && !io_open) {
			io_open = true;
			io_wait_cclks = 0;
			io_accept_edges = 0;
			io_started_under_owner = s.owner_pre;
			io_owner_seen = s.owner_pre || s.owner_post;
			io_is_read = s.cpu_rd;
			io_address = s.address;
			++io_transactions;
		}
		if (io_open)
			io_owner_seen = io_owner_seen || s.owner_pre || s.owner_post;
		if (io_open && s.phi_n && s.wait_n && s.cpu_rd &&
		    (s.address == 0xF400) && (s.data != 0xDF)) {
			fail("AY R14 read returned " + std::to_string(s.data) +
			     " instead of 223 (started-owner=" +
			     std::to_string(io_started_under_owner) +
			     ", owner-seen=" + std::to_string(io_owner_seen) + ")");
		}
		if (accept_edge && io_open) {
			++io_accept_edges;
			if (io_accept_edges > 1)
				fail("one CPU PPI transaction produced multiple accepted strobe edges");
		}
		if (io_open && s.cclk_p && !s.wait_n) {
			++io_wait_cclks;
			++io_waited_transactions;
		}
		if (io_open && !s.ppi_access) {
			if (io_accept_edges != 1)
				fail("completed CPU PPI transaction did not produce exactly one accepted strobe edge");
			if (io_is_read && (io_address == 0xF400) &&
			    !io_started_under_owner && io_owner_seen)
				saw_preowner_read_overlap = true;
			io_max_wait_cclks = (io_wait_cclks > io_max_wait_cclks) ?
				io_wait_cclks : io_max_wait_cclks;
			io_open = false;
		}
		ppi_strobe_prev = strobe;

		if (!load_open && !s.busy_pre && s.busy_post) {
			load_open = true;
			load = LoadWindow{};
			load.start_cclk = s.cclk;
		}
		if (load_open) {
			if (s.cclk_p && (s.owner_pre || s.owner_post))
				++load.owner_cclks;
			load.ppi_overlap = load.ppi_overlap || s.ppi_access;
			load.psg_overlap = load.psg_overlap || s.psg_write;
		}
		if (load_open && s.busy_pre && !s.busy_post) {
			load.end_cclk = s.cclk;
			load.duration = static_cast<unsigned>(load.end_cclk - load.start_cclk + 1);
			loads.push_back(load);
			load_open = false;
		}
	}

	void run(uint64_t clocks) {
		for (uint64_t i = 0; i < clocks; ++i)
			tick();
	}
};

void require(bool condition, const std::string& message) {
	if (!condition)
		fail(message);
}

void check_loads(const Bench& b) {
	bool saw8 = false, saw9 = false, saw10 = false;
	for (const LoadWindow& load : b.loads) {
		const unsigned expected = 8u + (load.psg_overlap ? 2u : load.ppi_overlap ? 1u : 0u);
		if (load.duration != expected) {
			fail("LOAD duration mismatch: observed " + std::to_string(load.duration) +
			     " CCLK, source-derived expected " + std::to_string(expected) +
			     " (ppi=" + std::to_string(load.ppi_overlap) +
			     ", psg=" + std::to_string(load.psg_overlap) + ")");
		}
		if (load.duration == 8) saw8 = true;
		if (load.duration == 9) saw9 = true;
		if (load.duration == 10) saw10 = true;
		if (load.owner_cclks > 8)
			fail("DMA ownership exceeded the source-required eight CCLK window");
	}
	require(saw8, "phase sweep did not produce an uncontended eight-CCLK LOAD");
	require(saw9, "phase sweep did not produce a one-cycle PPI-extended LOAD");
	require(saw10, "phase sweep did not produce a two-cycle PSG-extended LOAD");
}

int run() {
	Bench b;
	b.run(64);
	b.dut.reset = 0;
	b.dut.plus_aspage_on_i = 1;
	b.dut.ps2_key_i = 0x61C; // PS/2 make A; AY R14 row 8 must read 0xDF

	// Allow the PHI-aligned script to program CRTC3, PPI, and the DMA page.
	// The script then runs 96 alternating PPI reads/PSG writes with regular
	// idle gaps, supplying a deterministic phase sweep over repeated HSYNCs.
	const uint64_t timeout = 4000000;
	while (b.loads.size() < 24 && b.cycle < timeout)
		b.tick();

	if (b.loads.size() < 24) {
		fail("full motherboard fixture timed out before 24 DMA LOAD windows (" +
		     std::to_string(b.loads.size()) + " observed; CPU operations=" +
		     std::to_string(b.dut.cpu_operations_o) + ")");
	}
	check_loads(b);
	require(!b.saw_accept_on_owner,
	        "a new PPI write/read edge was accepted while dma_load_owner was asserted");
	require(b.io_transactions >= 20, "PHI-aligned script did not execute repeated PPI cycles");
	require(b.io_max_wait_cclks <= 8,
	        "a CPU PPI transaction waited more than the source-required eight CCLKs");
	require(b.dut.cpu_timing_error_o == 0,
	        "fake T80pa observed a bus transition outside PHI_EN_P/PHI_EN_N");
	require(b.saw_preowner_read_overlap,
	        "phase sweep did not exercise a Port-A read started before DMA ownership");
	require(b.dut.cpu_read_error_o == 0,
	        "PHI-aligned AY R14 keyboard read lost its 0xDF sentinel during DMA ownership");

	std::printf("PASS p10 full motherboard DMA/PPI concurrency: %zu LOADs, "
	            "PPI transactions=%u, max wait=%u CCLK, CPU operations=%u\n",
	            b.loads.size(), b.io_transactions, b.io_max_wait_cclks,
	            b.dut.cpu_operations_o);
	return 0;
}

} // namespace

int main() {
	try {
		return run();
	}
	catch (const TestFailure& e) {
		std::fprintf(stderr, "FAIL p10 full motherboard DMA/PPI concurrency: %s\n", e.what());
		return 1;
	}
}
