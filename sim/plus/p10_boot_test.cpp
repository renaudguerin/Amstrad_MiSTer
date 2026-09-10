#include "Vp10_boot_test_top.h"
#include "verilated.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <streambuf>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

constexpr uint8_t CMD_ACTIVE = 0b011;
constexpr uint8_t CMD_READ   = 0b101;
constexpr uint8_t CMD_WRITE  = 0b100;
constexpr unsigned kIoctlWaitLimit = 20000000;
constexpr uint16_t kCartridgeLoopPc = 0x0200;
constexpr uint64_t kLoopMeasureTicks = 4096;
constexpr unsigned kCartridgeLinkCount = 60;
constexpr uint64_t kCaptureWaitLimit = 16000000;
constexpr uint64_t kCaptureFrameTickLimit = 16000000;
constexpr uint64_t kCaptureWarmupFrames = 2;
constexpr uint64_t kCaptureMaxFrames = 16;
constexpr uint64_t kCaptureMaxCprBytes = 0x02000000;
constexpr uint64_t kCaptureMaxOutputBytes = 2ULL * 1024ULL * 1024ULL * 1024ULL;

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

struct FdcCpuLatchRead {
	uint16_t addr;
	uint8_t cpu_di;
	uint8_t fdc_dout;
	uint8_t expected;
	uint8_t fdc_bus;
	uint8_t ram_dout;
	uint8_t top_bus;
	uint8_t mb_bus;
	uint8_t msr;
	uint8_t state;
	uint8_t m_data;
	uint8_t buff_wait;
	uint16_t bytes_left;
};

std::vector<uint8_t> build_cpr_image(const std::vector<Chunk> &chunks, const std::string &form_type = "AMS!") {
    std::vector<uint8_t> image{'R', 'I', 'F', 'F'};
    uint32_t riff_len = 4;
    for (const auto &chunk : chunks) {
        riff_len += 8 + static_cast<uint32_t>(chunk.data.size());
        if (chunk.data.size() % 2 != 0) ++riff_len;
    }
    for (int i = 0; i < 4; ++i)
        image.push_back(static_cast<uint8_t>((riff_len >> (8 * i)) & 0xff));
    for (size_t i = 0; i < 4; ++i) {
        image.push_back(i < form_type.size() ? static_cast<uint8_t>(form_type[i]) : 0x20);
    }
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

// Keep the capture smoke on the same small, static CRTC program as the
// production VRAM-client proof below.  It is deliberately a real cartridge
// image: the T80, CPR parser/service, Plus MMU, motherboard and SDRAM client
// all participate before the frame taps are sampled.
std::vector<uint8_t> build_static_crtc_cpr() {
    std::vector<uint8_t> program(16384, 0x00);
    size_t pc = 0;
    auto emit = [&](uint8_t byte) { program[pc++] = byte; };
    auto write_crtc = [&](uint8_t reg, uint8_t value) {
        emit(0x01); emit(0x00); emit(0xBC); // LD BC,&BC00
        emit(0x3E); emit(reg);              // LD A,register
        emit(0xED); emit(0x79);             // OUT (C),A
        emit(0x01); emit(0x00); emit(0xBD); // LD BC,&BD00
        emit(0x3E); emit(value);            // LD A,value
        emit(0xED); emit(0x79);             // OUT (C),A
    };
    write_crtc(0, 63);
    write_crtc(1, 40);
    write_crtc(4, 38);
    write_crtc(6, 25);
    write_crtc(9, 7);
    emit(0x76); // HALT: the programmed video path keeps running.
    return build_cpr_image({{"cb00", program}});
}

class Harness;

struct VideoSample {
    bool raw_hsync = false;
    bool raw_vsync = false;
    bool raw_de = false;
    bool selected_hsync = false;
    bool selected_vsync = false;
    bool selected_hblank = false;
    bool selected_vblank = false;
    uint16_t rgb = 0;
    uint16_t ma = 0;
    uint8_t ra = 0;
    uint16_t vram_addr = 0;
    uint16_t vram_word = 0;
    uint8_t vram_byte = 0;
    uint8_t filter_selector = 0;
};

VideoSample sample_video(const Harness &h);

struct FrameCaptureStats {
    uint64_t frame_index = 0;
    uint64_t sample_count = 0;
    uint64_t hash = 0;
    uint64_t raw_hsync_edges = 0;
    uint64_t raw_hsync_rises = 0;
    uint64_t raw_vsync_edges = 0;
    uint64_t raw_vsync_rises = 0;
    uint64_t selected_hsync_edges = 0;
    uint64_t selected_hsync_rises = 0;
    uint64_t selected_vsync_edges = 0;
    uint64_t selected_vsync_rises = 0;
    uint64_t de_active_ticks = 0;
    uint64_t hblank_active_ticks = 0;
    uint64_t vblank_active_ticks = 0;
};

class Fnv1a64 {
public:
    static constexpr uint64_t kOffsetBasis = 14695981039346656037ULL;
    static constexpr uint64_t kPrime = 1099511628211ULL;

    void update(const void *data, size_t size) {
        const auto *bytes = static_cast<const unsigned char *>(data);
        for (size_t index = 0; index < size; ++index) {
            const unsigned char byte = bytes[index];
            value_ ^= byte;
            value_ *= kPrime;
        }
    }

    void update(std::string_view bytes) { update(bytes.data(), bytes.size()); }

    uint64_t value() const { return value_; }

private:
    uint64_t value_ = kOffsetBasis;
};

struct CaptureResult {
    uint8_t filter_selector = 0;
    std::vector<FrameCaptureStats> frames;
    uint64_t serialized_bytes = 0;
};

struct CaptureProvenance {
    uint64_t cpr_size_bytes = 0;
    uint64_t cpr_content_hash = 0;
    uint8_t plus_model = 0;
    bool production_clocking = false;
    std::string simulator_binary_identity;
};

uint64_t hash_bytes(const std::vector<uint8_t> &bytes);

void require_stream_ok(const std::ostream &output, const char *context);

class NullStreamBuf : public std::streambuf {
protected:
    int_type overflow(int_type character) override {
        return traits_type::not_eof(character);
    }
};

class ExclusiveFileBuf : public std::streambuf {
public:
    ExclusiveFileBuf() { setp(buffer_.data(), buffer_.data() + buffer_.size()); }
    ~ExclusiveFileBuf() override { close(); }

    bool open(const std::string &path) {
        if (fd_ >= 0) return false;
        // O_EXCL makes creation atomic against create races; O_NOFOLLOW
        // refuses a symlink at the output path (including a dangling link
        // the pre-check cannot see) instead of following it.
        fd_ = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0666);
        if (fd_ < 0) {
            error_number_ = errno;
            error_ = std::strerror(errno);
            return false;
        }
        setp(buffer_.data(), buffer_.data() + buffer_.size());
        return true;
    }

    bool close() {
        bool okay = sync() == 0;
        if (fd_ >= 0) {
            if (::close(fd_) != 0) okay = false;
            fd_ = -1;
        }
        return okay;
    }

    const std::string &error() const { return error_; }
    int error_number() const { return error_number_; }

protected:
    int_type overflow(int_type character) override {
        if (!flush_buffer()) return traits_type::eof();
        if (!traits_type::eq_int_type(character, traits_type::eof())) {
            *pptr() = traits_type::to_char_type(character);
            pbump(1);
        }
        return traits_type::not_eof(character);
    }

    std::streamsize xsputn(const char *data, std::streamsize size) override {
        std::streamsize written = 0;
        while (written < size) {
            const std::streamsize available = epptr() - pptr();
            if (available == 0 && !flush_buffer()) break;
            const std::streamsize chunk = std::min(available, size - written);
            std::memcpy(pptr(), data + written, static_cast<size_t>(chunk));
            pbump(static_cast<int>(chunk));
            written += chunk;
        }
        return written;
    }

    int sync() override { return flush_buffer() ? 0 : -1; }

private:
    bool flush_buffer() {
        if (fd_ < 0) return true;
        const char *data = pbase();
        std::streamsize remaining = pptr() - pbase();
        while (remaining > 0) {
            const ssize_t written = ::write(fd_, data, static_cast<size_t>(remaining));
            if (written <= 0) {
                error_number_ = errno;
                error_ = std::strerror(errno);
                return false;
            }
            data += written;
            remaining -= written;
        }
        setp(buffer_.data(), buffer_.data() + buffer_.size());
        return true;
    }

    int fd_ = -1;
    int error_number_ = 0;
    std::array<char, 64 * 1024> buffer_{};
    std::string error_;
};

class CaptureWriter {
public:
    explicit CaptureWriter(std::ostream &output) : output_(output) {}

    void write(std::string_view text) {
        if (text.size() > kCaptureMaxOutputBytes - bytes_written_)
            throw TestFailure("serialized capture exceeds 2 GiB output budget");
        output_.write(text.data(), static_cast<std::streamsize>(text.size()));
        require_stream_ok(output_, "stream write");
        bytes_written_ += text.size();
    }

    void flush() {
        output_.flush();
        require_stream_ok(output_, "stream flush");
    }

    uint64_t bytes_written() const { return bytes_written_; }

private:
    std::ostream &output_;
    uint64_t bytes_written_ = 0;
};

class Harness {
public:
    Vp10_boot_test_top dut;
    std::unordered_map<uint32_t, uint8_t> memory;
    uint64_t cycles = 0;
    uint32_t physical_cart_reads = 0;
	uint32_t fdc_sd_reads = 0;
	uint32_t fdc_last_lba = 0;
	std::vector<uint32_t> fdc_sd_lbas;
	std::vector<uint8_t> disk_image;
	std::vector<uint8_t> fdc_writes;
	std::vector<uint8_t> fdc_reads;
	std::vector<FdcCpuLatchRead> fdc_cpu_latch_reads;
	std::array<uint8_t, 512> fdc_payload{};
	std::array<bool, 512> fdc_payload_seen{};
	size_t fdc_first_payload_latch_count = 0;
	bool fdc_first_payload_latch_count_seen = false;
	std::array<uint8_t, 7> fdc_results{};
	std::array<bool, 7> fdc_result_seen{};
	bool fdc_success = false;
	bool cpr_load_abort_seen = false;
	bool cpr_load_error_seen = false;

    Harness() {
        dut.clk = 0;
        dut.clkref = 0;
        dut.init = 1;
        dut.reset_btn = 0;
        dut.plus_model_i = 2; // 6128+
        dut.cpr_download = 0;
        dut.ioctl_wr = 0;
        dut.ioctl_addr = 0;
        dut.ioctl_dout = 0;
        dut.memory_dq = 0;
        dut.memory_dq_oe = 0;
        dut.force_irq = 0;
		dut.production_clocking = 0;
		dut.fdc_img_mounted = 0;
		dut.fdc_img_wp = 1;
		dut.fdc_img_size = 0;
		dut.fdc_sd_ack = 0;
		dut.fdc_sd_buff_addr = 0;
		dut.fdc_sd_buff_dout = 0;
		dut.fdc_sd_buff_wr = 0;
        dut.eval();
    }

    ~Harness() { dut.final(); }

    void raw_tick() {
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
            if (!dut.sdram_dqml) {
                store(active_bank, address, data & 0xffU);
                if (active_bank == 0)
                    bank0_writes.push_back({address, static_cast<uint8_t>(data & 0xffU), cycles});
            }
            if (!dut.sdram_dqmh) {
                store(active_bank, address + 1, data >> 8);
                if (active_bank == 0)
                    bank0_writes.push_back({address + 1, static_cast<uint8_t>(data >> 8), cycles});
            }
        }
        if (read_drive_cycles > 0 && !started_read) --read_drive_cycles;

        dut.clk = 0;
        dut.eval();
		if (dut.dbg_cpr_load_abort) cpr_load_abort_seen = true;
		if (dut.dbg_cpr_load_error) cpr_load_error_seen = true;
        if (dut.dbg_cart_own && !dut.dbg_cart_stall && !dut.dbg_mreq_n &&
            !dut.dbg_rd_n && dut.dbg_addr == 0xC000 && dut.dbg_din == 0x42) {
            upper_page_magic_seen = true;
        }

		const bool fdc_io = !dut.dbg_iorq_n &&
		                    (!dut.dbg_rd_n || !dut.dbg_wr_n) &&
		                    dut.dbg_fdc_data_sel;
		if (fdc_io && !fdc_io_active) {
			if (!dut.dbg_wr_n) {
				fdc_writes.push_back(dut.dbg_dout);
			}
			if (!dut.dbg_rd_n) {
				fdc_reads.push_back(dut.dbg_din);
			}
		}
		fdc_io_active = fdc_io;

