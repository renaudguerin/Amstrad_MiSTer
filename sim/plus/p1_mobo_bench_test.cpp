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
// m5  ASIC-page bus writes land in sprite RAM (one nibble per CPU byte,
//     low bits stored), sprite regs and palette;
// m6  INT-acknowledge presents (IVR & F8) | source on the data bus;
// m7  classic mode keeps every Plus term inert with the page forced on;
// m8  end-to-end sprite vector: after the script enables sprite 0
//     (X=358, Y=16, MAG x1/x1), a bench-CPU auto-fill phase writes the
//     whole 16x16 image as alternating colours 10/5, plus pal[26]={R1,
//     G F,B2} / pal[21]={R3,G6,B4}. Paper expectations ([KT] compare
//     formulas; reference S5/S6; engine emission swap; asic_video's
//     registered RGB output lagging the engine plane by one dot):
//     exactly one 16-dot SPR_EN window per compare line vline 16..31
//     and nowhere else; window dot k>=1 carries source pixel k-1, so
//     odd k shows red_o=1 green_o=F blue_o=2 and even k shows 3/6/4
//     through the production bus -> regs -> fetch-port -> engine ->
//     video chain.
// m12 the 12-bit ASIC palette reaches the top-level RGB pins (HF-2): the
//     script's &6420 write leaves border entry 16 at {G0,R2,B1}, a level
//     the legacy 27-colour ROM cannot produce.
// m13 integrates the real sprite leaf and video compositor at both display
//     edges: X=0/-8 align with the locked-ASIC's delayed display origin;
//     X=640/+767 still arm in the leaf, standard R1=40 masks them with border,
//     and widened R1=50 exposes the same sprite pixels.

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

	explicit MoboBench(bool plus) : dut("p1_mobo_bench_top") {
		dut.clk = 0;
		dut.reset = 1;
		dut.asic_page_on = 0;
		dut.plus_mode_i = plus ? 1 : 0;
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
	auto* cpu_read_bf_r14() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__CPU__DOT__dbg_read_bf_r14;
	}
	auto* cpu_read_be_r14() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__CPU__DOT__dbg_read_be_r14;
	}
	auto* cpu_read_bf_r15() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__CPU__DOT__dbg_read_bf_r15;
	}
	auto* cpu_read_status2() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__CPU__DOT__dbg_read_status2;
	}
	auto* spr_ram_even() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_page__DOT__spr_ram_inst__DOT__even_bank__DOT__mem[0];
	}
	auto* spr_ram_odd() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_page__DOT__spr_ram_inst__DOT__odd_bank__DOT__mem[0];
	}
	auto* pal_word(unsigned e) {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_page__DOT__pal[0];
	}
	auto* x_lo(unsigned n) {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_page__DOT__spr_x_lo[n];
	}
	auto* x_hi(unsigned n) {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_page__DOT__spr_x_hi[n];
	}
	auto* asic_r0() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_vid__DOT__R0_h_total;
	}
	auto* asic_r1() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_vid__DOT__R1_h_displayed;
	}
	auto* asic_r2() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_vid__DOT__R2_h_sync_pos;
	}
	auto* asic_sscr() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_page__DOT__sscr_r;
	}
	auto* asic_hcc() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_vid__DOT__hcc;
	}
	auto* asic_pix_cnt() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_vid__DOT__pix_cnt;
	}
	auto* splt_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_splt;
	}
	auto* ssa_hi_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_ssa_hi;
	}
	auto* ssa_lo_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_ssa_lo;
	}
	auto* sscr_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__asic_sscr;
	}
	auto* plus_ra_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__plus_ra;
	}
	auto* plus_rc_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__plus_rc;
	}
	auto* sar0_lo_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__dma_sar0_lo;
	}
	auto* sar0_hi_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__dma_sar0_hi;
	}
	auto* ppr0_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__dma_ppr0;
	}
	auto* dcsr_ena_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__dma_dcsr_ena;
	}
	auto* sar0_cur_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__dma_sound__DOT__sar_cur[0];
	}
	auto* dma_int_req_tap() {
		return &dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__plus_dma_int_req;
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
	MoboBench b(true);

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
			if (*b.cpu_read_bf_r14() != 0x1A || *b.cpu_read_be_r14() != 0x1A)
				fail("m9: &BE/&BF did not return the same selected R14 value");
			if (*b.cpu_read_bf_r15() != 0x79)
				fail("m9: CRTC data-port IN did not write the live bus byte to R15");
			if ((*b.cpu_read_status2() & 0x50) != 0x10)
				fail("m9: CRTC select-port IN did not select STATUS2");
			if (b.inkr_entry(b.inkr_tap(), 6) != 0x06)
				fail("m9: Gate-Array IN trap did not update INKR[6]");
			done_checked = true;
			std::printf("PASS m1: plus_mode motherboard boots; CRTC3 programmed over the bus\n");
			std::printf("PASS m2: GA RMR writes reach asic_video via GAMODE_O (3 -> 2)\n");
			std::printf("PASS m3: INKR[5]/border payloads reach asic_video inputs\n");
			std::printf("PASS m9: CRTC3 read mux, dual read ports and IN traps reach CRTC/GA state\n");
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
		// The byte settles on the first clock edge inside the window
		// (ack_pending latch); the CPU samples at cycle end, so require
		// stability only after that settle, through window close.
		static bool prev_v = false;
		static unsigned vticks = 0;
		static uint8_t settled_b = 0;
		if (!b.dut.vec_valid_o) { prev_v = false; vticks = 0; }
		else {
			++vticks;
			if (vticks == 5) settled_b = b.dut.vec_byte_o; // post-settle baseline
			if (vticks > 5 && b.dut.vec_byte_o != settled_b)
				fail("m6: vector byte changed mid-acknowledge at cyc " +
				     std::to_string(b.cyc) + " (" +
				     std::to_string(settled_b) + " -> " +
				     std::to_string(b.dut.vec_byte_o) + ")");
			prev_v = true;
		}
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
			const auto* ram_even = b.spr_ram_even();
			const auto* ram_odd = b.spr_ram_odd();
			// Sprite 0 image comes from the m8 auto-fill phase: even
			// offsets written 0xDA -> nibble A, odd offsets 0x85 ->
			// nibble 5, so the low-nibble mask is exercised by every
			// one of the 256 writes.
			if (ram_even[0x000] != 0xA || ram_odd[0x000] != 0x5 ||
			    ram_even[0x080] != 0xE)
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

			// P6 m10: screen split and soft scroll register writes
			if (*b.splt_tap() != 0x05)
				fail("m10: SPLT register not written");
			if (*b.ssa_hi_tap() != 0x24 || *b.ssa_lo_tap() != 0x00)
				fail("m10: SSA register not written");
			if (*b.sscr_tap() != 0x34)
				fail("m10: SSCR register not written");
			// Check that plus_ra reflects vertical scanline offset 3: plus_ra == (plus_rc + 3) & 7
			const uint8_t exp_ra = (*b.plus_rc_tap() + 3) & 7;
			if ((*b.plus_ra_tap() & 7) != exp_ra)
				fail("m10: plus_ra does not reflect SSCR vertical scanline offset");
			std::printf("PASS m10: Screen split and scroll registers reached ASIC page and video pipeline\n");

			// P7 m11: DMA register writes
			if (*b.sar0_lo_tap() != 0x34 || *b.sar0_hi_tap() != 0x12)
				fail("m11: SAR0 register not written correctly (got lo=" +
				     std::to_string(*b.sar0_lo_tap()) + " hi=" + std::to_string(*b.sar0_hi_tap()) + ")");
			if (*b.sar0_cur_tap() < 0x1234)
				fail("m11: dma_sound sar_cur[0] not initialized from 0x1234 (got " + std::to_string(*b.sar0_cur_tap()) + ")");
			if (*b.ppr0_tap() != 0x05)
				fail("m11: PPR0 register not written (got " + std::to_string(*b.ppr0_tap()) + ")");
			if ((*b.dcsr_ena_tap() & 1) != 1)
				fail("m11: DCSR ch0 enable not written (got " + std::to_string(*b.dcsr_ena_tap()) + ")");
			std::printf("PASS m11: DMA registers (SAR0, PPR0, DCSR) reached ASIC page and DMA sound engine\n");
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

	//------------------------------------------------------------------
	// m8: end-to-end sprite vector (expectations derived above, header).
	// The engine asserts SPR_EN combinationally on the hp==X dot itself;
	// asic_video registers RGB, so the output sampled against engine dot
	// k carries pixel k-1 (one-dot pairing, see the per-dot expectation).
	//------------------------------------------------------------------
	{
		auto* ce16   = &b.dut.rootp->p1_mobo_bench_top__DOT__ce_16;
		auto* en_tap = &b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__plus_spr_en;
		auto* vc_tap = &b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__plus_vc;
		auto* rc_tap = &b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__plus_rc;

		bool in_win = false;
		unsigned win_pos = 0;
		unsigned win_vline = 0;
		bool vline_moved = false;
		unsigned windows_done = 0;
		bool width_bad = false;
		bool rgb_bad = false;
		std::string first_bad;
		unsigned win_count[128] = {0};
		const uint64_t deadline = b.cyc + 2000000;
		// If SPR_EN is already high at scan entry that first window is a
		// partial: observe one full quiet period before trusting records.
		bool seen_quiet = false;
		bool discard_current = true;

		while (windows_done < 48 && b.cyc < deadline) {
			b.tick();
			if (!*ce16)
				continue;
			const bool en = *en_tap != 0;
			const unsigned cur_vl =
			    unsigned((*vc_tap << 3) | (*rc_tap & 7));
			if (!en)
				seen_quiet = true;
			if (en && !in_win) {
				in_win = true;
				win_pos = 0;
				win_vline = cur_vl;
				vline_moved = false;
				discard_current = !seen_quiet;
			}
			else if (in_win && !en) {
				in_win = false;
				// Seam-straddling windows are skipped outright; the
				// closing sample's own line counts too - if SPR_EN fell
				// across a line boundary the window never had a home
				// line and width means nothing.
				if (cur_vl != win_vline)
					vline_moved = true;
				if (!discard_current && !vline_moved) {
					if (win_pos != 16) {
						width_bad = true;
						if (first_bad.empty())
							first_bad = "window width " +
							            std::to_string(win_pos) +
							            " at vline " +
							            std::to_string(win_vline);
					}
					else {
						win_count[win_vline & 127]++;
						windows_done++;
					}
				}
			}
			if (!in_win)
				continue;
			if (win_pos > 0 && cur_vl != win_vline)
				vline_moved = true; // straddles a seam: skip, not a vector
			// asic_video registers RGB on PIXEN, so the output sampled in
			// the shadow of engine dot k carries dot k-1: window dot 0
			// still shows the pre-window background (unchecked - it is
			// p1_video's ink chain, not the sprite plane), and dot k>=1
			// shows source pixel k-1: odd k -> colour 10 from pal[26]
			// {R=1,G=F,B=2}; even k -> colour 5 from pal[21]
			// {R=3,G=6,B=4}.
			if (!discard_current && win_pos >= 1) {
				const uint8_t er = (win_pos & 1) ? 0x1 : 0x3;
				const uint8_t eg = (win_pos & 1) ? 0xF : 0x6;
				const uint8_t eb = (win_pos & 1) ? 0x2 : 0x4;
				if (b.dut.red_o != er || b.dut.green_o != eg ||
				    b.dut.blue_o != eb) {
					rgb_bad = true;
					if (first_bad.empty())
						first_bad = "dot " + std::to_string(win_pos) +
						    " vline " + std::to_string(win_vline) + ": got {" +
						    std::to_string(b.dut.red_o) + "," +
						    std::to_string(b.dut.green_o) + "," +
						    std::to_string(b.dut.blue_o) + "} want {" +
						    std::to_string(er) + "," + std::to_string(eg) +
						    "," + std::to_string(eb) + "}";
				}
			}
			win_pos++;
		}
		if (width_bad || rgb_bad)
			fail("m8: " + first_bad);
		if (in_win || windows_done < 48)
			fail("m8: scan ended mid-window or timed out after " +
			     std::to_string(windows_done) + " complete windows" +
			     (first_bad.empty() ? std::string() : "; " + first_bad));
		// 48 accepted windows with 16 emitting lines per frame is exactly
		// three whole frames, so each line must carry exactly three
		// windows - duplicates and gaps both fail.
		for (unsigned vl = 0; vl < 128; ++vl) {
			if (vl >= 16 && vl <= 31) {
				if (win_count[vl] != 3)
					fail("m8: compare line " + std::to_string(vl) +
					     " carries " + std::to_string(win_count[vl]) +
					     " sprite windows, want exactly 3");
			}
			else if (win_count[vl] != 0) {
				fail("m8: compare line " + std::to_string(vl) +
				     " shows a sprite window outside Y..Y+15");
			}
		}
		std::printf("PASS m8: sprite plane end-to-end - %u windows on lines "
			            "16..31, exact alternating palette payloads on RGB pins\n",
			            windows_done);
	}

	//------------------------------------------------------------------
	// m12: the 12-bit ASIC palette reaches the RGB pins (HF-2).
	//
	// Script step 23 wrote legacy border = hw colour 4, which asic_regs
	// shadows into palette entry 16 as {G0,R0,B6}; step 30 then wrote
	// &6420 = 0x21 (even byte: D7-D4 RED = 2, D3-D0 BLUE = 1), leaving
	// entry 16 at {G0,R2,B1}. Nothing afterwards touches the GA registers,
	// so the border must render R=2 G=0 B=1. Level 2 is outside the legacy
	// ROM's 0/6/15 set and matches no other programmed entry, so seeing it
	// on the pins can only come from the 12-bit palette. Before HF-2 the
	// border rendered the ROM colour for hw 4, (0,0,6), which is asserted
	// absent here.
	//------------------------------------------------------------------
	{
		auto* ce16 = &b.dut.rootp->p1_mobo_bench_top__DOT__ce_16;
		unsigned asic_dots = 0;
		unsigned legacy_dots = 0;
		for (unsigned i = 0; i < 400000u; ++i) {
			b.tick();
			if (!*ce16)
				continue;
			if (b.dut.red_o == 0x2 && b.dut.green_o == 0x0 &&
			    b.dut.blue_o == 0x1)
				++asic_dots;
			if (b.dut.red_o == 0x0 && b.dut.green_o == 0x0 &&
			    b.dut.blue_o == 0x6)
				++legacy_dots;
		}
		if (legacy_dots != 0)
			fail("m12: legacy ROM border (0,0,6) still reaches the pins on " +
			     std::to_string(legacy_dots) + " dots");
		if (asic_dots < 1000)
			fail("m12: ASIC palette border (2,0,1) reached the pins on only " +
			     std::to_string(asic_dots) + " dots");
		std::printf("PASS m12: 12-bit ASIC palette border on the RGB pins over "
		            "%u dots\n", asic_dots);
	}

	//------------------------------------------------------------------
	// m13: source-backed display-origin and right-edge discriminator.
	//
	// Arnold §2.1 qualifies +639 as the right edge for STANDARD timing;
	// [KT] retains positive X through +767 and derives it from the CRTC
	// horizontal counter. The sprite leaf must therefore arm at 640/767,
	// while the real compositor hides those windows behind the border at
	// R1=40 and exposes them when R1=50 widens DE through dot 799.
	//------------------------------------------------------------------
	{
		auto* ce16   = &b.dut.rootp->p1_mobo_bench_top__DOT__ce_16;
		auto* en_tap = &b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__plus_spr_en;
		auto* vc_tap = &b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__plus_vc;
		auto* rc_tap = &b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__plus_rc;

		// Keep HSYNC beyond both windows so this vector distinguishes DE/border
		// precedence from force-blanking. Disable the earlier soft-scroll setup.
		*b.asic_r0() = 63;
		*b.asic_r2() = 55;
		*b.asic_sscr() = 0;

		auto check_case = [&](unsigned xpos, unsigned r1, unsigned visible_dots,
		                      unsigned expected_width, unsigned source_offset,
		                      unsigned expected_start_hcc,
		                      unsigned expected_start_pix, const char* label) {
			*b.x_lo(0) = uint8_t(xpos);
			*b.x_hi(0) = uint8_t((xpos >> 8) & 3);
			*b.asic_r1() = uint8_t(r1);

			bool in_win = false;
			bool seen_quiet = false;
			bool discard = true;
			unsigned win_pos = 0;
			unsigned win_vline = 0;
			const uint64_t deadline = b.cyc + 500000;
			while (b.cyc < deadline) {
				b.tick();
				if (!*ce16)
					continue;
				const bool en = *en_tap != 0;
				const unsigned cur_vline =
				    unsigned((*vc_tap << 3) | (*rc_tap & 7));
				if (!en)
					seen_quiet = true;
				if (en && !in_win) {
					in_win = true;
					win_pos = 0;
					win_vline = cur_vline;
					discard = !seen_quiet;
				}
				else if (in_win && !en) {
					in_win = false;
					if (!discard && cur_vline == win_vline) {
						if (win_pos != expected_width)
							fail(std::string("m13 ") + label +
							     ": leaf width " + std::to_string(win_pos));
						return;
					}
				}
				if (!in_win || discard)
					continue;
				if (cur_vline != win_vline)
					discard = true;
				else if (win_pos == 0) {
					if (*b.asic_hcc() != expected_start_hcc ||
					    *b.asic_pix_cnt() != expected_start_pix)
						fail(std::string("m13 ") + label +
						     ": raw window starts at hcc/pix=" +
						     std::to_string(*b.asic_hcc()) + "/" +
						     std::to_string(*b.asic_pix_cnt()));
				}
				else {
					const bool visible = win_pos <= visible_dots;
					if (visible) {
						const bool even_source =
						    ((source_offset + win_pos - 1) & 1) == 0;
						const uint8_t er = even_source ? 0x1 : 0x3;
						const uint8_t eg = even_source ? 0xF : 0x6;
						const uint8_t eb = even_source ? 0x2 : 0x4;
						if (b.dut.red_o != er || b.dut.green_o != eg ||
						    b.dut.blue_o != eb)
							fail(std::string("m13 ") + label +
								     ": display DE did not expose sprite RGB");
					}
					else if (b.dut.red_o != 0x2 || b.dut.green_o != 0x0 ||
					         b.dut.blue_o != 0x1) {
						fail(std::string("m13 ") + label +
						     ": standard border did not mask sprite RGB");
					}
				}
				++win_pos;
			}
			fail(std::string("m13 ") + label +
			     ": no complete raw sprite window before timeout");
		};

		check_case(0,    40, 16, 16, 0, 1,  0,  "standard X=0");
		check_case(1016, 40,  8,  8, 8, 1,  0,  "standard X=-8");
		check_case(639,  40,  1, 16, 0, 40, 15, "standard X=639");
		check_case(640,  40,  0, 16, 0, 41, 0,  "standard X=640");
		check_case(767,  40,  0, 16, 0, 48, 15, "standard X=767");
		check_case(640,  50, 16, 16, 0, 41, 0,  "widened X=640");
		check_case(767,  50, 16, 16, 0, 48, 15, "widened X=767");
		std::printf("PASS m13: X=0/-8 align to the delayed display origin; "
		            "X=640/+767 are masked at R1=40 and visible at R1=50\n");
	}
	return 0;
}

