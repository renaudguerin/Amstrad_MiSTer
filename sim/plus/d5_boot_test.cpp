#define main p10_unused_main
#include "p10_boot_test.cpp"
#undef main
// Exercise production model configuration through executed T80 ROM-select I/O.
// D5 handoff / Arnold V mapping: ROM7 stays disc on CPC models, GX maps it
// to BASIC; 128..255 directly select the low five bits on every Plus model.
void model_controls() {
    for (unsigned model : {1U, 2U, 3U}) {
        std::vector<unsigned> selects{0, 7};
        for (unsigned sel = 128; sel < 256; ++sel) selects.push_back(sel);
        std::vector<uint8_t> program{0xf3, 0x01, 0x00, 0xdf}; // DI; LD BC,DF00
        for (unsigned sel : selects) {
            for (uint8_t b : {uint8_t(0x3e), uint8_t(sel), uint8_t(0xed),
                              uint8_t(0x79), uint8_t(0x3a), uint8_t(0), uint8_t(0xc0)})
                program.push_back(b); // LD A,sel; OUT (C),A; LD A,(C000)
        }
        program.push_back(0x76); // HALT
        program.resize(16384, 0);
        std::vector<Chunk> chunks{{"cb00", program}};
        for (unsigned page = 1; page < 32; ++page) {
            char id[5]; std::snprintf(id, sizeof(id), "cb%02u", page);
            chunks.push_back({id, std::vector<uint8_t>(16384, uint8_t(page))});
        }
        Harness h;
        h.dut.d5_key = 0; h.dut.d5_tape_in = 0;
        h.dut.production_clocking = 1;
        h.dut.plus_model_i = model;
        h.initialize(); h.download(build_cpr_image(chunks)); wait_for_cpr_apply(h);
        size_t seen = 0; bool reading = false;
        for (unsigned tick = 0; tick < 2000000 && seen < selects.size(); ++tick) {
            h.tick();
            const bool read = !h.dut.dbg_mreq_n && !h.dut.dbg_rd_n &&
                              h.dut.dbg_wait_n && h.dut.dbg_addr == 0xc000 &&
                              h.dut.dbg_cart_own && !h.dut.dbg_cart_stall;
            if (read && !reading) {
                const unsigned sel = selects[seen];
                // Both CPC Plus models require BASIC at ROM0.
                const unsigned expected = sel >= 128 ? (sel & 31) :
                    model == 1 ? 1 : sel == 7 ? 3 : 1;
                require(h.dut.d5_romsel == sel && h.dut.dbg_cart_own &&
                        h.dut.dbg_cart_page == expected,
                        "model/ROM control failed: model=" + std::to_string(model) +
                        " select=" + std::to_string(sel));
                ++seen;
            }
            reading = read;
        }
        require(seen == selects.size(), "timeout in model/ROM controls");
        std::cout << "PASS model=" << model << " ROM0/ROM7/all direct pages" << std::endl;
    }
}