		if (dut.dbg_cpu_di_latch && dut.dbg_cpu_di_fdc_sel &&
		    dut.dbg_cpu_di_io_rd) {
			fdc_cpu_latch_reads.push_back({
				static_cast<uint16_t>(dut.dbg_cpu_di_addr),
				static_cast<uint8_t>(dut.dbg_cpu_di_reg),
				static_cast<uint8_t>(dut.dbg_cpu_di_fdc_dout),
				static_cast<uint8_t>(dut.dbg_cpu_di_expected),
				static_cast<uint8_t>(dut.dbg_cpu_di_fdc_bus),
				static_cast<uint8_t>(dut.dbg_cpu_di_ram_dout),
				static_cast<uint8_t>(dut.dbg_cpu_di_top_bus),
				static_cast<uint8_t>(dut.dbg_cpu_di_mb_bus),
				static_cast<uint8_t>(dut.dbg_cpu_di_msr),
				static_cast<uint8_t>(dut.dbg_cpu_di_fdc_state),
				static_cast<uint8_t>(dut.dbg_cpu_di_m_data),
				static_cast<uint8_t>(dut.dbg_cpu_di_buff_wait),
				static_cast<uint16_t>(dut.dbg_cpu_di_bytes_left),
			});
		}

		if (!dut.dbg_mreq_n && !dut.dbg_wr_n) {
			if (dut.dbg_addr >= 0x8000 && dut.dbg_addr < 0x8200) {
				const unsigned offset = dut.dbg_addr - 0x8000;
				fdc_payload[offset] = dut.dbg_dout;
				fdc_payload_seen[offset] = true;
				if (offset == 0 && !fdc_first_payload_latch_count_seen) {
					fdc_first_payload_latch_count_seen = true;
					fdc_first_payload_latch_count = fdc_cpu_latch_reads.size();
				}
			}
			if (dut.dbg_addr >= 0x8200 && dut.dbg_addr < 0x8207) {
				const unsigned offset = dut.dbg_addr - 0x8200;
				fdc_results[offset] = dut.dbg_dout;
				fdc_result_seen[offset] = true;
			}
			if (dut.dbg_addr == 0x82ff && dut.dbg_dout == 0xa5)
				fdc_success = true;
		}
        ++cycles;
    }

	void tick() {
		raw_tick();
		if (dut.fdc_sd_rd == 0) sd_request_serviced = false;
		if (dut.fdc_sd_wr != 0)
			throw TestFailure("read-only FDC fixture received an SD write request");
		if (dut.fdc_sd_rd != 0 && !sd_request_serviced) {
			sd_request_serviced = true;
			service_sd_read();
		}
	}

	void load_disk(const std::string &path) {
		std::ifstream input(path, std::ios::binary);
		if (!input) throw TestFailure("cannot open disk image " + path);
		disk_image.assign(std::istreambuf_iterator<char>(input),
		                  std::istreambuf_iterator<char>());
		require(!disk_image.empty(), "known-good disk image is empty");
	}

	void mount_disk() {
		require(!disk_image.empty(), "mount requested without a disk image");
		dut.fdc_img_size = static_cast<uint32_t>(disk_image.size());
		dut.fdc_img_mounted = 1;
		tick();
		dut.fdc_img_mounted = 0;
		for (unsigned wait = 0; wait < 4000000 && !dut.dbg_fdc_image_ready;
		     ++wait)
			tick();
		require(dut.dbg_fdc_image_ready,
		        "real u765 did not finish parsing the known-good EDSK");
	}

    void initialize() {
        for (int i = 0; i < 16; ++i) tick();
        dut.init = 0;
        for (int i = 0; i < 500; ++i) tick();
    }

    void download(const std::vector<uint8_t> &image) {
        dut.cpr_download = 1;
        tick();
        if (cpr_load_abort_seen)
            throw TestFailure("CPR parser aborted during download");
        for (size_t i = 0; i < image.size(); ++i) {
            dut.ioctl_addr = static_cast<uint32_t>(i);
            dut.ioctl_dout = image[i];
            dut.ioctl_wr = 1;
            tick();
            if (cpr_load_abort_seen)
                throw TestFailure("CPR parser aborted during download");
            dut.ioctl_wr = 0;
            unsigned wait_count = 0;
            while (dut.ioctl_wait) {
                tick();
                if (++wait_count > kIoctlWaitLimit) {
                    throw TestFailure("ioctl_wait stuck during CPR download");
                }
            }
        }
        dut.cpr_download = 0;
        tick();
        if (cpr_load_abort_seen)
            throw TestFailure("CPR parser aborted while applying download");
    }

    void run_until_pc(uint16_t target_pc, uint64_t max_cycles = 100000) {
        uint64_t start = cycles;
        while (dut.dbg_pc != target_pc) {
            tick();
            if (cycles - start > max_cycles) {
                std::cerr << "Timeout waiting for PC 0x" << std::hex << target_pc
                          << " (current PC: 0x" << dut.dbg_pc << ")\n";
                throw TestFailure("CPU PC timeout");
            }
        }
    }

    void run_cycles(uint64_t n) {
        for (uint64_t i = 0; i < n; ++i) tick();
    }

    bool upper_page_magic_seen = false;

    struct SdramWrite {
        uint32_t byte_address;
        uint8_t value;
        uint64_t tick;
    };
    std::vector<SdramWrite> bank0_writes;

    void preload(uint8_t bank, uint32_t address, uint8_t byte) {
        store(bank, address, byte);
    }

private:
    uint16_t active_row = 0;
    uint8_t active_bank = 0;
    uint16_t read_word = 0;
    int read_drive_cycles = 0;
	bool sd_request_serviced = false;
	bool fdc_io_active = false;

	void service_sd_read() {
		const uint32_t lba = dut.fdc_sd_lba;
		const uint64_t offset = static_cast<uint64_t>(lba) * 512U;
		fdc_last_lba = lba;
		++fdc_sd_reads;
		fdc_sd_lbas.push_back(lba);
		for (unsigned address = 0; address < 512; ++address) {
			dut.fdc_sd_ack = 1;
			dut.fdc_sd_buff_wr = 1;
			dut.fdc_sd_buff_addr = address;
			const uint64_t image_address = offset + address;
			dut.fdc_sd_buff_dout = image_address < disk_image.size()
			                         ? disk_image[image_address]
			                         : 0;
			raw_tick();
		}
		dut.fdc_sd_buff_wr = 0;
		dut.fdc_sd_ack = 0;
		raw_tick();
	}

    uint8_t command() const {
        return (dut.sdram_nras ? 0b100 : 0b000) |
               (dut.sdram_ncas ? 0b010 : 0b000) |
               (dut.sdram_nwe  ? 0b001 : 0b000);
    }

    uint32_t command_address() const {
        // Reconstruct sdram.v's byte address from ACTIVE row a[21:9]
        // and READ/WRITE column {a[22],a[8:1]}. The old row<<10 form
        // shifted every observed memory transaction by one address bit.
        return (static_cast<uint32_t>(active_row) << 9) |
               ((static_cast<uint32_t>(dut.sdram_a) & 0x100U) << 14) |
               ((static_cast<uint32_t>(dut.sdram_a) & 0xffU) << 1);
    }

    uint8_t load(uint8_t bank, uint32_t address) {
        const uint32_t key = (static_cast<uint32_t>(bank) << 23) | (address & 0x7fffffU);
        auto it = memory.find(key);
        return (it == memory.end()) ? 0xffU : it->second;
    }

    void store(uint8_t bank, uint32_t address, uint8_t byte) {
        const uint32_t key = (static_cast<uint32_t>(bank) << 23) | (address & 0x7fffffU);
        memory[key] = byte;
    }
};

VideoSample sample_video(const Harness &h) {
    VideoSample sample;
    sample.raw_hsync = h.dut.dbg_raw_hsync != 0;
    sample.raw_vsync = h.dut.dbg_raw_vsync != 0;
    sample.raw_de = h.dut.dbg_raw_de != 0;
    sample.selected_hsync = h.dut.dbg_selected_hsync != 0;
    sample.selected_vsync = h.dut.dbg_selected_vsync != 0;
    sample.selected_hblank = h.dut.dbg_selected_hblank != 0;
    sample.selected_vblank = h.dut.dbg_selected_vblank != 0;
    sample.rgb = static_cast<uint16_t>(h.dut.dbg_video_rgb);
    sample.ma = static_cast<uint16_t>(h.dut.dbg_video_ma);
    sample.ra = static_cast<uint8_t>(h.dut.dbg_video_ra);
    sample.vram_addr = static_cast<uint16_t>(h.dut.dbg_video_vram_addr);
    sample.vram_word = static_cast<uint16_t>(h.dut.dbg_video_vram_word);
    sample.vram_byte = static_cast<uint8_t>(h.dut.dbg_video_vram_byte);
    sample.filter_selector = static_cast<uint8_t>(h.dut.dbg_sync_filter);
    return sample;
}

// The first column (frame) is framing metadata.  The remaining serialized
// fields are also the exact byte sequence fed to the per-frame FNV-1a hash;
// this keeps equal steady-state frames hash-equal while retaining frame
// numbering in the text stream.
std::string serialize_sample_fields(uint64_t sample_index,
                                    const VideoSample &sample) {
    std::string fields = std::to_string(sample_index);
    const auto append = [&](uint64_t value) {
        fields.push_back(' ');
        fields += std::to_string(value);
    };
    append(sample.raw_hsync ? 1 : 0);
    append(sample.raw_vsync ? 1 : 0);
    append(sample.raw_de ? 1 : 0);
    append(sample.selected_hsync ? 1 : 0);
    append(sample.selected_vsync ? 1 : 0);
    append(sample.selected_hblank ? 1 : 0);
    append(sample.selected_vblank ? 1 : 0);
    append(sample.rgb);
    append(sample.ma);
    append(sample.ra);
    append(sample.vram_addr);
    append(sample.vram_word);
    append(sample.vram_byte);
    append(sample.filter_selector);
    return fields;
}

std::string format_hash(uint64_t hash) {
    std::ostringstream text;
    text << "0x" << std::hex << std::setw(16) << std::setfill('0') << hash;
    return text.str();
}

void require_stream_ok(const std::ostream &output, const char *context) {
    require(static_cast<bool>(output), std::string("capture ") + context + " failed");
}

const char *plus_model_name(uint8_t plus_model) {
    switch (plus_model) {
    case 0: return "Off";
    case 1: return "GX4000";
    case 2: return "6128+";
    case 3: return "464+";
    default: return "unknown";
    }
}

std::string sanitize_header_text(std::string text) {
    for (char &character : text) {
        if (character == '\n' || character == '\r') character = '_';
    }
    return text;
}

void write_capture_header(CaptureWriter &writer, uint8_t filter_selector,
                          uint64_t requested_frames, uint64_t warmup_frames,
                          const CaptureProvenance &provenance) {
    std::ostringstream header;
    header << "# p10_frame_capture version=1\n"
           << "# clock_hz=64000000\n"
           << "# delimiter=selected_vsync rising edge (0-to-1)\n"
           << "# cpu_surrogate=t80pa_wrapped_reduced_tv80 (fixture only; the production VHDL T80 is not compiled under Verilator, so JR/DJNZ/JP-cc/interrupt software is out of scope)\n"
           << "# raw_* = motherboard hs_sel/vs_sel/de_sel source taps; selected_* = production output tuple\n"
           << "# RGB/MA/RA/VRAM are shared and filter-dependent taps, not pre/post-filter taps\n"
           << "# GA/filter internal stages are absent and reserved for a future capture version\n"
           << "# numeric_encoding=decimal; boolean_encoding=0_or_1\n"
           << "# hash=FNV1A-64 over serialized sample fields from sample through filter_selector; frame and line ending excluded\n"
           << "# serialized_output_budget_bytes=" << kCaptureMaxOutputBytes << "\n"
           << "# cpr_size_bytes=" << provenance.cpr_size_bytes << "\n"
           << "# cpr_content_hash=" << format_hash(provenance.cpr_content_hash) << "\n"
           << "# plus_model_i=" << static_cast<unsigned>(provenance.plus_model)
           << " (" << plus_model_name(provenance.plus_model) << ")\n"
           << "# production_clocking=" << (provenance.production_clocking ? 1 : 0)
           << " (" << (provenance.production_clocking ? "production_divider" :
                           "fixture_divider") << ")\n"
           << "# simulator_binary="
           << sanitize_header_text(provenance.simulator_binary_identity) << "\n"
           << "# filter_selector=" << static_cast<unsigned>(filter_selector)
           << " (0=Full,1=Live_blank,2=Off)\n"
           << "# requested_frames=" << requested_frames << "\n"
           << "# warmup_frames_discarded=" << warmup_frames << "\n"
           << "frame sample raw_hsync raw_vsync raw_de selected_hsync selected_vsync selected_hblank selected_vblank rgb ma ra vram_addr vram_word vram_byte filter_selector\n";
    writer.write(header.str());
}

