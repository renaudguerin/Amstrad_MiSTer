// Phase P8 unit test suite: Plus platform polish & SNA v3 CPC+ chunk parser.
//
// Tests:
// 1. i8255 Plus PPI quirks (Port B input-only, Port C output-only, control word rewrite latch preservation).
// 2. plus_sna_parser CPC+ chunk unpacking (sprite RAM nibbles, sprite attributes, palette, control regs, DMA, lock).
// 3. plus_model_select model decodes.

#include <cstdint>
#include <cstddef>
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

	// In Plus mode, physical Port C pins (ppi_opc) MUST drive opc_r (0x88) even if mode is reset default 0x9B
	if (dut.ppi_opc != 0x88) fail("P8 i8255: Plus mode physical Port C pins did not drive opc_r under mode 0x9B (got " + std::to_string(dut.ppi_opc) + ")");

	// In Plus mode, Port B read always returns IPB pins 0x55
	if (rd(1) != 0x55) fail("P8 i8255: Plus mode Port B read did not return IPB pins");

	// 2. Control word rewrite: write mode = 0x92 (all input)
	wr(3, 0x92);

	// In Plus mode, physical Port C pins MUST remain driven by opc_r (0x88) under mode 0x92
	if (dut.ppi_opc != 0x88) fail("P8 i8255: Plus mode physical Port C pins were suppressed under mode 0x92 (got " + std::to_string(dut.ppi_opc) + ")");

	// In Plus mode, Port A and Port C output latches must be PRESERVED (0x33 and 0x88), not cleared!
	// Switching back to output mode (0x80)
	wr(3, 0x80);
	if (rd(2) != 0x88) fail("P8 i8255: Plus mode control rewrite cleared Port C latch");
	if (dut.ppi_opc != 0x88) fail("P8 i8255: Plus mode physical Port C pins did not drive 0x88 under mode 0x80");

	// 3. Classic mode test (plus_mode = 0): control word rewrite CLEARS latches and mode controls physical pins
	dut.ppi_plus_mode = 0;
	wr(0, 0x44);
	wr(2, 0x77);
	// In classic mode under mode 0x80, opc drives 0x77
	if (dut.ppi_opc != 0x77) fail("P8 i8255: Classic mode physical Port C pins failed under mode 0x80");

	// Control word rewrite in classic mode: clears latches to 0
	wr(3, 0x80);
	if (rd(2) != 0x00) fail("P8 i8255: Classic mode control rewrite did not clear Port C latch");
	if (dut.ppi_opc != 0x00) fail("P8 i8255: Classic mode physical Port C pins failed to drive cleared latch 0x00");

	// Set classic mode to 0x9B (all input): physical opc pins must be clamped to 0xFF
	wr(3, 0x9B);
	if (dut.ppi_opc != 0xFF) fail("P8 i8255: Classic mode physical Port C pins did not float/clamp to 0xFF under mode 0x9B (got " + std::to_string(dut.ppi_opc) + ")");

	std::printf("PASS p8_01: i8255 Plus PPI quirks (Port B in, Port C out, latch preservation, physical pin driving)\n");
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

	// 1. Send Sprite RAM bytes 0 & 1 consecutively (0x12, 0x34) without idle cycle
	dut.cpc_plus_byte_wr = 1; dut.cpc_plus_byte_data = 0x12; tick();
	if (dut.asic_sna_wr) writes.push_back({dut.asic_sna_addr, dut.asic_sna_data});
	dut.cpc_plus_byte_wr = 1; dut.cpc_plus_byte_data = 0x34; tick();
	if (dut.asic_sna_wr) writes.push_back({dut.asic_sna_addr, dut.asic_sna_data});
	dut.cpc_plus_byte_wr = 0;
	// Drain FIFO
	for (int d = 0; d < 5; ++d) {
		tick();
		if (dut.asic_sna_wr) writes.push_back({dut.asic_sna_addr, dut.asic_sna_data});
	}

	if (writes.size() != 4 ||
	    writes[0].addr != 0x0000 || writes[0].data != 0x01 ||
	    writes[1].addr != 0x0001 || writes[1].data != 0x02 ||
	    writes[2].addr != 0x0002 || writes[2].data != 0x03 ||
	    writes[3].addr != 0x0003 || writes[3].data != 0x04) {
		fail("P8 sna_parser: Consecutive Sprite RAM nibble splitting failed (size=" + std::to_string(writes.size()) + ")");
	}

	// 2. Fast forward through remaining sprite bytes (2046 bytes)
	for (int i = 2; i < 2048; ++i) {
		send_byte(0x00);
	}
	while (dut.asic_sna_wr) {
		tick();
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

	// The top-level applies sna_load only after the file download has ended
	// and the ordinary snapshot writes have drained.  These shadow values
	// must survive that gap so plus_mmu and asic_unlock can consume them.
	dut.sna_download = 0;
	tick();
	if (dut.asic_sna_rmr2 != 0x19)
		fail("P8 sna_parser: RMR2 was cleared before delayed sna_load apply");
	if (dut.asic_sna_unlock != 1)
		fail("P8 sna_parser: Unlock state was cleared before delayed sna_load apply");

	// Starting a later snapshot clears the retained shadow immediately; a
	// classic SNA with no CPC+ chunk must not inherit the previous Plus map.
	dut.sna_download = 1;
	tick();
	if (dut.asic_sna_rmr2 != 0 || dut.asic_sna_unlock != 0 || dut.asic_sna_active != 0)
		fail("P8 sna_parser: New snapshot inherited prior CPC+ shadow state");

	std::printf("PASS p8_02: plus_sna_parser CPC+ chunk decoding (Sprite RAM, Pal, Regs, MMU)\n");
}

