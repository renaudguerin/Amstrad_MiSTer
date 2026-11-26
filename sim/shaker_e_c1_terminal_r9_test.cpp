#include "VShakerEC1TerminalR9.h"

#include <cstdlib>
#include <iostream>

namespace {

class Fixture {
public:
    Fixture() {
        crtc.CRTC_TYPE = 1;
        crtc.ENABLE = 1;
        crtc.nCS = 1;
        crtc.nRESET = 0;
        crtc.nCLKEN = 0;
        crtc.R_nW = 0;
        crtc.SNA_LOAD = 0;
        character();
        crtc.nRESET = 1;
        write(0, 63);
        write(4, 38);
        write(5, 20);
        write(7, 0);
        write(8, 0);
        write(9, 7);
    }

    void write(unsigned reg, unsigned data) {
        crtc.nCS = 0;
        crtc.RS = 0;
        crtc.DI = reg;
        clock(false);
        crtc.RS = 1;
        crtc.DI = data;
        clock(false);
        crtc.nCS = 1;
    }

    void character() {
        clock(true);
        for (unsigned phase = 1; phase < 64; ++phase) {
            crtc.nCLKEN = phase == 32;
            clock(false);
        }
    }

    VShakerEC1TerminalR9 crtc;

private:
    void clock(bool character_edge) {
        crtc.CLKEN = character_edge;
        crtc.CLOCK = 0;
        crtc.eval();
        crtc.CLOCK = 1;
        crtc.eval();
        crtc.CLOCK = 0;
        crtc.eval();
    }
};

bool check_origin(unsigned write_c0, unsigned lines_after_write) {
    Fixture fixture;
    auto& crtc = fixture.crtc;
    // Original SHAKER 2.7 Module E 03FC first group. PC 7D32 has
    // refreshed R4=38. PC 7D3C writes R9=12, then PC 7D48 polls PPI B.
    for (unsigned i = 0; i < (39 * 8 - 1) * 64 + write_c0; ++i)
        fixture.character();
    if (crtc.SNAP_HCC != write_c0 || crtc.SNAP_ROW != 38 ||
        crtc.RA != 7 || crtc.SNAP_IN_ADJ || crtc.VSYNC) {
        std::cerr << "Did not reach the original final raster before the R9 write\n";
        return false;
    }

    fixture.write(9, 12);
    const unsigned expected = 64 - write_c0 + lines_after_write * 64;
    for (unsigned elapsed = 1; elapsed <= expected + 8 * 64; ++elapsed) {
        fixture.character();
        if (!crtc.VSYNC) continue;

        const bool passed = elapsed == expected && crtc.SNAP_ROW == 0 &&
                            crtc.RA == 0 && !crtc.SNAP_IN_ADJ;
        std::cout << "CRTC1 03FC R9 write at C0=" << write_c0
                  << ", first VSYNC after " << elapsed
                  << " characters, expected " << expected
                  << (passed ? " PASS\n" : " FAIL\n");
        return passed;
    }

    std::cerr << "VSYNC did not start within the expected interval\n";
    return false;
}

} // namespace

int main() {
    // French ACCC v1.11 §11.2.4 p.85 admits adjustment on the old
    // terminal equality at C0=R0. New R9 controls counting, so C9
    // reaches 12 after five adjustment lines. Those lines count toward
    // R5=20 (§11.3.2 p.87). An earlier write cancels admission and
    // makes those five ordinary lines precede the full adjustment.
    const bool terminal = check_origin(63, 20);
    const bool earlier = check_origin(62, 5 + 20);
    return terminal && earlier ? EXIT_SUCCESS : EXIT_FAILURE;
}
