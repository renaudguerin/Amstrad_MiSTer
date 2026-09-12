// SSM marker detector vectors (backlog B4, phase 1).
//
// Every expected code below is derived on paper from SSM v1.1
// (docs/references/SSM-STANDARD-EN.pdf) and cited at the assertion. None is
// read back out of the simulator.
//
// Two drivers, one production ssm_marker:
//
//   * a synthetic opcode-fetch driver, which is the only way to stretch a
//     fetch to an arbitrary number of wait states on demand and to present
//     byte streams no assembler would emit;
//   * a real Z80 executing bytes from RAM through the production GA divider
//     and WAIT, which is where the unknowns live: whether an undefined-ED
//     instruction really presents both bytes as opcode fetches, and what the
//     fetch stream looks like when an interrupt lands between two pairs.

#include <verilated.h>
#include "Vssm_marker_top.h"

#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[noreturn]] void fail(const std::string& message) {
    throw TestFailure(message);
}

template <typename A, typename B>
void expect_eq(const A& got, const B& want, const std::string& what) {
    if (static_cast<uint64_t>(got) != static_cast<uint64_t>(want)) {
        std::ostringstream os;
        os << what << ": got 0x" << std::hex << static_cast<uint64_t>(got)
           << ", expected 0x" << static_cast<uint64_t>(want);
        fail(os.str());
    }
}

// Ring layout from rtl/ssm_marker.v. Word indices are relative to the ring
// base; the harness's DDR model is indexed the same way.
constexpr unsigned kWordMagic = 0;
constexpr unsigned kWordHeader = 1;
constexpr unsigned kWordRecord0 = 2;
constexpr uint32_t kMagic = 0x53534D31u;  // "SSM1"
constexpr unsigned kRingEntries = 64;

class Harness {
public:
    Harness() : dut_(std::make_unique<Vssm_marker_top>()) {
        dut_->clk = 0;
        dut_->reset = 1;
        dut_->enable = 1;
        dut_->use_cpu = 0;
        dut_->m1_fetch_in = 0;
        dut_->bus_data_in = 0;
        dut_->cpu_reset = 1;
        dut_->int_n = 1;
        dut_->prog_we = 0;
        dut_->prog_addr = 0;
        dut_->prog_data = 0;
        dut_->ddr_stall = 0;
        dut_->peek_word = 0;
        dut_->eval();
        tick(8);
        dut_->reset = 0;
        tick(4);
    }

    void tick(unsigned n = 1) {
        for (unsigned i = 0; i < n; ++i) {
            dut_->clk = 1;
            dut_->eval();
            dut_->clk = 0;
            dut_->eval();
        }
    }

    Vssm_marker_top* operator->() { return dut_.get(); }

    // One synthetic opcode fetch: hold the motherboard's level for `hold`
    // clocks, then release it. `hold` > 1 is a wait-stated fetch.
    void fetch(uint8_t byte, unsigned hold = 2) {
        dut_->bus_data_in = byte;
        dut_->m1_fetch_in = 1;
        tick(hold);
        dut_->m1_fetch_in = 0;
        tick(3);
    }

    void fetch_all(std::initializer_list<uint8_t> bytes, unsigned hold = 2) {
        for (uint8_t b : bytes) fetch(b, hold);
    }

    void load(unsigned addr, std::initializer_list<uint8_t> bytes) {
        dut_->prog_we = 1;
        for (uint8_t b : bytes) {
            dut_->prog_addr = static_cast<uint8_t>(addr++);
            dut_->prog_data = b;
            tick();
        }
        dut_->prog_we = 0;
        tick();
    }

    uint64_t peek(unsigned word) {
        dut_->peek_word = word;
        tick(2);
        return dut_->peek_data;
    }

    // Run the executing CPU until it halts, or fail.
    void run_cpu(unsigned max_clocks = 200000) {
        dut_->use_cpu = 1;
        dut_->cpu_reset = 0;
        for (unsigned i = 0; i < max_clocks; ++i) {
            tick();
            if (!dut_->cpu_halt_n) return;
        }
        fail("CPU did not reach HALT within the clock budget");
    }

