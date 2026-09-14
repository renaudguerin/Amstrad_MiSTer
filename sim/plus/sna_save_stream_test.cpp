#include <algorithm>
#include "Vsna_save_stream_test_top.h"
#include "verilated.h"

#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr uint8_t CMD_ACTIVE  = 0b011;
constexpr uint8_t CMD_READ    = 0b101;
constexpr uint8_t CMD_WRITE   = 0b100;
constexpr uint8_t CMD_REFRESH = 0b001;

constexpr uint32_t BASE_WORD = 0x3E000000 >> 3; // 0x07C00000

struct Command {
    uint8_t kind;
    uint8_t bank;
    uint32_t address;
};

struct DDRWrite {
    uint32_t addr;
    uint64_t data;
    uint64_t cycle;
};

struct TestState {
    int failures = 0;

    void check(bool condition, const std::string &message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << '\n';
        }
    }
};

class Harness {
  public:
    Vsna_save_stream_test_top dut;
    std::unordered_map<uint32_t, uint8_t> sdram_memory;
    std::unordered_map<uint32_t, uint64_t> ddr3_memory;
    std::vector<Command> sdram_commands;
    std::vector<DDRWrite> ddr_writes;
    uint64_t cycles = 0;

    // Physical SDRAM model state
    uint16_t active_row = 0;
    uint8_t active_bank = 0;
    uint16_t read_word = 0;
    int read_drive_cycles = 0;

    // Protocol checking state
    bool check_protocol = true;
    bool stalled_write = false;
    uint32_t stalled_addr = 0;
    uint64_t stalled_din = 0;

    TestState *current_test = nullptr;

    Harness() {
        dut.clk = 0;
        dut.clkref = 0;
        dut.reset = 1;
        dut.stream_reset = 0;
        dut.mux_reset = 0;
        dut.init = 1;
        dut.start = 0;
        dut.ram128 = 1;
        dut.bank = 0;
        for (int i = 0; i < 64; ++i) dut.header[i] = 0;

        dut.a_addr = 0;
        dut.a_din = 0;
        dut.a_be = 0xFF;
        dut.a_burstcnt = 1;
        dut.a_we = 0;

        dut.ddram_busy = 0;

        dut.tape_rd = 0;
        dut.tape_addr = 0;
        dut.vram_addr = 0;
        dut.vram_bank = 0;

        dut.cs_req = 0;
        dut.cs_bank = 0;
        dut.cs_addr = 0;

        dut.memory_dq = 0;
        dut.memory_dq_oe = 0;
        dut.eval();
    }

    ~Harness() { dut.final(); }

    void set_header_byte(uint8_t offset, uint8_t val) {
        const int word_idx = offset / 4;
        const int byte_idx = offset % 4;
        const uint32_t mask = ~(0xFFU << (byte_idx * 8));
        dut.header[word_idx] = (dut.header[word_idx] & mask) | (static_cast<uint32_t>(val) << (byte_idx * 8));
    }

    uint8_t get_header_byte(uint8_t offset) const {
        const int word_idx = offset / 4;
        const int byte_idx = offset % 4;
        return static_cast<uint8_t>((dut.header[word_idx] >> (byte_idx * 8)) & 0xFFU);
    }

    static uint32_t sdram_key(uint8_t bank, uint32_t byte_address) {
        return (static_cast<uint32_t>(bank) << 23) | (byte_address & 0x7FFFFFU);
    }

    uint8_t sdram_load(uint8_t bank, uint32_t byte_address) const {
        const auto it = sdram_memory.find(sdram_key(bank, byte_address));
        return it == sdram_memory.end() ? 0xFF : it->second;
    }

    void sdram_store(uint8_t bank, uint32_t byte_address, uint8_t value) {
        sdram_memory[sdram_key(bank, byte_address)] = value;
    }

    uint8_t sdram_cmd() const {
        return static_cast<uint8_t>((dut.sdram_nras << 2) | (dut.sdram_ncas << 1) | dut.sdram_nwe);
    }

    uint32_t sdram_command_address() const {
        return (active_row << 9) |
               ((static_cast<uint32_t>(dut.sdram_a) & 0x100U) << 14) |
               ((static_cast<uint32_t>(dut.sdram_a) & 0xFFU) << 1);
    }

