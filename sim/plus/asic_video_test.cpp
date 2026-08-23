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
        dut.nRESET = 0;
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
        for (unsigned tick = 0; tick < kClockTicksPerCharacter; ++tick) {
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

    void run_characters(std::uint64_t characters) {
        for (std::uint64_t character = 0; character < characters; ++character) {
            for (unsigned tick = 0; tick < kClockTicksPerCharacter; ++tick) {
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

    void clock_tick() {
        dut.CLKEN = tick_in_character_ == kClkEnPhase ? 1 : 0;
        dut.CLOCK = 0;
        dut.eval();
        dut.CLOCK = 1;
        dut.eval();
        dut.CLOCK = 0;
        dut.eval();
        tick_in_character_ =
            (tick_in_character_ + 1) % kClockTicksPerCharacter;
    }

    template <typename Expected, typename Actual>
    [[noreturn]] static void fail(const std::string& what,
                                  Expected expected, Actual actual) {
        throw TestFailure(what + ": expected " + std::to_string(expected) +
                          ", actual " + std::to_string(actual));
    }

    Vasic_video dut;
    unsigned tick_in_character_ = 0;
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

// Shrinking R0 below the current C0 makes the equality unreachable until
// C0 has overflowed the full eight-bit range and returned (ACCC §13.5,
// p.121: "R0 accepts all values without causing problems"; §28.1.1
// general overflow form). There is no type-0-style freeze or stall.
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

constexpr std::array<TestCase, 27> kTests = {{
    {"t01a reset and R0=0 acceptance", t01a_reset_and_r0_zero},
    {"t01b R0=64-character line period", t01b_r63_period},
    {"t01c five-bit register select alias", t01c_register_select_alias},
    {"t01d live R0 widen mid-line", t01d_r0_widen_midline},
    {"t01e R0 shrink eight-bit overflow", t01e_r0_shrink_overflow},
    {"t02a normal frame counter cycle", t02a_normal_frame_cycle},
    {"t02b lowered R9 forces C9 reset", t02b_r9_lowered_forces_reset},
    {"t02c lowered R4 overflows C4", t02c_r4_lowered_overflows},
    {"t02d adjustment entry keeps C4=R4", t02d_adjustment_entry_keeps_c4},
    {"t02e adjustment length and restart", t02e_adjustment_length_and_restart},
    {"t02f shrunk R5 ends adjustment", t02f_r5_shrink_ends_adjustment},
    {"t02g grown R5 extends adjustment", t02g_r5_grow_extends_adjustment},
    {"t03a VMA reload and row advance", t03a_ma_reload_and_row_advance},
    {"t03b R1 border edges", t03b_r1_border_edges},
    {"t03c R1==R0 single-character blip", t03c_r1_eq_r0_blip},
    {"t03d R1>R0 no substitution, frozen rows", t03d_r1_gt_r0_no_substitution},
    {"t03e R6 line-start-only semantics", t03e_r6_line_start_semantics},
    {"t03f adjustment rows solidified", t03f_adjustment_rows_solidified},
    {"t03g SKEW-DISPTMG delay and BORDER ON", t03g_skew_delay_and_border_on},
    {"t04a HSYNC position and width", t04a_hsync_position_and_width},
    {"t04b R3l=0 sixteen-character HSYNC", t04b_r3_zero_means_sixteen},
    {"t04c R3l rewrite wraps the nibble", t04c_r3l_rewrite_wraps_nibble},
    {"t04d infinite-HSYNC bug", t04d_infinite_hsync},
    {"t04h live-R2 HSYNC end/start collision", t04h_live_r2_end_start_collision},
    {"t04e VSYNC gate and mid-row R7 write", t04e_vsync_gate_and_r7_write},
    {"t04f VSYNC width incl. legacy 16", t04f_vsync_width},
    {"t04g VSYNC refire without protection", t04g_no_reentrancy_continuous_refire},
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
