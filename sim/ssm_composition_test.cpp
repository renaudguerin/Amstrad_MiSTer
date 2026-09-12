// SSM marker + recorder + arbiter composition tests (Finding 1 regression).
//
// Proves that when both SSM marker and SSM sample recorder share the DDR3
// port via ssm_ddr_arb:
// 1. Both clients initially idle can originate requests without deadlock.
// 2. Initial DDR stall holds both requests and completes cleanly upon release.
// 3. Concurrent marker events and sample stream packets are arbitrated without loss.

#include <verilated.h>
#include "Vssm_composition_top.h"

#include <cstdint>
#include <iostream>
#include <map>
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

void expect_true(bool ok, const std::string& what) {
    if (!ok) fail(what);
}

constexpr uint32_t kRingWord = 0x30000000u >> 3;
constexpr uint32_t kCapWord  = 0x31000000u >> 3;
constexpr uint32_t kRingMagic = 0x53534D31u;
constexpr uint32_t kCapMagic  = 0x53534D43u;

struct Beat {
    uint32_t addr;
    uint64_t data;
};

class CompHarness {
public:
    explicit CompHarness(bool start_enabled = true)
        : dut_(std::make_unique<Vssm_composition_top>()) {
        dut_->clk = 0;
        dut_->reset = 1;
        dut_->enable = start_enabled;
        dut_->m1_fetch = 0;
        dut_->bus_data = 0;
        dut_->ce_pix = 0;
        dut_->hsync = 0;
        dut_->vsync = 0;
        dut_->field = 0;
        dut_->smp_ce = 0;
        dut_->smp_r = 0; dut_->smp_g = 0; dut_->smp_b = 0;
        dut_->smp_hbl = 0; dut_->smp_vbl = 0;
        dut_->smp_hs = 0; dut_->smp_vs = 0; dut_->smp_field = 0;
        dut_->applied_config = 0x81;
        dut_->ddr_stall = 0;
        dut_->eval();
        tick(8);
        dut_->reset = 0;
    }

    bool saw_marker_hit = false;

    void tick(unsigned n = 1) {
        for (unsigned i = 0; i < n; ++i) {
            if (dut_->marker_hit) saw_marker_hit = true;
            if (dut_->ddr_we_o && !dut_->ddr_busy_o) {
                beats.push_back({uint32_t(dut_->ddr_addr_o), uint64_t(dut_->ddr_din_o)});
                memory[uint32_t(dut_->ddr_addr_o)] = uint64_t(dut_->ddr_din_o);
            }
            dut_->clk = 1;
            dut_->eval();
            dut_->clk = 0;
            dut_->eval();
        }
    }

    void fetch_byte(uint8_t b, unsigned hold = 1) {
        dut_->bus_data = b;
        dut_->m1_fetch = 1;
        tick(hold);
        dut_->m1_fetch = 0;
        tick(1);
    }

    void fetch_marker(uint8_t lo, uint8_t hi) {
        fetch_byte(0xED);
        fetch_byte(lo);
        fetch_byte(0xED);
        fetch_byte(hi);
    }

    void sample(uint8_t r, uint8_t g, uint8_t b) {
        dut_->smp_r = r;
        dut_->smp_g = g;
        dut_->smp_b = b;
        dut_->smp_ce = 1;
        tick(1);
        dut_->smp_ce = 0;
    }

    Vssm_composition_top* operator->() { return dut_.get(); }

    std::vector<Beat> beats;
    std::map<uint32_t, uint64_t> memory;

private:
    std::unique_ptr<Vssm_composition_top> dut_;
};

