// Asset-free production-T80 mode-boundary regression.
//
// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND 4.0), French v1.11 §§9.1, 9.3.4 pp47, 56-67;
// fixed Mode 3 bitplane decoding and cadence: French ACCC v1.11 §9.1 p47, §9.3.4.3.4 p63;
// GA40010 ordinary 2->0 transition residual 5: French ACCC v1.11 §9.3.4.3.1 p59 (not p60 GA40007/8);
// same-mode genuine R3.JIT controls: French ACCC v1.11 §9.3.4 pp56-57 table.
// Fixture absolute clocks are from verified test harness alignment, not physical measurements.
// Pixel/blank oracles evaluate public classic RGB and true fullblank from
// independent French bitplanes/source oracles, without model metadata.

#include "Vclassic_mode_boundary_top.h"
#include "verilated.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// Palette ink to 6-bit classic RGB {R[1:0], G[1:0], B[1:0]}
// Mapping represents raw GA hardware code encoding qualified from existing fixed controls,
// NOT a new physical measurement:
static const uint8_t INK_TO_RGB[32] = {
    21, 63,  9, 43,  1, 35, 13, 47, // 0..7
    33,  0, 40, 42, 32, 34, 44, 46, // 8..15 (9 unused)
     0,  0,  0,  0,  0,  0,  0,  0, // 16..23
     0,  0,  0,  0,  0,  0,  0, 62  // 24..31 (31 is 62)
};

// Mode-dependent bitplane pixel decode from byte and cursor (residual shift)
// Fixed Mode 3 bitplane decoding: French ACCC v1.11 §9.1 p47, §9.3.4.3.4 p63.
static inline unsigned decode_pen(uint8_t byte_val, unsigned r, unsigned mode) {
    uint8_t x = (r < 8) ? uint8_t((byte_val << r) & 0xFF) : 0;
    unsigned z = ((x >> 7) & 1) | (((x >> 3) & 1) << 1) | (((x >> 5) & 1) << 2) | (((x >> 1) & 1) << 3);
    if (mode == 0) return z;
    if (mode == 2) return z & 1;
    return z & 3; // Mode 1 or Mode 3 (Mode 3 uses Mode 0 cadence with low two pen bits)
}

struct TestCase {
    const char* name;
    bool ordinary;
    bool fixed;
    unsigned width;
    unsigned r2;
    unsigned value;
    unsigned oldmode;
    unsigned newmode;
    unsigned type;
    std::array<uint8_t, 6> markers;
    std::array<uint8_t, 16> inks;
    bool same_mode_jit = false;
};

