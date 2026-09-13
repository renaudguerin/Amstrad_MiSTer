// SNA CPU-header decode vectors (B18 slice 2).
//
// Every expected value below is derived on paper from the SNA specification
// (docs/references/Snapshot (.SNA) file format.md, offsets and notes cited at
// each assertion) and the T80pa DIR layout (rtl/T80/T80pa.vhd REG/DIR
// comment: IFF2, IFF1, IM, IY, HL', DE', BC', IX, HL, DE, BC, PC, SP, R, I,
// F', A', F, A from high bits to low; T80.vhd consumes DIR(211) as IFF2,
// DIR(210) as IFF1 and DIR(209:208) as the interrupt mode). The DIR slice
// positions themselves are today's Amstrad.sv bit layout, which the module
// preserves verbatim. None is read back out of the simulator.

#include <verilated.h>

#include "Vsna_cpu_header.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[noreturn]] void fail(const std::string& message) {
    throw TestFailure(message);
}

template <typename A, typename B>
void expect_eq(const A& got, const B& want, const std::string& what) {
    if (static_cast<uint64_t>(got) != static_cast<uint64_t>(want)) {
        std::ostringstream os;
        os << what << ": got 0x" << std::hex << static_cast<uint64_t>(got)
           << ", expected 0x" << static_cast<uint64_t>(want);
        fail(os.str());
    }
}

class Harness {
public:
    Harness()
        : dut_(std::make_unique<Vsna_cpu_header>()) {
        dut_->clk = 0;
        dut_->sna_download = 0;
        dut_->wr = 0;
        dut_->addr = 0;
        dut_->data = 0;
        dut_->menu_model = 0;
        dut_->eval();
        tick(4);
    }

    void tick(unsigned n = 1) {
        for (unsigned i = 0; i < n; ++i) {
            dut_->clk = 1;
            dut_->eval();
            dut_->clk = 0;
            dut_->eval();
        }
    }

    Vsna_cpu_header* operator->() { return dut_.get(); }

    // Open a download: the module clears its reset fields on the rising edge
    // of sna_download, mirroring Amstrad.sv's download-start reset.
    void start_download(uint8_t menu_model) {
        dut_->menu_model = menu_model;
        dut_->wr = 0;
        dut_->sna_download = 1;
        tick(2);
    }

    void end_download() {
        dut_->wr = 0;
        dut_->sna_download = 0;
        tick(2);
    }

    // One header byte at its real file offset.
    void put(uint8_t offset, uint8_t byte) {
        dut_->addr = offset;
        dut_->data = byte;
        dut_->wr = 1;
        tick(1);
        dut_->wr = 0;
        tick(1);
    }

    // Byte-aligned DIR slice starting at `bit` (all multi-byte Z80 fields
    // land byte-aligned; see the module header comment).
    uint8_t dir_byte(unsigned bit) {
        unsigned w = bit / 32, s = bit % 32;
        return static_cast<uint8_t>((dut_->cpu_dir[w] >> s) & 0xFFu);
    }

    uint8_t psg_byte(unsigned k) {
        return static_cast<uint8_t>((dut_->psg_regs[k / 4] >> ((k % 4) * 8)) & 0xFFu);
    }

    void expect_dir_zero(const std::string& what) {
        for (int w = 0; w < 7; ++w)
            expect_eq(dut_->cpu_dir[w], 0u, what + " cpu_dir word " + std::to_string(w));
    }

    void expect_psg_zero(const std::string& what) {
        for (int w = 0; w < 4; ++w)
            expect_eq(dut_->psg_regs[w], 0u, what + " psg_regs word " + std::to_string(w));
    }

private:
    std::unique_ptr<Vsna_cpu_header> dut_;
};

// Per-download reset publishes deterministic values before any header byte.
void test_download_start_reset() {
    Harness h;
    // menu_model 1 (664): model must sample it at the download edge.
    h.start_download(1);
    h.expect_dir_zero("reset");
    h.expect_psg_zero("reset");
    expect_eq(h->mem_size, 64u, "reset mem_size (64K default)");
    expect_eq(h->model, 1u, "reset model samples menu_model");
    expect_eq(h->ppi_control, 0x9Bu, "reset ppi_control");
    h.end_download();
    std::cout << "PASS: download-start reset\n";
}

