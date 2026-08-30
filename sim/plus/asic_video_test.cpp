// Plus P1 bench: asic_video CRTC3 foundation.
//
// Timing discipline mirrors sim/sim_main.cpp: 16 CLOCK ticks per character,
// CLKEN asserted on tick 0 of each character, register bus writes land on
// full CLOCK edges away from the CLKEN phase so a write never races the
// counter edge it is meant to be observed against.
//
// Every expectation cites its ACCC v1.10 rule; values are derived from the
// cited rule on paper, never read back out of the simulator.

#include <verilated.h>

#include "Vasic_video.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

constexpr unsigned kClockTicksPerCharacter = 16;
constexpr unsigned kClkEnPhase = 0;
constexpr unsigned kBusPhase = 8;  // mid-character phase without CLKEN

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class TestBench {
public:
    TestBench() {
        dut.CLOCK = 0;
        dut.CLKEN = 0;
        dut.PIXEN = 0;
        dut.nRESET = 0;
        dut.VIDEOD = 0;
        dut.GAMODE = 0;
        dut.BORDER_I = 20;   // black border by default
        dut.SPLT = 0;
        dut.SSA = 0;
        dut.SSCR = 0;
        dut.SPR_EN = 0;
        dut.SPR_RGB = 0;
        dut.PAL_EN = 0;      // default: internal [KT] legacy-colour ROM
        dut.PAL_RGB = 0;
        for (unsigned k = 0; k < 16; ++k) {
            set_inkr(k, 20);
        }
        idle_bus();
        dut.eval();
        reset();
    }

    ~TestBench() { dut.final(); }

    TestBench(const TestBench&) = delete;
    TestBench& operator=(const TestBench&) = delete;

    void reset() {
        idle_bus();
        dut.nRESET = 0;
        while (tick_in_character_ != kClkEnPhase) {
            clock_tick();
        }
        for (unsigned tick = 0; tick < ticks_per_character_; ++tick) {
            clock_tick();
        }
        dut.nRESET = 1;
        dut.eval();
    }

    void write_register(std::uint8_t address, std::uint8_t value) {
        run_to_bus_phase();
        bus_edge(false, address);
        bus_edge(true, value);
        idle_bus();
        dut.eval();
    }

    void select_register(std::uint8_t address) {
        run_to_bus_phase();
        bus_edge(false, address);
        idle_bus();
        dut.eval();
    }

    // P5 readback is combinational and several status bits last one
    // character only (ACCC §21.3.4 p.248). Sample without advancing the
    // clock so the assertion names the counter state actually observed.
    std::uint8_t sample_selected(bool rs = true, bool selected = true,
                                 bool read_cycle = true) {
        dut.ENABLE = 1;
        dut.nCS = selected ? 0 : 1;
        dut.R_nW = read_cycle ? 1 : 0;
        dut.RS = rs ? 1 : 0;
        dut.DI = 0;
        dut.eval();
        const std::uint8_t value = dut.DO;
        idle_bus();
        dut.eval();
        return value;
    }

    std::uint8_t read_register(std::uint8_t address) {
        select_register(address);
        return sample_selected();
    }

    void run_characters(std::uint64_t characters) {
        for (std::uint64_t character = 0; character < characters; ++character) {
            for (unsigned tick = 0; tick < ticks_per_character_; ++tick) {
                clock_tick();
            }
        }
    }

    // Advance one character at a time until the counters sit at a frame
    // start (C4=C9=C0=0 sampled just after a CLKEN edge).
    unsigned vsync() const { return dut.VSYNC; }

    void dbg_state(const char* where) const {
        std::printf("DBG %s: HCC=%u LINE=%u ROW=%u MA=%u RA=%u DE=%u\n",
                    where, dut.HCC, dut.LINE, dut.ROW, dut.MA, dut.RA, dut.DE);
    }

    void run_to_frame_start() {
        unsigned guard = 0;
        do {
            run_characters(1);
            if (++guard > 4096) {
                throw TestFailure("run_to_frame_start did not converge");
            }
        } while (!(dut.LINE == 0 && dut.ROW == 0 && dut.HCC == 0));
    }

    void run_to_state(unsigned line, unsigned row, unsigned hcc,
                      const std::string& context) {
        unsigned guard = 0;
        while (!(dut.LINE == line && dut.ROW == row && dut.HCC == hcc)) {
            run_characters(1);
            if (++guard > 8192) {
                throw TestFailure(context + ": counter state did not converge");
            }
        }
    }

    // Run until the VSYNC pin is observed low: flushes any pulse that
    // legitimately persists out of the pinned R0=0 programming phase
    // (with R7=0 the §16.4.4 condition holds at every line start and the
    // pulse renews without a gap).
    void run_until_vsync_idle() {
        unsigned guard = 0;
        do {
            run_characters(1);
            if (++guard > 8192) {
                throw TestFailure("run_until_vsync_idle did not converge");
            }
        } while (dut.VSYNC != 0);
    }

    void run_until_hsync_idle() {
        unsigned guard = 0;
        do {
            run_characters(1);
            if (++guard > 256) {
                throw TestFailure("run_until_hsync_idle did not converge");
            }
        } while (dut.HSYNC != 0);
    }

    void run_until_adjustment() {
        unsigned guard = 0;
        do {
            run_characters(1);
            if (++guard > 4096) {
                throw TestFailure("run_until_adjustment did not converge");
            }
        } while (dut.ADJ == 0);
    }

    void expect_hcc(const std::string& expectation, unsigned expected) const {
        if (dut.HCC != expected) {
            fail(expectation + ": HCC", expected, dut.HCC);
        }
    }

    void expect_line(const std::string& expectation, unsigned expected) const {
        if (dut.LINE != expected) {
            fail(expectation + ": LINE(C4)", expected, dut.LINE);
        }
    }

    void expect_row(const std::string& expectation, unsigned expected) const {
        if (dut.ROW != expected) {
            fail(expectation + ": ROW(C9)", expected, dut.ROW);
        }
    }

    void expect_adj(bool expected, const std::string& expectation) const {
        if ((dut.ADJ != 0) != expected) {
            fail(expectation + ": ADJ", expected ? 1 : 0, dut.ADJ);
        }
    }

    void expect_de(const std::string& expectation, bool expected) const {
        if ((dut.DE != 0) != expected) {
            fail(expectation + ": DE", expected ? 1 : 0, dut.DE);
        }
    }

    void expect_ma(const std::string& expectation, unsigned expected) const {
        if (dut.MA != expected) {
            fail(expectation + ": MA", expected, dut.MA);
        }
    }

    void expect_hsync(const std::string& expectation, bool expected) const {
        if ((dut.HSYNC != 0) != expected) {
            fail(expectation + ": HSYNC", expected ? 1 : 0, dut.HSYNC);
        }
    }

    void expect_vsync(const std::string& expectation, bool expected) const {
        if ((dut.VSYNC != 0) != expected) {
            fail(expectation + ": VSYNC", expected ? 1 : 0, dut.VSYNC);
        }
    }

    void expect_ra(const std::string& expectation, unsigned expected) const {
        if (dut.RA != expected) {
            fail(expectation + ": RA", expected, dut.RA);
        }
    }

    // ---- Locked-ASIC pixel-path helpers (t05x vectors) ----

    void set_inkr(unsigned pen, unsigned hw_colour) {
        // INKR_I entry k occupies bits [k*5 +: 5]; Verilator models the
        // 80-bit port as three 32-bit words.
        for (unsigned b = 0; b < 5; ++b) {
            const unsigned idx = pen * 5 + b;
            const unsigned word = idx / 32;
            const unsigned off = idx % 32;
            if ((hw_colour >> b) & 1U) {
                inkr_[word] |= (1U << off);
            } else {
                inkr_[word] &= ~(1U << off);
            }
        }
        dut.INKR_I[0] = inkr_[0];
        dut.INKR_I[1] = inkr_[1];
        dut.INKR_I[2] = inkr_[2];
    }

    void set_ga(unsigned mode, unsigned border, unsigned videod_word) {
        dut.GAMODE = mode;
        dut.BORDER_I = border;
        dut.VIDEOD = videod_word;
    }

    // Sprite-plane inputs (P4 precedence vectors t06*).
    void set_sprite(unsigned en, unsigned r, unsigned g, unsigned b) {
        dut.SPR_EN = en;
        dut.SPR_RGB = (r << 8) | (g << 4) | b;
    }

    // Stand-in for asic_regs' 12-bit palette port (HF-2). asic_regs
    // registers the video-side read, but on the motherboard PIXEN is one
    // clock in four, so the registered word is already settled when the dot
    // edge samples it. By default this bench runs PIXEN every clock, so the
    // model is combinational to reproduce that same alignment.
    void enable_asic_palette() {
        dut.PAL_EN = 1;
        pal_model_ = true;
        pal_registered_ = false;
        drive_palette();
    }

    // The motherboard's actual arrangement: PAL_RGB is asic_regs' registered
    // read, one CLOCK behind PAL_ADDR. Pair with set_pixen_divider(4) so a
    // dot lasts four clocks and the registered word has settled before the
    // next PIXEN edge samples it (t05j).
    void enable_asic_palette_registered() {
        dut.PAL_EN = 1;
        pal_model_ = true;
        pal_registered_ = true;
        dut.PAL_RGB = 0;
        dut.eval();
    }

    // Slow PIXEN to one clock in `period`, keeping 16 dots per character.
    // Must be called on a character boundary so CLKEN stays coincident with
    // a PIXEN clock, as it is on the motherboard.
    void set_pixen_divider(unsigned period) {
        align_to_character_start();
        pixen_period_ = period;
        ticks_per_character_ = kClockTicksPerCharacter * period;
    }

    // Entry as asic_regs stores it: {G,R,B} nibbles (reference §6).
    void set_pal(unsigned entry, unsigned g, unsigned r, unsigned b) {
        pal_[entry & 31] = static_cast<std::uint16_t>(
            ((g & 15) << 8) | ((r & 15) << 4) | (b & 15));
        drive_palette();
    }

    void set_border(unsigned hw_colour) { dut.BORDER_I = hw_colour; }
    void set_mode(unsigned mode) { dut.GAMODE = mode; }
    void set_videod(unsigned videod_word) { dut.VIDEOD = videod_word; }
    void set_splt(std::uint8_t splt) { dut.SPLT = splt; dut.eval(); }
    void set_ssa(std::uint16_t ssa) { dut.SSA = ssa & 0x3FFFU; dut.eval(); }
    void set_sscr(std::uint8_t sscr) { dut.SSCR = sscr; dut.eval(); }
    bool hsync() const { return dut.HSYNC != 0; }

    void run_dots(unsigned dots) {
        for (unsigned d = 0; d < dots; ++d) {
            for (unsigned t = 0; t < pixen_period_; ++t) {
                clock_tick();
            }
        }
    }

    // Advance to the tick boundary so the next clock_tick() executes dot 0
    // of a character (the CLKEN edge that latches vid_even and de_hold).
    void align_to_character_start() {
        while (tick_in_character_ != kClkEnPhase) {
            clock_tick();
        }
    }

    void expect_rgb(const std::string& expectation, unsigned r, unsigned g,
                    unsigned b) const {
        if (dut.RGB_R != r || dut.RGB_G != g || dut.RGB_B != b) {
            fail(expectation + ": RGB", (r * 256) + (g * 16) + b,
                 (dut.RGB_R * 256) + (dut.RGB_G * 16) + dut.RGB_B);
        }
    }

    void expect_pal_addr(const std::string& expectation,
                         unsigned expected) const {
        if (dut.PAL_ADDR != expected) {
            fail(expectation + ": PAL_ADDR", expected,
                 static_cast<unsigned>(dut.PAL_ADDR));
        }
    }

    void expect_pen(const std::string& expectation, bool border,
                    unsigned nibble) const {
        const unsigned expected = (border ? 0x10U : 0U) | nibble;
        if (dut.PEN != expected) {
            fail(expectation + ": PEN", expected, static_cast<unsigned>(dut.PEN));
        }
    }

public:
    [[noreturn]] static void fail_unsigned(const std::string& what,
                                           unsigned expected,
                                           unsigned actual) {
        throw TestFailure(what + ": expected " + std::to_string(expected) +
                          ", actual " + std::to_string(actual));
    }

private:
    void run_to_bus_phase() {
        while (tick_in_character_ != kBusPhase) {
            clock_tick();
        }
    }

    void bus_edge(bool register_data, std::uint8_t value) {
        dut.ENABLE = 1;
        dut.nCS = 0;
        dut.R_nW = 0;
        dut.RS = register_data ? 1 : 0;
        dut.DI = value;
        clock_tick();
    }

    void idle_bus() {
        dut.ENABLE = 0;
        dut.nCS = 1;
        dut.R_nW = 1;
        dut.RS = 0;
        dut.DI = 0;
    }

    // PAL_ADDR depends only on registered state, so one settling pass is
    // enough; a no-op unless a test enabled the palette model.
    void drive_palette() {
        if (!pal_model_ || pal_registered_) {
            return;
        }
        dut.PAL_RGB = pal_[dut.PAL_ADDR & 31];
        dut.eval();
    }

    void clock_tick() {
        dut.CLKEN = tick_in_character_ == kClkEnPhase ? 1 : 0;
        // One dot per PIXEN clock, 16 dots per character. pixen_period_ == 1
        // is the default (every tick is a dot); set_pixen_divider() slows it
        // to the motherboard's one-in-four.
        dut.PIXEN = (tick_in_character_ % pixen_period_) == 0 ? 1 : 0;
        dut.CLOCK = 0;
        dut.eval();
        drive_palette();
        // asic_regs samples PAL_ADDR as it stands before the rising edge.
        const unsigned pal_addr_at_edge = dut.PAL_ADDR & 31U;
        dut.CLOCK = 1;
        dut.eval();
        if (pal_registered_) {
            dut.PAL_RGB = pal_[pal_addr_at_edge];
            dut.eval();
        }
        drive_palette();
        dut.CLOCK = 0;
        dut.eval();
        drive_palette();
        tick_in_character_ = (tick_in_character_ + 1) % ticks_per_character_;
    }

    template <typename Expected, typename Actual>
    [[noreturn]] static void fail(const std::string& what,
                                  Expected expected, Actual actual) {
        throw TestFailure(what + ": expected " + std::to_string(expected) +
                          ", actual " + std::to_string(actual));
    }

    Vasic_video dut;
    std::uint32_t inkr_[3] = {0, 0, 0};
    unsigned tick_in_character_ = 0;
    unsigned pixen_period_ = 1;
    unsigned ticks_per_character_ = kClockTicksPerCharacter;
    std::uint16_t pal_[32] = {0};
    bool pal_model_ = false;
    bool pal_registered_ = false;
};

// Program a standard frame: R0=7 (8-char lines), R9=3 (4-line char rows),
// R4=2 (3 char rows), R5=0 -> 3*4*8 = 96 characters per frame. Counter
// state is pinned at zero while R0=R4=R9=0 during programming, so callers
// align with run_to_frame_start() before reasoning about positions.
void program_standard_frame(TestBench& test) {
    test.write_register(0x00, 7);
    test.write_register(0x09, 3);
    test.write_register(0x04, 2);
    test.write_register(0x05, 0);
}

// Standard frame plus display programming: R1=4 displayed characters,
// R6=100 (never reached: no border), video base 0x1234 via R12/R13.
void program_display_frame(TestBench& test) {
    test.write_register(9, 3);
    test.write_register(4, 2);
    test.write_register(5, 0);
    test.write_register(1, 4);
    test.write_register(6, 100);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.write_register(0, 7);  // last: starts the 8-char line cadence
}

// Power-on R0=0 makes every character a complete line: C0 pins to 0. This
// is the §13.5 (p.121) "R0 accepts all values" acceptance at the reset
// value, and doubles as the reset contract for the counter.
void t01a_reset_and_r0_zero(TestBench& test) {
    test.expect_hcc("reset clears C0", 0);
    test.run_characters(10);
    test.expect_hcc("R0=0 keeps C0 pinned at 0 (ACCC §13.5)", 0);
}

// Program R0=63 mid-character; the live equality (ACCC §13.1, p.102) makes
// the line exactly 64 characters: C0 walks 1..63 then wraps to 0 twice.
void t01b_r63_period(TestBench& test) {
    test.write_register(0x00, 63);
    // One character completes after the write (the pending CLKEN edge).
    test.run_characters(1);
    test.expect_hcc("first post-write character advances C0 to 1", 1);
    for (unsigned step = 2; step <= 63; ++step) {
        test.run_characters(1);
        test.expect_hcc("C0 climb to R0", step);
    }
    test.run_characters(1);
    test.expect_hcc("C0 wraps at live C0=R0 equality (ACCC §13.1)", 0);
    test.run_characters(1);
    test.expect_hcc("next line restarts from 1", 1);
}

// Only the five low address bits select a register (same decode family as
// the classic types; ACCC §28.1.9 notes the type-3 read side narrows this
// further, which is P5 scope). Behaviour here proves the select landed.
void t01c_register_select_alias(TestBench& test) {
    test.write_register(0xE0, 3);  // select alias for index 0
    test.run_characters(1);
    test.expect_hcc("select 0xE0 addressed R0: line is 4 characters", 1);
    test.run_characters(2);
    test.expect_hcc("C0 reaches R0=3", 3);
    test.run_characters(1);
    test.expect_hcc("wrap on the four-character line", 0);
}

