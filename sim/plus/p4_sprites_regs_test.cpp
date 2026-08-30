// P4 production seam vector: real asic_regs sprite-page storage and
// registered row-fetch arbitration connected directly to real asic_sprites.
//
// The programmed image uses nibble 1 in row 0 and nibble 3 in row 3.  The
// latter is deliberately reached by a live Y/tap transition so the engine's
// row-tag gate is observable.  During one row-3 request a real ASIC-page
// pixel read is held for several clocks; asic_regs must suppress ACK while
// the engine holds FQ_REQ/FQ_ADDR, then grant the request after release.
//
// Expectations are derived from docs/plus/references/asic-reference.md §§3-6
// and the production contracts in rtl/plus/asic_regs.v (sprq_grant) and
// rtl/plus/asic_sprites.v (fq_stale / live row-tag emission gate).

#include "Vp4_sprites_regs_test_top.h"
#include "verilated.h"

#include <array>
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

constexpr unsigned kRow3FetchBase = 0x18; // {sprite 0, row 3, byte 0}
constexpr unsigned kRow3PageBase = 0x30;  // sprite 0, row 3, pixel 0
constexpr unsigned kSpriteX = 2;
constexpr unsigned kSpriteY0 = 0;
constexpr unsigned kSpriteY3 = 1;
constexpr unsigned kSpriteMag = 0x5;     // X×1, Y×1
constexpr unsigned kPalette1Word = 0x124; // stored {G,R,B}; output RGB=0x214
constexpr unsigned kPalette3Word = 0xABC; // stored {G,R,B}; output RGB=0xBAC
constexpr unsigned kPalette1Rgb = 0x214;
constexpr unsigned kPalette3Rgb = 0xBAC;

struct Edge {
	bool req;
	unsigned addr;
	bool ack;
	unsigned data;
};

class Bench {
public:
	Vp4_sprites_regs_test_top dut;
	uint64_t cycle = 0;
	bool previous_req = false;
	unsigned previous_addr = 0;
	std::array<unsigned, 8> row3_starts{};
	std::array<unsigned, 8> row3_pops{};

	Bench() {
		dut.clk = 0;
		dut.reset = 1;
		dut.line = 0;
		dut.row = 0;
		dut.pixen = 0;
		dut.clken = 0;
		dut.hwrap = 0;
		bus_idle();
		dut.eval();
		for (unsigned i = 0; i < 4; ++i)
			tick(false, false, false);
		dut.reset = 0;
		for (unsigned i = 0; i < 2; ++i)
			tick(false, false, false);
	}

	~Bench() { dut.final(); }
	Bench(const Bench&) = delete;
	Bench& operator=(const Bench&) = delete;

	Edge tick(bool pixen, bool clken, bool hwrap) {
		dut.pixen = pixen ? 1 : 0;
		dut.clken = clken ? 1 : 0;
		dut.hwrap = hwrap ? 1 : 0;
		dut.clk = 0;
		dut.eval();
		const Edge edge{
			dut.fq_req != 0,
			static_cast<unsigned>(dut.fq_addr),
			dut.fq_ack != 0,
			static_cast<unsigned>(dut.fq_data),
		};
		dut.clk = 1;
		dut.eval();
		++cycle;
		return edge;
	}

	void bus_idle() {
		dut.asic_cs = 0;
		dut.mem_wr = 0;
		dut.mem_rd = 0;
		dut.A = 0;
		dut.D_in = 0;
		dut.eval();
	}

	void page_write(unsigned address, unsigned value) {
		dut.asic_cs = 1;
		dut.mem_wr = 1;
		dut.mem_rd = 0;
		dut.A = address & 0x3FFF;
		dut.D_in = value & 0xFF;
		const Edge edge = tick(false, false, false);
		if (edge.req && edge.ack)
			fail("page write unexpectedly consumed a sprite fetch");
		bus_idle();
	}