    void restart() {
        dut_->reset = 1;
        dut_->use_cpu = 0;
        dut_->cpu_reset = 1;
        dut_->int_n = 1;
        tick(8);
        dut_->reset = 0;
        tick(4);
    }

private:
    std::unique_ptr<Vssm_marker_top> dut_;
};

// --- synthetic byte-stream vectors -----------------------------------------

void test_reserved_screenshot_code() {
    Harness h;
    // SSM v1.1, "SSM CODES": #FFFE is #ED #FE #ED #FF, and the code is
    // HH*256+LL, so LL=#FE and HH=#FF give #FFFE.
    h.fetch_all({0xED, 0xFE, 0xED, 0xFF});
    expect_eq(h->event_count, 1, "FFFE event count");
    expect_eq(h->last_code, 0xFFFE, "FFFE code");
}

void test_reserved_sync_code() {
    Harness h;
    // SSM v1.1: #0000 is #ED #00 #ED #00 and releases a CSL wait_ssm0000.
    h.fetch_all({0xED, 0x00, 0xED, 0x00});
    expect_eq(h->event_count, 1, "0000 event count");
    expect_eq(h->last_code, 0x0000, "0000 code");
}

void test_reserved_snapshot_code() {
    Harness h;
    // SSM v1.1: #FFFF is #ED #FF #ED #FF. Both bytes sit outside the ranges
    // the standard offers to user code, which is a reservation for these
    // codes and not a property of the Z80A, so the matcher must accept them.
    h.fetch_all({0xED, 0xFF, 0xED, 0xFF});
    expect_eq(h->event_count, 1, "FFFF event count");
    expect_eq(h->last_code, 0xFFFF, "FFFF code");
}

void test_spec_reset_example() {
    Harness h;
    // SSM v1.1, "DEFINITION": with the byte sequence ED 3F 00 ED 3E ED 3D the
    // LL code is 3E and the HH code is 3D, because the 00 resets the wait for
    // the 16-bit code. Exactly one event, #3D3E.
    h.fetch_all({0xED, 0x3F, 0x00, 0xED, 0x3E, 0xED, 0x3D});
    expect_eq(h->event_count, 1, "spec example event count");
    expect_eq(h->last_code, 0x3D3E, "spec example code");
}

void test_real_ed_instruction_is_not_a_marker() {
    Harness h;
    // #ED #4B is LD BC,(nn): #4B is outside every allowed range, so the pair
    // is a real instruction. Its operand bytes are not opcode fetches and
    // never reach the matcher.
    h.fetch_all({0xED, 0x4B, 0xED, 0x4B});
    expect_eq(h->event_count, 0, "LD BC,(nn) event count");
}

void test_allowed_range_boundaries() {
    // Endpoints of every range SSM v1.1 lists, plus the first value above
    // each range that the standard excludes.
    struct Case { uint8_t byte; bool allowed; };
    const Case cases[] = {
        {0x00, true},  {0x3F, true},  {0x40, false},
        {0x7E, false}, {0x7F, true},  {0x9F, true},  {0xA0, false},
        {0xA4, true},  {0xA7, true},  {0xA8, false},
        {0xAC, true},  {0xAF, true},  {0xB0, false},
        {0xB4, true},  {0xB7, true},  {0xB8, false},
        {0xBC, true},  {0xBF, true},
        {0xC0, true},  {0xFD, true},
        {0x10, true},
    };
    for (const Case& c : cases) {
        Harness h;
        h.fetch_all({0xED, c.byte, 0xED, c.byte});
        std::ostringstream os;
        os << "byte 0x" << std::hex << unsigned(c.byte) << " acceptance";
        expect_eq(h->event_count, c.allowed ? 1 : 0, os.str());
        if (c.allowed) {
            expect_eq(h->last_code, (unsigned(c.byte) << 8) | c.byte, os.str() + " code");
        }
    }
}

