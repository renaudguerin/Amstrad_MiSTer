// B7 dark-silicon path-ownership audit.
//
// Each invocation runs the same production-shaped p10_boot_test_top fixture.
// The Verilog top selects one simulation-only mutation from
// +mutate_module=<name>; this program only compares the resulting sampled
// signature with the mode-specific unmodified baseline.

#include "Vp10_boot_test_top.h"
#include "verilated.h"

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr uint8_t kCmdActive = 0b011;
constexpr uint8_t kCmdRead   = 0b101;
constexpr uint8_t kCmdWrite  = 0b100;
constexpr unsigned kIoctlWaitLimit = 20000000;
constexpr uint16_t kLoopPc = 0x0800;
constexpr unsigned kLoopLinks = 64;
constexpr uint64_t kWarmupTicks = 65536;
constexpr uint64_t kSampleTicks = 131072;

class AuditFailure : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

struct Chunk {
	std::string id;
	std::vector<uint8_t> data;
};

std::vector<uint8_t> build_cpr_image(const std::vector<Chunk>& chunks) {
	std::vector<uint8_t> image{'R', 'I', 'F', 'F'};
	uint32_t riff_len = 4;
	for (const auto& chunk : chunks) {
		riff_len += 8 + static_cast<uint32_t>(chunk.data.size());
		if (chunk.data.size() & 1U)
			++riff_len;
	}
	for (unsigned shift = 0; shift < 4; ++shift)
		image.push_back(static_cast<uint8_t>((riff_len >> (8 * shift)) & 0xff));
	image.insert(image.end(), {'A', 'M', 'S', '!'});
	for (const auto& chunk : chunks) {
		image.insert(image.end(), chunk.id.begin(), chunk.id.end());
		const uint32_t length = static_cast<uint32_t>(chunk.data.size());
		for (unsigned shift = 0; shift < 4; ++shift)
			image.push_back(static_cast<uint8_t>((length >> (8 * shift)) & 0xff));
		image.insert(image.end(), chunk.data.begin(), chunk.data.end());
		if (chunk.data.size() & 1U)
			image.push_back(0);
	}
	return image;
}

struct Program {
	std::vector<uint8_t> code;
	uint16_t handoff_pc = 0;
};

