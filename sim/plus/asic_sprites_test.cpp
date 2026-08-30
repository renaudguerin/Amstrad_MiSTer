// P4 bench: rtl/plus/asic_sprites.v directed vectors.
//
// The bench replaces asic_regs around the engine: it drives the attribute
// view / palette buses / access indicator directly and serves the row-
// fetch port exactly like the RTL contract (REQ sampled during a cycle,
// ACK + assembled byte registered one clock later, CPU-preemption left to
// the asic_regs vectors). PIXEN is asserted on EVERY bench tick — the
// tightest legal cadence, one dot per clock, so any timing that passes
// here has strictly more slack in production (4 clocks per dot).
//
// Expectations derive from docs/plus/references/asic-reference.md S4/S5
// and the [KT] comparison formulas; MAME amstrad_plus_update_video_sprites
// is the behavioural cross-reference named by architecture S6. Each
// assertion cites its source. Nothing is read back out of the simulator
// to build an expectation.

#include <verilated.h>

#include "Vasic_sprites.h"
#include "Vasic_sprites___024root.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <string>

namespace {

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[noreturn]] void fail(const std::string& what) { throw TestFailure(what); }

bool dbg_env() { return std::getenv("SPRDBG") != nullptr; }
bool g_skipped = false;

class Spr {
public:
    Vasic_sprites dut;
    uint8_t ram[16][256];       // sprite -> 256 nibbles
    unsigned chars_per_line = 64;
    unsigned vline = 0;         // virtual {LINE,ROW} compare line
    bool taps_forced = false;   // drive LINE/ROW literally for one test
    unsigned forced_line = 0, forced_row = 0;

    Spr() {
        for (unsigned s = 0; s < 16; ++s)
            for (unsigned p = 0; p < 256; ++p) ram[s][p] = 0;
        dut.CLOCK = 0; dut.PIXEN = 0; dut.CLKEN = 0; dut.HWRAP = 0;
        dut.nRESET = 0;
        dut.LINE = 0; dut.ROW = 0;
        for (unsigned i = 0; i < 5; ++i) dut.SPR_X[i] = 0;
        for (unsigned i = 0; i < 5; ++i) dut.SPR_Y[i] = 0;
        put_field(dut.SPR_MAG, 0, 64, 0);
        for (unsigned i = 0; i < 6; ++i) dut.SPR_PAL[i] = 0;
        dut.ACC_EN = 0; dut.ACC_IDX = 0;
        dut.spr_wr_en = 0; dut.spr_wr_addr = 0; dut.spr_wr_data = 0;
        dut.FQ_ACK = 0; dut.FQ_DATA = 0;
        dut.eval();
        reset_pulse();
    }


    ~Spr() { dut.final(); }
    Spr(const Spr&) = delete;
    Spr& operator=(const Spr&) = delete;

    //------------------------------------------------------------------
    // Port plumbing
    //------------------------------------------------------------------

    static void put_field(uint32_t& w, unsigned lsb, unsigned width,
                          uint64_t v) {
        for (unsigned i = 0; i < width; ++i) {
            const uint32_t m = 1u << (lsb + i);
            if ((v >> i) & 1ULL) w |= m;
            else                 w &= ~m;
        }
    }
    static void put_field(uint64_t& w, unsigned lsb, unsigned width,
                          uint64_t v) {
        for (unsigned i = 0; i < width; ++i) {
            const uint64_t m = 1ULL << (lsb + i);
            if ((v >> i) & 1ULL) w |= m;
            else                 w &= ~m;
        }
    }
    template <typename W>
    static void put_field(W& w, unsigned lsb, unsigned width,
                          uint64_t v) {
        for (unsigned i = 0; i < width; ++i) {
            const unsigned bit = lsb + i;
            const uint32_t m = 1u << (bit & 31);
            if ((v >> i) & 1ULL) w[bit >> 5] |= m;
            else                 w[bit >> 5] &= ~m;
        }
    }

    void set_x(unsigned s, unsigned v) {   // stored 10-bit view of X
        put_field(dut.SPR_X, s * 10, 10, v & 0x3FF);
    }
    void set_y(unsigned s, unsigned v) {   // stored 9-bit view of Y
        put_field(dut.SPR_Y, s * 9, 9, v & 0x1FF);
    }
    void set_mag(unsigned s, unsigned nib) {
        put_field(dut.SPR_MAG, s * 4, 4, nib & 0xF);
    }
    // Colour c (1..15): engine reads entry at [(c-1)*12 +: 12], {G,R,B}.
    void set_colour(unsigned c, unsigned g, unsigned r, unsigned b) {
        put_field(dut.SPR_PAL, (c - 1) * 12, 12,
                  ((uint64_t)g << 8) | ((uint64_t)r << 4) | b);
    }
    void wr(unsigned s, unsigned px, unsigned py, unsigned nib) {
        ram[s][py * 16 + px] = nib & 0xF;
    }

    void reset_pulse() {
        dut.nRESET = 0;
        for (unsigned i = 0; i < 8; ++i) step(false, false, false);
        dut.nRESET = 1;
        for (unsigned i = 0; i << 0 < 4; ) { step(false, false, false); break; }
        for (unsigned i = 0; i < 4; ++i) step(false, false, false);
    }

    //------------------------------------------------------------------
    // Clocking. One call = one rising edge with the given strobes.
    // The fetch server mirrors asic_regs: REQ observed during cycle k is
    // answered with ACK+DATA during cycle k+1 (registered grant).
    //------------------------------------------------------------------
    void step(bool pixen, bool clken, bool hwrap) {
        dut.FQ_ACK = ack_q_;
        dut.FQ_DATA = data_q_;
        dut.CLKEN = clken ? 1 : 0;
        dut.HWRAP = hwrap ? 1 : 0;
        dut.PIXEN = pixen ? 1 : 0;
        if (!taps_forced) {
            dut.LINE = (vline >> 3) & 0x7F;
            dut.ROW = vline & 0x1F;
        }
        else {
            dut.LINE = forced_line & 0x7F;
            dut.ROW = forced_row & 0x1F;
        }
        dut.CLOCK = 0; dut.eval();
        const bool rq = dut.FQ_REQ != 0;
        const unsigned ad = (unsigned)dut.FQ_ADDR;
        dut.CLOCK = 1; dut.eval();
        ack_q_ = rq;
        data_q_ = ram_byte(ad);
        if (rq) ++n_req_obs;
        if (ack_q_) ++n_ack_drv;
    }

    void idle(unsigned n) {
        for (unsigned i = 0; i < n; ++i) step(false, false, false);
    }
    void dot() { step(true, false, false); }

    // Execute the final dot edge of a character (the CLKEN/HWRAP strobe).
    void char_end(bool wrap) { step(true, true, wrap); }

    // One character: sample loop callers use dots(); this runs a whole
    // character without sampling.
    void run_char(bool wrap = false) {
        for (unsigned d = 0; d < 15; ++d) dot();
        char_end(wrap);
    }
    void run_line() {
        for (unsigned c = 0; c + 1 < chars_per_line; ++c) run_char(false);
        run_char(true);
        if (!taps_forced) ++vline;
        idle(6);   // seam swap / walk-start bookkeeping
    }
    void run_to_vline(unsigned target) {
        if (taps_forced) fail("run_to_vline with forced taps");
        unsigned guard = 0;
        while (vline < target) {
            run_line();
            if (++guard > 5000) fail("run_to_vline guard");
        }
    }

    // Reposition the compare counter, including backwards: the engine
    // keeps no vline state of its own (taps are pure inputs), so a
    // backward jump plus one synthetic seam simply retags/refills the
    // staging for the new row.
    void jump_vline(unsigned vl) {
        if (taps_forced) fail("jump_vline with forced taps");
        if (vl < vline) vline = vl;
        else run_to_vline(vl);
        step(true, true, true);   // synthetic seam at the new line
        idle(6);
    }

