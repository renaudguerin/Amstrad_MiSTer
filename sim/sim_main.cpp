#include <verilated.h>
#include <verilated_vcd_c.h>

#include "VUM6845R.h"
#include "VUM6845R___024root.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr unsigned kClockTicksPerCharacter = 16;
constexpr unsigned kClkEnPhase = 0;
constexpr unsigned kNClkEnPhase = 8;

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class KnownDivergenceFailure : public TestFailure {
public:
    using TestFailure::TestFailure;
};

class TestBench {
public:
    explicit TestBench(const std::string& trace_path)
        : context_(std::make_unique<VerilatedContext>()),
          dut_(std::make_unique<VUM6845R>(context_.get())),
          trace_(std::make_unique<VerilatedVcdC>()) {
        context_->traceEverOn(true);
        dut_->CLOCK = 0;
        dut_->CLKEN = 0;
        dut_->nCLKEN = 0;
        dut_->nRESET = 0;
        dut_->CRTC_TYPE = 0;
        idle_bus();
        dut_->SNA_LOAD = 0;
        dut_->SNA_ADDR = 0;
        for (unsigned word = 0; word < 5; ++word) {
            dut_->SNA_REGS[word] = 0;
        }

        dut_->trace(trace_.get(), 99);
        trace_->open(trace_path.c_str());
        eval_and_dump();
    }

    ~TestBench() {
        dut_->final();
        trace_->close();
    }

    TestBench(const TestBench&) = delete;
    TestBench& operator=(const TestBench&) = delete;

    void set_crtc_type(unsigned type) {
        if (type > 1) {
            fail("CRTC type 0 or 1", type);
        }
        dut_->CRTC_TYPE = type;
        eval_comb();
    }

    // Register values survive nRESET on the physical interface and in the RTL.
    // Program R8 before checking reset so FIELD does not depend on power-up data.
    void prepare_for_reset(unsigned type) {
        set_crtc_type(type);
        write_register(8, 0);
        reset();
    }

    void reset() {
        idle_bus();
        dut_->nRESET = 0;

        while (tick_in_character_ != kClkEnPhase) {
            clock_tick();
        }
        for (unsigned tick = 0; tick < kClockTicksPerCharacter; ++tick) {
            clock_tick();
        }

        dut_->nRESET = 1;
        character_cycles_ = 0;
        eval_comb();
    }

    void select_register(std::uint8_t address) {
        bus_write(false, address);
    }

    void write_register(std::uint8_t address, std::uint8_t value) {
        select_register(address);
        bus_write(true, value);
    }

    void write_selected_register_at_nclken(std::uint8_t value) {
        while (tick_in_character_ != kNClkEnPhase) {
            clock_tick();
        }
        bus_write(true, value);
    }

    void write_selected_register_at_clken(std::uint8_t value) {
        while (tick_in_character_ != kClkEnPhase) {
            clock_tick();
        }
        bus_write(true, value);
    }

    void write_selected_register_now(std::uint8_t value) {
        bus_write(true, value);
    }

    void hold_selected_register_at_clken(std::uint8_t value,
                                         unsigned clock_ticks) {
        while (tick_in_character_ != kClkEnPhase) {
            clock_tick();
        }
        dut_->ENABLE = 1;
        dut_->nCS = 0;
        dut_->R_nW = 0;
        dut_->RS = 1;
        dut_->DI = value;
        for (unsigned tick = 0; tick < clock_ticks; ++tick) {
            clock_tick();
        }
        idle_bus();
        eval_comb();
    }

    void load_snapshot_registers(
        const std::array<std::uint8_t, 10>& registers) {
        for (unsigned word = 0; word < 5; ++word) {
            dut_->SNA_REGS[word] = 0;
        }
        for (unsigned address = 0; address < registers.size(); ++address) {
            const unsigned bit = address * 8;
            dut_->SNA_REGS[bit / 32] |=
                static_cast<std::uint32_t>(registers[address]) << (bit % 32);
        }
        dut_->SNA_ADDR = 7;
        dut_->SNA_LOAD = 1;
        clock_tick();
        dut_->SNA_LOAD = 0;
        eval_comb();
    }

    std::uint8_t read_register(std::uint8_t address) {
        select_register(address);
        dut_->ENABLE = 1;
        dut_->nCS = 0;
        dut_->R_nW = 1;
        dut_->RS = 1;
        eval_comb();
        const std::uint8_t value = dut_->DO;
        idle_bus();
        eval_comb();
        return value;
    }

    std::uint8_t read_status() {
        dut_->ENABLE = 1;
        dut_->nCS = 0;
        dut_->R_nW = 1;
        dut_->RS = 0;
        eval_comb();
        const std::uint8_t value = dut_->DO;
        idle_bus();
        eval_comb();
        return value;
    }

    void run_characters(std::uint64_t characters) {
        for (std::uint64_t character = 0; character < characters; ++character) {
            for (unsigned tick = 0; tick < kClockTicksPerCharacter; ++tick) {
                clock_tick();
            }
        }
    }

    void run_clock_ticks(std::uint64_t ticks) {
        for (std::uint64_t tick = 0; tick < ticks; ++tick) {
            clock_tick();
        }
    }

    void expect_byte(const std::string& expectation,
                     std::uint8_t expected,
                     std::uint8_t actual) const {
        if (actual != expected) {
            std::ostringstream expected_text;
            std::ostringstream actual_text;
            expected_text << "0x" << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<unsigned>(expected);
            actual_text << "0x" << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<unsigned>(actual);
            fail(expectation + " == " + expected_text.str(), actual_text.str());
        }
    }

    void expect_known_byte(const std::string& expectation,
                           std::uint8_t expected,
                           std::uint8_t actual) const {
        if (actual != expected) {
            known_divergence(expectation + " == " + std::to_string(expected),
                             static_cast<unsigned>(actual));
        }
    }

    void expect_low(const std::string& signal, std::uint8_t actual) const {
        if (actual != 0) {
            fail(signal + " low", static_cast<unsigned>(actual));
        }
    }

    void expect_high(const std::string& signal, std::uint8_t actual) const {
        if (actual != 1) {
            fail(signal + " high", static_cast<unsigned>(actual));
        }
    }

    void expect_vsync_low(const std::string& expectation) const {
        expect_low(expectation, dut_->VSYNC);
    }

    void expect_vsync_high(const std::string& expectation) const {
        expect_high(expectation, dut_->VSYNC);
    }

    void expect_hsync_low(const std::string& expectation) const {
        expect_low(expectation, dut_->HSYNC);
    }

    void expect_hsync_high(const std::string& expectation) const {
        expect_high(expectation, dut_->HSYNC);
    }

    void expect_field_low(const std::string& expectation) const {
        expect_low(expectation, dut_->FIELD);
    }

    void expect_known_vsync_low(const std::string& expectation) const {
        if (dut_->VSYNC != 0) {
            known_divergence(expectation + " low",
                             static_cast<unsigned>(dut_->VSYNC));
        }
    }

    void expect_known_vsync_high(const std::string& expectation) const {
        if (dut_->VSYNC != 1) {
            known_divergence(expectation + " high",
                             static_cast<unsigned>(dut_->VSYNC));
        }
    }

    void expect_known_hsync_low(const std::string& expectation) const {
        if (dut_->HSYNC != 0) {
            known_divergence(expectation + " low",
                             static_cast<unsigned>(dut_->HSYNC));
        }
    }

    void expect_known_hsync_high(const std::string& expectation) const {
        if (dut_->HSYNC != 1) {
            known_divergence(expectation + " high",
                             static_cast<unsigned>(dut_->HSYNC));
        }
    }

    void expect_ra(const std::string& expectation, std::uint8_t expected) const {
        expect_byte(expectation, expected, dut_->RA);
    }

    void expect_c4(const std::string& expectation, std::uint8_t expected) const {
        expect_byte(expectation, expected, dut_->rootp->UM6845R__DOT__row);
    }

    void expect_known_ra(const std::string& expectation,
                         std::uint8_t expected) const {
        expect_known_byte(expectation, expected, dut_->RA);
    }

    void expect_known_c4(const std::string& expectation,
                         std::uint8_t expected) const {
        expect_known_byte(expectation, expected,
                          dut_->rootp->UM6845R__DOT__row);
    }

    // Whole-frame VSYNC observation for the section 28.1.1 R7 sweep: the pulse
    // is many characters wide, so one sample per character cannot miss it.
    bool vsync_within_characters(std::uint64_t characters) {
        for (std::uint64_t character = 0; character < characters; ++character) {
            run_characters(1);
            if (dut_->VSYNC != 0) {
                return true;
            }
        }
        return false;
    }

    void expect_vsync_observed(const std::string& expectation, bool seen) const {
        if (!seen) {
            fail(expectation, "no VSYNC pulse");
        }
    }

    void expect_no_vsync_observed(const std::string& expectation,
                                  bool seen) const {
        if (seen) {
            fail(expectation, "a VSYNC pulse");
        }
    }

    void expect_known_vsync_observed(const std::string& expectation,
                                     bool seen) const {
        if (!seen) {
            known_divergence(expectation, "no VSYNC pulse");
        }
    }

    void expect_adjustment_active(const std::string& expectation) const {
        expect_high(expectation, dut_->rootp->UM6845R__DOT__in_adj);
    }

    void expect_adjustment_inactive(const std::string& expectation) const {
        expect_low(expectation, dut_->rootp->UM6845R__DOT__in_adj);
    }

    void expect_type0_arbitration_latches(const std::string& expectation,
                                          bool r4_switch,
                                          bool r9_live,
                                          bool r9_at_r0,
                                          bool c0_1_adjust = false,
                                          bool r0_zero_consumed = false) const {
        expect_byte(expectation + " R4 switch", r4_switch,
                    dut_->rootp->UM6845R__DOT__type0_r4_adjust_switch);
        expect_byte(expectation + " live R9 compare", r9_live,
                    dut_->rootp->UM6845R__DOT__type0_r9_live_compare);
        expect_byte(expectation + " exact-R0 R9", r9_at_r0,
                    dut_->rootp->UM6845R__DOT__type0_r9_at_r0_pending);
        expect_byte(expectation + " C0=1 adjustment", c0_1_adjust,
                    dut_->rootp->UM6845R__DOT__type0_c0_1_adjust);
        expect_byte(expectation + " R0=0 entry consumed", r0_zero_consumed,
                    dut_->rootp->UM6845R__DOT__type0_r0_zero_entry_consumed);
    }

    std::uint16_t ma() const {
        return dut_->MA;
    }

    void expect_ma(const std::string& expectation, std::uint16_t expected) const {
        if (dut_->MA != expected) {
            fail(expectation + " == " + std::to_string(expected),
                 static_cast<unsigned>(dut_->MA));
        }
    }

    void expect_de_high(const std::string& expectation) const {
        expect_high(expectation, dut_->DE);
    }

    void expect_de_low(const std::string& expectation) const {
        expect_low(expectation, dut_->DE);
    }

    void expect_reset_outputs() const {
        expect_low("HSYNC after reset", dut_->HSYNC);
        expect_low("VSYNC after reset", dut_->VSYNC);
        expect_low("DE after reset", dut_->DE);
        expect_low("FIELD after reset", dut_->FIELD);
        expect_low("CURSOR after reset", dut_->CURSOR);
        expect_low("RA after reset", dut_->RA);
    }

    void expect_idle_bus_high() const {
        expect_byte("DO while the CRTC bus is inactive", 0xff, dut_->DO);
    }

    std::string timestamp() const {
        std::ostringstream text;
        text << "character " << character_cycles_ << ", tick "
             << tick_in_character_ << '/' << kClockTicksPerCharacter
             << ", trace-time " << trace_time_;
        return text.str();
    }

private:
    void idle_bus() {
        dut_->ENABLE = 0;
        dut_->nCS = 1;
        dut_->R_nW = 1;
        dut_->RS = 0;
        dut_->DI = 0;
    }

    void bus_write(bool register_data, std::uint8_t value) {
        dut_->ENABLE = 1;
        dut_->nCS = 0;
        dut_->R_nW = 0;
        dut_->RS = register_data ? 1 : 0;
        dut_->DI = value;
        clock_tick();
        idle_bus();
        eval_comb();
    }