    void tick() {
        dut.clkref = ((cycles & 7U) == 0U);
        dut.memory_dq_oe = (read_drive_cycles > 0);
        dut.memory_dq = read_word;

        dut.clk = 0;
        dut.eval();

        // Avalon DDR3 protocol assertion:
        // While stalled (previous clock edge saw ddram_we && ddram_busy),
        // we, addr, and din must remain stable until accepted.
        if (check_protocol && current_test && stalled_write) {
            current_test->check(dut.ddram_we == 1,
                                "Avalon protocol: ddram_we dropped while stalled by ddram_busy");
            current_test->check(dut.ddram_addr == stalled_addr,
                                "Avalon protocol: ddram_addr changed while stalled by ddram_busy");
            current_test->check(dut.ddram_din == stalled_din,
                                "Avalon protocol: ddram_din changed while stalled by ddram_busy");
        }

        // Check write acceptance or stall condition at the clock edge
        if (dut.ddram_we && !dut.ddram_busy) {
            ddr3_memory[dut.ddram_addr] = dut.ddram_din;
            ddr_writes.push_back({dut.ddram_addr, dut.ddram_din, cycles});
            stalled_write = false;
        } else if (dut.ddram_we && dut.ddram_busy) {
            stalled_write = true;
            stalled_addr  = dut.ddram_addr;
            stalled_din   = dut.ddram_din;
        } else {
            stalled_write = false;
        }

        dut.clk = 1;
        dut.eval();

        // Physical SDRAM command observer & response
        bool started_read = false;
        const uint8_t cmd = sdram_cmd();
        if (cmd == CMD_ACTIVE) {
            active_row  = dut.sdram_a & 0x1FFFU;
            active_bank = dut.sdram_ba;
            sdram_commands.push_back({cmd, active_bank, static_cast<uint32_t>(active_row << 9)});
        } else if (cmd == CMD_READ) {
            const uint32_t address = sdram_command_address();
            read_word = static_cast<uint16_t>(sdram_load(active_bank, address)) |
                        (static_cast<uint16_t>(sdram_load(active_bank, address + 1)) << 8);
            read_drive_cycles = 4;
            started_read = true;
            sdram_commands.push_back({cmd, active_bank, address});
        } else if (cmd == CMD_WRITE) {
            const uint32_t address = sdram_command_address();
            const uint16_t data = dut.observed_dq;
            if (!dut.sdram_dqml) sdram_store(active_bank, address, data & 0xFFU);
            if (!dut.sdram_dqmh) sdram_store(active_bank, address + 1, data >> 8);
            sdram_commands.push_back({cmd, active_bank, address});
        } else if (cmd == CMD_REFRESH) {
            sdram_commands.push_back({cmd, 0, 0});
        }

        if (read_drive_cycles > 0 && !started_read) --read_drive_cycles;

        dut.clk = 0;
        dut.eval();
        ++cycles;
    }

    void initialize(TestState &test) {
        current_test = &test;
        dut.reset = 1;
        dut.init = 1;
        for (int i = 0; i < 16; ++i) tick();
        dut.reset = 0;
        dut.init = 0;

        bool reached_normal = false;
        for (int i = 0; i < 400; ++i) {
            tick();
            if (dut.debug_mode == 0) {
                reached_normal = true;
                break;
            }
        }
        test.check(reached_normal, "SDRAM controller must complete initialization");
        for (int i = 0; i < 16; ++i) tick();
    }
};

// ============================================================================
// Case 1: 128K publication.
// Paper-derived expectations:
// - SDRAM bank 1, pages 8-15 preload: byte = (page * 37 + offset * 11 + 5) & 0xFF.
// - Header byte o = o ^ 0x5A (256 bytes = 32 64-bit words).
// - Publication order:
//     1. First DDR write: word 0 = 64'hFFFF_FFFF_FFFF_FFFF (invalid generation).
//     2. Exactly 0x4020 payload words (words 2 .. 0x4021), each written once.
//     3. Word 1 = {32'd0, 32'h8040} (file size in 32-bit words: 0x20100 / 4).
//     4. Word 0 = {32'd0, 32'd1} (generation 1).
// - Total DDR writes = 1 + 0x4020 + 1 + 1 = 0x4023.
// ============================================================================
void test_case_1_128k(Harness &h, TestState &test) {
    std::cout << "[Case 1] 128K publication\n";
    h.ddr_writes.clear();
    h.ddr3_memory.clear();

    // Preload SDRAM bank 1, pages 8-15
    for (uint32_t page = 8; page <= 15; ++page) {
        for (uint32_t offset = 0; offset < 0x4000; ++offset) {
            const uint8_t val = static_cast<uint8_t>((page * 37U + offset * 11U + 5U) & 0xFFU);
            h.sdram_store(1, (page << 14) | offset, val);
        }
    }

    // Set header pattern
    for (uint32_t o = 0; o < 256; ++o) {
        h.set_header_byte(o, static_cast<uint8_t>(o ^ 0x5AU));
    }

    h.dut.ram128 = 1;
    h.dut.bank   = 1;

    // Start save
    h.dut.start = 1;
    h.tick();
    h.dut.start = 0;

    int timeout = 6000000; // ample cycles for ~131K bytes at ~16 clocks/byte
    int done_pulses = 0;
    while (h.dut.active && --timeout > 0) {
        if (h.dut.done) ++done_pulses;
        h.tick();
    }
    if (h.dut.done) ++done_pulses;

    test.check(timeout > 0, "Case 1: Save did not time out");
    test.check(done_pulses == 1, "Case 1: done pulsed exactly once");
    test.check(!h.dut.active, "Case 1: active dropped upon completion");

    // Check DDR write count: 1 (invalid gen) + 0x4020 (payload) + 1 (len) + 1 (gen) = 0x4023
    test.check(h.ddr_writes.size() == 0x4023,
               "Case 1: exactly 0x4023 writes issued to DDR3");

    if (h.ddr_writes.size() >= 0x4023) {
        // First write: word 0 = all ones
        test.check(h.ddr_writes[0].addr == BASE_WORD + 0, "Case 1: first write is word 0");
        test.check(h.ddr_writes[0].data == 0xFFFFFFFFFFFFFFFFULL, "Case 1: first write data is all ones");

        // Payload writes: words 2 to 2 + 0x4020 - 1
        bool payload_ok = true;
        for (uint32_t i = 0; i < 0x4020; ++i) {
            if (h.ddr_writes[1 + i].addr != BASE_WORD + 2 + i) {
                payload_ok = false;
                break;
            }
        }
        test.check(payload_ok, "Case 1: exactly 0x4020 payload words written in order from word 2");

        // Word 1: length in 32-bit words (0x8040)
        test.check(h.ddr_writes[0x4021].addr == BASE_WORD + 1, "Case 1: word 1 written after payload");
        test.check(h.ddr_writes[0x4021].data == 0x0000000000008040ULL, "Case 1: word 1 value is 0x8040");

        // Final write: word 0 = generation 1
        test.check(h.ddr_writes[0x4022].addr == BASE_WORD + 0, "Case 1: last write is word 0");
        test.check(h.ddr_writes[0x4022].data == 0x0000000000000001ULL, "Case 1: word 0 final value is generation 1");
    }

    // Reconstruct file and verify against header and SDRAM contents
    bool content_ok = true;
    // Check header (first 256 bytes in words 2..33)
    for (uint32_t o = 0; o < 256; ++o) {
        const uint32_t word_addr = BASE_WORD + 2 + (o / 8);
        const uint8_t byte_val = static_cast<uint8_t>((h.ddr3_memory[word_addr] >> ((o % 8) * 8)) & 0xFFU);
        if (byte_val != (o ^ 0x5A)) {
            content_ok = false;
            break;
        }
    }
    test.check(content_ok, "Case 1: DDR payload header bytes match pattern");

    // Check RAM pages 8..15 (following 128K bytes in words 34..0x4021)
    bool ram_ok = true;
    for (uint32_t p = 0; p < 8; ++p) {
        const uint32_t page = 8 + p;
        for (uint32_t offset = 0; offset < 0x4000; ++offset) {
            const uint32_t file_k = 256 + p * 0x4000 + offset;
            const uint32_t word_addr = BASE_WORD + 2 + (file_k / 8);
            const uint8_t byte_val = static_cast<uint8_t>((h.ddr3_memory[word_addr] >> ((file_k % 8) * 8)) & 0xFFU);
            const uint8_t expected = static_cast<uint8_t>((page * 37U + offset * 11U + 5U) & 0xFFU);
            if (byte_val != expected) {
                ram_ok = false;
                break;
            }
        }
        if (!ram_ok) break;
    }
    test.check(ram_ok, "Case 1: DDR payload RAM pages 8..15 match expected SDRAM data");
}

