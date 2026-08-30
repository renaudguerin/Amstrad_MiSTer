#include <verilated.h>
#include <verilated_vcd_c.h>

#include "Vu765_test.h"
#include "Vu765_test___024root.h"

#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
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
    Bench()
        : context_(std::make_unique<VerilatedContext>()),
          dut_(std::make_unique<Vu765_test>(context_.get())),
          trace_(std::make_unique<VerilatedVcdC>()) {
        context_->traceEverOn(true);
        dut_->clk_sys = 0;
        dut_->ce = 0;
        dut_->reset = 0;
        dut_->ready = 0;
        dut_->motor = 0;
        dut_->available = 3;
        dut_->fast = 0;
        dut_->a0 = 0;
        dut_->nRD = 1;
        dut_->nWR = 1;
        dut_->din = 0;
        dut_->img_mounted = 0;
        dut_->img_wp = 0;
        dut_->img_size = 0;
        dut_->sd_ack = 0;
        dut_->sd_buff_addr = 0;
        dut_->sd_buff_dout = 0;
        dut_->sd_buff_wr = 0;
        dut_->trace(trace_.get(), 99);
        trace_->open("obj_dir/u765_tests.vcd");
        eval();
    }

    ~Bench() {
        dut_->final();
        trace_->close();
    }

    void tick(bool ce = false) {
        dut_->ce = ce;
        dut_->clk_sys = 0;
        eval();
        dut_->clk_sys = 1;
        eval();
        dut_->clk_sys = 0;
        eval();
        ++cycles_;
    }

    // Production pulses ce_u765 once per eight clk_sys cycles (Amstrad.sv).
    void fdc_step() {
        tick(true);
        for (unsigned cycle = 1; cycle < 8; ++cycle) {
            tick(false);
        }
    }

    void reset() {
        dut_->reset = 1;
        for (unsigned cycle = 0; cycle < 4; ++cycle) {
            tick(cycle == 0);
        }
        dut_->reset = 0;
        fdc_step();
    }

    std::uint8_t status() {
        dut_->a0 = 0;
        eval();
        return dut_->dout;
    }

    void write_data(std::uint8_t value, bool a0) {
        dut_->a0 = a0;
        dut_->din = value;
        dut_->nRD = 1;
        dut_->nWR = 0;
        fdc_step();
        dut_->nWR = 1;
        fdc_step();
    }

    std::uint8_t read_data() {
        dut_->a0 = 1;
        dut_->nWR = 1;
        dut_->nRD = 0;
        fdc_step();
        const std::uint8_t value = dut_->dout;
        dut_->nRD = 1;
        fdc_step();
        return value;
    }

    void mount_without_ack(std::uint32_t size) {
        dut_->img_size = size;
        dut_->img_mounted = 1;
        tick();
        dut_->img_mounted = 0;
    }

    void load_image(const std::string& path) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            fail("cannot open disk image " + path);
        }
        image_.assign(std::istreambuf_iterator<char>(input),
                      std::istreambuf_iterator<char>());
    }

    void service_read_request() {
        if (dut_->sd_rd == 0) {
            fail("service_read_request called without sd_rd");
        }
        const std::uint64_t offset =
            static_cast<std::uint64_t>(dut_->sd_lba) * 512;
        for (unsigned address = 0; address < 512; ++address) {
            dut_->sd_ack = 1;
            dut_->sd_buff_wr = 1;
            dut_->sd_buff_addr = address;
            const std::uint64_t image_address = offset + address;
            dut_->sd_buff_dout = image_address < image_.size()
                                    ? image_[image_address]
                                    : 0;
            tick();
        }
        dut_->sd_buff_wr = 0;
        dut_->sd_ack = 0;
        // Return while the arbiter is still retiring ACK. The next outer
        // fdc_step lets the command process clear its request token before
        // sdcontrol can observe idle ownership and reissue the old request.
        tick();
    }

    bool image_ready(unsigned drive) const {
        return dut_->rootp
            ->u765_test__DOT__u765__DOT__fdc__DOT__image_ready[drive];
    }

    void print_parser_state(unsigned drive) const {
        const auto& root = *dut_->rootp;
        std::cerr << "parser debug: scan="
                  << static_cast<unsigned>(root.u765_test__DOT__u765__DOT__fdc__DOT__image_scan_state[drive])
                  << " busy="
                  << static_cast<unsigned>(root.u765_test__DOT__u765__DOT__sd_busy_mount)
                  << " ack="
                  << static_cast<unsigned>(root.u765_test__DOT__u765__DOT__ack)
                  << " sd_rd=" << static_cast<unsigned>(dut_->sd_rd)
                  << " req="
                  << static_cast<unsigned>(root.u765_test__DOT__u765__DOT__sd_rd_mount)
                  << " lock="
                  << static_cast<unsigned>(root.u765_test__DOT__u765__DOT__fdc__DOT__i_scan_lock)
                  << " addr="
                  << static_cast<unsigned>(root.u765_test__DOT__u765__DOT__buff_addr)
                  << " tracks="
                  << static_cast<unsigned>(root.u765_test__DOT__u765__DOT__fdc__DOT__image_tracks[drive])
                  << '\n';
    }

    void wait_until(const std::string& description,
                    const std::function<bool()>& predicate,
                    unsigned limit = 2000) {
        for (unsigned step = 0; step < limit; ++step) {
            if (predicate()) {
                return;
            }
            fdc_step();
        }
        fail("timeout waiting for " + description);
    }

    void expect_equal(const std::string& description,
                      std::uint32_t expected,
                      std::uint32_t actual) const {
        if (expected != actual) {
            fail(description + ": expected " + std::to_string(expected) +
                 ", actual " + std::to_string(actual));
        }
    }

    Vu765_test& dut() { return *dut_; }

