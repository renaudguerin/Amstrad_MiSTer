// Directed exact-cycle vectors for the P3 programmable raster interrupt
// (reference §7). Drives asic_ga_timing alone with a synthetic CRTC that
// advances on the module's own CCLK_EN_N, as asic_video does in production:
// HSYNC_I pulses once per line, crtc_line counts {VC,RC} lines, and PRI is
// driven directly (its &6800 storage is asic_regs').
//
//   pr01  PRI=0 baseline: interrupt period stays exactly 52 lines; the
//         last-ack-was-raster level latches on each raster acknowledge,
//         never on the fire itself (lockstep already pins the full
//         output set; this pins the new export).
//   pr02  PRI=k: counter fires are suppressed; INT_N falls at the ordinary
//         request of the matching line (phase pinned by pr10), at the
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
//   pr07  raster_fire during an active acknowledge cycle (DMA ack or
//         prior raster ack) must not be lost (B19 residual): the coincident
//         fire is held pending across the acknowledge and asserts INT_N low
//         on the cycle following acknowledge deassertion.
//   pr08  a pending CPC request is masked, not lost, across PRI 0<->k.
//   pr09  PRI writes to the current line, guards and held-value control.
//   pr10  one request 1 us after raw HSYNC assertion at widths 1..11.
//   pr11  PRI write window, one character past raw HSYNC (probe 04).
//   pr12  HSYNC crossing into the PRI line at R2=49..63 (probes 05-09, 19-20).
//   pr10-pr12 use the plus_hw_probes geometry (R0=63).
//   pr14  classic (compatible) delivery one character after raw assertion:
//         V5 conditional bounds (not delivered by 55, delivered by 72),
//         MRER cancellation / DMA ACK retention in the
//         pending window, PRI mask/unmask maturation and SNA immediate
//         delivery controls.
//
// Expectations are derived from reference §7 / [ARNOLD-REV §2.4] and cited
// inline — never read back out of the simulator.

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Vasic_ga_timing.h"
#include "Vasic_ga_timing___024root.h"
#include "verilated.h"

namespace {

class TestFailure : public std::runtime_error {
public:
	explicit TestFailure(const std::string& m) : std::runtime_error(m) {}
};

[[noreturn]] void fail(const std::string& what) { throw TestFailure(what); }

constexpr unsigned kCharClks = 64;    // master clocks per CRTC character (1 us)
constexpr unsigned kLineClks = 512;   // ticks per default synthetic line
// Seven character clocks high within the eight-character synthetic line.
// This lets the monitor shaper finish its six-character sequence before
// raw HSYNC falls, and leaves a low interval at the next line boundary.
// The production-connected p1 fixture separately uses 4096-clock lines.
constexpr unsigned kHsWidth  = 448;

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
	// Synthetic CRTC. Like asic_video (CLKEN = this module's CCLK_EN_N), it
	// advances one character on each CCLK_EN_N edge, so raw HSYNC and the
	// {VC,RC} line change on the character boundaries production uses.
	// hcount counts master clocks since the current line started.
	unsigned hcount = 0;
	unsigned chr = 0;             // C0 of the current character
	unsigned line_chars = kLineClks / kCharClks; // R0 + 1
	unsigned hs_start = 0;        // R2: C0 where raw HSYNC starts
	unsigned hs_width = kHsWidth; // raw HSYNC width in clocks (R3 * 64)
	unsigned hsc = 0;
	bool in_hs = false;

	explicit PriBench() : dut("asic_ga_timing") {
		dut.clk = 0;
		dut.pri = 0;
		dut.crtc_line = 0;
		dut.crtc_adj = 0;
	}

	void tick() {
		// Production asic_video.HSYNC is active high. Inverting this made
		// the ordinary-only fixture overlap every line entry for 480 clocks.
		const bool hs_i = in_hs;
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
		const bool char_edge = dut.CCLK_EN_N;
		dut.clk = 1;
		dut.eval();
		++cyc;
		cen_phase = (cen_phase + 1) % 4;

		// Character bookkeeping on the same edge asic_video uses: C0 and the
		// line advance together, and HSYNC starts when the new C0 equals R2
		// and lasts hs_width / 64 characters (asic_video hsync_start/hsc),
		// crossing the line boundary when R2 + width exceeds R0.
		++hcount;
		if (char_edge) {
			chr = (chr + 1) % line_chars;
			if (chr == 0) { ++crtc_line; hcount = 0; }
			if (in_hs) {
				if (++hsc == hs_width / kCharClks) in_hs = false;
			} else if (chr == hs_start) {
				in_hs = true;
				hsc = 0;
			}
		}
	}

