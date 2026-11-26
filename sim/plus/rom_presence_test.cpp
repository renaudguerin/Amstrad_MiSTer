#include "Vrom_presence_test_top.h"

#include <iostream>

int main() {
    Vrom_presence_test_top dut;
    auto tick = [&] {
        dut.clk = 0;
        dut.eval();
        dut.clk = 1;
        dut.eval();
    };
    auto mark = [&](unsigned bank, unsigned slot) {
        dut.write_bank = bank;
        dut.write_slot = slot;
        dut.mark = 1;
        tick();
        dut.mark = 0;
    };

    unsigned failures = 0;
    auto expect_page = [&](const char* label, unsigned bank, unsigned slot,
                           unsigned expected_slot, bool snapshot = false) {
        dut.read_bank = bank;
        dut.select_slot = slot;
        dut.io_write = !snapshot;
        dut.sna_load = snapshot;
        tick();
        if (dut.upper_page != 0x100 + expected_slot) {
            std::cerr << label << ": expected page " << (0x100 + expected_slot)
                      << ", got " << dut.upper_page << '\n';
            ++failures;
        }
        dut.io_write = 0;
        dut.sna_load = 0;
        tick();
    };

    dut.reset = 1;
    dut.mark = 0;
    dut.io_write = 0;
    dut.sna_load = 0;
    tick();
    dut.reset = 0;

    // Boot firmware installs AMSDOS on the two disk models only.
    mark(0, 7);
    mark(1, 7);
    expect_page("6128 AMSDOS", 0, 7, 7);
    expect_page("664 AMSDOS", 1, 7, 7);
    expect_page("464 has no AMSDOS without an expansion", 2, 7, 0);
    expect_page("464 snapshot also falls back to BASIC", 2, 7, 0, true);

    mark(0, 5);
    expect_page("6128 loads its own expansion", 0, 5, 5);
    expect_page("664 cannot see 6128-only expansion", 1, 5, 0);
    mark(2, 3);
    expect_page("464 loads its own expansion", 2, 3, 3, true);
    expect_page("6128 cannot see 464-only expansion", 0, 3, 0);
    mark(3, 9);
    expect_page("Dandanator does not populate a classic slot", 0, 9, 0);

    if (failures) return 1;
    std::cout << "classic ROM presence and MMU selection passed\n";
}
