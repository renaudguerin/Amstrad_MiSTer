// Phase P8 unit test suite: Plus platform polish & SNA v3 CPC+ chunk parser.
//
// Tests:
// 1. i8255 Plus PPI quirks (Port B input-only, Port C output-only, control word rewrite latch preservation).
// 2. plus_sna_parser CPC+ chunk unpacking (sprite RAM nibbles, sprite attributes, palette, control regs, DMA, lock).
// 3. plus_model_select model decodes.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <stdexcept>

#include "Vplus_p8_test_top.h"
#include "verilated.h"

namespace {

void fail(const std::string& msg) {
	throw std::runtime_error(msg);
}

// -----------------------------------------------------------------------------
// Test 1: i8255 Plus PPI quirks
// -----------------------------------------------------------------------------
void test_p8_i8255_plus_quirks(Vplus_p8_test_top& dut) {
	dut.reset = 1;
	dut.clk = 0;
	dut.ppi_cs = 0;
	dut.ppi_we = 0;
	dut.ppi_oe = 0;
	dut.ppi_addr = 0;
	dut.ppi_idata = 0;
	dut.ppi_ipa = 0xAA;
	dut.ppi_ipb = 0x55;
	dut.ppi_ipc = 0xF0;
	dut.ppi_plus_mode = 1;
	dut.eval();

	auto tick = [&]() {
		dut.clk = 0; dut.eval();
		dut.clk = 1; dut.eval();
	};

	auto wr = [&](uint8_t a, uint8_t d) {
		dut.ppi_cs = 1; dut.ppi_we = 1; dut.ppi_oe = 0; dut.ppi_addr = a; dut.ppi_idata = d;
		tick();
		dut.ppi_we = 0;
		tick();
	};

	auto rd = [&](uint8_t a) -> uint8_t {
		dut.ppi_cs = 1; dut.ppi_we = 0; dut.ppi_oe = 1; dut.ppi_addr = a;
		dut.eval();
		return dut.ppi_odata;
	};

	// Reset pulse
	dut.reset = 1; tick(); tick();
	dut.reset = 0; tick();

	// 1. Write Port A = 0x33, Port C = 0x88
	wr(0, 0x33);
	wr(2, 0x88);

	// In Plus mode, Port C read returns output latch 0x88 (not IPC pins 0xF0)
	if (rd(2) != 0x88) fail("P8 i8255: Plus mode Port C read did not return output latch (got " + std::to_string(rd(2)) + ")");

	// In Plus mode, Port B read always returns IPB pins 0x55
	if (rd(1) != 0x55) fail("P8 i8255: Plus mode Port B read did not return IPB pins");

	// 2. Control word rewrite: write mode = 0x92 (all input)
	wr(3, 0x92);

	// In Plus mode, Port A and Port C output latches must be PRESERVED (0x33 and 0x88), not cleared!
	// Switching back to output mode (0x80)
	wr(3, 0x80);
	if (rd(2) != 0x88) fail("P8 i8255: Plus mode control rewrite cleared Port C latch");

	// 3. Classic mode test (plus_mode = 0): control word rewrite CLEARS latches
	dut.ppi_plus_mode = 0;
	wr(0, 0x44);
	wr(2, 0x77);
	// Control word rewrite in classic mode: clears latches to 0
	wr(3, 0x80);
	if (rd(2) != 0x00) fail("P8 i8255: Classic mode control rewrite did not clear Port C latch");

	std::printf("PASS p8_01: i8255 Plus PPI quirks (Port B in, Port C out, latch preservation)\n");
}

// -----------------------------------------------------------------------------
// Test 2: plus_sna_parser CPC+ chunk unpacking
// -----------------------------------------------------------------------------
void test_p8_sna_parser(Vplus_p8_test_top& dut) {
	dut.clk = 0;
	dut.reset = 1;
	dut.sna_download = 0;
	dut.cpc_plus_chunk_start = 0;
	dut.cpc_plus_byte_wr = 0;
	dut.cpc_plus_byte_data = 0;
	dut.eval();

	auto tick = [&]() {
		dut.clk = 0; dut.eval();
		dut.clk = 1; dut.eval();
	};

	dut.reset = 1; tick(); tick();
	dut.reset = 0;
	dut.sna_download = 1;
	tick();

	// Start of CPC+ chunk
	dut.cpc_plus_chunk_start = 1;
	tick();
	dut.cpc_plus_chunk_start = 0;

	struct AsicWrite { uint16_t addr; uint8_t data; };
	std::vector<AsicWrite> writes;

	auto send_byte = [&](uint8_t byte) {
		dut.cpc_plus_byte_wr = 1;
		dut.cpc_plus_byte_data = byte;
		tick();
		if (dut.asic_sna_wr) writes.push_back({dut.asic_sna_addr, dut.asic_sna_data});
		dut.cpc_plus_byte_wr = 0;
		tick();
		if (dut.asic_sna_wr) writes.push_back({dut.asic_sna_addr, dut.asic_sna_data});
	};

	// 1. Send Sprite RAM byte 0: 0xAB (pixel 0 = 0xA, pixel 1 = 0xB)
	send_byte(0xAB);
	if (writes.size() != 2 || writes[0].addr != 0x0000 || writes[0].data != 0x0A ||
	    writes[1].addr != 0x0001 || writes[1].data != 0x0B) {
		fail("P8 sna_parser: Sprite RAM nibble splitting failed");
	}

	// 2. Fast forward through remaining sprite bytes (2047 bytes)
	for (int i = 1; i < 2048; ++i) {
		send_byte(0x00);
	}

	// 3. Send Sprite 0 attributes (offset 0x800..0x804): X=0x0150, Y=0x0080, Mag=0x05
	writes.clear();
	send_byte(0x50); // &6000 X lo
	send_byte(0x01); // &6001 X hi
	send_byte(0x80); // &6002 Y lo
	send_byte(0x00); // &6003 Y hi
	send_byte(0x05); // &6004 Mag
	send_byte(0x00); send_byte(0x00); send_byte(0x00); // 0x805..0x807 unused

	if (writes.size() < 5 || writes[0].addr != 0x2000 || writes[0].data != 0x50 ||
	    writes[1].addr != 0x2001 || writes[1].data != 0x01 ||
	    writes[4].addr != 0x2004 || writes[4].data != 0x05) {
		fail("P8 sna_parser: Sprite attribute decoding failed");
	}

	// 4. Fast forward to 0x880 (15 remaining sprites * 8 bytes = 120 bytes)
	for (int i = 0; i < 120; ++i) send_byte(0x00);

	// 5. Send Palette entry 0 (offset 0x880..0x881): 0x12, 0x03 -> {G:3, R:1, B:2}
	writes.clear();
	send_byte(0x12); // even byte: &6400
	send_byte(0x03); // odd byte:  &6401
	if (writes.size() != 2 || writes[0].addr != 0x2400 || writes[0].data != 0x12 ||
	    writes[1].addr != 0x2401 || writes[1].data != 0x03) {
		fail("P8 sna_parser: Palette unpacking failed");
	}

	// 6. Fast forward to 0x8C0 (31 remaining palette entries * 2 = 62 bytes)
	for (int i = 0; i < 62; ++i) send_byte(0x00);

	// 7. Send PRI &6800 (offset 0x8C0) = 0x2A
	writes.clear();
	send_byte(0x2A);
	if (writes.size() != 1 || writes[0].addr != 0x2800 || writes[0].data != 0x2A) {
		fail("P8 sna_parser: PRI register unpacking failed");
	}

	// 8. Fast forward to 0x8F5 (RMR2) and 0x8F6 (Unlock)
	// Current offset is 0x8C1. 0x8F5 - 0x8C1 = 52 bytes
	for (int i = 0; i < 52; ++i) send_byte(0x00);

	// 0x8F5: RMR2 = 0x19 (ASIC page enabled, page 1)
	send_byte(0x19);
	if (dut.asic_sna_rmr2 != 0x19) fail("P8 sna_parser: RMR2 capture failed");

	// 0x8F6: ASIC unlocked = 1
	send_byte(0x01);
	if (dut.asic_sna_unlock != 1) fail("P8 sna_parser: Unlock status capture failed");

	std::printf("PASS p8_02: plus_sna_parser CPC+ chunk decoding (Sprite RAM, Pal, Regs, MMU)\n");
}

// -----------------------------------------------------------------------------
// Test 3: Model Capability Decodes
// -----------------------------------------------------------------------------
void test_p8_model_decodes(Vplus_p8_test_top& dut) {
	// 2'b00: Classic CPC
	dut.model_plus_model = 0; dut.eval();
	if (dut.model_plus_mode != 0) fail("P8 model: Classic mode not plus_mode=0");

	// 2'b01: GX4000
	dut.model_plus_model = 1; dut.eval();
	if (dut.model_plus_mode != 1 || dut.model_ram_128k != 0 || dut.model_has_fdc != 0 || dut.model_has_tape != 0)
		fail("P8 model: GX4000 capability decode incorrect");

	// 2'b10: CPC 6128 Plus
	dut.model_plus_model = 2; dut.eval();
	if (dut.model_plus_mode != 1 || dut.model_ram_128k != 1 || dut.model_has_fdc != 1 || dut.model_has_tape != 0)
		fail("P8 model: 6128 Plus capability decode incorrect");

	// 2'b11: CPC 464 Plus
	dut.model_plus_model = 3; dut.eval();
	if (dut.model_plus_mode != 1 || dut.model_ram_128k != 0 || dut.model_has_fdc != 0 || dut.model_has_tape != 1)
		fail("P8 model: 464 Plus capability decode incorrect");

	std::printf("PASS p8_03: plus_model_select capability decodes (GX4000, 6128+, 464+)\n");
}

} // namespace

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	try {
		Vplus_p8_test_top dut;
		test_p8_i8255_plus_quirks(dut);
		test_p8_sna_parser(dut);
		test_p8_model_decodes(dut);
		std::printf("All Phase P8 platform polish and SNA v3 tests PASSED.\n");
		return 0;
	} catch (const std::exception& e) {
		std::fprintf(stderr, "FAIL: %s\n", e.what());
		return 1;
	}
}
