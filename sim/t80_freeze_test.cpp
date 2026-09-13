// B18 slice 1: production-T80 snapshot freeze point (T80pa INSN_START).
//
// Design: docs/b18-sna-save.md, "Freeze point" and "Acceptance" item 1.
//
// At an INSN_START boundary, REG must hold the architectural state after the
// previous instruction, with PC the address of the instruction being fetched.
// Every expectation below comes from the program text and documented Z80
// semantics: Zilog Z80 CPU User Manual (UM0080) for instruction effects,
// interrupt modes, EI delay, RETN, LD A,I/R and OUTI/OTIR, and Sean Young,
// "The Undocumented Z80 Documented", for flag bits 3/5 (copies of the result)
// and R increments (one per M1 cycle, so two for CB/ED/DD/FD-prefixed
// opcodes). None is read back from the simulator. Reset values (A=F=FF,
// SP=FFFF, I=R=0, IFF=0, IM 0) are the T80 reset assignments in
// rtl/T80/T80.vhd; BC/DE/HL and the alternates are not reset there, so every
// program loads the ones it checks.
//
// Per-tick invariants (all tests): INSN_START only during an opcode fetch
// (M1, MREQ and RD low, address = REG.PC), never during an interrupt
// acknowledge (M1 and IORQ low).
#include "Vt80_freeze_top.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Snap {
  uint64_t tick = 0;
  std::array<uint32_t, 7> raw{};  // all 212 REG bits
  uint16_t pc = 0, sp = 0, bc = 0, de = 0, hl = 0, ix = 0, iy = 0;
  uint16_t bc2 = 0, de2 = 0, hl2 = 0;
  uint8_t a = 0, f = 0, a2 = 0, f2 = 0, i = 0, r = 0, im = 0;
  bool iff1 = false, iff2 = false, halted = false;

  // Architectural identity for continuation checks: every REG bit (the
  // alternate banks included) plus HALT. The tick is excluded.
  bool same_state(const Snap &o) const { return raw == o.raw && halted == o.halted; }
};

std::string hex(uint64_t v, int w) {
  std::ostringstream os;
  os << std::hex << std::uppercase << std::setfill('0') << std::setw(w) << v;
  return os.str();
}

std::string describe(const Snap &s) {
  return "PC=" + hex(s.pc, 4) + " SP=" + hex(s.sp, 4) + " A=" + hex(s.a, 2) +
         " F=" + hex(s.f, 2) + " BC=" + hex(s.bc, 4) + " DE=" + hex(s.de, 4) +
         " HL=" + hex(s.hl, 4) + " IX=" + hex(s.ix, 4) + " IY=" + hex(s.iy, 4) +
         " AF'=" + hex(s.a2, 2) + hex(s.f2, 2) + " BC'=" + hex(s.bc2, 4) +
         " DE'=" + hex(s.de2, 4) + " HL'=" + hex(s.hl2, 4) + " I=" + hex(s.i, 2) +
         " R=" + hex(s.r, 2) + " IM=" + std::to_string(s.im) +
         " IFF=" + std::to_string(s.iff1) + std::to_string(s.iff2) +
         (s.halted ? " HALT" : "");
}

[[noreturn]] void fail(const std::string &why) { throw std::runtime_error(why); }

void check(bool ok, const std::string &why) {
  if (!ok) fail(why);
}

struct IoWrite {
  uint16_t port;
  uint8_t data;
  bool operator==(const IoWrite &o) const { return port == o.port && data == o.data; }
};

struct Bench {
  Vt80_freeze_top d;
  uint64_t ticks = 0;
  bool prev_start = false;
  bool seen_refresh = false;
  bool prev_io_wr = false;
  std::vector<Snap> bounds;
  std::vector<IoWrite> io_writes;
  std::function<void(Bench &)> stimulus;  // runs after each eval, before the edge

  Bench() {
    d.reset = 1;
    d.cpu_reset = 1;
    d.int_n = 1;
    d.nmi_n = 1;
    d.cpu_hold = 0;
    d.ext_wait = 0;
    d.prog_we = 0;
    d.peek_addr = 0;
    d.save_req = 0;
    d.release_req = 0;
    d.admit = 1;
    d.rampage_ok = 1;
    d.use_controller = 0;
    raw_run(128);
    d.reset = 0;
    raw_run(256);
  }

  unsigned field(unsigned lo, unsigned width) {
    uint64_t v = 0;
    for (unsigned b = 0; b < width; ++b) {
      unsigned bit = lo + b;
      v |= uint64_t((d.reg_state[bit / 32] >> (bit % 32)) & 1u) << b;
    }
    return unsigned(v);
  }

  std::array<uint32_t, 7> raw_reg() {
    std::array<uint32_t, 7> w{};
    for (int k = 0; k < 7; ++k) w[k] = d.reg_state[k];
    w[6] &= 0x000FFFFFu;  // 212 bits: 6*32 + 20
    return w;
  }

  Snap snap() {
    Snap s;
    s.tick = ticks;
    s.raw = raw_reg();
    s.a = field(0, 8);
    s.f = field(8, 8);
    s.a2 = field(16, 8);
    s.f2 = field(24, 8);
    s.i = field(32, 8);
    s.r = field(40, 8);
    s.sp = field(48, 16);
    s.pc = field(64, 16);
    s.bc = field(80, 16);
    s.de = field(96, 16);
    s.hl = field(112, 16);
    s.ix = field(128, 16);
    s.bc2 = field(144, 16);
    s.de2 = field(160, 16);
    s.hl2 = field(176, 16);
    s.iy = field(192, 16);
    s.im = field(208, 2);
    s.iff1 = field(210, 1);
    s.iff2 = field(211, 1);
    s.halted = !d.t80_halt_n;
    return s;
  }

  void edge() {
    d.clk = 1;
    d.eval();
    ++ticks;
    d.clk = 0;
    d.eval();
  }

  void raw_run(unsigned n) {
    while (n--) edge();
  }

  // One clock: observe outputs, apply stimulus, then the edge.
  void tick() {
    d.clk = 0;
    d.eval();
    if (d.insn_start) {
      check(!d.t80_m1_n && !d.t80_mreq_n && !d.t80_rd_n,
            "INSN_START outside an opcode fetch at tick " + std::to_string(ticks));
      check(d.t80_a == field(64, 16),
            "INSN_START with address bus != REG.PC at tick " + std::to_string(ticks));
      // T80pa never resets IntCycleD_n, which drives IORQ_n on M1/T1; it is
      // only flushed to "11" by the first M1/T3 (t80_trace_test masks the same
      // startup X). Judge the acknowledge pins after the first refresh.
      if (seen_refresh)
        check(d.t80_m1_n || d.t80_iorq_n,
              "INSN_START during interrupt acknowledge at tick " + std::to_string(ticks));
      if (!prev_start) bounds.push_back(snap());
    }
    if (!d.t80_rfsh_n) seen_refresh = true;
    prev_start = d.insn_start;
    bool io_wr = !d.t80_iorq_n && !d.t80_wr_n;
    if (io_wr && !prev_io_wr) io_writes.push_back({uint16_t(d.t80_a), uint8_t(d.t80_dout)});
    prev_io_wr = io_wr;
    if (stimulus) stimulus(*this);
    edge();
  }

  void load(uint8_t at, const std::vector<uint8_t> &bytes) {
    for (size_t k = 0; k < bytes.size(); ++k) {
      d.prog_we = 1;
      d.prog_addr = uint8_t(at + k);
      d.prog_data = bytes[k];
      edge();
      d.prog_we = 0;
      edge();
    }
  }

  uint8_t peek(uint8_t addr) {
    d.peek_addr = addr;
    d.clk = 0;
    d.eval();
    return d.peek_data;
  }

  void start() { d.cpu_reset = 0; }

  void run_until(const std::function<bool()> &done, uint64_t limit = 400000) {
    const uint64_t stop = ticks + limit;
    while (!done()) {
      if (ticks >= stop) fail("timeout");
      tick();
    }
  }

