// V1 consumer proof through the production colour/gamma/mixer/crop chain.
// The monitor-input history establishes the steady displacement independently
// of receiver state. FRAME_FIELD is observed at the motherboard's VGA_F1 wire.
#define B6_BOUNDARY_LIBRARY
#include "b6_video_boundary_test.cpp"

#include <cstdlib>
#include <fstream>
#include <memory>
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
constexpr unsigned kBridgeDelay = 3;
constexpr unsigned kAscalCaptureClocks = 1280 * kLineClocks;

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

bool same_sample(const Sample& a, const Sample& b) {
    return a.rgb == b.rgb && a.timing == b.timing &&
           a.frame_field == b.frame_field && a.ce == b.ce;
}

Sample sample_ascal(const Harness& harness) {
    const auto& d = harness.dut;
    return {d.b6_ascal_rgb,
            unsigned((d.b6_ascal_hs << 2) | (d.b6_ascal_vs << 1) | d.b6_ascal_de),
            bool(d.b6_motherboard_frame), bool(d.b6_ascal_ce)};
}

void write_ascal_sample(std::ostream& stream, const Sample& value) {
    stream << ((value.timing >> 2) & 1) << ' ' << ((value.timing >> 1) & 1)
           << ' ' << value.frame_field << ' ' << (value.timing & 1)
           << ' ' << value.ce << ' ' << ((value.rgb >> 16) & 255)
           << ' ' << ((value.rgb >> 8) & 255) << ' ' << (value.rgb & 255) << '\n';
}

// Only reference CE/VS/DE chronology selects the exported cycle. Every
// master-clock record inside it is retained, including CE=0 and the closing
// VS boundary. Repeating its half-open alternating-field cycle lets ASCAL acquire without fabricating
// active pixels or assigning field rows from actual Avalon writeback.
class AscalTrace {
public:
    AscalTrace(const std::string& directory, bool plus, unsigned width,
               unsigned rgb_delay, unsigned source_r9, uint64_t capture_clock)
        : base_(directory + "/owner" + std::to_string(plus) + "-width" +
                std::to_string(width)), rgb_delay_(rgb_delay), source_r9_(source_r9), capture_clock_(capture_clock) {
        actual_.open(base_ + "-actual.txt");
        reference_.open(base_ + "-reference.txt");
        if (!actual_ || !reference_) throw AuditFailure("cannot open paired ASCAL traces");
    }

    void observe(unsigned index, const Sample& actual, const Sample& reference) {
        const bool vs = reference.timing & 2;
        if (reference.ce) {
            const bool rise = seen_ce_ && vs && !old_vs_;
            seen_ce_ = true;
            old_vs_ = vs;
            if (rise && !finished_) {
                if (!started_) {
                    started_ = true;
                    start_ = index;
                    first_field_ = reference.frame_field;
                    current_field_ = first_field_;
                } else {
                    if (old_de_ || row_width_ || !rows_)
                        throw AuditFailure("reference VS did not close a complete visible field");
                    field_rows_.push_back(rows_);
                    field_tags_.push_back(current_field_);
                    rows_ = 0;
                    if (field_rows_.size() == 2) {
                        if (reference.frame_field != first_field_ ||
                            field_tags_[0] == field_tags_[1])
                            throw AuditFailure("reference cycle did not alternate both fields");
                        end_ = index;
                        finished_ = true;
                    } else {
                        if (reference.frame_field == current_field_)
                            throw AuditFailure("consecutive reference fields did not alternate");
                        current_field_ = reference.frame_field;
                    }
                }
            }
        }
        if (!started_ || (finished_ && index != end_)) return;
        write_ascal_sample(actual_, actual);
        write_ascal_sample(reference_, reference);
        ++records_;
        if (reference.ce) {
            const bool de = reference.timing & 1;
            if (de) ++row_width_;
            if (!de && old_de_) {
                widths_.insert(row_width_);
                row_width_ = 0;
                ++rows_;
            }
            old_de_ = de;
        }
    }