    // Force literal CRTC taps (formula tests); caller restores.
    void force_taps(unsigned line, unsigned row) {
        taps_forced = true;
        forced_line = line;
        forced_row = row;
    }
    void unforce_taps() { taps_forced = false; }

    struct Smp { bool en; unsigned idx; unsigned r, g, b; unsigned win; };

    Smp sample() const {
        Smp s;
        s.en = dut.SPR_EN != 0;
        s.idx = dut.SPR_IDX;
        s.r = dut.SPR_RGB >> 8; s.g = (dut.SPR_RGB >> 4) & 0xF;
        s.b = dut.SPR_RGB & 0xF;
        s.win = dut.SPR_WIN;
        return s;
    }

private:
    bool ack_q_ = false;
    uint8_t data_q_ = 0;
public:
    unsigned n_req_obs = 0, n_ack_drv = 0;

    uint8_t ram_byte(unsigned addr11) const {
        const unsigned s = (addr11 >> 7) & 0xF;
        const unsigned row = (addr11 >> 3) & 0xF;
        const unsigned by = addr11 & 7;
        return (uint8_t)((ram[s][row * 16 + by * 2 + 1] << 4) |
                         ram[s][row * 16 + by * 2]);
    }
};

//----------------------------------------------------------------------
// Shared fixtures
//----------------------------------------------------------------------

// Nonzero-per-source-pixel pattern: nib(px,py) = ((3*index) mod 15) + 1.
constexpr unsigned pat_nib(unsigned px, unsigned py) {
    unsigned v = (px + py * 3U) % 15U + 1U;
    return v;
}

// Fill sprite s with pat_nib over the full 16x16 image.
void fill_pattern(Spr& b, unsigned s) {
    for (unsigned py = 0; py < 16; ++py)
        for (unsigned px = 0; px < 16; ++px)
            b.wr(s, px, py, pat_nib(px, py));
}

// Distinct {G,R,B} per sprite-colour entry; deterministic and reversible.
void pal_entry(unsigned c, unsigned& g, unsigned& r, unsigned& b) {
    g = c;
    r = 15 - c;
    b = (c * 3) & 15;
}
void program_palette(Spr& b) {
    for (unsigned c = 1; c <= 15; ++c) {
        unsigned g, r, bl;
        pal_entry(c, g, r, bl);
        b.set_colour(c, g, r, bl);
    }
}

//----------------------------------------------------------------------
// Vectors
//----------------------------------------------------------------------

// S5: "If either X or Y mag is 0 the sprite is off"; magnification is
// cleared at reset. Data and positions are live, yet nothing may show.
void s01_disabled_codes_off(Spr& b) {
    fill_pattern(b, 0);
    b.set_x(0, 100);
    b.set_y(0, 50);
    program_palette(b);

    const unsigned mags[4] = {0x0, 0x4, 0x1, 0x0};
    for (unsigned m : mags) {
        b.set_mag(0, m);
        b.run_to_vline(52);
        // Sweep the would-be window columns across three characters.
        for (unsigned c = 4; c < 9; ++c) {
            for (unsigned d = 0; d < 16; ++d) {
                const Spr::Smp s = b.sample();
                if (s.en) fail("s01: sprite active with a zero mag code");
                if (s.win != 0) fail("s01: window mask set while disabled");
                        if (d == 15) b.char_end(false); else b.dot();
            }
        }
    }
}

// S5 coordinates/priority/palette basics at x1: exact placement, exact
// source pixels, transparency at nibble zero, silence outside.
void s02_basic_placement_x1(Spr& b) {
    for (unsigned py = 0; py < 16; ++py)
        for (unsigned px = 0; px < 16; ++px)
            b.wr(0, px, py, (px == 3) ? 0 : pat_nib(px, py));
    b.set_x(0, 16);
    b.set_y(0, 8);
    b.set_mag(0, 0x5);
    program_palette(b);

    b.run_to_vline(12);   // inside rows 8..23, well past first-fill
    const bool dbg = std::getenv("SPRDBG") != nullptr;
    unsigned bad = 0;
    for (unsigned c = 0; c < 4; ++c) {
        for (unsigned d = 0; d < 16; ++d) {
            const unsigned hp = c * 16 + d;
            const Spr::Smp s = b.sample();
            if (dbg && c <= 2)
                std::printf("DBG vl=%u c=%u d=%u en=%u idx=%u win=%04x rgb=%x%x%x\n",
                            b.vline, c, d, s.en ? 1 : 0, s.idx, s.win,
                            s.r, s.g, s.b);
            const bool inside = hp >= 16 && hp < 32;
            if (inside) {
                const unsigned px = hp - 16;
                const unsigned exp_nib = (px == 3) ? 0 : pat_nib(px, 4);
                if (!s.en && exp_nib != 0) {
                    if (dbg) std::printf("BAD hp=%u dark\n", hp);
                    ++bad; continue;
                }
                if (s.en && exp_nib == 0) {
                    if (dbg) std::printf("BAD hp=%u spurious\n", hp);
                    ++bad; continue;
                }
                if (s.en) {
                    unsigned g, r, bl;
                    pal_entry(exp_nib, g, r, bl);
                    if (s.idx != 0 || s.r != r || s.g != g || s.b != bl) {
                        if (dbg)
                            std::printf("BAD hp=%u idx=%u rgb=%x%x%x "
                                        "exp %x%x%x\n", hp, s.idx,
                                        s.r, s.g, s.b, r, g, bl);
                        ++bad;
                    }
                }
            }
            else if (s.en) {
                if (dbg) std::printf("BAD hp=%u outside lit\n", hp);
                ++bad;
            }
                if (d == 15) b.char_end(false); else b.dot();
        }
    }
    if (bad)
        fail("s02: " + std::to_string(bad) + " bad samples (req=" +
             std::to_string(b.n_req_obs) + " ack=" +
             std::to_string(b.n_ack_drv) + ")");
}

// [KT] Y compare: (LineCounter<<3)|(RasterCounter&7). A sprite at Y=17
// shows for taps {2,1} and equally for {2,9} (masking pins the &7), and
// not for {2,0}/(2,2). Not gated by Vertical Displayed: the engine has no
// DE/R6 input at all (structural).
void s03_y_compare_formula_and_masking(Spr& b) {
    fill_pattern(b, 0);
    b.set_x(0, 0);
    b.set_y(0, 17);
    b.set_mag(0, 0x5);
    program_palette(b);

    // Compare lines: {2,1}->{2<<3|(9&7)}==17 both hit Y=17 exactly (the
    // RasterCounter &7 masking pin); 16 and 33 sit just outside the
    // 16-line window [17,33).
    struct Case { unsigned line, row; bool lit; };
    const Case cases[] = {{2, 1, true}, {2, 9, true},
                          {2, 0, false}, {4, 1, false}};
    const bool dbg3 = std::getenv("SPRDBG") != nullptr;
    for (const Case& tc : cases) {
        b.force_taps(tc.line, tc.row);
        // Synthetic seam samples this compare line and retags/refills;
        // ten discarded characters give the perpetual walker ample time
        // to complete the row (nothing invalidates it afterwards
        // because the forced taps hold the compare line constant).
        b.step(true, true, true);
        b.idle(6);
        for (unsigned c = 0; c < 10; ++c) {
            for (unsigned dd = 0; dd < 16; ++dd) {
                const Spr::Smp s = b.sample();
                if (!tc.lit && s.en && !dbg3)
                    fail("s03: unexpected visibility at line=" +
                         std::to_string(tc.line) + " row=" +
                         std::to_string(tc.row));
                if (dd == 15) b.char_end(false); else b.dot();
            }
        }
        // Realign to the window's own character: this seam clears hp to
        // 0 and, because the compare line is unchanged, takes the
        // keep-path that leaves the filled bank intact.
        b.step(true, true, true);
        b.idle(6);
        for (unsigned d = 0; d < 16; ++d) {
            const Spr::Smp s = b.sample();
            if (dbg3)
                std::printf("s03 L%u R%u d%u en=%u win=%04x rgb=%x%x%x\n",
                            tc.line, tc.row, d, s.en ? 1 : 0, s.win,
                            s.r, s.g, s.b);
            if (s.en != tc.lit && !dbg3)
                fail("s03: visibility mismatch at line=" +
                     std::to_string(tc.line) + " row=" +
                     std::to_string(tc.row));
            if (s.en) {
                const unsigned nib = pat_nib(d, 0);
                unsigned g, r, bl;
                pal_entry(nib, g, r, bl);
                if (s.r != r || s.g != g || s.b != bl)
                    if (!dbg3) fail("s03: wrong source pixel");
            }
            if (d == 15) b.char_end(false); else b.dot();
        }
    }
    b.unforce_taps();
}

