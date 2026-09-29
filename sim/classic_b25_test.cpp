// Executes the packaged B25 program on real CRTC0/1 -> GA40010 -> T80pa.
// No IRQ masking, snapshot load, forced CPU state, or internal CPU predicates.
// The driver supplies a firmware-free launch and RAM, not an AMSDOS boot test.
#include "Vclassic_irq_phase_top.h"
#include "verilated.h"
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

static void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
struct Bench {
    Vclassic_irq_phase_top d;
    uint64_t ticks = 0;
    unsigned acks = 0;
    bool old_ack = false, old_io_write = false, entered = false;
    unsigned selected = 0;
    std::array<unsigned,32> crtc_regs{};
    std::array<unsigned,3> launch_position{};
    void tick() {
        d.clk = 0; d.eval(); d.clk = 1; d.eval();
        const bool ack = !d.t80_iorq_n && !d.t80_m1_n && d.t80_mreq_n;
        const bool io_write = !d.t80_iorq_n && !d.t80_wr_n;
        if (io_write && !old_io_write && !(d.t80_a & 0x4000) && !(d.t80_a & 0x200)) {
            if (!(d.t80_a & 0x100)) selected = d.t80_dout & 31;
            else crtc_regs[selected] = d.t80_dout;
        }
        old_io_write = io_write;
        if (ack && !old_ack && !d.reset && !d.cpu_reset) {
            require(crtc_regs[7] == 0xFF, "measurement IRQ with VSYNC still enabled");
            ++acks;
        }
        if (!entered && !d.reset && !d.cpu_reset && !d.t80_m1_n &&
            !d.t80_mreq_n && !d.t80_rd_n && d.t80_a == 0x1000) {
            entered = true;
            launch_position = {unsigned(d.crtc_row), unsigned(d.crtc_line), unsigned(d.crtc_hcc)};
        }
        old_ack = ack;
        ++ticks;
    }
    void run(unsigned n) { while (n--) tick(); }
    void write(unsigned a, unsigned v) {
        d.prog_we = 1; d.prog_addr = a; d.prog_data = v; tick();
        d.prog_we = 0;
    }
    unsigned read(unsigned a) { d.peek_addr = a; d.eval(); return d.peek_data; }
    unsigned word(unsigned a) { return read(a) | (read(a+1) << 8); }
    Bench(unsigned type, unsigned phase, const std::vector<uint8_t>& program) {
        d.reset = 1; d.cpu_reset = 1; d.crtc_type = type;
        d.mask_cpu_irq = 0; d.sna_load = 0; d.prog_we = 0;
        run(128);
        for (unsigned i = 0; i < program.size(); ++i) write(0x1000+i, program[i]);
        // A minimal ROM-loader stand-in: configure a running ordinary CRTC,
        // then execute NOP padding before jumping to the unmodified disk body.
        // This makes initial line/frame and GA phases differ without a snapshot.
        std::vector<uint8_t> boot = {0xF3};
        const uint8_t regs[] = {63,40,46,0x8E,38,0,25,30,0,7,0,0,0x30,0};
        for (unsigned r = 0; r < sizeof(regs); ++r) {
            const uint8_t b[] = {0x01,uint8_t(r),0xBC,0xED,0x49,
                                 0x01,regs[r],0xBD,0xED,0x49};
            boot.insert(boot.end(), std::begin(b), std::end(b));
        }
        // Long pause takes the launch into different vertical phases.
        // DJNZ instruction count is deliberately not an oracle.
        for (unsigned i = 0; i < phase; ++i) {
            const uint8_t b[] = {0x06,0,0x10,0xFE};
            boot.insert(boot.end(), std::begin(b), std::end(b));
        }
        boot.push_back(0xC3); boot.push_back(0); boot.push_back(0x10);
        require(boot.size() < 0x1000, "bootstrap overlaps diagnostic");
        for (unsigned i = 0; i < boot.size(); ++i) write(i, boot[i]);
        d.reset = 0;
        run(phase * 7 + 3); // vary master-clock release phase too
        d.cpu_reset = 0;
    }
};

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    try {
        require(argc == 3, "usage: classic_b25_tests extracted.bin output-prefix");
        std::ifstream input(argv[1], std::ios::binary);
        require(bool(input), "cannot read extracted program");
        std::vector<uint8_t> body((std::istreambuf_iterator<char>(input)), {});
        require(!body.empty() && body.size() < 0x2FFE, "bad diagnostic body length");
        std::array<unsigned,144> baseline{};
        for (unsigned type : {0u, 1u}) {
            for (unsigned phase : {0u, 7u, 23u}) {
                Bench b(type, phase, body);
                while (b.read(0x8000) != 0xA5 && b.ticks < 100000000) b.run(1024);
                if (b.read(0x8000) != 0xA5) {
                    std::cerr << "timeout PC bus=" << std::hex << b.d.t80_a
                              << " status=" << b.read(0x8000) << " ack=" << std::dec << b.acks
                          << " entry=" << b.launch_position[0] << ':' << b.launch_position[1]
                          << ':' << b.launch_position[2] << std::endl;
                    throw std::runtime_error("diagnostic did not finish");
                }
                require(b.acks == 144, "expected two real GA acknowledgements per trial");
                require(b.crtc_regs[7] == 30, "normal VSYNC timing not restored");
                require(b.entered, "diagnostic entry was never fetched");
                std::cout << "CRTC" << type << " launch=" << phase << " ACK=" << b.acks
                          << " entry=" << b.launch_position[0] << ':' << b.launch_position[1]
                          << ':' << b.launch_position[2] << std::endl;
                for (unsigned c = 0; c < 9; ++c) {
                    const unsigned first_pc = b.word(0x8100+c*32);
                    const unsigned first_hl = b.word(0x8102+c*32);
                    for (unsigned t = 0; t < 8; ++t) {
                        const unsigned i = c*16+t*2;
                        const unsigned pc = b.word(0x8100+i*2);
                        const unsigned hl = b.word(0x8102+i*2);
                        require(pc >= 0x4000 && pc < 0x5000, "captured PC outside instruction sled");
                        require(pc == first_pc && hl == first_hl, "trial instability");
                        // HALT returns just after its single instruction. NOP and
                        // untaken RET leave HL unchanged. INC/ADD DE=1 count one
                        // per single-byte opcode (§26 FR281-282), so PC delta=HL.
                        if (c == 1) require(pc == 0x4001 && hl == 0, "HALT capture contract");
                        else if (c < 4) require(hl == 0, "NOP/RET changed HL");
                        else require(hl == pc-0x4000 && hl > 1000, "PC/HL instruction count mismatch");
                        if (phase == 0) { baseline[i] = pc; baseline[i+1] = hl; }
                        else require(baseline[i] == pc && baseline[i+1] == hl, "launch phase affected result");
                    }
                    std::cout << " case " << c << " PCdelta=" << std::hex << first_pc-0x4000
                              << " HL=" << first_hl << std::dec << " x8\n";
                }
                // A readable Mode2 RAM dump for screenshot conversion/QA. The
                // model has no video renderer or firmware/PPI keyboard model.
                if (phase == 0) {
                    std::ofstream screen(std::string(argv[2])+"-crtc"+std::to_string(type)+".scr",std::ios::binary);
                    for (unsigned a=0xC000; a<0x10000; ++a) screen.put(char(b.read(a)));
                }
                // Observe restored VSYNC region and an idle result screen.
                const unsigned before = b.acks;
                b.run(64*20000);
                require(b.read(0x8000) == 0xA5 && b.acks == before, "result display changed without input");
            }
        }
        std::cout << "classic-b25: PASS (CRTC0/1, three launch phases, 432 captures)\n";
    } catch (const std::exception& e) {
        std::cerr << "classic-b25: FAIL: " << e.what() << '\n'; return 1;
    }
}