// Cartridge ROM execution speed on the production T80, ASIC sequencer and
// SDRAM cartridge service with production clocking.
//
// Rule: the Gate Array/ASIC READY output is the only CPU wait source, and the
// arbitration "also applies to ROM access ... the Z80 always runs at the same
// speed, regardless of the type of memory being accessed" (CPCWiki "Gate
// Array", bus arbitration; local/scrapes/Gate Array - CPCWiki.pdf). Every
// instruction is stretched to a whole number of microseconds: 4 T-states at
// 4 MHz, 64 master ticks at 64 MHz. The CPC+ ASIC drives the cartridge /ROM
// and CA14-CA18 itself and has no other CPU wait output (CPCWiki "Gate Array
// and ASIC Pin-Outs", 160-pin ASIC). So cartridge code must run at exactly
// the RAM rate:
//   NOP        4T  -> 1 us =  64 ticks between opcode fetches
//   LD HL,nn  10T  -> 3 us = 192 ticks between opcode fetches
// The Sonic rearm trace (docs/investigations/sonic/cart-wait-2026-09-22.md)
// shows the model instead taking 128 ticks per cartridge NOP: the cartridge
// stall covers the only WAIT sample inside the READY window.
void cart_timing() {
    auto hx = [](unsigned v, int w) {
        char b[16]; std::snprintf(b, sizeof(b), "%0*X", w, v); return std::string(b);
    };
    // 0000 DI; 64 x NOP; 16 x LD HL,nn; LD (4000),HL; HALT.
    // Distinct operands make a late or open-bus (FF) data latch visible in
    // the executed addresses or in the final RAM write.
    std::vector<uint8_t> program{0xf3};
    const unsigned nops = 64, loads = 16;
    const unsigned nop_base = 0x0001, load_base = nop_base + nops;
    program.insert(program.end(), nops, 0x00);
    for (unsigned j = 0; j < loads; ++j) {
        program.push_back(0x21);
        program.push_back(uint8_t(0x10 + j));
        program.push_back(uint8_t(0x80 + j));
    }
    const unsigned store_pc = load_base + 3 * loads;
    for (uint8_t b : {uint8_t(0x22), uint8_t(0x00), uint8_t(0x40), uint8_t(0x76)})
        program.push_back(b);
    const unsigned halt_pc = store_pc + 3;
    program.resize(16384, 0x76);
    const uint8_t hl_lo = uint8_t(0x10 + loads - 1), hl_hi = uint8_t(0x80 + loads - 1);

    Harness h;
    h.dut.d5_key = 0; h.dut.d5_tape_in = 0;
    h.dut.production_clocking = 1;
    h.dut.plus_model_i = 2;
    h.initialize(); h.download(build_cpr_image({{"cb00", program}})); wait_for_cpr_apply(h);

    std::vector<std::pair<unsigned, uint64_t>> fetches;
    bool fetching = false, writing = false;
    std::vector<std::pair<unsigned, unsigned>> writes;
    for (uint64_t n = 0; n < 2000000; ++n) {
        h.tick();
        // Fetch start: the first tick with M1, MREQ and RD active.
        const bool f = !h.dut.dbg_m1_n && !h.dut.dbg_mreq_n && !h.dut.dbg_rd_n;
        if (f && !fetching) fetches.push_back({h.dut.dbg_addr, n});
        fetching = f;
        const bool w = !h.dut.dbg_mreq_n && !h.dut.dbg_wr_n;
        if (w && !writing) writes.push_back({h.dut.dbg_addr, h.dut.dbg_dout});
        writing = w;
        if (!fetches.empty() && fetches.back().first == halt_pc) break;
    }

    // The executed opcode stream must be exactly the program.
    std::vector<unsigned> expected{0x0000};
    for (unsigned i = 0; i < nops; ++i) expected.push_back(nop_base + i);
    for (unsigned j = 0; j < loads; ++j) expected.push_back(load_base + 3 * j);
    expected.push_back(store_pc);
    expected.push_back(halt_pc);
    require(fetches.size() >= expected.size(), "cart timing: program did not reach HALT");
    for (size_t i = 0; i < expected.size(); ++i)
        require(fetches[i].first == expected[i],
                "cart timing: fetch " + std::to_string(i) + " at " + hx(fetches[i].first, 4) +
                ", expected " + hx(expected[i], 4));
    require(writes.size() == 2 && writes[0] == std::make_pair(0x4000U, unsigned(hl_lo)) &&
            writes[1] == std::make_pair(0x4001U, unsigned(hl_hi)),
            "cart timing: LD (4000),HL did not store the last cartridge operand");

    // Fetch starts (MREQ edges) are not microsecond boundaries: an M1 whose
    // MREQ lands after the READY window takes a Tw in T2. Gaps equal the
    // instruction length only once consecutive fetches share a phase.
    // Worked on paper from the READY window (sequencer phases 6..26 of each
    // 64-tick microsecond) and T80pa sampling WAIT on CEN_n in T2:
    // - NOP chain: MREQ at phase 2, WAIT seen at 18, no Tw: 64 each. The
    //   first NOP follows DI's entry phase, so start at the second.
    // - LD HL,nn after that chain: M1 no Tw (64), M2 read no Tw (48), M3
    //   read MREQ at phase 50 takes one Tw (64), so the next M1 also starts
    //   at phase 50 with one Tw: the first gap is 176, not a rate. From the
    //   second LD on, M1 80 + M2 48 + M3 64 = 192 (3 us).
    std::string nop_gaps, load_gaps;
    bool ok = true;
    for (unsigned i = 2; i <= nops; ++i) {
        const uint64_t gap = fetches[i].second - fetches[i - 1].second;
        nop_gaps += " " + std::to_string(gap);
        ok = ok && gap == 64;
    }
    for (unsigned j = 2; j < loads; ++j) {
        const size_t i = 1 + nops + j;
        const uint64_t gap = fetches[i].second - fetches[i - 1].second;
        load_gaps += " " + std::to_string(gap);
        ok = ok && gap == 192;
    }
    std::cout << "NOP fetch gaps (expect 64):" << nop_gaps << std::endl;
    std::cout << "LD HL,nn fetch gaps (expect 192):" << load_gaps << std::endl;
    require(ok, "FAIL cart timing: cartridge code does not run at the READY-only rate");
    std::cout << "PASS cartridge NOP 1 us, LD HL,nn 3 us, operands latched" << std::endl;
}

