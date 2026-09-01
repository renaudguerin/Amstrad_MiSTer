#include "Vsdram_cartridge_test_top.h"
#include "verilated.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr uint8_t CMD_ACTIVE = 0b011;
constexpr uint8_t CMD_READ = 0b101;
constexpr uint8_t CMD_WRITE = 0b100;
constexpr uint8_t CMD_REFRESH = 0b001;

struct Command {
    uint8_t kind;
    uint8_t bank;
    uint32_t address;
    uint16_t data;
    bool dqml;
    bool dqmh;
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
    Vsdram_cartridge_test_top dut;
    std::unordered_map<uint32_t, uint8_t> memory;
    std::unordered_map<uint32_t, uint8_t> integration_memory;
    std::vector<Command> commands;
    std::vector<Command> integration_commands;
    uint64_t cycles = 0;

    Harness() {
        dut.clk = 0;
        dut.clkref = 0;
        dut.init = 1;
        dut.bank = 0;
        dut.din = 0;
        dut.addr = 0;
        dut.oe = 0;
        dut.we = 0;
        dut.cart_req = 0;
        dut.cart_wr = 0;
        dut.cart_bank = 0;
        dut.cart_addr = 0;
        dut.cart_din = 0;
        dut.vram_addr = 0;
        dut.vram_bank = 0;
        dut.tape_addr = 0;
        dut.tape_din = 0;
        dut.tape_wr = 0;
        dut.tape_rd = 0;
        dut.memory_dq = 0;
        dut.memory_dq_oe = 0;
        dut.service_cold_reset = 0;
        dut.service_detach = 0;
        dut.service_load_begin = 0;
        dut.service_load_commit = 0;
        dut.service_load_abort = 0;
        dut.service_load_valid = 0;
        dut.service_load_page = 0;
        dut.service_load_offset = 0;
        dut.service_load_data = 0;
        dut.service_cpu_valid = 0;
        dut.service_cpu_page = 0;
        dut.service_cpu_offset = 0;
        dut.integration_memory_dq = 0;
        dut.integration_memory_dq_oe = 0;
        dut.eval();
    }

    ~Harness() { dut.final(); }

    uint32_t key(uint8_t bank, uint32_t byte_address) const {
        return (static_cast<uint32_t>(bank) << 23) | (byte_address & 0x7fffffU);
    }

    uint8_t load(uint8_t bank, uint32_t byte_address) const {
        const auto it = memory.find(key(bank, byte_address));
        return it == memory.end() ? 0xff : it->second;
    }

    void store(uint8_t bank, uint32_t byte_address, uint8_t value) {
        memory[key(bank, byte_address)] = value;
    }

    uint8_t integration_load(uint8_t bank, uint32_t byte_address) const {
        const auto it = integration_memory.find(key(bank, byte_address));
        return it == integration_memory.end() ? 0xff : it->second;
    }

    void integration_store(uint8_t bank, uint32_t byte_address, uint8_t value) {
        integration_memory[key(bank, byte_address)] = value;
    }

    uint8_t command() const {
        return static_cast<uint8_t>((dut.sdram_nras << 2) |
                                    (dut.sdram_ncas << 1) | dut.sdram_nwe);
    }

    uint32_t command_address() const {
        return (active_row << 9) |
               ((static_cast<uint32_t>(dut.sdram_a) & 0x100U) << 14) |
               ((static_cast<uint32_t>(dut.sdram_a) & 0xffU) << 1);
    }

    uint8_t integration_command() const {
        return static_cast<uint8_t>((dut.integration_sdram_nras << 2) |
                                    (dut.integration_sdram_ncas << 1) |
                                    dut.integration_sdram_nwe);
    }

    uint32_t integration_command_address() const {
        return (integration_active_row << 9) |
               ((static_cast<uint32_t>(dut.integration_sdram_a) & 0x100U) << 14) |
               ((static_cast<uint32_t>(dut.integration_sdram_a) & 0xffU) << 1);
    }

