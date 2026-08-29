#include "Vp10_boot_test_top.h"
#include "verilated.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr uint8_t CMD_ACTIVE = 0b011;
constexpr uint8_t CMD_READ   = 0b101;
constexpr uint8_t CMD_WRITE  = 0b100;
constexpr unsigned kIoctlWaitLimit = 20000000;

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void require(bool condition, const std::string &message) {
    if (!condition) throw TestFailure(message);
}

struct Chunk {
    std::string id;
    std::vector<uint8_t> data;
};

std::vector<uint8_t> build_cpr_image(const std::vector<Chunk> &chunks, const std::string &form_type = "AMS!") {
    std::vector<uint8_t> image{'R', 'I', 'F', 'F'};
    uint32_t riff_len = 4;
    for (const auto &chunk : chunks) {
        riff_len += 8 + static_cast<uint32_t>(chunk.data.size());
        if (chunk.data.size() % 2 != 0) ++riff_len;
    }
    for (int i = 0; i < 4; ++i)
        image.push_back(static_cast<uint8_t>((riff_len >> (8 * i)) & 0xff));
    for (size_t i = 0; i < 4; ++i) {
        image.push_back(i < form_type.size() ? static_cast<uint8_t>(form_type[i]) : 0x20);
    }
    for (const auto &chunk : chunks) {
        for (size_t i = 0; i < 4; ++i)
            image.push_back(i < chunk.id.size() ? static_cast<uint8_t>(chunk.id[i]) : 0x20);
        const uint32_t len = static_cast<uint32_t>(chunk.data.size());
        for (int i = 0; i < 4; ++i)
            image.push_back(static_cast<uint8_t>((len >> (8 * i)) & 0xff));
        image.insert(image.end(), chunk.data.begin(), chunk.data.end());
        if (chunk.data.size() % 2 != 0) image.push_back(0x00);
    }
    return image;
}

class Harness {
public:
    Vp10_boot_test_top dut;
    std::unordered_map<uint32_t, uint8_t> memory;
    uint64_t cycles = 0;
    uint32_t physical_cart_reads = 0;

    Harness() {
        dut.clk = 0;
        dut.clkref = 0;
        dut.init = 1;
        dut.reset_btn = 0;
        dut.plus_model_i = 2; // 6128+
        dut.cpr_download = 0;
        dut.ioctl_wr = 0;
        dut.ioctl_addr = 0;
        dut.ioctl_dout = 0;
        dut.memory_dq = 0;
        dut.memory_dq_oe = 0;
        dut.force_irq = 0;
        dut.eval();
    }

    ~Harness() { dut.final(); }

    void tick() {
        dut.clkref = ((cycles & 7U) == 0U);
        dut.memory_dq_oe = read_drive_cycles > 0;
        dut.memory_dq = read_word;

        dut.clk = 0;
        dut.eval();
        dut.clk = 1;
        dut.eval();

        bool started_read = false;
        const uint8_t cmd = command();
        if (cmd == CMD_ACTIVE) {
            active_row = dut.sdram_a & 0x1fffU;
            active_bank = dut.sdram_ba;
        } else if (cmd == CMD_READ) {
            const uint32_t address = command_address();
            read_word = static_cast<uint16_t>(load(active_bank, address)) |
                        (static_cast<uint16_t>(load(active_bank, address + 1)) << 8);
            read_drive_cycles = 4;
            started_read = true;
            if (active_bank == 3) ++physical_cart_reads;
        } else if (cmd == CMD_WRITE) {
            const uint32_t address = command_address();
            const uint16_t data = dut.observed_dq;
            if (!dut.sdram_dqml) store(active_bank, address, data & 0xffU);
            if (!dut.sdram_dqmh) store(active_bank, address + 1, data >> 8);
        }
        if (read_drive_cycles > 0 && !started_read) --read_drive_cycles;

        dut.clk = 0;
        dut.eval();
        ++cycles;
    }