private:
    void eval() {
        dut_->eval();
        trace_->dump(context_->time());
        context_->timeInc(1);
    }

    [[noreturn]] void fail(const std::string& message) const {
        throw TestFailure("cycle " + std::to_string(cycles_) + ": " + message);
    }

    std::unique_ptr<VerilatedContext> context_;
    std::unique_ptr<Vu765_test> dut_;
    std::unique_ptr<VerilatedVcdC> trace_;
    std::vector<std::uint8_t> image_;
    std::uint64_t cycles_ = 0;
};

void test_writes_ignore_a0() {
    Bench bench;
    bench.reset();
    bench.expect_equal("idle status after reset", 0x80, bench.status());

    // CPC expansion-port map: A0 selects status/data only for reads. Both
    // FDC write aliases address the data register. Exercise command and
    // parameter bytes through A0=0.
    bench.write_data(0x04, false);  // SENSE DRIVE STATUS
    bench.wait_until("parameter phase after A0=0 command", [&] {
        return (bench.status() & 0xd0) == 0x90;
    });
    bench.write_data(0x00, false);
    bench.wait_until("result phase after A0=0 parameter", [&] {
        return (bench.status() & 0xd0) == 0xd0;
    });
    bench.expect_equal("ST3 ready bit from available drive", 0x20,
                       bench.read_data());
    bench.wait_until("idle after SENSE DRIVE STATUS", [&] {
        return bench.status() == 0x80;
    });
}

void test_reset_cancels_and_restarts_mount_request() {
    Bench bench;
    bench.reset();
    bench.mount_without_ack(194816);
    bench.wait_until("initial mount SD request", [&] {
        return bench.dut().sd_rd != 0;
    });

    bench.dut().reset = 1;
    bench.tick(true);
    bench.expect_equal("reset clears outstanding sd_rd", 0,
                       bench.dut().sd_rd);
    bench.expect_equal("reset clears outstanding sd_wr", 0,
                       bench.dut().sd_wr);
    bench.tick();
    bench.dut().reset = 0;

    // Model a host response and data burst that were already in flight when
    // CPR reset cancelled the old request. No retry or shared-buffer write is
    // accepted until ACK has returned low; after that the parser must present
    // a fresh request without another core reload.
    bench.dut().sd_ack = 1;
    bench.dut().sd_buff_wr = 1;
    bench.dut().sd_buff_addr = 7;
    bench.dut().sd_buff_dout = 0xa5;
    for (unsigned cycle = 0; cycle < 16; ++cycle) {
        bench.tick();
        bench.expect_equal("delayed ACK cannot claim a retry", 0,
                           bench.dut().sd_rd | bench.dut().sd_wr);
    }
    bench.dut().sd_buff_wr = 0;
    bench.dut().sd_ack = 0;

    bench.wait_until("fresh mount request after reset", [&] {
        return bench.dut().sd_rd != 0;
    });
}

void test_mount_recognizes_edsk(const std::string& image_path) {
    Bench bench;
    bench.reset();
    bench.load_image(image_path);
    std::ifstream image(image_path, std::ios::binary | std::ios::ate);
    const auto size = static_cast<std::uint32_t>(image.tellg());
    bench.dut().ready = 1;
    bench.dut().motor = 3;
    bench.mount_without_ack(size);

    for (unsigned step = 0; step < 4000 && !bench.image_ready(0); ++step) {
        if (bench.dut().sd_rd != 0) {
            bench.service_read_request();
        } else {
            bench.fdc_step();
        }
    }
    if (!bench.image_ready(0)) {
        bench.print_parser_state(0);
    }
    bench.expect_equal("mounted EDSK reaches image_ready", 1, bench.image_ready(0));

    // fdctest's ready/track-0 checks use SENSE DRIVE STATUS. Require both
    // bits through the public command path, not only the parser state.
    bench.write_data(0x04, true);
    bench.wait_until("SENSE DRIVE STATUS parameter phase", [&] {
        return (bench.status() & 0xd0) == 0x90;
    });
    bench.write_data(0x00, true);
    bench.wait_until("SENSE DRIVE STATUS result phase", [&] {
        return (bench.status() & 0xd0) == 0xd0;
    });
    const std::uint8_t st3 = bench.read_data();
    bench.expect_equal("ST3 reports ready and track 0", 0x30, st3 & 0x30);
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    unsigned passed = 0;
    const auto run = [&](const std::string& name, const auto& test) {
        try {
            test();
            ++passed;
            std::cout << "PASS  " << name << '\n';
        } catch (const std::exception& error) {
            std::cerr << "FAIL  " << name << "\n      " << error.what() << '\n';
            throw;
        }
    };

    try {
        run("u765_reset_cancels_and_restarts_mount_request",
            test_reset_cancels_and_restarts_mount_request);
        run("u765_writes_ignore_a0", test_writes_ignore_a0);
        const std::string image_path = argc > 1 ? argv[1] : "test.dsk";
        run("u765_mount_recognizes_edsk", [&] {
            test_mount_recognizes_edsk(image_path);
        });
    } catch (...) {
        return 1;
    }

    std::cout << "Summary: " << passed << " passed, 0 failed\n";
    return 0;
}
