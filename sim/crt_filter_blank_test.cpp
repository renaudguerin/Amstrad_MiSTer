// Deterministic seam tests for crt_filter live blanking and its production mux.

#include <verilated.h>
#include "Vcrt_filter_blank_test_top.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[noreturn]] void fail(const std::string& message) {
    throw TestFailure(message);
}

// Production is a 64 MHz master clock with a 4 MHz PHI_EN_N enable. Pixel-M2
// is 16 MHz, so the documented type-1 R2.JIT displacement of three pixels is
// twelve master clocks and is deliberately not CE-aligned.
constexpr unsigned kClksPerCe = 16;
constexpr unsigned kClksPerPixelM2 = 4;
constexpr unsigned kLineCeTicks = 256;
constexpr unsigned kLineClks = kLineCeTicks * kClksPerCe;
constexpr unsigned kFullBlankTicks = 64;
constexpr unsigned kFullBlankClks = kFullBlankTicks * kClksPerCe;
constexpr unsigned kFullActiveTicks = 192;
// The integrated GA vector measures the ordinary type-1 pulse as 64
// Pixel-M2, i.e. 16 CE or 256 master clocks.
constexpr unsigned kRawSyncWidthTicks = 16;
constexpr unsigned kRawSyncWidthClks = kRawSyncWidthTicks * kClksPerCe;
constexpr unsigned kR2JitPixels = 3;
constexpr unsigned kR2JitClks = kR2JitPixels * kClksPerPixelM2;

struct Sample {
    uint8_t hsync_i;
    uint8_t hsync_o;
    uint8_t vsync_o;
    uint8_t hblank;
    uint8_t hblank_live;
    uint8_t vblank;
    uint8_t hsync_selected;
    uint8_t vsync_selected;
    uint8_t hblank_selected;
    uint8_t vblank_selected;
    uint8_t no_hsync;
};

class CrtFilterBench {
public:
    CrtFilterBench()
        : context_(std::make_unique<VerilatedContext>()),
          dut_(std::make_unique<Vcrt_filter_blank_test_top>(context_.get())) {
        dut_->CLK = 0;
        dut_->CE_4 = 0;
        dut_->HSYNC_I = 0;
        dut_->VSYNC_I = 0;
        dut_->SYNC_FILTER = 0;
        dut_->HSYNC_RAW = 0;
        dut_->VSYNC_RAW = 0;
        dut_->HBLANK_RAW = 0;
        dut_->VBLANK_RAW = 0;
        dut_->eval();
    }

    ~CrtFilterBench() { dut_->final(); }

    Sample tick_clk(bool ce, bool hs_before, bool vs_before,
                    bool hs_after, bool vs_after) {
        drive_raw(hs_before, vs_before);
        dut_->CE_4 = ce;
        dut_->CLK = 0;
        dut_->eval();
        dut_->CLK = 1;
        dut_->eval();

        // CRTC/ASIC outputs are registered in the same master-clock domain.
        // Applying the transition after the edge models Verilog NBA timing.
        drive_raw(hs_after, vs_after);
        dut_->eval();
        Sample result = sample();

        dut_->CLK = 0;
        dut_->eval();
        ++master_cycle_;
        return result;
    }

    Sample tick_ce(bool hsync, bool vsync) {
        Sample at_ce{};
        for (unsigned sub = 0; sub < kClksPerCe; ++sub) {
            const auto value = tick_clk(sub == 0, hsync, vsync, hsync, vsync);
            if (sub == 0) at_ce = value;
        }
        return at_ce;
    }

    std::vector<Sample> run_line_ce(unsigned line_ticks = kLineCeTicks,
                                    unsigned sync_start = 0,
                                    unsigned sync_width = kRawSyncWidthTicks,
                                    bool vsync = false) {
        std::vector<Sample> samples;
        samples.reserve(line_ticks);
        for (unsigned tick = 0; tick < line_ticks; ++tick) {
            const bool hs = tick >= sync_start && tick < sync_start + sync_width;
            samples.push_back(tick_ce(hs, vsync));
        }
        return samples;
    }

    std::vector<Sample> run_pulses_master(
        const std::vector<std::pair<unsigned, unsigned>>& pulses,
        unsigned clocks = kLineClks) {
        std::vector<Sample> samples;
        samples.reserve(clocks);
        bool current_hs = false;
        for (unsigned clock = 0; clock < clocks; ++clock) {
            bool next_hs = false;
            for (const auto& pulse : pulses) {
                if (clock >= pulse.first && clock < pulse.second) {
                    next_hs = true;
                    break;
                }
            }
            samples.push_back(tick_clk(clock % kClksPerCe == 0,
                                       current_hs, false, next_hs, false));
            current_hs = next_hs;
        }
        return samples;
    }