Program build_program() {
	Program program{std::vector<uint8_t>(16384, 0x00), 0};
	size_t pc = 0x0100;
	auto emit = [&](uint8_t value) {
		if (pc >= program.code.size())
			throw AuditFailure("B7 audit program overflow");
		program.code[pc++] = value;
	};
	auto ld_a = [&](uint8_t value) {
		emit(0x3e); emit(value);
	};
	auto ld_bc = [&](uint16_t address) {
		emit(0x01);
		emit(static_cast<uint8_t>(address & 0xff));
		emit(static_cast<uint8_t>(address >> 8));
	};
	auto out_c = [&]() { emit(0xed); emit(0x79); };
	auto io_write = [&](uint16_t address, uint8_t value) {
		ld_bc(address); ld_a(value); out_c();
	};
	auto mem_write = [&](uint16_t address, uint8_t value) {
		ld_a(value);
		emit(0x32);
		emit(static_cast<uint8_t>(address & 0xff));
		emit(static_cast<uint8_t>(address >> 8));
	};
	auto mem_read = [&](uint16_t address) {
		emit(0x3a);
		emit(static_cast<uint8_t>(address & 0xff));
		emit(static_cast<uint8_t>(address >> 8));
	};

	// Cartridge-visible reset vector and a deterministic setup sequence.
	program.code[0] = 0xc3;
	program.code[1] = 0x00;
	program.code[2] = 0x01;
	ld_bc(0xdf00); ld_a(0x07); out_c(); // high cartridge page 3
	mem_read(0xc000);                    // cartridge data path

	// Plus ASIC unlock: sync FF,00 followed by FF 77 B3 51 A8 D4 62 39
	// 9C 46 2B 15 8A CD.  The detector consumes the first 14 sequence bytes;
	// the final state byte is CD, matching the production boot fixture.
	ld_bc(0xbc00);
	const uint8_t unlock[] = {
		0xff, 0x00, 0xff, 0x77, 0xb3, 0x51, 0xa8, 0xd4,
		0x62, 0x39, 0x9c, 0x46, 0x2b, 0x15, 0x8a, 0xcd
	};
	for (uint8_t value : unlock) {
		ld_a(value);
		out_c();
	}
	io_write(0x7f00, 0xb8); // unlocked RMR2: ASIC page at &4000

	// Program a standard type-3 video cadence through the motherboard's
	// shared CRTC bus.  The Plus asic_video instance sees these same writes.
	const uint8_t crtc[] = {
		0x3f, 0x28, 0x32, 0x22, 0x06, 0x00, 0x28, 0x05, 0x00, 0x07
	};
	for (uint8_t reg = 0; reg <= 9; ++reg) {
		io_write(0xbc00, reg);
		io_write(0xbd00, crtc[reg]);
	}

	// Legacy GA shadow and native ASIC palette/video state.
	io_write(0x7f00, 0x83);
	io_write(0x7f00, 0x82);
	io_write(0x7f00, 0x05);
	io_write(0x7f00, 0x55);
	io_write(0x7f00, 0x10);
	io_write(0x7f00, 0x44);
	mem_write(0x6400, 0x0f);
	mem_write(0x6401, 0x03);
	mem_write(0x6420, 0x21);

	// Sprite pixel RAM and an explicit host read.  The latter makes the
	// plus_sprite_ram mutation observable even when the display walker has
	// not yet requested the corresponding row.
	mem_write(0x4000, 0xa5);
	mem_read(0x4000);
	mem_write(0x4001, 0x5c);
	mem_write(0x4100, 0x3e);
	mem_write(0x6000, 0x66);
	mem_write(0x6001, 0x01);
	mem_write(0x6002, 0x10);
	mem_write(0x6003, 0x00);
	mem_write(0x6004, 0x05); // x1/y1 sprite 0
	mem_write(0x642a, 0x34);
	mem_write(0x642b, 0x06);
	mem_write(0x6434, 0x12);
	mem_write(0x6435, 0x0f);

	// Screen split/scroll and DMA register paths.
	mem_write(0x6801, 0x05);
	mem_write(0x6802, 0x24);
	mem_write(0x6803, 0x00);
	mem_write(0x6804, 0x34);
	mem_write(0x6c00, 0x34);
	mem_write(0x6c01, 0x12);
	mem_write(0x6c02, 0x05);
	mem_write(0x6c0f, 0x01);
	emit(0xf3); // DI before entering the measured loop
	// The TV80 substitute takes an immediate JP high byte from WZ on the
	// same edge. Prime WZ with the target exactly as p10_boot_test.cpp does.
	mem_read(kLoopPc);
	emit(0xc3); // JP measured loop
	emit(static_cast<uint8_t>(kLoopPc & 0xff));
	emit(static_cast<uint8_t>(kLoopPc >> 8));

	// Leave the real CPU executing ASIC-page reads while the signature is
	// sampled. The loop-back receives the same WZ priming read.
	const uint16_t handoff = static_cast<uint16_t>(kLoopPc + kLoopLinks * 3U);
	program.handoff_pc = handoff;
	for (unsigned link = 0; link < kLoopLinks; ++link) {
		const uint16_t at = static_cast<uint16_t>(kLoopPc + link * 3U);
		program.code[at] = 0x3a; // LD A,(&4000)
		program.code[at + 1] = 0x00;
		program.code[at + 2] = 0x40;
	}
	program.code[handoff] = 0x3a; // LD A,(&0800), prime WZ
	program.code[handoff + 1] = static_cast<uint8_t>(kLoopPc & 0xff);
	program.code[handoff + 2] = static_cast<uint8_t>(kLoopPc >> 8);
	program.code[handoff + 3] = 0xc3; // JP &0800
	program.code[handoff + 4] = static_cast<uint8_t>(kLoopPc & 0xff);
	program.code[handoff + 5] = static_cast<uint8_t>(kLoopPc >> 8);
	return program;
}

class Signature64 {
public:
	void put(uint64_t value, unsigned bytes) {
		for (unsigned i = 0; i < bytes; ++i) {
			state_ ^= static_cast<uint8_t>(value >> (i * 8));
			state_ *= 1099511628211ULL;
		}
	}

	uint64_t value() const { return state_; }

private:
	uint64_t state_ = 14695981039346656037ULL;
};

class Harness {
public:
	Vp10_boot_test_top dut;
	std::unordered_map<uint32_t, uint8_t> memory;
	uint64_t cycles = 0;