    void finish(unsigned capture_clocks) {
        if (!finished_ || records_ != end_ - start_ + 1 || widths_.size() != 1)
            throw AuditFailure("bounded ASCAL capture missed a complete reference cycle");
        actual_.close();
        reference_.close();
        std::ofstream metadata(base_ + "-metadata.json");
        if (!metadata) throw AuditFailure("cannot open ASCAL trace metadata");
        metadata << "{\n  \"format\": \"hs vs f1 de ce r g b, decimal per master clock\",\n"
                 << "  \"master_clock_period_ps\": 15625,\n"
                 << "  \"native_clock_ratio\": 4,\n"
                 << "  \"source_r4\": 38,\n"
                 << "  \"source_r8\": 3,\n"
                 << "  \"source_r9\": " << source_r9_ << ",\n"
                 << "  \"capture_start_clock\": " << capture_clock_ << ",\n"
                 << "  \"capture_master_clocks\": " << capture_clocks << ",\n"
                 << "  \"trim_start_index\": " << start_ << ",\n"
                 << "  \"trim_end_index_exclusive\": " << end_ + 1 << ",\n"
                 << "  \"repeat_period_master_clocks\": " << end_ - start_ << ",\n"
                 << "  \"terminal_boundary_records\": 1,\n"
                 << "  \"records\": " << records_ << ",\n"
                 << "  \"timing_lag_before_bridge\": " << kTimingDelay << ",\n"
                 << "  \"rgb_lag_before_bridge\": " << rgb_delay_ << ",\n"
                 << "  \"bridge_delay\": " << kBridgeDelay << ",\n"
                 << "  \"frame_field_lag\": " << kFrameDelay << ",\n"
                 << "  \"hs_vs_polarity\": \"positive, independently checked against Raw final pins\",\n"
                 << "  \"reference_engine_sha256\": \"299b3b205ddbe327af43bcea6b3f1106e1c04d823f00ffd836b25925c61b9ea7\",\n"
                 << "  \"preset_sha256\": \"c343be8623db7bd154de9f6ca087996c97842240861277a101795cf88fd79ac0\",\n"
                 << "  \"field_tags\": [" << field_tags_[0] << ", " << field_tags_[1] << "],\n"
                 << "  \"active_rows_per_field\": [" << field_rows_[0] << ", " << field_rows_[1] << "],\n"
                 << "  \"active_row_width\": " << *widths_.begin() << ",\n"
                 << "  \"record_edge\": \"post-producer-edge, sampled by next ASCAL input edge\"\n}\n";
        std::cout << "EXPORT ASCAL " << base_ << " records=" << records_
                  << " trim=" << start_ << ".." << end_ << " rows="
                  << field_rows_[0] << "/" << field_rows_[1] << std::endl;
    }

private:
    std::string base_;
    std::ofstream actual_, reference_;
    unsigned rgb_delay_, source_r9_, start_ = 0, end_ = 0, records_ = 0;
    uint64_t capture_clock_;
    bool seen_ce_ = false, old_vs_ = false, old_de_ = false;
    bool started_ = false, finished_ = false, first_field_ = false, current_field_ = false;
    unsigned rows_ = 0, row_width_ = 0;
    std::vector<unsigned> field_rows_, field_tags_;
    std::set<unsigned> widths_;
};

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

