#include <verilated.h>

#include "Vplus_mmu.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw TestFailure(message);
    }
}

// plus_mmu drives a held request/acknowledge CPU port; this bench plays the
// memory service side by hand so every handshake transition is observable.
class TestBench {
public:
    TestBench() {
        dut_.clk = 0;
        dut_.reset = 0;
        dut_.plus_mode = 1;
        dut_.gx4000 = 0;
        dut_.io_rd = 0;
        dut_.io_wr = 0;
        dut_.mem_rd = 0;
        dut_.A = 0;
        dut_.D = 0;
        dut_.rom_en = 1;
        dut_.exp_n = 1;
        dut_.cart_ready = 0;
        dut_.cart_data = 0;
        dut_.cart_busy = 0;
        dut_.eval();
    }

    ~TestBench() { dut_.final(); }

    Vplus_mmu& dut() { return dut_; }

    void tick() {
        dut_.clk = 1;
        dut_.eval();
        dut_.clk = 0;
        dut_.eval();
    }

    void hard_reset() {
        dut_.reset = 1;
        tick();
        dut_.reset = 0;
        tick();
        require(dut_.cart_valid == 0, "reset left cart_valid active");
        require(dut_.cart_own == 0, "reset left cart_own active");
        require(dut_.cart_stall == 0, "reset left cart_stall active");
        require(dut_.asic_page_on == 0, "reset left asic_page_on active");
    }

    void io_write(std::uint16_t addr, std::uint8_t data) {
        dut_.mem_rd = 0;
        dut_.io_rd = 0;
        dut_.io_wr = 1;
        dut_.A = addr;
        dut_.D = data;
        for (unsigned hold = 0; hold < 4; ++hold) tick();
        dut_.io_wr = 0;
        dut_.A = 0;
        dut_.D = 0;
        tick();
    }

    // Plus I/O reads on CRTC/Gate-Array write ports perform the matching
    // write with the live bus byte ([KT] Ports; asic-reference sections
    // 4/13). The DUT sees that byte on D exactly as production wiring does.
    void io_read(std::uint16_t addr, std::uint8_t data) {
        dut_.mem_rd = 0;
        dut_.io_wr = 0;
        dut_.io_rd = 1;
        dut_.A = addr;
        dut_.D = data;
        for (unsigned hold = 0; hold < 4; ++hold) tick();
        dut_.io_rd = 0;
        dut_.A = 0;
        dut_.D = 0;
        tick();
    }

    // Assert mem_rd at addr and step one edge: the claim registers here.
    void begin_read(std::uint16_t addr) {
        dut_.mem_rd = 1;
        dut_.A = addr;
        tick();
    }

    void end_read() {
        dut_.mem_rd = 0;
        dut_.A = 0;
        tick();
    }

    // Deliver the service response: ready pulse first, then the data edge
    // (the production service registers data and completion together, so
    // the MMU samples data one edge after the pulse).
    void respond(std::uint8_t data) {
        dut_.cart_ready = 1;
        tick();
        require(dut_.cart_valid == 0, "cart_valid did not drop on cart_ready");
        dut_.cart_ready = 0;
        dut_.cart_data = data;
        tick();
        dut_.cart_data = 0;
        require(dut_.cart_stall == 0, "stall did not release after response");
        require(dut_.cart_dout == data, "captured cartridge data mismatch");
        require(dut_.cart_own == 1, "cart_own dropped before the bus cycle ended");
    }

    unsigned valid_cycles = 0;  // edges with a request in flight
    unsigned valid_drops = 0;   // request deasserted before an acknowledge

    // Run `cycles` idle edges while the read stays asserted.
    void stall_cycles(unsigned cycles) {
        for (unsigned i = 0; i < cycles; ++i) {
            const bool was_valid = dut_.cart_valid != 0;
            tick();
            if (dut_.cart_valid) ++valid_cycles;
            if (was_valid && !dut_.cart_valid) ++valid_drops;
        }
    }

private:
    Vplus_mmu dut_;
};

void expect_claim(TestBench& tb, std::uint16_t addr, unsigned page,
                  const std::string& context) {
    tb.valid_cycles = 0;
    tb.valid_drops = 0;
    tb.begin_read(addr);
    require(tb.dut().cart_valid == 1, "no cart request for " + context);
    require(tb.dut().cart_page == page,
            "page mismatch for " + context + ": got " + std::to_string(tb.dut().cart_page) +
                " want " + std::to_string(page));
    require(tb.dut().cart_offset == (addr & 0x3fff), "offset mismatch for " + context);
    require(tb.dut().cart_stall == 1, "stall not asserted for " + context);
    require(tb.dut().cart_own == 1, "ownership not taken for " + context);
}