// Full Z80 mapping, offsets 0x11-0x2D into today's DIR bit layout.
void test_z80_mapping() {
    Harness h;
    h.start_download(0);
    // Distinct paper values per register (SNA offsets 0x11-0x18).
    h.put(0x11, 0xC3);  // F
    h.put(0x12, 0x42);  // A
    h.put(0x13, 0x11);  // C
    h.put(0x14, 0x22);  // B
    h.put(0x15, 0x33);  // E
    h.put(0x16, 0x44);  // D
    h.put(0x17, 0x55);  // L
    h.put(0x18, 0x66);  // H
    expect_eq(h.dir_byte(8), 0xC3u, "0x11 F -> cpu_dir[15:8]");
    expect_eq(h.dir_byte(0), 0x42u, "0x12 A -> cpu_dir[7:0]");
    expect_eq(h.dir_byte(80), 0x11u, "0x13 C -> cpu_dir[87:80]");
    expect_eq(h.dir_byte(88), 0x22u, "0x14 B -> cpu_dir[95:88]");
    expect_eq(h.dir_byte(96), 0x33u, "0x15 E -> cpu_dir[103:96]");
    expect_eq(h.dir_byte(104), 0x44u, "0x16 D -> cpu_dir[111:104]");
    expect_eq(h.dir_byte(112), 0x55u, "0x17 L -> cpu_dir[119:112]");
    expect_eq(h.dir_byte(120), 0x66u, "0x18 H -> cpu_dir[127:120]");

    h.put(0x19, 0x77);  // R
    h.put(0x1a, 0x88);  // I
    expect_eq(h.dir_byte(40), 0x77u, "0x19 R -> cpu_dir[47:40]");
    expect_eq(h.dir_byte(32), 0x88u, "0x1a I -> cpu_dir[39:32]");

    // SNA note 2: only bit 0 of the IFF bytes is significant.
    h.put(0x1b, 0x01);
    expect_eq((h->cpu_dir[6] >> 18) & 1u, 1u, "0x1b IFF1 bit0 -> cpu_dir[210]");
    h.put(0x1b, 0xFE);  // bit 0 clear, high bits set: must read 0
    expect_eq((h->cpu_dir[6] >> 18) & 1u, 0u, "0x1b high bits ignored");
    h.put(0x1b, 0x01);
    h.put(0x1c, 0x00);
    expect_eq((h->cpu_dir[6] >> 19) & 1u, 0u, "0x1c IFF2 bit0 -> cpu_dir[211]");
    h.put(0x1c, 0xFF);  // bit 0 set among high bits: must read 1
    expect_eq((h->cpu_dir[6] >> 19) & 1u, 1u, "0x1c high bits ignored");

    // SNA note 5: 16-bit pairs are little-endian (low byte first).
    h.put(0x1d, 0x34);  // IX low
    h.put(0x1e, 0x12);  // IX high
    h.put(0x1f, 0x78);  // IY low
    h.put(0x20, 0x56);  // IY high
    h.put(0x21, 0xCD);  // SP low
    h.put(0x22, 0xAB);  // SP high
    h.put(0x23, 0x00);  // PC low
    h.put(0x24, 0x40);  // PC high: 0x4000, deliberately not 0x0038 so the
                        // 6C model heuristic below stays quiet
    expect_eq(h.dir_byte(128), 0x34u, "0x1d IX low -> cpu_dir[135:128]");
    expect_eq(h.dir_byte(136), 0x12u, "0x1e IX high -> cpu_dir[143:136]");
    expect_eq(h.dir_byte(192), 0x78u, "0x1f IY low -> cpu_dir[199:192]");
    expect_eq(h.dir_byte(200), 0x56u, "0x20 IY high -> cpu_dir[207:200]");
    expect_eq(h.dir_byte(48), 0xCDu, "0x21 SP low -> cpu_dir[55:48]");
    expect_eq(h.dir_byte(56), 0xABu, "0x22 SP high -> cpu_dir[63:56]");
    expect_eq(h.dir_byte(64), 0x00u, "0x23 PC low -> cpu_dir[71:64]");
    expect_eq(h.dir_byte(72), 0x40u, "0x24 PC high -> cpu_dir[79:72]");

    // SNA note 3: interrupt mode is 0, 1 or 2; only bits 1:0 are taken.
    h.put(0x25, 0x02);
    expect_eq((h->cpu_dir[6] >> 16) & 3u, 2u, "0x25 IM -> cpu_dir[209:208]");
    h.put(0x25, 0xFC);  // low bits clear, high bits set: must read 0
    expect_eq((h->cpu_dir[6] >> 16) & 3u, 0u, "0x25 high bits ignored");
    h.put(0x25, 0x01);

    // Alternate register set, SNA note 4.
    h.put(0x26, 0xA1);  // F'
    h.put(0x27, 0xA2);  // A'
    h.put(0x28, 0xA3);  // C'
    h.put(0x29, 0xA4);  // B'
    h.put(0x2a, 0xA5);  // E'
    h.put(0x2b, 0xA6);  // D'
    h.put(0x2c, 0xA7);  // L'
    h.put(0x2d, 0xA8);  // H'
    expect_eq(h.dir_byte(24), 0xA1u, "0x26 F' -> cpu_dir[31:24]");
    expect_eq(h.dir_byte(16), 0xA2u, "0x27 A' -> cpu_dir[23:16]");
    expect_eq(h.dir_byte(144), 0xA3u, "0x28 C' -> cpu_dir[151:144]");
    expect_eq(h.dir_byte(152), 0xA4u, "0x29 B' -> cpu_dir[159:152]");
    expect_eq(h.dir_byte(160), 0xA5u, "0x2a E' -> cpu_dir[167:160]");
    expect_eq(h.dir_byte(168), 0xA6u, "0x2b D' -> cpu_dir[175:168]");
    expect_eq(h.dir_byte(176), 0xA7u, "0x2c L' -> cpu_dir[183:176]");
    expect_eq(h.dir_byte(184), 0xA8u, "0x2d H' -> cpu_dir[191:184]");
    h.end_download();
    std::cout << "PASS: Z80 mapping 0x11-0x2D\n";
}

