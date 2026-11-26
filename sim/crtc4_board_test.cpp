// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND 4.0), French v1.11.

#include "Vb18_snapshot_top.h"
#include "verilated.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <vector>

namespace {

constexpr unsigned ram_base = 0x20000;
constexpr unsigned result_addr = 0xa000;
constexpr unsigned irq_count_addr = result_addr + 6;
constexpr unsigned rom_probe = 0x01d0;
constexpr unsigned clocks_per_line = 64 * 64;
constexpr unsigned clocks_per_frame = 39 * 8 * clocks_per_line;
// With R12/R13 = &3020, the white patch is within the first visible raster
// (FR §§17.4.3 p185 and 20.2 p241). The address is a word offset in RAM.
constexpr unsigned patch_addr = 0x6028;
constexpr unsigned patch_words = 8;

std::vector<uint8_t> program() {
    std::vector<uint8_t> bytes;
    auto emit = [&](std::initializer_list<uint8_t> op) { bytes.insert(bytes.end(), op); };
    auto out = [&](unsigned port, uint8_t value) {
        emit({0x01, uint8_t(port), uint8_t(port >> 8), 0x3e, value, 0xed, 0x79});
    };
    auto store_result = [&](unsigned offset) {
        const unsigned address = result_addr + offset;
        emit({0x32, uint8_t(address), uint8_t(address >> 8)});
    };
    auto read_port = [&](unsigned port, unsigned offset) {
        emit({0x01, uint8_t(port), uint8_t(port >> 8), 0xed, 0x78});
        store_result(offset);
    };
    auto read_rom_probe = [&](unsigned offset) {
        emit({0x3a, uint8_t(rom_probe), uint8_t(rom_probe >> 8)});
        store_result(offset);
    };

    emit({0xf3}); // DI
    read_rom_probe(0); // Lower ROM enabled at reset.
    out(0x7f00, 0x9e); // Mode 2, both ROMs off, clear the interrupt counter.
    read_rom_probe(1); // Same CPU address now reads the underlying RAM.
    out(0x7f00, 0x00); out(0x7f00, 0x40); // Pen 0: grey.
    out(0x7f00, 0x01); out(0x7f00, 0x4b); // Pen 1: bright white.
    read_port(0x7f00, 5); // The 40226 leaves this selected ink intact.
    out(0x7f00, 0x10); out(0x7f00, 0x40); // Border: grey.

    constexpr std::array<uint8_t, 16> regs = {
        63, 40, 46, 0x8e, 38, 0, 25, 30, 0, 7, 0, 0, 0x30, 0x20, 0xd6, 0xa5
    };
    for (unsigned reg = 0; reg < regs.size(); ++reg) {
        out(0xbc00, reg);
        out(0xbd00, regs[reg]);
    }

    // FR §§21.2.3 p246 and 28.1.9 p293: reads decode only the low three
    // register-select bits. Both BE00 and BF00 use the same read table.
    out(0xbc00, 6);
    read_port(0xbf00, 2); // R6 reads the six-bit R14 cursor address high.
    out(0xbc00, 7);
    read_port(0xbe00, 3); // R7 reads the full R15 via the mirror port.

    out(0xf700, 0x80); // 8255 mode 0, all ports output.
    out(0xf500, 0x69); // Write Port B and read its output latch back.
    read_port(0xf500, 4);

    emit({0x31, 0xf0, 0xbf}); // LD SP,&BFF0
    emit({0xed, 0x56, 0xfb}); // IM 1; EI
    emit({0x76, 0x76}); // Wait for two interrupt acknowledges.
    emit({0xf3, 0x76}); // DI; HALT
    return bytes;
}

} // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vb18_snapshot_top d;
    std::vector<uint8_t> memory(0x800000, 0xff);
    const auto code = program();
    const std::array<uint8_t, 3> reset_rom = {0xc3, 0x00, 0x80}; // JP &8000.
    // Allow the cold-reset CPU/GA phases to settle before the first operand read.
    std::fill_n(memory.begin(), 16, 0);
    std::copy(reset_rom.begin(), reset_rom.end(), memory.begin() + 16);
    std::copy(code.begin(), code.end(), memory.begin() + ram_base + 0x8000);
    memory.at(rom_probe) = 0xc9;
    memory.at(ram_base + rom_probe) = 0x26;

    constexpr std::array<uint8_t, 12> im1_isr = {
        0xf5, // PUSH AF
        0x3a, uint8_t(irq_count_addr), uint8_t(irq_count_addr >> 8), // LD A,(count)
        0x3c, // INC A
        0x32, uint8_t(irq_count_addr), uint8_t(irq_count_addr >> 8), // LD (count),A
        0xf1, // POP AF
        0xfb, 0xed, 0x4d // EI; RETI
    };
    std::copy(im1_isr.begin(), im1_isr.end(), memory.begin() + ram_base + 0x38);
    uint8_t& irq_count = memory.at(ram_base + irq_count_addr);
    irq_count = 0;

    std::fill_n(memory.begin() + ram_base + 0xc000, 0x4000, 0);
    std::fill_n(memory.begin() + ram_base + 2 * patch_addr, 2 * patch_words, 0xff);

    d.pause = 0;
    d.filter_request = 0;
    d.model = 0; // Classic CPC 6128.
    d.crtc_type = 1;
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

    // Main holds status[0] high while replaying saved OSD settings and
    // loading the boot ROM. The saved Type 4 arrives after reset starts.
    d.reset_btn = 1;
    for (unsigned i = 0; i < 512; ++i) tick();
    d.crtc_type = 3;
    for (unsigned i = 0; i < 512; ++i) tick();
    d.reset_btn = 0;
    for (unsigned i = 0; i < 16 * clocks_per_line && d.cpu_halt_n; ++i) tick();
    if (d.cpu_halt_n) {
        std::cerr << "CRTC4 board: reset-ROM program did not HALT, CPU address 0x"
                  << std::hex << d.cpu_addr << '\n';
        return 1;
    }

    constexpr std::array<uint8_t, 6> expected = {0xc9, 0x26, 0x16, 0xa5, 0x69, 0xff};
    constexpr std::array<const char*, 6> reads = {
        "lower ROM enabled", "lower ROM disabled", "R6 alias on BF00",
        "R7 alias on BE00", "8255 Port B output latch", "GA read on 7F00"
    };
    bool pass = true;
    for (unsigned i = 0; i < expected.size(); ++i) {
        const uint8_t got = memory.at(ram_base + result_addr + i);
        if (got != expected[i]) {
            std::cerr << "CRTC4 board: " << reads[i] << " read 0x" << std::hex
                      << unsigned(got) << ", expected 0x" << unsigned(expected[i]) << '\n';
            pass = false;
        }
    }

    // Allow the initial programmed frame to settle and observe distinct
    // interrupt deliveries. FR §§27.1 p283 and 27.7.1 p288 tie rearm to
    // multiple HSYNCs. A full scanline between services rejects a stuck INT.
    unsigned first_irq_clock = 0, second_irq_clock = 0;
    for (unsigned clock = 1; clock <= clocks_per_frame; ++clock) {
        tick();
        if (irq_count == 1 && !first_irq_clock) first_irq_clock = clock;
        if (irq_count == 2 && !second_irq_clock) second_irq_clock = clock;
    }
    const unsigned irq_gap = second_irq_clock > first_irq_clock ? second_irq_clock - first_irq_clock : 0;
    if (irq_count != 2 || !first_irq_clock || irq_gap < clocks_per_line) {
        std::cerr << "CRTC4 board: IM1 serviced " << std::dec << unsigned(irq_count)
                  << " interrupts, with " << irq_gap << " clocks between the first two\n";
        pass = false;
    }

    // Observe the interior of the patch on the first visible raster.
    bool white_patch = false;
    for (unsigned i = 0; i < 2 * clocks_per_frame && !white_patch; ++i) {
        tick();
        white_patch = d.video_de && !d.video_hs && d.video_rgb == 0x222 &&
                      d.video_addr > patch_addr && d.video_addr < patch_addr + patch_words - 1;
    }
    if (!white_patch) {
        std::cerr << "CRTC4 board: white VRAM patch did not reach the motherboard RGB output\n";
        pass = false;
    }

    if (pass) {
        std::cout << "CRTC4 board PASS: ROM, CRTC aliases, 8255, GA read/VRAM RGB, and "
                  << unsigned(irq_count) << " IM1 services " << irq_gap << " clocks apart\n";
    }
    return pass ? 0 : 1;
}
