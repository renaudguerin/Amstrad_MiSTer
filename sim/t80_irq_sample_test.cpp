// T80 maskable-INT sampling edge (Zilog UM008011-0816, p12 and Fig9 p13,
// via docs/plus/pa7-interrupt-phase-followup.md "Sampling-edge diagnosis").
//
// Documented rule: INT is sampled at the rising edge that begins the final
// T-state of the last machine cycle, one CPU clock before the next M1.
// The T80 under test clocked the decision on live INT_n at T_Res, i.e. on
// the edge leaving the last T-state, one clock late.
//
// Method: every pulse window is placed against E_next, the master tick of
// the next M1_n fall after the target fetch. M1_n is a CPU pin, not the
// core's internal TState/MCycle counters, and E_next is measured in a
// calibration run with INT held high, so the anchor cannot mirror the
// implementation's own T-count. With the 16-tick CPU period used here,
// E_doc = E_next - 16 is the documented sample edge. Windows give eight
// master ticks of setup and hold around the edge they must (or must not)
// cover:
//
//   held   low [E_doc-8, E_next+64]   spans both edges      expect ACK
//   early  low [E_next-24, E_next-8]  spans E_doc only      expect ACK
//   late   low [E_next-8, E_next+64]  arrives after E_doc   expect no ACK
//
// A held-level case is the positive control for every target; early/late
// are the fail-before pair (pre-fix: early is missed, late is taken).
// ACK is the pin-level M1_n+IORQ_n cycle, watched for 256 ticks after
// E_next; "no ACK" cases guard the window with a following DI so a later
// boundary cannot produce a legitimate second ACK inside it.
//
// Boundaries (each a single preservation run, passing before and after the
// fix unless noted): multicycle LD A,(HL) pulse trio, WAIT-stretched RET
// pulse trio, EI inhibition, DD-prefix completion, HALT wake, IM2 vector,
// NMI-over-INT priority. Production snapshot restore is covered by
// b18-snapshot-test; this pin driver does not implement that restore protocol.

#include "Vt80_irq_sample_top.h"
#include "verilated.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr int kCpuPeriod = 16;   // master ticks per CPU clock (cp/cn grid)
constexpr int kResetTicks = 64;  // reset_n held low
constexpr int kGuardTicks = 256; // ignore bus events: T80pa never resets
                                 // IntCycleD_n, which can hold IORQ_n low
                                 // on the first M1 (see t80_freeze_test)

[[noreturn]] void fail(const std::string &why) { throw std::runtime_error(why); }

struct Bench {
  Vt80_irq_sample_top d;
  std::array<uint8_t, 65536> mem{};
  long t = 0;
  bool pm1 = true;   // M1_n starts high after reset in Verilator init
  bool pack = false;
  long guard = kGuardTicks;  // T80_IRQ_TRACE=1 records bus events from t=0
  std::vector<std::pair<long, uint16_t>> fetch;  // M1_n fall (tick, addr)
  std::vector<long> ack;  // first tick of each M1_n+IORQ_n overlap
  std::function<void(Bench &)> stimulus;

  Bench() {
    d.clk = 0;
    d.reset_n = 0;
    d.cp = 0;
    d.cn = 0;
    d.wait_n = 1;
    d.irq_n = 1;
    d.nmi_n = 1;
    d.dirset = 0;
    for (int k = 0; k < 7; ++k) d.dir[k] = 0;
    d.din = 0xFF;
    if (std::getenv("T80_IRQ_TRACE")) guard = 0;
    d.eval();
  }

  void tick() {
    d.cp = (t % kCpuPeriod) == 0;
    d.cn = (t % kCpuPeriod) == 8;
    if (stimulus) stimulus(*this);
    d.clk = 0;
    d.eval();
    bool mem_rd = !d.mreqn && !d.rdn;
    if (mem_rd)
      d.din = mem[d.addr];
    else
      d.din = 0xFF;  // idle CPC bus byte sampled by the IM2 vector read or unmapped
    d.eval();
    d.clk = 1;
    d.eval();
    if (std::getenv("T80_IRQ_TRACE") && t < 320)
      printf("t=%ld cp=%d cn=%d m1=%d io=%d mr=%d rd=%d wr=%d a=%04x din=%02x dout=%02x\n",
             t, (int)d.cp, (int)d.cn, (int)d.m1n, (int)d.iorqn, (int)d.mreqn,
             (int)d.rdn, (int)d.wrn, (unsigned)d.addr, (unsigned)d.din,
             (unsigned)d.dout);
    if (!d.mreqn && !d.wrn) mem[d.addr] = d.dout;
    bool m1now = !d.m1n;
    if (m1now && !pm1 && t >= guard) fetch.emplace_back(t, d.addr);
    bool pins_ack = m1now && !d.iorqn && d.mreqn;
    if (pins_ack && !pack && t >= guard) ack.push_back(t);
    pack = pins_ack;
    pm1 = m1now;
    ++t;
  }

