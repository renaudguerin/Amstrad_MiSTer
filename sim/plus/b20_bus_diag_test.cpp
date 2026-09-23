// B20-2/B20-3 production-bus discriminator (test-only).
//
// Closes the gap between b20_ack_matrix (production T80, but synthetic vector,
// no ASIC/motherboard) and b20_ack_diag (ASIC registers, but synthetic bus,
// no CPU/motherboard). Reuses the D5 prepared motherboard fixture
// (production T80pa, real Amstrad_motherboard, asic_regs, asic_ga_timing,
// asic_video, etc.) driven by a synthetic cartridge program (no private media).
//
// Authority split (cited per project guidelines):
// - Physical claims from S29 (Plus Vectored Interrupt Bug, CPCWiki pp.3-4):
//   1. Motherboard LK106/IC116 IORQ pulse-shaping logic makes the ASIC observe
//      two acknowledge pulses when an interrupt occurs during a multi-byte
//      memory read/write instruction with A13=0 in the interrupted context.
//   2. The first acknowledge clears the raster interrupt.
//   3. The second acknowledge finds no interrupt pending and vectors to
//      DMA0 / offset 4 (0x04).
//   4. Single-byte instructions (including HALT) and A13=1 contexts are
//      immune and exhibit normal single-acknowledge behaviour.
// - Production core properties (rtl/Amstrad_motherboard.v, rtl/T80/T80pa.vhd):
//   1. intack = plus_mode & ~M1_n & iorq: motherboard has no LK106/IC116
//      pulse multiplier or A13-dependent pulse-shaping mechanism.
//      Both CPU and ASIC observe a single shared acknowledge net; dbg_int_ack
//      monitors this net rather than independently probed physical endpoint pins.
//   2. In the three evaluated cells, a single acknowledge cycle occurs per
//      accepted interrupt on this shared net.
//   3. The ASIC provides raster vector 0x06 with within-ack stability.
//   4. Raster interrupt clears on acknowledge, setting DCSR bit 7 (last_raster).
//   5. IM2 vector table at 0x0306 branches to raster_handler (0x0600).
//   6. S29's secondary empty acknowledge and DMA0 vector (0x04) are NOT
//      reachable in the evaluated cells on the current production bus.
//
// Cells exercised:
//   Cell 1: LDIR @ 0x0100 (A13=0, S29 susceptible class)
//   Cell 2: LDIR @ 0x2100 (A13=1, S29 control class)
//   Cell 3: HALT @ 0x0100 (A13=0, S29 single-byte control class)
//
// In all three cells, the bench asserts:
//   - ASIC unlock and RMR2 page enable succeed.
//   - Real raster interrupt fires from ASIC (PRI=24).
//   - Exactly 1 acknowledge pulse on the shared CPU/ASIC acknowledge net.
//   - Vector byte 0x06 captured unconditionally at clock 2, stable across remaining clocks.
//   - GA pending request asserted before ack, deasserts at clock 2, remains clear.
//   - Interrupted instruction pinned at origin+9 (LDIR) or origin+1 (HALT).
//   - CPU enters raster_handler (0x0600), never DMA0 (0x0400) or DMA2 (0x0500).
//   - Handler reads DCSR &6C0F: bit 7 == 1 (last_raster), DMA flags/enables == 0.
//   - Handler writes completion marker 0xAA to RAM at 0x1001.

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
    bool is_ldir;
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

    // Target body placement
    idx = cfg.origin;
    if (cfg.is_ldir) {
        uint8_t r_hi = (cfg.origin == 0x0100) ? 0x18 : 0x28;
        uint8_t w_hi = (cfg.origin == 0x0100) ? 0x19 : 0x29;
        emit(0x21); emit(0x00); emit(r_hi); // LD HL, r_hi:00
        emit(0x11); emit(0x00); emit(w_hi); // LD DE, w_hi:00
        emit(0x01); emit(0x00); emit(0x01); // LD BC, 0x0100
        emit(0xED); emit(0xB0);             // LDIR
        emit(0x18); emit(0xF9);             // JR -7 (to LD BC)
    } else {
        emit(0x76);                         // HALT
        emit(0x18); emit(0xFE);             // JR -2 (to HALT)
    }

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

struct CellObservation {
    bool unlock_seen = false;
    bool asic_page_seen = false;
    bool origin_seen = false;

