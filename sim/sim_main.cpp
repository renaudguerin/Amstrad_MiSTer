#include <verilated.h>
#include <verilated_vcd_c.h>

#include "VUM6845R.h"

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

    // ACCC 1.9 section 21.3.3: setting R6=0 while C4>0 forces border,
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

    // ACCC 1.9 section 13.2.1: C0 remains pinned at zero, so it can never
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

    // ACCC 1.9 section 15.3: type 0 does not restart HSYNC on the second
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

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    const std::vector<TestCase> tests = {
        {"t00_reset_and_idle_bus", "UM6845R pin-level reset/bus contract", false,
         test_reset_and_idle_bus},
        {"t01_register_readback", "ACCC 1.9 sections 21.2 and 28.1.9; F1/F11c/F11d",
         false, test_register_readback_table},
        {"t02a_type0_r7_hcc0_blocked", "ACCC 1.9 section 16.4.1; F3",
         false, test_type0_r7_hcc0_blocked},
        {"t02b_type0_r7_hcc1_blocked", "ACCC 1.9 section 16.4.1; F3",
         false, test_type0_r7_hcc1_blocked},
        {"t02c_type0_r7_midline_extended", "ACCC 1.9 section 16.4.1; F3",
         false, test_type0_r7_midline_duration_extended},
        {"t02d_type1_r7_early_hcc_immediate", "ACCC 1.9 section 16.4.2; F3",
         false, test_type1_r7_early_hcc_immediate},
        {"t02e_type1_r7_midline_partial_counts", "ACCC 1.9 section 16.4.2; F3",
         false, test_type1_r7_midline_partial_counts},
        {"t02f_r7_level_write_and_active_rearm", "ACCC 1.9 sections 16.3-16.4; F3",
         false, test_r7_level_write_and_active_rearm},
        {"t02g_type0_dynamic_vsync_width_extremes", "ACCC 1.9 sections 14.2 and 16.4.1; F3",
         false, test_type0_dynamic_vsync_width_extremes},
        {"t02h_type0_r7_c0_2_at_line_boundary", "ACCC 1.9 section 16.4.1; F3",
         false, test_type0_r7_c0_2_at_line_boundary},
        {"t02i_type0_interlace_count_boundaries", "ACCC 1.9 sections 16.4-16.5; F3",
         false, test_type0_interlace_count_boundaries},
        {"t02j_type0_pending_skip_type_roundtrip", "UM6845R live CRTC_TYPE contract; F3/F11d",
         false, test_type0_pending_skip_clears_on_type_roundtrip},
        {"t02k_type0_pending_skip_snapshot_load", "UM6845R snapshot-load contract; F3/F11d",
         false, test_type0_pending_skip_clears_on_snapshot_load},
        {"t03a_vsync_compare_lock_and_rearm", "ACCC 1.9 section 16.3; F11b",
         false, test_vsync_compare_lock_and_rearm},
        {"t03b_vsync_reentrancy_bypass", "ACCC 1.9 section 16.3; F11b",
         false, test_vsync_reentrancy_bypass},
        {"t06a_status_waits_for_r0_sample", "ACCC 1.9 section 21.3.3; F2",
         false, test_type1_status_waits_for_r0_sample},
        {"t06b_status_r6_zero_forced_border", "ACCC 1.9 section 21.3.3; F2",
         false, test_type1_status_r6_zero_forced_border},
        {"t06c_status_samples_natural_r6_edge", "ACCC 1.9 section 21.3.3; F2",
         false, test_type1_status_samples_natural_r6_edge},
        {"t06d_status_clears_on_type_round_trip", "ACCC 1.9 section 21.3.3; F2",
         false, test_type1_status_clears_on_type_round_trip},
        {"t09a_type0_r0_zero_suppresses_nonzero_r2_hsync",
         "ACCC 1.9 section 13.2.1; F5", false,
        test_type0_r0_zero_suppresses_nonzero_r2_hsync},
        {"t09b_type0_r0_zero_allows_r2_zero_hsync",
         "ACCC 1.9 sections 13.2.1 and 15.3; F5", false,
        test_type0_r0_zero_allows_r2_zero_hsync},
        {"t09c_type0_r0_zero_resumes_after_nclken_write",
         "ACCC 1.9 section 13.2.1; F5", false,
         test_type0_r0_zero_resumes_after_nclken_write},
        {"t09d_type1_r0_zero_keeps_one_character_lines",
         "ACCC 1.9 section 13.3; F5 regression guard", false,
         test_type1_r0_zero_keeps_one_character_lines},
        {"t09e_type0_midline_r0_zero_free_runs_then_pins",
         "ACCC 1.9 section 13.2.1; F5 live-write regression guard", false,
         test_type0_midline_r0_zero_free_runs_then_pins},
        {"t09f_r0_zero_freeze_survives_type_round_trip",
         "UM6845R live CRTC_TYPE contract; F5/F11d", false,
         test_r0_zero_freeze_survives_type_round_trip},
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
