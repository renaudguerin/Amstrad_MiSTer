// V1 consumer proof through the production colour/gamma/mixer/crop chain.
// The monitor-input history establishes the steady displacement independently
// of receiver state. FRAME_FIELD is observed at the motherboard's VGA_F1 wire.
#define B6_BOUNDARY_LIBRARY
#include "b6_video_boundary_test.cpp"

#include <cstdlib>
#include <fstream>
#include <set>

namespace {
constexpr unsigned kLineClocks = 4096;
constexpr unsigned kTimingDelay = 260;
// FRAME_FIELD bypasses the pixel consumer (Amstrad.sv .frame_field(VGA_F1)).
// Use the documented full-clock transport, including its edge convention.
// A source output changing AFTER edge t first reaches timing_delay[0] at
// t+1, timing_delay[256] at t+257 (crt_response.sv), and motherboard
// crt_tuple_alignment[3] at t+261. Harness samples are post-edge values.
// This all-clock contract is independent of final CE_PIXEL's sample phase.
// ASCAL's interpretation of this separate field input is a subsequent gate.
constexpr unsigned kFrameDelay = 261;
constexpr unsigned kCaptureClocks = 640 * kLineClocks;

struct Sample {
    unsigned rgb;
    unsigned timing;
    bool frame_field;
    bool ce;
};

Sample sample(const Harness& harness) {
    const auto& d = harness.dut;
    return {d.b6_mixer_rgb,
            unsigned((d.b6_mixer_hs << 2) | (d.b6_mixer_vs << 1) | d.b6_mixer_de),
            bool(d.b6_motherboard_frame), bool(d.b6_mixer_ce)};
}

// Frozen qualitative preset c343be86...79ac0, reference pipeline
// 299b3b20...9ea7, and sim/crt_response_test.cpp::receiver_controls establish
// offset = round((tip_width - 256) / 8) native dots when monitor phase and
// acquisition origin are stationary. These are preset values, not measured
// CTM calibration. Raw and V1 share downstream processing. The relative
// timing delay is 64 native dots plus the held renderer dot, or260 clocks.
// RGB delay is260 - 4*offset while final acquisition stays at260.
// FRAME_FIELD is independently scored at its direct full-clock boundary.
struct InputHistory {
    explicit InputHistory(unsigned expected_width) : width(expected_width) {}

    void observe(const Harness& harness) {
        const auto& d = harness.dut;
        const bool hs = d.b6_full_tuple & 8;
        const bool ga_hs = d.b6_raw_tuple & 8;
        const bool ga_vs = d.b6_raw_tuple & 4;
        if (d.dbg_reset || (d.b6_sync_observation & (1U << 15)))
            throw AuditFailure("steady consumer fixture lost its acquisition source");
        if (!sampled) {
            old_hs = hs;
            old_ga_hs = ga_hs;
            sampled = true;
            ++tick;
            return;
        }
        if (hs && !old_hs) {
            if (have_hs && tick - last_hs != kLineClocks)
                throw AuditFailure("post-entry acquisition origin moved");
            have_hs = true;
            last_hs = tick;
            ++lines;
        }
        if (ga_hs && !old_ga_hs) {
            rise = tick;
            have_rise = true;
            straddled_vs = ga_vs;
            rise_phase = have_hs ? (tick - last_hs) % kLineClocks : 0;
            rise_has_hs = have_hs;
        }
        straddled_vs |= ga_vs;
        if (!ga_hs && old_ga_hs && have_rise && !straddled_vs) {
            if (tick - rise != width)
                throw AuditFailure("measured monitor tip differs from frozen width case");
            if (rise_has_hs) {
                if (have_phase && rise_phase != nominal_phase)
                    throw AuditFailure("monitor phase moved against acquisition origin");
                nominal_phase = rise_phase;
                have_phase = true;
                ++qualified_tips;
            }
        }
        old_hs = hs;
        old_ga_hs = ga_hs;
        ++tick;
    }

