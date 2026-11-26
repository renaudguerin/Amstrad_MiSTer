// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND). Scripted legal CPU bus phases, not T80 execution.
#include "Vcrtc_cpu_phase_top.h"
#include "verilated.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

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
  Bench(bool type = 1, unsigned reg = 5, unsigned r0 = 15, unsigned r6 = 3,
        bool video = false) {
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
    d.video_enable = video;
    d.video_data = 0;
    for (int i = 0; i < 5; i++)
      d.sna_regs[i] = 0;
    const unsigned char r[18] = {static_cast<unsigned char>(r0),
                                 8,
                                 10,
                                 0x22,
                                 3,
                                 0,
                                 static_cast<unsigned char>(r6),
                                 static_cast<unsigned char>(video ? 127 : 2),
                                 0,
                                 3,
                                 0,
                                 0,
                                 static_cast<unsigned char>(video ? 0x30 : 0x12),
                                 static_cast<unsigned char>(video ? 0 : 0x34),
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
  void write_pin(unsigned value, bool rs = true) {
    d.bus_rs = rs;
    d.write_data = value;
    d.direct_write = 1;
    tick();
    d.direct_write = 0;
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
  // Phase0 preserves existing behavior pending the OUTI chart-cell versus
  // physical-onset question; p127's OUT example does not establish this
  // phase's expectation.
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

void r6_zero_across_frame(unsigned phase, unsigned write_char, bool prior_equal) {
  Bench b(true, 6, 63, 50);
  b.pos(0, 0, 3);
  if (prior_equal) {
    b.write(phase, 4, false);
    b.run(64);
    b.release();
    b.write(phase, 0);
    b.run(64);
    b.release();
    b.write(phase, 6, false);
    b.run(64);
    b.release();
  }
  b.pos(prior_equal ? 0 : 3, 3, write_char);
  b.write(phase, 0);
  b.seam();
  b.release();
  b.pos(0, 0, 2);
  b.check(!b.d.de_out, "R6=0 must still blank display at frame origin");

  // French v1.11 §18.2.3 p190 and original 039C–039F. A zero
  // equality already seen on the preceding last character does
  // not become sticky again at the incoming origin.
  b.write(phase, 3);
  b.run(64);
  b.release();
  b.pos(0, 0, 7);
  b.check(bool(b.d.de_out) == prior_equal,
          "unequal R6 must reopen DE only after a carried zero equality");
}

void r6_zero_across_adjustment() {
  Bench b(true, 4, 15, 50);
  const unsigned phase = 0x0f;
  b.pos(0, 0, 3);
  b.write(phase, 127);
  b.release();
  b.write(phase, 5, false);
  b.release();
  b.write(phase, 1);
  b.release();
  b.write(phase, 6, false);
  b.release();
  b.pos(40, 0, 0);
  b.pos(80, 0, 0);
  b.pos(120, 0, 0);
  b.until([&] { return b.d.in_adj && b.d.row == 0 && b.d.c0 == 15; });
  b.write(phase, 0);
  b.seam();
  b.release();
  b.pos(0, 0, 2);
  b.check(!b.d.in_adj && !b.d.de_out,
          "the zero R6 write must blank the first character after adjustment");
  b.write(phase, 3);
  b.run(64);
  b.release();
  b.pos(0, 0, 7);
  b.check(b.d.de_out, "a carried zero equality at the end of adjustment must reopen DE");
}

// Original BU 021B opens a frozen CRTC0 display half just after S03.
// French v1.11 §17.6.2 p.188 and §18.3.2 p.191 describe the overlapping
// DE conditions. The caller replay supplies the byte aperture, while
// §9.1 p.47 and §9.3.4 pp.56–67 supply the Mode 2 bit and DAC mapping.
void r6_frozen_video(bool open_half) {
  Bench b(false, 0, 63, 13, true);
  // Two full 16-line frames without VSYNC let the GA leave blanking.
  b.pos(1, 0, 0);
  b.pos(0, 0, 0);
  b.write(0x0f, 0);
  b.release();
  b.write(0x0f, 6, false);
  b.release();
  b.d.bus_rs = 1;

  constexpr unsigned previous_byte = 0x80, tail_byte = 0x04;
  constexpr unsigned high_byte = 0xa1, following_byte = 0xf0;
  constexpr unsigned border = 62, ink0 = 42, ink1 = 13;
  constexpr unsigned dots_per_byte = 8, dots_per_word = 16;
  // In the original seam the first visible dot is twenty master
  // clocks (five Mode 2 dots) after the GA samples E0.
  constexpr unsigned first_pixel = 5;
  const std::array<unsigned, 3> even_bytes{previous_byte, tail_byte, following_byte};
  std::array<unsigned, 2 * dots_per_word + first_pixel + dots_per_byte> rgb{};
  unsigned even_fetches = 0, writes = 0;
  b.until([&] { return b.d.cclk_p; });
  for (unsigned dot = 0; dot < rgb.size(); ++dot) {
    b.until([&] { return b.d.ce16; });
    if (b.d.cclk_p)
      b.d.video_data = even_bytes.at(even_fetches++);
    if (b.d.cclk_n)
      b.d.video_data = high_byte;
    const bool write = open_half ? b.d.cclk_n && even_fetches == 1
                                 : b.d.cclk_p && even_fetches == 1;
    rgb[dot] = b.d.dac_rgb;
    b.tick();
    if (write) {
      b.check(b.d.c0 == 0 && b.d.row == 0 && b.d.line == 0 && b.d.ma == 0x3000,
              "R6 write must hit the frozen raster at C000");
      b.check(bool(b.d.de_out) == open_half && !b.d.ce16,
              "R6 write must start one master clock after the chosen CCLK edge");
      b.d.write_data = 0;
      b.d.direct_write = 1;
      b.tick();
      b.d.direct_write = 0;
      ++writes;
    }
  }
  b.check(even_fetches == even_bytes.size() && writes == 1,
          "fixture must cover three even fetches and one R6 write");
  for (unsigned dot = first_pixel; dot < rgb.size(); ++dot) {
    const unsigned offset = dot - first_pixel;
    const bool previous = offset < dots_per_byte;
    const bool tail = offset >= dots_per_word && offset < dots_per_word + dots_per_byte;
    unsigned expected = border;
    if (previous || (open_half && tail)) {
      const unsigned byte = previous ? previous_byte : tail_byte;
      expected = (byte & (0x80 >> (offset % dots_per_byte))) ? ink1 : ink0;
    }
    if (rgb[dot] != expected)
      throw std::runtime_error("DAC dot " + std::to_string(offset) + " was " +
                               std::to_string(rgb[dot]) + ", expected " +
                               std::to_string(expected));
  }
}

// SHAKER DH on a CRTC0 reaches C0=R2=6 a second time after eight
// characters with R0=7. Preserve that counter path and the production
// 64-clock character, then drive the observed data strobe at the pins.
void r3_terminal_collision(Bench& b, unsigned width, unsigned terminal_c0) {
  b.write_pin(2, false);
  b.write_pin(6);
  b.write_pin(3, false);
  b.write_pin(width);
  b.until([&] { return b.d.c0 == 6 && b.d.raw_hsync; });
  b.until([&] { return b.d.c0 == 7; });
  b.until([&] { return b.d.c0 == terminal_c0; });
  b.check(b.d.raw_hsync, "original pulse must still be high at terminal C0");
}

unsigned r3_terminal_late_write(Bench& b, bool cross_turnover, unsigned width) {
  r3_terminal_collision(b, cross_turnover ? 9 : 8, 6);
  b.check(b.d.c3 == 8, "C3 reaches eight at C0=R2=6");
  if (cross_turnover) {
    // Open the same aperture at the end of the character. The write
    // still precedes CCLK, but its scheduled rise comes after CCLK.
    b.run(50);
    b.write_pin(8);
  }
  b.tick();
  b.check(!b.d.raw_hsync, "old width must lower raw HSYNC at the terminal");
  const unsigned old_fall = b.ticks;
  b.run(7);
  b.write_pin(width);
  return old_fall;
}

void r3_terminal_late_restart(bool cross_turnover) {
  Bench b(false, 3, 7);
  // Original SHAKE27D PC 7AF0, page 0307.
  const unsigned old_fall = r3_terminal_late_write(b, cross_turnover, 11);
  b.until([&] { return b.d.raw_hsync; });

  // French ACCC v1.11 section 15.3.3 p.151 describes a restart about
  // 3.5 Mode 2 dots after the old fall. The original held replay and
  // its AmSpirit RGB edge constrain this bus phase to master ticks 15–18.
  const unsigned gap = b.ticks - old_fall;
  if (gap < 15 || gap > 18)
    throw std::runtime_error("late restart gap " + std::to_string(gap) +
                             " clocks, expected 15–18");
  b.check(b.d.c0 == (cross_turnover ? 7 : 6),
          "the restart must land in the expected character");
  b.check(b.d.c3 == (cross_turnover ? 9 : 8),
          "the restart gap must preserve the C3 count across CCLK");
  for (const auto& [c0, c3] : std::array<std::pair<unsigned, unsigned>, 3>{{
           {7, 9}, {0, 10}, {1, 11}}}) {
    b.until([&] { return b.d.c0 == c0; });
    b.check(b.d.raw_hsync && b.d.c3 == c3,
            "the restarted pulse must count through the original C3 sequence");
  }
  b.tick();
  b.check(!b.d.raw_hsync && b.d.c3 == 11,
          "the restarted pulse must fall at the new width");
}

void r3_terminal_late_expiry() {
  Bench b(false, 3, 7);
  r3_terminal_late_write(b, true, 9);
  b.until([&] { return b.d.c0 == 7; });
  b.check(!b.d.raw_hsync && b.d.c3 == 9,
          "the new terminal must be reached during the accepted low gap");

  // French ACCC v1.11 section 15.3.3 p.151 retains C3 through the gap.
  // The newly selected width is already exhausted before the scheduled
  // rise. Observe beyond that width and the next two character edges.
  for (unsigned tick = 0; tick < 2 * 64; ++tick) {
    b.tick();
    b.check(!b.d.raw_hsync, "an exhausted terminal restart must not resurrect");
  }
}

void r3_terminal_turnover() {
  Bench b(false, 3, 7);
  r3_terminal_collision(b, 9, 6);
  b.check(b.d.c3 == 8, "C3 must enter the collision character at eight");

  // Bring the old-width match late in this character so its falling
  // aperture is still live on the next CCLK. A data write on that CCLK
  // must not qualify the departing match of C0 and R2.
  b.run(56);
  b.write_pin(8);
  b.tick();
  b.check(!b.d.raw_hsync && b.d.c0 == 6,
          "old HSYNC must fall before turnover with C0 still equal to R2");
  const unsigned old_fall = b.ticks;
  b.until([&] { return b.d.cclk_n; });
  b.check(b.ticks - old_fall < 14, "turnover must land inside the fall aperture");
  b.write_pin(11);
  b.check(b.d.c0 == 7 && b.d.c3 == 0,
          "turnover must advance C0 and reset inactive C3");
  for (unsigned tick = 0; tick < 18; ++tick) {
    b.tick();
    b.check(!b.d.raw_hsync, "turnover cannot restart HSYNC from the old C0");
  }
}

void r3_terminal_inherited_fall() {
  Bench b(true, 3, 7);
  r3_terminal_collision(b, 7, 5);
  b.check(b.d.c3 == 7, "type 1 must reach its ordinary terminal width");
  b.tick();
  b.check(b.d.raw_hsync, "type 1 must defer its raw falling edge");

  b.write_pin(2, false);
  b.write_pin(5);
  b.check(b.d.raw_hsync, "old type 1 fall must still be pending at the new R2");
  b.d.crtc_type = 0;
  b.tick();
  b.check(b.d.raw_hsync, "pending phaseful pulse must survive the switch edge");
  b.until([&] { return !b.d.raw_hsync; });
  b.write_pin(3, false);
  b.run(6);
  b.write_pin(11);
  b.tick();
  b.check(b.d.raw_hsync,
          "inherited type 1 fall cannot delay the ordinary type 0 comparator");
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
    for (unsigned write_char : {62u, 63u}) {
      std::string aperture = s + " C0=" + std::to_string(write_char);
      test("R6 zero equality at frame boundary" + aperture,
           [&] { r6_zero_across_frame(phase, write_char, true); });
      test("R6 zero with no preceding equality" + aperture,
           [&] { r6_zero_across_frame(phase, write_char, false); });
    }
    test("R5 terminal C9" + s, [&] { r5_last_line(phase); });
  }
  test("R6 zero equality at adjustment frame boundary", r6_zero_across_adjustment);
  test("frozen R6 write completes the open Mode 2 byte", [] { r6_frozen_video(true); });
  test("frozen R6 write in the closed half leaves subsequent bytes in border",
       [] { r6_frozen_video(false); });
  test("type0 R3 late terminal restart (SHAKER DH 0307)",
       [] { r3_terminal_late_restart(false); });
  test("type0 R3 accepted restart preserves C3 across CCLK",
       [] { r3_terminal_late_restart(true); });
  test("type0 R3 terminal expires before its delayed rise", r3_terminal_late_expiry);
  test("type0 R3 terminal write on CCLK turnover", r3_terminal_turnover);
  test("type1 deferred fall after switching to type0", r3_terminal_inherited_fall);
  std::cout << failures << " failures\n";
  return failures ? 1 : 0;
}
