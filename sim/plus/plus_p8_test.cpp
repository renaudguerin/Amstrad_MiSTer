// Phase P8 unit test suite: Plus platform polish & SNA v3 CPC+ chunk parser.
//
// Tests:
// 1. i8255 Plus PPI quirks (Port B input-only, Port C output-only, control word rewrite latch preservation).
// 2. plus_sna_parser CPC+ chunk unpacking (sprite RAM nibbles, sprite attributes, palette, control regs, DMA, lock).
// 3. plus_fdc_decode model gating, and the parser/asic_regs/MMU reset seam.
// 4. B8-5 snapshot owner apply, run through the shared production
//    plus_sna_apply controller and the production plus_sna_header decoder fed
//    a real header byte stream:
//      a1-a8  DMA live SAR/pause/loop restore, the HSYNC same-edge race, MMU
//             ROM gates and unlock sequence state, controller sequencing,
//             repeated CPC+/plain/CPC+ isolation.
//      b1-b4  selected video register file and v3 counters, the Gate Array
//             VSYNC-delay phase (B2), interrupt counter and pending flag
//             (B3/B4), and palette provenance.
//    All of these are required passes; the temporary XFAIL omission vector
//    they replaced is gone.

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>
#include <stdexcept>

#include "Vplus_p8_test_top.h"
#include "verilated.h"

