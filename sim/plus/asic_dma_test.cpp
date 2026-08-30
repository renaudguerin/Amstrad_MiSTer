// Deterministic unit tests for rtl/plus/asic_dma.v (Phase P7)
//
// Expectations derived from docs/plus/references/asic-reference.md §9 and
// _Arnold V_ Specification - Issue 1.5 §2.6 / §2.7.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Vasic_dma.h"
#include "verilated.h"

namespace {

class TestFailure : public std::runtime_error {
public:
	explicit TestFailure(const std::string& msg) : std::runtime_error(msg) {}
};

[[noreturn]] void fail(const std::string& what) {
	throw TestFailure(what);
}

struct TestBench {
	std::unique_ptr<Vasic_dma> dut;
	uint64_t ticks = 0;
	std::vector<uint16_t> ram; // 64K words

	TestBench() : dut(std::make_unique<Vasic_dma>("asic_dma")), ram(65536, 0) {
		dut->clk = 0;
		dut->reset = 1;
		dut->cclk_en_p = 0;
		dut->cclk_en_n = 0;
		dut->hsync = 0;

		dut->sar0_lo = 0; dut->sar0_hi = 0; dut->ppr0 = 0; dut->sar0_wr = 0;
		dut->sar1_lo = 0; dut->sar1_hi = 0; dut->ppr1 = 0; dut->sar1_wr = 0;
		dut->sar2_lo = 0; dut->sar2_hi = 0; dut->ppr2 = 0; dut->sar2_wr = 0;
		dut->dcsr_ena = 0;
		dut->ram_data = 0;
	}

	void step_clock() {
		dut->clk = 0;
		dut->eval();

		// Drive RAM read data based on ram_req and ram_addr
		if (dut->ram_req) {
			uint16_t word_idx = (dut->ram_addr >> 1);
			dut->ram_data = ram[word_idx];
		}

		dut->clk = 1;
		dut->eval();
		ticks++;
	}

	void pulse_reset() {
		dut->reset = 1;
		dut->sar0_lo = 0; dut->sar0_hi = 0; dut->ppr0 = 0; dut->sar0_wr = 0;
		dut->sar1_lo = 0; dut->sar1_hi = 0; dut->ppr1 = 0; dut->sar1_wr = 0;
		dut->sar2_lo = 0; dut->sar2_hi = 0; dut->ppr2 = 0; dut->sar2_wr = 0;
		dut->dcsr_ena = 0;
		for (int i = 0; i < 8; ++i) step_clock();
		dut->reset = 0;
		step_clock();
	}

	void set_sar(unsigned ch, uint16_t addr) {
		if (ch == 0) {
			dut->sar0_lo = addr & 0xFF;
			dut->sar0_hi = (addr >> 8) & 0xFF;
			dut->sar0_wr = 1;
			step_clock();
			dut->sar0_wr = 0;
		} else if (ch == 1) {
			dut->sar1_lo = addr & 0xFF;
			dut->sar1_hi = (addr >> 8) & 0xFF;
			dut->sar1_wr = 1;
			step_clock();
			dut->sar1_wr = 0;
		} else if (ch == 2) {
			dut->sar2_lo = addr & 0xFF;
			dut->sar2_hi = (addr >> 8) & 0xFF;
			dut->sar2_wr = 1;
			step_clock();
			dut->sar2_wr = 0;
		}
	}

	void set_ppr(unsigned ch, uint8_t val) {
		if (ch == 0) dut->ppr0 = val;
		else if (ch == 1) dut->ppr1 = val;
		else if (ch == 2) dut->ppr2 = val;
	}

	void set_dcsr_ena(uint8_t val) {
		dut->dcsr_ena = val & 0x07;
	}

	void write_ram_word(uint16_t byte_addr, uint16_t val) {
		ram[byte_addr >> 1] = val;
	}

	// Helper to write a Z80 little-endian instruction into RAM:
	// In Z80 format: byte 0 is D[7:0], byte 1 is D[15:8]
	void write_instruction(uint16_t byte_addr, uint16_t instr) {
		// instr is high_byte << 8 | low_byte
		ram[byte_addr >> 1] = instr;
	}