    void tick() {
        dut.clkref = ((cycles & 7U) == 0U);
        dut.memory_dq_oe = read_drive_cycles > 0;
        dut.memory_dq = read_word;
        dut.integration_memory_dq_oe = integration_read_drive_cycles > 0;
        dut.integration_memory_dq = integration_read_word;

        dut.clk = 0;
        dut.eval();
        dut.clk = 1;
        dut.eval();

        bool started_read = false;
        const uint8_t cmd = command();
        if (cmd == CMD_ACTIVE) {
            active_row = dut.sdram_a & 0x1fffU;
            active_bank = dut.sdram_ba;
            commands.push_back({cmd, active_bank,
                                static_cast<uint32_t>(active_row << 9), 0,
                                static_cast<bool>(dut.sdram_dqml),
                                static_cast<bool>(dut.sdram_dqmh)});
        } else if (cmd == CMD_READ) {
            const uint32_t address = command_address();
            read_word = static_cast<uint16_t>(load(active_bank, address)) |
                        (static_cast<uint16_t>(load(active_bank, address + 1)) << 8);
            read_drive_cycles = 4;
            started_read = true;
            commands.push_back({cmd, active_bank, address, read_word,
                                static_cast<bool>(dut.sdram_dqml),
                                static_cast<bool>(dut.sdram_dqmh)});
        } else if (cmd == CMD_WRITE) {
            const uint32_t address = command_address();
            const uint16_t data = dut.observed_dq;
            if (!dut.sdram_dqml) store(active_bank, address, data & 0xffU);
            if (!dut.sdram_dqmh) store(active_bank, address + 1, data >> 8);
            commands.push_back({cmd, active_bank, address, data,
                                static_cast<bool>(dut.sdram_dqml),
                                static_cast<bool>(dut.sdram_dqmh)});
        } else if (cmd == CMD_REFRESH) {
            commands.push_back({cmd, 0, 0, 0, true, true});
        }

        if (read_drive_cycles > 0 && !started_read) --read_drive_cycles;

        bool integration_started_read = false;
        const uint8_t integration_cmd = integration_command();
        if (integration_cmd == CMD_ACTIVE) {
            integration_active_row = dut.integration_sdram_a & 0x1fffU;
            integration_active_bank = dut.integration_sdram_ba;
            integration_commands.push_back(
                {integration_cmd, integration_active_bank,
                 static_cast<uint32_t>(integration_active_row << 9), 0,
                 static_cast<bool>(dut.integration_sdram_dqml),
                 static_cast<bool>(dut.integration_sdram_dqmh)});
        } else if (integration_cmd == CMD_READ) {
            const uint32_t address = integration_command_address();
            integration_read_word =
                static_cast<uint16_t>(integration_load(integration_active_bank, address)) |
                (static_cast<uint16_t>(
                     integration_load(integration_active_bank, address + 1))
                 << 8);
            integration_read_drive_cycles = 4;
            integration_started_read = true;
            integration_commands.push_back(
                {integration_cmd, integration_active_bank, address,
                 integration_read_word,
                 static_cast<bool>(dut.integration_sdram_dqml),
                 static_cast<bool>(dut.integration_sdram_dqmh)});
        } else if (integration_cmd == CMD_WRITE) {
            const uint32_t address = integration_command_address();
            const uint16_t data = dut.integration_observed_dq;
            if (!dut.integration_sdram_dqml)
                integration_store(integration_active_bank, address, data & 0xffU);
            if (!dut.integration_sdram_dqmh)
                integration_store(integration_active_bank, address + 1, data >> 8);
            integration_commands.push_back(
                {integration_cmd, integration_active_bank, address, data,
                 static_cast<bool>(dut.integration_sdram_dqml),
                 static_cast<bool>(dut.integration_sdram_dqmh)});
        } else if (integration_cmd == CMD_REFRESH) {
            integration_commands.push_back(
                {integration_cmd, 0, 0, 0, true, true});
        }
        if (integration_read_drive_cycles > 0 && !integration_started_read)
            --integration_read_drive_cycles;

        dut.clk = 0;
        dut.eval();
        ++cycles;
    }

    void initialize(TestState &test, bool request_during_init = false) {
        if (request_during_init) {
            dut.cart_req = 1;
            dut.cart_bank = 3;
            dut.cart_addr = 0x080000;
        }

        const size_t before = commands.size();
        for (int i = 0; i < 16; ++i) tick();
        test.check(!dut.cart_ack, "cartridge acknowledge must be excluded while init is asserted");
        for (size_t i = before; i < commands.size(); ++i) {
            test.check(commands[i].kind != CMD_ACTIVE,
                       "no client ACTIVE command may be issued while init is asserted");
        }

        dut.init = 0;
        bool reached_normal = false;
        for (int i = 0; i < 400; ++i) {
            tick();
            if (dut.debug_mode == 0) {
                reached_normal = true;
                break;
            }
            test.check(!dut.cart_ack,
                       "cartridge acknowledge must be excluded during SDRAM command initialization");
        }
        test.check(reached_normal, "SDRAM controller must finish initialization");
        if (request_during_init) dut.cart_req = 0;
        for (int i = 0; i < 16; ++i) tick();
    }