void write_frame_summary(CaptureWriter &writer, const FrameCaptureStats &stats) {
    std::ostringstream summary;
    summary << "# frame_summary frame=" << stats.frame_index
            << " samples=" << stats.sample_count
            << " hash=" << format_hash(stats.hash)
            << " raw_hsync_edges=" << stats.raw_hsync_edges
            << " raw_hsync_rises=" << stats.raw_hsync_rises
            << " raw_vsync_edges=" << stats.raw_vsync_edges
            << " raw_vsync_rises=" << stats.raw_vsync_rises
            << " selected_hsync_edges=" << stats.selected_hsync_edges
            << " selected_hsync_rises=" << stats.selected_hsync_rises
            << " selected_vsync_edges=" << stats.selected_vsync_edges
            << " selected_vsync_rises=" << stats.selected_vsync_rises
            << " de_active_ticks=" << stats.de_active_ticks
            << " hblank_active_ticks=" << stats.hblank_active_ticks
            << " vblank_active_ticks=" << stats.vblank_active_ticks << "\n";
    writer.write(summary.str());
}

void report_frame_summary(const FrameCaptureStats &stats) {
    std::cout << "CAPTURE frame=" << stats.frame_index
              << " samples=" << stats.sample_count
              << " hash=" << format_hash(stats.hash)
              << " raw_hsync_edges=" << stats.raw_hsync_edges
              << " raw_vsync_edges=" << stats.raw_vsync_edges
              << " selected_hsync_edges=" << stats.selected_hsync_edges
              << " selected_vsync_edges=" << stats.selected_vsync_edges
              << " de_active_ticks=" << stats.de_active_ticks
              << " hblank_active_ticks=" << stats.hblank_active_ticks
              << " vblank_active_ticks=" << stats.vblank_active_ticks << std::endl;
}

void update_frame_counts(FrameCaptureStats &stats, const VideoSample &sample,
                         const VideoSample *previous) {
    if (sample.raw_de) ++stats.de_active_ticks;
    if (sample.selected_hblank) ++stats.hblank_active_ticks;
    if (sample.selected_vblank) ++stats.vblank_active_ticks;
    if (!previous) return;

    if (sample.raw_hsync != previous->raw_hsync) ++stats.raw_hsync_edges;
    if (!previous->raw_hsync && sample.raw_hsync) ++stats.raw_hsync_rises;
    if (sample.raw_vsync != previous->raw_vsync) ++stats.raw_vsync_edges;
    if (!previous->raw_vsync && sample.raw_vsync) ++stats.raw_vsync_rises;
    if (sample.selected_hsync != previous->selected_hsync)
        ++stats.selected_hsync_edges;
    if (!previous->selected_hsync && sample.selected_hsync)
        ++stats.selected_hsync_rises;
    if (sample.selected_vsync != previous->selected_vsync)
        ++stats.selected_vsync_edges;
    if (!previous->selected_vsync && sample.selected_vsync)
        ++stats.selected_vsync_rises;
}

void append_capture_sample(CaptureWriter &writer, FrameCaptureStats &stats,
                           Fnv1a64 &hasher, uint64_t sample_index,
                           const VideoSample &sample,
                           const VideoSample *previous) {
    const std::string fields = serialize_sample_fields(sample_index, sample);
    const std::string row = std::to_string(stats.frame_index) + " " + fields + "\n";
    writer.write(row);
    hasher.update(fields);
    ++stats.sample_count;
    update_frame_counts(stats, sample, previous);
}

void wait_for_reset_release(Harness &h, const char *name) {
    for (uint64_t tick = 0; tick < kCaptureWaitLimit; ++tick) {
        if (!h.dut.dbg_reset) return;
        h.tick();
    }
    throw TestFailure(std::string("timed out waiting for ") + name + " reset release");
}

void wait_for_cpr_apply(Harness &h) {
    for (uint64_t tick = 0; tick < kCaptureWaitLimit; ++tick) {
        if (h.cpr_load_abort_seen)
            throw TestFailure("CPR parser aborted while applying download");
        if (h.cpr_load_error_seen || h.dut.dbg_cpr_load_error)
            throw TestFailure("CPR cartridge service reported a load error");
        if (!h.dut.dbg_reset && h.dut.dbg_cart_image_valid &&
            !h.dut.dbg_cart_service_busy)
            return;
        h.tick();
    }
    throw TestFailure("timed out waiting for CPR apply/reset to finish");
}

bool m1_memory_read(const Harness &h);

void wait_for_m1_read(Harness &h, const char *name,
                      uint64_t max_ticks = kCaptureWaitLimit) {
    for (uint64_t tick = 0; tick < max_ticks; ++tick) {
        if (m1_memory_read(h)) return;
        h.tick();
    }
    throw TestFailure(std::string("timed out waiting for ") + name + " M1 read");
}

VideoSample wait_for_selected_vsync_rising(
    Harness &h, VideoSample &previous, const char *phase,
    uint64_t max_ticks = kCaptureWaitLimit,
    VideoSample *before_edge = nullptr) {
    for (uint64_t tick = 0; tick < max_ticks; ++tick) {
        h.tick();
        const VideoSample current = sample_video(h);
        const bool rising = !previous.selected_vsync && current.selected_vsync;
        const VideoSample prior = previous;
        previous = current;
        if (rising) {
            if (before_edge) *before_edge = prior;
            return current;
        }
    }
    throw TestFailure(std::string("timed out waiting for selected VSYNC rising edge during ") +
                      phase);
}

CaptureResult capture_video_frames(Harness &h, uint64_t frame_count,
                                   std::ostream &output,
                                   const CaptureProvenance &provenance,
                                   uint64_t warmup_frames = kCaptureWarmupFrames) {
    require(frame_count > 0, "capture frame count must be positive");
    require(frame_count <= kCaptureMaxFrames,
            "capture frame count exceeds bounded harness limit");

    CaptureResult result;
    result.frames.reserve(static_cast<size_t>(frame_count));
    VideoSample previous = sample_video(h);
    result.filter_selector = previous.filter_selector;
    require(provenance.cpr_size_bytes > 0, "capture provenance is missing CPR size");
    require(!provenance.simulator_binary_identity.empty(),
            "capture provenance is missing simulator-binary identity");
    CaptureWriter writer(output);
    write_capture_header(writer, result.filter_selector, frame_count,
                         warmup_frames, provenance);

    // Establish a boundary, then discard exactly warmup_frames complete
    // intervals.  The final boundary reached by that loop is the first
    // captured sample, so no unreported synchronization frame is inserted.
    VideoSample boundary_before;
    VideoSample boundary = wait_for_selected_vsync_rising(
        h, previous, "initial frame boundary", kCaptureWaitLimit,
        &boundary_before);
    for (uint64_t warmup = 0; warmup < warmup_frames; ++warmup) {
        boundary = wait_for_selected_vsync_rising(
            h, previous, "warm-up", kCaptureWaitLimit, &boundary_before);
    }

    FrameCaptureStats current;
    Fnv1a64 hasher;
    uint64_t sample_index = 0;
    uint64_t ticks_since_rising = 0;
    current.frame_index = 0;
    hasher = Fnv1a64{};
    append_capture_sample(writer, current, hasher, sample_index++, boundary,
                          &boundary_before);

    while (result.frames.size() < frame_count) {
        h.tick();
        const VideoSample sample = sample_video(h);
        const bool rising = !previous.selected_vsync && sample.selected_vsync;

        if (rising) {
            current.hash = hasher.value();
            write_frame_summary(writer, current);
            report_frame_summary(current);
            result.frames.push_back(current);
            if (result.frames.size() == frame_count) {
                previous = sample;
                break;
            }

            current = FrameCaptureStats{};
            current.frame_index = result.frames.size();
            hasher = Fnv1a64{};
            sample_index = 0;
            append_capture_sample(writer, current, hasher, sample_index++,
                                  sample, &previous);
            ticks_since_rising = 0;
        }
        else {
            if (ticks_since_rising++ >= kCaptureFrameTickLimit)
                throw TestFailure("timed out waiting for complete captured frame");
            append_capture_sample(writer, current, hasher, sample_index++,
                                  sample, &previous);
        }
        previous = sample;
    }

    require(result.frames.size() == frame_count,
            "capture did not produce the requested number of complete frames");
    writer.flush();
    result.serialized_bytes = writer.bytes_written();
    return result;
}

struct LoopMeasurement {
    uint64_t elapsed_ticks = 0;
    uint64_t m1_fetches = 0;
    uint64_t cart_stall_cycles = 0;
    uint64_t cpu_wait_low_cycles = 0;
    uint64_t max_cart_stall_run = 0;
    uint64_t max_cpu_wait_low_run = 0;
    uint32_t physical_cart_reads = 0;
};

bool m1_memory_read(const Harness &h) {
    return !h.dut.dbg_m1_n && !h.dut.dbg_mreq_n && !h.dut.dbg_rd_n;
}

void wait_for_loop_entry(Harness &h, uint16_t loop_pc, const char *name,
                         uint64_t max_ticks = 100000) {
    for (uint64_t i = 0; i < max_ticks; ++i) {
        h.tick();
        if (m1_memory_read(h) && h.dut.dbg_pc == loop_pc) return;
    }
    throw TestFailure(std::string("timed out entering ") + name + " loop");
}

LoopMeasurement measure_loop(Harness &h, uint64_t ticks) {
    LoopMeasurement result;
    const uint32_t reads_before = h.physical_cart_reads;
    bool previous_m1_read = false;
    uint64_t cart_stall_run = 0;
    uint64_t cpu_wait_low_run = 0;

    for (uint64_t i = 0; i < ticks; ++i) {
        h.tick();
        ++result.elapsed_ticks;

        const bool current_m1_read = m1_memory_read(h);
        if (current_m1_read && !previous_m1_read) {
            ++result.m1_fetches;
        }
        previous_m1_read = current_m1_read;

        if (h.dut.dbg_cart_stall) {
            ++result.cart_stall_cycles;
            ++cart_stall_run;
        } else {
            if (cart_stall_run > result.max_cart_stall_run)
                result.max_cart_stall_run = cart_stall_run;
            cart_stall_run = 0;
        }

        if (!h.dut.dbg_cpu_waitn) {
            ++result.cpu_wait_low_cycles;
            ++cpu_wait_low_run;
        } else {
            if (cpu_wait_low_run > result.max_cpu_wait_low_run)
                result.max_cpu_wait_low_run = cpu_wait_low_run;
            cpu_wait_low_run = 0;
        }
    }

    if (cart_stall_run > result.max_cart_stall_run)
        result.max_cart_stall_run = cart_stall_run;
    if (cpu_wait_low_run > result.max_cpu_wait_low_run)
        result.max_cpu_wait_low_run = cpu_wait_low_run;
    result.physical_cart_reads = h.physical_cart_reads - reads_before;
    return result;
}