// Widening R0 mid-line extends the current line (ACCC §13.5: any R0 value
// is accepted without disturbing other counters).
//
// Tick accounting: write_register positions the bus mid-character, which
// consumes exactly one CLKEN edge at the old line width before the new
// value lands. From a precondition of C0=5 the write therefore observes
// C0=6 internally, and each following character climbs one step from 6.
void t01d_r0_widen_midline(TestBench& test) {
    test.write_register(0x00, 10);
    test.run_characters(5);  // C0 now 5
    test.expect_hcc("precondition C0=5", 5);
    test.write_register(0x00, 63);
    // The equality moved away; C0 keeps climbing through the old limit.
    for (unsigned step = 7; step <= 63; ++step) {
        test.run_characters(1);
        test.expect_hcc("widened line continues past old R0", step);
    }
    test.run_characters(1);
    test.expect_hcc("wrap only at the new R0", 0);
}

// ACCC §13.5 (p.121) says CRTC3/4 accept all R0 values without the type-0
// freeze/stall. It does not provide a direct CRTC3 chronogram for shrinking
// R0 below the current C0. The exact 20..255..0..10 sequence asserted here is
// therefore an explicitly unverified P1 model assumption, retained as a
// regression expectation pending a direct rule/chronogram, Logon observation,
// or hardware capture. Do not use §28.1.1 as support: that section describes
// C4/C9 identification overflow, not this C0 case.
void t01e_r0_shrink_overflow(TestBench& test) {
    test.write_register(0x00, 63);
    test.run_characters(20);  // C0 now 20
    test.expect_hcc("precondition C0=20", 20);
    test.write_register(0x00, 10);
    // One CLKEN elapsed during bus positioning: C0 resumes its climb at 21
    // and cannot meet the lowered equality until the full eight-bit range
    // has overflowed.
    for (unsigned step = 22; step <= 255; ++step) {
        test.run_characters(1);
        test.expect_hcc("overflow climb toward 255", step);
    }
    test.run_characters(1);
    test.expect_hcc("eight-bit overflow wraps to 0", 0);
    for (unsigned step = 1; step <= 10; ++step) {
        test.run_characters(1);
        test.expect_hcc("climb to the lowered R0", step);
    }
    test.run_characters(1);
    test.expect_hcc("equality reached: normal wrap resumes", 0);
}


// ACCC §14.6.2 p.142 / §15.2.2 p.147: HSYNC starts at C0=R2 and lasts R3l
// characters (§14.1). Width 5 starting at R2=6 covers characters 6..10
// across the line boundary.
void t04a_hsync_position_and_width(TestBench& test) {
    program_display_frame(test);
    test.write_register(2, 255);
    test.write_register(3, 0x65);  // H width 5, V width 6
    // Discard the reset-programming pulse before checking the steady
    // width. A type-3 end/start collision can legitimately prolong that
    // transient (§15.3.1), while this vector is about §14 normal timing.
    test.run_until_hsync_idle();
    test.write_register(2, 6);
    test.run_to_frame_start();
    for (unsigned k = 0; k <= 15; ++k) {
        if (k > 0) {
            test.run_characters(1);
        }
        // Width 5 from C0=R2=6 on an 8-character line wraps: chars
        // 6,7 of every line plus 0,1,2 of the following one.
        const unsigned phase = k % 8;
        const bool high = (phase <= 2) || (phase >= 6);
        test.expect_hsync(("HSYNC wrap k=" + std::to_string(k)).c_str(), high);
    }
}

// ACCC §14.5 p.141: on types 2/3/4 an R3l of 0 still produces a
// 16-character HSYNC (full nibble), unlike types 0/1 which produce none.
void t04b_r3_zero_means_sixteen(TestBench& test) {
    program_display_frame(test);
    test.write_register(2, 6);
    test.write_register(3, 0x60);  // H width 0 -> 16
    test.run_to_frame_start();
    // Steady pattern with 8-character lines: each pulse lasts 16 chars
    // from C0=6, and a start landing inside an active pulse is ignored
    // (a new HSYNC cannot begin during one, ACCC §15.3.1), giving 16 on
    // / 8 off. The aligned character sits 14 characters into the pulse
    // that the previous line started.
    for (unsigned k = 0; k <= 23; ++k) {
        if (k > 0) {
            test.run_characters(1);
        }
        test.expect_hsync("R3l=0 gives a full-nibble HSYNC",
                          k < 14 || k >= 22);
    }
}

// Dynamic R3l rewrite below the already-counted value: the counter wraps
// its whole nibble before the new equality can hit, so the interrupted
// HSYNC is EXTENDED, not truncated (compendium-02 §4, ACCC §14.4 general
// rule incl. types 3/4). Start width 12 from C0=R2=6; a rewrite to 2
// landing during character 9 ends the pulse entering character 23.
void t04c_r3l_rewrite_wraps_nibble(TestBench& test) {
    program_display_frame(test);
    test.write_register(2, 6);
    test.write_register(3, 0x6C);  // H width 12, V width 6
    test.run_to_frame_start();
    test.run_characters(8);        // position: C0=8
    test.write_register(3, 0x62);  // lands mid-character 9; H width -> 2
    for (unsigned k = 9; k <= 26; ++k) {
        test.run_characters(1);
        const bool high = (k >= 10 && k <= 22);
        if (k == 10 || k == 22 || k == 23 || k == 26) {
            test.expect_hsync("nibble wrap extends the interrupted HSYNC",
                              high);
        }
    }
}

// ACCC §15.3.1/§15.3.2 p.148: with R0=0, R2=0, R3l=1 the width expires
// exactly on a C0=R2 character every time; types 1..4 keep the HSYNC
// asserted with the counter rolling through its wrapped nibble — the
// infinite-HSYNC configuration.
void t04d_infinite_hsync(TestBench& test) {
    test.write_register(2, 0);
    test.write_register(3, 0x01);
    test.write_register(0, 0);
    for (unsigned k = 1; k <= 40; ++k) {
        test.run_characters(1);
        test.expect_hsync("HSYNC restarts itself forever", true);
    }
}

// ACCC §15.3.5 p.151 CRTC3 chronogram: R2 starts at 11 with R3l=10,
// then a live R2=21 lands exactly where the active pulse would naturally
// end. C3 does not reset; it continues 10,11..15,0..10 and HSYNC therefore
// stays asserted through C0=36 before ending on entry to C0=37.
void t04h_live_r2_end_start_collision(TestBench& test) {
    program_display_frame(test);
    test.write_register(0, 63);
    test.write_register(2, 11);
    test.write_register(3, 0x6a);
    test.run_to_frame_start();

    test.run_characters(19);
    test.expect_hcc("live-R2 collision precondition", 19);
    test.expect_hsync("original R2=11 pulse is active", true);
    test.write_register(2, 21);

    test.expect_hcc("live R2 write lands before collision", 20);
    test.run_characters(1);
    test.expect_hcc("rewritten start meets natural end", 21);
    test.expect_hsync("end/start collision keeps HSYNC asserted", true);
    for (unsigned c0 = 22; c0 <= 36; ++c0) {
        test.run_characters(1);
        test.expect_hsync("C3 continues through nibble wrap", true);
    }
    test.run_characters(1);
    test.expect_hcc("second R3 equality position", 37);
    test.expect_hsync("continued pulse ends at second R3 equality", false);
}

// ACCC §14.5 p.141 establishes that type 3's R3l=0 encoding produces a
// 16-character HSYNC, but does not state whether the §15.3 end/start
// collision extends that pulse when its natural end lands on C0=R2. The
// current P1 model deliberately lets it end, then permits a fresh pulse at
// the following line start. This is an explicitly unverified model
// assumption, not an ACCC-derived expectation; keep it visible until a
// direct rule, Logon observation, or hardware capture settles the boundary.
void t04i_r3_zero_collision_stays_bounded(TestBench& test) {
    program_display_frame(test);
    test.write_register(2, 255);   // park starts while programming
    test.write_register(3, 0x60);  // R3l=0 -> 16-character pulse
    test.run_until_hsync_idle();
    test.write_register(0, 15);    // 16-character lines
    test.write_register(2, 0);     // every natural end meets a live start
    test.run_to_frame_start();

    // The first aligned C0=0 is the natural end of the pulse that was
    // already active while run_to_frame_start converged.
    test.expect_hsync("unverified model assumption lets collision end", false);
    for (unsigned c0 = 1; c0 <= 15; ++c0) {
        test.run_characters(1);
        test.expect_hsync("ended pulse stays low until the next start", false);
    }
    test.run_characters(1);
    test.expect_hcc("following line reaches the next start", 0);
    test.expect_hsync("fresh R3l=0 pulse starts after bounded gap", true);
    for (unsigned c0 = 1; c0 <= 15; ++c0) {
        test.run_characters(1);
        test.expect_hsync("R3l=0 pulse remains high for 16 characters", true);
    }
    test.run_characters(1);
    test.expect_hcc("R3l=0 natural end coincides with live start", 0);
    test.expect_hsync("zero-width collision remains bounded again", false);
}

// ACCC §16.4.4 p.170: VSYNC needs C4==R7 AND C9==0 AND C0==0 at a line
// start; rewriting R7 to the current C4 while C0>0 does NOT trigger
// until a qualifying line start arrives.
void t04e_vsync_gate_and_r7_write(TestBench& test) {
    // Positive control: R7=1 fires at the row-1 entry line start.
    program_display_frame(test);
    test.write_register(7, 1);
    test.write_register(3, 0x30);  // V width 3 lines
    test.run_until_vsync_idle();   // flush the pinned-phase pulse
    test.run_to_frame_start();
    unsigned first_high = 0;
    for (unsigned k = 1; k <= 95; ++k) {
        test.run_characters(1);
        if (first_high == 0 && test.vsync() != 0) {
            first_high = k;
        }
    }
    if (first_high != 32) {
        TestBench::fail_unsigned("VSYNC fires at row-1 entry (C4=R7, C9=C0=0)", 32, first_high);
    }

    // Negative: unreachable R7 during frame 1; a mid-row R7=1 write
    // lands after that row's qualifying line start has passed, so the
    // pulse waits for the NEXT frame's row-1 entry.
    TestBench late;
    program_display_frame(late);
    late.write_register(7, 200);
    late.write_register(3, 0x30);
    late.run_characters(200);      // flush the pinned-phase pulse
    late.run_to_frame_start();
    late.run_characters(40);       // inside row 1, past its entry line
    late.write_register(7, 1);     // C0>0: no trigger now
    bool fired_before_frame_end = false;
    for (unsigned k = 41; k <= 95; ++k) {   // rest of frame 1
        late.run_characters(1);
        if (late.vsync() != 0) {
            fired_before_frame_end = true;
        }
    }
    if (fired_before_frame_end) {
        TestBench::fail_unsigned(
            "mid-row R7 write stays inert until a line-start match", 0, 1);
    }
    // Frame 2 must fire at its row-1 entry (C4=R7=1, C9=C0=0) and the
    // 3-line pulse must be observable there.
    bool fired_next_frame = false;
    for (unsigned k = 0; k < 64; ++k) {     // frame 2 rows 0-1
        late.run_characters(1);
        if (late.vsync() != 0) {
            fired_next_frame = true;
        }
    }
    if (!fired_next_frame) {
        TestBench::fail_unsigned("next frame's row-1 entry fires", 1, 0);
    }
}

// ACCC §14.2 p.131: R3h programs the VSYNC length in lines, 0 meaning
// 16 (types 0/3/4).
void t04f_vsync_width(TestBench& test) {
    program_display_frame(test);
    test.write_register(7, 1);
    test.write_register(3, 0x35);  // V width 3
    test.run_until_vsync_idle();   // flush the pinned-phase pulse
    test.run_to_frame_start();
    for (unsigned k = 1; k <= 60; ++k) {
        test.run_characters(1);
        if (k == 31) {
            test.expect_vsync("pulse not yet started", false);
        }
        else if (k == 33 || k == 55) {
            test.expect_vsync("three programmed lines wide", true);
        }
        else if (k == 57) {
            test.expect_vsync("ends after the third line", false);
        }
    }

    TestBench legacy;
    program_display_frame(legacy);
    legacy.write_register(7, 1);
    legacy.write_register(3, 0x05);  // V width 0 -> 16 lines
    legacy.run_until_vsync_idle();
    legacy.run_to_frame_start();
    for (unsigned k = 1; k <= 170; ++k) {
        legacy.run_characters(1);
        if (k == 33 || k == 150) {
            legacy.expect_vsync("legacy 16-line VSYNC", true);
        }
        else if (k == 162) {
            legacy.expect_vsync("sixteen lines end the pulse", false);
        }
    }
}

// ACCC §16.4.4: there is NO re-entrancy protection — with the qualifying
// condition true at every line start (R7=R4=R9=0), a finished pulse is
// renewed immediately, which for this degenerate programming means the
// pin never visibly drops between renewals.
void t04g_no_reentrancy_continuous_refire(TestBench& test) {
    test.write_register(9, 0);
    test.write_register(4, 0);
    test.write_register(7, 0);
    test.write_register(5, 0);
    test.write_register(3, 0x20);  // V width 2 lines
    test.write_register(0, 1);     // 2-character lines
    for (unsigned k = 1; k <= 30; ++k) {
        test.run_characters(1);
        test.expect_vsync("renewed VSYNC whenever inactive", true);
    }
}

struct TestCase {
    const char* name;
    void (*run)(TestBench&);
};

constexpr unsigned kBase = 0x1234;

// ACCC §20.3.4 p.243 + §17.1 p.176: both pointers reload from R12/R13 at
// the frame start (C4=0 & C0=0); VMA counts every character cell; the
// row-end capture (C0=R1 & C9=R9) advances the row start by R1.
void t03a_ma_reload_and_row_advance(TestBench& test) {
    program_display_frame(test);
    test.run_to_frame_start();
    test.expect_ma("frame start reloads VMA from R12/R13", kBase);
    for (unsigned k = 1; k <= 7; ++k) {
        test.run_characters(1);
        if (k == 1 || k == 4 || k == 7) {
            test.expect_ma("VMA increments on every character cell", kBase + k);
        }
    }
    test.run_characters(25);  // chars 8..32: lines 2-4 of row 0 + row-1 start
    test.expect_ma("row 1 restarts from the captured VMA' (=base+R1)",
                   kBase + 4);
    test.run_characters(32);
    test.expect_ma("row 2 advances by R1 again", kBase + 8);
    test.run_characters(32);
    test.expect_ma("next frame wrap reloads R12/R13 into both pointers",
                   kBase);
}

// ACCC §17.1 p.175/§17.6.1: DISPTMG on at C0=0, off at C0=R1; with
// R1<R0 the rest of the line is border while VMA keeps counting.
void t03b_r1_border_edges(TestBench& test) {
    program_display_frame(test);
    test.run_to_frame_start();
    for (unsigned k = 0; k <= 7; ++k) {
        if (k > 0) {
            test.run_characters(1);
        }
        test.expect_de("display window is C0 in [0,R1)", k < 4);
        if (k == 6) {
            test.expect_ma("pointer keeps counting through border",
                           kBase + 6);
        }
    }
}

// ACCC §17.6.1 p.185 (all types): with R1==R0 exactly one border
// character appears at C0=R0 before the next line reloads DISPTMG.
void t03c_r1_eq_r0_blip(TestBench& test) {
    program_display_frame(test);
    test.write_register(0x01, 7);
    test.run_to_frame_start();
    for (unsigned k = 0; k <= 7; ++k) {
        if (k > 0) {
            test.run_characters(1);
        }
        test.expect_de("only the C0=R0 character borders", k < 7);
    }
    // ACCC §17.1 p.176 and §17.6.1 p.185: the simultaneous C0=R1=R0
    // row-end capture still advances VMA' normally. After the remaining
    // scanlines of row 0, row 1 must therefore start at base+R1 rather
    // than repeating the old row base.
    test.run_characters(25);
    test.expect_line("R1==R0 reaches the next character row", 1);
    test.expect_ma("R1==R0 simultaneous save/reload advances VMA'",
                   kBase + 7);
}

// ACCC §17.6.2/§19.2.4 (types 3/4 grouped with type 1): with R1>R0 no
// spurious border byte is substituted at C0=R0 — the whole line stays
// displayed — and §17.2 p.179 makes every row re-display the frozen VMA'
// base (capture can never fire).
void t03d_r1_gt_r0_no_substitution(TestBench& test) {
    program_display_frame(test);
    test.write_register(0x01, 9);
    test.run_to_frame_start();
    for (unsigned k = 0; k <= 7; ++k) {
        if (k > 0) {
            test.run_characters(1);
        }
        test.expect_de("whole line displayed when R1>R0", true);
    }
    test.run_characters(25);  // from mid-char 7 to the row-1 line start
    test.expect_ma("row 1 re-displays the frozen base (§17.2)", kBase);
    test.expect_line("positioned at char row 1", 1);
    test.run_characters(32);
    test.expect_de("row 2 still fully displayed", true);
    test.expect_ma("row 2 repeats the same base", kBase);
}

