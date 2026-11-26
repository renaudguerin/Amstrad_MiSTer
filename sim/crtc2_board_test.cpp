// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND 4.0), French v1.11.

#include "Vb18_snapshot_top.h"
#include "verilated.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <vector>

namespace {

constexpr unsigned ram_base = 0x20000;
constexpr unsigned result_addr = 0xa000;
constexpr unsigned clocks_per_character = 64;
constexpr unsigned characters_per_line = 64;
constexpr unsigned clocks_per_line = characters_per_line * clocks_per_character;
constexpr unsigned lines_per_frame = 39 * 8;
constexpr unsigned clocks_per_frame = lines_per_frame * clocks_per_line;
// FR §§17.4.3 p.185 and 20.2 p.241: VMA=0x3020 and C9=0/1 map to
// byte addresses C040/C840 on the first two rasters of a settled frame.
constexpr unsigned scanline_zero = 0x6020;
constexpr unsigned scanline_one = 0x6420;
constexpr unsigned displayed_characters = 40;

std::vector<uint8_t> program() {
    std::vector<uint8_t> bytes;
    auto emit = [&](std::initializer_list<uint8_t> op) { bytes.insert(bytes.end(), op); };
    auto out = [&](unsigned port, uint8_t value) {
        emit({0x01, uint8_t(port), uint8_t(port >> 8), 0x3e, value, 0xed, 0x79});
    };
    auto read_result = [&](unsigned port, unsigned offset) {
        const unsigned address = result_addr + offset;
        emit({0x01, uint8_t(port), uint8_t(port >> 8), 0xed, 0x78,
              0x32, uint8_t(address), uint8_t(address >> 8)});
    };

    emit({0xf3}); // DI
    out(0x7f00, 0x9e); // Mode 2, both ROMs off, clear the interrupt counter.
    out(0x7f00, 0x00); out(0x7f00, 0x40); // Pen 0: hardware ink 0 (grey).
    out(0x7f00, 0x01); out(0x7f00, 0x41); // Pen 1: hardware ink 1 (white).
    out(0x7f00, 0x10); out(0x7f00, 0x40); // Border: hardware ink 0 (grey).

    // Standard 64 us lines and 312 lines per frame, with R3=0. The display
    // begins at word 0x3020, with 40 words per character row and 8 rasters.
    constexpr std::array<uint8_t, 14> regs = {
        63, displayed_characters, 46, 0, 38, 0, 25, 30, 0, 7, 0, 0, 0x30, 0x20
    };
    for (unsigned reg = 0; reg < regs.size(); ++reg) {
        out(0xbc00, reg);
        out(0xbd00, regs[reg]);
    }

    // FR §4.4.1 pp.22–23: the low address byte is free on these ports.
    // FR §21.2.2 p.245: register select ignores the high three bits and R14
    // has six data bits. §28.1.9 p.293 differs on R14/15 availability. This
    // fixture uses the provisional readback observed on AmSpirit Lite.
    out(0xbc5a, 0xee); // R14 via alias.
    out(0xbdc3, 0xff);
    read_result(0xbe55, 0);
    read_result(0xbf19, 1);
    out(0xbcdd, 0x0f); // R15 via canonical index.
    out(0xbdab, 0xa5);
    out(0xbc00, 0xaf); // Read R15 via a different index alias.
    read_result(0xbfea, 2);
    out(0xbc00, 12);
    read_result(0xbf00, 3);
    out(0xbc00, 0xff); // R31.
    read_result(0xbf00, 4);
    emit({0x76}); // HALT
    return bytes;
}

} // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vb18_snapshot_top d;
    std::vector<uint8_t> memory(0x800000, 0xff);
    const auto code = program();
    const std::array<uint8_t, 3> reset_rom = {0xc3, 0x00, 0x80}; // JP &8000.
    // Let the cold-reset CPU/GA phases settle before the first operand read.
    std::fill_n(memory.begin(), 16, 0);
    std::copy(reset_rom.begin(), reset_rom.end(), memory.begin() + 16);
    std::copy(code.begin(), code.end(), memory.begin() + ram_base + 0x8000);
    std::fill_n(memory.begin() + ram_base + result_addr, 5, 0x5a);

    // A uniquely white range on the first two rasters. Everything else
    // supplies pen 0, so white needs real VRAM data as well as GA DISPEN.
    std::fill_n(memory.begin() + ram_base + 0xc000, 0x4000, 0);
    std::fill_n(memory.begin() + ram_base + 2 * scanline_zero, 2 * displayed_characters, 0xff);
    std::fill_n(memory.begin() + ram_base + 2 * scanline_one, 2 * displayed_characters, 0xff);

    d.pause = 0;
    d.filter_request = 0;
    d.model = 0;
    d.crtc_type = 0;
    d.save_req = 0;
    d.sna_download = 0;
    d.load_wr = 0;
    d.load_addr = 0;
    d.load_data = 0;
    d.cart_dout = 0xff;
    d.cart_ack = 0;
    d.ddram_busy = 0;
    d.video_word = 0;
    d.mem_din = 0xff;

    auto tick = [&] {
        d.clk = 0;
        d.eval();
        d.mem_din = memory.at(d.mem_addr);
        const unsigned video = ram_base + 2 * d.video_addr;
        d.video_word = memory.at(video) | (unsigned(memory.at(video + 1)) << 8);
        d.eval();
        if (d.mem_wr) memory.at(d.mem_addr) = d.mem_dout;
        d.clk = 1;
        d.eval();
    };

    d.reset_btn = 1;
    for (unsigned i = 0; i < 1024; ++i) tick();
    d.reset_btn = 0;
    for (unsigned i = 0; i < 16 * clocks_per_line && d.cpu_halt_n; ++i) tick();
    if (d.cpu_halt_n) {
        std::cerr << "CRTC2 board: synthetic reset-ROM program did not HALT, CPU address 0x"
                  << std::hex << d.cpu_addr << '\n';
        return 1;
    }

    // FR §21.3.2 p.247: CRTC2 leaves the status port undriven, yielding FF
    // on this board fixture's otherwise undriven bus.
    constexpr std::array<uint8_t, 5> expected = {0xff, 0x3f, 0xa5, 0x00, 0x00};
    constexpr std::array<const char*, 5> reads = {"status", "R14", "R15", "R12", "R31"};
    bool pass = true;
    for (unsigned i = 0; i < expected.size(); ++i) {
        const uint8_t got = memory.at(ram_base + result_addr + i);
        if (got != expected[i]) {
            std::cerr << "CRTC2 board: " << reads[i] << " RAM result 0x" << std::hex
                      << unsigned(got) << " expected 0x" << unsigned(expected[i]) << std::dec << '\n';
            pass = false;
        }
    }

    // Initial register programming can span a partial frame. Measure a full
    // settled frame, ignoring the partial HSYNC at either observation edge.
    for (unsigned i = 0; i < 2 * clocks_per_frame; ++i) tick();
    bool hs = d.video_hs;
    bool vs = d.video_vs;
    unsigned rise = 0, pulses = 0, wrong_width = 0, wrong_period = 0;
    unsigned last_width = 0, nonblack_in_sync = 0;
    unsigned vs_clocks = 0, vs_rises = 0, vs_falls = 0;
    bool first_address = false, second_address = false;
    bool first_white = false, second_white = false;
    for (unsigned clock = 1; clock <= clocks_per_frame; ++clock) {
        tick();
        vs_clocks += d.video_vs;
        vs_rises += d.video_vs && !vs;
        vs_falls += !d.video_vs && vs;
        vs = d.video_vs;
        if (d.video_hs && !hs) {
            if (rise && clock - rise != clocks_per_line) ++wrong_period;
            rise = clock;
        }
        if (!d.video_hs && hs && rise) {
            last_width = clock - rise;
            ++pulses;
            // FR §14.6 p.143: R3=0 gives 16 us on CRTC2 and no pulse on
            // types 0/1. Measure at the real GA's input in 64 MHz ticks.
            if (last_width != 16 * clocks_per_character) ++wrong_width;
        }
        // FR §14.3 p.134: the GA blanks during CRTC HSYNC. Skip the first
        // character to allow colour pipeline latency without pinning a dot.
        if (d.video_hs && rise && clock - rise >= clocks_per_character && d.video_rgb != 0)
            ++nonblack_in_sync;
        if (d.video_de && !d.video_hs) {
            first_address |= d.video_addr == scanline_zero;
            second_address |= d.video_addr == scanline_one;
            if (d.video_rgb == 0x3f) {
                first_white |= d.video_addr >= scanline_zero && d.video_addr < scanline_zero + displayed_characters;
                second_white |= d.video_addr >= scanline_one && d.video_addr < scanline_one + displayed_characters;
            }
        }
        hs = d.video_hs;
    }
    if (pulses < lines_per_frame - 1 || wrong_width || wrong_period) {
        std::cerr << "CRTC2 board: GA HSYNC got " << pulses << " pulses, " << wrong_width
                  << " wrong widths, " << wrong_period << " wrong periods, last width "
                  << last_width << " clocks\n";
        pass = false;
    }
    // FR §16.4.3 p.171: normal VSYNC ends on line 16. R2=46 and a
    // 16-character pulse leave the C0=0 start of VSYNC outside HSYNC.
    if (vs_clocks != 16 * clocks_per_line || vs_rises != 1 || vs_falls != 1) {
        std::cerr << "CRTC2 board: GA VSYNC high for " << vs_clocks
                  << " clocks, expected " << 16 * clocks_per_line
                  << ", rising/falling edges " << vs_rises << '/' << vs_falls << '\n';
        pass = false;
    }
    if (nonblack_in_sync || !first_address || !second_address || !first_white || !second_white) {
        std::cerr << "CRTC2 board: GA nonblack clocks during HSYNC=" << nonblack_in_sync
                  << ", DISPEN plus raster addresses=" << first_address << '/' << second_address
                  << ", white VRAM pixels during DISPEN=" << first_white << '/' << second_white << '\n';
        pass = false;
    }

    if (pass) std::cout << "CRTC2 board PASS: CPU I/O reads, GA HSYNC/VSYNC, blackout and VRAM display\n";
    return pass ? 0 : 1;
}
