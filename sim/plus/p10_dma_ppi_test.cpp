// Focused production-shaped P10e DMA/PPI/PSG/input concurrency test.
//
// The timing expectations are taken from
// docs/plus/references/asic-reference.md §9, lines 438-446: a LOAD occupies
// at least eight CCLK cycles, with +1 for a simultaneous 8255 access and +2
// when that access is a PSG register write.  The same paragraph requires the
// PPI port-A direction, selected AY register, and AY read/write state to be
// restored after the LOAD.

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "Vp10_dma_ppi_test_top.h"
#include "verilated.h"

namespace {

class TestFailure : public std::runtime_error {
public:
	explicit TestFailure(const std::string& message) : std::runtime_error(message) {}
};

[[noreturn]] void fail(const std::string& message) {
	throw TestFailure(message);
}

struct TickSample {
	uint64_t cycle = 0;
	uint64_t cclk_index = 0;
	bool cclk_p_pre = false;
	bool owner_pre = false;
	bool owner_post = false;
	bool busy_pre = false;
	bool busy_post = false;
	bool active_pre = false;
	bool active_post = false;
	bool wait_pre = false;
	bool wait_post = false;
	bool dma_wait_pre = false;
	bool ppi_access_pre = false;
	bool psg_write_pre = false;
	bool ppi_we_pre = false;
	bool ppi_oe_pre = false;
	uint8_t cpu_di_pre = 0xFF;
	bool dma_bdir_post = false;
	bool dma_bc1_post = false;
	uint8_t dma_di_post = 0;
};

struct IoResult {
	bool read = false;
	uint8_t data = 0xFF;
	unsigned wait_stalls = 0;
	unsigned wait_cclk = 0;
	unsigned strobe_edges = 0;
	bool saw_dma_wait = false;
	bool saw_psg_write = false;
	bool accepted_while_owner = false;
};

struct LoadResult {
	unsigned duration = 0;
	unsigned owner_samples = 0;
	bool saw_dma_address = false;
	bool saw_dma_data = false;
	bool saw_dma_restore = false;
	IoResult overlap;
	std::vector<TickSample> trace;
};

class Bench {
public:
	Vp10_dma_ppi_test_top dut;
	uint64_t cycle = 0;
	uint64_t cclk_count = 0;
	std::vector<TickSample> trace;

	Bench() : dut("p10_dma_ppi_test_top") {
		dut.clk = 0;
		dut.reset = 1;
		dut.hsync_i = 0;
		dut.ps2_key_i = 0;
		dut.user_in_i = 0x7F;
		dut.snac_player_i = 0;
		dut.joy1_usb_i = 0;
		dut.joy2_usb_i = 0;
		dut.dma_sar0_i = 0x2000;
		dut.dma_sar0_wr_i = 0;
		dut.dma_ppr0_i = 0;
		dut.dma_ena_i = 0;
		dut.dma_instr_addr_i = 0x2000;
		dut.dma_instr_i = 0x0700; // LOAD R7, 0x00
		bus_idle();
	}

	void bus_idle() {
		dut.cpu_addr_i = 0xFFFF;
		dut.cpu_do_i = 0xFF;
		dut.cpu_rd_n_i = 1;
		dut.cpu_wr_n_i = 1;
		dut.cpu_iorq_n_i = 1;
		dut.cpu_mreq_n_i = 1;
		dut.cpu_m1_n_i = 1;
	}

	void bus_io(uint16_t address, uint8_t data, bool read) {
		dut.cpu_addr_i = address;
		dut.cpu_do_i = data;
		dut.cpu_rd_n_i = read ? 0 : 1;
		dut.cpu_wr_n_i = read ? 1 : 0;
		dut.cpu_iorq_n_i = 0;
		dut.cpu_mreq_n_i = 1;
		dut.cpu_m1_n_i = 1;
	}