// All 64 qualified cases: 56 original retained cases + 8 same-numeric JIT controls
const std::vector<TestCase> ALL_CASES = {
    // --- Original 56 Retained Cases ---
    {"ordinary_2to0-type0-2to0", true, false, 2, 22, 22, 2, 0, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to0-type1-2to0", true, false, 2, 22, 22, 2, 0, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_legacy-type0-fixed0", true, true, 2, 46, 46, 0, 0, 0, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31}}},
    {"ordinary_legacy-type0-fixed1", true, true, 2, 46, 46, 1, 1, 0, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31}}},
    {"ordinary_legacy-type0-fixed2", true, true, 2, 46, 46, 2, 2, 0, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31}}},
    {"ordinary_legacy-type0-0to1", true, false, 2, 22, 22, 0, 1, 0, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31}}},
    {"ordinary_legacy-type0-1to2", true, false, 2, 22, 22, 1, 2, 0, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31}}},
    {"ordinary_legacy-type1-fixed0", true, true, 2, 46, 46, 0, 0, 1, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31}}},
    {"ordinary_legacy-type1-fixed1", true, true, 2, 46, 46, 1, 1, 1, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31}}},
    {"ordinary_legacy-type1-fixed2", true, true, 2, 46, 46, 2, 2, 1, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31}}},
    {"ordinary_legacy-type1-0to1", true, false, 2, 22, 22, 0, 1, 1, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31}}},
    {"ordinary_legacy-type1-1to2", true, false, 2, 22, 22, 1, 2, 1, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31, 0, 4, 6, 31}}},
    {"jit_legacy-type0-fixed0", false, true, 14, 46, 14, 0, 0, 0, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_legacy-type0-fixed1", false, true, 14, 46, 14, 1, 1, 0, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_legacy-type0-fixed2", false, true, 14, 46, 14, 2, 2, 0, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_legacy-type0-0to1", false, false, 14, 20, 2, 0, 1, 0, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_legacy-type0-1to2", false, false, 14, 20, 2, 1, 2, 0, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_legacy-type1-fixed0", false, true, 14, 46, 14, 0, 0, 1, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_legacy-type1-fixed1", false, true, 14, 46, 14, 1, 1, 1, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_legacy-type1-fixed2", false, true, 14, 46, 14, 2, 2, 1, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_legacy-type1-0to1", false, false, 14, 20, 2, 0, 1, 1, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_legacy-type1-1to2", false, false, 14, 20, 2, 1, 2, 1, {{182, 109, 227, 156, 91, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to1-type0-fixed0", true, true, 2, 46, 46, 0, 0, 0, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to1-type0-fixed1", true, true, 2, 46, 46, 1, 1, 0, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to1-type0-fixed2", true, true, 2, 46, 46, 2, 2, 0, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to1-type0-2to1", true, false, 2, 22, 22, 2, 1, 0, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to1-type1-fixed0", true, true, 2, 46, 46, 0, 0, 1, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to1-type1-fixed1", true, true, 2, 46, 46, 1, 1, 1, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to1-type1-fixed2", true, true, 2, 46, 46, 2, 2, 1, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to1-type1-2to1", true, false, 2, 22, 22, 2, 1, 1, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to1-type0-fixed0", false, true, 14, 46, 14, 0, 0, 0, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to1-type0-fixed1", false, true, 14, 46, 14, 1, 1, 0, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to1-type0-fixed2", false, true, 14, 46, 14, 2, 2, 0, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to1-type0-2to1", false, false, 14, 20, 2, 2, 1, 0, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to1-type1-fixed0", false, true, 14, 46, 14, 0, 0, 1, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to1-type1-fixed1", false, true, 14, 46, 14, 1, 1, 1, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to1-type1-fixed2", false, true, 14, 46, 14, 2, 2, 1, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to1-type1-2to1", false, false, 14, 20, 2, 2, 1, 1, {{182, 109, 227, 156, 93, 165}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to0-type0-fixed0", true, true, 2, 46, 46, 0, 0, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to0-type0-fixed1", true, true, 2, 46, 46, 1, 1, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to0-type0-fixed2", true, true, 2, 46, 46, 2, 2, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to0-type0-fixed3", true, true, 2, 46, 46, 3, 3, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to0-type1-fixed0", true, true, 2, 46, 46, 0, 0, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to0-type1-fixed1", true, true, 2, 46, 46, 1, 1, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to0-type1-fixed2", true, true, 2, 46, 46, 2, 2, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"ordinary_2to0-type1-fixed3", true, true, 2, 46, 46, 3, 3, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to0-type0-fixed0", false, true, 14, 46, 14, 0, 0, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to0-type0-fixed1", false, true, 14, 46, 14, 1, 1, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to0-type0-fixed2", false, true, 14, 46, 14, 2, 2, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to0-type0-fixed3", false, true, 14, 46, 14, 3, 3, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to0-type0-2to0", false, false, 14, 20, 2, 2, 0, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to0-type1-fixed0", false, true, 14, 46, 14, 0, 0, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to0-type1-fixed1", false, true, 14, 46, 14, 1, 1, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to0-type1-fixed2", false, true, 14, 46, 14, 2, 2, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to0-type1-fixed3", false, true, 14, 46, 14, 3, 3, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},
    {"jit_2to0-type1-2to0", false, false, 14, 20, 2, 2, 0, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}},

    // --- 8 Same-Numeric Genuine R3.JIT Controls ---
    {"jit-same-mode0-type0", false, false, 14, 20, 2, 0, 0, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}, true},
    {"jit-same-mode1-type0", false, false, 14, 20, 2, 1, 1, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}, true},
    {"jit-same-mode2-type0", false, false, 14, 20, 2, 2, 2, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}, true},
    {"jit-same-mode3-type0", false, false, 14, 20, 2, 3, 3, 0, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}, true},
    {"jit-same-mode0-type1", false, false, 14, 20, 2, 0, 0, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}, true},
    {"jit-same-mode1-type1", false, false, 14, 20, 2, 1, 1, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}, true},
    {"jit-same-mode2-type1", false, false, 14, 20, 2, 2, 2, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}, true},
    {"jit-same-mode3-type1", false, false, 14, 20, 2, 3, 3, 1, {{182, 109, 227, 156, 93, 166}}, {{0, 4, 6, 31, 1, 2, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15}}, true}
};