// -----------------------------------------------------------------------------
// Test 2b: SNA sprite expansion FIFO headroom at the production wait seam
// -----------------------------------------------------------------------------
void test_p8_sna_fifo_headroom(Vplus_p8_test_top& dut) {
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

	struct AsicWrite { uint16_t addr; uint8_t data; };
	std::vector<AsicWrite> writes;
	auto tick_capture = [&]() {
		tick();
		if (dut.asic_sna_wr)
			writes.push_back({dut.asic_sna_addr, dut.asic_sna_data});
	};

	dut.reset = 1; tick(); tick();
	dut.reset = 0;
	dut.sna_download = 1;
	tick();
	dut.cpc_plus_chunk_start = 1;
	tick();
	dut.cpc_plus_chunk_start = 0;

	// Each sprite payload byte expands to two FIFO writes while one old entry
	// can drain per clock.  Assert wait with five physical slots still free:
	// two bytes already accepted by the production pipeline can then arrive
	// after wait without filling the eight-entry ring or aliasing its pointers.
	dut.cpc_plus_byte_wr = 1; dut.cpc_plus_byte_data = 0x12; tick_capture();
	dut.cpc_plus_byte_data = 0x34; tick_capture();
	if (!dut.sna_ioctl_wait) {
		fail("P8 sna FIFO: wait left insufficient strict headroom for two accepted sprite bytes");
	}

	// Model the maximum production tail even though wait is already high.
	dut.cpc_plus_byte_data = 0x56; tick_capture();
	dut.cpc_plus_byte_data = 0x78; tick_capture();
	dut.cpc_plus_byte_wr = 0;
	dut.cpc_plus_byte_data = 0xFF;
	dut.sna_download = 0;
	dut.eval();
	if (!dut.sna_busy)
		fail("P8 sna FIFO: apply barrier dropped with accepted sprite writes queued");

	int drain_cycles = 0;
	while (dut.sna_busy && drain_cycles < 20) {
		tick_capture();
		++drain_cycles;
		if (!dut.sna_busy && writes.size() != 8)
			fail("P8 sna FIFO: apply barrier cleared before every expanded write drained");
	}
	if (dut.sna_busy)
		fail("P8 sna FIFO: busy failed to clear after adversarial tail drained");

	const std::vector<AsicWrite> expected = {
		{0x0000, 0x01}, {0x0001, 0x02},
		{0x0002, 0x03}, {0x0003, 0x04},
		{0x0004, 0x05}, {0x0005, 0x06},
		{0x0006, 0x07}, {0x0007, 0x08}
	};
	if (writes.size() != expected.size())
		fail("P8 sna FIFO: adversarial tail write count mismatch (expected 8, got " +
		     std::to_string(writes.size()) + ")");
	for (std::size_t i = 0; i < expected.size(); ++i) {
		if (writes[i].addr != expected[i].addr || writes[i].data != expected[i].data) {
			fail("P8 sna FIFO: pointer alias/order error at write " + std::to_string(i));
		}
	}

	std::printf("PASS p8_02b: SNA sprite FIFO preserves two-byte production tail and apply barrier\n");
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

// -----------------------------------------------------------------------------
// Test 4: P10c FDC / Motor / Tape Model Gating & Aliases
// -----------------------------------------------------------------------------
void test_p10c_fdc_motor_tape_gating(Vplus_p8_test_top& dut) {
	struct FdcCase {
		const char *name;
		uint16_t addr;
		bool plus_mode;
		bool has_fdc;
		bool fdc_disabled;
		bool expect_motor;
		bool expect_u765;
	};

	auto check_cases = [&](const FdcCase *cases, std::size_t count) {
		for (std::size_t i = 0; i < count; ++i) {
			const FdcCase &test = cases[i];
			dut.fdc_test_addr = test.addr;
			dut.fdc_test_status17 = test.fdc_disabled ? 1 : 0;
			dut.fdc_test_plus_mode = test.plus_mode ? 1 : 0;
			dut.fdc_test_has_fdc = test.has_fdc ? 1 : 0;
			dut.eval();
			if (dut.fdc_motor_sel != (test.expect_motor ? 1 : 0))
				fail("P10c FDC decode: " + std::string(test.name) +
				     " expected fdc_motor_sel=" + std::to_string(test.expect_motor) +
				     " got " + std::to_string(dut.fdc_motor_sel));
			if (dut.u765_sel != (test.expect_u765 ? 1 : 0))
				fail("P10c FDC decode: " + std::string(test.name) +
				     " expected u765_sel=" + std::to_string(test.expect_u765) +
				     " got " + std::to_string(dut.u765_sel));
		}
	};

	// Classic CPC: A10, A8 and A7 are decoded; A9 and A4-A1 are ignored.
	// A0 is passed to the uPD765 as its status/data register select.
	const FdcCase classic_cases[] = {
		{"Classic F87E motor (A9=0)", 0xF87E, false, false, false, true,  false},
		{"Classic F96E status (A9=0,A4=0)", 0xF96E, false, false, false, false, true},
		{"Classic FB6F data (A9=1)", 0xFB6F, false, false, false, false, true},
		{"Classic F800 motor (ignored lower bits)", 0xF800, false, false, false, true,  false},
		{"Classic FADD rejects A7=1 motor alias", 0xFADD, false, false, false, false, false},
		{"Classic FBDF rejects A7=1 data alias", 0xFBDF, false, false, false, false, false},
		// The menu switch disables controller access, not the independent motor
		// latch, matching the production wiring that existed before this split.
		{"Classic FB6F uPD765 suppressed when FDC disabled", 0xFB6F, false, false, true, false, false},
		{"Classic F87E motor remains decoded when FDC disabled", 0xF87E, false, false, true, true, false},
	};
	check_cases(classic_cases, sizeof(classic_cases) / sizeof(classic_cases[0]));

	// 6128 Plus: retain the firmware aliases while keeping PlayCity and the
	// Kempston mouse out of the FDC path.
	const FdcCase plus_cases[] = {
		{"6128+ FA7E motor", 0xFA7E, true, true, false, true,  false},
		{"6128+ FADD motor alias", 0xFADD, true, true, false, true,  false},
		{"6128+ FB7E status", 0xFB7E, true, true, false, false, true},
		{"6128+ FB7F data", 0xFB7F, true, true, false, false, true},
		{"6128+ FBDF data alias", 0xFBDF, true, true, false, false, true},
		{"6128+ FB7E suppressed when FDC disabled", 0xFB7E, true, true, true, false, false},
		{"6128+ FBEE rejects Kempston mouse", 0xFBEE, true, true, false, false, false},
		{"6128+ F800 rejects PlayCity", 0xF800, true, true, false, false, false},
		{"6128+ F87E rejects A9=0 motor", 0xF87E, true, true, false, false, false},
		{"6128+ F96E rejects A9=0 uPD765", 0xF96E, true, true, false, false, false},
	};
	check_cases(plus_cases, sizeof(plus_cases) / sizeof(plus_cases[0]));

	// GX4000 and 464 Plus expose no FDC, regardless of the address.
	const FdcCase no_fdc_cases[] = {
		{"GX4000 FA7E motor gated", 0xFA7E, true, false, false, false, false},
		{"GX4000 FADD motor gated", 0xFADD, true, false, false, false, false},
		{"GX4000 FB7E status gated", 0xFB7E, true, false, false, false, false},
		{"GX4000 FBDF data gated", 0xFBDF, true, false, false, false, false},
		{"464+ FA7E motor gated", 0xFA7E, true, false, false, false, false},
		{"464+ FB7E status gated", 0xFB7E, true, false, false, false, false},
	};
	check_cases(no_fdc_cases, sizeof(no_fdc_cases) / sizeof(no_fdc_cases[0]));

	std::printf("PASS p10_03: model-specific FDC/motor decode aliases\n");
}

// -----------------------------------------------------------------------------
// Test 5: Integrated SNA parser + motherboard / asic_regs / MMU reset seam
// -----------------------------------------------------------------------------
void test_p8_sna_integration_seam(Vplus_p8_test_top& dut) {
	auto tick = [&]() {
		dut.clk = 0; dut.eval();
		dut.clk = 1; dut.eval();
	};

	// Helper to read from asic_regs via the CPU memory port
	auto aregs_read = [&](uint16_t addr) -> uint8_t {
		dut.aregs_cs = 1;
		dut.aregs_mem_rd = 1;
		dut.aregs_mem_wr = 0;
		dut.aregs_addr = addr;
		dut.eval();
		uint8_t d = dut.aregs_dout;
		dut.aregs_cs = 0;
		dut.aregs_mem_rd = 0;
		dut.eval();
		return d;
	};

	// 1. Initial State: Machine running in Plus mode, initial values in asic_regs
	dut.clk = 0;
	dut.reset = 0;
	dut.seam_machine_reset = 0;
	dut.seam_plus_asic_reset = 1; // start from clean reset
	dut.seam_plus_mode = 1;
	dut.seam_sna_load = 0;
	dut.sna_download = 0;
	dut.cpc_plus_chunk_start = 0;
	dut.cpc_plus_byte_wr = 0;
	dut.cpc_plus_byte_data = 0;
	dut.aregs_cs = 0;
	dut.aregs_mem_rd = 0;
	dut.aregs_mem_wr = 0;
	dut.aregs_pal_raddr = 0;
	dut.eval();

	tick(); tick();
	dut.seam_plus_asic_reset = 0;
	tick();

	// 2. SNA download begins:
	// - seam_machine_reset asserts (holding CPU/MMU/motherboard in reset)
	// - seam_plus_asic_reset pulses for 1 clock cycle to reset asic_regs once
	dut.sna_download = 1;
	dut.seam_machine_reset = 1;
	dut.seam_plus_asic_reset = 1; // 1-clock start pulse
	tick();
	dut.seam_plus_asic_reset = 0; // returns to 0 while machine_reset stays 1
	tick();

	if (dut.aregs_pri != 0 || dut.aregs_ivr != 1 || dut.aregs_dcsr != 0) {
		fail("P8 seam: asic_regs was not reset cleanly at SNA start pulse");
	}

	// 3. Send CPC+ chunk header: 1-clock start strobe
	dut.cpc_plus_chunk_start = 1;
	tick();
	dut.cpc_plus_chunk_start = 0;

	// Simulate HPS stall: 4 idle clock cycles where cpc_plus_byte_wr == 0
	for (int i = 0; i < 4; ++i) {
		tick();
		if (dut.asic_sna_wr) {
			fail("P8 seam: asic_sna_wr asserted before any payload byte arrived");
		}
	}

	// Helper to send a byte as a 1-clock accepted pulse followed by possible stall
	auto stream_byte = [&](uint8_t byte, int stall_cycles = 0) {
		while (dut.sna_ioctl_wait) tick();
		dut.cpc_plus_byte_wr = 1;
		dut.cpc_plus_byte_data = byte;
		tick();
		dut.cpc_plus_byte_wr = 0;
		dut.cpc_plus_byte_data = 0xFF; // change data bus to confirm parser captured byte
		for (int s = 0; s < stall_cycles; ++s) {
			tick();
		}
	};

	// 4. Test the production pipeline tail and FIFO drain. The top-level
	// registers accepted payloads, so its final strobe can reach the parser on
	// the first clock after sna_download falls. That byte must still enqueue.
	// Send two ordinary bytes, then the third on the falling-download clock.
	struct AsicWrite { uint16_t addr; uint8_t data; };
	std::vector<AsicWrite> writes;
	auto capture_write = [&]() {
		if (dut.asic_sna_wr) writes.push_back({dut.asic_sna_addr, dut.asic_sna_data});
	};

	dut.cpc_plus_byte_wr = 1; dut.cpc_plus_byte_data = 0x12; tick();
	capture_write();
	dut.cpc_plus_byte_wr = 1; dut.cpc_plus_byte_data = 0x34; tick();
	capture_write();
	dut.sna_download = 0;
	dut.cpc_plus_byte_wr = 1; dut.cpc_plus_byte_data = 0x56;
	dut.eval();
	if (!dut.sna_busy) {
		fail("P8 seam: tail payload strobe did not hold sna_busy after download fell");
	}
	tick();
	capture_write();
	dut.cpc_plus_byte_wr = 0;
	dut.cpc_plus_byte_data = 0xFF;
	dut.eval();

	// Verify sna_busy handshake is asserted while FIFO has queued writes
	if (!dut.sna_busy) {
		fail("P8 seam: sna_busy was not asserted when sna_download dropped with queued FIFO writes");
	}

	// Drain FIFO until sna_busy clears
	int drain_cycles = 0;
	while (dut.sna_busy && drain_cycles < 20) {
		tick();
		capture_write();
		drain_cycles++;
	}

	if (dut.sna_busy) {
		fail("P8 seam: sna_busy failed to clear after FIFO drain");
	}
	const std::vector<AsicWrite> expected_writes = {
		{0x0000, 0x01},
		{0x0001, 0x02},
		{0x0002, 0x03},
		{0x0003, 0x04},
		{0x0004, 0x05},
		{0x0005, 0x06}
	};
	if (writes.size() != expected_writes.size()) {
		fail("P8 seam: FIFO write count mismatch after tail drain (expected " +
		     std::to_string(expected_writes.size()) + ", got " + std::to_string(writes.size()) + ")");
	}
	for (std::size_t i = 0; i < expected_writes.size(); ++i) {
		if (writes[i].addr != expected_writes[i].addr || writes[i].data != expected_writes[i].data) {
			fail("P8 seam: FIFO write mismatch at index " + std::to_string(i) +
			     " (expected addr=" + std::to_string(expected_writes[i].addr) +
			     " data=" + std::to_string(expected_writes[i].data) +
			     ", got addr=" + std::to_string(writes[i].addr) +
			     " data=" + std::to_string(writes[i].data) + ")");
		}
	}

	// 5. A rapid new snapshot must abort a stale write tail. Queue another
	// partial CPC+ image, let download fall for one clock, then start the next
	// snapshot before the old FIFO can drain. Production pulses ASIC reset on
	// the restart edge, while the parser must discard its old pointers/write.
	dut.sna_download = 1;
	dut.seam_plus_asic_reset = 1;
	tick();
	dut.seam_plus_asic_reset = 0;
	dut.cpc_plus_chunk_start = 1;
	tick();
	dut.cpc_plus_chunk_start = 0;
	stream_byte(0xAB);
	stream_byte(0xCD);
	stream_byte(0xEF);
	dut.sna_download = 0;
	tick();
	if (!dut.sna_busy) {
		fail("P8 seam: stale pre-restart FIFO unexpectedly drained in one clock");
	}

	dut.sna_download = 1;
	dut.seam_plus_asic_reset = 1;
	tick();
	dut.seam_plus_asic_reset = 0;
	for (int i = 0; i < 4; ++i) {
		tick();
		if (dut.asic_sna_wr) {
			fail("P8 seam: prior snapshot write tail leaked after rapid restart");
		}
	}

	// 6. Now stream the full CPC+ snapshot chunk.
	dut.cpc_plus_chunk_start = 1;
	tick();
	dut.cpc_plus_chunk_start = 0;

	// 6a. Stream Sprite RAM bytes (0x000-0x003: 4 bytes = 8 pixels).
	// Each input byte expands to two writes while the output drains one per
	// clock, so the third consecutive byte reaches the production wait
	// watermark. Stop accepting input until the parser releases wait.
	// Payload 0x12 -> pixels 1, 2; 0x34 -> pixels 3, 4; 0x56 -> pixels 5, 6; 0x78 -> pixels 7, 8
	stream_byte(0x12);
	stream_byte(0x34);
	stream_byte(0x56);
	if (!dut.sna_ioctl_wait) {
		fail("P8 seam: sprite expansion did not assert parser backpressure");
	}
	while (dut.sna_ioctl_wait) tick();
	stream_byte(0x78);

	// Fast-forward through remaining sprite RAM (total 2048 bytes)
	for (int i = 4; i < 2048; ++i) {
		stream_byte(0x00);
	}
	while (dut.sna_ioctl_wait) tick();

	// 6b. Sprite 0 attributes (0x800-0x807): X=0x150, Y=0x080, Mag=0x05
	stream_byte(0x50, 2); // X lo (&6000) with 2-clock stall
	stream_byte(0x01, 1); // X hi (&6001)
	stream_byte(0x80, 0); // Y lo (&6002)
	stream_byte(0x00, 3); // Y hi (&6003)
	stream_byte(0x05, 1); // Mag  (&6004)
	stream_byte(0x00); stream_byte(0x00); stream_byte(0x00); // 0x805-0x807 unused

	// Sprite 1 attributes (0x808-0x80F): X=0x0A0, Y=0x030, Mag=0x0A
	stream_byte(0xA0); // X lo
	stream_byte(0x00); // X hi
	stream_byte(0x30); // Y lo
	stream_byte(0x00); // Y hi
	stream_byte(0x0A); // Mag
	stream_byte(0x00); stream_byte(0x00); stream_byte(0x00);

	// Fast-forward remaining 14 sprites * 8 bytes = 112 bytes
	for (int i = 0; i < 112; ++i) stream_byte(0x00);

	// 6c. Palette entries:
	// Entry 0 (&6400/&6401): 0x24 (R=2, B=4), 0x09 (G=9) -> {G:9, R:2, B:4} = 0x924
	stream_byte(0x24, 1);
	stream_byte(0x09, 2);
	// Entry 1 (&6402/&6403): 0x56 (R=5, B=6), 0x0C (G=12) -> {G:12, R:5, B:6} = 0xC56
	stream_byte(0x56);
	stream_byte(0x0C);
	// Fast forward to entry 16 (Border): 14 entries * 2 = 28 bytes
	for (int i = 0; i < 28; ++i) stream_byte(0x00);
	// Entry 16 (&6420/&6421): 0x78 (R=7, B=8), 0x0E (G=14) -> {G:14, R:7, B:8} = 0xE78
	stream_byte(0x78);
	stream_byte(0x0E);
	// Fast forward remaining 15 entries * 2 = 30 bytes
	for (int i = 0; i < 30; ++i) stream_byte(0x00);

	// 6d. Control registers (0x8C0-0x8C5):
	stream_byte(0x2A); // PRI  (&6800)
	stream_byte(0x55); // SPLT (&6801)
	stream_byte(0x30); // SSA hi (&6802)
	stream_byte(0x40); // SSA lo (&6803)
	stream_byte(0x03); // SSCR (&6804)
	stream_byte(0xFE); // IVR  (&6805)

	// Fast forward to DMA registers (0x8D0): 10 bytes (0x8C6-0x8CF)
	for (int i = 0; i < 10; ++i) stream_byte(0x00);

	// 6e. Sound DMA registers (0x8D0-0x8DB, 0x8DF):
	stream_byte(0x11); // SAR0 lo (&6C00)
	stream_byte(0x22); // SAR0 hi (&6C01)
	stream_byte(0x33); // PPR0    (&6C02)
	stream_byte(0x00); // 0x8D3
	stream_byte(0x44); // SAR1 lo (&6C04)
	stream_byte(0x55); // SAR1 hi (&6C05)
	stream_byte(0x66); // PPR1    (&6C06)
	stream_byte(0x00); // 0x8D7
	stream_byte(0x77); // SAR2 lo (&6C08)
	stream_byte(0x88); // SAR2 hi (&6C09)
	stream_byte(0x99); // PPR2    (&6C0A)
	stream_byte(0x00); // 0x8DB
	// Fast forward to 0x8DF (DCSR): 3 bytes (0x8DC-0x8DE)
	stream_byte(0x00); stream_byte(0x00); stream_byte(0x00);
	stream_byte(0x87); // DCSR (&6C0F): stat=1, ena=3'b111

	// Fast forward to RMR2 (0x8F5): 21 bytes (0x8E0-0x8F4)
	for (int i = 0; i < 21; ++i) stream_byte(0x00);
	stream_byte(0x19); // RMR2 (0x8F5): D4D3=11 (ASIC page on), page 1
	stream_byte(0x01); // Unlock (0x8F6): 1=unlocked

	// 7. Snapshot stream ends: sna_download falls to 0
	dut.sna_download = 0;
	tick();

	// Drain any remaining cycles until sna_busy clears
	int drain_count = 0;
	while (dut.sna_busy && drain_count < 30) {
		tick();
		drain_count++;
	}

	if (dut.sna_busy) {
		fail("P8 seam: sna_busy failed to clear after stream end");
	}

	// 10. Verify retention during gap before sna_load
	if (dut.asic_sna_rmr2 != 0x19 || dut.asic_sna_unlock != 1) {
		fail("P8 seam: Shadow RMR2/unlock corrupted before sna_load");
	}

	// 11. Machine reset drops, sna_load fires
	dut.seam_machine_reset = 0;
	dut.seam_sna_load = 1;
	tick();
	dut.seam_sna_load = 0;
	tick();

	// 12. Verify MMU and Unlock state applied
	if (!dut.mmu_asic_page_on) {
		fail("P8 seam: plus_mmu asic_page_on not enabled after sna_load");
	}
	if (!dut.mmu_asic_unlocked) {
		fail("P8 seam: asic_unlock unlocked status not set after sna_load");
	}

	// 13. Verify all ASIC registers survived machine reset:
	if (dut.aregs_pri != 0x2A) fail("P8 seam: PRI mismatch (expected 0x2A, got " + std::to_string(dut.aregs_pri) + ")");
	if (dut.aregs_splt != 0x55) fail("P8 seam: SPLT mismatch (expected 0x55, got " + std::to_string(dut.aregs_splt) + ")");
	if (dut.aregs_ssa_hi != 0x30) fail("P8 seam: SSA_HI mismatch (expected 0x30, got " + std::to_string(dut.aregs_ssa_hi) + ")");
	if (dut.aregs_ssa_lo != 0x40) fail("P8 seam: SSA_LO mismatch (expected 0x40, got " + std::to_string(dut.aregs_ssa_lo) + ")");
	if (dut.aregs_sscr != 0x03) fail("P8 seam: SSCR mismatch (expected 0x03, got " + std::to_string(dut.aregs_sscr) + ")");
	if (dut.aregs_ivr != 0xFE) fail("P8 seam: IVR mismatch (expected 0xFE, got " + std::to_string(dut.aregs_ivr) + ")");

	// Sound DMA registers:
	if (dut.aregs_sar0_lo != 0x11 || dut.aregs_sar0_hi != 0x22 || dut.aregs_ppr0 != 0x33) {
		fail("P8 seam: Channel 0 DMA registers mismatch");
	}
	if (dut.aregs_sar1_lo != 0x44 || dut.aregs_sar1_hi != 0x55 || dut.aregs_ppr1 != 0x66) {
		fail("P8 seam: Channel 1 DMA registers mismatch");
	}
	if (dut.aregs_sar2_lo != 0x77 || dut.aregs_sar2_hi != 0x88 || dut.aregs_ppr2 != 0x99) {
		fail("P8 seam: Channel 2 DMA registers mismatch");
	}
	if ((dut.aregs_dcsr & 0x87) != 0x87) {
		fail("P8 seam: DCSR mismatch (expected bit7=1, bits2:0=7)");
	}

	// Palette entries:
	dut.aregs_pal_raddr = 0; tick();
	if (dut.aregs_pal_rdata != 0x924) {
		fail("P8 seam: Palette entry 0 mismatch (expected 0x924, got " + std::to_string(dut.aregs_pal_rdata) + ")");
	}
	dut.aregs_pal_raddr = 1; tick();
	if (dut.aregs_pal_rdata != 0xC56) {
		fail("P8 seam: Palette entry 1 mismatch (expected 0xC56, got " + std::to_string(dut.aregs_pal_rdata) + ")");
	}
	dut.aregs_pal_raddr = 16; tick();
	if (dut.aregs_pal_rdata != 0xE78) {
		fail("P8 seam: Palette entry 16 mismatch (expected 0xE78, got " + std::to_string(dut.aregs_pal_rdata) + ")");
	}

	// Sprite RAM bytes read via CPU memory port (&4000-&4003):
	uint8_t s0 = aregs_read(0x0000);
	uint8_t s1 = aregs_read(0x0001);
	uint8_t s2 = aregs_read(0x0002);
	uint8_t s3 = aregs_read(0x0003);
	if (s0 != 0x01 || s1 != 0x02 || s2 != 0x03 || s3 != 0x04) {
		fail("P8 seam: Sprite RAM readback mismatch (s0=" + std::to_string(s0) + " s1=" + std::to_string(s1) + ")");
	}

	// 14. Subsequent Ordinary Machine Reset:
	dut.seam_machine_reset = 1;
	dut.seam_plus_asic_reset = 1;
	tick();
	dut.seam_machine_reset = 0;
	dut.seam_plus_asic_reset = 0;
	tick();

	// Verify ordinary reset restored default values
	if (dut.aregs_pri != 0 || dut.aregs_ivr != 1 || dut.mmu_asic_page_on != 0 || dut.mmu_asic_unlocked != 0) {
		fail("P8 seam: Ordinary machine reset did not reset asic_regs/MMU state");
	}
	if (aregs_read(0x2000) != 0 || aregs_read(0x2001) != 0 || aregs_read(0x2002) != 0) {
		fail("P8 seam: Ordinary machine reset did not clear sprite position registers");
	}
	dut.aregs_pal_raddr = 17; tick(); // Sprite 0 Ink 1 (unmapped from legacy GA)
	if (dut.aregs_pal_rdata != 0x000) {
		fail("P8 seam: Ordinary machine reset did not clear sprite palette");
	}
	dut.aregs_pal_raddr = 0; tick(); // Pen 0 translated from default leg_inkr
	if (dut.aregs_pal_rdata != 0x666) {
		fail("P8 seam: Ordinary machine reset did not restore default legacy translation for pen 0");
	}

	std::printf("PASS p8_04: production parser / asic_regs / MMU lifecycle and reset seam\n");
}

} // namespace

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	try {
		Vplus_p8_test_top dut;
		test_p8_i8255_plus_quirks(dut);
		test_p8_sna_parser(dut);
		test_p8_sna_fifo_headroom(dut);
		test_p8_model_decodes(dut);
		test_p10c_fdc_motor_tape_gating(dut);
		test_p8_sna_integration_seam(dut);
		std::printf("All Phase P8 platform polish and P10 compatibility tests PASSED.\n");
		return 0;
	} catch (const std::exception& e) {
		std::fprintf(stderr, "FAIL: %s\n", e.what());
		return 1;
	}
}
