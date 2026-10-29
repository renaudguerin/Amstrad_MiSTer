#include <verilated.h>

#include "Vga40010_test.h"

#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Bench {
public:
    explicit Bench(bool crtc_type)
        : context_(std::make_unique<VerilatedContext>()),
          dut_(std::make_unique<Vga40010_test>(context_.get())) {
        dut_->clk = 0;
        dut_->RESET_N = 0;
        dut_->A = 0;
        dut_->RAM_DIN = 0;
        dut_->CPU_DIN = 0;
        dut_->MREQ_N = 1;
        dut_->M1_N = 1;
        dut_->RD_N = 1;
        dut_->WR_N = 1;
        dut_->IORQ_N = 1;
        dut_->CRTC_TYPE = crtc_type;
        eval();
        for (unsigned cycle = 0; cycle < 8; ++cycle) {
            clock();
        }
        dut_->RESET_N = 1;
        for (unsigned cycle = 0; cycle < 16; ++cycle) {
            clock();
        }
    }

    void configure(std::uint8_t r2, std::uint8_t r3 = 0x84) {
        // A short, stable raster with the sync position well inside display.
        write_crtc(0, 63);
        write_crtc(1, 56);
        write_crtc(2, r2);
        write_crtc(3, r3);
        write_crtc(4, 3);
        write_crtc(5, 0);
        write_crtc(6, 3);
        write_crtc(7, 2);
        write_crtc(8, 0);
        write_crtc(9, 7);
        write_crtc(12, 0x30);
        write_crtc(13, 0);
        write_io(0x7fff, 0x00);  // select ink 0
        write_io(0x7fff, 0x44);  // non-black ink 0
        // Mode 2 makes each cen_16 sample one documented pixel-M2.
        write_io(0x7fff, 0x82);
        // Let any pulse started while the register file was being programmed
        // retire, then begin only from a complete configured scanline.
        wait_for_c0(0);
        wait_for("displayed character 32", [&] {
            return dut_->CRTC_C0 == 32 && dut_->CRTC_DE_RAW;
        });
        wait_for("non-black displayed pixels", [&] {
            return dut_->CRTC_DE_RAW && !blank();
        });
    }

    void select_crtc_register(std::uint8_t reg) {
        write_io(0xbc00, reg);
    }

    void write_selected(std::uint8_t value) {
        write_io(0xbd00, value);
    }

    void wait_for_c0(std::uint8_t c0) {
        bool left_target = dut_->CRTC_C0 != c0;
        wait_for("fresh C0=" + std::to_string(c0), [&] {
            if (dut_->CRTC_C0 != c0) {
                left_target = true;
            }
            return left_target && dut_->CRTC_C0 == c0;
        });
    }

    void arm_r2_write_monitor() {
        r2_monitor_armed_ = true;
        old_r2_ = dut_->CRTC_R2;
        r2_write_seen_ = false;
    }

    bool r2_write_seen() const { return r2_write_seen_; }
    unsigned r2_write_c0() const { return r2_write_c0_; }
    unsigned r2_write_pixel() const { return r2_write_pixel_; }

    void arm_blank_monitor() {
        blank_monitor_armed_ = true;
        blank_seen_ = false;
        pixel_count_ = 0;
        previous_blank_ = blank();
        previous_raw_hsync_ = dut_->CRTC_HSYNC_RAW;
        raw_rise_seen_ = false;
        raw_fall_seen_ = false;
    }

    bool blank_seen() const { return blank_seen_; }
    unsigned blank_pixel() const { return blank_pixel_; }
    unsigned raw_rise_pixel() const { return raw_rise_pixel_; }
    unsigned raw_fall_pixel() const { return raw_fall_pixel_; }

    void run_until_blank() {
        wait_for("GA visible blank", [&] { return blank_seen_; });
    }

    void run_until_raw_pulse_complete() {
        wait_for("raw CRTC HSYNC trailing edge", [&] { return raw_fall_seen_; });
    }

    Vga40010_test& dut() { return *dut_; }

    void check_overflow_restart(bool type1, unsigned relocated_r2) {
        // French ACCC v1.11 §15.3.4 p.152 (rendered chronogram):
        // R2=11→21, R3l=10 keeps the type-1 raw pulse continuous while an
        // invisible end/restart generates a second monitor HS at C0vs=23.
        // Restoring R2=46 gives the ordinary pulse at48. Adjacent R2=22
        // has a genuine end/restart and monitor HS at24 on either type.
        configure(11, 0x8a);
        select_crtc_register(2);
        wait_for_c0(0);
        sync_monitor_armed_ = true;
        sync_rises_.clear();
        sync_last_ = dut_->HSYNC;
        continuous_raw_ = true;
        continuous_black_ = true;
        raw_guard_start_ = 13;
        raw_guard_end_ = 37;
        wait_for_c0(15);
        arm_r2_write_monitor();
        write_selected(relocated_r2);
        if (!r2_write_seen_ || r2_write_c0_ != 15)
            throw TestFailure("restart fixture must write R2 at actual T/E C0=15");
        wait_for_c0(20);
        const auto int_before = dut_->GA_INTCNT;
        const auto vert_before = dut_->GA_HCNT;
        wait_for_c0(22);
        const bool counters_held = dut_->GA_INTCNT == int_before &&
                                   dut_->GA_HCNT == vert_before;
        wait_for_c0(37);
        write_selected(46);
        wait_for_c0(60);
        sync_monitor_armed_ = false;
        const std::vector<unsigned> expected = relocated_r2 == 22
            ? std::vector<unsigned>{13, 24, 48}
            : type1 ? std::vector<unsigned>{13, 23, 48}
                    : std::vector<unsigned>{13, 48};
        std::cout << "restart type=" << type1 << " R2=" << relocated_r2
                  << " GA-rise C0vs:";
        for (auto c0 : sync_rises_) std::cout << ' ' << c0;
        std::cout << " raw-held=" << continuous_raw_
                  << " black-held=" << continuous_black_
                  << " counters-held=" << counters_held << '\n';
        if (sync_rises_ != expected)
            throw TestFailure("GA monitor pulses disagree with FR §15.3.3–4 pp.151–152");
        if (type1 && relocated_r2 == 21 &&
            (!continuous_raw_ || !continuous_black_ || !counters_held))
            throw TestFailure("invisible restart changed raw blanking or R52/V26 end counters");
    }

    void check_restart_mode(unsigned width) {
        // Same FR §15.3.4 terminal-equality interaction at three distinct
        // production phases: before monitor HS (1), during it (2), and after
        // its natural end (6). Only an actual monitor fall may latch MODE.
        configure(11, 0x80 | width);
        select_crtc_register(2);
        wait_for_c0(0);
        sync_monitor_armed_ = true;
        sync_rises_.clear();
        sync_last_ = dut_->HSYNC;
        mode_sync_count_ = restart_count_ = 0;
        continuous_raw_ = continuous_black_ = true;
        raw_guard_start_ = 12;
        raw_guard_end_ = 13 + width;
        write_io(0x7fff, 0x80); // mode 0 waits for a real GA mode-sync edge
        if (dut_->GA_MODE_IS_0)
            throw TestFailure("MODE latched before the target monitor pulse");
        // Width1 must be relocated after its start but before C0=12 ends
        // it; writing on12 would instead mix in the R2.JIT restart path.
        const unsigned write_c0 = width == 1 ? 11 : 12;
        wait_for_c0(write_c0);
        arm_r2_write_monitor();
        write_selected(11 + width);
        if (!r2_write_seen_ || r2_write_c0_ != write_c0)
            throw TestFailure("width-control R2 write missed its pre-terminal character");
        wait_for_c0(27);
        sync_monitor_armed_ = false;
        const std::vector<unsigned> expected = width == 1
            ? std::vector<unsigned>{14}
            : std::vector<unsigned>{13, 13 + width};
        const unsigned mode_edges = width == 1 ? 1 : 2;
        std::cout << "restart R3l=" << width << " mode-edges=" << mode_sync_count_
                  << " restart-events=" << restart_count_ << '\n';
        if (sync_rises_ != expected || mode_sync_count_ != mode_edges ||
            restart_count_ != 1 || !dut_->GA_MODE_IS_0 ||
            !continuous_raw_ || !continuous_black_)
            throw TestFailure("restart lost/duplicated monitor MODE qualification at production phase");
    }

    void check_infinite_restart() {
        // Original CPC SHAKER BR/E 01F4 reports #FF for R0=R2=0,R3l=1:
        // no R52 overflow in 3328us. This cross-module control protects the
        // distinction between an H06 restart and a genuine HSYNC end.
        configure(0);
        // Isolate end counting from the independent raw-VSYNC-start reset
        // of V26. Retire the configured vertical pulse before pinning C0.
        write_crtc(7, 0x7f);
        for (unsigned i = 0; i < 32 * 4096; ++i) clock();
        write_crtc(3, 0x81);
        write_crtc(0, 0);
        wait_for("infinite raw HSYNC", [&] {
            return dut_->CRTC_C0 == 0 && dut_->CRTC_HSYNC_RAW;
        });
        for (unsigned i = 0; i < 64; ++i) clock();
        const auto int_before = dut_->GA_INTCNT;
        const auto vert_before = dut_->GA_HCNT;
        unsigned events = 0;
        for (unsigned i = 0; i < 3328 * 64; ++i) {
            clock();
            events += dut_->CRTC_RESTART;
            if (!dut_->CRTC_HSYNC_RAW || dut_->GA_INTCNT != int_before ||
                dut_->GA_HCNT != vert_before)
                throw TestFailure("BR/E infinite HSYNC changed raw/end counters at tick " +
                    std::to_string(i) + " C0=" + std::to_string(dut_->CRTC_C0) +
                    " HS=" + std::to_string(dut_->CRTC_HSYNC_RAW) +
                    " R52=" + std::to_string(int_before) + "→" + std::to_string(dut_->GA_INTCNT) +
                    " V26=" + std::to_string(vert_before) + "→" + std::to_string(dut_->GA_HCNT));
        }
        if (events == 0) throw TestFailure("infinite-HSYNC control never exercised a restart");
        std::cout << "PASS  infinite_hsync_no_counter_ends (" << events << " restarts)\n";
    }

