#include <verilated.h>

#include "Vplus_cpr_parser.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw TestFailure(message);
    }
}

struct WriteRecord {
    unsigned page;
    unsigned offset;
    std::uint8_t data;
};

struct Chunk {
    std::string id;
    std::vector<std::uint8_t> data;
};

// Builder for CPR RIFF byte images in memory
std::vector<std::uint8_t> build_cpr_image(const std::vector<Chunk>& chunks,
                                          bool custom_riff_len = false,
                                          std::uint32_t override_riff_len = 0,
                                          const std::string& form_type = "Ams!") {
    std::vector<std::uint8_t> image;
    image.push_back('R');
    image.push_back('I');
    image.push_back('F');
    image.push_back('F');

    std::uint32_t riff_len = 4;
    for (const auto& chunk : chunks) {
        riff_len += 8 + static_cast<std::uint32_t>(chunk.data.size());
        if (chunk.data.size() % 2 != 0) {
            riff_len += 1;
        }
    }
    if (custom_riff_len) {
        riff_len = override_riff_len;
    }

    image.push_back(static_cast<std::uint8_t>(riff_len & 0xFF));
    image.push_back(static_cast<std::uint8_t>((riff_len >> 8) & 0xFF));
    image.push_back(static_cast<std::uint8_t>((riff_len >> 16) & 0xFF));
    image.push_back(static_cast<std::uint8_t>((riff_len >> 24) & 0xFF));

    for (size_t i = 0; i < 4; ++i) {
        image.push_back(i < form_type.size() ? static_cast<std::uint8_t>(form_type[i]) : 0x20);
    }

    for (const auto& chunk : chunks) {
        for (size_t i = 0; i < 4; ++i) {
            image.push_back(i < chunk.id.size() ? static_cast<std::uint8_t>(chunk.id[i]) : 0x20);
        }
        std::uint32_t chunk_len = static_cast<std::uint32_t>(chunk.data.size());
        image.push_back(static_cast<std::uint8_t>(chunk_len & 0xFF));
        image.push_back(static_cast<std::uint8_t>((chunk_len >> 8) & 0xFF));
        image.push_back(static_cast<std::uint8_t>((chunk_len >> 16) & 0xFF));
        image.push_back(static_cast<std::uint8_t>((chunk_len >> 24) & 0xFF));

        for (std::uint8_t b : chunk.data) {
            image.push_back(b);
        }
        if (chunk.data.size() % 2 != 0) {
            image.push_back(0x00);
        }
    }
    return image;
}

class TestBench {
public:
    TestBench() {
        dut_.clk = 0;
        dut_.reset = 0;
        dut_.cpr_download = 0;
        dut_.ioctl_wr = 0;
        dut_.ioctl_addr = 0;
        dut_.ioctl_dout = 0;
        dut_.load_ready = 0;
        dut_.load_error = 0;
        dut_.eval();
    }

    ~TestBench() {
        dut_.final();
    }

    Vplus_cpr_parser& dut() { return dut_; }
    const Vplus_cpr_parser& dut() const { return dut_; }

    const std::vector<WriteRecord>& writes() const { return writes_; }
    unsigned begin_pulses() const { return begin_pulses_; }
    unsigned commit_pulses() const { return commit_pulses_; }
    unsigned abort_pulses() const { return abort_pulses_; }

    void reset_counters() {
        writes_.clear();
        begin_pulses_ = 0;
        commit_pulses_ = 0;
        abort_pulses_ = 0;
    }

    void tick(bool drive_load_ready = false, bool drive_load_error = false) {
        dut_.load_ready = drive_load_ready ? 1 : 0;
        dut_.load_error = drive_load_error ? 1 : 0;

        const bool pre_valid = (dut_.load_valid != 0);
        const unsigned pre_page = dut_.load_page;
        const unsigned pre_offset = dut_.load_offset;
        const std::uint8_t pre_data = dut_.load_data;

        dut_.clk = 1;
        dut_.eval();

        if (pre_valid && drive_load_ready) {
            writes_.push_back({pre_page, pre_offset, pre_data});
        }

        if (dut_.load_begin) {
            require(!last_begin_, "load_begin pulse was wider than 1 cycle");
            begin_pulses_++;
            last_begin_ = true;
        } else {
            last_begin_ = false;
        }

        if (dut_.load_commit) {
            require(!last_commit_, "load_commit pulse was wider than 1 cycle");
            commit_pulses_++;
            last_commit_ = true;
        } else {
            last_commit_ = false;
        }

        if (dut_.load_abort) {
            require(!last_abort_, "load_abort pulse was wider than 1 cycle");
            abort_pulses_++;
            last_abort_ = true;
        } else {
            last_abort_ = false;
        }

        if (dut_.load_valid) {
            require(dut_.ioctl_wait == 1, "ioctl_wait was not asserted while load_valid was high");
            if (last_valid_) {
                require(dut_.load_page == last_page_, "load_page changed while waiting for load_ready");
                require(dut_.load_offset == last_offset_, "load_offset changed while waiting for load_ready");
                require(dut_.load_data == last_data_, "load_data changed while waiting for load_ready");
            }
            last_valid_ = true;
            last_page_ = dut_.load_page;
            last_offset_ = dut_.load_offset;
            last_data_ = dut_.load_data;
        } else {
            last_valid_ = false;
        }

        dut_.clk = 0;
        dut_.eval();
    }

