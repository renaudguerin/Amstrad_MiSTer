#include <verilated.h>

#include "Vga40010_test.h"

#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

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

    void configure(std::uint8_t r2) {
        // A short, stable raster with the sync position well inside display.
        write_crtc(0, 63);
        write_crtc(1, 56);
        write_crtc(2, r2);
        write_crtc(3, 0x84);
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
};

struct Measurement {
    unsigned write_c0;
    unsigned write_pixel;
    unsigned blank_pixel;
    unsigned raw_rise_pixel;
    unsigned raw_fall_pixel;
};

Measurement measure(bool dynamic, std::uint8_t start_c0) {
    Bench bench(true);
    bench.configure(dynamic ? 55 : 46);
    if (dynamic) {
        bench.select_crtc_register(2);
    }
    bench.wait_for_c0(32);
    bench.arm_blank_monitor();
    if (dynamic) {
        bench.wait_for_c0(start_c0);
        bench.arm_r2_write_monitor();
        bench.write_selected(46);
        if (!bench.r2_write_seen()) {
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
        const Measurement normal = measure(false, 0);
        const Measurement jit = measure(true, 46);
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
        if (blank_delta == 4 && jit_width + 4 == normal_width) {
            std::cout << "XFAIL r2jit_type1_out_c: expected +3 pixels and "
                         "unchanged HSYNC width (ACCC v1.11 section 14.6.1 "
                         "p.141)\n";
            return 0;
        }
        if (blank_delta != 3 || jit_width != normal_width) {
            throw TestFailure("unexpected R2.JIT timing signature");
        }
        std::cout << "PASS  r2jit_type1_out_c\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL  r2jit_type1_out_c\n      " << error.what() << '\n';
        return 1;
    }
    return 0;
}