struct CaseResult {
    std::string name;
    bool fixed = false;
    bool same_mode_jit = false;
    bool setup_passed = false;
    unsigned rgb_constraints = 0;
    unsigned rgb_bad = 0;
    unsigned blank_constraints = 0;
    unsigned blank_bad = 0;
    bool pass = false;
};

CaseResult run_case(const TestCase& c) {
    CaseResult res;
    res.name = c.name;
    res.fixed = c.fixed;
    res.same_mode_jit = c.same_mode_jit;

    // 1. Prepare synthetic uncompressed SNA image and guest RAM
    std::vector<uint8_t> s(0x20100, 0);
    std::copy_n("MV - SNA", 8, s.begin());
    s[0x10] = 2; // version 2

    // Explicit architectural register values
    s[0x11] = 0xbb; s[0x17] = 0x6a; s[0x18] = 0xbe; s[0x19] = 0x36;
    s[0x1d] = 0xfe; s[0x1e] = 0xbf; s[0x21] = 0xf8; s[0x22] = 0xbf; s[0x25] = 1;
    s[0x26] = 0x8c; s[0x27] = 0x8d; s[0x28] = 0x8d; s[0x29] = 0x7f;
    s[0x2a] = 0x85; s[0x2b] = 1; s[0x2c] = 0xee; s[0x2d] = 0xbc; s[0x2e] = 15;
    s[0x55] = 30; s[0x59] = 0x82; s[0x6b] = 128; s[0x6d] = 2;

    const uint8_t regs[] = {63, 40, 0, 0, 38, 0, 25, 30, 0, 7, 0, 0, 0x30, 0, 0, 0};
    for (unsigned i = 0; i < 16; ++i) s[0x43 + i] = regs[i];
    s[0x45] = c.r2;
    s[0x46] = c.ordinary ? 2 : 14;
    s[0x42] = c.ordinary ? 2 : 3;
    s[0x40] = 0x8c | c.oldmode;
    s[0x41] = 0;

    for (unsigned i = 0; i < 16; ++i) s[0x2f + i] = c.inks[i];
    s[0x3f] = 0; s[0x1b] = 0; s[0x1c] = 0;
    s[0x12] = 0x8c | c.newmode;
    s[0x13] = 0; s[0x14] = 0x7f; s[0x15] = c.value; s[0x16] = 0;
    s[0x23] = 0xf0; s[0x24] = 0xff; // Initial PC = 0xFFF0

    // Eight-byte guest program at 0x8000: ED79 0103BD ED59 76
    const uint8_t program[] = {0xed, 0x79, 0x01, 0x03, 0xbd, 0xed, 0x59, 0x76};
    for (unsigned i = 0; i < sizeof(program); ++i) s[0x100 + 0x8000 + i] = program[i];

    // Marker patterns across 200 scanlines
    for (unsigned y = 0; y < 200; ++y) {
        unsigned a = 0xc000 + (y & 7) * 0x800 + (y >> 3) * 80 + (c.ordinary ? 42 : 38);
        for (unsigned i = 0; i < 6; ++i) s[0x100 + a + i] = c.markers[i];
    }

    // 2. Instantiate and run simulation
    Vclassic_mode_boundary_top d;
    std::vector<uint8_t> mem(0x800000, 255);
    uint64_t tickno = 0, origin = 0;
    bool active = false, restored = false, oldins = false;
    bool oldhs = false;
    unsigned pc = 0;
    unsigned sync_after_apply = 0, rmr_edges = 0, marker_loads = 0;
    uint64_t store_tick = 0, rise_tick = 0, fall_tick = 0;
    unsigned store_c0 = 0, store_phase = 0, wait_samples = 0;
    bool jit_write_flag = false;

    // Buffer to record classic RGB for active window [0..3000)
    std::vector<uint8_t> recorded_rgb(3000, 0);

    auto fail_setup = [&](const std::string& why) {
        throw std::runtime_error("ACTUAL_SETUP_GUARD " + std::string(c.name) + " tick=" + std::to_string(tickno - origin) + " " + why);
    };

    auto tick = [&]() {
        d.clk = 0; d.eval();
        d.mem_din = mem[d.mem_addr];
        unsigned va = 0x20000 + (unsigned(d.video_addr) << 1);
        d.video_word = mem[va] | (unsigned(mem[va + 1]) << 8);
        d.eval();

        if (d.sna_load) sync_after_apply = 0;
        else if (d.mode_sync) sync_after_apply++;

        const unsigned relative = active ? unsigned(tickno - origin) : 0;
        bool verify_marker = false;
        unsigned marker_index = 0;

        if (active && d.ga_ctrl_accept) {
            bool phase = (relative >= 404 && relative <= 407) || (relative >= 468 && relative <= 471);
            if (!phase || pc != 0x8000 || d.cpu_addr != 0x7f00 || d.ga_data != (0x8c | c.newmode) || !d.cpu_iorq || !d.cpu_wr)
                fail_setup("RMR acceptance byte/address/phase");
            ++rmr_edges;
        }

        if (active && d.dbg_write && !d.dbg_write_d) {
            if (relative != 840 || d.c0 != 22 || d.dbg_char_phase != 8 || d.cpu_addr != 0xbd03 ||
                d.crtc_reg != (c.ordinary ? 2 : 3) || d.r2_value != c.r2 || d.r3_value != c.width)
                fail_setup("CRTC accepted write phase/selection/old registers");
            if (!c.ordinary && !c.fixed && (!d.raw_hs || d.hsc != 2))
                fail_setup("genuine R3.JIT activeHS/C3=2");
        }

        const unsigned first_load = c.ordinary ? 819 : 691;
        if (active && relative >= first_load && relative <= first_load + 160 && (relative - first_load) % 32 == 0) {
            marker_index = (relative - first_load) / 32;
            verify_marker = true;
            if (!d.ce16 || !d.ga_load || d.video_buf != c.markers[marker_index])
                fail_setup("original marker LOAD/input identity");
        }

        if (active) {
            if (d.cpu_insn_start && !oldins) {
                pc = d.cpu_addr;
                if (pc == 0x8000 && !restored) {
                    for (unsigned i = 0; i < 3; ++i) mem[0x2fff0 + i] = s[0x100 + 0xfff0 + i];
                    restored = true;
                }
            }
            oldins = d.cpu_insn_start;

            if (d.dbg_write && !d.dbg_write_d && d.crtc_reg == (c.ordinary ? 2 : 3) && !store_tick) {
                store_tick = relative;
                store_c0 = d.c0;
                store_phase = d.dbg_char_phase;
                jit_write_flag = d.dbg_jit_write;
            }

            if (d.cpu_iorq && d.cpu_wr && !d.ready) wait_samples++;
        }

        if (d.mem_wr && !d.owner_reset) mem[d.mem_addr] = d.mem_dout;
        if (d.load_mem_wr) mem[d.load_mem_addr] = d.load_data;

        d.clk = 1; d.eval();

        if (verify_marker) {
            if (d.shift_reg != c.markers[marker_index]) fail_setup("original marker postLOAD SR");
            ++marker_loads;
        }

        if (active) {
            bool hs = d.raw_hs;
            if (d.c0 >= 20 && d.c0 <= 27) {
                if (hs && !oldhs && !rise_tick) rise_tick = relative;
                if (!hs && oldhs && rise_tick && !fall_tick) fall_tick = relative;
            }
            oldhs = hs;
            if (relative < 3000) {
                recorded_rgb[relative] = d.classic_rgb;
            }
        }

        d.clk = 0; d.eval();
        tickno++;
    };

    // 3. Reset and SNA upload sequence
    d.filter_request = 0; d.save_req = 0; d.cart_dout = 0xff; d.cart_ack = 0;
    d.crtc_type = c.type;
    d.pause = 1;
    d.reset_btn = 1;
    for (int i = 0; i < 1024; ++i) tick();
    d.reset_btn = 0;
    d.sna_download = 1;
    for (int i = 0; i < 4; ++i) tick();
    for (unsigned i = 0; i < s.size(); ++i) {
        d.load_wr = 1; d.load_addr = i; d.load_data = s[i]; tick();
        d.load_wr = 0; tick();
    }
    d.sna_download = 0;
    for (int i = 0; i < 100 && !d.sna_load; ++i) tick();
    if (!d.sna_load) throw std::runtime_error("SNA apply absent " + std::string(c.name));
    tick();

    // 4. Settling beam until release condition: C4=6, C9=0, C0=9, S=7, unblanked, effective_mode == oldmode
    bool origin_seen = false, left_origin = false;
    for (unsigned i = 0; i < 2000000; ++i) {
        tick();
        if (d.c4 > 0) left_origin = true;
        if (left_origin && d.c4 == 0 && d.c9 == 0) origin_seen = true;
        if (origin_seen && d.c4 == 6 && d.c9 == 0 && d.c0 == 9 && d.seq == 7 && !d.classic_raw_blank && d.effective_mode == c.oldmode)
            break;
        if (i == 1999999) throw std::runtime_error("release condition timeout " + std::string(c.name));
    }

    // 5. Post-release setup guards
    if (!sync_after_apply || d.requested_mode != c.oldmode || d.effective_decode != std::array<unsigned, 4>{1, 2, 4, 0}[c.oldmode])
        throw std::runtime_error("ACTUAL_SETUP_GUARD mode/reset/SNA reacquisition " + std::string(c.name));

    for (unsigned k = 0; k < 16; ++k) {
        unsigned bit = k * 5;
        unsigned ink = (d.observed_inks[bit / 32] >> (bit % 32)) & 31;
        if (bit % 32 > 27) ink |= (d.observed_inks[bit / 32 + 1] << (32 - bit % 32)) & 31;
        if (ink != c.inks[k]) throw std::runtime_error("ACTUAL_SETUP_GUARD palette " + std::string(c.name));
    }

    // Temporary trampoline JP 0x8000 at 0xFFF0
    mem[0x2fff0] = 0xc3; mem[0x2fff1] = 0x00; mem[0x2fff2] = 0x80;
    origin = tickno;
    d.pause = 0;
    active = true;
    oldhs = d.raw_hs;

    // 6. Active execution: 3000 ticks
    for (unsigned i = 0; i < 3000; ++i) tick();

    // 7. Fatal setup guard verification
    if (!restored || !store_tick) throw std::runtime_error("CPU OUT not reached " + std::string(c.name));
    if (rmr_edges != 8 || marker_loads != 6 || store_tick != 840 || store_c0 != 22 || store_phase != 8 || wait_samples != 104)
        throw std::runtime_error("ACTUAL_SETUP_GUARD complete CPU/READY/marker counts " + std::string(c.name));
    if (!c.fixed && (rise_tick != (c.ordinary ? 832u : 704u) + 4 * c.type || fall_tick != (c.ordinary ? 960u : 841u) + 4 * c.type))
        throw std::runtime_error("ACTUAL_SETUP_GUARD rawHS schedule " + std::string(c.name));

    res.setup_passed = true;

    // 8. Public RGB and Blank Evaluation
    const unsigned source_origin = c.ordinary ? 827 : 699;

    if (c.same_mode_jit) {
        // Same-mode JIT controls: independently qualified fixed 192-clock sequence masked by JIT black
        // French ACCC v1.11 §9.3.4 pp56-57 (table); fixed Mode 3 French §9.1 p47, §9.3.4.3.4 p63.
        // Fixture absolute clocks are from verified test harness alignment, not physical measurements.
        unsigned m = c.newmode;
        unsigned width = (m == 0 ? 16 : (m == 1 ? 8 : (m == 2 ? 4 : 16)));
        unsigned start = source_origin + (m == 2 ? 0 : 4);
        unsigned black_lo = 715 + 4 * c.type;
        unsigned black_hi = 859;
        res.rgb_constraints = 192;
        for (unsigned i = 0; i < 192; ++i) {
            unsigned by = i / 32;
            unsigned r = (i % 32) / width;
            unsigned p = decode_pen(c.markers[by], r, m);
            unsigned tick_idx = start + i;
            unsigned expected_rgb = (tick_idx >= black_lo && tick_idx < black_hi) ? 0 : INK_TO_RGB[c.inks[p]];
            unsigned actual_rgb = recorded_rgb[tick_idx];
            if (actual_rgb != expected_rgb) {
                res.rgb_bad++;
            }
        }
        // Full blank boundary interval, applying fullblank loop to all !fixed
        unsigned lo = source_origin + 4 * (4 + c.type); // 715 (T0) or 719 (T1)
        unsigned hi = source_origin + 4 * 40;            // 859 (since ordinary == false)
        for (unsigned q = source_origin; q < source_origin + 4 * 49; ++q) {
            bool black = (recorded_rgb[q] == 0);
            bool should_be_black = (q >= lo && q < hi);
            res.blank_constraints++;
            if (black != should_be_black) {
                res.blank_bad++;
            }
        }
    } else if (c.fixed) {
        // Fixed cases: 192 continuous clocks across all 6 bytes
        // French ACCC v1.11 §9.1 p47, §9.3.4 pp56-67; Mode 3 §9.3.4.3.4 p63.
        unsigned m = c.newmode;
        unsigned width = (m == 0 ? 16 : (m == 1 ? 8 : (m == 2 ? 4 : 16)));
        unsigned start = source_origin + (m == 2 ? 0 : 4);
        res.rgb_constraints = 192;
        for (unsigned i = 0; i < 192; ++i) {
            unsigned by = i / 32;
            unsigned r = (i % 32) / width;
            unsigned p = decode_pen(c.markers[by], r, m);
            unsigned expected_rgb = INK_TO_RGB[c.inks[p]];
            unsigned tick_idx = start + i;
            unsigned actual_rgb = recorded_rgb[tick_idx];
            if (actual_rgb != expected_rgb) {
                res.rgb_bad++;
            }
        }
    } else {
        // Mixed mode transition cases: visible window slots
        unsigned start_slot = c.ordinary ? (36 + c.type) : 40;
        unsigned stop_slot = (c.newmode == 2 ? 47 : 48);

        for (unsigned s = start_slot; s <= stop_slot; ++s) {
            unsigned by, r, m;
            if (s == 36) {
                by = 4;
                static const unsigned old_r_tab[3] = {0, 1, 4};
                r = old_r_tab[c.oldmode];
                m = c.oldmode;
            } else if (c.newmode == 2 && s >= 40) {
                by = 5;
                r = s - 40;
                m = 2;
            } else if (c.newmode != 2 && s >= 41) {
                by = 5;
                r = (s - 41) / (c.newmode == 0 ? 4 : 2);
                m = c.newmode;
            } else {
                by = 4;
                // GA40010 ordinary 2->0 transition residual 5: French ACCC v1.11 §9.3.4.3.1 p59 (not p60 GA40007/8)
                static const unsigned old_anchor_r[3] = {1, 2, 5};
                unsigned div = (c.newmode == 0 ? 4 : (c.newmode == 1 ? 2 : 1));
                r = old_anchor_r[c.oldmode] + (s - 37) / div;
                m = c.newmode;
            }

            for (unsigned k = 0; k < 4; ++k) {
                unsigned tick_idx = source_origin + 4 * s + k;
                unsigned p = decode_pen(c.markers[by], r, m);
                unsigned expected_rgb = INK_TO_RGB[c.inks[p]];
                unsigned actual_rgb = recorded_rgb[tick_idx];
                res.rgb_constraints++;
                if (actual_rgb != expected_rgb) {
                    res.rgb_bad++;
                }
            }
        }

        // True fullblank evaluation over window [source_origin, source_origin + 4*49)
        unsigned lo = source_origin + 4 * (4 + c.type);
        unsigned hi = source_origin + 4 * (c.ordinary ? (36 + c.type) : 40);
        for (unsigned q = source_origin; q < source_origin + 4 * 49; ++q) {
            bool black = (recorded_rgb[q] == 0);
            bool should_be_black = (q >= lo && q < hi);
            res.blank_constraints++;
            if (black != should_be_black) {
                res.blank_bad++;
            }
        }
    }

    res.pass = (res.rgb_bad == 0 && res.blank_bad == 0);
    return res;
}

} // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    std::cout << std::unitbuf;

    bool same_numeric_only = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--same-numeric-only") {
            same_numeric_only = true;
        }
    }

    std::cout << "=== CLASSIC MODE BOUNDARY PRODUCTION-T80 RETENTIVE BENCH ===\n";
    if (same_numeric_only) {
        std::cout << "Executing 8 same-numeric JIT controls on production core tuple...\n\n";
    } else {
        std::cout << "Executing " << ALL_CASES.size() << " qualified cases on production core tuple...\n\n";
    }

    unsigned total_passed = 0;
    unsigned total_failed = 0;
    unsigned fixed_passed = 0;
    unsigned fixed_failed = 0;
    unsigned mixed_passed = 0;
    unsigned mixed_failed = 0;

    std::vector<CaseResult> results;
    unsigned cases_run = 0;

    for (const auto& c : ALL_CASES) {
        if (same_numeric_only && !c.same_mode_jit) continue;
        cases_run++;
        try {
            CaseResult res = run_case(c);
            results.push_back(res);

            std::cout << std::left << std::setw(32) << res.name
                      << " " << (res.pass ? "[PASS]" : "[FAIL]")
                      << " RGB_bad: " << std::setw(3) << res.rgb_bad
                      << " Blank_bad: " << std::setw(3) << res.blank_bad;
            if (!res.pass) {
                std::cout << "  (mismatch: ";
                if (res.rgb_bad > 0 && res.blank_bad > 0) std::cout << "RGB+Blank mismatch)";
                else if (res.rgb_bad > 0) std::cout << "RGB mismatch)";
                else std::cout << "blank mismatch)";
            }
            std::cout << "\n";

            if (res.pass) {
                total_passed++;
                if (res.fixed) fixed_passed++; else mixed_passed++;
            } else {
                total_failed++;
                if (res.fixed) fixed_failed++; else mixed_failed++;
            }
        } catch (const std::exception& e) {
            std::cerr << "\nSETUP_BLOCKED in case " << c.name << ": " << e.what() << "\n";
            return 3;
        }
    }

    std::cout << "\n==================== MATRIX SUMMARY ====================\n";
    std::cout << "Total Cases Run:  " << cases_run << " (of " << ALL_CASES.size() << " total)\n";
    std::cout << "Passed Controls:  " << total_passed << "\n";
    std::cout << "Mismatches:       " << total_failed << "\n";
    std::cout << "  - Fixed:        " << fixed_passed << " PASS / " << fixed_failed << " MISMATCH\n";
    std::cout << "  - Mixed/JIT:    " << mixed_passed << " PASS / " << mixed_failed << " MISMATCH\n";
    std::cout << "========================================================\n";

    if (fixed_failed > 0) {
        std::cerr << "FATAL: Fixed control failed! Fixed 40 required baseline controls must all pass.\n";
        return 2;
    }

    if (total_failed > 0) {
        std::cout << "RESULT: SOURCE_ORACLE_MISMATCH (" << total_failed
                  << " mismatches, " << total_passed << " passed, 0 setup blocks).\n";
        return 1;
    }

    std::cout << "RESULT: ALL " << cases_run << " CONTROLS PASSED!\n";
    return 0;
}