    void hard_reset() {
        dut_.reset = 1;
        tick();
        dut_.reset = 0;
        tick();
        require(dut_.load_begin == 0, "reset left load_begin active");
        require(dut_.load_commit == 0, "reset left load_commit active");
        require(dut_.load_abort == 0, "reset left load_abort active");
        require(dut_.load_valid == 0, "reset left load_valid active");
        require(dut_.ioctl_wait == 0, "reset left ioctl_wait active");
        reset_counters();
    }

    void start_download() {
        dut_.cpr_download = 1;
        tick();
        require(begin_pulses_ == 1, "cpr_download rise did not pulse load_begin");
    }

    void end_download() {
        dut_.cpr_download = 0;
        dut_.ioctl_wr = 0;
        tick();
    }

    void feed_bytes(const std::vector<std::uint8_t>& bytes,
                    unsigned wait_cycles = 1,
                    std::uint32_t start_addr = 0,
                    bool inject_load_error = false,
                    size_t error_at_byte = 0) {
        dut_.ioctl_addr = start_addr;
        for (size_t i = 0; i < bytes.size(); ++i) {
            dut_.ioctl_dout = bytes[i];
            dut_.ioctl_wr = 1;
            tick();

            while (dut_.load_valid) {
                dut_.ioctl_wr = 0;
                for (unsigned w = 0; w < wait_cycles; ++w) {
                    tick(false);
                }
                bool is_error = inject_load_error && (i >= error_at_byte);
                tick(true, is_error);
            }
            dut_.ioctl_wr = 0;
            dut_.ioctl_addr = static_cast<std::uint32_t>(dut_.ioctl_addr + 1);
        }
    }

private:
    Vplus_cpr_parser dut_;
    std::vector<WriteRecord> writes_;
    unsigned begin_pulses_ = 0;
    unsigned commit_pulses_ = 0;
    unsigned abort_pulses_ = 0;

    bool last_begin_ = false;
    bool last_commit_ = false;
    bool last_abort_ = false;
    bool last_valid_ = false;
    unsigned last_page_ = 0;
    unsigned last_offset_ = 0;
    std::uint8_t last_data_ = 0;
};

void test_nominal_multi_page() {
    TestBench tb;
    tb.hard_reset();

    std::vector<Chunk> chunks;
    for (unsigned p = 0; p < 3; ++p) {
        std::string id = "cb0" + std::to_string(p);
        std::vector<std::uint8_t> data(16384);
        for (unsigned i = 0; i < 16384; ++i) {
            data[i] = static_cast<std::uint8_t>((p * 73 + i) & 0xFF);
        }
        chunks.push_back({id, data});
    }

    auto cpr = build_cpr_image(chunks);
    tb.start_download();
    tb.feed_bytes(cpr, 2);
    tb.end_download();

    require(tb.begin_pulses() == 1, "load_begin was not pulsed once");
    require(tb.commit_pulses() == 1, "load_commit was not pulsed once on clean end");
    require(tb.abort_pulses() == 0, "load_abort was pulsed during nominal multi-page");
    require(tb.writes().size() == 3 * 16384, "write count mismatch");

    for (size_t i = 0; i < tb.writes().size(); ++i) {
        unsigned expected_page = static_cast<unsigned>(i / 16384);
        unsigned expected_offset = static_cast<unsigned>(i % 16384);
        std::uint8_t expected_data = static_cast<std::uint8_t>((expected_page * 73 + expected_offset) & 0xFF);
        require(tb.writes()[i].page == expected_page, "write page mismatch");
        require(tb.writes()[i].offset == expected_offset, "write offset mismatch");
        require(tb.writes()[i].data == expected_data, "write data mismatch");
    }
}