	unsigned line_clks() const { return line_chars * kCharClks; }

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
// pr02: PRI=k suppresses counter fires and fires at the ordinary event of
// the matching line. The intra-line fire offset is self-calibrated on the
// first event (pr10 pins its phase), then required to repeat exactly on
// the next match.
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
	for (unsigned i = 0; i < 55; ++i) tick_pub(b);
	b.iorq_n = true; b.m1_n = true;
	b.fast = false;
	b.run(8);
	if (b.dut.INT_N != 1) fail("pr05: ack did not raise INT");
	if (b.dut.int_last_raster != 1)
		fail("pr05: last-raster level must persist across its own ack");
	// Empty acknowledge (nothing pending): the level clears.
	b.fast = true;
	b.iorq_n = false; b.m1_n = false;
	for (unsigned i = 0; i < 55; ++i) tick_pub(b);
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

//----------------------------------------------------------------------
// pr07: raster_fire during an active acknowledge cycle must NOT be lost
// (B19 residual). When an interrupt acknowledge (DMA ack or prior raster
// ack) is in flight, a coincident raster_fire must be held pending and
// pull INT_N low on the cycle following acknowledge deassertion, where
// a subsequent acknowledge correctly latches the last-ack-was-raster level.
//
// Expectation: coincident raster fire during acknowledge is held pending
// and asserted on acknowledge release (simulation modelling choice for B19
// residual; real ASIC exact sub-cycle response unmeasured, device acceptance
// pending).
//----------------------------------------------------------------------
void pr07_raster_fire_during_intack(PriBench& b) {
	b.ga_mrer_clear();
	b.empty_ack();
	if (b.dut.INT_N != 1) fail("pr07: expected idle INT_N after clear");
	if (b.dut.int_last_raster != 0) fail("pr07: expected clear last-raster");

	// Dynamically calibrate the intra-line offset of the ordinary event
	// for the PRI match to ensure the test window precisely spans the fire tick.
	uint16_t cal_line = uint16_t((b.crtc_line + 4) & 0x7F);
	if (cal_line == 0) cal_line = 1;
	b.pri = uint8_t(cal_line);

	uint64_t guard = 0;
	while ((b.crtc_line & 0x1FF) != cal_line || b.dut.INT_N != 0) {
		b.tick();
		if (++guard > 800u * kLineClks) fail("pr07: timeout measuring fire offset");
	}
	const unsigned fire_hcount = b.hcount;
	b.empty_ack(); // clear calibration interrupt

	// ------------------------------------------------------------------
	// Case A: Raster fire during in-flight DMA acknowledge (intack=1)
	// ------------------------------------------------------------------
	uint16_t match_line_a = uint16_t((b.crtc_line + 4) & 0x7F);
	if (match_line_a == 0) match_line_a = 1;
	b.pri = uint8_t(match_line_a);

	// Advance to the start of match_line_a.
	guard = 0;
	while ((b.crtc_line & 0x1FF) != match_line_a || b.hcount != 0) {
		b.tick();
		if (++guard > 800u * kLineClks) fail("pr07: timeout reaching match line A");
	}

	// Advance to just before the measured fire point.
	while (b.hcount < fire_hcount - 15) {
		b.tick();
	}

	// Begin an interrupt acknowledge cycle while INT_N is idle (DMA ack).
	b.fast = true;
	b.iorq_n = false;
	b.m1_n = false;

	// Step across the measured ordinary event.
	while (b.hcount < fire_hcount + 15) {
		b.tick();
		if (b.dut.INT_N != 1)
			fail("pr07: INT_N must remain high during in-flight acknowledge cycle");
	}

	// Deassert the acknowledge cycle.
	b.iorq_n = true;
	b.m1_n = true;
	b.fast = false;

	// INT_N must assert low on the cycle following acknowledge deassertion.
	b.tick();
	if (b.dut.INT_N != 0)
		fail("pr07: raster_fire during intack did not pull INT_N low after intack deasserted");

	// Acknowledging the survived raster interrupt raises INT_N and sets bit 7.
	b.empty_ack();
	if (b.dut.INT_N != 1) fail("pr07: acknowledge did not raise INT_N");
	if (b.dut.int_last_raster != 1)
		fail("pr07: acknowledging survived raster interrupt must set last-raster level");

	// ------------------------------------------------------------------
	// Case B: Raster fire during prior raster interrupt acknowledge (irqack_rst)
	// ------------------------------------------------------------------
	uint16_t match_line_b1 = uint16_t((b.crtc_line + 4) & 0x7F);
	if (match_line_b1 == 0) match_line_b1 = 1;
	uint16_t match_line_b2 = uint16_t((match_line_b1 + 1) & 0x7F);
	if (match_line_b2 == 0) match_line_b2 = 1;

	// Fire on line b1
	b.pri = uint8_t(match_line_b1);
	guard = 0;
	while ((b.crtc_line & 0x1FF) != match_line_b1 || b.dut.INT_N != 0) {
		b.tick();
		if (++guard > 800u * kLineClks) fail("pr07: timeout reaching match line B1");
	}
	if (b.dut.INT_N != 0) fail("pr07: expected INT_N low from line B1 fire");

	// Set PRI for line b2
	b.pri = uint8_t(match_line_b2);

	// Advance to match_line_b2, just before the fire point
	guard = 0;
	while ((b.crtc_line & 0x1FF) != match_line_b2 || b.hcount < fire_hcount - 15) {
		b.tick();
		if (++guard > 800u * kLineClks) fail("pr07: timeout reaching match line B2");
	}

	// Begin acknowledge of line B1's raster interrupt (irqack_rst active)
	b.fast = true;
	b.iorq_n = false;
	b.m1_n = false;

	// Step across line B2's fire point
	while (b.hcount < fire_hcount + 15) {
		b.tick();
		if (b.dut.INT_N != 1)
			fail("pr07: INT_N must remain high during in-flight irqack_rst cycle");
	}

	// Deassert acknowledge
	b.iorq_n = true;
	b.m1_n = true;
	b.fast = false;

	// Line B2's coincident fire must assert INT_N on the next clock
	b.tick();
	if (b.dut.INT_N != 0)
		fail("pr07: raster_fire during irqack_rst did not pull INT_N low after ack deasserted");

	b.empty_ack();
	if (b.dut.INT_N != 1) fail("pr07: acknowledge did not raise INT_N");
	if (b.dut.int_last_raster != 1)
		fail("pr07: acknowledging line B2 raster interrupt must set last-raster level");

	// ------------------------------------------------------------------
	// Negative control: acknowledge without coincident fire leaves INT_N high
	// ------------------------------------------------------------------
	b.pri = 0xFF; // No match
	b.empty_ack();
	if (b.dut.INT_N != 1)
		fail("pr07: negative control - INT_N must remain high after ack without coincident fire");

	std::printf("PASS pr07: raster_fire during intack & irqack_rst survives and asserts on deassertion\n");
}

// A PRI mode change must suppress delivery of an already-pending CPC request,
// not just prevent new 52-line events. Arnold V section 2.4 selects the PRI
// mechanism instead of CPC interrupts; Kevin Thacker, Extra CPC Plus Hardware
// Information (interrupts), says CPC requests are inactive while ASIC raster
// interrupts are active. The retain/unmask control is observed in AmSpirit
// 1.15.1/core2491682; original-hardware adjudication remains outstanding.
// See docs/investigations/hardware-runs/
// eerie-forest-pending-classic-2026-09-23.md.
void pr08_pending_classic_mode_switch() {
	PriBench b;
	b.power_on();
	b.empty_ack();
	b.empty_ack();
	wait_fire(b, "pr08 classic pending", 120u * kLineClks);
	// No acknowledge or MRER clear between the mode changes. Choose a
	// nonmatching line and stay within this line, so neither transition can
	// be explained by a newly generated interrupt.
	b.pri = uint8_t(((b.crtc_line + 64) & 0x7f) + 1);
	b.run(2);
	if (b.dut.INT_N != 1)
		fail("pr08: nonzero PRI must mask an already-pending CPC interrupt");
	b.pri = 0;
	b.run(2);
	if (b.dut.INT_N != 0)
		fail("pr08: returning to PRI=0 must expose the retained CPC request");
	b.empty_ack();
	if (b.dut.INT_N != 1 || b.dut.int_last_raster != 1)
		fail("pr08: retained CPC request must acknowledge as raster");
	std::printf("PASS pr08: PRI masks and unmasks a pending CPC request\n");
}

// A changed PRI matching the delayed nine-bit line while delayed HSYNC is
// high requests an interrupt: the comparator term rises on the write
// (docs/plus/source-divergences.md, "PRI delayed-comparator candidate").
// FF2's production-T80 replay writes 46->48 on line 48 at C0=52, after the
// ordinary request and before raw HSYNC ends at C0=57; its palette/music
// chain otherwise waits a whole frame (references/ff2-runtime-pri-
// 2026-09-27.md; CPCEC c025aab cpcec.c:2106-2114 has the same rule).
// On this bench raw HSYNC covers C0 0..6 of each 8-character line, so
// HSYNC_d covers C0 1..7 and is low at C0 0 with line_d still the previous
// line. Phase 480 is C0 7, one character after raw HSYNC ends: original
// Plus probe 04 requests there (write at C0 60 with raw HSYNC 49..59; pr11
// pins the whole window). Line 49 phase 32 is C0 0, where line_d=48 but
// HSYNC_d is low. asic_video changes ADJ only at line starts, so the
// adjustment case holds adj for the whole line (adj_d follows at C0 1).
void pr09_live_pri_write() {
	struct Case { const char* name; unsigned line, phase, value; bool adj, ack, fire; };
	const Case cases[] = {
		{"late matching write", 48, 416, 48, false, false, true},
		{"write during DMA acknowledge", 48, 416, 48, false, true, true},
		{"write one character after raw HSYNC", 48, 480, 48, false, false, true},
		{"write outside delayed HSYNC", 49, 32, 48, false, false, false},
		{"nonmatching write", 48, 416, 49, false, false, false},
		{"ninth-bit mismatch", 304, 416, 48, false, false, false},
		{"PRI zero", 48, 416, 0, false, false, false},
		{"vertical adjustment", 48, 416, 48, true, false, false},
	};
	for (const auto& c : cases) {
		PriBench b;
		b.power_on();
		b.empty_ack();
		b.empty_ack();
		// PRI=255 has no match before line48; clear its earlier event when
		// exercising line304, before advancing to the late-write window.
		b.pri = 255;
		while (b.crtc_line < c.line - 1) b.tick();
		b.empty_ack();
		while (b.crtc_line != c.line || b.hcount != c.phase) {
			b.tick();
			if (b.crtc_line == c.line) b.adj = c.adj;
		}
		if (!b.dut.INT_N) fail(std::string("pr09 setup: ") + c.name);
		if (c.ack) { b.iorq_n = false; b.m1_n = false; b.run(2); }
		b.pri = uint8_t(c.value);
		b.run(2);
		if (c.ack) {
			if (!b.dut.INT_N) fail("pr09: write event asserted during DMA acknowledge");
			if (b.dut.int_last_raster) fail("pr09: write event changed DMA acknowledge provenance");
			b.iorq_n = true; b.m1_n = true;
			b.run(2);
		}
		if (bool(!b.dut.INT_N) != c.fire)
			fail(std::string("pr09: ") + c.name + (c.fire ? " lost its request" : " created a request"));
		if (c.fire) {
			// A different, nonmatching PRI must not clear a request already
			// pending. CPCEC's separate clear-on-write policy is not adopted.
			b.pri = 200; b.run(2);
			if (b.dut.INT_N) fail("pr09: nonmatching write cleared pending raster");
		}
	}
	// Hold an already-matching value across an acknowledge while raw HSYNC
	// remains high. A level comparator would continually reassert here.
	PriBench b;
	b.power_on(); b.empty_ack(); b.empty_ack();
	b.pri = 48;
	while (b.crtc_line != 48 || b.hcount != 400) b.tick();
	if (b.dut.INT_N) fail("pr09: ordinary event missing in held-value control");
	b.iorq_n = false; b.m1_n = false; b.run(4);
	b.iorq_n = true; b.m1_n = true;
	b.pri = 48;
	for (unsigned i=0; i<80; ++i) {
		b.tick();
		if (!b.dut.INT_N) fail("pr09: unchanged PRI retriggered after acknowledge");
	}
	std::printf("PASS pr09: late PRI writes, delayed-HSYNC/9-bit/adjust guards, ACK deferral and held-value control\n");
}

//----------------------------------------------------------------------
// Production-like geometry for pr10-pr12: R0=63 (64 characters per line),
// raw HSYNC from C0=R2 for R3 characters, as in the plus_hw_probes screens
// (scripts/diagnostics/plus_hw_probes.asm). Line 10 is the PRI line unless a
// vector says otherwise.
//----------------------------------------------------------------------
struct Req { uint16_t line; unsigned chr; uint64_t cyc; };

void probe_geometry(PriBench& b, unsigned r2, unsigned r3) {
	b.line_chars = 64;
	b.hs_start = r2;
	b.hs_width = r3 * kCharClks;
}

// Tick until the synthetic CRTC reaches (line, C0, clock offset in C0).
void run_to(PriBench& b, uint16_t line, unsigned chr, unsigned off = 0) {
	while (!(b.crtc_line == line && b.hcount == chr * kCharClks + off)) b.tick();
}

// Run until the synthetic CRTC reaches line `until`, acknowledging each
// request as INT_N falls. A request raised during that acknowledge is held
// pending and delivered afterwards (pr07), so none is lost from the count.
std::vector<Req> collect(PriBench& b, uint16_t until) {
	std::vector<Req> reqs;
	while (b.crtc_line != until) {
		b.tick();
		if (!b.dut.INT_N) {
			reqs.push_back({uint16_t(b.crtc_line), b.chr, b.cyc});
			b.empty_ack();
		}
	}
	return reqs;
}

std::string describe(const std::vector<Req>& reqs) {
	std::string s;
	for (const auto& r : reqs)
		s += " (" + std::to_string(r.line) + "," + std::to_string(r.chr) + ")";
	return s.empty() ? " none" : s;
}

//----------------------------------------------------------------------
// pr10: ordinary PRI phase and HSYNC width. The request is the rising edge
// of HSYNC_d && {0,PRI}==line_d && PRI!=0 && !adj_d, where the _d terms are
// the CRTC outputs one character (64 clocks) late and PRI is live
// (docs/plus/source-divergences.md, "PRI delayed-comparator candidate").
// HSYNC_d rises exactly one character after raw HSYNC, whatever the width,
// and a one-character HSYNC still gives a one-character HSYNC_d pulse.
//   - Width independence and the 1 us CPU slot: original Plus flat-plane
//     markers at R3=3/6/11 (~136/135/139 dots), AmSpirit 139/139/139;
//     docs/plus/references/eerie-pri-trigger-counterfactual-2026-09-27.md.
//   - Width 1 requests: original Plus probe 01, IRQ/FRAME=01.
// Measured from the first edge sampling raw HSYNC high, INT_N falls 64
// clocks later, exactly one request per matching line.
//----------------------------------------------------------------------
void pr10_ordinary_phase() {
	for (unsigned width : {1u, 2u, 3u, 6u, 11u}) {
		const std::string w = "pr10: width " + std::to_string(width);
		PriBench b;
		probe_geometry(b, 49, width);
		b.power_on();
		b.empty_ack();
		b.empty_ack();
		b.pri = 10;
		run_to(b, 10, 0);
		while (!b.dut.HSYNC_I) b.tick();
		const uint64_t hs_rise = b.cyc; // first edge sampling raw HSYNC high
		const auto reqs = collect(b, 12);
		if (reqs.size() != 1)
			fail(w + ": expected one request on line 10, got" + describe(reqs));
		const uint64_t delay = reqs[0].cyc - hs_rise;
		if (delay != 64)
			fail(w + " INT " + std::to_string(delay) + " clocks after raw HSYNC, expected 64");
	}
	std::printf("PASS pr10: one PRI request 1 us after raw HSYNC at widths 1/2/3/6/11\n");
}

//----------------------------------------------------------------------
// pr11: PRI written to the current line (original Plus probe 04). R2=49,
// R3=11: raw HSYNC covers C0 49..59, so HSYNC_d covers C0 50..60. PRI
// changes from a nonmatching 200 to the current line 10 at C0 x (+32
// clocks). The comparator term is already true or becomes true then:
//   x <= 49: C rises when HSYNC_d does, at C0 50 (ordinary request);
//   50 <= x <= 60: C rises at the write, inside C0 x;
//   x >= 61: HSYNC_d is low for the rest of line 10, and on line 11
//            line_d moves away from 10 as HSYNC_d returns: no request.
// Probe 04 photographs: marks for writes at C0 45..60, none at 61..63,
// IRQ/FRAME=16 (docs/plus/source-divergences.md). The old raw-HSYNC write
// window missed C0 60.
//----------------------------------------------------------------------
void pr11_pri_write_window() {
	for (unsigned x = 45; x <= 63; ++x) {
		const std::string w = "pr11: write at C0 " + std::to_string(x);
		PriBench b;
		probe_geometry(b, 49, 11);
		b.power_on();
		b.empty_ack();
		b.empty_ack();
		b.pri = 200;
		run_to(b, 10, x, 32);
		b.pri = 10;
		const auto reqs = collect(b, 12);
		if (x <= 60) {
			const unsigned want = x < 50 ? 50 : x;
			if (reqs.size() != 1 || reqs[0].line != 10 || reqs[0].chr != want)
				fail(w + ": expected one request at (10," + std::to_string(want) +
				     "), got" + describe(reqs));
		} else if (!reqs.empty()) {
			fail(w + ": expected no request, got" + describe(reqs));
		}
	}
	std::printf("PASS pr11: PRI writes request through C0 60 with raw HSYNC 49..59\n");
}

//----------------------------------------------------------------------
// pr12: HSYNC crossing into the PRI line (original Plus probes 05-09,
// R3=8, PRI=7). Raw HSYNC runs C0 R2..R2+7, wrapping into line 7 when
// R2 > 56; HSYNC_d is the same window one character later. The comparator
// matches PRI against both the live line and line_d, so a line change seen
// while HSYNC_d is high requests at once, at line 7 C0 0.
//   R2=49: HSYNC_d 50..57 of line 7                    -> (7,50)
//   R2=56: HSYNC_d line 6 57..63, line 7 0 (raw HSYNC ends exactly at
//          line 7's start)                             -> (7,0), (7,57)
//   R2=57: HSYNC_d line 6 58..63, line 7 0..1          -> (7,0), (7,58)
//   R2=58: HSYNC_d line 6 59..63, line 7 0..2          -> (7,0), (7,59)
//   R2=62: HSYNC_d line 6 63, line 7 0..6              -> (7,0), (7,63)
//   R2=63: HSYNC_d line 7 0..7; line 7's own HSYNC starts at C0 63, so
//          HSYNC_d rises at line 8 C0 0 with line_d still 7 -> (7,0), (8,0)
//   R2=50, R3=14: raw HSYNC 50..63 ends exactly at line 7's start
//                                                      -> (7,0), (7,51)
// Hardware IRQ/FRAME: 01, 02, 02, 02, 02 for R2=49/57/58/62/63
// (docs/plus/source-divergences.md). The two exact-end cases are probe
// screens 19-20: AmSpirit gives 02 for both (it matches every photographed
// screen), and the CRTC3 demo's plasma, sphere and Wolverine scenes (R2=50,
// R3=14) need the line-start request. The line_d-only comparator gave one.
//----------------------------------------------------------------------
void pr12_line_entry() {
	struct Case { unsigned r2, r3; std::vector<std::pair<unsigned, unsigned>> want; };
	const Case cases[] = {
		{49, 8, {{7, 50}}},
		{56, 8, {{7, 0}, {7, 57}}},
		{57, 8, {{7, 0}, {7, 58}}},
		{58, 8, {{7, 0}, {7, 59}}},
		{62, 8, {{7, 0}, {7, 63}}},
		{63, 8, {{7, 0}, {8, 0}}},
		{50, 14, {{7, 0}, {7, 51}}},
	};
	for (const auto& c : cases) {
		PriBench b;
		probe_geometry(b, c.r2, c.r3);
		b.power_on();
		b.empty_ack();
		b.empty_ack();
		b.pri = 7;
		run_to(b, 5, 0);
		const auto reqs = collect(b, 10);
		bool ok = reqs.size() == c.want.size();
		for (size_t i = 0; ok && i < reqs.size(); ++i)
			ok = reqs[i].line == c.want[i].first && reqs[i].chr == c.want[i].second;
		if (!ok) {
			std::vector<Req> want;
			for (const auto& p : c.want) want.push_back({uint16_t(p.first), p.second, 0});
			fail("pr12: R2=" + std::to_string(c.r2) + " R3=" + std::to_string(c.r3) + " expected" + describe(want) +
			     ", got" + describe(reqs));
		}
	}
	std::printf("PASS pr12: HSYNC crossing into the PRI line at R2=49/56/57/58/62/63, 50 (R3=14)\n");
}

} // namespace

