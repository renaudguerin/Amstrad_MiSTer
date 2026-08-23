#include "Vp0_boot_test_top.h"
#include "verilated.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr uint8_t CMD_ACTIVE = 0b011;
constexpr uint8_t CMD_READ = 0b101;
constexpr uint8_t CMD_WRITE = 0b100;
constexpr unsigned kIoctlWaitLimit = 20000000;

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void require(bool condition, const std::string &message) {
    if (!condition) throw TestFailure(message);
}

struct Chunk {
    std::string id;
    std::vector<uint8_t> data;
};

// Minimal CPR RIFF image builder (same envelope rules as the parser bench).
std::vector<uint8_t> build_cpr_image(const std::vector<Chunk> &chunks) {
    std::vector<uint8_t> image{'R', 'I', 'F', 'F'};
    uint32_t riff_len = 4;
    for (const auto &chunk : chunks) {
        riff_len += 8 + static_cast<uint32_t>(chunk.data.size());
        if (chunk.data.size() % 2 != 0) ++riff_len;
    }
    for (int i = 0; i < 4; ++i)
        image.push_back(static_cast<uint8_t>((riff_len >> (8 * i)) & 0xff));
    image.push_back('A');
    image.push_back('m');
    image.push_back('s');
    image.push_back('!');
    for (const auto &chunk : chunks) {
        for (size_t i = 0; i < 4; ++i)
            image.push_back(i < chunk.id.size() ? static_cast<uint8_t>(chunk.id[i]) : 0x20);
        const uint32_t len = static_cast<uint32_t>(chunk.data.size());
        for (int i = 0; i < 4; ++i)
            image.push_back(static_cast<uint8_t>((len >> (8 * i)) & 0xff));
        image.insert(image.end(), chunk.data.begin(), chunk.data.end());
        if (chunk.data.size() % 2 != 0) image.push_back(0x00);
    }
    return image;
}

class Harness {
public:
    Vp0_boot_test_top dut;
    std::unordered_map<uint32_t, uint8_t> memory;
    uint64_t cycles = 0;

    Harness() {
        dut.clk = 0;
        dut.clkref = 0;
        dut.init = 1;
        dut.reset = 0;
        dut.cpr_download = 0;
        dut.ioctl_wr = 0;
        dut.ioctl_addr = 0;
        dut.ioctl_dout = 0;
        dut.cpu_valid = 0;
        dut.cpu_page = 0;
        dut.cpu_offset = 0;
        dut.use_mmu = 0;
        dut.mmu_mem_rd = 0;
        dut.mmu_A = 0;
        dut.memory_dq = 0;
        dut.memory_dq_oe = 0;
        dut.eval();
    }

    ~Harness() { dut.final(); }

    void tick() {
        dut.clkref = ((cycles & 7U) == 0U);
        dut.memory_dq_oe = read_drive_cycles > 0;
        dut.memory_dq = read_word;

        dut.clk = 0;
        dut.eval();
        dut.clk = 1;
        dut.eval();

        bool started_read = false;
        const uint8_t cmd = command();
        if (cmd == CMD_ACTIVE) {
            active_row = dut.sdram_a & 0x1fffU;
            active_bank = dut.sdram_ba;
        } else if (cmd == CMD_READ) {
            const uint32_t address = command_address();
            read_word = static_cast<uint16_t>(load(active_bank, address)) |
                        (static_cast<uint16_t>(load(active_bank, address + 1)) << 8);
            read_drive_cycles = 4;
            started_read = true;
            if (active_bank == 3) ++physical_cart_reads;
        } else if (cmd == CMD_WRITE) {
            const uint32_t address = command_address();
            const uint16_t data = dut.observed_dq;
            if (!dut.sdram_dqml) store(active_bank, address, data & 0xffU);
            if (!dut.sdram_dqmh) store(active_bank, address + 1, data >> 8);
        }
        if (read_drive_cycles > 0 && !started_read) --read_drive_cycles;

        dut.clk = 0;
        dut.eval();
        ++cycles;
    }

    void initialize() {
        for (int i = 0; i < 16; ++i) tick();
        dut.init = 0;
        bool reached_normal = false;
        for (int i = 0; i < 400; ++i) {
            tick();
            if (dut.debug_q == 0) {
                reached_normal = true;
                break;
            }
        }
        require(reached_normal, "SDRAM controller must finish initialization");
        for (int i = 0; i < 16; ++i) tick();
    }

    // Machine reset pulse on the parser/service domain.
    void machine_reset(unsigned cycles_high = 3) {
        dut.reset = 1;
        for (unsigned i = 0; i < cycles_high; ++i) tick();
        dut.cpr_download = 0;
        dut.ioctl_wr = 0;
        for (unsigned i = 0; i < 2; ++i) tick();
        dut.reset = 0;
        for (unsigned i = 0; i < 4; ++i) tick();
    }