    void align_before_idle() {
        // clkref resets q on the slot boundary. q remains zero until the next
        // rising edge, which is the arbitration edge where inputs are sampled.
        for (int i = 0; i < 24; ++i) {
            if (dut.debug_q == 0 && ((cycles & 7U) != 0U)) return;
            tick();
        }
        throw std::runtime_error("failed to align to SDRAM IDLE arbitration");
    }

    uint8_t cart_transaction(TestState &test, bool write, uint8_t bank,
                             uint32_t address, uint8_t data = 0) {
        dut.cart_wr = write;
        dut.cart_bank = bank;
        dut.cart_addr = address;
        dut.cart_din = data;
        dut.cart_req = 1;
        bool acked = false;
        uint8_t result = 0;
        int high = 0;
        for (int i = 0; i < 80; ++i) {
            tick();
            if (dut.cart_ack) {
                ++high;
                result = dut.cart_dout;
                acked = true;
                dut.cart_req = 0;
            } else if (acked) {
                break;
            }
        }
        test.check(acked, "cartridge transaction must be acknowledged");
        test.check(high == 1, "cartridge acknowledge must be a one-cycle pulse");
        return result;
    }

  private:
    uint32_t active_row = 0;
    uint8_t active_bank = 0;
    uint16_t read_word = 0xffff;
    int read_drive_cycles = 0;
    uint32_t integration_active_row = 0;
    uint8_t integration_active_bank = 0;
    uint16_t integration_read_word = 0xffff;
    int integration_read_drive_cycles = 0;
};

void test_initialization_exclusion(TestState &test) {
    Harness h;
    h.initialize(test, true);
}

void test_cart_read_write_mapping(TestState &test) {
    Harness h;
    h.initialize(test);

    h.cart_transaction(test, true, 3, 0x080000, 0x12);
    h.cart_transaction(test, true, 3, 0x080001, 0x34);
    test.check(h.load(3, 0x080000) == 0x12, "even cartridge write must update low byte");
    test.check(h.load(3, 0x080001) == 0x34, "odd cartridge write must update high byte");
    const uint8_t even_read = h.cart_transaction(test, false, 3, 0x080000);
    test.check(even_read == 0x12,
               "even cartridge read must select DQ low byte (got " +
                   std::to_string(even_read) + ")");
    test.check(h.cart_transaction(test, false, 3, 0x080001) == 0x34,
               "odd cartridge read must select DQ high byte");

    h.cart_transaction(test, true, 3, 0x0fffff, 0xa5);
    test.check(h.load(3, 0x0fffff) == 0xa5,
               "cartridge region upper boundary must preserve the full address");
    test.check(h.cart_transaction(test, false, 3, 0x0fffff) == 0xa5,
               "cartridge region upper boundary must read back");

    bool saw_even_mask = false;
    bool saw_odd_mask = false;
    for (const auto &cmd : h.commands) {
        if (cmd.kind != CMD_WRITE || cmd.bank != 3) continue;
        if (cmd.address == 0x080000 && cmd.data == 0x1212)
            saw_even_mask = !cmd.dqml && cmd.dqmh;
        if (cmd.address == 0x080000 && cmd.data == 0x3434)
            saw_odd_mask = cmd.dqml && !cmd.dqmh;
    }
    test.check(saw_even_mask, "even write must unmask only DQ low byte");
    test.check(saw_odd_mask, "odd write must unmask only DQ high byte");
}