    void clock_tick() {
        const unsigned active_phase = tick_in_character_;
        dut_->CLKEN = active_phase == kClkEnPhase;
        dut_->nCLKEN = active_phase == kNClkEnPhase;

        dut_->CLOCK = 0;
        eval_and_dump();
        dut_->CLOCK = 1;
        eval_and_dump();
        dut_->CLOCK = 0;
        eval_and_dump();

        tick_in_character_ =
            (tick_in_character_ + 1) % kClockTicksPerCharacter;
        if (active_phase == kClkEnPhase) {
            ++character_cycles_;
        }
    }

    void eval_comb() {
        dut_->eval();
    }

    void eval_and_dump() {
        dut_->eval();
        trace_->dump(trace_time_++);
    }

    template <typename Actual>
    [[noreturn]] void fail(const std::string& expected, Actual actual) const {
        std::ostringstream text;
        text << timestamp() << ": expected " << expected << ", actual " << actual;
        throw TestFailure(text.str());
    }

    template <typename Actual>
    [[noreturn]] void known_divergence(const std::string& expected,
                                       Actual actual) const {
        std::ostringstream text;
        text << timestamp() << ": expected " << expected << ", actual " << actual;
        throw KnownDivergenceFailure(text.str());
    }

    std::unique_ptr<VerilatedContext> context_;
    std::unique_ptr<VUM6845R> dut_;
    std::unique_ptr<VerilatedVcdC> trace_;
    vluint64_t trace_time_ = 0;
    std::uint64_t character_cycles_ = 0;
    unsigned tick_in_character_ = 0;
};

struct TestCase {
    std::string name;
    std::string source_rule;
    bool known_divergence;
    std::function<void(TestBench&)> run;
};

void test_reset_and_idle_bus(TestBench& test) {
    for (unsigned type = 0; type <= 1; ++type) {
        test.prepare_for_reset(type);
        test.expect_reset_outputs();
        test.expect_idle_bus_high();
    }
}

void test_register_readback_table(TestBench& test) {
    constexpr std::array<std::uint8_t, 16> written = {
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0xea, 0xeb, 0xd2, 0xa5, 0xde, 0xaf,
    };

    for (unsigned type = 0; type <= 1; ++type) {
        test.set_crtc_type(type);
        for (unsigned address = 0; address < written.size(); ++address) {
            test.write_register(address, written[address]);
        }

        std::array<std::uint8_t, 32> expected{};
        if (type == 0) {
            expected[12] = 0x12;  // Stored R12 is six bits wide.
            expected[13] = 0xa5;
        }
        expected[14] = 0x1e;      // Stored R14 is six bits wide.
        expected[15] = 0xaf;
        if (type == 1) {
            expected[31] = 0xff;
        }

        for (unsigned address = 0; address < expected.size(); ++address) {
            std::ostringstream label;
            label << "type " << type << " register R" << address << " readback";
            test.expect_byte(label.str(), expected[address],
                             test.read_register(address));
        }

        // Only the five low address bits are decoded on both CRTC types.
        test.expect_byte("R12 modulo-32 alias", expected[12],
                         test.read_register(0xac));
        test.expect_byte("R31 modulo-32 alias", expected[31],
                         test.read_register(0xff));
    }
}

constexpr unsigned kF3LineCharacters = 8;
constexpr unsigned kF3MidlineHcc = 3;
constexpr unsigned kVsyncLines = 16;

void configure_f3_midline_fixture(TestBench& test,
                                  unsigned type,
                                  unsigned vertical_sync_width = 2) {
    test.set_crtc_type(type);

    // Hold C4 at zero for the whole measurement. R7 starts unequal to C4 so
    // that only the explicitly timed R7=0 write can arm VSYNC.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, kF3LineCharacters - 1}, {1, 4}, {2, 5},
        {3, static_cast<std::uint8_t>((vertical_sync_width << 4) | 1)}, {4, 3},
        {5, 0},                     {6, 3}, {7, 1}, {8, 0},    {9, 31},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(7);
    test.reset();
}

void write_r7_zero_at_hcc(TestBench& test, unsigned hcc) {
    // reset() leaves C0 at zero immediately before a CLKEN edge. Driving the
    // data write on that edge makes the old C0 value seen by the RTL exactly
    // `hcc`, without inspecting an internal counter.
    test.run_characters(hcc);
    test.expect_vsync_low("VSYNC before the timed R7 write");
    test.write_selected_register_at_clken(0);

    // VSYNC is deliberately registered once more at the output pin. One raw
    // 16 MHz tick exposes the R7 handler's immediate assertion there.
    test.run_clock_ticks(1);
}

void test_type0_r7_hcc_blocked(TestBench& test, unsigned hcc) {
    configure_f3_midline_fixture(test, 0);
    write_r7_zero_at_hcc(test, hcc);

    std::ostringstream immediate;
    immediate << "type 0 R7=C4 write at C0=" << hcc << " is blocked";
    test.expect_vsync_low(immediate.str());

    // A blocked comparison is consumed: remaining on C4=R7 must not produce a
    // delayed pulse on either of the following lines.
    test.run_characters(2 * kF3LineCharacters);
    test.expect_vsync_low("type 0 blocked R7 comparison remains consumed");
}

void test_type0_r7_hcc0_blocked(TestBench& test) {
    test_type0_r7_hcc_blocked(test, 0);
}

void test_type0_r7_hcc1_blocked(TestBench& test) {
    test_type0_r7_hcc_blocked(test, 1);
}

void test_type0_r7_midline_duration_extended(TestBench& test) {
    configure_f3_midline_fixture(test, 0);
    write_r7_zero_at_hcc(test, kF3MidlineHcc);
    test.expect_vsync_high("type 0 C0>1 R7 write asserts VSYNC immediately");

    // R3h=2. From C0=3, R0-C0=4 characters remain before the first line
    // boundary. Type 0 begins loading/counting C3h there, so two *complete*
    // lines must then elapse before VSYNC ends.
    test.run_characters((kF3LineCharacters - 1) - kF3MidlineHcc);
    test.expect_vsync_high("type 0 VSYNC at its first post-write line boundary");
    test.run_characters(kF3LineCharacters);
    test.expect_vsync_high(
        "type 0 partial line does not consume either R3h line");
    test.run_characters(kF3LineCharacters);
    test.expect_vsync_low("type 0 extended VSYNC ends after two complete lines");
}

void test_type1_r7_early_hcc_immediate(TestBench& test) {
    for (const unsigned hcc : {0U, 1U}) {
        configure_f3_midline_fixture(test, 1);
        write_r7_zero_at_hcc(test, hcc);

        std::ostringstream expectation;
        expectation << "type 1 R7=C4 write at C0=" << hcc
                    << " asserts VSYNC immediately";
        test.expect_vsync_high(expectation.str());
    }
}

void test_type1_r7_midline_partial_counts(TestBench& test) {
    configure_f3_midline_fixture(test, 1);
    write_r7_zero_at_hcc(test, kF3MidlineHcc);
    test.expect_vsync_high("type 1 mid-line R7 write asserts VSYNC immediately");

    // Type 1 always uses a 16-line VSYNC. Its current partial line is count 1:
    // at C0=3 the pulse therefore ends after 4 + 15*8 = 124 characters, four
    // characters (C0+1) shorter than a line-aligned 16*8-character pulse.
    test.run_characters((kF3LineCharacters - 1) - kF3MidlineHcc);
    test.run_characters((kVsyncLines - 2) * kF3LineCharacters);
    test.expect_vsync_high("type 1 VSYNC through its fifteenth line boundary");
    test.run_characters(kF3LineCharacters);
    test.expect_vsync_low("type 1 partial-line VSYNC at its sixteenth boundary");
}

void configure_vsync_reentrancy_fixture(TestBench& test,
                                        unsigned type,
                                        unsigned vertical_total,
                                        unsigned max_scanline);

void test_r7_level_write_and_active_rearm(TestBench& test) {
    constexpr unsigned line_characters = 4;
    constexpr unsigned held_characters = 2;

    for (unsigned type = 0; type <= 1; ++type) {
        configure_vsync_reentrancy_fixture(test, type, 0, 0);
        test.run_characters(line_characters);
        test.expect_vsync_high("initial VSYNC before held equal R7 write");

        test.hold_selected_register_at_clken(
            0, held_characters * kClockTicksPerCharacter);
        test.expect_vsync_high("held equal R7 write does not disturb active VSYNC");
        test.run_characters(kVsyncLines * line_characters - held_characters);
        test.expect_vsync_low("held equal R7 write remains consumed at VSYNC end");
        test.run_characters(2 * line_characters);
        test.expect_vsync_low("held equal R7 write cannot re-trigger while equal");

        configure_vsync_reentrancy_fixture(test, type, 1, 0);
        test.run_characters(2 * line_characters);
        test.expect_vsync_high("initial VSYNC before different R7 write");
        test.hold_selected_register_at_clken(
            1, held_characters * kClockTicksPerCharacter);
        test.expect_vsync_high("different R7 write does not cancel active VSYNC");
        test.run_characters(kVsyncLines * line_characters - held_characters);
        test.expect_vsync_low("active VSYNC completes after different R7 write");
        test.run_characters(line_characters);
        test.expect_vsync_high("different R7 write permits the next genuine match");
    }
}

void test_type0_dynamic_vsync_width_extremes(TestBench& test) {
    for (const unsigned programmed_width : {0U, 1U, 15U}) {
        const unsigned effective_width = programmed_width == 0 ? 16 : programmed_width;
        configure_f3_midline_fixture(test, 0, programmed_width);
        write_r7_zero_at_hcc(test, kF3MidlineHcc);
        test.expect_vsync_high("type 0 dynamic VSYNC starts for width extreme");

        test.run_characters((kF3LineCharacters - 1) - kF3MidlineHcc);
        test.expect_vsync_high("type 0 partial line is excluded for width extreme");
        if (effective_width > 1) {
            test.run_characters((effective_width - 1) * kF3LineCharacters);
            test.expect_vsync_high("type 0 VSYNC survives all but its final full line");
        }
        test.run_characters(kF3LineCharacters);
        test.expect_vsync_low("type 0 width extreme ends after complete lines");
    }
}

void test_type0_r7_c0_2_at_line_boundary(TestBench& test) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 2}, {1, 1}, {2, 1}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 3}, {7, 1}, {8, 0},    {9, 31},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(7);
    test.reset();

    write_r7_zero_at_hcc(test, 2);
    test.expect_vsync_high("type 0 R7=C4 write at C0=2 is not blocked");
    test.run_characters(3);
    test.expect_vsync_low("C0=2/R0 count boundary consumes no partial-line skip");
}

void configure_f3_interlace_fixture(TestBench& test) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 15}, {1, 8}, {2, 10}, {3, 0x11}, {4, 0},
        {5, 0},  {6, 1}, {7, 1},  {8, 3},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(7);
    test.reset();
    test.run_characters(16);
    test.expect_field_low("interlace fixture reaches the half-line-count field");
}

void expect_interlace_dynamic_vsync_end(TestBench& test,
                                        unsigned write_hcc,
                                        unsigned characters_to_end) {
    configure_f3_interlace_fixture(test);
    write_r7_zero_at_hcc(test, write_hcc);
    test.expect_vsync_high("interlace-field R7=C4 write asserts VSYNC");
    test.run_characters(characters_to_end - 1);
    test.expect_vsync_high("interlace-field VSYNC remains high before final count tick");
    test.run_characters(1);
    test.expect_vsync_low("interlace-field VSYNC ends on the expected count tick");
}

void test_type0_interlace_count_boundaries(TestBench& test) {
    // In the second field with R0=15, the count tick sees old C0=6
    // (hcc_next=R0/2).  Before/on/after exercise the shared predicate; C0=15
    // proves that hcc_last itself is not a count tick in this field.
    expect_interlace_dynamic_vsync_end(test, 5, 26);
    expect_interlace_dynamic_vsync_end(test, 6, 25);
    expect_interlace_dynamic_vsync_end(test, 7, 31);
    expect_interlace_dynamic_vsync_end(test, 15, 23);
}