  void run_bounds(size_t n) {
    run_until([&] { return bounds.size() >= n; });
  }
};

std::string pcs(const std::vector<Snap> &v) {
  std::string out;
  for (const auto &s : v) out += hex(s.pc, 2) + (s.halted ? "h " : " ");
  return out;
}

void expect_pcs(const std::vector<Snap> &got, const std::vector<unsigned> &want,
                const char *what) {
  bool ok = got.size() >= want.size();
  for (size_t k = 0; ok && k < want.size(); ++k) ok = got[k].pc == want[k];
  if (!ok) {
    std::string w;
    for (unsigned p : want) w += hex(p, 2) + " ";
    fail(std::string(what) + ": boundary PCs got [" + pcs(got) + "] want prefix [" + w + "]");
  }
}

void expect_snap(const Snap &s, bool cond, const std::string &what) {
  if (!cond) fail(what + " at " + describe(s));
}

size_t index_of_pc(const std::vector<Snap> &v, unsigned pc, const char *what) {
  for (size_t k = 0; k < v.size(); ++k)
    if (v[k].pc == pc) return k;
  fail(std::string(what) + ": no boundary at " + hex(pc, 4) + " in [" + pcs(v) + "]");
}

// 1. Deferred ALU writes and plain boundaries.
// ADD A,n and INC r commit A/F on the first enable of the NEXT fetch
// (Save_ALU_r); the boundary must already show the new values.
void test_deferred_alu() {
  Bench b;
  b.load(0x00, {
      0x3E, 0x12,  // 00: LD A,12h
      0xC6, 0x34,  // 02: ADD A,34h -> A=46h; S0 Z0 Y0 H0 X0 V0 N0 C0 = 00h
      0x06, 0x7F,  // 04: LD B,7Fh
      0x04,        // 06: INC B -> B=80h; S1 Z0 Y0 H1 X0 V1 N0, C kept 0 = 94h
      0x76,        // 07: HALT
  });
  b.start();
  b.run_bounds(7);
  const auto &v = b.bounds;
  expect_pcs(v, {0x00, 0x02, 0x04, 0x06, 0x07, 0x08, 0x08}, "deferred ALU");

  expect_snap(v[0], v[0].a == 0xFF && v[0].f == 0xFF && v[0].r == 0 && v[0].sp == 0xFFFF,
              "reset state at first fetch");
  expect_snap(v[1], v[1].a == 0x12 && v[1].f == 0xFF && v[1].r == 1,
              "after LD A,12h: A=12 F unchanged R=1");
  expect_snap(v[2], v[2].a == 0x46 && v[2].f == 0x00 && v[2].r == 2,
              "after ADD A,34h: deferred A=46 F=00 must be committed");
  expect_snap(v[3], (v[3].bc >> 8) == 0x7F && v[3].f == 0x00 && v[3].r == 3,
              "after LD B,7Fh");
  expect_snap(v[4], (v[4].bc >> 8) == 0x80 && v[4].f == 0x94 && v[4].r == 4 && !v[4].halted,
              "after INC B: deferred B=80 F=94 must be committed");
  // First halted fetch: HALT's own M1 counted (R=5); PC stays past HALT.
  expect_snap(v[5], v[5].halted && v[5].r == 5, "first halted fetch");
  expect_snap(v[6], v[6].halted && v[6].r == 6, "second halted fetch (R counts halted M1s)");
}

// 2. EI delay: the fetch right after EI is not a boundary, because EI sets
// IFF1/IFF2 on that fetch's T2 enable. DI/EI/NOP per UM0080.
void test_ei_exclusion() {
  Bench b;
  b.load(0x00, {
      0xF3,  // 00: DI
      0xFB,  // 01: EI
      0x00,  // 02: NOP   <- fetch after EI: no boundary
      0x00,  // 03: NOP
      0x76,  // 04: HALT
  });
  b.start();
  b.run_bounds(5);
  const auto &v = b.bounds;
  expect_pcs(v, {0x00, 0x01, 0x03, 0x04, 0x05}, "EI exclusion");
  expect_snap(v[1], !v[1].iff1 && !v[1].iff2, "after DI: IFF=00");
  expect_snap(v[2], v[2].iff1 && v[2].iff2, "after EI;NOP: IFF=11");
}

// 3. Prefix families: no boundary inside CB/ED/DD/FD/DDCB instructions.
void test_prefixes() {
  Bench b;
  b.load(0x00, {
      0xDD, 0x21, 0x34, 0x12,  // 00: LD IX,1234h   (R+2)
      0xFD, 0x21, 0x78, 0x56,  // 04: LD IY,5678h   (R+2)
      0xCB, 0x07,              // 08: RLC A: FF -> FF, C=1; S1 Z0 Y1 H0 X1 P1 N0 C1 = ADh (R+2)
      0xED, 0x47,              // 0A: LD I,A -> I=FF (R+2)
      0xDD, 0xCB, 0x05, 0xC6,  // 0C: SET 0,(IX+5) -> (1239h) aliases RAM 39h (R+2)
      0xDD, 0x23,              // 10: INC IX -> 1235h (R+2)
      0x76,                    // 12: HALT
  });
  b.start();
  b.run_bounds(8);
  const auto &v = b.bounds;
  expect_pcs(v, {0x00, 0x04, 0x08, 0x0A, 0x0C, 0x10, 0x12, 0x13}, "prefixes");
  expect_snap(v[1], v[1].ix == 0x1234 && v[1].r == 2, "after LD IX");
  expect_snap(v[2], v[2].iy == 0x5678 && v[2].r == 4, "after LD IY");
  expect_snap(v[3], v[3].a == 0xFF && v[3].f == 0xAD && v[3].r == 6, "after RLC A");
  expect_snap(v[4], v[4].i == 0xFF && v[4].r == 8, "after LD I,A");
  expect_snap(v[5], v[5].r == 10, "after SET 0,(IX+5)");
  check(b.peek(0x39) == 0x01, "SET 0,(IX+5) did not set bit 0 of RAM 39h");
  expect_snap(v[6], v[6].ix == 0x1235 && v[6].r == 12, "after INC IX");
}

// 4. Final opcode bytes that equal prefix values. The exclusion is T80's
// semantic Prefix, not the raw previous opcode: CB CB, CB DD and ED ED are
// complete instructions, so the fetch after each is a boundary.
void test_prefix_valued_opcodes() {
  Bench b;
  b.load(0x00, {
      0x11, 0x00, 0x00,  // 00: LD DE,0000h
      0x21, 0x00, 0x00,  // 03: LD HL,0000h
      0xCB, 0xCB,        // 06: CB = 11 001 011: SET 1,E -> E=02h (R+2)
      0xCB, 0xDD,        // 08: DD = 11 011 101: SET 3,L -> L=08h (R+2)
      0xED, 0xED,        // 0A: undefined ED opcode, a two-byte NOP (R+2)
      0x76,              // 0C: HALT
  });
  b.start();
  b.run_bounds(7);
  const auto &v = b.bounds;
  expect_pcs(v, {0x00, 0x03, 0x06, 0x08, 0x0A, 0x0C, 0x0D}, "prefix-valued opcodes");
  expect_snap(v[3], v[3].de == 0x0002 && v[3].r == 4, "after SET 1,E (CB CB)");
  expect_snap(v[4], v[4].hl == 0x0008 && v[4].r == 6, "after SET 3,L (CB DD)");
  expect_snap(v[5], v[5].r == 8, "after ED ED");
}

// 5. LD A,I and LD A,R copy IFF2 into P/V (UM0080); their F must be final at
// the boundary. LD A,R loads R after both of its own M1 increments.
void test_ld_a_i() {
  Bench b;
  b.load(0x00, {
      0xED, 0x57,  // 00: LD A,I: A=00; S0 Z1 Y0 H0 X0 P=IFF2=0 N0, C kept 1 = 41h
      0xFB,        // 02: EI
      0x00,        // 03: NOP  <- no boundary
      0xED, 0x57,  // 04: LD A,I: P=IFF2=1 -> 45h
      0x76,        // 06: HALT
  });
  b.start();
  b.run_bounds(5);
  const auto &v = b.bounds;
  expect_pcs(v, {0x00, 0x02, 0x04, 0x06, 0x07}, "LD A,I");
  expect_snap(v[1], v[1].a == 0x00 && v[1].f == 0x41, "LD A,I with IFF2=0");
  expect_snap(v[3], v[3].a == 0x00 && v[3].f == 0x45, "LD A,I with IFF2=1");
}

