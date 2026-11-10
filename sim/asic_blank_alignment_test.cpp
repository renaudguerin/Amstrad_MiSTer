// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND 4.0), French v1.11 §14.7.2 p144 and §9.2.1 p48.
// The Type3 chronogram is explicitly a working assumption in the French source.
// Expected RGB indices15..46 are independently measured SOFTWARE-reference
// behavior, not a silicon constant: four normal-boot AmSpirit Lite1.17.1 cases,
// full VRAM/palette/register controls and both Mode1/2 byte origins verified.
// See hardware observations#normal-boot-reference-controls.
// One primary motherboard/production-T80/READY/VRAM-to-RGB boundary regression;
// the leaf cases protect the distinct reset-free SNA_LOAD lifecycle boundary.
#include "Vasic_blank_alignment_top.h"
#include "verilated.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool ok, const std::string& why) {
    if (!ok) throw std::runtime_error(why);
}
constexpr std::array<uint8_t, 6> markers{{0xb6, 0x6d, 0xe3, 0x9c, 0x5b, 0xa5}};
constexpr std::array<unsigned, 4> colours{{0xfff, 0xf00, 0x0f0, 0x00f}};
constexpr unsigned clocks_per_dot = 4;
constexpr unsigned first_black_dot = 15, first_visible_dot = 47;

