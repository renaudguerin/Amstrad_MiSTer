// Directed exact-cycle vectors for the P3 programmable raster interrupt
// (reference §7). Drives asic_ga_timing alone with a synthetic CRTC that
// advances on the module's own CCLK_EN_N, as asic_video does in production:
// HSYNC_I pulses once per line, crtc_line counts {VC,RC} lines, and PRI is
// driven directly (its &6800 storage is asic_regs').
//
//   pr01  PRI=0 baseline: interrupt period stays exactly 52 lines; the
//         last-ack-was-raster level latches on each raster acknowledge,
//         never on the fire itself. GA lockstep owns common non-IRQ pins;
//         this owner checks delivered IRQ and raster-ACK provenance.
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
//   pr14  compatible delivery at real HS ends: VS second-end bit5 gating,
//         52 following ends, MRER clear, DMA-selected ACK retention,
//         established-request masking, settled SNA immediate pending and
//         late raster-ACK bit5 clear with low5-bit count retention.
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
	bool vsync_i = true; // changed only after producer character boundaries in pr14
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
		dut.VSYNC_I = vsync_i;
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
	// asserted idle baseline, so both measured fires are genuine events.
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
// See hardware observations.
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
// (Plus timing observations, "PRI delayed-comparator candidate").
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
// (Plus timing observations, "PRI delayed-comparator candidate").
// HSYNC_d rises exactly one character after raw HSYNC, whatever the width,
// and a one-character HSYNC still gives a one-character HSYNC_d pulse.
//   - Width independence and the 1 us CPU slot: original Plus flat-plane
//     markers at R3=3/6/11 (~136/135/139 dots), AmSpirit 139/139/139;
//     Plus probe results.
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
// IRQ/FRAME=16 (Plus timing observations). The old raw-HSYNC write
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
// (Plus timing observations). The two exact-end cases are probe
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

// pr14 owns the compatible IRQ contract at delivered INT_N and actual
// producer HS ends. ACCC FR §§27.1–27.2 p.283, §27.3.2 p.284: one update
// per end, unconditional second-end VS rephase, bit-5 qualification, 52
// subsequent ends to normal wrap. V5 owns tight software phase; private raw
// latch +55/+72 bounds depended on a counter phase the hardware rejected.
// Snapshot import below is a settled-history compatibility policy, not a
// claim that the SNA format records the silicon's subcharacter history.
void pr14_setup(PriBench& b) {
    b.line_chars = 64;
    b.hs_start = 16;
    b.hs_width = 8 * kCharClks;
    b.vsync_i = false; // park the monitor walk before a controlled restart
    b.power_on();
    b.empty_ack();
    b.empty_ack();
    // Cold hcnt is not yet the parked lost-sync origin: its initial walk
    // can rephase the counter even without a VS edge. Let the complete
    // monitor walk expire before defining the MRER/count epoch.
    b.run(34u * b.line_clks());
    while (b.chr != 32) b.tick(); // inactive HS, ample bus-write margin
    b.ga_mrer_clear();
    if (!b.dut.INT_N) fail("pr14 setup: MRER did not establish idle INT");
}

void pr14_ends(PriBench& b, unsigned ends, const char* who,
              bool require_idle = true) {
    unsigned seen = 0;
    for (uint64_t guard = 0; seen < ends; ++guard) {
        if (guard > uint64_t(ends + 2) * b.line_clks())
            fail(std::string(who) + ": producer HS timeout");
        const bool hs = b.in_hs;
        b.tick();
        if (require_idle && !b.dut.INT_N)
            fail(std::string(who) + ": early delivered IRQ after " +
                 std::to_string(seen) + " complete HS ends");
        if (hs && !b.in_hs) ++seen;
    }
}

void pr14_delivery_before_next_end(PriBench& b, const char* who,
                                   bool ordinary_wrap = false,
                                   uint64_t eligible_tick = 0) {
    // Strictly after the eligible producer end, before the next end.
    // Ordinary wraps additionally retain a coarse <=2-character regression
    // envelope: V5's original-Plus markers constrain the ordinary software
    // acceptance phase after the CPU sampling fix (pa7-interrupt-phase-
    // followup.md), and the MRER program constrains the HS-end anchor.
    // A whole extra character would cross a NOP acceptance slot. This is
    // a conservative model policy, NOT an ACCC electrical INT-delay bound;
    // VS rephase keeps only its independently sourced line window.
    if (!eligible_tick) eligible_tick = b.cyc;
    if (!b.dut.INT_N) fail(std::string(who) + ": IRQ preceded eligible HS end");
    for (unsigned guard = 0; guard < b.line_clks() + kCharClks; ++guard) {
        if (ordinary_wrap && b.cyc - eligible_tick >= 2u * kCharClks)
            fail(std::string(who) + ": ordinary delivery exceeded two-character regression envelope");
        const bool hs = b.in_hs;
        b.tick();
        if (hs && !b.in_hs)
            fail(std::string(who) + ": IRQ missing before next HS end");
        if (!b.dut.INT_N) return;
    }
    fail(std::string(who) + ": delivery timeout");
}