// ============================================================================
// Case 2: 64K publication.
// Paper-derived expectations:
// - ram128 = 0 on bank 0.
// - Word 1 = {32'd0, 32'h4040} (file size in 32-bit words: 0x10100 / 4).
// - Exactly 0x2020 payload words (32 header words + 8192 RAM words for pages 8-11).
// - No SDRAM ACTIVE command to pages 12-15 of bank 0 during the save.
// - A second save increments generation to 2 (word 0 final = {32'd0, 2}).
// ============================================================================
void test_case_2_64k(Harness &h, TestState &test) {
    std::cout << "[Case 2] 64K publication & generation increment\n";
    h.ddr_writes.clear();
    h.ddr3_memory.clear();
    h.sdram_commands.clear();

    // Preload SDRAM bank 0, pages 8-11 with formula, pages 12-15 with sentinel 0xA5
    for (uint32_t page = 8; page <= 11; ++page) {
        for (uint32_t offset = 0; offset < 0x4000; ++offset) {
            const uint8_t val = static_cast<uint8_t>((page * 19U + offset * 7U + 3U) & 0xFFU);
            h.sdram_store(0, (page << 14) | offset, val);
        }
    }
    for (uint32_t page = 12; page <= 15; ++page) {
        for (uint32_t offset = 0; offset < 0x4000; ++offset) {
            h.sdram_store(0, (page << 14) | offset, 0xA5);
        }
    }

    h.dut.ram128 = 0;
    h.dut.bank   = 0;

    h.dut.start = 1;
    h.tick();
    h.dut.start = 0;

    int timeout = 4000000;
    while (h.dut.active && --timeout > 0) {
        h.tick();
    }
    test.check(timeout > 0, "Case 2: 64K save did not time out");

    // Total writes: 1 (invalid gen) + 0x2020 (payload) + 1 (len) + 1 (gen) = 0x2023
    test.check(h.ddr_writes.size() == 0x2023, "Case 2: exactly 0x2023 writes issued to DDR3");
    test.check(h.ddr3_memory[BASE_WORD + 1] == 0x0000000000004040ULL, "Case 2: word 1 value is 0x4040");
    test.check(h.ddr3_memory[BASE_WORD + 0] == 0x0000000000000002ULL,
               "Case 2: second save incremented generation to 2");

    // The payload RAM words must carry pages 8-11 of bank 0 in order.
    bool ram64_ok = true;
    for (uint32_t page = 8; page <= 11 && ram64_ok; ++page) {
        for (uint32_t offset = 0; offset < 0x4000; ++offset) {
            const uint64_t k = 256 + (uint64_t(page - 8) << 14) + offset;  // file byte
            const uint64_t word = h.ddr3_memory[BASE_WORD + 2 + (k >> 3)];
            const uint8_t got = static_cast<uint8_t>(word >> ((k & 7) * 8));
            const uint8_t want = static_cast<uint8_t>((page * 19U + offset * 7U + 3U) & 0xFFU);
            if (got != want) { ram64_ok = false; break; }
        }
    }
    test.check(ram64_ok, "Case 2: 64K payload RAM pages 8..11 match the preload");

    // Verify no SDRAM ACTIVE command touched pages 12-15 of bank 0. sdram.v
    // drives the ACTIVE row with a[21:9] and the harness records row << 9, so
    // cart_addr[22:14] (8 + page index) is recorded address >> 14.
    bool touched_forbidden_pages = false;
    for (const auto &cmd : h.sdram_commands) {
        if (cmd.kind == CMD_ACTIVE && cmd.bank == 0) {
            const uint32_t page = cmd.address >> 14;
            if (page >= 12 && page <= 15) {
                touched_forbidden_pages = true;
                break;
            }
        }
    }
    test.check(!touched_forbidden_pages,
               "Case 2: no SDRAM ACTIVE command issued to pages 12-15 of bank 0");
}

