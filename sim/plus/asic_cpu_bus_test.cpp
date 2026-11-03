// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND). FR v1.11 §4.4.3 p26, rendered table:
// OUT(C),r8/0 takes effect in the fourth microsecond on types3/4;
// OUTI/OUTD remains in the fifth, OUT(n),A remains in the third.
#include "Vasic_cpu_bus_test_top.h"
#include "verilated.h"
#include <cstdint>
#include <iostream>
#include <iterator>
#include <utility>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct Bench {
    Vasic_cpu_bus_test_top d;
    unsigned ticks = 0;
    void eval() { d.clk = 0; d.eval(); }
    void tick() { eval(); d.clk = 1; d.eval(); ++ticks; eval(); }
    void run(unsigned n) { while (n--) tick(); }
    template<class F> void until(F condition) {
        for (unsigned n = 0; n < 2000000; ++n) {
            eval();
            if (condition()) return;
            tick();
        }
        throw std::runtime_error("fixture timed out");
    }
    void byte(unsigned address, unsigned value) {
        d.prog_we = 1; d.prog_addr = address; d.prog_data = value; tick();
        d.prog_we = 0; tick();
    }
    explicit Bench(unsigned selected, bool fast = false) {
        d.fast_i = fast;
        d.reset = 1; d.cpu_hold = 1; d.sna_load = 0; d.prog_we = 0;
        d.sna_addr = selected;
        for (unsigned i = 0; i < 5; ++i) d.sna_regs[i] = 0;
        const uint8_t regs[18] = {
            63, 48, 50, 0x1a, 1, 0, 127, 61, 0, 7, 0, 0, 0x30, 0x10, 0, 0, 0, 0
        };
        for (unsigned i = 0; i < 18; ++i)
            d.sna_regs[i / 4] |= unsigned(regs[i]) << (8 * (i % 4));
        run(1024);
        d.reset = 0; d.sna_load = 1; tick(); d.sna_load = 0; run(64);
    }
};