void pr14_rephase(unsigned preceding_ends) {
    PriBench b;
    pr14_setup(b);
    pr14_ends(b, preceding_ends, "pr14 A pre-VS");
    while (b.chr != 0) b.tick();
    // Producer advances VS on a character boundary, after DUT sampling,
    // as asic_video does. No new port or private latch observation.
    b.vsync_i = true;
    pr14_ends(b, 2, "pr14 A second-end rephase");
    // First VS-interval end increments p to p+1; the second samples that
    // old count before clearing. p30 ->31/silent, p31 ->32/request. This
    // matches physical frame N32/N33 counts0/64 (p=N-2), not p>=32.
    if (preceding_ends >= 31) {
        pr14_delivery_before_next_end(b, "pr14 A bit5 rephase");
        b.empty_ack();
        if (!b.dut.INT_N || !b.dut.int_last_raster)
            fail("pr14 A: rephase IRQ did not acknowledge as raster");
    }
    pr14_ends(b, 52, "pr14 A following wrap");
    pr14_delivery_before_next_end(b, "pr14 A following wrap", true);
    b.empty_ack();
    if (!b.dut.INT_N || !b.dut.int_last_raster)
        fail("pr14 A: normal wrap IRQ did not acknowledge as raster");
    pr14_ends(b, 2, "pr14 A no duplicate");
    std::printf("PASS pr14 A%u: second-end VS bit5 qualification, next wrap after 52 ends, ACK/no duplicate\n",
                preceding_ends);
}

void pr14_mrer() {
    PriBench b;
    pr14_setup(b);
    wait_fire(b, "pr14 B request", 54u * b.line_clks());
    b.ga_mrer_clear();
    if (!b.dut.INT_N) fail("pr14 B: MRER did not clear delivered request");
    pr14_ends(b, 2, "pr14 B MRER no refill ghost");
    std::printf("PASS pr14 B: MRER clears delivered compatible request, no refill ghost\n");
}

void pr14_dma_ack(unsigned hold_after_end) {
    PriBench b;
    pr14_setup(b);
    pr14_ends(b, 51, "pr14 C before wrap");
    while (b.chr != 8) b.tick(); // next line, before its eligible HS end
    b.iorq_n = false;
    b.m1_n = false; // no raster delivered at ACK start: DMA/expansion source
    pr14_ends(b, 1, "pr14 C eligible producer end");
    const uint64_t wrap_end_tick = b.cyc;
    // At this producer edge the DUT still saw the old HS level. The zero
    // hold releases ACK BEFORE the DUT samples raw fall; the long hold
    // crosses creation and delivery under the same DMA-selected ACK.
    for (unsigned i = 0; i < hold_after_end; ++i) {
        b.tick();
        if (b.dut.int_last_raster)
            fail("pr14 C: compatible creation changed held DMA provenance");
        if (b.dut.INT_N && b.cyc - wrap_end_tick >= 2u * kCharClks)
            fail("pr14 C: ordinary delivery exceeded two-character regression envelope");
    }
    b.iorq_n = true;
    b.m1_n = true;
    b.run(8); // observable bus release separates this ACK from the next
    // For the short ACK delivery is still due; for the long ACK it may
    // already be low. In either case the request must survive the release.
    if (b.dut.INT_N)
        pr14_delivery_before_next_end(b, "pr14 C retained request", true, wrap_end_tick);
    if (b.dut.int_last_raster)
        fail("pr14 C: DMA ACK was reclassified as raster");
    b.empty_ack();
    if (!b.dut.INT_N || !b.dut.int_last_raster)
        fail("pr14 C: subsequent raster ACK did not retire retained request");
    pr14_ends(b, 2, "pr14 C no duplicate");
    std::printf("PASS pr14 C %s: compatible request survives, next raster ACK retires once\n",
                hold_after_end ? "held through creation/delivery" : "released before DUT raw-fall sampling");
}

