// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND). Scripted legal CPU bus phases, not T80 execution.
#include "Vcrtc_cpu_phase_top.h"
#include "verilated.h"
#include <iostream>
#include <stdexcept>
#include <string>

struct Bench {
  Vcrtc_cpu_phase_top d;
  unsigned ticks = 0, arm_edges = 0;
  void eval() {
    d.clk = 0;
    d.eval();
  }
  void tick() {
    eval();
    if (d.arm)
      ++arm_edges;
    d.clk = 1;
    d.eval();
    ++ticks;
    eval();
  }
  void run(unsigned n) {
    while (n--)
      tick();
  }
  template <class F> void until(F f) {
    for (unsigned n = 0; n < 200000; n++) {
      eval();
      if (f())
        return;
      tick();
    }
    throw std::runtime_error("timeout");
  }
  void check(bool ok, const char *why) {
    if (!ok)
      throw std::runtime_error(why);
  }
  Bench(bool type = 1, unsigned reg = 5, unsigned r0 = 15, unsigned r6 = 3) {
    d.reset = 1;
    d.sna_load = 1;
    d.launch = 0;
    d.release_bus = 0;
    d.direct_write = 0;
    d.launch_s = 0;
    d.crtc_type = type;
    d.bus_rs = 1;
    d.sna_addr = reg;
    d.write_data = 1;
    for (int i = 0; i < 5; i++)
      d.sna_regs[i] = 0;
    const unsigned char r[18] = {static_cast<unsigned char>(r0),
                                 8,
                                 10,
                                 0x22,
                                 3,
                                 0,
                                 static_cast<unsigned char>(r6),
                                 2,
                                 0,
                                 3,
                                 0,
                                 0,
                                 0x12,
                                 0x34,
                                 0,
                                 0,
                                 0,
                                 0};
    for (unsigned i = 0; i < 18; i++)
      d.sna_regs[i / 4] |= unsigned(r[i]) << (8 * (i % 4));
    run(128);
    d.sna_load = 0;
    d.reset = 0;
    until([&] { return d.row == 2; });
    until([&] { return d.row == 0; });
  }
  void pos(unsigned row, unsigned line, unsigned c0) {
    until([&] { return d.row == row && d.line == line && d.c0 == c0; });
  }
  void write(unsigned phase, unsigned value, bool rs = true) {
    d.write_data = value;
    d.bus_rs = rs;
    d.launch_s = phase;
    d.launch = 1;
    until([&] { return d.phi_p && d.seq == phase; });
    tick();
    d.launch = 0;
    check(d.ce16 == 0 && !d.cclk_n,
          "CPU outputs must launch after enable, before register capture");
    tick(); // first capture is a system edge, without CRTC enable
  }
  void release() {
    d.release_bus = 1;
    tick();
    d.release_bus = 0;
    tick();
  }
  void seam() {
    until([&] { return d.cclk_n; });
    tick();
  }
};

