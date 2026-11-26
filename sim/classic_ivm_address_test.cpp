// Technical information from the Amstrad CPC CRTC Compendium by Longshot
// (CC BY-NC-ND 4.0), French v1.11 §19.8.1 pp. 220–221. The first VRAM
// pair of the raster following an admitted R8 change has that raster's
// C9.VMA. This test takes the real T80/CRTC/GA/RAM first-fetch path.
#include "Vclassic_mode_boundary_top.h"
#include "verilated.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Program {
    std::vector<uint8_t> rom = std::vector<uint8_t>(16'384, 0xff);
    unsigned enter_pc, leave_pc;
    const bool freeze_late;

    Program(bool row_carry, bool freeze) : freeze_late(freeze) {
        unsigned cursor = 0x100;
        auto byte = [&](unsigned value) { rom.at(cursor++) = value; };
        auto out = [&](unsigned port, unsigned value) {
            byte(0x01); byte(port); byte(port >> 8); // LD BC,nn
            byte(0x3e); byte(value);               // LD A,n
            byte(0xed); byte(0x79);                // OUT (C),A
        };
        rom[0] = 0xc3; rom[1] = 0; rom[2] = 1;      // JP &0100
        byte(0xf3);                                // DI
        const std::array<uint8_t, 14> regs = {
            63, 40, 46, 0x4e, 38, 0, 25, uint8_t(row_carry ? 2 : 1), 0, 7, 0, 0, 0x30, 0
        };
        for (unsigned reg = 0; reg < regs.size(); ++reg) {
            out(0xbc00, reg);
            out(0xbd00, regs[reg]);
        }
        out(0xf700, 0x82);
        out(0xbc00, 8);
        byte(0x01); byte(0x00); byte(0xf5);         // LD BC,&F500
        byte(0xed); byte(0x78); byte(0x1f);         // IN A,(C); RRA
        byte(0x38); byte(0xfb);                     // wait for VS low
        byte(0xed); byte(0x78); byte(0x1f);
        byte(0x30); byte(0xfb);                     // wait for VS rise
        byte(0x01); byte(0x00); byte(0xbd);         // LD BC,&BD00
        byte(0x3e); byte(3);                        // LD A,3
        for (unsigned n = 0; n < (row_carry ? 448u : 192u); ++n) byte(0);
        enter_pc = cursor;
        byte(0xed); byte(0x79);
        if (freeze_late) {
            out(0xbc00, 0); // select R0 and preload A=0
            byte(0x01); byte(0x00); byte(0xbd);
            for (unsigned n = 0; n < 34; ++n) byte(0);
        } else {
            byte(0x3e); byte(0);
            for (unsigned n = 0; n < 116; ++n) byte(0);
        }
        leave_pc = cursor;
        byte(0xed); byte(0x79);
        byte(0x76);                                 // HALT
    }
};

// Distinct bytes in each physical RA plane in ordinary CPC C000 RAM.
uint16_t tag(unsigned word_addr) {
    const unsigned plane = (word_addr >> 10) & 7;
    return uint16_t(0x4061u + 0x0809u * plane);
}

struct Raster {
    unsigned row, line, first_addr;
    unsigned data = 0;
    bool second_byte = false;
};

struct Bench {
    Vclassic_mode_boundary_top top;
    Program program;
    std::vector<uint8_t> ram = std::vector<uint8_t>(8'388'608, 0xff);
    std::vector<Raster> rasters;
    std::array<unsigned, 2> write_index{};
    unsigned writes = 0;
    unsigned pc = 0;
    bool old_insn = false, old_cycle = false, old_write = false;

    explicit Bench(bool row_carry, bool freeze_late=false) : program(row_carry,freeze_late) {
        std::copy(program.rom.begin(), program.rom.end(), ram.begin());
        top.reset_btn = 1;
        top.pause = top.filter_request = top.save_req = top.cart_ack = 0;
        top.crtc_type = top.sna_download = top.load_wr = 0;
        top.load_addr = top.load_data = 0;
        top.cart_dout = 255;
    }