// m7: classic mode must keep every new Plus term inert even with the
// page-enable input forced high (review finding 5 class).
void run_classic_probe() {
	MoboBench b(false);
	b.dut.reset = 1;
	for (unsigned i = 0; i < 64; ++i) b.tick();
	b.dut.reset = 0;
	b.dut.asic_page_on = 1;
	unsigned windows = 0;
	bool prev = false, ok = true;
	for (unsigned i = 0; i < 400000u; ++i) {
		b.tick();
		const bool low = b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__M1_n == 0 &&
		                 b.dut.rootp->p1_mobo_bench_top__DOT__mb__DOT__IORQ_n == 0;
		if (low && !prev) ++windows;
		prev = low;
		if (b.dut.vec_valid_o || b.dut.asic_rd_o) ok = false;
	}
	if (windows == 0) fail("m7: no acknowledge window observed");
	if (!*b.cpu_done()) fail("m7: scripted classic-mode probe did not complete");
	if (!ok) fail("m7: vec_valid or asic_rd asserted in classic mode");
	// Type 0 keeps its pre-P5 port asymmetry: RS=1 (&BF) reads R14,
	// while RS=0 (&BE) is open bus. Plus type 3 deliberately differs.
	if (*b.cpu_read_bf_r14() != 0x1A || *b.cpu_read_be_r14() != 0xFF)
		fail("m7: classic CRTC read path changed on &BE/&BF");
	if (*b.cpu_read_status2() != 0x00)
		fail("m7: Plus CRTC readback leaked onto the classic CPU bus");
	std::printf("PASS m7: classic mode inert across %u ack windows with page forced on\n",
	            windows);
}

} // namespace

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	int rc = 0;
	try {
		rc = run();
		run_classic_probe();
	} catch (const TestFailure& e) {
		std::printf("FAIL: %s\n", e.what());
		return 1;
	}
	return rc;
}