void initialize(Harness& harness, bool plus, unsigned width, unsigned source_r9) {
    harness.verify_mutation(0);
    harness.dut.production_clocking = 1;
    harness.dut.b6_mode = 1;
    auto program = video_program(width);
    // This CPU write exercises physical interlace FIELD/VGA_F1 transitions.
    const std::vector<uint8_t> key{1,0,0xbc,0x3e,8,0xed,0x79,1,0,0xbd,0x3e};
    auto at = std::search(program.code.begin(), program.code.end(), key.begin(), key.end());
    if (at == program.code.end()) throw AuditFailure("R8 setup absent");
    at[key.size()] = 3;
    // FR ACCC v1.11 §19.4.2 p204 uses N-1 for CRTC1 IVM, whereas
    // §19.4.4 p205 uses N-2 for CRTC3. The ASCAL export chooses N=16,
    // yielding8 rasters/C4 and39*8=312 body lines per field. The default
    // stage1 R9=7 remains its original short-field transport fixture.
    auto r9_key = key;
    r9_key[4] = 9;
    at = std::search(program.code.begin(), program.code.end(), r9_key.begin(), r9_key.end());
    if (at == program.code.end()) throw AuditFailure("R9 setup absent");
    at[r9_key.size()] = uint8_t(source_r9);
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
    const char* ascal_directory = std::getenv("V1_ASCAL_TRACE_DIR");
    const unsigned source_r9 = ascal_directory ? (plus ? 14 : 15) : 7;
    Harness raw(plus, 1), response(plus, 1);
    initialize(raw, plus, width, source_r9);
    initialize(response, plus, width, source_r9);
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
    const unsigned capture_clocks = ascal_directory ? kAscalCaptureClocks : kCaptureClocks;
    std::unique_ptr<AscalTrace> ascal_trace;
    if (ascal_directory)
        ascal_trace = std::make_unique<AscalTrace>(ascal_directory, plus, width,
                                                 rgb_delay, source_r9, raw.cycles);
    std::vector<Sample> raw_trace;
    raw_trace.reserve(capture_clocks);
    uint64_t checked = 0, frame_checks = 0, distinct_shift = 0, colored = 0;
    unsigned timing_edges = 0, frame_edges = 0;
    unsigned previous_ce = 0;
    bool have_ce = false;
    std::set<unsigned> colors;
    for (unsigned n = 0; n < capture_clocks; ++n) {
        if (raw.dut.b6_full_tuple != response.dut.b6_full_tuple ||
            raw.dut.b6_raw_tuple != response.dut.b6_raw_tuple)
            throw AuditFailure("presentation changed the monitor/acquisition input history");
        input.observe(raw);
        const auto native = sample(raw), got = sample(response);
        raw_trace.push_back(native);
        if (native.ce != got.ce) throw AuditFailure("final CE_PIXEL cadence differs");
        if (n >= rgb_delay + kBridgeDelay) {
            const auto raw_bridge = sample_ascal(raw);
            const auto actual = sample_ascal(response);
            Sample raw_reference = raw_trace[n - kBridgeDelay];
            raw_reference.rgb = (raw_reference.timing & 1) ? raw_reference.rgb : 0;
            raw_reference.frame_field = native.frame_field;
            if (!same_sample(raw_bridge, raw_reference))
                throw AuditFailure("production ASCAL bridge differs from Raw pins/three-clock mask");
            // Mask with timing DE, independently of the displaced source RGB.
            // FRAME_FIELD follows its actual bypass, without the bridge delay.
            const auto& timing = raw_trace[n - kTimingDelay - kBridgeDelay];
            Sample reference = timing;
            reference.rgb = (timing.timing & 1) ?
                raw_trace[n - rgb_delay - kBridgeDelay].rgb : 0;
            reference.ce = raw_trace[n - kBridgeDelay].ce;
            reference.frame_field = raw_trace[n - kFrameDelay].frame_field;
            if (!same_sample(actual, reference)) {
                std::cerr << "ASCAL input owner=" << plus << " width=" << width
                          << " clock=" << n << " RGB=" << actual.rgb << "/" << reference.rgb
                          << " timing=" << actual.timing << "/" << reference.timing
                          << " CE=" << actual.ce << "/" << reference.ce
                          << " F1=" << actual.frame_field << "/" << reference.frame_field
                          << std::endl;
                throw AuditFailure("actual ASCAL input differs from independent Raw-derived reference");
            }
            if (ascal_trace) ascal_trace->observe(n, actual, reference);
        }
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
    if (ascal_trace) ascal_trace->finish(capture_clocks);
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