private:
    bool blank() const {
        return dut_->BLUE_OE_N == 0 && dut_->BLUE == 0 &&
               dut_->GREEN_OE_N == 0 && dut_->GREEN == 0 &&
               dut_->RED_OE_N == 0 && dut_->RED == 0;
    }

    void eval() {
        dut_->eval();
        context_->timeInc(1);
    }

    void clock() {
        dut_->clk = 1;
        eval();

        // Master-clock observation catches a faulty one-tick visible HS low;
        // the existing pixel-M2 blank monitor is too coarse for this contract.
        if (sync_monitor_armed_) {
            mode_sync_count_ += dut_->GA_MODE_SYNC;
            restart_count_ += dut_->CRTC_RESTART;
            if (dut_->HSYNC && !sync_last_)
                sync_rises_.push_back(dut_->CRTC_C0);
            sync_last_ = dut_->HSYNC;
            if (dut_->CRTC_C0 >= raw_guard_start_ && dut_->CRTC_C0 < raw_guard_end_) {
                continuous_raw_ &= bool(dut_->CRTC_HSYNC_RAW);
                continuous_black_ &= blank();
            }
        }

        if (r2_monitor_armed_ && dut_->CRTC_R2 != old_r2_) {
            r2_write_seen_ = true;
            r2_write_c0_ = dut_->CRTC_C0;
            r2_write_pixel_ = pixel_count_;
            old_r2_ = dut_->CRTC_R2;
        }

        if (dut_->CEN_16) {
            if (blank_monitor_armed_) {
                const bool now_blank = blank();
                if (!blank_seen_ && !previous_blank_ && now_blank) {
                    blank_seen_ = true;
                    blank_pixel_ = pixel_count_;
                }
                if (!raw_rise_seen_ && !previous_raw_hsync_ &&
                    dut_->CRTC_HSYNC_RAW) {
                    raw_rise_seen_ = true;
                    raw_rise_pixel_ = pixel_count_;
                }
                if (raw_rise_seen_ && !raw_fall_seen_ &&
                    previous_raw_hsync_ && !dut_->CRTC_HSYNC_RAW) {
                    raw_fall_seen_ = true;
                    raw_fall_pixel_ = pixel_count_;
                }
                previous_blank_ = now_blank;
                previous_raw_hsync_ = dut_->CRTC_HSYNC_RAW;
            }
            ++pixel_count_;
        }

        dut_->clk = 0;
        eval();
    }

    void wait_for(const std::string& description,
                  const std::function<bool()>& predicate,
                  unsigned limit = 200000) {
        for (unsigned cycle = 0; cycle < limit; ++cycle) {
            if (predicate()) {
                return;
            }
            clock();
        }
        throw TestFailure("timeout waiting for " + description);
    }

    void write_io(std::uint16_t address, std::uint8_t value) {
        // Production Z80 I/O write phasing copied from the existing GA render
        // harness. R2 is latched during this cycle, not poked hierarchically.
        wait_for("PHI_N high before T1", [&] { return dut_->PHI_N; });
        dut_->A = address;
        wait_for("PHI_N low in T1", [&] { return !dut_->PHI_N; });
        dut_->CPU_DIN = value;
        wait_for("PHI_N high before T2", [&] { return dut_->PHI_N; });
        dut_->IORQ_N = 0;
        dut_->WR_N = 0;
        wait_for("READY high wait state", [&] {
            return dut_->READY && dut_->PHI_N;
        });
        wait_for("PHI_N low after wait state", [&] { return !dut_->PHI_N; });
        wait_for("PHI_N high in T3", [&] { return dut_->PHI_N; });
        wait_for("PHI_N low at transaction end", [&] { return !dut_->PHI_N; });
        dut_->IORQ_N = 1;
        dut_->WR_N = 1;
        clock();
    }

    void write_crtc(std::uint8_t reg, std::uint8_t value) {
        write_io(0xbc00, reg);
        write_io(0xbd00, value);
    }

    std::unique_ptr<VerilatedContext> context_;
    std::unique_ptr<Vga40010_test> dut_;
    bool r2_monitor_armed_ = false;
    std::uint8_t old_r2_ = 0;
    bool r2_write_seen_ = false;
    unsigned r2_write_c0_ = 0;
    unsigned r2_write_pixel_ = 0;
    bool blank_monitor_armed_ = false;
    bool blank_seen_ = false;
    bool previous_blank_ = false;
    unsigned blank_pixel_ = 0;
    unsigned raw_rise_pixel_ = 0;
    unsigned raw_fall_pixel_ = 0;
    unsigned pixel_count_ = 0;
    bool previous_raw_hsync_ = false;
    bool raw_rise_seen_ = false;
    bool raw_fall_seen_ = false;
    bool sync_monitor_armed_ = false, sync_last_ = false;
    bool continuous_raw_ = true, continuous_black_ = true;
    std::vector<unsigned> sync_rises_;
    unsigned mode_sync_count_ = 0, restart_count_ = 0;
    unsigned raw_guard_start_ = 13, raw_guard_end_ = 37;
};