// ACCC §18.2.4 p.189: the R6 test runs only at the beginning of a line;
// a mid-line update is not considered until the next line start, and
// there is no per-C0 evaluation (contrast types 0/1).
// ACCC §18.2.4 p.189: the R6 test runs only at the beginning of a line;
// a mid-line update is not considered until the next line start, and
// there is no per-C0 evaluation (contrast types 0/1). §18.3.4: R6=0 has
// no special case. R1>R0 keeps the horizontal term out of the way so DE
// mirrors the vertical decision alone.
//
// Tick accounting: every write_register consumes one CLKEN while moving
// the bus to mid-character; samples below name the character they land on.
void t03e_r6_line_start_semantics(TestBench& test) {
    program_display_frame(test);
    test.write_register(1, 9);  // R1>R0: DE mirrors the vertical term only
    test.run_to_frame_start();
    test.run_characters(3);        // sample position: body line 0, C0=3
    test.write_register(6, 0);     // C4==R6==0 from here, but mid-line;
                                   // positioning consumed C0=4's edge
    test.run_characters(3);        // characters 5..7 of that same line
    test.expect_de("line unaffected by mid-line R6 write", true);
    test.run_characters(1);        // next line start: C4==R6 -> border
    test.expect_de("border activates at the NEXT line start only", false);
    test.run_characters(7);        // remainder of that bordered line
    test.expect_de("whole line bordered while C4==R6 holds", false);
    // Positioning for the next write consumes the line-end edge into the
    // second bordered line; that start still evaluates the OLD R6 (=0).
    test.write_register(6, 7);     // new value lands mid-line
    test.run_characters(4);        // later characters of that line
    test.expect_de("mid-line R6 change does not clear the border", false);
    test.run_characters(4);        // next line start re-evaluates C4!=R6
    test.expect_de("display resumes once C4!=R6 at a line start", true);
}

// ACCC §11.2.6 p.84 + §20.3.4: adjustment lines do not update the video
// pointer; each one restores the captured row base, and RA carries the
// adjustment index instead of a fresh scanline count.
void t03f_adjustment_rows_solidified(TestBench& test) {
    program_display_frame(test);
    test.run_to_frame_start();
    // Arm adjustment from a known frame start; a mid-frame R5 write is
    // considered at that line's end (ACCC §11.4.1).
    test.write_register(5, 2);
    test.run_until_adjustment();
    test.expect_adj(true, "adjustment entered");
    // Row 2 began at base+8 and captured VMA'=base+12 at C0=R1 of its
    // last scanline (C9=R9).
    test.expect_ma("adjustment line restores the solidified base",
                   kBase + 12);
    test.expect_ra("RA indexes the adjustment lines", 0);
    test.run_characters(8);
    test.expect_ra("second adjustment line", 1);
    test.expect_ma("pointer unchanged between adjustment lines",
                   kBase + 12);
    test.expect_de("R6=100 leaves adjustment rows displayed", true);
}

// ACCC §18.1 / §18.2.4: Active vertical display requires charline < R6.
// Rows at or past R6 (C4 >= R6 up to R4 and vertical adjustment) must stay
// bordered (vde=0) and not wrap back to active display.
void t03h_vde_rows_past_r6_stay_bordered(TestBench& test) {
    program_display_frame(test);
    test.write_register(4, 5);  // R4=5: 6 character rows (0..5)
    test.write_register(6, 2);  // R6=2: rows 0 and 1 displayed, 2..5 bordered
    test.write_register(9, 1);  // R9=1: 2 scanlines per row
    test.write_register(1, 4);  // R1=4: C0 0..3 displayed within line
    test.run_to_frame_start();

    // Row 0 (displayed)
    test.expect_line("row 0", 0);
    test.expect_de("row 0 displayed", true);
    test.run_characters(8 * 2); // 2 lines

    // Row 1 (displayed)
    test.expect_line("row 1", 1);
    test.expect_de("row 1 displayed", true);
    test.run_characters(8 * 2); // 2 lines

    // Rows 2..5 (must stay bordered for all scanlines)
    for (unsigned row = 2; row <= 5; ++row) {
        test.expect_line("row >= R6", row);
        for (unsigned c0 = 0; c0 < 8; ++c0) {
            test.expect_de("rows past R6 must stay bordered", false);
            test.run_characters(1);
        }
        for (unsigned c0 = 0; c0 < 8; ++c0) {
            test.expect_de("scanline 1 of row past R6 must stay bordered", false);
            test.run_characters(1);
        }
    }

    // Frame restarts at Row 0: display resumes
    test.expect_line("frame restart row 0", 0);
    test.expect_de("display resumes at frame restart", true);
}

// ACCC §19.2.3 p.193-194 (SKEW-DISPTMG exists on types 0/3/4): delay +1
// shifts both visible border edges by one character; mode 3 forces
// BORDER ON regardless of R1/R6 state.
void t03g_skew_delay_and_border_on(TestBench& test) {
    program_display_frame(test);
    test.write_register(0x08, 0b00010000);  // SKEW-DISPTMG delay +1
    test.run_to_frame_start();
    for (unsigned k = 0; k <= 7; ++k) {
        if (k > 0) {
            test.run_characters(1);
        }
        const bool expected = (k >= 1 && k <= 4);
        test.expect_de("delay +1 shifts the window to C0 1..R1", expected);
    }

    TestBench border_on;
    program_display_frame(border_on);
    border_on.write_register(0x08, 0b00110000);  // BORDER ON
    border_on.run_to_frame_start();
    for (unsigned k = 0; k <= 15; ++k) {
        border_on.run_characters(1);
        border_on.expect_de("BORDER ON suppresses DISPTMG output", false);
    }
}

void t02a_normal_frame_cycle(TestBench& test) {
    program_standard_frame(test);
    test.run_to_frame_start();
    // Row accounting per ACCC §6.1.4 skeleton with the type-3 rules of
    // §10/§12: raster climbs within R9, charline within R4, then the
    // frame wraps (R5=0).
    test.run_characters(8);
    test.expect_row("first line completion advances C9", 1);
    test.expect_line("still character row 0", 0);
    test.run_characters(16);
    test.expect_row("two more lines complete the row count", 3);
    test.expect_line("row 0 still displayed-counting", 0);
    test.run_characters(8);   // fourth line completes the char row
    test.expect_row("row completion resets C9", 0);
    test.expect_line("and increments C4", 1);
    test.run_characters(32);
    test.expect_line("second row completes", 2);
    test.run_characters(32);
    test.expect_line("third row completes: frame wrap (ACCC §12.5)", 0);
    test.expect_row("new frame restarts C9 at 0", 0);
    test.expect_adj(false, "no adjustment with R5=0");
}

// ACCC §10.3.4 p.77: "If current-C9 > R9 then next-C9=0" — lowering R9
// below the running C9 forces the reset on the next line with normal row
// accounting (impossible to overflow C9). Previous R9 family table,
// C4<>R4 case: next line C9=0, C4=C4+1.
void t02b_r9_lowered_forces_reset(TestBench& test) {
    program_standard_frame(test);
    test.run_to_frame_start();
    test.run_characters(16);  // start of third scanline: C9=2
    test.expect_row("precondition C9=2", 2);
    // Write lands mid-line (bus positioning consumes one CLKEN; the line
    // is still on raster 2).
    test.write_register(0x09, 1);
    test.expect_row("write mid-row leaves current line untouched", 2);
    test.run_characters(8);
    test.expect_row("next-C9=0 forced by C9>R9 (ACCC §10.3.4)", 0);
    test.expect_line("row completed early: C4 incremented", 1);
    // Subsequent rows now hold two lines each (0..1).
    test.run_characters(8);
    test.expect_row("rows count to the lowered R9", 1);
    test.expect_line("same char row still", 1);
    test.run_characters(8);
    test.expect_row("next row starts", 0);
    test.expect_line("and C4 advanced again", 2);
}

// ACCC §12.5 p.101: an R4 updated below the current C4 makes the frame-end
// equality unreachable — "there is overflow of the C4 counter" (contrast
// with C9 above): C4 free-runs and the frame does not restart.
void t02c_r4_lowered_overflows(TestBench& test) {
    program_standard_frame(test);
    test.run_to_frame_start();
    test.run_characters(32);  // start of char row 1
    test.expect_line("precondition C4=1", 1);
    test.write_register(0x04, 0);
    test.run_characters(32);  // row 1 completes normally (C9 cycled)
    test.expect_line("row end with C4!=R4 increments C4 past old limit", 2);
    test.run_characters(32);
    test.expect_line("C4 keeps counting: equality unreachable", 3);
    test.expect_row("raster still cycles normally", 0);
    test.expect_adj(false, "no adjustment entered");
}

// ACCC §11.2.6 p.84: on types 3/4 entering vertical adjustment does NOT
// increment C4 — it stays equal to R4 — while C9 resets to 0.
void t02d_adjustment_entry_keeps_c4(TestBench& test) {
    test.write_register(0x00, 7);
    test.write_register(0x09, 3);
    test.write_register(0x04, 2);
    test.write_register(0x05, 2);
    test.run_until_adjustment();
    test.expect_adj(true, "adjustment active after last character row");
    test.expect_line("C4 frozen at R4, not incremented (ACCC §11.2.6)", 2);
    test.expect_row("adjustment lines index from C9=0", 0);
}

// ACCC §11.3.3 p.86 + §11.2.6: adjustment runs exactly R5 lines, then the
// next line is a fresh frame (C4=C9=0). With R0=7/R9=3 each line is 8
// characters; frame body = 3 rows x 4 lines, plus R5=2 adjustment lines.
void t02e_adjustment_length_and_restart(TestBench& test) {
    test.write_register(0x00, 7);
    test.write_register(0x09, 3);
    test.write_register(0x04, 2);
    test.write_register(0x05, 2);
    test.run_until_adjustment();      // first adjustment line in progress
    test.run_characters(8);           // it completes
    test.expect_row("second adjustment line indexes C9=1", 1);
    test.expect_adj(true, "still adjusting");
    test.expect_line("C4 still frozen at R4", 2);
    test.run_characters(8);           // adjustment line 1 == last (R5=2)
    test.expect_adj(false, "adjustment ended after R5 lines");
    test.expect_line("fresh frame starts", 0);
    test.expect_row("with C9=0", 0);
    // Full frame period: 3*4*8 + 2*8 = 112 characters. We sit at character
    // 0 of the new frame's first line; the body ends and adjustment entry
    // fires on the same edge after exactly 96 characters.
    test.run_characters(95);
    test.expect_adj(false, "still in the final body line before entry");
    test.run_characters(1);
    test.expect_adj(true, "second frame enters adjustment on schedule");
    test.expect_line("entry again freezes C4 at R4", 2);
    test.expect_row("and restarts the adjustment index", 0);
}

// ACCC §11.3.3: shrinking R5 below C9+1 during adjustment makes the
// current line the last one — management ends, no overflow chase.
void t02f_r5_shrink_ends_adjustment(TestBench& test) {
    test.write_register(0x00, 7);
    test.write_register(0x09, 3);
    test.write_register(0x04, 2);
    test.write_register(0x05, 4);
    test.run_until_adjustment();   // adjustment line index 0 in progress
    test.write_register(0x05, 1);  // lands during that line
    test.run_characters(8);        // index 0 completes: 0+1 >= 1
    test.expect_adj(false, "shrunken R5 ends adjustment immediately");
    test.expect_line("frame restarts", 0);
    test.expect_row("at C9=0", 0);
}

// ACCC §11.3.3 ("Whether with R5 or R9, it is impossible to overflow
// C9") combined with §11.3 general: growing R5 mid-adjustment extends the
// count to the new value.
void t02g_r5_grow_extends_adjustment(TestBench& test) {
    test.write_register(0x00, 7);
    test.write_register(0x09, 3);
    test.write_register(0x04, 2);
    test.write_register(0x05, 2);
    test.run_until_adjustment();   // index 0 in progress
    test.write_register(0x05, 5);  // lands during that line
    test.run_characters(8 * 4);    // indices 1..4 pass without ending
    test.expect_adj(true, "grown R5 keeps management active");
    test.expect_row("adjustment index follows", 4);
    test.expect_line("C4 frozen throughout", 2);
    test.run_characters(8);
    test.expect_adj(false, "ends when C9+1 reaches the new R5");
    test.expect_line("fresh frame", 0);
}

// ACCC v1.11 §19.8.4 pp.235-240 and §19.5.5 pp.213-215: in IVM (R8=3),
// an odd R9 alternates ParityC9 at each C4 transition, while a frame restart
// reloads ParityC9 from the newly toggled ParityFrame.  This fixture uses
// R9=7 (the documented 4/5-line alternation), R4=1, and R5=2.  R0=63 keeps
// the setup seam deterministic: its first reset-frame rollover is followed
// by one no-adjustment frame, leaving the next frame's ParityFrame even.
// R5 and R8 are then written at that frame origin, so the first adjustment
// below is the even-frame case.  The companion vectors below separately pin
// the additional-line and VSYNC phase rules.
void program_ivm_adjustment_frame(TestBench& test) {
    test.write_register(0, 63);
    test.write_register(1, 2);
    test.write_register(4, 1);
    // R5 remains zero while setup is aligned; this avoids an adjustment
    // during the parity-normalising frame below.
    test.write_register(6, 100);
    test.write_register(7, 100);
    test.write_register(9, 7);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.run_to_frame_start();
    test.write_register(5, 2);
    test.write_register(8, 3);
}

// ACCC v1.11 §19.8.4 pp.235-240: the even-frame terminal C9 is 7 on the
// final (odd-parity) C4, then R5 adjustment starts with C4 held at R4 and
// C9 reset to the adjustment index 0.  §11.2.6 p.84 says adjustment lines do
// not update the video pointer.  §20.3.4 p.243 supplies the row-end capture
// and frame-origin reload used to derive MA: R1=2 and two character rows
// place the adjustment at R12/R13 + 4, then the frame restart reloads the
// programmed base.  §19.5.5 pp.213-215 requires the restart C9 to expose the
// newly toggled ParityFrame (odd, hence C9=1 here).
void t02k_ivm_even_frame_adjustment(TestBench& test) {
    constexpr unsigned kLineCharacters = 64;
    constexpr unsigned kAdjustmentMa = kBase + 4;
    program_ivm_adjustment_frame(test);

    test.run_until_adjustment();
    test.expect_adj(true, "t02k even-frame adjustment entry is active");
    test.expect_line("t02k adjustment entry holds C4 at R4", 1);
    test.expect_row("t02k adjustment entry resets C9 to index 0", 0);
    test.expect_ma("t02k adjustment entry uses captured row base", kAdjustmentMa);

    test.expect_adj(true, "t02k adjustment line 0 remains active");
    test.expect_line("t02k adjustment line 0 holds C4", 1);
    test.expect_row("t02k adjustment line 0 has C9 index 0", 0);
    test.expect_ma("t02k adjustment line 0 reuses captured VMA'", kAdjustmentMa);
    test.run_characters(kLineCharacters);

    test.expect_adj(true, "t02k adjustment line 1 remains active");
    test.expect_line("t02k adjustment line 1 holds C4", 1);
    test.expect_row("t02k adjustment line 1 has C9 index 1", 1);
    test.expect_ma("t02k adjustment line 1 reuses captured VMA'", kAdjustmentMa);
    test.run_characters(kLineCharacters);

    // ACCC v1.11 §19.6.4 p.217: after the R5 lines, an even frame gains
    // one interlace line.  C4 is not incremented, C9 is unconditionally 0,
    // and the solidified pointer from the last C4 remains in use.
    test.expect_adj(false, "t02k added interlace line is not R5 adjustment");
    test.expect_line("t02k interlace line keeps C4 at R4", 1);
    test.expect_row("t02k interlace line forces C9 to zero", 0);
    test.expect_de("t02k carried C4 remains inside the R6 display window", true);
    test.expect_ma("t02k interlace line keeps captured VMA'", kAdjustmentMa);
    test.run_characters(17);
    test.expect_adj(false, "t02k mid-line remains outside R5 adjustment");
    test.expect_line("t02k mid-line keeps the terminal C4", 1);
    test.expect_row("t02k mid-line keeps the forced C9", 0);
    test.expect_ma("t02k pointer advances within the added line",
                   kAdjustmentMa + 17);
    test.run_characters(kLineCharacters - 17);

    test.expect_line("t02k frame restart resets C4 after interlace line", 0);
    test.expect_row("t02k frame restart aligns C9 with odd ParityFrame", 1);
    test.expect_ma("t02k frame restart reloads R12/R13", kBase);
}

