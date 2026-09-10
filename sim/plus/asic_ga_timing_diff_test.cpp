// Lockstep differential bench for rtl/plus/asic_ga_timing.v against the
// classic netlist-derived ga40010 (docs/plus/architecture.md §5 Risk 1,
// decision 2026-08-24: Plus-mode CPU/SDRAM timing is reproduced behaviourally
// and pinned cycle-exact to the synthesised classic contract here).
//
// d01-d04 drive BOTH modules inside sim/plus/asic_ga_diff_top.v with identical
// randomised bus/sync/reset traffic (fixed seed schedule) and require every
// shared output to agree on every clock edge. Register payload decoding
// (BORDER/INKR/mode/ROM map/INT semantics) cannot be compared against
// ga40010, which does not export those registers, so r01-r03 pin them
// directly on the replica with expectations derived from the published Gate
// Array port description cited in the RTL header.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <array>
#include <vector>

#include "Vasic_ga_diff_top.h"
#include "verilated.h"

namespace {

class TestFailure : public std::runtime_error {
public:
	explicit TestFailure(const std::string& m) : std::runtime_error(m) {}
};

[[noreturn]] void fail(const std::string& what) { throw TestFailure(what); }

// Deterministic stimulus source; the seed is part of the pinned schedule.
struct Lcg {
	uint64_t s;
	explicit Lcg(uint64_t seed) : s(seed) {}
	uint32_t next() {
		s = s * 6364136223846793005ULL + 1442695040888963407ULL;
		return uint32_t(s >> 33);
	}
	uint32_t below(uint32_t n) { return n ? next() % n : 0; }
	bool coin(unsigned pct) { return below(100) < pct; }
};

constexpr uint32_t kCenPeriod = 4;   // ce_16: one clk in four (64 MHz / 4)
constexpr unsigned kLineClks  = 64;  // synthetic line length

class DiffBench {
public:
	Vasic_ga_diff_top dut;
	Lcg rng;
	uint64_t cyc = 0;

	// Inputs (shared).
	uint8_t cen_phase = 0;
	bool fast = false;
	bool reset_n = false;
	bool plus_unlocked = false;
	uint16_t addr = 0;
	uint8_t data = 0;
	bool mreq_n = true, m1_n = true, rd_n = true, iorq_n = true;
	bool hsync_i = true, vsync_i = true, dispen = false;

	// Synthetic CRTC timing state.
	unsigned hcount = 0;        // clks into current line
	bool in_hsync = false;
	unsigned lines_since_vsync = 0;
	unsigned frame_lines = 52;
	unsigned vsync_lines_left = 0;
	bool vsync_suppressed = false;

	DiffBench(uint64_t seed) : dut("asic_ga_diff_top"), rng(seed) {}

	void apply_inputs() {
		dut.clk = 0;
		dut.cen_16 = (cen_phase == 0);
		dut.fast = fast;
		dut.RESET_N = reset_n;
		dut.plus_unlocked = plus_unlocked;
		dut.A = addr;
		dut.D = data;
		dut.MREQ_N = mreq_n;
		dut.M1_N = m1_n;
		dut.RD_N = rd_n;
		dut.IORQ_N = iorq_n;
		dut.HSYNC_I = hsync_i;
		dut.VSYNC_I = vsync_i;
		dut.DISPEN = dispen;
	}

	void step_synthetic_crtc(bool allow_vsync = true) {
		// HSYNC_I: active-low pulse of 8 clks every kLineClks; VSYNC_I an
		// active-low burst of three whole lines every frame_lines lines.
		if (hcount == 0) {
			in_hsync = true;
			if (vsync_lines_left > 0) {
				--vsync_lines_left;
				if (vsync_lines_left == 0) lines_since_vsync = 0; // re-arm
			} else {
				++lines_since_vsync;
				if (allow_vsync && !vsync_suppressed &&
				    lines_since_vsync >= frame_lines)
					vsync_lines_left = 3;
			}
		}
		if (in_hsync && hcount >= 8) in_hsync = false;
		hsync_i = !in_hsync;
		vsync_i = !(vsync_lines_left > 0);
		dispen = (hcount >= 16 && hcount < 48);
		hcount = (hcount + 1) % kLineClks;
	}