    // Blocker (2): Prior execution / transfer evidence and pinned instruction
    uint32_t ldir_reads_before_ack = 0;
    uint32_t ldir_writes_before_ack = 0;
    bool ldir_ed_fetch_seen = false;
    bool ldir_b0_fetch_seen = false;
    bool halt_opcode_fetch_seen = false;
    uint32_t halt_cycles_before_ack = 0;

    // Blocker (1) & (4): Single acknowledge on shared net, registered latency separation, clock 2 capture
    int ack_rises = 0;
    int ack_clocks = 0;
    uint64_t first_ack_tick = 0;
    uint16_t first_ack_a = 0;
    uint8_t ack_vec_clock1 = 0;
    uint8_t ack_vec_clock2 = 0;
    bool ack_vec_clock2_seen = false;
    bool vec_unstable = false;

    // Blocker (3): GA pending request lifecycle (asserted before ack, deasserted at clock 2, clear through handler)
    bool ga_pending_before_ack = false;
    bool ga_pending_ack_clock1 = false;
    bool ga_pending_ack_clock2 = false;
    bool ga_reasserted_in_ack = false;
    bool ga_reasserted_in_handler = false;
    bool dma_int_asserted = false;

    // Handler entry, DCSR readback, completion marker
    bool entered_raster_handler = false;
    bool entered_dma0_handler = false;
    bool entered_dma2_handler = false;
    bool entered_unexpected_handler = false;
    bool dcsr_read_seen = false;
    uint8_t dcsr_val = 0;
    bool marker_written = false;
    uint8_t marker_val = 0;
};

