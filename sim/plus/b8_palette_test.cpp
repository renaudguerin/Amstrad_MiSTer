// B8-3 palette event regression (fail-first, source-derived).
//
// Production boundary executed: real Amstrad_motherboard in Plus mode via
// b8_palette_bench_top, with the real asic_ga_timing (legacy PENR/INKR
// decoder), the real asic_regs palette owner, and the production
// plus_asic_rd/dout read mux (the exact Amstrad.sv selection for the enabled
// page). All stimulus is real bus cycles from the bench CPU; no C++ pokes
// colour-shadow arrays as write events.
//
// Rule basis (primary sources, never simulator-derived):
//   - Arnold V Issue 1.5 §2.2: the 32x12-bit palette has TWO ports. The
//     primary port is &6400-&643F (pairs: even byte RED high/BLUE low, odd
//     byte GREEN low). The secondary port is the existing 5-bit interface:
//     a block of logic maps the 5-bit colour written to the palette entry at
//     the palette-pointer address. First 17 registers only (pens 0-15 +
//     border, pointer 00-0F pens / 10-1F border).
//   - [KT] Extra CPC Plus Hardware Information, Palette table (measured):
//     HW20 Black R0 G0 B0 -> word 0x000; HW21 Bright Blue R0 G0 B15 -> 0x00F;
//     HW0 White R6 G6 B6 -> 0x666. Sprite colours are NOT reachable via the
//     legacy port (reference §6).
//
// Sequence (all plus_mode=1, page forced on like the p1 bench):
//   pen:  select pen0 (00) -> legacy black 54 (HW20) -> page white FFF at
//         &6400/&6401 -> repeat SAME legacy black 54 -> page read &6400.
//         Expect black 000 (event, not value). Buggy shadow-compare keeps FFF.
//   border: select alias 11 (border, nonzero low nibble) -> legacy black 54
//         -> page white at &6420/&6421 -> repeat same black -> read &6420.
//         Expect black 000 at the BORDER entry; pen1 must be undisturbed
//         (alias reaches border, not pen1).
//   positive: changed legacy blue 55 (HW21) to pen0 -> expect 00F (works even
//         with the bug, proving the path is alive when the shadow differs).
//   guard: pen1 still grey 666 (HW0 reset import), sprite colour 1 still 000.

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "Vb8_palette_bench_top.h"
#include "Vb8_palette_bench_top___024root.h"
#include "verilated.h"

namespace {

class TestFailure : public std::runtime_error {
public:
    explicit TestFailure(const std::string& m) : std::runtime_error(m) {}
};

[[noreturn]] void fail(const std::string& what) { throw TestFailure(what); }

constexpr uint64_t kReadyTimeout = 10000;
constexpr uint64_t kBusTimeout = 5000;

class Bench {
public:
    Vb8_palette_bench_top dut;
    uint64_t cyc = 0;

    Bench() : dut("b8_palette_bench_top") {
        dut.clk = 0;
        dut.reset = 1;
        dut.asic_reset_i = 0;
        dut.plus_mode_i = 1;
        dut.asic_page_on = 0;
    }

    void tick() {
        dut.clk = 0;
        dut.eval();
        dut.clk = 1;
        dut.eval();
        ++cyc;
    }
    void run(uint64_t n) { for (uint64_t i = 0; i < n; ++i) tick(); }