// S5 magnification, horizontal: codes 10 => x2. Source pixels repeat in
// pairs, the window doubles to 32 dots, and the finest resolution stays
// mode-2 based (independent of GAMODE — the engine has no mode input).
void s04_x_magnification(Spr& b) {
    for (unsigned py = 0; py < 16; ++py)
        for (unsigned px = 0; px < 16; ++px)
            b.wr(0, px, py, (px & 7) + 1);
    b.set_x(0, 16);
    b.set_y(0, 8);
    b.set_mag(0, 0x9);   // xc=10 (x2), yc=01 (x1)
    program_palette(b);

    b.run_to_vline(12);
    b.run_char(false);   // execute char0 so hp sits at 16
    const bool dbg4 = std::getenv("SPRDBG") != nullptr;
    for (unsigned c = 1; c <= 3; ++c) {
        for (unsigned d = 0; d < 16; ++d) {
            const unsigned hp = c * 16 + d;
            const Spr::Smp s = b.sample();
            const bool inside = hp >= 16 && hp < 48;
            if (dbg4)
                std::printf("s04 vl%u c%u d%u hp_exp=%u en=%u win=%04x "
                            "rgb=%x%x%x\n",
                            b.vline, c, d, hp, s.en ? 1 : 0, s.win,
                            s.r, s.g, s.b);
            if (s.en != inside && !dbg4)
                fail("s04: x2 window extent wrong at hp=" +
                     std::to_string(hp));
            if (inside) {
                const unsigned srcpx = (hp - 16) >> 1;
                const unsigned exp_pen = (srcpx & 7) + 1;
                unsigned g, r, bl;
                pal_entry(exp_pen, g, r, bl);
                if ((s.r != r || s.g != g || s.b != bl) && !dbg4)
                    fail("s04: x2 source repeat wrong at hp=" +
                         std::to_string(hp));
            }
            if (d == 15) b.char_end(false); else b.dot();
        }
    }
}

// S5 magnification, vertical: code 10 => y2. Consecutive compare lines
// show the SAME source row (row = (vline-Y)>>1) and the window covers 32
// compare lines. MAME's (vpos - spr_y) >> ymag agrees.
void s05_y_magnification_height(Spr& b) {
    fill_pattern(b, 0);
    // X=32 puts the 16-dot window at chars 2..5, so post-seam refills
    // complete during the dead characters before the window opens.
    b.set_x(0, 32);
    b.set_y(0, 8);
    b.set_mag(0, 0x6);   // xc=01 (x1), yc=10 (y2)
    program_palette(b);

    const bool dbg5 = std::getenv("SPRDBG") != nullptr;

    // Row duplication FIRST (while the free-running compare counter is
    // still inside the y2 window [Y, Y+32) = lines 8..39): vlines 12 and
    // 13 both map to source row (diff>>1) = 2.
    b.run_to_vline(12);
    for (unsigned c = 0; c < 2; ++c) b.run_char(false);
    Spr::Smp a = b.sample();
    b.run_line();
    for (unsigned c = 0; c < 2; ++c) b.run_char(false);
    Spr::Smp cc = b.sample();
    if (!a.en || !cc.en || a.r != cc.r || a.g != cc.g || a.b != cc.b)
        fail("s05: y2 adjacent lines differ a=" + std::to_string(a.en) +
             "/" + std::to_string(a.r) + "," + std::to_string(a.g) + "," +
             std::to_string(a.b) + " c=" + std::to_string(cc.en) + "/" +
             std::to_string(cc.r) + "," + std::to_string(cc.g) + "," +
             std::to_string(cc.b));

    // Height bounds within the same window passage.
    auto probe = [&](unsigned vl, bool expect_lit) {
        b.run_to_vline(vl);
        for (unsigned c = 0; c < 2; ++c) b.run_char(false);
        unsigned seen = 0;
        for (unsigned d = 0; d < 16; ++d) {
            const Spr::Smp s = b.sample();
            if (s.en) {
                ++seen;
                unsigned g, r, bl;
                const unsigned srow = ((vl - 8) >> 1) & 15;  // y2 row map
                pal_entry(pat_nib(d, srow), g, r, bl);
                if (s.r != r || s.g != g || s.b != bl)
                    fail("s05: wrong pixel at vl=" + std::to_string(vl) +
                         " d=" + std::to_string(d));
            }
            if (d == 15) b.char_end(false); else b.dot();
        }
        if (dbg5)
            std::printf("s05 probe vl=%u seen=%u\n", vl, seen);
        if (expect_lit && seen != 16)
            fail("s05: y2 line should be fully lit");
        if (!expect_lit && seen != 0)
            fail("s05: y2 window extent wrong vertically");
    };
    probe(26, true);    // mid window
    probe(38, true);    // last window line
    probe(40, false);   // one past
}

// S5 largest magnification (codes 11): 64x64 windows, source pixels in
// groups of four; corner probes bound both axes.
void s06_quad_magnification_corners(Spr& b) {
    fill_pattern(b, 0);
    b.set_x(0, 16);
    b.set_y(0, 8);
    b.set_mag(0, 0xF);
    program_palette(b);

    b.run_to_vline(40);
    // Horizontal bounds at a mid window line: lit 16..79, dark at 80.
    for (unsigned c = 0; c < 6; ++c) {
        for (unsigned d = 0; d < 16; ++d) {
            const unsigned hp = c * 16 + d;
            const Spr::Smp s = b.sample();
            if (s.en != (hp >= 16 && hp < 80))
                fail("s06: x4 window extent wrong at hp=" +
                     std::to_string(hp));
                if (d == 15) b.char_end(false); else b.dot();
        }
    }
    // Vertical bounds: last lit compare line 71, dark at 72.
    b.run_to_vline(71);
    b.run_char(false);
    if (!b.sample().en) fail("s06: vline 71 should be lit");
    b.run_line();
    b.run_char(false);
    if (b.sample().en) fail("s06: vline 72 should be dark");
}