std::vector<uint8_t> snapshot(unsigned mode, unsigned r2) {
    std::vector<uint8_t> s(0x20100, 0);
    std::copy_n("MV - SNA", 8, s.begin());
    s[0x10] = 3;
    s[0x21] = 0xf0; s[0x22] = 0xbf; // SP=BFF0, IFF1/2=0.
    s[0x24] = 0x80;                 // PC8000: our own HALT, no trampoline.
    s[0x25] = 1;
    s[0x40] = uint8_t(0x8c | mode); // Both ROMs off, locked ASIC.
    const uint8_t regs[] = {63,40,uint8_t(r2),2,38,0,25,30,0,7,0,0,48,0};
    std::copy(std::begin(regs), std::end(regs), s.begin() + 0x43);
    const uint8_t inks[] = {11,12,18,21}; // Every data colour differs from black.
    for (unsigned p = 0; p < 16; ++p) s[0x2f + p] = inks[p % 4];
    s[0x3f] = 20;
    s[0x59] = 0x82; s[0x6b] = 128; s[0x6d] = 4; s[0xa4] = 3;
    s[0x8100] = 0x76;
    for (unsigned y = 0; y < 200; ++y) {
        const unsigned at = 0x100 + 0xc000 + (y & 7) * 0x800 + (y >> 3) * 80 + 42;
        std::copy(markers.begin(), markers.end(), s.begin() + at);
    }
    // Counter/IRQ zero is an explicit lab seed, not a measured checkpoint.
    // Two complete frame wraps settle hidden VMA/serializer state naturally.
    return s;
}
struct Sample { unsigned rgb, pen; bool hs, vs, de; };
struct Bench {
    Vasic_blank_alignment_top d;
    std::vector<uint8_t> memory = std::vector<uint8_t>(0x800000, 0xff);
    bool fetched_halt = false, old_insn = false;
    Bench() {
        d.reset_btn = 1; d.sna_download = 0; d.load_wr = 0;
        d.load_addr = 0; d.load_data = 0; d.mem_din = 0xff; d.video_word = 0;
        d.life_reset_n = 0; d.life_pix = 0; d.life_ce = 0; d.life_load = 0;
        d.life_hs = 0; d.life_hcc = 0; d.life_hsw = 0;
    }
    void tick() {
        d.clk = 0; d.eval();
        d.mem_din = memory.at(d.mem_addr);
        const unsigned va = 0x20000 + unsigned(d.video_addr) * 2;
        d.video_word = memory.at(va) | unsigned(memory.at(va + 1)) << 8;
        d.eval();
        if (!d.owner_reset && !d.sna_hold) {
            if (d.cpu_insn_start && !old_insn && d.cpu_addr == 0x8000) fetched_halt = true;
            old_insn = d.cpu_insn_start;
        }
        if (d.mem_wr && !d.owner_reset) memory.at(d.mem_addr) = d.mem_dout;
        if (d.load_mem_wr) memory.at(d.load_mem_addr) = d.load_data;
        d.clk = 1; d.eval(); d.clk = 0; d.eval();
    }
    void run(unsigned count) { while (count--) tick(); }
    void load(unsigned mode, unsigned r2) {
        const auto s = snapshot(mode, r2);
        run(1024); d.reset_btn = 0; d.sna_download = 1; run(4);
        for (unsigned i = 0; i < s.size(); ++i) {
            d.load_wr = 1; d.load_addr = i; d.load_data = s[i]; tick();
            d.load_wr = 0; tick();
        }
        d.sna_download = 0;
        unsigned remaining = 1000;
        while (!d.sna_load && remaining--) tick();
        require(d.sna_load, "normal SNA owner did not apply"); tick();
    }
    std::vector<Sample> body_window() {
        unsigned frames = 0;
        bool left_origin = false, observing = false;
        std::vector<Sample> samples;
        for (unsigned n = 0; n < 5000000; ++n) {
            if (!observing && frames >= 2 && d.row == 6 && d.raster == 0 && d.hcc == 18)
                observing = true;
            if (observing) {
                if (d.hcc >= 30) {
                    require(fetched_halt && !d.cpu_halt_n,
                            "production CPU did not naturally fetch/HALT at8000");
                    return samples;
                }
                samples.push_back({d.rgb, d.pen, bool(d.hs), bool(d.vs), bool(d.de)});
            }
            tick();
            if (d.row || d.raster) left_origin = true;
            if (left_origin && !d.row && !d.raster) { ++frames; left_origin = false; }
        }
        throw std::runtime_error("natural frame/body observation timed out");
    }
};
std::vector<unsigned> expected_pens(unsigned mode) {
    std::vector<unsigned> pens;
    for (unsigned byte : markers) {
        for (unsigned pixel = 0; pixel < (mode == 1 ? 4u : 8u); ++pixel) {
            // Independent CPC byte bitplane decode. Mode1: {b3,b7}..{b0,b4}.
            const unsigned pen = mode == 1 ? ((byte >> (7-pixel)) & 1) |
                                               (((byte >> (3-pixel)) & 1) << 1)
                                           : (byte >> (7-pixel)) & 1;
            pens.insert(pens.end(), clocks_per_dot * (mode == 1 ? 2 : 1), pen);
        }
    }
    return pens;
}
unsigned boundary_case(unsigned mode, unsigned& failures) {
    Bench control; control.load(mode, 46); const auto u = control.body_window();
    Bench blanked; blanked.load(mode, 22); const auto b = blanked.body_window();
    const auto pens = expected_pens(mode);
    require(u.size() == b.size() && pens.size() == 192, "matched window size");
    std::vector<unsigned> origins;
    for (unsigned t = 0; t + pens.size() <= u.size(); ++t) {
        bool match = true;
        for (unsigned i = 0; i < pens.size(); ++i)
            if (u[t+i].rgb != colours[pens[i]] || u[t+i].pen != pens[i]) { match = false; break; }
        if (match) origins.push_back(t);
    }
    require(origins.size() == 1, "unblanked six-byte origin must match all192 RGB/PEN slots uniquely");
    const unsigned origin = origins.front();
    unsigned mismatch = 0, rise = 0, fall = 0;
    for (unsigned i = 1; i < b.size(); ++i) {
        if (b[i].hs && !b[i-1].hs) rise = i;
        if (!b[i].hs && b[i-1].hs) fall = i;
    }
    require(fall > rise && fall - rise == 2 * 16 * clocks_per_dot,
            "R3=2 raw HSYNC width must remain two character clocks");
    for (unsigned i = 0; i < pens.size(); ++i) {
        const bool black = i >= first_black_dot * clocks_per_dot &&
                           i < first_visible_dot * clocks_per_dot;
        const auto& sample = b.at(origin + i);
        require(sample.pen == pens[i] && sample.de, "blank must not corrupt complete byte/pen stream or DE");
        require(sample.vs == u[origin+i].vs, "ordinary body VSYNC control changed");
        const unsigned expected = black ? 0 : colours[pens[i]];
        if (sample.rgb != expected) {
            if (!mismatch) std::cout << "FAIL Mode" << mode << " source slot" << i
                << " expectedRGB=" << std::hex << expected << " actual=" << sample.rgb << std::dec << '\n';
            ++mismatch;
        }
    }
    failures += mismatch;
    std::cout << "Mode" << mode << " origin=" << origin << " full192 RGB/PEN control; rawHS="
              << rise << ".." << fall << "; reference black=" << origin+60 << ".." << origin+188
              << " mismatches=" << mismatch << '\n';
    return origin;
}

