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
    bool expected_failure;
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

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    const std::vector<TestCase> tests = {
        {"t00_reset_and_idle_bus", "UM6845R pin-level reset/bus contract", false,
         test_reset_and_idle_bus},
        {"t01_register_readback", "ACCC 1.9 sections 21.2 and 28.1.9; F1/F11c/F11d",
         false, test_register_readback_table},
        {"t06a_status_waits_for_r0_sample", "ACCC 1.9 section 21.3.3; F2",
         false, test_type1_status_waits_for_r0_sample},
        {"t06b_status_r6_zero_forced_border", "ACCC 1.9 section 21.3.3; F2",
         false, test_type1_status_r6_zero_forced_border},
        {"t06c_status_samples_natural_r6_edge", "ACCC 1.9 section 21.3.3; F2",
         false, test_type1_status_samples_natural_r6_edge},
        {"t06d_status_clears_on_type_round_trip", "ACCC 1.9 section 21.3.3; F2",
         false, test_type1_status_clears_on_type_round_trip},
    };

    unsigned passed = 0;
    unsigned xfailed = 0;
    unsigned xpassed = 0;
    unsigned failed = 0;

    for (const TestCase& test : tests) {
        const std::string trace_path = "obj_dir/" + test.name + ".vcd";
        std::string failure;
        {
            TestBench bench(trace_path);
            try {
                test.run(bench);
            } catch (const TestFailure& error) {
                failure = error.what();
            } catch (const std::exception& error) {
                failure = std::string("unexpected exception: ") + error.what();
            }
        }

        if (failure.empty()) {
            std::error_code remove_error;
            std::filesystem::remove(trace_path, remove_error);
            if (test.expected_failure) {
                ++xpassed;
                std::cout << "XPASS " << test.name << " [" << test.source_rule
                          << "] -- known-divergence marker can be removed\n";
            } else {
                ++passed;
                std::cout << "PASS  " << test.name << " [" << test.source_rule << "]\n";
            }
        } else if (test.expected_failure) {
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