void test_type0_pending_skip_clears_on_type_roundtrip(TestBench& test) {
    configure_f3_midline_fixture(test, 0, 1);
    write_r7_zero_at_hcc(test, kF3MidlineHcc);
    test.expect_vsync_high("type 0 mid-line VSYNC has a pending first-line skip");

    test.set_crtc_type(1);
    test.run_clock_ticks(1);
    test.set_crtc_type(0);
    test.run_characters((kF3LineCharacters - 1) - kF3MidlineHcc);
    test.expect_vsync_low("live type round-trip clears the type 0 pending skip");
}

void test_type0_pending_skip_clears_on_snapshot_load(TestBench& test) {
    configure_f3_midline_fixture(test, 0, 1);
    write_r7_zero_at_hcc(test, kF3MidlineHcc);
    test.expect_vsync_high("type 0 mid-line VSYNC before snapshot load");

    const std::array<std::uint8_t, 10> snapshot_registers = {{
        kF3LineCharacters - 1, 4, 5, 0x11, 3, 0, 3, 0, 0, 31,
    }};
    test.load_snapshot_registers(snapshot_registers);
    test.run_characters((kF3LineCharacters - 1) - kF3MidlineHcc);
    test.expect_vsync_low("snapshot load clears the derived pending-line skip");
}

void configure_vsync_reentrancy_fixture(TestBench& test,
                                        unsigned type,
                                        unsigned vertical_total,
                                        unsigned max_scanline) {
    test.set_crtc_type(type);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x01}, {4, vertical_total},
        {5, 0}, {6, 1}, {7, 0}, {8, 0},    {9, max_scanline},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(7);
    test.reset();
}

void test_vsync_compare_lock_and_rearm(TestBench& test) {
    constexpr unsigned line_characters = 4;

    for (unsigned type = 0; type <= 1; ++type) {
        configure_vsync_reentrancy_fixture(test, type, 0, 0);
        test.expect_vsync_low("VSYNC before the first C4=R7 comparison");

        test.run_characters(line_characters);
        test.expect_vsync_high("R7=0/R4=0 initial VSYNC");
        test.run_characters(kVsyncLines * line_characters);
        test.expect_vsync_low("R7=0/R4=0 VSYNC completes once");
        test.run_characters(2 * line_characters);
        test.expect_vsync_low("unchanged C4=R7 truth does not refire VSYNC");

        // Rewriting R7 away and back changes the comparison truth and re-arms
        // mechanism 2. Return to R7=0 at C0=2, outside type 0's blocked window.
        test.write_selected_register_at_clken(1);
        test.run_characters(1);
        test.write_selected_register_at_clken(0);
        test.run_clock_ticks(1);
        test.expect_vsync_high("R7 truth-value change re-arms VSYNC");
    }
}

void test_vsync_reentrancy_bypass(TestBench& test) {
    constexpr unsigned line_characters = 4;
    constexpr unsigned frame_lines = 16;  // (R4+1) * (R9+1) = 2 * 8.

    for (unsigned type = 0; type <= 1; ++type) {
        configure_vsync_reentrancy_fixture(test, type, 1, 7);

        test.run_characters((frame_lines - 1) * line_characters);
        test.expect_vsync_low("R7=0/R4=1 VSYNC before the frame boundary");
        test.run_characters(line_characters);
        test.expect_vsync_high("R7=0/R4=1 initial VSYNC at frame boundary");

        // During each 16-line pulse C4 visits 1 and returns to 0. That false-
        // then-true transition re-arms VSYNC exactly as C3h expires, yielding
        // the documented continuous/infinite raw CRTC VSYNC.
        for (unsigned character = 0;
             character < 3 * frame_lines * line_characters;
             ++character) {
            test.run_characters(1);
            test.expect_vsync_high(
                "R7=0/R4=1/R9=7 VSYNC remains continuous across retriggers");
        }
    }
}

void test_type1_status_r6_zero_forced_border(TestBench& test) {
    test.set_crtc_type(1);

    // Eight characters per line, one scanline per row, four rows per frame.
    // R6=3 leaves rows 0..2 displayed after the first complete frame.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 3}, {7, 2}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(6);
    test.reset();

    test.run_characters(32);  // Complete the initial frame and enable display.
    test.run_characters(10);  // Enter row 1, away from C0=R0.
    test.expect_de_high("DE before R6=0 forced border");
    test.expect_byte("type 1 status in displayed row", 0x00, test.read_status());

    // ACCC v1.10 section 21.3.3: setting R6=0 while C4>0 forces border,
    // but that special case must not set the C4==R6 status condition.
    test.write_selected_register_at_nclken(0);
    test.expect_de_low("DE after R6=0 forced border");
    test.expect_byte("type 1 status after R6=0 forced border at C4>0",
                     0x00, test.read_status());

    test.run_characters(5);
    test.expect_byte("type 1 status after sampling R6=0 forced border",
                     0x00, test.read_status());

    test.run_characters(15);
    test.expect_byte("type 1 R6=0 status before frame origin", 0x00,
                     test.read_status());
    test.run_characters(1);
    test.expect_byte("type 1 R6=0 status at frame origin", 0x00,
                     test.read_status());
}

void test_type1_status_waits_for_r0_sample(TestBench& test) {
    test.set_crtc_type(1);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 3}, {7, 2}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(6);
    test.reset();

    test.run_characters(32);  // Display becomes active at the frame boundary.
    test.run_characters(10);  // Row 1, several characters before C0=R0.
    test.expect_byte("type 1 status before an R6-border write", 0x00,
                     test.read_status());

    // Matching R6 to current C4 activates border immediately. Status bit 5 is
    // a separate latch and must remain unchanged until C0=R0 is sampled.
    test.write_selected_register_at_nclken(1);
    test.expect_byte("type 1 status before the C0=R0 sample", 0x00,
                     test.read_status());

    test.run_characters(4);
    test.expect_byte("type 1 status one character before the C0=R0 sample",
                     0x00, test.read_status());

    test.run_characters(1);
    test.expect_byte("type 1 status after the C0=R0 sample", 0x20,
                     test.read_status());
}

void test_type1_status_samples_natural_r6_edge(TestBench& test) {
    test.set_crtc_type(1);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 2}, {7, 2}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();

    test.run_characters(32);  // Complete the initial frame and clear status.
    test.expect_byte("type 1 status at the start of a displayed frame", 0x00,
                     test.read_status());

    test.run_characters(15);
    test.expect_byte("type 1 status before entering the R6 row", 0x00,
                     test.read_status());

    test.run_characters(1);
    test.expect_byte("type 1 status when the R6 row is sampled", 0x20,
                     test.read_status());

    test.run_characters(15);
    test.expect_byte("type 1 high status before frame origin", 0x20,
                     test.read_status());
    test.run_characters(1);
    test.expect_byte("type 1 high status clears at frame origin", 0x00,
                     test.read_status());

    test.run_characters(15);
    test.expect_byte("type 1 status before the next R6 row", 0x00,
                     test.read_status());
    test.run_characters(1);
    test.expect_byte("type 1 status set again before reset", 0x20,
                     test.read_status());
    test.reset();
    test.expect_byte("type 1 high status clears on reset", 0x00,
                     test.read_status());

    test.set_crtc_type(0);
    test.expect_byte("type 0 status remains unchanged", 0xff,
                     test.read_status());
}

void test_type1_status_clears_on_type_round_trip(TestBench& test) {
    test.set_crtc_type(1);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 2}, {7, 2}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();

    test.run_characters(48);
    test.expect_byte("type 1 status before type round-trip", 0x20,
                     test.read_status());

    test.set_crtc_type(0);
    test.run_characters(1);
    test.expect_byte("type 0 status during type round-trip", 0xff,
                     test.read_status());

    test.set_crtc_type(1);
    test.expect_byte("type 1 status after type round-trip", 0x00,
                     test.read_status());
}

void configure_f5_r0_zero_fixture(TestBench& test,
                                  unsigned type,
                                  std::uint8_t horizontal_sync_position) {
    test.set_crtc_type(type);

    // R9 is deliberately nonzero: type 0 must freeze C9/RA at zero while
    // R0=0, whereas type 1 must continue through one-character lines.  R2 is
    // varied independently to exercise the pin-level HSYNC consequence.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 0}, {1, 0}, {2, horizontal_sync_position}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 3}, {7, 2},                       {8, 0},    {9, 7},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(0);
    test.reset();
    test.expect_reset_outputs();
}

void test_type0_r0_zero_suppresses_nonzero_r2_hsync(TestBench& test) {
    constexpr unsigned horizontal_sync_position = 4;
    constexpr unsigned observation_characters = 260;
    constexpr unsigned former_free_running_r2_tick =
        (horizontal_sync_position - 1) * kClockTicksPerCharacter + 1;

    configure_f5_r0_zero_fixture(test, 0, horizontal_sync_position);
    test.expect_ra("type 0 R0=0 initial raster counter", 0);

    // ACCC v1.10 section 13.2.1: C0 remains pinned at zero, so it can never
    // reach a nonzero R2.  Sample every raw CLOCK tick, not merely character
    // boundaries, so a sub-character HSYNC pulse cannot escape the vector.
    for (unsigned tick = 0;
         tick < observation_characters * kClockTicksPerCharacter;
         ++tick) {
        test.run_clock_ticks(1);
        if ((tick + 1) % kClockTicksPerCharacter == 0) {
            test.expect_ra("type 0 R0=0 keeps C9/RA frozen", 0);
        }
        if (tick == former_free_running_r2_tick) {
            test.expect_hsync_low(
                "type 0 R0=0 suppresses the former free-running R2 edge");
        } else {
            test.expect_hsync_low("type 0 R0=0 cannot reach nonzero R2");
        }
    }
}

void test_type0_r0_zero_allows_r2_zero_hsync(TestBench& test) {
    configure_f5_r0_zero_fixture(test, 0, 0);

    // ACCC v1.10 section 15.3: type 0 does not restart HSYNC on the second
    // C0=R2=0 occurrence.  Its stopped C3l then permits a restart on the
    // third occurrence.  Sample every raw CLOCK tick so the brief low window
    // cannot be hidden by character-boundary observations.
    for (unsigned tick = 0; tick < kClockTicksPerCharacter; ++tick) {
        test.run_clock_ticks(1);
        test.expect_hsync_high(
            "type 0 first R2=0 occurrence keeps HSYNC high");
        test.expect_ra("type 0 R2=0 first occurrence keeps C9/RA frozen", 0);
    }

    test.run_clock_ticks(1);
    test.expect_hsync_high(
        "type 0 second R2=0 occurrence does not restart HSYNC");
    test.expect_ra("type 0 R2=0 second occurrence keeps C9/RA frozen", 0);
    for (unsigned tick = 1; tick < kClockTicksPerCharacter; ++tick) {
        test.run_clock_ticks(1);
        test.expect_hsync_low(
            "type 0 HSYNC stays off after the second R2=0 occurrence");
        test.expect_ra("type 0 R2=0 second occurrence keeps C9/RA frozen", 0);
    }

    test.run_clock_ticks(1);
    test.expect_hsync_low(
        "type 0 third R2=0 occurrence first clears stopped C3l");
    test.expect_ra("type 0 R2=0 third occurrence keeps C9/RA frozen", 0);
    for (unsigned tick = 1; tick < kClockTicksPerCharacter; ++tick) {
        test.run_clock_ticks(1);
        if (tick == 1) {
            test.expect_hsync_high(
                "type 0 third R2=0 occurrence restarts HSYNC");
        } else {
            test.expect_hsync_high(
                "type 0 third R2=0 occurrence keeps HSYNC high");
        }
        test.expect_ra("type 0 R2=0 third occurrence keeps C9/RA frozen", 0);
    }
}