void test_ed_is_itself_an_allowed_value() {
    Harness h;
    // #ED falls inside #C0-#FD, so #ED #ED is a complete undefined-ED
    // instruction carrying LL = #ED, not a restart of the prefix.
    h.fetch_all({0xED, 0xED, 0xED, 0xED});
    expect_eq(h->event_count, 1, "ED ED ED ED event count");
    expect_eq(h->last_code, 0xEDED, "ED ED ED ED code");
}

void test_wait_states_do_not_multiply_a_fetch() {
    // The motherboard tap is a level spanning the whole M1 read. Stretching
    // each fetch must still produce exactly one marker, and exactly one
    // strobe per fetch.
    for (unsigned hold : {1u, 2u, 5u, 13u}) {
        Harness h;
        h.fetch_all({0xED, 0xFE, 0xED, 0xFF}, hold);
        std::ostringstream os;
        os << "hold=" << hold;
        expect_eq(h->event_count, 1, os.str() + " event count");
        expect_eq(h->last_code, 0xFFFE, os.str() + " code");
        expect_eq(h->fetch_strobes, 4, os.str() + " strobe count");
    }
}

// --- ring transport ---------------------------------------------------------

void test_ring_header_and_record() {
    Harness h;
    h.fetch_all({0xED, 0xFE, 0xED, 0xFF});
    h.tick(64);

    const uint64_t magic = h.peek(kWordMagic);
    expect_eq(uint32_t(magic), kMagic, "ring magic");
    expect_eq((magic >> 32) & 0xFFFF, 1, "ring format version");
    expect_eq((magic >> 48) & 0xFF, kRingEntries, "ring entry count");

    const uint64_t header = h.peek(kWordHeader);
    expect_eq(uint32_t(header), 1, "written record count");
    expect_eq((header >> 32) & 0xFF, 0, "dropped count");

    const uint64_t rec_a = h.peek(kWordRecord0);
    expect_eq(rec_a & 0xFFFF, 0xFFFE, "record code");

    const uint64_t rec_b = h.peek(kWordRecord0 + 1);
    expect_eq(rec_b & 0xFFFF, 0, "record sequence number");
}

void test_ring_wraps_and_keeps_counting() {
    Harness h;
    const unsigned events = kRingEntries + 3;
    for (unsigned i = 0; i < events; ++i) {
        h.fetch_all({0xED, uint8_t(i & 0x3F), 0xED, 0x20});
        h.tick(32);
    }
    expect_eq(h->event_count, events, "wrapped event count");
    expect_eq(uint32_t(h.peek(kWordHeader)), events, "wrapped written count");
    // Slot 0 now holds event 64, whose LL is 64 & 0x3F = 0.
    const uint64_t rec_a = h.peek(kWordRecord0);
    expect_eq(rec_a & 0xFFFF, 0x2000, "slot 0 after wrap holds the newer code");
    expect_eq(h.peek(kWordRecord0 + 1) & 0xFFFF, kRingEntries,
              "slot 0 after wrap carries the newer sequence number");
}

void test_ddr_stall_delays_but_never_drops() {
    Harness h;
    h->ddr_stall = 7;
    h.fetch_all({0xED, 0x12, 0xED, 0x34});
    h.tick(400);
    expect_eq(h->event_count, 1, "stalled event count");
    expect_eq(h->dropped_count, 0, "stalled dropped count");
    expect_eq(uint32_t(h.peek(kWordHeader)), 1, "stalled written count");
    expect_eq(h.peek(kWordRecord0) & 0xFFFF, 0x3412, "stalled record code");
}