// RAM config, PPI, PSG select and PSG registers.
void test_peripherals() {
    Harness h;
    h.start_download(0);
    h.put(0x41, 0x05);  // RAM config, SNA note 13
    expect_eq(h->ram_config, 0x05u, "0x41 ram_config");
    // PPI ports: notes 6/7 read inputs, note 8 reads port C outputs.
    h.put(0x56, 0xAA);
    h.put(0x57, 0x55);
    h.put(0x58, 0xF0);
    expect_eq(h->ppi_a, 0xAAu, "0x56 PPI A");
    expect_eq(h->ppi_b, 0x55u, "0x57 PPI B");
    expect_eq(h->ppi_c, 0xF0u, "0x58 PPI C");
    // PPI control, note 9 (overwrites the 0x9b download default).
    h.put(0x59, 0x82);
    expect_eq(h->ppi_control, 0x82u, "0x59 PPI control");
    // PSG select, note 17: only the low nibble is kept.
    h.put(0x5a, 0x07);
    expect_eq(h->psg_addr, 7u, "0x5a PSG select");
    h.put(0x5a, 0x1B);
    expect_eq(h->psg_addr, 0xBu, "0x5a PSG select keeps bits 3:0");
    // PSG registers 0-15 at 0x5B-0x6A, in order.
    for (unsigned k = 0; k < 16; ++k)
        h.put(static_cast<uint8_t>(0x5b + k), static_cast<uint8_t>(0x10 + k));
    for (unsigned k = 0; k < 16; ++k)
        expect_eq(h.psg_byte(k), 0x10 + k, "PSG reg " + std::to_string(k));
    h.end_download();
    std::cout << "PASS: RAM config / PPI / PSG\n";
}

// Model decision at 0x6B-0x6D: size gate, CPC-type map, v1 PC heuristic.
void test_model_decision() {
    // 64K file: type byte selects the model (SNA v2/v3 offset 0x6D).
    {
        Harness h;
        h.start_download(1);  // menu 664: discriminates "unchanged" cases
        h.put(0x23, 0x00);
        h.put(0x24, 0x40);  // PC 0x4000, keeps the 6C heuristic quiet
        h.put(0x6b, 0x40);
        h.put(0x6c, 0x00);  // size 64: not > 64, PC != 0x38, model untouched
        expect_eq(h->mem_size, 64u, "6B/6C little-endian size");
        expect_eq(h->model, 1u, "64K non-0x38 PC leaves menu model");
        h.put(0x6d, 0x00);  // CPC464
        expect_eq(h->model, 2u, "type 0 -> 464 (model 2)");
        h.put(0x6d, 0x01);  // CPC664
        expect_eq(h->model, 1u, "type 1 -> 664 (model 1)");
        h.put(0x6d, 0x02);  // CPC6128
        expect_eq(h->model, 0u, "type 2 -> 6128 (model 0)");
        h.put(0x6d, 0x01);
        h.put(0x6d, 0x03);  // unknown: leaves the model unchanged
        expect_eq(h->model, 1u, "type 3 leaves model unchanged");
        h.put(0x6d, 0x07);  // reserved: leaves the model unchanged
        expect_eq(h->model, 1u, "type 7 leaves model unchanged");
        h.put(0x6d, 0x04);  // 6128 Plus
        expect_eq(h->model, 0u, "type 4 -> 128K map (model 0)");
        h.put(0x6d, 0x05);  // 464 Plus
        expect_eq(h->model, 0u, "type 5 -> 128K map (model 0)");
        h.put(0x6d, 0x06);  // GX4000
        expect_eq(h->model, 0u, "type 6 -> 128K map (model 0)");
        h.end_download();
    }
    // 128K file: the size gate forces model 0 at 6C and holds it at 6D.
    {
        Harness h;
        h.start_download(1);
        h.put(0x23, 0x38);
        h.put(0x24, 0x00);  // PC 0x0038: would fire the heuristic at 64K
        h.put(0x6b, 0x80);
        h.put(0x6c, 0x00);  // size 128 > 64: model 0 despite PC == 0x38
        expect_eq(h->mem_size, 128u, "128K size");
        expect_eq(h->model, 0u, ">64K forces model 0 at 6C");
        h.put(0x6d, 0x00);  // would select 464 at 64K: still gated
        expect_eq(h->model, 0u, ">64K forces model 0 at 6D");
        h.end_download();
    }
    // Size boundary: exactly 65K is already "> 64".
    {
        Harness h;
        h.start_download(2);
        h.put(0x6b, 0x41);
        h.put(0x6c, 0x00);  // size 65 > 64
        expect_eq(h->model, 0u, "65K forces model 0 at 6C");
        h.end_download();
    }
    // 64K v1 file (no type byte) with PC == 0x0038: the 6C heuristic
    // selects the 464 before any 6D byte arrives.
    {
        Harness h;
        h.start_download(1);
        h.put(0x23, 0x38);
        h.put(0x24, 0x00);  // PC 0x0038
        h.put(0x6b, 0x40);
        h.put(0x6c, 0x00);  // size 64 with PC == 0x38 -> model 2
        expect_eq(h->model, 2u, "64K PC==0x38 selects 464 at 6C");
        h.end_download();
    }
    std::cout << "PASS: model decision\n";
}