	Harness(bool plus_mode, unsigned crtc_type = 0) {
		dut.clk = 0;
		dut.clkref = 0;
		dut.init = 1;
		dut.reset_btn = plus_mode ? 0 : 1;
		dut.plus_model_i = plus_mode ? 2 : 0;
		dut.cpr_download = 0;
		dut.ioctl_wr = 0;
		dut.ioctl_addr = 0;
		dut.ioctl_dout = 0;
		dut.memory_dq = 0;
		dut.memory_dq_oe = 0;
		dut.force_irq = 0;
		dut.b7_crtc_type = crtc_type;
		dut.eval();
	}

	~Harness() { dut.final(); }

	void verify_mutation(unsigned expected_id) const {
		const bool expected_enable = expected_id != 0;
		if (dut.b7_mutation_id != expected_id ||
		    static_cast<bool>(dut.b7_mutation_enable) != expected_enable) {
			throw AuditFailure("B7 plusarg mutation decoder did not select the expected module");
		}
	}

	void store(uint8_t bank, uint32_t address, uint8_t byte) {
		memory[(static_cast<uint32_t>(bank) << 23) |
		       (address & 0x7fffffU)] = byte;
	}

	uint8_t load(uint8_t bank, uint32_t address) const {
		const uint32_t key = (static_cast<uint32_t>(bank) << 23) |
		                     (address & 0x7fffffU);
		auto it = memory.find(key);
		return it == memory.end() ? 0xff : it->second;
	}

	void seed_classic_ram(const Program& program) {
		for (size_t i = 0; i < program.code.size(); ++i) {
			// The classic 64K map starts at physical SDRAM 0x20000 when no
			// ROM is selected; also seed physical zero for reset-ROM decode.
			store(0, static_cast<uint32_t>(i), program.code[i]);
			store(0, 0x20000U + static_cast<uint32_t>(i), program.code[i]);
		}
	}

	void tick(Signature64* signature = nullptr) {
		dut.clkref = ((cycles & 7U) == 0U);
		dut.memory_dq_oe = read_drive_cycles > 0;
		dut.memory_dq = read_word;

		dut.clk = 0;
		dut.eval();
		dut.clk = 1;
		dut.eval();

		const uint8_t command = (dut.sdram_nras ? 0b100 : 0) |
		                        (dut.sdram_ncas ? 0b010 : 0) |
		                        (dut.sdram_nwe  ? 0b001 : 0);
		bool started_read = false;
		if (command == kCmdActive) {
			active_row = dut.sdram_a & 0x1fffU;
			active_bank = dut.sdram_ba;
		} else if (command == kCmdRead) {
			const uint32_t address = command_address();
			read_word = static_cast<uint16_t>(load(active_bank, address)) |
			            (static_cast<uint16_t>(load(active_bank, address + 1)) << 8);
			read_drive_cycles = 4;
			started_read = true;
		} else if (command == kCmdWrite) {
			const uint32_t address = command_address();
			const uint16_t data = dut.observed_dq;
			if (!dut.sdram_dqml)
				store(active_bank, address, data & 0xffU);
			if (!dut.sdram_dqmh)
				store(active_bank, address + 1, data >> 8);
		}
		if (read_drive_cycles > 0 && !started_read)
			--read_drive_cycles;

		dut.clk = 0;
		dut.eval();
		if (signature)
			sample(*signature);
		++cycles;
	}

	void run(uint64_t count, Signature64* signature = nullptr) {
		for (uint64_t i = 0; i < count; ++i)
			tick(signature);
	}

	void initialize(const Program& classic_program, bool plus) {
		if (!plus)
			seed_classic_ram(classic_program);
		for (unsigned i = 0; i < 16; ++i)
			tick();
		dut.init = 0;
		for (unsigned i = 0; i < 500; ++i)
			tick();
		if (!plus) {
			dut.reset_btn = 0;
			run(64);
		}
	}

	void download(const std::vector<uint8_t>& image) {
		dut.cpr_download = 1;
		tick();
		for (size_t i = 0; i < image.size(); ++i) {
			dut.ioctl_addr = static_cast<uint32_t>(i);
			dut.ioctl_dout = image[i];
			dut.ioctl_wr = 1;
			tick();
			dut.ioctl_wr = 0;
			unsigned waited = 0;
			while (dut.ioctl_wait) {
				tick();
				if (++waited > kIoctlWaitLimit)
					throw AuditFailure("B7 CPR download ioctl_wait timeout");
			}
		}
		dut.cpr_download = 0;
		tick();
	}