void test_legacy_write_paths(TestState &test) {
    Harness h;
    h.initialize(test);

    auto main_write = [&](uint8_t bank, uint32_t address, uint8_t data) {
        h.align_before_idle();
        const size_t marker = h.commands.size();
        h.dut.bank = bank;
        h.dut.addr = address;
        h.dut.din = data;
        h.dut.we = 1;
        h.tick();

        bool wrote = false;
        for (int i = 0; i < 12 && !wrote; ++i) {
            h.tick();
            for (size_t j = marker; j < h.commands.size(); ++j)
                if (h.commands[j].kind == CMD_WRITE) wrote = true;
        }
        h.dut.we = 0;
        test.check(wrote, "legacy main write must issue a physical WRITE command");
        for (int i = 0; i < 8; ++i) h.tick();
        return marker;
    };

    const size_t even_marker = main_write(1, 0x001220, 0x5a);
    const size_t odd_marker = main_write(1, 0x001221, 0xa6);
    bool even_ok = false;
    bool odd_ok = false;
    for (size_t i = even_marker; i < odd_marker; ++i) {
        const auto &cmd = h.commands[i];
        if (cmd.kind == CMD_WRITE)
            even_ok = cmd.bank == 1 && cmd.address == 0x001220 &&
                      cmd.data == 0x5a5a && !cmd.dqml && cmd.dqmh;
    }
    for (size_t i = odd_marker; i < h.commands.size(); ++i) {
        const auto &cmd = h.commands[i];
        if (cmd.kind == CMD_WRITE)
            odd_ok = cmd.bank == 1 && cmd.address == 0x001220 &&
                     cmd.data == 0xa6a6 && cmd.dqml && !cmd.dqmh;
    }
    test.check(even_ok,
               "legacy even main write must preserve bank/address/data and low-byte mask");
    test.check(odd_ok,
               "legacy odd main write must preserve bank/address/data and high-byte mask");
    test.check(h.load(1, 0x001220) == 0x5a && h.load(1, 0x001221) == 0xa6,
               "legacy main writes must update the externally modelled DQ bytes");

    h.align_before_idle();
    const size_t tape_marker = h.commands.size();
    h.dut.tape_addr = 0x003301;
    h.dut.tape_din = 0x6e;
    h.dut.tape_wr = 1;
    bool tape_acked = false;
    int tape_ack_cycles = 0;
    for (int i = 0; i < 24 && !tape_acked; ++i) {
        h.tick();
        if (h.dut.tape_wr_ack) {
            tape_acked = true;
            ++tape_ack_cycles;
            h.dut.tape_wr = 0;
        }
    }
    h.tick();
    if (h.dut.tape_wr_ack) ++tape_ack_cycles;
    bool tape_write_ok = false;
    for (size_t i = tape_marker; i < h.commands.size(); ++i) {
        const auto &cmd = h.commands[i];
        if (cmd.kind == CMD_WRITE)
            tape_write_ok = cmd.bank == 2 && cmd.address == 0x003300 &&
                            cmd.data == 0x6e6e && cmd.dqml && !cmd.dqmh;
    }
    test.check(tape_acked && tape_ack_cycles == 1,
               "legacy tape write must produce one completion pulse");
    test.check(tape_write_ok,
               "legacy tape write must preserve bank/address/data and odd-byte mask");
    test.check(h.load(2, 0x003301) == 0x6e,
               "legacy tape write must reach the externally modelled DQ byte");
}

void test_main_then_cart_arbitration(TestState &test) {
    Harness h;
    h.initialize(test);
    h.store(1, 0x001234, 0x5a);
    h.store(3, 0x080020, 0xc3);

    h.align_before_idle();
    const size_t marker = h.commands.size();
    h.dut.bank = 1;
    h.dut.addr = 0x001234;
    h.dut.oe = 1;
    h.dut.cart_req = 1;
    h.dut.cart_wr = 0;
    h.dut.cart_bank = 3;
    h.dut.cart_addr = 0x080020;
    h.tick();

    bool cart_acked = false;
    for (int i = 0; i < 40 && !cart_acked; ++i) {
        h.tick();
        cart_acked = h.dut.cart_ack;
    }
    test.check(cart_acked, "held cartridge request must complete after main port wins");
    test.check(h.dut.cart_dout == 0xc3, "held cartridge read must return its own data");
    test.check(h.dut.dout == 0x5a, "main-port read behavior must remain intact with cart port present");
    h.dut.oe = 0;
    h.dut.cart_req = 0;

    std::vector<Command> active;
    for (size_t i = marker; i < h.commands.size(); ++i)
        if (h.commands[i].kind == CMD_ACTIVE) active.push_back(h.commands[i]);
    test.check(active.size() >= 2, "main/cart arbitration must issue both ACTIVE commands");
    if (active.size() >= 2) {
        test.check(active[0].bank == 1 && active[0].address == 0x001200,
                   "main rising edge must outrank simultaneous cartridge request");
        test.check(active[1].bank == 3 && active[1].address == 0x080000,
                   "held cartridge request must be next after main completes");
    }
}

