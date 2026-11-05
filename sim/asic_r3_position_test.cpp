// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND 4.0), French v1.11 §14.4 pp134-135.
// Ordinary R3l4->5 moves the whole rendered line LEFT8 Mode2 dots.
// Full5/6 hashes preserve the old source, not an analogue timing oracle.
#include "Vasic_r3_position_top.h"
#include "verilated.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool ok, const std::string& why) {
    if (!ok) throw std::runtime_error(why);
}
uint8_t marker(unsigned address) {
    return uint8_t((address * 73u + (address >> 7) * 29u) ^ 0xa6u);
}
struct Guest {
    std::vector<uint8_t> bytes = std::vector<uint8_t>(16384, 0xff);
    unsigned idle = 0;
    explicit Guest(unsigned width) {
        unsigned address = 0x100;
        auto emit = [&](unsigned value) { bytes.at(address++) = value; };
        auto out = [&](unsigned port, unsigned value) {
            emit(1); emit(port); emit(port >> 8); // LD BC,port
            emit(0x3e); emit(value);             // LD A,value
            emit(0xed); emit(0x79);              // OUT (C),A
        };
        bytes[0] = 0xc3; bytes[1] = 0; bytes[2] = 1;
        emit(0xf3); emit(0x31); emit(0xf0); emit(0xbf);
        const unsigned regs[] = {63,40,50,width,38,0,25,30,0,7,0,0,48,0};
        for (unsigned reg = 0; reg < 14; ++reg) {
            out(0xbc00, reg); out(0xbd00, regs[reg]);
        }
        out(0x7f00, 0); out(0x7f00, 0x4b);  // pen0 white
        out(0x7f00, 1); out(0x7f00, 0x4c);  // pen1 red
        out(0x7f00, 16); out(0x7f00, 0x52); // green border differs from both
        out(0x7f00, 0x82);                  // Mode2
        idle = address; emit(0x18); emit(0xfe);
    }
};
// Read-only observations also form optional full input/source/context receipts.
#define SAMPLE_FIELDS(X) \
    X(c0) X(c4) X(c9) X(ce16) X(ce4) X(public_rgb) X(source_rgb) \
    X(raw_hs) X(raw_vs) X(raw_de) X(eff_de) X(blank) \
    X(selected_hs) X(selected_hb) X(selected_vb) \
    X(filtered_hs) X(filtered_vs) X(filtered_hb) X(filtered_vb) \
    X(filter_start) X(filter_end) X(filter_extra) X(filter_nohs) \
    X(filter_shift) X(filter_size) X(filter_applied) \
    X(mode_requested) X(mode_effective) X(pen) X(palette_addr) X(palette_rgb) \
    X(sprite_en) X(ready) X(crtc_write) X(crtc_addr) X(bus_data) \
    X(video_addr) X(video_byte) X(filter_observation)
struct Sample {
#define DECLARE(name) unsigned name = 0;
    SAMPLE_FIELDS(DECLARE)
#undef DECLARE
    unsigned video_word = 0;
};
struct Bench {
    Vasic_r3_position_top dut;
    Guest guest;
    unsigned width;
    std::vector<uint8_t> ram = std::vector<uint8_t>(0x800000, 0xff);
    std::ofstream ledger;
    uint64_t ticks = 0;
    unsigned frames = 0, writes = 0, readyLow = 0, complete = 0;
    bool oldInsn = false, armed = false, leftOrigin = false, oldWrite = false;
    bool oldHS = false, rise = false, fall = false, oldEnd = false;