void test_composition_startup_both_idle_ddr_ready() {
    CompHarness h;
    // Both clients are enabled and initially idle, DDR ready.
    // Wait for startup to complete.
    unsigned guard = 0;
    while (!h->recorder_ready) {
        h.tick();
        if (++guard > 5000) fail("recorder never became ready behind arbiter");
    }
    // Give enough cycles for marker to publish its 2 startup beats
    h.tick(64);

    // Verify marker wrote startup magic and zero header
    expect_true(h.memory.find(kRingWord) != h.memory.end(), "marker magic was not written");
    expect_eq(uint32_t(h.memory[kRingWord]), kRingMagic, "marker magic value");
    expect_true(h.memory.find(kRingWord + 1) != h.memory.end(), "marker header was not written");
    expect_eq(uint32_t(h.memory[kRingWord + 1]), 0, "marker written count on startup must be 0");

    // Verify recorder wrote ready magic
    expect_true(h.memory.find(kCapWord) != h.memory.end(), "recorder magic was not written");
    expect_eq(uint32_t(h.memory[kCapWord]), kCapMagic, "recorder magic value");
    expect_eq((h.memory[kCapWord] >> 48) & 0xFFFF, 1, "recorder ready flag");

    expect_eq(h->marker_event_count, 0, "no marker event should be invented during startup");
}

void test_composition_startup_both_idle_ddr_stalled() {
    CompHarness h;
    h->ddr_stall = 15;
    // Hold DDR in heavy stall during initial startup
    h.tick(100);
    // Release stall
    h->ddr_stall = 0;
    unsigned guard = 0;
    while (!h->recorder_ready) {
        h.tick();
        if (++guard > 5000) fail("recorder never became ready after stall released");
    }
    h.tick(64);

    expect_eq(uint32_t(h.memory[kRingWord]), kRingMagic, "marker magic after stall");
    expect_eq(uint32_t(h.memory[kRingWord + 1]), 0, "marker header after stall");
    expect_eq(uint32_t(h.memory[kCapWord]), kCapMagic, "recorder magic after stall");
    expect_eq((h.memory[kCapWord] >> 48) & 0xFFFF, 1, "recorder ready flag after stall");
}

void test_composition_concurrent_marker_and_sample_traffic() {
    CompHarness h;
    unsigned guard = 0;
    while (!h->recorder_ready) {
        h.tick();
        if (++guard > 5000) fail("recorder not ready");
    }
    h.tick(64);

    // Stream samples while firing a marker
    for (unsigned i = 0; i < 16; ++i) {
        h->smp_r = i; h->smp_g = i; h->smp_b = i;
        h->smp_ce = 1;
        h.tick(1);
        h->smp_ce = 0;
        h.tick(3);
    }
    // Fire marker
    h.fetch_marker(0x12, 0x34);
    h.tick(4);
    expect_true(h.saw_marker_hit, "marker hit must be asserted");

    // Continue streaming samples
    for (unsigned i = 16; i < 32; ++i) {
        h->smp_r = i; h->smp_g = i; h->smp_b = i;
        h->smp_ce = 1;
        h.tick(1);
        h->smp_ce = 0;
        h.tick(3);
    }
    h.tick(500);

    expect_eq(h->marker_event_count, 1, "marker event count");
    expect_eq(h->recorder_captures_published, 1, "recorder capture published");
    // Verify marker event was written to event ring (entry 0 at kRingWord + 2)
    expect_true(h.memory.find(kRingWord + 2) != h.memory.end(), "marker event record word A written");
    expect_eq(uint32_t(h.memory[kRingWord + 1]), 1, "marker header written count must be 1");
}