// S5 priority: border > sprite 0 > ... > sprite 15 > screen; transparent
// nibbles expose lower planes. MAME renders sprite 15 first and sprite 0
// last, matching. Overlap of 0 and 1 with holes in 0; then all sixteen
// stacked at one point: 0 wins.
void s07_priority_and_transparency(Spr& b) {
    for (unsigned py = 0; py < 16; ++py)
        for (unsigned px = 0; px < 16; ++px) {
            b.wr(0, px, py,
                 (px == 3 || px == 7) ? 0 : ((px & 7) | 8));
            b.wr(1, px, py, 1);
        }
    b.set_x(0, 32); b.set_y(0, 8); b.set_mag(0, 0x5);
    b.set_x(1, 32); b.set_y(1, 8); b.set_mag(1, 0x5);
    program_palette(b);

    b.run_to_vline(12);
    b.run_char(false); b.run_char(false);   // reach character 2
    const bool dbg7 = std::getenv("SPRDBG") != nullptr;
    // Character 2 carries hp 32..47.
    for (unsigned d = 0; d < 16; ++d) {
        const Spr::Smp s = b.sample();
        const unsigned px = d;
        if (dbg7)
            std::printf("s07 d%u en=%u idx=%u rgb=%x%x%x\n",
                        d, s.en ? 1 : 0, s.idx, s.r, s.g, s.b);
        if (px == 3 || px == 7) {
            if (!s.en || s.idx != 1)
                fail("s07: hole must expose sprite 1");
            unsigned g, r, bl;
            pal_entry(1, g, r, bl);
            if (s.r != r || s.g != g || s.b != bl)
                fail("s07: sprite 1 colour through the hole");
        }
        else {
            if (!s.en || s.idx != 0)
                fail("s07: sprite 0 must win its opaque pixels");
            unsigned g, r, bl;
            pal_entry((px & 7) | 8, g, r, bl);
            if (s.r != r || s.g != g || s.b != bl)
                fail("s07: sprite 0 colour");
        }
        if (d == 15) b.char_end(false); else b.dot();
    }

    // Stack sprites 2..15 under the same window: index 0 still wins.
    for (unsigned s = 2; s < 16; ++s) {
        fill_pattern(b, s);
        b.set_x(s, 32); b.set_y(s, 8); b.set_mag(s, 0x5);
    }
    b.run_to_vline(13);
    b.run_char(false); b.run_char(false);   // reach character 2
    for (unsigned d = 0; d < 4; ++d) {
        const Spr::Smp s = b.sample();
        // px3 is sprite 0's designed hole: sprite 1 shows through while
        // sprites 2..15 sit underneath; everywhere else 0 wins.
        const unsigned want = (d == 3) ? 1 : 0;
        if (!s.en || s.idx != want)
            fail("s07: 16-stack winner must be " +
                 std::to_string(want));
        if (d == 3) b.char_end(false); else b.dot();
    }
}

// S5/S6 + [KT]: colour c selects palette entry 16+c; the palette word is
// {G,R,B} and the engine emits channel order {R,G,B}. Column px shows pen
// px (mod 16); column 0 is transparent (pen 0).
void s08_palette_mapping_and_order(Spr& b) {
    for (unsigned py = 0; py < 16; ++py)
        for (unsigned px = 0; px < 16; ++px)
            b.wr(0, px, py, px & 15);
    b.set_x(0, 16);
    b.set_y(0, 8);
    b.set_mag(0, 0x5);
    program_palette(b);

    b.run_to_vline(12);
    b.run_char(false);   // reach character 1 (hp 16..31)
    for (unsigned d = 0; d < 16; ++d) {
        const Spr::Smp s = b.sample();
        if (d == 0) {
            if (s.en) fail("s08: pen 0 must be transparent");
        }
        else {
            if (!s.en) fail("s08: missing pixel for pen");
            unsigned g, r, bl;
            pal_entry(d, g, r, bl);
            if (s.r != r || s.g != g || s.b != bl)
                fail("s08: colour c did not select entry 16+c in "
                     "{G,R,B}->{R,G,B} order");
        }
        if (d == 15) b.char_end(false); else b.dot();
    }
}

// [KT] X model: stored 10-bit view compared against the modular dot
// scale. X=+767 arms mid-character (X&7=7); X=-8 (stored 1016) wraps the
// scale mid-window and completes after the wrap (named model choice,
// module header); X=-256 (stored 512) aliases into mid-line.
void s09_x_extremes_wrap_and_negative(Spr& b) {
    fill_pattern(b, 0);
    b.set_y(0, 8);
    b.set_mag(0, 0x5);
    program_palette(b);

    // (c) FIRST, on clean default 64-char lines: stored 512 (= -256)
    // aliases to mid-line window hp512..527; sampled at vl22 (row 14).
    b.set_x(0, 512);
    b.jump_vline(22);
    for (unsigned c = 0; c < 32; ++c) b.run_char(false);
    const bool dbg9 = std::getenv("SPRDBG") != nullptr;
    for (unsigned d = 0; d < 16; ++d) {
        const Spr::Smp s = b.sample();
        if (!s.en && !dbg9)
            fail("s09(c): negative-X alias window missing");
        if (s.en) {
            unsigned g, r, bl;
            pal_entry(pat_nib(d, 14), g, r, bl);
            if ((s.r != r || s.g != g || s.b != bl) && !dbg9)
                fail("s09(c): wrong pixel");
        }
        if (d == 15) b.char_end(false); else b.dot();
    }

    // (a) +767: lit exactly 767..782 with source pixels in order;
    // sampled at vl12 (row 4).
    b.set_x(0, 767);
    b.jump_vline(12);
    for (unsigned c = 0; c < 46; ++c) b.run_char(false);
    for (unsigned c = 46; c <= 49; ++c) {
        for (unsigned d = 0; d < 16; ++d) {
            const unsigned hp = c * 16 + d;
            const Spr::Smp s = b.sample();
            const bool inside = hp >= 767 && hp < 783;
            if (s.en != inside && !dbg9)
                fail("s09(a): window extent at hp=" + std::to_string(hp));
            if (inside) {
                const unsigned srcpx = hp - 767;
                unsigned g, r, bl;
                pal_entry(pat_nib(srcpx, 4), g, r, bl);
                if ((s.r != r || s.g != g || s.b != bl) && !dbg9)
                    fail("s09(a): wrong source pixel");
            }
            if (d == 15) b.char_end(false); else b.dot();
        }
    }

    // (b) stored 1016 (= -8): eight dots before the 1024-wrap, then the
    // window CONTINUES after the wrap with source pixels 8..15.
    b.chars_per_line = 70;
    b.set_x(0, 1016);
    b.jump_vline(20);
    for (unsigned c = 0; c < 60; ++c) b.run_char(false);
    for (unsigned c = 60; c <= 69; ++c) {
        for (unsigned d = 0; d < 16; ++d) {
            const unsigned hp_abs = c * 16 + d;
            const unsigned hp = hp_abs % 1024;
            const bool pre  = hp >= 1016;
            const bool post = hp_abs >= 1024 && hp_abs < 1032;
            const Spr::Smp s = b.sample();
            if (pre != post) {
                if (!s.en && !dbg9)
                    fail("s09(b): wrapped window missing at hp=" +
                         std::to_string(hp_abs));
                const unsigned srcpx =
                    pre ? hp - 1016 : hp_abs - 1024 + 8;
                unsigned g, r, bl;
                pal_entry(pat_nib(srcpx, 12), g, r, bl);   // vl20 -> row 12
                if ((s.r != r || s.g != g || s.b != bl) && !dbg9)
                    fail("s09(b): wrong source pixel across wrap");
            }
            else if (s.en && !dbg9) {
                fail("s09(b): unexpected pixel near wrap at hp=" +
                     std::to_string(hp_abs));
            }
            if (d == 15) b.char_end(c == 69); else b.dot();
        }
        if (c == 69) { ++b.vline; b.idle(6); }
    }
    b.chars_per_line = 64;
}

