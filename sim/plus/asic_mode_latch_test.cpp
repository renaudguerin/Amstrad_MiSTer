// Technical information sourced from the Amstrad CPC CRTC Compendium
// by Longshot (CC BY-NC-ND 4.0). FR v1.11 §§9.3.1–9.3.3 pp52–53, rendered:
// every type needs >=2us HSYNC; the ASIC mode-request window ends one
// character later than classic (raw R2+7, not R2+6, for a long HSYNC).
// FF in mode0 decodes pen15, in mode2 pen1 on every dot. Score well after
// the pulse: FR §9.3.4.1 p54 leaves the exact type3 mixed pixel unresolved.
#include "Vasic_mode_latch_test_top.h"
#include "verilated.h"
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr unsigned character_ticks = 64; // actual GA, 16 dots at 1-in-4
struct Bench {
    Vasic_mode_latch_test_top d;
    unsigned ticks = 0, rise = 0, width = 0, stored = 0, mode_events = 0;
    bool previous_hs = false, previous_event = false;

    void tick() {
        d.clk = 0; d.eval();
        if (d.hs && !previous_hs) rise = ticks;
        if (!d.hs && previous_hs) width = ticks - rise;
        if (d.mode_event && !previous_event) ++mode_events;
        previous_hs = d.hs;
        previous_event = d.mode_event;
        d.clk = 1; d.eval(); ++ticks;
        d.clk = 0; d.eval();
    }
    void run(unsigned count) { while (count--) tick(); }
    template<class F> void until(F condition) {
        for (unsigned n = 0; n < 100000; ++n) {
            if (condition()) return;
            tick();
        }
        throw std::runtime_error("fixture timed out");
    }
    Bench(unsigned pulse_chars, bool no_hsync) {
        d.RESET_N = 0; d.sna_load = 0; d.snapshot_hs = 0; d.ga_iorq_n = 1; d.ga_data = 0;
        for (unsigned i = 0; i < 5; ++i) d.regs[i] = 0;
        const uint8_t registers[18] = {
            63, 63, uint8_t(no_hsync ? 127 : 16), uint8_t(0x80 | pulse_chars),
            1, 0, 1, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };
        for (unsigned i = 0; i < 18; ++i)
            d.regs[i / 4] |= unsigned(registers[i]) << (8 * (i % 4));
        run(128);
        d.RESET_N = 1; d.sna_load = 1; tick(); d.sna_load = 0; run(8);
        // Let the real snapshot display reseed finish before scoring pixels.
        until([&] { return bool(d.de); });
        until([&] { return d.hcc == 2; });
        mode_events = 0; width = 0;
    }
    void request_mode2() {
        d.ga_data = 0x82; d.ga_iorq_n = 0;
        for (unsigned i = 0; i < character_ticks; ++i) {
            tick();
            if (!stored && d.requested_mode == 2) stored = ticks;
        }
        d.ga_iorq_n = 1; tick();
        if (d.requested_mode != 2 || !stored)
            throw std::runtime_error("RMR request not stored by the GA");
    }
    bool score(const char* name, unsigned expected_pen) {
        unsigned good = 0, wrong = 0;
        for (unsigned i = 0; i < 128; ++i) {
            if (d.dot && d.de && !d.hs) {
                if (d.pen_o == expected_pen) ++good;
                else ++wrong;
            }
            tick();
        }
        const bool pass = good == 32 && wrong == 0;
        std::cout << name << ": expected pen=" << expected_pen
                  << " good=" << good << " wrong=" << wrong
                  << " raw-width=" << width
                  << " request-offset=" << int(stored) - int(rise)
                  << " mode-events=" << mode_events
                  << (pass ? " PASS\n" : " FAIL\n");
        return pass;
    }
};

// A restored active HSYNC has no serialized qualification history. The
// mode event must not escape the restore. DE itself restarts at a later
// line seam, so check the public event here, then decoded pens after a
// fresh complete pulse. No internal timer state supplies an expectation.
bool restore_running_pulse() {
    Bench b(4, false);
    b.until([&] { return bool(b.d.hs); });
    b.run(character_ticks);
    b.d.snapshot_hs = 1; b.d.sna_load = 1; b.tick(); b.d.sna_load = 0;
    b.until([&] { return !b.d.hs; });
    b.run(3 * character_ticks);
    // The restored pulse's delayed window has closed. A real request
    // inside it would be a valid update, not a phantom event.
    b.request_mode2();
    bool pass = b.mode_events == 0;
    std::cout << "restore-running-pulse: no phantom mode event "
              << (pass ? "PASS\n" : "FAIL\n");
    b.until([&] { return bool(b.d.de); });
    b.until([&] { return b.d.hcc == 2; });
    b.until([&] { return bool(b.d.hs); });
    b.until([&] { return !b.d.hs; });
    b.run(3 * character_ticks);
    return b.score("post-restore-fresh-pulse", 1) && pass;
}

bool exercise(const std::string& name) {
    const bool absent = name == "no-hsync";
    const bool during = name == "during-valid";
    const bool early = name == "before-asic-deadline";
    const bool late = name == "after-asic-deadline";
    const bool after_fall = name == "after-short-fall";
    const unsigned pulse = absent ? 2 : name == "short" ? 1 : during ? 4 :
                           after_fall ? 3 : (early || late) ? 14 : 2;
    Bench b(pulse, absent);
    if (during || early || late || after_fall) {
        b.until([&] { return bool(b.d.hs); });
        const unsigned start = b.ticks;
        b.run(during ? character_ticks : early ? 6 * character_ticks + 8 :
              late ? 7 * character_ticks + 8 : 3 * character_ticks + 8);
        b.request_mode2();
        // Guard actual GA storage time, not merely when the driver raised IORQ.
        const unsigned accepted = b.stored - start;
        // For N=3, p52's one-us ASIC delay plus R3-limited GA interval
        // derives an N+1 deadline; p53's chart itself uses only long R3=14.
        if (after_fall && !(accepted > 3 * character_ticks && accepted < 4 * character_ticks))
            throw std::runtime_error("post-fall request missed the derived 3-to-4us window");
        if (early && !(accepted > 6 * character_ticks && accepted < 7 * character_ticks))
            throw std::runtime_error("early request missed the source 6-to-7us window");
        if (late && !(accepted > 7 * character_ticks && accepted < 8 * character_ticks))
            throw std::runtime_error("late request missed the source after-7us window");
    } else b.request_mode2();
    if (absent) b.until([&] { return b.d.hcc == 30; });
    else {
        // The post-fall request already crossed this pulse. Waiting for
        // another rise here would hide an incorrectly early mode event.
        if (!after_fall) {
            b.until([&] { return bool(b.d.hs); });
            b.until([&] { return !b.d.hs; });
        }
        b.run(3 * character_ticks);
    }
    const unsigned expected = (name == "short" || absent || late) ? 15 : 1;
    return b.score(name.c_str(), expected);
}
} // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    try {
        bool pass = true;
        for (const char* name : {"short", "exact-two", "during-valid",
                "before-asic-deadline", "after-asic-deadline",
                "after-short-fall", "no-hsync"})
            pass = exercise(name) && pass;
        pass = restore_running_pulse() && pass;
        std::cout << "asic-mode-latch: " << (pass ? "PASS" : "FAIL") << '\n';
        return pass ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "asic-mode-latch: " << error.what() << '\n';
        return 2;
    }
}