void test_back_to_back_markers_are_counted_even_when_one_is_dropped() {
    Harness h;
    // Hold the DDR port off long enough that a second marker arrives while
    // the first is still pending. The event must be counted and the loss
    // reported, never silently discarded.
    h->ddr_stall = 15;
    h.fetch_all({0xED, 0x01, 0xED, 0x01}, 1);
    h.fetch_all({0xED, 0x02, 0xED, 0x02}, 1);
    h.fetch_all({0xED, 0x03, 0xED, 0x03}, 1);
    h.tick(2000);
    expect_eq(h->event_count, 3, "back-to-back event count");
    const unsigned written = uint32_t(h.peek(kWordHeader));
    const unsigned dropped = (h.peek(kWordHeader) >> 32) & 0xFF;
    expect_eq(written + dropped, 3, "written plus dropped accounts for every event");
    if (dropped == 0) fail("expected the stalled port to force at least one drop");
    expect_eq(h->dropped_count, dropped, "dropped count matches the header");
}

void test_disabled_detector_is_completely_silent() {
    Harness h;
    h->enable = 0;
    h.tick(4);
    h.fetch_all({0xED, 0xFE, 0xED, 0xFF});
    h.tick(64);
    expect_eq(h->event_count, 0, "disabled event count");
    expect_eq(h->ddr_write_count, 0, "disabled DDR write count");
    expect_eq(h.peek(kWordMagic), 0, "disabled leaves the ring untouched");

    // Re-enabling starts from a clean state rather than resuming mid-marker.
    h->enable = 1;
    h.tick(4);
    h.fetch_all({0xFE, 0xED, 0xFF});
    expect_eq(h->event_count, 0, "a half marker spanning the toggle is not completed");
    h.fetch_all({0xED, 0xFE, 0xED, 0xFF});
    expect_eq(h->event_count, 1, "detection resumes after re-enabling");
}

// --- executing CPU ----------------------------------------------------------

void test_cpu_undefined_ed_pair_emits_the_marker() {
    Harness h;
    // 0x00 F3        DI
    // 0x01 ED FE     undefined ED, LL = FE
    // 0x03 ED FF     undefined ED, HH = FF  -> SSM #FFFE
    // 0x05 76        HALT
    h.load(0x00, {0xF3, 0xED, 0xFE, 0xED, 0xFF, 0x76});
    h.run_cpu();
    expect_eq(h->event_count, 1, "executed FFFE event count");
    expect_eq(h->last_code, 0xFFFE, "executed FFFE code");
}

void test_cpu_spec_reset_example() {
    Harness h;
    // The standard's own example, assembled: ED 3F and ED 3E and ED 3D are
    // undefined ED instructions, 00 is NOP. Exactly one event, #3D3E.
    h.load(0x00, {0xF3, 0xED, 0x3F, 0x00, 0xED, 0x3E, 0xED, 0x3D, 0x76});
    h.run_cpu();
    expect_eq(h->event_count, 1, "executed spec example event count");
    expect_eq(h->last_code, 0x3D3E, "executed spec example code");
}

void test_cpu_real_ed_instruction_emits_nothing() {
    Harness h;
    // ED 4B nn nn is LD BC,(nn). Its two operand bytes are read with M1
    // high, so they are not opcode fetches and cannot be mistaken for a
    // marker.
    h.load(0x00, {0xF3, 0xED, 0x4B, 0x40, 0x00, 0xED, 0x4B, 0x40, 0x00, 0x76});
    h.run_cpu();
    expect_eq(h->event_count, 0, "executed LD BC,(nn) event count");
}

void test_cpu_sync_code() {
    Harness h;
    h.load(0x00, {0xF3, 0xED, 0x00, 0xED, 0x00, 0x76});
    h.run_cpu();
    expect_eq(h->event_count, 1, "executed 0000 event count");
    expect_eq(h->last_code, 0x0000, "executed 0000 code");
}