void test_type0_r0_zero_resumes_after_nclken_write(TestBench& test) {
    constexpr unsigned horizontal_sync_position = 2;
    configure_f5_r0_zero_fixture(test, 0, horizontal_sync_position);

    // Prove the recovered, in-range R2 has not been reached while R0=0.  The
    // first released CLKEN is included; HSYNC must remain low through nCLKEN.
    for (unsigned tick = 0; tick < kNClkEnPhase; ++tick) {
        test.expect_hsync_low("type 0 R0=0 keeps recovered R2 out of range");
        test.run_clock_ticks(1);
        test.expect_ra("type 0 raster counter during R0=0 stall", 0);
    }

    // nCLKEN is tick 8, opposite CLKEN at tick 0.  Landing R0=3 there makes
    // the new total stable before the next character edge.
    test.write_selected_register_at_nclken(3);
    test.expect_ra("type 0 raster counter at R0 recovery write", 0);
    test.expect_hsync_low("type 0 HSYNC remains low at the recovery write");

    // The first post-write CLKEN resumes frozen C0 as 1, not 2.
    test.run_clock_ticks(kClockTicksPerCharacter - kNClkEnPhase - 1);
    test.expect_hsync_low("type 0 recovery remains below R2 before CLKEN");
    test.run_clock_ticks(1);
    test.expect_hsync_low("type 0 recovery advances C0 from zero to one");
    test.run_clock_ticks(1);
    test.expect_hsync_low(
        "type 0 recovery does not assert HSYNC before C0=R2");

    // On the second post-write CLKEN C0 reaches R2=2.  HSYNC registers on
    // the following raw CLOCK edge.
    test.run_clock_ticks(kClockTicksPerCharacter - 2);
    test.expect_hsync_low("type 0 recovery remains below R2 until CLKEN");
    test.run_clock_ticks(1);
    test.expect_hsync_low("type 0 recovery reaches R2 before HSYNC registers");
    test.run_clock_ticks(1);
    test.expect_hsync_high("type 0 recovery asserts HSYNC exactly at C0=R2");

    // From C0=R2, two further CLKENs reach C0=R0 and then wrap to zero.
    test.run_clock_ticks(2 * kClockTicksPerCharacter - 2);
    test.expect_ra("type 0 recovery before the first widened line end", 0);
    test.run_clock_ticks(1);
    test.expect_ra("type 0 recovery advances C9/RA after one R0=3 line", 1);
}

void test_type1_r0_zero_keeps_one_character_lines(TestBench& test) {
    configure_f5_r0_zero_fixture(test, 1, 4);

    // Type 1 has no R0=0 freeze: tick 0 completes each one-character line,
    // while C0=0 still cannot reach the nonzero R2 value.  Sample every raw
    // CLOCK tick so a sub-character HSYNC pulse cannot escape the guard.
    for (std::uint8_t raster = 1; raster <= 4; ++raster) {
        for (unsigned tick = 0; tick < kClockTicksPerCharacter; ++tick) {
            test.run_clock_ticks(1);
            test.expect_hsync_low("type 1 R0=0 cannot reach nonzero R2");
            if (tick == kClkEnPhase) {
                test.expect_ra(
                    "type 1 R0=0 advances C9/RA at each CLKEN", raster);
            }
        }
    }

    // The helper reaches nCLKEN by first executing the documented tick-0
    // CLKEN, completing one last R0=0 line (RA 4->5).  R0=3 is then stable
    // before subsequent CLKENs and produces a four-character line.
    test.write_selected_register_at_nclken(3);
    test.expect_ra("type 1 final one-character line before R0 widening", 5);
    test.run_characters(3);
    test.expect_ra("type 1 widened line before C0 reaches R0", 5);
    test.run_characters(1);
    test.expect_ra("type 1 widened line advances C9/RA at C0=R0", 6);
}

void test_type0_midline_r0_zero_free_runs_then_pins(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 0}, {2, 4}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 3}, {7, 2}, {8, 0},    {9, 7},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(0);
    test.reset();
    test.expect_reset_outputs();

    // Two complete characters leave C0=2.  The helper executes the next
    // CLKEN before reaching nCLKEN, so R0 becomes zero live at C0=3.
    test.run_characters(2);
    test.write_selected_register_at_nclken(0);
    const std::uint16_t ma_at_write = test.ma();
    const auto advanced_ma = [ma_at_write](unsigned characters) {
        return static_cast<std::uint16_t>(
            (ma_at_write + characters) & 0x3fff);
    };
    test.expect_ra("type 0 mid-line R0=0 write does not create a line", 0);

    // The live comparator cannot match zero while C0 is nonzero.  C0 must
    // continue to increment rather than clamp immediately; reaching R2=4 and
    // advancing MA on the first following CLKEN makes that externally visible.
    test.run_clock_ticks(kClockTicksPerCharacter - kNClkEnPhase - 1);
    test.expect_ma("type 0 MA holds until the first post-write CLKEN",
                   ma_at_write);
    test.run_clock_ticks(1);
    test.expect_ma("type 0 mid-line R0=0 continues from C0=3 to C0=4",
                   advanced_ma(1));
    test.expect_ra("type 0 free-run does not create a line at C0=4", 0);
    test.run_clock_ticks(1);
    test.expect_hsync_high("type 0 free-run still reaches the live R2=4");

    // From C0=4, another 251 character clocks reach C0=255, and the next
    // wraps the eight-bit counter to zero.  None is a true C0=R0 line end.
    test.run_characters(251);
    test.expect_ma("type 0 free-run advances MA through C0=255",
                   advanced_ma(252));
    test.expect_ra("type 0 free-run through C0=255 creates no false line", 0);
    test.run_characters(1);
    test.expect_ma("type 0 C0 overflow advances MA once before pinning",
                   advanced_ma(253));
    test.expect_ra("type 0 C0 overflow creates no false line", 0);

    // Once the overflow has produced C0=0, the repeated C0=R0 equality pins
    // C0 and must not reload/increment the visible memory address.
    test.run_characters(3);
    test.expect_ma("type 0 R0=0 pins MA after the eight-bit wrap",
                   advanced_ma(253));
    test.expect_ra("type 0 R0=0 pins C9 after the eight-bit wrap", 0);

    // Widen R0 at nCLKEN.  C0 resumes from zero, MA advances for C0=1..3,
    // and the first genuine C0=R0 boundary advances C9 exactly once.
    test.write_selected_register_at_nclken(3);
    test.expect_ma("type 0 MA remains pinned at the recovery write",
                   advanced_ma(253));
    test.run_clock_ticks(kClockTicksPerCharacter - kNClkEnPhase - 1);
    test.expect_ma("type 0 recovered MA waits for CLKEN", advanced_ma(253));
    test.run_clock_ticks(1);
    test.expect_ma("type 0 recovery resumes MA from frozen C0=0",
                   advanced_ma(254));
    test.run_characters(2);
    test.expect_ma("type 0 recovery advances MA through C0=3",
                   advanced_ma(256));
    test.expect_ra("type 0 recovery has not ended the widened line early", 0);
    test.run_characters(1);
    test.expect_ra("type 0 recovery ends one complete R0=3 line", 1);
}

void test_r0_zero_freeze_survives_type_round_trip(TestBench& test) {
    configure_f5_r0_zero_fixture(test, 0, 4);

    test.run_characters(2);
    test.expect_ra("type 0 begins the type round-trip frozen", 0);

    // CRTC_TYPE is a live input.  Type 1 treats R0=0 as one-character lines,
    // then returning to type 0 at C0=0 must immediately restore the freeze.
    test.set_crtc_type(1);
    test.run_characters(2);
    test.expect_ra("type 1 advances two R0=0 lines during round-trip", 2);
    test.set_crtc_type(0);
    test.run_characters(3);
    test.expect_ra("type 0 re-pins C9 after the type round-trip", 2);

    test.write_selected_register_at_nclken(3);
    test.run_clock_ticks(kClockTicksPerCharacter - kNClkEnPhase - 1);
    test.run_clock_ticks(1);
    test.expect_ra("round-trip recovery starts from the frozen raster", 2);
    test.run_characters(3);
    test.expect_ra("round-trip recovery completes one widened line", 3);
}

void test_type0_interlace_r0_zero_freezes_vsync_count(TestBench& test) {
    test.set_crtc_type(0);
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 15}, {1, 8}, {2, 10}, {3, 0x11}, {4, 0},
        {5, 0},  {6, 1}, {7, 1},  {8, 3},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(7);
    test.reset();

    // Start a one-count VSYNC in field=0 at C0=3.  Its partial-line guard is
    // consumed by the genuine C0=15->0 line tick, which also enters field=1.
    // Thus C3h=0 while VSYNC remains active at C0=0 in the half-line field.
    write_r7_zero_at_hcc(test, 3);
    test.expect_vsync_high("interlaced VSYNC starts before the R0=0 freeze");
    test.run_characters(12);
    test.expect_vsync_high("interlaced VSYNC reaches the next line active");
    test.expect_field_low("interlaced VSYNC remains in the half-line field");

    // Land R0=0 at nCLKEN while C0 is already zero.  A frozen type-0 C0 must
    // not masquerade as the field=1 half-line predicate on every CLKEN.
    test.select_register(0);
    test.write_selected_register_at_nclken(0);
    test.run_characters(3);
    test.expect_vsync_high(
        "type 0 interlaced R0=0 freeze does not consume C3h character-wise");
    test.expect_ra("type 0 interlaced R0=0 keeps the raster frozen", 1);

    // Widening R0 restores the real half-line predicate.  With R0=7 it lands
    // at C0=2->3: no earlier character may end VSYNC, and that count ends it.
    test.write_selected_register_at_nclken(7);
    test.run_characters(2);
    test.expect_vsync_high("recovered interlaced VSYNC waits for R0/2");
    test.run_characters(1);
    test.expect_vsync_low("recovered interlaced VSYNC ends at R0/2");
}

void test_type0_r0_zero_c9_equal_single_c4_increment_deferred(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC 13.2.1's narrow deferred subcase: when C9 already equals R9 as
    // type 0 enters R0=0, the previously armed decision increments C4 once
    // on the second C0=0 occurrence, then freezes.  The first normal R0=3
    // line below reaches C4=1/C9=0; R0 is then changed at that exact C0=0.
    // R7=2 makes the deferred increment observable after recovery: a correct
    // frozen C4=2 must not start VSYNC on the first widened line, whereas the
    // current all-state freeze leaves C4=1 and produces a false 1->2 match.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 0}, {2, 2}, {3, 0x11}, {4, 3},
        {5, 0}, {6, 3}, {7, 2}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(0);
    test.reset();

    // Stop immediately after the C0=3->0 boundary rather than running the
    // remainder of the character.  The raw phase-1 bus write changes R0 at
    // C0=0 without introducing another CLKEN.
    test.run_characters(3);
    test.run_clock_ticks(1);
    test.write_selected_register_now(0);
    test.run_characters(2);
    test.expect_vsync_low("deferred C9=R9 subcase stays quiet while frozen");
    test.write_selected_register_at_nclken(3);
    test.run_clock_ticks(kClockTicksPerCharacter - kNClkEnPhase - 1);
    test.run_clock_ticks(1);
    test.run_characters(3);
    test.run_clock_ticks(1);
    test.expect_vsync_low(
        "deferred C9=R9 entry increments C4 once before recovery");
}

void test_type0_adjustment_r4_write_switches_c9_to_r5(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 11.2.2: once the last-line state is established,
    // changing R4 from C0=2 through C0=R0 switches the line-end comparator
    // from C9/R9 to C9/R5.  With C4=R4=2, C9=R9=3, and R5=5, the next line
    // must therefore be C4=2/C9=4 in vertical adjustment, not C4=3/C9=0.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    constexpr unsigned line_characters = 8;
    constexpr unsigned lines_before_last = 11;
    test.run_characters(lines_before_last * line_characters);
    test.run_characters(2);  // Last-line state is now established; C0=2.
    test.write_selected_register_at_clken(1);
    test.run_characters(5);  // Complete C0=3..7 and enter the next line.

    test.expect_adjustment_active("type 0 R4 write enters vertical adjustment");
    test.expect_c4("type 0 R4 write keeps C4 on the entry line", 2);
    test.expect_ra("type 0 R4 write compares C9 with R5", 4);
}

void test_type0_adjustment_r9_write_uses_new_r9(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 11.2.2: an R9 write from C0=2 through
    // C0=R0-1 is compared with C9 on the current last-frame line. Here the
    // new R9=1 differs from C9=0, so C9 increments while C4 stays at zero.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 4}, {1, 3}, {2, 3}, {3, 0x11}, {4, 0},
        {5, 2}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    test.run_characters(2);
    test.write_selected_register_at_clken(1);
    test.run_characters(2);

    test.expect_adjustment_active("type 0 R9 write retains vertical adjustment");
    test.expect_c4("type 0 R9 write does not increment C4", 0);
    test.expect_ra("type 0 R9 write increments C9 against the new R9", 1);
}