void test_sparse_and_out_of_order() {
    TestBench tb;
    tb.hard_reset();

    std::vector<Chunk> chunks;
    chunks.push_back({"cb15", std::vector<std::uint8_t>(16384, 0x15)});
    chunks.push_back({"cb00", std::vector<std::uint8_t>(16384, 0x00)});
    chunks.push_back({"cb31", std::vector<std::uint8_t>(16384, 0x31)});
    chunks.push_back({"cb07", std::vector<std::uint8_t>(16384, 0x07)});

    auto cpr = build_cpr_image(chunks);
    tb.start_download();
    tb.feed_bytes(cpr, 1);
    tb.end_download();

    require(tb.commit_pulses() == 1, "sparse CPR failed to commit");
    require(tb.abort_pulses() == 0, "sparse CPR aborted");
    require(tb.writes().size() == 4 * 16384, "sparse write count mismatch");

    for (size_t i = 0; i < 16384; ++i) {
        require(tb.writes()[i].page == 15 && tb.writes()[i].data == 0x15, "page 15 write mismatch");
        require(tb.writes()[16384 + i].page == 0 && tb.writes()[16384 + i].data == 0x00, "page 0 write mismatch");
        require(tb.writes()[32768 + i].page == 31 && tb.writes()[32768 + i].data == 0x31, "page 31 write mismatch");
        require(tb.writes()[49152 + i].page == 7 && tb.writes()[49152 + i].data == 0x07, "page 7 write mismatch");
    }
}

void test_duplicate_pages_last_wins() {
    TestBench tb;
    tb.hard_reset();

    std::vector<Chunk> chunks;
    chunks.push_back({"cb00", std::vector<std::uint8_t>(16384, 0xAA)});
    chunks.push_back({"cb00", std::vector<std::uint8_t>(16384, 0xBB)});

    auto cpr = build_cpr_image(chunks);
    tb.start_download();
    tb.feed_bytes(cpr, 1);
    tb.end_download();

    require(tb.commit_pulses() == 1, "duplicate chunk CPR failed to commit");
    require(tb.abort_pulses() == 0, "duplicate chunk CPR aborted");
    require(tb.writes().size() == 32768, "duplicate write count mismatch");

    for (size_t i = 0; i < 16384; ++i) {
        require(tb.writes()[i].page == 0 && tb.writes()[i].data == 0xAA, "first chunk write mismatch");
        require(tb.writes()[16384 + i].page == 0 && tb.writes()[16384 + i].data == 0xBB, "second chunk write mismatch");
    }
}

void test_short_and_oversized_blocks() {
    TestBench tb;
    tb.hard_reset();

    std::vector<Chunk> chunks;
    chunks.push_back({"cb01", std::vector<std::uint8_t>(256, 0x42)});
    chunks.push_back({"cb02", std::vector<std::uint8_t>(18000, 0x99)});

    auto cpr = build_cpr_image(chunks);
    tb.start_download();
    tb.feed_bytes(cpr, 1);
    tb.end_download();

    require(tb.commit_pulses() == 1, "short/oversized CPR failed to commit");
    require(tb.abort_pulses() == 0, "short/oversized CPR aborted");
    require(tb.writes().size() == 256 + 16384, "short/oversized forwarded write count mismatch");

    for (size_t i = 0; i < 256; ++i) {
        require(tb.writes()[i].page == 1 && tb.writes()[i].offset == i && tb.writes()[i].data == 0x42,
                "short chunk payload mismatch");
    }
    for (size_t i = 0; i < 16384; ++i) {
        require(tb.writes()[256 + i].page == 2 && tb.writes()[256 + i].offset == i && tb.writes()[256 + i].data == 0x99,
                "oversized clamped chunk payload mismatch");
    }
}

void test_metadata_chunks_even_and_odd() {
    TestBench tb;
    tb.hard_reset();

    std::vector<Chunk> chunks;
    chunks.push_back({"INFO", std::vector<std::uint8_t>(10, 0x55)});
    chunks.push_back({"cb03", std::vector<std::uint8_t>(16384, 0x33)});
    chunks.push_back({"LIST", std::vector<std::uint8_t>(15, 0x77)});
    chunks.push_back({"cb04", std::vector<std::uint8_t>(16384, 0x44)});
    chunks.push_back({"JUNK", std::vector<std::uint8_t>(3, 0x88)});

    auto cpr = build_cpr_image(chunks);
    tb.start_download();
    tb.feed_bytes(cpr, 1);
    tb.end_download();

    require(tb.commit_pulses() == 1, "metadata CPR failed to commit");
    require(tb.abort_pulses() == 0, "metadata CPR aborted");
    require(tb.writes().size() == 2 * 16384, "metadata CPR write count mismatch");

    for (size_t i = 0; i < 16384; ++i) {
        require(tb.writes()[i].page == 3 && tb.writes()[i].data == 0x33, "page 3 write mismatch");
        require(tb.writes()[16384 + i].page == 4 && tb.writes()[16384 + i].data == 0x44, "page 4 write mismatch");
    }
}