    void settle(unsigned lines = 24) {
        for (unsigned line = 0; line < 4; ++line)
            run_line_ce(kLineCeTicks, 0, kRawSyncWidthTicks, true);
        for (unsigned line = 0; line < lines; ++line)
            run_line_ce();
    }

    Sample selector(unsigned mode, bool raw_hs, bool raw_vs,
                    bool raw_hblank, bool raw_vblank) {
        dut_->SYNC_FILTER = mode;
        dut_->HSYNC_RAW = raw_hs;
        dut_->VSYNC_RAW = raw_vs;
        dut_->HBLANK_RAW = raw_hblank;
        dut_->VBLANK_RAW = raw_vblank;
        dut_->eval();
        return sample();
    }

private:
    void drive_raw(bool hsync, bool vsync) {
        dut_->HSYNC_I = hsync;
        dut_->VSYNC_I = vsync;
        dut_->HSYNC_RAW = hsync;
        dut_->VSYNC_RAW = vsync;
        dut_->HBLANK_RAW = hsync;
        dut_->VBLANK_RAW = vsync;
    }

    Sample sample() const {
        return {
            static_cast<uint8_t>(dut_->HSYNC_I),
            static_cast<uint8_t>(dut_->HSYNC_O),
            static_cast<uint8_t>(dut_->VSYNC_O),
            static_cast<uint8_t>(dut_->HBLANK),
            static_cast<uint8_t>(dut_->HBLANK_LIVE),
            static_cast<uint8_t>(dut_->VBLANK),
            static_cast<uint8_t>(dut_->HSYNC_SELECTED),
            static_cast<uint8_t>(dut_->VSYNC_SELECTED),
            static_cast<uint8_t>(dut_->HBLANK_SELECTED),
            static_cast<uint8_t>(dut_->VBLANK_SELECTED),
            static_cast<uint8_t>(dut_->NO_HSYNC_DEBUG),
        };
    }

    std::unique_ptr<VerilatedContext> context_;
    std::unique_ptr<Vcrt_filter_blank_test_top> dut_;
    uint64_t master_cycle_ = 0;
};

void test_full_mode_baseline() {
    CrtFilterBench tb;
    tb.settle();
    const auto samples = tb.run_line_ce();
    unsigned hsync_high = 0;
    unsigned hblank_high = 0;
    for (const auto& sample : samples) {
        hsync_high += sample.hsync_o != 0;
        hblank_high += sample.hblank != 0;
    }
    if (hsync_high != 16)
        fail("Full regenerated HSYNC width is not 16 CE ticks");
    if (hblank_high != kFullBlankTicks) {
        std::ostringstream out;
        out << "Full HBLANK width is " << hblank_high << " CE ticks, expected "
            << kFullBlankTicks << " blank / " << kFullActiveTicks << " active";
        fail(out.str());
    }
}

void check_phase_window(unsigned start, unsigned raw_width,
                        const std::string& label) {
    CrtFilterBench tb;
    tb.settle();
    const auto samples = tb.run_pulses_master({{start, start + raw_width}});
    unsigned high = 0;
    for (const auto& sample : samples) high += sample.hblank_live != 0;
    for (unsigned clock = 0; clock < start; ++clock) {
        if (samples[clock].hblank_live)
            fail(label + " asserted HBLANK_LIVE before the raw edge");
    }
    for (unsigned clock = start; clock < start + kFullBlankClks; ++clock) {
        if (!samples[clock].hblank_live)
            fail(label + " dropped HBLANK_LIVE before 1024 master clocks");
    }
    if (samples[start + kFullBlankClks].hblank_live)
        fail(label + " extended HBLANK_LIVE past 1024 master clocks");
    if (high != kFullBlankClks) {
        std::ostringstream out;
        out << label << " produced " << high << " blank master clocks, expected "
            << kFullBlankClks;
        fail(out.str());
    }
}

void test_live_blank_master_phase_and_r2jit() {
    check_phase_window(0, kRawSyncWidthClks, "normal phase");
    // R2.JIT moves the start by +3 Pixel-M2 while the ordinary trailing edge
    // remains fixed. The acquisition window keeps its width and live phase.
    check_phase_window(kR2JitClks, kRawSyncWidthClks - kR2JitClks,
                       "type-1 +3-pixel R2.JIT phase");
}

