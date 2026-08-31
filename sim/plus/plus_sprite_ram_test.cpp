// P10j leaf fixture for rtl/plus/plus_sprite_ram.v.
//
// This bench pins the storage contract that lets Quartus infer two 2Kx4 M10Ks
// without changing the CPU-visible ASIC-page transaction. Expectations are
// derived from the P10j resource/timing contract and the existing defined-zero
// FPGA model assumption in rtl/plus/asic_regs.v.

#include "Vplus_sprite_ram.h"
#include "verilated.h"

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {

class TestFailure : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

[[noreturn]] void fail(const std::string& what) { throw TestFailure(what); }

class Bench {
public:
	Vplus_sprite_ram dut;
	uint64_t cycle = 0;

	Bench() : dut("plus_sprite_ram") {
		dut.clk = 0;
		dut.reset = 0;
		dut.host_rd = 0;
		dut.host_wr = 0;
		dut.host_addr = 0;
		dut.host_wdata = 0;
		dut.video_rd = 0;
		dut.video_addr = 0;
		dut.eval();
	}

	~Bench() { dut.final(); }
	Bench(const Bench&) = delete;
	Bench& operator=(const Bench&) = delete;

	// One call is one complete 64 MHz rising edge. There is no WAIT pin in this
	// contract; reset clears only the observable read pipelines,
	// while defined-zero initial contents retain the existing FPGA assumption.
	void tick() {
		dut.clk = 0;
		dut.eval();
		dut.clk = 1;
		dut.eval();
		++cycle;
	}

	void idle() {
		dut.host_rd = 0;
		dut.host_wr = 0;
		dut.video_rd = 0;
		dut.eval();
	}

	void host_write(unsigned address, unsigned nibble) {
		dut.host_rd = 0;
		dut.host_wr = 1;
		dut.host_addr = address & 0xFFF;
		dut.host_wdata = nibble & 0xF;
		tick();
		idle();
	}

	// host_rdata is sampled after exactly the edge that sees host_rd.  The
	// following idle assignment is only bus release; it is not another CPU
	// transaction or a second read edge.
	unsigned host_read(unsigned address) {
		dut.host_rd = 1;
		dut.host_wr = 0;
		dut.host_addr = address & 0xFFF;
		tick();
		const unsigned value = static_cast<unsigned>(dut.host_rdata) & 0xF;
		idle();
		return value;
	}

	// video_rdata is likewise sampled after one rising edge, and is the
	// packed byte {odd bank nibble, even bank nibble}.
	unsigned video_read(unsigned address) {
		dut.video_rd = 1;
		dut.video_addr = address & 0x7FF;
		tick();
		const unsigned value = static_cast<unsigned>(dut.video_rdata) & 0xFF;
		idle();
		return value;
	}
};

void expect_equal(unsigned got, unsigned want, const std::string& what) {
	if (got != want)
		fail(what + ": got 0x" +
		     [&] {
			     char buf[3 + 1]{};
			     std::snprintf(buf, sizeof(buf), "%02X", got);
			     return std::string(buf);
		     }() + ", expected 0x" +
		     [&] {
			     char buf[3 + 1]{};
			     std::snprintf(buf, sizeof(buf), "%02X", want);
			     return std::string(buf);
		     }());
}

void host_zero_and_latency(Bench& b) {
	// The module uses the existing named FPGA power-up assumption:
	// untouched locations read as zero.  Probe both logical banks and both
	// ends of the 12-bit host address space.
	const unsigned probes[] = {0x000, 0x001, 0x7FF, 0x800, 0xFFE, 0xFFF};
	for (unsigned address : probes)
		expect_equal(b.host_read(address), 0,
		             "defined-zero host read at 0x" +
		                 std::to_string(address));

	b.host_write(0x012, 0x0A);
	b.host_write(0x013, 0x05);
	b.host_write(0x014, 0x03);

	// Address changes must not alter the registered result before the next
	// rising edge.  The old result (A) and the new result (5) are distinct,
	// so this catches an accidental combinational host read.
	b.dut.host_rd = 1;
	b.dut.host_wr = 0;
	b.dut.host_addr = 0x012;
	b.tick();
	expect_equal(static_cast<unsigned>(b.dut.host_rdata) & 0xF, 0x0A,
	             "host read after one edge");
	b.dut.host_addr = 0x013;
	b.dut.eval();
	expect_equal(static_cast<unsigned>(b.dut.host_rdata) & 0xF, 0x0A,
	             "host result changed before its read edge");
	b.tick();
	expect_equal(static_cast<unsigned>(b.dut.host_rdata) & 0xF, 0x05,
	             "host read did not follow the next address after one edge");
	b.idle();

	// Host writes are low-nibble storage writes.  The direct 4-bit port
	// means no high byte can leak into a later read; exercise both banks.
	expect_equal(b.host_read(0x012), 0x0A, "even-bank host write/read");
	expect_equal(b.host_read(0x013), 0x05, "odd-bank host write/read");
	expect_equal(b.host_read(0x014), 0x03, "adjacent host write/read");
	expect_equal(b.host_read(0x015), 0x00,
	             "neighbour host location was disturbed");
}

void video_fetch_and_latency(Bench& b) {
	// Host addresses {video_addr, 0} and {video_addr, 1} form one video byte.
	// Seed adjacent byte pairs with distinct nibbles so bank and address order
	// are both observable.
	b.host_write(0x2A0, 0x1);
	b.host_write(0x2A1, 0xE);
	b.host_write(0x2A2, 0x4);
	b.host_write(0x2A3, 0xB);
	expect_equal(b.video_read(0x150), 0xE1,
	             "video packed byte at address 0x150");
	expect_equal(b.video_read(0x151), 0xB4,
	             "video packed byte at address 0x151");

	// The video result is registered too: a changed video address is not
	// visible until its own rising edge.
	b.dut.video_rd = 1;
	b.dut.video_addr = 0x150;
	b.tick();
	expect_equal(static_cast<unsigned>(b.dut.video_rdata) & 0xFF, 0xE1,
	             "video read after one edge");
	b.dut.video_addr = 0x151;
	b.dut.eval();
	expect_equal(static_cast<unsigned>(b.dut.video_rdata) & 0xFF, 0xE1,
	             "video result changed before its read edge");
	b.tick();
	expect_equal(static_cast<unsigned>(b.dut.video_rdata) & 0xFF, 0xB4,
	             "video read did not follow the next address after one edge");
	b.idle();
}

void dual_port_and_collision(Bench& b) {
	constexpr unsigned video_address = 0x1A5;
	constexpr unsigned even_host_address = video_address << 1;
	constexpr unsigned odd_host_address = even_host_address | 1;

	b.host_write(even_host_address, 0x2);
	b.host_write(odd_host_address, 0xD);

	// Both clients are active on the same edge, but each keeps its own
	// address/data path.  Use unrelated addresses to catch accidental port
	// sharing in either direction.
	b.host_write(0x020, 0x9);
	b.host_write(0x021, 0x6);
	b.dut.host_rd = 1;
	b.dut.host_wr = 0;
	b.dut.host_addr = 0x020;
	b.dut.video_rd = 1;
	b.dut.video_addr = video_address;
	b.tick();
	expect_equal(static_cast<unsigned>(b.dut.host_rdata) & 0xF, 0x9,
	             "simultaneous host read");
	expect_equal(static_cast<unsigned>(b.dut.video_rdata) & 0xFF, 0xD2,
	             "simultaneous video read");
	b.idle();

	// Block-RAM read-during-write is explicitly read-first for the video
	// port: the collision edge sees the old byte, and the next video read
	// edge sees the committed nibble.  Check both even and odd banks.
	b.dut.host_rd = 0;
	b.dut.host_wr = 1;
	b.dut.host_addr = even_host_address;
	b.dut.host_wdata = 0x7;
	b.dut.video_rd = 1;
	b.dut.video_addr = video_address;
	b.tick();
	expect_equal(static_cast<unsigned>(b.dut.video_rdata) & 0xFF, 0xD2,
	             "even-bank write/video collision exposed new data early");

	b.dut.host_wr = 0;
	b.tick();
	expect_equal(static_cast<unsigned>(b.dut.video_rdata) & 0xFF, 0xD7,
	             "even-bank write was not visible on the next video edge");

	b.dut.host_wr = 1;
	b.dut.host_addr = odd_host_address;
	b.dut.host_wdata = 0x4;
	b.tick();
	expect_equal(static_cast<unsigned>(b.dut.video_rdata) & 0xFF, 0xD7,
	             "odd-bank write/video collision exposed new data early");

	b.dut.host_wr = 0;
	b.tick();
	expect_equal(static_cast<unsigned>(b.dut.video_rdata) & 0xFF, 0x47,
	             "odd-bank write was not visible on the next video edge");
	b.idle();
}

void output_reset_preserves_memory(Bench& b) {
	b.host_write(0x4A0, 0x6);
	b.host_write(0x4A1, 0x9);
	expect_equal(b.host_read(0x4A0), 0x6, "pre-reset host data");
	expect_equal(b.video_read(0x250), 0x96, "pre-reset video data");

	b.dut.reset = 1;
	b.tick();
	expect_equal(static_cast<unsigned>(b.dut.host_rdata) & 0xF, 0,
	             "reset did not clear host read pipeline");
	expect_equal(static_cast<unsigned>(b.dut.video_rdata) & 0xFF, 0,
	             "reset did not clear video read pipeline");
	b.dut.reset = 0;
	b.idle();

	expect_equal(b.host_read(0x4A0), 0x6,
	             "reset incorrectly cleared host pixel storage");
	expect_equal(b.video_read(0x250), 0x96,
	             "reset incorrectly cleared video pixel storage");
}

} // namespace

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	try {
		Bench b;
		host_zero_and_latency(b);
		std::printf("PASS host: defined-zero, one-edge read latency, write/read mapping\n");
		video_fetch_and_latency(b);
		std::printf("PASS video: one-edge packed {odd,even} fetch latency\n");
		dual_port_and_collision(b);
		std::printf("PASS dual-port: independent reads and read-first write collisions\n");
		output_reset_preserves_memory(b);
		std::printf("PASS reset: read pipelines clear and pixel storage survives\n");
		std::printf("All plus_sprite_ram contract vectors passed\n");
		return 0;
	}
	catch (const std::exception& error) {
		std::printf("FAIL plus_sprite_ram: %s\n", error.what());
		return 1;
	}
}