// ACCC v1.11 §19.8.4 pp.235-240: after the even-frame adjustment restarts
// into an odd frame, the first C4 uses C9=1,3,5,7 and the next C4 uses
// C9=0,2,4,6,8.  §19.5.5 pp.213-215 then aligns ParityC9 to the toggled
// ParityFrame at the following restart.  This is the odd-frame companion to
// t02k; it deliberately checks the body sequence only to establish which
// parity reaches the adjustment seam, and makes no VSYNC/additional-line
// claim.
void t02l_ivm_odd_frame_adjustment(TestBench& test) {
    constexpr unsigned kLineCharacters = 64;
    constexpr unsigned kAdjustmentMa = kBase + 4;
    program_ivm_adjustment_frame(test);

    // Consume the even frame's adjustment, exposing the next frame's odd
    // ParityFrame and the simultaneous R12/R13 VMA reload.
    test.run_until_adjustment();
    // Two R5 lines plus the even frame's §19.6.4 interlace line.
    test.run_characters(kLineCharacters * 3);
    test.expect_adj(false, "t02l even-frame adjustment has ended");
    test.expect_line("t02l odd frame restarts C4 at zero", 0);
    test.expect_row("t02l odd frame starts on C9 parity 1", 1);
    test.expect_ma("t02l odd frame reloads R12/R13", kBase);

    for (unsigned expected : {1U, 3U, 5U, 7U}) {
        test.expect_line("t02l odd frame first C4", 0);
        test.expect_row("t02l odd frame C9 sequence", expected);
        test.expect_ma("t02l odd frame first row base", kBase);
        test.run_characters(kLineCharacters);
    }
    test.expect_line("t02l odd frame advances C4 after C9=7", 1);
    test.expect_row("t02l odd frame toggles ParityC9 for C4=1", 0);
    test.expect_ma("t02l odd frame advances captured VMA'", kBase + 2);

    for (unsigned expected : {0U, 2U, 4U, 6U, 8U}) {
        test.expect_line("t02l odd frame second C4", 1);
        test.expect_row("t02l odd frame even C9 sequence", expected);
        test.expect_ma("t02l odd frame second row base", kBase + 2);
        test.run_characters(kLineCharacters);
    }

    test.expect_adj(true, "t02l odd-frame adjustment entry is active");
    test.expect_line("t02l adjustment entry holds C4 at R4", 1);
    test.expect_row("t02l adjustment entry resets C9 to index 0", 0);
    test.expect_ma("t02l adjustment entry uses captured row base", kAdjustmentMa);

    test.expect_adj(true, "t02l adjustment line 0 remains active");
    test.expect_line("t02l adjustment line 0 holds C4", 1);
    test.expect_row("t02l adjustment line 0 has C9 index 0", 0);
    test.expect_ma("t02l adjustment line 0 reuses captured VMA'", kAdjustmentMa);
    test.run_characters(kLineCharacters);

    test.expect_adj(true, "t02l adjustment line 1 remains active");
    test.expect_line("t02l adjustment line 1 holds C4", 1);
    test.expect_row("t02l adjustment line 1 has C9 index 1", 1);
    test.expect_ma("t02l adjustment line 1 reuses captured VMA'", kAdjustmentMa);
    test.run_characters(kLineCharacters);

    test.expect_adj(false, "t02l R5 ends adjustment after two lines");
    test.expect_line("t02l next frame restart resets C4", 0);
    test.expect_row("t02l next frame aligns C9 with even ParityFrame", 0);
    test.expect_ma("t02l next frame reloads R12/R13", kBase);
}

// Program a stable interlace frame whose first active field is even.  R0=63
// makes the ACCC p.214/p.218 half-line position C0=31 directly observable.
// The initial R0=0 reset rollover makes ParityFrame odd;
// run_to_frame_start() consumes the programmed setup frame and therefore
// enters an even frame.  The mode defaults to IVM (R8=3); the sync-only
// vector below passes R8=1 while sharing the same deterministic setup.
void program_ivm_sync_frame(TestBench& test, unsigned r7,
                            unsigned vsync_lines = 1,
                            unsigned interlace_mode = 3) {
    test.write_register(0, 63);
    test.write_register(1, 2);
    test.write_register(3, ((vsync_lines & 15U) << 4) | 1U);
    test.write_register(4, 3);
    test.write_register(5, 0);
    test.write_register(6, 100);
    test.write_register(7, r7);
    test.write_register(9, 7);
    test.run_to_frame_start();
    test.write_register(8, interlace_mode);
}

// ACCC v1.11 §19.5.5 pp.213-215 and §19.7.1/19.7.3 p.218.  With odd R9
// and R7=1, the even frame starts VSYNC in the middle of the first line of
// C4=1 (C9=1, C0=R0/2).  On the following odd frame C4=1 begins at C9=0,
// but the odd-C4 balancing rule delays VSYNC by one whole line to C9=2.
// These expectations are the highlighted R7=1 rows in the p.214 diagram.
void t04j_ivm_mid_vsync_and_odd_frame_delay(TestBench& test) {
    constexpr unsigned kLineCharacters = 64;
    program_ivm_sync_frame(test, 1);

    test.run_to_state(1, 1, 0, "t04j even frame C4=R7 first line");
    test.expect_vsync("t04j even frame is low at first-line seam", false);
    test.run_characters(30);
    test.expect_hcc("t04j before MID-VSYNC", 30);
    test.expect_vsync("t04j low before C0=R0/2", false);
    test.run_characters(1);
    test.expect_hcc("t04j MID-VSYNC horizontal phase", 31);
    test.expect_vsync("t04j even frame fires at C0=R0/2", true);
    // ACCC v1.11 §16.1 p.159 counts R3h lines at C0=0 even when the
    // interlace rule moved only the start to mid-line.  R3h=1 therefore
    // ends at the immediately following seam, not at the next midpoint.
    test.run_characters(32);
    test.expect_hcc("t04j before C0=0 width count", 63);
    test.expect_vsync("t04j one-line pulse remains high through C0=R0", true);
    test.run_characters(1);
    test.expect_hcc("t04j C0=0 width count", 0);
    test.expect_vsync("t04j R3h=1 MID-VSYNC ends at the next seam", false);

    // The even frame has one §19.6.4 line; the next frame starts on odd C9.
    test.run_to_state(0, 1, 0, "t04j following odd frame origin");
    test.run_to_state(1, 0, 0, "t04j odd frame C4=R7 first line");
    test.expect_vsync("t04j odd frame suppresses first-line VSYNC", false);
    test.run_characters(kLineCharacters);
    test.expect_line("t04j delayed VSYNC remains on C4=R7", 1);
    test.expect_row("t04j delayed VSYNC starts on second line C9=2", 2);
    test.expect_hcc("t04j delayed VSYNC is line-aligned", 0);
    test.expect_vsync("t04j odd frame fires one line late", true);
}

// ACCC v1.11 §19.7.3 p.218 gives CRTC3/4 a special R7=0 priority rule:
// the outgoing ParityFrame is sampled before it toggles.  Therefore an even
// outgoing frame schedules MID-VSYNC in the new odd frame, while an odd
// outgoing frame starts an ordinary seam VSYNC in the new even frame.
void t04k_ivm_r7_zero_uses_outgoing_parity(TestBench& test) {
    program_ivm_sync_frame(test, 100);
    test.write_register(7, 0);

    // End the active even frame.  Its additional line precedes the new odd
    // origin; the outgoing-even decision schedules a mid-line start there.
    test.run_to_state(0, 1, 0, "t04k odd frame origin");
    test.expect_vsync("t04k outgoing-even frame suppresses seam start", false);
    test.run_characters(30);
    test.expect_vsync("t04k R7=0 remains low before half line", false);
    test.run_characters(1);
    test.expect_hcc("t04k R7=0 MID-VSYNC phase", 31);
    test.expect_vsync("t04k outgoing-even frame schedules MID-VSYNC", true);

    // The odd frame has no additional line.  At its end, the outgoing-odd
    // comparison fires before ParityFrame becomes even, so VSYNC is already
    // high at C4=C9=C0=0.
    test.run_to_state(0, 0, 0, "t04k even frame origin");
    test.expect_vsync("t04k outgoing-odd frame starts seam VSYNC", true);
}

// ACCC v1.11 §16.4.4 p.170 evaluates the type-3 VSYNC condition whenever
// the new line exposes C4=R7,C9=C0=0.  The pre-interlace implementation's
// charline_n/raster_n condition therefore also applies when C4 remains R4
// while R5 management starts.  §19.7 moves an even-frame IVM occurrence to
// mid-line but does not remove the condition on the added §19.6.4 line.
void t04l_r7_r4_adjustment_and_interlace_line_seams(TestBench& test) {
    // Legacy/non-IVM R5 entry: the earlier C4=R7 pulse has ended by C9=1;
    // entering adjustment presents C4=R7=R4,C9=C0=0 and must refire.
    test.write_register(0, 7);
    test.write_register(3, 0x11);
    test.write_register(4, 1);
    test.write_register(5, 2);
    test.write_register(7, 1);
    test.write_register(9, 1);
    test.run_until_vsync_idle();
    test.run_to_frame_start();
    test.run_to_state(1, 1, 0, "t04l R5 precondition after first pulse");
    test.expect_vsync("t04l first C4=R7 pulse has ended", false);
    test.run_characters(8);
    test.expect_adj(true, "t04l R5 adjustment has started");
    test.expect_line("t04l R5 entry keeps C4=R7=R4", 1);
    test.expect_row("t04l R5 entry exposes C9=0", 0);
    test.expect_vsync("t04l R5 entry refires legacy seam VSYNC", true);

    // Even-frame IVM, R7=R4=3: C4=3 has only odd body C9 values, so the
    // next C9=0 occurrence is unambiguously the added interlace line.  That
    // occurrence schedules a fresh MID-VSYNC at C0=31.
    TestBench interlace;
    program_ivm_sync_frame(interlace, 3);
    interlace.run_to_state(3, 0, 0, "t04l even-frame interlace line seam");
    interlace.expect_adj(false, "t04l added interlace line is not R5 adjustment");
    interlace.expect_line("t04l added line carries C4=R4", 3);
    interlace.expect_row("t04l added line forces C9=0", 0);
    interlace.expect_de("t04l carried C4 remains inside the R6 display window", true);
    interlace.expect_ma("t04l added line starts from terminal captured VMA'", 8);
    interlace.expect_vsync("t04l interlace line is low at its seam", false);
    interlace.run_characters(30);
    interlace.expect_adj(false, "t04l mid-line remains outside R5 adjustment");
    interlace.expect_ma("t04l pointer progresses before the true origin reload", 38);
    interlace.expect_vsync("t04l interlace line stays low before midpoint", false);
    interlace.run_characters(1);
    interlace.expect_hcc("t04l interlace-line MID-VSYNC phase", 31);
    interlace.expect_vsync("t04l added line preserves C4/R7 VSYNC condition", true);
    interlace.run_characters(33);
    interlace.expect_line("t04l true origin reloads C4 after the added line", 0);
    interlace.expect_row("t04l true origin exposes odd-frame C9", 1);
    interlace.expect_adj(false, "t04l true origin remains outside adjustment");
    interlace.expect_ma("t04l true origin reloads R12/R13", 0);
    interlace.expect_vsync("t04l R3h=1 ends at the true-origin seam", false);

    // Odd-frame R5 recurrence is intentionally left as a residual: this
    // vector pins the source-backed even-frame added-line seam only.
}

// ACCC v1.11 §16.1 p.159: C3h advances at C0=0.  With R3h=2, a pulse
// starting at C0=R0/2 remains high across the first seam and ends at the
// second seam.  §19.3.4 pp.201-202 makes interlace management live, so an
// R8 exit before the pending midpoint must cancel that scheduled start.
void t04m_ivm_mid_vsync_width_and_exit(TestBench& test) {
    program_ivm_sync_frame(test, 1, 2);
    test.run_to_state(1, 1, 0, "t04m R3h=2 target line");
    test.run_characters(31);
    test.expect_vsync("t04m R3h=2 starts at midpoint", true);
    test.run_characters(33);
    test.expect_hcc("t04m first C0=0 width count", 0);
    test.expect_vsync("t04m R3h=2 remains high after first seam", true);
    test.run_characters(64);
    test.expect_hcc("t04m second C0=0 width count", 0);
    test.expect_vsync("t04m R3h=2 ends at second seam", false);

    TestBench exit;
    program_ivm_sync_frame(exit, 1);
    exit.run_to_state(1, 1, 0, "t04m pending MID-VSYNC before R8 exit");
    exit.write_register(8, 0);
    exit.run_to_state(1, 1, 31, "t04m former midpoint after R8 exit");
    exit.expect_vsync("t04m R8 exit cancels pending MID-VSYNC", false);

    // §19.3.4 says interlace management is real-time and §19.7.1 defines
    // the trigger from the live R0/2 position.  A pre-midpoint R0 rewrite
    // therefore moves the already scheduled fire to the new half position.
    TestBench live_r0;
    program_ivm_sync_frame(live_r0, 1);
    live_r0.run_to_state(1, 1, 0, "t04m pending MID-VSYNC before R0 rewrite");
    live_r0.write_register(0, 31);
    live_r0.run_to_state(1, 1, 14, "t04m before rewritten R0 midpoint");
    live_r0.expect_vsync("t04m low before live R0/2", false);
    live_r0.run_characters(1);
    live_r0.expect_hcc("t04m rewritten live R0/2 position", 15);
    live_r0.expect_vsync("t04m pending MID-VSYNC follows live R0", true);

    // §14.2 p.131 permits live R3h changes during VSYNC.  After one C0=0
    // count of an R3h=3 MID pulse, lowering R3h to 2 makes the second seam
    // the live equality and ends the pulse there.
    TestBench live_r3;
    program_ivm_sync_frame(live_r3, 1, 3);
    live_r3.run_to_state(1, 1, 0, "t04m live-R3 target line");
    live_r3.run_characters(31);
    live_r3.expect_vsync("t04m live-R3 pulse starts at midpoint", true);
    live_r3.run_characters(33);
    live_r3.expect_vsync("t04m live-R3 pulse survives first seam", true);
    live_r3.write_register(3, 0x21);
    live_r3.run_to_state(1, 5, 0, "t04m live-R3 second seam");
    live_r3.expect_vsync("t04m live R3h=2 ends on second seam", false);

    // Reset must cancel latent and visible sync state, rather than merely
    // being sampled after a pulse has already ended.
    TestBench pending_reset;
    program_ivm_sync_frame(pending_reset, 1);
    pending_reset.run_to_state(1, 1, 0, "t04m pending MID-VSYNC reset seam");
    pending_reset.expect_vsync("t04m midpoint start is pending at the seam", false);
    pending_reset.reset();
    pending_reset.expect_vsync("t04m reset clears pending MID-VSYNC", false);

    TestBench active_reset;
    program_ivm_sync_frame(active_reset, 1);
    active_reset.run_to_state(1, 1, 0, "t04m active VSYNC reset target");
    active_reset.run_characters(31);
    active_reset.expect_vsync("t04m active-reset precondition", true);
    active_reset.reset();
    active_reset.expect_vsync("t04m reset clears active VSYNC", false);

    // The dedicated t04n vector below covers R8=1 sync-only interlace;
    // these source-backed phase and reset vectors exercise IVM (R8=3).
}