void test_bad_headers() {
    // Bad RIFF magic
    {
        TestBench tb;
        tb.hard_reset();
        auto cpr = build_cpr_image({{"cb00", std::vector<std::uint8_t>(16384, 0x01)}});
        cpr[0] = 'X';
        tb.start_download();
        tb.feed_bytes(cpr, 0);
        tb.end_download();
        require(tb.abort_pulses() == 1, "bad RIFF magic did not abort");
        require(tb.commit_pulses() == 0, "bad RIFF magic committed");
    }

    // Bad RIFF length < 4
    {
        TestBench tb;
        tb.hard_reset();
        auto cpr = build_cpr_image({{"cb00", std::vector<std::uint8_t>(16384, 0x01)}}, true, 2);
        tb.start_download();
        tb.feed_bytes(cpr, 0);
        tb.end_download();
        require(tb.abort_pulses() == 1, "bad RIFF length < 4 did not abort");
        require(tb.commit_pulses() == 0, "bad RIFF length < 4 committed");
    }

    // Bad form type ("WAVE")
    {
        TestBench tb;
        tb.hard_reset();
        auto cpr = build_cpr_image({{"cb00", std::vector<std::uint8_t>(16384, 0x01)}}, false, 0, "WAVE");
        tb.start_download();
        tb.feed_bytes(cpr, 0);
        tb.end_download();
        require(tb.abort_pulses() == 1, "bad form type did not abort");
        require(tb.commit_pulses() == 0, "bad form type committed");
    }
}

void test_bad_chunk_ids() {
    const std::vector<std::string> bad_ids = {
        "cb32", "cb99", "cb0A", "cb!0", "cba0", "cb1 ", "cb0\0"
    };

    for (const auto& bad_id : bad_ids) {
        TestBench tb;
        tb.hard_reset();
        auto cpr = build_cpr_image({{bad_id, std::vector<std::uint8_t>(100, 0x01)}});
        tb.start_download();
        tb.feed_bytes(cpr, 0);
        tb.end_download();
        require(tb.abort_pulses() == 1, "bad chunk ID " + bad_id + " did not abort");
        require(tb.commit_pulses() == 0, "bad chunk ID " + bad_id + " committed");
    }
}

void test_extent_beyond_riff() {
    // Declared chunk length extends beyond RIFF length
    {
        TestBench tb;
        tb.hard_reset();
        auto cpr = build_cpr_image({{"cb00", std::vector<std::uint8_t>(16384, 0x01)}}, true, 100);
        tb.start_download();
        tb.feed_bytes(cpr, 0);
        tb.end_download();
        require(tb.abort_pulses() == 1, "chunk extent beyond RIFF did not abort");
        require(tb.commit_pulses() == 0, "chunk extent beyond RIFF committed");
    }

    // Incomplete chunk header at RIFF boundary
    {
        TestBench tb;
        tb.hard_reset();
        // RIFF declares 16 bytes (4 for "Ams!" + 12 extra bytes, which is 8 for chunk header + 4 data bytes)
        // But we set RIFF length to 15 (less than 8 bytes for next chunk header after Ams!)
        auto cpr = build_cpr_image({{"cb00", std::vector<std::uint8_t>(4, 0x01)}}, true, 10);
        tb.start_download();
        tb.feed_bytes(cpr, 0);
        tb.end_download();
        require(tb.abort_pulses() == 1, "truncated chunk header in RIFF did not abort");
        require(tb.commit_pulses() == 0, "truncated chunk header committed");
    }
}