void r5(unsigned phase, bool direct = false, bool off = false) {
  Bench b;
  b.pos(1, 0, off ? 14 : 15);
  if (direct) {
    b.until([&] { return b.d.cclk_n; });
    b.d.direct_write = 1;
    b.eval();
    b.tick();
  } else {
    b.write(phase, 1);
    b.seam();
  }
  // French v1.11 §11.6 pp89-90: only R5 0->nonzero at C0=R0 arms;
  // C9!=R9 selects R12/R13 at line origin. R1=8 previously saved 1234+8.
  if (off) {
    b.seam();
  }
  b.check(b.d.c0 == 0 && b.d.row == 1 && b.d.line == 1,
          "ordinary counters must survive RFD");
  b.check(b.d.vma_flag == !off && b.d.parity_flag == !off,
          "R5 RFD flags lost at production phase");
  b.check(b.d.ma == (off ? 0x123c : 0x1234),
          "RFD must replace saved MA with R12/R13");
  b.check(b.arm_edges == unsigned(!off), "held bus must arm exactly once");
  b.run(64);
  b.check(b.arm_edges == unsigned(!off),
          "held write must not re-arm next character");
}
void r0_type1(unsigned phase, bool cancel) {
  Bench b(true, 0);
  b.pos(3, 3, 15);
  b.write(phase, 63);
  b.seam();
  // French §13.7.1.2 p126: widening true last line retains a window;
  // only a cancelled R9/R4 equality at the actual extended end arms RFD.
  b.check(b.d.c0 == 16 && b.d.row == 3 && b.d.line == 3,
          "R0 must extend the last line");
  b.check(b.d.pending && !b.d.parity_flag,
          "R0 widening window lost at production phase");
  b.release();
  if (cancel) {
    b.write(phase, 9, false);
    b.release();
    b.write(phase, 5);
    b.release();
  }
  b.pos(3, 3, 63);
  b.seam();
  b.check(!b.d.pending, "extended line end must consume R0 window");
  b.check(b.d.parity_flag == cancel && b.d.vma_flag == cancel,
          "R0 cancellation must decide RFD at extended end");
  b.check(b.d.row == (cancel ? 3 : 0) && b.d.line == (cancel ? 4 : 0),
          "R0 cancellation counter semantics");
  b.check(b.d.ma == 0x1234, "R0 route must reload start address");
}
void r0_type0(unsigned phase, bool safe) {
  Bench b(false, 0, 1);
  b.pos(3, 3, safe ? 0 : 1);
  b.write(phase, 15);
  b.seam();
  // French §13.7.2 pp126-128: unsafe C0=1 old equality survives widening;
  // by C0=2 C4=R4+1, C9 retained, and R5=0 still counts adjustment to 31.
  if (!safe) {
    b.check(b.d.c0 == 2,
            "type0 widening must not insert a duplicate C0=1 character");
    b.check(b.d.row == 4 && b.d.line == 3 && b.d.in_adj,
            "type0 old R0 equality/adjustment lost");
    b.pos(4, 3, 15);
    b.seam();
    b.check(b.d.line == 4 && b.d.in_adj,
            "R5=0 must continue additional management");
  } else {
    b.pos(3, 3, 15);
    b.seam();
    b.check(b.d.row == 0 && b.d.line == 0 && !b.d.in_adj,
            "safe C0=0 widening must restart frame");
  }
}
void r6_sticky(unsigned phase) {
  Bench b(true, 6, 15, 1);
  b.pos(1, 1, 4);
  b.check(!b.d.vde && !b.d.vde_r, "R6 equality must close the row before test");
  b.write(phase, 3);
  b.run(128);
  // French §18.2.3 p190: C4=R6 border is definitive until frame origin.
  // Do not preserve old-R6 reopening predicate as part of the RFD repair.
  b.check(!b.d.vde && !b.d.vde_r, "ordinary R6 border must remain sticky");
}
// F17 / French §11.6.1 p90: an RFD on C9=R9 arms parity while
// disabling the start-address source; ordinary row advance/save still wins.
void r5_last_line(unsigned phase) {
  Bench b;
  b.pos(1, 3, 15);
  b.write(phase, 1);
  b.seam();
  b.check(b.d.row == 2 && b.d.line == 0 && !b.d.vma_flag && b.d.parity_flag,
          "terminal-C9 RFD must disable source while arming parity");
  b.check(b.d.ma == 0x1244,
          "terminal-C9 RFD must use normally saved next-row MA");
}
// Cross-module lifecycle control: an accepted bus event must not outlive
// the selected engine or reset/snapshot ownership change before consumption.
void lifecycle(unsigned route, unsigned kind) {
  bool type = route != 2;
  Bench b(type, route == 0 ? 5 : 0, route == 2 ? 1 : 15);
  b.pos(route == 0 ? 1 : 3, route == 0 ? 0 : 3, route == 2 ? 1 : 15);
  b.write(0x0f, route == 0 ? 1 : 63);
  b.release();
  if (kind == 0) {
    b.d.crtc_type = !type;
    b.tick();
    b.d.crtc_type = type;
  } else if (kind == 1) {
    b.d.reset = 1;
    b.tick();
    b.d.reset = 0;
  } else {
    b.d.sna_load = 1;
    b.tick();
    b.d.sna_load = 0;
  }
  b.seam();
  if (route == 0)
    b.check(!b.d.vma_flag && !b.d.parity_flag,
            "discarded R5 write must not arm RFD");
  else if (route == 1)
    b.check(!b.d.pending && !b.d.parity_flag,
            "discarded R0 write must not open RFD window");
  else
    b.check(b.d.row == (kind == 1 ? 0 : 3),
            "discarded type0 write must not increment C4");
}
void r6_temporary(unsigned phase) {
  Bench b(true, 6);
  b.pos(1, 1, 4);
  b.check(b.d.vde && b.d.vde_r, "temporary R6 fixture starts displayed");
  b.write(phase, 0);
  b.run(64);
  b.release();
  b.check(!b.d.vde && b.d.vde_r,
          "R6=0 outside row0 must blank without closing sticky gate");
  b.write(phase, 3);
  b.run(64);
  // French §18.2.3 p190: R6=0 outside row0 is temporary, unlike equality.
  b.check(b.d.vde && b.d.vde_r, "R6 nonzero must remove temporary border");
}
int main() {
  unsigned failures = 0;
  auto test = [&](const std::string &name, auto fn) {
    try {
      fn();
      std::cout << "PASS " << name << "\n";
    } catch (const std::exception &e) {
      ++failures;
      std::cerr << "FAIL " << name << ": " << e.what() << "\n";
    }
  };
  test("R5 direct-CLKEN control", [] { r5(0, true); });
  for (unsigned route = 0; route < 3; route++)
    for (unsigned kind = 0; kind < (route == 2 ? 2u : 3u); kind++)
      test("event lifecycle route=" + std::to_string(route) +
               " kind=" + std::to_string(kind),
           [&] { lifecycle(route, kind); });
  for (unsigned phase : {0x0f, 0xff, 0xf0, 0x00}) {
    std::string s = " phase=" + std::to_string(phase);
    test("R5 RFD" + s, [&] { r5(phase); });
    test("R5 off-terminal held control" + s, [&] { r5(phase, false, true); });
    test("R0 type1 cancellation" + s, [&] { r0_type1(phase, true); });
    test("R0 type1 no-cancel" + s, [&] { r0_type1(phase, false); });
    test("R0 type0 unsafe" + s, [&] { r0_type0(phase, false); });
    test("R0 type0 safe" + s, [&] { r0_type0(phase, true); });
    test("R6 sticky border" + s, [&] { r6_sticky(phase); });
    test("R6 temporary border" + s, [&] { r6_temporary(phase); });
    test("R5 terminal C9" + s, [&] { r5_last_line(phase); });
  }
  std::cout << failures << " failures\n";
  return failures ? 1 : 0;
}