  void load(uint16_t at, const std::vector<uint8_t> &bytes) {
    for (size_t k = 0; k < bytes.size(); ++k) mem[at + k] = bytes[k];
  }

  void run(long limit = 20000) {
    for (long k = 0; k < limit; ++k) tick();
  }

  // Bounded event dump for failure details: first fetches and all ACKs.
  std::string dump() const {
    std::string s;
    size_t n = 0;
    for (auto [ft, fa] : fetch) {
      if (n++ >= 25) break;
      char buf[32];
      snprintf(buf, sizeof(buf), " F@%ld=%04x", ft, fa);
      s += buf;
    }
    for (long at : ack) s += " ACK@" + std::to_string(at);
    return s;
  }
};

// First M1 fall to `addr` at or after `from`, then the M1 fall after it.
std::pair<long, long> calibrate(Bench &b, uint16_t target, const char *what) {
  long tc = -1;
  for (auto [ft, fa] : b.fetch)
    if (fa == target) {
      tc = ft;
      break;
    }
  if (tc < 0) fail(std::string(what) + ": target fetch never seen");
  for (auto [ft, fa] : b.fetch)
    if (ft > tc) return {tc, ft};
  fail(std::string(what) + ": no M1 after target");
}

// Case A/B/C program builder: DI / LD SP,F000 / IM1, then body.
std::vector<uint8_t> prologue() { return {0xF3, 0x31, 0x00, 0xF0, 0xED, 0x56}; }

// IM1 return stub at 0038h: EI; RET.
void load_isr(Bench &b) { b.load(0x38, {0xFB, 0xC9}); }

struct Tally {
  unsigned pass = 0, fail = 0;
  void check(bool ok, const std::string &name, const std::string &detail = "") {
    std::cout << (ok ? "PASS " : "FAIL ") << name;
    if (!ok && !detail.empty()) std::cout << ": " << detail;
    std::cout << "\n";
    ok ? ++pass : ++fail;
  }
};

// Pulse-timing case on one target instruction. `body` starts at 0x06,
// `target` is the fetch address, `extra` pre-loads tables/operands.
void pulse_case(Tally &tally, const char *name, const std::vector<uint8_t> &body,
                uint16_t target,
                const std::function<void(Bench &)> &mem_setup = {},
                const std::function<void(Bench &, long tc)> &wait_setup = {}) {
  auto init = [&](Bench &b) {
    auto prog = prologue();
    prog.insert(prog.end(), body.begin(), body.end());
    b.load(0x00, prog);
    load_isr(b);
    if (mem_setup) mem_setup(b);
  };
  // Calibration: INT held high, identical WAIT. E_next is a pin event.
  Bench c;
  init(c);
  c.stimulus = [&](Bench &bb) {
    bb.d.reset_n = bb.t >= kResetTicks;
    long tc = -1;
    for (auto [ft, fa] : bb.fetch)
      if (fa == target) {
        tc = ft;
        break;
      }
    if (wait_setup && tc >= 0) wait_setup(bb, tc);
  };
  c.run();
  auto cal = calibrate(c, target, name);
  long tc = cal.first;
  long enext = cal.second;
  (void)tc;
  long edoc = enext - kCpuPeriod;

  struct Pulse {
    const char *kind;
    long lo, hi;  // irq_n low on [lo, hi], relative to enext
    bool expect_ack;
  };
  // Held spans both edges; early spans E_doc with 8 ticks of setup/hold
  // and releases 8 before E_next; late asserts 8 after E_doc.
  const Pulse pulses[] = {
      {"held", edoc - 8 - enext, 64, true},
      {"early", -24, -8, true},
      {"late", -8, 64, false},
  };
  for (auto p : pulses) {
    Bench b;
    init(b);
    b.stimulus = [&](Bench &bb) {
      bb.d.reset_n = bb.t >= kResetTicks;
      long cur = -1;
      for (auto [ft, fa] : bb.fetch)
        if (fa == target) {
          cur = ft;
          break;
        }
      if (wait_setup && cur >= 0) wait_setup(bb, cur);
      bb.d.irq_n = (bb.t >= enext + p.lo && bb.t <= enext + p.hi) ? 0 : 1;
    };
    b.run();
    bool seen = false;
    for (long at : b.ack)
      if (at >= enext && at <= enext + 256) seen = true;
    std::string casename =
        std::string(name) + "/" + p.kind + " (E_next=" + std::to_string(enext) + ")";
    if (p.expect_ack)
      tally.check(seen, casename, "no pin ACK after E_next");
    else
      tally.check(!seen, casename, "pin ACK taken on a pulse that arrives after E_doc");
  }
}

}  // namespace