	TickSample tick(bool record = true) {
		dut.clk = 0;
		dut.eval();

		TickSample sample;
		sample.cycle = cycle;
		sample.cclk_p_pre = dut.cclk_en_p_o;
		sample.cclk_index = cclk_count;
		sample.owner_pre = dut.dma_load_owner_o;
		sample.busy_pre = dut.dma_load_busy_o;
		sample.active_pre = dut.dma_psg_active_o;
		sample.wait_pre = dut.wait_n_o;
		sample.dma_wait_pre = dut.dma_ppi_wait_o;
		sample.ppi_access_pre = dut.cpu_ppi_access_o;
		sample.psg_write_pre = dut.cpu_psg_write_o;
		sample.ppi_we_pre = dut.ppi_we_o;
		sample.ppi_oe_pre = dut.ppi_oe_o;
		sample.cpu_di_pre = dut.cpu_di_o;
		if (sample.cclk_p_pre)
			++cclk_count;

		dut.clk = 1;
		dut.eval();
		++cycle;
		sample.owner_post = dut.dma_load_owner_o;
		sample.busy_post = dut.dma_load_busy_o;
		sample.active_post = dut.dma_psg_active_o;
		sample.wait_post = dut.wait_n_o;
		sample.dma_bdir_post = dut.psg_bdir_o;
		sample.dma_bc1_post = dut.psg_bc1_o;
		sample.dma_di_post = dut.psg_di_o;
		if (record)
			trace.push_back(sample);
		return sample;
	}

	void run(unsigned clocks) {
		for (unsigned i = 0; i < clocks; ++i)
			tick();
	}

	void reset_and_configure() {
		dut.reset = 1;
		dut.hsync_i = 0;
		dut.dma_ena_i = 0;
		// Return the edge-coded PS/2 input to idle before each scenario so
		// the following make code is a fresh event after reset.
		dut.ps2_key_i = 0;
		bus_idle();
		run(48);
		dut.reset = 0;
		run(16);

		// The ASIC page decoder is represented by the direct SAR/PPR/DCSR
		// seam.  A one-clock SAR write mirrors asic_regs' sar0_wr pulse.
		dut.dma_sar0_i = 0x2000;
		dut.dma_sar0_wr_i = 1;
		tick();
		dut.dma_sar0_wr_i = 0;
		dut.dma_ena_i = 1;
		bus_idle();
		run(8);

		// Program the AY while Port A is an output, then use Plus control
		// 0x9B for keyboard reads.  (0x9B makes Port A an input, so the
		// programming phase must precede it.)  Select R14 and leave Port C
		// on keyboard row 8 with the AY bus inactive.  Addresses are the
		// real CPC F4xx/F6xx/F7xx
		// aliases; only A[11] and A[9:8] matter to the production decode.
		io_write(0xF700, 0x82);
		select_ay_register(7);
		io_write(0xF400, 0x80);
		io_write(0xF600, 0x88);
		io_write(0xF600, 0x00);
		select_ay_register(0x0E);
		io_write(0xF600, 0x08);
		io_write(0xF700, 0x9B);
		if (dut.psg_addr_o != 0x0E)
			fail("setup: CPU AY register shadow did not select R14");
		if (dut.ppi_port_c_o != 0x08)
			fail("setup: Port C did not retain keyboard row 8/inactive AY state");
	}

	void select_ay_register(uint8_t reg) {
		io_write(0xF400, reg);
		io_write(0xF600, 0xC8); // BDIR=1, BC1=1, row 8
		io_write(0xF600, 0x00);
	}

	void io_write(uint16_t address, uint8_t data) {
		IoResult result = io_cycle(address, data, false);
		if (result.strobe_edges == 0)
			fail("setup: PPI write never produced a gated write strobe");
		// Allow the real 1 MHz AY character-clock edge to consume a PPI
		// control/data write after the PPI's registered output changes.
		run(24);
	}

	IoResult io_read(uint16_t address) {
		IoResult result = io_cycle(address, 0xFF, true);
		if (result.strobe_edges == 0)
			fail("PPI read never produced a gated read strobe");
		return result;
	}

	IoResult io_cycle(uint16_t address, uint8_t data, bool read) {
		bus_io(address, data, read);
		IoResult result;
		result.read = read;
		bool previous_strobe = false;
		for (unsigned guard = 0; guard < 20000; ++guard) {
			TickSample sample = tick();
			if (!sample.wait_pre)
				++result.wait_stalls;
			if (sample.dma_wait_pre && sample.ppi_access_pre)
				result.saw_dma_wait = true;
			if (sample.psg_write_pre)
				result.saw_psg_write = true;
			const bool strobe = read ? sample.ppi_oe_pre : sample.ppi_we_pre;
			if (strobe && !previous_strobe) {
				++result.strobe_edges;
				result.data = sample.cpu_di_pre;
				result.accepted_while_owner = sample.owner_pre;
				bus_idle();
				// Retire the cycle while the accepted PPI edge is still in the
				// trace, then leave one raw clock for the registered PPI edge.
				tick();
				return result;
			}
			previous_strobe = strobe;
		}
		fail(std::string("I/O cycle timed out at ") + (read ? "read" : "write"));
	}