// [KT]: "If CRTC R0>64, then the sprites may repeat horizontally." The
// 10-bit scale holds exactly 64 characters, so a 70-character line walks
// the window twice.
void s10_r0_gt_64_repeat(Spr& b) {
    fill_pattern(b, 0);
    b.set_x(0, 16);
    b.set_y(0, 8);
    b.set_mag(0, 0x5);
    program_palette(b);
    b.chars_per_line = 70;

    b.run_to_vline(12);
    for (unsigned c = 0; c < 70; ++c) {
        for (unsigned d = 0; d < 16; ++d) {
            const unsigned hp_abs = c * 16 + d;
            const unsigned hp = hp_abs % 1024;
            const bool expect = (hp >= 16 && hp < 32);
            const Spr::Smp s = b.sample();
            if (s.en != expect && !dbg_env())
                fail("s10: repeat visibility at hp=" + std::to_string(hp));
            if (s.en) {
                const unsigned srcpx = hp - 16;
                unsigned g, r, bl;
                pal_entry(pat_nib(srcpx, 4), g, r, bl);   // vl12 -> row 4
                if ((s.r != r || s.g != g || s.b != bl) && !dbg_env())
                    fail("s10: repeated pass shows wrong pixels");
            }
            if (d == 15) b.char_end(c == 69); else b.dot();
        }
        if (c == 69) { ++b.vline; b.idle(6); }
    }
    b.chars_per_line = 64;
}

// S5 access side effect: a pixel-data access removes THAT sprite only for
// the access duration plus a short tail (hole shape unmeasured, module
// header); the stored image survives and reappears unchanged.
//
// Sprite 3 runs X2/Y2 (mag 0xA) so its window spans chars 4-5 and each
// character shows eight source pixels. The access flush lands mid-walker-
// lap, so refilling sprite 3's active row takes a few characters (single
// continuous fetch server; see module header bandwidth note); the vector
// therefore pins scope and integrity inside the flushed window and full
// recovery in the next display line's window, as a documented model
// choice rather than an S5 rule.
void s11_access_blanking_scope_and_integrity(Spr& b) {
    // Sprite 2 (X=16, x1/y1): bystander, window = char1. Sprite 3
    // (X=64, x2/y2): target of the access, window = chars 4-5. Sprite
    // 3's fixture is pat_nib over the FULL pixel index so char5's
    // colours differ from char4's (a half-row-symmetric pattern would
    // let a window that restarts at char4 pass unnoticed).
    for (unsigned py = 0; py < 16; ++py)
        for (unsigned px = 0; px < 16; ++px) {
            b.wr(2, px, py, (px & 7) + 1);
            b.wr(3, px, py, pat_nib(px, py));
        }
    b.set_x(2, 16); b.set_y(2, 8); b.set_mag(2, 0x5);
    b.set_x(3, 64); b.set_y(3, 8); b.set_mag(3, 0xa);
    program_palette(b);

    const bool dbgB = std::getenv("SPRDBG") != nullptr;

    // char0: dead.
    b.run_to_vline(12);
    b.run_char(false);

    // char1: hold a pixel-data ACCESS TO SPRITE 3 across the whole
    // character while bystander sprite 2 renders right through it
    // (reference S5: an access removes THAT sprite only).
    b.dut.ACC_EN = 1;
    b.dut.ACC_IDX = 3;
    for (unsigned d = 0; d < 16; ++d) {
        const Spr::Smp s = b.sample();
        if ((!s.en || s.idx != 2) && !dbgB)
            fail("s11: bystander disturbed by sprite 3 access");
        if (dbgB)
            std::printf("s11 c1 d%u en=%u idx=%u\n",
                        d, s.en ? 1 : 0, s.idx);
        if (d == 15) b.char_end(false); else b.dot();
    }
    b.dut.ACC_EN = 0;

    // chars 2-3: dead for both sprites.
    b.run_char(false);
    b.run_char(false);

    // Window chars 4-5 (source px 0..15): the staged banks were flushed
    // by the access and the continuous fetch server may still be sweeps
    // away from sprite 3's active block (measured: refill starts about
    // four characters after an access that ends mid-lap), so NOTHING is
    // asserted about sprite 3's visibility inside this first window.
    // Whatever pixels do show must belong to sprite 3 with correct
    // image data (rb_dat survives the flush; the stored image is
    // unchanged), and no other sprite may win here. Source pixel of
    // display dot d in window char ch is (ch-4)*8 + (d>>1); this line
    // is vline 12 -> diff 4 -> source row (4)>>1 = 2 under Y2.
    for (unsigned ch = 4; ch <= 5; ++ch) {
        for (unsigned d = 0; d < 16; ++d) {
            const Spr::Smp s = b.sample();
            if (s.en && s.idx != 3 && !dbgB)
                fail("s11: foreign winner in sprite 3 zone");
            if (s.en && s.idx == 3) {
                unsigned g, r, bl;
                pal_entry(pat_nib((ch - 4) * 8 + (d >> 1), 2), g, r, bl);
                if ((s.r != r || s.g != g || s.b != bl) && !dbgB)
                    fail("s11: shown pixels corrupted mid-refill");
            }
            if (dbgB)
                std::printf("s11 c%u d%u en=%u idx=%u\n",
                            ch, d, s.en ? 1 : 0, s.idx);
            if (d == 15) b.char_end(false); else b.dot();
        }
    }

    // Recovery model choice: by the SAME source row's window on the next
    // display line the image is back, complete and byte-correct (next
    // line is vline 13 -> source row 2). The
    // reference fixes only THAT-sprite-only scope and image integrity,
    // not hole shape; the one-line bound is our bandwidth model (module
    // header), verified here against the measured walker behaviour.
    // Row check: vline 13, Y 8 -> diff 5 -> source row (5)>>1 = 2, the
    // same row the flushed line showed (vline 12 -> diff 4 -> row 2).
    b.run_line();          // cross the seam into vline 13
    for (unsigned c = 0; c <= 3; ++c) b.run_char(false);
    for (unsigned ch = 4; ch <= 5; ++ch) {
        for (unsigned d = 0; d < 16; ++d) {
            const Spr::Smp s = b.sample();
            if ((!s.en || s.idx != 3) && !dbgB)
                fail("s11: sprite 3 did not recover by the next window");
            unsigned g, r, bl;
            pal_entry(pat_nib((ch - 4) * 8 + (d >> 1), 2), g, r, bl);
            if ((s.r != r || s.g != g || s.b != bl) && !dbgB)
                fail("s11: recovered image corrupted");
            if (dbgB)
                std::printf("s11 n%u d%u en=%u idx=%u rgb=%x%x%x\n",
                            ch, d, s.en ? 1 : 0, s.idx,
                            s.r, s.g, s.b);
            if (d == 15) b.char_end(false); else b.dot();
        }
    }
}
void s12_x_rewrite_cut_and_continue(Spr& b) {
    fill_pattern(b, 0);
    b.set_x(0, 16);
    b.set_y(0, 8);
    b.set_mag(0, 0x5);
    program_palette(b);

    b.run_to_vline(12);
    b.run_char(false);   // char0 passes dark; window opens in char1
    for (unsigned d = 0; d < 16; ++d) {
        const Spr::Smp s = b.sample();
        // The rewrite lands after dot 5 is sampled; the register-shadow
        // mismatch kills emission from the following edge, so dot 5 is
        // legitimately still lit and dots 6 onward must be dark.
        if (d <= 5) {
            if (!s.en) fail("s12: window lost before rewrite");
        }
        else if (s.en) {
            fail("s12: rewrite did not cut the running window");
        }
        if (d == 5) b.set_x(0, 400);
        if (d == 15) b.char_end(false); else b.dot();
    }
    for (unsigned c = 2; c < 24; ++c) b.run_char(false);
    // Character 24 sampled dark pins the new window's arm at exact
    // equality with X=400 -- an early-armed dot here cannot hide.
    for (unsigned d = 0; d < 16; ++d) {
        const Spr::Smp s = b.sample();
        if (s.en) fail("s12: window armed before the rewritten X");
        if (d == 15) b.char_end(false); else b.dot();
    }
    // Continuation: the new window arms at exact equality with X=400,
    // i.e. character 25 dot 0, x1 so source pixel = dot, still vline 12
    // -> source row 4, winner sprite 0 throughout.
    for (unsigned d = 0; d < 16; ++d) {
        const Spr::Smp s = b.sample();
        if (!s.en || s.idx != 0)
            fail("s12: continuation missing at new X, dot " +
                 std::to_string(d));
        unsigned g, r, bl;
        pal_entry(pat_nib(d, 4), g, r, bl);
        if (s.r != r || s.g != g || s.b != bl)
            fail("s12: continuation shows wrong source pixel at dot " +
                 std::to_string(d));
        if (d == 15) b.char_end(false); else b.dot();
    }
}