// PA6b/PA6d: asic-reference §2 [ARNOLD §2.6] forbids internal RAM
// write-through under the ASIC page; §12 model table gives tape on 464+
// only (GX4000/6128+ have none). PPI PortB bit7 is tape input, PortC
// bits4/5 drive motor/output (8255 PPI source, "Port B"/"Port C").
// Run actual Z80 I/O/memory instructions through production MMU, ASIC,
// motherboard, SDRAM gates extracted from Amstrad.sv, and SDRAM storage.
// Classic CPC is deliberately outside this cartridge-based Plus vector.
void audit_page_and_tape() {
    bool disconnected_input[4] = {};
    for (unsigned model : {1U, 2U, 3U}) for (unsigned tape : {0U, 1U}) {
        std::vector<uint8_t> program{0xf3}; // DI
        auto emit = [&](std::initializer_list<uint8_t> bytes) {
            program.insert(program.end(), bytes.begin(), bytes.end());
        };
        auto write = [&](uint16_t a, uint8_t d) {
            emit({0x3e, d, 0x32, uint8_t(a), uint8_t(a >> 8)});
        };
        auto out = [&](uint16_t port, uint8_t d) {
            emit({0x01, uint8_t(port), uint8_t(port >> 8), 0x3e, d, 0xed, 0x79});
        };
        auto copy = [&](uint16_t from, uint16_t to) {
            emit({0x3a, uint8_t(from), uint8_t(from >> 8),
                  0x32, uint8_t(to), uint8_t(to >> 8)});
        };
        // Distinct RAM sentinels below sprite RAM, palette, and an unmapped
        // page address: none may become the later ASIC-page payload.
        write(0x4000, 0x5a); write(0x6400, 0xa6); write(0x7000, 0x3c);
        const uint8_t unlock[] = {0xff,0,0xff,0x77,0xb3,0x51,0xa8,0xd4,
                                 0x62,0x39,0x9c,0x46,0x2b,0x15,0x8a,0xcd};
        for (uint8_t b : unlock) out(0xbc00, b);
        out(0x7f00, 0xb8);
        write(0x4000, 0x0b); write(0x6400, 0x12); write(0x7000, 0xe7);
        copy(0x4000, 0x8000); // positive control: ASIC write really landed
        out(0x7f00, 0xa0);
        copy(0x4000, 0x8001); copy(0x6400, 0x8002); copy(0x7000, 0x8003);
        out(0xf700, 0x82); // PPI B input, C output
        for (uint8_t c : {0x00, 0x10, 0x20, 0x30}) {
            out(0xf600, c);
            emit({0x3a, uint8_t(c), 0x90}); // read marker 9000+C to sample tape pins
        }
        emit({0x01, 0x00, 0xf5, 0xed, 0x78, 0x32, 0x04, 0x80}); // IN A,(C); LD(8004),A
        const unsigned halt_pc = program.size();
        emit({0x76});
        program.resize(16384, 0x76);
        Harness h;
        h.dut.d5_key = 0;
        h.dut.d5_tape_in = tape;
        h.dut.production_clocking = 1;
        h.dut.plus_model_i = model;
        h.initialize(); h.download(build_cpr_image({{"cb00", program}})); wait_for_cpr_apply(h);
        bool reached_halt = false, reading = false;
        unsigned markers = 0;
        for (unsigned n = 0; n < 2000000; ++n) {
            h.tick();
            const bool marker = !h.dut.dbg_mreq_n && !h.dut.dbg_rd_n &&
                                (h.dut.dbg_addr & 0xffcf) == 0x9000;
            if (marker && !reading) {
                const unsigned c = h.dut.dbg_addr & 0x30;
                require(c == markers * 0x10, "PA6d: unexpected tape marker order");
                const bool motor = model == 3 && (c & 0x10);
                const bool output = model == 3 && (c & 0x20);
                require(h.dut.d5_tape_motor == motor && h.dut.d5_tape_out == output,
                        "PA6d: motherboard tape outputs wrong for model " + std::to_string(model));
                ++markers;
            }
            reading = marker;
            if (m1_memory_read(h) && h.dut.dbg_addr == halt_pc) { reached_halt = true; break; }
        }
        require(reached_halt && markers == 4, "PA6b/d: cartridge did not complete");
        // CPU-produced results are observed in the real SDRAM model below.
        const uint8_t expected[] = {0x0b, 0x5a, 0xa6, 0x3c};
        for (unsigned i = 0; i < 4; ++i)
            require(h.memory.at(0x28000 + i) == expected[i],
                    "PA6b: ASIC/RAM readback mismatch at result " + std::to_string(i));
        const bool input = bool(h.memory.at(0x28004) & 0x80);
        if (model == 3)
            require(input == bool(tape), "PA6d: 464+ tape input did not reach PPI Port B");
        else if (tape == 0)
            disconnected_input[model] = input;
        else
            // No tape on these models (§12): changing the external input
            // must not affect PB7. The source does not specify its idle level.
            require(input == disconnected_input[model],
                    "PA6d: tape input affected a model without tape");
        std::cout << "PASS PA6b/d model=" << model << " tape=" << tape
                  << " ASIC write isolation and motherboard tape input/output/motor" << std::endl;
    }
}