	uint8_t last_ena_clr = 0;
	uint8_t last_int_set = 0;

	// Simulate one scanline: pulse HSYNC, then generate 64 1us CCLK pulses (64us standard scanline).
	// If requested, record the CCLK indices at which the DMA RAM request is active.
	void run_scanline(std::vector<std::pair<uint8_t, uint8_t>>* psg_writes = nullptr,
	                  std::vector<int>* dma_fetch_cycles = nullptr) {
		last_ena_clr = 0;
		last_int_set = 0;
		if (dma_fetch_cycles) dma_fetch_cycles->clear();

		// HSYNC leading edge
		dut->hsync = 1;
		step_clock();

		// Track PSG writes during scanline
		uint8_t last_psg_addr = 0xFF;

		// 64 CRTC clock cycles (4 MHz clock enables = 16 master clocks per 1us)
		for (int cyc = 0; cyc < 64; ++cyc) {
			// Lower HSYNC after 4us
			if (cyc == 4) dut->hsync = 0;

			for (int clk_phase = 0; clk_phase < 16; ++clk_phase) {
				dut->cclk_en_p = (clk_phase == 0);
				dut->cclk_en_n = (clk_phase == 8);

				if (dut->cclk_en_p && dut->ram_req && dma_fetch_cycles)
					dma_fetch_cycles->push_back(cyc);

				if (dut->dcsr_ena_clr) {
					last_ena_clr |= dut->dcsr_ena_clr;
					dut->dcsr_ena &= ~dut->dcsr_ena_clr;
				}
				if (dut->dma_int_set) {
					last_int_set |= dut->dma_int_set;
				}

				if (dut->cclk_en_p && dut->psg_active) {
					// PSG address latch: BDIR=1, BC1=1
					if (dut->psg_bdir && dut->psg_bc1) {
						last_psg_addr = dut->psg_dout;
					}
					// PSG write data: BDIR=1, BC1=0
					else if (dut->psg_bdir && !dut->psg_bc1 && last_psg_addr != 0xFF) {
						if (psg_writes) {
							psg_writes->push_back({last_psg_addr, dut->psg_dout});
						}
					}
				}

				step_clock();
			}
		}
		dut->cclk_en_p = 0;
		dut->cclk_en_n = 0;
	}
};

// d01: Reset and default state
void test_d01_reset_and_defaults(TestBench& tb) {
	tb.pulse_reset();
	if (tb.dut->sar0_addr != 0 || tb.dut->sar1_addr != 0 || tb.dut->sar2_addr != 0)
		fail("d01: SAR addresses not 0 after reset");
	if (tb.dut->psg_active != 0 || tb.dut->ram_req != 0)
		fail("d01: Bus active after reset");
	std::printf("PASS d01: Reset and default state\n");
}

// d02: LOAD R, DD instruction (&0RDD)
void test_d02_load_instruction(TestBench& tb) {
	tb.pulse_reset();
	tb.set_sar(0, 0x1000);
	tb.set_dcsr_ena(1); // enable ch0

	// Write LOAD R7, 0x42 (&0742) at 0x1000
	tb.write_instruction(0x1000, 0x0742);

	std::vector<std::pair<uint8_t, uint8_t>> psg_writes;
	tb.run_scanline(&psg_writes);

	if (psg_writes.size() != 1)
		fail("d02: Expected 1 PSG write, got " + std::to_string(psg_writes.size()));
	if (psg_writes[0].first != 7 || psg_writes[0].second != 0x42)
		fail("d02: Incorrect PSG write: R=" + std::to_string(psg_writes[0].first) +
		     " D=" + std::to_string(psg_writes[0].second));
	if (tb.dut->sar0_addr != 0x1002)
		fail("d02: SAR0 did not advance to 0x1002");
	std::printf("PASS d02: LOAD R, DD instruction\n");
}

// d03: PAUSE N instruction (&1NNN) and PPR prescaler
void test_d03_pause_and_prescaler(TestBench& tb) {
	tb.pulse_reset();
	tb.set_sar(0, 0x1000);
	tb.set_ppr(0, 2); // 3 lines per pause tick
	tb.set_dcsr_ena(1);

	// RAM 0x1000: PAUSE 2 (&1002) -> 2 ticks * 3 lines = 6 scanlines pause
	// RAM 0x1002: LOAD R0, 0x55 (&0055)
	tb.write_instruction(0x1000, 0x1002);
	tb.write_instruction(0x1002, 0x0055);

	std::vector<std::pair<uint8_t, uint8_t>> writes;
	// Line 1: Executes PAUSE 2
	tb.run_scanline(&writes);
	if (!writes.empty()) fail("d03: Unexpected write on line 1");
	if (tb.dut->sar0_addr != 0x1002) fail("d03: SAR0 not pointing to next instruction");

	// Lines 2..7 (6 lines of countdown): Should be idle
	for (int l = 2; l <= 7; ++l) {
		tb.run_scanline(&writes);
		if (!writes.empty()) fail("d03: Unexpected write while pausing on line " + std::to_string(l));
		if (tb.dut->sar0_addr != 0x1002) fail("d03: SAR0 changed while pausing");
	}

	// Line 8: Pause finished, executes LOAD R0, 0x55
	tb.run_scanline(&writes);
	if (writes.size() != 1) fail("d03: Expected LOAD on line 8");
	if (writes[0].first != 0 || writes[0].second != 0x55) fail("d03: Incorrect write on line 8");
	if (tb.dut->sar0_addr != 0x1004) fail("d03: SAR0 did not advance after LOAD");

	std::printf("PASS d03: PAUSE N and PPR prescaler countdown\n");
}

// d04: REPEAT N and LOOP instruction (&2NNN / &4001)
void test_d04_repeat_and_loop(TestBench& tb) {
	tb.pulse_reset();
	tb.set_sar(0, 0x2000);
	tb.set_dcsr_ena(1);

	// RAM 0x2000: REPEAT 2 (&2002) -> Body executes 3 times (initial + 2 loops)
	// RAM 0x2002: LOAD R1, 0xAA (&01AA)
	// RAM 0x2004: LOOP (&4001)
	// RAM 0x2006: STOP (&4020)
	tb.write_instruction(0x2000, 0x2002);
	tb.write_instruction(0x2002, 0x01AA);
	tb.write_instruction(0x2004, 0x4001);
	tb.write_instruction(0x2006, 0x4020);

	std::vector<std::pair<uint8_t, uint8_t>> writes;

	// Line 1: Executes REPEAT 2
	tb.run_scanline(&writes);
	if (tb.dut->sar0_addr != 0x2002) fail("d04: SAR0 after REPEAT wrong");

	// Line 2: Iteration 1 - LOAD R1, 0xAA
	tb.run_scanline(&writes);
	if (writes.size() != 1 || writes.back().second != 0xAA) fail("d04: Iteration 1 LOAD failed");
	if (tb.dut->sar0_addr != 0x2004) fail("d04: SAR0 after Iteration 1 LOAD wrong");

	// Line 3: LOOP 1 - Jumps back to 0x2002
	tb.run_scanline(&writes);
	if (tb.dut->sar0_addr != 0x2002) fail("d04: LOOP 1 jump failed");

	// Line 4: Iteration 2 - LOAD R1, 0xAA
	tb.run_scanline(&writes);
	if (writes.size() != 2 || writes.back().second != 0xAA) fail("d04: Iteration 2 LOAD failed");

	// Line 5: LOOP 2 - Jumps back to 0x2002
	tb.run_scanline(&writes);
	if (tb.dut->sar0_addr != 0x2002) fail("d04: LOOP 2 jump failed");

	// Line 6: Iteration 3 - LOAD R1, 0xAA
	tb.run_scanline(&writes);
	if (writes.size() != 3 || writes.back().second != 0xAA) fail("d04: Iteration 3 LOAD failed");

	// Line 7: LOOP 3 - Loop finished, falls through to 0x2006
	tb.run_scanline(&writes);
	if (tb.dut->sar0_addr != 0x2006) fail("d04: Loop exit failed, expected 0x2006");

	// Line 8: STOP - Clears enable, SAR0 stays 0x2008
	tb.run_scanline(&writes);
	if (tb.last_ena_clr != 1) fail("d04: STOP did not assert dcsr_ena_clr");
	if (tb.dut->sar0_addr != 0x2008) fail("d04: SAR0 after STOP not 0x2008");

	std::printf("PASS d04: REPEAT N and LOOP iteration execution\n");
}

// d05: INT instruction (&4010)
void test_d05_int_instruction(TestBench& tb) {
	tb.pulse_reset();
	tb.set_sar(1, 0x3000);
	tb.set_dcsr_ena(2); // enable ch1

	// RAM 0x3000: INT (&4010)
	tb.write_instruction(0x3000, 0x4010);

	tb.run_scanline();
	if (tb.last_int_set != 2) // bit 1 for ch1
		fail("d05: INT instruction did not assert dma_int_set[1]");
	if (tb.dut->sar1_addr != 0x3002)
		fail("d05: SAR1 did not advance after INT");
	std::printf("PASS d05: INT instruction\n");
}

// d06: STOP instruction (&4020)
void test_d06_stop_instruction(TestBench& tb) {
	tb.pulse_reset();
	tb.set_sar(2, 0x4000);
	tb.set_dcsr_ena(4); // enable ch2

	// RAM 0x4000: STOP (&4020)
	tb.write_instruction(0x4000, 0x4020);

	tb.run_scanline();
	if (tb.last_ena_clr != 4) // bit 2 for ch2
		fail("d06: STOP instruction did not assert dcsr_ena_clr[2]");
	if (tb.dut->sar2_addr != 0x4002)
		fail("d06: SAR2 did not point to next instruction");
	std::printf("PASS d06: STOP instruction\n");
}

// d07: INT + STOP compound instruction (&4030)
void test_d07_compound_int_stop(TestBench& tb) {
	tb.pulse_reset();
	tb.set_sar(0, 0x5000);
	tb.set_dcsr_ena(1);

	// RAM 0x5000: INT | STOP (&4030)
	tb.write_instruction(0x5000, 0x4030);

	tb.run_scanline();
	if (tb.last_int_set != 1) fail("d07: INT flag not asserted in &4030");
	if (tb.last_ena_clr != 1) fail("d07: STOP clear not asserted in &4030");
	std::printf("PASS d07: INT | STOP compound instruction\n");
}

// d08: 3-channel interleave priority order
void test_d08_multi_channel_interleave(TestBench& tb) {
	tb.pulse_reset();
	tb.set_sar(0, 0x6000);
	tb.set_sar(1, 0x7000);
	tb.set_sar(2, 0x8000);
	tb.set_dcsr_ena(7); // all 3 channels enabled

	tb.write_instruction(0x6000, 0x0011); // ch0: LOAD R0, 0x11
	tb.write_instruction(0x7000, 0x0122); // ch1: LOAD R1, 0x22
	tb.write_instruction(0x8000, 0x0233); // ch2: LOAD R2, 0x33

	std::vector<std::pair<uint8_t, uint8_t>> writes;
	tb.run_scanline(&writes);

	if (writes.size() != 3)
		fail("d08: Expected 3 writes, got " + std::to_string(writes.size()));
	if (writes[0].first != 0 || writes[0].second != 0x11)
		fail("d08: Ch0 write incorrect");
	if (writes[1].first != 1 || writes[1].second != 0x22)
		fail("d08: Ch1 write incorrect");
	if (writes[2].first != 2 || writes[2].second != 0x33)
		fail("d08: Ch2 write incorrect");

	std::printf("PASS d08: 3-channel interleave execution order\n");
}

// d09: Undocumented &3xxx (PAUSE then REPEAT)
void test_d09_undocumented_pause_repeat(TestBench& tb) {
	tb.pulse_reset();
	tb.set_sar(0, 0x9000);
	tb.set_ppr(0, 0); // 1 line per tick
	tb.set_dcsr_ena(1);

	// RAM 0x9000: &3001 (PAUSE 1, REPEAT 1)
	// RAM 0x9002: LOAD R0, 0x99
	// RAM 0x9004: LOOP (&4001)
	tb.write_instruction(0x9000, 0x3001);
	tb.write_instruction(0x9002, 0x0099);
	tb.write_instruction(0x9004, 0x4001);

	std::vector<std::pair<uint8_t, uint8_t>> writes;
	// Line 1: executes &3001
	tb.run_scanline(&writes);
	// Line 2: paused (1 scanline pause countdown)
	tb.run_scanline(&writes);
	if (!writes.empty()) fail("d09: Unexpected write during pause");

	// Line 3: resumes, executes LOAD R0, 0x99
	tb.run_scanline(&writes);
	if (writes.size() != 1 || writes[0].second != 0x99) fail("d09: LOAD after pause failed");

	// Line 4: executes LOOP, loops back to 0x9002
	tb.run_scanline(&writes);
	if (tb.dut->sar0_addr != 0x9002) fail("d09: LOOP after &3xxx failed");

	std::printf("PASS d09: Undocumented &3xxx (PAUSE then REPEAT)\n");
}

// d10: Sequential 8-bit SAR byte writes
void test_d10_byte_sar_writes(TestBench& tb) {
	tb.pulse_reset();
	// Write low byte 0x78 first
	tb.dut->sar0_lo = 0x78;
	tb.dut->sar0_wr = 1;
	tb.step_clock();
	tb.dut->sar0_wr = 0;
	tb.step_clock();
	if (tb.dut->sar0_addr != 0x0078)
		fail("d10: Low byte write did not update SAR0 to 0x0078 (got 0x" + std::to_string(tb.dut->sar0_addr) + ")");

	// Write high byte 0x56 second
	tb.dut->sar0_hi = 0x56;
	tb.dut->sar0_wr = 1;
	tb.step_clock();
	tb.dut->sar0_wr = 0;
	tb.step_clock();
	if (tb.dut->sar0_addr != 0x5678)
		fail("d10: High byte write did not update SAR0 to 0x5678 (got 0x" + std::to_string(tb.dut->sar0_addr) + ")");

	std::printf("PASS d10: Sequential 8-bit SAR writes\n");
}

// d11: LOAD timing (8 cycles), AY register restore, and dma_load_owner assertion
void test_d11_load_timing_and_ay_restore(TestBench& tb) {
	tb.pulse_reset();
	tb.dut->cpu_psg_addr = 0x0E; // CPU currently selected register 14 (keyboard row)
	tb.set_sar(0, 0x2000);
	tb.set_dcsr_ena(1); // enable ch0

	// Write LOAD R7, 0x3F (&073F) at 0x2000
	tb.write_instruction(0x2000, 0x073F);

	// Start scanline
	tb.dut->hsync = 1;
	tb.step_clock();

	// Dead cycle (cyc 0)
	auto advance_cclk = [&]() {
		for (int clk_phase = 0; clk_phase < 16; ++clk_phase) {
			tb.dut->cclk_en_p = (clk_phase == 0);
			tb.dut->cclk_en_n = (clk_phase == 8);
			tb.step_clock();
		}
		tb.dut->cclk_en_p = 0;
		tb.dut->cclk_en_n = 0;
	};

	// Cycle 0: Dead cycle
	advance_cclk();
	// Cycle 1: Fetch the sole active channel 0.  Inactive channel slots are
	// not present in the documented per-active-channel cadence.
	advance_cclk();

	// Now Channel 0 executes LOAD R7, 0x3F (8 cycles total: ST_EXEC0_A..ST_EXEC0_H)
	// Substep 0: Address set (R7)
	advance_cclk();
	if (!tb.dut->dma_load_owner || !tb.dut->psg_active || !tb.dut->psg_bdir || !tb.dut->psg_bc1 || tb.dut->psg_dout != 0x07)
		fail("d11: Substep 0 failed (expected R7 address select with dma_load_owner=1)");

	// Substep 1: Inactive sep
	advance_cclk();
	if (!tb.dut->dma_load_owner || !tb.dut->psg_active || tb.dut->psg_bdir || tb.dut->psg_bc1)
		fail("d11: Substep 1 failed (expected inactive separation)");

	// Substep 2: Data write (0x3F)
	advance_cclk();
	if (!tb.dut->dma_load_owner || !tb.dut->psg_active || !tb.dut->psg_bdir || tb.dut->psg_bc1 || tb.dut->psg_dout != 0x3F)
		fail("d11: Substep 2 failed (expected data write 0x3F)");

	// Substep 3: Inactive sep
	advance_cclk();
	if (!tb.dut->dma_load_owner || !tb.dut->psg_active || tb.dut->psg_bdir || tb.dut->psg_bc1)
		fail("d11: Substep 3 failed (expected inactive separation)");

	// Substep 4: Restore CPU AY register (0x0E)
	advance_cclk();
	if (!tb.dut->dma_load_owner || !tb.dut->psg_active || !tb.dut->psg_bdir || !tb.dut->psg_bc1 || tb.dut->psg_dout != 0x0E)
		fail("d11: Substep 4 failed (expected CPU AY register 14 restore)");

	// Substep 5: Inactive sep
	advance_cclk();
	if (!tb.dut->dma_load_owner || !tb.dut->psg_active || tb.dut->psg_bdir || tb.dut->psg_bc1)
		fail("d11: Substep 5 failed (expected inactive separation)");

	// Substep 6: Hold
	advance_cclk();
	if (!tb.dut->dma_load_owner || !tb.dut->psg_active)
		fail("d11: Substep 6 failed (expected dma_load_owner held)");

	// Substep 7: Release
	advance_cclk();
	if (tb.dut->dma_load_owner || tb.dut->psg_active)
		fail("d11: Substep 7 failed (expected release of dma_load_owner and psg_active)");

	std::printf("PASS d11: LOAD timing (8 cycles), AY register restore, and dma_load_owner assertion\n");
}

// d12: Per-active-channel RAM fetch cadence (§9, Timing & bus interaction)
void test_d12_active_channel_fetch_timing(TestBench& tb) {
	const uint16_t sar_start[3] = {0x6000, 0x7000, 0x8000};
	const uint16_t load_instr[3] = {0x0011, 0x0122, 0x0233};

	for (unsigned mask = 0; mask < 8; ++mask) {
		tb.pulse_reset();
		for (unsigned ch = 0; ch < 3; ++ch) {
			tb.set_sar(ch, sar_start[ch]);
			tb.write_instruction(sar_start[ch], load_instr[ch]);
		}
		tb.set_dcsr_ena(static_cast<uint8_t>(mask));

		std::vector<int> fetch_cycles;
		tb.run_scanline(nullptr, &fetch_cycles);

		const unsigned expected_count =
			((mask & 0x1u) ? 1u : 0u) +
			((mask & 0x2u) ? 1u : 0u) +
			((mask & 0x4u) ? 1u : 0u);
		if (fetch_cycles.size() != expected_count) {
			fail("d12: mask 0x" + std::to_string(mask) +
			     " expected " + std::to_string(expected_count) +
			     " fetch cycles, got " + std::to_string(fetch_cycles.size()));
		}

		// HSYNC's CCLK edge is cycle 0 (the one dead cycle).  Every active
		// channel must then occupy one consecutive fetch cycle, with no holes
		// for inactive lower-numbered channels.
		for (unsigned i = 0; i < expected_count; ++i) {
			if (fetch_cycles[i] != static_cast<int>(i + 1)) {
				fail("d12: mask 0x" + std::to_string(mask) +
				     " fetch " + std::to_string(i) +
				     " occurred at CCLK " + std::to_string(fetch_cycles[i]) +
				     ", expected " + std::to_string(i + 1));
			}
		}
	}

	std::printf("PASS d12: one consecutive fetch cycle per active channel (0/1/2/3 active)\n");
}

} // namespace

int main() {
	try {
		TestBench tb;
		test_d01_reset_and_defaults(tb);
		test_d02_load_instruction(tb);
		test_d03_pause_and_prescaler(tb);
		test_d04_repeat_and_loop(tb);
		test_d05_int_instruction(tb);
		test_d06_stop_instruction(tb);
		test_d07_compound_int_stop(tb);
		test_d08_multi_channel_interleave(tb);
		test_d09_undocumented_pause_repeat(tb);
		test_d10_byte_sar_writes(tb);
		test_d11_load_timing_and_ay_restore(tb);
		test_d12_active_channel_fetch_timing(tb);
		std::printf("All 12 asic_dma unit tests PASSED.\n");
		return 0;
	} catch (const std::exception& e) {
		std::fprintf(stderr, "FAIL: %s\n", e.what());
		return 1;
	}
}