// Unmapped offsets change nothing; menu_model is sampled at the edge only.
void test_ignored_offsets() {
    Harness h;
    h.start_download(0);
    h.put(0x12, 0x42);
    h.put(0x6b, 0x40);
    h.put(0x6c, 0x00);
    h.put(0x6d, 0x00);
    expect_eq(h->model, 2u, "setup model");
    h.put(0x10, 0xFF);  // snapshot version: outside this slice
    h.put(0x2e, 0xFF);  // GA pen: outside this slice
    h.put(0x6e, 0xFF);  // v2 interrupt number: outside this slice
    expect_eq(h.dir_byte(0), 0x42u, "unmapped writes keep cpu_dir");
    expect_eq(h->model, 2u, "unmapped writes keep model");
    h->menu_model = 1;  // mid-download menu change: not resampled
    h.tick(2);
    expect_eq(h->model, 2u, "menu_model sampled at download start only");
    h.end_download();
    std::cout << "PASS: ignored offsets\n";
}

// A second download clears the reset fields but keeps the quirk fields,
// and the decoder works again from the clean state.
void test_second_download_isolation() {
    Harness h;
    h.start_download(0);
    h.put(0x11, 0xC3);
    h.put(0x12, 0x42);
    h.put(0x41, 0x05);  // ram_config: NOT reset (quirk, always in file)
    h.put(0x56, 0xAA);  // ppi_a: NOT reset
    h.put(0x57, 0x55);
    h.put(0x58, 0xF0);
    h.put(0x59, 0x82);
    h.put(0x5a, 0x07);  // psg_addr: NOT reset
    for (unsigned k = 0; k < 16; ++k)
        h.put(static_cast<uint8_t>(0x5b + k), static_cast<uint8_t>(0xE0 + k));
    h.put(0x6b, 0x80);
    h.put(0x6c, 0x00);  // 128K -> model 0
    expect_eq(h->model, 0u, "first download model");
    h.end_download();

    h.start_download(2);  // different menu model
    h.expect_dir_zero("second download");
    h.expect_psg_zero("second download");
    expect_eq(h->mem_size, 64u, "second download mem_size reset");
    expect_eq(h->model, 2u, "second download model follows new menu");
    expect_eq(h->ppi_control, 0x9Bu, "second download ppi_control reset");
    // Quirk retention: fields the inline code never cleared survive.
    expect_eq(h->ram_config, 0x05u, "ram_config not reset (quirk)");
    expect_eq(h->ppi_a, 0xAAu, "ppi_a not reset");
    expect_eq(h->ppi_b, 0x55u, "ppi_b not reset");
    expect_eq(h->ppi_c, 0xF0u, "ppi_c not reset");
    expect_eq(h->psg_addr, 7u, "psg_addr not reset");
    // The decoder works again from the clean state.
    h.put(0x12, 0x99);
    expect_eq(h.dir_byte(0), 0x99u, "decode after reset");
    expect_eq(h.dir_byte(8), 0x00u, "neighbour field stays clear");
    h.end_download();
    std::cout << "PASS: second-download isolation\n";
}

}  // namespace

int main() {
    try {
        test_download_start_reset();
        test_z80_mapping();
        test_peripherals();
        test_model_decision();
        test_ignored_offsets();
        test_second_download_isolation();
    } catch (const TestFailure& e) {
        std::cerr << "FAIL: " << e.what() << "\n";
        return 1;
    }
    std::cout << "sna_cpu_header: all tests passed\n";
    return 0;
}
