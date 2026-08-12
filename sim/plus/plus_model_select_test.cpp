#include <verilated.h>

#include "Vplus_model_select.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace {

struct ExpectedCapabilities {
    std::uint8_t plus_model;
    std::uint8_t plus_mode;
    std::uint8_t ram_128k;
    std::uint8_t has_fdc;
    std::uint8_t has_tape;
    const char* name;
};

constexpr std::array<ExpectedCapabilities, 4> kModels = {{
    {0, 0, 0, 0, 0, "Off"},
    {1, 1, 0, 0, 0, "GX4000"},
    {2, 1, 1, 1, 0, "6128+"},
    {3, 1, 0, 0, 1, "464+"},
}};

bool check_output(const char* signal, std::uint8_t actual,
                  std::uint8_t expected, const char* model_name) {
    if (actual == expected) {
        return true;
    }

    std::cerr << "FAIL: " << model_name << ' ' << signal << ": expected "
              << static_cast<unsigned>(expected) << ", actual "
              << static_cast<unsigned>(actual) << '\n';
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    Vplus_model_select dut;
    bool passed = true;

    for (const auto& expected : kModels) {
        dut.plus_model = expected.plus_model;
        dut.eval();

        passed &= check_output("plus_mode", dut.plus_mode,
                               expected.plus_mode, expected.name);
        passed &= check_output("ram_128k", dut.ram_128k,
                               expected.ram_128k, expected.name);
        passed &= check_output("has_fdc", dut.has_fdc,
                               expected.has_fdc, expected.name);
        passed &= check_output("has_tape", dut.has_tape,
                               expected.has_tape, expected.name);
    }

    dut.final();

    if (!passed) {
        return 1;
    }

    std::cout << "PASS: all Plus model capability decodes\n";
    return 0;
}