void test_cart_tape_video_priority(TestState &test) {
    Harness h;
    h.initialize(test);
    h.store(3, 0x080040, 0x44);
    h.store(2, 0x000080, 0x88);

    h.align_before_idle();
    const size_t marker = h.commands.size();
    h.dut.cart_req = 1;
    h.dut.cart_bank = 3;
    h.dut.cart_addr = 0x080040;
    h.dut.tape_rd = 1;
    h.dut.tape_addr = 0x000080;
    h.dut.vram_bank = 1;
    h.dut.vram_addr = 0x000100;

    bool cart_acked = false;
    for (int i = 0; i < 24 && !cart_acked; ++i) {
        h.tick();
        cart_acked = h.dut.cart_ack;
    }
    h.dut.cart_req = 0;
    for (int i = 0; i < 24; ++i) h.tick();
    h.dut.tape_rd = 0;
    for (int i = 0; i < 24; ++i) h.tick();

    std::vector<Command> active;
    for (size_t i = marker; i < h.commands.size(); ++i)
        if (h.commands[i].kind == CMD_ACTIVE) active.push_back(h.commands[i]);
    bool saw_video_after_tape = false;
    for (const auto &cmd : active)
        if (cmd.bank == 1) saw_video_after_tape = true;
    test.check(active.size() >= 2, "cart/tape/video arbitration must service cart and tape");
    if (active.size() >= 2) {
        test.check(active[0].bank == 3 && active[0].address == 0x080000,
                   "cartridge must outrank simultaneous tape and video requests");
        test.check(active[1].bank == 2 && active[1].address == 0x000000,
                   "tape must outrank a pending video address change");
    }
    test.check(saw_video_after_tape, "video request must run after tape is released");
    test.check(h.dut.tape_dout == 0x88,
               "legacy tape read data must remain intact with cart port present");
}

void test_held_back_to_back_and_refresh_guard(TestState &test) {
    Harness h;
    h.initialize(test);
    h.dut.cart_req = 1;
    h.dut.cart_wr = 0;
    h.dut.cart_bank = 3;
    h.dut.cart_addr = 0x080100;
    for (uint32_t i = 0; i < 34; ++i) h.store(3, 0x080100 + i, i);

    int acknowledgements = 0;
    int consecutive_ack_high = 0;
    std::vector<uint8_t> values;
    std::vector<uint32_t> requested;
    while (acknowledgements < 32 && h.cycles < 1200) {
        const size_t before_tick = h.commands.size();
        h.tick();
        for (size_t i = before_tick; i < h.commands.size(); ++i)
            test.check(h.commands[i].kind != CMD_REFRESH,
                       "continuous cartridge stream must not force refresh before grant 32");
        if (h.dut.cart_ack) {
            ++consecutive_ack_high;
            values.push_back(h.dut.cart_dout);
            requested.push_back(h.dut.cart_addr);
            ++acknowledgements;
            test.check(acknowledgements == 32 || !h.dut.debug_refresh_due,
                       "forced refresh must not become due before 32 cartridge grants");
            h.dut.cart_addr = 0x080100 + acknowledgements;
        } else {
            test.check(consecutive_ack_high <= 1,
                       "held-request acknowledge must never span two cycles");
            consecutive_ack_high = 0;
        }
    }
    test.check(acknowledgements == 32, "32 held back-to-back cartridge requests must complete");
    for (int i = 0; i < acknowledgements; ++i) {
        test.check(values[i] == static_cast<uint8_t>(i),
                   "held request must not duplicate the address acknowledged previously");
        test.check(requested[i] == 0x080100U + static_cast<uint32_t>(i),
                   "request fields must advance once per acknowledge");
    }
    test.check(h.dut.debug_refresh_due,
               "the 32nd continuous cartridge grant must make refresh overdue");

    // Enter the slot boundary, then raise main edges on two consecutive
    // arbitrations. Both may defer the overdue refresh, but neither clears it.
    h.align_before_idle();
    const size_t marker = h.commands.size();
    h.dut.bank = 1;
    h.dut.addr = 0x002000;
    h.dut.oe = 1;
    h.tick();
    h.dut.oe = 0;

    h.align_before_idle();
    h.dut.addr = 0x002200;
    h.dut.oe = 1;
    h.tick();
    h.dut.oe = 0;

    int extra_ack = 0;
    bool cart_resume_command_seen = false;
    bool ack_before_cart_resume = false;
    for (int i = 0; i < 28; ++i) {
        const size_t before_tick = h.commands.size();
        h.tick();
        for (size_t j = before_tick; j < h.commands.size(); ++j) {
            if (h.commands[j].kind == CMD_ACTIVE && h.commands[j].bank == 3)
                cart_resume_command_seen = true;
        }
        if (h.dut.cart_ack) {
            if (!cart_resume_command_seen) ack_before_cart_resume = true;
            ++extra_ack;
            h.dut.cart_addr = h.dut.cart_addr + 1;
        }
    }
    h.dut.cart_req = 0;

    std::vector<uint8_t> relevant;
    for (size_t i = marker; i < h.commands.size(); ++i) {
        if (h.commands[i].kind == CMD_ACTIVE || h.commands[i].kind == CMD_REFRESH)
            relevant.push_back(h.commands[i].kind);
    }
    test.check(relevant.size() >= 4,
               "repeated-main/deferred-refresh/cart sequence must be observable");
    if (relevant.size() >= 4) {
        test.check(relevant[0] == CMD_ACTIVE,
                   "main edge must still win when cartridge refresh is overdue");
        test.check(relevant[1] == CMD_ACTIVE,
                   "a repeated main edge may defer, but must not clear, overdue refresh");
        test.check(relevant[2] == CMD_REFRESH,
                   "overdue refresh must remain pending after repeated main grants");
        test.check(relevant[3] == CMD_ACTIVE,
                   "held cartridge request must resume after the forced refresh slot");
    }
    test.check(extra_ack >= 1, "held cartridge request must resume and acknowledge after refresh");
    test.check(!ack_before_cart_resume,
               "main and forced-refresh slots must not acknowledge the held cartridge request");
}