void test_long_raw_force_blank_wins() {
    CrtFilterBench tb;
    tb.settle();
    constexpr unsigned kLongRawClks = kFullBlankClks + 256;
    const auto samples = tb.run_pulses_master({{0, kLongRawClks}},
                                               kLongRawClks + 32);
    for (unsigned clock = 0; clock < kLongRawClks; ++clock) {
        if (!samples[clock].hblank_live)
            fail("Live acquisition opened while raw force blank remained high");
    }
    if (samples[kLongRawClks].hblank_live)
        fail("Live blank remained high after the longer raw pulse ended");
}

void test_frequent_sync_does_not_restart_window() {
    CrtFilterBench tb;
    tb.settle();
    const auto samples = tb.run_pulses_master({{0, 64}, {512, 576}}, 1200);
    for (unsigned clock = 0; clock < kFullBlankClks; ++clock) {
        if (!samples[clock].hblank_live)
            fail("A short retrigger opened the original acquisition window early");
    }
    if (samples[kFullBlankClks].hblank_live)
        fail("A too-frequent raw sync restarted and lengthened Live blanking");
}

void test_expiry_clock_sync_starts_new_window() {
    CrtFilterBench tb;
    tb.settle();
    const auto samples = tb.run_pulses_master(
        {{0, 64}, {kFullBlankClks - 1, kFullBlankClks - 1 + 64}},
        2 * kFullBlankClks + 32);
    const unsigned second_expiry = (kFullBlankClks - 1) + kFullBlankClks;
    for (unsigned clock = 0; clock < second_expiry; ++clock) {
        if (!samples[clock].hblank_live)
            fail("HSYNC on the exact expiry clock failed to acquire a new Live window");
    }
    if (samples[second_expiry].hblank_live)
        fail("expiry-clock reacquisition extended beyond its own 1024 clocks");
}

void test_missing_sync_uses_full_blank() {
    CrtFilterBench tb;
    tb.settle();
    for (unsigned tick = 0; tick < 65540; ++tick) tb.tick_ce(false, false);
    auto sample = tb.tick_ce(false, true);
    if (!sample.no_hsync)
        fail("Horizontal-sync absence was not latched at VSYNC");
    tb.tick_ce(true, false);
    tb.tick_ce(false, false);

    bool saw_blank = false;
    bool saw_active = false;
    for (unsigned tick = 0; tick < 2300; ++tick) {
        sample = tb.tick_ce(false, false);
        if (sample.hblank_live != sample.hblank)
            fail("Missing-sync Live mode did not fall back to Full HBLANK");
        saw_blank |= sample.hblank != 0;
        saw_active |= sample.hblank == 0;
    }
    if (!saw_blank || !saw_active)
        fail("Missing-sync recovery did not produce both Full blank and active intervals");
}

void test_stuck_high_without_vsync_uses_full_blank() {
    CrtFilterBench tb;
    tb.settle();

    // Neither a new HSYNC edge nor VSYNC is available after the input sticks
    // high. The horizontal watchdog must still promote the already learned
    // Full cadence instead of leaving Live permanently forced blank.
    Sample sample{};
    for (unsigned tick = 0; tick < 65540; ++tick)
        sample = tb.tick_ce(true, false);
    if (!sample.no_hsync)
        fail("stuck-high HSYNC without VSYNC never promoted missing-sync fallback");

    bool saw_blank = false;
    bool saw_active = false;
    for (unsigned tick = 0; tick < 2300; ++tick) {
        sample = tb.tick_ce(true, false);
        if (sample.hblank_live != sample.hblank)
            fail("stuck-high fallback did not select established Full HBLANK");
        saw_blank |= sample.hblank != 0;
        saw_active |= sample.hblank == 0;
    }
    if (!saw_blank || !saw_active)
        fail("stuck-high fallback did not recover alternating blank/active intervals");
}

