// B20 production-T80 instruction/address acknowledge matrix (test-only).
//
// DUT: the GHDL-synthesized production T80pa netlist (sim/obj_dir/t80/T80pa.v
// via `make -C sim t80-netlist`), driven directly exactly like
// sim/t80_trace_test.cpp does (CEN_p=CEN_n=1, simple CLK). This is the
// production CPU model -- NOT the TV80 substitute used by the p1/p10/p10_dma
// benches (whose wrapper reshapes the acknowledge). No motherboard, no ASIC,
// no private firmware: INT_n, WAIT_n and the vector byte are testbench
// stimulus, so every "request/clear provenance" below is synthetic harness
// provenance, not hardware evidence.
//
// Authority split (read the recommendation critically):
// - Physical claims come from S29 only and are NEVER asserted here: the bug
//   needs A13=0 in the interrupted instruction/address context (S29 p.3),
//   not as a generic address-during-acknowledge condition; single-byte
//   instructions including HALT are fine while memory read/write
//   instructions are affected (S29 p.3); the ASIC sees two acknowledges and
//   the second defaults to DMA0 (S29 pp.3-4). This bench records the
//   interrupted instruction class and origin-selected A13 context of each
//   executed acknowledge next to those S29 expectations, separately from
//   what it asserts.
// - Asserted here are only code-based CPU-model properties, read from the
//   VHDL sources (analysis cites): IORQ_n falls once per accepted interrupt
//   (T80pa.vhd:181-186 shift register, Auto_Wait hold T80.vhd:1338-1341,
//   1352, reset to "11" at T3 :189); IORQ_n and M1_n rise on the same CEN_p
//   edge at TState 2 (T80pa.vhd:159-161, T80.vhd:1287-1289); the vector is
//   latched at that T2 edge while the acknowledge is still valid
//   (T80.vhd:503-506 WZ<=DInst); the core WAIT_n ties to '1'
//   (T80pa.vhd:125) so external WAIT only stretches via CEN_pol
//   (T80pa.vhd:109,172-177); intack has no address term in the model
//   (Amstrad_motherboard.v:479,683). A single intack rise per accepted
//   interrupt is therefore expected in EVERY cell; a second rise would be a
//   new model finding, not hardware proof either way.
//
// Matrix (minimal by brief: memory-vs-NOP/HALT plus IRQ-withdrawal race;
// DD-prefix chain and full OUT/READY coverage deferred):
// - NOP @ A13=0 (0x0100) with a 2-clock WAIT stretch during the acknowledge.
// - NOP @ A13=1 (0x2100), no stretch (A13 control pair).
// - HALT @ A13=0 (S29 "single byte incl. HALT fine" context).
// - LDIR @ A13=0 and @ A13=1 (memory read/write context, S29 affected
//   class). LDIR cells assert INT only after observed LDIR data access (a
//   source-range read or dest-range write), so the acknowledged interrupt
//   truly interrupts LDIR execution; a fixed-tick assert would land in the
//   LD HL/DE/BC setup instead (prior first_A 0x0106/0x2106 was LD BC).
// - OUT (C),C @ A13=0 with 3-clock WAIT stretches on I/O cycles (IOWait /
//   Plus READY shape; WAIT/READY gating per Amstrad_motherboard.v:339).
// - RACE: NOP @ A13=0, INT_n withdrawn K=1..4 clocks after assert.
//   Observational only: no assertion, no invented hardware timing. An
//   acknowledge observed after a pre-boundary withdrawal would be reported
//   as a new finding, never fixed to green.
//
// Per cell the bench captures: INT assert/withdraw ticks, intack rise tick
// (first M1+IORQ rise edge, NOT an M1 fall), IORQ fall/rise ticks, WAIT-low
// ticks inside the acknowledge, A[15:0] sampled during intack (I2 evidence,
// NO assertion), the presented vector (0x06, IM2 table 0x0306 -> handler
// 0x0C00), handler entry, LDIR execution/data evidence (LDIR cells only),
// and intack rise / IORQ-fall-while-M1 counts.
// Race limitation: DI is hard-wired to 0x06 during every intack, so vector
// provenance after an INT withdrawal is untested here -- there is no
// pending-source model and no "empty acknowledge" signal to observe.