void test_ld_a_r() {
  Bench b;
  b.load(0x00, {
      0xED, 0x5F,  // 00: LD A,R after M1s ED,5F: A=02h; S0 Z0 Y0 H0 X0 P=0 N0, C kept 1 = 01h
      0xFB,        // 02: EI
      0x00,        // 03: NOP  <- no boundary
      0xED, 0x5F,  // 04: LD A,R after M1s ED,5F,FB,00,ED,5F: A=06h; P=IFF2=1 -> 05h
      0x76,        // 06: HALT
  });
  b.start();
  b.run_bounds(5);
  const auto &v = b.bounds;
  expect_pcs(v, {0x00, 0x02, 0x04, 0x06, 0x07}, "LD A,R");
  expect_snap(v[1], v[1].a == 0x02 && v[1].f == 0x01, "LD A,R with IFF2=0");
  expect_snap(v[3], v[3].a == 0x06 && v[3].f == 0x05, "LD A,R with IFF2=1");
}

// 6. Block repeats and DJNZ: each LDIR/OTIR repeat refetches its ED opcode and
// is a new instruction start; DJNZ taken branches likewise.
const std::vector<uint8_t> kBlockProgram = {
    0x21, 0x30, 0x00,  // 00: LD HL,0030h
    0x11, 0x40, 0x00,  // 03: LD DE,0040h
    0x01, 0x03, 0x00,  // 06: LD BC,0003h
    0xED, 0xB0,        // 09: LDIR (3 iterations)
    0x06, 0x02,        // 0B: LD B,02h
    0x10, 0xFE,        // 0D: DJNZ 0Dh (2 iterations)
    0x76,              // 0F: HALT
};

void test_block_repeat() {
  Bench b;
  b.load(0x00, kBlockProgram);
  b.load(0x30, {0xA1, 0xB2, 0xC3});
  b.start();
  b.run_bounds(11);
  const auto &v = b.bounds;
  expect_pcs(v, {0x00, 0x03, 0x06, 0x09, 0x09, 0x09, 0x0B, 0x0D, 0x0D, 0x0F, 0x10},
             "LDIR/DJNZ");
  expect_snap(v[3], v[3].bc == 3 && v[3].hl == 0x30 && v[3].de == 0x40, "LDIR iteration 1");
  expect_snap(v[4], v[4].bc == 2 && v[4].hl == 0x31 && v[4].de == 0x41, "LDIR iteration 2");
  expect_snap(v[5], v[5].bc == 1 && v[5].hl == 0x32 && v[5].de == 0x42, "LDIR iteration 3");
  expect_snap(v[6], v[6].bc == 0 && v[6].hl == 0x33 && v[6].de == 0x43, "after LDIR");
  // R: three one-byte loads (+3), then two M1 per LDIR iteration.
  expect_snap(v[4], v[4].r == 5, "R after one LDIR iteration");
  expect_snap(v[6], v[6].r == 9, "R after LDIR");
  expect_snap(v[7], (v[7].bc >> 8) == 2, "DJNZ first pass");
  expect_snap(v[8], (v[8].bc >> 8) == 1, "DJNZ second pass");
  expect_snap(v[9], (v[9].bc >> 8) == 0, "after DJNZ");
  check(b.peek(0x40) == 0xA1 && b.peek(0x41) == 0xB2 && b.peek(0x42) == 0xC3,
        "LDIR copy mismatch");
}

// OUTI decrements B, then puts C on A7-A0 and the decremented B on A15-A8
// (UM0080), so OTIR with BC=0342h writes ports 0242h, 0142h, 0042h.
void test_otir() {
  Bench b;
  b.load(0x00, {
      0x21, 0x30, 0x00,  // 00: LD HL,0030h
      0x01, 0x42, 0x03,  // 03: LD BC,0342h
      0xED, 0xB3,        // 06: OTIR (3 iterations)
      0x76,              // 08: HALT
  });
  b.load(0x30, {0xA1, 0xB2, 0xC3});
  b.start();
  b.run_bounds(7);
  const auto &v = b.bounds;
  expect_pcs(v, {0x00, 0x03, 0x06, 0x06, 0x06, 0x08, 0x09}, "OTIR");
  expect_snap(v[2], v[2].bc == 0x0342 && v[2].hl == 0x30, "OTIR iteration 1");
  expect_snap(v[3], v[3].bc == 0x0242 && v[3].hl == 0x31, "OTIR iteration 2");
  expect_snap(v[4], v[4].bc == 0x0142 && v[4].hl == 0x32, "OTIR iteration 3");
  expect_snap(v[5], v[5].bc == 0x0042 && v[5].hl == 0x33, "after OTIR");
  const std::vector<IoWrite> want = {{0x0242, 0xA1}, {0x0142, 0xB2}, {0x0042, 0xC3}};
  check(b.io_writes == want, "OTIR port writes differ from 0242h:A1 0142h:B2 0042h:C3");
}

// 7. IM 1 interrupt: acknowledge is not a boundary; ISR entry is, with the
// return address pushed and IFFs cleared. EI;RET inside the ISR exercises the
// EI exclusion again on the RET fetch.
void test_im1_interrupt() {
  Bench b;
  b.load(0x00, {
      0x31, 0xF0, 0x00,  // 00: LD SP,00F0h
      0xED, 0x56,        // 03: IM 1
      0xFB,              // 05: EI
      0x00,              // 06: NOP  <- no boundary
      0x18, 0xFE,        // 07: JR 07h
  });
  b.load(0x38, {
      0x3C,  // 38: INC A (FF -> 00)
      0xFB,  // 39: EI
      0xC9,  // 3A: RET  <- no boundary
  });
  // An acknowledge only counts once INT is raised: the unreset IntCycleD_n
  // also pulls IORQ_n low on the very first fetch after reset.
  bool raised = false, acked = false;
  b.stimulus = [&](Bench &bb) {
    size_t loops = 0;
    for (const auto &s : bb.bounds) loops += (s.pc == 0x07);
    if (!raised && loops >= 2) { bb.d.int_n = 0; raised = true; }
    if (raised && !bb.d.t80_m1_n && !bb.d.t80_iorq_n) acked = true;
    if (acked) bb.d.int_n = 1;
  };
  b.start();
  b.run_until([&] {
    for (size_t k = 0; k + 1 < b.bounds.size(); ++k)
      if (b.bounds[k].pc == 0x39 && b.bounds[k + 1].pc == 0x07) return true;
    return false;
  });
  const auto &v = b.bounds;
  expect_pcs(v, {0x00, 0x03, 0x05, 0x07}, "IM1 prologue");
  size_t k = index_of_pc(v, 0x38, "IM1");
  expect_snap(v[k - 1], v[k - 1].pc == 0x07 && v[k - 1].iff1, "boundary before ISR");
  expect_snap(v[k], v[k].sp == 0x00EE && !v[k].iff1 && !v[k].iff2 && v[k].im == 1 && v[k].a == 0xFF,
              "ISR entry");
  check(b.peek(0xEE) == 0x07 && b.peek(0xEF) == 0x00, "pushed return address not 0007h");
  expect_snap(v[k + 1], v[k + 1].pc == 0x39 && v[k + 1].a == 0x00, "after INC A");
  expect_snap(v[k + 2], v[k + 2].pc == 0x07 && v[k + 2].sp == 0x00F0 && v[k + 2].iff1 && v[k + 2].iff2,
              "after EI;RET (no boundary at RET fetch)");
}