    auto* cpu_done() {
        return &dut.rootp->b8_palette_bench_top__DOT__mb__DOT__CPU__DOT__dbg_done;
    }
    auto* bench_addr() {
        return &dut.rootp->b8_palette_bench_top__DOT__mb__DOT__CPU__DOT__bench_addr;
    }
    auto* bench_data() {
        return &dut.rootp->b8_palette_bench_top__DOT__mb__DOT__CPU__DOT__bench_data;
    }
    auto* bench_op() {
        return &dut.rootp->b8_palette_bench_top__DOT__mb__DOT__CPU__DOT__bench_op;
    }
    auto* bench_req() {
        return &dut.rootp->b8_palette_bench_top__DOT__mb__DOT__CPU__DOT__bench_req;
    }
    auto* bench_ack() {
        return &dut.rootp->b8_palette_bench_top__DOT__mb__DOT__CPU__DOT__bench_ack;
    }
    auto* bench_rdata() {
        return &dut.rootp->b8_palette_bench_top__DOT__mb__DOT__CPU__DOT__bench_rdata;
    }
};

// One real bus cycle through the production motherboard decode.
void bus_cycle(Bench& b, uint16_t addr, uint8_t data, uint8_t op, const char* ctx) {
    *b.bench_addr() = addr;
    *b.bench_data() = data;
    *b.bench_op() = op;
    *b.bench_req() = 1;
    uint64_t n = 0;
    while (!*b.bench_ack()) {
        b.tick();
        if (++n > kBusTimeout) fail(std::string(ctx) + ": bus cycle no ack");
    }
    *b.bench_req() = 0;
    n = 0;
    while (*b.bench_ack()) {
        b.tick();
        if (++n > kBusTimeout) fail(std::string(ctx) + ": bus ack stuck");
    }
}

void io_write(Bench& b, uint16_t addr, uint8_t data, const char* ctx) {
    bus_cycle(b, addr, data, 0, ctx);
}
void mem_write(Bench& b, uint16_t addr, uint8_t data, const char* ctx) {
    bus_cycle(b, addr, data, 1, ctx);
}
uint8_t mem_read(Bench& b, uint16_t addr, const char* ctx) {
    bus_cycle(b, addr, 0xFF, 2, ctx);
    return *b.bench_rdata();
}

// Legacy helpers: &7Fxx port. PENR 00xxxxxx selects, INKR 01xxxxxx writes.
void legacy_select(Bench& b, uint8_t pen, const char* ctx) {
    io_write(b, 0x7F00, pen & 0x1F, ctx); // 00 + 5-bit pointer
    b.run(32); // let the GA inksel settle before the colour cycle
}
void legacy_colour(Bench& b, uint8_t hw, const char* ctx) {
    io_write(b, 0x7F00, uint8_t(0x40 | (hw & 0x1F)), ctx); // 01 + HW
    b.run(32); // let the accepted strobe reach the palette owner
}

int run() {
    Bench b;
    b.dut.reset = 1;
    b.run(64);
    b.dut.reset = 0;
    b.run(8);
    // Page-enable input stands in for plus_mmu's captured RMR2 state (as in
    // the p1 bench): raise before ASIC-page traffic and leave on.
    b.dut.asic_page_on = 1;

    uint64_t n = 0;
    while (!*b.cpu_done()) {
        b.tick();
        if (++n > kReadyTimeout) fail("bench CPU did not become ready");
    }
    std::printf("setup: bench ready at cyc %llu, plus_mode=1 page_on=1\n",
                (unsigned long long)b.cyc);

    int failures = 0;
    auto check = [&](const char* label, uint8_t got, uint8_t want) {
        if (got != want) {
            std::printf("FAIL %s: got %02X, want %02X\n", label, got, want);
            ++failures;
        } else {
            std::printf("PASS %s: %02X\n", label, got);
        }
    };

    // --- pen0 repeated-write (Arnold §2.2 secondary-port event) ---
    // HW20 black = 0,0,0 -> word 0x000 ([KT] Palette row 20).
    legacy_select(b, 0x00, "pen-select0");
    legacy_colour(b, 20, "pen-black1"); // 0x54
    check("pen0 first legacy black even (&6400)", mem_read(b, 0x6400, "pen-rd0"), 0x00);
    check("pen0 first legacy black odd (&6401)", mem_read(b, 0x6401, "pen-rd0"), 0x00);
    // Primary-port white: even R=F B=F, odd G=F -> 0xFFF (Arnold §2.2 layout).
    mem_write(b, 0x6400, 0xFF, "pen-white-lo");
    mem_write(b, 0x6401, 0x0F, "pen-white-hi");
    check("pen0 page white even", mem_read(b, 0x6400, "pen-rdW"), 0xFF);
    check("pen0 page white odd", mem_read(b, 0x6401, "pen-rdW"), 0x0F);
    // Repeat the SAME legacy value: a write event, not a value change.
    legacy_select(b, 0x00, "pen-select0b");
    legacy_colour(b, 20, "pen-black2"); // same 0x54
    check("pen0 repeated legacy black even (B8-3)", mem_read(b, 0x6400, "pen-rd1"), 0x00);
    check("pen0 repeated legacy black odd (B8-3)", mem_read(b, 0x6401, "pen-rd1"), 0x00);

    // --- border via nonzero-low-nibble alias (pointer 11 -> border) ---
    legacy_select(b, 0x11, "border-select-alias");
    legacy_colour(b, 20, "border-black1");
    check("border first legacy black even (&6420)", mem_read(b, 0x6420, "bd-rd0"), 0x00);
    check("border first legacy black odd (&6421)", mem_read(b, 0x6421, "bd-rd0"), 0x00);
    mem_write(b, 0x6420, 0xFF, "border-white-lo");
    mem_write(b, 0x6421, 0x0F, "border-white-hi");
    check("border page white even", mem_read(b, 0x6420, "bd-rdW"), 0xFF);
    legacy_select(b, 0x11, "border-select-alias2");
    legacy_colour(b, 20, "border-black2");
    check("border repeated legacy black even (B8-3 alias)", mem_read(b, 0x6420, "bd-rd1"), 0x00);
    check("border repeated legacy black odd (B8-3 alias)", mem_read(b, 0x6421, "bd-rd1"), 0x00);
    // Alias guard: pen1 (entry 1) must not have taken the border write.
    // Reset INKR=0 -> pen1 holds HW0 grey 6,6,6 -> 0x666 ([KT] row 0).
    check("alias guard pen1 even stays grey (&6402)", mem_read(b, 0x6402, "guard-pen1"), 0x66);
    check("alias guard pen1 odd stays grey (&6403)", mem_read(b, 0x6403, "guard-pen1"), 0x06);

    // --- changed-value positive control (HW21 bright blue 0,0,15 -> 0x00F) ---
    legacy_select(b, 0x00, "pen-select-blue");
    legacy_colour(b, 21, "pen-blue"); // 0x55
    check("positive control pen0 blue even (HW21->0F)", mem_read(b, 0x6400, "pos-blue"), 0x0F);
    check("positive control pen0 blue odd (HW21->00)", mem_read(b, 0x6401, "pos-blue"), 0x00);

    // --- unaffected destination guard: sprite colours unreachable by legacy ---
    // Entry 17 (&6422/&6423) keeps reset 0x000 across all legacy traffic.
    check("guard sprite colour 1 even (&6422)", mem_read(b, 0x6422, "guard-spr"), 0x00);
    check("guard sprite colour 1 odd (&6423)", mem_read(b, 0x6423, "guard-spr"), 0x00);

    // --- ASIC-only reset import (production reset boundary, no new silicon
    // claim) ---
    // GA resets with machine reset; asic_regs resets with plus_asic_reset.
    // An ASIC-only reset must re-import the CURRENT retained GA shadows, not
    // the cold 666/006 defaults. All programming uses real bus cycles; the
    // reset window holds the bus idle so no LEGACY_PAL_WR event fires and any
    // palette change after release is the shadow import, not an event.
    legacy_select(b, 0x00, "rst-pen-select");
    legacy_colour(b, 20, "rst-pen-black"); // pen0 HW20 black -> 000
    legacy_select(b, 0x11, "rst-border-select");
    legacy_colour(b, 21, "rst-border-blue"); // border HW21 blue -> 00F
    check("rst setup pen0 black even (&6400)", mem_read(b, 0x6400, "rst-rd-pen"), 0x00);
    check("rst setup pen0 black odd (&6401)", mem_read(b, 0x6401, "rst-rd-pen"), 0x00);
    check("rst setup border blue even (&6420)", mem_read(b, 0x6420, "rst-rd-bd"), 0x0F);
    check("rst setup border blue odd (&6421)", mem_read(b, 0x6421, "rst-rd-bd"), 0x00);
    // Diverge the page so the import must overwrite page state.
    mem_write(b, 0x6400, 0xFF, "rst-diverge-lo");
    mem_write(b, 0x6401, 0x0F, "rst-diverge-hi");
    check("rst setup pen0 page white even", mem_read(b, 0x6400, "rst-rdW"), 0xFF);
    check("rst setup pen0 page white odd", mem_read(b, 0x6401, "rst-rdW"), 0x0F);
    // ASIC-only reset: machine reset stays 0 (GA shadows retained), bus idle.
    b.dut.asic_reset_i = 1;
    b.run(16);
    b.dut.asic_reset_i = 0;
    b.run(64); // one-shot import + settle, no legacy traffic (no events)
    check("asic-only reset pen0 imports retained black even", mem_read(b, 0x6400, "rst-imp-pen"), 0x00);
    check("asic-only reset pen0 imports retained black odd", mem_read(b, 0x6401, "rst-imp-pen"), 0x00);
    check("asic-only reset border imports retained blue even", mem_read(b, 0x6420, "rst-imp-bd"), 0x0F);
    check("asic-only reset border imports retained blue odd", mem_read(b, 0x6421, "rst-imp-bd"), 0x00);
    check("asic-only reset pen1 stays grey even (&6402)", mem_read(b, 0x6402, "rst-imp-pen1"), 0x66);
    check("asic-only reset pen1 stays grey odd (&6403)", mem_read(b, 0x6403, "rst-imp-pen1"), 0x06);
    check("asic-only reset sprite stays 000 even (&6422)", mem_read(b, 0x6422, "rst-imp-spr"), 0x00);
    check("asic-only reset sprite stays 000 odd (&6423)", mem_read(b, 0x6423, "rst-imp-spr"), 0x00);
    // No delayed replay: a later page edit sticks while shadows don't change.
    mem_write(b, 0x6400, 0xFF, "rst-post-lo");
    mem_write(b, 0x6401, 0x0F, "rst-post-hi");
    b.run(64); // shadows idle, no events
    check("page edit after import sticks even", mem_read(b, 0x6400, "rst-post-rd"), 0xFF);
    check("page edit after import sticks odd", mem_read(b, 0x6401, "rst-post-rd"), 0x0F);

    if (failures != 0)
        fail("B8-3 palette events: " + std::to_string(failures) + " assertion(s) failed");
    std::printf("PASS B8-3: repeated legacy writes restore palette entries through the production bus/page path\n");
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    try {
        return run();
    } catch (const TestFailure& e) {
        std::printf("FAIL: %s\n", e.what());
        return 1;
    }
}