// ACCC v1.11 §19.6.4 p.217 and §19.7.3 p.218: R8=1 adds one C9=0 line
// after the even frame without incrementing C4, and moves that frame's VSYNC
// to C0=R0/2. The added line starts from the VMA captured at C4=R4,C9=R9;
// ordinary R8=1 body lines retain their non-IVM C9/RA/address cadence. Use an
// even R7 so the separate odd-C4 IVM correction cannot obscure the required
// one seam start and one midpoint start over the two fields.
void t04n_sync_interlace_half_line_vsync(TestBench& test) {
    constexpr unsigned kLineCharacters = 64;
    constexpr unsigned kFieldLines = 4 * 8;
    program_ivm_sync_frame(test, 2, 1, 1);
    test.run_to_state(2, 0, 0, "t04n first field C4=R7 first line");

    unsigned seam_starts = 0;
    unsigned midpoint_starts = 0;
    for (unsigned field = 0; field < 2; ++field) {
        // R8=1 must leave the ordinary C4/C9, VMA, RA, and DE state intact:
        // row 2 starts with C9=0, VMA base+2*R1=4, RA=0, and the first two
        // character cells are in the display window (ACCC §17.1 p.175).
        test.expect_line("t04n C4 at VSYNC target seam", 2);
        test.expect_row("t04n C9 at VSYNC target seam", 0);
        test.expect_hcc("t04n seam horizontal phase", 0);
        test.expect_ma("t04n ordinary row-start VMA", 4);
        test.expect_ra("t04n ordinary row-start RA", 0);
        test.expect_de("t04n ordinary row-start DE", true);
        const bool seam_high = test.vsync() != 0;
        if (seam_high)
            ++seam_starts;

        test.run_characters(30);
        test.expect_line("t04n C4 stays ordinary before midpoint", 2);
        test.expect_row("t04n C9 stays ordinary before midpoint", 0);
        test.expect_hcc("t04n before midpoint", 30);
        test.expect_ma("t04n VMA advances ordinarily before midpoint", 34);
        test.expect_ra("t04n RA stays ordinary before midpoint", 0);
        test.expect_de("t04n DE leaves its ordinary two-character window", false);
        test.expect_vsync("t04n seam/midpoint polarity before midpoint",
                          seam_high);

        test.run_characters(1);
        test.expect_line("t04n C4 stays ordinary at midpoint", 2);
        test.expect_row("t04n C9 stays ordinary at midpoint", 0);
        test.expect_hcc("t04n half-line VSYNC phase", 31);
        test.expect_ma("t04n VMA advances ordinarily at midpoint", 35);
        test.expect_ra("t04n RA stays ordinary at midpoint", 0);
        test.expect_de("t04n DE stays ordinary at midpoint", false);
        test.expect_vsync("t04n VSYNC starts by midpoint", true);
        if (!seam_high)
            ++midpoint_starts;

        // R3h=1 counts at the following C0=0 boundary.  Pin the falling
        // edge explicitly so a stuck-high pulse cannot masquerade as the
        // expected seam/midpoint alternation in the level counts below.
        test.run_characters(33);
        test.expect_hcc("t04n R3h=1 falling-edge seam", 0);
        test.expect_vsync("t04n R3h=1 pulse ends at next seam", false);

        // The even field must gain exactly the §19.6.4 additional line. From
        // C4=2,C9=1 after the falling seam, 15 ordinary lines reach the added
        // line; it carries C4=R4=3, forces C9=RA=0, and starts at terminal
        // VMA'. One added line plus 16 body lines then reach C4=R7 again.
        if (field == 0) {
            test.run_characters(kLineCharacters * 15);
            test.expect_line("t04n added line keeps C4=R4", 3);
            test.expect_row("t04n added line forces C9=0", 0);
            test.expect_hcc("t04n added line seam", 0);
            test.expect_ma("t04n added line uses terminal captured VMA", 8);
            test.expect_ra("t04n added line forces RA=0", 0);
            test.expect_de("t04n added line retains ordinary C4 display state", true);

            test.run_characters(kLineCharacters * 17);
            test.expect_line("t04n next field C4", 2);
            test.expect_row("t04n next field C9", 0);
            test.expect_hcc("t04n next field seam", 0);
            test.expect_ma("t04n next field ordinary VMA", 4);
            test.expect_ra("t04n next field ordinary RA", 0);
            test.expect_de("t04n next field ordinary DE", true);
        }
        else {
            // The odd field has no additional line. From C4=2,C9=1, the
            // remaining 15 ordinary lines must reach the real origin
            // directly; an incorrectly parity-blind extra line would still
            // expose carried C4=R4,C9=0 here.
            test.run_characters(kLineCharacters * 15);
            test.expect_line("t04n odd field restarts without added line", 0);
            test.expect_row("t04n odd field origin C9", 0);
            test.expect_hcc("t04n odd field origin seam", 0);
            test.expect_ma("t04n odd field origin reloads VMA", 0);
            test.expect_ra("t04n odd field origin RA", 0);
        }
    }

    if (seam_starts != 1 || midpoint_starts != 1) {
        throw TestFailure(
            "t04n expected one seam-start and one midpoint-start VSYNC over "
            "two fields: seam=" + std::to_string(seam_starts) +
            ", midpoint=" + std::to_string(midpoint_starts));
    }

    // Live R8 exit must cancel a midpoint already scheduled at the target
    // seam.  Locate the midpoint field without assuming its polarity, then
    // leave sync-only interlace before R0/2 and prove no stale pulse fires.
    TestBench exit;
    program_ivm_sync_frame(exit, 2, 1, 1);
    exit.run_to_state(2, 0, 0, "t04n mode-1 pending midpoint exit");
    if (exit.vsync()) {
        exit.run_characters(kLineCharacters * (kFieldLines + 1));
        exit.expect_vsync("t04n alternate field schedules midpoint", false);
    }
    exit.write_register(8, 0);
    exit.run_to_state(2, 0, 31, "t04n former midpoint after mode-1 exit");
    exit.expect_vsync("t04n R8=1 exit cancels pending midpoint", false);

    // §19.6.4 p.217 says the added line starts from the VMA' captured at
    // C9=R9,C0=R1.  R4=0 is the aliasing discriminator: the added line has
    // C4=C9=0, but it is not a real frame origin and must not reload R12/R13.
    TestBench r4_zero;
    r4_zero.write_register(0, 7);
    r4_zero.write_register(1, 2);
    r4_zero.write_register(3, 0x11);
    r4_zero.write_register(4, 0);
    r4_zero.write_register(5, 0);
    r4_zero.write_register(6, 1);
    r4_zero.write_register(7, 1);
    r4_zero.write_register(9, 1);
    r4_zero.write_register(12, 1);
    r4_zero.write_register(13, 0);
    r4_zero.run_to_frame_start();
    r4_zero.write_register(8, 1);
    r4_zero.run_characters(16);
    r4_zero.expect_line("t04n R4=0 added line keeps C4", 0);
    r4_zero.expect_row("t04n R4=0 added line forces C9", 0);
    r4_zero.expect_hcc("t04n R4=0 added line sampled phase", 1);
    r4_zero.expect_ma("t04n R4=0 added line uses captured VMA'", 0x103);
}

// ACCC v1.11 section 19.8.4 pp.235-238: entering IVM (R8=3)
// immediately seeds ParityC9 from the current C9 parity.  Thereafter C9
// advances by two and ORs that parity.  With even R9 the parity is retained
// across C4 changes, so entry on an even/odd C9 produces an even/odd raster
// sequence respectively.
void t02h_ivm_even_r9_preserves_entry_parity(TestBench& test) {
    test.write_register(0x09, 6);
    test.write_register(0x04, 3);
    test.write_register(0x05, 0);
    test.write_register(0x00, 7);
    test.run_to_frame_start();

    test.run_characters(16);  // C9=2, even entry
    test.expect_row("t02h even-entry precondition", 2);
    test.write_register(0x08, 3);
    test.run_characters(8);
    test.expect_row("t02h even-entry IVM step 2->4", 4);
    test.run_characters(8);
    test.expect_row("t02h even-entry IVM step 4->6", 6);
    test.run_characters(8);
    test.expect_line("t02h even R9 advances C4", 1);
    test.expect_row("t02h even parity retained on next C4", 0);

    TestBench odd_entry;
    odd_entry.write_register(0x09, 6);
    odd_entry.write_register(0x04, 3);
    odd_entry.write_register(0x05, 0);
    odd_entry.write_register(0x00, 7);
    odd_entry.run_to_frame_start();
    odd_entry.run_characters(8);  // C9=1, odd entry
    odd_entry.expect_row("t02h odd-entry precondition", 1);
    odd_entry.write_register(0x08, 3);
    odd_entry.run_characters(8);
    odd_entry.expect_row("t02h odd-entry IVM step 1->3", 3);
    odd_entry.run_characters(8);
    odd_entry.expect_row("t02h odd-entry IVM step 3->5", 5);
    odd_entry.run_characters(8);
    odd_entry.expect_row("t02h odd-entry IVM step 5->7", 7);
    odd_entry.run_characters(8);
    odd_entry.expect_line("t02h odd sequence advances C4", 1);
    odd_entry.expect_row("t02h odd parity retained on next C4", 1);
}

// ACCC v1.11 section 19.8.4 pp.235-236: odd R9 toggles ParityC9
// whenever C4 increments.  At the frame boundary ParityFrame toggles and
// ParityC9 is assigned the new frame parity, taking priority over the
// ordinary per-C4 toggle.
void t02i_ivm_odd_r9_balances_rows_and_frames(TestBench& test) {
    test.write_register(0x09, 7);
    test.write_register(0x04, 1);
    test.write_register(0x05, 0);
    test.write_register(0x01, 4);
    test.write_register(0x0c, 0x12);
    test.write_register(0x0d, 0x34);
    test.write_register(0x00, 7);
    test.run_to_frame_start();
    test.expect_ma("t02i initial frame reload", 0x1234);
    test.write_register(0x08, 3);  // enter on even C9=0

    for (unsigned expected : {2U, 4U, 6U, 8U}) {
        test.run_characters(8);
        test.expect_row("t02i even C4 uses even C9", expected);
    }
    test.run_characters(8);
    test.expect_line("t02i odd R9 advances to C4=1", 1);
    test.expect_row("t02i C4 increment toggles ParityC9", 1);
    // The mid-line R8 write advanced HCC once, so this samples character 1
    // of the new row: row base 0x1238 plus one character.
    test.expect_ma("t02i IVM terminal C9 advances the row base", 0x1239);
    for (unsigned expected : {3U, 5U, 7U}) {
        test.run_characters(8);
        test.expect_row("t02i odd C4 uses odd C9", expected);
    }
    test.run_characters(8);
    test.expect_line("t02i even-frame interlace line keeps C4 at R4", 1);
    test.expect_row("t02i even-frame interlace line forces C9=0", 0);
    test.run_characters(8);
    test.expect_line("t02i frame boundary resets C4 after interlace line", 0);
    test.expect_row("t02i new frame starts on toggled frame parity", 1);
    test.expect_ma("t02i odd-parity frame still reloads R12/R13", 0x1235);
}

// ACCC v1.11 section 19.8.4 exit tables pp.239-240: disabling IVM
// mid-row immediately returns to +1 C9 progression.  The next ordinary
// C9>=R9 decision then restores the non-IVM C9=0 row origin; no hidden
// post-IVM parity state may leak into subsequent R8=0 rows.
void t02j_ivm_exit_returns_to_plain_counting(TestBench& test) {
    test.write_register(0x09, 6);
    test.write_register(0x04, 3);
    test.write_register(0x05, 0);
    test.write_register(0x00, 7);
    test.run_to_frame_start();
    test.run_characters(8);  // C9=1
    test.write_register(0x08, 3);
    test.run_characters(8);
    test.expect_row("t02j IVM odd step 1->3", 3);
    test.run_characters(8);
    test.expect_row("t02j IVM odd step 3->5", 5);
    test.write_register(0x08, 0);
    test.run_characters(8);
    test.expect_row("t02j exit mid-row resumes +1 progression", 6);
    test.run_characters(8);
    test.expect_line("t02j ordinary row end advances C4", 1);
    test.expect_row("t02j ordinary row end restores C9=0", 0);
    test.run_characters(8);
    test.expect_row("t02j following R8=0 row remains ordinary", 1);
}

// ------------------------------------------------------------------
// P1 remainder: locked-ASIC pixel path vectors (t05x).
//
// Every expectation below is derived on paper from the cited source:
//  - [KT] legacy-colour table (web.archive.org capture 20230923001014,
//    Palette section), independently cross-checked entry by entry against
//    the checked-in ga40010 netlist DAC equations during extraction.
//  - Grimware Gate Array page, §RMR "Byte/Pixel structure" table for the
//    per-mode bit layouts; corroborated against the netlist cidx taps.
// The bench drives PIXEN on every tick, so dot d of a character is tick d
// after align_to_character_start(); RGB is registered once per dot, so the
// value sampled during dot d reflects the pixel decoded at dot d-1. That
// one-dot presentation latency is part of the documented unverified
// phase-alignment assumption in asic_video.v (t04i discipline).
// ------------------------------------------------------------------

// Frame used by the pixel vectors: 64-character lines with display on
// characters 0-3 and HSYNC at characters 40-41, so a full character walk
// from the frame start never meets sync.
void program_pixel_frame(TestBench& test) {
    test.write_register(9, 3);
    test.write_register(4, 2);
    test.write_register(5, 0);
    test.write_register(1, 4);
    test.write_register(6, 100);
    test.write_register(2, 40);
    test.write_register(3, 0x11);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.write_register(0, 63);  // last: starts the 64-char line cadence
}

// Palette shared by the layout vectors; entries chosen so every expected
// colour is unique and traceable back to its [KT] row.
struct TestPalette {
    unsigned ink0 = 20;  // black          -> 0,0,0
    unsigned ink1 = 11;  // bright white   -> 15,15,15
    unsigned ink2 = 18;  // bright green   -> 0,15,0
    unsigned ink3 = 10;  // bright yellow  -> 15,15,0
    unsigned ink4 = 21;  // bright blue    -> 0,0,15
    unsigned ink6 = 26;  // lime           -> 6,15,0
    unsigned ink9 = 25;  // pastel green   -> 6,15,6
    unsigned ink10 = 23; // sky blue       -> 0,6,15
    unsigned ink13 = 12; // bright red     -> 15,0,0
};

void apply_palette(TestBench& test, const TestPalette& pal) {
    test.set_inkr(0, pal.ink0);
    test.set_inkr(1, pal.ink1);
    test.set_inkr(2, pal.ink2);
    test.set_inkr(3, pal.ink3);
    test.set_inkr(4, pal.ink4);
    test.set_inkr(6, pal.ink6);
    test.set_inkr(9, pal.ink9);
    test.set_inkr(10, pal.ink10);
    test.set_inkr(13, pal.ink13);
}

constexpr unsigned kBlackR = 0, kBlackG = 0, kBlackB = 0;
constexpr unsigned kWhiteR = 15, kWhiteG = 15, kWhiteB = 15;
constexpr unsigned kYellowR = 15, kYellowG = 15, kYellowB = 0;
constexpr unsigned kBBlueR = 0, kBBlueG = 0, kBBlueB = 15;
constexpr unsigned kLimeR = 6, kLimeG = 15, kLimeB = 0;
constexpr unsigned kPGreenR = 6, kPGreenG = 15, kPGreenB = 6;
constexpr unsigned kSkyR = 0, kSkyG = 6, kSkyB = 15;
constexpr unsigned kGreenR = 0, kGreenG = 15, kGreenB = 0;
constexpr unsigned kRedR = 15, kRedG = 0, kRedB = 0;

// [KT] table sweep via the border path (DE low everywhere: R1=0 keeps hde
// cleared at every line start, R6=0 keeps vde cleared too). Sampling sits at
// character ~3 of each 64-char line, far from HSYNC at 40-41, and the border
// write is given a full character to propagate through the registered RGB.
void t05a_legacy_colour_rom_sweep(TestBench& test) {
    test.write_register(9, 2);
    test.write_register(4, 2);   // short frame: fits the convergence guard
    test.write_register(5, 0);
    test.write_register(1, 0);   // no displayed characters: border always
    test.write_register(6, 0);   // vertical border as well
    test.write_register(2, 60);  // sync late: the 32-entry sweep stays clear
    test.write_register(3, 0x11);
    test.write_register(0, 63);
    test.run_to_frame_start();
    // [KT] hardware colour index -> R,G,B (mid level = 6).
    static const unsigned kKt[32][3] = {
        {6, 6, 6},    {6, 6, 6},    {0, 15, 6},   {15, 15, 6},
        {0, 0, 6},    {15, 0, 6},   {0, 6, 6},    {15, 6, 6},
        {15, 0, 6},   {15, 15, 6},  {15, 15, 0},  {15, 15, 15},
        {15, 0, 0},   {15, 0, 15},  {15, 6, 0},   {15, 6, 15},
        {0, 0, 6},    {0, 15, 6},   {0, 15, 0},   {0, 15, 15},
        {0, 0, 0},    {0, 0, 15},   {0, 6, 0},    {0, 6, 15},
        {6, 0, 6},    {6, 15, 6},   {6, 15, 0},   {6, 15, 15},
        {6, 0, 0},    {6, 0, 15},   {6, 6, 0},    {6, 6, 15},
    };
    for (unsigned hw = 0; hw < 32; ++hw) {
        test.set_border(hw);
        test.run_characters(1);  // crosses a CLKEN edge: RGB re-registers
        test.run_dots(4);        // settle mid-character
        test.expect_rgb("legacy ROM entry [KT] hw=" + std::to_string(hw),
                        kKt[hw][0], kKt[hw][1], kKt[hw][2]);
        test.expect_pen("border flag for hw=" + std::to_string(hw), true, 0);
    }
}

// Walks dots 0..15 of the first displayed character and asserts RGB per dot,
// accounting for the one-dot registered presentation latency.
void walk_char_expect(TestBench& test, const std::string& tag,
                     const unsigned (&exp)[16][3]) {
    test.run_to_frame_start();
    test.align_to_character_start();
    // d == 0 is the CLKEN edge itself; d == 16 is the next character's CLKEN
    // edge, whose registered output still carries this character's dot 15.
    for (unsigned d = 0; d < 17; ++d) {
        test.run_dots(1);
        if (d == 0) {
            continue;  // dot 0 still carries the reset/previous value
        }
        test.expect_rgb(tag + " dot " + std::to_string(d), exp[d - 1][0],
                        exp[d - 1][1], exp[d - 1][2]);
    }
}

// Mode 2 (%10): eight sequential bits MSB-first per byte half, one dot per
// pen (Grimware mode-2 row A..H; netlist tap r7). Even byte F0 = %11110000:
// four white then four black pens; odd byte 0F mirrors it.
void t05b_mode2_sequential_pixels(TestBench& test) {
    program_pixel_frame(test);
    TestPalette pal;
    apply_palette(test, pal);
    test.set_ga(2, 24 /* magenta border */, (15U << 8) | 0xF0U);
    walk_char_expect(test, "mode 2", {
        {kWhiteR, kWhiteG, kWhiteB}, {kWhiteR, kWhiteG, kWhiteB},
        {kWhiteR, kWhiteG, kWhiteB}, {kWhiteR, kWhiteG, kWhiteB},
        {kBlackR, kBlackG, kBlackB}, {kBlackR, kBlackG, kBlackB},
        {kBlackR, kBlackG, kBlackB}, {kBlackR, kBlackG, kBlackB},
        {kBlackR, kBlackG, kBlackB}, {kBlackR, kBlackG, kBlackB},
        {kBlackR, kBlackG, kBlackB}, {kBlackR, kBlackG, kBlackB},
        {kWhiteR, kWhiteG, kWhiteB}, {kWhiteR, kWhiteG, kWhiteB},
        {kWhiteR, kWhiteG, kWhiteB}, {kWhiteR, kWhiteG, kWhiteB},
    });
    test.expect_pen("mode 2 pen nibble tracks the bit stream", false, 1);
}