// 8. NMI and RETN: NMI entry at 0066h keeps IFF2 (UM0080); RETN restores IFF1
// in its own execution, so the boundary right after RETN already shows IFF1=1.
void test_nmi_retn() {
  Bench b;
  b.load(0x00, {
      0x31, 0xF0, 0x00,  // 00: LD SP,00F0h
      0xED, 0x56,        // 03: IM 1
      0xFB,              // 05: EI
      0x00,              // 06: NOP
      0x18, 0xFE,        // 07: JR 07h
  });
  b.load(0x66, {0xED, 0x45});  // 66: RETN
  bool pulsed = false;
  uint64_t pulse_end = 0;
  b.stimulus = [&](Bench &bb) {
    size_t loops = 0;
    for (const auto &s : bb.bounds) loops += (s.pc == 0x07);
    if (!pulsed && loops >= 2) {
      bb.d.nmi_n = 0;
      pulsed = true;
      pulse_end = bb.ticks + 64;
    }
    if (pulsed && bb.ticks >= pulse_end) bb.d.nmi_n = 1;
  };
  b.start();
  b.run_until([&] {
    for (size_t k = 0; k + 1 < b.bounds.size(); ++k)
      if (b.bounds[k].pc == 0x66 && b.bounds[k + 1].pc == 0x07) return true;
    return false;
  });
  const auto &v = b.bounds;
  size_t k = index_of_pc(v, 0x66, "NMI");
  // The NMI acknowledge M1 fetches at the return address after IFF1 has been
  // cleared; were it a boundary, it would record PC=0007h with IFF1=0 and the
  // NMI lost. The last boundary before entry is the loop, still enabled.
  expect_snap(v[k - 1], v[k - 1].pc == 0x07 && v[k - 1].iff1 && v[k - 1].iff2,
              "no boundary on the NMI acknowledge fetch");
  expect_snap(v[k], v[k].sp == 0x00EE && !v[k].iff1 && v[k].iff2, "NMI entry keeps IFF2");
  check(b.peek(0xEE) == 0x07 && b.peek(0xEF) == 0x00, "NMI pushed return address not 0007h");
  expect_snap(v[k + 1], v[k + 1].pc == 0x07 && v[k + 1].sp == 0x00F0 && v[k + 1].iff1 && v[k + 1].iff2,
              "boundary after RETN shows IFF1 restored");
}

// 9. EI;HALT with INT already pending. EI enables interrupts only after the
// following instruction (UM0080), so HALT executes; the interrupt is then
// taken, so no halted fetch is a boundary (it is the acknowledge), and the
// ISR returns past HALT.
const std::vector<uint8_t> kEiHaltProgram = {
    0x31, 0xF0, 0x00,  // 00: LD SP,00F0h
    0xED, 0x56,        // 03: IM 1
    0xFB,              // 05: EI
    0x76,              // 06: HALT <- no boundary (EI)
    0x00,              // 07: NOP
    0x18, 0xFE,        // 08: JR 08h
};

void test_ei_halt_pending_int() {
  Bench b;
  b.load(0x00, kEiHaltProgram);
  b.load(0x38, {0xFB, 0xC9});  // 38: EI; 39: RET <- no boundary
  bool raised = false, acked = false;
  b.stimulus = [&](Bench &bb) {
    // Raise INT once IM 1 is the next instruction, well before EI runs.
    if (!raised && !bb.bounds.empty() && bb.bounds.back().pc == 0x03) {
      bb.d.int_n = 0;
      raised = true;
    }
    if (raised && !bb.d.t80_m1_n && !bb.d.t80_iorq_n) acked = true;
    if (acked) bb.d.int_n = 1;
  };
  b.start();
  b.run_until([&] {
    for (const auto &s : b.bounds)
      if (s.pc == 0x08) return true;
    return false;
  });
  const auto &v = b.bounds;
  expect_pcs(v, {0x00, 0x03, 0x05, 0x38, 0x07, 0x08}, "EI;HALT with pending INT");
  for (const auto &s : v) check(!s.halted, "a halted fetch was a boundary: " + describe(s));
  expect_snap(v[3], v[3].sp == 0x00EE && !v[3].iff1, "ISR entry after EI;HALT");
  check(b.peek(0xEE) == 0x07 && b.peek(0xEF) == 0x00, "return address not 0007h (past HALT)");
  expect_snap(v[4], v[4].iff1 && v[4].sp == 0x00F0, "after ISR");
}

// 10. EI;HALT, then an interrupt arriving while already halted: the halted
// fetch is a boundary with IFF1=1, and the ISR returns past HALT.
void test_halt_wake() {
  Bench b;
  b.load(0x00, kEiHaltProgram);
  b.load(0x38, {0xFB, 0xC9});  // 38: EI; 39: RET <- no boundary
  bool raised = false, acked = false;
  b.stimulus = [&](Bench &bb) {
    size_t halted = 0;
    for (const auto &s : bb.bounds) halted += s.halted;
    if (!raised && halted >= 2) { bb.d.int_n = 0; raised = true; }
    if (raised && !bb.d.t80_m1_n && !bb.d.t80_iorq_n) acked = true;
    if (acked) bb.d.int_n = 1;
  };
  b.start();
  b.run_until([&] {
    for (const auto &s : b.bounds)
      if (s.pc == 0x08) return true;
    return false;
  });
  const auto &v = b.bounds;
  expect_pcs(v, {0x00, 0x03, 0x05, 0x07}, "HALT wake prologue");
  expect_snap(v[3], v[3].halted && v[3].iff1 && v[3].iff2, "halted fetch after EI;HALT");
  size_t k = index_of_pc(v, 0x38, "HALT wake");
  check(b.peek(0xEE) == 0x07 && b.peek(0xEF) == 0x00, "halted ISR return address not 0007h");
  expect_snap(v[k + 1], v[k + 1].pc == 0x07 && !v[k + 1].halted && v[k + 1].iff1,
              "after ISR: resumes after HALT");
}

// 11. Freeze continuation. A hold at a boundary must be equivalent to a
// hardware WAIT of the same length: every REG bit (alternate banks loaded with
// distinct values) identical to an uninterrupted run at every boundary, the
// same memory and port writes, and an identical post-release bus trace
// (address, data, controls) between hold and WAIT. Entry follows the
// production shape: the controller registers INSN_START, so the gate takes
// effect one clock after the rise, which must precede the negative enable
// that samples WAIT_n.
const std::vector<uint8_t> kContinuationProgram = {
    0x01, 0x11, 0x11,  // 00: LD BC,1111h
    0x11, 0x22, 0x22,  // 03: LD DE,2222h
    0x21, 0x33, 0x33,  // 06: LD HL,3333h
    0x3E, 0x44,        // 09: LD A,44h
    0xD9,              // 0B: EXX
    0x08,              // 0C: EX AF,AF'
    0x21, 0x30, 0x00,  // 0D: LD HL,0030h
    0x11, 0x40, 0x00,  // 10: LD DE,0040h
    0x01, 0x03, 0x00,  // 13: LD BC,0003h
    0xED, 0xB0,        // 16: LDIR (3 iterations)
    0x21, 0x30, 0x00,  // 18: LD HL,0030h
    0x01, 0x42, 0x03,  // 1B: LD BC,0342h
    0xED, 0xB3,        // 1E: OTIR (3 iterations)
    0x76,              // 20: HALT
};
const size_t kContinuationBounds = 20;  // through the first halted fetch

void load_continuation(Bench &b) {
  b.load(0x00, kContinuationProgram);
  b.load(0x30, {0xA1, 0xB2, 0xC3});
}

struct BusEvent {
  uint64_t rel;
  uint64_t word;
  bool operator==(const BusEvent &o) const { return rel == o.rel && word == o.word; }
};

uint64_t bus_word(Bench &b) {
  return (uint64_t(b.d.t80_a) << 16) | (uint64_t(b.d.t80_dout) << 8) |
         (b.d.t80_m1_n << 7) | (b.d.t80_mreq_n << 6) | (b.d.t80_iorq_n << 5) |
         (b.d.t80_rd_n << 4) | (b.d.t80_wr_n << 3) | (b.d.t80_rfsh_n << 2) |
         (b.d.t80_halt_n << 1);
}