// ============================================================================
// Case 3: DDR stalls.
// Paper-derived expectations:
// - Pseudo-random busy with fixed seed, including long stall runs.
// - On every clock where ddram_busy && ddram_we, addr/din/we are unchanged.
// - Reconstructed DDR memory is bit-for-bit identical to Case 1.
// ============================================================================
void test_case_3_ddr_stalls(Harness &h, TestState &test) {
    std::cout << "[Case 3] DDR stalls under pseudo-random busy\n";
    h.ddr_writes.clear();
    h.ddr3_memory.clear();

    // Re-setup Case 1 parameters (bank 1, 128K)
    for (uint32_t page = 8; page <= 15; ++page) {
        for (uint32_t offset = 0; offset < 0x4000; ++offset) {
            const uint8_t val = static_cast<uint8_t>((page * 37U + offset * 11U + 5U) & 0xFFU);
            h.sdram_store(1, (page << 14) | offset, val);
        }
    }
    for (uint32_t o = 0; o < 256; ++o) {
        h.set_header_byte(o, static_cast<uint8_t>(o ^ 0x5AU));
    }

    h.dut.ram128 = 1;
    h.dut.bank   = 1;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> run_dist(1, 40); // stall lengths up to 40 cycles
    std::uniform_int_distribution<int> coin_dist(0, 3);

    int stall_remaining = 0;

    h.dut.start = 1;
    h.tick();
    h.dut.start = 0;

    int timeout = 8000000;
    while (h.dut.active && --timeout > 0) {
        if (stall_remaining > 0) {
            --stall_remaining;
            h.dut.ddram_busy = 1;
        } else {
            if (coin_dist(rng) == 0) {
                stall_remaining = run_dist(rng);
                h.dut.ddram_busy = 1;
            } else {
                h.dut.ddram_busy = 0;
            }
        }
        h.tick();
    }
    h.dut.ddram_busy = 0;
    h.tick();

    test.check(timeout > 0, "Case 3: Save under stalls did not time out");
    test.check(h.ddr_writes.size() == 0x4023, "Case 3: exactly 0x4023 writes accepted");

    // Verify word 0 and word 1
    test.check(h.ddr3_memory[BASE_WORD + 1] == 0x0000000000008040ULL, "Case 3: word 1 is 0x8040");
    // Generation was 2 after case 2, so this third save produces generation 3
    test.check(h.ddr3_memory[BASE_WORD + 0] == 0x0000000000000003ULL, "Case 3: word 0 is generation 3");

    // Check RAM pages match Case 1 data
    bool ram_ok = true;
    for (uint32_t p = 0; p < 8; ++p) {
        const uint32_t page = 8 + p;
        for (uint32_t offset = 0; offset < 0x4000; ++offset) {
            const uint32_t file_k = 256 + p * 0x4000 + offset;
            const uint32_t word_addr = BASE_WORD + 2 + (file_k / 8);
            const uint8_t byte_val = static_cast<uint8_t>((h.ddr3_memory[word_addr] >> ((file_k % 8) * 8)) & 0xFFU);
            const uint8_t expected = static_cast<uint8_t>((page * 37U + offset * 11U + 5U) & 0xFFU);
            if (byte_val != expected) {
                ram_ok = false;
                break;
            }
        }
        if (!ram_ok) break;
    }
    test.check(ram_ok, "Case 3: Memory contents under DDR stalls match expected data");
}

