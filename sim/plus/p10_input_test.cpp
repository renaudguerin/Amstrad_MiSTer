// Focused production PPI -> YM2149 -> HID/joystick test.

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "Vp10_input_test_top.h"
#include "verilated.h"

namespace {

class TestFailure : public std::runtime_error {
public:
	explicit TestFailure(const std::string& m) : std::runtime_error(m) {}
};

[[noreturn]] void fail(const std::string& m) { throw TestFailure(m); }

class Bench {
public:
	Vp10_input_test_top dut;
	uint64_t cycle = 0;

	explicit Bench() : dut("p10_input_test_top") {
		dut.clk = 0;
		dut.reset = 1;
		dut.ps2_key = 0;
		dut.user_in = 0x7F;
		dut.snac_player = 1;
		dut.joy1_usb = 0;
	}

	void tick() {
		dut.clk = 0;
		dut.eval();
		dut.clk = 1;
		dut.eval();
		++cycle;
	}

	void run(unsigned n) { for (unsigned i = 0; i < n; ++i) tick(); }

	uint8_t read_count() const {
		return dut.read_count_o;
	}
	uint8_t read_value(unsigned n) const {
		switch (n) {
		case 0: return dut.read0_o;
		case 1: return dut.read1_o;
		case 2: return dut.read2_o;
		case 3: return dut.read3_o;
		case 4: return dut.read4_o;
		default: return 0;
		}
	}
	bool done() const {
		return dut.done_o;
	}
};

void check_eq(uint8_t actual, uint8_t expected, const char* label) {
	if (actual != expected) {
		char msg[160];
		std::snprintf(msg, sizeof(msg), "%s: expected 0x%02X, got 0x%02X",
		              label, expected, actual);
		fail(msg);
	}
}

int run() {
	Bench b;
	b.run(32);
	b.dut.reset = 0;

	// Change sources only after the preceding read has been captured.  The
	// script leaves >15 clocks between reads, so HID's synchronous PS/2 edge
	// detector settles before the next AY R14 sample.
	bool injected = false;
	bool snac_fire = false;
	bool usb_fire = false;
	for (uint64_t guard = 0; guard < 200000 && !b.done(); ++guard) {
		b.tick();
		const uint8_t n = b.read_count();
		if (!injected && n >= 1) {
			// PS/2 make code A: bit 10 toggles, bit 9 is press, code 0x1C.
			b.dut.ps2_key = 0x61C;
			injected = true;
		}
		if (!snac_fire && n >= 3) {
			// DB9 pin 7 / Fire 1 is active low at USER_IN[2].
			b.dut.user_in = 0x7B;
			snac_fire = true;
		}
		if (!usb_fire && n >= 4) {
			// Disable SNAC and assert USB joystick 1 Fire 1 (bit 4).
			b.dut.snac_player = 0;
			b.dut.joy1_usb = 0x10;
			usb_fire = true;
		}
	}
	if (!b.done()) fail("script did not finish");
	if (b.read_count() != 5) fail("expected five AY R14 samples");

	check_eq(b.read_value(0), 0xFF, "row 8 idle");
	check_eq(b.read_value(1), 0xDF, "row 8 PS2 A");
	check_eq(b.read_value(2), 0xFF, "row 9 SNAC P1 idle");
	check_eq(b.read_value(3), 0xEF, "row 9 SNAC P1 fire 1");
	check_eq(b.read_value(4), 0xEF, "row 9 USB fire 1");

	if (b.dut.port_c_o != 0x49)
		fail("Plus PPI Port C did not retain row-9 read selection");
	std::printf("p10 input checks passed: PPI 0x9B, AY R7/R14, PS2 A, SNAC and USB Fire 1\n");
	return 0;
}

} // namespace

int main() {
	try {
		return run();
	} catch (const TestFailure& e) {
		std::fprintf(stderr, "p10 input test failed: %s\n", e.what());
		return 1;
	}
}