void test_p10a_deterministic_boot() {
    std::cout << "Running test_p10a_deterministic_boot..." << std::endl;
    Harness h;
    h.initialize();

    // Construct a deterministic Z80 test program in cartridge page 0. Keep
    // the bootstrap above the IM1 vector and leave a long cartridge-resident
    // timing chain before the original EI/HALT interrupt check.
    std::vector<uint8_t> p0_code(16384, 0x00); // NOP fill

    p0_code[0x0000] = 0xC3; // JP &0100 (reset vector)
    p0_code[0x0001] = 0x00;
    p0_code[0x0002] = 0x01;
    size_t idx = 0x0100;
    auto emit = [&](uint8_t b) { p0_code[idx++] = b; };

    // 0x0100: DI
    emit(0xF3);
    // 0x0101: LD SP, 0xC000
    emit(0x31); emit(0x00); emit(0xC0);
    // 0x0104: Select Upper ROM page 3 via &DF00: LD BC, &DF00; LD A, 7; OUT (C), A
    emit(0x01); emit(0x00); emit(0xDF);
    emit(0x3E); emit(0x07);
    emit(0xED); emit(0x79);
    // 0x010B: Read byte from upper window 0xC000: LD A, (0xC000)
    emit(0x3A); emit(0x00); emit(0xC0);

    // 0x010E: 16-byte ASIC unlock sequence to &BC00:
    // Sync: FF 00, Sequence: FF 77 B3 51 A8 D4 62 39 9C 46 2B 15 8A CD
    const uint8_t unlock_seq[16] = {
        0xFF, 0x00, 0xFF, 0x77, 0xB3, 0x51, 0xA8, 0xD4,
        0x62, 0x39, 0x9C, 0x46, 0x2B, 0x15, 0x8A, 0xCD
    };
    emit(0x01); emit(0x00); emit(0xBC); // LD BC, &BC00
    for (int i = 0; i < 16; ++i) {
        emit(0x3E); emit(unlock_seq[i]); // LD A, byte
        emit(0xED); emit(0x79);          // OUT (C), A
    }

    // Enable ASIC register page via RMR2: LD BC, &7F00; LD A, &B8; OUT (C), A
    emit(0x01); emit(0x00); emit(0x7F);
    emit(0x3E); emit(0xB8);
    emit(0xED); emit(0x79);

    // Write to ASIC palette register &6420 (Border): LD A, &34; LD (0x6420), A
    emit(0x3E); emit(0x34);
    emit(0x32); emit(0x20); emit(0x64);

    // Write to CRTC3 register (R1 = 40):
    // LD BC, &BC00; LD A, 1; OUT (C), A
    emit(0x01); emit(0x00); emit(0xBC);
    emit(0x3E); emit(0x01);
    emit(0xED); emit(0x79);
    // LD BC, &BD00; LD A, 40; OUT (C), A
    emit(0x01); emit(0x00); emit(0xBD);
    emit(0x3E); emit(0x28);
    emit(0xED); emit(0x79);

    // Write to FDC motor port &FA00 (Motor ON): LD BC, &FA00; LD A, 1; OUT (C), A
    emit(0x01); emit(0x00); emit(0xFA);
    emit(0x3E); emit(0x01);
    emit(0xED); emit(0x79);

    // The TV80's immediate-jump high byte is taken from its preceding WZ
    // value on the same edge as the high-byte load. Prime WZ with a read from
    // the cartridge chain before the handoff JP so the target is 0x0200.
    emit(0x3A); emit(static_cast<uint8_t>(kCartridgeLoopPc & 0xff));
    emit(static_cast<uint8_t>(kCartridgeLoopPc >> 8)); // LD A, (&0200)
    emit(0xC3); emit(static_cast<uint8_t>(kCartridgeLoopPc & 0xff));
    emit(static_cast<uint8_t>(kCartridgeLoopPc >> 8)); // JP &0200

    // Use an unrolled NOP/JP chain for a sustained cartridge window. This
    // avoids unsupported DJNZ/LDIR behavior in the TV80 replacement and any
    // dependence on a fragile debugger-PC loop heuristic.
    const uint16_t handoff_pc = static_cast<uint16_t>(
        kCartridgeLoopPc + kCartridgeLinkCount * 4U);
    for (unsigned link = 0; link < kCartridgeLinkCount; ++link) {
        const uint16_t link_pc = static_cast<uint16_t>(
            kCartridgeLoopPc + link * 4U);
        const bool last_link = link + 1U == kCartridgeLinkCount;
        const uint16_t target = last_link
            ? handoff_pc
            : static_cast<uint16_t>(link_pc + 4U);
        p0_code[link_pc] = 0x00; // NOP
        p0_code[link_pc + 1] = 0xC3; // JP target
        p0_code[link_pc + 2] = static_cast<uint8_t>(target & 0xff);
        p0_code[link_pc + 3] = static_cast<uint8_t>(target >> 8);
    }
    // The host keeps force_irq low through the measured chain, then waits for
    // this original interrupt-ready state before asserting it.
    p0_code[handoff_pc + 0] = 0xFB; // EI
    p0_code[handoff_pc + 1] = 0x76; // HALT
    // Page 3 data
    std::vector<uint8_t> p3_data(16384, 0x00);
    p3_data[0] = 0x42; // Magic byte at 0xC000

    std::vector<Chunk> chunks = {
        {"cb00", p0_code},
        {"cb03", p3_data}
    };
    auto image = build_cpr_image(chunks);

    // 1. CPR Download
    h.download(image);

    // 2. Observe reset apply countdown & release
    require(h.dut.dbg_reset == 1, "Reset must be asserted after CPR download");
    wait_for_reset_release(h, "deterministic CPR apply");
    require(h.dut.dbg_reset == 0, "Reset must release after apply countdown");

    // 3. Observe real T80 opcode fetch at PC = 0x0000 from cartridge page 0
    std::cout << "  Observing reset-vector execution at PC=0x0000..." << std::endl;
    // Step until M1 memory read phase
    wait_for_m1_read(h, "reset-vector");
    require(h.dut.dbg_pc == 0x0000, "First instruction PC must be 0x0000");

    // 4. Run until ASIC unlock sequence completes
    std::cout << "  Running through 16-byte ASIC unlock sequence..." << std::endl;
    int last_seq = -1;
    uint16_t last_pc = 0xFFFF;
    int traced = 0;
    for (int i = 0; i < 100000; ++i) {
        h.tick();
        if (getenv("P10RAW") ? (traced < 300) : (getenv("P10TRACE") && h.dut.dbg_pc != last_pc && traced < 200)) {
            last_pc = h.dut.dbg_pc;
            ++traced;
            std::cout << "T pc=" << std::hex << last_pc
                      << " din=" << (int)h.dut.dbg_din
                      << " addr=" << h.dut.dbg_addr
                      << " iorq=" << (int)!h.dut.dbg_iorq_n
                      << " wr=" << (int)!h.dut.dbg_wr_n
                      << " own=" << (int)h.dut.dbg_cart_own
                      << " stall=" << (int)h.dut.dbg_cart_stall
                      << " mc=" << (int)h.dut.dbg_mcycle
                      << " ts=" << (int)h.dut.dbg_tstate
                      << " ir=" << (int)h.dut.dbg_ir
                      << " mcmax=" << (int)h.dut.dbg_mcmax
                      << " waitn=" << (int)h.dut.dbg_cpu_waitn
                      << " cenp=" << (int)h.dut.dbg_cen_p
                      << " mreq=" << (int)!h.dut.dbg_mreq_n
                      << " rd=" << (int)!h.dut.dbg_rd_n
                      << " m1=" << (int)!h.dut.dbg_m1_n
                      << std::dec << std::endl;
        }
        if ((int)h.dut.dbg_unlock_seq != last_seq) {
            last_seq = (int)h.dut.dbg_unlock_seq;
            std::cout << "Cycle " << i << ": unlock_seq=" << last_seq
                      << " unlock_done=" << (int)h.dut.dbg_unlock_done
                      << " PC=0x" << std::hex << h.dut.dbg_pc
                      << " Addr=0x" << h.dut.dbg_addr
                      << " Dout=0x" << (int)h.dut.dbg_dout << std::dec << std::endl;
        }
        if (h.dut.dbg_unlock_done) break;
    }
    require(h.dut.dbg_unlock_done == 1, "ASIC must be unlocked after 16-byte unlock sequence");

    // 5. Run until RMR2 ASIC page mapping (&4000-&7FFF) is enabled
    std::cout << "  Observing RMR2 ASIC page mapping (&4000-&7FFF)..." << std::endl;
    bool aspage_on = false;
    for (int i = 0; i < 5000; ++i) {
        h.tick();
        if (h.dut.dbg_asic_page_on) {
            aspage_on = true;
            break;
        }
    }
    require(aspage_on, "ASIC page must be enabled via RMR2");

    // 6. Run until Palette write and CRTC3 register write
    std::cout << "  Observing Palette and CRTC3 writes..." << std::endl;
    bool saw_palette_wr = false;
    bool saw_crtc_wr = false;
    for (int i = 0; i < 5000; ++i) {
        h.tick();
        if (h.dut.dbg_asic_wr && (h.dut.dbg_asic_addr == 0x2420) && (h.dut.dbg_asic_val == 0x34)) {
            saw_palette_wr = true;
        }
        if (h.dut.dbg_crtc_wr && (h.dut.dbg_crtc_reg == 1) && (h.dut.dbg_crtc_val == 0x28)) {
            saw_crtc_wr = true;
        }
        if (saw_palette_wr && saw_crtc_wr) break;
    }
    require(saw_palette_wr, "ASIC Palette write to &6420 must be captured");
    require(saw_crtc_wr, "CRTC R1 write must be captured");

    // 7. Run until FDC motor write
    std::cout << "  Observing FDC motor write..." << std::endl;
    bool saw_motor_on = false;
    for (int i = 0; i < 5000; ++i) {
        h.tick();
        if (h.dut.dbg_motor) {
            saw_motor_on = true;
            break;
        }
    }
    require(saw_motor_on, "FDC motor must be turned on by write to &FA00");

    // The upper-page read is visible on the existing production-path data
    // probe.  Keep this as an assertion rather than merely exercising the
    // page-select write, since the CPR fixture deliberately prepares 0x42.
    require(h.upper_page_magic_seen, "upper cartridge page read must return 0x42");

    // 8. Measure a fixed 64 MHz window while the unrolled NOP/JP chain
    // executes from cartridge. Every distinct M1 memory-read phase counts as
    // forward execution progress; the other counters are sampled directly
    // from the production probes.
    std::cout << "  Measuring cartridge execution timing..." << std::endl;
    wait_for_loop_entry(h, kCartridgeLoopPc, "cartridge");
    const LoopMeasurement cartridge =
        measure_loop(h, kLoopMeasureTicks);
    std::cout << "    cartridge: ticks=" << cartridge.elapsed_ticks
              << " m1=" << cartridge.m1_fetches
              << " cart_stall=" << cartridge.cart_stall_cycles
              << " max_stall_run=" << cartridge.max_cart_stall_run
              << " wait_low=" << cartridge.cpu_wait_low_cycles
              << " max_wait_run=" << cartridge.max_cpu_wait_low_run
              << " physical_cart_reads=" << cartridge.physical_cart_reads
              << std::endl;
    require(cartridge.m1_fetches > 0, "cartridge loop made no progress");
    require(cartridge.physical_cart_reads > 0,
            "cartridge loop must issue physical cartridge reads");
    require(cartridge.cart_stall_cycles > 0,
            "cartridge loop must expose nonzero cartridge stalls");
    require(cartridge.cpu_wait_low_cycles > 0,
            "cartridge loop must expose low CPU WAIT cycles");
    require(cartridge.max_cart_stall_run > 0 &&
                cartridge.max_cart_stall_run < cartridge.elapsed_ticks,
            "cartridge stall distribution must be bounded and nonzero");
    require(cartridge.max_cpu_wait_low_run > 0 &&
                cartridge.max_cpu_wait_low_run < cartridge.elapsed_ticks,
            "CPU WAIT-low distribution must be bounded and nonzero");
    require(cartridge.max_cart_stall_run == 11,
            "deterministic harness cartridge stall run must remain 11 ticks");
    require(cartridge.cpu_wait_low_cycles == cartridge.cart_stall_cycles &&
                cartridge.max_cpu_wait_low_run == cartridge.max_cart_stall_run,
            "CPU WAIT must exactly follow cartridge ownership stalls");

    // 9. Retain the original production-path interrupt acknowledge check.
    h.run_until_pc(static_cast<uint16_t>(handoff_pc + 1U));
    std::cout << "  Observing Interrupt request & Z80 acknowledge cycle..." << std::endl;
    h.dut.force_irq = 1;
    bool saw_int_ack = false;
    for (int i = 0; i < 5000; ++i) {
        h.tick();
        if (h.dut.dbg_int_ack) {
            saw_int_ack = true;
            break;
        }
    }
    require(saw_int_ack, "CPU must acknowledge interrupt with M1+IORQ active");
    h.dut.force_irq = 0;

    h.run_cycles(100);

    std::cout << "PASS: test_p10a_deterministic_boot" << std::endl;
}