void test_early_download_fall() {
    // Early fall mid-header
    {
        TestBench tb;
        tb.hard_reset();
        auto cpr = build_cpr_image({{"cb00", std::vector<std::uint8_t>(16384, 0x01)}});
        tb.start_download();
        std::vector<std::uint8_t> partial(cpr.begin(), cpr.begin() + 10);
        tb.feed_bytes(partial, 0);
        tb.end_download();
        require(tb.abort_pulses() == 1, "early fall in header did not abort");
        require(tb.commit_pulses() == 0, "early fall in header committed");
    }

    // Early fall mid-payload
    {
        TestBench tb;
        tb.hard_reset();
        auto cpr = build_cpr_image({{"cb00", std::vector<std::uint8_t>(16384, 0x01)}});
        tb.start_download();
        std::vector<std::uint8_t> partial(cpr.begin(), cpr.begin() + 500);
        tb.feed_bytes(partial, 0);
        tb.end_download();
        require(tb.abort_pulses() == 1, "early fall in payload did not abort");
        require(tb.commit_pulses() == 0, "early fall in payload committed");
    }

    // Exact RIFF boundary reached but 0 blocks present
    {
        TestBench tb;
        tb.hard_reset();
        auto cpr = build_cpr_image({{"INFO", std::vector<std::uint8_t>(20, 0x01)}});
        tb.start_download();
        tb.feed_bytes(cpr, 0);
        tb.end_download();
        require(tb.abort_pulses() == 1, "zero-block CPR commit was not aborted");
        require(tb.commit_pulses() == 0, "zero-block CPR committed");
    }

    // Early fall while write is pending (load_valid == 1)
    {
        TestBench tb;
        tb.hard_reset();
        auto cpr = build_cpr_image({{"cb00", std::vector<std::uint8_t>(100, 0x01)}});
        tb.start_download();
        // Feed up to the first data byte (offset 20 is first data byte of cb00)
        std::vector<std::uint8_t> up_to_first_byte(cpr.begin(), cpr.begin() + 20);
        tb.feed_bytes(up_to_first_byte, 0);

        // Feed first data byte without pulsing load_ready
        tb.dut().ioctl_dout = cpr[20];
        tb.dut().ioctl_wr = 1;
        tb.tick();

        // Now load_valid should be asserted for the first byte
        require(tb.dut().load_valid == 1, "expected pending write");
        // Drop cpr_download while load_valid is 1 without pulsing load_ready
        tb.dut().cpr_download = 0;
        tb.dut().ioctl_wr = 0;
        tb.tick(false);

        require(tb.abort_pulses() == 1, "early fall with pending write did not abort");
        require(tb.commit_pulses() == 0, "early fall with pending write committed");
        require(tb.dut().load_valid == 0, "early fall with pending write left load_valid active");
    }
}

void test_extra_and_nonsequential_bytes() {
    // Extra byte after exact RIFF boundary
    {
        TestBench tb;
        tb.hard_reset();
        auto cpr = build_cpr_image({{"cb00", std::vector<std::uint8_t>(100, 0x01)}});
        cpr.push_back(0xFF); // Extra byte beyond declared RIFF size
        tb.start_download();
        tb.feed_bytes(cpr, 0);
        tb.end_download();
        require(tb.abort_pulses() == 1, "extra byte after RIFF boundary did not abort");
        require(tb.commit_pulses() == 0, "extra byte after RIFF boundary committed");
    }

    // Non-sequential starting address
    {
        TestBench tb;
        tb.hard_reset();
        auto cpr = build_cpr_image({{"cb00", std::vector<std::uint8_t>(100, 0x01)}});
        tb.start_download();
        tb.feed_bytes(cpr, 0, 5); // start at address 5 instead of 0
        tb.end_download();
        require(tb.abort_pulses() == 1, "non-zero start address did not abort");
        require(tb.commit_pulses() == 0, "non-zero start address committed");
    }

    // Non-sequential address mid-stream
    {
        TestBench tb;
        tb.hard_reset();
        auto cpr = build_cpr_image({{"cb00", std::vector<std::uint8_t>(100, 0x01)}});
        tb.start_download();
        std::vector<std::uint8_t> part1(cpr.begin(), cpr.begin() + 10);
        std::vector<std::uint8_t> part2(cpr.begin() + 10, cpr.end());
        tb.feed_bytes(part1, 0, 0);
        tb.feed_bytes(part2, 0, 15); // gap from address 10 to 15
        tb.end_download();
        require(tb.abort_pulses() == 1, "address jump mid-stream did not abort");
        require(tb.commit_pulses() == 0, "address jump mid-stream committed");
    }
}