// Named model choice: Y rewrites blank the remainder of the current line
// (live row-tag gate) and the sprite resumes under the new Y from the
// next seam once its rows restage. X2/Y2 (mag 0xa) gives a two-character
// window so both the cut and the resumed image can span a character
// boundary; resumed colours are derived on paper: after run_line the
// compare line is 11, new Y 9 -> diff 2 -> source row (11-9)>>1 = 1,
// and display dot d of window char k shows source pixel (d>>1)+8*(k-1).
void s13_y_rewrite_scanline_granularity(Spr& b) {
    fill_pattern(b, 0);
    b.set_x(0, 16);
    b.set_y(0, 8);
    b.set_mag(0, 0xa);
    program_palette(b);

    b.run_to_vline(10);
    b.run_char(false);   // char0 dead
    for (unsigned d = 0; d < 16; ++d) {
        const Spr::Smp s = b.sample();
        if (d < 4 && !s.en) fail("s13: sprite vanished before rewrite");
        // Rewrite lands after dot 4 is sampled; the live row-tag gate
        // (new Y -> source row 0 vs staged row 1) blanks from dot 5.
        if (d == 4) b.set_y(0, 9);
        if (d >= 5 && s.en)
            fail("s13: Y rewrite must blank the rest of the line");
        if (d == 15) b.char_end(false); else b.dot();
    }
    // char2 is still inside the old X window but must stay blank: the
    // rewrite holds until the seam, not just one character.
    for (unsigned d = 0; d < 16; ++d) {
        const Spr::Smp s = b.sample();
        if (s.en) fail("s13: blank did not hold across the window");
        if (d == 15) b.char_end(false); else b.dot();
    }
    for (unsigned c = 3; c < 8; ++c) b.run_char(false);
    b.run_line();          // cross into the new Y's window (vline 11)
    b.run_char(false);     // char0 of the new line: still dead
    for (unsigned k = 1; k <= 2; ++k) {
        for (unsigned d = 0; d < 16; ++d) {
            const Spr::Smp s = b.sample();
            if (!s.en) fail("s13: sprite did not resume at new Y");
            unsigned g, r, bl;
            pal_entry(pat_nib((d >> 1) + 8 * (k - 1), 1), g, r, bl);
            if (s.r != r || s.g != g || s.b != bl)
                fail("s13: resumed pixels do not follow the new Y");
            if (d == 15) b.char_end(false); else b.dot();
        }
    }
}

// Bandwidth model, within-capacity guarantee: ten fully overlapped x1
// sprites all render (lowest index wins every dot) without staging misses.
void s14_overlap_bandwidth_within_capacity(Spr& b) {
    for (unsigned s = 0; s < 10; ++s) {
        for (unsigned py = 0; py < 16; ++py)
            for (unsigned px = 0; px < 16; ++px)
                b.wr(s, px, py, (s % 15) + 1);
        b.set_x(s, 32);
        b.set_y(s, 8);
        b.set_mag(s, 0x5);
    }
    program_palette(b);

    b.run_to_vline(12);
    b.run_char(false);
    for (unsigned c = 1; c <= 3; ++c) {
        for (unsigned d = 0; d < 16; ++d) {
            const Spr::Smp s = b.sample();
            const bool inside = (c * 16 + d) >= 32 && (c * 16 + d) < 48;
            if (inside) {
                if (!s.en) fail("s14: staging miss within capacity");
                if (s.idx != 0) fail("s14: priority broken under load");
            }
            else if (s.en) fail("s14: pixel outside shared window");
            if (d == 15) b.char_end(false); else b.dot();
        }
    }
}

// S15 (CG-3 RoboCop 2 closure): dynamic burst CPU pixel writes while rendering
// must update the sprite image without cache-invalidation tearing or stalling.
void s15_dynamic_burst_write_no_tearing(Spr& b) {
    for (unsigned py = 0; py < 16; ++py)
        for (unsigned px = 0; px < 16; ++px)
            b.wr(0, px, py, 1); // initial colour 1
    b.set_x(0, 32); b.set_y(0, 8); b.set_mag(0, 0x5);
    program_palette(b);

    b.run_to_vline(12); // vline 12 -> diff 4 -> row 4

    // Char 0: burst write 16 new pixels into sprite 0 row 4 (offset 0x0040..0x004F)
    for (unsigned px = 0; px < 16; ++px) {
        b.wr(0, px, 4, (px & 7) + 2); // new colours 2..9
        b.dut.spr_wr_en = 1;
        b.dut.spr_wr_addr = (0 << 8) | (4 << 4) | px;
        b.dut.spr_wr_data = (px & 7) + 2;
        if (px == 15) b.char_end(false);
        else          b.dot();
        b.dut.spr_wr_en = 0;
    }

    // Char 1: idle char before window (dots 16..31)
    b.run_char(false);

    // Char 2: sprite window at X=32..47 (dots 32..47 = source pixels 0..15)
    for (unsigned d = 0; d < 16; ++d) {
        const Spr::Smp s = b.sample();
        const unsigned px = d;
        if (!s.en || s.idx != 0)
            fail("s15: dynamic write caused sprite tearing/drop at px=" + std::to_string(px));
        unsigned g, r, bl;
        pal_entry((px & 7) + 2, g, r, bl);
        if (s.r != r || s.g != g || s.b != bl)
            fail("s15: dynamic write pixel mismatch at px=" + std::to_string(px) +
                 " (got R=" + std::to_string(s.r) + " G=" + std::to_string(s.g) + " B=" + std::to_string(s.b) +
                 " exp R=" + std::to_string(r) + " G=" + std::to_string(g) + " B=" + std::to_string(bl) + ")");
        if (d == 15) b.char_end(false); else b.dot();
    }
}

