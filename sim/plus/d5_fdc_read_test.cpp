// Execute a synthetic READ DATA guest on the generated production T80.
// Reuse the existing P10 SDRAM/media host service. Expected bytes come only
// from the tracked EDSK descriptors and input payload, before media mutation.
#define main p10_unused_main
#include "p10_boot_test.cpp"
#undef main

namespace {
constexpr uint16_t kStatusPort = 0xfb7e;
constexpr uint16_t kDataPort = 0xfb7f;
constexpr uint16_t kPayload = 0x8000;
constexpr uint16_t kResults = 0x8200;
constexpr uint16_t kSuccess = 0x82ff;
constexpr uint32_t kRamBase = 0x20000;

struct SectorReference {
    size_t offset = 0;
    std::array<uint8_t, 512> bytes{};
};

SectorReference sector_reference(const std::vector<uint8_t>& image) {
    const std::string signature = "EXTENDED CPC DSK File\r\nDisk-Info\r\n";
    require(image.size() >= 512 &&
                std::equal(signature.begin(), signature.end(), image.begin()),
            "fixture input is not an extended CPC DSK image");
    const unsigned tracks = image[0x30], sides = image[0x31];
    require(tracks && (sides == 1 || sides == 2) && tracks * sides <= 204,
            "invalid EDSK track/side size table");
    // Physical track0/head0 is the first entry in the EDSK size table.
    const size_t track = 256, track_size = size_t(image[0x34]) * 256;
    const std::string track_signature = "Track-Info\r\n";
    require(track_size >= 256 && track + track_size <= image.size() &&
                std::equal(track_signature.begin(), track_signature.end(), image.begin() + track) &&
                image[track + 0x10] == 0 && image[track + 0x11] == 0,
            "EDSK physical track0/head0 is missing or truncated");
    const unsigned sectors = image[track + 0x15];
    require(0x18 + sectors * 8 <= 256, "EDSK sector descriptors exceed track header");
    size_t payload = track + 256;
    unsigned matches = 0;
    SectorReference result;
    for (unsigned slot = 0; slot < sectors; ++slot) {
        const size_t descriptor = track + 0x18 + slot * 8;
        const size_t length = unsigned(image[descriptor + 6]) |
                              (unsigned(image[descriptor + 7]) << 8);
        require(payload + length <= track + track_size,
                "EDSK sector payload exceeds the physical track block");
        if (image[descriptor] == 0 && image[descriptor + 1] == 0 &&
            image[descriptor + 2] == 0x41 && image[descriptor + 3] == 2) {
            require(length == result.bytes.size(), "selected EDSK sector is not 512 bytes");
            result.offset = payload;
            std::copy_n(image.begin() + payload, result.bytes.size(), result.bytes.begin());
            ++matches;
        }
        payload += length;
    }
    require(matches == 1, "EDSK needs exactly one C0/H0/R41/N2 sector");
    return result;
}

struct Guest {
    std::vector<uint8_t> page;
    size_t used = 0;
    uint16_t halt_pc = 0;
};

Guest read_guest() {
    Guest guest;
    auto& code = guest.page;
    code = {0xc3, 0x00, 0x01}; // JP0100, below payload/result RAM
    code.resize(0x100, 0x76);
    auto emit = [&](std::initializer_list<uint8_t> bytes) {
        require(code.size() + bytes.size() <= 16384,
                "synthetic READ DATA guest exceeds its 16KiB CPR page");
        code.insert(code.end(), bytes);
    };
    auto poll = [&](uint8_t mask, uint8_t expected) {
        const size_t start = code.size();
        emit({0xed, 0x78, 0xe6, mask, 0xfe, expected, 0x20, 0});
        const int displacement = int(start) - int(code.size());
        require(displacement >= -128 && displacement <= 127,
                "RQM poll branch exceeds relative range");
        code.back() = uint8_t(displacement);
    };
    auto read_to = [&](uint16_t address, uint8_t phase) {
        poll(0xf0, phase);
        emit({0x0c, 0xed, 0x78, 0x32, uint8_t(address), uint8_t(address >> 8), 0x0d});
    };
    emit({0xf3, 0x31, 0x00, 0xc0}); // DI; SP=C000
    emit({0x01, 0x7e, 0xfa, 0x3e, 1, 0xed, 0x79}); // motor, FA7E
    const std::array<uint8_t, 9> command{0x46, 0, 0, 0, 0x41, 2, 0x41, 0x1e, 0xff};
    for (uint8_t byte : command) {
        emit({0x01, 0x7e, 0xfb}); // canonical status FB7E
        poll(0xc0, 0x80); // RQM=1, CPU-to-controller direction
        emit({0x0c, 0x3e, byte, 0xed, 0x79});
    }
    emit({0x01, 0x7e, 0xfb});
    // Straight-line service keeps the ready-to-data latency bounded. This
    // preserves the existing P10 discriminator's instruction path while
    // changing its CPU owner to the actual T80. Guest waits are host-bounded.
    for (unsigned index = 0; index < 512; ++index)
        read_to(kPayload + index, 0xf0);
    for (unsigned index = 0; index < 7; ++index)
        read_to(kResults + index, 0xd0);
    emit({0x3e, 0xa5, 0x32, uint8_t(kSuccess), uint8_t(kSuccess >> 8)});
    guest.halt_pc = uint16_t(code.size());
    emit({0x76});
    guest.used = code.size();
    code.resize(16384, 0x76);
    return guest;
}

void run_read(bool corrupt_media) {
    Harness h;
#ifdef FDC_PRODUCTION_T80
    h.dut.d5_key = 0;
    h.dut.d5_tape_in = 0;
#endif
    h.dut.production_clocking = 1;
    h.initialize();
    h.load_disk("../../rtl/u765/test.dsk");
    const auto reference = sector_reference(h.disk_image);
    if (corrupt_media) h.disk_image[reference.offset + 37] ^= 0x80;
    h.mount_disk();
    const auto guest = read_guest();
    std::cout << "INPUT sector_offset=" << reference.offset << " bytes=" << reference.bytes.size()
              << " guest_bytes=" << guest.used << " CPR_page=" << guest.page.size()
              << " corrupt_media=" << corrupt_media << std::endl;
    h.download(build_cpr_image({{"cb00", guest.page}}));
    wait_for_cpr_apply(h);
    const size_t media_start = h.fdc_sd_lbas.size();
    bool halted = false;
    for (uint64_t n = 0; n < 30000000; ++n) {
        h.tick();
        if (h.fdc_success && m1_memory_read(h) && h.dut.dbg_addr == guest.halt_pc) {
            halted = true;
            break;
        }
    }
    require(halted, "production READ DATA guest did not reach its completion/HALT marker");
    h.run_cycles(256); // drain the final physical SDRAM write
    require(h.dut.dbg_motor, "canonical FA7E write did not enable drive A motor");
    require(std::find(h.fdc_sd_lbas.begin() + media_start, h.fdc_sd_lbas.end(),
                      uint32_t(reference.offset / 512)) != h.fdc_sd_lbas.end(),
            "READ DATA did not request the descriptor-derived payload LBA");
    const std::array<uint8_t, 9> command{0x46, 0, 0, 0, 0x41, 2, 0x41, 0x1e, 0xff};
    require(h.fdc_writes.size() == command.size() &&
                std::equal(command.begin(), command.end(), h.fdc_writes.begin()),
            "executed CPU command differs from synthetic READ DATA");

    std::array<uint8_t, 7> result_latches{};
    unsigned data_reads = 0, status_reads = 0;
    uint8_t last_status = 0;
    bool polled = false;
    for (const auto& sample : h.fdc_cpu_latch_reads) {
        if (sample.addr == kStatusPort) {
            last_status = sample.cpu_di;
            polled = true;
            ++status_reads;
        } else {
            require(sample.addr == kDataPort, "FDC latch used an unexpected port alias");
            const uint8_t phase = data_reads < 512 ? 0xf0 : 0xd0;
            require(polled && (last_status & 0xf0) == phase,
                    "FDC data read lacks preceding RQM-qualified execution/result poll at byte " +
                    std::to_string(data_reads));
            if (data_reads >= 512 && data_reads < 519)
                result_latches[data_reads - 512] = sample.cpu_di;
            polled = false;
            ++data_reads;
        }
    }
    require(data_reads == 519, "CPU did not perform 512 payload plus 7 result reads");
    require(status_reads >= data_reads, "CPU/FDC status polling was not observed");

    unsigned physical_payload_writes = 0, physical_result_writes = 0;
    for (const auto& write : h.bank0_writes) {
        if (write.byte_address >= kRamBase + kPayload &&
            write.byte_address < kRamBase + kPayload + 512) ++physical_payload_writes;
        if (write.byte_address >= kRamBase + kResults &&
            write.byte_address < kRamBase + kResults + 7) ++physical_result_writes;
    }
    require(physical_payload_writes == 512 && physical_result_writes == 7,
            "physical SDRAM writes did not contain exactly 512 payload and 7 result bytes");
    for (unsigned index = 0; index < reference.bytes.size(); ++index) {
        require(h.fdc_payload_seen[index], "CPU payload store absent at byte " + std::to_string(index));
        const auto stored = h.memory.find(kRamBase + kPayload + index);
        require(stored != h.memory.end(), "physical payload RAM byte missing");
        require(stored->second == reference.bytes[index],
                "CPU RAM payload mismatch at byte " + std::to_string(index) +
                " expected=" + std::to_string(reference.bytes[index]) +
                " actual=" + std::to_string(stored->second));
    }
    std::cout << "RAW RESULTS" << std::hex;
    for (unsigned index = 0; index < 7; ++index) {
        require(h.fdc_result_seen[index], "CPU result store absent");
        const auto stored = h.memory.find(kRamBase + kResults + index);
        require(stored != h.memory.end() && stored->second == h.fdc_results[index],
                "result byte did not reach physical SDRAM");
        require(stored->second == result_latches[index],
                "result RAM differs from CPU-latched FDC byte " + std::to_string(index));
        std::cout << ' ' << unsigned(stored->second);
    }
    std::cout << std::dec << std::endl;
    require((h.fdc_results[1] & 0x10) == 0, "READ DATA returned ST1 overrun");
    require(h.memory.at(kRamBase + kSuccess) == 0xa5,
            "guest success marker did not reach physical SDRAM");
    std::cout << "PASS production-T80 FDC payload512 results7 RQM_polls=" << status_reads
              << " data_reads=" << data_reads << " physical_stores=" << physical_payload_writes
              << " media_requests=" << h.fdc_sd_reads << " LBAs";
    for (unsigned lba : h.fdc_sd_lbas) std::cout << ' ' << lba;
    std::cout << std::endl;
}
} // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    try {
        require(argc == 1 || (argc == 2 && std::string(argv[1]) == "--corrupt-media"),
                "usage d5_fdc_read [--corrupt-media]");
        run_read(argc == 2);
    } catch (const std::exception& error) {
        std::cerr << "FDC acceptance " << error.what() << std::endl;
        return 1;
    }
}
