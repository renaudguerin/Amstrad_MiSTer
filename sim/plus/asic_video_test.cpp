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

    void expect_hcc(const std::string& expectation, unsigned expected) const {
        if (dut.HCC != expected) {
            fail(expectation + ": HCC", expected, dut.HCC);
        }
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

struct TestCase {
    const char* name;
    void (*run)(TestBench&);
};

constexpr std::array<TestCase, 5> kTests = {{
    {"t01a reset and R0=0 acceptance", t01a_reset_and_r0_zero},
    {"t01b R0=64-character line period", t01b_r63_period},
    {"t01c five-bit register select alias", t01c_register_select_alias},
    {"t01d live R0 widen mid-line", t01d_r0_widen_midline},
    {"t01e R0 shrink eight-bit overflow", t01e_r0_shrink_overflow},
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