uint8_t get_header_byte(const Vt80_freeze_top &d, unsigned offset) {
  return uint8_t((d.header[offset / 4] >> ((offset % 4) * 8)) & 0xFF);
}

struct StallRun {
  std::vector<Snap> bounds;
  std::vector<BusEvent> after;
  std::vector<IoWrite> io_writes;
  uint8_t dest[3];
};

StallRun run_stall(int mode /* 0 free, 1 hold, 2 WAIT, 3 controller */, size_t at_boundary,
                   unsigned hold_ticks, std::vector<uint8_t> *captured_hdr = nullptr,
                   size_t *actual_freeze = nullptr) {
  Bench b;
  load_continuation(b);
  StallRun out;
  enum { WAITING, ARMED, STALLED, RELEASED } phase = mode ? WAITING : RELEASED;
  uint64_t stall_start = 0, release = 0;
  uint64_t frozen_bus = 0;
  std::array<uint32_t, 7> frozen_reg{};
  uint64_t last_bus = 0;
  bool have_last = false;
  bool req_pulsed = false;

  if (mode == 3) {
    b.d.use_controller = 1;
    b.d.admit = 1;
    b.d.rampage_ok = 1;
  }

  b.stimulus = [&](Bench &bb) {
    switch (phase) {
      case WAITING:
        if (mode == 3) {
          if (!req_pulsed && bb.bounds.size() == at_boundary && !bb.d.insn_start) {
            bb.d.save_req = 1;
            req_pulsed = true;
          } else if (bb.d.save_req) {
            bb.d.save_req = 0;
          }
          if (bb.d.captured) {
            if (actual_freeze) *actual_freeze = bb.bounds.size() - 1;
            if (captured_hdr) {
              captured_hdr->resize(256);
              for (unsigned o = 0; o < 256; ++o) (*captured_hdr)[o] = get_header_byte(bb.d, o);
            }
            stall_start = bb.ticks;
            frozen_bus = bus_word(bb);
            frozen_reg = bb.raw_reg();
            phase = STALLED;
          }
        } else {
          if (bb.bounds.size() == at_boundary + 1 && bb.d.insn_start) {
            check(!bb.d.phi_n, "negative enable coincides with INSN_START rise");
            phase = ARMED;  // registered controller: gate from the next clock
          }
        }
        break;
      case ARMED:
        check(!bb.d.phi_n, "negative enable before registered hold takes effect");
        if (mode == 1) bb.d.cpu_hold = 1; else bb.d.ext_wait = 1;
        stall_start = bb.ticks;
        frozen_bus = bus_word(bb);
        frozen_reg = bb.raw_reg();
        phase = STALLED;
        break;
      case STALLED:
        if (mode == 3 && bb.ticks - stall_start >= hold_ticks) {
          bb.d.release_req = 0;
          release = bb.ticks;
          phase = RELEASED;
          break;
        }
        if (mode == 1 || mode == 3) {
          check(bb.d.insn_start, "INSN_START dropped during hold");
          check(bus_word(bb) == frozen_bus, "bus moved during hold");
          check(bb.raw_reg() == frozen_reg, "REG moved during hold");
        }
        if (mode == 3) {
          check(bb.d.ctrl_hold, "ctrl_hold not high during controller stall");
          if (bb.ticks - stall_start == hold_ticks - 1) {
            bb.d.release_req = 1;
          }
        } else {
          if (bb.ticks - stall_start >= hold_ticks) {
            bb.d.cpu_hold = 0;
            bb.d.ext_wait = 0;
            release = bb.ticks;
            phase = RELEASED;
          }
        }
        break;
      case RELEASED: {
        if (mode == 0) break;
        uint64_t w = bus_word(bb);
        if (!have_last || w != last_bus) out.after.push_back({bb.ticks - release, w});
        last_bus = w;
        have_last = true;
        break;
      }
    }
  };
  b.start();
  b.run_bounds(kContinuationBounds);
  check(mode == 0 || phase == RELEASED, "stall never released");
  out.bounds = b.bounds;
  out.io_writes = b.io_writes;
  for (int k = 0; k < 3; ++k) out.dest[k] = b.peek(uint8_t(0x40 + k));
  return out;
}

void test_hold_vs_wait() {
  const unsigned kStall = 64 * 20;  // 20 GA sequencer cycles
  StallRun free_run = run_stall(0, 0, 0);

  // Paper anchors for the free run: the EXX / EX AF,AF' prologue moves
  // BC=1111h DE=2222h HL=3333h AF=44FFh into the alternate banks, and OTIR
  // writes the three source bytes to ports 0242h/0142h/0042h.
  const Snap &s = free_run.bounds[7];  // boundary at 10h, after EX AF,AF'
  expect_snap(s, s.pc == 0x10 && s.bc2 == 0x1111 && s.de2 == 0x2222 && s.hl2 == 0x3333 &&
                     s.a2 == 0x44 && s.f2 == 0xFF,
              "alternate banks after EXX; EX AF,AF'");
  const std::vector<IoWrite> want_io = {{0x0242, 0xA1}, {0x0142, 0xB2}, {0x0042, 0xC3}};
  check(free_run.io_writes == want_io, "free-run OTIR writes differ from paper");
  check(free_run.dest[0] == 0xA1 && free_run.dest[1] == 0xB2 && free_run.dest[2] == 0xC3,
        "free-run LDIR destination differs from paper");

  // Stall inside LDIR (boundary 10: second iteration) and inside OTIR
  // (boundary 16: second iteration).
  for (size_t at : {size_t(10), size_t(16)}) {
    StallRun held = run_stall(1, at, kStall);
    StallRun waited = run_stall(2, at, kStall);
    const std::string where = " (stall at boundary " + std::to_string(at) + ")";
    for (size_t k = 0; k < kContinuationBounds; ++k) {
      check(held.bounds[k].same_state(free_run.bounds[k]),
            "hold changed boundary " + std::to_string(k) + where + ": " +
                describe(held.bounds[k]) + " vs free " + describe(free_run.bounds[k]));
      check(waited.bounds[k].same_state(free_run.bounds[k]),
            "WAIT changed boundary " + std::to_string(k) + where);
    }
    check(held.io_writes == want_io && waited.io_writes == want_io,
          "port writes changed by the stall" + where);
    for (int k = 0; k < 3; ++k)
      check(held.dest[k] == free_run.dest[k] && waited.dest[k] == free_run.dest[k],
            "LDIR destination changed by the stall" + where);
    check(!held.after.empty() && held.after.size() == waited.after.size(),
          "post-release bus traces differ in length" + where + ": hold " +
              std::to_string(held.after.size()) + " wait " + std::to_string(waited.after.size()));
    for (size_t k = 0; k < held.after.size(); ++k)
      check(held.after[k] == waited.after[k],
            "post-release bus event " + std::to_string(k) + " differs" + where + ": hold rel=" +
                std::to_string(held.after[k].rel) + " word=" + hex(held.after[k].word, 10) +
                " wait rel=" + std::to_string(waited.after[k].rel) +
                " word=" + hex(waited.after[k].word, 10));
    for (size_t k = at + 1; k < kContinuationBounds; ++k)
      check(held.bounds[k].tick == waited.bounds[k].tick,
            "boundary " + std::to_string(k) + " timing differs between hold and WAIT" + where);
    std::cout << "[obs: at " << at << " delay vs free="
              << (held.bounds[at + 1].tick - free_run.bounds[at + 1].tick) << "t] " << std::flush;
  }
}