namespace {

void fail(const std::string& msg) {
	throw std::runtime_error(msg);
}

std::string hex_str(uint32_t val, int width) {
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%0*X", width, val);
	return std::string(buf);
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

	// D4: Plus control readback is the documented bit-4 pattern, not the
	// stored control word. Check both documented 80-9F examples and the
	// wider 80-FF decode so an implementation cannot special-case only the
	// common reset/control words.
	// Source: Extra CPC Plus Hardware Information, PPI / PPI Control port.
	const uint8_t plus_control_words[] = {
		0x80, 0x82, 0x8F, 0x90, 0x9B, 0x9F, 0xA2, 0xBF,
	};
	for (uint8_t control : plus_control_words) {
		wr(3, control);
		const uint8_t expected = (control & 0x10) ? 0xFF : 0x00;
		if (rd(3) != expected) {
			fail("P8 i8255: Plus control readback for 0x" + hex_str(control, 2) +
			     " was 0x" + hex_str(rd(3), 2) + " (expected 0x" +
			     hex_str(expected, 2) + ")");
		}
	}
	// Leave the Plus fixture in the normal all-output mode before switching
	// the capability input to the classic path below.
	wr(3, 0x80);

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
// Test 3: P10c FDC / Motor / Tape Model Gating & Aliases
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
// Test 4: Integrated SNA parser + motherboard / asic_regs / MMU reset seam
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
		// Sprite storage is a synchronous M10K-compatible host read. Sample
		// after its one 64 MHz edge without adding WAIT or a second bus cycle.
		if ((addr & 0x3000) == 0x0000)
			tick();
		else
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
	dut.pal_probe_sel = 1;   // this test probes the palette storage directly
	dut.ga_iorq_n = 1;       // no legacy I/O traffic here
	dut.ga_m1_n = 1;
	dut.ga_A = 0;
	dut.ga_D = 0;
	dut.hdr_wr = 0;
	dut.hdr_addr = 0;
	dut.hdr_data = 0;
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

// The temporary B8-5 omission vector that used to live here is gone: every gap
// it reproduced is now a required-pass assertion in the slice A/B focused
// tests below, made through the shared production apply controller and a real
// header byte stream instead of a hand-driven seam.

// -----------------------------------------------------------------------------
// B8-5 slice A: DMA/MMU/apply focused tests (required-pass, --b8-dma-mmu).
//
// Contract under test (docs/plus/b8-5-snapshot-apply-2026-09-08.md, SNA format
// docs/references/Snapshot (.SNA) file format.md):
// - Parser captures CPC+ DMA internal bytes 0x8E0-0x8F4 (three 7-byte
//   records: LE16 loop count lower12, LE16 loop address, LE16 pause count
//   lower12, prescaler8) and the 0x8F7 unlock sequence state; shadows clear
//   on every new download and survive drain/apply.
// - One apply reloads each live DMA SAR from the settled register-file
//   outputs (never replayed CPU SAR writes), restores loop/pause/prescaler,
//   resets pending FSM/instruction/RAM/LOAD/PSG transactions, and seeds the
//   HSYNC edge history from the settled header payload (v3 B0 bit1; 0 else).
// - plus_mmu applies header RMR bits 2/3 (ordinary ROM disables) and the
//   full header romsel byte alongside RMR2/unlock; 0x8F7 restores the
//   detector (0 waits nonzero, 1 waits zero, 2..E wait FF..8A, F waits CD,
//   10h waits terminal EE without replaying CD).
// - The shared plus_sna_apply controller sequences drain/apply; sna_hold
//   blocks CPU execution through the load pulse (motherboard gates both T80
//   CENs; T80 RESET priority / DIRSet-independence verified in source:
//   rtl/T80/T80pa.vhd CEN_pol machine, T80.vhd/T80_Reg.vhd DIRSet-before-
//   ClkEn branches, T80se.vhd async RESET_n).
// -----------------------------------------------------------------------------

// SNA header payload for one restore. Streamed byte by byte into the
// production plus_sna_header decoder, at the real file offsets, so a
// mis-mapped offset fails here instead of only in hardware.
struct B8Header {
	uint8_t version   = 3;
	uint8_t inksel    = 0;      // 2E
	uint8_t palette[17] = {0};  // 2F-3F: pens 0-15 then border, HW colours
	uint8_t ga_config = 0x80;   // 40:  RMR (bit7 set for CPCEMU, ROM enables
	                            //      in bits 3:2, mode in bits 1:0)
	uint8_t crtc_sel  = 0;      // 42
	uint8_t crtc[18]  = {0};    // 43-54: R0..R17
	uint8_t romsel    = 0;      // 55
	uint8_t hcc       = 0;      // A9
	uint8_t line      = 0;      // AB
	uint8_t raster    = 0;      // AC
	uint8_t vta       = 0;      // AD
	uint8_t hsw       = 0;      // AE
	uint8_t vsw       = 0;      // AF
	uint8_t b0        = 0;      // B0: bit0 VSYNC, bit1 HSYNC, bit7 adjust
	uint8_t vsdelay   = 0;      // B2
	uint8_t intcnt    = 0;      // B3
	uint8_t intreq    = 0;      // B4
};

// A CRTC programming that produces short, countable lines and a VSYNC long
// enough to hold still across the phase checks below. R0=15 -> 16-character
// lines, R2=8 with R3l=2 -> a two-character HSYNC each line, R3h=8 -> an
// eight-line VSYNC, R7=6 keeps the next natural VSYNC far away.
void b8_set_short_frame_crtc(B8Header& h) {
	h.crtc[0]  = 15;    // R0 horizontal total
	h.crtc[1]  = 8;     // R1 horizontal displayed
	h.crtc[2]  = 8;     // R2 HSYNC position
	h.crtc[3]  = 0x82;  // R3: v width 8, h width 2
	h.crtc[4]  = 7;     // R4 vertical total
	h.crtc[5]  = 0;     // R5 vertical total adjust
	h.crtc[6]  = 4;     // R6 vertical displayed
	h.crtc[7]  = 6;     // R7 VSYNC position
	h.crtc[8]  = 0;     // R8
	h.crtc[9]  = 1;     // R9 max raster
}

// Legacy [KT] hardware colour -> asic_regs palette word {G,R,B}. Mirrors
// legacy_colour_gbr in rtl/plus/asic_regs.v; kept as an independent copy so a
// silent edit to that table shows up as a test failure.
uint16_t hw_colour_gbr(int hw) {
	static const uint16_t tab[32] = {
		0x666, 0x666, 0xF06, 0xFF6, 0x006, 0x0F6, 0x606, 0x6F6,
		0x0F6, 0xFF6, 0xFF0, 0xFFF, 0x0F0, 0xFF0, 0x6F0, 0x6FF,
		0x006, 0xF06, 0xF00, 0xF0F, 0x000, 0x00F, 0x600, 0x60F,
		0x066, 0xF66, 0xF60, 0xF6F, 0x060, 0x06F, 0x660, 0x66F,
	};
	return tab[hw & 31];
}

// Owner state captured on the clock that consumes the apply pulse. The CRTC
// counters advance on the 1 us character enable, so reading them "shortly
// after" the apply is not deterministic; every counter assertion below uses
// this sample instead.
struct B8ApplySample {
	uint8_t  hcc = 0;
	uint8_t  line = 0;
	uint8_t  row = 0;
	uint8_t  adj = 0;
	uint8_t  hs = 0;
	uint8_t  vs = 0;
	uint16_t ma = 0;
	uint8_t  ga_mode = 0;
	uint8_t  ga_border = 0;
	uint8_t  ga_vsync_o = 0;
	uint8_t  ga_int_n = 0;
	uint8_t  int_n_merged = 0;
	bool     valid = false;
};

void b8_capture(Vplus_p8_test_top& dut, B8ApplySample* s) {
	if (!s) return;
	s->hcc          = dut.video_hcc;
	s->line         = dut.video_line_out;
	s->row          = dut.video_row_out;
	s->adj          = dut.video_adj_out;
	s->hs           = dut.video_hs;
	s->vs           = dut.video_vs;
	s->ma           = dut.video_ma;
	s->ga_mode      = dut.ga_mode_out;
	s->ga_border    = dut.ga_border_out;
	s->ga_vsync_o   = dut.ga_vsync_o;
	s->ga_int_n     = dut.ga_int_n_out;
	s->int_n_merged = dut.int_n_merged;
	s->valid        = true;
}

// Snapshot DMA/loop/pause parameters for one CPC+ chunk.
struct B8ChunkParams {
	uint8_t sar_lo[3] = {0x11, 0x44, 0x77};
	uint8_t sar_hi[3] = {0x22, 0x55, 0x88};
	uint8_t ppr[3] = {0x10, 0x20, 0x30};
	uint8_t dcsr = 0x87;
	uint16_t loop_cnt[3] = {0, 0, 0};
	uint16_t loop_addr[3] = {0, 0, 0};
	uint16_t pause_cnt[3] = {0, 0, 0};
	uint8_t pause_presc[3] = {0, 0, 0};
	uint8_t rmr2 = 0x19;
	uint8_t unlock = 1;
	uint8_t seq_state = 0;
	// Chunk 0x880-0x8BF: 32 x 12-bit palette entries, low byte {R,B} then
	// high byte {-,G} (asic_regs stores the word as {G,R,B}).
	uint8_t pal_lo[32] = {0};
	uint8_t pal_hi[32] = {0};
};

void b8_ctl_quiesce(Vplus_p8_test_top& dut) {
	dut.reset = 0;
	dut.seam_machine_reset = 0;
	dut.seam_plus_asic_reset = 0;
	dut.seam_plus_mode = 1;
	dut.seam_sna_load = 0;
	dut.seam_use_ctl = 1;
	dut.sna_download = 0;
	dut.cpc_plus_chunk_start = 0;
	dut.cpc_plus_byte_wr = 0;
	dut.cpc_plus_byte_data = 0;
	dut.aregs_cs = 0;
	dut.aregs_mem_rd = 0;
	dut.aregs_mem_wr = 0;
	dut.aregs_addr = 0;
	dut.aregs_din = 0;
	dut.aregs_pal_raddr = 0;
	dut.dma_test_hsync = 0;
	dut.dma_ram_data = 0;
	dut.video_crtc_cs = 0;
	dut.video_crtc_rd = 0;
	dut.video_crtc_rs = 0;
	dut.video_crtc_din = 0;
	dut.mmu_test_A = 0;
	dut.mmu_test_mem_rd = 0;
	dut.mmu_test_io_wr = 0;
	dut.mmu_test_D = 0;
	dut.hdr_wr = 0;
	dut.hdr_addr = 0;
	dut.hdr_data = 0;
	dut.ga_iorq_n = 1;
	dut.ga_m1_n = 1;
	dut.ga_A = 0;
	dut.ga_D = 0;
	dut.pal_probe_sel = 1;
	dut.eval();
}

// Stream one SNA header into the production decoder, at real file offsets.
void b8_stream_header(Vplus_p8_test_top& dut,
                      const std::function<void()>& tick,
                      const B8Header& h) {
	auto put = [&](uint8_t addr, uint8_t data) {
		dut.hdr_wr = 1;
		dut.hdr_addr = addr;
		dut.hdr_data = data;
		tick();
		dut.hdr_wr = 0;
	};
	put(0x10, h.version);
	put(0x2e, h.inksel);
	for (int i = 0; i < 17; ++i) put(0x2f + i, h.palette[i]);
	put(0x40, h.ga_config);
	put(0x42, h.crtc_sel);
	for (int i = 0; i < 18; ++i) put(0x43 + i, h.crtc[i]);
	put(0x55, h.romsel);
	put(0xa9, h.hcc);
	put(0xab, h.line);
	put(0xac, h.raster);
	put(0xad, h.vta);
	put(0xae, h.hsw);
	put(0xaf, h.vsw);
	put(0xb0, h.b0);
	put(0xb2, h.vsdelay);
	put(0xb3, h.intcnt);
	put(0xb4, h.intreq);
	tick();
}

// One 5-bit ink entry out of the 80-bit INKR_O bus (entry k at [k*5 +: 5]).
uint32_t ga_ink_entry(Vplus_p8_test_top& dut, int k) {
	int bit = k * 5;
	int w = bit / 32;
	int off = bit % 32;
	uint64_t v = (uint64_t)dut.ga_inkr_out[w];
	if (w < 2) v |= (uint64_t)dut.ga_inkr_out[w + 1] << 32;
	return (uint32_t)((v >> off) & 0x1F);
}

// Read one CRTC register through the production mod-8 readback map
// (asic_video §21.2.3: slots {R16,R17,STATUS1,STATUS2,R12,R13,R14,R15}).
uint8_t b8_crtc_read(Vplus_p8_test_top& dut,
                     const std::function<void()>& tick, uint8_t sel) {
	dut.video_crtc_cs = 1;
	dut.video_crtc_rs = 0;
	dut.video_crtc_rd = 0;
	dut.video_crtc_din = sel;
	tick();
	dut.video_crtc_rs = 1;
	dut.video_crtc_rd = 1;
	dut.eval();
	uint8_t v = dut.video_crtc_dout;
	dut.video_crtc_cs = 0;
	dut.video_crtc_rd = 0;
	dut.video_crtc_rs = 0;
	dut.eval();
	return v;
}

// Read one palette entry through the video-side port (probe mux).
uint16_t b8_pal_read(Vplus_p8_test_top& dut,
                     const std::function<void()>& tick, int entry) {
	dut.pal_probe_sel = 1;
	dut.aregs_pal_raddr = entry;
	tick(); tick();
	return (uint16_t)dut.aregs_pal_rdata;
}

// Hold a legacy Gate Array I/O write (&7Fxx: A15=0, A14=1) long enough to
// cross at least one register-latch window of the sequencer ring, then
// release. Repeats within the hold are idempotent for PENR/INKR.
void b8_ga_io_write(Vplus_p8_test_top& dut,
                    const std::function<void()>& tick, uint8_t d) {
	dut.ga_A = 0x1;      // A[15:14] = 01
	dut.ga_D = d;
	dut.ga_iorq_n = 0;
	dut.ga_m1_n = 1;
	for (int i = 0; i < 40; ++i) tick();
	dut.ga_iorq_n = 1;
	dut.ga_D = 0;
	dut.ga_A = 0;
	tick(); tick();
}

void b8_global_reset(Vplus_p8_test_top& dut,
                     const std::function<void()>& tick) {
	b8_ctl_quiesce(dut);
	dut.reset = 1;
	tick(); tick();
	dut.reset = 0;
	tick();
}

// Fixture precondition: on the controller path the DMA HSYNC source is the
// clean testbench line (see plus_p8_test_top), so DMA edge counting is
// deterministic. Assert the line itself is quiet before use.
void b8_require_video_hs_static(Vplus_p8_test_top& dut,
                                const std::function<void()>& tick) {
	if (dut.dma_test_hsync)
		fail("B8-5 harness: dma_test_hsync started high");
	for (int i = 0; i < 40; ++i) {
		if (dut.dma_ram_req)
			fail("B8-5 harness: dma_ram_req asserted before any HSYNC stimulus");
		tick();
	}
}

// Stream one full CPC+ chunk (0x8F8 payload bytes) under the shared
// controller path, mimicking the production plus_asic_reset edge on the
// download rise. Returns after the controller settles; fails unless exactly
// one apply pulse fired.
int b8_stream_chunk_apply(Vplus_p8_test_top& dut,
                          const std::function<void()>& tick,
                          const B8ChunkParams& p,
                          const B8Header& hdr = B8Header(),
                          B8ApplySample* sample = nullptr) {
	auto stream_byte = [&](uint8_t b) {
		while (dut.sna_ioctl_wait) tick();
		dut.cpc_plus_byte_wr = 1;
		dut.cpc_plus_byte_data = b;
		tick();
		dut.cpc_plus_byte_wr = 0;
	};

	dut.sna_download = 1;
	dut.seam_plus_asic_reset = 1; // production plus_asic_reset edge
	tick();
	dut.seam_plus_asic_reset = 0;
	tick();
	// Header bytes precede the memory dump and the chunks in the file.
	b8_stream_header(dut, tick, hdr);
	dut.cpc_plus_chunk_start = 1;
	tick();
	dut.cpc_plus_chunk_start = 0;

	for (int i = 0; i < 2048; ++i) stream_byte(0x00); // sprite bitmaps
	for (int i = 0; i < 128; ++i) stream_byte(0x00);  // sprite attributes
	for (int i = 0; i < 32; ++i) {                   // palette
		stream_byte(p.pal_lo[i]);
		stream_byte(p.pal_hi[i]);
	}
	stream_byte(0x00); stream_byte(0x00); stream_byte(0x00); // PRI/SPLT/SSA
	stream_byte(0x00); stream_byte(0x00); stream_byte(0x00); // SSA/SSCR/IVR
	for (int i = 0; i < 10; ++i) stream_byte(0x00);   // 0x8C6-0x8CF
	for (int c = 0; c < 3; ++c) {                    // 0x8D0-0x8DB SAR/PPR
		stream_byte(p.sar_lo[c]);
		stream_byte(p.sar_hi[c]);
		stream_byte(p.ppr[c]);
		stream_byte(0x00);
	}
	stream_byte(0x00); stream_byte(0x00); stream_byte(0x00); // 0x8DC-0x8DE
	stream_byte(p.dcsr);                             // 0x8DF DCSR
	for (int c = 0; c < 3; ++c) {                    // 0x8E0-0x8F4 internals
		stream_byte(p.loop_cnt[c] & 0xFF);
		stream_byte((p.loop_cnt[c] >> 8) & 0xFF);
		stream_byte(p.loop_addr[c] & 0xFF);
		stream_byte((p.loop_addr[c] >> 8) & 0xFF);
		stream_byte(p.pause_cnt[c] & 0xFF);
		stream_byte((p.pause_cnt[c] >> 8) & 0xFF);
		stream_byte(p.pause_presc[c]);
	}
	stream_byte(p.rmr2);      // 0x8F5 RMR2
	stream_byte(p.unlock);    // 0x8F6 lock
	stream_byte(p.seq_state); // 0x8F7 unlock sequence state

	dut.sna_download = 0;

	int loads = 0;
	int last_load = 0;
	bool consume_next = false;
	for (int i = 0; i < 4000; ++i) {
		tick();
		int cur = dut.ctl_sna_load ? 1 : 0;
		if (consume_next) {
			// This edge is the one the owners latched the pulse on.
			b8_capture(dut, sample);
			consume_next = false;
		}
		if (cur && !last_load) { ++loads; consume_next = true; }
		last_load = cur;
		if (!dut.sna_busy && !dut.ctl_finish_pending && dut.ctl_apply_cnt == 0 &&
		    !consume_next && i > 10)
			break;
	}
	if (loads != 1)
		fail("B8-5 apply: expected exactly one sna_load pulse, saw " + std::to_string(loads));
	if (sample && !sample->valid)
		fail("B8-5 apply: owner state was never sampled at the apply edge");
	tick(); tick();
	return loads;
}

// Plain SNA: download with no CPC+ chunk. Parser shadows must clear; the
// controller still produces exactly one apply.
void b8_stream_plain_apply(Vplus_p8_test_top& dut,
                           const std::function<void()>& tick,
                           const B8Header& hdr = B8Header(),
                           B8ApplySample* sample = nullptr) {
	dut.sna_download = 1;
	dut.seam_plus_asic_reset = 1;
	tick();
	dut.seam_plus_asic_reset = 0;
	b8_stream_header(dut, tick, hdr);
	for (int i = 0; i < 3; ++i) tick();
	dut.sna_download = 0;
	int loads = 0;
	int last = 0;
	bool consume_next = false;
	for (int i = 0; i < 1000; ++i) {
		tick();
		int cur = dut.ctl_sna_load ? 1 : 0;
		if (consume_next) {
			b8_capture(dut, sample);
			consume_next = false;
		}
		if (cur && !last) { ++loads; consume_next = true; }
		last = cur;
		if (!dut.sna_busy && !dut.ctl_finish_pending && dut.ctl_apply_cnt == 0 &&
		    !consume_next && i > 10)
			break;
	}
	if (loads != 1)
		fail("B8-5 apply: plain SNA produced " + std::to_string(loads) + " loads, expected 1");
	if (sample && !sample->valid)
		fail("B8-5 apply: owner state was never sampled at the plain apply edge");
	tick(); tick();
}

void b8_unlock_write(Vplus_p8_test_top& dut,
                     const std::function<void()>& tick, uint8_t b) {
	dut.mmu_test_A = 0xBC00; // nCS=A14=0, R_nW=A9=0, RS=A8=0; A15=1 dodges RMR2
	dut.mmu_test_D = b;
	dut.mmu_test_io_wr = 1;
	tick();
	dut.mmu_test_io_wr = 0;
	tick(); tick();
}

void b8_expect_idle_dma(Vplus_p8_test_top& dut,
                        const std::function<void()>& tick,
                        const std::string& where) {
	for (int i = 0; i < 30; ++i) {
		tick();
		if (dut.dma_ram_req)
			fail("B8-5 " + where + ": stale dma_ram_req asserted while idle");
	}
	if (dut.dma_load_owner_o)
		fail("B8-5 " + where + ": stale dma_load_owner held while idle");
	if (dut.dma_psg_active_o)
		fail("B8-5 " + where + ": stale PSG activity while idle");
}

// Pulse the live HSYNC line once, then sample one line window for RAM
// requests at the actual DMA consume cadence: ST_FETCH0/1/2 latch ram_data
// at the pre-edge cclk_en_p && ram_req with no ACK, and ram_req can stay
// high across channels (including the same address), so rising edges see
// only one fetch. Sampling ram_addr on the pre-edge enable records each
// channel exactly once, in order, even for duplicate addresses.
std::vector<uint16_t> b8_hsync_window(Vplus_p8_test_top& dut,
                                      const std::function<void()>& tick,
                                      uint16_t ram_data, int settle = 60) {
	std::vector<uint16_t> addrs;
	dut.dma_test_hsync = 1;
	tick();
	dut.dma_test_hsync = 0;
	for (int i = 0; i < settle; ++i) {
		dut.dma_ram_data = ram_data;
		dut.eval();
		if (dut.dma_cclk_en_p && dut.dma_ram_req)
			addrs.push_back(dut.dma_ram_addr);
		tick();
	}
	return addrs;
}

// --- Test 1: live SAR restore on all three channels + first fetch addresses.
void test_b8_dma_sar_restore(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };
	b8_global_reset(dut, tick);
	b8_require_video_hs_static(dut, tick);

	B8ChunkParams p;
	p.dcsr = 0x87; // all three channels enabled
	b8_stream_chunk_apply(dut, tick, p);

	// Parser shadows captured before apply and survive it.
	if (dut.sna_loop_cnt0 != 0 || dut.sna_pause_cnt0 != 0 || dut.sna_seq_state != 0)
		fail("B8-5 DMA: zero internal shadows mis-captured");

	if (dut.dma_sar0_addr != 0x2211)
		fail("B8-5 DMA: live SAR0 is 0x" + hex_str(dut.dma_sar0_addr, 4) + ", expected 0x2211 from settled asic_regs");
	if (dut.dma_sar1_addr != 0x5544)
		fail("B8-5 DMA: live SAR1 is 0x" + hex_str(dut.dma_sar1_addr, 4) + ", expected 0x5544");
	if (dut.dma_sar2_addr != 0x8877)
		fail("B8-5 DMA: live SAR2 is 0x" + hex_str(dut.dma_sar2_addr, 4) + ", expected 0x8877");

	// First genuine HSYNC services all three channels in order from the
	// restored addresses (word-aligned fetch addresses).
	auto addrs = b8_hsync_window(dut, tick, 0x4020 /* STOP */, 500);
	if (addrs.size() < 3)
		fail("B8-5 DMA: only " + std::to_string(addrs.size()) + " channel fetches serviced, expected 3");
	if (addrs[0] != 0x2210 || addrs[1] != 0x5544 || addrs[2] != 0x8876)
		fail("B8-5 DMA: first-fetch addresses 0x" + hex_str(addrs[0], 4) +
		     "/0x" + hex_str(addrs[1], 4) + "/0x" + hex_str(addrs[2], 4) +
		     ", expected 0x2210/0x5544/0x8876");

	std::printf("PASS b8_5_a1: DMA live SAR restore on all channels + ordered first fetch\n");
}

// --- Test 2: pause/prescaler first transitions from restored fields.
void test_b8_dma_pause_prescaler(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };
	b8_global_reset(dut, tick);
	b8_require_video_hs_static(dut, tick);

