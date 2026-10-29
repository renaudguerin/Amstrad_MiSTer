// Observe the public framebuffer metadata of the real ASCAL after replaying
// production CRTC/output pins. A short R8 pulse is not an interlaced frame;
// holding IVM must still deliver both alternating fields.
#include "Vvideo_field_test_top.h"
#include "verilated.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

struct Bench {
    Vvideo_field_test_top d;
    uint64_t ticks = 0;
    std::ofstream trace;
    std::ofstream raw_reference;
    unsigned raw_edges = 0;
    bool old_field = false;

    void tick() {
        d.clken = ticks % 64 == 0;
        d.nclken = ticks % 64 == 32;
        d.ce_16 = ticks % 4 == 0;
        d.clk = 0; d.eval();
        d.clk = 1; d.eval();
        if (trace.is_open() && d.ce) {
            trace << unsigned(d.hs) << ' ' << unsigned(d.vs) << ' '
                  << unsigned(d.field_out) << ' ' << unsigned(d.de) << '\n';
            if (raw_reference.is_open())
                raw_reference << unsigned(d.hs) << ' ' << unsigned(d.vs) << ' '
                              << unsigned(d.raw_field) << ' ' << unsigned(d.de) << '\n';
        }
        if (trace.is_open() && d.raw_field != old_field) ++raw_edges;
        old_field = d.raw_field;
        ++ticks;
    }
    void write(bool data, unsigned value) {
        d.enable = 1; d.rs = data; d.data = value; tick();
        d.enable = 0; d.eval();
    }
    void reg(unsigned n, unsigned value) { write(false, n); write(true, value); }
    void run(const std::filesystem::path& path, bool pulses, bool interlace, bool recovery,
             bool border) {
        d.reset_n = 0; d.crtc_type = 1; d.enable = 0; d.border = border;
        for (unsigned n = 0; n < 128; ++n) tick();
        // Small valid rasters keep the full ASCAL simulation inexpensive.
        // The contract is frame identity, independent of the raster dimensions.
        // Type 1 VSYNC lasts sixteen scanlines. Leave room for its blank
        // interval to end before the origin even when IVM halves the body.
        const unsigned regs[] = {7,4,6,0x11,23,0,10,12,0,3};
        for (unsigned r : {4,9,0,1,2,3,5,6,7,8}) reg(r, regs[r]);
        for (unsigned n = 0; n < 128; ++n) tick();
        d.reset_n = 1;
        if (interlace) reg(8,3);
        trace.open(path);
        if (interlace && !recovery)
            raw_reference.open(path.parent_path()/(path.stem().string()+"-raw.txt"));
        bool old_vs = false, pulsed = false, restored = false, exited = false;
        unsigned frames = 0, pulse_count = 0;
        for (unsigned n = 0; n < 1000000; ++n) {
            if (d.raw_vs && !old_vs) {
                ++frames; pulsed = false; restored = false;
            }
            old_vs = d.raw_vs;
            if (recovery && frames == 5 && !exited) {
                reg(8,0); exited = true;
            }
            if (pulses && ticks % 64 == 1 && d.c4 == 0 && d.c9 == 2) {
                if (d.c0 == 1 && !pulsed) { reg(8,3); pulsed = true; ++pulse_count; }
                if (d.c0 == 4 && pulsed && !restored) { reg(8,0); restored = true; }
            }
            tick();
            if (frames == 12) break;
        }
        if (frames != 12 || (pulses && (pulse_count < 7 || raw_edges < 4)))
            throw std::runtime_error("CRTC stimulus failed to produce the required frames/FIELD pulses");
        trace.close();
        std::cout << path.filename().string() << ": frames=" << frames
                  << " R8_pulses=" << pulse_count << " raw_FIELD_edges=" << raw_edges << '\n';
    }
};

int main(int argc, char** argv) {
    Verilated::commandArgs(argc,argv);
    try {
        if (argc != 2) throw std::runtime_error("usage: video_field_tests output-directory");
        std::filesystem::path dir(argv[1]);
        std::filesystem::create_directories(dir);
        for (bool border : {false, true})
          for (const auto& name : {"progressive", "pulse", "interlace", "recovery"}) {
            Bench b; b.run(dir / ((border ? "border-" : "") + std::string(name) + ".txt"),
                          std::string(name) == "pulse", std::string(name) == "interlace" ||
                          std::string(name) == "recovery", std::string(name) == "recovery", border);
        }
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
