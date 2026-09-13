// B18 slice 1: production-T80 snapshot freeze point (T80pa INSN_START).
//
// Design: docs/b18-sna-save.md, "Freeze point" and "Acceptance" item 1.
//
// At an INSN_START boundary, REG must hold the architectural state after the
// previous instruction, with PC the address of the instruction being fetched.
// Every expectation below comes from the program text and documented Z80
// semantics: Zilog Z80 CPU User Manual (UM0080) for instruction effects,
// interrupt modes, EI delay and RETN, and Sean Young, "The Undocumented Z80
// Documented", for flag bits 3/5 (copies of the result) and R increments
// (one per M1 cycle, so two for CB/ED/DD/FD-prefixed opcodes). None is read
// back from the simulator. Reset values (A=F=FF, SP=FFFF, I=R=0, IFF=0,
// IM 0) are the T80 reset assignments in rtl/T80/T80.vhd.
//
// Per-tick invariants (all tests): INSN_START only during an opcode fetch
// (M1, MREQ and RD low, address = REG.PC), never during an interrupt
// acknowledge (M1 and IORQ low).
#include "Vt80_freeze_top.h"
#include "verilated.h"

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
  uint16_t pc = 0, sp = 0, bc = 0, de = 0, hl = 0, ix = 0, iy = 0;
  uint8_t a = 0, f = 0, i = 0, r = 0, im = 0;
  bool iff1 = false, iff2 = false, halted = false;

  // Architectural identity for continuation checks (tick excluded).
  bool same_state(const Snap &o) const {
    return pc == o.pc && sp == o.sp && bc == o.bc && de == o.de && hl == o.hl &&
           ix == o.ix && iy == o.iy && a == o.a && f == o.f && i == o.i &&
           r == o.r && im == o.im && iff1 == o.iff1 && iff2 == o.iff2 &&
           halted == o.halted;
  }
};

std::string hex(unsigned v, int w) {
  std::ostringstream os;
  os << std::hex << std::uppercase << std::setfill('0') << std::setw(w) << v;
  return os.str();
}

std::string describe(const Snap &s) {
  return "PC=" + hex(s.pc, 4) + " SP=" + hex(s.sp, 4) + " A=" + hex(s.a, 2) +
         " F=" + hex(s.f, 2) + " BC=" + hex(s.bc, 4) + " DE=" + hex(s.de, 4) +
         " HL=" + hex(s.hl, 4) + " IX=" + hex(s.ix, 4) + " IY=" + hex(s.iy, 4) +
         " I=" + hex(s.i, 2) + " R=" + hex(s.r, 2) + " IM=" + std::to_string(s.im) +
         " IFF=" + std::to_string(s.iff1) + std::to_string(s.iff2) +
         (s.halted ? " HALT" : "");
}

[[noreturn]] void fail(const std::string &why) { throw std::runtime_error(why); }

void check(bool ok, const std::string &why) {
  if (!ok) fail(why);
}

struct Bench {
  Vt80_freeze_top d;
  uint64_t ticks = 0;
  bool prev_start = false;
  bool seen_refresh = false;
  std::vector<Snap> bounds;
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

