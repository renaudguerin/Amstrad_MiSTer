#include <verilated.h>
#include <verilated_vcd_c.h>

#include "Vu765_test.h"
#include "Vu765_test___024root.h"

#include <algorithm>
#include <array>
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

    void load_synthetic_edsk() {
        // One 0x100-byte Disk-Info block, followed by two 0x300-byte
        // Track-Info/data blocks containing one 512-byte sector each.
        image_.assign(0x700, 0);
        const std::string signature =
            "EXTENDED CPC DSK File\r\nDisk-Info\r\n";
        std::copy(signature.begin(), signature.end(), image_.begin());
        image_[0x30] = 2;     // tracks
        image_[0x31] = 1;     // sides
        image_[0x34] = 3;     // track 0 block size in 0x100-byte units
        image_[0x35] = 3;     // track 1 block size in 0x100-byte units

        const std::string track_signature = "Track-Info\r\n";
        std::copy(track_signature.begin(), track_signature.end(),
                  image_.begin() + 0x100);
        image_[0x114] = 2;    // N: 512-byte sectors
        image_[0x115] = 1;    // sectors per track
        image_[0x116] = 0x1e; // GAP3
        image_[0x117] = 0xe5; // filler
        image_[0x118] = 0;    // C
        image_[0x119] = 0;    // H
        image_[0x11a] = 1;    // R
        image_[0x11b] = 2;    // N
        image_[0x11c] = 0;    // ST1
        image_[0x11d] = 0;    // ST2
        image_[0x11e] = 0x00; // stored length low
        image_[0x11f] = 0x02; // stored length high
        for (unsigned address = 0; address < 512; ++address) {
            image_[0x200 + address] =
                static_cast<std::uint8_t>((address * 37 + 0x5a) & 0xff);
        }

        std::copy(track_signature.begin(), track_signature.end(),
                  image_.begin() + 0x400);
        image_[0x410] = 1;    // physical track
        image_[0x414] = 2;    // N: 512-byte sectors
        image_[0x415] = 1;    // sectors per track
        image_[0x416] = 0x1e; // GAP3
        image_[0x417] = 0xe5; // filler
        image_[0x418] = 1;    // C
        image_[0x419] = 0;    // H
        image_[0x41a] = 2;    // R (distinct from track 0)
        image_[0x41b] = 2;    // N
        image_[0x41c] = 0;    // ST1
        image_[0x41d] = 0;    // ST2
        image_[0x41e] = 0x00; // stored length low
        image_[0x41f] = 0x02; // stored length high
        for (unsigned address = 0; address < 512; ++address) {
            image_[0x500 + address] =
                static_cast<std::uint8_t>((address * 53 + 0xa6) & 0xff);
        }
    }

    std::uint32_t image_size() const {
        return static_cast<std::uint32_t>(image_.size());
    }

    void service_read_request() {
        if (dut_->sd_rd == 0) {
            fail("service_read_request called without sd_rd");
        }
        service_read_response(dut_->sd_lba);
    }

    void service_read_response(std::uint32_t lba) {
        const std::uint64_t offset = static_cast<std::uint64_t>(lba) * 512;
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

    std::uint8_t pcn(unsigned drive) const {
        return dut_->rootp
            ->u765_test__DOT__u765__DOT__fdc__DOT__pcn[drive];
    }

    bool trackinfo_dirty(unsigned drive) const {
        return dut_->rootp
            ->u765_test__DOT__u765__DOT__fdc__DOT__image_trackinfo_dirty[drive];
    }

    bool mount_busy() const {
        return dut_->rootp->u765_test__DOT__u765__DOT__sd_busy_mount;
    }

    std::uint8_t image_byte(std::uint64_t address) const {
        if (address >= image_.size()) {
            fail("image byte address is out of range");
        }
        return image_[address];
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

    std::uint8_t sector_byte(unsigned address) const {
        return dut_->rootp
            ->u765_test__DOT__u765__DOT__sector_ram__DOT__ram[address];
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

    // Seed byte 7 while ACK is already high. Reset may withdraw this acknowledged
    // request immediately, but the still-active burst must remain quarantined.
    bench.dut().sd_ack = 1;
    bench.dut().sd_buff_wr = 1;
    bench.dut().sd_buff_addr = 7;
    bench.dut().sd_buff_dout = 0x3c;
    bench.tick();
    bench.dut().sd_buff_wr = 0;
    bench.expect_equal("pre-reset transfer seeds sector buffer", 0x3c,
                       bench.sector_byte(7));

    bench.dut().reset = 1;
    bench.tick(true);
    bench.expect_equal("acknowledged reset withdraws outstanding sd_rd", 0,
                       bench.dut().sd_rd);
    bench.expect_equal("acknowledged reset withdraws outstanding sd_wr", 0,
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
        bench.tick(cycle % 8 == 0);
        bench.expect_equal("delayed ACK cannot claim a retry", 0,
                           bench.dut().sd_rd | bench.dut().sd_wr);
    }
    bench.expect_equal("stale burst cannot overwrite sector buffer", 0x3c,
                       bench.sector_byte(7));
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

void mount_synthetic_edsk(Bench& bench) {
    bench.reset();
    bench.load_synthetic_edsk();
    bench.dut().ready = 1;
    bench.dut().motor = 3;
    bench.dut().fast = 1;
    bench.mount_without_ack(bench.image_size());

    for (unsigned step = 0; step < 4000 && !bench.image_ready(0); ++step) {
        if (bench.dut().sd_rd != 0) {
            bench.service_read_request();
        } else {
            bench.fdc_step();
        }
    }
    bench.expect_equal("EDSK mount completes", 1, bench.image_ready(0));
}

void issue_read_data(Bench& bench, std::uint8_t cylinder,
                     std::uint8_t sector) {
    // The synthetic EDSK independently defines track 0/head 0/sector 1 as
    // N=2 at byte offset 0x200. READ DATA must therefore request LBA 1 and
    // return the generated 512-byte payload at 0x200..0x3ff.
    const std::uint8_t command[] = {
        0x46,  // MFM READ DATA
        0x00,  // drive 0, head 0
        cylinder,
        0x00,  // H
        sector,
        0x02,  // N: 512 bytes
        sector,
        0x1e,  // GPL from the fixture Track-Info
        0xff   // DTL ignored for N != 0
    };
    for (const auto byte : command) {
        bench.wait_until("u765 ready for READ DATA byte", [&] {
            return (bench.status() & 0x80) != 0;
        });
        bench.write_data(byte, true);
    }
}

void issue_seek(Bench& bench, std::uint8_t cylinder) {
    const std::uint8_t command[] = {0x0f, 0x00, cylinder};
    for (const auto byte : command) {
        bench.wait_until("u765 ready for SEEK byte", [&] {
            return (bench.status() & 0x80) != 0;
        });
        bench.write_data(byte, true);
    }
}

void wait_for_track_ready(Bench& bench, std::uint8_t cylinder) {
    for (unsigned step = 0; step < 20000; ++step) {
        if (bench.pcn(0) == cylinder && !bench.trackinfo_dirty(0)) {
            return;
        }
        if (bench.dut().sd_rd != 0) {
            bench.service_read_request();
        } else {
            bench.fdc_step();
        }
    }
    throw TestFailure("timeout waiting for track metadata reload");
}

void wait_for_sector_read_request(Bench& bench, std::uint32_t lba) {
    for (unsigned step = 0; step < 20000; ++step) {
        if (bench.dut().sd_rd != 0) {
            if (bench.dut().sd_lba == lba) {
                return;
            }
            bench.service_read_request();
        } else {
            bench.fdc_step();
        }
    }
    throw TestFailure("timeout waiting for READ DATA sector request");
}

void expect_sector_1_payload(Bench& bench) {
    bench.service_read_request();
    for (unsigned address = 0; address < 512; ++address) {
        bench.wait_until("READ DATA byte ready", [&] {
            return (bench.status() & 0xf0) == 0xf0;
        }, 20000);
        bench.expect_equal("READ DATA payload byte " + std::to_string(address),
                           bench.image_byte(0x200 + address),
                           bench.read_data());
    }
    bool result_ready = false;
    for (unsigned step = 0; step < 300000; ++step) {
        if ((bench.status() & 0xf0) == 0xd0) {
            result_ready = true;
            break;
        }
        if (bench.dut().sd_rd != 0) {
            bench.service_read_request();
        } else {
            bench.fdc_step();
        }
    }
    if (!result_ready) {
        throw TestFailure("timeout waiting for READ DATA result phase");
    }
    std::array<std::uint8_t, 7> results{};
    for (unsigned index = 0; index < 7; ++index) {
        results[index] = bench.read_data();
    }
    bench.expect_equal("READ DATA ST0 reports abnormal EOT", 0x40, results[0]);
    bench.expect_equal("READ DATA ST1 reports end of cylinder", 0x80,
                       results[1]);
    bench.expect_equal("READ DATA ST2 is clear", 0x00, results[2]);
    // C/R result updates at automatic EOT are a separate compatibility
    // question. Consume them to prove phase completion without freezing the
    // current RTL values as a hardware oracle.
    bench.expect_equal("READ DATA result head", 0x00, results[4]);
    bench.expect_equal("READ DATA result size", 0x02, results[6]);
    bench.wait_until("idle after READ DATA results", [&] {
        return bench.status() == 0x80;
    });
}

void test_reset_after_ack_fall_uses_history() {
    Bench bench;
    bench.reset();
    bench.load_synthetic_edsk();
    bench.dut().ready = 1;
    bench.dut().motor = 3;
    bench.mount_without_ack(bench.image_size());
    bench.wait_until("mount request for ACK-history fixture", [&] {
        return bench.dut().sd_rd != 0;
    });
    bench.service_read_request();
    bench.expect_equal("ACK rise withdrew mount request", 0,
                       bench.dut().sd_rd);
    bench.expect_equal("falling ACK is still retiring mount ownership", 1,
                       bench.mount_busy());

    // Raw ACK is already low, but the six-bit sampled ACK history still
    // proves that this transaction completed. Reset must use that history to
    // drain ownership rather than waiting forever for another ACK rise.
    bench.dut().reset = 1;
    bench.tick(true);
    bench.dut().reset = 0;
    bench.wait_until("fresh mount request after ACK-history drain", [&] {
        return bench.dut().sd_rd != 0;
    });
    bench.expect_equal("ACK-history recovery restarts header LBA", 0,
                       bench.dut().sd_lba);
}

void test_read_data_survives_reset_during_active_request() {
    Bench bench;
    mount_synthetic_edsk(bench);
    issue_read_data(bench, 0, 1);
    wait_for_sector_read_request(bench, 1);
    bench.expect_equal("fixture reaches sector LBA", 1, bench.dut().sd_lba);
    const std::uint8_t quarantined_byte = bench.sector_byte(7);

    // hps_io polls sd_rd as a held request and may return sd_ack later.  A CPC
    // reset must not withdraw and replace that transaction while its response
    // can still arrive: without a transaction tag, the only unambiguous seam
    // is to keep the old request owned until ACK rises and falls.  The payload
    // is quarantined because the command that requested it has been reset.
    bench.dut().reset = 1;
    bench.tick(true);
    bench.expect_equal("reset retains cancelled request until host ACK", 1,
                       bench.dut().sd_rd);
    bench.expect_equal("cancelled request retains original sector LBA", 1,
                       bench.dut().sd_lba);

    bench.service_read_response(1);
    bench.expect_equal("cancelled request retires after ACK", 0,
                       bench.dut().sd_rd | bench.dut().sd_wr);
    bench.expect_equal("cancelled payload remains quarantined",
                       quarantined_byte, bench.sector_byte(7));
    bench.dut().reset = 0;
    bench.fdc_step();

    // Reset invalidates track metadata, so the core must reload it and then
    // complete a fresh real READ DATA command from the same EDSK sector.
    issue_read_data(bench, 0, 1);
    wait_for_sector_read_request(bench, 1);
    expect_sector_1_payload(bench);
}

void test_short_reset_waits_to_reload_trackinfo() {
    Bench bench;
    mount_synthetic_edsk(bench);
    issue_seek(bench, 1);
    wait_for_track_ready(bench, 1);
    issue_read_data(bench, 1, 2);
    wait_for_sector_read_request(bench, 2);

    // Pulse reset, as a keyboard/core reset may do, but deliberately leave the
    // track-1 sector response outstanding. Track-0 metadata reload must not
    // enqueue and discard a request behind the cancelled global owner.
    bench.dut().reset = 1;
    bench.tick(true);
    bench.dut().reset = 0;
    for (unsigned step = 0; step < 32; ++step) {
        bench.fdc_step();
        bench.expect_equal("short reset retains old track-1 request", 1,
                           bench.dut().sd_rd);
        bench.expect_equal("short reset preserves old track-1 LBA", 2,
                           bench.dut().sd_lba);
    }
    bench.service_read_response(2);

    // Reset returns the drive to PCN 0. A fresh track-0 READ DATA must force
    // the real LBA-0 Track-Info reload before reading sector 1 from LBA 1.
    issue_read_data(bench, 0, 1);
    wait_for_sector_read_request(bench, 1);
    expect_sector_1_payload(bench);
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
        run("u765_reset_after_ack_fall_uses_history",
            test_reset_after_ack_fall_uses_history);
        const std::string image_path = argc > 1 ? argv[1] : "test.dsk";
        run("u765_mount_recognizes_edsk", [&] {
            test_mount_recognizes_edsk(image_path);
        });
        run("u765_read_data_survives_reset_during_active_request", [&] {
            test_read_data_survives_reset_during_active_request();
        });
        run("u765_short_reset_waits_to_reload_trackinfo",
            test_short_reset_waits_to_reload_trackinfo);
    } catch (...) {
        return 1;
    }

    std::cout << "Summary: " << passed << " passed, 0 failed\n";
    return 0;
}