void test_pressure_preserves_delayed_hh_cut_history() {
    CompHarness h;
    for (unsigned i = 0; !h->recorder_ready; ++i) {
        if (i == 5000) fail("recorder not ready for pressure test");
        h.tick();
    }
    h.tick(64);
    auto dot = [&]() { h.sample(1, 2, 3); h.tick(3); };
    // Pin window 0, then fill windows 0..2 and 30 dots of window 3.
    // The four-slot pool must expire this older pin, not a recent predecessor.
    for (unsigned i = 0; i < 4; ++i) dot();
    h.fetch_marker(0x01, 0x02);
    h.tick(8);
    for (unsigned i = 4; i < 126; ++i) dot();

    // Native samples 127/128 occur four clocks apart. HH completes on the
    // latter edge; its exclusive cut is 127 and its registered capture reaches
    // the recorder two clocks later, during the pending window rotation.
    auto cycle = [&](unsigned phase, uint8_t byte, bool fetch) {
        h->smp_ce = (phase == 4 || phase == 8);
        h->bus_data = byte; h->m1_fetch = fetch; h.tick();
    };
    cycle(1, 0xED, true); cycle(2, 0xED, false);
    cycle(3, 0x11, true); cycle(4, 0x11, false);
    cycle(5, 0xED, true); cycle(6, 0xED, false);
    cycle(7, 0x22, true); cycle(8, 0x22, false);
    h->smp_ce = 0;
    h.tick(1000); // still inside the fixture's 2000-clock pin lifetime

    expect_eq(h->recorder_captures_published, 2, "both pressured captures publish");
    const uint32_t record = kCapWord + (0x200u >> 3) + 8; // capture index 1
    expect_eq(h.memory.at(record + 2), 127, "HH cut must exclude the coincident sample");
    for (unsigned word = 3; word <= 5; ++word) {
        const uint64_t ref = h.memory.at(record + word);
        const uint32_t index = uint32_t(ref);
        const uint64_t desc = h.memory.at(kCapWord + 8 + index * 4);
        expect_eq(uint32_t(desc), uint32_t(ref >> 32), "required history generation was reused");
        expect_eq(desc >> 32, 2, "required history must remain sealed");
    }
    expect_true(uint32_t(h.memory.at(kCapWord + 7)) > 0,
                "pressure must explicitly report expiry of the older pinned capture");
}