// 14. Controller hold equals hardware WAIT (B18 slice 4a).
// Paper derivation:
// Continuation program has BC=1111h DE=2222h HL=3333h AF=44FFh in the alternate
// bank, LDIR copying 3 bytes from 0030h to 0040h, and OTIR writing 3 bytes to
// ports 0242h/0142h/0042h.
// Boundary 10 is inside LDIR (second iteration, PC=0016h).
// Boundary 16 is inside OTIR (second iteration, PC=001Eh).
// Pulsing save_req prior to the target boundary arms the controller.
// On the rising edge of INSN_START at the target boundary:
//   - captured pulses, ctrl_hold is registered high, and header is latched.
//   - actual_freeze == at_boundary (proves the controller froze at the boundary expected,
//     not one later).
// After hold_ticks = kStall (1280 ticks = 20 GA sequencer cycles), release_req
// releases hold.
// Assertions:
//   - Architectural state across all boundaries matches free_run.
//   - Post-release bus events and event timing match hardware WAIT at that boundary.
//   - Port writes and memory writes match free_run.
// Discrimination check for mutant (a):
// Assert save_req when INSN_START is ALREADY high (mid-window, tested at boundary 6).
// Rising-edge detection ignores the already-high level and waits for boundary 7
// (the next rising edge). Level detection would freeze at boundary 6.
void test_controller_hold_vs_wait() {
  const unsigned kStall = 64 * 20;  // 20 GA sequencer cycles
  StallRun free_run = run_stall(0, 0, 0);
  const std::vector<IoWrite> want_io = {{0x0242, 0xA1}, {0x0142, 0xB2}, {0x0042, 0xC3}};

  for (size_t at : {size_t(10), size_t(16)}) {
    size_t actual_freeze = 999;
    std::vector<uint8_t> hdr;
    StallRun ctrl_run = run_stall(3, at, kStall, &hdr, &actual_freeze);
    StallRun waited = run_stall(2, at, kStall);
    const std::string where = " (controller stall at boundary " + std::to_string(at) + ")";

    check(actual_freeze == at,
          "controller froze at boundary " + std::to_string(actual_freeze) + " instead of " +
              std::to_string(at));

    for (size_t k = 0; k < kContinuationBounds; ++k) {
      check(ctrl_run.bounds[k].same_state(free_run.bounds[k]),
            "controller hold changed boundary " + std::to_string(k) + where + ": " +
                describe(ctrl_run.bounds[k]) + " vs free " + describe(free_run.bounds[k]));
    }
    check(ctrl_run.io_writes == want_io, "controller port writes changed by stall" + where);
    for (int k = 0; k < 3; ++k)
      check(ctrl_run.dest[k] == free_run.dest[k],
            "controller LDIR destination changed by stall" + where);
    check(!ctrl_run.after.empty() && ctrl_run.after.size() == waited.after.size(),
          "post-release bus traces differ in length" + where + ": ctrl " +
              std::to_string(ctrl_run.after.size()) + " wait " + std::to_string(waited.after.size()));
    for (size_t k = 0; k < ctrl_run.after.size(); ++k)
      check(ctrl_run.after[k] == waited.after[k],
            "post-release bus event " + std::to_string(k) + " differs" + where + ": ctrl rel=" +
                std::to_string(ctrl_run.after[k].rel) + " word=" + hex(ctrl_run.after[k].word, 10) +
                " wait rel=" + std::to_string(waited.after[k].rel) +
                " word=" + hex(waited.after[k].word, 10));
    for (size_t k = at + 1; k < kContinuationBounds; ++k)
      check(ctrl_run.bounds[k].tick == waited.bounds[k].tick,
            "boundary " + std::to_string(k) + " timing differs between controller hold and WAIT" + where);
  }

  // Mid-window request discrimination check (mutant a):
  // Assert save_req when INSN_START is ALREADY high at boundary 6 (fetch of LD HL, 0030h at 0Dh).
  // Rising-edge detection must ignore the already-high level and freeze at boundary 7 (fetch of LD DE at 10h).
  // Level detection would freeze at boundary 6.
  {
    Bench b;
    load_continuation(b);
    b.d.use_controller = 1;
    b.d.admit = 1;
    b.d.rampage_ok = 1;
    size_t freeze_boundary = 999;
    bool pulsed = false;

    b.stimulus = [&](Bench &bb) {
      if (!pulsed && bb.bounds.size() == 7 && bb.d.insn_start) {
        bb.d.save_req = 1;
        pulsed = true;
      } else if (bb.d.save_req) {
        bb.d.save_req = 0;
      }
      if (bb.d.captured) {
        freeze_boundary = bb.bounds.size() - 1;
        bb.d.release_req = 1;
      } else if (bb.d.release_req) {
        bb.d.release_req = 0;
      }
    };
    b.start();
    b.run_bounds(kContinuationBounds);
    check(freeze_boundary == 7,
          "mid-window request froze at boundary " + std::to_string(freeze_boundary) +
              " instead of boundary 7 (must wait for rising edge)");
  }
}