	void note(const Edge& edge, bool watch_emission) {
		// A pop consumes the registered response presented before this edge.
		// This is the real two-register handshake: asic_regs raises ACK/DATA
		// one edge after observing FQ_REQ, and asic_sprites consumes them on the
		// following edge.
		if (edge.req && edge.ack &&
		    edge.addr >= kRow3FetchBase &&
		    edge.addr < kRow3FetchBase + 8) {
			const unsigned byte = edge.addr - kRow3FetchBase;
			if (edge.data != 0x33)
				fail("row3 fetch returned data other than packed 0x33");
			++row3_pops[byte];
		}

		if (dut.fq_req) {
			const unsigned address = static_cast<unsigned>(dut.fq_addr);
			if ((!previous_req || address != previous_addr) &&
			    address >= kRow3FetchBase && address < kRow3FetchBase + 8)
				++row3_starts[address - kRow3FetchBase];
			previous_req = true;
			previous_addr = address;
		}
		else {
			previous_req = false;
		}

		if (watch_emission && dut.spr_en) {
			// Row 0 is colour 1 and row 3 is colour 3.  Any colour-1
			// emission after the live transition is stale-row leakage.
			if (dut.spr_idx != 0 || dut.spr_rgb != kPalette3Rgb)
				fail("stale row0 (or wrong palette) emitted during row3 transition");
		}
	}

	void arm_row0() {
		// d1w/d2w in asic_sprites intentionally delay seam maintenance and
		// walker activation.  Keep taps at vline 0 while row 0 is armed.
		const Edge seam = tick(false, true, true);
		note(seam, false);
		const Edge maintenance = tick(false, false, false);
		note(maintenance, false);
		const Edge arm = tick(false, false, false);
		note(arm, false);
	}

	void verify_row0_stage() {
		std::array<unsigned, 8> pops{};
		unsigned complete = 0;
		for (unsigned guard = 0; guard < 256 && complete < 8; ++guard) {
			const Edge edge = tick(false, false, false);
			if (edge.req && edge.ack && edge.addr < 8) {
				if (edge.data != 0x11)
					fail("row0 fetch returned data other than packed 0x11");
				if (++pops[edge.addr] == 1)
					++complete;
			}
		}
		if (complete != 8)
			fail("did not receive all eight row0 bytes from the real register port");
		if (!dut.spr_en || dut.spr_idx != 0 || dut.spr_rgb != kPalette1Rgb)
			fail("row0 did not emit sprite 0 through palette colour 1 (en=" +
			     std::to_string(static_cast<unsigned>(dut.spr_en)) +
			     ", idx=" + std::to_string(static_cast<unsigned>(dut.spr_idx)) +
			     ", rgb=" + std::to_string(static_cast<unsigned>(dut.spr_rgb)) +
			     ", win=" + std::to_string(static_cast<unsigned>(dut.spr_win)) +
			     ", x=" + std::to_string(static_cast<unsigned>(dut.spr0_x)) +
			     ", y=" + std::to_string(static_cast<unsigned>(dut.spr0_y)) +
			     ", mag=" + std::to_string(static_cast<unsigned>(dut.spr0_mag)) + ")");
	}