void test_p10a_vram_client_wiring() {
    std::cout << "Running test_p10a_vram_client_wiring..." << std::endl;
    Harness h;
    h.initialize();

    h.download(build_static_crtc_cpr());
    wait_for_reset_release(h, "VRAM wiring CPR apply");

    bool programmed = false;
    for (uint64_t tick = 0; tick < 100000 && !programmed; ++tick) {
        h.tick();
        programmed = h.dut.dbg_crtc_wr && h.dut.dbg_crtc_reg == 9 &&
                     h.dut.dbg_crtc_val == 7;
    }
    require(programmed, "fixture program did not configure the production CRTC path");

    bool previous_request = h.dut.dbg_sdram_vram_req;
    unsigned checked = 0;
    std::unordered_set<uint32_t> distinct_addresses;
    bool transaction_pending = false;
    uint32_t expected_address = 0;
    uint32_t active_row = 0;
    bool active_seen = false;

    for (uint64_t tick = 0; tick < 400000 && checked < 32; ++tick) {
        const uint32_t source_word_addr = h.dut.dbg_video_vram_addr;
        h.tick();
        const bool request = h.dut.dbg_sdram_vram_req;
        if (request && !previous_request) {
            require(!transaction_pending,
                    "second VRAM request admitted before the first physical read");
            expected_address = 0x20000U | (source_word_addr << 1U);
            transaction_pending = true;
            active_seen = false;
        }

        const uint8_t command = (h.dut.sdram_nras ? 0b100 : 0b000) |
                                (h.dut.sdram_ncas ? 0b010 : 0b000) |
                                (h.dut.sdram_nwe  ? 0b001 : 0b000);
        if (transaction_pending && command == CMD_ACTIVE) {
            require(!active_seen, "VRAM transaction issued two ACTIVE commands");
            require(h.dut.sdram_ba == 0,
                    "6128+ fixture VRAM transaction used the wrong physical SDRAM bank");
            active_row = h.dut.sdram_a & 0x1fffU;
            active_seen = true;
        } else if (transaction_pending && command == CMD_READ) {
            require(active_seen, "VRAM READ appeared without its ACTIVE command");
            const uint32_t physical_address = (active_row << 9U) |
                ((static_cast<uint32_t>(h.dut.sdram_a) & 0x100U) << 14U) |
                ((static_cast<uint32_t>(h.dut.sdram_a) & 0xffU) << 1U);
            require(physical_address == expected_address,
                    "motherboard VRAM word address did not reach the physical SDRAM read");
            distinct_addresses.insert(physical_address);
            ++checked;
            transaction_pending = false;
        }
        previous_request = request;
    }

    require(checked == 32, "too few SDRAM video-client requests for a wiring proof");
    require(distinct_addresses.size() >= 4,
            "VRAM wiring proof observed fewer than four unique physical addresses");
    std::cout << "PASS: production VRAM address/bank reaches the SDRAM video client"
              << std::endl;
}

void require_capture_shape_equal(const FrameCaptureStats &first,
                                 const FrameCaptureStats &second) {
    require(first.sample_count == second.sample_count,
            "consecutive capture frames have different sample counts");
    require(first.raw_hsync_edges == second.raw_hsync_edges &&
                first.raw_hsync_rises == second.raw_hsync_rises &&
                first.raw_vsync_edges == second.raw_vsync_edges &&
                first.raw_vsync_rises == second.raw_vsync_rises,
            "consecutive capture frames have different raw sync edge counts");
    require(first.selected_hsync_edges == second.selected_hsync_edges &&
                first.selected_hsync_rises == second.selected_hsync_rises &&
                first.selected_vsync_edges == second.selected_vsync_edges &&
                first.selected_vsync_rises == second.selected_vsync_rises,
            "consecutive capture frames have different selected sync edge counts");
    require(first.de_active_ticks == second.de_active_ticks &&
                first.hblank_active_ticks == second.hblank_active_ticks &&
                first.vblank_active_ticks == second.vblank_active_ticks,
            "consecutive capture frames have different active/blank counts");
}

CaptureProvenance make_synthetic_provenance(const Harness &h,
                                            const std::vector<uint8_t> &image,
                                            const std::string &simulator_identity) {
    CaptureProvenance provenance;
    provenance.cpr_size_bytes = image.size();
    provenance.cpr_content_hash = hash_bytes(image);
    provenance.plus_model = static_cast<uint8_t>(h.dut.plus_model_i);
    provenance.production_clocking = h.dut.production_clocking != 0;
    provenance.simulator_binary_identity = simulator_identity;
    return provenance;
}

void test_b3_frame_capture_smoke(const std::string &simulator_identity) {
    std::cout << "Running test_b3_frame_capture_smoke..." << std::endl;
    Harness h;
    h.initialize();
    const std::vector<uint8_t> image = build_static_crtc_cpr();
    h.download(image);
    wait_for_cpr_apply(h);

    bool programmed = false;
    for (uint64_t tick = 0; tick < 100000 && !programmed; ++tick) {
        h.tick();
        programmed = h.dut.dbg_crtc_wr && h.dut.dbg_crtc_reg == 9 &&
                     h.dut.dbg_crtc_val == 7;
    }
    require(programmed, "capture smoke program did not configure the CRTC");

    // Discard two complete frame delimiters after the last CRTC write.  This
    // leaves the capture on a repeatable steady-state boundary without
    // baking a current-RTL image or hash into the test.
    NullStreamBuf stream_buffer;
    std::ostream stream(&stream_buffer);
    const CaptureProvenance provenance =
        make_synthetic_provenance(h, image, simulator_identity);
    const CaptureResult capture = capture_video_frames(
        h, 2, stream, provenance, kCaptureWarmupFrames);
    require(capture.frames.size() == 2,
            "capture smoke did not return two complete frames");
    const FrameCaptureStats &first = capture.frames[0];
    const FrameCaptureStats &second = capture.frames[1];
    require_capture_shape_equal(first, second);
    require(first.hash != 0 && second.hash != 0,
            "capture smoke produced an empty frame hash");
    require(first.hash == second.hash,
            "steady-state capture frames produced different hashes");
    require(first.de_active_ticks > 0 && second.de_active_ticks > 0,
            "capture smoke observed no active video ticks");
    require(first.selected_hsync_edges > 0 &&
                first.selected_vsync_rises > 0 &&
                second.selected_hsync_edges > 0 &&
                second.selected_vsync_rises > 0,
            "capture smoke observed no selected sync edges");
    std::cout << "PASS: B3 two complete steady-state frames capture identically"
              << std::endl;
}

// A CPR with a corrupt RIFF magic must trip the parser's STATE_ERROR abort
// (plus_cpr_parser.v STATE_HEADER_RIFF expects 'R' first) instead of
// booting into an undefined machine state.
void test_b3_malformed_cpr_rejected() {
    std::cout << "Running test_b3_malformed_cpr_rejected..." << std::endl;
    Harness h;
    h.initialize();
    std::vector<uint8_t> image = build_static_crtc_cpr();
    image[0] = 0x58; // 'X': not 'R', so the header state aborts.
    bool rejected = false;
    try {
        h.download(image);
    } catch (const TestFailure &) {
        rejected = true;
    }
    require(rejected, "corrupt-magic CPR download did not abort");
    std::cout << "PASS: corrupt-magic CPR cannot boot" << std::endl;
}

// A truncated-but-well-formed prefix must abort at apply time (parser never
// reaches STATE_DONE, so no load_commit): the harness must fail, not run.
void test_b3_truncated_cpr_rejected() {
    std::cout << "Running test_b3_truncated_cpr_rejected..." << std::endl;
    Harness h;
    h.initialize();
    const std::vector<uint8_t> image = build_static_crtc_cpr();
    const std::vector<uint8_t> prefix(image.begin(), image.begin() + 8);
    bool rejected = false;
    try {
        h.download(prefix);
        wait_for_cpr_apply(h);
    } catch (const TestFailure &) {
        rejected = true;
    }
    require(rejected, "truncated CPR prefix was accepted as a cartridge");
    std::cout << "PASS: truncated CPR cannot boot" << std::endl;
}

// Two independent harnesses capturing the same synthetic cartridge must emit
// byte-identical serializations.  This pins determinism of the declared
// stream format; it is not a hardware oracle (no golden hash is minted).
void test_b3_repeat_capture_stable(const std::string &simulator_identity) {
    std::cout << "Running test_b3_repeat_capture_stable..." << std::endl;
    const std::vector<uint8_t> image = build_static_crtc_cpr();
    std::string first_serialized;
    FrameCaptureStats first_stats;
    {
        Harness h;
        h.initialize();
        h.download(image);
        wait_for_cpr_apply(h);
        std::ostringstream stream;
        const CaptureResult capture = capture_video_frames(
            h, 1, stream, make_synthetic_provenance(h, image, simulator_identity),
            kCaptureWarmupFrames);
        require(capture.frames.size() == 1,
                "first repeat capture did not return one complete frame");
        first_serialized = stream.str();
        first_stats = capture.frames[0];
    }
    {
        Harness h;
        h.initialize();
        h.download(image);
        wait_for_cpr_apply(h);
        std::ostringstream stream;
        const CaptureResult capture = capture_video_frames(
            h, 1, stream, make_synthetic_provenance(h, image, simulator_identity),
            kCaptureWarmupFrames);
        require(capture.frames.size() == 1,
                "second repeat capture did not return one complete frame");
        require(stream.str() == first_serialized,
                "independent synthetic captures serialized differently");
        require_capture_shape_equal(first_stats, capture.frames[0]);
        require(first_stats.hash == capture.frames[0].hash,
                "independent synthetic captures hashed differently");
    }
    std::cout << "PASS: independent synthetic captures serialize identically"
              << std::endl;
}

