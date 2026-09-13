// Directed exact-cycle vectors for the P3 programmable raster interrupt
// (reference §7). Drives asic_ga_timing alone with a synthetic line
// generator: HSYNC_I pulses once per line, crtc_line counts {VC,RC}
// lines, and PRI is driven directly (its &6800 storage is asic_regs').
//
//   pr01  PRI=0 baseline: interrupt period stays exactly 52 lines; the
//         last-ack-was-raster level latches on each raster acknowledge,
//         never on the fire itself (lockstep already pins the full
//         output set; this pins the new export).
//   pr02  PRI=k: counter fires are suppressed; INT_N falls exactly at the
//         shaped-monitor trailing edge following the matching line, at the
//         same intra-line offset every time (self-calibrated on the first
//         fire), and never on line 256+k: bit 8 of the compare is a fixed 0.
//   pr03  vertical adjust gates firing: no interrupt for a match inside
//         adjustment, fire resumes when adj releases.
//   pr04  MRER bit 4 (GA write D[4]) clears a pending raster interrupt.
//   pr05  DCSR bit-7 level persists across its own acknowledge; an empty
//         acknowledge clears it.
//   pr06  a DMA-sourced acknowledge leaves the level clear even when the
//         raster fires afterwards (reference §9: set iff the LAST ack was
//         raster) — the Copter 271 DMA-timer/raster slip.
//
// Expectations are derived from reference §7 / [ARNOLD-REV §2.4] and cited
// inline — never read back out of the simulator.

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "Vasic_ga_timing.h"
#include "verilated.h"

namespace {

class TestFailure : public std::runtime_error {
public:
	explicit TestFailure(const std::string& m) : std::runtime_error(m) {}
};

[[noreturn]] void fail(const std::string& what) { throw TestFailure(what); }

constexpr unsigned kLineClks = 512;   // ticks per synthetic line
constexpr unsigned kHsWidth  = 32;    // CRTC HSYNC_I low pulse width

class PriBench {
public:
	Vasic_ga_timing dut;
	uint64_t cyc = 0;
	unsigned cen_phase = 0;

	bool reset_n = false;
	uint8_t pri = 0;
	uint16_t crtc_line = 0;
	bool adj = false;
	// persistent bus stimulus (tick() applies these verbatim)
	bool iorq_n = true, mreq_n = true, m1_n = true, rd_n = true;
	uint16_t bus_a = 0;
	uint8_t bus_d = 0xFF;
	bool fast = false;
	// synthetic CRTC
	unsigned hcount = 0;
	bool in_hs = true;

	explicit PriBench() : dut("asic_ga_timing") {
		dut.clk = 0;
		dut.pri = 0;
		dut.crtc_line = 0;
		dut.crtc_adj = 0;
	}

	void tick() {
		const bool hs_i = !(in_hs);
		dut.clk = 0;
		dut.cen_16 = (cen_phase == 0);
		dut.fast = fast;
		dut.RESET_N = reset_n;
		dut.A = bus_a;
		dut.D = bus_d;
		dut.MREQ_N = mreq_n ? 1 : 0;
		dut.M1_N = m1_n ? 1 : 0;
		dut.RD_N = rd_n ? 1 : 0;
		dut.IORQ_N = iorq_n ? 1 : 0;
		dut.HSYNC_I = hs_i;
		dut.VSYNC_I = 1;
		dut.pri = pri;
		dut.intack = (!iorq_n && !m1_n) ? 1 : 0;
		// The real input comes from asic_video's VC/RC taps which wrap at
		// their own widths; the bench counter is unbounded, so mask to the
		// 9-bit port here.
		dut.crtc_line = crtc_line & 0x1FF;
		dut.crtc_adj = adj ? 1 : 0;
		dut.eval();
		dut.clk = 1;
		dut.eval();
		++cyc;
		cen_phase = (cen_phase + 1) % 4;

		// Line bookkeeping: HSYNC_I asserted for kHsWidth at line start.
		if (hcount == 0) in_hs = true;
		if (in_hs && hcount >= kHsWidth) in_hs = false;
		hcount = (hcount + 1) % kLineClks;
		if (hcount == 0) ++crtc_line; // line value advances at line start
	}

	void run(unsigned n) { for (unsigned i = 0; i < n; ++i) tick(); }

	void power_on() {
		reset_n = false;
		run(64);
		reset_n = true;
		run(64);
	}