// ============================================================================
// Case 4: Fairness and refresh.
// Derivation of fairness bound:
// Priority order in sdram.v at q == STATE_IDLE:
//   1. CPU RAM (held during save, 0 requests)
//   2. refresh_due (forced every 32 cart grants)
//   3. cart_req (save stream)
//   4. tape_rd | tape_wr
//   5. vram
//   6. idle refresh
//
// Because sna_save_stream yields after every cart_ack by keeping cart_req low
// through clkref_rising and the subsequent q == STATE_IDLE arbitration cycle,
// cartridge never requests on two consecutive slots. Every cartridge grant is
// separated by at least one yielded slot.
// In that yielded slot, tape has higher priority than VRAM.
// If refresh_due is 1, refresh takes that yielded slot and clears refresh_due;
// then after the next cartridge grant, tape is granted in the next yielded slot.
// Therefore, whenever tape requests, it is guaranteed to be granted in at most
// 2 cartridge grants (bound: cart_grants_between_tape_grants <= 2).
// When tape is not requesting, VRAM gets the yielded slot.
// Refreshes continue to occur throughout the save.
// ============================================================================
void test_case_4_fairness_refresh(Harness &h, TestState &test) {
    std::cout << "[Case 4] Fairness and refresh\n";
    h.sdram_commands.clear();
    h.ddr_writes.clear();

    h.dut.ram128 = 1;
    h.dut.bank   = 1;

    h.dut.start = 1;
    h.tick();
    h.dut.start = 0;

    int cart_grants = 0;
    int tape_grants = 0;
    int vram_grants = 0;
    int refresh_cmds = 0;

    int cart_grants_since_tape_req = 0;
    int cart_quarter = 0;
    int tape_grants_by_quarter[4] = {0, 0, 0, 0};
    int max_cart_gap = 0;

    bool prev_tape_ack = h.dut.tape_rd_ack;
    bool tape_active = false;
    int tape_delay = 0;

    uint32_t vram_counter = 0;

    int timeout = 8000000;
    while (h.dut.active && --timeout > 0) {
        // Track cartridge grants
        if (h.dut.stream_cart_ack) {
            ++cart_grants;
            cart_quarter = std::min(3, cart_grants * 4 / 131072);
            if (tape_active) {
                ++cart_grants_since_tape_req;
            }
        }

        // Tape client: requests streaming reads periodically with a handshake
        // (requests, waits for ack toggle, pauses briefly so VRAM gets slots, repeats)
        if (!tape_active) {
            if (++tape_delay >= 4) { // brief pause between tape requests allows VRAM slots
                tape_active = true;
                h.dut.tape_rd = 1;
                h.dut.tape_addr = 0x100000 + (tape_grants & 0xFFF);
                cart_grants_since_tape_req = 0;
            }
        } else {
            if (h.dut.tape_rd_ack != prev_tape_ack) {
                // Tape read accepted & completed!
                ++tape_grants;
                ++tape_grants_by_quarter[cart_quarter];
                prev_tape_ack = h.dut.tape_rd_ack;
                if (cart_grants_since_tape_req > max_cart_gap) {
                    max_cart_gap = cart_grants_since_tape_req;
                }
                h.dut.tape_rd = 0;
                tape_active = false;
                tape_delay = 0;
            }
        }

        // VRAM client: change address every few slots
        if ((h.cycles & 0x1F) == 0) {
            ++vram_counter;
            h.dut.vram_addr = (vram_counter & 0x7FFFF) << 1;
            h.dut.vram_bank = 0;
        }

        // Observe VRAM reads (when dut_sdram.vram_req is active)
        if (h.dut.debug_q == 7 && h.dut.debug_mode == 0) {
            // Check if VRAM was served in this slot
            // (We can check via sdram_commands)
        }

        h.tick();
    }

    test.check(timeout > 0, "Case 4: Save did not time out");

    // Count commands from recorded sdram_commands
    for (const auto &cmd : h.sdram_commands) {
        if (cmd.kind == CMD_REFRESH) ++refresh_cmds;
        if (cmd.kind == CMD_READ && cmd.bank == 0) ++vram_grants;
    }

    std::cout << "  Cartridge grants: " << cart_grants
              << ", Tape grants: " << tape_grants
              << ", VRAM reads: " << vram_grants
              << ", Refreshes: " << refresh_cmds
              << ", Max cart grants per tape grant: " << max_cart_gap << '\n';

    test.check(cart_grants == 131072, "Case 4: exactly 128K cartridge reads completed");
    test.check(tape_grants > 1000, "Case 4: tape grants kept occurring during the save");
    for (int q = 0; q < 4; ++q) {
        test.check(tape_grants_by_quarter[q] > 100,
                   "Case 4: tape served in quarter " + std::to_string(q) + " of the cartridge stream");
    }
    test.check(vram_grants > 1000, "Case 4: VRAM reads kept occurring throughout the save");
    test.check(refresh_cmds > 1000, "Case 4: SDRAM refresh kept occurring throughout the save");

    // Derived bound: max cartridge grants between tape request and grant must be <= 2
    test.check(max_cart_gap <= 2,
               "Case 4: Fairness bound verified: at most 2 cartridge grants between tape request and grant");

    // Allow registered grant to return to Master A
    for (int i = 0; i < 8; ++i) h.tick();
}