    void initialize() {
        for (int i = 0; i < 16; ++i) tick();
        dut.init = 0;
        for (int i = 0; i < 500; ++i) tick();
    }

    void download(const std::vector<uint8_t> &image) {
        dut.cpr_download = 1;
        tick();
        for (size_t i = 0; i < image.size(); ++i) {
            dut.ioctl_addr = static_cast<uint32_t>(i);
            dut.ioctl_dout = image[i];
            dut.ioctl_wr = 1;
            tick();
            dut.ioctl_wr = 0;
            unsigned wait_count = 0;
            while (dut.ioctl_wait) {
                tick();
                if (++wait_count > kIoctlWaitLimit) {
                    throw TestFailure("ioctl_wait stuck during CPR download");
                }
            }
        }
        dut.cpr_download = 0;
        tick();
    }

    void run_until_pc(uint16_t target_pc, uint64_t max_cycles = 100000) {
        uint64_t start = cycles;
        while (dut.dbg_pc != target_pc) {
            tick();
            if (cycles - start > max_cycles) {
                std::cerr << "Timeout waiting for PC 0x" << std::hex << target_pc
                          << " (current PC: 0x" << dut.dbg_pc << ")\n";
                throw TestFailure("CPU PC timeout");
            }
        }
    }

    void run_cycles(uint64_t n) {
        for (uint64_t i = 0; i < n; ++i) tick();
    }

private:
    uint16_t active_row = 0;
    uint8_t active_bank = 0;
    uint16_t read_word = 0;
    int read_drive_cycles = 0;

    uint8_t command() const {
        return (dut.sdram_nras ? 0b100 : 0b000) |
               (dut.sdram_ncas ? 0b010 : 0b000) |
               (dut.sdram_nwe  ? 0b001 : 0b000);
    }

    uint32_t command_address() const {
        return (static_cast<uint32_t>(active_row) << 10) |
               (static_cast<uint32_t>(dut.sdram_a & 0x1ffU) << 1);
    }

    uint8_t load(uint8_t bank, uint32_t address) {
        const uint32_t key = (static_cast<uint32_t>(bank) << 23) | (address & 0x7fffffU);
        auto it = memory.find(key);
        return (it == memory.end()) ? 0xffU : it->second;
    }

    void store(uint8_t bank, uint32_t address, uint8_t byte) {
        const uint32_t key = (static_cast<uint32_t>(bank) << 23) | (address & 0x7fffffU);
        memory[key] = byte;
    }
};