// 15. Header CPU bytes (B18 slice 4a).
// Paper derivation:
// Captured at boundary 10 of kContinuationProgram (second iteration of LDIR).
// Program state at boundary 10:
// - Magic: "MV - SNA" at offsets 0x00..0x07 (4Dh, 56h, 20h, 2Dh, 20h, 53h, 4Eh, 41h)
// - Version 3 at offset 0x10 (03h)
// - Unused zero padding: 0x08..0x0F and 0xB5..0xFF
// - Registers:
//   F (0x11): C5h. LDI/LDIR (Sean Young, "The Undocumented Z80 Documented",
//             block instructions): S, Z and C are unchanged (FFh reset F, swapped
//             back in by EX AF,AF'), H=0, N=0, P/V=1 because BC is nonzero, YF =
//             bit 1 and XF = bit 3 of A + (HL) = FFh + A1h = 1A0h, both 0.
//             S Z Y H X P N C = 1 1 0 0 0 1 0 1 = C5h.
//   A (0x12): FFh (reset value untouched)
//   C (0x13): 02h (BC decremented from 0003h to 0002h)
//   B (0x14): 00h
//   E (0x15): 41h (DE incremented from 0040h to 0041h)
//   D (0x16): 00h
//   L (0x17): 31h (HL incremented from 0030h to 0031h)
//   H (0x18): 00h
//   R (0x19): 0Bh (9 instruction fetches + 2 M1s in LDIR iteration 1 = 11)
//   I (0x1A): 00h (reset value)
//   IFF1 (0x1B): 00h, IFF2 (0x1C): 00h
//   IX (0x1D-0x1E): 0000h, IY (0x1F-0x20): 0000h
//   SP (0x21-0x22): FFFFh (reset value)
//   PC (0x23-0x24): 0016h (LDIR opcode fetch address)
//   IM (0x25): 00h
//   F' (0x26): FFh (reset F moved to F' via EX AF,AF' at 0Ch)
//   A' (0x27): 44h (LD A,44h at 09h moved to A' via EX AF,AF' at 0Ch)
//   C' (0x28): 11h, B' (0x29): 11h (BC=1111h moved to BC' via EXX at 0Bh)
//   E' (0x2A): 22h, D' (0x2B): 22h (DE=2222h moved to DE' via EXX at 0Bh)
//   L' (0x2C): 33h, H' (0x2D): 33h (HL=3333h moved to HL' via EXX at 0Bh)
// - Classic hardware header: offsets 0x2E..0xB4 match hw_hdr (byte i = i).
void test_header_cpu_bytes() {
  const unsigned kStall = 64 * 4;
  std::vector<uint8_t> hdr;
  size_t freeze_at = 0;
  run_stall(3, 10, kStall, &hdr, &freeze_at);
  check(hdr.size() == 256, "header size not 256 bytes");

  // Identification string "MV - SNA"
  const std::string sig(hdr.begin(), hdr.begin() + 8);
  check(sig == "MV - SNA", "SNA magic signature mismatch: got '" + sig + "'");

  // Unused 0x08-0x0F
  for (unsigned o = 0x08; o <= 0x0F; ++o) {
    check(hdr[o] == 0x00, "unused byte at 0x" + hex(o, 2) + " nonzero");
  }

  // Version 3
  check(hdr[0x10] == 0x03, "version byte at 0x10 != 3: got " + hex(hdr[0x10], 2));

  // Z80 registers (derived on paper)
  check(hdr[0x11] == 0xC5, "F at 0x11 mismatch: expected C5h, got " + hex(hdr[0x11], 2));
  check(hdr[0x12] == 0xFF, "A at 0x12 mismatch: expected FFh, got " + hex(hdr[0x12], 2));
  check(hdr[0x13] == 0x02, "C at 0x13 mismatch: expected 02h, got " + hex(hdr[0x13], 2));
  check(hdr[0x14] == 0x00, "B at 0x14 mismatch: expected 00h, got " + hex(hdr[0x14], 2));
  check(hdr[0x15] == 0x41, "E at 0x15 mismatch: expected 41h, got " + hex(hdr[0x15], 2));
  check(hdr[0x16] == 0x00, "D at 0x16 mismatch: expected 00h, got " + hex(hdr[0x16], 2));
  check(hdr[0x17] == 0x31, "L at 0x17 mismatch: expected 31h, got " + hex(hdr[0x17], 2));
  check(hdr[0x18] == 0x00, "H at 0x18 mismatch: expected 00h, got " + hex(hdr[0x18], 2));
  check(hdr[0x19] == 0x0B, "R at 0x19 mismatch: expected 0Bh, got " + hex(hdr[0x19], 2));
  check(hdr[0x1A] == 0x00, "I at 0x1A mismatch: expected 00h, got " + hex(hdr[0x1A], 2));
  check(hdr[0x1B] == 0x00, "IFF1 at 0x1B mismatch: expected 00h, got " + hex(hdr[0x1B], 2));
  check(hdr[0x1C] == 0x00, "IFF2 at 0x1C mismatch: expected 00h, got " + hex(hdr[0x1C], 2));
  check(hdr[0x1D] == 0x00, "IX low at 0x1D mismatch: expected 00h, got " + hex(hdr[0x1D], 2));
  check(hdr[0x1E] == 0x00, "IX high at 0x1E mismatch: expected 00h, got " + hex(hdr[0x1E], 2));
  check(hdr[0x1F] == 0x00, "IY low at 0x1F mismatch: expected 00h, got " + hex(hdr[0x1F], 2));
  check(hdr[0x20] == 0x00, "IY high at 0x20 mismatch: expected 00h, got " + hex(hdr[0x20], 2));
  check(hdr[0x21] == 0xFF, "SP low at 0x21 mismatch: expected FFh, got " + hex(hdr[0x21], 2));
  check(hdr[0x22] == 0xFF, "SP high at 0x22 mismatch: expected FFh, got " + hex(hdr[0x22], 2));
  check(hdr[0x23] == 0x16, "PC low at 0x23 mismatch: expected 16h, got " + hex(hdr[0x23], 2));
  check(hdr[0x24] == 0x00, "PC high at 0x24 mismatch: expected 00h, got " + hex(hdr[0x24], 2));
  check(hdr[0x25] == 0x00, "IM at 0x25 mismatch: expected 00h, got " + hex(hdr[0x25], 2));
  check(hdr[0x26] == 0xFF, "F' at 0x26 mismatch: expected FFh, got " + hex(hdr[0x26], 2));
  check(hdr[0x27] == 0x44, "A' at 0x27 mismatch: expected 44h, got " + hex(hdr[0x27], 2));
  check(hdr[0x28] == 0x11, "C' at 0x28 mismatch: expected 11h, got " + hex(hdr[0x28], 2));
  check(hdr[0x29] == 0x11, "B' at 0x29 mismatch: expected 11h, got " + hex(hdr[0x29], 2));
  check(hdr[0x2A] == 0x22, "E' at 0x2A mismatch: expected 22h, got " + hex(hdr[0x2A], 2));
  check(hdr[0x2B] == 0x22, "D' at 0x2B mismatch: expected 22h, got " + hex(hdr[0x2B], 2));
  check(hdr[0x2C] == 0x33, "L' at 0x2C mismatch: expected 33h, got " + hex(hdr[0x2C], 2));
  check(hdr[0x2D] == 0x33, "H' at 0x2D mismatch: expected 33h, got " + hex(hdr[0x2D], 2));

  // Hardware header 0x2E..0xB4 matches pattern byte i = i
  for (unsigned o = 0x2E; o <= 0xB4; ++o) {
    uint8_t want = uint8_t(o - 0x2E);
    check(hdr[o] == want, "hw_hdr at 0x" + hex(o, 2) + " mismatch: expected " +
                              hex(want, 2) + ", got " + hex(hdr[o], 2));
  }

  // Trailing unused 0xB5..0xFF
  for (unsigned o = 0xB5; o <= 0xFF; ++o) {
    check(hdr[o] == 0x00, "trailing unused byte at 0x" + hex(o, 2) + " nonzero: " + hex(hdr[o], 2));
  }
}

// 16. HALT adjustment (B18 slice 4a).
// Paper derivation:
// Program:
//   00: 3E 85       LD A, 85h       ; A = 85h (bit 7 = 1, low 7 bits = 5)
//   02: ED 4F       LD R, A         ; sets R = 85h
//   04: 76          HALT            ; HALT instruction at address 0004h
// During fetch of HALT at 04h, R low 7 bits increment from 5 to 6 (R becomes 86h).
// T80 enters HALT with halt_n low. Internal PC points to 0005h (address after HALT).
// On dummy M1 fetch during HALT, INSN_START pulses.
// SNA save rule for HALT (docs/b18-sna-save.md):
//   - Saved PC = PC - 1 = 0005h - 1 = 0004h (address of HALT opcode).
//   - Saved R = {R[7], R[6:0] - 1} = {1'b1, 7'd6 - 7'd1} = 85h.
// Bit 7 of R is preserved as 1.
void test_halt_freeze() {
  Bench b;
  b.load(0x00, {
      0x3E, 0x85,  // 00: LD A, 85h
      0xED, 0x4F,  // 02: LD R, A
      0x76,        // 04: HALT
  });
  b.d.use_controller = 1;
  b.d.admit = 1;
  b.d.rampage_ok = 1;

  bool req_sent = false;
  bool captured_seen = false;
  uint16_t saved_pc = 0;
  uint8_t saved_r = 0;

  b.stimulus = [&](Bench &bb) {
    if (!bb.d.t80_halt_n && !req_sent && !bb.d.insn_start) {
      bb.d.save_req = 1;
      req_sent = true;
    } else if (bb.d.save_req) {
      bb.d.save_req = 0;
    }
    if (bb.d.captured) {
      captured_seen = true;
      saved_pc = uint16_t(get_header_byte(bb.d, 0x23)) |
                 (uint16_t(get_header_byte(bb.d, 0x24)) << 8);
      saved_r = get_header_byte(bb.d, 0x19);
      bb.d.release_req = 1;
    } else if (bb.d.release_req) {
      bb.d.release_req = 0;
    }
  };

  b.start();
  b.run_bounds(10);

  check(captured_seen, "controller never captured during HALT");
  check(saved_pc == 0x0004,
        "HALT saved PC mismatch: expected 0004h (HALT opcode address), got " + hex(saved_pc, 4));
  check(saved_r == 0x85,
        "HALT saved R mismatch: expected 85h, got " + hex(saved_r, 2));
  check((saved_r & 0x80) != 0, "HALT saved R bit 7 not preserved");
}