	// Case PPR-reload: pause=2, prescaler phase 0, PPR=1. HSYNC1 reloads the
	// phase and counts 2->1; HSYNC2 ticks the phase; HSYNC3 reloads and
	// counts 1->0 (still gated on the old count); HSYNC4 fetches.
	B8ChunkParams p;
	p.sar_lo[0] = 0x00; p.sar_hi[0] = 0x40; p.ppr[0] = 0x01;
	p.pause_cnt[0] = 2; p.pause_presc[0] = 0;
	p.dcsr = 0x81; // ch0 only
	b8_stream_chunk_apply(dut, tick, p);
	if (dut.sna_pause_cnt0 != 2 || dut.sna_pause_presc0 != 0)
		fail("B8-5 DMA: pause shadow is " + std::to_string(dut.sna_pause_cnt0) +
		     "/" + std::to_string(dut.sna_pause_presc0) + ", expected 2/0");
	for (int h = 1; h <= 3; ++h) {
		auto addrs = b8_hsync_window(dut, tick, 0x4020);
		if (!addrs.empty())
			fail("B8-5 DMA: pause leaked a fetch on HSYNC" + std::to_string(h));
	}
	auto addrs = b8_hsync_window(dut, tick, 0x4020, 500);
	if (addrs.size() != 1 || addrs[0] != 0x4000)
		fail("B8-5 DMA: pause did not expire exactly on HSYNC4");

	// Case phase-decrement: pause=1, prescaler=3, PPR=5. Three HSYNCs tick
	// the phase, the fourth reloads and retires the pause (gated), the
	// fifth fetches.
	b8_global_reset(dut, tick);
	B8ChunkParams q;
	q.sar_lo[0] = 0x00; q.sar_hi[0] = 0x40; q.ppr[0] = 0x05;
	q.pause_cnt[0] = 1; q.pause_presc[0] = 3;
	q.dcsr = 0x81;
	b8_stream_chunk_apply(dut, tick, q);
	if (dut.sna_pause_cnt0 != 1 || dut.sna_pause_presc0 != 3)
		fail("B8-5 DMA: pause shadow is " + std::to_string(dut.sna_pause_cnt0) +
		     "/" + std::to_string(dut.sna_pause_presc0) + ", expected 1/3");
	for (int h = 1; h <= 4; ++h) {
		auto w = b8_hsync_window(dut, tick, 0x4020);
		if (!w.empty())
			fail("B8-5 DMA: pause leaked a fetch on HSYNC" + std::to_string(h) + " (phase case)");
	}
	auto w = b8_hsync_window(dut, tick, 0x4020, 500);
	if (w.size() != 1 || w[0] != 0x4000)
		fail("B8-5 DMA: pause did not expire exactly on HSYNC5 (phase case)");