void test_ordinary_refresh_resets_cart_cadence(TestState &test) {
    Harness h;
    h.initialize(test);
    h.dut.cart_req = 1;
    h.dut.cart_bank = 3;
    h.dut.cart_addr = 0x080300;
    for (uint32_t i = 0; i < 40; ++i) h.store(3, 0x080300 + i, i + 1);

    int acknowledgements = 0;
    while (acknowledgements < 5 && h.cycles < 800) {
        h.tick();
        if (h.dut.cart_ack) {
            ++acknowledgements;
            h.dut.cart_addr = 0x080300 + acknowledgements;
        }
    }
    test.check(acknowledgements == 5, "cadence prelude cartridge reads must complete");
    h.dut.cart_req = 0;

    bool saw_idle_refresh = false;
    for (int i = 0; i < 24 && !saw_idle_refresh; ++i) {
        h.tick();
        saw_idle_refresh = h.command() == CMD_REFRESH;
    }
    test.check(saw_idle_refresh, "an idle slot must issue an ordinary refresh");
    test.check(h.dut.debug_cart_grants == 0 && !h.dut.debug_refresh_due,
               "ordinary refresh must reset cartridge refresh cadence");

    h.dut.cart_req = 1;
    int after_refresh = 0;
    while (after_refresh < 27 && h.cycles < 1400) {
        h.tick();
        if (h.dut.cart_ack) {
            ++after_refresh;
            test.check(!h.dut.debug_refresh_due,
                       "27 grants after ordinary refresh must remain below forced threshold");
            h.dut.cart_addr = h.dut.cart_addr + 1;
        }
    }
    h.dut.cart_req = 0;
    test.check(after_refresh == 27,
               "post-refresh cadence must accept 27 held cartridge requests");
}