    void tick() {
        top.clk = 0;
        top.eval();
        top.mem_din = ram.at(top.mem_addr);
        top.video_word = tag(top.video_addr);
        top.eval();
        if (top.cpu_insn_start && !old_insn) pc = top.cpu_addr;
        old_insn = top.cpu_insn_start;
        if (top.dbg_write && !old_write && (pc == program.enter_pc || pc == program.leave_pc)) {
            const unsigned reg = writes == 1 && program.freeze_late ? 0 : 8;
            require(writes < write_index.size() && top.crtc_reg == reg &&
                    top.crtc_data == (writes == 0 ? 3 : 0), "missing accepted R8 sequence");
            std::cout << "R" << reg << "=" << unsigned(top.crtc_data) << " C0/C4/C9="
                      << unsigned(top.c0) << "/" << unsigned(top.c4) << "/"
                      << unsigned(top.c9) << '\n';
            require(!top.owner_reset, "guest write occurred in reset");
            if (reg==0) {
                std::cout << "late R0 master phase=" << unsigned(top.dbg_char_phase) << '\n';
                require(top.c0==0 && top.dbg_char_phase < 36, "R0 write is outside new C0 before first CAS");
            } else {
                require(top.c0 > 2 && top.c0 < 60, "R8 write outside ordinary raster aperture");
            }
            write_index[writes++] = rasters.size();
        }
        old_write = top.dbg_write;
        const bool first_phase = top.fetch && !top.bs;
        const bool second_phase = top.fetch && top.bs;
        if (!top.owner_reset && first_phase && !old_cycle && top.c0 == 0) {
            rasters.push_back({top.c4, top.c9, top.video_addr});
        }
        const bool take_second = !rasters.empty() && second_phase &&
                                 !rasters.back().second_byte && top.c0 == 0;
        old_cycle = first_phase;
        if (top.mem_wr && !top.owner_reset && !top.reset_btn) ram.at(top.mem_addr) = top.mem_dout;
        top.clk = 1;
        top.eval();
        if (take_second) {
            rasters.back().data = top.byte_out;
            rasters.back().second_byte = true;
        }
    }

    void run() {
        for (unsigned tick = 0; tick < 1024; ++tick) this->tick();
        top.reset_btn = 0;
        for (unsigned tick = 0; tick < 2'000'000; ++tick) {
            this->tick();
            if (writes == 2 && rasters.size() >= write_index[1]+2 && rasters.back().second_byte)
                return;
        }
        throw std::runtime_error("T80 did not reach both transitions and VRAM fetches");
    }
};

bool check_raster(const char* name, const Raster& r, unsigned row, unsigned line, unsigned expected_ra) {
    require(r.row == row && r.line == line && r.second_byte, "raster or second CAS setup");
    const unsigned expected_addr = 0x6000 + row * 40 + (expected_ra & 7) * 1024;
    const unsigned expected_byte = tag(expected_addr) >> 8;
    std::cout << name << " first address $" << std::hex << r.first_addr
              << " high byte $" << r.data << " expected $" << expected_addr
              << "/$" << expected_byte << std::dec << '\n';
    return r.first_addr == expected_addr && r.data == expected_byte;
}
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    try {
        bool ok = true;
        for (bool row_carry : {false, true}) {
            Bench b(row_carry);
            b.run();
            const auto entry = b.write_index[0];
            const auto exit = b.write_index[1];
            const unsigned old_row = row_carry ? 2 : 1;
            const unsigned new_row = row_carry ? 3 : 1;
            const unsigned entry_line = row_carry ? 0 : 4;
            const unsigned exit_line = row_carry ? 2 : 6;
            ok &= check_raster("ordinary switch raster", b.rasters.at(entry-1),
                               old_row, row_carry ? 7 : 3, row_carry ? 7 : 3);
            ok &= check_raster("steady IVM", b.rasters.at(entry+1),
                               new_row, entry_line+1, 2*(entry_line+1)+1);
            ok &= check_raster("IVM exit's departing raster", b.rasters.at(exit-1),
                               new_row, exit_line-1, 2*(exit_line-1)+1);
            ok &= check_raster("following ordinary raster", b.rasters.at(exit+1),
                               new_row, exit_line+1, exit_line+1);
            ok &= check_raster("first IVM fetch", b.rasters.at(entry),
                               new_row, entry_line, 2*entry_line+1);
            ok &= check_raster("first ordinary fetch after exit", b.rasters.at(exit),
                               new_row, exit_line, exit_line);
        }
        Bench freeze(false,true);
        freeze.run();
        require(freeze.write_index[0] == freeze.write_index[1],
                "accepted C0=0 R0 write must precede the first VRAM CAS of the incoming line");
        const auto frozen = freeze.write_index[1];
        ok &= check_raster("R0 becomes zero in new C0 before first CAS", freeze.rasters.at(frozen), 1, 4, 9);
        ok &= check_raster("next frozen character fetch", freeze.rasters.at(frozen+1), 1, 4, 9);
        if (!ok) return 1;
    } catch (const std::exception& error) {
        std::cerr << "classic-ivm-address: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
