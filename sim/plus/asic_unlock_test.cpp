#include <verilated.h>

#include "Vasic_unlock.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

constexpr std::array<std::uint8_t, 13> kSequence = {
    0xFF, 0x77, 0xB3, 0x51, 0xA8, 0xD4, 0x62,
    0x39, 0x9C, 0x46, 0x2B, 0x15, 0x8A,
};

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class TestBench {
public:
    TestBench() {
        dut_.clk = 0;
        dut_.RESET_N = 1;
        dut_.write_strobe = 0;
        dut_.write_data = 0;
        dut_.eval();
        reset();
    }

    ~TestBench() {
        dut_.final();
    }

    void reset() {
        dut_.write_strobe = 0;
        dut_.RESET_N = 0;
        dut_.eval();
        expect_locked("asynchronous hard reset");
        clock_cycle();
        dut_.RESET_N = 1;
        dut_.eval();
        expect_locked("hard reset release");
    }

    void write(std::uint8_t value) {
        dut_.write_data = value;
        dut_.write_strobe = 1;
        clock_cycle();
        dut_.write_strobe = 0;
        dut_.eval();
    }

    void idle(std::uint8_t value, unsigned cycles = 1) {
        dut_.write_data = value;
        dut_.write_strobe = 0;
        for (unsigned cycle = 0; cycle < cycles; ++cycle) {
            clock_cycle();
        }
    }

    void sync(std::uint8_t leading_nonzero) {
        if (leading_nonzero == 0) {
            throw TestFailure("test error: sync lead must be nonzero");
        }
        write(leading_nonzero);
        write(0x00);
    }

    void write_fixed_sequence() {
        for (const auto byte : kSequence) {
            write(byte);
        }
    }

    void apply_sequence(std::uint8_t leading_nonzero,
                        std::uint8_t state) {
        sync(leading_nonzero);
        write_fixed_sequence();
        write(state);
    }

    void expect_locked(const std::string& context) const {
        if (dut_.unlocked != 0) {
            throw TestFailure(context + ": expected locked, actual unlocked");
        }
    }

    void expect_unlocked(const std::string& context) const {
        if (dut_.unlocked != 1) {
            throw TestFailure(context + ": expected unlocked, actual locked");
        }
    }

private:
    void clock_cycle() {
        dut_.clk = 0;
        dut_.eval();
        dut_.clk = 1;
        dut_.eval();
        dut_.clk = 0;
        dut_.eval();
    }

    Vasic_unlock dut_;
};

void test_reset_and_no_strobe_stability() {
    TestBench test;

    for (unsigned value = 0; value <= 0xFF; ++value) {
        test.idle(static_cast<std::uint8_t>(value));
        test.expect_locked("unstrobed data while locked");
    }

    test.apply_sequence(0xFF, 0xCD);
    test.expect_unlocked("complete sequence without trailing EE");

    test.idle(0x00, 4);
    test.idle(0xCD, 4);
    test.idle(0xEE, 4);
    test.expect_unlocked("unstrobed data while unlocked");

    test.reset();
    test.expect_locked("reset after unlock");

    test.sync(0x29);
    for (std::size_t index = 0; index < 5; ++index) {
        test.write(kSequence[index]);
    }
    test.idle(kSequence[5], 8);
    for (std::size_t index = 5; index < kSequence.size(); ++index) {
        test.write(kSequence[index]);
    }
    test.write(0xCD);
    test.expect_unlocked("no-strobe cycles preserve sequence progress");
}

void test_partial_prefixes_do_not_unlock() {
    for (std::size_t length = 0; length <= kSequence.size(); ++length) {
        TestBench test;
        test.sync(0x31);
        for (std::size_t index = 0; index < length; ++index) {
            test.write(kSequence[index]);
        }
        test.expect_locked("partial fixed prefix of length " +
                           std::to_string(length));
    }
}

void test_wrong_bytes_fail_closed() {
    for (std::size_t wrong_index = 0; wrong_index < kSequence.size();
         ++wrong_index) {
        TestBench test;
        test.sync(0x42);
        for (std::size_t index = 0; index < kSequence.size(); ++index) {
            const auto byte = index == wrong_index
                                  ? static_cast<std::uint8_t>(kSequence[index] ^ 0x01)
                                  : kSequence[index];
            test.write(byte);
        }
        test.write(0xCD);
        test.expect_locked("wrong fixed byte at index " +
                           std::to_string(wrong_index));
    }

    TestBench test;
    test.write(0x00);
    test.write(0x00);
    test.write_fixed_sequence();
    test.write(0xCD);
    test.expect_locked("zero, zero is not a valid sync pair");

    TestBench zero_after_sync;
    zero_after_sync.sync(0x48);
    zero_after_sync.write(0x00);
    zero_after_sync.write_fixed_sequence();
    zero_after_sync.write(0xCD);
    zero_after_sync.expect_locked(
        "zero immediately after the sync zero aborts matching");
}