	void wait_for_reset_release() {
		for (uint64_t i = 0; i < 5000000; ++i) {
			if (!dut.dbg_reset)
				return;
			tick();
		}
		throw AuditFailure("B7 fixture reset did not release");
	}

private:
	uint16_t active_row = 0;
	uint8_t active_bank = 0;
	uint16_t read_word = 0;
	unsigned read_drive_cycles = 0;

	uint32_t command_address() const {
		return (static_cast<uint32_t>(active_row) << 9) |
		       ((static_cast<uint32_t>(dut.sdram_a) & 0x100U) << 14) |
		       ((static_cast<uint32_t>(dut.sdram_a) & 0xffU) << 1);
	}

	void sample(Signature64& signature) const {
		if (dut.dbg_reset)
			throw AuditFailure("whole-design reset entered the B7 sample window");

		// Hash only post-selection motherboard outputs and the externally
		// visible CPU bus. Raw module outputs and mutation controls are
		// deliberately excluded: including either would make a mutation look
		// live even when the production ownership mux discarded it.
		signature.put(dut.b7_rgb, 2);
		signature.put(dut.b7_hsync, 1);
		signature.put(dut.b7_vsync, 1);
		signature.put(dut.b7_de, 1);
		signature.put(dut.b7_ma, 2);
		signature.put(dut.b7_ra, 1);
		signature.put(dut.b7_mode, 1);
		signature.put(dut.b7_audio_l, 1);
		signature.put(dut.b7_audio_r, 1);
		signature.put(dut.b7_cpu_irq_n, 1);

		signature.put(dut.dbg_pc, 2);
		signature.put(dut.dbg_addr, 2);
		signature.put(dut.dbg_dout, 1);
		signature.put(dut.dbg_din, 1);
		signature.put(dut.dbg_m1_n, 1);
		signature.put(dut.dbg_mreq_n, 1);
		signature.put(dut.dbg_iorq_n, 1);
		signature.put(dut.dbg_rd_n, 1);
		signature.put(dut.dbg_wr_n, 1);
		signature.put(dut.dbg_wait_n, 1);
		signature.put(dut.dbg_cpu_waitn, 1);
		signature.put(dut.dbg_int_n, 1);
		signature.put(dut.dbg_int_ack, 1);
		signature.put(dut.dbg_vec_byte, 1);
		signature.put(dut.dbg_vec_valid, 1);
		signature.put(dut.dbg_mcycle, 1);
		signature.put(dut.dbg_tstate, 1);
		signature.put(dut.dbg_ir, 1);
		signature.put(dut.dbg_mcmax, 1);
		signature.put(dut.dbg_cen_p, 1);
	}
};

struct Options {
	bool plus = true;
	bool signature_only = false;
	bool xfail = false;
	unsigned crtc_type = 0;
	std::string expected;
	std::string baseline;
	std::string group = "baseline";
	std::string mutation = "none";
};

Options parse_options(int argc, char** argv) {
	Options options;
	for (int i = 1; i < argc; ++i) {
		const std::string arg(argv[i]);
		if (arg == "--mode" && i + 1 < argc) {
			options.plus = std::string(argv[++i]) == "plus";
		} else if (arg == "--signature-only") {
			options.signature_only = true;
		} else if (arg == "--xfail") {
			options.xfail = true;
		} else if (arg == "--crtc-type" && i + 1 < argc) {
			options.crtc_type = static_cast<unsigned>(std::stoul(argv[++i]));
		} else if (arg == "--expected" && i + 1 < argc) {
			options.expected = argv[++i];
		} else if (arg == "--baseline" && i + 1 < argc) {
			options.baseline = argv[++i];
		} else if (arg == "--group" && i + 1 < argc) {
			options.group = argv[++i];
		} else if (arg.rfind("+mutate_module=", 0) == 0) {
			options.mutation = arg.substr(std::string("+mutate_module=").size());
		}
	}
	return options;
}

unsigned mutation_id(const std::string& mutation) {
	if (mutation == "none")                    return 0;
	if (mutation == "asic_video")              return 1;
	if (mutation == "asic_sprites")            return 2;
	if (mutation == "asic_dma")                return 3;
	if (mutation == "asic_regs")               return 4;
	if (mutation == "asic_ga_timing")          return 5;
	if (mutation == "asic_unlock")             return 6;
	if (mutation == "plus_mmu")                return 7;
	if (mutation == "plus_sprite_ram")         return 8;
	if (mutation == "plus_cartridge_memory")   return 9;
	if (mutation == "CRTC")                    return 10;
	if (mutation == "crtc_type0_engine")       return 11;
	if (mutation == "crtc_type1_engine")       return 12;
	if (mutation == "ga40010")                 return 13;
	if (mutation == "negative_control")        return 14;
	throw AuditFailure("unknown B7 mutation name: " + mutation);
}