struct Measurement {
    unsigned write_c0;
    unsigned write_pixel;
    unsigned blank_pixel;
    unsigned raw_rise_pixel;
    unsigned raw_fall_pixel;
};

Measurement measure(bool crtc_type, bool dynamic, std::uint8_t initial_r2,
                    std::uint8_t start_c0, std::uint8_t written_r2) {
    Bench bench(crtc_type);
    bench.configure(initial_r2);
    if (dynamic) {
        bench.select_crtc_register(2);
    }
    bench.wait_for_c0(32);
    bench.arm_blank_monitor();
    if (dynamic) {
        bench.wait_for_c0(start_c0);
        bench.arm_r2_write_monitor();
        bench.write_selected(written_r2);
        if (initial_r2 != written_r2 && !bench.r2_write_seen()) {
            throw TestFailure("R2 write did not reach the CRTC");
        }
    }
    bench.run_until_blank();
    bench.run_until_raw_pulse_complete();
    return {bench.r2_write_c0(), bench.r2_write_pixel(),
            bench.blank_pixel(), bench.raw_rise_pixel(),
            bench.raw_fall_pixel()};
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    try {
        const Measurement normal = measure(true, false, 46, 0, 0);
        const Measurement jit = measure(true, true, 55, 46, 46);
        const int blank_delta = static_cast<int>(jit.blank_pixel) -
                                static_cast<int>(normal.blank_pixel);
        const unsigned normal_width = normal.raw_fall_pixel - normal.raw_rise_pixel;
        const unsigned jit_width = jit.raw_fall_pixel - jit.raw_rise_pixel;
        std::cout << "type1 normal: blank=" << normal.blank_pixel
                  << " raw=[" << normal.raw_rise_pixel << ','
                  << normal.raw_fall_pixel << ") width=" << normal_width << '\n'
                  << "type1 R2.JIT: write C0=" << jit.write_c0
                  << " pixel=" << jit.write_pixel
                  << " blank=" << jit.blank_pixel
                  << " raw=[" << jit.raw_rise_pixel << ','
                  << jit.raw_fall_pixel << ") width=" << jit_width
                  << " delta=" << blank_delta << '\n';
        if (jit.write_c0 != 46) {
            throw TestFailure("OUT(C) did not latch R2 at C0=R2");
        }
        if (blank_delta != 3 || jit_width + 3 != normal_width) {
            throw TestFailure("expected +3-pixel start and -3-pixel pulse "
                              "(ACCC v1.11 sections 9.3.4.1/9.3.4.3 "
                              "pp.54-58 and 14.7.1 p.142)");
        }
        std::cout << "PASS  r2jit_type1_out_c\n";

        const Measurement type0_normal = measure(false, false, 46, 0, 0);
        const Measurement type0_jit = measure(false, true, 55, 46, 46);
        const int type0_delta = static_cast<int>(type0_jit.blank_pixel) -
                                static_cast<int>(type0_normal.blank_pixel);
        const unsigned type0_normal_width = type0_normal.raw_fall_pixel -
                                            type0_normal.raw_rise_pixel;
        const unsigned type0_jit_width = type0_jit.raw_fall_pixel -
                                         type0_jit.raw_rise_pixel;
        if (type0_delta != 4 || type0_jit_width + 4 != type0_normal_width) {
            throw TestFailure("expected type-0 +4-pixel start and -4-pixel pulse "
                              "(ACCC v1.11 section 9.3.4.3 p.58)");
        }
        std::cout << "PASS  r2jit_type0_out_c\n";

        const Measurement same_value = measure(true, true, 46, 46, 46);
        if (same_value.blank_pixel != normal.blank_pixel ||
            same_value.raw_rise_pixel != normal.raw_rise_pixel ||
            same_value.raw_fall_pixel != normal.raw_fall_pixel) {
            throw TestFailure("same-value R2 rewrite changed normal HSYNC timing");
        }
        std::cout << "PASS  r2jit_same_value_control\n";

        for (bool type1 : {false, true}) {
            Bench adjacent(type1);
            adjacent.check_overflow_restart(type1, 22);
        }
        Bench type0_overflow(false);
        type0_overflow.check_overflow_restart(false, 21);
        std::cout << "PASS  invisible_restart_adjacent_and_type0_controls\n";
        Bench type1_overflow(true);
        type1_overflow.check_overflow_restart(true, 21);
        std::cout << "PASS  invisible_restart_type1_ga_boundary\n";
        for (unsigned width : {1u, 2u, 6u}) {
            Bench mode(true);
            mode.check_restart_mode(width);
        }
        std::cout << "PASS  invisible_restart_mode_qualification\n";
        Bench infinite(true);
        infinite.check_infinite_restart();
    } catch (const std::exception& error) {
        std::cerr << "FAIL  r2jit_integrated_controls\n      " << error.what() << '\n';
        return 1;
    }
    return 0;
}