void test_type0_adjustment_accepts_r5_write_at_c0_2(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 10.3.1 and 11.2.2: C0=2 is the final
    // arbitration point. An R5 write from zero to one there must select one
    // adjustment line, reset C9, and increment C4 on the following line.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(5);
    test.reset();

    test.run_characters(2);
    test.write_selected_register_at_clken(1);
    test.run_characters(1);

    test.expect_adjustment_active("type 0 accepts R5>0 at C0=2");
    test.expect_c4("type 0 accepted R5 write increments C4", 1);
    test.expect_ra("type 0 accepted R5 write resets C9", 0);
}

void test_type0_adjustment_rejects_r5_write_after_c0_2(TestBench& test) {
    test.set_crtc_type(0);

    // The same v1.10 arbitration window closes once C0 has reached 3.
    // A later R5 write must not turn the already-decided frame boundary into
    // an adjustment line.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 4}, {1, 3}, {2, 3}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(5);
    test.reset();

    test.run_characters(3);
    test.write_selected_register_at_clken(1);
    test.run_characters(1);

    test.expect_adjustment_inactive("type 0 rejects R5>0 after C0=2");
    test.expect_c4("type 0 late R5 write completes the frame", 0);
    test.expect_ra("type 0 late R5 write completes C9", 0);
}

void test_type0_adjustment_r9_write_at_r0_increments_c4_and_c9(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 11.2.2 exact-boundary case: the old C9/R9
    // equality increments C4 first. C4 then differs from R4, switching C9
    // to the R5 comparison, so C9 increments as well.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    constexpr unsigned line_characters = 8;
    constexpr unsigned lines_before_last = 11;
    test.run_characters(lines_before_last * line_characters);
    test.run_characters(7);
    test.write_selected_register_at_clken(4);

    test.expect_adjustment_active("type 0 exact-R0 R9 write enters adjustment");
    test.expect_c4("type 0 exact-R0 R9 write increments C4", 3);
    test.expect_ra("type 0 exact-R0 R9 write increments C9 against R5", 4);
}

void test_type0_adjustment_completion_resets_the_next_line(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(7);
    test.write_selected_register_at_clken(4);
    test.run_characters(8);

    // The exact-R0 split leaves C9=R5-1. The following line completes the
    // R5 count, re-establishes Last Line, and resets C4/C9 for the new frame.
    test.expect_adjustment_inactive("type 0 R5 completion leaves adjustment");
    test.expect_c4("type 0 R5 completion resets C4 on the following line", 0);
    test.expect_ra("type 0 R5 completion resets C9 on the following line", 0);
}

void test_type0_adjustment_captures_mid_character_r4_write(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(1);  // C0=1 at tick 0.
    test.write_selected_register_at_nclken(1);  // R4 write within C0=2.
    test.run_characters(6);

    test.expect_adjustment_active("type 0 captures an R4 write within C0=2");
    test.expect_c4("type 0 mid-character R4 write keeps C4", 2);
    test.expect_ra("type 0 mid-character R4 write compares C9 with R5", 4);
}

void test_type0_adjustment_captures_mid_character_r9_write_at_r0(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(6);  // C0=6 at tick 0.
    test.write_selected_register_at_nclken(4);  // R9 write within C0=R0.
    test.run_characters(1);

    test.expect_adjustment_active("type 0 captures an exact-R0 R9 bus write");
    test.expect_c4("type 0 mid-character exact-R0 R9 write increments C4", 3);
    test.expect_ra("type 0 mid-character exact-R0 R9 write increments C9", 4);
}

void test_type0_adjustment_r4_write_at_r0_switches_c9_to_r5(TestBench& test) {
    test.set_crtc_type(0);

    // R4's documented window includes C0=R0. A same-edge write must affect
    // the imminent rollover, even though the register file itself uses NBA
    // semantics and still presents the previous R4 to the counter block.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(7);
    test.write_selected_register_at_clken(1);

    test.expect_adjustment_active("type 0 accepts an R4 write at C0=R0");
    test.expect_c4("type 0 exact-R0 R4 write keeps C4", 2);
    test.expect_ra("type 0 exact-R0 R4 write compares C9 with R5", 4);
}

void test_type0_adjustment_captures_mid_character_r4_write_at_r0(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(6);
    test.write_selected_register_at_nclken(1);
    test.run_characters(1);

    test.expect_adjustment_active("type 0 captures an R4 bus write within C0=R0");
    test.expect_c4("type 0 mid-character exact-R0 R4 write keeps C4", 2);
    test.expect_ra("type 0 mid-character exact-R0 R4 write uses R5", 4);
}

void test_type0_adjustment_latches_clear_on_snapshot_load(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(2);
    test.write_selected_register_at_nclken(1);
    test.expect_type0_arbitration_latches(
        "type 0 R4 write arms only the R4 comparator latch", true, false, false);

    const std::array<std::uint8_t, 10> snapshot_registers = {{
        7, 4, 5, 0x11, 2, 5, 2, 1, 0, 3,
    }};
    test.load_snapshot_registers(snapshot_registers);
    test.expect_type0_arbitration_latches(
        "snapshot load clears type 0 arbitration state", false, false, false);
}

void test_type0_adjustment_latches_clear_on_type_roundtrip(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 7}, {1, 4}, {2, 5}, {3, 0x11}, {4, 2},
        {5, 5}, {6, 2}, {7, 1}, {8, 0},    {9, 3},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    test.run_characters(11 * 8);
    test.run_characters(2);
    test.write_selected_register_at_nclken(4);
    test.expect_type0_arbitration_latches(
        "type 0 mid-line R9 write arms the live comparator latch", false, true, false);

    test.set_crtc_type(1);
    test.run_clock_ticks(1);
    test.expect_type0_arbitration_latches(
        "type 1 clears type 0 live-comparator state", false, false, false);

    test.set_crtc_type(0);
    test.write_register(9, 3);
    test.select_register(9);
    test.reset();
    test.run_characters(11 * 8);
    test.run_characters(6);
    test.write_selected_register_at_nclken(4);
    test.expect_type0_arbitration_latches(
        "type 0 exact-R0 R9 write arms the boundary latch", false, false, true);

    test.set_crtc_type(1);
    test.run_clock_ticks(1);
    test.set_crtc_type(0);
    test.expect_type0_arbitration_latches(
        "live type round-trip clears exact-R0 state", false, false, false);
}

void test_type0_r4_write_at_c0_1_enters_zero_adjustment(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 10.3.1, 11.2.2, and 12.2: if Last Line was
    // established at C0=0, an R4 write at C0=1 that breaks C4==R4 makes the
    // current line the first adjustment line even when R5=0. The changed R4
    // keeps C4 at zero, while C9 advances from zero instead of being reset.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(1);
    test.write_selected_register_at_clken(1);
    test.expect_type0_arbitration_latches(
        "type 0 C0=1 R4 break arms only zero-adjustment entry", false, false,
        false, true);
    test.run_characters(2);

    test.expect_adjustment_active("type 0 C0=1 R4 break enters adjustment with R5=0");
    test.expect_c4("type 0 C0=1 R4 break keeps C4", 0);
    test.expect_ra("type 0 C0=1 R4 break increments C9", 1);
    test.expect_type0_arbitration_latches(
        "type 0 C0=1 entry latch clears at the line boundary", false, false,
        false);
}

void test_type0_r9_write_within_c0_1_enters_zero_adjustment(TestBench& test) {
    test.set_crtc_type(0);

    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(9);
    test.reset();

    test.write_selected_register_at_nclken(1);
    test.run_characters(3);

    test.expect_adjustment_active("type 0 C0=1 R9 break enters adjustment with R5=0");
    test.expect_c4("type 0 C0=1 R9 break keeps C4", 0);
    test.expect_ra("type 0 C0=1 R9 break increments C9", 1);
}

void test_type0_r4_write_at_c0_0_overrides_last_line(TestBench& test) {
    test.set_crtc_type(0);

    // At C0=0 the register write participates in the Last Line comparison
    // itself. Breaking C4==R4 here must select ordinary counting, not the
    // special C0=1 zero-adjustment route and not an immutable frame reset.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.write_selected_register_at_clken(1);
    test.run_characters(3);

    test.expect_adjustment_inactive("type 0 C0=0 R4 break does not enter adjustment");
    test.expect_c4("type 0 C0=0 R4 break selects ordinary C4 increment", 1);
    test.expect_ra("type 0 C0=0 R4 break resets matching C9", 0);
}

void test_type0_r0_one_runs_default_zero_adjustment_line(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 11.2.2 and 13.2.1/13.2.5: R0=1 never reaches
    // C0=2, so it cannot cancel the adjustment armed by C4==R4/C9==R9.
    // With R4=R9=R5=0, one two-character adjustment line at C4=1/C9=0
    // follows the frame line, then the next boundary resets both counters.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 1}, {1, 1}, {2, 1}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.reset();

    test.run_characters(2);
    test.expect_adjustment_active("type 0 R0=1 enters default adjustment");
    test.expect_c4("type 0 R0=1 increments C4 once for adjustment", 1);
    test.expect_ra("type 0 R0=1 keeps C9 zero for R5=0 adjustment", 0);

    test.run_characters(2);
    test.expect_adjustment_inactive("type 0 R0=1 zero adjustment lasts one line");
    test.expect_c4("type 0 R0=1 completion resets C4", 0);
    test.expect_ra("type 0 R0=1 completion resets C9", 0);
}

void test_type0_r0_zero_starts_default_zero_adjustment(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 11.2.2 and 13.2.1/13.2.6: with R0 already zero,
    // the repeated C0=0 equality freezes C9 but consumes the C4 increment
    // once on the second C0=0. Because C4=R4 and C9=R9 established Last
    // Line, the short line also takes the default adjustment route even
    // though R5=0.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 0}, {1, 0}, {2, 0}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(0);
    test.reset();

    test.run_characters(1);
    test.expect_adjustment_active("type 0 R0=0 enters default adjustment");
    test.expect_c4("type 0 R0=0 consumes one C4 increment", 1);
    test.expect_ra("type 0 R0=0 freezes C9", 0);
    test.expect_type0_arbitration_latches(
        "type 0 R0=0 records that its entry increment was consumed", false,
        false, false, false, true);

    test.run_characters(3);
    test.expect_c4("type 0 R0=0 does not repeat the C4 increment", 1);
    test.expect_ra("type 0 R0=0 keeps C9 frozen", 0);

    test.write_selected_register_at_nclken(3);
    test.run_characters(4);
    test.expect_adjustment_inactive("type 0 R0=0 recovery completes adjustment");
    test.expect_c4("type 0 R0=0 recovery resets C4", 0);
    test.expect_ra("type 0 R0=0 recovery resets C9", 0);
    test.expect_type0_arbitration_latches(
        "type 0 R0=0 entry guard rearms after recovery", false, false, false);
}

void test_type0_r0_zero_during_adjustment_freezes_c4(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 13.2.3: once adjustment is active, changing R0 to
    // zero at C0=0 freezes the existing C4/C9 values. The pre-adjustment
    // deferred C4 increment must not fire again merely because C9 equals R9.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 3}, {1, 1}, {2, 1}, {3, 0x11}, {4, 0},
        {5, 3}, {6, 1}, {7, 1}, {8, 0},    {9, 1},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(0);
    test.reset();

    test.run_characters(11);
    test.expect_adjustment_active("type 0 reaches active adjustment before R0=0");
    test.expect_c4("type 0 adjustment has incremented C4 once", 1);
    test.expect_ra("type 0 adjustment is one character before C9=R9", 0);

    // The helper crosses the imminent CLKEN boundary, then writes at nCLKEN
    // while the resulting C0=0/C9=R9 character is active.
    test.write_selected_register_at_nclken(0);
    test.expect_ra("type 0 R0=0 write lands with C9=R9", 1);
    test.run_characters(1);
    test.expect_adjustment_active("type 0 R0=0 preserves active adjustment");
    test.expect_c4("type 0 R0=0 does not increment C4 again in adjustment", 1);
    test.expect_ra("type 0 R0=0 freezes adjustment C9", 1);
}