CellObservation run_cell(const CellConfig& cfg) {
    std::cout << "--- Running: " << cfg.name << " ---" << std::endl;
    auto program = build_cell_program(cfg);

    Harness h;
    h.dut.d5_key = 0;
    h.dut.production_clocking = 1;
    h.dut.plus_model_i = 2; // 6128+
    h.dut.force_irq = 0;
    h.initialize();
    h.download(build_cpr_image({{"cb00", program}}));
    wait_for_cpr_apply(h);

    CellObservation obs;
    bool prev_int_ack = false;
    uint64_t completion_tick = 0;
    const uint64_t kMaxTicks = 500000;

    for (uint64_t tick = 0; tick < kMaxTicks; ++tick) {
        h.tick();

        if (h.dut.dbg_unlock_done) obs.unlock_seen = true;
        if (h.dut.dbg_asic_page_on) obs.asic_page_seen = true;
        if (h.dut.dbg_pc == cfg.origin) obs.origin_seen = true;

        // GA interrupt request (active-low: 0 = asserted/pending, 1 = deasserted/clear).
        // In Plus mode without DMA (dma_int_set == 0), Amstrad_motherboard routes:
        //   plus_int_n = plus_ga_int_n & ~plus_dma_int_req;
        //   INT_n = plus_int_n;
        // so plus_ga_int_n directly represents the merged CPU interrupt request line.
        bool ga_pending = (h.dut.rootp->p10_boot_test_top__DOT__mb__DOT__plus_ga_int_n == 0);
        if (h.dut.rootp->p10_boot_test_top__DOT__mb__DOT__dma_int_set != 0) {
            obs.dma_int_asserted = true;
        }

        // Before acknowledge: freeze/require read+write and collect opcode execution evidence
        if (obs.origin_seen && obs.ack_rises == 0) {
            if (ga_pending) {
                obs.ga_pending_before_ack = true;
            }

            if (cfg.is_ldir) {
                uint16_t read_hi = (cfg.origin == 0x0100) ? 0x1800 : 0x2800;
                uint16_t write_hi = (cfg.origin == 0x0100) ? 0x1900 : 0x2900;
                if (!h.dut.dbg_mreq_n && !h.dut.dbg_rd_n && h.dut.dbg_m1_n &&
                    (h.dut.dbg_addr & 0xFF00) == read_hi) {
                    obs.ldir_reads_before_ack++;
                }
                if (!h.dut.dbg_mreq_n && !h.dut.dbg_wr_n &&
                    (h.dut.dbg_addr & 0xFF00) == write_hi) {
                    obs.ldir_writes_before_ack++;
                }

                // Opcode fetch evidence for ED B0 at origin+9 and origin+10
                if (!h.dut.dbg_m1_n && !h.dut.dbg_mreq_n && !h.dut.dbg_rd_n && h.dut.dbg_wait_n) {
                    if (h.dut.dbg_addr == cfg.origin + 9)  obs.ldir_ed_fetch_seen = true;
                    if (h.dut.dbg_addr == cfg.origin + 10) obs.ldir_b0_fetch_seen = true;
                }
            } else {
                // HALT control execution evidence: opcode fetch of 0x76 at origin,
                // followed by CPU execution in HALT state holding PC at origin+1
                if (!h.dut.dbg_m1_n && !h.dut.dbg_mreq_n && !h.dut.dbg_rd_n && h.dut.dbg_wait_n) {
                    if (h.dut.dbg_addr == cfg.origin) obs.halt_opcode_fetch_seen = true;
                }
                if (obs.halt_opcode_fetch_seen && h.dut.dbg_pc == cfg.origin + 1) {
                    obs.halt_cycles_before_ack++;
                }
            }
        }

        // Acknowledge tracking on the single shared motherboard net (plus_mode & ~M1_n & iorq).
        bool int_ack = h.dut.dbg_int_ack;
        if (int_ack && !prev_int_ack) {
            obs.ack_rises++;
            if (obs.ack_rises == 1) {
                obs.first_ack_tick = tick;
                obs.first_ack_a = h.dut.dbg_addr;
            }
        }
        if (int_ack) {
            obs.ack_clocks++;
            if (obs.ack_clocks == 1) {
                // Initial one-clock registered latency:
                // ack_src is latched on posedge clk while intack && !intack_d.
                obs.ga_pending_ack_clock1 = ga_pending;
                obs.ack_vec_clock1 = h.dut.dbg_vec_byte;
            } else if (obs.ack_clocks == 2) {
                // Captured unconditionally exactly at second master-clock observation of ack:
                // ack_src is now latched and driving the vector byte.
                obs.ga_pending_ack_clock2 = ga_pending;
                obs.ack_vec_clock2 = h.dut.dbg_vec_byte;
                obs.ack_vec_clock2_seen = true;
            } else {
                // Compare every remaining clock against the vector captured at clock 2
                if (h.dut.dbg_vec_byte != obs.ack_vec_clock2) {
                    obs.vec_unstable = true;
                }
                if (ga_pending) {
                    obs.ga_reasserted_in_ack = true;
                }
            }
        }
        prev_int_ack = int_ack;

        // Post-acknowledge / handler tracking: ensure request remains clear through handler
        if (obs.ack_rises > 0 && !int_ack) {
            if (ga_pending) {
                obs.ga_reasserted_in_handler = true;
            }
        }

        // Handler opcode fetch
        if (!h.dut.dbg_m1_n && !h.dut.dbg_mreq_n && !h.dut.dbg_rd_n && h.dut.dbg_wait_n) {
            if (h.dut.dbg_addr == 0x0600) obs.entered_raster_handler = true;
            if (h.dut.dbg_addr == 0x0400) obs.entered_dma0_handler = true;
            if (h.dut.dbg_addr == 0x0500) obs.entered_dma2_handler = true;
            if (h.dut.dbg_addr == 0x0700) obs.entered_unexpected_handler = true;
        }

        // DCSR read in handler
        if (h.dut.dbg_addr == 0x6C0F && !h.dut.dbg_mreq_n && !h.dut.dbg_rd_n && h.dut.dbg_wait_n) {
            obs.dcsr_read_seen = true;
            obs.dcsr_val = h.dut.dbg_din;
        }

        // Marker write in handler (only after entering a handler)
        if ((obs.entered_raster_handler || obs.entered_dma0_handler ||
             obs.entered_dma2_handler || obs.entered_unexpected_handler) &&
            h.dut.dbg_addr == 0x1001 && !h.dut.dbg_mreq_n && !h.dut.dbg_wr_n) {
            obs.marker_written = true;
            obs.marker_val = h.dut.dbg_dout;
            completion_tick = tick;
        }

        if (obs.marker_written && tick > completion_tick + 100) {
            break;
        }
    }

    // Rigorous assertions
    require(obs.unlock_seen, cfg.name + ": ASIC unlock sequence failed");
    require(obs.asic_page_seen, cfg.name + ": ASIC register page not enabled");
    require(obs.origin_seen, cfg.name + ": CPU never reached target execution origin 0x" + hex_str(cfg.origin, 4));

    // Blocker (2): Prior execution / transfer evidence and pinned instruction
    if (cfg.is_ldir) {
        require(obs.ldir_reads_before_ack > 0 && obs.ldir_writes_before_ack > 0,
                cfg.name + ": LDIR data transfers not observed prior to acknowledge (reads=" +
                std::to_string(obs.ldir_reads_before_ack) + " writes=" + std::to_string(obs.ldir_writes_before_ack) + ")");
        require(obs.ldir_ed_fetch_seen && obs.ldir_b0_fetch_seen,
                cfg.name + ": LDIR ED B0 opcode fetches not observed at origin+9/origin+10 prior to acknowledge");
        require(obs.first_ack_a == cfg.origin + 9,
                cfg.name + ": Expected first acknowledge address to pin interrupted LDIR at 0x" +
                hex_str(cfg.origin + 9, 4) + ", got 0x" + hex_str(obs.first_ack_a, 4));
    } else {
        require(obs.halt_opcode_fetch_seen,
                cfg.name + ": HALT opcode fetch not observed at origin 0x" + hex_str(cfg.origin, 4));
        require(obs.halt_cycles_before_ack > 0,
                cfg.name + ": CPU did not enter/remain in HALT state (PC at origin+1) prior to acknowledge");
        require(obs.first_ack_a == cfg.origin + 1,
                cfg.name + ": Expected first acknowledge address to pin HALT control at 0x" +
                hex_str(cfg.origin + 1, 4) + ", got 0x" + hex_str(obs.first_ack_a, 4));
    }

    // Blocker (3): GA pending request lifecycle (asserted before ack, deasserted at clock 2, clear through handler)
    require(obs.ga_pending_before_ack, cfg.name + ": GA interrupt request was not asserted prior to acknowledge");
    require(obs.ga_pending_ack_clock1, cfg.name + ": GA interrupt request was not asserted during first clock of acknowledge");
    require(!obs.ga_pending_ack_clock2, cfg.name + ": GA interrupt request not deasserted after first latch edge (clock 2) of acknowledge");
    require(!obs.ga_reasserted_in_ack, cfg.name + ": GA interrupt request unexpectedly reasserted during acknowledge");
    require(!obs.ga_reasserted_in_handler, cfg.name + ": GA interrupt request unexpectedly reasserted during handler execution");
    require(!obs.dma_int_asserted, cfg.name + ": DMA interrupt request unexpectedly asserted");

    // Blocker (1) & (4): Single acknowledge on shared net, registered latency separation, clock 2 unconditional capture
    require(obs.ack_rises > 0, cfg.name + ": No interrupt acknowledge observed on shared net (timeout waiting for PRI=24 raster fire)");
    require(obs.ack_rises == 1, cfg.name + ": Expected single acknowledge on shared net in this cell, got " + std::to_string(obs.ack_rises));
    require(obs.ack_vec_clock2_seen, cfg.name + ": Acknowledge did not persist to second master clock");
    require(obs.ack_vec_clock2 == 0x06, cfg.name + ": Expected vector 0x06 (raster) at second master clock of ack, got 0x" + hex_str(obs.ack_vec_clock2, 2));
    require(!obs.vec_unstable, cfg.name + ": Vector fluctuated during remaining clocks of acknowledge");

    // Handlers & DCSR
    require(!obs.entered_dma0_handler, cfg.name + ": S29 bug vector entered (DMA0 handler at 0x0400 entered)!");
    require(!obs.entered_dma2_handler, cfg.name + ": Empty/DMA2 vector entered (handler at 0x0500 entered)!");
    require(!obs.entered_unexpected_handler, cfg.name + ": Unexpected handler entered (0x0700)!");
    require(obs.entered_raster_handler, cfg.name + ": CPU failed to enter raster handler at 0x0600 (vector 0x06 not consumed)");
    require(obs.dcsr_read_seen, cfg.name + ": Handler did not read DCSR from &6C0F");
    require((obs.dcsr_val & 0x80) != 0, cfg.name + ": DCSR bit 7 (last_raster) not set (got 0x" + hex_str(obs.dcsr_val, 2) + ")");
    require((obs.dcsr_val & 0x70) == 0, cfg.name + ": Uncontrolled DMA flags in DCSR (got 0x" + hex_str(obs.dcsr_val, 2) + ")");
    require((obs.dcsr_val & 0x07) == 0, cfg.name + ": Uncontrolled DMA enables in DCSR (got 0x" + hex_str(obs.dcsr_val, 2) + ")");
    require(obs.marker_written && obs.marker_val == 0xAA,
            cfg.name + ": Completion marker not written or incorrect (got 0x" + hex_str(obs.marker_val, 2) + ")");

    std::cout << "  Setup: unlock=OK asic_page=OK origin=0x" << hex_str(cfg.origin, 4) << std::endl;
    if (cfg.is_ldir) {
        std::cout << "  LDIR transfer before ack: reads=" << obs.ldir_reads_before_ack
                  << " writes=" << obs.ldir_writes_before_ack
                  << " fetches: ED@0x" << hex_str(cfg.origin + 9, 4) << "=OK B0@0x" << hex_str(cfg.origin + 10, 4) << "=OK" << std::endl;
    } else {
        std::cout << "  HALT execution before ack: opcode@0x" << hex_str(cfg.origin, 4)
                  << "=OK halted_pc_clocks=" << obs.halt_cycles_before_ack << std::endl;
    }
    std::cout << "  GA request lifecycle: pending_before_ack=YES clock1_asserted="
              << (obs.ga_pending_ack_clock1 ? "YES" : "NO")
              << " clock2_deasserted=" << (!obs.ga_pending_ack_clock2 ? "YES" : "NO")
              << " handler_clear=" << (!obs.ga_reasserted_in_handler ? "YES" : "NO")
              << " dma_req=NONE" << std::endl;
    std::cout << "  Acknowledge (shared net): rises=" << obs.ack_rises << " duration=" << obs.ack_clocks
              << " clocks, first_A=0x" << hex_str(obs.first_ack_a, 4)
              << " (pins " << (cfg.is_ldir ? "LDIR at origin+9" : "HALT at origin+1")
              << ", A13=" << ((obs.first_ack_a & 0x2000) ? 1 : 0) << ")" << std::endl;
    std::cout << "  Vector sampling: clock1(registered_latency)=0x" << hex_str(obs.ack_vec_clock1, 2)
              << " clock2(unconditional)=0x" << hex_str(obs.ack_vec_clock2, 2)
              << " clocks3.." << obs.ack_clocks << "=0x" << hex_str(obs.ack_vec_clock2, 2)
              << " (STABLE)" << std::endl;
    std::cout << "  Handler entered: 0x0600 (raster, vector consumed), marker=0x" << hex_str(obs.marker_val, 2) << std::endl;
    std::cout << "  DCSR read: 0x" << hex_str(obs.dcsr_val, 2)
              << " (bit 7 last_raster=1, DMA_INT=0, DMA_ENA=0)" << std::endl;
    std::cout << "  PASS: single ack on shared net, exact request deassertion, pinned interrupted opcode, stable raster vector" << std::endl;

    return obs;
}

} // namespace