void test_backend_load_error() {
    TestBench tb;
    tb.hard_reset();
    auto cpr = build_cpr_image({{"cb00", std::vector<std::uint8_t>(100, 0x01)}});
    tb.start_download();
    // Inject load_error on 5th data byte write
    tb.feed_bytes(cpr, 1, 0, true, 24);
    tb.end_download();

    require(tb.abort_pulses() == 1, "backend load_error did not trigger parser abort");
    require(tb.commit_pulses() == 0, "backend load_error committed");
    require(tb.dut().load_valid == 0, "load_valid was not cleared on load_error");
}

void test_backpressure_stability_and_pulse_widths() {
    // Verify variable wait states and output stability
    for (unsigned wait_states : {0, 1, 3, 7, 15}) {
        TestBench tb;
        tb.hard_reset();
        auto cpr = build_cpr_image({{"cb05", std::vector<std::uint8_t>(64, 0x7E)}});
        tb.start_download();
        tb.feed_bytes(cpr, wait_states);
        tb.end_download();

        require(tb.begin_pulses() == 1, "load_begin pulse count != 1 with wait_states=" + std::to_string(wait_states));
        require(tb.commit_pulses() == 1, "load_commit pulse count != 1 with wait_states=" + std::to_string(wait_states));
        require(tb.abort_pulses() == 0, "load_abort pulsed with wait_states=" + std::to_string(wait_states));
        require(tb.writes().size() == 64, "write count mismatch with wait_states=" + std::to_string(wait_states));
    }
}

void test_simultaneous_download_and_byte_zero_write() {
    TestBench tb;
    tb.hard_reset();

    std::vector<Chunk> chunks;
    chunks.push_back({"cb00", std::vector<std::uint8_t>(16384, 0xA5)});
    auto cpr = build_cpr_image(chunks);

    // Present cpr_download and the first byte write (address 0) simultaneously
    tb.dut().cpr_download = 1;
    tb.dut().ioctl_wr = 1;
    tb.dut().ioctl_addr = 0;
    tb.dut().ioctl_dout = cpr[0];
    tb.dut().eval();

    // Verify wait is high before the start edge
    require(tb.dut().ioctl_wait == 1,
            "ioctl_wait was not asserted combinationally during download-start cycle");

    // Clock edge: session initializes and load_begin pulses without advancing the stream
    tb.tick();
    require(tb.begin_pulses() == 1, "load_begin did not pulse on start edge");
    require(tb.dut().load_abort == 0, "load_abort pulsed on start edge");

    // After start edge, wait drops
    require(tb.dut().ioctl_wait == 0, "ioctl_wait did not drop after download start cycle");

    // Retry address 0 and feed remaining bytes
    tb.feed_bytes(cpr, 1, 0);
    tb.end_download();

    require(tb.begin_pulses() == 1, "load_begin pulsed more than once");
    require(tb.commit_pulses() == 1, "simultaneous start with address-zero retry failed to commit");
    require(tb.abort_pulses() == 0, "simultaneous start with address-zero retry aborted");
    require(tb.writes().size() == 16384, "write count mismatch");

    for (size_t i = 0; i < 16384; ++i) {
        require(tb.writes()[i].page == 0 && tb.writes()[i].offset == i && tb.writes()[i].data == 0xA5,
                "page 0 payload mismatch");
    }
}

void run_test(const char* name, void (*test)()) {
    test();
    std::cout << "PASS: " << name << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    try {
        run_test("nominal multi-page CPR loading", test_nominal_multi_page);
        run_test("sparse and out-of-order block chunks", test_sparse_and_out_of_order);
        run_test("duplicate pages with last-wins semantics", test_duplicate_pages_last_wins);
        run_test("short chunks and oversized tail clamping", test_short_and_oversized_blocks);
        run_test("even and odd metadata chunk skipping with odd padding", test_metadata_chunks_even_and_odd);
        run_test("bad RIFF headers and magic fail closed", test_bad_headers);
        run_test("malformed and out-of-range cb chunk IDs fail closed", test_bad_chunk_ids);
        run_test("chunk extents beyond RIFF boundary fail closed", test_extent_beyond_riff);
        run_test("early download falls including pending writes abort cleanly", test_early_download_fall);
        run_test("extra data and non-sequential ioctl addresses fail closed", test_extra_and_nonsequential_bytes);
        run_test("backend load_error sticky abort handling", test_backend_load_error);
        run_test("backpressure stability and single-cycle pulse widths", test_backpressure_stability_and_pulse_widths);
        run_test("simultaneous download and byte-zero write asserts wait and retries cleanly", test_simultaneous_download_and_byte_zero_write);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }

    std::cout << "PASS: all Plus CPR parser tests\n";
    return 0;
}