// ============================================================================
// Case 5: Mux with a second master (scripted SSM stand-in).
// Expectations:
// - Stand-in writes before save complete normally.
// - A write pending when save requests completes before save's first write.
// - While save owns bus, stand-in sees a_busy = 1; no stand-in write lands
//   between save's first and last write.
// - Stand-in write pending during save completes after save finishes.
// - No write of either master is lost or duplicated.
// ============================================================================
void test_case_5_mux_second_master(Harness &h, TestState &test) {
    std::cout << "[Case 5] Mux with second master (SSM stand-in)\n";
    h.ddr_writes.clear();

    constexpr uint32_t SSM_BASE = 0x30000000 >> 3;

    // 1. Before save: Master A issues writes 1..3
    for (uint32_t i = 1; i <= 3; ++i) {
        h.dut.a_addr = SSM_BASE + i;
        h.dut.a_din  = 0xAA00000000000000ULL | i;
        h.dut.a_we   = 1;
        do {
            h.tick();
        } while (h.dut.a_busy);
        h.dut.a_we = 0;
        h.tick();
    }
    test.check(h.ddr_writes.size() == 3, "Case 5: 3 stand-in writes completed before save");

    // 2. Master A asserts write 4 and gets stalled by ddram_busy
    h.dut.a_addr = SSM_BASE + 4;
    h.dut.a_din  = 0xAA00000000000000ULL | 4;
    h.dut.a_we   = 1;
    h.dut.ddram_busy = 1;
    for (int i = 0; i < 4; ++i) h.tick();

    // While write 4 is stalled, trigger save
    h.dut.ram128 = 0; // 64K for quicker test
    h.dut.bank   = 0;
    h.dut.start  = 1;
    h.tick();
    h.dut.start  = 0;

    // Stream requests the bus, but Master A holds a_we = 1, so b_grant must remain 0!
    test.check(!h.dut.stream_ddr_grant, "Case 5: b_grant must not be given while Master A holds a_we");

    // Release DDR busy: Master A's write 4 should complete first
    h.dut.ddram_busy = 0;
    while (h.dut.a_busy) h.tick();
    test.check(h.ddr_writes.size() == 4, "Case 5: write 4 accepted by DDR3");
    test.check(h.ddr_writes[3].addr == SSM_BASE + 4, "Case 5: write 4 is at SSM_BASE + 4");

    // Now Master A lowers a_we
    h.dut.a_we = 0;
    h.tick();

    // Now Master B should receive grant
    test.check(h.dut.stream_ddr_grant, "Case 5: Master B granted after Master A drops a_we");

    // Verify save's first write lands after write 4
    while (h.ddr_writes.size() == 4 && h.dut.active) h.tick();
    test.check(h.ddr_writes.size() >= 5 && h.ddr_writes[4].addr == BASE_WORD + 0,
               "Case 5: save's first write (word 0 all ones) occurs after stand-in write 4");

    // 3. While save owns bus, Master A attempts write 5
    h.dut.a_addr = SSM_BASE + 5;
    h.dut.a_din  = 0xAA00000000000000ULL | 5;
    h.dut.a_we   = 1;
    h.tick();

    test.check(h.dut.a_busy, "Case 5: Master A sees a_busy = 1 while save owns bus");

    // Run save to completion while Master A keeps write 5 pending
    int timeout = 4000000;
    while (h.dut.active && --timeout > 0) {
        test.check(h.dut.a_busy, "Case 5: Master A must see a_busy = 1 for duration of save");
        h.tick();
    }
    test.check(timeout > 0, "Case 5: save completed without timing out");

    // Save completed. Master B dropped request on last save write.
    // On the next clock, registered b_grant drops to 0 and ownership returns to A.
    h.tick();
    test.check(!h.dut.stream_ddr_grant, "Case 5: b_grant released after save");
    test.check(!h.dut.a_busy, "Case 5: Master A sees real busy after save");

    // Write 5 is now presented to DDR3 (ddram_we == 1, addr == SSM_BASE + 5).
    // Tick once so DDR3 slave accepts write 5.
    h.tick();
    // Now Master A deasserts a_we
    h.dut.a_we = 0;
    h.tick();

    // Master A issues write 6
    h.dut.a_addr = SSM_BASE + 6;
    h.dut.a_din  = 0xAA00000000000000ULL | 6;
    h.dut.a_we   = 1;
    h.tick();
    h.dut.a_we   = 0;
    h.tick();

    // Total writes: 4 (before/at save start) + 0x2023 (save) + 2 (writes 5 & 6) = 0x2029
    test.check(h.ddr_writes.size() == 4 + 0x2023 + 2,
               "Case 5: all writes of both masters completed with none lost or duplicated");

    // Verify write 5 and 6 landed after save's last write
    test.check(h.ddr_writes[4 + 0x2023].addr == SSM_BASE + 5, "Case 5: write 5 landed after save");
    test.check(h.ddr_writes[4 + 0x2023 + 1].addr == SSM_BASE + 6, "Case 5: write 6 landed after write 5");
}