    Bench(unsigned width_, const std::string& directory) : guest(width_), width(width_) {
        // Plus bypasses the classic ROM decoder. B18 has no cartridge overlay;
        // the original guest executes from base RAM at logical PC0.
        std::copy(guest.bytes.begin(), guest.bytes.end(), ram.begin() + 0x20000);
        for (unsigned a = 0xc000; a < 0x10000; ++a) ram[0x20000 + a] = marker(a);
        if (!directory.empty()) {
            ledger.open(directory + "/width" + std::to_string(width) + "-events.csv");
            require(bool(ledger), "cannot open capture event ledger");
            ledger << "tick,event,c0,c4,c9,crtc_addr,bus_data,r3,filter_start,filter_end,extra,"
                      "raw_hs,selected_hs,selected_hb,selected_vb,filter_size,nohs,frames\n";
            std::ofstream rom(directory + "/guest-width" + std::to_string(width) + ".rom", std::ios::binary);
            rom.write(reinterpret_cast<const char*>(guest.bytes.data()), guest.bytes.size());
            std::ofstream vram(directory + "/marker-vram.bin", std::ios::binary);
            vram.write(reinterpret_cast<const char*>(ram.data() + 0x2c000), 16384);
            require(bool(rom) && bool(vram), "cannot write original guest/marker inputs");
        }
        dut.reset_btn = 1; dut.sna_download = 0; dut.load_wr = 0; dut.filter_request = 0;
        dut.load_addr = 0; dut.load_data = 0;
    }
    void event(const char* name) {
        if (!ledger.is_open()) return;
        ledger << ticks << ',' << name << ',' << unsigned(dut.c0) << ',' << unsigned(dut.c4)
               << ',' << unsigned(dut.c9) << ',' << unsigned(dut.crtc_addr)
               << ',' << unsigned(dut.bus_data) << ',' << unsigned(dut.r3)
               << ',' << unsigned(dut.filter_start) << ',' << unsigned(dut.filter_end)
               << ',' << unsigned(dut.filter_extra) << ',' << unsigned(dut.raw_hs)
               << ',' << unsigned(dut.selected_hs) << ',' << unsigned(dut.selected_hb)
               << ',' << unsigned(dut.selected_vb) << ',' << unsigned(dut.filter_size)
               << ',' << unsigned(dut.filter_nohs) << ',' << frames << '\n';
    }
    Sample tick() {
        dut.clk = 0; dut.eval(); dut.mem_din = ram.at(dut.mem_addr);
        const unsigned address = 0x20000 + unsigned(dut.video_addr) * 2;
        dut.video_word = ram.at(address) | (unsigned(ram.at(address + 1)) << 8);
        dut.eval();
        if (!dut.owner_reset) {
            if (dut.cpu_insn_start && !oldInsn && dut.cpu_addr == guest.idle && !armed) {
                armed = true; frames = 0; leftOrigin = false; event("idle");
            }
            if (dut.crtc_write && !oldWrite) {
                event("CPU_OUT_CRTC");
                if (dut.crtc_addr == 3) {
                    require(dut.bus_data == width, "actual CPU R3 data"); ++writes;
                }
            }
            if (armed) {
                if (dut.c4 || dut.c9) leftOrigin = true;
                if (leftOrigin && !dut.c4 && !dut.c9) {
                    ++frames; leftOrigin = false; event("natural_origin");
                }
            }
            if (!oldHS && dut.raw_hs) { rise = true; fall = false; }
            if (oldHS && !dut.raw_hs && rise) fall = true;
            if (dut.filter_end != oldEnd && fall) { ++complete; rise = fall = false; }
        }
        oldInsn = dut.cpu_insn_start; oldWrite = dut.crtc_write;
        oldHS = dut.raw_hs; oldEnd = dut.filter_end; readyLow += !dut.ready;
        Sample sample;
#define COPY(name) sample.name = dut.name;
        SAMPLE_FIELDS(COPY)
#undef COPY
        sample.video_word = dut.video_word;
        // owner_reset is registered; the initial reset_btn edge must not
        // retire a spurious backend write over the guest's entry opcode.
        if (dut.mem_wr && !dut.owner_reset && !dut.reset_btn) ram.at(dut.mem_addr) = dut.mem_dout;
        if (dut.load_mem_wr) ram.at(dut.load_mem_addr) = dut.load_data;
        dut.clk = 1; dut.eval(); dut.clk = 0; dut.eval(); ++ticks;
        return sample;
    }
    std::vector<Sample> capture() {
        for (unsigned i = 0; i < 1024; ++i) tick();
        dut.reset_btn = 0;
        return body();
    }
    std::vector<Sample> body() {
        bool found = false;
        for (unsigned i = 0; i < 12000000; ++i) {
            if (armed && frames >= 3 && dut.c4 == 6 && dut.c9 == 0 && dut.c0 == 0) {
                found = true; break;
            }
            tick();
        }
        require(found, "bounded natural frame/body capture timed out");
        require(writes == 1 && dut.r0 == 63 && dut.r1 == 40 && dut.r2 == 50 && dut.r3 == width,
                "actual CPU nominal CRTC registers");
        require(dut.filter_applied == 0 && dut.filter_size == 256 && !dut.filter_nohs && complete > 10,
                "Full natural acquisition qualification");
        require(dut.mode_requested == 2 && dut.mode_effective == 2 && readyLow > 10,
                "production CPU/READY/Mode2 qualification");
        event("capture");
        std::vector<Sample> samples; samples.reserve(8192);
        for (unsigned i = 0; i < 8192; ++i) samples.push_back(tick());
        std::cout << "width" << width << " qualified: ticks=" << ticks << " natural_origins=" << frames
                  << " CPU_R3_writes=" << writes << " complete_pulses=" << complete
                  << " READY_low=" << readyLow << '\n';
        return samples;
    }
    void mode(unsigned requested) {
        dut.filter_request = requested;
        for (unsigned i = 0; i < 3000000 && dut.filter_applied != requested; ++i) tick();
        require(dut.filter_applied == requested, "actual filter request did not commit");
        event("filter_commit_observed");
    }
    void raw_modes() {
        for (unsigned requested : {1u, 2u}) {
            mode(requested);
            unsigned nonzero = 0;
            for (unsigned i = 0; i < 8192; ++i) {
                const auto sample = tick();
                require(sample.filter_applied == requested && sample.public_rgb == sample.source_rgb,
                        "non-Full public 12-bit RGB must directly follow ASIC source");
                nonzero += sample.ce16 && sample.source_rgb != 0;
            }
            require(nonzero > 100, "raw-mode direct-RGB control must see visible source pixels");
            std::cout << "actual filter mode" << requested << " direct12-bitRGB8192 nonzeroDots=" << nonzero << '\n';
        }
        mode(0);
        // Observation-only frame qualification; no machine state or CPU release changes.
        frames = 0; leftOrigin = false;
    }
};
void dump(const std::vector<Sample>& samples, const std::string& path) {
    std::ofstream stream(path);
    require(bool(stream), "cannot open complete capture");
#define HEADER(name) stream << #name << ',';
    SAMPLE_FIELDS(HEADER)
#undef HEADER
    stream << "video_word\n";
    for (const auto& sample : samples) {
#define VALUE(name) stream << sample.name << ',';
        SAMPLE_FIELDS(VALUE)
#undef VALUE
        stream << sample.video_word << '\n';
    }
}
uint64_t rgb_hash(const std::vector<Sample>& samples, bool source) {
    uint64_t hash = 14695981039346656037ull;
    for (const auto& sample : samples) {
        const unsigned rgb = source ? sample.source_rgb : sample.public_rgb;
        for (unsigned byte = 0; byte < 4; ++byte) {
            hash ^= (rgb >> (8 * byte)) & 255; hash *= 1099511628211ull;
        }
    }
    return hash;
}
// Original-source capture, FNV1a over all8192 uint32-LE 12-bit RGB values.
// Frozen private qualifying run4 at c0939d8, before the RGB-owner change.
// Source hashes protect the upstream path; public5/6 retain the baseline.
// These are regression anchors, not expected analogue/monitor behavior.
constexpr std::array<uint64_t, 3> original_rgb = {
    0x8dec7e0eae46dab5ull, 0x9aff0de4e72aa2b5ull, 0x5c53983a7b15020dull
};
void nonvacuous(const std::vector<Sample>& samples, unsigned width) {
    unsigned sourceDiff = 0, white = 0, red = 0, green = 0, visible = 0;
    unsigned rawRise = 0, rawFall = 0, selectedRise = 0;
    for (unsigned i = 0; i < samples.size(); ++i) {
        const auto& s = samples[i]; sourceDiff += s.public_rgb != s.source_rgb;
        require(!s.sprite_en, "unexpected sprite obscures marker proof");
        if (s.ce16 && !s.selected_hb && !s.selected_vb) {
            ++visible; white += s.public_rgb == 0xfff;
            red += s.public_rgb == 0xf00; green += s.public_rgb == 0x0f0;
        }
        if (i) {
            rawRise += s.raw_hs && !samples[i-1].raw_hs;
            rawFall += !s.raw_hs && samples[i-1].raw_hs;
            selectedRise += s.selected_hs && !samples[i-1].selected_hs;
        }
    }
    require(white > 50 && red > 50 && green > 20 && rawRise == 2 && rawFall == 2 && selectedRise == 2,
            "complete 12-bit RGB/border/sync nonvacuity");
    std::vector<unsigned> expected;
    for (unsigned byte = 0; byte < 6; ++byte) {
        const unsigned value = marker(0xc000 + 6*80 + 20 + byte);
        for (unsigned bit = 0; bit < 8; ++bit) expected.push_back(value & (128 >> bit) ? 0xf00 : 0xfff);
    }
    std::vector<unsigned> dots;
    for (const auto& s : samples) if (s.ce16 && s.c9 == 0) dots.push_back(s.source_rgb);
    unsigned matches = 0, origin = 0;
    for (unsigned i = 0; i + expected.size() <= dots.size(); ++i) {
        if (std::equal(expected.begin(), expected.end(), dots.begin() + i)) { ++matches; origin = i; }
    }
    require(matches == 1, "independent complete six-byte Mode2 bitmap origin not unique");
    require(rgb_hash(samples, true) == original_rgb.at(width - 4), "upstream ASIC source RGB anchor changed");
    if (width >= 5)
        require(rgb_hash(samples, false) == original_rgb.at(width - 4), "old public Full5/6 RGB anchor changed");
    std::cout << "width" << width << " nonvacuity: public/source differences=" << sourceDiff
              << " visibleDots=" << visible << " white/red/green=" << white << '/' << red << '/' << green
              << " marker48_dot_origin=" << origin << " rawRises/Falls=" << rawRise << '/' << rawFall
              << " selectedHS_rises=" << selectedRise << '\n';
}
unsigned edge(const std::vector<Sample>& samples, unsigned raster, bool entry) {
    bool sawBefore = false;
    unsigned result = ~0u;
    for (unsigned i = 0; i < samples.size(); ++i) {
        const auto& s = samples[i];
        if (!s.ce16 || s.c9 != raster || s.c0 < (entry ? 0 : 38) || s.c0 > (entry ? 3 : 43)) continue;
        require(s.public_rgb == 0x0f0 || s.public_rgb == 0xfff || s.public_rgb == 0xf00,
                "unexpected edge RGB masks bitmap/border");
        const bool green = s.public_rgb == 0x0f0;
        if (entry ? green : !green) sawBefore = true;
        if (sawBefore && (entry ? !green : green) && result == ~0u) result = i;
    }
    require(result != ~0u, "visible complete border/bitmap transition absent");
    return result;
}
void position(const std::vector<Sample>& four, const std::vector<Sample>& five) {
        unsigned mismatches = 0, checked = 0, identical = 0, first = ~0u;
        // French §14.4 pp134-135: increasing4->5 moves the rendered line
        // LEFT8 M2 dots (32 master clocks), independent of bitmap contents.
        for (unsigned i = 0; i + 32 < four.size(); ++i) {
            if (!four[i].ce16 || four[i].c0 < 4 || four[i].c0 > 38 || four[i+32].c0 > 38) continue;
            require(four[i].c0 == five[i].c0 && four[i].c4 == five[i].c4 && four[i].c9 == five[i].c9 &&
                    four[i].video_word == five[i].video_word && four[i].video_addr == five[i].video_addr,
                    "unmatched upstream/beam input");
            require(four[i].selected_hs == five[i].selected_hs && four[i].selected_hb == five[i].selected_hb &&
                    four[i].selected_vb == five[i].selected_vb, "acquisition contexts differ; coordinates invalid");
            ++checked; identical += four[i].public_rgb == five[i].public_rgb;
            if (five[i].public_rgb != four[i+32].public_rgb) {
                ++mismatches; if (first == ~0u) first = i;
            }
        }
        require(checked > 1000, "nonvacuous complete fixed bitmap window");
        std::cout << "French14.4 LEFT8 check: samples=" << checked << " mismatches=" << mismatches
                  << " sameCoordinateEqual=" << identical << " firstMismatchMasterClock=" << first << '\n';
        unsigned edgeFailures = 0;
        for (unsigned raster : {0u,1u}) for (bool entry : {true,false}) {
            const unsigned x = edge(four, raster, entry), y = edge(five, raster, entry);
            const int delta = int(x) - int(y);
            std::cout << "visible " << (entry ? "entry" : "exit") << " raster" << raster
                      << " width4=" << x << " width5=" << y << " leftDeltaMasterClocks=" << delta
                      << " expected=32\n";
            edgeFailures += delta != 32;
        }
        require(!mismatches && !edgeFailures, "French ordinary4/5 whole-line LEFT8 missing (bitmap and border)");
}
} // namespace
int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    const std::string directory = argc > 1 ? argv[1] : "";
    try {
        std::array<std::vector<Sample>, 3> captures;
        std::vector<Sample> returnedFull;
        for (unsigned width = 4; width <= 6; ++width) {
            Bench bench(width, directory); captures[width-4] = bench.capture();
            if (!directory.empty()) dump(captures[width-4], directory + "/width" + std::to_string(width) + "-capture.csv");
            nonvacuous(captures[width-4], width);
            if (width == 4) {
                bench.raw_modes(); returnedFull = bench.body();
                if (!directory.empty()) dump(returnedFull, directory + "/width4-return-capture.csv");
            }
        }
        position(captures[0], captures[1]);
        nonvacuous(returnedFull, 4);
        position(returnedFull, captures[1]);
        std::cout << "asic-r3-position: PASS Full4/5 LEFT8, four visible border edges, source/old5/6 anchors, raw modes and Full return\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "asic-r3-position: FAIL " << error.what() << '\n'; return 1;
    }
}