void test_masked_retrigger_preserves_watchdog_period() {
    CrtFilterBench tb;
    tb.settle();

    // Establish one healthy line edge, then present another raw edge only
    // 32 CE later while the too-frequent-sync mask is still active. Holding
    // that second pulse high removes both later HSYNC edges and VSYNC. The
    // fallback must reuse the accepted 256-CE cadence, not the masked 32-CE
    // interval (which would keep Full HBLANK permanently high).
    tb.tick_ce(true, false);
    tb.tick_ce(false, false);
    for (unsigned tick = 0; tick < 30; ++tick) tb.tick_ce(false, false);

    Sample sample = tb.tick_ce(true, false);
    for (unsigned tick = 0; tick < 65540; ++tick)
        sample = tb.tick_ce(true, false);
    if (!sample.no_hsync)
        fail("masked retrigger fixture never entered missing-sync fallback");

    std::vector<unsigned> rising_edges;
    std::vector<unsigned> falling_edges;
    bool previous_blank = sample.hblank != 0;
    for (unsigned tick = 0; tick < 2300; ++tick) {
        sample = tb.tick_ce(true, false);
        const bool blank = sample.hblank != 0;
        if (!previous_blank && blank) rising_edges.push_back(tick);
        if (previous_blank && !blank) falling_edges.push_back(tick);
        previous_blank = blank;
    }
    if (rising_edges.size() < 3)
        fail("masked short retrigger did not recover three Full HBLANK starts");
    for (unsigned edge = 1; edge < 3; ++edge) {
        if (rising_edges[edge] - rising_edges[edge - 1] != kLineCeTicks)
            fail("masked short retrigger changed the recovered 256-CE period");
    }
    for (unsigned edge = 0; edge < 3; ++edge) {
        auto falling = falling_edges.end();
        for (auto candidate = falling_edges.begin(); candidate != falling_edges.end();
             ++candidate) {
            if (*candidate > rising_edges[edge]) {
                falling = candidate;
                break;
            }
        }
        if (falling == falling_edges.end() ||
            *falling - rising_edges[edge] != kFullBlankTicks)
            fail("masked short retrigger changed the recovered 64-CE blank width");
    }
}

void test_production_mode_selector() {
    CrtFilterBench tb;
    tb.settle();

    auto full = tb.selector(0, true, true, true, true);
    if (full.hsync_selected != full.hsync_o ||
        full.vsync_selected != full.vsync_o ||
        full.hblank_selected != full.hblank ||
        full.vblank_selected != full.vblank)
        fail("Full mode did not select the complete filtered output tuple");

    auto live = tb.selector(1, !full.hsync_o, !full.vsync_o,
                            !full.hblank_live, !full.vblank);
    if (live.hsync_selected != live.hsync_o ||
        live.vsync_selected != live.vsync_o ||
        live.hblank_selected != live.hblank_live ||
        live.vblank_selected == live.vblank)
        fail("Live mode did not select filtered sync, live HBLANK, and raw VBLANK");

    auto off = tb.selector(2, !live.hsync_o, !live.vsync_o,
                           !live.hblank_live, !live.vblank);
    if (off.hsync_selected != static_cast<uint8_t>(!live.hsync_o) ||
        off.vsync_selected != static_cast<uint8_t>(!live.vsync_o) ||
        off.hblank_selected != static_cast<uint8_t>(!live.hblank_live) ||
        off.vblank_selected != static_cast<uint8_t>(!live.vblank))
        fail("Off mode did not select the complete raw output tuple");
}

struct TestCase {
    const char* name;
    void (*run)();
};

constexpr std::array<TestCase, 9> kTests = {{
    {"settled regenerated HSYNC and Full HBLANK baseline", test_full_mode_baseline},
    {"Live HBLANK preserves master phase and the +3-pixel R2.JIT shift", test_live_blank_master_phase_and_r2jit},
    {"raw force blank longer than the scaler minimum remains blank", test_long_raw_force_blank_wins},
    {"too-frequent raw sync does not restart the acquisition window", test_frequent_sync_does_not_restart_window},
    {"sync on the exact expiry clock starts a new Live window", test_expiry_clock_sync_starts_new_window},
    {"missing raw sync falls back to the established Full HBLANK", test_missing_sync_uses_full_blank},
    {"stuck-high raw sync without VSYNC falls back to Full HBLANK", test_stuck_high_without_vsync_uses_full_blank},
    {"masked short retrigger preserves the learned watchdog cadence", test_masked_retrigger_preserves_watchdog_period},
    {"production selector pins Full, Live, and Off output tuples", test_production_mode_selector},
}};

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    unsigned passed = 0;
    unsigned failed = 0;
    for (const auto& test : kTests) {
        try {
            test.run();
            std::cout << "PASS: " << test.name << '\n';
            ++passed;
        } catch (const std::exception& error) {
            std::cerr << "FAIL: " << test.name << ": " << error.what() << '\n';
            ++failed;
        }
    }
    std::cout << "Summary: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