void test_real_u765_edsk_read() {
	std::cout << "Running test_real_u765_edsk_read..." << std::endl;
	Harness h;
	h.dut.production_clocking = 1;
	h.initialize();
	h.load_disk("../../rtl/u765/test.dsk");
	h.mount_disk();
	const uint32_t mount_reads = h.fdc_sd_reads;

	std::vector<uint8_t> page(16384, 0x00);
	page[0] = 0xc3; // JP &0100
	page[1] = 0x00;
	page[2] = 0x01;
	size_t pc = 0x0100;
	auto emit = [&](uint8_t byte) { page[pc++] = byte; };
	auto patch_relative = [&](size_t operand, size_t target) {
		const int offset = static_cast<int>(target) -
		                   static_cast<int>(operand + 1);
		require(offset >= -128 && offset <= 127,
		        "fixture relative branch is out of range");
		page[operand] = static_cast<uint8_t>(offset);
	};
	auto emit_wait_rqm = [&]() {
		emit(0x01); emit(0xde); emit(0xfb); // LD BC,&FBDE (MSR)
		const size_t poll = pc;
		emit(0xed); emit(0x78);             // IN A,(C)
		emit(0xe6); emit(0x80);             // AND &80 (RQM)
		emit(0x28);                         // JR Z,poll
		const size_t displacement = pc;
		emit(0x00);
		patch_relative(displacement, poll);
	};
	auto emit_fdc_byte = [&](uint8_t byte) {
		emit_wait_rqm();
		emit(0x0c);                         // INC C -> &FBDF data
		emit(0x3e); emit(byte);             // LD A,byte
		emit(0xed); emit(0x79);             // OUT (C),A
	};

	emit(0xf3);                            // DI
	emit(0x31); emit(0x00); emit(0xc0);   // LD SP,&C000
	emit(0x01); emit(0xdd); emit(0xfa);   // LD BC,&FADD (Plus motor alias)
	emit(0x3e); emit(0x01);               // LD A,1
	emit(0xed); emit(0x79);               // OUT (C),A
	// Track 0/head 0/sector &41 is the first independently described sector
	// in the tracked known-good EDSK. Its 512-byte payload starts at file LBA 1.
	const uint8_t read_command[] = {
		0x46, 0x00, 0x00, 0x00, 0x41, 0x02, 0x41, 0x1e, 0xff
	};
	for (const uint8_t byte : read_command) emit_fdc_byte(byte);
	emit(0x01); emit(0xde); emit(0xfb);   // LD BC,&FBDE (MSR)
	auto emit_fdc_read_to = [&](uint16_t address, uint8_t mask,
	                            uint8_t expected) {
		const size_t poll = pc;
		emit(0xed); emit(0x78);             // IN A,(C)
		emit(0xe6); emit(mask);             // AND mask
		emit(0xfe); emit(expected);         // CP expected
		emit(0x20);                         // JR NZ,poll
		const size_t displacement = pc;
		emit(0x00);
		patch_relative(displacement, poll);
		emit(0x0c);                         // INC C -> data
		emit(0xed); emit(0x78);             // IN A,(C)
		emit(0x32);                         // LD (address),A
		emit(static_cast<uint8_t>(address & 0xff));
		emit(static_cast<uint8_t>(address >> 8));
		emit(0x0d);                         // DEC C -> status
	};
	// Unroll the transfer so this production-path discriminator depends only
	// on IN/OUT and absolute stores, not on TV80 loop-register corner cases.
	for (unsigned byte = 0; byte < 512; ++byte)
		emit_fdc_read_to(static_cast<uint16_t>(0x8000 + byte), 0xf0, 0xf0);
	for (unsigned delay = 0; delay < 128; ++delay) emit(0x00); // result settle

	// Consume and preserve the seven-byte result phase at &8200..&8206.
	for (unsigned result = 0; result < 7; ++result)
		emit_fdc_read_to(static_cast<uint16_t>(0x8200 + result), 0xf0, 0xd0);
	emit(0x3e); emit(0xa5);               // LD A,&A5
	emit(0x32); emit(0xff); emit(0x82);   // LD (&82FF),A completion marker
	emit(0x76);                           // HALT

	h.download(build_cpr_image({{"cb00", page}}));
	require(h.dut.dbg_reset,
	        "CPR apply must reset the mounted controller before execution");
	for (uint64_t ticks = 0; ticks < 30000000 && !h.fdc_success; ++ticks)
		h.tick();
	require(h.fdc_success,
	        "production CPU did not complete the real-u765 READ DATA program");
	std::cout << "  trace checkpoint: mount_sd_reads=" << mount_reads
	          << " total_sd_reads=" << h.fdc_sd_reads
	          << " last_lba=" << h.fdc_last_lba
	          << " pending_rd=" << static_cast<unsigned>(h.dut.fdc_sd_rd)
	          << " pending_lba=" << h.dut.fdc_sd_lba
	          << " fdc_writes=" << h.fdc_writes.size()
	          << " fdc_reads=" << h.fdc_reads.size()
	          << " fdc_cpu_latches=" << h.fdc_cpu_latch_reads.size()
	          << " state=" << static_cast<unsigned>(h.dut.dbg_fdc_state)
	          << " msr=" << std::hex
	          << static_cast<unsigned>(h.dut.dbg_fdc_msr)
	          << " seek=" << h.dut.dbg_fdc_seek_pos
	          << " dirty=" << static_cast<unsigned>(h.dut.dbg_fdc_trackinfo_dirty)
	          << " sector_pos=" << h.dut.dbg_fdc_sector_pos
	          << " byte_count=" << static_cast<unsigned>(h.dut.dbg_fdc_byte_count)
	          << " results=" << std::hex
	          << static_cast<unsigned>(h.fdc_results[0]) << "/"
	          << static_cast<unsigned>(h.fdc_results[1]) << "/"
	          << static_cast<unsigned>(h.fdc_results[2]) << std::dec << '\n';
	// DIAG (bounded first-divergence record, 2026-09-03; recovered from the
	// preserved experiment's early observe-only revision, not its later
	// synthetic stop/resume workaround): poll-spin histogram,
	// shifted-payload signature, and raw result-slot reads. Prints only;
	// asserts nothing. Result slots are phase-unverified (controller state
	// 13 at the trace checkpoint, before COMMAND_READ_RESULTS): overrun is
	// UNKNOWN from this trace. The XFAIL below is unchanged in strength.
	{
		unsigned data_blocks = 0;
		unsigned blocks_with_spin = 0;
		unsigned polls_since_data = 0;
		unsigned max_polls_per_block = 0;
		for (const auto &latch : h.fdc_cpu_latch_reads) {
			if (latch.addr == 0xfbdf) {
				++data_blocks;
				max_polls_per_block = std::max(max_polls_per_block,
				                               polls_since_data);
				if (polls_since_data > 1) ++blocks_with_spin;
				polls_since_data = 0;
			} else if (latch.addr == 0xfbde) {
				++polls_since_data;
			}
		}
		std::cout << "  DIAG polls: data_blocks=" << data_blocks
		          << " blocks_with_spin(>1 poll)=" << blocks_with_spin
		          << " max_polls_per_block=" << max_polls_per_block
		          << " total_latches=" << h.fdc_cpu_latch_reads.size()
		          << '\n';
		unsigned shift_match = 0;
		for (unsigned i = 1; i < 512; ++i)
			if (h.fdc_payload[i] == h.disk_image[0x200 + i - 1])
				++shift_match;
		std::cout << "  DIAG shift: payload[0]=0x" << std::hex
		          << static_cast<unsigned>(h.fdc_payload[0])
		          << " disk[0x200]=0x"
		          << static_cast<unsigned>(h.disk_image[0x200])
		          << " shifted_bytes_matching=" << std::dec << shift_match
		          << "/511 last_disk_byte=0x" << std::hex
		          << static_cast<unsigned>(h.disk_image[0x3ff]) << std::dec
		          << '\n';
		unsigned shown = 0;
		for (unsigned i = 1; i < 512 && shown < 8; ++i) {
			if (h.fdc_payload[i] != h.disk_image[0x200 + i - 1]) {
				std::cout << "  DIAG shift deviation at byte " << i
				          << ": got 0x" << std::hex
				          << static_cast<unsigned>(h.fdc_payload[i])
				          << " shifted-expect 0x"
				          << static_cast<unsigned>(
				                 h.disk_image[0x200 + i - 1])
				          << " unshifted-expect 0x"
				          << static_cast<unsigned>(h.disk_image[0x200 + i])
				          << std::dec << '\n';
				++shown;
			}
		}
		std::cout << "  DIAG result-slots (raw, phase-unverified):";
		for (unsigned i = 0; i < 7; ++i)
			std::cout << (i ? "/" : " ") << std::hex
			          << static_cast<unsigned>(h.fdc_results[i]);
		std::cout << std::dec
		          << " results_shifted_by_one="
		          << ((h.fdc_results[0] == h.disk_image[0x3ff]) ? 1 : 0)
		          << '\n';
	}
	require(h.dut.dbg_motor, "Plus motor alias did not enable Drive A");
	require(h.fdc_sd_reads > mount_reads,
	        "READ DATA did not issue a post-reset SD request");
	require(std::find(h.fdc_sd_lbas.begin(), h.fdc_sd_lbas.end(), 1) !=
	            h.fdc_sd_lbas.end(),
	        "first EDSK sector read did not request payload LBA 1");
	require(h.fdc_writes.size() >= 9,
	        "CPU/FDC trace missed READ DATA command bytes");
	for (unsigned i = 0; i < 9; ++i)
		require(h.fdc_writes[h.fdc_writes.size() - 9 + i] == read_command[i],
		        "CPU/FDC command trace diverged at byte " + std::to_string(i));
	unsigned first_payload_mismatch = 512;
	for (unsigned i = 0; i < 512; ++i) {
		require(h.fdc_payload_seen[i],
		        "CPU did not store payload byte " + std::to_string(i));
		if (first_payload_mismatch == 512 &&
		    h.fdc_payload[i] != h.disk_image[0x200 + i])
			first_payload_mismatch = i;
	}
	for (unsigned i = 0; i < 7; ++i)
		require(h.fdc_result_seen[i],
		        "CPU did not consume result byte " + std::to_string(i));
	require(h.fdc_first_payload_latch_count_seen,
	        "first payload store did not capture the preceding CPU latch boundary");
	require(h.fdc_first_payload_latch_count >= 2,
	        "first payload store lacks preceding FDC status/data latch events");
	const FdcCpuLatchRead &data_latch =
		h.fdc_cpu_latch_reads[h.fdc_first_payload_latch_count - 1];
	const FdcCpuLatchRead *status_latch = nullptr;
	for (size_t i = h.fdc_first_payload_latch_count - 1; i-- > 0;) {
		if (h.fdc_cpu_latch_reads[i].addr == 0xfbde) {
			status_latch = &h.fdc_cpu_latch_reads[i];
			break;
		}
	}
	require(status_latch != nullptr,
	        "first payload store lacks a preceding FDC status latch");
	require(data_latch.addr == 0xfbdf,
	        "last FDC latch before the first payload store was not the data port");
	auto require_clean_fdc_path = [](const FdcCpuLatchRead &sample,
	                                const char *kind) {
		require(sample.expected == sample.fdc_dout &&
		            sample.fdc_dout == sample.fdc_bus &&
		            static_cast<uint8_t>(sample.ram_dout & sample.fdc_bus) ==
		                sample.top_bus &&
		            sample.fdc_bus == sample.top_bus &&
		            sample.top_bus == sample.mb_bus &&
		            sample.mb_bus == sample.cpu_di,
		        std::string("FDC ") + kind +
		            " differed across controller, bus muxes, and CPU latch");
	};
	require_clean_fdc_path(*status_latch, "status");
	require(status_latch->expected == status_latch->msr,
	        "FDC status-port source differed from the controller MSR");
	require_clean_fdc_path(data_latch, "data");
	require(h.fdc_payload[0] == data_latch.cpu_di,
	        "first payload store differed from the preceding CPU data latch");
	require(first_payload_mismatch != 512,
	        "XPASS: production-clock TV80 consumed the complete EDSK payload; remove the XFAIL");
	require(first_payload_mismatch == 0,
	        "payload XFAIL changed shape; re-trace the first divergence");
	std::cout << "XFAIL fdc-payload-poll: production-clock TV80 stored 0x"
	          << std::hex << static_cast<unsigned>(h.fdc_payload[0])
	          << " instead of 0x"
	          << static_cast<unsigned>(h.disk_image[0x200])
	          << " at payload byte 0; exact CPU latch saw selected u765 data/state/MSR="
	          << static_cast<unsigned>(data_latch.cpu_di) << "/"
	          << static_cast<unsigned>(data_latch.state) << "/"
          << static_cast<unsigned>(data_latch.msr)
          << " m_data/buff_wait/bytes_left="
          << static_cast<unsigned>(data_latch.m_data) << "/"
          << static_cast<unsigned>(data_latch.buff_wait) << "/"
          << static_cast<unsigned>(data_latch.bytes_left)
          << " after status latch=" << static_cast<unsigned>(status_latch->cpu_di)
	          << "/" << static_cast<unsigned>(status_latch->state) << std::dec << '\n';
	std::cout << "PASS: production decode/command/media request; payload divergence retained as XFAIL"
	          << std::endl;
}

