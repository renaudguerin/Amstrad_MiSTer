#include <verilated.h>

#include <cstdint>
#include <iostream>

#include "Vdandanator_loader_bounds.h"

namespace {

int failures = 0;

void expect(Vdandanator_loader_bounds &dut, bool download, bool wr,
            std::uint32_t addr, bool accepted, const char *name) {
    dut.dan_download = download;
    dut.ioctl_wr = wr;
    dut.ioctl_addr = addr;
    dut.eval();

    if (static_cast<bool>(dut.write_accepted) != accepted) {
        std::cerr << "FAIL: " << name << " (addr=0x" << std::hex << addr
                  << std::dec << ")\n";
        ++failures;
    }
}

} // namespace

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Vdandanator_loader_bounds dut;

    expect(dut, true,  true,  0x000000, true,  "first byte accepted");
    expect(dut, true,  true,  0x07ffff, true,  "last byte accepted");
    expect(dut, true,  true,  0x080000, false, "first overflow byte rejected");
    expect(dut, true,  true,  0x1ffffff, false, "maximum address rejected");

    expect(dut, false, true,  0x000000, false, "inactive download rejected");
    expect(dut, true,  false, 0x000000, false, "inactive write rejected");
    expect(dut, false, false, 0x000000, false, "both controls inactive rejected");
    expect(dut, false, true,  0x07ffff, false, "download gates upper boundary");
    expect(dut, true,  false, 0x07ffff, false, "write gates upper boundary");

    dut.final();
    if (failures) {
        std::cerr << failures << " Dandanator loader bounds test(s) failed\n";
        return 1;
    }

    std::cout << "Dandanator loader bounds tests passed\n";
    return 0;
}
