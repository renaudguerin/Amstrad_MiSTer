#include <verilated.h>

#include "Vb6_menu_mask_test_top.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace {

struct ModelCase {
    std::uint8_t model;
    std::uint16_t mask_without_crop;
    std::uint16_t mask_with_crop;
    const char* name;
};

constexpr std::array<ModelCase, 4> kModels = {{
    // Classic keeps both media controls and the classic-only options.
    {0, 0x0038, 0x003a, "classic"},
    // Plus-only visibility is independent of the selected media capability.
    {1, 0x0004, 0x0006, "GX4000"},
    {2, 0x0014, 0x0016, "6128+"},
    {3, 0x0024, 0x0026, "464+"},
}};

bool check(const char* name, std::uint16_t actual, std::uint16_t expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << "FAIL: " << name << ": expected 0x" << std::hex << expected
              << ", actual 0x" << actual << std::dec << '\n';
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    Vb6_menu_mask_test_top dut;
    bool passed = true;

    for (const auto& model : kModels) {
        dut.plus_model = model.model;

        dut.en270p = 0;
        dut.eval();
        passed &= check(model.name, dut.status_menumask, model.mask_without_crop);

        dut.en270p = 1;
        dut.eval();
        passed &= check(model.name, dut.status_menumask, model.mask_with_crop);
    }

    dut.final();

    if (!passed) {
        return 1;
    }

    std::cout << "PASS: B6 menu mask follows model and media capabilities\n";
    return 0;
}