	void pulse_hsync() {
		dut.hsync_i = 0;
		tick();
		dut.hsync_i = 1;
		tick();
		dut.hsync_i = 0;
	}

	LoadResult run_load(unsigned overlap_kind) {
		// 0=none, 1/2=early PPI/PSG, 3/4=eighth-cycle PPI/PSG.
		const bool psg_overlap = (overlap_kind == 2 || overlap_kind == 4);
		const bool late_overlap = (overlap_kind >= 3);
		reset_and_configure();
		if (psg_overlap) {
			// Port A is input under 0x9B, so briefly use the output mode to
			// select R13.  The data byte for the overlapping 0x88 write is
			// intentionally the input-side open value 0xFF, matching the
			// real PPI output pin state in this mode.  The final 0x88 write
			// is the PSG-register access counted by the +2 rule.
			io_write(0xF700, 0x82);
			select_ay_register(0x0D);
			io_write(0xF600, 0x08);
			io_write(0xF700, 0x9B);
		}
		dut.ps2_key_i = 0x61C; // PS/2 make A: bit10 toggles, bit9=press.
		run(4);

		trace.clear();
		cclk_count = 0;
		pulse_hsync();
		while (!dut.dma_load_owner_o) {
			if (trace.size() > 20000)
				fail("LOAD owner never asserted after HSYNC");
			tick();
		}

		LoadResult result;
		if (late_overlap) {
			// A is already complete when ownership becomes visible. Advance
			// through B..G, then present the request before ordinary cycle H.
			unsigned cclk_edges = 0;
			while (cclk_edges < 6) {
				TickSample sample = tick();
				if (sample.cclk_p_pre)
					++cclk_edges;
			}
		}
		if (!psg_overlap && overlap_kind != 0) {
			// Start on the first raw clock after DMA ownership is visible;
			// the read is held through the owner window by dma_ppi_wait.
			bus_io(0xF600, 0xFF, true);
			result.overlap = finish_current_io(true);
		} else if (psg_overlap) {
			bus_io(0xF600, 0x88, false);
			result.overlap = finish_current_io(false);
		}
		while (dut.dma_load_busy_o) {
			if (trace.size() > 20000)
				fail("LOAD busy interval did not finish");
			tick();
		}
		run(32);

		result.trace = trace;
		bool saw_start = false;
		uint64_t start = 0;
		uint64_t end = 0;
		const uint8_t restore_reg = psg_overlap ? 0x0D : 0x0E;
		for (const TickSample& sample : trace) {
			if (sample.cclk_p_pre && !sample.busy_pre && sample.busy_post) {
				saw_start = true;
				start = sample.cclk_index;
			}
			if (sample.cclk_p_pre && sample.busy_pre && !sample.busy_post) {
				if (!saw_start)
				fail("LOAD busy interval ended before its start edge");
				end = sample.cclk_index;
				break;
			}
		}
		if (!saw_start)
		fail("LOAD busy start edge was not observed on CCLK_EN_P");
		if (end < start)
		fail("LOAD busy end edge preceded its start edge");
		result.duration = static_cast<unsigned>(end - start + 1);
		for (const TickSample& sample : trace) {
			if (sample.cclk_p_pre && sample.active_post) {
				++result.owner_samples;
				if (sample.dma_bdir_post && sample.dma_bc1_post && sample.dma_di_post == 0x07)
					result.saw_dma_address = true;
				if (sample.dma_bdir_post && !sample.dma_bc1_post && sample.dma_di_post == 0x00)
					result.saw_dma_data = true;
				if (sample.dma_bdir_post && sample.dma_bc1_post &&
				    sample.dma_di_post == restore_reg)
					result.saw_dma_restore = true;
			}
		}
		return result;
	}