void pr14_mask() {
    PriBench b;
    pr14_setup(b);
    wait_fire(b, "pr14 D request", 54u * b.line_clks());
    // The physical coverage is retention/masking of an established request.
    // Hidden undelivered maturation remains model policy without a raw-latch
    // timing assertion. Adjustment prevents an unrelated programmed match.
    b.pri = 255;
    b.adj = true;
    b.run(2);
    if (!b.dut.INT_N) fail("pr14 D: PRI mask did not hide compatible request");
    pr14_ends(b, 2, "pr14 D masked request");
    b.pri = 0;
    b.run(2);
    if (b.dut.INT_N) fail("pr14 D: unmask lost established compatible request");
    b.empty_ack();
    if (!b.dut.INT_N || !b.dut.int_last_raster)
        fail("pr14 D: unmasked request did not acknowledge as raster");
    pr14_ends(b, 2, "pr14 D no duplicate");
    std::printf("PASS pr14 D: established compatible request masks, survives, unmasks and clears\n");
}

void pr14_snapshot() {
    PriBench b;
    pr14_setup(b);
    b.dut.SNA_LOAD = 1;
    b.dut.SNA_INTCNT = 0;
    b.dut.SNA_VSDELAY = 0;
    b.dut.SNA_VS = 0;
    b.dut.SNA_HS = 0;
    b.dut.SNA_INT = 1;
    b.tick();
    b.dut.SNA_LOAD = 0;
    if (b.dut.INT_N) fail("pr14 E: restored pending was not immediate");
    b.empty_ack();
    if (!b.dut.INT_N || !b.dut.int_last_raster)
        fail("pr14 E: restored pending did not acknowledge as raster");
    pr14_ends(b, 2, "pr14 E no refill ghost");
    std::printf("PASS pr14 E: settled pending import immediate, first ACK retires, no refill ghost\n");
}

// ACCC FR §27.3.1 p.284 and §27.7.1 p.288: a delayed raster ACK
// clears only bit5. The producer ends below are counted independently of
// private counter state; ACK is settled well outside the HS end aperture.
void pr14_late_raster_ack() {
    PriBench b;
    pr14_setup(b);
    pr14_ends(b, 52, "pr14 F first wrap");
    pr14_delivery_before_next_end(b, "pr14 F first wrap", true);
    pr14_ends(b, 40, "pr14 F pending accumulation", false);
    while (b.chr != 32) b.tick(); // let end40 commit before bus ACK
    if (b.dut.INT_N)
        fail("pr14 F: compatible request did not remain pending through 40 ends");
    b.empty_ack();
    if (!b.dut.INT_N || !b.dut.int_last_raster)
        fail("pr14 F: late compatible ACK did not retire as raster");
    // 40 &31 =8; 52-8=44. Retaining40 would fire after12 ends;
    // clearing the whole count would incorrectly postpone it to52.
    pr14_ends(b, 43, "pr14 F low5 retained/no refill ghost");
    pr14_ends(b, 1, "pr14 F end44");
    pr14_delivery_before_next_end(b, "pr14 F end44", true);
    b.empty_ack();
    if (!b.dut.INT_N || !b.dut.int_last_raster)
        fail("pr14 F: following wrap did not retire as raster");
    pr14_ends(b, 2, "pr14 F no duplicate");
    std::printf("PASS pr14 F: late raster ACK clears40 to8; idle43 ends, delivery after44, no ghost\n");
}

void pr14_classic_delivery() {
    unsigned failures = 0;
    auto run = [&](const char* name, auto fn) {
        try { fn(); }
        catch (const TestFailure& e) {
            ++failures;
            std::printf("FAIL %s: %s\n", name, e.what());
        }
    };
    // Independent instances/catches ensure a failed phase guard cannot
    // suppress MRER, overlap, masking or import evidence.
    run("pr14 A8", [] { pr14_rephase(8); });
    run("pr14 A30", [] { pr14_rephase(30); });
    run("pr14 A31", [] { pr14_rephase(31); });
    run("pr14 A32", [] { pr14_rephase(32); });
    run("pr14 B", pr14_mrer);
    run("pr14 C pre-sample release", [] { pr14_dma_ack(0); });
    run("pr14 C held through creation", [] { pr14_dma_ack(2 * kCharClks); });
    run("pr14 D", pr14_mask);
    run("pr14 E", pr14_snapshot);
    run("pr14 F", pr14_late_raster_ack);
    if (failures) fail("pr14: " + std::to_string(failures) + " independent subcases failed");
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