// Mode 1 (%01): four two-bit pens per byte half, two dots each
// (Grimware row A0 B0 C0 D0 A1 B1 C1 D1; netlist taps {r3,r7}; ink bit 1
// comes from the high-position bit, ink bit 0 from the low-position bit).
// Even byte 9C=%10011100: A={A1,A0}={b3,b7}={1,1}=3, B={b2,b6}={1,0}=2,
// C={b1,b5}={0,0}=0, D={D1,D0}={b0,b4}={0,1}=1. Odd byte 63=%01100011:
// A={b3,b7}={0,0}=0, B={b2,b6}={0,1}=1, C={b1,b5}={1,1}=3,
// D={b0,b4}={1,0}=2.
void t05c_mode1_pair_pixels(TestBench& test) {
    program_pixel_frame(test);
    TestPalette pal;
    apply_palette(test, pal);
    test.set_ga(1, 24, (0x63U << 8) | 0x9CU);
    walk_char_expect(test, "mode 1", {
        {kYellowR, kYellowG, kYellowB}, {kYellowR, kYellowG, kYellowB},
        {kGreenR, kGreenG, kGreenB}, {kGreenR, kGreenG, kGreenB},
        {kBlackR, kBlackG, kBlackB}, {kBlackR, kBlackG, kBlackB},
        {kWhiteR, kWhiteG, kWhiteB}, {kWhiteR, kWhiteG, kWhiteB},
        {kBlackR, kBlackG, kBlackB}, {kBlackR, kBlackG, kBlackB},
        {kWhiteR, kWhiteG, kWhiteB}, {kWhiteR, kWhiteG, kWhiteB},
        {kYellowR, kYellowG, kYellowB}, {kYellowR, kYellowG, kYellowB},
        {kGreenR, kGreenG, kGreenB}, {kGreenR, kGreenG, kGreenB},
    });
}

// Mode 0 (%00): two four-bit pens per byte half, four dots each
// (Grimware row A0 B0 A2 B2 A1 B1 A3 B3; netlist cidx taps {r1,r5,r3,r7};
// nibble = {A3,A2,A1,A0} = {b1,b5,b3,b7}).
// Even byte B2=%10110010: A={b1,b5,b3,b7}={1,1,0,1}=13,
// B={b0,b4,b2,b6}={0,1,0,0}=4. Odd byte 4B=%01001011: A={1,0,1,0}=10,
// B={1,0,0,1}=9.
void t05d_mode0_nibble_pixels(TestBench& test) {
    program_pixel_frame(test);
    TestPalette pal;
    apply_palette(test, pal);
    test.set_ga(0, 24, (0x4BU << 8) | 0xB2U);
    walk_char_expect(test, "mode 0", {
        {kRedR, kRedG, kRedB}, {kRedR, kRedG, kRedB},
        {kRedR, kRedG, kRedB}, {kRedR, kRedG, kRedB},
        {kBBlueR, kBBlueG, kBBlueB}, {kBBlueR, kBBlueG, kBBlueB},
        {kBBlueR, kBBlueG, kBBlueB}, {kBBlueR, kBBlueG, kBBlueB},
        {kSkyR, kSkyG, kSkyB}, {kSkyR, kSkyG, kSkyB},
        {kSkyR, kSkyG, kSkyB}, {kSkyR, kSkyG, kSkyB},
        {kPGreenR, kPGreenG, kPGreenB}, {kPGreenR, kPGreenG, kPGreenB},
        {kPGreenR, kPGreenG, kPGreenB}, {kPGreenR, kPGreenG, kPGreenB},
    });
}

// Outside DE the border colour is substituted; while HSYNC is active the
// RGB pins are forced to black (netlist FORCE_BLANK analogue). Border here
// is hw 24 = magenta (6,0,6) per [KT].
void t05e_border_substitution_and_sync_blank(TestBench& test) {
    program_pixel_frame(test);
    TestPalette pal;
    apply_palette(test, pal);
    test.set_ga(2, 24, (15U << 8) | 0xF0U);
    test.run_to_frame_start();
    test.run_characters(6);  // characters 4..5: past R1=4, still clear of sync
    test.run_characters(1);
    test.expect_rgb("border substitution outside DE", 6, 0, 6);
    test.expect_pen("border flag asserted outside DE", true, 0);
    do {
        test.run_characters(1);
    } while (!test.hsync());
    // Still inside the pulse (R3h=1): give the registered output two dots
    // to pick up the forced-blank value.
    test.run_dots(3);
    test.expect_rgb("HSYNC forces RGB to black", 0, 0, 0);
    test.run_until_hsync_idle();
    test.expect_rgb("border returns after HSYNC", 6, 0, 6);
}

// Grimware §RMR: the video mode takes effect only after the next HSYNC.
// Switching GAMODE mid-line leaves mode-2 decoding in charge until the
// line boundary; after crossing HSYNC the same VIDEOD word decodes with
// mode-1 cadence.
void t05f_mode_change_latches_at_hsync(TestBench& test) {
    program_pixel_frame(test);
    TestPalette pal;
    apply_palette(test, pal);
    test.set_ga(2, 24, (0x63U << 8) | 0x9CU);
    test.run_to_frame_start();
    test.align_to_character_start();
    // Walk character 0 under mode 2, then flip GAMODE mid-line and confirm
    // the next character still decodes as mode 2 (same word => same pattern).
    // Mode-2 decode of 9C: bits b7..b0 = 1,0,0,1,1,1,0,0 -> pens 1,0,0,1,1,
    // 1,0,0; of 63: 0,1,1,0,0,0,1,1 -> pens 0,1,1,0,0,0,1,1.
    static const unsigned m2pix[16] = {1, 0, 0, 1, 1, 1, 0, 0,
                                       0, 1, 1, 0, 0, 0, 1, 1};
    auto rgb_of_pen2 = [](unsigned p, unsigned(&out)[3]) {
        if (p == 1) { out[0] = 15; out[1] = 15; out[2] = 15; }
        else { out[0] = 0; out[1] = 0; out[2] = 0; }
    };
    test.run_dots(1);  // execute dot 0
    for (unsigned d = 1; d < 16; ++d) {
        test.run_dots(1);
        unsigned c[3];
        rgb_of_pen2(m2pix[d - 1], c);
        test.expect_rgb("pre-change mode 2 dot " + std::to_string(d),
                        c[0], c[1], c[2]);
    }
    test.set_mode(1);  // lands mid-line: must stay inert
    for (unsigned d = 0; d < 16; ++d) {
        test.run_dots(1);
        if (d == 0) {
            continue;  // boundary sample still carries the previous cell
        }
        unsigned c[3];
        rgb_of_pen2(m2pix[d - 1], c);
        test.expect_rgb("inert mid-line mode change dot " + std::to_string(d),
                        c[0], c[1], c[2]);
    }
    // Cross HSYNC (characters 40-41) and realign; mode 1 now applies to the
    // same word: pairs A={b3,b7}=3, B={b2,b6}=2, C=0, D={b0,b4}=1 for 9C;
    // A=0, B=1, C=3, D=2 for 63.
    test.run_characters(45);
    test.run_to_frame_start();
    test.align_to_character_start();
    static const unsigned m1[16][3] = {
        {kYellowR, kYellowG, kYellowB}, {kYellowR, kYellowG, kYellowB},
        {kGreenR, kGreenG, kGreenB}, {kGreenR, kGreenG, kGreenB},
        {kBlackR, kBlackG, kBlackB}, {kBlackR, kBlackG, kBlackB},
        {kWhiteR, kWhiteG, kWhiteB}, {kWhiteR, kWhiteG, kWhiteB},
        {kBlackR, kBlackG, kBlackB}, {kBlackR, kBlackG, kBlackB},
        {kWhiteR, kWhiteG, kWhiteB}, {kWhiteR, kWhiteG, kWhiteB},
        {kYellowR, kYellowG, kYellowB}, {kYellowR, kYellowG, kYellowB},
        {kGreenR, kGreenG, kGreenB}, {kGreenR, kGreenG, kGreenB},
    };
    for (unsigned d = 0; d < 16; ++d) {
        test.run_dots(1);
        if (d == 0) continue;
        test.expect_rgb("post-HSYNC mode 1 dot " + std::to_string(d),
                        m1[d - 1][0], m1[d - 1][1], m1[d - 1][2]);
    }
}

// Mode 3 (%11): Grimware documents A={A1,A0}={b3,b7} then B={b2,b6} at
// mode-0-like cadence (four dots per pen); pen value = {X1,X0}.
// Even byte B2: A={b3,b7}={0,1}=1, B={b2,b6}={0,0}=0; odd 4B:
// A={b3,b7}={1,0}=2, B={b2,b6}={0,1}=1.
void t05g_mode3_two_bit_pixels(TestBench& test) {
    program_pixel_frame(test);
    TestPalette pal;
    apply_palette(test, pal);
    test.set_ga(3, 24, (0x4BU << 8) | 0xB2U);
    walk_char_expect(test, "mode 3", {
        {kWhiteR, kWhiteG, kWhiteB}, {kWhiteR, kWhiteG, kWhiteB},
        {kWhiteR, kWhiteG, kWhiteB}, {kWhiteR, kWhiteG, kWhiteB},
        {kBlackR, kBlackG, kBlackB}, {kBlackR, kBlackG, kBlackB},
        {kBlackR, kBlackG, kBlackB}, {kBlackR, kBlackG, kBlackB},
        {kGreenR, kGreenG, kGreenB}, {kGreenR, kGreenG, kGreenB},
        {kGreenR, kGreenG, kGreenB}, {kGreenR, kGreenG, kGreenB},
        {kWhiteR, kWhiteG, kWhiteB}, {kWhiteR, kWhiteG, kWhiteB},
        {kWhiteR, kWhiteG, kWhiteB}, {kWhiteR, kWhiteG, kWhiteB},
    });
}

// Regression for the byte-latch phase: each half of a character must
// display THAT character's own byte. Every other t05x vector holds VIDEOD
// constant for the whole walk, so a one-dot-late odd-byte latch is
// invisible to them; this one changes the word exactly at the character
// boundary. Layout source is the same Grimware mode-2 row (MSB first, one
// dot per pen); palette from [KT] (ink0 = hw20 black, ink1 = hw11 white).
//
// What is source-backed and what is not (t04i discipline, and the same
// deferral the module header carries). Sourced: two video bytes per CRTC
// character, even half first, because ga40010 latches VIDEO_BUF twice per
// character; the mode-2 bit order (Grimware); the colours ([KT]). NOT
// sourced: the dot index at which the odd half takes over, i.e. the
// intra-character phase. That is the unverified P1 model assumption
// deferred to motherboard integration (architecture §5 Risk 1), so the
// per-dot boundary asserted below moves with it and is not a hardware rule.
// What survives any cadence is the discontinuity this vector was written
// for: a character leaking the PREVIOUS character's byte for a single dot
// while the dots either side of it are correct. If integration shifts the
// phase, re-derive the boundary here; do not read a failure as proof the
// pipeline regressed until that has been checked.
//
// Tick bookkeeping: after align_to_character_start() the next tick is the
// CLKEN edge that opens character A. Because RGB is registered once per
// dot, dot d of a character is read one tick later, so a character's dot 15
// is read on the very tick that is also the next character's CLKEN edge.
// The next word therefore has to be presented just before that tick.
void t05h_byte_halves_belong_to_their_character(TestBench& test) {
    program_pixel_frame(test);
    TestPalette pal;
    apply_palette(test, pal);
    // A: even 00 -> pen 0 on dots 0-7, odd FF -> pen 1 on dots 8-15.
    // B: even FF -> pen 1 on dots 0-7, odd 00 -> pen 0 on dots 8-15.
    constexpr unsigned kWordA = (0xFFU << 8) | 0x00U;
    constexpr unsigned kWordB = (0x00U << 8) | 0xFFU;
    test.set_ga(2, 24, kWordA);
    test.run_to_frame_start();
    test.align_to_character_start();

    auto walk = [&test](const std::string& tag, bool high_first,
                        unsigned next_word) {
        // Dots 0..14, then present the following word and read dot 15 on
        // the shared boundary tick.
        for (unsigned d = 0; d < 16; ++d) {
            if (d == 15) {
                test.set_videod(next_word);
            }
            test.run_dots(1);
            const bool lit = (d < 8) == high_first;
            const unsigned lvl = lit ? 15U : 0U;
            test.expect_rgb(tag + " dot " + std::to_string(d), lvl, lvl, lvl);
            test.expect_pen(tag + " pen at dot " + std::to_string(d), false,
                            lit ? 1U : 0U);
        }
    };

    test.run_dots(1);  // CLKEN edge opening character A
    walk("char A", false, kWordB);
    // Character B's even byte was latched on the boundary tick above. A
    // one-dot-late odd latch shows character A's FF at B's dot 8 (white
    // where pen 0 black is required).
    walk("char B", true, kWordA);
}

// HF-2: with PAL_EN high the pen and border lookup comes from the ASIC's
// 32-entry 12-bit palette (reference §6) instead of the fixed 27-colour [KT]
// ROM, so colours outside the legacy set reach the pins. The legacy
// INKR_I/BORDER_I inputs are left programmed to values whose ROM colours
// differ from every expectation here, which is what fails if the mux is
// still reading the ROM.
//
// Entries are written in asic_regs' storage order {G,R,B}; the module owes
// the pipeline {R,G,B}, so the channel swap is under test too.
void t05i_asic_palette_drives_rgb(TestBench& test) {
    program_pixel_frame(test);
    TestPalette pal;
    apply_palette(test, pal);
    // Legacy border 24 = magenta (6,0,6); legacy pen 1 = (15,15,15), pen 0 =
    // (0,0,0). None of those match the palette entries below.
    test.set_ga(2, 24, (15U << 8) | 0xF0U);
    test.enable_asic_palette();
    test.set_pal(0, 0x4, 0x5, 0x6);    // pen 0  -> R5 G4 B6
    test.set_pal(1, 0x7, 0x8, 0x9);    // pen 1  -> R8 G7 B9
    test.set_pal(16, 0x1, 0x2, 0x3);   // border -> R2 G1 B3

    // Mode 2, even byte F0 = %11110000 then odd byte 0F: pen 1 four dots,
    // pen 0 eight dots, pen 1 four dots (same stream as t05b).
    const unsigned kP1[3] = {0x8, 0x7, 0x9};
    const unsigned kP0[3] = {0x5, 0x4, 0x6};
    walk_char_expect(test, "ASIC palette mode 2", {
        {kP1[0], kP1[1], kP1[2]}, {kP1[0], kP1[1], kP1[2]},
        {kP1[0], kP1[1], kP1[2]}, {kP1[0], kP1[1], kP1[2]},
        {kP0[0], kP0[1], kP0[2]}, {kP0[0], kP0[1], kP0[2]},
        {kP0[0], kP0[1], kP0[2]}, {kP0[0], kP0[1], kP0[2]},
        {kP0[0], kP0[1], kP0[2]}, {kP0[0], kP0[1], kP0[2]},
        {kP0[0], kP0[1], kP0[2]}, {kP0[0], kP0[1], kP0[2]},
        {kP1[0], kP1[1], kP1[2]}, {kP1[0], kP1[1], kP1[2]},
        {kP1[0], kP1[1], kP1[2]}, {kP1[0], kP1[1], kP1[2]},
    });

    // Outside DE the border comes from entry 16, and HSYNC still forces
    // black ahead of the palette (reference §5 precedence unchanged).
    test.run_to_frame_start();
    test.run_characters(7);  // past R1=4, clear of sync at R2=40
    test.expect_rgb("border from palette entry 16", 0x2, 0x1, 0x3);

    // A sprite pixel still wins over the palette-sourced screen ink.
    test.run_to_frame_start();
    test.align_to_character_start();
    test.run_dots(2);
    test.expect_rgb("screen ink from palette before sprite", kP1[0], kP1[1],
                    kP1[2]);
    test.set_sprite(1, 5, 10, 15);
    test.run_dots(1);
    test.expect_rgb("sprite pixel still overrides the palette", 5, 10, 15);
    test.set_sprite(0, 0, 0, 0);

    // Re-pointing an entry re-colours the pen on the next dot: the mux is a
    // live lookup, not a value latched at DE start.
    test.set_pal(1, 0xF, 0x0, 0xF);
    test.run_dots(2);
    test.expect_rgb("palette rewrite reaches the pins", 0x0, 0xF, 0xF);
}

// HF-2 follow-up: the same palette path under the motherboard's real
// arrangement, where PAL_RGB is asic_regs' REGISTERED read (one CLOCK behind
// PAL_ADDR) and PIXEN is one clock in four. t05i runs a combinational palette
// with PIXEN every clock, so it cannot see a latency or skew error: there the
// lookup and the dot edge are the same clock. Here they are not, and the
// pen changes on every single dot, so any half-dot slip in either direction
// shows up as a swapped colour rather than as a repeated one.
//
// The two pipeline facts being pinned (both read off asic_video.v, not out of
// the simulator):
//   - PAL_ADDR is combinational from pen_delayed, which is registered on
//     PIXEN. So after the PIXEN edge that opens dot d, PAL_ADDR names dot d.
//   - RGB is registered on PIXEN from rgb_mux, which reads PAL_RGB. PAL_RGB
//     after that edge is asic_regs' read of the PREVIOUS PAL_ADDR. So after
//     the edge that opens dot d, RGB carries dot d-1.
// PAL_ADDR therefore leads RGB by exactly one dot, and the assertions below
// check both pins on the same tick so a common-mode shift cannot hide.
//
// Mode 2 with both byte halves = AA gives an unbroken pen 1/pen 0 alternation
// across all 16 dots (Grimware mode-2 row, MSB first, one dot per pen), which
// also carries the alternation across the even/odd byte seam at dot 8.
void t05j_registered_palette_alternating_pens(TestBench& test) {
    program_pixel_frame(test);
    TestPalette pal;
    apply_palette(test, pal);
    // Legacy pen 0/1 and border 24 all differ from the palette entries below,
    // so a fallback to the [KT] ROM fails rather than passing by accident.
    test.set_ga(2, 24, 0xAAAAU);
    test.enable_asic_palette_registered();
    test.set_pixen_divider(4);
    test.set_pal(0, 0x1, 0x2, 0x3);   // pen 0 -> R2 G1 B3
    test.set_pal(1, 0xC, 0xD, 0xE);   // pen 1 -> R13 G12 B14

    const unsigned kP0[3] = {0x2, 0x1, 0x3};
    const unsigned kP1[3] = {0xD, 0xC, 0xE};

    test.run_to_frame_start();
    test.align_to_character_start();
    // d == 0 is the CLKEN edge itself, whose registered output still carries
    // the previous character; dots 0..15 are read on ticks 1..16.
    for (unsigned d = 0; d < 17; ++d) {
        test.run_dots(1);
        const std::string tag = "registered palette dot " + std::to_string(d);
        if (d > 0) {
            // Even dot indices decode pen 1 (AA = %10101010, MSB first).
            const bool pen1 = ((d - 1) % 2) == 0;
            const unsigned* want = pen1 ? kP1 : kP0;
            test.expect_rgb(tag, want[0], want[1], want[2]);
            test.expect_pen(tag + " PEN", false, pen1 ? 1U : 0U);
        }
        if (d < 16) {
            // PAL_ADDR leads RGB by one dot: on this tick it already names
            // the pen whose colour is read back on the next one.
            test.expect_pal_addr(tag + " PAL_ADDR leads by one dot",
                                 (d % 2) == 0 ? 1U : 0U);
        }
    }

    // Outside DE the address falls back to border entry 16 and the registered
    // read still lands on the correct dot.
    test.set_pal(16, 0x9, 0xA, 0xB);  // border -> RA G9 BB
    test.run_to_frame_start();
    test.run_characters(7);  // past R1=4, clear of sync at R2=40
    test.expect_pal_addr("registered palette border address", 16U);
    test.expect_rgb("registered palette border colour", 0xA, 0x9, 0xB);
}