	IoResult finish_current_io(bool read) {
		IoResult result;
		result.read = read;
		bool previous_strobe = false;
		for (unsigned guard = 0; guard < 20000; ++guard) {
			TickSample sample = tick();
			if (!sample.wait_pre)
				++result.wait_stalls;
			if (sample.cclk_p_pre && !sample.wait_pre)
				++result.wait_cclk;
			if (sample.dma_wait_pre && sample.ppi_access_pre)
				result.saw_dma_wait = true;
			if (sample.psg_write_pre)
				result.saw_psg_write = true;
			const bool strobe = read ? sample.ppi_oe_pre : sample.ppi_we_pre;
			if (strobe && !previous_strobe) {
				++result.strobe_edges;
				result.data = sample.cpu_di_pre;
				result.accepted_while_owner = sample.owner_pre;
				bus_idle();
				tick();
				return result;
			}
			previous_strobe = strobe;
		}
		fail("overlapping I/O cycle timed out");
	}
};

void require(bool condition, const std::string& message) {
	if (!condition)
		fail(message);
}

void check_common(const LoadResult& result, const char* label) {
	require(result.saw_dma_address, std::string(label) + ": DMA AY address phase not observed");
	require(result.saw_dma_data, std::string(label) + ": DMA AY data phase not observed");
	require(result.saw_dma_restore, std::string(label) + ": DMA CPU AY-register restore phase not observed");
	require(result.owner_samples > 0, std::string(label) + ": no DMA ownership samples observed");
}

void check_post_state(Bench& bench, const LoadResult& result, const char* label,
                   uint8_t expected_selected_reg, uint8_t expected_port_c,
                   bool expected_psg_write_active) {
	if (bench.dut.psg_addr_o != expected_selected_reg)
		fail(std::string(label) + ": selected AY register was not restored");
	if (bench.dut.ppi_port_c_o != expected_port_c)
		fail(std::string(label) + ": Port C state was not preserved/accepted (expected 0x" +
		     std::to_string(expected_port_c) + ", got 0x" +
		     std::to_string(bench.dut.ppi_port_c_o) + ")");
	if (expected_psg_write_active) {
		// The overlap is a real CPU Port-C write (BDIR=1, BC1=0), so its
		// accepted post-DMA state is intentionally active until the script
		// later writes the neutral keyboard-row value.
		if (!bench.dut.psg_bdir_o || bench.dut.psg_bc1_o)
			fail(std::string(label) + ": accepted CPU PSG write did not remain on the AY bus");
	} else if (bench.dut.psg_bdir_o || bench.dut.psg_bc1_o) {
		fail(std::string(label) + ": AY bus remained active after DMA release");
	}
	if (bench.dut.joy1_selected_o != 0 || bench.dut.joy2_selected_o != 0)
		fail(std::string(label) + ": inactive joystick input was not preserved");
	if (bench.dut.key_matrix_o != 0xDF)
		fail(std::string(label) + ": PS/2 A input was not preserved (expected row 8 = 0xDF)");
	IoResult ppi_mode = bench.io_read(0xF700);
	if (ppi_mode.data != 0x9B)
		fail(std::string(label) + ": PPI mode/direction changed (expected 0x9B)");
	// Read AY R14 through the real PPI/YM/HID path.  C=0x48 means BDIR=0,
	// BC1=1, row 8; the resulting byte must contain the pressed A key.
	if (expected_selected_reg != 0x0E) {
		bench.io_write(0xF700, 0x82);
		bench.select_ay_register(0x0E);
		bench.io_write(0xF700, 0x9B);
	}
	bench.io_write(0xF600, 0x48);
	IoResult ay_read = bench.io_read(0xF400);
	if (ay_read.data != 0xDF)
		fail(std::string(label) + ": AY R14 did not return preserved PS/2 row state");
	bench.io_write(0xF600, 0x08);
	(void)result;
}

void check_physical_psg_write_classes(Bench& bench) {
	bench.reset_and_configure();
	bench.io_write(0xF700, 0x82); // Port A and Port C outputs

	// Port A is the PSG data bus. With BDIR/BC1 already at 10, its write is
	// physically a PSG register write even though the CPU address is F4xx.
	bench.io_write(0xF600, 0x88);
	IoResult port_a = bench.io_cycle(0xF400, 0x55, false);
	require(port_a.saw_psg_write,
	        "physical PSG classification missed Port-A write under PC7:6=10");

	// BSR changes to either PC7 or PC6 can themselves enter BDIR/BC1=10.
	bench.io_write(0xF600, 0x08);
	IoResult bsr_pc7 = bench.io_cycle(0xF700, 0x0F, false); // set PC7
	require(bsr_pc7.saw_psg_write,
	        "physical PSG classification missed PC7 BSR transition into 10");
	bench.run(24);

	bench.io_write(0xF600, 0xC8);
	IoResult bsr_pc6 = bench.io_cycle(0xF700, 0x0C, false); // reset PC6
	require(bsr_pc6.saw_psg_write,
	        "physical PSG classification missed PC6 BSR transition into 10");
	std::printf("PASS physical PSG write classes: Port A, full Port C, PC7 BSR and PC6 BSR\n");
}

int run() {
	Bench bench;
	check_physical_psg_write_classes(bench);

	LoadResult base = bench.run_load(0);
	check_common(base, "base");
	if (base.duration != 8)
		fail("base: expected 8 CCLK LOAD cycles, got " + std::to_string(base.duration));
	check_post_state(bench, base, "base", 0x0E, 0x08, false);
	std::printf("PASS base: DMA LOAD occupies 8 CCLK cycles and restores PPI/AY/input state\n");

	LoadResult ppi = bench.run_load(1);
	check_common(ppi, "PPI overlap");
	if (ppi.duration != 9)
		fail("PPI overlap: expected 9 CCLK cycles (8 + 1), got " + std::to_string(ppi.duration));
	if (!ppi.overlap.saw_dma_wait ||
	    ppi.overlap.strobe_edges == 0 || ppi.overlap.accepted_while_owner)
		fail("PPI overlap: WAIT/strobe gating did not hold the PPI read until DMA release");
	if (ppi.overlap.wait_cclk > 8)
		fail("PPI overlap: CPU wait exceeded Arnold's 8us ceiling");
	if (ppi.overlap.data != 0x08)
		fail("PPI overlap: expected held Port C read value 0x08, got 0x" +
		     std::to_string(ppi.overlap.data));
	check_post_state(bench, ppi, "PPI overlap", 0x0E, 0x08, false);
	std::printf("PASS PPI overlap: WAIT and PPI OE hold through LOAD; duration is 8 + 1 CCLK\n");

	LoadResult psg = bench.run_load(2);
	check_common(psg, "PSG overlap");
	if (psg.duration != 10)
		fail("PSG overlap: expected 10 CCLK cycles (8 + 2), got " + std::to_string(psg.duration));
	if (!psg.overlap.saw_dma_wait || !psg.overlap.saw_psg_write ||
	    psg.overlap.strobe_edges == 0 || psg.overlap.accepted_while_owner)
		fail("PSG overlap: WAIT/PSG-write classification or PPI gating failed");
	if (psg.overlap.wait_cclk > 8)
		fail("PSG overlap: CPU wait exceeded Arnold's 8us ceiling");
	check_post_state(bench, psg, "PSG overlap", 0x0D, 0x88, true);
	std::printf("PASS PSG overlap: PSG register write waits behind DMA; duration is 8 + 2 CCLK\n");

	LoadResult late_ppi = bench.run_load(3);
	check_common(late_ppi, "late PPI overlap");
	if (late_ppi.duration != 9 || !late_ppi.overlap.saw_dma_wait ||
	    late_ppi.overlap.accepted_while_owner || late_ppi.overlap.wait_cclk > 8)
		fail("late PPI overlap: ordinary eighth-cycle request did not produce bounded 8 + 1 behavior");
	std::printf("PASS late PPI overlap: eighth-cycle request extends LOAD by 1 without extending ownership\n");

	LoadResult late_psg = bench.run_load(4);
	check_common(late_psg, "late PSG overlap");
	if (late_psg.duration != 10 || !late_psg.overlap.saw_dma_wait ||
	    !late_psg.overlap.saw_psg_write || late_psg.overlap.accepted_while_owner ||
	    late_psg.overlap.wait_cclk > 8)
		fail("late PSG overlap: ordinary eighth-cycle request did not produce bounded 8 + 2 behavior");
	std::printf("PASS late PSG overlap: eighth-cycle request extends LOAD by 2 without extending ownership\n");

	std::printf("p10 DMA/PPI/PSG concurrency checks passed\n");
	return 0;
}

} // namespace

int main() {
	try {
		return run();
	} catch (const TestFailure& error) {
		std::fprintf(stderr, "p10 DMA/PPI/PSG test failed: %s\n", error.what());
		return 1;
	}
}