// Different contract: SNA_LOAD can replace the active pin without reset.
// Observe public RGB; do not expose/history-test individual pipeline stages.
struct Lifecycle : Bench {
    unsigned checks = 0, failures = 0;
    void edge(bool pix = false, bool ce = false) { d.life_pix = pix; d.life_ce = ce; tick(); }
    void dot() { for (int i = 0; i < 3; ++i) edge(); edge(true); }
    void apply(bool hs, unsigned hcc = 10, unsigned hsw = 0) {
        d.life_hs = hs; d.life_hcc = hcc; d.life_hsw = hsw;
        d.life_load = 1; edge(); d.life_load = 0;
        require(bool(d.life_raw_hs) == hs, "SNA lifecycle stimulus did not restore accepted HS");
    }
    void check_dots(bool black, unsigned n, const char* reason) {
        while (n--) {
            dot(); ++checks;
            if (d.life_rgb != (black ? 0 : 0xfff)) {
                if (!failures) std::cout << "FAIL lifecycle " << reason << '\n';
                ++failures;
            }
        }
    }
    void rise() { apply(false, 21); edge(false, true); require(d.life_raw_hs, "raw rise stimulus"); }
    void fall() { apply(true, 10, 1); edge(false, true); require(!d.life_raw_hs, "raw fall stimulus"); }
    void reset_pending(const char* name) {
        d.life_reset_n = 0; edge();
        require(!d.life_raw_hs && d.life_rgb == 0, "reset must clear rawHS and forceRGBblack");
        d.life_reset_n = 1; check_dots(false, 20, name);
    }
    unsigned exercise() {
        edge(); d.life_reset_n = 1;
        apply(false); check_dots(false, 1, "initial inactive restore");
        apply(true); check_dots(true, 20, "incomingHS1 vs oldHS0 must not leak during history fill");
        apply(false); check_dots(false, 20, "incomingHS0 must discard prior active blank");
        rise(); check_dots(false, 5, "onset is still pending before replacement");
        apply(false); check_dots(false, 20, "SNA cancels stale pending onset");
        fall(); check_dots(true, 5, "fall is still pending before replacement");
        apply(true); check_dots(true, 20, "SNA cancels stale pending fall");
        reset_pending("reset cancels active blank");
        rise(); for (int i = 0; i < 5; ++i) dot(); reset_pending("reset cancels pending onset");
        fall(); for (int i = 0; i < 5; ++i) dot(); reset_pending("reset cancels pending fall");
        std::cout << "Lifecycle " << checks << " publicRGB checks, failures=" << failures << '\n';
        return failures;
    }
};
} // namespace
int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    try {
        unsigned failures = 0;
        const unsigned one = boundary_case(1, failures), two = boundary_case(2, failures);
        require(one == two, "French §9.2.1:40489 must not introduce a Mode2 one-dot advance");
        Lifecycle life; failures += life.exercise();
        require(!failures, "ASIC blank alignment/reference or lifecycle mismatch");
        std::cout << "asic-blank-alignment: PASS 2 complete mode windows + SNA/reset lifecycle\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "asic-blank-alignment: FAIL " << e.what() << '\n'; return 1;
    }
}