bool exercise(const std::string& kind) {
    const bool alias = kind.rfind("alias", 0) == 0;
    const bool seam = kind == "seam" || kind == "seam-safe";
    Bench b(seam ? 4 : 13, kind == "fast");
    std::vector<uint8_t> program;
    unsigned critical_pc;
    // A real select-register OUT precedes each measured transfer. The leading
    // NOP absorbs T80 reset's initial M1/IORQ overlap without using a test-only
    // CPU reset or replacing the production motherboard's decode.
    if (kind == "fast") {
        program = {0};
        // Same static frame setup as the cartridge boot fixture. Fast mode
        // skips WAIT alignment, so payload delivery remains required while
        // the normal instruction-microsecond windows do not apply.
        for (const auto& pair : {std::pair<uint8_t,uint8_t>{0,63}, {1,40}, {4,38}, {6,25}, {9,7}}) {
            const uint8_t bytes[] = {1,0,0xbc,0x3e,pair.first,0xed,0x79,
                                     1,0,0xbd,0x3e,pair.second,0xed,0x79};
            program.insert(program.end(), std::begin(bytes), std::end(bytes));
        }
        program.push_back(0x76);
        critical_pc = 6;
    } else if (kind == "outi" || kind == "outd") {
        program = {0, 1, 13, 0xbc, 0xed, 0x49, 1, 0x77, 0xbe,
                   0x21, 0x40, 0, 0xed, uint8_t(kind == "outi" ? 0xa3 : 0xab), 0x76};
        critical_pc = 12;
        b.byte(0x40, 0x77);
    } else if (kind == "outn") {
        program = {0, 1, 13, 0xbc, 0xed, 0x49, 0x3e, 0xbd, 0xd3, 0x77, 0x76};
        critical_pc = 8;
    } else if (kind == "read") {
        program = {0, 1, 12, 0xbc, 0xed, 0x49, 1, 0, 0xbe, 0xed, 0x78, 0x76};
        critical_pc = 9;
    } else if (kind == "in-trap" || kind == "alias-in") {
        program = {0, 1, 13, 0xbc, 0xed, 0x49, 1, 0x77, 0xbd, 0xed, 0x78, 0x76};
        critical_pc = 9;
    } else if (seam) {
        program = {0, 1, 4, 0xbc, 0xed, 0x49, 1, 4, 0xbd, 0xed, 0x71, 0x76};
        critical_pc = 9;
    } else {
        program = {0, 1, 13, 0xbc, 0xed, 0x49, 1, 0x77, 0xbd, 0xed, 0x49, 0x76};
        critical_pc = 9;
    }
    if (alias) {
        // A13-low aliases traverse production IC116/READY IORQ shaping;
        // they must retain the same register/read/IN-side-effect contracts.
        program[3] = 0x08;
        program[8] = 0x09;
    }
    for (unsigned i = 0; i < program.size(); ++i) b.byte(i, program[i]);
    const unsigned release_c0 = seam ? (kind == "seam-safe" ? 50 : 49) : 10;
    b.until([&] {
        return b.d.c4 == (seam ? 1 : 0) && b.d.c9 == (seam ? 7 : 1) &&
               b.d.c0 == release_c0;
    });
    b.d.cpu_hold = 0; // release existing sna_hold; all timing stays production
    unsigned fetch = 0, store = 0, fetch_c0 = 0, store_c0 = 0;
    const unsigned old_register = seam ? b.d.r4 : b.d.r13;
    bool halted = false;
    for (unsigned n = 0; n < 12000; ++n) {
        if (!fetch && !b.d.m1_n && !b.d.mreq_n && !b.d.rd_n &&
            b.d.bus_addr == critical_pc) {
            fetch = b.ticks;
            fetch_c0 = b.d.c0;
        }
        if (fetch && !store && (seam ? b.d.r4 : b.d.r13) != old_register) {
            store = b.ticks;
            store_c0 = b.d.c0;
        }
        b.tick();
        if ((store || kind == "read" || kind == "fast") && !b.d.halt_n) { halted = true; break; }
    }
    const unsigned interval = store ? store - fetch : 0;
    std::cout << kind << ": fetch C0=" << fetch_c0 << ", store C0=" << store_c0
              << ", interval=" << interval << " master ticks, C4/C9="
              << unsigned(b.d.c4) << '/' << unsigned(b.d.c9) << ", R1/R4/R9=" << unsigned(b.d.r1) << '/' << unsigned(b.d.r4) << '/' << unsigned(b.d.r9) << ", A=" << unsigned(b.d.cpu_acc) << ", R13=" << unsigned(b.d.r13) << '\n';
    bool pass = fetch && halted && (store || kind == "read" || kind == "fast");
    // Windows measured from the first M1 fetch of the opcode, as in the
    // existing crtc_t80 tests. 64 master clocks = 1us. Expectations come
    // from FR §4.4.3, not from the observed interval.
    if (kind == "fast") pass &= b.d.r1 == 40 && b.d.r4 == 38 && b.d.r9 == 7;
    if (kind == "out" || kind == "alias-out") pass &= interval >= 192 && interval < 256;
    if (kind == "outi" || kind == "outd") pass &= interval >= 256 && interval < 320;
    if (kind == "out" || kind == "alias-out" || kind == "outi" || kind == "outd")
        pass &= b.d.r13 == 0x77;
    if (kind == "outn") pass &= interval >= 128 && interval < 192 && b.d.r13 == 0xbd;
    // Keep read ports continuously readable and retain IN's opcode-byte
    // write side effect on a write-only port (motherboard P5 bus contract).
    if (kind == "read") pass &= b.d.cpu_acc == 0x30 && b.d.r13 == 0x10;
    if (kind == "in-trap" || kind == "alias-in") pass &= b.d.r13 == 0x78;
    if (seam) {
        // Fixture calibration guard: OUT(C),0 starts at internal C0=60
        // (61 in the adjacent safe control). FR §4.4.3 places acceptance
        // on the following character after old C0=R0=63. FR §12.5 p103:
        // old C4=R4=1/C9=R9=7 must wrap to zero before R4 becomes zero.
        // An early store at C0=63 instead loses the old-R4 frame end and
        // increments C4 to2. This is a counter consequence, not merely
        // an assertion of which strobe expression the motherboard uses.
        pass &= fetch_c0 == (kind == "seam-safe" ? 61U : 60U) &&
                b.d.c4 == 0 && b.d.r4 == 0;
    }
    std::cout << (pass ? "PASS " : "FAIL ") << kind << '\n';
    return pass;
}
} // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    try {
        bool pass = true;
        if (argc > 1) pass = exercise(argv[1]);
        else for (const auto* name : {"out", "outi", "outd", "outn", "in-trap", "read", "alias-out", "alias-in", "seam", "seam-safe", "fast"})
            pass = exercise(name) && pass;
        std::cout << "asic-cpu-bus: " << (pass ? "PASS" : "FAIL") << '\n';
        return pass ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "asic-cpu-bus: " << error.what() << '\n';
        return 1;
    }
}