#include "VT80pa.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void fail(const std::string& msg) {
	throw std::runtime_error(msg);
}

std::string hx(unsigned v, int w) {
	char b[16];
	std::snprintf(b, sizeof(b), "%0*X", w, v & 0xFFFFFFu);
	return std::string(b);
}

// 64 KiB memory image shared by all cells. Layout:
// 0x0000 init, 0x0100 body (A13=0), 0x2100 body (A13=1), 0x0300 IM2 table,
// 0x0800/0x0900 LDIR buffers, 0x0C00 handler, stack at 0x0FFF.
struct Image {
	std::array<uint8_t, 65536> m;
	Image() { m.fill(0x00); }
	void put(uint16_t a, std::initializer_list<uint8_t> bs) {
		for (uint8_t b : bs) m[a++] = b;
	}
};

void build_image(Image& img, uint16_t origin) {
	// init: SP, I=0x03, IM2, EI, JP origin
	img.put(0x0000, {0x31, 0xFF, 0x0F});
	img.put(0x0003, {0x3E, 0x03});
	img.put(0x0005, {0xED, 0x47});
	img.put(0x0007, {0xED, 0x5E});
	img.put(0x0009, {0xFB});
	img.put(0x000A, {0xC3, uint8_t(origin & 0xFF), uint8_t(origin >> 8)});
	// IM2 table entry for vector 0x06 -> handler 0x0C00
	img.m[0x0306] = 0x00;
	img.m[0x0307] = 0x0C;
	// handler: EI, RETI
	img.put(0x0C00, {0xFB, 0xED, 0x4D});
	// A13=0 body select and A13=1 body select share encodings; the JP
	// origin picks which one executes.
	// NOP loop at both origins: 16x NOP, JR -2 (self)
	for (uint16_t org : {uint16_t(0x0100), uint16_t(0x2100)}) {
		for (int i = 0; i < 16; ++i) img.m[org + i] = 0x00;
		img.m[org + 16] = 0x18;
		img.m[org + 17] = 0xFE;
	}
	// (HALT/LDIR/OUT variants overwrite the 0x0100 or 0x2100 area per cell.)
}

enum class Body { NopLoop, Halt, Ldir, OutLoop };

void place_body(Image& img, uint16_t origin, Body body) {
	switch (body) {
	case Body::NopLoop:
		break; // already in the image
	case Body::Halt:
		img.m[origin + 0] = 0x76; // HALT
		for (int i = 1; i <= 8; ++i) img.m[origin + i] = 0x00;
		img.m[origin + 9] = 0x18;
		img.m[origin + 10] = 0xFE;
		break;
	case Body::Ldir:
		img.put(origin, {0x21, 0x00, 0x08, 0x11, 0x00, 0x09,
		                 0x01, 0x10, 0x00, 0xED, 0xB0, 0x18, 0xF9});
		break; // JR -7 back to LD BC
	case Body::OutLoop:
		img.put(origin, {0x01, 0x00, 0x80, 0xED, 0x49, 0x18, 0xFC});
		break; // LD BC,8000; OUT (C),C; JR -4 to OUT
	}
}

struct CellConfig {
	std::string name;
	uint16_t origin; // code placement: 0x0100 (A13=0) or 0x2100 (A13=1)
	Body body;
	bool ack_wait_stretch; // hold WAIT_n low 2 clocks once intack rises
	bool io_wait_stretch;  // hold WAIT_n low 3 clocks on each I/O cycle
	int int_assert_tick;   // CLK ticks after reset release
	int int_withdraw_tick; // -1: hold until 2 ticks after ack (normal clear)
};

