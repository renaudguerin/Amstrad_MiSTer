// Authentic SHAKER execution; software arithmetic checks, no hardware oracle.
#include "Vb9_t80_top.h"
#include "verilated.h"
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
static Vb9_t80_top d;
static uint64_t ticks;
static void tick() {
  d.clk = 0;
  d.eval();
  d.clk = 1;
  d.eval();
  ++ticks;
  d.clk = 0;
  d.eval();
}
static void put(uint16_t a, uint8_t v) {
  d.prog_we = 1;
  d.prog_addr = a;
  d.prog_data = v;
  tick();
  d.prog_we = 0;
  tick();
}
static uint8_t peek(uint16_t a) {
  d.peek_addr = a;
  d.eval();
  return d.peek_data;
}
static unsigned reg(unsigned lo, unsigned n = 16) {
  unsigned v = 0;
  for (unsigned b = 0; b < n; ++b)
    v |= ((d.reg_state[(lo + b) / 32] >> ((lo + b) % 32)) & 1) << b;
  return v;
}
static void require(bool v, const char *s) {
  if (!v)
    throw std::runtime_error(s);
}
static unsigned hexword(uint16_t a) {
  unsigned v = 0;
  for (unsigned i = 0; i < 4; ++i) {
    unsigned c = peek(a + i);
    require((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'),
            "non-hex formatter output");
    v = v * 16 + (c <= '9' ? c - '0' : c - 'A' + 10);
  }
  return v;
}
struct State {
  unsigned c0, c9, c4, stage, ivm, pf, pc9, ln, a, b, poke, bit, ce, r8, wr,
      addr, data;
};
static State state() {
  return {d.c0,           d.line,       d.row,
          d.tog_stage,    d.ivm,        d.parity_frame_o,
          d.parity_c9_o,  d.line_new_o, d.stage_a_edge,
          d.stage_b_edge, d.line_poke,  d.line_poke_bit,
          d.cclk_n,       d.r8_il,      d.r8_write_hit_o,
          d.t80_a,        d.t80_dout};
}
static void trace(const char *what, unsigned p, const State &s) {
  printf(
      "%s case=%u tick=%llu C0=%02X C9=%u C4=%u stage=%u ivm=%u PF=%u PC9=%u "
      "CLKEN=%u line_new=%u A=%u B=%u poke=%u/%u R8=%u bus=%04X/%02X\n",
      what, p, (unsigned long long)ticks, s.c0, s.c9, s.c4, s.stage, s.ivm,
      s.pf, s.pc9, s.ce, s.ln, s.a, s.b, s.poke, s.bit, s.r8, s.addr, s.data);
}
int main(int argc, char **argv) {
  setvbuf(stdout, nullptr, _IOLBF, 0);
  try {
    require(argc == 2, "usage: b9_t80_tests <header-stripped SHAKE27B.BIN>");
    std::ifstream f(argv[1], std::ios::binary);
    require(bool(f), "cannot open private payload");
    std::vector<uint8_t> img((std::istreambuf_iterator<char>(f)), {});
    require(img.size() == 26683, "payload size mismatch");
    // run.sh also verifies the complete SHA-256 before building/executing.
    for (auto a : {0x92ac, 0x92cd, 0x92d7, 0x92fe, 0x9307, 0x932b}) {
      unsigned expected = a == 0x92ac                    ? 0xed
                          : a == 0x932b                  ? 0xc9
                          : (a == 0x92cd || a == 0x92fe) ? 0x23
                                                         : 0x29;
      require(img[a - 0x3900] == expected,
              "payload instruction anchor mismatch");
    }
    d.reset = 1;
    d.cpu_reset = 1;
    d.sna_load = 1;
    d.crtc_type = 1;
    d.sna_addr = 8;
    const uint8_t rs[18] = {63, 40, 46, 0x8e, 38,   0, 25, 0, 0,
                            7,  0,  0,  0x12, 0x34, 0, 0,  0, 0};
    for (unsigned i = 0; i < 5; ++i)
      d.sna_regs[i] = 0;
    for (unsigned i = 0; i < 18; ++i)
      d.sna_regs[i / 4] |= unsigned(rs[i]) << (8 * (i % 4));
    for (unsigned i = 0; i < 128; ++i)
      tick();
    d.sna_load = 0;
    d.reset = 0;
    for (unsigned i = 0; i < img.size(); ++i)
      put(0x3900 + i, img[i]);
    // Full page-B caller: setup, three update-delay blocks, then four MID
    // cases. Only the reset vector and type detector byte are fixture-injected.
    put(0, 0x31); // LD SP,4065; JP 8E5A
    put(1, 0x65);
    put(2, 0x40);
    put(3, 0xc3);
    put(4, 0x5a);
    put(5, 0x8e);
    put(0xbf03, 1);
    auto settle = [&](unsigned row) {
      uint64_t end = ticks + 4000000;
      while (d.row != row && ticks < end)
        tick();
      require(d.row == row, "frame settlement timeout");
    };
    settle(2);
    settle(0);
    d.cpu_reset = 0;
    printf(
        "ENTRY SP=4065 JP 8E5A type=1 tick=%llu; scale=64 ticks/us,4096 ticks/line\n",
        (unsigned long long)ticks);
    unsigned cases = 0, done = 0, raw[2] = {}, loops[2] = {}, writes = 0,
             stages = 0;
    bool entered_payload = false;
    bool active = false, armed = false, was_fetch = false, prev_vs = d.vs_o,
         window = false;
    uint64_t last_vs = 0, prev_rise = 0, write_tick = 0, fetch_tick = 0;
    uint16_t last_pc = 0;
    const unsigned entries[4] = {0x9266, 0x926b, 0x9271, 0x9277};
    const unsigned pair[4][2] = {
        {0x32, 0x32}, {0x7f, 0x32}, {0x32, 0x7f}, {0x7f, 0x7f}};
    const uint64_t cap = ticks + 400000000;
    while (done < 4 && ticks < cap) {
      State pre = state();
      bool accepted = armed && pre.wr && pre.data == 3 && pre.r8 != 3;
      bool pulse = window && (pre.a || pre.b);
      if (accepted) {
        trace("R8_ACCEPT_PRE", cases, pre);
        ++writes;
        write_tick = ticks + 1;
        window = true;
      }
      if (pulse) {
        trace("STAGE_PRE", cases, pre);
        ++stages;
      }
      tick();
      if (accepted) {
        trace("R8_ACCEPT_POST", cases, state());
        printf("WRITE_FETCH_DELTA ticks=%llu\n",
               (unsigned long long)(write_tick - fetch_tick));
        armed = false;
      }
      if (pulse)
        trace("STAGE_POST", cases, state());
      if (window && ticks > write_tick + 256)
        window = false;
      if (d.vs_o && !prev_vs) {
        prev_rise = last_vs;
        last_vs = ticks;
        if (active)
          printf("VSYNC case=%u tick=%llu interval_ticks=%llu us=%.3f "
                 "lines=%.4f\n",
                 cases, (unsigned long long)ticks,
                 (unsigned long long)(last_vs - prev_rise),
                 (last_vs - prev_rise) / 64.0, (last_vs - prev_rise) / 4096.0);
      }
      prev_vs = d.vs_o;
      bool fetch = !d.t80_m1_n && !d.t80_mreq_n && !d.t80_rd_n;
      if (fetch && (!was_fetch || d.t80_a != last_pc)) {
        unsigned pc = d.t80_a;
        require(!(entered_payload && pc == 0), "unexpected reset-stub re-entry");
        if (pc == 0x8e5a)
          entered_payload = true;
        if (cases < 4 && pc == entries[cases]) {
          ++cases;
          active = true;
          raw[0] = raw[1] = loops[0] = loops[1] = writes = stages = 0;
          printf("CASE_BEGIN n=%u entry=%04X R6=%02X/%02X tick=%llu\n", cases,
                 pc, pair[cases - 1][0], pair[cases - 1][1],
                 (unsigned long long)ticks);
        }
        if (active) {
          if (pc == 0x92ac) {
            armed = true;
            fetch_tick = ticks;
            trace("R8_FETCH", cases, state());
          }
          if (pc == 0x92cd || pc == 0x92fe) {
            unsigned n = pc == 0x92fe;
            if (!loops[n])
              printf("LOOP_BEGIN case=%u pc=%04X tick=%llu HL=%04X\n", cases,
                     pc, (unsigned long long)ticks, reg(112));
            ++loops[n];
          }
          if (pc == 0x92d7 || pc == 0x9307) {
            unsigned n = pc == 0x9307;
            raw[n] = reg(112);
            printf("LOOP_END case=%u pc=%04X tick=%llu HL=%04X iterations=%u\n",
                   cases, pc, (unsigned long long)ticks, raw[n], loops[n]);
            require(raw[n] == loops[n], "HL/loop count mismatch");
          }
          if (pc == 0x92ec) {
            unsigned expected = (0x5228 - (raw[0] * 16 + 0x464)) & 65535;
            printf("DELAY case=%u DE=%04X expected=%04X\n", cases, reg(96),
                   expected);
            require(reg(96) == expected,
                    "intervening delay arithmetic mismatch");
          }
          if (pc == 0x9312 || pc == 0x931f)
            // REG can expose transitional DE writeback at these fetches.
            // Destinations are verified through the completed RAM buffers.
            printf("FORMAT_FETCH case=%u pc=%04X observed_HL=%04X\n", cases, pc,
                   reg(112));
          if (pc == 0x932b) {
            unsigned mid = hexword(0x9053), total = hexword(0x902d);
            require(writes == 1 && stages == 2,
                    "missing R8 acceptance/stage completion");
            require(raw[0] && raw[1], "missing count-loop completion");
            require(peek(0x92be) == pair[cases - 1][0] &&
                        peek(0x92f5) == pair[cases - 1][1],
                    "R6 pair mismatch");
            require(mid == ((raw[0] * 16 + 0x470) & 65535),
                    "MID formatter arithmetic mismatch");
            require(total == ((raw[1] * 16 + 0x5270) & 65535),
                    "total formatter arithmetic mismatch");
            printf("CASE_DONE n=%u R6=%02X/%02X raw_HL=%04X/%04X MID=%04X "
                   "TOTAL=%04X last_VSYNC_ticks=%llu\n",
                   cases, peek(0x92be), peek(0x92f5), raw[0], raw[1], mid,
                   total, (unsigned long long)(last_vs - prev_rise));
            ++done;
            active = false;
          }
        }
        last_pc = pc;
      }
      was_fetch = fetch;
      if (ticks % 50000000 == 0)
        printf("PROGRESS tick=%llu cases=%u done=%u last_M1=%04X debug_SP=%04X HALT_n=%u\n",
               (unsigned long long)ticks, cases, done, last_pc, reg(48), unsigned(d.t80_halt_n));
    }
    require(done == 4, "bounded replay did not complete all four MID cases");
    printf("SUCCESS: four MID cases completed; software arithmetic verified; "
           "hardware reference 4E40/4F40 unresolved\n");
    return 0;
  } catch (const std::exception &e) {
    fprintf(stderr, "FAIL tick=%llu: %s\n", (unsigned long long)ticks,
            e.what());
    return 1;
  }
}