void test_type0_r0_one_c0_1_break_is_consumed_at_rollover(TestBench& test) {
    test.set_crtc_type(0);

    // With R0=1, C0=1 is also the line rollover. The equality-breaking R4
    // write must affect that rollover directly and be consumed there; retaining
    // the entry latch would force an extra adjustment line.
    const std::array<std::pair<std::uint8_t, std::uint8_t>, 10> registers = {{
        {0, 1}, {1, 1}, {2, 1}, {3, 0x11}, {4, 0},
        {5, 0}, {6, 1}, {7, 1}, {8, 0},    {9, 0},
    }};
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
    test.select_register(4);
    test.reset();

    test.run_characters(1);
    test.write_selected_register_at_clken(1);
    test.expect_adjustment_active("type 0 R0=1 C0=1 break enters adjustment");
    test.expect_c4("type 0 R0=1 C0=1 break keeps C4", 0);
    test.expect_ra("type 0 R0=1 C0=1 break advances C9", 1);
    test.expect_type0_arbitration_latches(
        "type 0 R0=1 consumes the C0=1 entry latch at rollover", false, false,
        false);

    test.run_characters(2);
    test.expect_adjustment_inactive("type 0 R0=1 C0=1 adjustment completes next line");
    test.expect_c4("type 0 R0=1 C0=1 completion resets C4", 0);
    test.expect_ra("type 0 R0=1 C0=1 completion resets C9", 0);
}

// ---------------------------------------------------------------------------
// t07 / t08: F4 equality-only counter overflow and the section 28.1.1 CRTC
// identification boundaries.  Test-only checkpoint: these vectors encode the
// v1.10 rules, and the cases the current comparator shortcuts cannot satisfy
// are registered as named known divergences rather than weakened.
// ---------------------------------------------------------------------------

using RegisterProgram = std::array<std::pair<std::uint8_t, std::uint8_t>, 10>;

void program_registers(TestBench& test, const RegisterProgram& registers) {
    for (const auto& [address, value] : registers) {
        test.write_register(address, value);
    }
}

// R9=7 frame with a long C4 limit, so a lowered R9 exercises C9 alone.
constexpr RegisterProgram kC9OverflowRegisters = {{
    {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 20},
    {5, 0}, {6, 20}, {7, 100}, {8, 0}, {9, 7},
}};

// R9=0 makes every character line a row, so a lowered R4 exercises C4 alone.
constexpr RegisterProgram kC4OverflowRegisters = {{
    {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 5},
    {5, 0}, {6, 40}, {7, 100}, {8, 0}, {9, 0},
}};

constexpr unsigned kOverflowLineCharacters = 4;

// Reach C9=4 (C9-overflow fixture) or C4=4 (C4-overflow fixture) with C0 back
// at 0, ready for the register write that lowers the limit below the counter.
void run_to_fourth_line_start(TestBench& test) {
    test.run_characters(4 * kOverflowLineCharacters);
}

void run_lines(TestBench& test, unsigned lines) {
    test.run_characters(lines * kOverflowLineCharacters);
}

void test_type1_c9_counts_through_31_and_wraps(TestBench& test) {
    test.set_crtc_type(1);

    // ACCC v1.10 sections 10.3 and 10.3.2: C9 uses equality, never
    // magnitude.  R9 lowered below C9 leaves C9 counting up to 31, wrapping,
    // and only then meeting the new limit.  C4 must not move while C9 wraps.
    program_registers(test, kC9OverflowRegisters);
    test.select_register(9);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_ra("type 1 C9 overflow fixture reaches C9=4", 4);
    test.expect_c4("type 1 C9 overflow fixture keeps C4 at zero", 0);

    test.write_selected_register_at_clken(2);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_ra("type 1 R9 below C9 keeps counting C9", 5);
    test.expect_c4("type 1 R9 below C9 does not increment C4", 0);

    run_lines(test, 26);
    test.expect_ra("type 1 C9 reaches its 5-bit limit", 31);
    test.expect_c4("type 1 C9 overflow still does not increment C4", 0);

    run_lines(test, 1);
    test.expect_ra("type 1 C9 wraps past 31", 0);
    test.expect_c4("type 1 C9 wrap is not a C9=R9 match", 0);

    // 0 -> 1 -> 2 meets the new R9, so the third line resets C9 and moves C4.
    run_lines(test, 3);
    test.expect_ra("type 1 wrapped C9 meets the new R9", 0);
    test.expect_c4("type 1 wrapped C9 match increments C4", 1);
}

void test_type1_c9_zero_limit_overflows(TestBench& test) {
    test.set_crtc_type(1);

    // ACCC v1.10 section 10.3: "If R9 is rewritten to 0 while C9>0, C9 does
    // not snap to 0 - it overflows up to 31 then wraps."  Zero is an ordinary
    // limit value, so the same overflow applies as for the nonzero case above.
    program_registers(test, kC9OverflowRegisters);
    test.select_register(9);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_ra("type 1 zero-limit fixture reaches C9=4", 4);
    test.expect_c4("type 1 zero-limit fixture keeps C4 at zero", 0);

    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_known_ra("type 1 R9=0 below C9 keeps counting C9", 5);
    test.expect_known_c4("type 1 R9=0 below C9 does not increment C4", 0);

    run_lines(test, 26);
    test.expect_known_ra("type 1 R9=0 lets C9 reach its 5-bit limit", 31);

    run_lines(test, 1);
    test.expect_known_ra("type 1 R9=0 wraps C9 onto the new limit", 0);
    test.expect_known_c4("type 1 R9=0 wrap does not yet increment C4", 0);

    run_lines(test, 1);
    test.expect_known_ra("type 1 wrapped C9 matches R9=0", 0);
    test.expect_known_c4("type 1 wrapped C9 match increments C4 once", 1);
}

void test_type1_c4_counts_through_127_and_wraps(TestBench& test) {
    test.set_crtc_type(1);

    // ACCC v1.10 sections 12 and 12.3: outside vertical adjustment C4 uses
    // equality too.  R4 lowered below C4 leaves C4 counting to its 7-bit
    // limit, wrapping, and only then meeting the new limit.
    program_registers(test, kC4OverflowRegisters);
    test.select_register(4);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_c4("type 1 C4 overflow fixture reaches C4=4", 4);

    test.write_selected_register_at_clken(1);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_c4("type 1 R4 below C4 keeps counting C4", 5);

    run_lines(test, 122);
    test.expect_c4("type 1 C4 reaches its 7-bit limit", 127);

    run_lines(test, 1);
    test.expect_c4("type 1 C4 wraps past 127", 0);

    run_lines(test, 1);
    test.expect_c4("type 1 wrapped C4 is not yet the new limit", 1);

    run_lines(test, 1);
    test.expect_c4("type 1 wrapped C4 meets the new R4", 0);
}

void test_type1_c4_zero_limit_overflows(TestBench& test) {
    test.set_crtc_type(1);

    // ACCC v1.10 section 12.3: on type 1, R4=0 is an ordinary value with no
    // last-line latch, so writing it while C4>0 overflows C4 rather than
    // ending the frame.
    program_registers(test, kC4OverflowRegisters);
    test.select_register(4);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_c4("type 1 zero-limit C4 fixture reaches C4=4", 4);

    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_c4("type 1 R4=0 below C4 keeps counting C4", 5);

    run_lines(test, 122);
    test.expect_c4("type 1 R4=0 lets C4 reach its 7-bit limit", 127);

    run_lines(test, 1);
    test.expect_c4("type 1 R4=0 wraps C4 onto the new limit", 0);

    run_lines(test, 1);
    test.expect_c4("type 1 R4=0 holds C4 at the reached limit", 0);
}

void test_type0_c9_counts_through_31_and_wraps(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 sections 10.3.1 and 12.2: type 0 establishes the last-line
    // state while C0<2, so the R9 write lands at C0=0 and participates in that
    // comparison.  The new limit is below C9 and not a last line, so the live
    // general case applies: C9 counts through 31 and wraps.
    program_registers(test, kC9OverflowRegisters);
    test.select_register(9);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_ra("type 0 C9 overflow fixture reaches C9=4", 4);
    test.expect_c4("type 0 C9 overflow fixture keeps C4 at zero", 0);

    test.write_selected_register_at_clken(2);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_ra("type 0 R9 below C9 keeps counting C9", 5);
    test.expect_c4("type 0 R9 below C9 does not increment C4", 0);

    run_lines(test, 26);
    test.expect_ra("type 0 C9 reaches its 5-bit limit", 31);
    test.expect_c4("type 0 C9 overflow still does not increment C4", 0);

    run_lines(test, 1);
    test.expect_ra("type 0 C9 wraps past 31", 0);
    test.expect_c4("type 0 C9 wrap is not a C9=R9 match", 0);

    run_lines(test, 3);
    test.expect_ra("type 0 wrapped C9 meets the new R9", 0);
    test.expect_c4("type 0 wrapped C9 match increments C4", 1);
}

void test_type0_c9_zero_limit_overflows(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 10.3: R9=0 written while C9>0 must overflow exactly
    // like any other lowered limit.  C4=0 is far from R4=20, so no last-line
    // exception can apply and no adjustment route is open.
    program_registers(test, kC9OverflowRegisters);
    test.select_register(9);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_ra("type 0 zero-limit fixture reaches C9=4", 4);
    test.expect_c4("type 0 zero-limit fixture keeps C4 at zero", 0);

    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_known_ra("type 0 R9=0 below C9 keeps counting C9", 5);
    test.expect_known_c4("type 0 R9=0 below C9 does not increment C4", 0);

    run_lines(test, 26);
    test.expect_known_ra("type 0 R9=0 lets C9 reach its 5-bit limit", 31);

    run_lines(test, 1);
    test.expect_known_ra("type 0 R9=0 wraps C9 onto the new limit", 0);
    test.expect_known_c4("type 0 R9=0 wrap does not yet increment C4", 0);

    run_lines(test, 1);
    test.expect_known_ra("type 0 wrapped C9 matches R9=0", 0);
    test.expect_known_c4("type 0 wrapped C9 match increments C4 once", 1);
}

void test_type0_c4_counts_through_127_and_wraps(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 12.2: outside adjustment, a C4 that has passed a
    // newly lowered R4 advances on ordinary C9=R9 row completions through 127
    // and wraps.  The R4 write lands at C0=0, inside the last-line window.
    program_registers(test, kC4OverflowRegisters);
    test.select_register(4);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_c4("type 0 C4 overflow fixture reaches C4=4", 4);

    test.write_selected_register_at_clken(1);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_c4("type 0 R4 below C4 keeps counting C4", 5);

    run_lines(test, 122);
    test.expect_c4("type 0 C4 reaches its 7-bit limit", 127);

    run_lines(test, 1);
    test.expect_c4("type 0 C4 wraps past 127", 0);

    run_lines(test, 1);
    test.expect_c4("type 0 wrapped C4 is not yet the new limit", 1);

    run_lines(test, 1);
    test.expect_c4("type 0 wrapped C4 meets the new R4", 0);
}

void test_type0_c4_zero_limit_overflows(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 12.2: R4=0 is reached through equality plus the
    // Last Line / adjustment state, not through a magnitude special case.
    // Written while C4=4, it must let C4 overflow instead of ending the frame.
    program_registers(test, kC4OverflowRegisters);
    test.select_register(4);
    test.reset();

    run_to_fourth_line_start(test);
    test.expect_c4("type 0 zero-limit C4 fixture reaches C4=4", 4);

    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_known_c4("type 0 R4=0 below C4 keeps counting C4", 5);

    run_lines(test, 122);
    test.expect_known_c4("type 0 R4=0 lets C4 reach its 7-bit limit", 127);

    run_lines(test, 1);
    test.expect_known_c4("type 0 R4=0 wraps C4 onto the new limit", 0);
}

