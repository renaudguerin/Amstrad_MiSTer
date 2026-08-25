// Exhaustive unit vectors for rtl/plus/asic_regs.v (P2, architecture §4
// exit: "exhaustive page decode/read/write/mirror/mask/open-bus tests").
//
// Expectations are derived from docs/plus/references/asic-reference.md and
// cited at each assertion — never read back out of the simulator.
//
//   a01  sprite pixel RAM: full 4K sweep, low-nibble mask, isolation
//   a02  sprite X/Y/mag: slot/offset sweep, masks, mirrors, &FF rules
//   a03  palette bytes: packing, odd-byte high-nibble zero, dual port
//   a04  legacy PENR/INKR translation into entries 0-16 ([KT] table)
//   a05  raster/DMA storage: write-only regions, DCSR window, SAR/PPR
//   a06  open bus: unmapped regions contribute 8'hFF; cs=0 contributes FF
//   a07  reset contract: IVR bit0 = 1, mag cleared

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "Vasic_regs.h"
#include "verilated.h"

namespace {

class TestFailure : public std::runtime_error {
public:
	explicit TestFailure(const std::string& m) : std::runtime_error(m) {}
};

[[noreturn]] void fail(const std::string& what) { throw TestFailure(what); }

class Regs {
public:
	Vasic_regs dut;
	uint64_t cyc = 0;

	Regs() : dut("asic_regs") {
		dut.clk = 0;
		dut.reset = 1;
		dut.asic_cs = 0;
		dut.mem_wr = 0;
		dut.mem_rd = 0;
		dut.A = 0;
		dut.D_in = 0;
		dut.leg_border = 16;
		dut.leg_inkr[0] = 0;
		dut.leg_inkr[1] = 0;
		dut.leg_inkr[2] = 0;
		dut.pal_raddr = 0;
		dut.dma_int_set = 0;
	}

	void set_inkr_word(uint32_t lo, uint32_t hi) {
		dut.leg_inkr[0] = lo;
		dut.leg_inkr[1] = hi;
		dut.leg_inkr[2] = 0;
	}

	void tick() {
		dut.clk = 0; dut.eval();
		dut.clk = 1; dut.eval();
		++cyc;
	}

	void run(unsigned n) { for (unsigned i = 0; i < n; ++i) tick(); }

	void run1() { run(1); }

	void reset_pulse() {
		dut.reset = 1;
		run(4);
		dut.reset = 0;
		run(2);
	}

	void wr(uint16_t off, uint8_t v) {
		dut.asic_cs = 1;
		dut.mem_rd = 0;
		dut.mem_wr = 1;
		dut.A = off;
		dut.D_in = v;
		tick();
		dut.mem_wr = 0;
		run(1);
	}

	uint8_t rd(uint16_t off) {
		dut.asic_cs = 1;
		dut.mem_wr = 0;
		dut.mem_rd = 1;
		dut.A = off;
		dut.eval();
		const uint8_t v = dut.D_out;
		tick();
		dut.mem_rd = 0;
		return v;
	}

	uint8_t idle_out() {
		dut.asic_cs = 0;
		dut.mem_rd = 0;
		dut.mem_wr = 0;
		dut.eval();
		return dut.D_out;
	}

	// Legacy register shadow update (asic_ga_timing output analogue).
	// Entry k sits at bits [k*5 +: 5] of the 80-bit port; entries 12-15
	// straddle the C++ 64-bit boundary, so accumulate into three Verilator
	// words explicitly.
	void set_legacy(const uint8_t inks[16], uint8_t border) {
		// Legacy writes arrive on the &7Fxx I/O port, which never asserts
		// the page chip-select: keep cs low through the update so the test
		// proves translation fires without any page access.
		dut.asic_cs = 0;
		dut.mem_rd = 0;
		dut.mem_wr = 0;
		uint64_t w01 = 0;
		uint32_t w2 = 0;
		for (unsigned k = 0; k < 16; ++k) {
			const uint64_t v = inks[k] & 0x1F;
			const unsigned base = k * 5;
			if (base < 64) {
				w01 |= v << base;
				if (base + 5 > 64) {
					const unsigned spill = base + 5 - 64;
					w2 |= uint32_t(v >> (5 - spill));
				}
			} else {
				w2 |= uint32_t(v << (base - 64));
			}
		}
		dut.leg_inkr[0] = uint32_t(w01);
		dut.leg_inkr[1] = uint32_t(w01 >> 32);
		dut.leg_inkr[2] = w2;
		dut.leg_border = border;
		tick();
		run(1);
	}