	std::printf("PASS b8_5_a2: DMA pause/prescaler first transitions (reload + phase)\n");
}

// --- Test 3: LOOP returns to the restored address/count.
void test_b8_dma_loop_restore(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };
	b8_global_reset(dut, tick);
	b8_require_video_hs_static(dut, tick);

	B8ChunkParams p;
	p.sar_lo[0] = 0x00; p.sar_hi[0] = 0x40; // SAR0 = 0x4000
	p.loop_cnt[0] = 5; p.loop_addr[0] = 0x1000;
	p.dcsr = 0x81;
	b8_stream_chunk_apply(dut, tick, p);
	if (dut.sna_loop_cnt0 != 5 || dut.sna_loop_addr0 != 0x1000)
		fail("B8-5 DMA: loop shadow is " + std::to_string(dut.sna_loop_cnt0) +
		     "/0x" + hex_str(dut.sna_loop_addr0, 4) + ", expected 5/0x1000");
	if (dut.dma_sar0_addr != 0x4000)
		fail("B8-5 DMA: SAR0 is 0x" + hex_str(dut.dma_sar0_addr, 4) + ", expected 0x4000");

	// Fetch a LOOP control word; execution must return SAR0 to the
	// restored loop address (loop 5->4 internally).
	auto addrs = b8_hsync_window(dut, tick, 0x4001 /* LOOP */, 500);
	if (addrs.empty() || addrs[0] != 0x4000)
		fail("B8-5 DMA: LOOP fetch did not start at 0x4000");
	if (dut.dma_sar0_addr != 0x1000)
		fail("B8-5 DMA: LOOP did not return to restored 0x1000 (SAR0=0x" +
		     hex_str(dut.dma_sar0_addr, 4) + ")");

	std::printf("PASS b8_5_a3: DMA LOOP returns to restored address/count\n");
}

// --- Test 4: same-edge HSYNC race — restoring active HS must not fake an edge.
void test_b8_dma_hsync_race(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };
	b8_global_reset(dut, tick);
	b8_require_video_hs_static(dut, tick);

	// Live line held active while the settled header payload also says
	// active HSYNC. The apply edge itself must not start a fetch.
	B8ChunkParams p;
	p.sar_lo[0] = 0x11; p.sar_hi[0] = 0x22;
	p.dcsr = 0x81; // ch0 enabled, no pause
	B8Header h;
	h.b0 = 0x02;            // settled v3 B0 bit 1 = HSYNC active
	dut.dma_test_hsync = 1; // live line active through download/drain/apply
	b8_stream_chunk_apply(dut, tick, p, h);
	for (int i = 0; i < 30; ++i) {
		dut.dma_ram_data = 0x4020;
		tick();
		if (dut.dma_ram_req)
			fail("B8-5 DMA race: apply edge with settled HSYNC=1 fabricated a fetch");
	}
	b8_expect_idle_dma(dut, tick, "race post-apply");

	// The next genuine rising edge triggers exactly the intended fetch.
	dut.dma_test_hsync = 0;
	tick(); tick();
	auto addrs = b8_hsync_window(dut, tick, 0x4020, 500);
	if (addrs.size() != 1 || addrs[0] != 0x2210)
		fail("B8-5 DMA race: genuine post-apply HSYNC fetched " +
		     std::to_string(addrs.size()) + " requests, expected exactly one at 0x2210");

	// Disabled channels stay silent across the same active-HS restore.
	b8_global_reset(dut, tick);
	B8ChunkParams q;
	q.dcsr = 0x00;
	dut.dma_test_hsync = 1;
	b8_stream_chunk_apply(dut, tick, q, h);
	b8_expect_idle_dma(dut, tick, "race disabled");
	dut.dma_test_hsync = 0;
	tick(); tick();
	auto w = b8_hsync_window(dut, tick, 0x4020);
	if (!w.empty())
		fail("B8-5 DMA race: disabled channel fetched after genuine HSYNC");

	std::printf("PASS b8_5_a4: HSYNC same-edge race (no false fetch, one genuine fetch)\n");
}

// --- Test 5: MMU ordinary ROM disables + full romsel byte.
void test_b8_mmu_rom_gates(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };
	b8_global_reset(dut, tick);

	auto cart_read = [&](uint16_t a) {
		dut.mmu_test_A = a;
		dut.mmu_test_mem_rd = 1;
		tick();
	};
	auto cart_release = [&]() {
		dut.mmu_test_mem_rd = 0;
		tick(); tick();
	};

	// Both ROMs disabled by header RMR bits (0x8D: bit2=1 lower, bit3=1
	// upper): base RAM shows through on both windows.
	B8ChunkParams p;
	p.dcsr = 0x00;
	B8Header h;
	h.ga_config = 0x8D; // bit2 lower ROM disabled, bit3 upper ROM disabled
	h.romsel = 0x85;
	b8_stream_chunk_apply(dut, tick, p, h);
	cart_read(0x0000);
	if (dut.mmu_cart_own != 0)
		fail("B8-5 MMU: lower window owned with header lower-ROM disabled");
	cart_release();
	cart_read(0xC000);
	if (dut.mmu_cart_own != 0)
		fail("B8-5 MMU: upper window owned with header upper-ROM disabled");
	// cart_page is meaningless while the window is disabled: assert nothing.
	cart_release();

	// Both enabled with a cartridge romsel >= 128: real request, ownership
	// and non-default page on the upper window; RMR2 page on the lower.
	b8_global_reset(dut, tick);
	B8ChunkParams q;
	q.dcsr = 0x00;
	q.rmr2 = 0x1A; // pos 00 (ASIC-page case), page 2
	B8Header hq;
	hq.ga_config = 0x80; // neither disable bit set
	hq.romsel = 0x85;    // cartridge page 5
	b8_stream_chunk_apply(dut, tick, q, hq);
	cart_read(0xC000);
	if (dut.mmu_cart_own != 1)
		fail("B8-5 MMU: upper window not owned with header upper-ROM enabled");
	if (dut.mmu_cart_page != 5)
		fail("B8-5 MMU: upper page is " + std::to_string(dut.mmu_cart_page) + ", expected 5 for romsel 0x85");
	if (dut.mmu_cart_offset != 0)
		fail("B8-5 MMU: upper offset is not 0 at 0xC000");
	cart_release();
	cart_read(0x0005);
	if (dut.mmu_cart_own != 1)
		fail("B8-5 MMU: lower window not owned with header lower-ROM enabled");
	if (dut.mmu_cart_page != 2)
		fail("B8-5 MMU: lower page is " + std::to_string(dut.mmu_cart_page) + ", expected 2 for RMR2 0x1A");
	cart_release();

	std::printf("PASS b8_5_a5: MMU header ROM disables + full romsel byte\n");
}

// --- Test 6: unlock sequence-state restore.
void test_b8_unlock_restore(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };
	const uint8_t suffix[] = {0x51, 0xA8, 0xD4, 0x62, 0x39, 0x9C,
	                          0x46, 0x2B, 0x15, 0x8A};
	const uint8_t full[] = {0xFF, 0x77, 0xB3, 0x51, 0xA8, 0xD4, 0x62,
	                        0x39, 0x9C, 0x46, 0x2B, 0x15, 0x8A};

	// Mid-sequence suffix: state 5 waits for 0x51; the remaining suffix +
	// CD must unlock without replaying the prefix.
	b8_global_reset(dut, tick);
	B8ChunkParams p;
	p.dcsr = 0x00; p.unlock = 0; p.seq_state = 5;
	b8_stream_chunk_apply(dut, tick, p);
	if (dut.sna_seq_state != 5)
		fail("B8-5 unlock: seq shadow is " + std::to_string(dut.sna_seq_state) + ", expected 5");
	if (dut.mmu_asic_unlocked != 0)
		fail("B8-5 unlock: restored unlocked=1 from 8F6=0");
	for (uint8_t b : suffix) b8_unlock_write(dut, tick, b);
	if (dut.mmu_asic_unlocked != 0)
		fail("B8-5 unlock: suffix unlocked before terminal CD");
	b8_unlock_write(dut, tick, 0xCD);
	if (dut.mmu_asic_unlocked != 1)
		fail("B8-5 unlock: mid-sequence suffix + CD did not unlock");

	// Wait-zero synchronization: state 1 needs a zero byte before the
	// sequence; a nonzero byte must not arm it.
	b8_global_reset(dut, tick);
	B8ChunkParams q;
	q.dcsr = 0x00; q.unlock = 0; q.seq_state = 1;
	b8_stream_chunk_apply(dut, tick, q);
	b8_unlock_write(dut, tick, 0x42);
	for (uint8_t b : full) b8_unlock_write(dut, tick, b);
	b8_unlock_write(dut, tick, 0xCD);
	if (dut.mmu_asic_unlocked != 0)
		fail("B8-5 unlock: nonzero byte wrongly armed wait-zero sync");
	b8_unlock_write(dut, tick, 0x00);
	for (uint8_t b : full) b8_unlock_write(dut, tick, b);
	b8_unlock_write(dut, tick, 0xCD);
	if (dut.mmu_asic_unlocked != 1)
		fail("B8-5 unlock: wait-zero sync + full sequence + CD did not unlock");

	// Terminal EE: state 0x10 keeps the 8F6 lock; EE changes nothing and a
	// fresh sync still works afterwards (no CD replay).
	b8_global_reset(dut, tick);
	B8ChunkParams r;
	r.dcsr = 0x00; r.unlock = 1; r.seq_state = 0x10;
	b8_stream_chunk_apply(dut, tick, r);
	if (dut.mmu_asic_unlocked != 1)
		fail("B8-5 unlock: EE restore lost the 8F6 lock");
	b8_unlock_write(dut, tick, 0xEE);
	if (dut.mmu_asic_unlocked != 1)
		fail("B8-5 unlock: terminal EE altered the settled lock");
	b8_unlock_write(dut, tick, 0x00);
	for (uint8_t b : full) b8_unlock_write(dut, tick, b);
	b8_unlock_write(dut, tick, 0xCD);
	if (dut.mmu_asic_unlocked != 1)
		fail("B8-5 unlock: detector dead after terminal EE");

	// Post-CD wait: state 0xF with lock 0; a non-CD byte locks (clears).
	b8_global_reset(dut, tick);
	B8ChunkParams s;
	s.dcsr = 0x00; s.unlock = 0; s.seq_state = 0x0F;
	b8_stream_chunk_apply(dut, tick, s);
	b8_unlock_write(dut, tick, 0x00);
	if (dut.mmu_asic_unlocked != 0)
		fail("B8-5 unlock: non-CD byte at wait-CD set unlocked");

	std::printf("PASS b8_5_a6: unlock sequence-state restore (suffix/zero/EE/CD)\n");
}