void test_p10a_deterministic_boot() {
    std::cout << "Running test_p10a_deterministic_boot..." << std::endl;
    Harness h;
    h.initialize();

    // Construct deterministic Z80 test program in cartridge page 0
    std::vector<uint8_t> p0_code(16384, 0x00); // NOP fill

    size_t idx = 0;
    auto emit = [&](uint8_t b) { p0_code[idx++] = b; };

    // 0x0000: DI
    emit(0xF3);
    // 0x0001: LD SP, 0xC000
    emit(0x31); emit(0x00); emit(0xC0);
    // 0x0004: Select Upper ROM page 3 via &DF00: LD BC, &DF00; LD A, 3; OUT (C), A
    emit(0x01); emit(0x00); emit(0xDF);
    emit(0x3E); emit(0x03);
    emit(0xED); emit(0x79);
    // 0x000B: Read byte from upper window 0xC000: LD A, (0xC000)
    emit(0x3A); emit(0x00); emit(0xC0);

    // 0x000E: 16-byte ASIC unlock sequence to &BC00:
    // Sync: FF 00, Sequence: FF 77 B3 51 A8 D4 62 39 9C 46 2B 15 8A CD
    const uint8_t unlock_seq[16] = {
        0xFF, 0x00, 0xFF, 0x77, 0xB3, 0x51, 0xA8, 0xD4,
        0x62, 0x39, 0x9C, 0x46, 0x2B, 0x15, 0x8A, 0xCD
    };
    emit(0x01); emit(0x00); emit(0xBC); // LD BC, &BC00
    for (int i = 0; i < 16; ++i) {
        emit(0x3E); emit(unlock_seq[i]); // LD A, byte
        emit(0xED); emit(0x79);          // OUT (C), A
    }

    // 0x0041: Enable ASIC register page via RMR2: LD BC, &7F00; LD A, &B8; OUT (C), A
    emit(0x01); emit(0x00); emit(0x7F);
    emit(0x3E); emit(0xB8);
    emit(0xED); emit(0x79);

    // 0x0048: Write to ASIC palette register &6420 (Border): LD A, &34; LD (0x6420), A
    emit(0x3E); emit(0x34);
    emit(0x32); emit(0x20); emit(0x64);

    // 0x004D: Write to CRTC3 register (R1 = 40):
    // LD BC, &BC00; LD A, 1; OUT (C), A
    emit(0x01); emit(0x00); emit(0xBC);
    emit(0x3E); emit(0x01);
    emit(0xED); emit(0x79);
    // LD BC, &BD00; LD A, 40; OUT (C), A
    emit(0x01); emit(0x00); emit(0xBD);
    emit(0x3E); emit(0x28);
    emit(0xED); emit(0x79);

    // 0x005B: Write to FDC motor port &FA00 (Motor ON): LD BC, &FA00; LD A, 1; OUT (C), A
    emit(0x01); emit(0x00); emit(0xFA);
    emit(0x3E); emit(0x01);
    emit(0xED); emit(0x79);

    // 0x0062: EI; HALT (Wait for interrupt)
    emit(0xFB); // EI
    emit(0x76); // HALT

    // Page 3 data
    std::vector<uint8_t> p3_data(16384, 0x00);
    p3_data[0] = 0x42; // Magic byte at 0xC000

    std::vector<Chunk> chunks = {
        {"cb00", p0_code},
        {"cb03", p3_data}
    };
    auto image = build_cpr_image(chunks);

    // 1. CPR Download
    h.download(image);

    // 2. Observe reset apply countdown & release
    require(h.dut.dbg_reset == 1, "Reset must be asserted after CPR download");
    while (h.dut.dbg_reset) {
        h.tick();
    }
    require(h.dut.dbg_reset == 0, "Reset must release after apply countdown");

    // 3. Observe real T80 opcode fetch at PC = 0x0000 from cartridge page 0
    std::cout << "  Observing reset-vector execution at PC=0x0000..." << std::endl;
    // Step until M1 memory read phase
    while (h.dut.dbg_m1_n || h.dut.dbg_mreq_n || h.dut.dbg_rd_n) {
        h.tick();
    }
    require(h.dut.dbg_pc == 0x0000, "First instruction PC must be 0x0000");

    // 4. Run until ASIC unlock sequence completes
    std::cout << "  Running through 16-byte ASIC unlock sequence..." << std::endl;
    int last_seq = -1;
    uint16_t last_pc = 0xFFFF;
    int traced = 0;
    for (int i = 0; i < 100000; ++i) {
        h.tick();
        if (getenv("P10RAW") ? (traced < 300) : (getenv("P10TRACE") && h.dut.dbg_pc != last_pc && traced < 200)) {
            last_pc = h.dut.dbg_pc;
            ++traced;
            std::cout << "T pc=" << std::hex << last_pc
                      << " din=" << (int)h.dut.dbg_din
                      << " addr=" << h.dut.dbg_addr
                      << " iorq=" << (int)!h.dut.dbg_iorq_n
                      << " wr=" << (int)!h.dut.dbg_wr_n
                      << " own=" << (int)h.dut.dbg_cart_own
                      << " stall=" << (int)h.dut.dbg_cart_stall
                      << " mc=" << (int)h.dut.dbg_mcycle
                      << " ts=" << (int)h.dut.dbg_tstate
                      << " ir=" << (int)h.dut.dbg_ir
                      << " mcmax=" << (int)h.dut.dbg_mcmax
                      << " waitn=" << (int)h.dut.dbg_cpu_waitn
                      << " cenp=" << (int)h.dut.dbg_cen_p
                      << " mreq=" << (int)!h.dut.dbg_mreq_n
                      << " rd=" << (int)!h.dut.dbg_rd_n
                      << " m1=" << (int)!h.dut.dbg_m1_n
                      << std::dec << std::endl;
        }
        if ((int)h.dut.dbg_unlock_seq != last_seq) {
            last_seq = (int)h.dut.dbg_unlock_seq;
            std::cout << "Cycle " << i << ": unlock_seq=" << last_seq
                      << " unlock_done=" << (int)h.dut.dbg_unlock_done
                      << " PC=0x" << std::hex << h.dut.dbg_pc
                      << " Addr=0x" << h.dut.dbg_addr
                      << " Dout=0x" << (int)h.dut.dbg_dout << std::dec << std::endl;
        }
        if (h.dut.dbg_unlock_done) break;
    }
    require(h.dut.dbg_unlock_done == 1, "ASIC must be unlocked after 16-byte unlock sequence");

    // 5. Run until RMR2 ASIC page mapping (&4000-&7FFF) is enabled
    std::cout << "  Observing RMR2 ASIC page mapping (&4000-&7FFF)..." << std::endl;
    bool aspage_on = false;
    for (int i = 0; i < 5000; ++i) {
        h.tick();
        if (h.dut.dbg_asic_page_on) {
            aspage_on = true;
            break;
        }
    }
    require(aspage_on, "ASIC page must be enabled via RMR2");

    // 6. Run until Palette write and CRTC3 register write
    std::cout << "  Observing Palette and CRTC3 writes..." << std::endl;
    bool saw_palette_wr = false;
    bool saw_crtc_wr = false;
    for (int i = 0; i < 5000; ++i) {
        h.tick();
        if (h.dut.dbg_asic_wr && (h.dut.dbg_asic_addr == 0x2420) && (h.dut.dbg_asic_val == 0x34)) {
            saw_palette_wr = true;
        }
        if (h.dut.dbg_crtc_wr && (h.dut.dbg_crtc_reg == 1) && (h.dut.dbg_crtc_val == 0x28)) {
            saw_crtc_wr = true;
        }
        if (saw_palette_wr && saw_crtc_wr) break;
    }
    require(saw_palette_wr, "ASIC Palette write to &6420 must be captured");
    require(saw_crtc_wr, "CRTC R1 write must be captured");

    // 7. Run until FDC motor write
    std::cout << "  Observing FDC motor write..." << std::endl;
    bool saw_motor_on = false;
    for (int i = 0; i < 5000; ++i) {
        h.tick();
        if (h.dut.dbg_motor) {
            saw_motor_on = true;
            break;
        }
    }
    require(saw_motor_on, "FDC motor must be turned on by write to &FA00");

    // 8. Interrupt stimulus & CPU acknowledge cycle
    std::cout << "  Observing Interrupt request & Z80 acknowledge cycle..." << std::endl;
    h.dut.force_irq = 1;
    bool saw_int_ack = false;
    for (int i = 0; i < 5000; ++i) {
        h.tick();
        if (h.dut.dbg_int_ack) {
            saw_int_ack = true;
            break;
        }
    }
    require(saw_int_ack, "CPU must acknowledge interrupt with M1+IORQ active");
    h.dut.force_irq = 0;
    h.run_cycles(100);

    std::cout << "PASS: test_p10a_deterministic_boot" << std::endl;
}

} // namespace

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    try {
        test_p10a_deterministic_boot();
        std::cout << "\nAll P10a Production CPR Boot Harness tests PASSED.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "\nFAILED: " << e.what() << "\n";
        return 1;
    }
}