void test_service_to_real_sdram_integration(TestState &test) {
    Harness h;
    h.initialize(test);
    const size_t marker = h.integration_commands.size();

    h.dut.service_load_begin = 1;
    h.tick();
    h.dut.service_load_begin = 0;
    test.check(h.dut.service_busy && !h.dut.service_image_valid,
               "integrated load_begin must start an unpublished load");

    auto integration_writes_since = [&](size_t start) {
        std::vector<Command> writes;
        for (size_t i = start; i < h.integration_commands.size(); ++i)
            if (h.integration_commands[i].kind == CMD_WRITE)
                writes.push_back(h.integration_commands[i]);
        return writes;
    };

    bool clear_settled = false;
    for (int i = 0; i < 160 && !clear_settled; ++i) {
        h.tick();
        const auto writes = integration_writes_since(marker);
        clear_settled = writes.size() == 4 && !h.dut.service_mem_req;
    }
    const auto clear_writes = integration_writes_since(marker);
    test.check(clear_settled && clear_writes.size() == 4,
               "real SDRAM acknowledgements must produce exactly four configured clear writes");
    for (size_t i = 0; i < clear_writes.size(); ++i) {
        const auto &cmd = clear_writes[i];
        test.check(cmd.bank == 3 && cmd.address == 0x080000 + (i & ~size_t{1}) &&
                       cmd.data == 0 &&
                       ((i & 1U) ? (cmd.dqml && !cmd.dqmh) : (!cmd.dqml && cmd.dqmh)),
                   "integrated clear must advance once per ack with exact byte masks");
        test.check(h.integration_load(3, 0x080000 + i) == 0,
                   "integrated clear byte must reach external SDRAM model exactly once");
    }

    const size_t before_load = h.integration_commands.size();
    h.dut.service_load_page = 31;
    h.dut.service_load_offset = 0x3fff;
    h.dut.service_load_data = 0xa5;
    h.dut.service_load_valid = 1;
    bool load_ready = false;
    int load_ready_cycles = 0;
    for (int i = 0; i < 80 && !load_ready; ++i) {
        h.tick();
        if (h.dut.service_load_ready) {
            load_ready = true;
            ++load_ready_cycles;
        }
    }
    for (int i = 0; i < 24; ++i) {
        h.tick();
        if (h.dut.service_load_ready) ++load_ready_cycles;
    }
    const auto load_writes = integration_writes_since(before_load);
    test.check(load_ready && load_ready_cycles == 1,
               "real SDRAM acknowledge must complete held load_valid exactly once");
    test.check(load_writes.size() == 1 && load_writes[0].bank == 3 &&
                   load_writes[0].address == 0x0ffffe && load_writes[0].data == 0xa5a5 &&
                   load_writes[0].dqml && !load_writes[0].dqmh,
               "integrated loader write must reach exact page-31 boundary byte once");
    test.check(h.integration_load(3, 0x0fffff) == 0xa5,
               "integrated loader byte must be stored at canonical boundary address");
    h.dut.service_load_valid = 0;
    h.tick();

    h.dut.service_load_commit = 1;
    h.tick();
    h.dut.service_load_commit = 0;
    for (int i = 0; i < 8 && !h.dut.service_image_valid; ++i) h.tick();
    test.check(h.dut.service_image_valid && !h.dut.service_busy,
               "integrated commit must publish only after clear/load acknowledgements");

    const size_t before_read = h.integration_commands.size();
    h.dut.service_cpu_page = 31;
    h.dut.service_cpu_offset = 0x3fff;
    h.dut.service_cpu_valid = 1;
    bool cpu_ready = false;
    int cpu_ready_cycles = 0;
    for (int i = 0; i < 80 && !cpu_ready; ++i) {
        h.tick();
        if (h.dut.service_cpu_ready) {
            cpu_ready = true;
            ++cpu_ready_cycles;
        }
    }
    for (int i = 0; i < 24; ++i) {
        h.tick();
        if (h.dut.service_cpu_ready) ++cpu_ready_cycles;
    }
    int physical_reads = 0;
    for (size_t i = before_read; i < h.integration_commands.size(); ++i) {
        const auto &cmd = h.integration_commands[i];
        if (cmd.kind == CMD_READ) {
            ++physical_reads;
            test.check(cmd.bank == 3 && cmd.address == 0x0ffffe,
                       "integrated CPU read must use canonical page-31 boundary address");
        }
    }
    test.check(cpu_ready && cpu_ready_cycles == 1 && h.dut.service_cpu_data == 0xa5,
               "integrated CPU readback must complete once with the loaded byte");
    test.check(physical_reads == 1,
               "held integrated cpu_valid must not duplicate the physical SDRAM read");
    h.dut.service_cpu_valid = 0;
    h.tick();
}