struct CellResult {
	int intack_rises = 0;
	int iorq_falls_while_m1 = 0;
	int intack_low_clocks = 0;
	int wait_lows_during_ack = 0;
	int wait_lows_total = 0;
	int io_cycles_stretched = 0;
	int intack_rise_tick = -1; // first M1+IORQ rise edge (ack start)
	int iorq_fall_tick = -1;
	int iorq_rise_tick = -1;
	int int_assert_tick_actual = -1;
	int int_release_tick = -1;
	int ldir_exec_tick = -1; // first M1 fetch from origin+9/+10 (LDIR cells)
	int ldir_data_tick = -1; // first source/dest-range data access (LDIR cells)
	unsigned ldir_data_addr = 0;
	int handler_entry_tick = -1;
	int fetch_marks_assert_to_end = 0;
	unsigned first_a_during_ack = 0;
	bool saw_a13_0 = false;
	bool saw_a13_1 = false;
	int ack_clocks_sampled = 0;
};

CellResult run_cell(const Image& base, const CellConfig& cfg) {
	Image img = base;
	place_body(img, cfg.origin, cfg.body);

	VT80pa dut;
	dut.RESET_n = 0;
	dut.CLK = 0;
	dut.CEN_p = 1;
	dut.CEN_n = 1;
	dut.WAIT_n = 1;
	dut.INT_n = 1;
	dut.NMI_n = 1;
	dut.BUSRQ_n = 1;
	dut.OUT0 = 0;
	dut.R800_mode = 0;
	dut.DIRSet = 0;
	for (int i = 0; i < 7; i++) dut.DIR[i] = 0;
	dut.eval();

	CellResult r;
	const uint8_t kVector = 0x06; // synthetic ASIC vector byte during intack
	int wait_hold = 0;
	bool in_io = false;
	bool int_asserted = false;
	bool int_released = false;
	bool ack_seen = false;
	int post_ack_ticks = 0;
	bool prev_intack = false;
	bool prev_iorq = true;
	const int kBudget = 4000;

	std::ofstream trace;
	if (std::getenv("B20_TRACE")) {
		std::string name = cfg.name;
		for (char& c : name) {
			if (c == ' ' || c == '(' || c == ')' || c == '+' || c == '=') c = '_';
		}
		const char* trace_dir = std::getenv("B20_TRACE_DIR");
		if (!trace_dir || !*trace_dir) trace_dir = ".";
		std::string trace_path = std::string(trace_dir) + "/b20-matrix-trace-" +
		                         name + "-" + hx(cfg.origin, 4) + ".log";
		trace.open(trace_path);
		if (!trace.is_open())
			fail("B20 matrix " + cfg.name + ": cannot open trace " + trace_path);
		trace << "# rel M1 IORQ MREQ RD WR INT WAIT A DI DO INSN HALT\n";
	}

	const bool await_ldir = (cfg.body == Body::Ldir && cfg.int_withdraw_tick < 0);
	const int kLdirAssertDelay = 8; // ticks after first LDIR data access
	for (int t = 0; t < kBudget; ++t) {
		if (t == 4) dut.RESET_n = 1;
		int rel = t - 4; // ticks since reset release
		if (!await_ldir && rel == cfg.int_assert_tick) {
			dut.INT_n = 0;
			int_asserted = true;
			r.int_assert_tick_actual = rel;
		}
		if (await_ldir && !int_asserted && r.ldir_data_tick >= 0 &&
		    rel >= r.ldir_data_tick + kLdirAssertDelay) {
			dut.INT_n = 0;
			int_asserted = true;
			r.int_assert_tick_actual = rel;
		}
		if (cfg.int_withdraw_tick >= 0 && rel == cfg.int_withdraw_tick && !int_released) {
			dut.INT_n = 1;
			int_released = true;
			r.int_release_tick = rel;
		}

		// Combinational DI for the falling CLK half (as in t80_trace_test,
		// plus the synthetic vector byte while the acknowledge is valid).
		auto eval_bus = [&]() {
			uint16_t a = dut.A;
			if (!dut.M1_n && !dut.IORQ_n) {
				dut.DI = kVector;
			} else if (!dut.MREQ_n && !dut.RD_n) {
				dut.DI = img.m[a];
			} else {
				dut.DI = 0xFF;
			}
		};

		dut.CLK = 0;
		eval_bus();
		dut.eval();

		// WAIT driver for the rising edge: programmed stretches only.
		bool io_cycle = (dut.M1_n && !dut.IORQ_n);
		if (cfg.io_wait_stretch && io_cycle && !in_io) {
			wait_hold = 3;
			++r.io_cycles_stretched;
		}
		in_io = io_cycle;
		bool intack_now = (!dut.M1_n && !dut.IORQ_n);
		(void)intack_now; // acknowledge edges are observed post-rising-edge
		// below; the (b)-phase trigger below only handles I/O cycles.
		dut.WAIT_n = (wait_hold > 0) ? 0 : 1;
		if (wait_hold > 0) {
			--wait_hold;
			++r.wait_lows_total;
		}

		dut.CLK = 1;
		eval_bus();
		dut.eval();

		// Post-edge sampling and memory-write capture.
		uint16_t a = dut.A;
		if (!dut.MREQ_n && !dut.WR_n) img.m[a] = dut.DO;
		if (dut.RESET_n && cfg.body == Body::Ldir) {
			if (r.ldir_exec_tick < 0 && !dut.M1_n && !dut.MREQ_n &&
			    (a == cfg.origin + 9 || a == cfg.origin + 10))
				r.ldir_exec_tick = rel;
			bool src_read = !dut.MREQ_n && !dut.RD_n && a >= 0x0800 && a < 0x0810;
			bool dst_write = !dut.MREQ_n && !dut.WR_n && a >= 0x0900 && a < 0x0910;
			if (r.ldir_data_tick < 0 && (src_read || dst_write)) {
				r.ldir_data_tick = rel;
				r.ldir_data_addr = a;
			}
		}

		bool intack = (!dut.M1_n && !dut.IORQ_n);
		if (!dut.RESET_n) {
			// Verilator initializes outputs to 0, so intack reads true
			// through reset: track levels but count nothing until release.
			prev_intack = intack;
			prev_iorq = dut.IORQ_n;
			continue;
		}
		// The T80 model holds M1/IORQ low for a few clocks while leaving
		// reset (trace rel 1-3: not an acknowledge). Arm observation at INT
		// assert: no acknowledge before it can belong to this interrupt.
		// (Dynamic LDIR cells arm at the observed-execution assert, so a
		// fixed early tick can never silently re-target the setup code.)
		bool armed = int_asserted;
		if (intack && armed) {
			++r.intack_low_clocks;
			if (dut.WAIT_n == 0) ++r.wait_lows_during_ack;
			if (r.ack_clocks_sampled < 64) {
				if (r.ack_clocks_sampled == 0) r.first_a_during_ack = a;
				++r.ack_clocks_sampled;
			}
			if (a & 0x2000) r.saw_a13_1 = true;
			else r.saw_a13_0 = true;
		}
		if (intack && armed && !prev_intack) {
			++r.intack_rises;
			ack_seen = true;
			if (r.intack_rise_tick < 0) r.intack_rise_tick = rel;
			if (cfg.ack_wait_stretch) wait_hold = 2; // external WAIT stretches
			// the pulse from the next edge (code-based); still a single rise.
		}
		if (!dut.IORQ_n && prev_iorq && !dut.M1_n && armed && r.iorq_fall_tick < 0)
			r.iorq_fall_tick = rel;
		if (!dut.IORQ_n && prev_iorq && !dut.M1_n && armed) ++r.iorq_falls_while_m1;
		if (dut.IORQ_n && !prev_iorq && ack_seen && r.iorq_rise_tick < 0)
			r.iorq_rise_tick = rel;
		prev_intack = intack;
		prev_iorq = dut.IORQ_n;

		if (dut.INSN_START && int_asserted && !ack_seen) ++r.fetch_marks_assert_to_end;

		// Normal clear provenance (synthetic GA): release INT 2 ticks after
		// the acknowledge rise, then keep running into the handler.
		if (cfg.int_withdraw_tick < 0 && ack_seen && !int_released &&
		    rel >= r.intack_rise_tick + 2) {
			dut.INT_n = 1;
			int_released = true;
			r.int_release_tick = rel;
		}

		// Handler entry: instruction fetch from 0x0C00.
		if (!dut.M1_n && !dut.MREQ_n && a == 0x0C00 && r.handler_entry_tick < 0)
			r.handler_entry_tick = rel;

		if (trace.is_open() && dut.RESET_n)
			trace << rel << " " << (int)dut.M1_n << " " << (int)dut.IORQ_n << " "
			      << (int)dut.MREQ_n << " " << (int)dut.RD_n << " " << (int)dut.WR_n
			      << " " << (int)dut.INT_n << " " << (int)dut.WAIT_n << " " << hx(a, 4)
			      << " " << hx(dut.DI, 2) << " " << hx(dut.DO, 2) << " "
			      << (int)dut.INSN_START << " " << (int)dut.HALT_n << "\n";

		if (ack_seen) {
			if (++post_ack_ticks > 150) break;
		}
		if (!ack_seen && cfg.int_withdraw_tick >= 0 && rel > cfg.int_withdraw_tick + 600)
			break; // race with no ack: bounded stop, observational
		if (await_ldir && !int_asserted && rel > 1500)
			fail("B20 matrix " + cfg.name + ": no LDIR execution observed within "
			     "1500 ticks (prerequisite: cell must execute LDIR before INT assert)");
		if (int_asserted && !ack_seen && cfg.int_withdraw_tick < 0 &&
		    rel > r.int_assert_tick_actual + 1500)
			fail("B20 matrix " + cfg.name + ": no acknowledge within 1500 ticks of INT assert");
	}
	return r;
}