  Snap snap() {
    Snap s;
    s.tick = ticks;
    s.a = field(0, 8);
    s.f = field(8, 8);
    s.i = field(32, 8);
    s.r = field(40, 8);
    s.sp = field(48, 16);
    s.pc = field(64, 16);
    s.bc = field(80, 16);
    s.de = field(96, 16);
    s.hl = field(112, 16);
    s.ix = field(128, 16);
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

// 4. LD A,I copies IFF2 into P/V (UM0080); its F must be final at the boundary.
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

// 5. Block repeat and DJNZ: each LDIR repeat refetches ED B0 and is a new
// instruction start; DJNZ taken branches likewise.
const std::vector<uint8_t> kBlockProgram = {
    0x21, 0x30, 0x00,  // 00: LD HL,0030h
    0x11, 0x40, 0x00,  // 03: LD DE,0040h
    0x01, 0x03, 0x00,  // 06: LD BC,0003h
    0xED, 0xB0,        // 09: LDIR (3 iterations)
    0x06, 0x02,        // 0B: LD B,02h
    0x10, 0xFE,        // 0D: DJNZ 0Dh (2 iterations)
    0x76,              // 0F: HALT
};

void load_block(Bench &b) {
  b.load(0x00, kBlockProgram);
  b.load(0x30, {0xA1, 0xB2, 0xC3});
}

void test_block_repeat() {
  Bench b;
  load_block(b);
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

// 6. IM 1 interrupt: acknowledge is not a boundary; ISR entry is, with the
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
  size_t k = 0;
  while (k < v.size() && v[k].pc != 0x38) ++k;
  check(k < v.size() && k > 0, "no ISR boundary at 0038h: [" + pcs(v) + "]");
  expect_pcs(v, {0x00, 0x03, 0x05, 0x07}, "IM1 prologue");
  expect_snap(v[k - 1], v[k - 1].pc == 0x07 && v[k - 1].iff1, "boundary before ISR");
  expect_snap(v[k], v[k].sp == 0x00EE && !v[k].iff1 && !v[k].iff2 && v[k].im == 1 && v[k].a == 0xFF,
              "ISR entry");
  check(b.peek(0xEE) == 0x07 && b.peek(0xEF) == 0x00, "pushed return address not 0007h");
  expect_snap(v[k + 1], v[k + 1].pc == 0x39 && v[k + 1].a == 0x00, "after INC A");
  expect_snap(v[k + 2], v[k + 2].pc == 0x07 && v[k + 2].sp == 0x00F0 && v[k + 2].iff1 && v[k + 2].iff2,
              "after EI;RET (no boundary at RET fetch)");
}

// 7. NMI and RETN: NMI entry at 0066h keeps IFF2 (UM0080); RETN restores IFF1
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
  size_t k = 0;
  while (v[k].pc != 0x66) ++k;
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

// 8. EI;HALT with an interrupt arriving while halted: the HALT fetch after EI
// is excluded, the halted fetch is a boundary with IFF1=1, and the ISR returns
// to the byte after HALT.
void test_ei_halt_interrupt() {
  Bench b;
  b.load(0x00, {
      0x31, 0xF0, 0x00,  // 00: LD SP,00F0h
      0xED, 0x56,        // 03: IM 1
      0xFB,              // 05: EI
      0x76,              // 06: HALT <- no boundary (EI)
      0x00,              // 07: NOP
      0x18, 0xFE,        // 08: JR 08h
  });
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
  expect_pcs(v, {0x00, 0x03, 0x05, 0x07}, "EI;HALT prologue");
  expect_snap(v[3], v[3].halted && v[3].iff1 && v[3].iff2, "halted fetch after EI;HALT");
  size_t k = 0;
  while (v[k].pc != 0x38) ++k;
  check(b.peek(0xEE) == 0x07 && b.peek(0xEF) == 0x00, "halted ISR return address not 0007h");
  expect_snap(v[k + 1], v[k + 1].pc == 0x07 && !v[k + 1].halted && v[k + 1].iff1,
              "after ISR: resumes after HALT");
}

// 9. Freeze continuation. A hold at a boundary must be equivalent to a
// hardware WAIT of the same length: same architectural boundaries as an
// uninterrupted run (no interrupts or time-dependent inputs here), and an
// identical post-release bus trace between hold and WAIT. Entry follows the
// production shape: the controller registers INSN_START, so the gate takes
// effect one clock after the rise, which must precede the negative enable
// that samples WAIT_n.
struct BusEvent {
  uint64_t rel;
  uint32_t word;
  bool operator==(const BusEvent &o) const { return rel == o.rel && word == o.word; }
};

uint32_t bus_word(Bench &b) {
  return (uint32_t(b.d.t80_a) << 8) | (b.d.t80_m1_n << 7) | (b.d.t80_mreq_n << 6) |
         (b.d.t80_iorq_n << 5) | (b.d.t80_rd_n << 4) | (b.d.t80_wr_n << 3) |
         (b.d.t80_rfsh_n << 2) | (b.d.t80_halt_n << 1);
}

struct StallRun {
  std::vector<Snap> bounds;
  std::vector<BusEvent> after;
};

StallRun run_stall(bool use_hold, size_t at_boundary, unsigned hold_ticks) {
  Bench b;
  load_block(b);
  StallRun out;
  enum { WAITING, ARMED, STALLED, RELEASED } phase = WAITING;
  uint64_t stall_start = 0, release = 0;
  uint32_t frozen_bus = 0;
  unsigned frozen_pc = 0;
  uint32_t last_bus = 0;
  bool have_last = false;

  b.stimulus = [&](Bench &bb) {
    switch (phase) {
      case WAITING:
        if (bb.bounds.size() == at_boundary + 1 && bb.d.insn_start) {
          check(!bb.d.phi_n, "negative enable coincides with INSN_START rise");
          phase = ARMED;  // registered controller: gate from the next clock
        }
        break;
      case ARMED:
        check(!bb.d.phi_n, "negative enable before registered hold takes effect");
        if (use_hold) bb.d.cpu_hold = 1; else bb.d.ext_wait = 1;
        stall_start = bb.ticks;
        frozen_bus = bus_word(bb);
        frozen_pc = bb.field(64, 16);
        phase = STALLED;
        break;
      case STALLED:
        if (use_hold) {
          check(bb.d.insn_start, "INSN_START dropped during hold");
          check(bus_word(bb) == frozen_bus, "bus moved during hold");
          check(bb.field(64, 16) == frozen_pc, "REG.PC moved during hold");
        }
        if (bb.ticks - stall_start >= hold_ticks) {
          bb.d.cpu_hold = 0;
          bb.d.ext_wait = 0;
          release = bb.ticks;
          phase = RELEASED;
        }
        break;
      case RELEASED: {
        uint32_t w = bus_word(bb);
        if (!have_last || w != last_bus) out.after.push_back({bb.ticks - release, w});
        last_bus = w;
        have_last = true;
        break;
      }
    }
  };
  b.start();
  b.run_bounds(11);
  check(phase == RELEASED, "stall never released");
  out.bounds = b.bounds;
  return out;
}

void test_hold_vs_wait() {
  Bench free_run;
  load_block(free_run);
  free_run.start();
  free_run.run_bounds(11);

  const unsigned kStall = 64 * 20;  // 20 GA sequencer cycles
  const size_t kAt = 4;             // second LDIR iteration
  StallRun held = run_stall(true, kAt, kStall);
  StallRun waited = run_stall(false, kAt, kStall);

  for (size_t k = 0; k < 11; ++k) {
    check(held.bounds[k].same_state(free_run.bounds[k]),
          "hold changed boundary " + std::to_string(k) + ": " + describe(held.bounds[k]) +
              " vs free " + describe(free_run.bounds[k]));
    check(waited.bounds[k].same_state(free_run.bounds[k]),
          "WAIT changed boundary " + std::to_string(k));
  }
  check(!held.after.empty() && held.after.size() == waited.after.size(),
        "post-release bus traces differ in length: hold " + std::to_string(held.after.size()) +
            " wait " + std::to_string(waited.after.size()));
  for (size_t k = 0; k < held.after.size(); ++k)
    check(held.after[k] == waited.after[k],
          "post-release bus event " + std::to_string(k) + " differs: hold rel=" +
              std::to_string(held.after[k].rel) + " word=" + hex(held.after[k].word, 6) +
              " wait rel=" + std::to_string(waited.after[k].rel) +
              " word=" + hex(waited.after[k].word, 6));
  for (size_t k = kAt + 1; k < 11; ++k)
    check(held.bounds[k].tick == waited.bounds[k].tick,
          "boundary " + std::to_string(k) + " timing differs between hold and WAIT");
  std::cout << "[obs: stall delay vs free run="
            << (held.bounds[kAt + 1].tick - free_run.bounds[kAt + 1].tick) << "t] " << std::flush;
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
  run_case("LD A,I P/V", test_ld_a_i);
  run_case("LDIR and DJNZ repeats", test_block_repeat);
  run_case("IM 1 interrupt", test_im1_interrupt);
  run_case("NMI and RETN", test_nmi_retn);
  run_case("EI;HALT with interrupt", test_ei_halt_interrupt);
  run_case("hold versus hardware WAIT", test_hold_vs_wait);
  std::cout << "Summary: " << passes << " passed, " << failures << " failed\n";
  return failures == 0 ? 0 : 1;
}