// B8-4 production-motherboard coherence: CPU 0x0000 aliases the low byte
// of SDRAM video word 0 (Amstrad_MMU base map; fixture banks 0;
// p10_boot_test_top maps word 0 to byte 0x20000). Degenerate raster
// R0=0,R1=1,R2=255,R4=0,R6=1,R7=127,R9=0 holds VA=0 with DE active and no
// HSYNC/VSYNC blanking. production_clocking=1 uses the Amstrad.sv divider
// for ce_16 and clkref_int. GA RMR 0x82 programs gamode 2, but with no HSYNC
// the pixel decoder never re-latches: mode_q stays 0, so this fixture pins
// mode-0 mapping, not mode-2 accuracy. Pen 0/border HW20 black, pens 1..15
// HW11 white. Byte path (Amstrad_motherboard/asic_video): vram_dout word ->
// vram_d byte -> plus_vidword low half on cclk_p -> vid_even on
// CLKEN(cclk_n)&PIXEN(ce_16) with pix_cnt reset to 0 -> mode-0 pen ->
// palette -> RGB registered on the next PIXEN (one-dot presentation
// latency). Mode-0 byte 0xFF gives pen 15 ({b1,b5,b3,b7}/{b0,b4,b2,b6} all
// ones, programmed white) for both pixels of the half; 0x00 gives pen 0.
// Steady baseline is reset-grey HW0 0x666, not programmed black.
void test_p10b_video_coherence_pixel() {
    std::cout << "Running test_p10b_video_coherence_pixel..." << std::endl;
    Harness h;
    for (unsigned w = 0; w < 128; ++w) {
        h.preload(0, 0x20000U + (w << 1), 0x00);
        h.preload(0, 0x20000U + (w << 1) + 1, 0x00);
    }
    h.dut.production_clocking = 1;
    h.initialize();

    std::vector<uint8_t> program(16384, 0x00);
    size_t pc = 0;
    auto emit = [&](uint8_t byte) { program[pc++] = byte; };
    auto write_crtc = [&](uint8_t reg, uint8_t value) {
        emit(0x01); emit(0x00); emit(0xBC); // LD BC,&BC00
        emit(0x3E); emit(reg);              // LD A,reg
        emit(0xED); emit(0x79);             // OUT (C),A
        emit(0x01); emit(0x00); emit(0xBD); // LD BC,&BD00
        emit(0x3E); emit(value);            // LD A,value
        emit(0xED); emit(0x79);             // OUT (C),A
    };
    auto write_ga = [&](uint8_t value) {
        emit(0x01); emit(0x00); emit(0x7F); // LD BC,&7F00
        emit(0x3E); emit(value);            // LD A,value
        emit(0xED); emit(0x79);             // OUT (C),A
    };

    emit(0xF3); // DI

    // Degenerate raster holding video address at word 0 continuously
    write_crtc(0, 0);   // R0 = 0 (1 char per line)
    write_crtc(1, 1);   // R1 = 1 (1 char displayed)
    write_crtc(2, 255); // R2 = 255 (no HSYNC blanking)
    write_crtc(4, 0);   // R4 = 0 (1 row per frame)
    write_crtc(6, 1);   // R6 = 1 (1 row displayed)
    write_crtc(7, 127); // R7 = 127 (no VSYNC blanking)
    write_crtc(9, 0);   // R9 = 0 (1 scanline per row)

    // GA configuration: mode 2, ROMs enabled, black pen 0, white pens 1..15, black border
    write_ga(0x82); // RMR: mode 2, ROMs enabled
    write_ga(0x00); write_ga(0x40 | 20); // pen 0 <- HW20 black
    for (uint8_t p = 1; p <= 15; ++p) {
        write_ga(p); write_ga(0x40 | 11); // pens 1..15 <- HW11 white
    }
    write_ga(0x10); write_ga(0x40 | 20); // border <- HW20 black

    const size_t setup_end_pc = pc;

    // Straight-line NOP baseline delay: 256 NOPs (no jumps)
    for (int i = 0; i < 256; ++i) emit(0x00);

    // Write 0xFF to watched RAM alias (CPU 0x0000 -> physical RAM 0x20000)
    emit(0x3E); emit(0xFF);             // LD A, &FF
    emit(0x32); emit(0x00); emit(0x00); // LD (0000), A
    emit(0x76);                         // HALT

    require(pc < 0x0400, "coherence fixture program overflowed initial area");

    h.download(build_cpr_image({{"cb00", program}}));
    while (h.dut.dbg_reset) h.tick();

    const uint32_t kPhys = 0x20000U;
    const uint16_t kWhite = 0x0FFF;
    const uint64_t kBudget = 200000;

    size_t write_index = h.bank0_writes.size();
    bool accepted = false;
    uint64_t accepted_tick = 0;
    // Baseline is steady reset-grey (HW0, 0x666) with DE active and no
    // HSYNC blanking, not programmed black: RMR mode-2 lands (gamode==2
    // observed) but pen-0 black does not yield black RGB in this degenerate
    // fixture (observed steady 0x666 with word 0, addr 0). Forbid white
    // throughout the baseline; white after the write remains discriminating
    // because pens 1..15 are programmed white (proven by white appearing).
    unsigned baseline_ticks = 0;
    bool gamode_ok = false;
    uint8_t modeq_seen = 0xff;
    uint16_t baseline_rgb = 0xffff;

    bool word_ok = false;
    uint64_t word_tick = 0;
    bool byte_ok = false;
    uint64_t byte_tick = 0;
    bool load_ok = false;
    uint64_t load_tick = 0;
    bool white_ok = false;
    uint64_t white_tick = 0;

    for (uint64_t i = 0; i < kBudget; ++i) {
        // Pre-edge sample: the enables/source the posedge inside tick()
        // will consume. Post-edge assertions below prove actual loads,
        // not already-held levels.
        const bool pre_cclk_p = h.dut.dbg_video_cclk_p;
        const uint8_t pre_vram_d = h.dut.dbg_video_vram_byte;
        const bool pre_ce16 = h.dut.dbg_video_ce16;
        const bool pre_cclk_n = h.dut.dbg_video_cclk_n;
        const uint8_t pre_plus_lo =
            static_cast<uint8_t>(h.dut.dbg_video_plus_vidword & 0xffU);
        const uint8_t pre_vid_even = h.dut.dbg_video_vid_even;
        const uint8_t pre_pixcnt = h.dut.dbg_video_pixcnt;
        const uint8_t pre_modeq = h.dut.dbg_video_modeq;

        h.tick();

        for (; write_index < h.bank0_writes.size(); ++write_index) {
            const auto &wr = h.bank0_writes[write_index];
            if (wr.byte_address == kPhys && wr.value == 0xFF && !accepted) {
                accepted = true;
                accepted_tick = wr.tick;
                require(baseline_ticks >= 1000,
                        "write accepted before 1000 baseline ticks");
                require(gamode_ok, "RMR mode-2 programming never observed before write");
                std::cout << "  accepted: LD (0000),A -> phys 0x20000 = 0xff @t="
                          << accepted_tick << " (baseline_ticks=" << baseline_ticks
                          << " baseline_rgb=0x" << std::hex << baseline_rgb << std::dec
                          << " gamode=2 modeq=" << (unsigned)modeq_seen << ")\n";
            }
        }

        const uint16_t rgb = h.dut.dbg_video_rgb;
        const bool is_white = (rgb == kWhite);
        const bool de = h.dut.dbg_raw_de;
        const bool hsync = h.dut.dbg_raw_hsync;
        const uint8_t post_plus_lo =
            static_cast<uint8_t>(h.dut.dbg_video_plus_vidword & 0xffU);
        const uint8_t post_vid_even = h.dut.dbg_video_vid_even;
        const uint8_t post_pixcnt = h.dut.dbg_video_pixcnt;
        const uint8_t post_modeq = h.dut.dbg_video_modeq;

        if (!accepted) {
            if (h.dut.dbg_pc >= setup_end_pc) {
                require(!is_white, "white observed before CPU write");
                require((h.dut.dbg_video_vram_addr & 0x7fff) == 0,
                        "video address moved away from 0 during baseline");
                require(h.dut.dbg_video_vram_word == 0x0000,
                        "video word non-zero during baseline");
                if (h.dut.dbg_video_gamode == 2) gamode_ok = true;
                modeq_seen = h.dut.dbg_video_modeq;
                baseline_rgb = rgb;
                ++baseline_ticks;
            }
        } else {
            require((h.dut.dbg_video_vram_addr & 0x7fff) == 0,
                    "video address moved away from word 0 after CPU write");

            if (!word_ok && h.dut.dbg_video_vram_word == 0x00FF) {
                word_ok = true;
                word_tick = h.cycles;
                std::cout << "  word: same-address video word 0x00ff @t=" << word_tick << "\n";
            }

            // Consumed source byte at its load edge: pre-edge vram_d==FF
            // with cclk_p asserted must capture into the plus_vidword
            // low half on this posedge. Post-edge plus low proves the
            // actual load, not a held level.
            if (word_ok && !byte_ok && pre_cclk_p && pre_vram_d == 0xFF) {
                require(post_plus_lo == 0xFF,
                        "cclk_p pre-edge vram_d=FF did not load plus_vidword low half");
                byte_ok = true;
                byte_tick = h.cycles;
                std::cout << "  byte: pre cclk_p vram_d=0xff -> post plus_lo=0xff @t=" << byte_tick << "\n";
            }

            // Pixel-load boundary: pre-edge CLKEN&PIXEN with source low
            // half FF must load vid_even on this posedge and reset
            // pix_cnt to 0 (asic_video: PIXEN&&CLKEN -> vid_even<=VIDEOD,
            // pix_cnt<=0). A held vid_even with nonzero pix_cnt is not a
            // load. Must follow the byte load within two chars (2x64).
            if (byte_ok && !load_ok && pre_ce16 && pre_cclk_n &&
                pre_plus_lo == 0xFF) {
                require(post_vid_even == 0xFF,
                        "CLKEN&PIXEN pre-edge source FF did not load vid_even");
                require(post_pixcnt == 0,
                        "vid_even load did not reset pix_cnt to 0 (held level, not load)");
                require(post_modeq == 0,
                        "pixel latch ran with mode_q!=0; fixture pins mode-0 mapping");
                require(h.cycles - byte_tick <= 128,
                        "vid_even load did not follow consumed byte within 2 chars");
                load_ok = true;
                load_tick = h.cycles;
                std::cout << "  load: pre CLKEN&PIXEN src=0xff -> post vid_even=0xff pix=0 @t=" << load_tick
                          << " (pre_pix=" << (unsigned)pre_pixcnt << ")\n";
            }

            // Corresponding pixel: pre-edge PIXEN presentation of the
            // loaded even byte under latched mode 0. Mode-0 0xFF selects
            // pen 15 on both pixels ({b1,b5,b3,b7}/{b0,b4,b2,b6} all ones,
            // programmed white). Post-edge RGB on that PIXEN is the
            // presentation; require white within two dots with DE/no blank.
            if (load_ok && !white_ok && pre_ce16 && pre_vid_even == 0xFF &&
                pre_modeq == 0 && ((pre_pixcnt & 0x8U) == 0U)) {
                require(de && !hsync, "white outside active display");
                require(h.cycles - load_tick <= 8,
                        "white did not follow pixel load within 2 dots");
                require(is_white,
                        "PIXEN presentation of mode-0 vid_even=FF was not white");
                white_ok = true;
                white_tick = h.cycles;
                std::cout << "  pixel: pre PIXEN even=0xff mode0 -> post white RGB @t=" << white_tick
                          << " pix=" << (unsigned)post_pixcnt
                          << " latency=" << (white_tick - load_tick) << "\n";
            }

            if (word_ok && byte_ok && load_ok && white_ok) {
                std::cout << "PASS: accepted 0x20000=FF -> word 0x00ff @t=" << word_tick
                          << " -> byte @t=" << byte_tick << " -> load @t=" << load_tick
                          << " -> white @t=" << white_tick << "\n";
                return;
            }

            if (h.cycles - accepted_tick > 20000) {
                std::ostringstream msg;
                msg << "B8-4: stale video after accepted 0xff to phys 0x20000 @t="
                    << accepted_tick << " (word=0x" << std::hex
                    << h.dut.dbg_video_vram_word << " byte=0x"
                    << (unsigned)h.dut.dbg_video_vram_byte << " vid_even=0x"
                    << (unsigned)h.dut.dbg_video_vid_even << " rgb=0x" << rgb
                    << std::dec << " word/byte/load/white="
                    << word_ok << "/" << byte_ok << "/" << load_ok << "/" << white_ok << ")";
                throw TestFailure(msg.str());
            }
        }
    }

    if (!accepted) {
        throw TestFailure("timeout waiting for physical write to be accepted");
    }
    throw TestFailure("budget exhausted without a verdict");
}

struct CaptureOptions {
    bool show_help = false;
    bool capture_requested = false;
    bool have_cpr_path = false;
    bool have_frame_count = false;
    bool have_output_path = false;
    bool have_build_id = false;
    std::string cpr_path;
    std::string output_path;
    std::string build_id;
    uint64_t frame_count = 0;
};

void print_capture_usage(std::ostream &output) {
    output << "Usage: p10_boot_tests [--capture-cpr <path> --frames <n> "
              "--output <path> [--build-id <text>]]\n"
           << "       p10_boot_tests --help\n"
           << "Default mode runs the P10 regression suite. Capture mode uploads "
              "a CPR and writes 64-MHz frame samples.\n";
}

uint64_t parse_capture_frame_count(const std::string &text) {
    if (text.empty())
        throw TestFailure("--frames requires a positive decimal integer");
    uint64_t value = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() ||
        value == 0)
        throw TestFailure("--frames requires a positive decimal integer");
    if (value > kCaptureMaxFrames)
        throw TestFailure("--frames exceeds bounded harness limit of " +
                          std::to_string(kCaptureMaxFrames));
    return value;
}

CaptureOptions parse_capture_options(int argc, char **argv) {
    CaptureOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i] ? argv[i] : "";
        if (argument == "--help") {
            if (argc != 2)
                throw TestFailure("--help cannot be combined with other arguments");
            options.show_help = true;
            continue;
        }

        if (argument == "--capture-cpr") {
            if (options.have_cpr_path)
                throw TestFailure("--capture-cpr specified more than once");
            if (++i >= argc || !argv[i] || argv[i][0] == '\0' ||
                argv[i][0] == '-')
                throw TestFailure("--capture-cpr requires a file path");
            options.have_cpr_path = true;
            options.capture_requested = true;
            options.cpr_path = argv[i];
        }
        else if (argument == "--frames") {
            if (options.have_frame_count)
                throw TestFailure("--frames specified more than once");
            if (++i >= argc || !argv[i] || argv[i][0] == '\0' ||
                argv[i][0] == '-')
                throw TestFailure("--frames requires a positive decimal integer");
            options.have_frame_count = true;
            options.capture_requested = true;
            options.frame_count = parse_capture_frame_count(argv[i]);
        }
        else if (argument == "--output") {
            if (options.have_output_path)
                throw TestFailure("--output specified more than once");
            if (++i >= argc || !argv[i] || argv[i][0] == '\0' ||
                argv[i][0] == '-')
                throw TestFailure("--output requires a file path");
            options.have_output_path = true;
            options.capture_requested = true;
            options.output_path = argv[i];
        }
        else if (argument == "--build-id") {
            if (options.have_build_id)
                throw TestFailure("--build-id specified more than once");
            if (++i >= argc || !argv[i] || argv[i][0] == '\0' ||
                argv[i][0] == '-')
                throw TestFailure("--build-id requires non-empty text");
            options.have_build_id = true;
            options.capture_requested = true;
            options.build_id = argv[i];
        }
        else {
            if (!argument.empty() && argument[0] == '+') continue;
            throw TestFailure("unknown argument '" + argument + "'");
        }
    }

    if (options.show_help) return options;
    if (options.capture_requested &&
        (!options.have_cpr_path || !options.have_frame_count ||
         !options.have_output_path))
        throw TestFailure("capture mode requires --capture-cpr, --frames, and --output");
    return options;
}

