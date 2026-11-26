#include <verilated.h>

#include "Vb6_menu_mask_test_top.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace {

struct ModelCase {
    std::uint8_t model;
    std::uint8_t ram_128k;
    std::uint16_t mask_without_crop;
    std::uint16_t mask_with_crop;
    const char* name;
};

constexpr std::array<ModelCase, 4> kModels = {{
// ram_128k follows the machine facts behind the decoder contract
// (rtl/plus/plus_model_select.v header): GX4000 and 464+ are 64K,
// the 6128+ is 128K, and Off fails closed. The mask contract does not
// cover ram_128k, so this fixture pins it directly; it is the only
// consumer-side check of that output alongside the production RAM gates.
    // Classic keeps both media controls and the classic-only options.
    {0, 0, 0x0038, 0x003a, "classic"},
    // Plus-only visibility is independent of the selected media capability.
    {1, 0, 0x0004, 0x0006, "GX4000"},
    {2, 1, 0x0014, 0x0016, "6128+"},
    {3, 0, 0x0024, 0x0026, "464+"},
}};

bool check(const char* name, std::uint16_t actual, std::uint16_t expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << "FAIL: " << name << ": expected 0x" << std::hex << expected
              << ", actual 0x" << actual << std::dec << '\n';
    return false;
}

bool check_ram(const char* name, std::uint8_t actual, std::uint8_t expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << "FAIL: " << name << " ram_128k: expected "
              << static_cast<unsigned>(expected) << ", actual "
              << static_cast<unsigned>(actual) << '\n';
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    Vb6_menu_mask_test_top dut;
    bool passed = true;

    dut.clk = 0;
    dut.reset_apply = 0;
    dut.crtc_request = 0;
    dut.sna_download_request = 0;
    for (const auto& model : kModels) {
        dut.plus_model = model.model;

        dut.en270p = 0;
        dut.eval();
        passed &= check(model.name, dut.status_menumask, model.mask_without_crop);
        passed &= check_ram(model.name, dut.ram_128k, model.ram_128k);

        dut.en270p = 1;
        dut.eval();
        passed &= check(model.name, dut.status_menumask, model.mask_with_crop);
        passed &= check_ram(model.name, dut.ram_128k, model.ram_128k);
    }

    auto tick = [&] {
        dut.clk = 1;
        dut.eval();
        dut.clk = 0;
        dut.eval();
    };
    auto apply = [&] {
        dut.reset_apply = 1;
        tick();
        dut.reset_apply = 0;
        tick();
    };

    auto retire_snapshot = [&](std::uint8_t classic_type) {
        dut.sna_download_request = 0;
        for (unsigned clock = 0; clock < 12 && !dut.sna_load; ++clock) tick();
        passed &= check("admitted F6 reaches snapshot apply", dut.sna_load, 1);
        passed &= check("classic selection at snapshot apply", dut.crtc_type, classic_type);
        tick();
    };

    dut.plus_model = 0;
    dut.en270p = 0;
    tick();
    passed &= check("legacy default Type 1", dut.crtc_type, 1);
    dut.crtc_request = 1;
    tick();
    passed &= check("legacy live Type 0", dut.crtc_type, 0);
    dut.crtc_request = 2;
    tick();
    passed &= check("pending Type 2 keeps Type 0 running", dut.crtc_type, 0);
    passed &= check("pending Type 2 keeps snapshot menu", dut.status_menumask, 0x0038);
    dut.sna_download_request = 1;
    tick();
    passed &= check("pending Type 2 admits classic F6", dut.sna_download, 1);
    apply();
    passed &= check("in-flight snapshot defers Type 2", dut.crtc_type, 0);
    retire_snapshot(0);

    apply();
    passed &= check("applied Type 2", dut.crtc_type, 2);
    passed &= check("Type 2 hides snapshots only", dut.status_menumask, 0x0078);
    dut.crtc_request = 3;
    tick();
    passed &= check("pending Type 4 keeps Type 2 running", dut.crtc_type, 2);
    apply();
    passed &= check("applied Type 4", dut.crtc_type, 3);
    passed &= check("Type 4 hides snapshots only", dut.status_menumask, 0x0078);
    dut.sna_download_request = 1;
    tick();
    passed &= check("active Type 4 rejects external F6", dut.sna_download, 0);

    dut.plus_model = 2;
    tick();
    passed &= check("Plus overrides classic snapshot menu", dut.status_menumask, 0x0014);
    passed &= check("rejected F6 cannot start midway", dut.sna_download, 0);
    passed &= check("rejected F6 leaves snapshot hold idle", dut.sna_hold, 0);
    dut.sna_download_request = 0;
    tick();
    dut.sna_download_request = 1;
    tick();
    passed &= check("Plus admits new F6", dut.sna_download, 1);
    retire_snapshot(3);

    dut.plus_model = 0;
    dut.crtc_request = 1;
    tick();
    passed &= check("pending Type 0 keeps Type 4 running", dut.crtc_type, 3);
    passed &= check("pending exit keeps snapshots hidden", dut.status_menumask, 0x0078);
    apply();
    passed &= check("explicit reset exits Type 4", dut.crtc_type, 0);
    passed &= check("Type 0 restores snapshot menu", dut.status_menumask, 0x0038);
    dut.crtc_request = 0;
    tick();
    passed &= check("legacy live Type 1 restored", dut.crtc_type, 1);

    // An already-admitted Plus upload may still finish after Plus is
    // deselected. Preserve the last running Type 1 through that apply,
    // even with a different legacy choice currently shown in the menu.
    dut.crtc_request = 3;
    apply();
    dut.plus_model = 2;
    tick();
    dut.sna_download_request = 1;
    tick();
    dut.crtc_request = 1;
    tick();
    dut.plus_model = 0;
    dut.eval();
    passed &= check("Plus exit during F6 immediately selects legacy Type 1", dut.crtc_type, 1);
    tick();
    passed &= check("Plus exit does not truncate the admitted F6", dut.sna_download, 1);
    retire_snapshot(1);

    dut.crtc_request = 3;
    tick();
    passed &= check("Type 4 remains pending after fallback", dut.crtc_type, 1);
    apply();
    passed &= check("explicit reset reapplies Type 4 after fallback", dut.crtc_type, 3);

    dut.final();

    if (!passed) {
        return 1;
    }

    std::cout << "PASS: B6 menu mask, CRTC selection and snapshot upload admission\n";
    return 0;
}