    // Feed a whole CPR through the ioctl protocol: one write strobe per
    // byte, holding off while the parser asserts ioctl_wait.
    void download(const std::vector<uint8_t> &image) {
        require(!dut.image_valid, "download precondition: no published image");
        dut.cpr_download = 1;
        tick();
        while (dut.ioctl_wait) tick();
        for (size_t i = 0; i < image.size(); ++i) {
            dut.ioctl_addr = static_cast<uint32_t>(i);
            dut.ioctl_dout = image[i];
            dut.ioctl_wr = 1;
            tick();
            dut.ioctl_wr = 0;
            unsigned guard = 0;
            while (dut.ioctl_wait) {
                tick();
                require(++guard < kIoctlWaitLimit, "ioctl backpressure never released");
            }
        }
        dut.cpr_download = 0;
        tick();
        unsigned guard = 0;
        while (dut.service_busy && guard < 200000) {
            tick();
            ++guard;
        }
        require(!dut.service_busy, "service stayed busy after the download ended");
    }

    // One CPU cartridge read through the held request/acknowledge port.
    uint8_t cpu_read(uint8_t page, uint16_t offset) {
        const uint64_t start_cycles = cycles;
        const size_t reads_before = physical_cart_reads;
        dut.cpu_page = page;
        dut.cpu_offset = offset;
        dut.cpu_valid = 1;
        bool ready_seen = false;
        int ready_pulses = 0;
        while (!ready_seen) {
            tick();
            if (dut.cpu_ready) {
                ++ready_pulses;
                ready_seen = true;
                last_read_data = dut.cpu_data;
            }
            require(cycles - start_cycles < 500,
                    "CPU cartridge read timed out (page " + std::to_string(page) + ")");
        }
        dut.cpu_valid = 0;
        tick();
        require(ready_pulses == 1, "cpu_ready must be a single-cycle completion");
        if (!dut.image_valid) {
            require(physical_cart_reads == reads_before,
                    "read with no published image must not touch SDRAM");
        }
        return last_read_data;
    }

    size_t physical_cart_reads = 0;

private:
    uint32_t key(uint8_t bank, uint32_t byte_address) const {
        return (static_cast<uint32_t>(bank) << 23) | (byte_address & 0x7fffffU);
    }
    uint8_t load(uint8_t bank, uint32_t byte_address) const {
        const auto it = memory.find(key(bank, byte_address));
        return it == memory.end() ? 0xff : it->second;
    }
    void store(uint8_t bank, uint32_t byte_address, uint8_t value) {
        memory[key(bank, byte_address)] = value;
    }
    uint8_t command() const {
        return static_cast<uint8_t>((dut.sdram_nras << 2) | (dut.sdram_ncas << 1) | dut.sdram_nwe);
    }
    uint32_t command_address() const {
        return (active_row << 9) |
               ((static_cast<uint32_t>(dut.sdram_a) & 0x100U) << 14) |
               ((static_cast<uint32_t>(dut.sdram_a) & 0xffU) << 1);
    }

    uint32_t active_row = 0;
    uint8_t active_bank = 0;
    uint16_t read_word = 0xffff;
    int read_drive_cycles = 0;
    uint8_t last_read_data = 0xff;
};

std::vector<uint8_t> nominal_two_page_image() {
    std::vector<Chunk> chunks;
    std::vector<uint8_t> page0(16384);
    for (size_t i = 0; i < page0.size(); ++i) page0[i] = static_cast<uint8_t>((i * 7 + 13) & 0xff);
    chunks.push_back({"cb00", page0});
    chunks.push_back({"cb01", std::vector<uint8_t>(100, 0x42)});
    return build_cpr_image(chunks);
}

// Reset state of the whole path: no image published, CPU reads fail open
// without issuing a single physical SDRAM read.
void test_no_image_reads_fail_open() {
    Harness h;
    h.initialize();
    h.machine_reset();

    require(h.cpu_read(0, 0x0000) == 0xff, "unpublished image must read FF at page 0");
    require(h.cpu_read(31, 0x3fff) == 0xff, "unpublished image must read FF at page 31");
}