	void tick(bool advance_crtc = true) {
		apply_inputs();
		dut.eval();
		compare("negedge");
		dut.clk = 1;
		dut.eval();
		compare("posedge");
		++cyc;
		cen_phase = (cen_phase + 1) % kCenPeriod;
		if (advance_crtc) step_synthetic_crtc();
	}

	void run(unsigned n, bool advance_crtc = true) {
		for (unsigned i = 0; i < n; ++i) tick(advance_crtc);
	}

	// Re-evaluate combinational outputs after changing input fields
	// without clocking (used by directed bus probes).
	void settle() {
		apply_inputs();
		dut.eval();
	}

	void power_on_reset(unsigned ticks = 12) {
		reset_n = false;
		mreq_n = m1_n = rd_n = iorq_n = true;
		run(ticks);
		reset_n = true;
		run(ticks);
	}

#define CMP(sig) \
	do { \
		if (dut.ga_##sig != dut.as_##sig) { \
			char buf[256]; \
			snprintf(buf, sizeof buf, \
			         "cycle %llu (%s): %s mismatch ga40010=%d asic_ga_timing=%d" \
			         " [cen=%u fast=%d rstN=%d mreq=%d m1=%d rd=%d iorq=%d" \
			         " HS_I=%d VS_I=%d HS_O=%d/%d VS_O=%d/%d]", \
			         (unsigned long long)cyc, phase, #sig, \
			         (int)dut.ga_##sig, (int)dut.as_##sig, \
			         cen_phase, (int)fast, (int)reset_n, (int)mreq_n, \
			         (int)m1_n, (int)rd_n, (int)iorq_n, \
			         (int)hsync_i, (int)vsync_i, \
			         (int)dut.ga_HSYNC_O, (int)dut.as_HSYNC_O, \
			         (int)dut.ga_VSYNC_O, (int)dut.as_VSYNC_O); \
			fail(std::string(buf)); \
		} \
	} while (0)

	void compare(const char* phase) {
		// The netlist reference is the locked/classic GA. Directed Plus-only
		// vectors deliberately suppress lockstep comparison after unlock and
		// assert the replica's register state directly.
		if (plus_unlocked) return;
		CMP(CCLK); CMP(CCLK_EN_P); CMP(CCLK_EN_N);
		CMP(PHI_N); CMP(PHI_EN_N); CMP(PHI_EN_P);
		CMP(RAS_N); CMP(CASAD_N); CMP(CAS_N);
		CMP(READY); CMP(CPU_N); CMP(MWE_N); CMP(E244_N);
		CMP(ROMEN_N); CMP(RAMRD_N); CMP(ROM);
		CMP(HSYNC_O); CMP(VSYNC_O); CMP(SYNC_N);
		CMP(INT_N); CMP(VBLANK); CMP(MODE);
	}
#undef CMP

	// The 80-bit INKR port arrives as three 32-bit words (little-endian
	// word order); extract select entry k's five-bit payload.
	static uint8_t inkr_entry(const Vasic_ga_diff_top& d, unsigned k) {
		const unsigned lo = k * 5;
		if (lo >= 64) return uint8_t((d.as_INKR_O[2] >> (lo - 64)) & 0x1F);
		const uint64_t low = uint64_t(uint32_t(d.as_INKR_O[0])) |
		                     (uint64_t(uint32_t(d.as_INKR_O[1])) << 32);
		if (lo + 5 <= 64) return uint8_t((low >> lo) & 0x1F);
		const unsigned n_low = 64 - lo; // bits from the low half
		uint8_t v = uint8_t(low >> lo) & uint8_t((1u << n_low) - 1);
		v |= uint8_t(uint32_t(d.as_INKR_O[2]) & ((1u << (5 - n_low)) - 1)) << n_low;
		return v;
	}
	static bool inkr_zero(const Vasic_ga_diff_top& d) {
		return !d.as_INKR_O[0] && !d.as_INKR_O[1] && !d.as_INKR_O[2];
	}