// PA4: original 6128 Plus V4 screen29, IMG_3971 (2026-09-28),
// docs/plus/asic-audit-probes-v4.md: absolute reads retain operand high
// bytes 50/68; LD A,(HL) retains opcode 7E. Observe CPU-written results,
// not the combinational module output. Cartridge execution also proves
// real operand fetches survive the retained-byte path.
void audit_page_readback() {
    std::vector<uint8_t> program{0xf3};
    auto emit = [&](std::initializer_list<uint8_t> bytes) {
        program.insert(program.end(), bytes.begin(), bytes.end());
    };
    auto write = [&](uint16_t a, uint8_t d) {
        emit({0x3e, d, 0x32, uint8_t(a), uint8_t(a >> 8)});
    };
    auto out = [&](uint16_t port, uint8_t d) {
        emit({0x01, uint8_t(port), uint8_t(port >> 8), 0x3e, d, 0xed, 0x79});
    };
    unsigned result = 0;
    auto save = [&]() { emit({0x32, uint8_t(result++), 0x80}); };
    auto read = [&](uint16_t a) { emit({0x3a, uint8_t(a), uint8_t(a >> 8)}); save(); };
    write(0x5000, 0xa5); write(0x6800, 0x5a);
    read(0x5000); read(0x6800); // page-off RAM controls from the photo
    const uint8_t unlock[] = {0xff,0,0xff,0x77,0xb3,0x51,0xa8,0xd4,
                             0x62,0x39,0x9c,0x46,0x2b,0x15,0x8a,0xcd};
    for (uint8_t b : unlock) out(0xbc00, b);
    out(0x7f00, 0xb8);
    read(0x5000); read(0x6800);
    for (uint16_t a : {0x5000, 0x6800}) {
        emit({0x21, uint8_t(a), uint8_t(a >> 8), 0x7e}); save();
    }
    write(0x4000, 0x0b); write(0x6400, 0x5a);
    read(0x4000); read(0x6400); // mapped controls from the photo
    // A legitimate FF read must remain driven, not look like an unclaimed
    // page read. Sprite X-high 3 reads FF (ASIC reference §4).
    write(0x6001, 3); read(0x6001);
    read(0x6808); // ADC is claimed beside the write-only register range
    out(0x7f00, 0xa0);
    read(0x5000); read(0x6800); // page-off path restored
    const unsigned halt_pc = program.size(); emit({0x76});
    program.resize(16384, 0x76);
    Harness h;
    h.dut.d5_key = 0; h.dut.d5_tape_in = 0;
    h.dut.production_clocking = 1; h.dut.plus_model_i = 2;
    h.initialize(); h.download(build_cpr_image({{"cb00", program}})); wait_for_cpr_apply(h);
    bool done = false;
    for (unsigned n = 0; n < 2000000; ++n) {
        h.tick();
        if (m1_memory_read(h) && h.dut.dbg_addr == halt_pc) { done = true; break; }
    }
    require(done, "PA4: production CPU did not reach HALT");
    const uint8_t expected[] = {0xa5,0x5a,0x50,0x68,0x7e,0x7e,0x0b,0x5a,0xff,0x3f,0xa5,0x5a};
    bool ok = true;
    for (unsigned i = 0; i < sizeof(expected); ++i) {
        const unsigned actual = h.memory.at(0x28000 + i);
        if (actual != expected[i]) {
            std::cerr << "PA4 result " << i << ": got " << std::hex << actual
                      << " expected " << unsigned(expected[i]) << std::dec << std::endl;
            ok = false;
        }
    }
    require(ok, "FAIL PA4: CPU ASIC-page readback");
    std::cout << "PASS PA4: production T80 50/68/7E/7E; RAM, mapped ASIC, driven FF and cartridge controls" << std::endl;
}