	// MRER bit 4 write through the fast GA-port path clears any interrupt.
	// Z80-style acknowledge: raises a stuck INT and latches the
	// last-ack-was-raster level from the raster request pending at
	// acknowledge start (clears it when nothing is pending).
	void empty_ack() {
		fast = true;
		iorq_n = false;
		m1_n = false;
		for (unsigned i = 0; i < 96; ++i) tick();
		iorq_n = true;
		m1_n = true;
		fast = false;
		run(8);
	}

	void ga_mrer_clear() {
		// fast=1 widens the register-latch window beyond the ring phase
		// (same technique as the differential bench's ga_write).
		fast = true;
		iorq_n = false;
		bus_a = 0x4000 >> 14; // DUT port is only A[15:14]; decode wants 2'b01
		bus_d = 0x90;   // ctrl range, bit4 = irq reset (reference §7)
		for (unsigned i = 0; i < 96; ++i) tick(); // > one full ring (64)
		iorq_n = true;
		fast = false;
		run(8);
	}
};

// Wait for INT_N falling edge; return cycle stamp. Fails on timeout.
uint64_t wait_fire(PriBench& b, const char* who, uint64_t budget) {
	uint64_t guard = 0;
	while (b.dut.INT_N != 0) {
		b.tick();
		if (++guard > budget) fail(std::string(who) + ": INT never fired");
	}
	return b.cyc;
}

//----------------------------------------------------------------------
void pr01_baseline(PriBench& b) {
	b.power_on();
	// INT_N has no reset term and starts stuck low in simulation; the
	// first acknowledge raises it and the second establishes the
	// asserted idle baseline, so both measured fires are genuine
	// events (same discipline as r02).
	b.empty_ack(); // clears the simulator zero-init INT level
	b.empty_ack(); // genuinely idle acknowledge
	if (b.dut.int_last_raster != 0)
		fail("pr01: last-raster level should be zero before any fire");
	// Two consecutive fires must be exactly 52 lines apart (reference §7:
	// PRI=0 keeps the normal Gate Array 52-line counter).
	uint64_t t1 = wait_fire(b, "pr01 first", 120u * kLineClks);
	// No acknowledge since the idle baseline: the fire alone must not
	// set the level (reference §9 — the Copter 271 DMA/raster slip).
	if (b.dut.int_last_raster != 0)
		fail("pr01: fire without acknowledge must not set the level");
	// INT_N holds low until acknowledged: an empty acknowledge raises it.
	// The acknowledge consumes the pending raster interrupt, so the
	// last-ack-was-raster level latches set (pinned by pr05/pr06).
	b.empty_ack();
	if (b.dut.INT_N != 1) fail("pr01: acknowledge did not raise INT_N");
	if (b.dut.int_last_raster != 1)
		fail("pr01: raster acknowledge must set the level");
	uint64_t t2 = wait_fire(b, "pr01 second", 240u * kLineClks);
	uint64_t dt = t2 - t1;
	if (dt != 52u * kLineClks)
		fail("pr01: expected exactly 52-line period (" +
		     std::to_string(52u * kLineClks) + "), got " + std::to_string(dt));
	// The second fire is still pending unacknowledged: the level holds
	// its latched set state until the next acknowledge.
	if (b.dut.int_last_raster != 1)
		fail("pr01: level must hold across a pending fire");
	std::printf("PASS pr01: PRI=0 keeps the exact 52-line cadence; raster level tracks\n");
}

//----------------------------------------------------------------------
// pr02: PRI=k suppresses counter fires and fires at the shaped-monitor
// trailing edge of the matching line. The intra-line fire offset is
// self-calibrated on the first event (the shaping microsequence is
// deterministic), then required to repeat exactly on the next match.
//
// [ARNOLD-REV §2.4] gives the compare as
//   0 PRI7..PRI0 == VC5..VC0 RC2..RC0
// so the ninth bit must be 0 and line 256+k never matches. k=&37 is the
// value Copter 271's title chain programs; 256+&37 = 311 is the last line
// of a 312-line frame, where a don't-care bit 8 fired the sky palette 55
// lines early (MiSTer capture vs AmSpirit, 2026-09-13). On the bench's
// 512-line counter the two consecutive fires must therefore be exactly
// 512 lines apart, with nothing at line 311 between them.
//----------------------------------------------------------------------
void pr02_pri_line(PriBench& b) {
	b.ga_mrer_clear();
	const uint16_t k = 0x37;
	b.pri = uint8_t(k);
	b.run(2); // input settle

	int64_t first_offset = -1;
	unsigned fires = 0;
	uint16_t fire_lines[2];
	uint64_t fire_cyc[2];
	bool prev_int = true;
	uint64_t guard = 0;
	while (fires < 2) {
		const uint16_t line_at_tick = b.crtc_line & 0x1FF;
		b.tick();
		if (++guard > 800u * kLineClks)
			fail("pr02: no PRI interrupt within budget");
		if (prev_int && b.dut.INT_N == 0) {
			fire_lines[fires] = line_at_tick;
			fire_cyc[fires] = b.cyc;
			const int64_t off = int64_t(b.cyc % kLineClks);
			if (fires == 0) first_offset = off;
			else if (off != first_offset)
				fail("pr02: second fire at a different intra-line offset");
			++fires;
			b.ga_mrer_clear(); // acknowledge so the next event is visible
			guard = 0;
		}
		prev_int = b.dut.INT_N == 0;
	}
	for (unsigned i = 0; i < 2; ++i) {
		if (fire_lines[i] != k && fire_lines[i] != k + 1)
			fail("pr02: fire " + std::to_string(i) + " on line " +
			     std::to_string(fire_lines[i]) + ", expected near " +
			     std::to_string(k) + " (256+k must not match)");
	}
	if (fire_cyc[1] - fire_cyc[0] != 512u * kLineClks)
		fail("pr02: consecutive fires " + std::to_string(fire_cyc[1] - fire_cyc[0]) +
		     " ticks apart, expected one 512-line counter period");
	std::printf("PASS pr02: PRI=&37 fires on line %u only, never on line 311\n",
	            (unsigned)fire_lines[0]);
}

//----------------------------------------------------------------------
// pr03: vertical adjust gates firing (reference §7). With adj asserted
// across a matching line there must be no fire; releasing adj lets the
// next matching line fire normally.
//----------------------------------------------------------------------
void pr03_adjustment_gate(PriBench& b) {
	b.ga_mrer_clear();
	// Only lines with bit 8 clear can match ([ARNOLD-REV §2.4]).
	uint16_t k9 = uint16_t((b.crtc_line + 8) & 0x1FF);
	if (k9 & 0x100) k9 = uint16_t(k9 & 0xFF);
	b.pri = uint8_t(k9);
	b.run(2);

	// Forward distance to the match line (mod 512).
	auto dist = [&]() { return uint16_t((uint16_t(k9) - b.crtc_line) & 0x1FF); };

	// Raise adj on the line before the match, hold across the match plus
	// two lines, release (reference section 7: PRI does not trigger during
	// vertical adjust).
	while (dist() != 1) {
		if (b.dut.INT_N == 0)
			fail("pr03: fired before the adjustment window");
		b.tick();
	}
	b.adj = true;
	uint64_t guard = 0;
	while (dist() != 508) {
		if (b.dut.INT_N == 0)
			fail("pr03: fired while vertical adjust was active");
		b.tick();
		if (++guard > 8u * kLineClks)
			fail("pr03: adjustment window never ended");
	}
	b.adj = false;

	// After release the very next matching line must fire. With bit 8 a
	// fixed 0 the bench's 512-line counter matches once per period.
	guard = 0;
	uint16_t lastl = b.crtc_line;
	bool seen_match = false;
	while (b.dut.INT_N != 0) {
		b.tick();
		if (++guard > 600u * kLineClks)
			fail("pr03: no fire after adjustment released");
		if (b.crtc_line != lastl) {
			lastl = b.crtc_line;
			if ((lastl & 0x1FF) == k9) seen_match = true;
		}
	}
	if (!seen_match)
		fail("pr03: released without crossing a matching line");
	std::printf("PASS pr03: no fire during vertical adjust; fires resume after\n");
}

//----------------------------------------------------------------------
// pr04: MRER bit 4 clears a PENDING raster interrupt immediately
// (reference §7) — already exercised between pr02 fires, pinned here as
// an explicit assertion on the PRI-sourced level.
//----------------------------------------------------------------------
void pr04_mrer_clears_pri(PriBench& b) {
	// pr03 left PRI armed; wait for the pending fire then clear it.
	wait_fire(b, "pr04", 800u * kLineClks);
	if (b.dut.INT_N != 0) fail("pr04: INT_N should hold low pending ack");
	b.ga_mrer_clear();
	if (b.dut.INT_N != 1)
		fail("pr04: MRER bit4 must raise INT_N for a PRI interrupt");
	if (b.dut.int_last_raster != 1)
		fail("pr04: MRER is not an acknowledge: the level must persist");
	b.empty_ack();
	if (b.dut.int_last_raster != 0)
		fail("pr04: empty acknowledge must clear the level");
	std::printf("PASS pr04: MRER bit4 clears a PRI raster interrupt\n");
}

//----------------------------------------------------------------------
// pr05: DCSR bit-7 level semantics (reference section 9). The level
// latches at the START of each acknowledge from the raster request
// pending then — clearing it on int_reset instead inverted the rule
// and broke the documented read-DCSR-at-handler-head dispatch
// (review finding 3). A fire alone never sets it (see pr01/pr06).
//----------------------------------------------------------------------
void tick_pub(PriBench& b) { b.tick(); }

void pr05_dcsr_level(PriBench& b) {
	b.ga_mrer_clear();
	wait_fire(b, "pr05", 800u * kLineClks);
	if (b.dut.INT_N != 0) fail("pr05: INT should hold pending ack");
	// Z80-style acknowledge of the pending raster interrupt.
	b.fast = true;
	b.iorq_n = false; b.m1_n = false;
	for (unsigned i = 0; i < 32; ++i) tick_pub(b);
	b.iorq_n = true; b.m1_n = true;
	b.fast = false;
	b.run(8);
	if (b.dut.INT_N != 1) fail("pr05: ack did not raise INT");
	if (b.dut.int_last_raster != 1)
		fail("pr05: last-raster level must persist across its own ack");
	// Empty acknowledge (nothing pending): the level clears.
	b.fast = true;
	b.iorq_n = false; b.m1_n = false;
	for (unsigned i = 0; i < 32; ++i) tick_pub(b);
	b.iorq_n = true; b.m1_n = true;
	b.fast = false;
	b.run(8);
	if (b.dut.int_last_raster != 0)
		fail("pr05: level must clear on an empty acknowledge");
	std::printf("PASS pr05: last-raster level persists across its ack; empty ack clears\n");
}

//----------------------------------------------------------------------
// pr06: a DMA-sourced acknowledge must NOT set the last-ack-was-raster
// level (reference §9 DCSR table: bit 7 is "set if last INT ack was
// raster"). This bench has no DMA engine, but at this module's pins a
// DMA acknowledge appears exactly as an acknowledge cycle with no
// raster pending (INT_N high) — which is what Copter 271's title does:
// its DMA-timer INT is acknowledged, the PRI raster fires a few µs
// later, and the IM1 handler's DCSR read must still see bit 7 = 0 so
// it runs the DMA path. The old set-on-fire term reported 1 here, so
// the handler ran the raster step early; with the request still
// pending the next acknowledge ran the following step immediately,
// reloading the logo palette for the rest of the frame (title flash).
//----------------------------------------------------------------------
void pr06_dma_ack_then_raster(PriBench& b) {
	b.ga_mrer_clear();
	if (b.dut.INT_N != 1) fail("pr06: expected idle INT_N after clear");
	// DMA-only acknowledge: ack cycle, nothing raster-pending.
	b.empty_ack();
	if (b.dut.int_last_raster != 0)
		fail("pr06: DMA acknowledge must leave the level clear");
	// The raster fires AFTER the DMA acknowledge (Copter 271 slip).
	wait_fire(b, "pr06", 800u * kLineClks);
	if (b.dut.int_last_raster != 0)
		fail("pr06: raster fire after a DMA ack must not set the level");
	// Acknowledging the now-pending raster sets it.
	b.empty_ack();
	if (b.dut.INT_N != 1) fail("pr06: ack did not raise INT_N");
	if (b.dut.int_last_raster != 1)
		fail("pr06: raster acknowledge must set the level");
	std::printf("PASS pr06: DMA ack leaves bit7 clear across a later raster fire\n");
}

} // namespace

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	try {
		PriBench b;
		pr01_baseline(b);
		pr02_pri_line(b);
		pr03_adjustment_gate(b);
		pr04_mrer_clears_pri(b);
		pr05_dcsr_level(b);
		pr06_dma_ack_then_raster(b);
	} catch (const TestFailure& e) {
		std::printf("FAIL: %s\n", e.what());
		return 1;
	}
	std::printf("All asic_pri vectors passed\n");
	return 0;
}