// ---- t06: sprite-plane precedence (P4) ---------------------------------
//
// asic_video owns the final mux. Reference §5: border > sprite 0..15 >
// screen, with HSYNC force-blank above everything. The bench drives the
// SPR_EN/SPR_RGB inputs directly; the engine behind them is pinned by
// sim/plus/asic_sprites_test.cpp.

// Inside DE a sprite pixel replaces the decoded ink; PEN keeps reporting
// the screen-side decode regardless (it observes the CRTC/GA path only).
void t06a_sprite_over_screen_ink(TestBench& test) {
    program_pixel_frame(test);
    TestPalette pal;
    apply_palette(test, pal);
    test.set_ga(2, 24, 0xFFFFU);  // mode 2, all pens = 1 (bright white)
    test.run_to_frame_start();
    test.align_to_character_start();
    // Registered RGB lags one dot: after two executed dots the sample
    // shows body dot 0's decode.
    test.run_dots(2);
    test.expect_rgb("t06a screen ink before sprite", kWhiteR, kWhiteG,
                    kWhiteB);
    test.set_sprite(1, 5, 10, 15);
    test.run_dots(1);
    test.expect_rgb("t06a sprite pixel over ink", 5, 10, 15);
    test.expect_pen("t06a PEN still reports the screen pen", false, 1);
    test.set_sprite(0, 5, 10, 15);
    test.run_dots(1);
    test.expect_rgb("t06a screen ink restored", kWhiteR, kWhiteG, kWhiteB);
}

// Outside DE the border beats an active sprite pixel.
void t06b_border_over_sprite(TestBench& test) {
    program_pixel_frame(test);
    TestPalette pal;
    apply_palette(test, pal);
    test.set_ga(2, 24, 0xFFFFU);
    test.run_to_frame_start();
    test.run_characters(6);  // characters 4..5: past R1=4, clear of sync
    test.expect_rgb("t06b border without sprite", 6, 0, 6);
    test.set_sprite(1, 5, 10, 15);
    test.run_dots(1);
    test.expect_rgb("t06b border still wins over sprite", 6, 0, 6);
    // t06c: HSYNC force-blank beats everything, sprite included.
    do {
        test.run_characters(1);
    } while (!test.hsync());
    test.run_dots(3);  // registered output picks up the forced blank
    test.expect_rgb("t06c HSYNC blanks active sprite", 0, 0, 0);
}

// ---- t07: CRTC-3 bus readback and status groups (P5) -----------------

void expect_mask(const std::string& context, std::uint8_t actual,
                 std::uint8_t mask, std::uint8_t expected) {
    if ((actual & mask) != expected) {
        TestBench::fail_unsigned(context, expected, actual & mask);
    }
}

// ACCC §21.2.3 p.246: reads use addr[2:0] through the fixed
// R16/R17/STATUS1/STATUS2/R12/R13/R14/R15 map. R12 is a full eight-bit
// readback register (§20.5 p.244), while R14/R16 force bits 7:6 to zero.
// Writes keep the full index: writing R4 above must not change slot 4's R12.
void t07a_mod8_read_map_and_storage(TestBench& test) {
    test.write_register(0, 7);
    test.write_register(1, 4);
    test.write_register(2, 5);
    test.write_register(3, 0x22);
    test.write_register(4, 2);
    test.write_register(5, 0);
    test.write_register(6, 100);
    test.write_register(7, 100);
    test.write_register(9, 3);
    test.write_register(12, 0xD2); // D7:D6 stored for readback; VMA uses 0x12
    test.write_register(13, 0x34);
    test.write_register(14, 0x5A); // six-bit R14 stores 0x1A
    test.write_register(15, 0xC3);
    test.run_to_frame_start();

    test.expect_ma("t07a VMA uses R12[5:0], not extended bits", 0x1234);
    if (test.read_register(0) != 0x00 || test.read_register(1) != 0x00)
        TestBench::fail_unsigned("t07a reset light-pen slots", 0, 1);
    if (test.read_register(4) != 0xD2)
        TestBench::fail_unsigned("t07a slot 4 returns full R12", 0xD2,
                                 test.read_register(4));
    if (test.read_register(12) != 0xD2 || test.read_register(20) != 0xD2)
        TestBench::fail_unsigned("t07a modulo-8 R12 aliases", 0xD2,
                                 test.read_register(20));
    if (test.read_register(5) != 0x34)
        TestBench::fail_unsigned("t07a slot 5 returns R13", 0x34,
                                 test.read_register(5));
    if (test.read_register(6) != 0x1A)
        TestBench::fail_unsigned("t07a slot 6 returns masked R14", 0x1A,
                                 test.read_register(6));
    if (test.read_register(7) != 0xC3)
        TestBench::fail_unsigned("t07a slot 7 returns R15", 0xC3,
                                 test.read_register(7));
}

// ACCC §21.3.4.1 p.248, paper-derived for R0=7/R1=4/R2=5/R3l=2:
// at C0=3 the R0/2 and R1-1 active-low flags coincide (F8); C0=5 is
// HSYNC start (F6); §21.3.4.1 puts bit 4 low at C0=R2+R3=7, where
// C0=R0 also sets bit 0.
void t07b_status1_horizontal_events(TestBench& test) {
    test.write_register(9, 3);
    test.write_register(4, 2);
    test.write_register(5, 0);
    test.write_register(1, 4);
    test.write_register(2, 5);
    test.write_register(3, 0x22);
    test.write_register(6, 100);
    test.write_register(7, 100);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.write_register(0, 7);
    test.select_register(2);
    test.run_to_frame_start();

    if (test.sample_selected() != 0xFE)
        TestBench::fail_unsigned("t07b status1 at C0=0", 0xFE,
                                 test.sample_selected());
    test.run_characters(3);
    if (test.sample_selected() != 0xF8)
        TestBench::fail_unsigned("t07b coincident R0/2 and R1-1", 0xF8,
                                 test.sample_selected());
    test.run_characters(2);
    if (test.sample_selected() != 0xF6)
        TestBench::fail_unsigned("t07b C0=R2 active-low", 0xF6,
                                 test.sample_selected());
    test.run_characters(1);
    if (test.sample_selected() != 0xFE)
        TestBench::fail_unsigned("t07b before STATUS1 R2+R3", 0xFE,
                                 test.sample_selected());
    test.run_characters(1);
    if (test.sample_selected() != 0xEF)
        TestBench::fail_unsigned("t07b C0=R0 and R2+R3", 0xEF,
                                 test.sample_selected());
}

// ACCC §21.3.4.1 p.248: status-1 bit 7 is active-low when the next
// character wraps VMA.LSB, or at C0=R0 when the saved row base LSB is 00.
void t07c_status1_pointer_preview(TestBench& test) {
    test.write_register(0, 7);
    test.write_register(1, 4);
    test.write_register(4, 3);
    test.write_register(5, 0);
    test.write_register(6, 100);
    test.write_register(7, 100);
    test.write_register(9, 0);
    test.write_register(12, 0x12);
    test.write_register(13, 0xFD);
    test.select_register(2);
    test.run_to_frame_start();
    test.run_characters(2); // VMA=0x12FF at C0=2
    expect_mask("t07c VMA low-byte wrap preview", test.sample_selected(),
                0x80, 0x00);

    test.reset();
    test.write_register(0, 7);
    test.write_register(1, 4);
    test.write_register(4, 3);
    test.write_register(5, 0);
    test.write_register(6, 100);
    test.write_register(7, 100);
    test.write_register(9, 0);
    test.write_register(12, 0x12);
    test.write_register(13, 0xFC);
    test.select_register(2);
    test.run_to_frame_start();
    test.run_characters(7); // R1 capture saved 0x1300; now C0=R0
    expect_mask("t07c VMA' zero-low-byte reload preview", test.sample_selected(),
                0x80, 0x00);
}

// ACCC §21.3.4.1 p.248 + [KT] CRTC Status 1: bit 5 is zero only on
// the final line of the effective VSYNC pulse. [KT] supplies the R3h=0
// sixteenth-line result; ACCC documents only the preceding 15 lines.
void t07d_status1_last_vsync_line(TestBench& test) {
    auto seek_vsync = [&](const std::string& context) {
        unsigned guard = 0;
        while (!test.vsync()) {
            test.run_characters(1);
            if (++guard > 4096) throw TestFailure(context + ": no VSYNC");
        }
    };

    test.write_register(0, 3);  // four characters per line
    test.write_register(3, 0x22);
    test.write_register(4, 4);
    test.write_register(5, 0);
    test.write_register(7, 1);
    test.write_register(9, 7);
    test.select_register(2);
    test.run_to_frame_start();
    seek_vsync("t07d width-2");
    expect_mask("t07d first of two VSYNC lines", test.sample_selected(),
                0x20, 0x20);
    test.run_characters(4);
    expect_mask("t07d second/last VSYNC line", test.sample_selected(),
                0x20, 0x00);
    test.run_characters(4);
    expect_mask("t07d after width-2 VSYNC", test.sample_selected(),
                0x20, 0x20);

    test.reset();
    test.write_register(0, 1);  // two characters per line
    test.write_register(3, 0x02); // R3h=0 -> 16 lines
    test.write_register(4, 4);
    test.write_register(5, 0);
    test.write_register(7, 1);
    test.write_register(9, 31);
    test.select_register(2);
    test.run_to_frame_start();
    seek_vsync("t07d width-16");
    test.run_characters(28); // line index 14
    expect_mask("t07d fifteenth of sixteen VSYNC lines",
                test.sample_selected(), 0x20, 0x20);
    test.run_characters(2); // line index 15
    expect_mask("t07d sixteenth/last VSYNC line", test.sample_selected(),
                0x20, 0x00);
    test.run_characters(2);
    expect_mask("t07d after width-16 VSYNC", test.sample_selected(),
                0x20, 0x20);
}

// ACCC §21.3.4.2 p.249. Constants: bit4=1 and bit6=0. Bit5 is zero
// throughout C9=R9. Bit7 is one at C9=0 before C0=R0 and at the final
// C9=R9/C0=R0 character. Bits 1/2/0 pulse low at the three named line ends.
void t07e_status2_vertical_events(TestBench& test) {
    test.write_register(0, 3);
    test.write_register(4, 3);
    test.write_register(5, 0);
    test.write_register(6, 2);
    test.write_register(7, 3);
    test.write_register(9, 1);
    test.select_register(3);
    test.run_to_frame_start();

    expect_mask("t07e STATUS2 constants and C9=0 body",
                test.sample_selected(), 0xF0, 0xB0);
    test.run_characters(3); // C9=0, C0=R0
    expect_mask("t07e C9=0 final character", test.sample_selected(),
                0xA0, 0x20);
    test.run_characters(1); // C9=R9, C0=0
    expect_mask("t07e C9=R9 body", test.sample_selected(), 0xA0, 0x00);
    test.run_characters(3); // C9=R9, C0=R0
    expect_mask("t07e C9=R9 final character", test.sample_selected(),
                0xA0, 0x80);
    expect_mask("t07e no vertical terminal in row 0", test.sample_selected(),
                0x07, 0x07);

    test.run_to_state(1, 1, 3, "t07e last displayed");
    expect_mask("t07e C4=R6-1 terminal", test.sample_selected(), 0x02, 0x00);
    test.run_to_state(2, 1, 3, "t07e last before VSYNC");
    expect_mask("t07e C4=R7-1 terminal", test.sample_selected(), 0x04, 0x00);
    test.run_to_state(3, 1, 3, "t07e last screen character");
    expect_mask("t07e C4=R4 terminal", test.sample_selected(), 0x01, 0x00);
}

// ACCC §21.3.4.2 p.249: bit 3 is stable for a whole frame and toggles
// every 16 frame origins. With R9=7, R4=0, and R0=1, each frame has eight
// scanlines and 16 characters. Two 16-frame intervals catch a frame-origin
// pulse that fires on every C9 line end, independent of the reset phase.
// The absolute reset phase is the RTL's named zero assumption; this vector
// derives only the documented period.
void t07f_status2_frame16_timer(TestBench& test) {
    test.write_register(0, 1); // two characters per scanline
    test.write_register(4, 0);
    test.write_register(5, 0);
    test.write_register(7, 100);
    test.write_register(9, 7); // eight scanlines per frame
    test.select_register(3);
    test.run_to_frame_start();
    const std::uint8_t start = test.sample_selected() & 0x08;
    constexpr unsigned kFrameCharacters = 8 * 2;
    test.run_characters(kFrameCharacters * 16); // 16 frames * 8 lines * 2 chars
    const std::uint8_t flipped = test.sample_selected() & 0x08;
    if (flipped == start)
        TestBench::fail_unsigned("t07f timer did not toggle after 16 frames",
                                 start ^ 0x08, flipped);
    test.run_characters(kFrameCharacters * 16);
    if ((test.sample_selected() & 0x08) != start)
        TestBench::fail_unsigned("t07f timer did not return after 32 frames",
                                 start, test.sample_selected() & 0x08);
}

// §21.2.3 lists R10/R11/R16/R17 as read-only. With no light-pen strobe,
// writes to 16/17 do not move their named-zero model. Both RS levels read
// the selected register (&BE00/&BF00); unselected and write cycles remain
// wired-AND neutral FF.
void t07g_readonly_and_neutral_cycles(TestBench& test) {
    test.write_register(16, 0xAA);
    test.write_register(17, 0x55);
    if (test.read_register(0) != 0 || test.read_register(1) != 0)
        TestBench::fail_unsigned("t07g light-pen registers are read-only", 0, 1);
    test.select_register(4);
    const std::uint8_t bf_value = test.sample_selected(true);
    const std::uint8_t be_value = test.sample_selected(false);
    if (bf_value != 0 || be_value != 0)
        TestBench::fail_unsigned("t07g BE/BF read ports differ", 0, be_value);
    if (test.sample_selected(true, false) != 0xFF)
        TestBench::fail_unsigned("t07g unselected read is neutral", 0xFF,
                                 test.sample_selected(true, false));
    if (test.sample_selected(true, true, false) != 0xFF)
        TestBench::fail_unsigned("t07g write cycle is neutral", 0xFF,
                                 test.sample_selected(true, true, false));
}

// P6: Screen split (SPLT &6801, SSA &6802/&6803) and Soft scroll (SSCR &6804).
// Sources: docs/plus/references/asic-reference.md §8 ([ARNOLD §2.3/§2.5], [KT], [QUASAR], ACCC §20.5).

// t08a: Screen split capture at HCC==R1 on the SPLT line, line reload from SSA,
// and subsequent row advance from the SSA base.
void t08a_split_screen_capture_and_advance(TestBench& test) {
    program_display_frame(test);
    // program_display_frame: R0=7 (8 chars/line), R1=4, R9=3 (4 lines/row), R4=2.
    // SPLT = 9: {charline=1, raster=1} -> (1 << 3) | 1 = 9. SSA = 0x2400.
    test.set_splt(9);
    test.set_ssa(0x2400);
    test.run_until_vsync_idle();
    test.run_to_frame_start();

    // Row 0 lines 0..3 (scanlines 0..3): MA starts at 0x1234.
    test.expect_ma("t08a row 0 line 0", 0x1234);
    test.run_characters(8);
    test.expect_ma("t08a row 0 line 1", 0x1234);
    test.run_characters(8);
    test.expect_ma("t08a row 0 line 2", 0x1234);
    test.run_characters(8);
    test.expect_ma("t08a row 0 line 3", 0x1234);

    // Row 1 line 0 (scanline 4, {1, 0} = 8): starts at 0x1234 + 4 = 0x1238.
    test.run_characters(8);
    test.expect_ma("t08a row 1 line 0", 0x1238);

    // Row 1 line 1 (scanline 5, {1, 1} = 9 == SPLT): starts at 0x1238.
    test.run_characters(8);
    test.expect_ma("t08a split line starts with pre-split base", 0x1238);

    // Row 1 line 2 (scanline 6): line after SPLT starts with SSA (0x2400)!
    test.run_characters(8);
    test.expect_ma("t08a line after SPLT starts with SSA", 0x2400);

    // Row 1 line 3 (scanline 7): starts with SSA (0x2400).
    test.run_characters(8);
    test.expect_ma("t08a row 1 line 3 starts with SSA", 0x2400);

    // Row 2 line 0 (scanline 8): starts with SSA + R1 (0x2400 + 4 = 0x2404)!
    test.run_characters(8);
    test.expect_ma("t08a subsequent row advances from SSA base", 0x2404);

    // Verify next frame start reloads from R12/R13 (0x1234).
    test.run_to_frame_start();
    test.expect_ma("t08a frame start restores R12/R13 base", 0x1234);
}