// --- Test 7: shared controller — stalls, delayed drain, single pulse,
// --- CPU hold through load, registered reset latency, abort.
void test_b8_apply_controller(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };
	b8_global_reset(dut, tick);

	// Delayed final-write drain: three sprite bytes (six FIFO writes) with
	// the download falling immediately. Busy must hold the barrier; the
	// controller must wait, then emit exactly one load after the drain.
	dut.sna_download = 1;
	dut.seam_plus_asic_reset = 1;
	tick();
	dut.seam_plus_asic_reset = 0;
	tick();
	dut.cpc_plus_chunk_start = 1;
	tick();
	dut.cpc_plus_chunk_start = 0;
	auto send_nowait = [&](uint8_t b) {
		dut.cpc_plus_byte_wr = 1;
		dut.cpc_plus_byte_data = b;
		tick();
		dut.cpc_plus_byte_wr = 0;
	};
	send_nowait(0x12); send_nowait(0x34); send_nowait(0x56);
	dut.sna_download = 0;

	struct Sample { int busy, fin, cnt, load, hold, rst; };
	std::vector<Sample> trace;
	for (int i = 0; i < 200; ++i) {
		tick();
		trace.push_back({(int)dut.sna_busy, (int)dut.ctl_finish_pending,
		                 (int)dut.ctl_apply_cnt, (int)dut.ctl_sna_load,
		                 (int)dut.ctl_sna_hold, (int)dut.ctl_owner_reset});
		if (!dut.sna_busy && !dut.ctl_finish_pending && dut.ctl_apply_cnt == 0 && i > 10)
			break;
	}
	int loads = 0;
	int load_at = -1;
	for (size_t i = 1; i < trace.size(); ++i) {
		if (trace[i].load && !trace[i-1].load) { ++loads; load_at = (int)i; }
		if (trace[i].load && trace[i].busy)
			fail("B8-5 controller: sna_load fired while the parser FIFO still drained");
	}
	if (loads != 1)
		fail("B8-5 controller: saw " + std::to_string(loads) + " loads after delayed drain, expected 1");
	bool saw_fin_while_busy = false;
	for (auto& s : trace) if (s.fin && s.busy) saw_fin_while_busy = true;
	if (!saw_fin_while_busy)
		fail("B8-5 controller: finish_pending never covered the drain window");
	// CPU hold spans download/drain/apply through the load pulse itself.
	for (int i = 0; i <= load_at; ++i)
		if (!trace[i].hold)
			fail("B8-5 controller: sna_hold dropped before the load pulse");
	if (trace.back().hold)
		fail("B8-5 controller: sna_hold stuck after apply settled");
	// Registered reset latency: reset still asserted the cycle before the
	// load, released on the load cycle (DIRSet never held through reset).
	if (load_at < 1)
		fail("B8-5 controller: no load cycle captured");
	if (trace[load_at].rst)
		fail("B8-5 controller: owner reset held through the DIRSet load cycle");
	if (!trace[load_at-1].rst)
		fail("B8-5 controller: owner reset released early (no registered latency)");

	// A new restore aborts a pending older apply: fall, then rise again
	// mid-drain. No load may fire while the new download is up, and the
	// second falling edge produces exactly one fresh load.
	b8_global_reset(dut, tick);
	dut.sna_download = 1;
	dut.seam_plus_asic_reset = 1;
	tick();
	dut.seam_plus_asic_reset = 0;
	tick();
	dut.cpc_plus_chunk_start = 1;
	tick();
	dut.cpc_plus_chunk_start = 0;
	send_nowait(0x12); send_nowait(0x34); send_nowait(0x56);
	dut.sna_download = 0;
	tick(); tick();
	dut.sna_download = 1; // abort: new restore while draining/counting
	dut.seam_plus_asic_reset = 1;
	tick();
	dut.seam_plus_asic_reset = 0;
	if (dut.ctl_finish_pending || dut.ctl_apply_cnt != 0)
		fail("B8-5 controller: new restore did not abort the pending apply");
	int stale_loads = 0;
	for (int i = 0; i < 30; ++i) {
		tick();
		if (dut.ctl_sna_load) ++stale_loads;
		if (dut.ctl_finish_pending || dut.ctl_apply_cnt != 0)
			fail("B8-5 controller: stale sequencing survived the abort");
	}
	if (stale_loads)
		fail("B8-5 controller: stale apply fired during the new download");
	dut.sna_download = 0;
	loads = 0;
	int last = 0;
	for (int i = 0; i < 200; ++i) {
		tick();
		int cur = dut.ctl_sna_load ? 1 : 0;
		if (cur && !last) ++loads;
		last = cur;
		if (!dut.sna_busy && !dut.ctl_finish_pending && dut.ctl_apply_cnt == 0 && i > 10)
			break;
	}
	if (loads != 1)
		fail("B8-5 controller: post-abort restore produced " + std::to_string(loads) + " loads");

	// Boundary: a new download rising exactly when apply_cnt==1. The NBA
	// clear is late, so without suppression ctl_sna_load is still 1 on
	// the consuming edge and the stale apply would fire into the new
	// image. The old sna_load/DIRSet/owner apply must be suppressed on
	// the raw new download while sna_hold stays high.
	b8_global_reset(dut, tick);
	dut.sna_download = 1;
	tick(); tick();
	dut.sna_download = 0;
	tick();
	int waited = 0;
	while (dut.ctl_apply_cnt != 1 && waited < 200) {
		tick();
		++waited;
	}
	if (dut.ctl_apply_cnt != 1)
		fail("B8-5 controller: setup never reached apply_cnt==1 for boundary abort");
	if (!dut.ctl_sna_load)
		fail("B8-5 controller: setup has cnt==1 without sna_load asserted");
	if (!dut.ctl_sna_hold)
		fail("B8-5 controller: sna_hold dropped before the boundary load");
	dut.sna_download = 1;
	dut.seam_plus_asic_reset = 1;
	dut.eval();
	if (dut.ctl_sna_load)
		fail("B8-5 controller: stale sna_load survived new download at apply (expected 0 before consuming edge)");
	if (!dut.ctl_sna_hold)
		fail("B8-5 controller: sna_hold dropped on the aborting download");
	tick();
	dut.seam_plus_asic_reset = 0;
	if (dut.ctl_finish_pending || dut.ctl_apply_cnt != 0)
		fail("B8-5 controller: new restore did not abort the pending apply at cnt==1");
	stale_loads = 0;
	for (int i = 0; i < 30; ++i) {
		tick();
		if (dut.ctl_sna_load) ++stale_loads;
		if (!dut.ctl_sna_hold)
			fail("B8-5 controller: sna_hold dropped during the aborting download");
		if (dut.ctl_finish_pending || dut.ctl_apply_cnt != 0)
			fail("B8-5 controller: stale sequencing survived the boundary abort");
	}
	if (stale_loads)
		fail("B8-5 controller: stale apply fired during the boundary download");
	dut.sna_download = 0;
	loads = 0;
	last = 0;
	for (int i = 0; i < 200; ++i) {
		tick();
		int cur = dut.ctl_sna_load ? 1 : 0;
		if (cur && !last) ++loads;
		last = cur;
		if (!dut.sna_busy && !dut.ctl_finish_pending && dut.ctl_apply_cnt == 0 && i > 10)
			break;
	}
	if (loads != 1)
		fail("B8-5 controller: post-boundary restore produced " + std::to_string(loads) + " loads");

	std::printf("PASS b8_5_a7: shared controller (drain/hold/latency/abort)\n");
}