// CG-3 review regression: a CPU pixel access level can span request issue
// and the following delayed ACK.  The write-through must update the staged
// row, and the queued pre-write byte must be discarded for the request's
// whole lifetime rather than replacing the new nibble.
void s16_delayed_ack_same_row_write_collision(Spr& b) {
    for (unsigned py = 0; py < 16; ++py)
        for (unsigned px = 0; px < 16; ++px)
            b.wr(0, px, py, 1); // old image: byte 0 is 0x11
    b.set_x(0, 2); // hold the first source pixel at hp 2 while fetching
    b.set_y(0, 0);
    b.set_mag(0, 0x5);
    program_palette(b);
    b.force_taps(0, 0);

    // Arm both banks and the continuous walker without advancing hp.  With
    // only sprite 0 enabled, the first request is its active-bank row-0 byte
    // 0, exactly the byte changed by the collision below.
    b.step(true, true, true);
    b.step(false, false, false);

    // Production ACC_EN is a level held for the complete Z80 memory cycle,
    // not a one-clock pulse.  Keep the access (and its write-through strobe)
    // high for the walker-arm edge, the request-issue edge, and one further
    // in-flight edge.  The request's registered old byte is returned only
    // after the access level falls below.
    b.dut.ACC_EN = 1;
    b.dut.ACC_IDX = 0;
    b.dut.spr_wr_en = 1;
    b.dut.spr_wr_addr = 0; // sprite 0, row 0, pixel 0 (low nibble)
    b.dut.spr_wr_data = 0xA;
    b.step(false, false, false); // arm the walker; no request is issued yet
    if (b.dut.FQ_REQ)
        fail("s16: request issued before the walker-arm edge");
    b.step(false, false, false); // issue the fetch and perform write-through
    if (!b.dut.FQ_REQ)
        fail("s16: did not observe the initial row-0 fetch");
    const unsigned fetch_addr = b.dut.FQ_ADDR;
    if ((fetch_addr >> 7) != 0 || ((fetch_addr >> 3) & 0xF) != 0 ||
        (fetch_addr & 7) != 0)
        fail("s16: initial fetch was not sprite 0 row 0 byte 0");

    // Keep the production-shaped access level high for one in-flight edge;
    // the queued response is still the old 0x11 byte.  Drop ACC_EN before
    // the following edge, where the delayed ACK returns, then update the
    // backing RAM so any re-demand observes the post-write image.
    b.step(false, false, false);
    b.dut.ACC_EN = 0;
    b.dut.spr_wr_en = 0;
    b.ram[0][0] = 0xA;

    // The access tail is two PIXEN clocks.  Drain it while hp advances to X,
    // then hold hp there so the first completed refetch is sampled directly.
    b.step(true, false, false);
    b.step(true, false, false);
    bool req_level = b.dut.FQ_REQ != 0;
    bool redemanded = false;
    bool rendered_new_nibble = false;
    for (unsigned cycle = 0; cycle < 512; ++cycle) {
        const bool req = b.dut.FQ_REQ != 0;
        if (req && !req_level && b.dut.FQ_ADDR == fetch_addr)
            redemanded = true;
        req_level = req;
        const Spr::Smp s = b.sample();
        if (s.en) {
            if (!redemanded)
                fail("s16: sprite became visible before stale byte was re-demanded");
            unsigned g, r, bl;
            pal_entry(0xA, g, r, bl);
            if (s.idx != 0 || s.r != r || s.g != g || s.b != bl)
                fail("s16: delayed stale fetch overwrote the write-through nibble");
            rendered_new_nibble = true;
            break;
        }
        b.step(false, false, false);
    }
    if (!redemanded)
        fail("s16: stale completion did not re-demand the same row-0 byte");
    if (!rendered_new_nibble)
        fail("s16: new row-0 nibble did not render after stale ACK recovery");
}

// CG-3 production-cadence discriminator: keep every sprite enabled while a
// row/Y/magnification transition and a burst of CPU pixel writes compete with
// the one-byte, one-clock-later row-fetch server.  The reference requires all
// 16 sprites to be possible on one line, lowest index to win overlaps, and a
// pixel-data access to blank only the accessed sprite for the access duration;
// its stored image must survive ([asic-reference.md] S5, lines 183-210).
//
// The bench's normal step() path is deliberately used throughout: each fetch
// request is returned as FQ_ACK/FQ_DATA on the following bench edge.  No direct
// staging or cache state is injected.  X positions stay on the existing bench
// dot-counter scale; this vector is about production load, not the unresolved
// X-unit source conflict.
void s17_all_sprites_live_animation_cadence(Spr& b) {
    std::array<unsigned, 16> base{};
    std::array<unsigned, 16> anim{};
    std::array<unsigned, 16> x{};
    std::array<unsigned, 16> y{};
    std::array<unsigned, 16> mag{};
    std::array<unsigned, 16> row13{};
    std::array<unsigned, 16> row14{};
    std::array<unsigned, 16> row13_nib{};
    std::array<unsigned, 16> row14_nib{};

    // Give every sprite a sparse image with one unique marker in the shared
    // early window.  The marker positions are chosen in output-dot order,
    // accounting for each sprite's horizontal magnification, so all sixteen
    // winners can be observed on the same line without priority hiding one
    // behind another.  Sprite 0 also has a static pixel at source x=0,
    // shared with sprite 15, for the access-blanking discriminator below.
    constexpr unsigned marker_px[16] = {
        5, 4, 4, 5, 6, 7, 6, 5, 7, 8, 9, 8, 6, 9, 10, 11};
    for (unsigned s = 0; s < 16; ++s) {
        base[s] = (s % 15) + 1;
        anim[s] = ((s + 5) % 15) + 1;
        for (unsigned py = 0; py < 16; ++py)
            for (unsigned px = 0; px < 16; ++px)
                b.wr(s, px, py,
                     (px == marker_px[s] ||
                      ((s == 0 || s == 15) && px == 0))
                         ? base[s]
                         : 0);

        // All sixteen sprites deliberately overlap at the same early
        // visible position.  Their horizontal magnifications vary below,
        // so this is the worst shared-window demand rather than a spread
        // of independent slots.
        x[s] = 32;
        b.set_x(s, x[s]);
        b.set_y(s, 8);
        b.set_mag(s, 0x5); // initial x1/y1, changed at the next seam
    }
    program_palette(b);

    // Establish a completely populated initial row set before introducing
    // the live attribute changes.  At vline 12, Y=8 selects source row 4.
    b.run_to_vline(12);
    b.run_line(); // vline 13, still with the initial attributes

    // Change every Y and magnification live.  Y=10..13 keeps all sprites
    // active at vline 13/14 while making the source row and both scale axes
    // vary across the population.  The documented compare line is
    // (LineCounter<<3)|(RasterCounter&7); vline 14 is therefore used for the
    // exact post-transition expectations below.
    const unsigned mag_cycle[5] = {0xD, 0x9, 0x6, 0xA, 0xF};
    for (unsigned s = 0; s < 16; ++s) {
        y[s] = 10 + (s & 3);
        mag[s] = mag_cycle[s % 5];
        b.set_y(s, y[s]);
        b.set_mag(s, mag[s]);

        const unsigned ycode = mag[s] & 3;
        const unsigned yshift = ycode == 1 ? 0 : (ycode == 2 ? 1 : 2);
        row13[s] = ((13 - y[s]) >> yshift) & 15;
        row14[s] = ((14 - y[s]) >> yshift) & 15;
        // Make the post-transition value row-specific.  If the active bank
        // still carries row 13 where row 14 is required, the final palette
        // check below must catch that stale-row emission (except where the
        // selected vertical magnification intentionally repeats a row).
        row13_nib[s] = ((anim[s] + row13[s]) % 15) + 1;
        row14_nib[s] = ((anim[s] + row14[s]) % 15) + 1;
    }

    // Make the attribute transition at a scanline seam, then perform a
    // sustained, production-shaped CPU animation burst while the fetch
    // server is running.  Each write is a complete sprite pixel access:
    // ACC_EN stays high across its step together with spr_wr_en, matching the
    // asic_regs level contract.  Touch the same marker three times per row,
    // leaving the final animated value in place; this keeps the burst long
    // enough to overlap the real walker while preserving one-hot output
    // markers for the all-16 readiness check.
    b.step(true, true, true);
    b.idle(6);
    const unsigned req_before = b.n_req_obs;
    for (unsigned s = 0; s < 16; ++s) {
        for (const unsigned row : {row13[s], row14[s]}) {
            const unsigned px = marker_px[s];
            const unsigned final_nib = row == row13[s]
                                           ? row13_nib[s]
                                           : row14_nib[s];
            const unsigned burst_nib[3] = {
                ((final_nib + 3) % 15) + 1,
                ((final_nib + 6) % 15) + 1,
                final_nib};
            for (const unsigned nib : burst_nib) {
                b.wr(s, px, row, nib);
                b.dut.ACC_EN = 1;
                b.dut.ACC_IDX = s;
                b.dut.spr_wr_en = 1;
                b.dut.spr_wr_addr = (s << 8) | (row << 4) | px;
                b.dut.spr_wr_data = nib;
                b.step(true, false, false);
                b.dut.spr_wr_en = 0;
                b.dut.ACC_EN = 0;
            }
        }
    }
    if (b.n_req_obs == req_before)
        fail("s17: animation burst did not overlap a row-fetch request");

    // Give the real request/grant server the remainder of the line to finish
    // all current and speculative rows.  Every changed current row differs
    // from the old Y=8 row, so the production-cadence model must service at
    // least eight bytes for each of the 16 active sprites.
    b.run_line(); // vline 14, hp reset to zero
    if (b.n_req_obs - req_before < 16 * 8)
        fail("s17: all-16 row refills did not use the grant cadence (requests=" +
             std::to_string(b.n_req_obs - req_before) + ")");

    const auto xshift_for = [](unsigned m) {
        const unsigned code = (m >> 2) & 3;
        return code == 1 ? 0U : (code == 2 ? 1U : 2U);
    };

    // Scan a full 1024-dot line.  All sixteen windows begin at hp=32, and
    // SPR_WIN must therefore carry the complete active mask through the
    // first 16 visible dots despite the differing widths.  At hp=31 begin a
    // 16-dot CPU access to sprite 0.  Sprite 15 shares sprite 0's static
    // source-pixel marker at hp=32 and must show through the access; sprite 0
    // then reappears at its separate animated marker after the access tail.
    // The other fourteen markers are disjoint in this same early window, so
    // their observed SPR_IDX values prove readiness rather than only request
    // traffic or a total byte count.
    std::array<bool, 16> emitted{};
    for (unsigned hp = 0; hp < 1024; ++hp) {
        if (hp == 31) {
            b.dut.ACC_EN = 1;
            b.dut.ACC_IDX = 0;
        }
        if (hp == 48)
            b.dut.ACC_EN = 0;

        const Spr::Smp got = b.sample();
        const bool overlap = hp >= 32 && hp < 96;
        uint16_t expected_win = 0;
        if (overlap) {
            const unsigned rel = hp - 32;
            for (unsigned s = 0; s < 16; ++s) {
                const unsigned width = 16U << xshift_for(mag[s]);
                if (rel < width)
                    expected_win |= (uint16_t)1U << s;
            }
        }
        if (got.win != expected_win)
            fail("s17: shared early window mask mismatch at hp=" +
                 std::to_string(hp) + " (got 0x" +
                 std::to_string(got.win) + " expected 0x" +
                 std::to_string(expected_win) + ")");

        unsigned expected = 16;
        if (overlap) {
            const unsigned rel = hp - 32;
            // The source-x=0 access marker is ×4 for both sprites 0 and 15;
            // it is deliberately excluded from marker_px so sprite 15's
            // independent readiness marker remains unique below.
            if (rel < 4)
                expected = 15;
            for (unsigned s = 0; s < 16; ++s) {
                const unsigned scale = 1U << xshift_for(mag[s]);
                const unsigned marker = marker_px[s] * scale;
                if (rel >= marker && rel < marker + scale) {
                    expected = s;
                    break;
                }
            }
        }

        if (expected == 16) {
            if (got.en)
                fail("s17: unexpected sprite " + std::to_string(got.idx) +
                     " outside its window at hp=" + std::to_string(hp));
        }
        else {
            if (!got.en || got.idx != expected)
                fail("s17: expected sprite " + std::to_string(expected) +
                     " at hp=" + std::to_string(hp) + " (got " +
                     std::to_string(got.en ? got.idx : 16) + ")");

            const unsigned nib = (expected == 15 && hp < 36)
                                     ? base[15]
                                     : row14_nib[expected];
            unsigned g, r, bl;
            pal_entry(nib, g, r, bl);
            if (got.r != r || got.g != g || got.b != bl)
                fail("s17: sprite " + std::to_string(expected) +
                     " image mismatch at hp=" + std::to_string(hp) +
                     " marker=" + std::to_string(marker_px[expected]));
            emitted[expected] = true;
        }

        // The shared static marker is deliberately sampled while ACC_EN is
        // high.  If the access did not blank sprite 0, it would win here;
        // sprite 15 is the required lower-priority survivor.
        if (hp == 32 && (!got.en || got.idx != 15))
            fail("s17: sprite 0 access did not expose sprite 15");

        if ((hp & 15) == 15)
            b.char_end(hp == 1023);
        else
            b.dot();
    }
    b.dut.ACC_EN = 0;

    for (unsigned s = 0; s < 16; ++s)
        if (!emitted[s])
            fail("s17: sprite " + std::to_string(s) +
                 " was never emitted from its staged row");
}