// 17. Postponement and cancel (B18 slice 4a).
// Paper derivation:
// Postponement:
//   Program: 4x EI, 3x DD prefixes, LD IX, 1234h, LD A, 55h, HALT.
//   Address 00-03: EI (FBh). SetEI suppresses INSN_START.
//   Address 04-06: DD prefixes. Prefix /= "00" suppresses INSN_START.
//   Address 07-09: 21 34 12 (LD IX, 1234h).
//   Address 0A: 3E 55 (LD A, 55h). First qualifying boundary!
//   When save_req is pulsed during EI execution, INSN_START does not rise
//   until address 0Ah. The freeze must land at PC = 000Ah, not earlier.
// Cancel:
//   During EI execution, pulse save_req to arm the controller.
//   Before any qualifying boundary, pulse save_req a second time.
//   Controller must pulse cancelled and return to IDLE without asserting hold.
void test_postponement_and_cancel() {
  // Part A: Postponement
  {
    Bench b;
    b.load(0x00, {
        0xFB,             // 00: EI
        0xFB,             // 01: EI
        0xFB,             // 02: EI
        0xFB,             // 03: EI
        0xDD, 0xDD, 0xDD, // 04, 05, 06: DD prefixes
        0x21, 0x34, 0x12, // 07: LD IX, 1234h
        0x3E, 0x55,       // 0A: LD A, 55h
        0x76,             // 0C: HALT
    });
    b.d.use_controller = 1;
    b.d.admit = 1;
    b.d.rampage_ok = 1;

    bool req_sent = false;
    bool captured_seen = false;
    uint16_t capture_pc = 0;

    b.stimulus = [&](Bench &bb) {
      if (!req_sent && bb.bounds.size() == 1 && !bb.d.insn_start) {
        bb.d.save_req = 1;
        req_sent = true;
      } else if (bb.d.save_req) {
        bb.d.save_req = 0;
      }
      if (bb.d.captured) {
        captured_seen = true;
        capture_pc = uint16_t(get_header_byte(bb.d, 0x23)) |
                     (uint16_t(get_header_byte(bb.d, 0x24)) << 8);
        bb.d.release_req = 1;
      } else if (bb.d.release_req) {
        bb.d.release_req = 0;
      }
    };

    b.start();
    b.run_bounds(10);

    check(captured_seen, "controller never captured after postponement");
    check(capture_pc == 0x000A,
          "postponed capture PC mismatch: expected 000Ah (LD A, 55h), got " + hex(capture_pc, 4));
  }

  // Part B: Cancel on second press
  {
    Bench b;
    b.load(0x00, {
        0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB,  // 00-07: 8x EI
        0x3E, 0x77,                                      // 08: LD A, 77h
        0x76,                                            // 0A: HALT
    });
    b.d.use_controller = 1;
    b.d.admit = 1;
    b.d.rampage_ok = 1;

    bool req1_sent = false;
    bool req2_sent = false;
    bool cancelled_seen = false;
    bool captured_seen = false;
    bool hold_seen = false;
    unsigned cycle_count = 0;

    b.stimulus = [&](Bench &bb) {
      if (bb.bounds.size() == 1 && !bb.d.insn_start) {
        ++cycle_count;
        if (cycle_count == 2) {
          bb.d.save_req = 1;
          req1_sent = true;
        } else if (cycle_count == 3) {
          bb.d.save_req = 0;
        } else if (cycle_count == 10) {
          bb.d.save_req = 1;
          req2_sent = true;
        } else if (cycle_count == 11) {
          bb.d.save_req = 0;
        }
      }

      if (bb.d.cancelled) cancelled_seen = true;
      if (bb.d.captured) captured_seen = true;
      if (bb.d.ctrl_hold) hold_seen = true;
    };

    b.start();
    b.run_bounds(10);

    check(req1_sent && req2_sent, "cancel requests were not both sent");
    check(cancelled_seen, "cancelled pulse not seen after second save_req");
    check(!captured_seen, "captured was asserted despite cancel");
    check(!hold_seen, "hold was asserted despite cancel");
  }
}

// 18. Refusal when !admit or !rampage_ok (B18 slice 4a).
// Paper derivation:
// If save_req arrives while admit=0 (core in reset, download or apply)
// or rampage_ok=0 (MMU expansion memory mapped, RAMpage != 3),
// the controller must pulse refused for 1 clock and remain in IDLE (hold=0, busy=0).
void test_refusal() {
  // Case 5a: rampage_ok = 0
  {
    Bench b;
    b.d.use_controller = 1;
    b.d.admit = 1;
    b.d.rampage_ok = 0;

    b.d.save_req = 1;
    b.tick();
    b.d.save_req = 0;

    check(b.d.refused, "refused not pulsed when rampage_ok == 0");
    check(!b.d.ctrl_hold, "ctrl_hold asserted when refused");

    b.tick();
    check(!b.d.refused, "refused pulse persisted past 1 clock");
  }

  // Case 5b: admit = 0
  {
    Bench b;
    b.d.use_controller = 1;
    b.d.admit = 0;
    b.d.rampage_ok = 1;

    b.d.save_req = 1;
    b.tick();
    b.d.save_req = 0;

    check(b.d.refused, "refused not pulsed when admit == 0");
    check(!b.d.ctrl_hold, "ctrl_hold asserted when refused");

    b.tick();
    check(!b.d.refused, "refused pulse persisted past 1 clock");
  }

  // Case 5c: admitted request, then RAMpage leaves 3 before the boundary.
  // docs/b18-sna-save.md "Mapping admission": the dump covers the base 128K
  // only, so a mapping change between request and freeze must refuse at the
  // boundary rather than capture a header naming memory the file lacks.
  {
    Bench b;
    b.load(0x00, {0x00, 0x00, 0x00, 0x00, 0x76});  // NOP x4, HALT
    b.d.use_controller = 1;
    b.d.admit = 1;
    b.d.rampage_ok = 1;
    bool req_sent = false, refused_seen = false, captured_seen = false, hold_seen = false;
    b.stimulus = [&](Bench &bb) {
      if (!req_sent && bb.bounds.size() == 1 && !bb.d.insn_start) {
        bb.d.save_req = 1;
        req_sent = true;
      } else if (bb.d.save_req) {
        bb.d.save_req = 0;
        bb.d.rampage_ok = 0;  // armed; mapping changes before the next boundary
      }
      if (bb.d.refused) refused_seen = true;
      if (bb.d.ctrl_hold) hold_seen = true;
      if (bb.d.captured) {
        captured_seen = true;
        bb.d.release_req = 1;  // let a wrong freeze fail on the check below, not a timeout
      } else if (bb.d.release_req) {
        bb.d.release_req = 0;
      }
    };
    b.start();
    b.run_bounds(4);
    check(req_sent, "mapping-change request was not sent");
    check(refused_seen, "mapping change while armed was not refused at the boundary");
    check(!captured_seen && !hold_seen, "controller froze after the mapping changed");
  }
}

}  // namespace

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  std::cout << "=== B18 production-T80 freeze point ===\n";
  unsigned passes = 0, failures = 0;
  auto run_case = [&](const char *name, void (*fn)()) {
    std::cout << "[TEST] " << name << "... " << std::flush;
    try {
      fn();
      std::cout << "PASS\n";
      ++passes;
    } catch (const std::exception &e) {
      std::cout << "FAIL: " << e.what() << "\n";
      ++failures;
    }
  };
  run_case("deferred ALU writes", test_deferred_alu);
  run_case("EI exclusion", test_ei_exclusion);
  run_case("prefix families", test_prefixes);
  run_case("prefix-valued final opcodes", test_prefix_valued_opcodes);
  run_case("LD A,I P/V", test_ld_a_i);
  run_case("LD A,R value and P/V", test_ld_a_r);
  run_case("LDIR and DJNZ repeats", test_block_repeat);
  run_case("OTIR repeats and ports", test_otir);
  run_case("IM 1 interrupt", test_im1_interrupt);
  run_case("NMI and RETN", test_nmi_retn);
  run_case("EI;HALT with pending INT", test_ei_halt_pending_int);
  run_case("HALT wake by interrupt", test_halt_wake);
  run_case("hold versus hardware WAIT", test_hold_vs_wait);
  run_case("controller hold versus hardware WAIT", test_controller_hold_vs_wait);
  run_case("header CPU bytes and pattern", test_header_cpu_bytes);
  run_case("HALT PC and R adjustment", test_halt_freeze);
  run_case("postponement and cancel", test_postponement_and_cancel);
  run_case("save refusal on admit/rampage", test_refusal);
  std::cout << "Summary: " << passes << " passed, " << failures << " failed\n";
  return failures == 0 ? 0 : 1;
}