// PA6e: KT "Extra CPC Plus Hardware Information", Raster Interrupts:
// a PRI fire clears only bit5 of the six-bit GA counter. Seed 40 (0x28),
// so the fire must leave 8 BEFORE acknowledge (which also clears bit5).
// Re-enabling CPC then requires 52-8=44 HSYNC trailing edges. A full
// reset would require 52; omitting the fire clear leaves 40 until ACK.
// KT also infers "not closer than 32 lines"; that is not general for the
// stated bit operation (e.g. 31 stays 31). This vector pins the operation.
void pr13_fire_clears_counter_bit5() {
    PriBench b;
    b.line_chars = 64;
    b.hs_start = 16;
    b.hs_width = 8 * kCharClks;
    b.power_on();
    b.pri = 1;
    while (b.crtc_line != 1 || b.chr != 8) b.tick();
    b.dut.SNA_LOAD = 1;
    b.dut.SNA_INTCNT = 40;
    b.dut.SNA_VSDELAY = 0;
    b.dut.SNA_VS = 1;
    b.dut.SNA_HS = 0;
    b.dut.SNA_INT = 0;
    b.tick();
    b.dut.SNA_LOAD = 0;
    wait_fire(b, "pr13 PRI", b.line_clks());
    if (b.dut.rootp->asic_ga_timing__DOT__intcnt_reg != 8)
        fail("pr13: PRI fire did not leave count 8 before acknowledge");
    b.empty_ack();
    b.pri = 0;
    b.tick();
    unsigned falls = 0;
    bool hs = b.in_hs;
    const uint64_t deadline = b.cyc + 46u * b.line_clks();
    while (b.dut.INT_N && b.cyc < deadline) {
        b.tick();
        if (hs && !b.in_hs) ++falls;
        hs = b.in_hs;
    }
    if (b.dut.INT_N || falls != 44)
        fail("pr13: CPC re-enable expected 44 HSYNC falls, got " + std::to_string(falls));
    std::printf("PASS pr13: PRI fire clears 40 to 8 before ACK; CPC resumes after 44 falls\n");
}