// t08b: SPLT=0 turns off the screen split facility; SSA is never latched.
void t08b_split_screen_disabled_when_zero(TestBench& test) {
    program_display_frame(test);
    test.set_splt(0);
    test.set_ssa(0x3000);
    test.run_until_vsync_idle();
    test.run_to_frame_start();

    test.expect_ma("t08b row 0", 0x1234);
    test.run_characters(8 * 4); // 4 lines of row 0
    test.expect_ma("t08b row 1", 0x1234 + 4);
    test.run_characters(8 * 4); // 4 lines of row 1
    test.expect_ma("t08b row 2", 0x1234 + 8);
}

// t08c: Multiple splits per frame by reprogramming SPLT/SSA mid-frame.
void t08c_split_screen_multiple_splits(TestBench& test) {
    program_display_frame(test);
    // Split 1 at row 1, line 0 ({1, 0} = 8) to 0x2000.
    test.set_splt(8);
    test.set_ssa(0x2000);
    test.run_until_vsync_idle();
    test.run_to_frame_start();

    test.run_characters(8 * 4); // scanline 4 (split 1 line)
    test.expect_ma("t08c split 1 line start", 0x1238);
    test.run_characters(8);     // scanline 5 (line after split 1)
    test.expect_ma("t08c split 1 applied", 0x2000);

    // Reprogram mid-frame for second split at row 2, line 0 ({2, 0} = 16) to 0x3000.
    test.set_splt(16);
    test.set_ssa(0x3000);

    test.run_characters(8 * 2); // scanlines 6 and 7 (row 1 lines 2 and 3)
    test.expect_ma("t08c row 1 line 3 still 0x2000", 0x2000);

    test.run_characters(8);     // scanline 8 (split 2 line == row 2 line 0)
    test.expect_ma("t08c row 2 line 0 starts at 0x2004", 0x2004);
    test.run_characters(8);     // scanline 9 (line after split 2)
    test.expect_ma("t08c split 2 applied", 0x3000);
}

// t08d: Split on row boundary (last line of a character row: raster == R9).
void t08d_split_screen_row_boundary(TestBench& test) {
    program_display_frame(test);
    // SPLT = 11: row 1, raster 3 ({1, 3} = (1 << 3) | 3 = 11).
    test.set_splt(11);
    test.set_ssa(0x2800);
    test.run_until_vsync_idle();
    test.run_to_frame_start();

    test.run_characters(8 * 7); // scanline 7 (row 1, raster 3)
    test.expect_ma("t08d row 1 line 3 start", 0x1238);
    test.run_characters(8);     // scanline 8 (row 2, line 0)
    // Next row starts directly from SSA (0x2800) because split overrode row capture.
    test.expect_ma("t08d next row starts from SSA", 0x2800);
}

// t08e: SSCR[6:4] vertical scanline offset added to RA[2:0].
void t08e_sscr_vertical_scanline_offset(TestBench& test) {
    program_display_frame(test);
    test.run_until_vsync_idle();
    test.run_to_frame_start();

    // Line 0: raster = 0. SSCR = 0 -> RA = 0.
    test.set_sscr(0x00);
    test.expect_ra("t08e sscr 0 raster 0", 0);
    test.expect_row("t08e row 0", 0);

    // Advance to line 1: raster = 1. SSCR = 0x10 (offset 1) -> RA = 2.
    test.run_characters(8);
    test.expect_row("t08e row 1", 1);
    test.set_sscr(0x10);
    test.expect_ra("t08e sscr 1 raster 1", 2);

    // Advance to line 2: raster = 2. SSCR = 0x30 (offset 3) -> RA = 5.
    test.set_sscr(0x30);
    test.run_characters(8);
    test.expect_row("t08e row 2", 2);
    test.expect_ra("t08e sscr 3 raster 2", 5);

    // Advance to line 3: raster = 3. SSCR = 0x30 (offset 3) -> RA = 6.
    test.run_characters(8);
    test.expect_row("t08e row 3", 3);
    test.expect_ra("t08e sscr 3 raster 3", 6);
}

// t08f: SSCR[3:0] horizontal pixel delay (0-15 mode-2 pixels).
void t08f_sscr_horizontal_pixel_delay(TestBench& test) {
    program_pixel_frame(test);
    TestPalette pal;
    apply_palette(test, pal);
    test.set_ga(2, 20 /* black border */, 0x0080 /* even byte = 0x80: bit 7 is ink 1 */);
    test.run_until_vsync_idle();
    test.run_to_frame_start();
    test.align_to_character_start();
    test.run_dots(1); // CLKEN edge: latches vid_even and decodes dot 0 pen

    // Delay 0: bit 7 (ink 1 = white) is at dot 0 -> output RGB at dot 1 is white.
    test.set_sscr(0x00);
    test.run_dots(1); // dot 0 presented to RGB flops
    test.expect_rgb("t08f delay 0 dot 1 is white", 15, 15, 15);
    test.run_dots(1); // dot 1 presented
    test.expect_rgb("t08f delay 0 dot 2 is black", 0, 0, 0);

    // Delay 4: bit 7 appears at dot 4 -> output RGB at dot 5 is white.
    test.run_until_vsync_idle();
    test.run_to_frame_start();
    test.align_to_character_start();
    test.set_sscr(0x04);
    test.run_dots(1); // CLKEN edge

    test.run_dots(1);
    test.expect_rgb("t08f delay 4 dot 1 is black", 0, 0, 0);
    test.run_dots(3);
    test.expect_rgb("t08f delay 4 dot 4 is black", 0, 0, 0);
    test.run_dots(1);
    test.expect_rgb("t08f delay 4 dot 5 is white", 15, 15, 15);
    test.run_dots(1);
    test.expect_rgb("t08f delay 4 dot 6 is black", 0, 0, 0);
}

// t08g: SSCR[7] border mask over first 16 dots of active display, with sprites unaffected.
void t08g_sscr_border_mask_and_sprites(TestBench& test) {
    program_pixel_frame(test);
    TestPalette pal;
    apply_palette(test, pal);
    test.set_ga(2, 20 /* black border */, 0xFFFF /* all ink 1 = white */);
    test.run_until_vsync_idle();
    test.run_to_frame_start();
    test.align_to_character_start();
    test.run_dots(1); // CLKEN edge

    // With SSCR[7] = 1 (0x80):
    test.set_sscr(0x80);
    // Character 0 (dots 0..15) of active display is masked to border (black).
    for (unsigned d = 1; d <= 16; ++d) {
        test.run_dots(1);
        test.expect_rgb("t08g char 0 masked to border", 0, 0, 0);
    }
    // Character 1 (dots 16..31) displays screen ink (white).
    test.run_dots(1);
    test.expect_rgb("t08g char 1 unmasked screen ink", 15, 15, 15);

    // Now test with sprite enabled: sprite is green (0, 15, 0).
    test.set_sprite(1, 0, 15, 0);
    test.run_until_vsync_idle();
    test.run_to_frame_start();
    test.align_to_character_start();
    test.run_dots(1); // CLKEN edge

    test.run_dots(1);
    test.expect_rgb("t08g sprite displays over masked border", 0, 15, 0);
    test.set_sprite(0, 0, 0, 0);
}

// t08h: 14-bit VMA overscan carry across 10-bit and 12-bit boundaries (§20.5 p.244).
void t08h_overscan_carry_14bit(TestBench& test) {
    program_display_frame(test);
    test.write_register(12, 0x03);
    test.write_register(13, 0xFE);
    test.run_until_vsync_idle();
    test.run_to_frame_start();

    test.expect_ma("t08h row 0 line 0 start", 0x03FE);
    test.run_characters(2);
    test.expect_ma("t08h overscan carry across 0x03FF to 0x0400", 0x0400);

    test.run_characters(6);
    test.expect_ma("t08h row 0 line 1 start", 0x03FE);

    test.run_characters(8 * 3);
    test.expect_ma("t08h row 1 starts with overscan carried base", 0x0402);
}

// t08i: SSCR vertical offset wraps RA at the row boundary and advances the
// source row base on the raw-raster-7/RA0 line (asic-reference §8).
void t08i_sscr_vertical_wrap_advances_ma(TestBench& test) {
    // Eight scanlines per source row, four displayed characters per line,
    // and a deliberately quiet VSYNC position make the boundary observable.
    test.write_register(9, 7);
    test.write_register(4, 2);
    test.write_register(5, 0);
    test.write_register(1, 4);
    test.write_register(6, 100);
    test.write_register(7, 100);
    test.write_register(12, 0x12);
    test.write_register(13, 0x34);
    test.write_register(0, 7);
    test.set_sscr(0x10); // vertical offset = 1
    test.run_until_vsync_idle();
    test.run_to_frame_start();

    // The offset changes only RA's low three bits.  The source row base must
    // nevertheless advance before the wrapped RA=0 line: raw raster 0..6
    // are source row 0 (RA 1..7), raw raster 7 is source row 1 (RA 0), and
    // the following raw raster 0 remains on source row 1 (RA 1).
    for (unsigned raw_raster = 0; raw_raster < 7; ++raw_raster) {
        test.expect_line("t08i source row 0", 0);
        test.expect_row("t08i raw raster before wrap", raw_raster);
        test.expect_ma("t08i source base before wrap", 0x1234);
        test.expect_ra("t08i offset raster before wrap", raw_raster + 1);
        test.run_characters(8);
    }

    test.expect_line("t08i source row 1 at raw raster 7", 0);
    test.expect_row("t08i raw raster 7", 7);
    test.expect_ma("t08i raw raster 7 advances source base", 0x1238);
    test.expect_ra("t08i raw raster 7 wraps to RA 0", 0);
    test.run_characters(8);

    test.expect_line("t08i next source row", 1);
    test.expect_row("t08i next row starts at raw raster 0", 0);
    test.expect_ma("t08i next row keeps advanced source base", 0x1238);
    test.expect_ra("t08i next row starts at offset RA 1", 1);
}

constexpr std::array<TestCase, 67> kTests = {{
    {"t01a reset and R0=0 acceptance", t01a_reset_and_r0_zero},
    {"t01b R0=64-character line period", t01b_r63_period},
    {"t01c five-bit register select alias", t01c_register_select_alias},
    {"t01d live R0 widen mid-line", t01d_r0_widen_midline},
    {"t01e R0 shrink eight-bit overflow (unverified model assumption)",
     t01e_r0_shrink_overflow},
    {"t02a normal frame counter cycle", t02a_normal_frame_cycle},
    {"t02b lowered R9 forces C9 reset", t02b_r9_lowered_forces_reset},
    {"t02c lowered R4 overflows C4", t02c_r4_lowered_overflows},
    {"t02d adjustment entry keeps C4=R4", t02d_adjustment_entry_keeps_c4},
    {"t02e adjustment length and restart", t02e_adjustment_length_and_restart},
    {"t02f shrunk R5 ends adjustment", t02f_r5_shrink_ends_adjustment},
    {"t02g grown R5 extends adjustment", t02g_r5_grow_extends_adjustment},
    {"t02h IVM even-R9 entry parity", t02h_ivm_even_r9_preserves_entry_parity},
    {"t02i IVM odd-R9 row/frame parity", t02i_ivm_odd_r9_balances_rows_and_frames},
    {"t02j IVM exit to plain counting", t02j_ivm_exit_returns_to_plain_counting},
    {"t02k IVM even-frame adjustment", t02k_ivm_even_frame_adjustment},
    {"t02l IVM odd-frame adjustment", t02l_ivm_odd_frame_adjustment},
    {"t03a VMA reload and row advance", t03a_ma_reload_and_row_advance},
    {"t03b R1 border edges", t03b_r1_border_edges},
    {"t03c R1==R0 single-character blip", t03c_r1_eq_r0_blip},
    {"t03d R1>R0 no substitution, frozen rows", t03d_r1_gt_r0_no_substitution},
    {"t03e R6 line-start-only semantics", t03e_r6_line_start_semantics},
    {"t03h VDE rows past R6 stay bordered", t03h_vde_rows_past_r6_stay_bordered},
    {"t03f adjustment rows solidified", t03f_adjustment_rows_solidified},
    {"t03g SKEW-DISPTMG delay and BORDER ON", t03g_skew_delay_and_border_on},
    {"t04a HSYNC position and width", t04a_hsync_position_and_width},
    {"t04b R3l=0 sixteen-character HSYNC", t04b_r3_zero_means_sixteen},
    {"t04c R3l rewrite wraps the nibble", t04c_r3l_rewrite_wraps_nibble},
    {"t04d infinite-HSYNC bug", t04d_infinite_hsync},
    {"t04h live-R2 HSYNC end/start collision", t04h_live_r2_end_start_collision},
    {"t04i R3l=0 collision stays bounded (unverified model assumption)",
     t04i_r3_zero_collision_stays_bounded},
    {"t04e VSYNC gate and mid-row R7 write", t04e_vsync_gate_and_r7_write},
    {"t04f VSYNC width incl. legacy 16", t04f_vsync_width},
    {"t04g VSYNC refire without protection", t04g_no_reentrancy_continuous_refire},
    {"t04j IVM MID-VSYNC and odd-frame delay",
     t04j_ivm_mid_vsync_and_odd_frame_delay},
    {"t04k IVM R7=0 outgoing-parity priority",
     t04k_ivm_r7_zero_uses_outgoing_parity},
    {"t04l R7=R4 adjustment/interlace-line VSYNC seams",
     t04l_r7_r4_adjustment_and_interlace_line_seams},
    {"t04m IVM MID-VSYNC width and R8 exit",
     t04m_ivm_mid_vsync_width_and_exit},
    {"t04n sync-only interlace half-line VSYNC",
     t04n_sync_interlace_half_line_vsync},
    {"t05a legacy-colour ROM sweep ([KT])", t05a_legacy_colour_rom_sweep},
    {"t05b mode 2 sequential pixels", t05b_mode2_sequential_pixels},
    {"t05c mode 1 pair pixels", t05c_mode1_pair_pixels},
    {"t05d mode 0 nibble pixels", t05d_mode0_nibble_pixels},
    {"t05e border substitution and sync blank", t05e_border_substitution_and_sync_blank},
    {"t05f mode change latches at HSYNC", t05f_mode_change_latches_at_hsync},
    {"t05g mode 3 two-bit pixels", t05g_mode3_two_bit_pixels},
    {"t05h byte halves belong to their character "
     "(intra-character phase is an unverified model assumption)",
     t05h_byte_halves_belong_to_their_character},
    {"t05i ASIC 12-bit palette drives RGB (HF-2)",
     t05i_asic_palette_drives_rgb},
    {"t05j registered palette, alternating pens, PIXEN 1-in-4",
     t05j_registered_palette_alternating_pens},
    {"t06a sprite pixel over screen ink", t06a_sprite_over_screen_ink},
    {"t06b border over sprite; HSYNC blank over all",
     t06b_border_over_sprite},
    {"t07a modulo-8 read map and pointer storage", t07a_mod8_read_map_and_storage},
    {"t07b STATUS1 horizontal events", t07b_status1_horizontal_events},
    {"t07c STATUS1 video-pointer preview", t07c_status1_pointer_preview},
    {"t07d STATUS1 last VSYNC line", t07d_status1_last_vsync_line},
    {"t07e STATUS2 vertical events", t07e_status2_vertical_events},
    {"t07f STATUS2 16-frame timer", t07f_status2_frame16_timer},
    {"t07g read-only slots, dual read ports and neutral cycles", t07g_readonly_and_neutral_cycles},
    {"t08a screen split capture and advance", t08a_split_screen_capture_and_advance},
    {"t08b screen split disabled when zero", t08b_split_screen_disabled_when_zero},
    {"t08c multiple splits per frame", t08c_split_screen_multiple_splits},
    {"t08d split on row boundary", t08d_split_screen_row_boundary},
    {"t08e SSCR vertical scanline offset", t08e_sscr_vertical_scanline_offset},
    {"t08f SSCR horizontal pixel delay", t08f_sscr_horizontal_pixel_delay},
    {"t08g SSCR border mask and sprites", t08g_sscr_border_mask_and_sprites},
    {"t08h 14-bit VMA overscan carry", t08h_overscan_carry_14bit},
    {"t08i SSCR vertical wrap advances MA", t08i_sscr_vertical_wrap_advances_ma},
}};

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    unsigned passed = 0;
    for (const auto& test : kTests) {
        try {
            TestBench bench;
            test.run(bench);
            ++passed;
            std::cout << "PASS: " << test.name << '\n';
        } catch (const std::exception& error) {
            std::cerr << "FAIL: " << test.name << ": " << error.what() << '\n';
            return 1;
        }
    }

    std::cout << "All " << passed << " asic_video foundation tests passed\n";
    return 0;
}