// --- Test 8: repeated restore isolation, no stale pre-apply work.
void test_b8_restore_isolation(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };
	b8_global_reset(dut, tick);
	b8_require_video_hs_static(dut, tick);

	B8Header h;
	h.ga_config = 0x80;
	h.romsel = 0x85;
	// A CPC+ palette entry that no 5-bit hardware colour can express, so a
	// later plain restore cannot accidentally look correct.
	h.palette[0] = 26; h.palette[16] = 31;

	// Nonzero CPC+ restore.
	B8ChunkParams a;
	a.sar_lo[0] = 0x11; a.sar_hi[0] = 0x22;
	a.loop_cnt[0] = 5; a.loop_addr[0] = 0x1000;
	a.pause_cnt[1] = 7; a.pause_presc[1] = 2;
	a.dcsr = 0x87; a.rmr2 = 0x19; a.unlock = 1; a.seq_state = 5;
	a.pal_lo[0] = 0x24; a.pal_hi[0] = 0x09;   // entry 0 = 0x924
	a.pal_lo[16] = 0x78; a.pal_hi[16] = 0x0E; // entry 16 = 0xE78
	b8_stream_chunk_apply(dut, tick, a, h);
	if (dut.dma_sar0_addr != 0x2211 || dut.sna_loop_cnt0 != 5 ||
	    dut.sna_pause_cnt1 != 7 || dut.sna_seq_state != 5)
		fail("B8-5 isolation: first CPC+ restore did not land");
	if (!dut.mmu_asic_page_on || !dut.mmu_asic_unlocked)
		fail("B8-5 isolation: first CPC+ MMU/unlock did not land");
	if (b8_pal_read(dut, tick, 0) != 0x924 || b8_pal_read(dut, tick, 16) != 0xE78)
		fail("B8-5 isolation: CPC+ 12-bit palette was quantised by the apply");
	b8_expect_idle_dma(dut, tick, "isolation post-A");

	// Plain SNA: no chunk, but the ASIC-reset edge still clears storage, so
	// nothing of A may survive; header ROM controls still apply. Its 5-bit
	// header colours must now replace the retained 12-bit entries.
	b8_stream_plain_apply(dut, tick, h);
	if (b8_pal_read(dut, tick, 0) != hw_colour_gbr(26) ||
	    b8_pal_read(dut, tick, 16) != hw_colour_gbr(31))
		fail("B8-5 isolation: plain SNA did not translate the header colours");
	if (dut.dma_sar0_addr != 0 || dut.dma_sar1_addr != 0 || dut.dma_sar2_addr != 0)
		fail("B8-5 isolation: plain SNA inherited live SARs");
	if (dut.sna_loop_cnt0 != 0 || dut.sna_pause_cnt1 != 0 || dut.sna_seq_state != 0)
		fail("B8-5 isolation: plain SNA inherited CPC+ shadows");
	if (dut.mmu_asic_page_on || dut.mmu_asic_unlocked)
		fail("B8-5 isolation: plain SNA inherited RMR2/unlock");
	dut.mmu_test_A = 0xC000;
	dut.mmu_test_mem_rd = 1;
	tick();
	if (dut.mmu_cart_own != 1 || dut.mmu_cart_page != 5)
		fail("B8-5 isolation: plain SNA lost header romsel page 5");
	dut.mmu_test_mem_rd = 0;
	tick(); tick();
	b8_expect_idle_dma(dut, tick, "isolation post-plain");

	// Different CPC+ restore replaces everything, no stale requests.
	B8ChunkParams b;
	b.sar_lo[0] = 0x33; b.sar_hi[0] = 0x66;
	b.loop_cnt[0] = 9; b.loop_addr[0] = 0x2000;
	b.dcsr = 0x81; b.rmr2 = 0x1A; b.unlock = 1; b.seq_state = 0;
	b.pal_lo[0] = 0x35; b.pal_hi[0] = 0x0B;   // entry 0 = 0xB35
	b8_stream_chunk_apply(dut, tick, b, h);
	if (dut.dma_sar0_addr != 0x6633 || dut.sna_loop_cnt0 != 9 ||
	    dut.sna_loop_addr0 != 0x2000)
		fail("B8-5 isolation: second CPC+ restore did not replace the first");
	if (b8_pal_read(dut, tick, 0) != 0xB35)
		fail("B8-5 isolation: second CPC+ palette did not replace the plain translation");
	if (!dut.mmu_asic_page_on || !dut.mmu_asic_unlocked)
		fail("B8-5 isolation: second CPC+ MMU/unlock did not land");
	dut.mmu_test_A = 0x0005;
	dut.mmu_test_mem_rd = 1;
	tick();
	if (dut.mmu_cart_own != 1 || dut.mmu_cart_page != 2)
		fail("B8-5 isolation: second CPC+ lower page is not 2");
	dut.mmu_test_mem_rd = 0;
	tick(); tick();
	b8_expect_idle_dma(dut, tick, "isolation post-B");

	std::printf("PASS b8_5_a8: repeated restore isolation (CPC+/plain/CPC+)\n");
}

// -----------------------------------------------------------------------------
// B8-5 slice B: selected video / Gate Array / palette apply (required-pass).
//
// These replace the temporary XFAIL omission vector: every gap it printed is
// now an assertion here, made on owner-visible state (register readback,
// counter outputs, sync levels, INT_n, consumed palette RGB) rather than on
// the new restore ports themselves.
// -----------------------------------------------------------------------------

// Advance until the CRTC HSYNC output falls `n` times, or fail.
void b8_wait_hsync_falls(Vplus_p8_test_top& dut,
                         const std::function<void()>& tick,
                         int n, const std::string& where) {
	int seen = 0;
	int last = dut.video_hs;
	for (int i = 0; i < 20000 && seen < n; ++i) {
		tick();
		int cur = dut.video_hs;
		if (last && !cur) ++seen;
		last = cur;
	}
	if (seen < n)
		fail("B8-5 " + where + ": only " + std::to_string(seen) + " of " +
		     std::to_string(n) + " CRTC HSYNC falling edges arrived");
}

// --- Test 9: selected video register file, mapped v3 counters, sync state.
void test_b8_video_restore(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };
	b8_global_reset(dut, tick);

	B8Header h;
	b8_set_short_frame_crtc(h);
	h.crtc_sel   = 13;    // header 42: selected register index
	h.crtc[12]   = 0x31;  // R12: start address high (bits 7:6 are §20.5 extras)
	h.crtc[13]   = 0x84;  // R13: start address low
	h.crtc[14]   = 0x2A;  // R14 cursor high (6-bit)
	h.crtc[15]   = 0x5C;  // R15 cursor low
	h.crtc[16]   = 0x15;  // R16 light-pen high (6-bit)
	h.crtc[17]   = 0x73;  // R17 light-pen low
	h.crtc[10]   = 0x6F;  // R10/R11 are read-only status groups on type 3:
	h.crtc[11]   = 0x1F;  // present in the file, deliberately not stored
	h.hcc        = 5;     // A9
	h.line       = 3;     // AB
	h.raster     = 1;     // AC
	h.hsw        = 1;     // AE
	h.vsw        = 2;     // AF
	h.b0         = 0x03;  // VSYNC active + HSYNC active
	h.ga_config  = 0x81;  // RMR: mode 1, both ROMs enabled
	h.inksel     = 3;
	h.palette[0] = 26; h.palette[1] = 11; h.palette[16] = 4;

	B8ChunkParams p;
	p.dcsr = 0x00;
	B8ApplySample s;
	b8_stream_chunk_apply(dut, tick, p, h, &s);

	// Header 42, the selected register index: read WITHOUT writing a new
	// select, so the answering mod-8 slot is the restored one (13 -> slot 5,
	// which returns R13).
	dut.video_crtc_cs = 1;
	dut.video_crtc_rs = 1;
	dut.video_crtc_rd = 1;
	dut.eval();
	uint8_t selected = dut.video_crtc_dout;
	dut.video_crtc_cs = 0;
	dut.video_crtc_rd = 0;
	dut.video_crtc_rs = 0;
	dut.eval();
	if (selected != 0x84)
		fail("B8-5 video: the restored register index does not select R13 (read 0x" +
		     hex_str(selected, 2) + ", expected the R13 value 0x84)");

	// Register file, through the production mod-8 readback map.
	if (b8_crtc_read(dut, tick, 12) != 0x31)
		fail("B8-5 video: R12 readback is not the restored 0x31");
	if (b8_crtc_read(dut, tick, 13) != 0x84)
		fail("B8-5 video: R13 readback is not the restored 0x84");
	if (b8_crtc_read(dut, tick, 14) != 0x2A)
		fail("B8-5 video: R14 readback is not the restored 0x2A");
	if (b8_crtc_read(dut, tick, 15) != 0x5C)
		fail("B8-5 video: R15 readback is not the restored 0x5C");
	if (b8_crtc_read(dut, tick, 16) != 0x15)
		fail("B8-5 video: R16 light-pen latch was not seeded from the header");
	if (b8_crtc_read(dut, tick, 17) != 0x73)
		fail("B8-5 video: R17 light-pen latch was not seeded from the header");

	// Mapped v3 counters land on the owner's own outputs.
	if (s.hcc != 5)
		fail("B8-5 video: C0 is " + std::to_string(s.hcc) + ", expected header A9 = 5");
	if (s.line != 3)
		fail("B8-5 video: C4 is " + std::to_string(s.line) + ", expected header AB = 3");
	if (s.row != 1)
		fail("B8-5 video: C9 is " + std::to_string(s.row) + ", expected header AC = 1");
	if (s.adj)
		fail("B8-5 video: vertical adjust active with header B0 bit 7 clear");
	if (!s.hs)
		fail("B8-5 video: header B0 bit 1 set but HSYNC is inactive after apply");
	if (!s.vs)
		fail("B8-5 video: header B0 bit 0 set but VSYNC is inactive after apply");
	// Deterministic address seed from the settled R12/R13 (the format does not
	// serialize the pointer; see the module comment).
	if (s.ma != (uint16_t)(((0x31 & 0x3F) << 8) | 0x84))
		fail("B8-5 video: VMA is 0x" + hex_str(s.ma, 4) +
		     ", expected the R12/R13 seed 0x3184");

	// The Gate Array shadows come from the same header.
	if (s.ga_mode != 1)
		fail("B8-5 video: GA mode is " + std::to_string(s.ga_mode) + ", expected header RMR mode 1");
	if (s.ga_border != 4)
		fail("B8-5 video: GA border is " + std::to_string(s.ga_border) + ", expected header colour 4");
	if (ga_ink_entry(dut, 0) != 26 || ga_ink_entry(dut, 1) != 11)
		fail("B8-5 video: GA ink entries 0/1 were not restored from header 2F-30");

	// Vertical total adjust: header AD is the live index, AC is meaningless.
	b8_global_reset(dut, tick);
	B8Header hv = h;
	hv.b0     = 0x80;  // adjust active, no sync
	hv.raster = 9;     // must be ignored
	hv.vta    = 2;
	hv.crtc[5] = 4;    // R5 vertical total adjust
	B8ApplySample sv;
	b8_stream_chunk_apply(dut, tick, p, hv, &sv);
	if (!sv.adj)
		fail("B8-5 video: header B0 bit 7 did not restore vertical total adjust");
	if (sv.row != 2)
		fail("B8-5 video: adjust index is " + std::to_string(sv.row) +
		     ", expected header AD = 2 (type 3 carries it in C9)");
	if (sv.hs || sv.vs)
		fail("B8-5 video: sync asserted with both B0 sync bits clear");

	// A v1 snapshot has none of these fields: deterministic reset phase, never
	// a stale v3 value from the previous restore.
	b8_global_reset(dut, tick);
	B8Header h1 = h;
	h1.version = 1;
	B8ApplySample s1;
	b8_stream_chunk_apply(dut, tick, p, h1, &s1);
	if (s1.hcc != 0 || s1.line != 0 || s1.row != 0 || s1.adj || s1.hs || s1.vs)
		fail("B8-5 video: a v1 snapshot applied v3 counter/flag fields");
	if (b8_crtc_read(dut, tick, 12) != 0x31)
		fail("B8-5 video: a v1 snapshot lost the v1 CRTC register file");

	std::printf("PASS b8_5_b1: video register file, v3 counters, sync flags, v1 gating\n");
}

