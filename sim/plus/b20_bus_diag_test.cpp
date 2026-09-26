// Production T80 + real motherboard/ASIC, original synthetic CPR.
// CPC Plus CPU schematic IC116 G1/G2/G3, D201/D202/R210; Gerald's
// 29 July 2017 trace shows raster 06 then empty 04 on one CPU acknowledge.
// https://oldwiki.cpcwiki.eu/imgs/7/7e/IM2_Plus_Ack_Bug.png
// LD(DE),A has a memory-write cycle despite its one-byte opcode; HALT does not.
#define main p10_unused_main
#include "p10_boot_test.cpp"
#undef main
#include "Vp10_boot_test_top___024root.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdint>

namespace {

static std::string hex_str(uint32_t val, int width) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%0*X", width, val);
    return std::string(buf);
}

struct CellConfig {
    std::string name;
    uint16_t origin;
    bool memory_write;
};

std::vector<uint8_t> build_cell_program(const CellConfig& cfg) {
    std::vector<uint8_t> code(16384, 0x00);
    size_t idx = 0;
    auto emit = [&](uint8_t b) { code[idx++] = b; };

    // Reset vector at 0x0000
    emit(0xF3);                         // DI
    emit(0x31); emit(0x00); emit(0x1F); // LD SP, 0x1F00
    emit(0xC3); emit(0x20); emit(0x00); // JP 0x0020

    // Init code at 0x0020
    idx = 0x0020;
    auto emit_crtc = [&](uint8_t reg, uint8_t val) {
        emit(0x01); emit(0x00); emit(0xBC); // LD BC, 0xBC00
        emit(0x3E); emit(reg);              // LD A, reg
        emit(0xED); emit(0x79);             // OUT (C), A
        emit(0x01); emit(0x00); emit(0xBD); // LD BC, 0xBD00
        emit(0x3E); emit(val);              // LD A, val
        emit(0xED); emit(0x79);             // OUT (C), A
    };

    // Initialize CRTC for normal line timing
    emit_crtc(0, 63);   // R0: horizontal total
    emit_crtc(1, 40);   // R1: horizontal displayed
    emit_crtc(2, 46);   // R2: horizontal sync pos
    emit_crtc(3, 0x8E); // R3: sync widths
    emit_crtc(4, 38);   // R4: vertical total
    emit_crtc(6, 25);   // R6: vertical displayed
    emit_crtc(7, 30);   // R7: vertical sync pos
    emit_crtc(9, 7);    // R9: max raster

    // Unlock ASIC: 16 bytes to &BC00
    const uint8_t unlock_seq[16] = {
        0xFF, 0x00, 0xFF, 0x77, 0xB3, 0x51, 0xA8, 0xD4,
        0x62, 0x39, 0x9C, 0x46, 0x2B, 0x15, 0x8A, 0xCD
    };
    emit(0x01); emit(0x00); emit(0xBC); // LD BC, 0xBC00
    for (int i = 0; i < 16; ++i) {
        emit(0x3E); emit(unlock_seq[i]);
        emit(0xED); emit(0x79);
    }

    // Enable ASIC page at &4000-&7FFF (RMR2 = 0xB8 to &7F00)
    emit(0x01); emit(0x00); emit(0x7F); // LD BC, 0x7F00
    emit(0x3E); emit(0xB8);             // LD A, 0xB8
    emit(0xED); emit(0x79);             // OUT (C), A

    // IVR: write 0x00 to &6805 (vectored mode, base 0x00)
    emit(0x3E); emit(0x00);             // LD A, 0x00
    emit(0x32); emit(0x05); emit(0x68); // LD (0x6805), A

    // PRI: write 24 (0x18) to &6800 (fire at raster line 24)
    emit(0x3E); emit(0x18);             // LD A, 0x18
    emit(0x32); emit(0x00); emit(0x68); // LD (0x6800), A

    // IM2 setup: I = 0x03, IM 2
    emit(0x3E); emit(0x03);             // LD A, 0x03
    emit(0xED); emit(0x47);             // LD I, A
    emit(0xED); emit(0x5E);             // IM 2

    // Clear RAM markers
    emit(0xAF);                         // XOR A
    emit(0x32); emit(0x00); emit(0x10); // LD (0x1000), A (DCSR readback)
    emit(0x32); emit(0x01); emit(0x10); // LD (0x1001), A (completion marker)

    // Copy an original loop into RAM, so origin/A13 changes only execution location.
    emit(0x21); emit(0x00); emit(0x08); // LD HL,0800
    emit(0x11); emit(cfg.origin & 255); emit(cfg.origin >> 8);
    emit(0x01); emit(0x80); emit(0x00); // LD BC,128
    emit(0xED); emit(0xB0);             // LDIR
    emit(0x11); emit(0x04); emit(0xBF); // LD DE,BF04
    // Enable interrupts and jump to target origin
    emit(0xFB);                         // EI
    emit(0xC3); emit(cfg.origin & 0xFF); emit(cfg.origin >> 8); // JP origin

    // IM2 table at 0x0300..0x03FF
    // Default all even entries to unexpected_handler (0x0700)
    for (int v = 0; v < 256; v += 2) {
        code[0x0300 + v]     = 0x00;
        code[0x0300 + v + 1] = 0x07;
    }
    // Vector 0x06 (raster): points to 0x0600
    code[0x0306] = 0x00;
    code[0x0307] = 0x06;
    // Vector 0x04 (DMA0): points to 0x0400
    code[0x0304] = 0x00;
    code[0x0305] = 0x04;
    // Vector 0x00 (DMA2 / empty): points to 0x0500
    code[0x0300] = 0x00;
    code[0x0301] = 0x05;

    // 64 consecutive memory writes, matching the instruction class of FlowLIB.
    idx = 0x0800;
    for (int i=0; i<64; ++i) emit(cfg.memory_write ? 0x12 : 0x76);
    emit(0xC3); emit(cfg.origin & 255); emit(cfg.origin >> 8);

    // Handlers
    // 0x0600: raster_handler
    idx = 0x0600;
    emit(0x3A); emit(0x0F); emit(0x6C); // LD A, (0x6C0F) (DCSR)
    emit(0x32); emit(0x00); emit(0x10); // LD (0x1000), A
    emit(0x3E); emit(0xAA);             // LD A, 0xAA (raster marker)
    emit(0x32); emit(0x01); emit(0x10); // LD (0x1001), A
    emit(0x76);                         // HALT
    emit(0x18); emit(0xFE);             // JR -2

    // 0x0400: dma0_handler (S29 bug vector)
    idx = 0x0400;
    emit(0x3A); emit(0x0F); emit(0x6C); // LD A, (0x6C0F)
    emit(0x32); emit(0x00); emit(0x10); // LD (0x1000), A
    emit(0x3E); emit(0xD0);             // LD A, 0xD0 (DMA0 marker)
    emit(0x32); emit(0x01); emit(0x10); // LD (0x1001), A
    emit(0x76);                         // HALT
    emit(0x18); emit(0xFE);             // JR -2

    // 0x0500: dma2_handler (empty vector fallback)
    idx = 0x0500;
    emit(0x3A); emit(0x0F); emit(0x6C); // LD A, (0x6C0F)
    emit(0x32); emit(0x00); emit(0x10); // LD (0x1000), A
    emit(0x3E); emit(0xD2);             // LD A, 0xD2 (DMA2 marker)
    emit(0x32); emit(0x01); emit(0x10); // LD (0x1001), A
    emit(0x76);                         // HALT
    emit(0x18); emit(0xFE);             // JR -2

    // 0x0700: unexpected_handler
    idx = 0x0700;
    emit(0x3E); emit(0xEE);             // LD A, 0xEE (unexpected marker)
    emit(0x32); emit(0x01); emit(0x10); // LD (0x1001), A
    emit(0x76);                         // HALT
    emit(0x18); emit(0xFE);             // JR -2

    return code;
}