	//------------------------------------------------------------------
	// Randomised Z80-flavoured bus traffic.
	//------------------------------------------------------------------
	enum class Cyc { Idle, Fetch, MemRd, MemWr, IoRd, IoWr, IntAck };

	Cyc pick_cycle(bool intack_storm) {
		uint32_t r = rng.below(intack_storm ? 100 : 100);
		if (intack_storm) {
			if (r < 25) return Cyc::IntAck;
			if (r < 55) return Cyc::IoWr;
			if (r < 70) return Cyc::IoRd;
		}
		if (r < 30) return Cyc::Idle;
		if (r < 50) return Cyc::Fetch;
		if (r < 62) return Cyc::MemRd;
		if (r < 72) return Cyc::MemWr;
		if (r < 82) return Cyc::IoRd;
		if (r < 95) return Cyc::IoWr;
		return Cyc::IntAck;
	}

	uint8_t pick_ga_data() {
		uint32_t r = rng.below(100);
		if (r < 20) return uint8_t(rng.below(0x20));             // ink select
		if (r < 45) return uint8_t(0xC0 | rng.below(0x20));      // border range
		if (r < 70) return uint8_t(0x40 | rng.below(0x20));      // ink range
		if (r < 90) return uint8_t(0x80 | rng.below(0x20));      // RMR range
		return uint8_t(rng.next());
	}

