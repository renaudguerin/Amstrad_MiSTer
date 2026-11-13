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

    // Production-shaped data read: the T80 IO read pulse spans T2+wait+T3
    // (several 8 MHz ce ticks), and T80pa latches DI at mid-T3
    // (tstate==011 on cen_n), near the END of the pulse while RD is still
    // asserted. So hold nRD across three ce pulses and sample at the end.
    // Intermediate samples pin that a held level does not re-trigger the
    // edge detector (one edge, one byte).
    std::uint8_t read_data_production() {
        dut_->a0 = 1;
        dut_->nWR = 1;
        dut_->nRD = 0;
        fdc_step();
        const std::uint8_t early = dut_->dout;
        fdc_step();
        const std::uint8_t mid = dut_->dout;
        fdc_step();
        const std::uint8_t late = dut_->dout;
        dut_->nRD = 1;
        fdc_step();
        expect_equal("held-RD sample 2 stable", early, mid);
        expect_equal("held-RD sample 3 stable", early, late);
        return late;
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

void issue_rw_data(Bench& bench, std::uint8_t opcode,
                   std::uint8_t cylinder, std::uint8_t sector) {
    // The synthetic EDSK independently defines track 0/head 0/sector 1 as
    // N=2 at byte offset 0x200. READ DATA must therefore request LBA 1 and
    // return the generated 512-byte payload at 0x200..0x3ff.
    const std::uint8_t command[] = {
        opcode,  // MFM READ DATA (46) or WRITE DATA (45)
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
        bench.wait_until("u765 ready for READ/WRITE DATA command byte", [&] {
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
    issue_rw_data(bench, 0x46, 0, 1);
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
    issue_rw_data(bench, 0x46, 0, 1);
    wait_for_sector_read_request(bench, 1);
    expect_sector_1_payload(bench);
}

void test_short_reset_waits_to_reload_trackinfo() {
    Bench bench;
    mount_synthetic_edsk(bench);
    issue_seek(bench, 1);
    wait_for_track_ready(bench, 1);
    issue_rw_data(bench, 0x46, 1, 2);
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
    issue_rw_data(bench, 0x46, 0, 1);
    wait_for_sector_read_request(bench, 1);
    expect_sector_1_payload(bench);
}

void test_read_data_production_shaped_first_byte() {
    Bench bench;
    mount_synthetic_edsk(bench);
    issue_rw_data(bench, 0x46, 0, 1);
    wait_for_sector_read_request(bench, 1);
    bench.service_read_request();
    bench.wait_until("READ DATA first payload byte ready", [&] {
        return (bench.status() & 0xf0) == 0xf0;
    }, 20000);
    // Byte 0 of the synthetic sector-1 payload is fixed by the in-file
    // generator ((0*37+0x5a)&0xff), independent of the simulator and of
    // test.dsk.
    bench.expect_equal("production-shaped first payload byte", 0x5a,
                       bench.read_data_production());
    // The held pulse must have consumed exactly one byte: the next
    // production-shaped read returns byte 1 ((1*37+0x5a)&0xff).
    bench.wait_until("READ DATA second payload byte ready", [&] {
        return (bench.status() & 0xf0) == 0xf0;
    }, 20000);
    bench.expect_equal("production-shaped second payload byte", 0x7f,
                       bench.read_data_production());
}

void test_write_data_uninterrupted_payload() {
    Bench bench;
    mount_synthetic_edsk(bench);
    issue_rw_data(bench, 0x45, 0, 1);
    wait_for_sector_read_request(bench, 1);
    bench.service_read_request();
    const std::uint8_t changed = bench.image_byte(0x200) ^ 0xff;
    for (unsigned address = 0; address < 512; ++address) {
        bench.wait_until("WRITE DATA payload byte ready", [&] {
            return (bench.status() & 0xf0) == 0xb0;
        }, 20000);
        const auto byte = address == 0 ? changed : bench.image_byte(0x200 + address);
        bench.write_data(byte, true);
    }
    bench.wait_until("completed WRITE DATA media request", [&] {
        return bench.dut().sd_wr == 1;
    }, 20000);
    bench.expect_equal("uninterrupted WRITE DATA target LBA", 1,
                       bench.dut().sd_lba);
    // The host consumes this write through the existing synchronous buffer
    // read port. All other bytes retain the independent EDSK input payload.
    bench.dut().sd_ack = 1;
    for (unsigned address = 0; address < 512; ++address) {
        bench.dut().sd_buff_addr = address;
        bench.tick();
        const auto expected = address == 0 ? changed : bench.image_byte(0x200 + address);
        bench.expect_equal("uninterrupted WRITE DATA host byte " + std::to_string(address),
                           expected, bench.dut().sd_buff_din);
    }
    bench.dut().sd_ack = 0;
    bench.tick();
    std::cout << "WRITE control: payload512 host_lba=1 first=" << unsigned(changed)
              << " remaining511 match EDSK input\n";
}

void test_reset_after_write_byte_preserves_fresh_read() {
    Bench bench;
    mount_synthetic_edsk(bench);
    issue_rw_data(bench, 0x45, 0, 1);
    wait_for_sector_read_request(bench, 1);
    bench.service_read_request();
    bench.wait_until("WRITE DATA first payload byte ready before reset", [&] {
        return (bench.status() & 0xf0) == 0xb0;
    }, 20000);
    const auto original = bench.image_byte(0x200);
    const std::uint8_t changed = original ^ 0xff;
    // Accept the CPU byte on ce, then reset between ce pulses. The next
    // ce would normally finish that write. Memory still clocks throughout.
    bench.dut().a0 = 1;
    bench.dut().din = changed;
    bench.dut().nWR = 0;
    bench.tick(true);
    bench.dut().nWR = 1;
    bench.tick();
    bench.expect_equal("accepted WRITE DATA byte reaches buffer", changed,
                       bench.sector_byte(0));
    bench.dut().reset = 1;
    for (unsigned clock = 2; clock < 8; ++clock) {
        if (clock == 4) bench.dut().reset = 0;
        bench.tick();
        bench.expect_equal("aborted WRITE has no media write before fresh READ", 0,
                           bench.dut().sd_wr);
    }
    // Let the released CPU write level retire on ce before a new command.
    bench.fdc_step();
    bench.expect_equal("reset-aborted command returns idle", 0x80, bench.status());
    bench.expect_equal("aborted WRITE has no queued media write", 0,
                       bench.dut().sd_wr);
    issue_rw_data(bench, 0x46, 0, 1);
    wait_for_sector_read_request(bench, 1);
    bench.service_read_request();
    // Observe after the complete host fill, with its write port inactive.
    // Run one normal 8-clock fdc_step() (rather than ce=0 tick()s) so the FDC
    // sees a ce pulse to clear sd_rd_sector before sdcontrol retires ACK and
    // sees sd_arbiter_idle, while still separating the RAM check by 8 clk_sys
    // edges from any dual-port collision on the earlier host write to byte zero.
    bench.fdc_step();
    bench.expect_equal("fresh READ neighbor byte 1 after aborted WRITE",
                       bench.image_byte(0x201), bench.sector_byte(1));
    bench.expect_equal("fresh READ neighbor byte 511 after aborted WRITE",
                       bench.image_byte(0x3ff), bench.sector_byte(511));
    std::cout << "WRITE/reset/fresh READ: accepted=" << unsigned(changed)
              << " original=" << unsigned(original)
              << " refreshed=" << unsigned(bench.sector_byte(0))
              << " neighbor1=" << unsigned(bench.sector_byte(1))
              << " neighbor511=" << unsigned(bench.sector_byte(511))
              << " host_write_inactive_clocks=8\n";
    bench.expect_equal("reset-aborted WRITE cannot overwrite fresh host-filled byte 0",
                       original, bench.sector_byte(0));
    bench.wait_until("fresh READ first payload byte ready after aborted WRITE", [&] {
        return (bench.status() & 0xf0) == 0xf0;
    }, 20000);
    bench.expect_equal("fresh READ returns original first payload byte", original,
                       bench.read_data());
}

void test_normal_seek_keeps_other_drive_idle() {
    Bench bench;
    mount_synthetic_edsk(bench);
    wait_for_track_ready(bench, 0);
    bench.dut().fast = 0;
    bench.expect_equal("both drives idle before normal-speed SEEK", 0,
                       bench.status() & 0x03);
    // Only drive A receives SEEK. Normal stepping provides a sustained
    // execution interval for repeated observations through the public MSR.
    issue_seek(bench, 1);
    unsigned drive_a_busy_samples = 0;
    unsigned unexpected_drive_b_busy = 0;
    for (unsigned sample = 0; sample < 64; ++sample) {
        const auto msr = bench.status();
        if (msr & 0x01) ++drive_a_busy_samples;
        unexpected_drive_b_busy |= msr & 0x02;
        bench.fdc_step();
    }
    bench.expect_equal("normal-speed SEEK supplies sustained drive-A busy samples", 1,
                       drive_a_busy_samples >= 8);
    std::cout << "SEEK isolation: command=0F 00 01 fast=0 samples=64 A_busy_samples="
              << drive_a_busy_samples << " unexpected_B_busy_mask="
              << unexpected_drive_b_busy << '\n';
    // NEC µPD765A/µPD765B datasheet, Table 2, Main Status Register:
    // D0B describes FDD0 Seek mode and D1B describes FDD1 Seek mode.
    // Drive B has received no SEEK command in this fixture.
    bench.expect_equal("drive B remains idle throughout drive-A SEEK", 0,
                       unexpected_drive_b_busy);
}

void test_completed_seek_stays_busy_until_sense_interrupt() {
    Bench bench;
    mount_synthetic_edsk(bench);
    wait_for_track_ready(bench, 0);
    bench.expect_equal("both drives idle before fast SEEK", 0,
                       bench.status() & 0x03);
    issue_seek(bench, 1);
    // Internal position/metadata observations only bound fixture setup.
    // The public SIS result below must independently prove a successful
    // completed drive-A seek before interpreting the saved MSR sample.
    wait_for_track_ready(bench, 1);
    bench.wait_until("command-ready after completed SEEK", [&] {
        return (bench.status() & 0xd0) == 0x80;
    });
    const auto before_sis = bench.status();

    bench.write_data(0x08, true);  // SENSE INTERRUPT STATUS
    bench.wait_until("SIS result available after completed SEEK", [&] {
        return (bench.status() & 0xd0) == 0xd0;
    });
    const auto st0 = bench.read_data();
    const auto pcn = bench.read_data();
    bench.wait_until("command-ready after SIS result", [&] {
        return (bench.status() & 0xd0) == 0x80;
    });
    const auto after_sis = bench.status();
    std::cout << "Completed SEEK/SIS: command=0F 00 01 fast=1 MSR_before=0x"
              << std::hex << unsigned(before_sis) << " ST0=0x" << unsigned(st0)
              << " PCN=0x" << unsigned(pcn) << " MSR_after=0x"
              << unsigned(after_sis) << std::dec << '\n';

    // NEC uPD765A datasheet, "Sense Interrupt Status", Table 9:
    // normal seek termination has IC=00/SE=1; unit0/head0 gives ST0=20.
    bench.expect_equal("SIS reports normal Seek End for drive A", 0x20, st0);
    bench.expect_equal("SIS reports completed SEEK cylinder 1", 1, pcn);
    bench.expect_equal("drive B remains idle before SIS", 0, before_sis & 0x02);
    bench.expect_equal("completed SEEK releases controller busy before SIS", 0,
                       before_sis & 0x10);
    bench.expect_equal("SIS retires drive-A busy while drive B stays idle", 0,
                       after_sis & 0x03);
    // NEC uPD765A datasheet, "Seek": D0B-D3B are set during seeking
    // and cleared by SENSE INTERRUPT STATUS, after physical stepping ends.
    bench.expect_equal("completed SEEK retains drive-A busy until SIS", 1,
                       before_sis & 0x01);
}

// Extended DSK Rev. 5 specification: Sector Information List offsets 06-07 store
// actual data length in bytes in little-endian order; each sector's offset is
// the 256-byte Track-Info header plus the sum of previous sector sizes; weak
// sectors store multiple copies with stored length as a multiple of N size.
// Retained to pin shared command/cache state and host/RAM transport when
// command selection and the background parser own different disk formats.
std::vector<std::uint8_t> build_std_dsk_image(const std::string& creator,
                                              std::uint8_t n_code,
                                              std::uint8_t xor_mask,
                                              std::uint8_t mul,
                                              std::uint8_t add) {
    const unsigned sec_bytes = 128u << n_code;
    const unsigned track_units = 1u + ((sec_bytes + 255u) / 256u);
    std::vector<std::uint8_t> img(0x100u + track_units * 0x100u, 0);
    const std::string sig = "MV - CPCEMU Disk-File\r\nDisk-Info\r\n";
    std::copy(sig.begin(), sig.end(), img.begin());
    std::copy_n(creator.begin(), std::min<std::size_t>(creator.size(), 14), img.begin() + 0x22);
    img[0x30] = 1; img[0x31] = 1; img[0x33] = static_cast<std::uint8_t>(track_units);
    const std::string tsig = "Track-Info\r\n";
    std::copy(tsig.begin(), tsig.end(), img.begin() + 0x100);
    img[0x114] = n_code; img[0x115] = 1; img[0x116] = 0x1e; img[0x117] = 0xe5;
    img[0x11a] = 1; img[0x11b] = n_code;
    for (unsigned i = 0; i < sec_bytes; ++i)
        img[0x200 + i] = static_cast<std::uint8_t>(xor_mask ^ ((i * mul + add) & 0xff));
    return img;
}

std::vector<std::uint8_t> build_edsk_drive_b_image() {
    // Track 0: Track-Info (0x100) + R1 N=1 stored 0x0200 (two 256B weak copies)
    // + R2 N=2 stored 0x0200 (512B target at 0x400..0x5ff). Track 1 (0x600..0x8ff)
    // keeps the legal LBA 3 block-boundary prefetch inside stored file bounds.
    std::vector<std::uint8_t> img(0x900, 0);
    const std::string sig = "EXTENDED CPC DSK File\r\nDisk-Info\r\n";
    const std::string creator = "EDSK-B-PROBE";
    const std::string tsig = "Track-Info\r\n";
    std::copy(sig.begin(), sig.end(), img.begin());
    std::copy(creator.begin(), creator.end(), img.begin() + 0x22);
    img[0x30] = 2; img[0x31] = 1; img[0x34] = 5; img[0x35] = 3;
    std::copy(tsig.begin(), tsig.end(), img.begin() + 0x100);
    img[0x114] = 2; img[0x115] = 2; img[0x116] = 0x1e; img[0x117] = 0xe5;
    img[0x11a] = 1; img[0x11b] = 1; img[0x11e] = 0x00; img[0x11f] = 0x02;
    img[0x122] = 2; img[0x123] = 2; img[0x126] = 0x00; img[0x127] = 0x02;
    for (unsigned i = 0; i < 256; ++i) {
        img[0x200 + i] = static_cast<std::uint8_t>((i * 29 + 0x11) & 0x7f);
        img[0x300 + i] = static_cast<std::uint8_t>((i * 31 + 0x41) & 0x7f);
    }
    for (unsigned i = 0; i < 512; ++i)
        img[0x400 + i] = (i == 0) ? 0x80 : static_cast<std::uint8_t>(0x81 + ((i * 37) % 127));
    std::copy(tsig.begin(), tsig.end(), img.begin() + 0x600);
    img[0x610] = 1; img[0x614] = 2; img[0x615] = 1; img[0x616] = 0x1e; img[0x617] = 0xe5;
    img[0x618] = 1; img[0x61a] = 1; img[0x61b] = 2; img[0x61e] = 0x00; img[0x61f] = 0x02;
    for (unsigned i = 0; i < 512; ++i)
        img[0x700 + i] = static_cast<std::uint8_t>(0x40 + (i & 0x3f));
    return img;
}

struct EdskDriveContextRun {
    Bench bench;
    std::vector<std::uint8_t> std_a, std_b, edsk_b;
    const std::vector<std::uint8_t>* b_active = nullptr;
    unsigned ce = 0;
    static constexpr unsigned max_ce = 500000;

    EdskDriveContextRun()
        : std_a(build_std_dsk_image("STD-A-PRIME", 2, 0xa0, 13, 0x11)),
          std_b(build_std_dsk_image("STD-B-PRIME", 1, 0x55, 17, 0x23)),
          edsk_b(build_edsk_drive_b_image()) {
        check(std_a.size() == 0x400 && std_b.size() == 0x300 && edsk_b.size() == 0x900,
              "unexpected synthetic image size");
        const unsigned r1_len = edsk_b[0x11e] | (unsigned(edsk_b[0x11f]) << 8);
        const unsigned r2_len = edsk_b[0x126] | (unsigned(edsk_b[0x127]) << 8);
        const unsigned r2_base = 0x100u + 0x100u + r1_len;
        check(r1_len == 0x200 && r2_len == 0x200 && r2_base == 0x400 &&
                  r2_base + r2_len <= edsk_b.size(),
              "EDSK B stored offsets violate expected layout/bounds");
        for (unsigned i = 0; i < 256; ++i) {
            check(edsk_b[0x300 + i] != edsk_b[0x400 + i], "R1 copy 1 aliases R2 first half");
            check(edsk_b[0x400] != edsk_b[0x500 + i], "R2 byte 0 aliases R2 second half");
        }
        bench.reset();
        bench.dut().ready = 3;
        bench.dut().motor = 0;
        bench.dut().fast = 0;
    }

    auto& root() { return *bench.dut().rootp; }
    unsigned state() { return root().u765_test__DOT__u765__DOT__fdc__DOT__state; }
    unsigned ds0() { return root().u765_test__DOT__u765__DOT__ds0; }
    void check(bool ok, const std::string& why) { if (!ok) throw TestFailure(why); }
    void qualify(bool ok, const std::string& why) { if (!ok) throw TestFailure("inconclusive: " + why); }
    void step() { qualify(ce < max_ce, "500000 ce budget exceeded"); bench.fdc_step(); ++ce; }

    void wait_status(std::uint8_t mask, std::uint8_t val, unsigned limit, const char* why) {
        for (unsigned w = 0; w < limit && (bench.status() & mask) != val; ++w) step();
        check((bench.status() & mask) == val, why);
    }

    void write(std::uint8_t value) {
        wait_status(0xc0, 0x80, 2000, "CPU command byte was not RQM/DIO qualified");
        bench.write_data(value, true);
        ce += 2;
    }

    std::uint8_t read() { const auto v = bench.read_data(); ce += 2; return v; }

    void service_host_read(unsigned mask, unsigned lba) {
        check(mask == 1 || mask == 2, "host request is not one-hot");
        const auto& image = (mask == 1) ? std_a : *b_active;
        const std::uint64_t base = static_cast<std::uint64_t>(lba) * 512ULL;
        check(base < image.size(), "host request LBA exceeds stored file bounds");
        if (mask == 2 && b_active == &edsk_b)
            check(base + 512ULL <= image.size(), "EDSK B request block exceeds stored file bounds");
        auto& d = bench.dut();
        check(d.sd_ack == 0, "ACK unexpectedly high before host service");
        for (unsigned i = 0; i < 4; ++i) {
            check(d.sd_rd == mask && d.sd_lba == lba && !d.sd_wr, "held request changed before ACK");
            step();
        }
        check(d.sd_rd == mask && d.sd_lba == lba && !d.sd_wr, "held request changed on final step");
        std::cout << "HOST mask=" << mask << " lba=" << lba << " ce=" << ce << " stable_ce=4\n";
        d.sd_ack = 1; d.sd_buff_wr = 1;
        for (unsigned i = 0; i < 512; ++i) {
            d.sd_buff_addr = i;
            d.sd_buff_dout = (base + i < image.size()) ? image[base + i] : 0;
            bench.tick();
        }
        d.sd_buff_wr = 0; d.sd_ack = 0; bench.tick();
        step();
    }

    bool drive_quiescent(unsigned drive) {
        auto& r = root();
        return bench.image_ready(drive) && !bench.trackinfo_dirty(drive) &&
               r.u765_test__DOT__u765__DOT__fdc__DOT__image_scan_state[drive] == 0 &&
               r.u765_test__DOT__u765__DOT__fdc__DOT__seek_state[drive] == 0 &&
               r.u765_test__DOT__u765__DOT__fdc__DOT__i_secinfo_valid[drive][0] &&
               r.u765_test__DOT__u765__DOT__fdc__DOT__i_secinfo_valid[drive][1] &&
               !r.u765_test__DOT__u765__DOT__tinfo_lock &&
               r.u765_test__DOT__u765__DOT__fdc__DOT__sector_search_state == 0 &&
               !bench.mount_busy() && !r.u765_test__DOT__u765__DOT__sd_busy_tinfo &&
               !r.u765_test__DOT__u765__DOT__sd_busy_sector &&
               bench.status() == 0x80 && !bench.dut().sd_rd && !bench.dut().sd_wr;
    }

    void mount_quiesce(unsigned drive, const std::vector<std::uint8_t>& image, const char* tag) {
        const unsigned mask = 1u << drive;
        if (drive == 1) b_active = &image;
        bench.dut().img_size = static_cast<std::uint32_t>(image.size());
        bench.dut().img_mounted = static_cast<std::uint8_t>(mask);
        bench.tick();
        bench.dut().img_mounted = 0;
        std::cout << "MOUNT mask=" << mask << " tag=" << tag << " ce=" << ce << "\n";
        unsigned setup_reads = 0;
        for (unsigned i = 0; i < 6000; ++i) {
            if (bench.dut().sd_rd) {
                check(bench.dut().sd_rd == mask && bench.dut().sd_lba == 0,
                      "setup request mask/LBA differs from header or Track-Info");
                service_host_read(mask, 0);
                ++setup_reads;
            } else {
                step();
            }
            if (drive_quiescent(drive)) {
                qualify(setup_reads >= 2, "mount reached ready without both header and Track-Info reads");
                std::cout << "QUIESCENT mask=" << mask << " tag=" << tag
                          << " reads=" << setup_reads << " ce=" << ce << "\n";
                return;
            }
        }
        qualify(false, std::string(tag) + " mount/parser did not reach quiescence");
    }

    void select_drive(unsigned drive) {
        check((bench.status() & 0xd3) == 0x80, "controller or seek busy before SENSE DRIVE STATUS");
        write(0x04);
        wait_status(0xd0, 0x90, 2000, "SENSE DRIVE STATUS parameter phase not reached");
        write(static_cast<std::uint8_t>(drive & 1));
        wait_status(0xd0, 0xd0, 2000, "SENSE DRIVE STATUS result phase not reached");
        const auto st3 = read();
        check((st3 & 0x3f) == (0x30u | (drive & 1)), "ST3 unexpected after SENSE DRIVE STATUS");
        wait_status(0xff, 0x80, 64, "controller did not return idle after SENSE DRIVE STATUS");
        qualify(ds0() == (drive & 1), "command ds0 did not match SENSE DRIVE STATUS selection");
        std::cout << "SELECT ds0=" << ds0() << " st3=" << unsigned(st3) << " ce=" << ce << "\n";
    }

    void wait_r2_cache_with_context(unsigned expected_ds0) {
        auto& r = root();
        bench.dut().motor = 2; // start only drive B motor
        for (unsigned i = 0; i < 200000; ++i) {
            check(!bench.dut().sd_rd && !bench.dut().sd_wr, "unexpected host request during B rotation");
            qualify(ds0() == expected_ds0, "command ds0 changed during B EDSK R2 cache construction");
            if (r.u765_test__DOT__u765__DOT__fdc__DOT__i_secinfo_valid[1][0] &&
                r.u765_test__DOT__u765__DOT__fdc__DOT__i_current_sector_pos[1][0] == 1) {
                qualify(r.u765_test__DOT__u765__DOT__fdc__DOT__sector_search_state == 0 &&
                        !r.u765_test__DOT__u765__DOT__tinfo_lock &&
                        r.u765_test__DOT__u765__DOT__fdc__DOT__sector_r[1][0] == 2 &&
                        r.u765_test__DOT__u765__DOT__fdc__DOT__sector_byte_pos[1][0] < 10,
                        "R2 cache precondition missed clean sector-start window");
                std::cout << "R2_CACHED ce=" << ce << " ds0=" << ds0()
                          << " cached_len=" << unsigned(r.u765_test__DOT__u765__DOT__fdc__DOT__sector_length[1][0])
                          << " cached_off=" << unsigned(r.u765_test__DOT__u765__DOT__fdc__DOT__sector_offset[1][0])
                          << " byte_pos=" << unsigned(r.u765_test__DOT__u765__DOT__fdc__DOT__sector_byte_pos[1][0])
                          << "\n";
                return;
            }
            step();
        }
        qualify(false, "drive B did not reach R2 cache construction within 200000 ce");
    }

    void issue_b_read_r2_and_verify(const char* variant) {
        for (auto v : std::array<std::uint8_t, 9>{0x46, 0x01, 0x00, 0x00, 0x02, 0x02, 0x02, 0x1e, 0xff})
            write(v);
        bool exec_entered = false;
        for (unsigned i = 0; i < 64; ++i) {
            if (state() == 8 || state() == 9) {
                check((bench.status() & 0xc0) == 0x40, "accepted B read lacks read direction/RQM-low phase");
                std::cout << "B_ACCEPTED ce=" << ce << " state=" << state()
                          << " msr=" << unsigned(bench.status()) << "\n";
                exec_entered = true;
                break;
            }
            step();
        }
        qualify(exec_entered, "B READ R2 did not pass EXEC1 within 64 ce");

        bool first_served = false;
        for (unsigned wait = 0; wait < 20000 && !first_served; ++wait) {
            auto& d = bench.dut();
            check(!d.sd_wr, "unexpected media write before B READ payload");
            if (d.sd_rd) {
                check(d.sd_rd == 2 && (d.sd_lba == 1 || d.sd_lba == 2),
                      "initial B READ host request selected unexpected drive/LBA");
                service_host_read(2, d.sd_lba);
                first_served = true;
            } else {
                qualify((bench.status() & 0xf0) != 0xf0, "B data ready before any host response");
                step();
            }
        }
        check(first_served, "B READ host request did not arrive");

        unsigned correct = 0, first_bad = 512, first_actual = 0, first_expected = 0;
        for (unsigned i = 0; i < 512; ++i) {
            bool ready = false;
            for (unsigned wait = 0; wait < 2000; ++wait) {
                auto& d = bench.dut();
                check(!d.sd_wr, "unexpected media write during B READ payload");
                if (d.sd_rd) {
                    check(d.sd_rd == 2 && d.sd_lba == 2, "mid-payload host request selected unexpected drive/LBA");
                    service_host_read(2, 2);
                }
                if ((bench.status() & 0xf0) == 0xf0) { ready = true; break; }
                step();
            }
            check(ready, "timeout waiting for B READ payload byte");
            const auto got = read();
            const auto expected = edsk_b[0x400 + i];
            if (got == expected) ++correct;
            else if (first_bad == 512) { first_bad = i; first_actual = got; first_expected = expected; }
        }
        std::cout << "B_SCORE correct=" << correct << "/512 first_bad=" << first_bad
                  << " expected=" << first_expected << " actual=" << first_actual << " ce=" << ce << "\n";

        unsigned extra_reads = 0;
        bool completed = false;
        for (unsigned wait = 0; wait < 200000; ++wait) {
            if (bench.dut().sd_rd) {
                check(bench.dut().sd_rd == 2 && bench.dut().sd_lba == 3 && extra_reads == 0,
                      "unexpected request during B completion");
                service_host_read(2, 3);
                ++extra_reads;
            }
            if ((bench.status() & 0xf0) == 0xd0) {
                std::array<unsigned, 7> results{};
                for (unsigned i = 0; i < 7; ++i) {
                    check((bench.status() & 0xf0) == 0xd0, "B result byte lacks result-phase RQM");
                    results[i] = read();
                }
                check((results[1] & 0x10) == 0, "B returned overrun");
                // Other status/CHRN bytes are recorded, not graded by this
                // file-format ownership test; automatic EOT remains separate.
                wait_status(0x10, 0x00, 32, "B did not release command busy");
                std::cout << "B_COMPLETE ce=" << ce << " eof_prefetches=" << extra_reads << " results";
                for (auto b : results) std::cout << ' ' << b;
                std::cout << '\n';
                completed = true;
                break;
            }
            step();
        }
        qualify(completed, "B result wait exceeded 200000 ce");
        check(correct == 512, std::string(variant) + " B R2 payload differs from independent EDSK 0x0400..0x05ff");
    }
};

void run_edsk_drive_context_variant(bool control) {
    EdskDriveContextRun r;
    auto& root = r.root();
    r.mount_quiesce(0, r.std_a, "STD-A");
    r.mount_quiesce(1, r.std_b, "STD-B");
    r.qualify(!root.u765_test__DOT__u765__DOT__fdc__DOT__image_edsk[0] &&
              !root.u765_test__DOT__u765__DOT__fdc__DOT__image_edsk[1] &&
              root.u765_test__DOT__u765__DOT__fdc__DOT__sector_length[1][0] == 0x0100 &&
              root.u765_test__DOT__u765__DOT__fdc__DOT__i_current_sector_pos[1][0] == 0,
              "standard DSK prime did not leave B sector_length=0x0100 via parse");

    const unsigned cache_ds0 = control ? 1u : 0u;
    r.select_drive(cache_ds0);
    r.mount_quiesce(1, r.edsk_b, "EDSK-B");
    r.qualify(!root.u765_test__DOT__u765__DOT__fdc__DOT__image_edsk[0] &&
              root.u765_test__DOT__u765__DOT__fdc__DOT__image_edsk[1] && r.ds0() == cache_ds0,
              "EDSK B remount did not preserve expected image_edsk/ds0 precondition");

    r.wait_r2_cache_with_context(cache_ds0);
    const char* variant = control ? "control" : "suspect";
    r.issue_b_read_r2_and_verify(variant);
    std::cout << "PASS " << variant << " B READ R2 payload512\n";
}

void test_edsk_r2_read_preserves_drive_context_after_sds() {
    run_edsk_drive_context_variant(true);
    run_edsk_drive_context_variant(false);
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
        run("u765_read_data_production_shaped_first_byte",
            test_read_data_production_shaped_first_byte);
        run("u765_write_data_uninterrupted_payload",
            test_write_data_uninterrupted_payload);
        run("u765_reset_after_write_byte_preserves_fresh_read",
            test_reset_after_write_byte_preserves_fresh_read);
        run("u765_normal_seek_keeps_other_drive_idle",
            test_normal_seek_keeps_other_drive_idle);
        run("u765_completed_seek_stays_busy_until_sense_interrupt",
            test_completed_seek_stays_busy_until_sense_interrupt);
        run("u765_edsk_r2_read_preserves_drive_context_after_sds",
            test_edsk_r2_read_preserves_drive_context_after_sds);
    } catch (...) {
        return 1;
    }

    std::cout << "Summary: " << passed << " passed, 0 failed\n";
    return 0;
}