	void transition_and_hold_fetch() {
		// This is a real ASIC-page register write.  At vline 0 it makes the
		// current line inactive (Y=1); the following live tap transition to
		// vline 4 selects source row 3.
		page_write(0x2002, kSpriteY3);
		if (dut.spr_en)
			fail("Y page write left the old row visible before the seam");

		bool watching = true;
		const Edge seam = tick(false, true, true);
		note(seam, watching);
		dut.line = 0;
		dut.row = 4;
		const Edge maintenance = tick(false, false, false);
		note(maintenance, watching);

		// Select a freshly issued request rather than one whose grant is
		// already pending.  This makes the multi-clock CPU hold deterministic.
		unsigned held_addr = 0;
		bool found = false;
		for (unsigned guard = 0; guard < 256; ++guard) {
			const Edge edge = tick(false, false, false);
			note(edge, watching);
			if (dut.fq_req && !dut.fq_ack &&
			    dut.fq_addr >= kRow3FetchBase &&
			    dut.fq_addr < kRow3FetchBase + 8) {
				held_addr = static_cast<unsigned>(dut.fq_addr);
				found = true;
				break;
			}
		}
		if (!found)
			fail("did not reach an ungranted row3 fetch request");
		const std::array<unsigned, 8> starts_before_access = row3_starts;
		const std::array<unsigned, 8> pops_before_access = row3_pops;

		// A real CPU read of row3 pixel data.  D_out is the register-file
		// readback (low nibble only), while the same access suppresses sprq ACK
		// and arms the sprite's access-blanking side effect.
		dut.asic_cs = 1;
		dut.mem_wr = 0;
		dut.mem_rd = 1;
		dut.A = kRow3PageBase + ((held_addr - kRow3FetchBase) << 1);
		dut.D_in = 0;
		dut.clk = 0;
		dut.eval();
		if (dut.D_out != 0x03)
			fail("real ASIC-page row3 read did not return the programmed nibble");

		for (unsigned hold = 0; hold < 4; ++hold) {
			const Edge edge = tick(false, false, false);
			note(edge, watching);
			if (!dut.fq_req || dut.fq_addr != held_addr)
				fail("FQ_REQ/FQ_ADDR changed while the CPU read held the port");
			if (dut.fq_ack)
				fail("asic_regs granted a row fetch during the held CPU read");
			if (dut.D_out != 0x03)
				fail("held CPU page read lost its real pixel-data response");
		}

		// Release the CPU read.  The register file grants on this edge; the
		// sprite consumes that registered grant on the next edge.  Because the
		// access was held over the request, the first completion is stale and
		// must be re-demanded; row3 byte 0 therefore appears at least twice.
		bus_idle();
		const Edge release = tick(false, false, false);
		note(release, watching);
		if (!dut.fq_req || dut.fq_addr != held_addr || !dut.fq_ack)
			fail("row3 request was not granted immediately after CPU release");
		const Edge consume = tick(false, false, false);
		note(consume, watching);
		if (!consume.req || !consume.ack || consume.addr != held_addr)
			fail("released row3 grant was not consumed on the next clock");

		bool all_bytes = false;
		for (unsigned guard = 0; guard < 512 && !all_bytes; ++guard) {
			const Edge edge = tick(false, false, false);
			note(edge, watching);
			all_bytes = true;
			for (unsigned byte = 0; byte < 8; ++byte) {
				const unsigned required = (byte == held_addr - kRow3FetchBase) ? 2 : 1;
				if (row3_pops[byte] < required)
					all_bytes = false;
			}
		}
		if (!all_bytes)
			fail("did not complete all row3 bytes, including the stale-fetch re-demand");
		const unsigned held_byte = held_addr - kRow3FetchBase;
		if (row3_starts[held_byte] - starts_before_access[held_byte] < 1)
			fail("stale row3 request was not re-issued after CPU access blanking");
		if (row3_pops[held_byte] - pops_before_access[held_byte] < 2)
			fail("held row3 request did not produce stale plus fresh completions");

		// The access blanking tail is two PIXEN clocks in the production
		// engine.  Advance inside the x1 window and require the new row and
		// palette entry to be visible afterwards.
		bool saw_row3 = false;
		for (unsigned dot = 0; dot < 8; ++dot) {
			const Edge edge = tick(true, false, false);
			note(edge, watching);
			if (dut.spr_en) {
				if (dut.spr_idx != 0 || dut.spr_rgb != kPalette3Rgb)
					fail("post-blanking sprite emission has wrong row or palette");
				saw_row3 = true;
			}
		}
		if (!saw_row3)
			fail("sprite 0 did not reappear with palette colour 3 after blanking");

		// A second, post-staging discriminator exercises the visible access
		// side effect independently of the in-flight stale-response case above.
		// The real register-file access indicator drives this same-sprite blank
		// level; keep PIXEN live so the two-clock tail is consumed exactly as in
		// production.  The engine reappears on the edge that consumes tail 2.
		dut.asic_cs = 1;
		dut.mem_wr = 0;
		dut.mem_rd = 1;
		dut.A = kRow3PageBase;
		dut.D_in = 0;
		dut.clk = 0;
		dut.eval();
		if (dut.D_out != 0x03)
			fail("post-stage same-sprite pixel read lost its real page response");
		for (unsigned access = 0; access < 3; ++access) {
			const Edge edge = tick(true, false, false);
			note(edge, watching);
			if (dut.spr_en)
				fail("sprite remained visible during a same-sprite pixel read");
		}
		bus_idle();
		for (unsigned tail = 0; tail < 2; ++tail) {
			if (dut.spr_en)
				fail("sprite reappeared before the two-clock access tail elapsed");
			const Edge edge = tick(true, false, false);
			note(edge, watching);
		}
		if (!dut.spr_en || dut.spr_idx != 0 || dut.spr_rgb != kPalette3Rgb)
			fail("sprite did not reappear with palette 3 after the access tail");
	}
};