void check_single_ack(const CellConfig& cfg, const CellResult& r) {
	// Code-based expectations only (T80pa.vhd / T80.vhd cites above).
	if (cfg.body == Body::Ldir) {
		if (r.ldir_data_tick < 0)
			fail("B20 matrix " + cfg.name + ": no LDIR data access observed -- "
			     "cell stopped being LDIR, refusing a setup-interrupt pass");
		if (r.intack_rise_tick < 0 || r.intack_rise_tick <= r.ldir_data_tick)
			fail("B20 matrix " + cfg.name + ": acknowledge did not follow LDIR "
			     "execution (data@" + std::to_string(r.ldir_data_tick) +
			     " ack@" + std::to_string(r.intack_rise_tick) + ")");
	}
	if (r.intack_rises != 1)
		fail("B20 matrix " + cfg.name + ": intack rises=" + std::to_string(r.intack_rises) +
		     ", expected exactly 1 per accepted interrupt (code-based; behaviour moved)");
	if (r.iorq_falls_while_m1 != 1)
		fail("B20 matrix " + cfg.name + ": IORQ falls while M1=" +
		     std::to_string(r.iorq_falls_while_m1) + ", expected 1 (T80pa.vhd:181-186)");
	if (r.handler_entry_tick < 0)
		fail("B20 matrix " + cfg.name + ": CPU never fetched handler 0x0C00 "
		     "(vector 0x06 not captured via IM2 table)");
}