// §11/§12 reset state: RMR2=0 puts cartridge page 0 at &0000; ROM-select 0
// resolves through /EXP (high => AMSDOS page 3) on non-GX4000 models.
void test_reset_defaults_and_exp_sampling() {
    TestBench tb;
    tb.hard_reset();

    expect_claim(tb, 0x0000, 0, "low window at reset");
    tb.respond(0x11);
    tb.end_read();

    expect_claim(tb, 0x3fff, 0, "low window top byte");
    tb.respond(0x22);
    tb.end_read();

    // exp_n high: bare machine, value 0 -> page 3
    expect_claim(tb, 0xc000, 3, "high window with /EXP high");
    tb.respond(0x33);
    tb.end_read();

    // /EXP is sampled live: pulling it low flips value 0 to BASIC page 1
    tb.dut().exp_n = 0;
    expect_claim(tb, 0xc123, 1, "high window with /EXP low");
    tb.respond(0x44);
    tb.end_read();

    tb.dut().exp_n = 1;
    expect_claim(tb, 0xffff, 3, "high window back at /EXP high");
    tb.respond(0x55);
    tb.end_read();
}

// §11 ROM-select port rules for values below and above 128, including the
// classic A13=0 alias decode.
void test_rom_select_rules() {
    TestBench tb;
    tb.hard_reset();

    struct Case { std::uint16_t port; std::uint8_t value; unsigned page; };
    const Case cases[] = {
        {0xdf00, 0x80, 0},   // 128 -> physical page 0
        {0xdfff, 0x9f, 31},  // 159 -> physical page 31
        {0xdf00, 0xff, 31},  // 255 -> physical page 31
        {0xdf00, 0x07, 3},   // disc ROM code -> AMSDOS page 3
        {0xdf00, 0x2a, 1},   // other 1..126 -> BASIC page 1
        {0xc123, 0x05, 1},   // &C0xx alias with A13=0 also selects
    };
    for (const auto& c : cases) {
        tb.io_write(c.port, c.value);
        expect_claim(tb, 0xc000, c.page, "ROM-select write");
        tb.respond(0x00);
        tb.end_read();
    }

    // Writes with A13=1 are not ROM selects.
    tb.io_write(0xf000, 0x85);
    expect_claim(tb, 0xc000, 1, "port with A13=1 must not change selection");
    tb.respond(0x00);
    tb.end_read();
}

// §12 GX4000 has no disc-select hardware: every value below 128 lands on
// page 1 regardless of /EXP; values >= 128 still select physical pages.
void test_gx4000_overrides() {
    TestBench tb;
    tb.hard_reset();
    tb.dut().gx4000 = 1;

    tb.io_write(0xdf00, 0x07);
    tb.dut().exp_n = 0;
    expect_claim(tb, 0xc000, 1, "GX4000 disc code must give page 1");
    tb.respond(0x00);
    tb.end_read();

    tb.io_write(0xdf00, 0x00);
    expect_claim(tb, 0xc000, 1, "GX4000 value 0 must give page 1");
    tb.respond(0x00);
    tb.end_read();

    tb.dut().exp_n = 1;
    tb.io_write(0xdf00, 0x42);
    expect_claim(tb, 0xc000, 1, "GX4000 plain value must give page 1");
    tb.respond(0x00);
    tb.end_read();

    tb.io_write(0xdf00, 0x85);
    expect_claim(tb, 0xc000, 5, "GX4000 >=128 must still select physical pages");
    tb.respond(0x00);
    tb.end_read();
}

// MRER ROM-enable bits gate both windows, matching the Gate Array / ASIC:
// disabled ROM shows RAM through, so no cartridge request may start.
void test_rom_enable_gating() {
    TestBench tb;
    tb.hard_reset();

    // Disable lower ROM via MRER: D[7:6]=10, D[2]=1 (8'h84)
    tb.io_write(0x7f00, 0x84);
    tb.begin_read(0x0000);
    tb.stall_cycles(4);
    require(tb.valid_cycles == 0, "low window claimed while lower ROM disabled via MRER");
    require(tb.dut().cart_stall == 0, "stalled while lower ROM disabled");
    tb.end_read();

    // Re-enable lower ROM, disable upper ROM via MRER: D[3]=1 (8'h88)
    tb.io_write(0x7f00, 0x88);
    tb.begin_read(0xc000);
    tb.stall_cycles(4);
    require(tb.valid_cycles == 0, "high window claimed while upper ROM disabled via MRER");
    tb.end_read();

    // Re-enable both ROMs (8'h80)
    tb.io_write(0x7f00, 0x80);
    expect_claim(tb, 0x0000, 0, "low window active when lower ROM enabled");
    tb.respond(0x00);
    tb.end_read();

    // plus_mode off must gate everything, whatever the other inputs say.
    tb.dut().plus_mode = 0;
    tb.begin_read(0x0000);
    tb.stall_cycles(4);
    require(tb.valid_cycles == 0, "low window claimed outside Plus mode");
    tb.end_read();
    tb.dut().plus_mode = 1;
}