void test_top_level_wiring(TestState &test) {
    // The P-1 tie-off pin became obsolete when P0 production-connected the
    // cartridge service (docs/plus/architecture.md, "Cartridge SDRAM
    // contract"). This check pins the new contract: the reserved SDRAM port
    // is driven by the service's memory interface, and the CPR loader side
    // is still explicitly tied off until the parser joins.
    std::ifstream input("../../Amstrad.sv");
    if (!input.good()) input.open("Amstrad.sv");
    std::ostringstream text;
    text << input.rdbuf();
    const std::string source = text.str();
    test.check(input.good() || input.eof(), "top-level source must be readable for wiring check");

    test.check(source.find(".cart_req(cart_mem_req)") != std::string::npos &&
                   source.find(".cart_wr(cart_mem_write)") != std::string::npos &&
                   source.find(".cart_bank(cart_mem_bank)") != std::string::npos &&
                   source.find(".cart_addr(cart_mem_addr)") != std::string::npos &&
                   source.find(".cart_din(cart_mem_wdata)") != std::string::npos,
               "Amstrad top must drive the cartridge SDRAM port from the memory service");
    test.check(source.find(".cart_dout(cart_mem_rdata)") != std::string::npos &&
                   source.find(".cart_ack(cart_mem_ack)") != std::string::npos,
               "Amstrad top must consume the cartridge SDRAM responses");
    test.check(source.find("plus_cartridge_memory cartridge_memory") != std::string::npos,
               "Amstrad top must instantiate the cartridge memory service");
    test.check(source.find(".cpu_valid(plus_cart_valid)") != std::string::npos,
               "the service CPU port must face the Plus MMU read bridge");
    test.check(source.find("plus_mmu plus_mmu") != std::string::npos,
               "Amstrad top must instantiate the Plus MMU");
    test.check(source.find(".plus_mem_wait(plus_cart_stall)") != std::string::npos,
               "cartridge stalls must reach the CPU WAIT input on the motherboard");
    test.check(source.find("plus_cpr_parser cpr_parser") != std::string::npos,
               "Amstrad top must instantiate the CPR parser");
    const auto legacy_gate_start =
        source.find("plus_legacy_cart_gate legacy_cart_gate");
    const auto legacy_gate_end = source.find(");", legacy_gate_start);
    const std::string legacy_gate =
        legacy_gate_start != std::string::npos &&
                legacy_gate_end != std::string::npos
            ? source.substr(legacy_gate_start,
                            legacy_gate_end - legacy_gate_start)
            : std::string();
    test.check(!legacy_gate.empty() &&
                   legacy_gate.find(".clk(clk_sys)") != std::string::npos &&
                   legacy_gate.find(".plus_mode(plus_mode)") !=
                       std::string::npos &&
                   legacy_gate.find(".dandanator_download(dan_download)") !=
                       std::string::npos &&
                   legacy_gate.find(".dandanator_detach(status[32])") !=
                       std::string::npos &&
                   legacy_gate.find(".dandanator_nce(dan_eeprom_nce)") !=
                       std::string::npos &&
                   legacy_gate.find(".dandanator_loaded(dan_eeprom_loaded)") !=
                       std::string::npos &&
                   legacy_gate.find(".dandanator_active(dan_ena)") !=
                       std::string::npos &&
                   source.find("dan_ena ? {4'd0, dan_bank, cpu_addr[13:0]} : ram_a") !=
                       std::string::npos &&
                   source.find("dan_ena ? 2'b11 : mem_bank") !=
                       std::string::npos,
               "the production Dandanator lifecycle and both SDRAM overrides must pass through the Plus isolation gate");
    test.check(source.find("cpr_download = ioctl_download && (ioctl_index == 8)") !=
                   std::string::npos,
               "the CPR stream must own its own ioctl index (8)");
    test.check(source.find("\"F8,CPR,Load Plus cartridge;\"") != std::string::npos,
               "the OSD must offer a CPR cartridge entry");
    test.check(source.find("| cpr_ioctl_wait") != std::string::npos,
               "parser backpressure must join the ioctl download throttle");
    test.check(source.find("wire cart_load_begin = 1'b0") == std::string::npos,
               "the interim loader tie-off must be gone once the parser joins");
}

} // namespace

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    TestState test;

    test_initialization_exclusion(test);
    test_cart_read_write_mapping(test);
    test_legacy_write_paths(test);
    test_main_then_cart_arbitration(test);
    test_cart_tape_video_priority(test);
    test_held_back_to_back_and_refresh_guard(test);
    test_ordinary_refresh_resets_cart_cadence(test);
    test_service_to_real_sdram_integration(test);
    test_top_level_wiring(test);

    if (test.failures != 0) {
        std::cerr << test.failures << " SDRAM cartridge assertion(s) failed\n";
        return 1;
    }
    std::cout << "PASS: SDRAM legacy writes, held port, service integration, arbitration, refresh, and top wiring\n";
    return 0;
}