uint64_t parse_hash(const std::string& text) {
	if (text.empty())
		throw AuditFailure("missing B7 baseline signature");
	return std::stoull(text, nullptr, 0);
}

uint64_t run_fixture(const Options& options) {
	const Program program = build_program();
	Harness harness(options.plus, options.crtc_type);
	harness.verify_mutation(mutation_id(options.mutation));
	harness.initialize(program, options.plus);
	if (options.plus) {
		std::vector<uint8_t> page3(16384, 0);
		page3[0] = 0x42;
		harness.download(build_cpr_image({{"cb00", program.code}, {"cb03", page3}}));
		harness.wait_for_reset_release();
	}
	Signature64 signature;
	harness.run(kWarmupTicks, &signature);
	// The measured loop has executed DI, so this deterministic interrupt pulse
	// exercises the CPU-visible input without allowing the CPU to leave it.
	harness.dut.force_irq = 1;
	for (uint64_t i = 0; i < kSampleTicks; ++i) {
		if (i == 256)
			harness.dut.force_irq = 0;
		harness.tick(&signature);
	}
	return signature.value();
}

int evaluate(const Options& options, uint64_t signature) {
	std::cout << "SIGNATURE 0x" << std::hex << std::setfill('0') << std::setw(16)
	          << signature << std::dec << '\n';
	if (options.signature_only)
		return 0;

	const uint64_t baseline = parse_hash(options.baseline);
	const bool changed = signature != baseline;
	const bool expected_changed = options.expected == "changed";
	const bool assertion_ok = expected_changed ? changed : !changed;
	if (assertion_ok && !options.xfail) {
		std::cout << "B7 PASS group=" << options.group
		          << " mode=" << (options.plus ? "plus" : "classic")
		          << " module=" << options.mutation
		          << " baseline=0x" << std::hex << baseline
		          << " mutated=0x" << signature << std::dec
		          << " changed=" << (changed ? "yes" : "no")
		          << " expected=" << (expected_changed ? "changed" : "unchanged")
		          << '\n';
		return 0;
	}
	if (assertion_ok && options.xfail) {
		std::cout << "B7 XPASS group=" << options.group
		          << " mode=" << (options.plus ? "plus" : "classic")
		          << " module=" << options.mutation
		          << " baseline=0x" << std::hex << baseline
		          << " mutated=0x" << signature << std::dec
		          << " changed=" << (changed ? "yes" : "no")
		          << " expected=" << (expected_changed ? "changed" : "unchanged")
		          << " finding=no longer reproduces; update the B7 findings table\n";
		return 1;
	}

	std::cout << (options.xfail ? "B7 XFAIL" : "B7 FAIL")
	          << " group=" << options.group
	          << " mode=" << (options.plus ? "plus" : "classic")
	          << " module=" << options.mutation
	          << " baseline=0x" << std::hex << baseline
	          << " mutated=0x" << signature << std::dec
	          << " changed=" << (changed ? "yes" : "no")
	          << " expected=" << (expected_changed ? "changed" : "unchanged")
	          << " finding=";
	if (options.group == "plus-active")
		std::cout << "Plus mutation did not change the Plus signature; the module may be dead or muxed away";
	else if (options.group == "classic-in-plus")
		std::cout << "classic mutation changed the Plus signature; classic logic leaks into Plus mode";
	else if (options.group == "plus-in-classic")
		std::cout << "Plus mutation changed the classic signature; Plus logic is not isolated";
	else if (options.group == "classic-active-control")
		std::cout << "classic mutation did not change its active classic signature; the isolation control is invalid";
	else
		std::cout << "negative control changed the signature; the harness is nondiscriminating";
	std::cout << '\n';
	return options.xfail ? 0 : 1;
}

} // namespace

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	try {
		const Options options = parse_options(argc, argv);
		if (options.crtc_type > 1)
			throw AuditFailure("--crtc-type must be 0 or 1");
		const uint64_t signature = run_fixture(options);
		return evaluate(options, signature);
	} catch (const std::exception& error) {
		std::cerr << "B7 FAIL: " << error.what() << '\n';
		return 1;
	}
}