// docs/plus/references/asic-reference.md §1-§2: while locked, 101xxxxx is
// the legacy MRER alias (bit 5 ignored); after unlock it is RMR2, whose
// positions relocate the low window and whose D4D3=11 sets ASIC-page-enable.
void test_rmr2_locking_positions_pages() {
    TestBench tb;
    tb.hard_reset();

    // Locked: bit 5 is ignored by the legacy MRER decoder, so 10100100
    // disables the lower ROM rather than selecting RMR2 page 4.
    tb.io_write(0x7f00, 0xa4);
    tb.begin_read(0x0000);
    tb.stall_cycles(4);
    require(tb.valid_cycles == 0,
            "locked 10100100 did not act as MRER lower-ROM disable");
    tb.end_read();

    // Its locked 10100000 alias re-enables both ROM windows.  RMR2 position
    // and page remain at their reset values until the unlock sequence.
    tb.io_write(0x7f00, 0xa0);
    expect_claim(tb, 0x0000, 0, "locked 10100000 MRER re-enable");
    tb.respond(0x00);
    tb.end_read();

    // Unlock sequence on the CRTC register-select port: FF 00 sync then the
    // 13 fixed bytes then STATE=CD (asic-reference.md §1).
    const std::uint8_t seq[] = {0xff, 0x00, 0xff, 0x77, 0xb3, 0x51, 0xa8, 0xd4,
                                0x62, 0x39, 0x9c, 0x46, 0x2b, 0x15, 0x8a, 0xcd};
    for (std::uint8_t b : seq) tb.io_write(0xbc00, b);

    // RMR2 &B8: position 11 => low ROM stays at &0000, ASIC-page flag on.
    tb.io_write(0x7f00, 0xb8);
    require(tb.dut().asic_page_on == 1, "RMR2 B8 did not set asic_page_on");
    expect_claim(tb, 0x0000, 0, "post-unlock RMR2 B8 low window");

    tb.respond(0x00);
    tb.end_read();

    // RMR2 &A4: position 00, page 4, ASIC page off.
    tb.io_write(0x7f00, 0xa4);
    require(tb.dut().asic_page_on == 0, "RMR2 A4 did not clear asic_page_on");
    expect_claim(tb, 0x0000, 4, "RMR2 A4 low window page");
    tb.respond(0x00);
    tb.end_read();

    // RMR2 &AC: D4D3=01 relocates the low window to &4000-&7FFF.
    tb.io_write(0x7f00, 0xac);
    require(tb.dut().asic_page_on == 0, "RMR2 AC must not set asic_page_on");
    expect_claim(tb, 0x4000, 4, "relocated low window at &4000");
    tb.respond(0x00);
    tb.end_read();

    // RMR2 &B4: D4D3=10 relocates the low window to &8000-&BFFF.
    tb.io_write(0x7f00, 0xb4);
    expect_claim(tb, 0x8000, 4, "relocated low window at &8000");
    tb.respond(0x00);
    tb.end_read();

    // &C000 remains the high window while relocated; ROM-select is still at
    // its reset value 0, so /EXP high resolves it to AMSDOS page 3.
    expect_claim(tb, 0xc000, 3, "high window survives low-window relocation");
    tb.respond(0x00);
    tb.end_read();
}

// The unlock stream and RMR2 are both write-side effects of Plus IN cycles.
// One read byte inside the otherwise ordinary unlock stream proves the
// detector consumes io_rd; the final RMR2 read proves its GA payload capture.
// The second half pins the plus_mode gate around the same trap path.
void test_io_read_traps_unlock_and_rmr2() {
    TestBench tb;
    tb.hard_reset();

    const std::uint8_t seq[] = {0xff, 0x00, 0xff, 0x77, 0xb3, 0x51, 0xa8, 0xd4,
                                0x62, 0x39, 0x9c, 0x46, 0x2b, 0x15, 0x8a, 0xcd};
    for (unsigned i = 0; i < sizeof(seq); ++i) {
        if (i == 8) tb.io_read(0xbc00, seq[i]);
        else        tb.io_write(0xbc00, seq[i]);
    }
    tb.io_read(0x7f00, 0xb8);
    require(tb.dut().asic_page_on == 1,
            "IN trap did not carry unlock byte and RMR2 payload");

    tb.hard_reset();
    tb.dut().plus_mode = 0;
    for (std::uint8_t byte : seq) tb.io_read(0xbc00, byte);
    tb.dut().plus_mode = 1;
    tb.io_read(0x7f00, 0xb8);
    require(tb.dut().asic_page_on == 0,
            "classic-mode IN sequence armed the Plus unlock detector");
}