// --- Test 10: Gate Array VSYNC-delay phase (header B2).
void test_b8_ga_vsync_delay(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };

	// hcnt walks 00 -> 01 -> 06 on HSYNC falling edges and the shaped monitor
	// VSYNC is high from state 06: B2 is exactly how many of those edges are
	// still owed. Each case restores an active raw VSYNC and counts real CRTC
	// HSYNC falling edges.
	struct Case { uint8_t vsdelay; uint8_t b0; int falls_to_shaped; const char* name; };
	const Case cases[] = {
		{2, 0x01, 2, "B2=2 (no HSYNC seen yet)"},
		{1, 0x01, 1, "B2=1 (one HSYNC seen)"},
	};

	for (const Case& c : cases) {
		b8_global_reset(dut, tick);
		B8Header h;
		b8_set_short_frame_crtc(h);
		h.b0 = c.b0;
		h.vsdelay = c.vsdelay;
		B8ChunkParams p;
		p.dcsr = 0x00;
		B8ApplySample s;
		b8_stream_chunk_apply(dut, tick, p, h, &s);

		if (s.ga_vsync_o)
			fail(std::string("B8-5 GA delay: ") + c.name +
			     " restored an already-shaped VSYNC");
		for (int i = 1; i < c.falls_to_shaped; ++i) {
			b8_wait_hsync_falls(dut, tick, 1, "GA delay");
			if (dut.ga_vsync_o)
				fail(std::string("B8-5 GA delay: ") + c.name +
				     " shaped VSYNC rose one HSYNC early");
		}
		b8_wait_hsync_falls(dut, tick, 1, "GA delay");
		// The shaped level follows the combinational hcnt, so it is up within
		// a couple of clocks of the edge.
		bool up = false;
		for (int i = 0; i < 8 && !up; ++i) { if (dut.ga_vsync_o) up = true; tick(); }
		if (!up)
			fail(std::string("B8-5 GA delay: ") + c.name +
			     " shaped VSYNC did not rise on its owed HSYNC");
	}

	// B2=0 with an active raw VSYNC: the re-sync already happened, so the
	// shaped pulse is up at the apply and no second re-sync is manufactured.
	b8_global_reset(dut, tick);
	B8Header h0;
	b8_set_short_frame_crtc(h0);
	h0.b0 = 0x01;
	h0.vsdelay = 0;
	B8ChunkParams p0;
	p0.dcsr = 0x00;
	B8ApplySample s0;
	b8_stream_chunk_apply(dut, tick, p0, h0, &s0);
	if (!s0.ga_vsync_o)
		fail("B8-5 GA delay: B2=0 with active VSYNC did not restore the shaped pulse");

	// B2=0 with no VSYNC: parked lost-sync state, nothing shapes.
	b8_global_reset(dut, tick);
	B8Header hp;
	b8_set_short_frame_crtc(hp);
	hp.crtc[7] = 60;  // R7 beyond R4: no natural VSYNC during the window
	hp.b0 = 0x00;
	hp.vsdelay = 0;
	B8ApplySample sp;
	b8_stream_chunk_apply(dut, tick, p0, hp, &sp);
	if (sp.ga_vsync_o)
		fail("B8-5 GA delay: B2=0 without VSYNC shaped a pulse anyway");
	for (int i = 0; i < 3; ++i) {
		b8_wait_hsync_falls(dut, tick, 1, "GA delay parked");
		if (dut.ga_vsync_o)
			fail("B8-5 GA delay: parked state shaped a VSYNC from HSYNCs alone");
	}

	std::printf("PASS b8_5_b2: GA VSYNC-delay phase for B2 = 2/1/0\n");
}

// --- Test 11: GA interrupt counter (B3) and pending flag (B4).
void test_b8_ga_interrupt_restore(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };

	// B3 = 51: the very next shaped line completes the 52-line count and the
	// classic interrupt fires. Nothing may fire before that edge.
	b8_global_reset(dut, tick);
	B8Header h;
	b8_set_short_frame_crtc(h);
	h.crtc[7] = 60;   // keep VSYNC (and its counter re-sync) out of the window
	h.intcnt = 51;
	B8ChunkParams p;
	p.dcsr = 0x00;
	B8ApplySample s51;
	b8_stream_chunk_apply(dut, tick, p, h, &s51);
	if (!s51.ga_int_n)
		fail("B8-5 GA int: restoring B3=51 with B4=0 asserted an interrupt at the apply");
	b8_wait_hsync_falls(dut, tick, 1, "GA int");
	bool fired = false;
	for (int i = 0; i < 8 && !fired; ++i) { if (!dut.ga_int_n_out) fired = true; tick(); }
	if (!fired)
		fail("B8-5 GA int: B3=51 did not complete the 52-line count on the next line");

	// B3 = 0: the counter is at the start of its cycle, so no interrupt for
	// several lines.
	b8_global_reset(dut, tick);
	B8Header h0 = h;
	h0.intcnt = 0;
	b8_stream_chunk_apply(dut, tick, p, h0);
	for (int i = 0; i < 4; ++i) {
		b8_wait_hsync_falls(dut, tick, 1, "GA int early");
		if (!dut.ga_int_n_out)
			fail("B8-5 GA int: B3=0 fired within four lines of the apply");
	}

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

	// B4 = 1 with no DMA flag pending: the GA holds the level, the first
	// acknowledge retires it, and it does not come back on its own.
	b8_global_reset(dut, tick);
	B8Header hi = h;
	hi.intcnt = 0;
	hi.intreq = 1;
	B8ApplySample si;
	b8_stream_chunk_apply(dut, tick, p, hi, &si);
	if (si.ga_int_n)
		fail("B8-5 GA int: header B4=1 did not restore a pending interrupt");
	if (si.int_n_merged)
		fail("B8-5 GA int: aggregate INT_n level lost after restore");
	if (aregs_read(0x2C0F) & 0x80)
		fail("B8-5 GA int: restored DCSR bit 7 set before raster acknowledge");
	// Acknowledge: M1 and IORQ low together while INT is asserted.
	dut.ga_m1_n = 0;
	dut.ga_iorq_n = 0;
	for (int i = 0; i < 8; ++i) tick();
	if (!dut.ack_vec_valid || dut.ack_vec_byte != 0x06)
		fail("B8-5 GA int: raster acknowledge did not present vector 0x06");
	if (!dut.ga_int_n_out)
		fail("B8-5 GA int: the first acknowledge did not retire the restored interrupt");
	dut.ga_iorq_n = 1;
	dut.ga_m1_n = 1;
	for (int i = 0; i < 4; ++i) tick();
	if (dut.int_n_merged != 1)
		fail("B8-5 GA int: aggregate INT_n not retired after raster acknowledge");
	bool raster_stat_ok = ((aregs_read(0x2C0F) & 0x80) != 0);
	for (int i = 0; i < 200; ++i) {
		tick();
		if (!dut.ga_int_n_out)
			fail("B8-5 GA int: the restored interrupt was duplicated after its acknowledge");
	}

	// B4 = 1 that the restored DCSR already explains: attributed to the DMA
	// path, so the GA line stays clear while the aggregate level is preserved.
	// DCSR bit 7 (last-ack-was-raster) survives the apply untouched.
	b8_global_reset(dut, tick);
	B8ChunkParams pd;
	pd.dcsr = 0xC1;   // bit7 stat, bit6 = ch0 flag, ch0 enabled
	B8ApplySample sd;
	b8_stream_chunk_apply(dut, tick, pd, hi, &sd);
	if (!sd.ga_int_n)
		fail("B8-5 GA int: a DMA-explained pending flag was also credited to the GA");
	if (sd.int_n_merged)
		fail("B8-5 GA int: aggregate INT_n level lost for a DMA-sourced pending flag");
	if (!(dut.aregs_dcsr & 0x80) || !(aregs_read(0x2C0F) & 0x80))
		fail("B8-5 GA int: restored DCSR bit 7 was cleared by the apply");
	for (int i = 0; i < 200; ++i) {
		tick();
		if (!(dut.aregs_dcsr & 0x80))
			fail("B8-5 GA int: a post-apply runtime edge overwrote the restored DCSR bit 7");
	}

	// First DMA acknowledge: IVR bit 0 was cleared to 0 by chunk apply (&6805=0),
	// so automatic flag retirement is enabled.
	dut.ga_m1_n = 0;
	dut.ga_iorq_n = 0;
	for (int i = 0; i < 8; ++i) tick();
	if (!dut.ack_vec_valid || dut.ack_vec_byte != 0x04)
		fail("B8-5 GA int: DMA acknowledge did not present vector 0x04");
	dut.ga_iorq_n = 1;
	dut.ga_m1_n = 1;
	for (int i = 0; i < 4; ++i) tick();
	if (dut.int_n_merged != 1)
		fail("B8-5 GA int: aggregate INT_n not retired after DMA acknowledge");
	bool dma_stat_ok = ((aregs_read(0x2C0F) & 0x80) == 0);

	if (!raster_stat_ok && !dma_stat_ok)
		fail("B8-5 GA int: both first-ack provenances failed: raster did not set bit 7 and DMA left bit 7 sticky");
	else if (!raster_stat_ok)
		fail("B8-5 GA int: first raster acknowledge did not set DCSR bit 7");
	else if (!dma_stat_ok)
		fail("B8-5 GA int: first DMA acknowledge left restored DCSR bit 7 sticky");

	std::printf("PASS b8_5_b3: GA interrupt counter B3 and pending-flag B4 attribution\n");
}

