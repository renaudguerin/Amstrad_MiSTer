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
    uint8_t shift;
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
        dut_->BYTE_OWNER = 1;
        dut_->H_CONTEXT_CLEAR = 0;
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

    Sample configure_byte_owner(bool eligible, bool clear) {
        dut_->BYTE_OWNER = eligible;
        dut_->H_CONTEXT_CLEAR = clear;
        dut_->eval();
        return sample();
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
            static_cast<uint8_t>(dut_->SHIFT),
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

// SHAKER B0 00CC leaves equal-period raw HSYNC 16 CE4 ticks early
// after its short line. Preserve the ordinary selected lag, then release
// the phase on live return or missing source without another VSYNC.
void test_full_relocks_early_phase_and_releases_without_vsync() {
    constexpr unsigned frame_lines = 312;
    constexpr unsigned ordinary_rise = 160;
    constexpr unsigned early_rise = ordinary_rise - 16;
    constexpr unsigned raw_width = 24;
    const auto selected_rise = [](const std::vector<Sample>& line) {
        unsigned rises = 0, rise = 0, width = 0;
        for (unsigned tick = 1; tick < line.size(); ++tick) {
            if (line[tick].hsync_o && !line[tick-1].hsync_o) {
                ++rises;
                rise = tick;
            }
            width += line[tick].hsync_o;
        }
        if (rises != 1 || width != 16) fail("expected one 16-CE4 Full HSYNC in the line");
        return rise;
    };

    for (bool lose_source : {false, true}) {
        CrtFilterBench tb;
        unsigned ordinary_output = 0, previous = 0;
        for (unsigned frame = 0; frame < 6; ++frame) {
            const unsigned limit = frame < 5 ? frame_lines : 128;
            for (unsigned y = 0; y < limit; ++y) {
                const unsigned rise = y < 22 ? ordinary_rise : early_rise;
                const auto line = tb.run_line_ce(kLineCeTicks, rise, raw_width, y < 16);
                if (frame == 5 && y == 20) ordinary_output = selected_rise(line);
                if (frame == 5 && y >= 85) {
                    previous = selected_rise(line);
                    if (previous + 16 != ordinary_output)
                        fail("Full HSYNC did not recover ordinary lag for the stable early pulse");
                }
            }
        }
        if (ordinary_output < ordinary_rise || ordinary_output >= ordinary_rise + 16)
            fail("fixture did not establish ordinary Full HSYNC lag before the step");

        unsigned observed = 0, handoffs = 0;
        for (unsigned y = 0; y < (lose_source ? 300u : 100u); ++y) {
            const auto line = tb.run_line_ce(kLineCeTicks, ordinary_rise,
                                            lose_source ? 0 : raw_width);
            if (lose_source && !line.front().no_hsync) continue;
            const auto rise = selected_rise(line);
            if (!lose_source) {
                const auto spacing = kLineCeTicks + rise - previous;
                if (spacing == kLineCeTicks + 16) ++handoffs;
                else if (spacing != kLineCeTicks)
                    fail("live Full phase handoff duplicated or skipped HSYNC");
            }
            previous = rise;
            if ((!lose_source && y < 6)) continue;
            if (rise != ordinary_output)
                fail(lose_source ? "missing-source Full HSYNC retained the early phase" :
                                   "Full HSYNC retained the early phase after a live return without VSYNC");
            unsigned active = 0;
            for (const auto& sample : line) active += !sample.hblank;
            if (active != kFullActiveTicks) fail("Full phase handoff damaged the active span");
            ++observed;
        }
        if (observed < 30 || (!lose_source && handoffs != 1))
            fail("Full phase handoff was not observed for both stable intervals");
    }
}

// SHAKER B0 00CF parks a short horizontal pulse two enables ahead of
// the nominal selected phase. A fresh H/VS anchor can arrive in the
// portion of the nominal pulse that the output has already finished.
void test_full_keeps_midpoint_slot_through_vsync_anchor() {
    constexpr unsigned retrace_period = 312 * kLineCeTicks - 4;
    constexpr unsigned first_retrace = 23448;
    constexpr unsigned final_retrace = first_retrace + 6 * retrace_period;
    constexpr unsigned source_rise = final_retrace + 1972 + 21 * kLineCeTicks;
    constexpr unsigned midpoint_rise = source_rise + 6;
    constexpr unsigned pulse_width = 16;

    for (unsigned anchor_delay : {1, 13, 17, 18, 19}) {
        CrtFilterBench tb;
        const unsigned anchor = midpoint_rise + anchor_delay;
        const unsigned stop = anchor + 3 * kLineCeTicks + 32;
        std::vector<unsigned> rises, falls;
        bool previous = false;
        for (unsigned tick = 0; tick < stop; ++tick) {
            bool hs = false, vs = false;
            if (tick >= anchor) {
                hs = (tick - anchor) % kLineCeTicks < pulse_width;
                vs = true;
            } else if (tick < first_retrace) {
                hs = tick >= 80 && (tick - 80) % kLineCeTicks < pulse_width;
            } else {
                const unsigned field = (tick - first_retrace) / retrace_period;
                const unsigned t = (tick - first_retrace) % retrace_period;
                const unsigned early = 460 + 4 * (field / 2);
                const unsigned early_width = field % 2 ? 20 : pulse_width;
                vs = t < 4092;
                if (tick < source_rise) {
                    hs = (t < 184 && (t + 72) % kLineCeTicks < pulse_width) ||
                         (t >= 184 && t < 184 + pulse_width) ||
                         (t >= early && t < early + early_width) ||
                         (t >= 932 && t < 1716 && (t - 932) % kLineCeTicks <
                            (t < 952 ? early_width : pulse_width)) ||
                         (t >= 1972 && (t - 1972) % kLineCeTicks < pulse_width);
                }
            }
            const bool high = tb.tick_ce(hs, vs).hsync_o;
            if (tick >= midpoint_rise - 2 && high != previous)
                (high ? rises : falls).push_back(tick);
            previous = high;
        }

        std::vector<unsigned> expected{midpoint_rise};
        // After the nominal pulse has fallen, a fresh anchor owns its slot.
        for (unsigned slot = anchor_delay == 19 ? 0 : 1; slot <= 3; ++slot)
            expected.push_back(anchor + slot * kLineCeTicks + 8);
        if (rises != expected || rises.size() != falls.size()) {
            std::ostringstream out;
            out << "wrong Full HSYNC slots after midpoint anchor +" << anchor_delay;
            for (auto rise : rises) out << " rise@" << rise - midpoint_rise;
            for (auto fall : falls) out << " fall@" << fall - midpoint_rise;
            fail(out.str());
        }
        for (unsigned index = 0; index < rises.size(); ++index)
            if (falls[index] != rises[index] + pulse_width)
                fail("fresh VSYNC changed the width of a selected HSYNC pulse");
    }
}

// SHAKER BU 021B enters a 55-CE4 raw pulse one enable later, on the
// clock of the next legacy origin. The preceding pulse is 56 CE4 wide,
// leaving the raw fall and the shaped monitor sync on their old grid.
void test_full_preserves_bu_crop_phase() {
    constexpr unsigned step_line = 2 * 312 + 66;
    constexpr unsigned release_line = step_line + 8;
    struct PulseCase {
        unsigned width;
        bool narrow_before_step;
        bool translate;
        bool expect_phase_shift;
        unsigned delayed_fall;
        const char* name;
    };
    const std::array<PulseCase, 6> pulse_cases = {{
        {56, false, false, false, 0, "long fixed-fall BU pulse"},
        {56, false, true, true, 0, "long equal-width translated pulse"},
        {24, false, false, true, 0, "ordinary-width fixed-fall pulse"},
        {56, true, true, true, 0, "translation after a width-only change"},
        {56, false, false, false, 57, "delayed whole-pulse translation"},
        {56, false, false, false, 24, "delayed ordinary-width fall"},
    }};
    unsigned crop_steps = 0;
    for (const auto& pulse : pulse_cases) {
        CrtFilterBench tb;
        bool hs = false;
        unsigned base_rise = 0, base_open = 0;
        const unsigned last_line = pulse.delayed_fall ? release_line + 8 : release_line;
        for (unsigned line = 0; line < last_line; ++line) {
            const unsigned step = line >= step_line ? kClksPerCe : 0;
            const unsigned raw_start = 160 * kClksPerCe + step;
            const unsigned raw_width = pulse.width - unsigned(pulse.narrow_before_step &&
                                                            line >= step_line - 8);
            const bool released = pulse.delayed_fall && line >= release_line;
            const unsigned raw_end = released ? (160 + pulse.delayed_fall) * kClksPerCe
                                              : (160 + raw_width) * kClksPerCe +
                                                  (pulse.translate ? step : 0);
            const bool vs = line % 312 < 4;
            bool old_out = false, old_hblank = false;
            unsigned rise = 0, open = 0, rises = 0, opens = 0, width = 0;
            for (unsigned clock = 0; clock < kLineClks; ++clock) {
                const bool next_hs = clock >= raw_start && clock < raw_end;
                const auto sample = tb.tick_clk(clock % kClksPerCe == 0,
                                                hs, vs, next_hs, vs);
                hs = next_hs;
                if (clock && sample.hsync_selected && !old_out) {
                    rise = clock;
                    ++rises;
                }
                if (clock && !sample.hblank_selected && old_hblank) {
                    open = clock;
                    ++opens;
                }
                width += sample.hsync_selected != 0;
                old_out = sample.hsync_selected;
                old_hblank = sample.hblank_selected;
                if (line >= step_line - 2 && sample.no_hsync)
                    fail("BU crop lost the raw source");
            }
            if (line < step_line - 2) continue;
            if (rises != 1 || opens != 1 || width != 16 * kClksPerCe)
                fail("BU crop fixture needs one intact Full HSYNC and opening per line");
            if (line == step_line - 2) {
                base_rise = rise;
                base_open = open;
            }
            if (line == step_line - 1 && (rise != base_rise || open != base_open))
                fail("BU crop fixture did not train a stable line");
            if (pulse.expect_phase_shift || released) {
                const unsigned settle_line = (released ? release_line : step_line) + 4;
                if (line >= settle_line &&
                        (rise != base_rise + kClksPerCe || open != base_open + kClksPerCe))
                    fail(std::string("Full crop rejected phase movement for ") + pulse.name);
            } else if (line >= step_line && (rise != base_rise || open != base_open)) {
                ++crop_steps;
                std::cout << "BU line " << line << " Full HS " << int(rise) - int(base_rise)
                          << " and HBlank open " << int(open) - int(base_open)
                          << " master-clock shifts\n";
            }
        }
    }
    if (crop_steps) fail("Full crop moved across the BU fixed monitor-sync grid");
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

// A rejected early VS can carry several raw H pulses inside one admission
// window. Two short edges corroborate the burst before the provisional H
// becomes visible. The ordinary phase and period come from the wire history.
void test_full_ignores_corroborated_retrace_for_horizontal_lock() {
    constexpr unsigned period = 248;
    constexpr unsigned normal_h = 160;
    constexpr unsigned early_h = 132;
    constexpr unsigned h_lag = 8;
    constexpr unsigned pulse = 16;
    const auto check_h = [](const Sample& sample, unsigned tick,
                           unsigned rise, const char* label) {
        if (sample.hsync_selected != (tick >= rise && tick < rise + pulse)) {
            std::ostringstream out;
            out << label << " at CE " << tick << ", expected HS at " << rise;
            fail(out.str());
        }
    };

    // Only the early pulse shape changes. A late confirmation cannot
    // retract the original H. A single ordinary pulse still reacquires.
    struct Case {
        const char* label;
        unsigned first_width;
        unsigned second;
        unsigned selected_h;
        unsigned repeats;
    };
    for (const auto& test : {
            Case{"no corroboration", 4, 0, early_h, 1},
            Case{"late corroboration", 4, 16, early_h, 1},
            Case{"ordinary reanchor", 24, 0, early_h, 1},
            Case{"early corroboration", 4, 8, normal_h, 2}}) {
        CrtFilterBench tb;
        for (unsigned row = 0; row < 312; ++row)
            tb.run_line_ce(period, normal_h, 24, row < 16);
        for (unsigned epoch = 0; epoch < test.repeats; ++epoch) {
            // A fresh ordinary VS and settled raw H provide a learned 248-CE
            // grid. The injected VS follows only 100 lines later.
            for (unsigned row = 0; row < 100; ++row) {
                const auto line = tb.run_line_ce(period, normal_h, 24, row < 16);
                if (row >= 20)
                    for (unsigned tick = 0; tick < period; ++tick)
                        check_h(line[tick], tick, normal_h + h_lag, "established grid");
            }

            for (unsigned tick = 0; tick < period; ++tick) {
                const bool first = tick >= early_h && tick < early_h + test.first_width;
                const bool second = test.second && tick >= early_h + test.second &&
                                    tick < early_h + test.second + 4;
                const bool ordinary = tick >= normal_h && tick < normal_h + 24;
                const auto sample = tb.tick_ce(first || second || ordinary,
                                               tick >= early_h && tick < early_h + 32);
                check_h(sample, tick, test.selected_h + h_lag, test.label);
                if (sample.vsync_selected) fail("early retrace was selected as a vertical sync");
            }

            // The externally selected phase on every following raw line
            // pins both the restart decision and the full learned period.
            for (unsigned row = 0; row < 5; ++row) {
                const auto line = tb.run_line_ce(period, test.selected_h, 24);
                for (unsigned tick = 0; tick < period; ++tick)
                    check_h(line[tick], tick, test.selected_h + h_lag, test.label);
            }
            if (epoch + 1 < test.repeats)
                for (unsigned row = 106; row < 312; ++row)
                    tb.run_line_ce(period, normal_h, 24);
        }
    }
}

// SHAKER D3 0326 and 0331 supply an early 20/16-CE pulse, then
// the ordinary pulse at the usual origin. The stock ruler phase is
// stationary relative to the blue border through the split-sync band.
void test_full_keeps_byte_phase_after_split_sync() {
    constexpr unsigned normal_h = 160;
    constexpr unsigned early_fall = normal_h - 24;
    constexpr unsigned probe_start = normal_h + 64;

    struct Case {
        const char* label;
        unsigned width;
        unsigned primary_width;
        unsigned primary_offset;
        unsigned mode;
        bool retained;
    };
    for (const auto& test : {
             Case{"width-5 split", 20, 20, 0, 0, true},
             Case{"width-4 split", 16, 16, 0, 0, true},
             Case{"off-origin pulse", 20, 20, 1, 0, false},
             Case{"short primary", 16, 12, 0, 0, false},
             Case{"missing primary", 20, 0, 0, 0, false},
             Case{"Raw pixels", 20, 20, 0, 1, false},
             Case{"Raw CRT", 16, 16, 0, 2, false}}) {
        CrtFilterBench tb;
        tb.selector(test.mode, false, false, false, false);
        for (unsigned row = 0; row < 312; ++row)
            tb.run_line_ce(kLineCeTicks, normal_h, test.width, row < 16);

        for (unsigned row = 0; row < 100; ++row)
            tb.run_line_ce(kLineCeTicks, normal_h, test.width, row < 15);
        const auto baseline = tb.run_line_ce(kLineCeTicks, normal_h, test.width);
        if (!baseline[probe_start].shift || baseline[probe_start].hblank ||
            baseline[probe_start].vblank) fail("unqualified ordinary capture");

        for (unsigned row = 0; row < 4; ++row) {
            for (unsigned dot = 0; dot < kLineCeTicks; ++dot) {
                const bool extra = dot >= early_fall - test.width && dot < early_fall;
                const unsigned primary = normal_h + test.primary_offset;
                const bool original = dot >= primary && dot < primary + test.primary_width;
                const auto sample = tb.tick_ce(extra || original, false);
                if (sample.hsync_o != baseline[dot].hsync_o ||
                    sample.hblank != baseline[dot].hblank)
                    fail(std::string(test.label) + ": acquisition phase changed");
                if (dot == normal_h + 8 && sample.shift != baseline[dot].shift)
                    fail(std::string(test.label) + ": stale byte phase at the next ordinary HS strobe");
                if (dot >= probe_start && sample.shift != test.retained)
                    fail(std::string(test.label) + ": wrong byte phase in active display");
            }
        }
    }
}

// A valid split pulse arms Full recovery. Owners and mode requests can
// change on a master clock outside CE4, before any further normal HS strobe.
void test_full_discards_recovered_byte_phase_on_context_change() {
    constexpr unsigned normal_h = 160;
    constexpr unsigned early_fall = normal_h - 24;
    constexpr unsigned width = 20;
    struct Context {
        const char* label;
        bool eligible;
        bool clear;
        unsigned mode;
    };
    for (const auto& change : {
            Context{"classic owner change", true, true, 0},
            Context{"other timing owner", false, true, 0},
            Context{"owner disabled", false, false, 0},
            Context{"Raw pixels", true, false, 1},
            Context{"Raw CRT", true, false, 2}}) {
        CrtFilterBench tb;
        CrtFilterBench legacy;
        legacy.configure_byte_owner(false, false);
        for (unsigned row = 0; row < 68; ++row) {
            tb.run_line_ce(kLineCeTicks, normal_h, width, row < 16);
            legacy.run_line_ce(kLineCeTicks, normal_h, width, row < 16);
        }
        Sample recovered{}, plain{};
        for (unsigned dot = 0; dot <= normal_h + width; ++dot) {
            const bool early = dot >= early_fall - width && dot < early_fall;
            const bool primary = dot >= normal_h && dot < normal_h + width;
            recovered = tb.tick_ce(early || primary, false);
            plain = legacy.tick_ce(early || primary, false);
        }
        if (!recovered.shift || plain.shift)
            fail("fixture did not distinguish a confirmed recovery from ordinary SHIFT");

        tb.selector(change.mode, false, false, false, false);
        const auto switched = tb.configure_byte_owner(change.eligible, change.clear);
        if (switched.shift != plain.shift)
            fail(std::string(change.label) + ": stale phase before the next master edge");
        tb.tick_clk(false, false, false, false, false);
        legacy.tick_clk(false, false, false, false, false);
        tb.selector(0, false, false, false, false);
        const auto returned = tb.configure_byte_owner(true, false);
        if (returned.shift != plain.shift)
            fail(std::string(change.label) + ": stale phase after returning to Full");
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

    // B6 replaced Live blanking with Raw pixels, which keeps the complete
    // Full acquisition tuple and changes pixels only. Every raw input is
    // driven to the opposite of the filtered value here, so selecting any of
    // them would show. The independent B6 boundary regression scores what
    // that mode does to the bytes and the pixel mask.
    auto raw_pixels = tb.selector(1, !full.hsync_o, !full.vsync_o,
                                  !full.hblank, !full.vblank);
    if (raw_pixels.hsync_selected != raw_pixels.hsync_o ||
        raw_pixels.vsync_selected != raw_pixels.vsync_o ||
        raw_pixels.hblank_selected != raw_pixels.hblank ||
        raw_pixels.vblank_selected != raw_pixels.vblank)
        fail("Raw pixels mode did not select the complete filtered output tuple");

    auto raw_crt = tb.selector(2, !raw_pixels.hsync_o, !raw_pixels.vsync_o,
                               !raw_pixels.hblank, !raw_pixels.vblank);
    if (raw_crt.hsync_selected != static_cast<uint8_t>(!raw_pixels.hsync_o) ||
        raw_crt.vsync_selected != static_cast<uint8_t>(!raw_pixels.vsync_o) ||
        raw_crt.hblank_selected != static_cast<uint8_t>(!raw_pixels.hblank) ||
        raw_crt.vblank_selected != static_cast<uint8_t>(!raw_pixels.vblank))
        fail("Raw CRT mode did not select the complete raw output tuple");

    // Reserved value 3 is normalised to Full by the motherboard's applied-mode
    // register; the seam itself must not fall through to a raw member either.
    auto reserved = tb.selector(3, !raw_crt.hsync_o, !raw_crt.vsync_o,
                                !raw_crt.hblank, !raw_crt.vblank);
    if (reserved.hsync_selected != reserved.hsync_o ||
        reserved.vsync_selected != reserved.vsync_o ||
        reserved.hblank_selected != reserved.hblank ||
        reserved.vblank_selected != reserved.vblank)
        fail("reserved mode 3 did not fall back to the filtered output tuple");
}

void test_full_recovers_completed_short_vertical_pair() {
    constexpr unsigned kFrameTicks = 312 * kLineCeTicks;
    constexpr unsigned kFirstCheckpoint = 192;
    constexpr unsigned kRecoveredRise = kLineCeTicks + kFirstCheckpoint;
    constexpr unsigned kRecoveredFall = kRecoveredRise + kLineCeTicks;
    constexpr unsigned kSecondRawRise = 152 * kLineCeTicks;
    constexpr unsigned kNextPulseRise = kSecondRawRise + kFirstCheckpoint;
    constexpr unsigned kNextPulseFall = kSecondRawRise + kLineCeTicks;
    CrtFilterBench tb;

    // Train on two ordinary fields, let the next pair corroborate the
    // waveform, then observe the following complete pair in Full.
    std::vector<unsigned> rises, falls;
    bool prior = false;
    for (unsigned field = 0; field < 4; ++field) {
        for (unsigned tick = 0; tick < kFrameTicks; ++tick) {
            const unsigned hpos = tick % kLineCeTicks;
            const bool hs = hpos >= 184 && hpos < 208;
            const bool vs = field < 2 ? tick < 16 * kLineCeTicks
                : tick < kLineCeTicks || (tick >= kSecondRawRise && tick < kNextPulseFall);
            if (field == 3 && tick == kRecoveredFall + 1) {
                // The recovery was already visible and then completed.
                // The next owner must still get its ordinary pulse.
                tb.configure_byte_owner(true, true);
                tb.tick_clk(false, hs, vs, hs, vs);
                tb.configure_byte_owner(true, false);
            }
            const bool high = tb.tick_ce(hs, vs).vsync_selected;
            if (field == 3 && high != prior)
                (high ? rises : falls).push_back(tick);
            prior = high;
        }
    }
    if (rises != std::vector<unsigned>{kRecoveredRise, kNextPulseRise} ||
        falls != std::vector<unsigned>{kRecoveredFall, kNextPulseFall}) {
        std::ostringstream message;
        message << "expected both the recovered pulse and the next owner's ordinary pulse";
        for (auto tick : rises) message << " rise@" << tick;
        for (auto tick : falls) message << " fall@" << tick;
        fail(message.str());
    }
}

struct TestCase {
    const char* name;
    void (*run)();
};

constexpr std::array<TestCase, 16> kTests = {{
    {"Full HSYNC relocks an early step and releases before another VSYNC", test_full_relocks_early_phase_and_releases_without_vsync},
    {"Full keeps the short-pulse midpoint slot through a VSYNC anchor", test_full_keeps_midpoint_slot_through_vsync_anchor},
    {"Full crop preserves the BU fixed monitor-sync grid", test_full_preserves_bu_crop_phase},
    {"settled regenerated HSYNC and Full HBLANK baseline", test_full_mode_baseline},
    {"Live HBLANK preserves master phase and the +3-pixel R2.JIT shift", test_live_blank_master_phase_and_r2jit},
    {"raw force blank longer than the scaler minimum remains blank", test_long_raw_force_blank_wins},
    {"too-frequent raw sync does not restart the acquisition window", test_frequent_sync_does_not_restart_window},
    {"sync on the exact expiry clock starts a new Live window", test_expiry_clock_sync_starts_new_window},
    {"missing raw sync falls back to the established Full HBLANK", test_missing_sync_uses_full_blank},
    {"stuck-high raw sync without VSYNC falls back to Full HBLANK", test_stuck_high_without_vsync_uses_full_blank},
    {"masked short retrigger preserves the learned watchdog cadence", test_masked_retrigger_preserves_watchdog_period},
    {"Full horizontal lock survives corroborated early retrace", test_full_ignores_corroborated_retrace_for_horizontal_lock},
    {"Full completes a corroborated short vertical pair and retires it on switch", test_full_recovers_completed_short_vertical_pair},
    {"Full byte phase survives an on-origin ordinary split sync", test_full_keeps_byte_phase_after_split_sync},
    {"Full retires the recovered byte phase on a context change", test_full_discards_recovered_byte_phase_on_context_change},
    {"production selector pins Full, Raw pixels, Raw CRT and reserved tuples", test_production_mode_selector},
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