void test_composition_one_clock_disable_holds_selected_stalled_beat() {
    CompHarness h;
    unsigned guard = 0;
    while (!h->recorder_ready) {
        h.tick();
        if (++guard > 5000) fail("recorder not ready before lifecycle test");
    }
    h.tick(64);

    // Leave an observable event and capture behind in the shared physical
    // window. The second run must publish fresh zero headers rather than
    // resuming this state after a short enable pulse.
    h.fetch_marker(0x12, 0x34);
    h.tick(500);
    expect_eq(uint32_t(h.memory[kRingWord + 1]), 1,
              "old event header must contain the completed event");
    expect_eq(uint32_t(h.memory[kCapWord + 6]), 1,
              "old capture header must contain the completed capture");

    // Queue one recorder payload word while the arbiter is idle. On the next
    // edge, start a finite physical DDR stall at the same time the recorder
    // presents that request. This makes client 1 the arbiter-selected owner,
    // with a real Avalon beat visible at the composition output.
    h.sample(0x11, 0x22, 0x33);
    h.sample(0x44, 0x55, 0x66);
    // Dispatch the queued FIFO command while DDR is still ready; the next
    // edge can now present it while loading the physical stall.
    h.tick();
    expect_eq(h->ddr_we_o, 0, "recorder request should be queued before the stall");
    expect_eq(h->ddr_busy_o, 0, "DDR must be ready before loading the stall");

    h->ddr_stall = 4;
    h.tick();
    expect_eq(h->ddr_we_o, 1, "recorder request was not presented");
    expect_eq(h->ddr_busy_o, 1, "presented recorder beat was not physically stalled");
    const uint64_t held_addr = h->ddr_addr_o;
    const uint64_t held_din = h->ddr_din_o;
    const size_t accepted_before_toggle = h.beats.size();
    const uint64_t expected_payload = (uint64_t(0x00665544) << 32) | 0x00332211u;
    expect_eq(held_addr, kCapWord + (0x1000u >> 3),
              "arbiter selected an unexpected recorder payload address");
    expect_eq(held_din, expected_payload,
              "arbiter selected an unexpected recorder payload value");

    // Disable for exactly one clock, then re-enable while waitrequest is still
    // asserted. The selected request must remain a single stable Avalon beat;
    // it has not yet been accepted at either edge.
    h->enable = 0;
    h.tick();
    expect_eq(h->ddr_we_o, 1, "disable dropped the selected request");
    expect_eq(h->ddr_busy_o, 1, "stall ended before the one-clock disable completed");
    expect_eq(h->ddr_addr_o, held_addr, "address changed during the disabled stall");
    expect_eq(h->ddr_din_o, held_din, "data changed during the disabled stall");
    expect_eq(h.beats.size(), accepted_before_toggle,
              "selected beat was accepted during the disabled clock");

    h->enable = 1;
    h.tick();
    expect_eq(h->ddr_we_o, 1, "re-enable dropped the selected request before release");
    expect_eq(h->ddr_busy_o, 1, "stall ended before re-enable was sampled");
    expect_eq(h->ddr_addr_o, held_addr, "address changed before stalled beat release");
    expect_eq(h->ddr_din_o, held_din, "data changed before stalled beat release");
    expect_eq(h.beats.size(), accepted_before_toggle,
              "selected beat was accepted before the stall was released");

    // Remove the stall and let the one held beat drain. The selected payload
    // must be accepted once, then both clients must start their own fresh
    // initialization sequences.
    h->ddr_stall = 0;
    h.tick(16);
    h.tick(120);
    unsigned ready_guard = 0;
    while (!h->recorder_ready) {
        h.tick();
        if (++ready_guard > 5000) fail("recorder did not reinitialize after release");
    }
    h.tick(64);

    unsigned held_accepts = 0;
    for (const Beat& beat : h.beats) {
        if (beat.addr == held_addr && beat.data == held_din) ++held_accepts;
    }
    expect_eq(held_accepts, 1, "the stalled recorder beat was accepted exactly once");

    expect_eq(h->marker_event_count, 0,
              "old marker event resumed after the one-clock disable");
    expect_eq(h->recorder_captures_published, 0,
              "old capture resumed after the one-clock disable");
    expect_eq(h->recorder_sample_count, 0,
              "old recorder sample history resumed after the one-clock disable");
    expect_eq(uint32_t(h.memory[kRingWord]), kRingMagic,
              "event header magic after one-clock re-enable");
    expect_eq(uint32_t(h.memory[kRingWord + 1]), 0,
              "event header count after one-clock re-enable");
    expect_eq((h.memory[kRingWord + 1] >> 32) & 0xFF, 0,
              "event header dropped count after one-clock re-enable");
    expect_eq(uint32_t(h.memory[kCapWord]), kCapMagic,
              "capture header magic after one-clock re-enable");
    expect_eq(uint32_t(h.memory[kCapWord + 6]), 0,
              "capture header published count after one-clock re-enable");
    expect_eq(uint32_t(h.memory[kCapWord + 7]), 0,
              "capture header expiry/drop count after one-clock re-enable");
}

struct Test {
    const char* name;
    void (*run)();
};

const Test kTests[] = {
    {"pressured rotation preserves the delayed exclusive HH cut",
     test_pressure_preserves_delayed_hh_cut_history},
    {"startup with both clients idle and DDR ready",
     test_composition_startup_both_idle_ddr_ready},
    {"startup with both clients idle and DDR stalled",
     test_composition_startup_both_idle_ddr_stalled},
    {"concurrent marker and sample traffic arbitration",
     test_composition_concurrent_marker_and_sample_traffic},
    {"one-clock disable holds selected stalled beat and restarts both clients",
     test_composition_one_clock_disable_holds_selected_stalled_beat},
};

} // namespace

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
    std::cout << (failures ? "SSM composition tests FAILED: " : "SSM composition tests passed: ")
              << (sizeof(kTests) / sizeof(kTests[0]) - failures) << "/"
              << sizeof(kTests) / sizeof(kTests[0]) << "\n";
    return failures ? 1 : 0;
}