// pr14: classic (compatible) delivery one character after raw assertion.
//
// Source: docs/plus/pa7-interrupt-phase-followup.md V5 original-6128-Plus
// results. With the shared-CPU sampling correction applied in T80, compatible delivery needs approximately one additional
// microsecond: disposable counterfactual C16 (common 16 ticks) + K64
// (compatible-only 64 ticks) matches all six V5 screens, while K64 alone
// leaves the two pass-A RET edges 32 dots early. The surviving hardware
// window with CPU correction is roughly 56..71 master ticks of compatible
// delay. Under the retained raw-counter phase, this vector asserts loose
// bounds on the chosen delivery model:
//
//   - INT_N must still be high 55 ticks after the raw classic latch falls
//     (current RTL delivers immediately, so this fails before the fix);
//   - INT_N must be low by 72 ticks after raw.
//
// These bounds are inferred from V5 with the retained raw-counter phase;
// they are not measurements of the physical INT pin. The chosen next
// CCLK_EN_N sampling edge (about one character / ~63 master ticks after raw) is a separately labeled MODEL
// POLICY: these screens do not distinguish delays within the same CPU
// sampling interval, and the follow-up
// records that a fractional delay, a shared-HSYNC shift or a different
// sub-character phase could give the same pictures. The conditional 55/72
// window checks this model; an exact next-CCLK assertion would merely
// mirror the implementation and is deliberately omitted.
//
// MRER cancellation must prevent a delivered pulse. Original-Plus overlap
// photographs supersede the former ACK-cancels-undelivered assumption:
// DMA ACK retains compatible pending. Hidden classic pending keeps
// maturing while PRI masks delivery, and an SNA-restored pending request
// is seeded already delivered (SNA_LOAD sets delivery exactly as raw).
static uint64_t pr14_wait_raw(PriBench& b, const char* who) {
    uint64_t guard = 0;
    while (b.dut.rootp->asic_ga_timing__DOT__classic_int_n != 1) {
        b.tick();
        if (++guard > 120u * kLineClks)
            fail(std::string(who) + ": never reached idle classic before raw");
    }
    guard = 0;
    while (true) {
        b.tick();
        if (++guard > 120u * kLineClks)
            fail(std::string(who) + ": no classic raw event within budget");
        if (b.dut.rootp->asic_ga_timing__DOT__classic_int_n == 0)
            return b.cyc;
    }
}