int main(int argc, char** argv) {
    try {
        Verilated::commandArgs(argc, argv);
        std::cout << "=== B20 Production-Bus Discriminator ===" << std::endl;
        std::cout << "Target: production T80 + motherboard/ASIC acknowledge/vector/request path" << std::endl;

        const std::vector<CellConfig> cells = {
            {"LDIR @ 0x0100 (A13=0, susceptible class)", 0x0100, true},
            {"LDIR @ 0x2100 (A13=1, control class)",     0x2100, true},
            {"HALT @ 0x0100 (A13=0, single-byte control)", 0x0100, false},
        };

        for (const auto& cell : cells) {
            run_cell(cell);
        }

        std::cout << "\n=== Summary & Discriminator Report ===" << std::endl;
        std::cout << "All 3 cells PASSED on production bus:" << std::endl;
        std::cout << "  - LDIR @ 0x0100 (A13=0): exactly 1 ack, vector 0x06 stable, raster handler fetched, DCSR=0x80" << std::endl;
        std::cout << "  - LDIR @ 0x2100 (A13=1): exactly 1 ack, vector 0x06 stable, raster handler fetched, DCSR=0x80" << std::endl;
        std::cout << "  - HALT @ 0x0100 (A13=0): exactly 1 ack, vector 0x06 stable, raster handler fetched, DCSR=0x80" << std::endl;
        std::cout << "First-divergence evidence in evaluated cells:" << std::endl;
        std::cout << "  S29 claim (LK106/IC116 double acknowledge -> DMA0 0x04) is absent from the production core." << std::endl;
        std::cout << "  Production Amstrad_motherboard routes a single shared acknowledge net (plus_mode & ~M1_n & iorq)" << std::endl;
        std::cout << "  directly from CPU to ASIC intack without pulse-multiplier logic." << std::endl;
        std::cout << "  In all 3 evaluated cells, exactly one acknowledge pulse occurs on this net." << std::endl;
        std::cout << "  No secondary acknowledge occurs; S29 empty-vector fallback to DMA0 (0x04) is unreachable in these cells." << std::endl;
        std::cout << "EXPECTED-MISMATCH to S29 hardware claim: production exhibits single acknowledge across evaluated cells." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << std::endl;
        return 1;
    }
}