void test_resynchronization() {
    TestBench test;

    test.sync(0x12);
    for (std::size_t index = 0; index < 5; ++index) {
        test.write(kSequence[index]);
    }
    test.write(0x7E);
    test.write(0xA5);
    test.expect_locked("mismatched partial attempt");

    test.apply_sequence(0x03, 0xCD);
    test.expect_unlocked("fresh sync after a mismatch");

    test.reset();
    test.sync(0x6D);
    test.write(kSequence[0]);
    test.write(kSequence[1]);
    test.write(0x00);  // The preceding 0x77 plus this zero is a new sync.
    test.write_fixed_sequence();
    test.write(0xCD);
    test.expect_unlocked(
        "zero after a later matched nonzero restarts matching");
}

void test_malformed_input_preserves_unlocked() {
    TestBench test;
    test.apply_sequence(0xFF, 0xCD);
    test.expect_unlocked("precondition before malformed input");

    test.sync(0x24);
    for (std::size_t index = 0; index < kSequence.size(); ++index) {
        const auto byte = index == 4
                              ? static_cast<std::uint8_t>(kSequence[index] ^ 0x01)
                              : kSequence[index];
        test.write(byte);
    }
    test.write(0x00);
    test.expect_unlocked("malformed sequence does not change unlocked state");
}

void test_every_nonzero_sync_lead_unlocks() {
    for (unsigned leading = 1; leading <= 0xFF; ++leading) {
        TestBench test;
        test.apply_sequence(static_cast<std::uint8_t>(leading), 0xCD);
        test.expect_unlocked("nonzero sync lead " + std::to_string(leading));
    }
}

void test_state_byte_and_trailing_ee() {
    TestBench test;
    test.sync(0xFF);
    test.write_fixed_sequence();
    test.expect_locked("state byte has not been written");
    test.write(0xCD);
    test.expect_unlocked("CD state byte");
    test.write(0xEE);
    test.expect_unlocked("trailing EE is irrelevant");

    test.reset();
    test.apply_sequence(0xFF, 0xCD);
    test.sync(0x55);
    test.write_fixed_sequence();
    test.write(0x00);
    test.expect_locked("zero state byte locks");
    // The preceding fixed 0x8A and STATE 0x00 also form a fresh sync pair.
    test.write_fixed_sequence();
    test.write(0xCD);
    test.expect_unlocked("zero state byte also starts a fresh sequence");

    for (unsigned state_value = 0; state_value <= 0xFF; ++state_value) {
        const auto state = static_cast<std::uint8_t>(state_value);
        if (state == 0xCD) {
            continue;
        }
        test.reset();
        test.apply_sequence(0xFF, 0xCD);
        test.expect_unlocked("precondition before lock sequence");

        test.sync(0x55);
        test.write_fixed_sequence();
        test.expect_unlocked("valid prefix does not change unlocked state");
        test.write(state);
        test.expect_locked("non-CD state byte " + std::to_string(state));
    }
}

struct TestCase {
    const char* name;
    void (*run)();
};

constexpr std::array<TestCase, 7> kTests = {{
    {"reset and no-strobe stability", test_reset_and_no_strobe_stability},
    {"partial prefixes", test_partial_prefixes_do_not_unlock},
    {"wrong bytes fail closed", test_wrong_bytes_fail_closed},
    {"resynchronization", test_resynchronization},
    {"malformed input while unlocked", test_malformed_input_preserves_unlocked},
    {"all nonzero sync leads", test_every_nonzero_sync_lead_unlocks},
    {"state byte and trailing EE", test_state_byte_and_trailing_ee},
}};

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    unsigned passed = 0;
    for (const auto& test : kTests) {
        try {
            test.run();
            ++passed;
            std::cout << "PASS: " << test.name << '\n';
        } catch (const std::exception& error) {
            std::cerr << "FAIL: " << test.name << ": " << error.what() << '\n';
            return 1;
        }
    }

    std::cout << "All " << passed << " ASIC unlock tests passed\n";
    return 0;
}