void run_cell(const CellConfig& cfg) {
    Harness h;
    h.dut.d5_key=0; h.dut.production_clocking=1;
    h.dut.plus_model_i=2; h.dut.force_irq=0;
    h.initialize(); h.download(build_cpr_image({{"cb00", build_cell_program(cfg)}}));
    wait_for_cpr_apply(h);
    bool seen=false, write_seen=false, prev_raw=false, prev_ack=false;
    bool unlocked=false, page=false, opcode=false, pending_before=false, dcsr_seen=false;
    uint8_t dcsr=0;
    int samples=0; uint8_t sampled=0;
    uint16_t ack_address=0;
    int raw_rises=0, rises=0, pulse_age=0;
    uint8_t pulse_vec=0, marker=0;
    std::vector<unsigned> vectors;
    const bool susceptible=cfg.memory_write && !(cfg.origin & 0x2000);
    for (int tick=0; tick<500000; ++tick) {
        // Timestamp is the production IM2 sample edge. dbg_din observes
        // the upstream byte before the motherboard wired-AND bus; the
        // handler marker below independently proves the consumed vector.
        if (h.dut.d5_ack_sample) {
            ++samples; sampled=h.dut.dbg_din;
            if (std::getenv("B20_TRACE")) std::printf("SAMPLE %s tick=%d byte=%02X\n",cfg.name.c_str(),tick,sampled);
        }
        h.tick();
        seen |= h.dut.dbg_pc==cfg.origin;
        unlocked |= h.dut.dbg_unlock_done; page |= h.dut.dbg_asic_page_on;
        if (seen && !h.dut.dbg_mreq_n && !h.dut.dbg_m1_n && !h.dut.dbg_rd_n &&
            h.dut.dbg_addr>=cfg.origin && h.dut.dbg_addr<cfg.origin+64)
            opcode |= h.dut.dbg_din==(cfg.memory_write ? 0x12:0x76);
        write_seen |= seen && !h.dut.dbg_mreq_n && !h.dut.dbg_wr_n && h.dut.dbg_addr==0xBF04;
        bool raw=!h.dut.dbg_iorq_n && !h.dut.dbg_m1_n;
        bool ack=h.dut.dbg_vec_valid;
        if (raw && !prev_raw) {
            ++raw_rises; ack_address=h.dut.dbg_addr;
            pending_before=!h.dut.rootp->p10_boot_test_top__DOT__mb__DOT__plus_ga_int_n;
        }
        if (ack && !prev_ack) { ++rises; pulse_age=0; }
        if (ack) {
            ++pulse_age;
            if (pulse_age==2) {
                require(h.dut.rootp->p10_boot_test_top__DOT__mb__DOT__plus_ga_int_n,cfg.name+": raster not cleared on first latch edge");
                pulse_vec=h.dut.dbg_vec_byte; vectors.push_back(pulse_vec); }
            if (pulse_age>2) require(h.dut.dbg_vec_byte==pulse_vec, cfg.name+": vector changed within ASIC pulse");
        }
        if (std::getenv("B20_TRACE") && (raw || (prev_raw && !raw)))
            std::printf("TRACE %s tick=%d A=%04X raw=%d asic=%d ready=%d wait=%d m1=%d ts=%d cen=%d vec=%02X pending=%d\n",
                cfg.name.c_str(),tick,h.dut.dbg_addr,raw,ack,
                h.dut.rootp->p10_boot_test_top__DOT__mb__DOT__plus_ready,
                h.dut.dbg_cpu_waitn,h.dut.dbg_m1_n,h.dut.dbg_tstate,h.dut.dbg_cen_p,
                h.dut.dbg_vec_byte,!h.dut.rootp->p10_boot_test_top__DOT__mb__DOT__plus_ga_int_n);
        if (raw_rises && !h.dut.dbg_mreq_n && !h.dut.dbg_rd_n && h.dut.dbg_addr==0x6C0F) {
            dcsr_seen=true; dcsr=h.dut.dbg_din;
        }
        prev_raw=raw; prev_ack=ack;
        if (raw_rises && !h.dut.dbg_wr_n && !h.dut.dbg_mreq_n && h.dut.dbg_addr==0x1001) {
            marker=h.dut.dbg_dout; break;
        }
    }
    std::printf("RESULT %s raw=%d pulses=%d marker=%02X vectors=",cfg.name.c_str(),raw_rises,rises,marker);
    for (auto v:vectors) std::printf("%02X ",v);
    std::puts("");
    require(unlocked && page,cfg.name+": ASIC setup incomplete");
    require(opcode,cfg.name+": expected target opcode not fetched");
    require(pending_before,cfg.name+": no pending raster before acknowledge");
    require(cfg.memory_write ? (ack_address > cfg.origin && ack_address <= cfg.origin+64)
                             : ack_address==cfg.origin+1,
            cfg.name+": acknowledge did not follow the intended opcode class");
    require(dcsr_seen && dcsr==(susceptible ? 0:0x80),cfg.name+": handler DCSR provenance incorrect");
    require(seen, cfg.name+": no target execution");
    require(!cfg.memory_write || write_seen,cfg.name+": no LD(DE),A memory write");
    require(samples==1 && sampled==(susceptible ? 4:6),cfg.name+": wrong byte at production T80 IM2 sample edge");
    require(raw_rises==1,cfg.name+": expected one CPU acknowledge");
    require(rises==(susceptible ? 2:1),cfg.name+": wrong ASIC pulse count");
    require(vectors== (susceptible ? std::vector<unsigned>{6,4}:std::vector<unsigned>{6}),cfg.name+": wrong per-pulse vector (must distinguish 04 from 00/02)");
    require(marker==(susceptible ? 0xD0:0xAA),cfg.name+": CPU consumed wrong vector");
}
} // namespace
int main(int argc,char** argv) {
    Verilated::commandArgs(argc,argv);
    int failures=0;
    for (const auto& cell:std::vector<CellConfig>{{"write-low",0x9000,true},{"write-high",0xB000,true},{"halt-low",0x9000,false}}) {
        try { run_cell(cell); } catch (const std::exception& e) { std::fprintf(stderr,"FAIL %s\n",e.what()); ++failures; }
    }
    return failures ? 1:0;
}