// The boot path end to end: stream a CPR in, verify publication, then read
// written bytes and zero-filled short-page tail bytes through the service.
void test_nominal_load_and_readback() {
    Harness h;
    h.initialize();
    h.machine_reset();

    const auto image = nominal_two_page_image();
    h.download(image);
    require(h.dut.image_valid, "nominal download did not publish the image");

    require(h.cpu_read(0, 0x0000) == image[20], "page 0 first byte mismatch");
    require(h.cpu_read(0, 0x0001) == image[21], "page 0 second byte mismatch");
    require(h.cpu_read(0, 0x3fff) == ((16383 * 7 + 13) & 0xff), "page 0 last byte mismatch");
    require(h.cpu_read(1, 0x0000) == 0x42, "page 1 first payload byte mismatch");
    require(h.cpu_read(1, 0x0063) == 0x42, "page 1 last payload byte mismatch");
    // Short-page zero fill comes from the service's clear sweep.
    require(h.cpu_read(1, 0x0064) == 0x00, "short page tail must read 0 after clear+load");
    require(h.cpu_read(1, 0x3fff) == 0x00, "short page final byte must read 0");

    // A held request consumed exactly once per transfer: the second read
    // must issue its own physical read.
    const size_t before = h.physical_cart_reads;
    h.cpu_read(0, 0x0002);
    require(h.physical_cart_reads > before, "second CPU read issued no physical read");
}

// Malformed envelopes and malformed blocks abort the load; nothing is
// published afterwards and the bridge keeps failing open.
void test_malformed_images_abort() {
    // Bad RIFF magic.
    {
        Harness h;
        h.initialize();
        auto image = build_cpr_image({{"cb00", std::vector<uint8_t>(64, 0x01)}});
        image[0] = 'X';
        h.download(image);
        require(!h.dut.image_valid, "bad magic must not publish");
        require(h.cpu_read(0, 0) == 0xff, "aborted load must leave the bridge open-bus");
    }

    // cbNN chunk declaring more than one page (A5a decision, end to end).
    {
        Harness h;
        h.initialize();
        const auto image =
            build_cpr_image({{"cb05", std::vector<uint8_t>(20000, 0x5a)}});
        h.download(image);
        require(!h.dut.image_valid, "oversized cbNN chunk must not publish");
        require(h.cpu_read(5, 0) == 0xff, "oversized-chunk abort must leave the bridge open-bus");
    }
}

// A raw machine reset mid-load clears parser state and service transients
// without committing anything; a fresh download afterwards works normally
// (reset-time cleanup ownership, docs/plus/architecture.md).
void test_reset_during_load_cleans_up() {
    Harness h;
    h.initialize();
    h.machine_reset();

    const auto image = nominal_two_page_image();

    // Start a real download, get part-way into the stream, then reset.
    h.dut.cpr_download = 1;
    h.tick();
    while (h.dut.ioctl_wait) h.tick();
    for (size_t i = 0; i < 40; ++i) {
        h.dut.ioctl_addr = static_cast<uint32_t>(i);
        h.dut.ioctl_dout = image[i];
        h.dut.ioctl_wr = 1;
        h.tick();
        h.dut.ioctl_wr = 0;
        unsigned guard = 0;
        while (h.dut.ioctl_wait) {
            h.tick();
            require(++guard < kIoctlWaitLimit, "ioctl backpressure never released");
        }
    }
    require(!h.dut.image_valid,
            "mid-download precondition: image must still be unpublished");

    // Reset arrives while cpr_download is still asserted.
    h.machine_reset(4);
    require(!h.dut.image_valid, "reset mid-load must not leave an image published");

    // A clean re-download from scratch must succeed: proves both sides
    // cleared their state instead of relying on load_abort during reset.
    h.download(image);
    require(h.dut.image_valid, "re-download after mid-load reset failed to publish");
    require(h.cpu_read(0, 0x0000) == image[20], "post-reset reload data mismatch");
    require(h.cpu_read(1, 0x0000) == 0x42, "post-reset reload page 1 mismatch");
}