int main(int argc, char **argv) {
  Verilated::commandArgs(argc, argv);
  Tally tally;
  std::cout << "=== T80 INT sampling edge (Zilog UM008011-0816 Fig9) ===\n";

  // A: single-M1 five-T RET NC (untaken, carry set by SCF), the PA7 probe
  // program. DI follows so a late pulse cannot ack at a later boundary.
  pulse_case(tally, "ret-nc", {0x37, 0xFB, 0x00, 0xD0, 0xF3, 0x18, 0xFE}, 0x09);

  // B: multicycle LD A,(HL) (M1 + 3T read). Same windows, new E_next.
  pulse_case(
      tally, "ld-a-hl", {0x21, 0x00, 0x20, 0xFB, 0x7E, 0xF3, 0x18, 0xFE}, 0x0A,
      [](Bench &b) { b.mem[0x2000] = 0x55; });

  // C: RET NC stretched by hardware WAIT across its M1/T2 (sampled at the
  // cn edge inside T2 by T80pa). Identical WAIT in calibration and test, so
  // E_next already includes the stretch; the latch must follow enabled
  // edges across the gap.
  {
    auto waitfn = [](Bench &bb, long tc) {
      bb.d.wait_n = (bb.t >= tc && bb.t < tc + 200) ? 0 : 1;
    };
    pulse_case(tally, "ret-nc-wait", {0x37, 0xFB, 0x00, 0xD0, 0xF3, 0x18, 0xFE},
               0x09, {}, waitfn);
  }

  // D: EI inhibition. INT held low from reset; the EI end (NOP1 fetch, f1)
  // must not ack, the first ACK belongs to the NOP1 end (f2's cycle).
  {
    Bench b;
    b.load(0x00, {0xF3, 0x31, 0x00, 0xF0, 0xED, 0x56, 0xFB, 0x00, 0x00, 0x18,
                  0xFE});
    load_isr(b);
    b.stimulus = [](Bench &bb) {
      bb.d.reset_n = bb.t >= kResetTicks;
      bb.d.irq_n = 0;
    };
    b.run();
    long f1 = -1, f2 = -1;
    for (auto [ft, fa] : b.fetch) {
      if (fa == 0x07 && f1 < 0) f1 = ft;
      if (fa == 0x08 && f2 < 0) f2 = ft;
    }
    bool ok_window = f1 >= 0 && f2 >= 0 && !b.ack.empty() && b.ack[0] >= f2;
    std::string detail = "f1=" + std::to_string(f1) + " f2=" + std::to_string(f2) +
                         " ack0=" + (b.ack.empty() ? std::string("none") : std::to_string(b.ack[0])) +
                         b.dump();
    tally.check(ok_window, "ei-inhibit", detail);
  }

  // E: DD-prefix completion. An ACK inside the prefixed instruction would
  // push a mid-instruction PC, so its ISR would return into an operand
  // byte: no M1 fetch of 09h/0Ah (operands) may occur, and no ACK may precede
  // completion (0Bh fetched).
  {
    Bench b;
    b.load(0x00, {0xF3, 0x31, 0x00, 0xF0, 0xED, 0x56, 0xFB, 0xDD, 0x21, 0x34,
                  0x12, 0x00, 0x18, 0xFE});
    load_isr(b);
    b.stimulus = [](Bench &bb) {
      bb.d.reset_n = bb.t >= kResetTicks;
      bb.d.irq_n = 0;
    };
    b.run();
    bool mid_fetch = false;
    long f_done = -1;
    for (auto [ft, fa] : b.fetch) {
      if (fa == 0x09 || fa == 0x0A) mid_fetch = true;
      if (fa == 0x0B && f_done < 0) f_done = ft;
    }
    bool ack_ok = !b.ack.empty() && f_done >= 0 && b.ack[0] >= f_done;
    tally.check(!mid_fetch && ack_ok, "prefix-complete",
                "mid_fetch=" + std::to_string(mid_fetch) +
                    " f_done=" + std::to_string(f_done) +
                    " ack0=" + (b.ack.empty() ? std::string("none") : std::to_string(b.ack[0])) +
                    b.dump());
  }

  // F: HALT wake. INT raised only once halted; ACK then resume past HALT.
  {
    Bench b;
    b.load(0x00, {0xF3, 0x31, 0x00, 0xF0, 0xED, 0x56, 0xFB, 0x00, 0x76, 0x00,
                  0x18, 0xFE});
    load_isr(b);
    long halted_at = -1;
    b.stimulus = [&](Bench &bb) {
      bb.d.reset_n = bb.t >= kResetTicks;
      if (halted_at < 0 && !bb.d.halt_n) halted_at = bb.t;
      bb.d.irq_n = (halted_at >= 0 && bb.t >= halted_at + 32) ? 0 : 1;
    };
    b.run();
    bool acked = !b.ack.empty() && b.ack[0] > halted_at;
    long resumed = -1;
    if (acked)
      for (auto [ft, fa] : b.fetch)
        if (ft > b.ack[0] && fa == 0x09) {
          resumed = ft;
          break;
        }
    tally.check(halted_at >= 0 && acked && resumed >= 0, "halt-wake",
                "no ACK in HALT or resume missed 0009h");
  }

  // G: IM2 vector. I=02h, bus idle FFh, table 02FFh -> 0040h handler.
  {
    Bench b;
    b.load(0x00, {0xF3, 0x31, 0x00, 0xF0, 0x3E, 0x02, 0xED, 0x47, 0xED, 0x5E,
                  0xFB, 0x00, 0x18, 0xFE});
    b.mem[0x02FF] = 0x40;
    b.mem[0x0300] = 0x00;
    b.load(0x40, {0xFB, 0xC9});
    b.stimulus = [](Bench &bb) {
      bb.d.reset_n = bb.t >= kResetTicks;
      bb.d.irq_n = 0;
    };
    b.run();
    long vec = -1;
    if (!b.ack.empty())
      for (auto [ft, fa] : b.fetch)
        if (ft > b.ack[0]) {
          vec = fa;
          break;
        }
    tally.check(vec == 0x40, "im2-vector",
                "vec=" + std::to_string(vec) +
                    " ack0=" + (b.ack.empty() ? std::string("none") : std::to_string(b.ack[0])));
  }

  // H: NMI priority over a held INT. Both asserted at instruction 0007h;
  // 0066h (NMI) must be fetched before 0038h (INT).
  {
    Bench b;
    b.load(0x00, {0xF3, 0x31, 0x00, 0xF0, 0xED, 0x56, 0xFB, 0x00, 0x18, 0xFE});
    b.load(0x38, {0xFB, 0xC9});
    b.load(0x66, {0xED, 0x45});
    long nmi_at = -1;
    b.stimulus = [&](Bench &bb) {
      bb.d.reset_n = bb.t >= kResetTicks;
      bb.d.irq_n = 0;
      if (nmi_at < 0) {
        for (auto [ft, fa] : bb.fetch)
          if (fa == 0x07) { nmi_at = bb.t; break; }
      }
      bb.d.nmi_n = (nmi_at >= 0 && bb.t >= nmi_at && bb.t < nmi_at + 48) ? 0 : 1;
    };
    b.run();
    long f66 = -1, f38 = -1;
    for (auto [ft, fa] : b.fetch) {
      if (fa == 0x66 && f66 < 0) f66 = ft;
      if (fa == 0x38 && f38 < 0) f38 = ft;
    }
    tally.check(f66 >= 0 && f38 >= 0 && f66 < f38, "nmi-priority",
                "f66=" + std::to_string(f66) + " f38=" + std::to_string(f38));
  }

  std::cout << "Summary: " << tally.pass << " passed, " << tally.fail << " failed\n";
  return tally.fail ? 1 : 0;
}