void test_type0_rlal_zero_limit_arms_last_line(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 12.2: to arm Last Line while C4=C9=0, one limit must
    // already be zero and the other positive.  Here R9=0 already holds, so
    // writing the remaining positive limit R4 to 0 while C0<2 arms the state
    // and every following line repeats C4=C9=0.  This is the RLAL steady state
    // that removing the comparator shortcuts must preserve.
    program_registers(test, {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 2},
        {5, 0}, {6, 40}, {7, 100}, {8, 0}, {9, 0},
    }});
    test.select_register(4);
    test.reset();

    test.expect_c4("type 0 RLAL fixture starts at C4=0", 0);
    test.expect_ra("type 0 RLAL fixture starts at C9=0", 0);

    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_c4("type 0 zero-limit arm keeps C4 at zero", 0);
    test.expect_ra("type 0 zero-limit arm keeps C9 at zero", 0);
    test.expect_adjustment_inactive("type 0 zero-limit arm is not adjustment");

    run_lines(test, 4);
    test.expect_c4("type 0 zero-limit arm perpetuates C4=0", 0);
    test.expect_ra("type 0 zero-limit arm perpetuates C9=0", 0);
}

void test_type0_rlal_single_zero_limit_does_not_arm(TestBench& test) {
    test.set_crtc_type(0);

    // The same section rejects the loose "R4>0 or R9>0" reading: with R9=2
    // still positive, writing only R4=0 while C4=C9=0 leaves C9 unequal to R9,
    // so Last Line is not armed and C9 simply counts on.
    program_registers(test, {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 2},
        {5, 0}, {6, 40}, {7, 100}, {8, 0}, {9, 2},
    }});
    test.select_register(4);
    test.reset();

    test.expect_c4("type 0 single-zero-limit fixture starts at C4=0", 0);
    test.expect_ra("type 0 single-zero-limit fixture starts at C9=0", 0);

    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_c4("type 0 single zero limit does not arm Last Line", 0);
    test.expect_ra("type 0 single zero limit lets C9 count", 1);
    test.expect_adjustment_inactive(
        "type 0 single zero limit does not enter adjustment");

    // C9 still reaches R9 two lines later; only then does the now-equal R4=0
    // complete a frame, and C4 has never left zero.
    run_lines(test, 2);
    test.expect_ra("type 0 single zero limit completes the C9 count", 0);
    test.expect_c4("type 0 single zero limit never moves C4", 0);
}

void test_type0_rlal_first_line_delayed_arming(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 12.2.1 first-line variant: on the first frame line
    // (C4=C9=0) the Last Line test has already run and found inequality, so a
    // later R9=0/R4 write cannot make that line the last one.  The documented
    // fix writes R4=1 with R9=0 after the C0<2 window: the live rollover
    // compares C9 with the new R9 and increments C4, so line 2 holds C4=1/C9=0
    // and arms Last Line in its own C0<2 window, resetting both on line 3.
    program_registers(test, {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 5},
        {5, 0}, {6, 40}, {7, 100}, {8, 0}, {9, 3},
    }});
    test.select_register(9);
    test.reset();

    test.expect_c4("type 0 delayed-arming fixture starts at C4=0", 0);
    test.expect_ra("type 0 delayed-arming fixture starts at C9=0", 0);

    // C0=2 and C0=3 are both outside the C0<2 window this rule needs.
    test.run_characters(2);
    test.write_selected_register_at_clken(0);
    test.write_register(4, 1);
    test.run_characters(1);

    test.expect_known_c4("type 0 delayed arming increments C4 at the rollover", 1);
    test.expect_known_ra("type 0 delayed arming resets C9 against the new R9", 0);

    run_lines(test, 1);
    test.expect_known_c4("type 0 delayed arming resets C4 on the third line", 0);
    test.expect_known_ra("type 0 delayed arming resets C9 on the third line", 0);
}

void test_type0_rlal_from_the_genuine_last_line(TestBench& test) {
    test.set_crtc_type(0);

    // ACCC v1.10 section 12.2.1, RLAL from the genuine last line: rewriting
    // R9 and R4 to 0 around the true frame end perpetuates C9=C4=0.  The same
    // section requires waiting until C0=2 before rewriting R9 once C9 has
    // become equal to it, so R9=0 lands at C0=2 of the last line and R4=0 at
    // C0=0 of the following line, inside its C0<2 arming window.
    program_registers(test, {{
        {0, 3}, {1, 2}, {2, 2}, {3, 0x11}, {4, 2},
        {5, 0}, {6, 40}, {7, 100}, {8, 0}, {9, 1},
    }});
    test.select_register(9);
    test.reset();

    // Frame lines are (C4,C9) = (0,0) (0,1) (1,0) (1,1) (2,0) (2,1); the last
    // pair is the genuine last line.
    run_lines(test, 5);
    test.expect_c4("type 0 RLAL fixture reaches the last row", 2);
    test.expect_ra("type 0 RLAL fixture reaches the last line", 1);

    test.run_characters(2);
    test.write_selected_register_at_clken(0);
    test.run_characters(1);
    test.expect_c4("type 0 genuine last line still completes the frame", 0);
    test.expect_ra("type 0 genuine last line still resets C9", 0);

    test.select_register(4);
    test.write_selected_register_at_clken(0);
    test.run_characters(kOverflowLineCharacters - 1);
    test.expect_c4("type 0 RLAL keeps C4 at zero", 0);
    test.expect_ra("type 0 RLAL keeps C9 at zero", 0);
    test.expect_adjustment_inactive("type 0 RLAL does not enter adjustment");

    run_lines(test, 4);
    test.expect_c4("type 0 RLAL perpetuates C4=0", 0);
    test.expect_ra("type 0 RLAL perpetuates C9=0", 0);
}

// ACCC v1.10 section 28.1.1: R4=36, R9=7, R5=16 is the documented CRTC
// identification frame.  Use standard 64-character CPC horizontal timing so
// this counter-only oracle is directly comparable with hardware traces.
constexpr RegisterProgram kIdentificationRegisters = {{
    {0, 63}, {1, 40}, {2, 46}, {3, 0x11}, {4, 36},
    {5, 16}, {6, 25}, {7, 0}, {8, 0}, {9, 7},
}};

constexpr unsigned kIdentificationFrameLines = (36 + 1) * (7 + 1) + 16;
constexpr unsigned kIdentificationLineCharacters = 64;
constexpr unsigned kIdentificationObserveCharacters =
    (kIdentificationFrameLines + 1) * kIdentificationLineCharacters;

bool run_identification_sweep(TestBench& test,
                              unsigned type,
                              std::uint8_t r7) {
    test.set_crtc_type(type);
    // Program R7 once with its swept value: an R7 write whose value equals the
    // current C4 is itself a VSYNC trigger, so a placeholder write would add a
    // pulse the sweep must not see.
    RegisterProgram registers = kIdentificationRegisters;
    registers[7].second = r7;
    program_registers(test, registers);
    test.reset();

    return test.vsync_within_characters(kIdentificationObserveCharacters);
}

void test_type0_identification_r7_36_fires(TestBench& test) {
    // Control for the sweep: C4 reaches 36 during ordinary counting, so the
    // fixture must produce a VSYNC before any overflow question arises.
    const bool seen = run_identification_sweep(test, 0, 36);
    test.expect_vsync_observed("type 0 R7=36 still triggers VSYNC", seen);
}

void test_type0_identification_r7_37_fires(TestBench& test) {
    // C4 repeats past R4 exactly once for the type-0 additional-line
    // management, so 37 is the last value R7 can reach.
    const bool seen = run_identification_sweep(test, 0, 37);
    test.expect_vsync_observed("type 0 R7=37 triggers VSYNC in adjustment", seen);
}

void test_type0_identification_r7_38_is_silent(TestBench& test) {
    // The type-0 half of the published discriminator: VSYNC ceases above 37.
    const bool seen = run_identification_sweep(test, 0, 38);
    test.expect_no_vsync_observed("type 0 R7=38 never triggers VSYNC", seen);
}

void test_type1_identification_r7_36_fires(TestBench& test) {
    const bool seen = run_identification_sweep(test, 1, 36);
    test.expect_vsync_observed("type 1 R7=36 still triggers VSYNC", seen);
}

void test_type1_identification_r7_37_fires(TestBench& test) {
    const bool seen = run_identification_sweep(test, 1, 37);
    test.expect_vsync_observed("type 1 R7=37 triggers VSYNC in adjustment", seen);
}

void test_type1_identification_r7_38_fires(TestBench& test) {
    // This is the direct cross-type discriminator: type 0 is already silent
    // at 38, while type 1 increments C4 again during adjustment.  That extra
    // counting depends on F8's separate C5 counter.
    const bool seen = run_identification_sweep(test, 1, 38);
    test.expect_known_vsync_observed("type 1 R7=38 triggers VSYNC in adjustment",
                                     seen);
}

void test_type1_identification_r7_39_fires(TestBench& test) {
    // Type 1 keeps incrementing C4 whenever C9 reaches R9 during adjustment
    // (section 11.2.1): C4=37 on entry, 38 after eight adjustment lines, and
    // 39 as the sixteenth completes the R5 count.  That extra counting is F8's
    // separate C5 counter, which is not implemented, so this boundary is a
    // named F8 divergence rather than a weakened expectation.
    const bool seen = run_identification_sweep(test, 1, 39);
    test.expect_known_vsync_observed("type 1 R7=39 triggers VSYNC in adjustment",
                                     seen);
}