// A CPU read racing an in-flight download gets no completion while the
// service is busy (P-1 arbitration: loader outranks the CPU port). The held
// request then resolves against the committed image once the load finishes,
// and the load itself is undisturbed.
void test_cpu_read_during_active_load() {
    Harness h;
    h.initialize();
    h.machine_reset();

    const auto image = nominal_two_page_image();

    auto feed_one = [&](size_t i) {
        h.dut.ioctl_addr = static_cast<uint32_t>(i);
        h.dut.ioctl_dout = image[i];
        h.dut.ioctl_wr = 1;
        h.tick();
        h.dut.ioctl_wr = 0;
        unsigned guard = 0;
        while (h.dut.ioctl_wait) {
            h.tick();
            require(++guard < kIoctlWaitLimit, "ioctl backpressure never released");
        }
    };

    h.dut.cpr_download = 1;
    h.tick();
    while (h.dut.ioctl_wait) h.tick();
    for (size_t i = 0; i < 30; ++i) feed_one(i);

    // Load is in flight: issue a CPU read and hold it. It must not complete
    // while the service is busy.
    require(!h.dut.image_valid, "image must be unpublished while loading");
    require(h.dut.service_busy, "load should be visibly busy here");
    h.dut.cpu_page = 1;
    h.dut.cpu_offset = 0x0100;
    h.dut.cpu_valid = 1;
    for (int i = 0; i < 400; ++i) {
        h.tick();
        require(!h.dut.cpu_ready, "CPU read must not complete during an active load");
    }

    // Finish the stream; publication must go ahead untouched.
    for (size_t i = 30; i < image.size(); ++i) feed_one(i);
    h.dut.cpr_download = 0;
    h.tick();
    unsigned guard = 0;
    while (h.dut.service_busy && guard < 200000) {
        h.tick();
        ++guard;
    }
    require(h.dut.image_valid, "load interrupted by a CPU read failed to commit");

    // The held request now resolves against the committed image: page 1
    // offset 0x0100 lies beyond the short chunk, so the clear sweep left
    // committed zeros there.
    bool resolved = false;
    uint8_t value = 0xff;
    guard = 0;
    while (!resolved && guard < 500) {
        h.tick();
        if (h.dut.cpu_ready) {
            resolved = true;
            value = h.dut.cpu_data;
        }
        ++guard;
    }
    h.dut.cpu_valid = 0;
    h.tick();
    require(resolved, "held CPU read never resolved after the load finished");
    require(value == 0x00, "held read must resolve against committed content");
    h.tick();

    require(h.cpu_read(1, 0x0000) == 0x42, "data after mid-load CPU read mismatch");
}

// Production seam: the MMU watchdog must not turn the guaranteed-long
// 512 KiB clear into an open-bus completion. The held bus read starts during
// the clear, remains stalled well beyond STALL_TIMEOUT, then resolves only
// after the new image is atomically published.
void test_mmu_read_waits_out_production_sized_load() {
    Harness h;
    h.initialize();
    h.machine_reset();

    const auto image = nominal_two_page_image();
    h.dut.use_mmu = 1;
    h.dut.cpr_download = 1;
    h.tick();
    for (unsigned guard = 0; !h.dut.service_busy && guard < 8; ++guard) h.tick();
    require(h.dut.service_busy, "production clear did not start");

    h.dut.mmu_A = 0x0000;
    h.dut.mmu_mem_rd = 1;
    h.tick();
    require(h.dut.mmu_cart_own && h.dut.mmu_cart_stall,
            "MMU did not claim the load-time cartridge read");

    for (unsigned clear_wait = 0; clear_wait < 1400; ++clear_wait) {
        h.tick();
        require(h.dut.service_busy,
                "production clear ended before watchdog-boundary exercise");
        require(h.dut.mmu_cart_stall,
                "MMU watchdog released the read during production clear");
    }

    for (size_t i = 0; i < image.size(); ++i) {
        h.dut.ioctl_addr = static_cast<uint32_t>(i);
        h.dut.ioctl_dout = image[i];
        h.dut.ioctl_wr = 1;
        h.tick();
        h.dut.ioctl_wr = 0;
        unsigned guard = 0;
        while (h.dut.ioctl_wait) {
            h.tick();
            require(h.dut.mmu_cart_stall,
                    "MMU released the read while cartridge bytes loaded");
            require(++guard < kIoctlWaitLimit, "ioctl backpressure never released");
        }
    }

    h.dut.cpr_download = 0;
    h.tick();
    unsigned completion_guard = 0;
    while (h.dut.mmu_cart_stall) {
        h.tick();
        require(++completion_guard < 200000,
                "held MMU read did not resolve after publication");
    }
    require(h.dut.image_valid, "load-time MMU read prevented publication");
    require(h.dut.mmu_cart_dout == image[20],
            "load-time MMU read returned open bus or stale data");

    h.dut.mmu_mem_rd = 0;
    h.tick();
    require(!h.dut.mmu_cart_own, "MMU ownership survived the bus-cycle end");
}

}  // namespace

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    try {
        test_no_image_reads_fail_open();
        std::cout << "PASS: unpublished image fails open without SDRAM traffic\n";
        test_nominal_load_and_readback();
        std::cout << "PASS: CPR download publishes and reads back incl. short-page zero fill\n";
        test_malformed_images_abort();
        std::cout << "PASS: malformed magic and oversized cbNN abort before publication\n";
        test_reset_during_load_cleans_up();
        std::cout << "PASS: machine reset mid-load cleans up; fresh download commits\n";
        test_cpu_read_during_active_load();
        std::cout << "PASS: CPU reads during an active load wait out the loader and resolve against the commit\n";
        test_mmu_read_waits_out_production_sized_load();
        std::cout << "PASS: MMU read waits out production-sized clear/load and returns committed data\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }

    std::cout << "PASS: all P0 boot integration tests\n";
    return 0;
}
