#include <verilated.h>

#include <array>
#include <cstdint>
#include <iostream>

#include "Vrom_loader_route.h"

namespace {

int failures = 0;

struct RouteExpectation {
    uint8_t ioctl_index;
    uint32_t ioctl_addr;  // byte address in stream (ioctl_addr[24:14] is chunk index)
    uint16_t page;
    bool expected_rom_active;
    bool expected_addr_valid;
    uint8_t expected_bank;
    uint16_t expected_dest_a_hi;
    bool expected_promote;
    const char* description;
};

struct RouteResult {
    bool rom_active;
    bool addr_valid;
    uint8_t bank;
    uint16_t dest_a_hi;
    bool promote;
};

// Frozen reference model of the pre-extraction Amstrad.sv decoder. Keeping it
// in C++ makes the test compare two independently expressed implementations,
// rather than reading expected values back out of the new helper.
RouteResult legacy_route(uint8_t ioctl_index, uint16_t chunk, uint16_t page) {
    RouteResult result{};
    result.rom_active = ((ioctl_index & 0x1fU) < 4U) || ioctl_index == 7U;
    result.promote = ((ioctl_index >> 6U) == 1U) || ((ioctl_index & 0x3fU) != 0U);

    if (ioctl_index != 0U) {
        result.addr_valid = true;
        result.bank = ioctl_index == 7U ? 2U : (((ioctl_index >> 6U) == 3U) ? 1U : 0U);
        result.dest_a_hi = (page & 0x100U) |
            (((page & 0xffU) + (chunk & 0xffU)) & 0xffU);
        return result;
    }

    switch (chunk) {
    case 0: case 4: result.dest_a_hi = 0x000; break;
    case 1: case 5: result.dest_a_hi = 0x100; break;
    case 2: case 6: result.dest_a_hi = 0x107; break;
    case 3: case 7: result.dest_a_hi = 0x0ff; break;
    case 8:         result.dest_a_hi = 0x000; break;
    case 9:         result.dest_a_hi = 0x100; break;
    default:        result.addr_valid = false; return result;
    }
    result.addr_valid = true;
    result.bank = chunk < 4U ? 0U : (chunk < 8U ? 1U : 2U);
    return result;
}

bool check_case(Vrom_loader_route& dut, const RouteExpectation& test) {
    dut.ioctl_index = test.ioctl_index;
    dut.ioctl_addr = static_cast<uint32_t>((test.ioctl_addr >> 14) & 0x7FF);
    dut.page = test.page & 0x1FF;
    dut.eval();

    bool pass = true;
    if (static_cast<bool>(dut.rom_active) != test.expected_rom_active) {
        std::cerr << "FAIL: " << test.description << " (rom_active expected "
                  << test.expected_rom_active << ", got "
                  << static_cast<int>(dut.rom_active) << ")\n";
        pass = false;
    }
    if (static_cast<bool>(dut.addr_valid) != test.expected_addr_valid) {
        std::cerr << "FAIL: " << test.description << " (addr_valid expected "
                  << test.expected_addr_valid << ", got "
                  << static_cast<int>(dut.addr_valid) << ")\n";
        pass = false;
    }
    if (static_cast<uint8_t>(dut.initial_bank) != test.expected_bank) {
        std::cerr << "FAIL: " << test.description << " (initial_bank expected "
                  << static_cast<int>(test.expected_bank) << ", got "
                  << static_cast<int>(dut.initial_bank) << ")\n";
        pass = false;
    }
    if (static_cast<uint16_t>(dut.dest_a_hi) != test.expected_dest_a_hi) {
        std::cerr << "FAIL: " << test.description << " (dest_a_hi expected 0x"
                  << std::hex << test.expected_dest_a_hi << ", got 0x"
                  << dut.dest_a_hi << std::dec << ")\n";
        pass = false;
    }
    if (static_cast<bool>(dut.promote_bank0_to_bank1) != test.expected_promote) {
        std::cerr << "FAIL: " << test.description << " (promote expected "
                  << test.expected_promote << ", got "
                  << static_cast<int>(dut.promote_bank0_to_bank1) << ")\n";
        pass = false;
    }

    if (!pass) {
        ++failures;
    }
    return pass;
}

bool check_exhaustive_equivalence(Vrom_loader_route& dut) {
    constexpr std::array<uint16_t, 6> pages = {0x000, 0x001, 0x0ff, 0x100, 0x1ee, 0x1ff};
    for (unsigned index = 0; index < 256; ++index) {
        for (unsigned chunk = 0; chunk < 2048; ++chunk) {
            for (uint16_t page : pages) {
                const RouteResult expected = legacy_route(index, chunk, page);
                dut.ioctl_index = index;
                dut.ioctl_addr = chunk;
                dut.page = page;
                dut.eval();
                if (static_cast<bool>(dut.rom_active) != expected.rom_active ||
                    static_cast<bool>(dut.addr_valid) != expected.addr_valid ||
                    static_cast<uint8_t>(dut.initial_bank) != expected.bank ||
                    static_cast<uint16_t>(dut.dest_a_hi) != expected.dest_a_hi ||
                    static_cast<bool>(dut.promote_bank0_to_bank1) != expected.promote) {
                    std::cerr << "FAIL: exhaustive legacy equivalence at index=0x"
                              << std::hex << index << " chunk=0x" << chunk
                              << " page=0x" << page << std::dec << "\n";
                    ++failures;
                    return false;
                }
            }
        }
    }
    return true;
}

// 1. Production 10-chunk main boot bundle mapping (index 0, releases/boot.rom)
const RouteExpectation kMainBundleCases[] = {
    {0, 0x00000, 0x000, true, true, 0, 0x000, false, "Main bundle chunk 0: CPC6128 OS -> bank 0, a_hi 0x000"},
    {0, 0x04000, 0x000, true, true, 0, 0x100, false, "Main bundle chunk 1: CPC6128 BASIC -> bank 0, a_hi 0x100"},
    {0, 0x08000, 0x000, true, true, 0, 0x107, false, "Main bundle chunk 2: CPC6128 AMSDOS -> bank 0, a_hi 0x107"},
    {0, 0x0C000, 0x000, true, true, 0, 0x0FF, false, "Main bundle chunk 3: CPC6128 MF2 -> bank 0, a_hi 0x0FF"},
    {0, 0x10000, 0x000, true, true, 1, 0x000, false, "Main bundle chunk 4: CPC664 OS -> bank 1, a_hi 0x000"},
    {0, 0x14000, 0x000, true, true, 1, 0x100, false, "Main bundle chunk 5: CPC664 BASIC -> bank 1, a_hi 0x100"},
    {0, 0x18000, 0x000, true, true, 1, 0x107, false, "Main bundle chunk 6: CPC664 AMSDOS -> bank 1, a_hi 0x107"},
    {0, 0x1C000, 0x000, true, true, 1, 0x0FF, false, "Main bundle chunk 7: CPC664 MF2 -> bank 1, a_hi 0x0FF"},
    {0, 0x20000, 0x000, true, true, 2, 0x000, false, "Main bundle chunk 8: CPC464 OS -> bank 2, a_hi 0x000"},
    {0, 0x24000, 0x000, true, true, 2, 0x100, false, "Main bundle chunk 9: CPC464 BASIC -> bank 2, a_hi 0x100"},
    {0, 0x28000, 0x000, true, false, 0, 0x000, false, "Main bundle chunk 10: unmapped overflow rejected"},
    {0, 0x3C000, 0x000, true, false, 0, 0x000, false, "Main bundle chunk 15: unmapped overflow rejected"},
    {0, 0x1FFC000, 0x000, true, false, 0, 0x000, false, "Main bundle chunk 2047: max overflow rejected"}
};

// 2. Index 7 forced CPC464 route
const RouteExpectation kIndex7Cases[] = {
    {7, 0x00000, 0x000, true, true, 2, 0x000, true, "Index 7 CPC464 ROM: page 0x000 -> bank 2, a_hi 0x000"},
    {7, 0x04000, 0x100, true, true, 2, 0x101, true, "Index 7 CPC464 ROM: page 0x100, chunk 1 -> bank 2, a_hi 0x101"},
    {7, 0x08000, 0x107, true, true, 2, 0x109, true, "Index 7 CPC464 ROM: page 0x107, chunk 2 -> bank 2, a_hi 0x109"},
    {7, 0x00000, 0x1EE, true, true, 2, 0x1EE, true, "Index 7 CPC464 ROM: page 0x1EE malformed -> bank 2, a_hi 0x1EE"}
};

// 3. Generic nonzero index bank selection and auto routes
const RouteExpectation kGenericCases[] = {
    // Menu entries
    {0xC0, 0x00000, 0x000, true, true, 1, 0x000, false, "Index 0xC0 Load Main ROM: bank 1, no promotion"},
    {0xC0, 0x04000, 0x100, true, true, 1, 0x101, false, "Index 0xC0 Load Main ROM: chunk 1, bank 1"},
    {0xC3, 0x00000, 0x000, true, true, 1, 0x000, true,  "Index 0xC3 Load expansion: bank 1, promote flag set"},
    {0xC3, 0x08000, 0x107, true, true, 1, 0x109, true,  "Index 0xC3 Load expansion: chunk 2, bank 1, a_hi 0x109"},

    // Inferred auto routes
    {0x40, 0x00000, 0x000, true, true, 0, 0x000, true,  "Index 0x40 auto route: bank 0, promote to bank 1"},
    {0x41, 0x04000, 0x100, true, true, 0, 0x101, true,  "Index 0x41 auto route: bank 0, chunk 1, promote"},
    {0x80, 0x00000, 0x000, true, true, 0, 0x000, false, "Index 0x80 auto route: bank 0, no promotion"},
    {0x82, 0x08000, 0x107, true, true, 0, 0x109, true,  "Index 0x82 auto route: bank 0, chunk 2, promote to bank 1"},

    // Generic slots 1, 2, 3
    {1, 0x00000, 0x000, true, true, 0, 0x000, true, "Index 1: bank 0, promote to bank 1"},
    {2, 0x00000, 0x100, true, true, 0, 0x100, true, "Index 2: bank 0, promote to bank 1"},
    {3, 0x04000, 0x107, true, true, 0, 0x108, true, "Index 3: chunk 1, bank 0, promote to bank 1"},

    // Non-ROM download indices (must qualify rom_active = false)
    {4, 0x00000, 0x000, false, true, 0, 0x000, true,  "Index 4 (Tape): rom_active false"},
    {5, 0x00000, 0x000, false, true, 0, 0x000, true,  "Index 5 (Dandanator): rom_active false"},
    {6, 0x00000, 0x000, false, true, 0, 0x000, true,  "Index 6 (Snapshot): rom_active false"},
    {8, 0x00000, 0x000, false, true, 0, 0x000, true,  "Index 8 (Plus CPR): rom_active false"},
    {0x14, 0x00000, 0x000, false, true, 0, 0x000, true,  "Index 0x14: rom_active false"},
    {0x1F, 0x00000, 0x000, false, true, 0, 0x000, true,  "Index 0x1F: rom_active false"}
};

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vrom_loader_route dut;

    // 1. Run main bundle tests
    for (const auto& test : kMainBundleCases) {
        check_case(dut, test);
    }

    // 2. Run index 7 tests
    for (const auto& test : kIndex7Cases) {
        check_case(dut, test);
    }

    // 3. Run generic and auto route tests
    for (const auto& test : kGenericCases) {
        check_case(dut, test);
    }

    // 4. Exhaustively compare the entire decoder input space over pages that
    // exercise low-byte carry, page[8], malformed-file fallback and wrap.
    check_exhaustive_equivalence(dut);

    dut.final();

    if (failures) {
        std::cerr << failures << " rom_loader_route test(s) FAILED\n";
        return 1;
    }

    std::cout << "PASS: directed routes and exhaustive legacy equivalence\n";
    return 0;
}