void test_cpu_interrupt_between_the_pairs_drops_the_marker() {
    Harness h;
    // 0x00 31 00 01  LD SP,$0100
    // 0x03 ED 56     IM 1
    // 0x05 FB        EI     -> the interrupt is accepted at the end of the
    // 0x06 ED FE            next instruction, which is this pair
    // 0x08 ED FF
    // 0x0A 76        HALT
    // 0x38 76        HALT   (mode 1 vector; the handler stops there so the
    //                        run ends inside the interrupt)
    //
    // The acknowledge cycle asserts IORQ with MREQ high, so it is not an
    // opcode fetch and injects no byte. What does reach the matcher is the
    // handler's own fetch at the vector, and that non-ED byte drops the
    // marker. SSM v1.1 gives no exemption for this.
    h.load(0x00, {0x31, 0x00, 0x01, 0xED, 0x56, 0xFB, 0xED, 0xFE, 0xED, 0xFF, 0x76});
    h.load(0x38, {0x76});
    h->int_n = 0;
    h.run_cpu();
    expect_eq(h->cpu_addr, 0x38, "the run stopped at the mode 1 vector");
    expect_eq(h->event_count, 0, "split marker event count");

    // The identical program with interrupts disabled must produce the
    // marker, so the case above is the interrupt and not a broken program.
    Harness control;
    control.load(0x00, {0x31, 0x00, 0x01, 0xED, 0x56, 0xF3, 0xED, 0xFE, 0xED, 0xFF, 0x76});
    control.load(0x38, {0x76});
    control->int_n = 0;
    control.run_cpu();
    expect_eq(control->event_count, 1, "control event count");
    expect_eq(control->last_code, 0xFFFE, "control code");
}

void test_cpu_marker_reaches_the_ring() {
    Harness h;
    h.load(0x00, {0xF3, 0xED, 0xFE, 0xED, 0xFF, 0x76});
    h.run_cpu();
    h.tick(64);
    expect_eq(uint32_t(h.peek(kWordMagic)), kMagic, "executed ring magic");
    expect_eq(uint32_t(h.peek(kWordHeader)), 1, "executed written count");
    expect_eq(h.peek(kWordRecord0) & 0xFFFF, 0xFFFE, "executed record code");
}

struct Test {
    const char* name;
    void (*run)();
};

const Test kTests[] = {
    {"reserved screenshot code", test_reserved_screenshot_code},
    {"reserved sync code", test_reserved_sync_code},
    {"reserved snapshot code", test_reserved_snapshot_code},
    {"spec reset example", test_spec_reset_example},
    {"real ED instruction is not a marker", test_real_ed_instruction_is_not_a_marker},
    {"allowed range boundaries", test_allowed_range_boundaries},
    {"ED is itself an allowed value", test_ed_is_itself_an_allowed_value},
    {"wait states do not multiply a fetch", test_wait_states_do_not_multiply_a_fetch},
    {"ring header and record", test_ring_header_and_record},
    {"ring wraps and keeps counting", test_ring_wraps_and_keeps_counting},
    {"DDR stall delays but never drops", test_ddr_stall_delays_but_never_drops},
    {"back-to-back markers are accounted for", test_back_to_back_markers_are_counted_even_when_one_is_dropped},
    {"disabled detector is completely silent", test_disabled_detector_is_completely_silent},
    {"CPU: undefined ED pair emits the marker", test_cpu_undefined_ed_pair_emits_the_marker},
    {"CPU: spec reset example", test_cpu_spec_reset_example},
    {"CPU: real ED instruction emits nothing", test_cpu_real_ed_instruction_emits_nothing},
    {"CPU: sync code", test_cpu_sync_code},
    {"CPU: interrupt between the pairs drops the marker",
     test_cpu_interrupt_between_the_pairs_drops_the_marker},
    {"CPU: marker reaches the ring", test_cpu_marker_reaches_the_ring},
};

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    unsigned failures = 0;
    for (const Test& test : kTests) {
        try {
            test.run();
            std::cout << "PASS " << test.name << "\n";
        } catch (const std::exception& exc) {
            std::cout << "FAIL " << test.name << ": " << exc.what() << "\n";
            ++failures;
        }
    }
    std::cout << (failures ? "SSM marker tests FAILED: " : "SSM marker tests passed: ")
              << (sizeof(kTests) / sizeof(kTests[0]) - failures) << "/"
              << sizeof(kTests) / sizeof(kTests[0]) << "\n";
    return failures ? 1 : 0;
}