// ============================================================================
// Case 6:
// - Start while active ignored.
// - Reset during a stalled DDR write releases the bus only after that write
//   is accepted.
// ============================================================================
void test_case_6_start_active_and_reset_quiesce(Harness &h, TestState &test) {
    std::cout << "[Case 6] Start while active ignored & reset quiesce\n";

    // Part A: Start while active ignored
    h.dut.ram128 = 0;
    h.dut.bank   = 0;
    h.dut.start  = 1;
    h.tick();
    h.dut.start  = 0;

    test.check(h.dut.active, "Case 6: save is active");

    // Issue second start with conflicting parameters
    h.dut.ram128 = 1;
    h.dut.bank   = 1;
    h.dut.start  = 1;
    h.tick();
    h.dut.start  = 0;

    // Run to completion
    int timeout = 4000000;
    while (h.dut.active && --timeout > 0) {
        h.tick();
    }
    test.check(timeout > 0, "Case 6A: did not time out");
    // Should have saved 64K (not 128K) because second start was ignored
    test.check(h.ddr3_memory[BASE_WORD + 1] == 0x0000000000004040ULL,
               "Case 6A: second start while active was ignored (length is 64K)");

    // Part B: Reset during stalled DDR write
    // Start a new save
    h.dut.start = 1;
    h.tick();
    h.dut.start = 0;

    // Wait until stream asserts ddram_we
    timeout = 1000;
    while (!h.dut.ddram_we && --timeout > 0) {
        h.tick();
    }
    test.check(h.dut.ddram_we, "Case 6B: stream asserted ddram_we");

    // Stall the write
    h.dut.ddram_busy = 1;
    h.tick();

    const uint32_t stalled_addr = h.dut.ddram_addr;
    const uint64_t stalled_din  = h.dut.ddram_din;

    // Now assert reset while write is stalled!
    h.dut.stream_reset = 1;
    h.tick();

    // Check that for several cycles of busy, ddram_we and ddr_request remain asserted!
    for (int i = 0; i < 5; ++i) {
        test.check(h.dut.ddram_we, "Case 6B: ddram_we remains high during reset while busy");
        test.check(h.dut.stream_ddr_request, "Case 6B: ddr_request remains high during reset while busy");
        test.check(h.dut.ddram_addr == stalled_addr, "Case 6B: ddram_addr held stable");
        test.check(h.dut.ddram_din == stalled_din, "Case 6B: ddram_din held stable");
        h.tick();
    }

    // Now release ddram_busy
    h.dut.ddram_busy = 0;
    h.tick();

    // On the clock after busy is released, ddram_we and ddr_request must drop!
    test.check(!h.dut.ddram_we, "Case 6B: ddram_we dropped after stalled write accepted");
    test.check(!h.dut.stream_ddr_request, "Case 6B: ddr_request dropped after stalled write accepted");
    test.check(!h.dut.active, "Case 6B: active dropped after quiesce");

    h.dut.stream_reset = 0;
    h.tick();

    // Part C: the generation survives a core reset. The host finds a new save
    // by a changed generation and resets are routine, so a counter restarted
    // at 1 could repeat a value the host already saw. Completed saves so far:
    // case 1 (1), case 2 (2), case 3 (3), case 4 (4), case 5 (5), case 6A (6);
    // the save aborted by the reset in 6B never publishes a generation. The
    // next completed save must therefore publish 7.
    h.dut.ram128 = 0;
    h.dut.bank   = 0;
    h.dut.start  = 1;
    h.tick();
    h.dut.start  = 0;
    timeout = 4000000;
    while (h.dut.active && --timeout > 0) {
        h.tick();
    }
    test.check(timeout > 0, "Case 6C: save after reset did not time out");
    test.check(h.ddr3_memory[BASE_WORD + 0] == 0x0000000000000007ULL,
               "Case 6C: generation continues at 7 after a core reset");

    // Parts D and E: a reset (stream and mux together, as the core wires one
    // reset to both) that meets the final generation write. The write is
    // either drained through backpressure (D) or accepted on the reset clock
    // (E). Either way the slave received that generation, so it is consumed:
    // the next completed save must publish the following value, never repeat
    // it, or a host copy spanning the replacement would see the same value
    // before and after a changed payload.
    auto wait_final_write = [&](const char *what) {
        int t = 4000000;
        while (!(h.dut.ddram_we && h.dut.ddram_addr == BASE_WORD &&
                 h.dut.ddram_din != 0xFFFFFFFFFFFFFFFFULL) && --t > 0) {
            h.tick();
        }
        test.check(t > 0, std::string(what) + ": final generation write reached");
    };
    auto run_save = [&](const char *what) {
        h.dut.start = 1;
        h.tick();
        h.dut.start = 0;
        int t = 4000000;
        while (h.dut.active && --t > 0) h.tick();
        test.check(t > 0, std::string(what) + ": save completed");
    };

    // D: generation 8 stalled by busy when reset arrives.
    h.dut.start = 1;
    h.tick();
    h.dut.start = 0;
    wait_final_write("Case 6D");
    h.dut.ddram_busy = 1;
    h.tick();
    const uint32_t final_addr = h.dut.ddram_addr;
    const uint64_t final_din  = h.dut.ddram_din;
    h.dut.stream_reset = 1;
    h.dut.mux_reset    = 1;
    for (int i = 0; i < 6; ++i) {
        h.tick();
        test.check(h.dut.ddram_we && h.dut.ddram_addr == final_addr && h.dut.ddram_din == final_din,
                   "Case 6D: slave keeps seeing the stalled final write through reset");
    }
    h.dut.ddram_busy = 0;
    h.tick();
    h.tick();
    h.dut.stream_reset = 0;
    h.dut.mux_reset    = 0;
    h.tick();
    test.check(h.ddr3_memory[BASE_WORD + 0] == 0x0000000000000008ULL,
               "Case 6D: drained final write published generation 8");
    run_save("Case 6D");
    test.check(h.ddr3_memory[BASE_WORD + 0] == 0x0000000000000009ULL,
               "Case 6D: next save publishes 9, not a repeated 8");

    // E: generation 10 accepted on the same clock that sees reset.
    h.dut.start = 1;
    h.tick();
    h.dut.start = 0;
    wait_final_write("Case 6E");
    h.dut.stream_reset = 1;
    h.dut.mux_reset    = 1;
    h.tick();
    h.tick();
    h.dut.stream_reset = 0;
    h.dut.mux_reset    = 0;
    h.tick();
    test.check(h.ddr3_memory[BASE_WORD + 0] == 0x000000000000000AULL,
               "Case 6E: final write accepted on the reset clock published 10");
    run_save("Case 6E");
    test.check(h.ddr3_memory[BASE_WORD + 0] == 0x000000000000000BULL,
               "Case 6E: next save publishes 11, not a repeated 10");
}