void report_cell(const CellConfig& cfg, const CellResult& r, bool race) {
	int assert_tick = (r.int_assert_tick_actual >= 0) ? r.int_assert_tick_actual
	                                                  : cfg.int_assert_tick;
	std::printf("B20 matrix %-16s origin=0x%s A13=%d intack_rises=%d iorq_falls_m1=%d "
	            "ack_clocks=%d wait_ack=%d wait_total=%d io_stretched=%d first_A=0x%s "
	            "A13seen=%s%s handler=%s intackrise@%d assert@%d release@%d%s%s\n",
	            cfg.name.c_str(), hx(cfg.origin, 4).c_str(), (cfg.origin & 0x2000) ? 1 : 0,
	            r.intack_rises, r.iorq_falls_while_m1, r.intack_low_clocks,
	            r.wait_lows_during_ack, r.wait_lows_total, r.io_cycles_stretched,
	            hx(r.first_a_during_ack, 4).c_str(),
	            r.saw_a13_0 ? "0" : "-", r.saw_a13_1 ? "1" : "-",
	            r.handler_entry_tick >= 0
	                ? ("@+" + std::to_string(r.handler_entry_tick - assert_tick)).c_str()
	                : "none",
	            r.intack_rise_tick, assert_tick, r.int_release_tick,
	            race ? " (race: observational, no assert)" : "",
	            (cfg.body == Body::Ldir)
	                ? (" ldir_exec@" + std::to_string(r.ldir_exec_tick) + " data@" +
	                   std::to_string(r.ldir_data_tick) + "=0x" + hx(r.ldir_data_addr, 4))
	                      .c_str()
	                : "");
}

} // namespace

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	try {
		const std::vector<CellConfig> cells = {
			{"NOP", 0x0100, Body::NopLoop, true, false, 120, -1},
			{"NOP", 0x2100, Body::NopLoop, false, false, 120, -1},
			{"HALT", 0x0100, Body::Halt, false, false, 120, -1},
			{"LDIR", 0x0100, Body::Ldir, false, false, 120, -1},
			{"LDIR", 0x2100, Body::Ldir, false, false, 120, -1},
			{"OUT(C)+WAIT", 0x0100, Body::OutLoop, false, true, 120, -1},
		};
		for (const CellConfig& cfg : cells) {
			Image img;
			build_image(img, cfg.origin);
			CellResult r = run_cell(img, cfg);
			check_single_ack(cfg, r);
			std::string tag = cfg.name + " A13=" +
			                  std::string((cfg.origin & 0x2000) ? "1" : "0");
			CellConfig named = cfg;
			named.name = tag;
			report_cell(named, r, false);
		}
		// IRQ-withdrawal race sweep: observational, separate expectations.
		// NOTE on INSN_START: the trace shows it marks opcode fetches, and an
		// accepted interrupt replaces the next fetch with the acknowledge, so
		// zero marks between assert and ack is the NORMAL shape here -- it is
		// not an acknowledge signal either way. What the sweep discriminates
		// is cruder: withdraw-before-sample (no ack) vs withdraw-after-sample
		// (single ack), with zero double acknowledges as the finding. There
		// is deliberately NO empty-acknowledge claim: DI is hard-wired to
		// 0x06 during every intack, so vector provenance after a withdrawal
		// is untested -- this CPU-only harness has no pending-source model.
		for (int k : {1, 2, 3, 4}) {
			CellConfig cfg{"RACE-K" + std::to_string(k), 0x0100, Body::NopLoop,
			               false, false, 120, 120 + k};
			Image img;
			build_image(img, cfg.origin);
			CellResult r = run_cell(img, cfg);
			if (r.intack_rises > 1)
				fail("B20 matrix " + cfg.name + ": double acknowledge in race cell "
				     "(new model finding; never fixed to green)");
			std::printf("B20 matrix %-16s intack_rises=%d fetch_marks=%d "
			            "handler=%s first_A=0x%s withdraw@%d release@%d ack@%d "
			            "(observational: %s; vector always 0x06 while intack, provenance untested)\n",
			            cfg.name.c_str(), r.intack_rises,
			            r.fetch_marks_assert_to_end,
			            r.handler_entry_tick >= 0 ? "entered" : "none",
			            hx(r.first_a_during_ack, 4).c_str(),
			            cfg.int_withdraw_tick, r.int_release_tick, r.intack_rise_tick,
			            r.intack_rises == 0 ? "withdraw won, no ack"
			                                : "sampled before withdraw, single ack");
		}
	} catch (const std::exception& e) {
		std::fprintf(stderr, "FAIL: %s\n", e.what());
		return 1;
	}
	std::printf("All B20 production-T80 matrix cells executed: one intack rise per accepted "
	            "interrupt in every non-race cell (code-based); A[15:0] during intack recorded "
	            "as I2 evidence with no assertion; S29 physical expectations kept separate.\n");
	return 0;
}