// PA1: original 6128 Plus V4 screen26, IMG_3968 (2026-09-28).
// CPU bytes and palette words are independently photographed observations:
// docs/plus/asic-audit-probes-v4.md. Keep the existing IN-performs-write
// decoder path pinned while repairing the CPU's returned byte.
void audit_ga_readback() {
    std::vector<uint8_t> program{0xf3};
    std::vector<uint8_t> expected;
    auto emit = [&](std::initializer_list<uint8_t> bytes) {
        program.insert(program.end(), bytes.begin(), bytes.end());
    };
    auto write = [&](uint16_t a, uint8_t d) {
        emit({0x3e, d, 0x32, uint8_t(a), uint8_t(a >> 8)});
    };
    auto out = [&](uint16_t port, uint8_t d) {
        emit({0x01, uint8_t(port), uint8_t(port >> 8), 0x3e, d, 0xed, 0x79});
    };
    auto save = [&](uint8_t value) {
        emit({0x32, uint8_t(expected.size()), 0x80}); expected.push_back(value);
    };
    auto palette = [&](uint16_t value) {
        emit({0x3a,0x20,0x64}); save(uint8_t(value));
        emit({0x3a,0x21,0x64}); save(uint8_t(value >> 8));
    };
    auto in_a = [&](uint16_t port) {
        emit({0x01, uint8_t(port), uint8_t(port >> 8), 0xed,0x78});
    };
    const uint8_t unlock[] = {0xff,0,0xff,0x77,0xb3,0x51,0xa8,0xd4,
                             0x62,0x39,0x9c,0x46,0x2b,0x15,0x8a,0xcd};
    for (uint8_t b : unlock) out(0xbc00, b);
    out(0x7f00, 0xb8);
    const uint8_t opcodes[] = {0x78,0x40,0x50,0x58,0x60,0x68,0x78};
    const uint8_t move_a[]  = {0x7f,0x78,0x7a,0x7b,0x7c,0x7d,0x7f};
    const uint16_t colours[] = {0x066,0x666,0x006,0x066,0x666,0x0f6,0x066};
    for (unsigned i = 0; i < 7; ++i) {
        write(0x6420, 0x0f); write(0x6421, 0); // per-row sentinel
        out(0x7f00, 0x10); // select border; reload BC after IN B
        emit({0x01, uint8_t(i == 6 ? 0x54 : 0),0x7f,0xed,opcodes[i],move_a[i]});
        save(opcodes[i]); palette(colours[i]);
    }
    out(0x7f00, 0x78); palette(0x066);
    out(0x7f00, 0x79); palette(0xf66);
    // Real responders overlapping GA's partial decode must beat retention.
    // PPI control reads replicate mode bit4 in Plus mode, including driven FF.
    out(0xf700, 0x9b); in_a(0x7700); save(0xff);
    out(0xf700, 0x82); in_a(0x7700); save(0x00);
    out(0xf600, 0xa5); in_a(0x7600); save(0xa5);
    // CRTC R12 is a readable full byte, outside the GA select.
    out(0xbc00, 12); out(0xbd00, 0xa5); in_a(0xbf00); save(0xa5);
    // Compare two aliases of the same idle production FDC status register;
    // 7B7E also selects GA, FB7E does not. Neither read consumes data.
    const unsigned fdc_result = expected.size();
    in_a(0xfb7e); save(0); in_a(0x7b7e); save(0);
    const unsigned halt_pc = 0x9000 + program.size(); emit({0x76});
    // Execute the measured stream from real SDRAM. PA4 above executes from
    // cartridge; together they pin both operand/opcode retention sources.
    const unsigned size = program.size();
    std::vector<uint8_t> cartridge{0xf3,0x31,0x00,0xc0, // DI; LD SP,C000
        0x21,0x00,0x01,0x11,0x00,0x90,                // HL=0100; DE=9000
        0x01,uint8_t(size),uint8_t(size >> 8),0xed,0xb0, // BC=size; LDIR
        0xc3,0x00,0x90};                              // JP 9000
    cartridge.resize(0x100, 0x76);
    cartridge.insert(cartridge.end(), program.begin(), program.end());
    cartridge.resize(16384, 0x76);
    Harness h;
    h.dut.d5_key = 0; h.dut.d5_tape_in = 0;
    h.dut.production_clocking = 1; h.dut.plus_model_i = 2;
    h.initialize(); h.download(build_cpr_image({{"cb00", cartridge}})); wait_for_cpr_apply(h);
    bool done = false;
    for (unsigned n = 0; n < 2000000; ++n) {
        h.tick();
        if (m1_memory_read(h) && h.dut.dbg_addr == halt_pc) { done = true; break; }
    }
    require(done, "PA1: production CPU did not reach HALT");
    bool ok = true;
    for (unsigned i = 0; i < fdc_result; ++i) {
        const unsigned actual = h.memory.at(0x28000 + i);
        if (actual != expected[i]) {
            std::cerr << "PA1 result " << i << ": got " << std::hex << actual
                      << " expected " << unsigned(expected[i]) << std::dec << std::endl;
            ok = false;
        }
    }
    require(h.memory.at(0x28000 + fdc_result) == h.memory.at(0x28001 + fdc_result) &&
            h.memory.at(0x28000 + fdc_result) != 0x78 &&
            h.memory.at(0x28000 + fdc_result) != 0xff,
            "PA1: GA-overlapping FDC status differs from ordinary FDC read");
    require(ok, "FAIL PA1: CPU GA readback or decoder palette side effect");
    std::cout << "PASS PA1: production T80 from SDRAM 78/40/50/58/60/68/78; opcode palettes and PPI/CRTC/FDC controls" << std::endl;
}