// --- Test 12: palette provenance, no phantom legacy event, first CPU write.
void test_b8_palette_provenance(Vplus_p8_test_top& dut) {
	auto tick = [&]() { dut.clk = 0; dut.eval(); dut.clk = 1; dut.eval(); };

	// Plain SNA: the header's 5-bit colours translate into entries 0-16,
	// including hardware colour 31 which the reset import's sentinel would
	// have skipped. Sprite entries stay untouched.
	b8_global_reset(dut, tick);
	B8Header h;
	b8_set_short_frame_crtc(h);
	h.inksel      = 7;    // header 2E, the selected pen
	h.palette[0]  = 31;   // sentinel-shaped value
	h.palette[1]  = 11;
	h.palette[15] = 20;
	h.palette[16] = 26;   // border
	b8_stream_plain_apply(dut, tick, h);
	if (b8_pal_read(dut, tick, 0) != hw_colour_gbr(31))
		fail("B8-5 palette: plain SNA skipped hardware colour 31 at entry 0");
	if (b8_pal_read(dut, tick, 1) != hw_colour_gbr(11))
		fail("B8-5 palette: plain SNA entry 1 mistranslated");
	if (b8_pal_read(dut, tick, 15) != hw_colour_gbr(20))
		fail("B8-5 palette: plain SNA entry 15 mistranslated");
	if (b8_pal_read(dut, tick, 16) != hw_colour_gbr(26))
		fail("B8-5 palette: plain SNA border entry mistranslated");
	if (b8_pal_read(dut, tick, 17) != 0)
		fail("B8-5 palette: plain SNA disturbed a sprite palette entry");

	// The restored border colour reaches the screen through the production
	// palette read port, not just the storage array.
	dut.pal_probe_sel = 0;
	int settled = 0;
	uint16_t want = hw_colour_gbr(26);
	for (int i = 0; i < 4000 && !settled; ++i) {
		tick();
		if (!dut.video_de && !dut.video_hs &&
		    dut.video_rgb_r == ((want >> 4) & 0xF) &&
		    dut.video_rgb_g == ((want >> 8) & 0xF) &&
		    dut.video_rgb_b == (want & 0xF))
			settled = 1;
	}
	if (!settled)
		fail("B8-5 palette: the restored border colour never reached the video RGB output");
	dut.pal_probe_sel = 1;

	// CPC+ snapshot: a 12-bit entry no hardware colour can express survives,
	// and the differing header colours must not overwrite it.
	b8_global_reset(dut, tick);
	B8ChunkParams p;
	p.dcsr = 0x00;
	p.pal_lo[0] = 0x24; p.pal_hi[0] = 0x09;   // 0x924
	p.pal_lo[16] = 0x78; p.pal_hi[16] = 0x0E; // 0xE78
	p.pal_lo[20] = 0x11; p.pal_hi[20] = 0x02; // sprite entry 0x211
	b8_stream_chunk_apply(dut, tick, p, h);
	if (b8_pal_read(dut, tick, 0) != 0x924 || b8_pal_read(dut, tick, 16) != 0xE78)
		fail("B8-5 palette: CPC+ 12-bit entries were overwritten by the header colours");
	if (b8_pal_read(dut, tick, 20) != 0x211)
		fail("B8-5 palette: CPC+ sprite palette entry did not survive the apply");

	// The apply emits no legacy write event at any point (a snapshot must not
	// impersonate an ordinary I/O write).
	b8_global_reset(dut, tick);
	dut.sna_download = 1;
	dut.seam_plus_asic_reset = 1;
	tick();
	dut.seam_plus_asic_reset = 0;
	int events = 0;
	auto watch = [&]() {
		tick();
		if (dut.ga_leg_pal_wr) ++events;
	};
	b8_stream_header(dut, watch, h);
	dut.sna_download = 0;
	for (int i = 0; i < 400; ++i) watch();
	if (events)
		fail("B8-5 palette: the snapshot apply raised " + std::to_string(events) +
		     " phantom legacy palette write events");

	// Header 2E: an INKR write with no preceding PENR must land at the
	// RESTORED selected pen, which is the only owner-visible proof that the
	// pen index came back.
	b8_ga_io_write(dut, tick, 0x40 | 12);   // INKR: hardware colour 12
	if (ga_ink_entry(dut, 7) != 12)
		fail("B8-5 palette: header 2E did not restore the selected pen (INKR landed elsewhere)");
	if (b8_pal_read(dut, tick, 7) != hw_colour_gbr(12))
		fail("B8-5 palette: the restored-pen INKR write did not reach the palette");

	// An ordinary PENR + INKR pair still works afterwards.
	b8_ga_io_write(dut, tick, 0x02);        // PENR: select pen 2
	b8_ga_io_write(dut, tick, 0x40 | 18);   // INKR: hardware colour 18
	if (ga_ink_entry(dut, 2) != 18)
		fail("B8-5 palette: the first post-restore INKR write did not reach the GA shadow");
	if (b8_pal_read(dut, tick, 2) != hw_colour_gbr(18))
		fail("B8-5 palette: the first post-restore INKR write did not reach the palette");

	std::printf("PASS b8_5_b4: palette provenance (plain/CPC+, no phantom event, first CPU write)\n");
}

} // namespace

// Every B8-5 focused case. Each runs on its own DUT instance so a restore
// under test cannot inherit another case's state.
int run_b8_focused() {
	struct Entry { const char* name; void (*fn)(Vplus_p8_test_top&); };
	static const Entry entries[] = {
		{"b8_5_a1", test_b8_dma_sar_restore},
		{"b8_5_a2", test_b8_dma_pause_prescaler},
		{"b8_5_a3", test_b8_dma_loop_restore},
		{"b8_5_a4", test_b8_dma_hsync_race},
		{"b8_5_a5", test_b8_mmu_rom_gates},
		{"b8_5_a6", test_b8_unlock_restore},
		{"b8_5_a7", test_b8_apply_controller},
		{"b8_5_a8", test_b8_restore_isolation},
		{"b8_5_b1", test_b8_video_restore},
		{"b8_5_b2", test_b8_ga_vsync_delay},
		{"b8_5_b3", test_b8_ga_interrupt_restore},
		{"b8_5_b4", test_b8_palette_provenance},
	};
	const int total = (int)(sizeof(entries) / sizeof(entries[0]));
	int failures = 0;
	for (const Entry& e : entries) {
		try {
			Vplus_p8_test_top dut;
			e.fn(dut);
		} catch (const std::exception& ex) {
			std::fprintf(stderr, "FAIL %s: %s\n", e.name, ex.what());
			++failures;
		}
	}
	if (failures) {
		std::fprintf(stderr, "B8-5: %d/%d focused snapshot-apply tests FAILED\n",
		             failures, total);
		return 1;
	}
	std::printf("All %d B8-5 focused snapshot-apply tests PASSED.\n", total);
	return 0;
}

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	bool b8_only = false;
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--b8-dma-mmu" || arg == "--b8") b8_only = true;
	}

	if (b8_only) return run_b8_focused();

	try {
		Vplus_p8_test_top dut;
		test_p8_i8255_plus_quirks(dut);
		test_p8_sna_parser(dut);
		test_p8_sna_fifo_headroom(dut);
		test_p10c_fdc_motor_tape_gating(dut);
		test_p8_sna_integration_seam(dut);
	} catch (const std::exception& e) {
		std::fprintf(stderr, "FAIL: %s\n", e.what());
		return 1;
	}
	if (run_b8_focused()) return 1;
	std::printf("All Phase P8 platform polish and P10 compatibility tests PASSED.\n");
	return 0;
}