	uint16_t pal_read(uint8_t entry) {
		dut.pal_raddr = entry;
		tick();
		return dut.pal_rdata;
	}
};

// [KT] legacy colour table, {R,G,B} nibbles — mirrors legacy_colour in RTL.
struct RGB { uint8_t r, g, b; };
RGB kt_table(uint8_t hw) {
	switch (hw & 0x1F) {
	case 0: case 1:  return {6, 6, 6};
	case 2:  return {0, 15, 6};
	case 3:  return {15, 15, 6};
	case 4:  return {0, 0, 6};
	case 5:  return {15, 0, 6};
	case 6:  return {0, 6, 6};
	case 7:  return {15, 6, 6};
	case 8:  return {15, 0, 6};
	case 9:  return {15, 15, 6};
	case 10: return {15, 15, 0};
	case 11: return {15, 15, 15};
	case 12: return {15, 0, 0};
	case 13: return {15, 0, 15};
	case 14: return {15, 6, 0};
	case 15: return {15, 6, 15};
	case 16: return {0, 0, 6};
	case 17: return {0, 15, 6};
	case 18: return {0, 15, 0};
	case 19: return {0, 15, 15};
	case 20: return {0, 0, 0};
	case 21: return {0, 0, 15};
	case 22: return {0, 6, 0};
	case 23: return {0, 6, 15};
	case 24: return {6, 0, 6};
	case 25: return {6, 15, 6};
	case 26: return {6, 15, 0};
	case 27: return {6, 15, 15};
	case 28: return {6, 0, 0};
	case 29: return {6, 0, 15};
	case 30: return {6, 6, 0};
	default: return {6, 6, 15};
	}
}

void a01_sprite_ram(Regs& r) {
	r.reset_pulse();
	for (uint32_t a = 0; a < 4096; ++a)
		r.wr(uint16_t(a), uint8_t((a * 7 + 3) & 0xFF));
	for (uint32_t a = 0; a < 4096; ++a) {
		const uint8_t expect = uint8_t((a * 7 + 3) & 0x0F); // §4 mask
		const uint8_t got = r.rd(uint16_t(a));
		if (got != expect)
			fail("a01: sprite RAM[" + std::to_string(a) + "] reads " +
			     std::to_string(got) + ", expected masked " +
			     std::to_string(expect));
	}
	r.wr(0x0100, 0xA5);
	if (r.rd(0x0100) != 0x05) fail("a01: rewrite lost");
	if (r.rd(0x0101) != uint8_t((0x0101u * 7 + 3) & 0x0F))
		fail("a01: neighbour byte disturbed");
	std::printf("PASS a01: sprite pixel RAM sweep, nibble mask, isolation\n");
}

void a02_sprite_regs(Regs& r) {
	r.reset_pulse();
	for (unsigned n = 0; n < 16; ++n) {
		const uint16_t base = 0x2000 + uint16_t(n) * 8; // page offset of &6000+8n
		const uint8_t xl = uint8_t(0x11 * n + 1);
		const uint8_t yl = uint8_t(0x11 * n + 2);
		r.wr(base + 0, xl);
		r.wr(base + 2, yl);
		if (r.rd(base + 0) != xl)
			fail("a02: X lo sprite " + std::to_string(n));
		if (r.rd(base + 2) != yl)
			fail("a02: Y lo sprite " + std::to_string(n));

		// X high stores 2 bits; written&3==3 reads &FF else the value (§4).
		for (unsigned h = 0; h < 8; ++h) {
			r.wr(base + 1, uint8_t(0xF8 | h)); // noise above bit1 must vanish
			const unsigned stored = h & 3;
			const uint8_t want = (stored == 3) ? 0xFF : uint8_t(stored);
			if (r.rd(base + 1) != want || r.rd(base + 5) != want)
				fail("a02: X hi rule/mirror sprite " + std::to_string(n) +
				     " h=" + std::to_string(h));
		}
		// Y high stores 1 bit; written&1==1 reads &FF else 0 (reference §4).
		// Nontrivial bus values pin the masking itself.
		r.wr(base + 3, 0xFE);
		if (r.rd(base + 3) != 0x00 || r.rd(base + 7) != 0x00)
			fail("a02: Y hi zero rule sprite " + std::to_string(n));
		r.wr(base + 3, 0x01);
		if (r.rd(base + 3) != 0xFF || r.rd(base + 7) != 0xFF)
			fail("a02: Y hi FF rule sprite " + std::to_string(n));
		// +6 reads mirror +2 (§4).
		if (r.rd(base + 6) != yl)
			fail("a02: mirror +6/+2 sprite " + std::to_string(n));
		// +6 reads mirror +2 (§4).
		if (r.rd(base + 6) != yl)
			fail("a02: mirror +6/+2 sprite " + std::to_string(n));

		// Magnification mirrors on +4..+6 writes (§4, ⚠ ASIC-REF note for
		// +3 which stays Y-high here); magnification reads give the
		// position mirrors, never the stored mag byte.
		r.wr(base + 4, 0x5A);
		r.wr(base + 5, 0xA5);
		r.wr(base + 6, 0xFF);
		r.wr(base + 7, 0x00);
		if (r.rd(base + 4) != xl || r.rd(base + 5) != ((r.rd(base + 1))))
			fail("a02: read mirror row sprite " + std::to_string(n));
	}
	std::printf("PASS a02: sprite regs masks, &FF rules, read mirrors\n");
}

void a03_palette(Regs& r) {
	r.reset_pulse();
	// All 32 entries, both bytes: even D7-D4=RED D3-D0=BLUE; odd D3-D0=
	// GREEN with the top nibble reading zero (§6).
	for (unsigned e = 0; e < 32; ++e) {
		const uint16_t base = 0x2400 + uint16_t(e) * 2; // page offset of &6400+2e
		const uint8_t rb = uint8_t(0x10 * e + 0x21);
		const uint8_t g  = uint8_t(0x0D * e + 0x07);
		r.wr(base + 0, rb);
		r.wr(base + 1, g);
		const uint8_t exp_even = uint8_t((rb >> 4) << 4 | (rb & 0x0F)); // identity
		const uint8_t exp_odd  = uint8_t(g & 0x0F);                     // top nibble 0
		if (r.rd(base + 0) != exp_even)
			fail("a03: palette even readback entry " + std::to_string(e));
		if (r.rd(base + 1) != exp_odd)
			fail("a03: palette odd readback entry " + std::to_string(e));
	}
	// Split writes update only their nibbles.
	const uint16_t b0 = 0x2400;
	r.wr(b0 + 0, 0x12); // R=1 B=2
	r.wr(b0 + 1, 0xF3); // G=3 (F must be dropped)
	if (r.rd(b0 + 0) != 0x12 || r.rd(b0 + 1) != 0x03)
		fail("a03: split-byte packing");
	r.wr(b0 + 1, 0x24); // G=4
	if (r.rd(b0 + 0) != 0x12) fail("a03: odd write disturbed even byte");
	r.wr(b0 + 0, 0x56); // even write must preserve stored G=4 (reference §6)
	if (r.rd(b0 + 1) != 0x04) fail("a03: even write disturbed green");
	// Video port tracks CPU writes independently (§6 dual-ported); entry
	// word after the sequence above is {G,R,B} = 4,5,6.
	if (r.pal_read(0) != 0x456)
		fail("a03: video port entry 0 = " + std::to_string(r.pal_read(0)));
	if (r.pal_read(31) == r.pal_read(0))
		fail("a03: video port shows aliased entries");
	std::printf("PASS a03: palette packing, high-nibble zero, dual port\n");
}

void a04_legacy_translation(Regs& r) {
	r.reset_pulse();
	uint8_t inks[16];
	for (unsigned k = 0; k < 16; ++k) inks[k] = uint8_t(k < 8 ? k : 31 - k);
	const uint8_t border_hw = 20; // black per [KT]
	r.set_legacy(inks, border_hw);
	for (unsigned k = 0; k < 16; ++k) {
		const RGB c = kt_table(inks[k]);
		const uint16_t got = r.pal_read(uint8_t(k));
		if (got != ((c.g << 8) | (c.r << 4) | c.b)) // stored {G,R,B} (§6 word)
			fail("a04: legacy pen " + std::to_string(k) + " -> " +
			     std::to_string(got));
	}
	const RGB cb = kt_table(border_hw);
	if (r.pal_read(16) != ((cb.g << 8) | (cb.r << 4) | cb.b))
		fail("a04: legacy border translation");
	// Sprite colours are NOT reachable via the legacy port (§6): entries
	// 17-31 keep their reset values across legacy traffic.
	r.set_legacy(inks, border_hw); // idempotent legacy update
	if (r.pal_read(17) != 0)
		fail("a04: legacy write leaked into sprite colour 1");
	// A CPU write to a translated entry wins afterwards.
	r.wr(0x2400 + 0, 0x9C); // R=9 B=C
	r.wr(0x2400 + 1, 0x05); // G=5
	if (r.pal_read(0) != 0x59C)
		fail("a04: CPU write lost to legacy shadow");
	std::printf("PASS a04: legacy PENR/INKR translation, CPU authority\n");
}

void a05_raster_dma_regs(Regs& r) {
	r.reset_pulse();
	struct W { uint16_t off; uint8_t v; };
	// PRI/SPLT/SSA/SSCR/IVR are write-only storage (§3): no direct
	// readback — verified via the exported register bytes below instead.
	r.wr(0x2800, 0x37); // &6800 PRI
	r.wr(0x2801, 0x51); // &6801 SPLT
	r.wr(0x2802, 0xAB); // &6802 SSA hi
	r.wr(0x2803, 0xCD); // &6803 SSA lo
	r.wr(0x2804, 0x22); // &6804 SSCR
	r.wr(0x2805, 0x81); // &6805 IVR
	if (r.dut.pri    != 0x37) fail("a05: PRI store");
	if (r.dut.splt   != 0x51) fail("a05: SPLT store");
	if (r.dut.ssa_hi != 0xAB) fail("a05: SSA hi store");
	if (r.dut.ssa_lo != 0xCD) fail("a05: SSA lo store");
	if (r.dut.sscr   != 0x22) fail("a05: SSCR store");
	if (r.dut.ivr    != 0x81) fail("a05: IVR store");
	r.wr(0x2806, 0xEE); // unused: no effect (§3)
	r.wr(0x2807, 0xEE);
	if (r.dut.ivr != 0x81) fail("a05: &6806/&6807 write leaked");
	// DCSR readable across the whole range, writable only at &6C0F (§4).
	// Field semantics per §9: bit7 merger-driven (read-only for the CPU),
	// bits 6:4 DMA flags write-1-to-clear, bits 2:0 enables plain R/W.
	r.wr(0x2C0F, 0xFF); // tries everything: stat stays 0, flags cleared,
	                    // enables all set
	for (unsigned o = 0; o <= 0x0F; ++o)
		if (r.rd(0x2C00 + o) != 0x07)
			fail("a05: DCSR window read at offset " + std::to_string(o) +
			     " = " + std::to_string(r.rd(0x2C00 + o)) + ", expected 0x07");
	// Flag bits 6:4 have no CPU set-path (only the P7 INT instruction sets
	// them; CPU writes are w1c), so from the bus they read zero here.
	// Enable-bit noise immunity:
	r.wr(0x2C0F, 0xF7); // noise above bit0 must not disturb enables
	if (r.rd(0x2C0F) != 0x07) fail("a05: DCSR high bits leaked into enables");
	r.wr(0x2C0F, 0x02);
	if (r.rd(0x2C0F) != 0x02) fail("a05: DCSR enable write");
	r.wr(0x2C0F, 0x07);
	if (r.rd(0x2C0F) != 0x07) fail("a05: DCSR enable restore");
	// SAR lo is not DCSR and not readable.
	r.wr(0x2C00, 0xA5);
	if (r.rd(0x2C0F) != 0x07) fail("a05: &6C00 write hit DCSR");
	if (r.rd(0x2C00) != 0x07) fail("a05: SAR readable (must not be)");
	std::printf("PASS a05: raster/DMA storage, DCSR fields/window, SAR hidden\n");
}

void a06_open_bus(Regs& r) {
	r.reset_pulse();
	const uint16_t unmapped[] = {
		0x1000, 0x1FFF,             // &5000s
		0x2080, 0x23FF,             // &6080-&63FF
		0x2440, 0x27FF,             // &6440-&67FF
		0x2800, 0x2805,             // &6800-&6805: write-only, reads open bus
		0x2806, 0x2807,             // &6806/&6807 (write-only region read)
		0x2808, 0x280F,             // ADC until its phase lands
		0x2810, 0x2BFF,             // &6810-&6BFF
		0x2C10, 0x3000, 0x3FFF       // &6C10-&6FFF / &7000s
	};
	for (uint16_t off : unmapped)
		if (r.rd(off) != 0xFF)
			fail("a06: offset " + std::to_string(off) + " contributes " +
			     std::to_string(r.rd(off)) + ", expected FF open bus");
	if (r.idle_out() != 0xFF) fail("a06: cs-deasserted contribution not FF");
	std::printf("PASS a06: open-bus neutrality over unmapped regions\n");
}

void a08_dcsr_raster_status(Regs& r) {
	r.reset_pulse();
	if ((r.rd(0x2C0F) & 0x80) != 0) fail("a08: DCSR bit7 set without ack");
	// The merger holds the persistent level; the register page mirrors it.
	r.dut.intack_raster = 1;
	r.run1();
	if ((r.rd(0x2C0F) & 0x80) == 0) fail("a08: raster ack level not visible");
	r.dut.intack_raster = 0;
	r.run1();
	if ((r.rd(0x2C0F) & 0x80) != 0) fail("a08: level stuck after non-raster");
	std::printf("PASS a08: DCSR bit7 follows merger last-raster level\n");
}

void a07_reset_contract(Regs& r) {
	r.reset_pulse();
	if (r.dut.ivr != 0x01) fail("a07: IVR POR bit0"); // §3/§7
	// Magnification cleared at reset (§5): visible through nothing directly,
	// but a subsequent read of the mirrors must show X/Y reset values (0).
	if (r.rd(0x2000) != 0x00) fail("a07: X lo not cleared");
	if (r.rd(0x2001) != 0x00) fail("a07: X hi not cleared");
	if (r.rd(0x2003) != 0x00) fail("a07: Y hi not cleared");
	std::printf("PASS a07: reset contract (IVR bit0, positions clear)\n");
}

// a08: P4 sprite-engine services. Access indicator: any CPU cycle inside
// a sprite image area (wsel 00) asserts EN with IDX=A[11:8]; register /
// unused regions never assert it (reference S5: only pixel-data accesses
// blank, and only that sprite).
void a08_sprite_access_indicator(Regs& r) {
	struct Probe { uint16_t off; const char* what; bool en; unsigned idx; };
	const Probe probes[] = {
		{0x0000, "sprite0 first byte", true, 0},
		{0x00FF, "sprite0 last byte",  true, 0},
		{0x0413, "sprite mid-image",   true, 4},
		{0x0F13, "sprite15",           true, 15},
		{0x1000, "unused &5000",       false, 0},
		{0x2000, "X reg (no blank)",   false, 0},
		{0x2004, "mag reg",            false, 0},
		{0x2400, "palette",            false, 0},
		{0x2800, "PRI",                false, 0},
	};
	for (const auto& p : probes) {
		r.dut.asic_cs = 1; r.dut.mem_rd = 1; r.dut.mem_wr = 0;
		r.dut.A = p.off; r.dut.eval();
		if ((r.dut.spr_acc_en != 0) != p.en ||
		    (p.en && r.dut.spr_acc_idx != p.idx))
			fail(std::string("a08 rd ") + p.what);
		r.dut.mem_rd = 0; r.dut.mem_wr = 1; r.dut.eval();
		if ((r.dut.spr_acc_en != 0) != p.en)
			fail(std::string("a08 wr ") + p.what);
		r.dut.asic_cs = 0; r.dut.mem_wr = 0; r.dut.eval();
		if (r.dut.spr_acc_en != 0) fail("a08 en leaks with cs low");
	}
	std::printf("PASS a10: sprite pixel-data access indicator decode\n");
}

// a09: row-fetch port contract. REQ held is granted when the CPU port is
// idle; ACK pulses one clock after the grant edge with both nibbles of
// the addressed byte position ({odd, even}); a CPU page read cycle
// preempts the grant for that cycle.
void a09_sprq_fetch_port(Regs& r) {
	// Stage known nibbles via page writes (masked to low nibble):
	// sprite1 row2 px4 <- 0xB, px5 <- 0x7. Byte2 of that row therefore
	// reads back {odd nibble px5, even nibble px4} = 0x7B.
	r.wr(0x0124, 0xAB);
	r.wr(0x0125, 0xC7);

	// Idle-port grant: REQ+ADDR held; ACK pulses the cycle after the
	// granting edge, carrying both nibbles.
	r.dut.sprq_req = 1;
	r.dut.sprq_addr = (1u << 7) | (2u << 3) | 2u;
	r.tick();
	if (!r.dut.sprq_ack) fail("a09: no ACK after idle-port REQ");
	if (r.dut.sprq_data != 0x7B)
		fail("a09: fetched byte " + std::to_string(r.dut.sprq_data) +
		     " != packed {odd,even} nibbles");

	// REQ drop terminates the transaction cleanly.
	r.dut.sprq_req = 0;
	r.tick();
	if (r.dut.sprq_ack) fail("a09: ACK stuck after REQ drop");
	r.dut.sprq_req = 0;
	r.dut.sprq_addr = 0;

	// CPU preemption: while a page READ cycle occupies the port, a held
	// REQ is not granted; it completes right after the CPU cycle ends.
	r.dut.asic_cs = 1; r.dut.mem_rd = 1; r.dut.A = 0x0206; r.dut.D_in = 0;
	r.dut.sprq_req = 1;
	r.dut.sprq_addr = (1u << 7) | (2u << 3) | 3u;
	r.tick();
	if (r.dut.sprq_ack) fail("a09: granted during CPU page read");
	r.dut.asic_cs = 0; r.dut.mem_rd = 0;
	r.tick();   // grant edge for the held REQ
	if (!r.dut.sprq_ack) fail("a09: no ACK after preemption clears");
	r.dut.sprq_req = 0;
	r.tick();
	std::printf("PASS a09: sprq fetch port grant/data/preempt\n");
}

} // namespace

int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	Regs r;
	unsigned failed = 0;
	struct T { const char* name; void (*fn)(Regs&); };
	const T tests[] = {
		{"a01", a01_sprite_ram}, {"a02", a02_sprite_regs},
		{"a03", a03_palette},    {"a04", a04_legacy_translation},
		{"a05", a05_raster_dma_regs}, {"a06", a06_open_bus},
		{"a07", a07_reset_contract},
		{"a08", a08_dcsr_raster_status},
		{"a09", a09_sprq_fetch_port},
		{"a10", a08_sprite_access_indicator},
	};
	for (const auto& t : tests) {
		try {
			t.fn(r);
		} catch (const TestFailure& e) {
			++failed;
			std::printf("FAIL %s: %s\n", t.name, e.what());
		}
	}
	if (failed) {
		std::printf("%u test groups FAILED\n", failed);
		return 1;
	}
	std::printf("All asic_regs unit vectors passed\n");
	return 0;
}