int main(int argc,char **argv) {
 try {
  Verilated::commandArgs(argc,argv);
  if(argc == 2 && std::string(argv[1]) == "--asic-audit") { audit_ga_readback(); audit_page_readback(); audit_page_and_tape(); return 0; }
  if(argc == 2 && std::string(argv[1]) == "--ga-readback") { audit_ga_readback(); return 0; }
  if(argc == 2 && std::string(argv[1]) == "--page-readback") { audit_page_readback(); return 0; }
  if(argc == 2 && std::string(argv[1]) == "--controls") { model_controls(); return 0; }
  if(argc == 2 && std::string(argv[1]) == "--cart-timing") { cart_timing(); return 0; }
  require(argc >= 2, "usage: d5_boot image.cpr [--classic-rom] [--464]");
  bool no_menu = false, model464 = false;
  for (int i = 2; i < argc; ++i) {
    const std::string option = argv[i];
    if (option == "--classic-rom") no_menu = true;
    else if (option == "--464") model464 = true;
    else throw TestFailure("unknown option: " + option);
  }
  const unsigned entry=no_menu?0xc1bc:0xc1b3, discpc=no_menu?0xc1dc:0xc1d3, errorpc=no_menu?0xc223:0xc21a;
  std::ifstream f(argv[1],std::ios::binary);
  require(bool(f),"CPR missing");
  std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});
  Harness h; h.dut.d5_key=0; h.dut.d5_tape_in=0; h.dut.production_clocking=1;
  h.dut.plus_model_i = model464 ? 3 : 2;
  h.initialize(); h.download(bytes); wait_for_cpr_apply(h);
  bool fetch=false,io=false,menu=false,disc=false,error=false,basic=false;
  uint64_t basicat=0; std::string output; bool ready=false,missing=false; uint64_t errorat=0; unsigned lastsel=256; unsigned rom_events=0, fetch_events=0;
  for(uint64_t n=0;n<320000000;n++) {
   h.tick();
   bool fi=!h.dut.dbg_m1_n&&!h.dut.dbg_mreq_n&&!h.dut.dbg_rd_n&&h.dut.dbg_wait_n;
   bool iow=!h.dut.dbg_iorq_n&&!h.dut.dbg_wr_n;
   if(iow&&!io&&! (h.dut.dbg_addr&0x2000) && h.dut.dbg_dout!=lastsel) {
    lastsel=h.dut.dbg_dout;
    if(rom_events++<100) std::cout<<"ROMSEL tick="<<std::dec<<n<<" value="<<std::hex<<lastsel<<" pc="<<h.dut.dbg_pc<<std::endl;
   }
   io=iow;
   if(fi&&!fetch) {
    unsigned a=h.dut.dbg_addr,p=h.dut.dbg_cart_page;
    // Firmware TXT OUTPUT jumpblock; CPU A is the actual character argument.
    if(a==0xbb5a) {
     unsigned ch=h.dut.d5_a;
     if(ch>=32&&ch<127) {output.push_back(char(ch)); std::cout<<"CHAR "<<char(ch)<<std::endl;}
     else if(ch==10||ch==13) output.push_back(' ');
     if(output.size()>512)output.erase(0,256);
     ready=output.find("Ready")!=std::string::npos;
     missing=output.find("missing")!=std::string::npos;
    }
    if(h.dut.dbg_cart_own && (a==entry||a==discpc||a==errorpc||a==0xce1b||a==0xc006||a==0xce06)) {
     if((a!=0xce06||!menu)&&fetch_events++<120) std::cout<<"FETCH tick="<<std::dec<<n<<" pc="<<std::hex<<a<<" page="<<p<<" romsel="<<unsigned(h.dut.d5_romsel)<<" byte="<<unsigned(h.dut.dbg_din)<<std::endl;
     if(!no_menu&&p==3&&a==0xce06&&!menu) {menu=true;h.dut.d5_key=0x605;}
     if(p==3&&a==discpc)disc=true;
     if(p==3&&a==errorpc&&!error){error=true;errorat=n;}
     if((menu||no_menu)&&a==0xc006&&h.dut.d5_romsel==0&&h.dut.dbg_din==0x31&&!basic) {basic=true;basicat=n;}
    }
   }
   fetch=fi;
   if(menu&&h.dut.dbg_pc==0xce1b)h.dut.d5_key=0x005;
   if(missing || ready || (basic&&n>basicat+32000000) || (error&&n>errorat+8000000))break;
   if(n%16000000==0)std::cout<<"PROGRESS tick="<<std::dec<<n<<" pc="<<std::hex<<h.dut.dbg_pc<<std::endl;
  }
  std::cout<<"RESULT menu="<<menu<<" basic_entry="<<basic<<" ready="<<ready<<" missing="<<missing<<" disc_branch="<<disc<<" error_branch="<<error<<" fdc_writes="<<std::dec<<h.fdc_writes.size()<<" pc="<<std::hex<<h.dut.dbg_pc<<std::endl;
  if(missing) throw TestFailure("FAIL: firmware emitted disc missing before Ready");
  if(!ready) throw TestFailure("FAIL: timeout without firmware Ready output");
  std::cout << "PASS: firmware Ready output" << std::endl;
  return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<std::endl;return 1;}
}