    unsigned width, lines = 0, qualified_tips = 0;
    uint64_t tick = 0, last_hs = 0, rise = 0;
    unsigned nominal_phase = 0, rise_phase = 0;
    bool old_hs = false, old_ga_hs = false;
    bool sampled = false, have_hs = false, have_rise = false, have_phase = false;
    bool rise_has_hs = false, straddled_vs = false;
};

void initialize(Harness& harness, bool plus, unsigned width) {
    harness.verify_mutation(0);
    harness.dut.production_clocking = 1;
    harness.dut.b6_mode = 1;
    auto program = video_program(width);
    // This CPU write exercises physical interlace FIELD/VGA_F1 transitions.
    const std::vector<uint8_t> key{1,0,0xbc,0x3e,8,0xed,0x79,1,0,0xbd,0x3e};
    auto at = std::search(program.code.begin(), program.code.end(), key.begin(), key.end());
    if (at == program.code.end()) throw AuditFailure("R8 setup absent");
    at[key.size()] = 3;
    for (unsigned address = 0xc000; address < 0x10000; ++address) {
        harness.store(0, 0x20000 + address,
                      uint8_t(0x13 ^ ((address >> 1) & 0x7f) ^ ((address & 1) ? 0xe8 : 0)));
    }
    harness.initialize(program, plus);
    if (plus) {
        harness.download(build_cpr_image({{"cb00", program.code},
                                         {"cb03", std::vector<uint8_t>(16384, 0)}}));
        harness.wait_for_reset_release();
    }
}

void run_case(bool plus, unsigned width) {
    Harness raw(plus, 1), response(plus, 1);
    initialize(raw, plus, width);
    initialize(response, plus, width);
    if (raw.cycles != response.cycles) throw AuditFailure("source launch clocks differ");
    auto tick = [&]() { raw.tick(); response.tick(); };
    for (unsigned n = 0; n < 1800000; ++n) tick();

    // Begin both sources as Raw pixels, then request V1 through its normal
    // safe commit. This gives the steady-width controls a fresh receiver
    // epoch after boot's acquisition-origin changes. The old Plus boot-into-V1
    // fixture correctly retained a +4-dot ruler correction and was therefore
    // not a zero-offset control. Its external-input frozen replay is recorded
    // in the 2026-11-13 consumer investigation.
    response.dut.b6_mode = 3;
    unsigned waited = 0;
    while (response.dut.b6_applied != 3) {
        tick();
        if (++waited > 1600000) throw AuditFailure("V1 request missed its safe commit");
    }
    if (raw.dut.b6_applied != 1) throw AuditFailure("Raw pixels control changed mode");

    const unsigned tip_width = width == 6 ? 256 : (plus ? 128 : 133);
    const int offset = width == 6 ? 0 : (plus ? -16 : -15);
    const unsigned rgb_delay = unsigned(int(kTimingDelay) - 4 * offset);
    InputHistory input(tip_width);
    // Eight healthy priming lines plus64 relaxation steps bound the frozen
    // Q8 phase error below one LSB for these targets. A96-line epoch also
    // covers the incomplete first pulse and ordinary VS-inhibited tips.
    for (unsigned n = 0; n < 96 * kLineClocks; ++n) {
        input.observe(raw);
        tick();
    }
    if (input.qualified_tips < 80)
        throw AuditFailure("natural priming/settling input history was incomplete");

    std::ofstream trace;
    if (const char* directory = std::getenv("V1_CONSUMER_TRACE_DIR")) {
        const std::string path = std::string(directory) + "/owner" +
            std::to_string(plus) + "-width" + std::to_string(width) + ".csv";
        trace.open(path);
        if (!trace) throw AuditFailure("cannot open final public-pin trace");
        trace << "clock,CE_PIXEL,raw_RGB,v1_RGB,raw_HS,raw_VS,raw_DE,raw_FRAME_FIELD,"
                 "v1_HS,v1_VS,v1_DE,v1_FRAME_FIELD\n";
    }
    std::vector<Sample> raw_trace;
    raw_trace.reserve(kCaptureClocks);
    uint64_t checked = 0, frame_checks = 0, distinct_shift = 0, colored = 0;
    unsigned timing_edges = 0, frame_edges = 0;
    unsigned previous_ce = 0;
    bool have_ce = false;
    std::set<unsigned> colors;
    for (unsigned n = 0; n < kCaptureClocks; ++n) {
        if (raw.dut.b6_full_tuple != response.dut.b6_full_tuple ||
            raw.dut.b6_raw_tuple != response.dut.b6_raw_tuple)
            throw AuditFailure("presentation changed the monitor/acquisition input history");
        input.observe(raw);
        const auto native = sample(raw), got = sample(response);
        raw_trace.push_back(native);
        if (native.ce != got.ce) throw AuditFailure("final CE_PIXEL cadence differs");
        if (n >= kFrameDelay) {
            if (got.frame_field != raw_trace[n - kFrameDelay].frame_field)
                throw AuditFailure("motherboard FRAME_FIELD lost261-clock transport");
            ++frame_checks;
        }
        if (native.ce) {
            // Preserve the pins ASCAL receives, at the real final enable.
            // Exported clocks count completed Harness rising edges.
            if (trace) {
                trace << raw.cycles << ",1," << std::hex << native.rgb << ',' << got.rgb
                      << std::dec << ',' << ((native.timing >> 2) & 1)
                      << ',' << ((native.timing >> 1) & 1) << ',' << (native.timing & 1)
                      << ',' << native.frame_field << ',' << ((got.timing >> 2) & 1)
                      << ',' << ((got.timing >> 1) & 1) << ',' << (got.timing & 1)
                      << ',' << got.frame_field << '\n';
            }
            if (have_ce && n - previous_ce != 4)
                throw AuditFailure("final CE_PIXEL lost native one-in-four cadence");
            previous_ce = n;
            have_ce = true;
            if (n >= rgb_delay) {
                const auto& timing = raw_trace[n - kTimingDelay];
                const auto& pixel = raw_trace[n - rgb_delay];
                if (!pixel.ce || !timing.ce)
                    throw AuditFailure("expected public output sample missed native cadence");
                if (got.timing != timing.timing)
                    throw AuditFailure("final HS/VS/DE lost260-clock transport");
                if (got.rgb != pixel.rgb) {
                    std::cerr << "consumer owner=" << plus << " width=" << width
                              << " clock=" << n << " offset=" << offset
                              << " RGB=" << std::hex << got.rgb << "/" << pixel.rgb
                              << std::dec << '\n';
                    throw AuditFailure("final VGA RGB lost frozen-preset displacement");
                }
                timing_edges |= native.timing ^ raw_trace[n - 4].timing;
                frame_edges |= native.frame_field ^ raw_trace[n - 4].frame_field;
                distinct_shift += pixel.rgb != timing.rgb;
                colored += got.rgb != 0;
                colors.insert(got.rgb);
                ++checked;
            }
        }
        tick();
    }
    if (timing_edges != 7 || frame_edges != 1 || colored < 1000 || colors.size() < 3)
        throw AuditFailure("final output proof missed nonuniform pixels or timing/field edges");
    if (width == 4 && distinct_shift < 1000)
        throw AuditFailure("nonzero case did not demand displaced pixels");
    if (width == 6 && distinct_shift != 0)
        throw AuditFailure("reference-width case failed its zero-displacement control");
    std::cout << "PASS consumer owner=" << plus << " width=" << width
              << " measured_tip=" << tip_width << " offset=" << offset
              << " RGB_lag=" << rgb_delay << " timing_lag=" << kTimingDelay
              << " native_samples=" << checked << " frame_samples=" << frame_checks
              << " shifted_pixels=" << distinct_shift
              << " colors=" << colors.size() << " timing_edges=" << timing_edges
              << " frame_edges=" << frame_edges << " qualified_tips=" << input.qualified_tips
              << std::endl;
}
} // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    try {
        for (bool plus : {false, true}) {
            for (unsigned width : {6U, 4U}) run_case(plus, width);
        }
    } catch (const std::exception& error) {
        std::cerr << "B6 response " << error.what() << '\n';
        return 1;
    }
}