std::vector<uint8_t> read_cpr_file(const std::string &path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input)
        throw TestFailure("cannot open CPR file " + path);
    const std::streamoff size = input.tellg();
    if (size <= 0)
        throw TestFailure("CPR file is empty: " + path);
    if (static_cast<uint64_t>(size) > kCaptureMaxCprBytes)
        throw TestFailure("CPR file exceeds bounded size limit of " +
                          std::to_string(kCaptureMaxCprBytes) + " bytes");
    input.seekg(0, std::ios::beg);
    std::vector<uint8_t> image(static_cast<size_t>(size));
    input.read(reinterpret_cast<char *>(image.data()),
               static_cast<std::streamsize>(image.size()));
    if (input.gcount() != static_cast<std::streamsize>(image.size()))
        throw TestFailure("could not read complete CPR file " + path);
    return image;
}

uint64_t hash_bytes(const std::vector<uint8_t> &bytes) {
    Fnv1a64 hasher;
    if (!bytes.empty()) hasher.update(bytes.data(), bytes.size());
    return hasher.value();
}

std::string simulator_binary_identity(const std::string &argv0) {
    if (argv0.empty()) return {};
    std::error_code error;
    std::filesystem::path path(argv0);
    if (path.is_relative()) {
        path = std::filesystem::absolute(path, error);
        if (error) return {};
    }
    const std::filesystem::path canonical =
        std::filesystem::weakly_canonical(path, error);
    if (!error) path = canonical;

    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) return {};
    const std::streamoff size = input.tellg();
    if (size < 0) return {};
    input.seekg(0, std::ios::beg);
    Fnv1a64 hasher;
    std::array<char, 64 * 1024> buffer{};
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = input.gcount();
        if (count > 0) hasher.update(buffer.data(), static_cast<size_t>(count));
    }
    if (!input.eof()) return {};
    return path.string() + " size=" + std::to_string(size) +
           " fnv1a64=" + format_hash(hasher.value());
}

bool paths_identical(const std::string &first, const std::string &second) {
    std::error_code first_error;
    std::error_code second_error;
    const std::filesystem::path first_canonical =
        std::filesystem::weakly_canonical(first, first_error);
    const std::filesystem::path second_canonical =
        std::filesystem::weakly_canonical(second, second_error);
    if (!first_error && !second_error && first_canonical == second_canonical)
        return true;

    first_error.clear();
    second_error.clear();
    if (std::filesystem::exists(first, first_error) &&
        std::filesystem::exists(second, second_error) &&
        !first_error && !second_error) {
        std::error_code equivalent_error;
        const bool equivalent =
            std::filesystem::equivalent(first, second, equivalent_error);
        if (!equivalent_error && equivalent) return true;
    }
    return false;
}

void reject_existing_capture_output(const std::string &cpr_path,
                                    const std::string &output_path) {
    if (paths_identical(cpr_path, output_path))
        throw TestFailure("capture output path must differ from CPR input path");
    std::error_code error;
    const bool exists = std::filesystem::exists(output_path, error);
    if (error)
        throw TestFailure("cannot inspect capture output path " + output_path +
                          ": " + error.message());
    if (exists)
        throw TestFailure("capture output already exists; choose a fresh path");
}

std::string resolve_simulator_identity(const CaptureOptions &options,
                                       const char *argv0) {
    if (options.have_build_id)
        return "user_build_id=" + sanitize_header_text(options.build_id);
    const std::string identity = simulator_binary_identity(argv0 ? argv0 : "");
    if (identity.empty())
        throw TestFailure("simulator-binary identity unavailable; pass --build-id <text>");
    return identity;
}

void run_capture_cli(const CaptureOptions &options,
                     const std::string &binary_identity) {
    const std::vector<uint8_t> image = read_cpr_file(options.cpr_path);
    reject_existing_capture_output(options.cpr_path, options.output_path);

    Harness h;
    h.initialize();
    h.download(image);
    wait_for_cpr_apply(h);

    ExclusiveFileBuf output_buffer;
    if (!output_buffer.open(options.output_path)) {
        // A symlink at the output path is refused without being followed.
        // Depending on the platform the exclusive open reports ELOOP
        // (follow refused) or EEXIST (the link entry itself collides with
        // O_CREAT|O_EXCL); symlink_status distinguishes it either way.
        std::error_code status_error;
        const bool is_link = std::filesystem::is_symlink(
            std::filesystem::symlink_status(options.output_path, status_error));
        if (!status_error && is_link)
            throw TestFailure("capture output path is a symlink; refusing to follow it");
        if (output_buffer.error_number() == EEXIST)
            throw TestFailure("capture output appeared during simulation; refusing to overwrite");
        throw TestFailure("cannot create capture output " + options.output_path +
                          ": " + output_buffer.error());
    }
    std::ostream output(&output_buffer);
    CaptureProvenance provenance;
    provenance.cpr_size_bytes = image.size();
    provenance.cpr_content_hash = hash_bytes(image);
    provenance.plus_model = static_cast<uint8_t>(h.dut.plus_model_i);
    provenance.production_clocking = h.dut.production_clocking != 0;
    provenance.simulator_binary_identity = binary_identity;
    const CaptureResult capture = capture_video_frames(
        h, options.frame_count, output, provenance, kCaptureWarmupFrames);
    require(capture.frames.size() == options.frame_count,
            "capture result count differs from requested frame count");
    if (!output_buffer.close())
        throw TestFailure("failed to close capture output " + options.output_path +
                          ": " + output_buffer.error());
    std::cout << "CAPTURE complete: frames=" << capture.frames.size()
              << " filter_selector="
              << static_cast<unsigned>(capture.filter_selector)
              << " serialized_bytes=" << capture.serialized_bytes
              << " output=" << options.output_path << std::endl;
}

// One focused case per CLI boundary class (parse shape, non-clobbering
// output).  These pin the fail-closed contract without enumerating every
// micro-variant; simulation is not involved.
void test_b3_capture_cli_validation() {
    std::cout << "Running test_b3_capture_cli_validation..." << std::endl;
    auto parse = [](std::vector<const char *> args) {
        std::vector<char *> mutable_args;
        for (const char *arg : args)
            mutable_args.push_back(const_cast<char *>(arg));
        return parse_capture_options(static_cast<int>(mutable_args.size()),
                                     mutable_args.data());
    };
    auto expect_reject = [&](std::vector<const char *> args, const char *why) {
        bool rejected = false;
        try {
            parse(args);
        } catch (const TestFailure &) {
            rejected = true;
        }
        require(rejected, std::string("CLI accepted invalid input: ") + why);
    };
    expect_reject({"p10_boot_tests", "--capture-cpr", "a.cpr", "--frames", "0",
                   "--output", "o.txt"}, "zero frame count");
    expect_reject({"p10_boot_tests", "--capture-cpr", "a.cpr", "--frames", "17",
                   "--output", "o.txt"}, "over-limit frame count");
    expect_reject({"p10_boot_tests", "--capture-cpr", "a.cpr", "--frames", "two",
                   "--output", "o.txt"}, "non-numeric frame count");
    expect_reject({"p10_boot_tests", "--capture-cpr", "a.cpr", "--frames", "2"},
                  "missing --output");
    expect_reject({"p10_boot_tests", "--frames", "2", "--frames", "2",
                   "--capture-cpr", "a.cpr", "--output", "o.txt"},
                  "duplicate --frames");
    expect_reject({"p10_boot_tests", "--help", "--frames", "2"},
                  "--help combined with other arguments");
    expect_reject({"p10_boot_tests", "--frobnicate"}, "unknown argument");

    const CaptureOptions valid = parse({"p10_boot_tests", "--capture-cpr", "a.cpr",
                                        "--frames", "2", "--output", "o.txt"});
    require(valid.capture_requested && valid.frame_count == 2 &&
                valid.cpr_path == "a.cpr" && valid.output_path == "o.txt",
            "CLI rejected a well-formed capture invocation");

    // Non-clobbering output: same input/output path and pre-existing output
    // must both fail before any simulation starts.
    {
        bool rejected_same = false;
        try {
            reject_existing_capture_output("same.cpr", "same.cpr");
        } catch (const TestFailure &) {
            rejected_same = true;
        }
        require(rejected_same,
                "CLI accepted an output path identical to its CPR input");
    }
    // Non-clobbering output probes live in the test working directory and
    // are removed afterwards: the harness must not depend on an OS temp
    // directory.  A stale probe from a crashed run is cleared first so a
    // leftover cannot wedge the suite.
    const std::string existing = "b3_cli_exists_probe.tmp";
    const std::string link_to_existing = "b3_cli_link_probe.tmp";
    const std::string dangling_link = "b3_cli_dangling_probe.tmp";
    {
        std::error_code ignored;
        std::filesystem::remove(existing, ignored);
        std::filesystem::remove(link_to_existing, ignored);
        std::filesystem::remove(dangling_link, ignored);
    }
    {
        std::ofstream probe(existing, std::ios::binary);
        require(static_cast<bool>(probe), "could not create CLI probe file");
        probe << "occupied";
    }
    bool rejected_existing = false;
    try {
        reject_existing_capture_output("some.cpr", existing);
    } catch (const TestFailure &) {
        rejected_existing = true;
    }
    // A symlink to an existing file is an existing output: the pre-check
    // follows it to the occupied target and refuses before any simulation.
    bool rejected_link = false;
    {
        std::error_code link_error;
        std::filesystem::create_symlink("b3_cli_exists_probe.tmp",
                                        link_to_existing, link_error);
        require(!link_error,
                "could not create CLI symlink probe: " + link_error.message());
        try {
            reject_existing_capture_output("some.cpr", link_to_existing);
        } catch (const TestFailure &) {
            rejected_link = true;
        }
    }
    // A dangling symlink is invisible to the pre-check (no target exists),
    // so the exclusive open must refuse it rather than follow it: prove the
    // boundary on the primitive directly, without simulation. Either errno
    // is a refusal (ELOOP where O_NOFOLLOW reports the refused follow,
    // EEXIST where the link entry itself collides with O_CREAT|O_EXCL, as
    // on macOS); what matters is that nothing is followed or created.
    bool refused_dangling = false;
    {
        std::error_code link_error;
        std::filesystem::create_symlink("b3_cli_no_such_target.tmp",
                                        dangling_link, link_error);
        require(!link_error,
                "could not create CLI dangling probe: " + link_error.message());
        ExclusiveFileBuf dangling;
        refused_dangling = !dangling.open(dangling_link);
        require(refused_dangling &&
                    (dangling.error_number() == ELOOP ||
                     dangling.error_number() == EEXIST),
                "exclusive open followed a dangling symlink instead of refusing it");
    }
    {
        std::error_code remove_error;
        std::filesystem::remove(existing, remove_error);
        std::filesystem::remove(link_to_existing, remove_error);
        std::filesystem::remove(dangling_link, remove_error);
    }
    require(rejected_existing, "CLI accepted a pre-existing output path");
    require(rejected_link, "CLI accepted a symlink to a pre-existing output path");
    require(refused_dangling, "CLI exclusive open followed a dangling symlink");
    std::cout << "PASS: capture CLI fails closed on invalid input" << std::endl;
}

} // namespace

int main(int argc, char **argv) {
    try {
		const CaptureOptions options = parse_capture_options(argc, argv);
		if (options.show_help) {
			print_capture_usage(std::cout);
			return 0;
		}
		Verilated::commandArgs(argc, argv);
		const std::string binary_identity =
			resolve_simulator_identity(options, argc > 0 ? argv[0] : nullptr);
		if (options.capture_requested) {
			run_capture_cli(options, binary_identity);
			return 0;
		}
		test_p10a_vram_client_wiring();
		test_p10b_video_coherence_pixel();
		test_b3_frame_capture_smoke(binary_identity);
		test_b3_malformed_cpr_rejected();
		test_b3_truncated_cpr_rejected();
		test_b3_repeat_capture_stable(binary_identity);
		test_b3_capture_cli_validation();
        test_p10a_deterministic_boot();
		test_real_u765_edsk_read();
		std::cout << "\nAll P10 Production CPR Boot Harness tests PASSED.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "\nFAILED: " << e.what() << "\n";
		if (argc > 1) print_capture_usage(std::cerr);
        return 1;
    }
}