void pr14_classic_delivery() {
    // A1: hardware bounds — not delivered by 55, delivered by 72.
    {
        PriBench b;
        b.power_on();
        b.empty_ack();
        b.empty_ack();
        b.pri = 0;
        b.run(2);
        pr14_wait_raw(b, "pr14 delay");
        for (unsigned i = 0; i < 55; ++i) {
            b.tick();
            if (b.dut.INT_N == 0)
                fail("pr14 delay: classic INT delivered " + std::to_string(i + 1) +
                     " ticks after raw, must stay high for 55 (V5 needs ~64)");
        }
        bool delivered = false;
        for (unsigned i = 55; i < 72; ++i) {
            b.tick();
            if (b.dut.INT_N == 0) { delivered = true; break; }
        }
        if (!delivered)
            fail("pr14 delay: classic INT not delivered by 72 ticks after raw (V5 needs ~64)");
        b.empty_ack();
    }
    // B: MRER bit 4 inside the pending window cancels with no ghost pulse.
    {
        PriBench b;
        b.power_on();
        b.empty_ack();
        b.empty_ack();
        b.pri = 0;
        b.run(2);
        pr14_wait_raw(b, "pr14 MRER");
        if (b.dut.INT_N == 0)
            fail("pr14 MRER: INT already low at raw, must stay high until delivery (too early)");
        b.fast = true;
        b.iorq_n = false;
        b.bus_a = 0x4000 >> 14;
        b.bus_d = 0x90;
        bool cleared = false;
        for (unsigned i = 0; i < 60; ++i) {
            b.tick();
            if (b.dut.INT_N == 0)
                fail("pr14 MRER: ghost INT asserted before MRER cleared raw (old delivers too early)");
            if (b.dut.rootp->asic_ga_timing__DOT__classic_int_n == 1) { cleared = true; break; }
        }
        if (!cleared) fail("pr14 MRER: MRER did not clear raw within 60 ticks");
        b.iorq_n = true;
        b.fast = false;
        b.run(2);
        for (unsigned i = 0; i < 100; ++i) {
            b.tick();
            if (b.dut.INT_N == 0)
                fail("pr14 MRER: ghost INT after window-cancelled raw");
        }
    }
    // C: Original-Plus overlap (2026-09-28) retains both sources. Exercise
    // release before delivery and delivery inside the same held DMA ACK.
    // Internal timing is model preservation; source survival is hardware.
    for (unsigned ack_ticks : {4u, 100u}) {
        PriBench b;
        b.power_on();
        b.empty_ack();
        b.empty_ack();
        b.pri = 0;
        b.run(2);
        pr14_wait_raw(b, "pr14 ACK");
        if (b.dut.INT_N == 0)
            fail("pr14 ACK: INT already low at raw, must stay high until delivery (too early)");
        b.iorq_n = false;
        b.m1_n = false;
        for (unsigned i = 0; i < ack_ticks; ++i) {
            b.tick();
            if (i < 55 && b.dut.INT_N == 0)
                fail("pr14 ACK: delivery moved earlier during DMA ACK");
            if (b.dut.int_last_raster)
                fail("pr14 ACK: compatible maturation changed DMA provenance");
            if (b.dut.rootp->asic_ga_timing__DOT__irqack_rst)
                fail("pr14 ACK: DMA ACK armed held raster counter clear");
        }
        b.iorq_n = true;
        b.m1_n = true;
        b.run(2);
        b.run(100);
        if (b.dut.INT_N || b.dut.rootp->asic_ga_timing__DOT__classic_int_n)
            fail("pr14 ACK: DMA ACK lost compatible request");
        b.empty_ack();
        if (!b.dut.INT_N || !b.dut.int_last_raster)
            fail("pr14 ACK: subsequent raster ACK must retire retained request");
    }
    // D: MODEL POLICY: hidden classic pending matures under PRI mask.
    // This pins the selected delayed-delivery mechanism, not later raw timing.
    // Classic only fires with PRI==0, so the pending must exist before the
    // mask (pr08 pattern): raw with PRI=0, mask inside the undelivered
    // window, let delivery mature hidden, then unmask for immediate INT.
    {
        PriBench b;
        b.power_on();
        b.empty_ack();
        b.empty_ack();
        b.pri = 0;
        b.run(2);
        pr14_wait_raw(b, "pr14 PRI mask");
        b.pri = 200;
        b.run(2);
        for (unsigned i = 0; i < 100; ++i) {
            b.tick();
            if (b.dut.INT_N != 1)
                fail("pr14 PRI: INT must stay high while PRI nonzero masks hidden pending");
        }
        b.pri = 0;
        b.run(2);
        if (b.dut.INT_N != 0)
            fail("pr14 PRI: unmasking to PRI=0 must expose matured classic immediately");
        b.empty_ack();
        if (b.dut.INT_N != 1 || b.dut.int_last_raster != 1)
            fail("pr14 PRI: unmasked classic must acknowledge as raster");
    }
    // E: SNA restore seeds delivery as raw, so restored pending is immediate.
    {
        PriBench b;
        b.power_on();
        b.empty_ack();
        b.empty_ack();
        b.pri = 0;
        b.run(2);
        b.dut.SNA_LOAD = 1;
        b.dut.SNA_INTCNT = 0;
        b.dut.SNA_VSDELAY = 0;
        b.dut.SNA_VS = 1;
        b.dut.SNA_HS = 0;
        b.dut.SNA_INT = 1;
        b.tick();
        b.dut.SNA_LOAD = 0;
        b.run(2);
        if (b.dut.INT_N != 0)
            fail("pr14 SNA: restored classic pending must be immediately delivered");
        b.empty_ack();
        if (b.dut.INT_N != 1)
            fail("pr14 SNA: restored pending must acknowledge");
    }
    std::printf("PASS pr14: compatible delivery bounds, MRER cancel, DMA ACK retention, PRI/SNA controls\n");
}

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
		pr07_raster_fire_during_intack(b);
		pr08_pending_classic_mode_switch();
		pr09_live_pri_write();
		pr10_ordinary_phase();
		pr11_pri_write_window();
		pr12_line_entry();
		pr13_fire_clears_counter_bit5();
		pr14_classic_delivery();
	} catch (const TestFailure& e) {
		std::printf("FAIL: %s\n", e.what());
		return 1;
	}
	std::printf("All asic_pri vectors passed\n");
	return 0;
}