	void run_cycle(Cyc c) {
		switch (c) {
		case Cyc::Idle:
			mreq_n = m1_n = rd_n = iorq_n = true;
			addr = uint16_t(rng.next());
			data = uint8_t(rng.next());
			run(1 + rng.below(3));
			break;
		case Cyc::Fetch:
			m1_n = mreq_n = rd_n = false; iorq_n = true;
			addr = uint16_t(rng.next());
			run(3);
			mreq_n = m1_n = rd_n = true;
			break;
		case Cyc::MemRd:
			mreq_n = rd_n = false; m1_n = iorq_n = true;
			addr = uint16_t(rng.next());
			run(2 + rng.below(2));
			mreq_n = rd_n = true;
			break;
		case Cyc::MemWr:
			mreq_n = false; m1_n = rd_n = iorq_n = true;
			addr = uint16_t(rng.next());
			data = uint8_t(rng.next());
			run(2 + rng.below(2));
			mreq_n = true;
			break;
		case Cyc::IoRd:
			iorq_n = rd_n = false; m1_n = mreq_n = true;
			addr = rng.coin(50) ? uint16_t(0x7F00 | rng.below(0x100)) : uint16_t(0xFB00 | rng.below(0x100));
			run(2 + rng.below(3));
			iorq_n = rd_n = true;
			break;
		case Cyc::IoWr:
			iorq_n = false; m1_n = mreq_n = rd_n = true;
			addr = uint16_t(0x7F00 | rng.below(0x100));
			data = pick_ga_data();
			run(3 + rng.below(4));
			iorq_n = true;
			break;
		case Cyc::IntAck:
			m1_n = iorq_n = false; mreq_n = rd_n = true;
			run(2);
			m1_n = iorq_n = true;
			break;
		}
		run(rng.below(3)); // inter-cycle idle gap
	}
};

//----------------------------------------------------------------------
// d01: mixed traffic, steady frames near the 52-line interrupt cadence.
//----------------------------------------------------------------------
void d01_lockstep_mixed(DiffBench& b) {
	b.power_on_reset();
	b.frame_lines = 52;
	for (unsigned i = 0; i < 4000; ++i) {
		b.frame_lines = 47 + b.rng.below(12);
		// Randomised no-wait mode inside the lockstep traffic: today `fast`
		// only widens the register-latch window in both modules, but any
		// future fast-dependent divergence in shared strobes must surface
		// here rather than in hardware.
		b.fast = b.rng.coin(20);
		b.run_cycle(b.pick_cycle(false));
	}
	b.fast = false;
}

//----------------------------------------------------------------------
// d02: reset pulses and interrupt-acknowledge storms interleaved with
// GA-port writes, exercising U204 restarts, READY/CAS masking and the
// interrupt acknowledge latch.
//----------------------------------------------------------------------
void d02_lockstep_reset_ack_storm(DiffBench& b) {
	b.power_on_reset();
	b.frame_lines = 52;
	for (unsigned i = 0; i < 1500; ++i) {
		b.run_cycle(b.pick_cycle(true));
		if (b.rng.coin(12)) {
			b.mreq_n = b.m1_n = b.rd_n = b.iorq_n = true;
			b.reset_n = false;
			b.run(1 + b.rng.below(6));
			b.reset_n = true;
			b.run(b.rng.below(4));
		}
	}
}

//----------------------------------------------------------------------
// d03: lost-sync park. Long stretches without VSYNC drive hcnt to its
// parked end state; resumption must re-lock identically.
//----------------------------------------------------------------------
void d03_lockstep_lost_sync(DiffBench& b) {
	b.power_on_reset();
	b.frame_lines = 52;
	b.vsync_suppressed = true;
	b.run(40 * kLineClks);
	b.vsync_suppressed = false;
	b.lines_since_vsync = 0;
	b.run(10 * kLineClks);
	b.vsync_suppressed = true;
	b.run(30 * kLineClks);
	b.vsync_suppressed = false;
	b.lines_since_vsync = 0;
	for (unsigned i = 0; i < 300; ++i) b.run_cycle(b.pick_cycle(false));
}

//----------------------------------------------------------------------
// d04: directed U204 restart. U204's reset term fires when an
// interrupt-acknowledge-flavoured bus state (M1/IORQ/RD all low) is held
// ACROSS reset — d02 deliberately idles the bus before pulsing reset, so
// this term was uncovered. The sequencer ring must restart from bit 1 in
// both modules and every shared strobe must stay edge-equal through the
// restart, the ack release, and the following traffic.
//----------------------------------------------------------------------
void d04_lockstep_u204_restart(DiffBench& b) {
	b.power_on_reset();
	b.frame_lines = 52;
	for (unsigned i = 0; i < 12; ++i) {
		// Ordinary traffic between restarts.
		for (unsigned j = 0; j < 5; ++j) b.run_cycle(b.pick_cycle(false));
		// Hold an interrupt-acknowledge cycle while reset asserts.
		b.mreq_n = true;
		b.m1_n = b.iorq_n = b.rd_n = false;
		b.fast = b.rng.coin(30);
		b.reset_n = false;
		b.run(2 + b.rng.below(8)); // both held through several ring phases
		b.reset_n = true;
		b.run(2 + b.rng.below(4)); // reset released first, ack still held
		b.m1_n = b.iorq_n = b.rd_n = true;
		b.fast = false;
		b.run(4);
	}
}

//----------------------------------------------------------------------
// Directed helpers operating on the replica alone (fast-path register
// window: with fast=1, reg_latch holds whenever ~E244_N, i.e. during the
// sequencer phases with S[2]&S[3]; holding a full ring guarantees
// capture). Derived from the port equations in the RTL header source.
//----------------------------------------------------------------------
void ga_write(DiffBench& b, uint8_t value) {
	b.m1_n = true;
	b.iorq_n = false;
	b.rd_n = true;
	b.addr = 0x7F10;
	b.data = value;
	b.fast = true;
	// The fast-path latch window needs a sequencer phase with S[2]&S[3]
	// set; the ring misses at most four consecutive phases (00,01,03,07),
	// so holding ten full character clocks guarantees capture.
	b.run(40);
	b.iorq_n = true;
	b.fast = false;
	b.run(4);
}

// Raise INT_N through an acknowledge cycle held right at the current
// assertion; returns once INT_N is observed high again.
void ack_interrupt(DiffBench& b) {
	b.m1_n = false;
	b.iorq_n = false;
	b.mreq_n = true;
	b.rd_n = true;
	unsigned guard = 0;
	while (b.dut.as_INT_N != 1) {
		b.tick();
		if (++guard > 8) fail("ack: INT_N did not rise on acknowledge");
	}
	b.m1_n = b.iorq_n = true;
	b.run(2);
}

uint64_t extract_int_period(DiffBench& b) {
	// The interrupt counter self-clears on the 52nd shaped line and INT_N
	// stays asserted until acknowledged. Both falling edges below are
	// genuine fires of the same strictly periodic event, so their
	// cycle-stamp distance must be exactly 52 synthetic lines regardless
	// of when the acknowledges land.
	b.run(kLineClks * 60); // settle past any pending state
	if (b.dut.as_INT_N == 0) ack_interrupt(b); // clear power-on-low level
	while (b.dut.as_INT_N != 0) {
		b.tick();
		if (b.cyc > 1u << 21) fail("r02: INT_N never asserted");
	}
	uint64_t first_fall = b.cyc;
	ack_interrupt(b);
	while (b.dut.as_INT_N != 0) {
		b.tick();
		if (b.cyc - first_fall > (1u << 21)) fail("r02: second INT never arrived");
	}
	return b.cyc - first_fall;
}

//----------------------------------------------------------------------
// r01: legacy register payloads — ink select, INKR packing, border gate,
// screen mode, ROM mapping.
//----------------------------------------------------------------------
void r01_register_payloads(DiffBench& b) {
	b.power_on_reset();

	ga_write(b, 0x83); // RMR: hromen=0 lromen=0 mode=11
	if (b.dut.as_MODE != 3) fail("r01: MODE should be 3 after RMR 0x83");
	if (b.dut.as_GAMODE_O != 3) fail("r01: GAMODE_O should track RMR mode");

	// RMR D3/D2 store ACTIVE-LOW ROM enables: writing 8Dh sets lromen=hromen=1
	// (no address maps ROM) with a distinctive mode=1 to prove the capture.
	ga_write(b, 0x8D);
	if (b.dut.as_MODE != 1) fail("r01: MODE should be 1 after RMR 0x8D");
	// Probe all three zones with MREQ+RD active; every region must read RAM.
	b.mreq_n = false; b.rd_n = false; b.iorq_n = true; b.m1_n = true;
	struct Probe { uint16_t a; };
	const Probe probes[] = {{0x0000}, {0x4000}, {0xC000}};
	for (const auto& p : probes) {
		b.addr = p.a;
		b.settle();
		if (b.dut.as_ROMEN_N != 1 || b.dut.as_RAMRD_N != 0)
			fail(std::string("r01: expected RAM at ") + std::to_string(p.a));
	}
	// Clear both enables (D3=D2=0) with mode=2: upper and lower regions map ROM.
	ga_write(b, 0x82); // hromen=0 lromen=0 mode=2
	if (b.dut.as_MODE != 2) fail("r01: MODE should be 2 after RMR 0x82");
	b.mreq_n = false; b.rd_n = false;
	b.addr = 0xC000;
	b.settle();
	if (b.dut.as_ROMEN_N != 0 || b.dut.as_RAMRD_N != 1)
		fail("r01: expected upper ROM after hromen=0");
	b.addr = 0x0000;
	b.settle();
	if (b.dut.as_ROMEN_N != 0)
		fail("r01: expected lower ROM after lromen=0");
	// Middle bank never maps ROM regardless.
	b.addr = 0x4000;
	b.settle();
	if (b.dut.as_ROMEN_N != 1) fail("r01: middle bank must stay RAM");
	b.mreq_n = b.rd_n = true;
	b.run(2);

	// Ink select 3, then PENR write 0x15 into slot 3.
	ga_write(b, 0x03);
	ga_write(b, 0x40 | 0x15);
	uint8_t got = DiffBench::inkr_entry(b.dut, 3);
	if (got != 0x15) fail("r01: INKR[3] should be 0x15");

	// Ink select with bit4 set switches 0x4X..0x7F writes to BORDER.
	ga_write(b, 0x10);
	ga_write(b, 0x40 | 0x06);
	if (b.dut.as_BORDER_O != 0x06) fail("r01: BORDER should be 6");
	// ...and blocks further INKR updates while bit4 set.
	ga_write(b, 0x41); // would-be ink write, must land nowhere
	if (DiffBench::inkr_entry(b.dut, 3) != 0x15)
		fail("r01: border-mode write leaked into INKR");

	// Slot writes are mod-16 through the select nibble.
	ga_write(b, 0x05);
	ga_write(b, 0x40 | 0x0A);
	got = DiffBench::inkr_entry(b.dut, 5);
	if (got != 0x0A) fail("r01: INKR[5] should be 0x0A");
}

//----------------------------------------------------------------------
// r02: the shaped-line interrupt fires every 52 synthetic lines and an
// interrupt-acknowledge-style cycle raises INT_N.
//----------------------------------------------------------------------
void r02_interrupt_cadence_and_ack(DiffBench& b) {
	b.power_on_reset();
	b.frame_lines = 52;

	// Phase A — pure counter cadence. With VSYNC suppressed, no shaped
	// VSYNC edge (intcntclr_4) intervenes: the 6-bit counter self-clears
	// when its next value hits %110100 (=52), so successive assertions are
	// exactly 52 synthetic lines apart. This is the documented GA interrupt
	// contract (40010-simplified syncgen section; see RTL header).
	b.vsync_suppressed = true;
	uint64_t period = extract_int_period(b);
	if (period != 52u * kLineClks)
		fail("r02: expected INT period 52 lines (" + std::to_string(52u * kLineClks) +
		     " clks), measured " + std::to_string(period));

	// Phase B — VSYNC re-alignment. Re-enabling frames whose burst begins
	// on the wrap line re-phases the counter through intcntclr_4: the
	// shaped VSYNC_O rises three lines later (hcnt walk 01->06->07->04
	// after the res1 zeroing), clearing the counter there, so the locked
	// regime asserts every frame_lines+3 lines for this generator. The
	// first fire after re-enabling still belongs to the old phase, so skip
	// one fire before measuring.
	b.vsync_suppressed = false;
	while (b.dut.as_INT_N != 0) b.tick();
	ack_interrupt(b);
	while (b.dut.as_INT_N != 0) b.tick(); // old-phase fire
	ack_interrupt(b);
	while (b.dut.as_INT_N != 0) b.tick();
	uint64_t t1 = b.cyc;
	ack_interrupt(b);
	while (b.dut.as_INT_N != 0) b.tick();
	if (b.cyc - t1 != 55u * kLineClks)
		fail("r02: expected VSYNC-realigned period 55 lines (" +
		     std::to_string(55u * kLineClks) + " clks), measured " +
		     std::to_string(b.cyc - t1));

	// An interrupt-acknowledge-style cycle raises INT_N.
	while (b.dut.as_INT_N != 0) b.tick();
	ack_interrupt(b);
	if (b.dut.as_INT_N != 1)
		fail("r02: INT_N should be high after the acknowledge");

	// RMR bit 4 also clears a live interrupt.
	while (b.dut.as_INT_N != 0) b.tick();
	ga_write(b, 0x90); // RMR with bit4 set, mode/romens unchanged
	if (b.dut.as_INT_N != 1) fail("r02: RMR bit4 should clear INT_N");
}

//----------------------------------------------------------------------
// r03: defined power-up state (border 16, mode 0, inks cleared) survives
// the release of RESET_N; border keeps its reset value until written.
// The second half proves the INKR/ink-select clears are RTL resets rather
// than simulator zero-init: written entries are scrubbed by a reset pulse
// and the ink select returns to slot 0.
//----------------------------------------------------------------------
void r03_powerup_state(DiffBench& b) {
	b.reset_n = false;
	b.mreq_n = b.m1_n = b.rd_n = b.iorq_n = true;
	b.run(16);
	b.reset_n = true;
	b.run(16);
	if (b.dut.as_BORDER_O != 0x10)
		fail("r03: border should power up at hardware colour 16");
	if (b.dut.as_MODE != 0 || b.dut.as_GAMODE_O != 0)
		fail("r03: screen mode should power up at 0");
	if (!DiffBench::inkr_zero(b.dut))
		fail("r03: INKR entries should power up cleared (RTL reset)");

	// Park ink select on slot 15 and mark it, then reset again: the mark
	// must be scrubbed even though it was written before the reset.
	ga_write(b, 0x0F);            // ink select = 15
	ga_write(b, 0x40 | 0x1F);     // INKR[15] = 0x1F
	if (DiffBench::inkr_entry(b.dut, 15) != 0x1F)
		fail("r03: INKR[15] should hold 0x1F before the reset");
	b.reset_n = false;
	b.run(16);
	b.reset_n = true;
	b.run(16);
	if (!DiffBench::inkr_zero(b.dut))
		fail("r03: INKR must be cleared by an explicit RTL reset, not only at time 0");

	// With ink select back at its reset value 0, a border-gated-off ink
	// write lands in slot 0 — proving the select itself was reset.
	ga_write(b, 0x40 | 0x07);
	if (DiffBench::inkr_entry(b.dut, 0) != 0x07)
		fail("r03: post-reset ink select should be 0 (write landed in slot 0)");
	if (DiffBench::inkr_entry(b.dut, 15) != 0x00)
		fail("r03: slot 15 must stay clear when ink select was reset to 0");
}

//----------------------------------------------------------------------
// r04: once the Plus ASIC is unlocked, 101xxxxx belongs to RMR2 and must
// not also update the legacy GA control register (mode, ROM enables, IRQ).
//----------------------------------------------------------------------
void r04_unlocked_rmr2_is_not_legacy_rmr(DiffBench& b) {
	b.power_on_reset();

	ga_write(b, 0x83); // mode 3, both ROM windows enabled
	if (b.dut.as_MODE != 3 || b.dut.as_GAMODE_O != 3)
		fail("r04: locked setup RMR did not select mode 3");

	b.plus_unlocked = true;
	ga_write(b, 0xB8); // RMR2 position 11/page 0; legacy view would change mode/ROM
	if (b.dut.as_MODE != 3 || b.dut.as_GAMODE_O != 3)
		fail("r04: unlocked RMR2 leaked into legacy GA mode");

	// The prior 0x83 left both legacy ROM windows enabled. An erroneous
	// legacy interpretation of 0xB8 would disable the upper window via D3.
	b.mreq_n = false;
	b.rd_n = false;
	b.iorq_n = true;
	b.m1_n = true;
	b.addr = 0xC000;
	b.settle();
	if (b.dut.as_ROMEN_N != 0 || b.dut.as_RAMRD_N != 1)
		fail("r04: unlocked RMR2 altered legacy upper-ROM enable");
	b.addr = 0x0000;
	b.settle();
	if (b.dut.as_ROMEN_N != 0)
		fail("r04: unlocked RMR2 altered legacy lower-ROM enable");
}

struct TestCase {
	const char* name;
	bool differential; // true = lockstep comparison active throughout
	void (*fn)(DiffBench&);
};

const std::array<TestCase, 8> kTests = {{
	{"d01 lockstep mixed traffic",            true,  d01_lockstep_mixed},
	{"d02 lockstep reset/ack storm",          true,  d02_lockstep_reset_ack_storm},
	{"d03 lockstep lost-sync park",           true,  d03_lockstep_lost_sync},
	{"d04 lockstep directed U204 restart",    true,  d04_lockstep_u204_restart},
	{"r01 legacy register payloads",          false, r01_register_payloads},
	{"r02 interrupt cadence and ack",         false, r02_interrupt_cadence_and_ack},
	{"r03 defined power-up state",            false, r03_powerup_state},
	{"r04 unlocked RMR2 does not update legacy GA", false, r04_unlocked_rmr2_is_not_legacy_rmr},
}};

} // namespace

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	unsigned failed = 0;
	for (const auto& t : kTests) {
		DiffBench b(0xa51c20260824ULL + (&t - kTests.data()));
		try {
			t.fn(b);
			std::printf("PASS: %s\n", t.name);
		} catch (const TestFailure& e) {
			++failed;
			std::printf("FAIL: %s: %s\n", t.name, e.what());
		}
	}
	if (failed) {
		std::printf("%u/%zu tests FAILED\n", failed, kTests.size());
		return 1;
	}
	std::printf("All %zu asic_ga_timing differential/directed tests passed\n", kTests.size());
	return 0;
}