void p4_sprites_regs_fixture() {
	Bench bench;

	// Sprite image: all sixteen row-0 nibbles are 1; all sixteen row-3
	// nibbles are 3.  asic_regs stores one low nibble per page byte, and the
	// row-fetch port packs adjacent pixels as {odd, even}.
	for (unsigned pixel = 0; pixel < 16; ++pixel)
		bench.page_write(pixel, 0x01);
	for (unsigned pixel = 0; pixel < 16; ++pixel)
		bench.page_write(kRow3PageBase + pixel, 0x03);
	bench.bus_idle();
	// Pixel writes leave the real access-blanking indicator high for two
	// PIXEN clocks; clear that production tail before enabling the sprite.
	bench.note(bench.tick(true, false, false), false);
	bench.note(bench.tick(true, false, false), false);

	// Palette colour 1 = {G,R,B}=0x124 and colour 3 = 0xABC.  The ASIC page
	// stores each little-endian word as low {R,B}, then low-nibble G.
	bench.page_write(0x2422, 0x24);
	bench.page_write(0x2423, 0x01);
	bench.page_write(0x2426, 0xBC);
	bench.page_write(0x2427, 0x0A);

	// Sprite 0 attributes: X=2, Y=0, and x1/y1 magnification.  Write the
	// magnification last so no fetch can begin before the image is complete.
	bench.page_write(0x2000, kSpriteX);
	bench.page_write(0x2001, 0x00);
	bench.page_write(0x2002, kSpriteY0);
	bench.page_write(0x2003, 0x00);
	bench.page_write(0x2004, kSpriteMag);
	// Let the live X value enter the engine's rewrite shadow.  This edge is
	// also the first dot inside the x1 window (the two cleanup dots above
	// left hp at X), so the staged row can be observed at the seam.
	bench.note(bench.tick(true, false, false), false);
	if (bench.dut.spr0_x != kSpriteX || bench.dut.spr0_y != kSpriteY0 ||
	    bench.dut.spr0_mag != kSpriteMag ||
	    bench.dut.spr0_pal3 != kPalette3Word)
		fail("ASIC page writes did not reach sprite attributes/palette 3");

	bench.arm_row0();
	bench.verify_row0_stage();
	bench.transition_and_hold_fetch();

	std::printf("PASS p4-sprites-regs: page writes, row arbitration, stale-row gate, and blanking\n");
}

} // namespace

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	try {
		p4_sprites_regs_fixture();
		return 0;
	}
	catch (const std::exception& error) {
		std::printf("FAIL p4-sprites-regs: %s\n", error.what());
		return 1;
	}
}