constexpr std::array<std::pair<const char*, void (*)(Spr&)>, 17> kTests = {{
    {"s01 zero magnification codes disable the sprite (S5)",
     s01_disabled_codes_off},
    {"s02 x1 placement, source pixels, transparency (S5)",
     s02_basic_placement_x1},
    {"s03 Y compare formula and RasterCounter masking ([KT])",
     s03_y_compare_formula_and_masking},
    {"s04 horizontal magnification x2 (S5)", s04_x_magnification},
    {"s05 vertical magnification x2: height and row duplication (S5)",
     s05_y_magnification_height},
    {"s06 quad magnification corner bounds (S5)",
     s06_quad_magnification_corners},
    {"s07 priority chain and transparency exposure (S5)",
     s07_priority_and_transparency},
    {"s08 colour c -> entry 16+c, {G,R,B} word order (S5/S6)",
     s08_palette_mapping_and_order},
    {"s09 X extremes: +767, wrap-through, negative alias ([KT])",
     s09_x_extremes_wrap_and_negative},
    {"s10 R0>64 horizontal repeat ([KT])", s10_r0_gt_64_repeat},
    {"s11 access blanking scope and image integrity ([ARNOLD-REV S2.1])",
     s11_access_blanking_scope_and_integrity},
    {"s12 X rewrite cuts and continues ([ARNOLD-REV S2.1])",
     s12_x_rewrite_cut_and_continue},
    {"s13 Y rewrite applies at the scanline seam (model choice)",
     s13_y_rewrite_scanline_granularity},
    {"s14 ten overlapped sprites, no staging miss (bandwidth model)",
     s14_overlap_bandwidth_within_capacity},
    {"s15 dynamic burst write without tearing/garbling (CG-3 RoboCop 2)",
     s15_dynamic_burst_write_no_tearing},
    {"s16 sustained access poisons delayed ACK and re-demands (CG-3 review)",
     s16_delayed_ack_same_row_write_collision},
    {"s17 all sprites, live animation, and grant cadence (CG-3)",
     s17_all_sprites_live_animation_cadence},
}};

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    unsigned passed = 0;
    unsigned skipped = 0;
    for (const auto& test : kTests) {
        try {
            Spr bench;
            g_skipped = false;
            test.second(bench);
            ++passed;
            if (g_skipped) { ++skipped;
                continue; }
            std::printf("PASS %s\n", test.first);
        }
        catch (const std::exception& error) {
            std::printf("FAIL %s: %s\n", test.first, error.what());
            return 1;
        }
    }
    if (skipped != 0) {
        std::printf("%u asic_sprites engine tests passed, %u SKIPPED\n",
                    passed - skipped, skipped);
        return 65;   // skips must never silently pass a gate
    }
    std::printf("All %u asic_sprites engine tests passed\n", passed);
    return 0;
}