void test_type1_identification_r7_40_is_silent(TestBench& test) {
    // The type-1 half of the discriminator.  It holds on the current model for
    // the wrong reason (C4 stops at 37 without F8), so it is a required pass
    // now and becomes load-bearing once F8 lands.
    const bool seen = run_identification_sweep(test, 1, 40);
    test.expect_no_vsync_observed("type 1 R7=40 never triggers VSYNC", seen);
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    const std::vector<TestCase> tests = {
        {"t00_reset_and_idle_bus", "UM6845R pin-level reset/bus contract", false,
         test_reset_and_idle_bus},
        {"t01_register_readback", "ACCC v1.10 sections 21.2 and 28.1.9; F1/F11c/F11d",
         false, test_register_readback_table},
        {"t02a_type0_r7_hcc0_blocked", "ACCC v1.10 section 16.4.1; F3",
         false, test_type0_r7_hcc0_blocked},
        {"t02b_type0_r7_hcc1_blocked", "ACCC v1.10 section 16.4.1; F3",
         false, test_type0_r7_hcc1_blocked},
        {"t02c_type0_r7_midline_extended", "ACCC v1.10 section 16.4.1; F3",
         false, test_type0_r7_midline_duration_extended},
        {"t02d_type1_r7_early_hcc_immediate", "ACCC v1.10 section 16.4.2; F3",
         false, test_type1_r7_early_hcc_immediate},
        {"t02e_type1_r7_midline_partial_counts", "ACCC v1.10 section 16.4.2; F3",
         false, test_type1_r7_midline_partial_counts},
        {"t02f_r7_level_write_and_active_rearm", "ACCC v1.10 sections 16.3-16.4; F3",
         false, test_r7_level_write_and_active_rearm},
        {"t02g_type0_dynamic_vsync_width_extremes", "ACCC v1.10 sections 14.2 and 16.4.1; F3",
         false, test_type0_dynamic_vsync_width_extremes},
        {"t02h_type0_r7_c0_2_at_line_boundary", "ACCC v1.10 section 16.4.1; F3",
         false, test_type0_r7_c0_2_at_line_boundary},
        {"t02i_type0_interlace_count_boundaries", "ACCC v1.10 sections 16.4-16.5; F3",
         false, test_type0_interlace_count_boundaries},
        {"t02j_type0_pending_skip_type_roundtrip", "UM6845R live CRTC_TYPE contract; F3/F11d",
         false, test_type0_pending_skip_clears_on_type_roundtrip},
        {"t02k_type0_pending_skip_snapshot_load", "UM6845R snapshot-load contract; F3/F11d",
         false, test_type0_pending_skip_clears_on_snapshot_load},
        {"t03a_vsync_compare_lock_and_rearm", "ACCC v1.10 section 16.3; F11b",
         false, test_vsync_compare_lock_and_rearm},
        {"t03b_vsync_reentrancy_bypass", "ACCC v1.10 section 16.3; F11b",
         false, test_vsync_reentrancy_bypass},
        {"t06a_status_waits_for_r0_sample", "ACCC v1.10 section 21.3.3; F2",
         false, test_type1_status_waits_for_r0_sample},
        {"t06b_status_r6_zero_forced_border", "ACCC v1.10 section 21.3.3; F2",
         false, test_type1_status_r6_zero_forced_border},
        {"t06c_status_samples_natural_r6_edge", "ACCC v1.10 section 21.3.3; F2",
         false, test_type1_status_samples_natural_r6_edge},
        {"t06d_status_clears_on_type_round_trip", "ACCC v1.10 section 21.3.3; F2",
         false, test_type1_status_clears_on_type_round_trip},
        {"t09a_type0_r0_zero_suppresses_nonzero_r2_hsync",
         "ACCC v1.10 section 13.2.1; F5", false,
        test_type0_r0_zero_suppresses_nonzero_r2_hsync},
        {"t09b_type0_r0_zero_allows_r2_zero_hsync",
         "ACCC v1.10 sections 13.2.1 and 15.3; F5", false,
        test_type0_r0_zero_allows_r2_zero_hsync},
        {"t09c_type0_r0_zero_resumes_after_nclken_write",
         "ACCC v1.10 section 13.2.1; F5", false,
         test_type0_r0_zero_resumes_after_nclken_write},
        {"t09d_type1_r0_zero_keeps_one_character_lines",
         "ACCC v1.10 section 13.3; F5 regression guard", false,
         test_type1_r0_zero_keeps_one_character_lines},
        {"t09e_type0_midline_r0_zero_free_runs_then_pins",
         "ACCC v1.10 section 13.2.1; F5 live-write regression guard", false,
         test_type0_midline_r0_zero_free_runs_then_pins},
        {"t09f_r0_zero_freeze_survives_type_round_trip",
         "UM6845R live CRTC_TYPE contract; F5/F11d", false,
         test_r0_zero_freeze_survives_type_round_trip},
        {"t09g_type0_interlace_r0_zero_freezes_vsync_count",
         "ACCC v1.10 sections 13.2.1 and 16.5; F3/F5 regression guard", false,
         test_type0_interlace_r0_zero_freezes_vsync_count},
        {"t09h_type0_r0_zero_c9_equal_single_c4_increment_deferred",
         "ACCC v1.10 sections 13.2.1 and 13.2.6; F5/F12", false,
         test_type0_r0_zero_c9_equal_single_c4_increment_deferred},
        {"t16a_type0_r4_write_switches_c9_to_r5",
         "ACCC v1.10 section 11.2.2; F12", false,
         test_type0_adjustment_r4_write_switches_c9_to_r5},
        {"t16b_type0_r9_write_uses_new_r9",
         "ACCC v1.10 section 11.2.2; F12", false,
         test_type0_adjustment_r9_write_uses_new_r9},
        {"t16c_type0_accepts_r5_write_at_c0_2",
         "ACCC v1.10 sections 10.3.1 and 11.2.2; F12", false,
         test_type0_adjustment_accepts_r5_write_at_c0_2},
        {"t16d_type0_rejects_r5_write_after_c0_2",
         "ACCC v1.10 sections 10.3.1 and 11.2.2; F12 guard", false,
         test_type0_adjustment_rejects_r5_write_after_c0_2},
        {"t16e_type0_r9_write_at_r0_increments_c4_and_c9",
         "ACCC v1.10 section 11.2.2; F9/F12", false,
         test_type0_adjustment_r9_write_at_r0_increments_c4_and_c9},
        {"t16f_type0_adjustment_completion_resets_the_next_line",
         "ACCC v1.10 sections 10.3.1 and 11.2.2; F12", false,
         test_type0_adjustment_completion_resets_the_next_line},
        {"t16g_type0_captures_mid_character_r4_write",
         "ACCC v1.10 section 11.2.2; F12 bus-phase guard", false,
         test_type0_adjustment_captures_mid_character_r4_write},
        {"t16h_type0_captures_mid_character_r9_write_at_r0",
         "ACCC v1.10 section 11.2.2; F9/F12 bus-phase guard", false,
         test_type0_adjustment_captures_mid_character_r9_write_at_r0},
        {"t16i_type0_r4_write_at_r0_switches_c9_to_r5",
         "ACCC v1.10 section 11.2.2; F12 boundary guard", false,
         test_type0_adjustment_r4_write_at_r0_switches_c9_to_r5},
        {"t16j_type0_captures_mid_character_r4_write_at_r0",
         "ACCC v1.10 section 11.2.2; F12 bus-phase boundary guard", false,
         test_type0_adjustment_captures_mid_character_r4_write_at_r0},
        {"t16k_type0_arbitration_latches_clear_on_snapshot_load",
         "UM6845R snapshot-load contract; F12 lifecycle guard", false,
         test_type0_adjustment_latches_clear_on_snapshot_load},
        {"t16l_type0_arbitration_latches_clear_on_type_roundtrip",
         "UM6845R live CRTC_TYPE contract; F12 lifecycle guard", false,
         test_type0_adjustment_latches_clear_on_type_roundtrip},
        {"t16m_type0_r4_write_at_c0_1_enters_zero_adjustment",
         "ACCC v1.10 sections 10.3.1, 11.2.2, and 12.2; F12", false,
         test_type0_r4_write_at_c0_1_enters_zero_adjustment},
        {"t16n_type0_r9_write_within_c0_1_enters_zero_adjustment",
         "ACCC v1.10 sections 10.3.1, 11.2.2, and 12.2; F12 bus-phase guard", false,
         test_type0_r9_write_within_c0_1_enters_zero_adjustment},
        {"t16o_type0_r4_write_at_c0_0_overrides_last_line",
         "ACCC v1.10 sections 10.3.1 and 12.2; F12 C0=0 guard", false,
         test_type0_r4_write_at_c0_0_overrides_last_line},
        {"t16p_type0_r0_one_runs_default_zero_adjustment_line",
         "ACCC v1.10 sections 11.2.2 and 13.2.1/13.2.5; F5/F12", false,
         test_type0_r0_one_runs_default_zero_adjustment_line},
        {"t16q_type0_r0_zero_starts_default_zero_adjustment",
         "ACCC v1.10 sections 11.2.2 and 13.2.1/13.2.6; F5/F12", false,
         test_type0_r0_zero_starts_default_zero_adjustment},
        {"t16r_type0_r0_zero_during_adjustment_freezes_c4",
         "ACCC v1.10 section 13.2.3; F5/F12 interaction guard", false,
         test_type0_r0_zero_during_adjustment_freezes_c4},
        {"t16s_type0_r0_one_c0_1_break_is_consumed_at_rollover",
         "ACCC v1.10 sections 10.3.1, 11.2.2, and 13.2.1; F5/F12 boundary",
         false, test_type0_r0_one_c0_1_break_is_consumed_at_rollover},
        {"t07a_type1_c9_counts_through_31_and_wraps",
         "ACCC v1.10 sections 10.3 and 10.3.2; F4", false,
         test_type1_c9_counts_through_31_and_wraps},
        {"t07b_type1_c9_zero_limit_overflows",
         "ACCC v1.10 section 10.3; F4 zero-limit divergence", true,
         test_type1_c9_zero_limit_overflows},
        {"t07c_type1_c4_counts_through_127_and_wraps",
         "ACCC v1.10 sections 12 and 12.3; F4", false,
         test_type1_c4_counts_through_127_and_wraps},
        {"t07d_type1_c4_zero_limit_overflows",
         "ACCC v1.10 section 12.3; F4 zero-limit guard", false,
         test_type1_c4_zero_limit_overflows},
        {"t07e_type0_c9_counts_through_31_and_wraps",
         "ACCC v1.10 sections 10.3.1 and 12.2; F4", false,
         test_type0_c9_counts_through_31_and_wraps},
        {"t07f_type0_c9_zero_limit_overflows",
         "ACCC v1.10 section 10.3; F4 zero-limit divergence", true,
         test_type0_c9_zero_limit_overflows},
        {"t07g_type0_c4_counts_through_127_and_wraps",
         "ACCC v1.10 section 12.2; F4", false,
         test_type0_c4_counts_through_127_and_wraps},
        {"t07h_type0_c4_zero_limit_overflows",
         "ACCC v1.10 section 12.2; F4 zero-limit divergence", true,
         test_type0_c4_zero_limit_overflows},
        {"t07i_type0_rlal_zero_limit_arms_last_line",
         "ACCC v1.10 section 12.2; F4/F12 RLAL guard", false,
         test_type0_rlal_zero_limit_arms_last_line},
        {"t07j_type0_rlal_single_zero_limit_does_not_arm",
         "ACCC v1.10 section 12.2; F4/F12 RLAL guard", false,
         test_type0_rlal_single_zero_limit_does_not_arm},
        {"t07k_type0_rlal_first_line_delayed_arming",
         "ACCC v1.10 sections 10.3.1 and 12.2.1; F4/F12 divergence", true,
         test_type0_rlal_first_line_delayed_arming},
        {"t07l_type0_rlal_from_the_genuine_last_line",
         "ACCC v1.10 section 12.2.1; F4/F12 RLAL guard", false,
         test_type0_rlal_from_the_genuine_last_line},
        {"t08a_type0_identification_r7_36_fires",
         "ACCC v1.10 section 28.1.1; F4 control", false,
         test_type0_identification_r7_36_fires},
        {"t08b_type0_identification_r7_37_fires",
         "ACCC v1.10 sections 28.1.1 and 11.2.1; F4/F12", false,
         test_type0_identification_r7_37_fires},
        {"t08c_type0_identification_r7_38_is_silent",
         "ACCC v1.10 section 28.1.1; F4 type-0 boundary", false,
         test_type0_identification_r7_38_is_silent},
        {"t08d_type1_identification_r7_36_fires",
         "ACCC v1.10 section 28.1.1; F4 control", false,
         test_type1_identification_r7_36_fires},
        {"t08e_type1_identification_r7_37_fires",
         "ACCC v1.10 section 28.1.1; F4/F8 boundary", false,
         test_type1_identification_r7_37_fires},
        {"t08f_type1_identification_r7_38_fires",
         "ACCC v1.10 sections 28.1.1 and 11.2.1; F8 divergence", true,
         test_type1_identification_r7_38_fires},
        {"t08g_type1_identification_r7_39_fires",
         "ACCC v1.10 sections 28.1.1 and 11.2.1; F8 divergence", true,
         test_type1_identification_r7_39_fires},
        {"t08h_type1_identification_r7_40_is_silent",
         "ACCC v1.10 section 28.1.1; F4/F8 type-1 boundary", false,
         test_type1_identification_r7_40_is_silent},
    };

    unsigned passed = 0;
    unsigned xfailed = 0;
    unsigned xpassed = 0;
    unsigned failed = 0;

    for (const TestCase& test : tests) {
        const std::string trace_path = "obj_dir/" + test.name + ".vcd";
        std::string failure;
        bool divergence_failure = false;
        {
            TestBench bench(trace_path);
            try {
                test.run(bench);
            } catch (const KnownDivergenceFailure& error) {
                divergence_failure = true;
                failure = error.what();
            } catch (const TestFailure& error) {
                failure = error.what();
            } catch (const std::exception& error) {
                failure = std::string("unexpected exception: ") + error.what();
            }
        }

        if (failure.empty()) {
            std::error_code remove_error;
            std::filesystem::remove(trace_path, remove_error);
            if (test.known_divergence) {
                ++xpassed;
                std::cout << "XPASS " << test.name << " [" << test.source_rule
                          << "] -- known-divergence marker can be removed\n";
            } else {
                ++passed;
                std::cout << "PASS  " << test.name << " [" << test.source_rule << "]\n";
            }
        } else if (test.known_divergence && divergence_failure) {
            std::error_code remove_error;
            std::filesystem::remove(trace_path, remove_error);
            ++xfailed;
            std::cout << "XFAIL " << test.name << " [" << test.source_rule << "]\n"
                      << "      " << failure << '\n';
        } else {
            ++failed;
            std::cerr << "FAIL  " << test.name << " [" << test.source_rule << "]\n"
                      << "      " << failure << "\n"
                      << "      VCD retained at " << trace_path << '\n';
        }
    }

    std::cout << "\nSummary: " << passed << " passed, " << xfailed << " xfailed, "
              << xpassed << " xpassed, " << failed << " failed\n";
    return failed == 0 && xpassed == 0 ? 0 : 1;
}