// Handshake discipline: exactly one request per bus cycle, fields stable
// while held, ownership held until the cycle ends, then re-armed.
void test_handshake_single_request_per_cycle() {
    TestBench tb;
    tb.hard_reset();

    tb.valid_cycles = 0;
    tb.valid_drops = 0;
    tb.begin_read(0x0123);
    require(tb.dut().cart_valid == 1 && tb.dut().cart_offset == 0x0123,
            "claim did not latch offset");

    // Wiggle the address within the same window mid-request: the held
    // contract requires stable request fields until the acknowledged edge.
    // (Leaving the window entirely would legitimately cancel the request.)
    tb.dut().A = 0x2765;
    tb.stall_cycles(6);
    require(tb.valid_drops == 0, "request dropped/re-issued while waiting for acknowledge");
    require(tb.dut().cart_offset == 0x0123, "latched offset changed while held");

    tb.respond(0x5a);
    require(tb.dut().cart_dout == 0x5a, "first response data mismatch");

    // Ownership persists until the Z80 ends the cycle...
    tb.stall_cycles(3);
    require(tb.dut().cart_own == 1, "ownership lost while mem_rd held");
    require(tb.valid_drops == 0, "request cycled inside the same bus cycle");

    tb.end_read();
    require(tb.dut().cart_own == 0, "ownership kept after the bus cycle ended");

    // ...and the next cycle issues a fresh request.
    tb.dut().A = 0;
    tb.valid_cycles = 0;
    tb.valid_drops = 0;
    tb.begin_read(0x0124);
    require(tb.dut().cart_valid == 1, "second cycle did not issue a new request");
    tb.respond(0x77);
    tb.end_read();
    require(tb.dut().cart_dout == 0x77, "second response data mismatch");
}

// A wedged backend must release the CPU with open-bus FF instead of hanging
// the machine (watchdog STALL_TIMEOUT).
void test_watchdog_releases_stuck_response() {
    TestBench tb;
    tb.hard_reset();

    tb.begin_read(0x2000);
    require(tb.dut().cart_stall == 1, "watchdog precondition: stall expected");

    bool released = false;
    for (unsigned i = 0; i < 1200; ++i) {
        tb.tick();
        if (!tb.dut().cart_stall) {
            released = true;
            break;
        }
    }
    require(released, "watchdog never released the stalled read");
    require(!tb.dut().cart_valid, "watchdog left the request asserted");
    require(tb.dut().cart_dout == 0xff, "watchdog must present open-bus FF");
    require(tb.dut().cart_own == 1, "watchdog release should hold ownership until the cycle ends");

    tb.end_read();
    require(tb.dut().cart_own == 0, "ownership kept past watchdog-released cycle");

    // The bridge must be fully usable afterwards.
    expect_claim(tb, 0x0000, 0, "post-watchdog claim");
    tb.respond(0x99);
    tb.end_read();
}

void test_watchdog_waits_out_legitimate_load() {
    TestBench tb;
    tb.hard_reset();
    tb.dut().cart_busy = 1;
    tb.begin_read(0x2000);

    tb.stall_cycles(1400);
    require(tb.dut().cart_valid == 1,
            "watchdog cancelled a request during cartridge load");
    require(tb.dut().cart_stall == 1,
            "watchdog released the CPU during cartridge load");

    tb.dut().cart_busy = 0;
    tb.respond(0x6d);
    require(tb.dut().cart_dout == 0x6d,
            "post-load response did not complete the held read");
    tb.end_read();
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    try {
        test_reset_defaults_and_exp_sampling();
        std::cout << "PASS: reset defaults and live /EXP sampling\n";
        test_rom_select_rules();
        std::cout << "PASS: ROM-select high-window rules and alias decode\n";
        test_gx4000_overrides();
        std::cout << "PASS: GX4000 fixed page-1 semantics\n";
        test_rom_enable_gating();
        std::cout << "PASS: ROM-enable and plus_mode gating\n";
        test_rmr2_locking_positions_pages();
        std::cout << "PASS: RMR2 locking, relocation, pages, ASIC-page flag\n";
        test_io_read_traps_unlock_and_rmr2();
        std::cout << "PASS: IN traps reach unlock and RMR2 only in Plus mode\n";
        test_handshake_single_request_per_cycle();
        std::cout << "PASS: single held request per cycle and ownership window\n";
        test_watchdog_releases_stuck_response();
        std::cout << "PASS: watchdog releases stuck responses fail-open to FF\n";
        test_watchdog_waits_out_legitimate_load();
        std::cout << "PASS: watchdog waits out legitimate cartridge loads\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }

    std::cout << "PASS: all Plus MMU tests\n";
    return 0;
}