// ============================================================================
// Case 7: SDRAM cartridge-port handover (rtl/sna_cart_mux.v, slice 4c).
// In the core, the cartridge memory service shares sdram.v's cart_* port with
// the stream. Here a scripted client (cs_*) plays that service: it holds its
// request until it sees its acknowledge, then drops it. Rules used (sdram.v):
// a request is admitted at the q == STATE_IDLE arbitration and acknowledged at
// STATE_READ of the same slot, whether or not the requester still asserts it;
// the acknowledge carries the byte at the admitted address.
// A: the stream is aborted on the clock its RAM read reaches the SDRAM (ACTIVE
//    on the stream's bank and row), and the service requests on that same
//    clock. The in-flight acknowledge belongs to the stream. The service's
//    acknowledge must come after a READ at its own address and carry its own
//    byte.
// B: the service requests on the clock after a stream acknowledge, while the
//    stream keeps reading. The stream re-requests one clock after each
//    arbitration, so the service must still be served within a bounded time
//    (64 clocks = 8 SDRAM slots here), with its own byte; and the stream's
//    following acknowledges must carry the bytes at its addresses.
// Model memory is the reference for every byte: bank 3 0x012344 = 0xA7,
// bank 3 0x023456 = 0x5E, stream page 8 offset 0 on bank 1 (0x020000) = 0x3C.
// ============================================================================
void test_case_7_cart_port_handover(Harness &h, TestState &test) {
    std::cout << "[Case 7] SDRAM cartridge-port handover\n";

    h.sdram_store(3, 0x012344, 0xA7);
    h.sdram_store(3, 0x023456, 0x5E);
    h.sdram_store(1, 0x020000, 0x3C);

    auto start_stream = [&]() {
        h.dut.ram128 = 1;
        h.dut.bank   = 1;
        h.dut.start  = 1;
        h.tick();
        h.dut.start  = 0;
    };
    auto read_seen = [&](size_t from, uint8_t bank, uint32_t address) {
        for (size_t i = from; i < h.sdram_commands.size(); ++i) {
            const Command &c = h.sdram_commands[i];
            if (c.kind == CMD_READ && c.bank == bank && (c.address & ~1U) == (address & ~1U)) return true;
        }
        return false;
    };

    // Part A: abort with a read in flight.
    start_stream();
    size_t mark = h.sdram_commands.size();
    int timeout = 20000;
    bool in_flight = false;
    while (!in_flight && --timeout > 0) {
        h.tick();
        for (size_t i = mark; i < h.sdram_commands.size(); ++i) {
            const Command &c = h.sdram_commands[i];
            if (c.kind == CMD_ACTIVE && c.bank == 1 && c.address == 0x020000) in_flight = true;
        }
        mark = h.sdram_commands.size();
    }
    test.check(in_flight, "Case 7A: stream RAM read reached the SDRAM");

    h.dut.stream_reset = 1;
    h.dut.cs_req  = 1;
    h.dut.cs_bank = 3;
    h.dut.cs_addr = 0x012344;
    const size_t cs_mark = h.sdram_commands.size();
    h.tick();
    h.dut.stream_reset = 0;

    bool stale_to_stream = false;
    bool cs_done = false;
    for (int i = 0; i < 400 && !cs_done; ++i) {
        h.tick();
        if (h.dut.stream_cart_ack) stale_to_stream = true;
        if (h.dut.cs_ack) {
            cs_done = true;
            test.check(read_seen(cs_mark, 3, 0x012344),
                       "Case 7A: service acknowledge follows a READ at its own address");
            test.check(h.dut.stream_cart_dout == 0xA7,
                       "Case 7A: service acknowledge carries its own byte, not the aborted stream's");
            h.dut.cs_req = 0;
        }
    }
    test.check(stale_to_stream, "Case 7A: the in-flight acknowledge was routed to the aborted stream");
    test.check(cs_done, "Case 7A: service request completed");
    for (int i = 0; i < 32; ++i) h.tick();

    // Part B: bounded service wait during an active stream, data integrity both ways.
    start_stream();
    timeout = 20000;
    while (!h.dut.stream_cart_ack && --timeout > 0) h.tick();
    test.check(timeout > 0, "Case 7B: stream reached its first acknowledge");

    h.dut.cs_req  = 1;
    h.dut.cs_bank = 3;
    h.dut.cs_addr = 0x023456;
    int waited = 0;
    cs_done = false;
    int stream_acks_checked = 0;
    for (int i = 0; i < 2000 && (stream_acks_checked < 8 || !cs_done); ++i) {
        h.tick();
        if (!cs_done) ++waited;
        if (h.dut.cs_ack) {
            test.check(!cs_done, "Case 7B: one acknowledge per service request");
            test.check(h.dut.stream_cart_dout == 0x5E, "Case 7B: service acknowledge carries its own byte");
            cs_done = true;
            h.dut.cs_req = 0;
        }
        if (h.dut.stream_cart_ack && cs_done) {
            const uint8_t expected = h.sdram_load(1, h.dut.stream_cart_addr);
            test.check(h.dut.stream_cart_dout == expected,
                       "Case 7B: stream acknowledge after handover carries the byte at its address");
            ++stream_acks_checked;
        }
    }
    test.check(cs_done && waited <= 64, "Case 7B: service served within 64 clocks while the stream is active");
    test.check(stream_acks_checked >= 8, "Case 7B: stream resumed after the service request");

    h.dut.stream_reset = 1;
    h.tick();
    h.dut.stream_reset = 0;
    for (int i = 0; i < 32; ++i) h.tick();
}

} // namespace

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    TestState test;
    Harness h;

    std::cout << "Starting B18 slice 4b sna_save_stream testbench...\n";
    h.initialize(test);

    // "handover" runs only case 7, which sets up its own memory and needs no
    // earlier case; used for quick mutant checks of sna_cart_mux.
    if (argc > 1 && std::string(argv[1]) == "handover") {
        test_case_7_cart_port_handover(h, test);
        std::cout << (test.failures == 0 ? "Case 7 PASSED\n" : "Case 7 FAILED\n");
        return test.failures == 0 ? 0 : 1;
    }

    test_case_1_128k(h, test);
    test_case_2_64k(h, test);
    test_case_3_ddr_stalls(h, test);
    test_case_4_fairness_refresh(h, test);
    test_case_5_mux_second_master(h, test);
    test_case_6_start_active_and_reset_quiesce(h, test);
    test_case_7_cart_port_handover(h, test);

    if (test.failures == 0) {
        std::cout << "All B18 slice 4b tests PASSED (0 failures).\n";
        return 0;
    } else {
        std::cerr << "B18 slice 4b test FAILED with " << test.failures << " failure(s).\n";
        return 1;
    }
}
