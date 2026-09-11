// B8-2 Plus FIELD ownership regression (fail-first, source-derived).
//
// Production boundary executed: real Amstrad_motherboard in Plus mode on the
// NORMAL filtered sync path (sync_filter=1) via b8_field_bench_top, with the
// real asic_video (CRTC type 3), the real classic CRTC, the production field
// mux, and the extracted production scaler consumer (rtl/video_interlace.v:
// behavioral extraction of the Amstrad.sv history+enable decision, shared
// with production). Observed
// outputs are motherboard pins (FIELD=VGA_F1, selected filtered VSYNC) and
// the consumer's interlace/scandoubler outputs (real RTL execution).
//
// Programming is production I/O throughout for the Plus engine: C++ drives
// a bounded bench CPU source (b8_field_cpu.v, module T80pa) that issues real
// I/O writes; the motherboard decode delivers them to BOTH CRTCs, exactly
// like a Z80 OUT. The ONLY hierarchical register poke is the inactive
// classic R8 in the negative-control case (deliberate: it proves the mux,
// and the classic engine is unselected). Plus R8 is never poked and
// parity_c9 is never forced; entry seeding (ParityC9=C9.0) happens inside
// the RTL from the live bus write.
//
// Rule basis (French ACCC v1.11 canonical; English pages differ by reflow):
//   - FR §19.5.5 pp.214-216 (EN pp.214-216): ParityFrame toggles every frame
//     at C4=C9=C0=0 whatever R8; even schedules the additional line + MID.
//     At frame start ParityC9=ParityFrame; R8->1/3 seeds ParityC9=C9.0, so a
//     transition entry on odd C9 really mismatches C9 vs frame parity (p.214)
//     and settles the following frame. Even R9 keeps C9 parity aligned to
//     frame parity once settled (used here: R9=6, so origins are C9=0).
//   - FR §19.6.4 p.218 (EN p.218): either R8=1 or R8=3 adds exactly one
//     C9=0 line after the R5 block on the even frame; C4 not incremented.
//   - FR §19.7.3 p.219 (EN p.219): MID (C0=R0/2) on the even frame for
//     either mode; seam (C0=0) on the odd frame. R7=0 is the outgoing-parity
//     exception: VSYNC is computed BEFORE the toggle, so MID then carries
//     the NEW-odd FIELD0 and seam carries NEW-even FIELD1 — the opposite of
//     ordinary R7. FIELD itself is never inverted by R7.
// Polarity (SOURCE-DERIVED MiSTer convention, not hardware clearance):
// sys/ascal.vhd:1233-1252 latches field at DE rise and, at the VSYNC+DE
// frame start, writes the base buffer when FIELD=1 (first woven line) but
// offsets one line when FIELD=0. Settled even-R9 IVM aligns C9 parity to
// frame parity, so ~parity_frame puts even lines first. The classic
// FIELD=~field resemblance is a separate legacy flop, not evidence.
// Transition C9/frame mismatch is real (FR p.214); FIELD follows frame
// parity, never raster parity.
//
// Phase oracle (raw ASIC VSYNC only): with R0=63, MID rises at hcc=31 and
// seam at hcc=0 (leaf t04j/t04k pin the same rule). FIELD expectations come
// from the documented pairing, never from internal parity. The FILTERED
// selected VSYNC drives the consumer only; its hcc phase is NOT used.
// Limit: this verifies the local downstream decision, not full ASCAL
// simulation (no mixer/HQ2x/freeze/T80-Configure paths).
//
// Cases (frame R0=63 R1=40 R4=67 R5=0 R6=100 R9=6: 68 rows x 7 lines = 476
// ordinary lines/field, 68 x 4 = 272 IVM lines/field; both clear the
// production crt_filter VSYNC debounce (vSyncFlt>260) on the normal filtered
// path. R0=63 is also required so the filter's HSYNC mask clears every line
// (line-low 62 us >= 190 CE_4); shorter lines halve its line rate and its
// accepts go sporadic (probed, not assumed). MID is at hcc=31, seam at
// hcc=0, even-field bonus exactly one 4096-clk line.
// All plus_mode=1, sync_filter=1 (normal):
//   s1 ordinary R7=5, Plus R8=3 via bus: MID+long FIELD1 / seam+short FIELD0.
//   s2 ordinary R7=5, Plus R8=1 via bus: same (R8=1 is interlace).
//   z1 R7=0, Plus R8=3 via bus: MID carries FIELD0, seam FIELD1 (exception).
//   z2 R7=0, Plus R8=1 via bus: same exception shape.
//   t1 live entry: R8=3 via bus at ODD raster mid-frame; FIELD stable to the
//      next origin, then the settled s1 pairing over the next two fields.
//   t2 live exit + consumer recovery: from interlace, R8=0 via bus
//      mid-frame; FIELD 0 from the next raw VSYNC, and after three SELECTED
//      (filtered) VSYNC rises the production consumer is 0 with the
//      scandoubler re-enabled.
//   p1 Plus R8=0 via bus: constant 0, all seam, equal lengths.
//   p2 Plus R8=2 via bus: constant 0 progressive guard (R8[0]=0).
//   n1 negative control: Plus R8=0 via bus, classic R8=3 poked (inactive):
//      pin constant 0 at three witnessed selected accepts, all seam,
//      consumer stays enabled after sampling the perturbed window.
// Unchanged RTL fails s1/s2/z1/z2 (no toggle), t1/t2 (no toggle/recovery)
// and n1 (spurious toggle); p1/p2 pass.

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "Vb8_field_bench_top.h"
#include "Vb8_field_bench_top___024root.h"
#include "verilated.h"

namespace {

class TestFailure : public std::runtime_error {
public:
    explicit TestFailure(const std::string& m) : std::runtime_error(m) {}
};

[[noreturn]] void fail(const std::string& what) { throw TestFailure(what); }

constexpr uint64_t kReadyTimeout = 10000;
constexpr uint64_t kBusTimeout = 5000;
constexpr uint64_t kOriginTimeout = 2000000;
constexpr uint64_t kFieldTimeout = 4000000;
// n1 accept window: three progressive selected rises cost ~5.85M clocks worst
// case at R0=63/R4=67/R9=6; 8M is the explicit bound (fails if unwitnessed).
constexpr uint64_t kN1AcceptTimeout = 8000000;

class Bench {
public:
    Vb8_field_bench_top dut;
    uint64_t cyc = 0;

    Bench() : dut("b8_field_bench_top") {
        dut.clk = 0;
        dut.reset = 1;
        dut.sync_filter_i = 1; // normal filtered path (never raw 2)
        dut.asic_page_on = 0;
        dut.plus_mode_i = 1;
    }

    void tick() {
        dut.clk = 0;
        dut.eval();
        dut.clk = 1;
        dut.eval();
        ++cyc;
    }
    void run(uint64_t n) { for (uint64_t i = 0; i < n; ++i) tick(); }

    auto* cpu_done() {
        return &dut.rootp->b8_field_bench_top__DOT__mb__DOT__CPU__DOT__dbg_done;
    }
    auto* bench_addr() {
        return &dut.rootp->b8_field_bench_top__DOT__mb__DOT__CPU__DOT__bench_addr;
    }
    auto* bench_data() {
        return &dut.rootp->b8_field_bench_top__DOT__mb__DOT__CPU__DOT__bench_data;
    }
    auto* bench_req() {
        return &dut.rootp->b8_field_bench_top__DOT__mb__DOT__CPU__DOT__bench_req;
    }
    auto* bench_ack() {
        return &dut.rootp->b8_field_bench_top__DOT__mb__DOT__CPU__DOT__bench_ack;
    }
    // Stimulus-adjacent counter taps (never parity; never expectations).
    auto* plus_hcc() {
        return &dut.rootp->b8_field_bench_top__DOT__mb__DOT__asic_vid__DOT__hcc;
    }
    auto* plus_line() {
        return &dut.rootp->b8_field_bench_top__DOT__mb__DOT__asic_vid__DOT__charline;
    }
    auto* plus_row() {
        return &dut.rootp->b8_field_bench_top__DOT__mb__DOT__asic_vid__DOT__raster;
    }
    auto* plus_vsync_raw() {
        return &dut.rootp->b8_field_bench_top__DOT__mb__DOT__asic_vid__DOT__VSYNC;
    }
    // Deliberate negative-control poke: INACTIVE classic engine only.
    auto* classic_r8() {
        return &dut.rootp->b8_field_bench_top__DOT__mb__DOT__crtc__DOT__R8_interlace;
    }
    // Origin scaffolding only (never a FIELD expectation): IVM odd-frame
    // origins restart at C9=1 (rtl/plus/asic_video.v raster_n), while even
    // IVM origins and every non-IVM origin restart at C9=0. R8=1/0/2 always
    // restart at C9=0, so gating row 1 on R8==3 isolates real origins (the
    // additional line holds C4=R4, so line==0 still excludes it).
    auto* plus_r8() {
        return &dut.rootp->b8_field_bench_top__DOT__mb__DOT__asic_vid__DOT__R8_interlace;
    }
};

// One real I/O write through the production motherboard decode.
void bus_write(Bench& b, uint16_t addr, uint8_t data, const char* ctx) {
    *b.bench_addr() = addr;
    *b.bench_data() = data;
    *b.bench_req() = 1;
    uint64_t n = 0;
    while (!*b.bench_ack()) {
        b.tick();
        if (++n > kBusTimeout) fail(std::string(ctx) + ": bus write no ack");
    }
    *b.bench_req() = 0;
    n = 0;
    while (*b.bench_ack()) {
        b.tick();
        if (++n > kBusTimeout) fail(std::string(ctx) + ": bus ack stuck");
    }
}

// Production CRTC programming: index OUT then data OUT (A[14]=0 select,
// A[8] index vs data, A[9]=0 write). Reaches BOTH engines like a Z80 OUT.
void write_crtc(Bench& b, uint8_t reg, uint8_t val, const char* ctx) {
    bus_write(b, 0x0800, reg, ctx);
    bus_write(b, 0x0900, val, ctx);
}

// Even R9=6: settled origins are C4=C9=C0=0 on even fields and C4=0,C9=1
// on odd IVM fields (FR §19.5.5 p.214; the R8 gate below is counter
// scaffolding, never a FIELD expectation).
bool at_plus_origin(Bench& b) {
    const uint8_t row = *b.plus_row();
    return *b.plus_hcc() == 0 && *b.plus_line() == 0 &&
           (row == 0 || ((*b.plus_r8() & 3) == 3 && row == 1));
}

void run_to_plus_origin(Bench& b, uint64_t timeout, const char* ctx) {
    uint64_t guard = 0;
    while (at_plus_origin(b)) {
        b.tick();
        if (++guard > 10000) fail(std::string(ctx) + ": stuck in origin window");
    }
    uint64_t n = 0;
    while (!at_plus_origin(b)) {
        b.tick();
        if (++n > timeout) fail(std::string(ctx) + ": no Plus frame origin");
    }
}

// Program R8+R7 via the bus at a fresh origin (seeding no-op: raster 0).
void program_at_origin(Bench& b, uint8_t r8, uint8_t r7, const char* ctx) {
    run_to_plus_origin(b, kOriginTimeout, ctx);
    write_crtc(b, 8, r8, ctx);
    write_crtc(b, 7, r7, ctx);
    b.tick();
}

struct VsyncEvent {
    uint64_t cyc;
    uint8_t hcc;
    uint8_t field;
};
struct OriginEvent {
    uint64_t cyc;
    uint8_t field;
};
struct FieldSpan {
    std::vector<OriginEvent> origins;
    std::vector<VsyncEvent> vsyncs;
};

// Two settled fields delimited by three origins, raw-VSYNC rises strictly
// inside (origin, origin]. Caller programs at an origin then skips one
// entry-transient field here; the live-entry case (t1) does not use this.
FieldSpan collect_two_settled_fields(Bench& b, const char* ctx) {
    FieldSpan out;
    run_to_plus_origin(b, kOriginTimeout, ctx); // skip entry-transient field
    out.origins.push_back({b.cyc, b.dut.vsync_field});
    bool prev_origin = true;
    bool prev_vsync = *b.plus_vsync_raw() != 0;
    uint64_t n = 0;
    while (out.origins.size() < 3) {
        b.tick();
        if (++n > kFieldTimeout) fail(std::string(ctx) + ": two-field span timeout");
        bool origin = at_plus_origin(b);
        bool vsync = *b.plus_vsync_raw() != 0;
        if (vsync && !prev_vsync)
            out.vsyncs.push_back({b.cyc, *b.plus_hcc(), b.dut.vsync_field});
        if (origin && !prev_origin) out.origins.push_back({b.cyc, b.dut.vsync_field});
        prev_origin = origin;
        prev_vsync = vsync;
    }
    return out;
}

// Check one settled two-field span. polarity_exception=false: ordinary R7
// (MID carries FIELD1, seam FIELD0); true: R7=0 outgoing-parity rule (MID
// carries FIELD0, seam FIELD1). FIELD itself is never inverted by R7.
int check_span(Bench& b, const FieldSpan& span, const char* label,
               bool expect_interlace, bool polarity_exception,
               bool check_consumer, bool report = true) {
    int failures = 0;
    auto note = [&](const std::string& m) {
        std::printf("FAIL %s: %s\n", label, m.c_str());
        ++failures;
    };
    if (span.vsyncs.size() != 2) {
        note("expected exactly 2 raw Plus VSYNC rises over two fields, saw " +
             std::to_string(span.vsyncs.size()));
        return failures;
    }
    const uint8_t f0 = span.vsyncs[0].field & 1;
    const uint8_t f1 = span.vsyncs[1].field & 1;
    const uint8_t h0 = span.vsyncs[0].hcc;
    const uint8_t h1 = span.vsyncs[1].hcc;
    const uint64_t len0 = span.origins[1].cyc - span.origins[0].cyc;
    const uint64_t len1 = span.origins[2].cyc - span.origins[1].cyc;
    std::printf("%s: fields=%u/%u hcc=%u/%u lens=%llu/%llu interlace_o=%u scandbl=%u\n",
                label, (unsigned)f0, (unsigned)f1, (unsigned)h0, (unsigned)h1,
                (unsigned long long)len0, (unsigned long long)len1,
                (unsigned)b.dut.interlace_o, (unsigned)b.dut.scandoubler_o);
    if (expect_interlace) {
        // FR §19.6.4 p.218 + FR §19.7.3 p.219: one even field carries the
        // additional line and starts VSYNC at C0=R0/2 (hcc=31, R0=63);
        // the odd field starts at the seam (hcc=0).
        const bool phase_ok = (h0 == 31 && h1 == 0) || (h0 == 0 && h1 == 31);
        if (!phase_ok)
            note("expected one MID (hcc=31) and one seam (hcc=0) VSYNC, saw " +
                 std::to_string(h0) + "/" + std::to_string(h1));
        if (f0 == f1)
            note("expected toggling scaler FIELD over two interlaced fields, saw constant " +
                 std::to_string(f0));
        else {
            // Polarity contract, R7=0 excepted (FR §19.7.3 p.219): ordinary
            // R7 puts FIELD1 on MID; R7=0 puts FIELD0 on MID (outgoing even).
            const uint8_t mid_field = (h0 == 31) ? f0 : (h1 == 31) ? f1 : 2;
            const uint8_t seam_field = (h0 == 0) ? f0 : (h1 == 0) ? f1 : 2;
            const uint8_t want_mid = polarity_exception ? 0 : 1;
            const uint8_t want_seam = polarity_exception ? 1 : 0;
            if (mid_field != want_mid)
                note("expected FIELD=" + std::to_string(want_mid) +
                     " on the MID-VSYNC field, MID carried " + std::to_string(mid_field));
            if (seam_field != want_seam)
                note("expected FIELD=" + std::to_string(want_seam) +
                     " on the seam field, seam carried " + std::to_string(seam_field));
        }
        if (len0 == len1)
            note("expected unequal field lengths (even field +1 line per FR §19.6.4 p.218)");
        else {
            const uint64_t long_len = len0 > len1 ? len0 : len1;
            const uint64_t short_len = len0 > len1 ? len1 : len0;
            if (long_len - short_len != 4096)
                note("expected exactly +1 line (4096 clks, R0=63) on the even field, saw +" +
                     std::to_string(long_len - short_len));
            else if (phase_ok) {
                // Associate the +1 line with the even field from actual
                // origin/event matching: vsyncs[k] falls in the segment
                // ending at origins[k+1], so the MID (hcc=31) rise identifies
                // the segment that must carry the additional line.
                const bool mid_first = (h0 == 31);
                const uint64_t mid_len = mid_first ? len0 : len1;
                if (mid_len != long_len)
                    note("expected the MID-phase (hcc=31) field to be the longer one "
                         "(+1 line per FR §19.6.4 p.218)");
            }
        }
        if (check_consumer && b.dut.scandoubler_o != 0)
            note("production consumer still enables the scandoubler on toggling FIELD");
        if (check_consumer && b.dut.interlace_o == 0)
            note("production consumer history is 0 despite toggling FIELD");
    } else {
        if (h0 != 0 || h1 != 0)
            note("expected seam (hcc=0) VSYNC on both progressive fields, saw " +
                 std::to_string(h0) + "/" + std::to_string(h1));
        if (f0 != 0 || f1 != 0)
            note("expected constant FIELD=0 on progressive fields, saw " +
                 std::to_string(f0) + "/" + std::to_string(f1));
        if (len0 != len1)
            note("expected equal progressive field lengths, saw " +
                 std::to_string(len0) + " vs " + std::to_string(len1));
        if (span.origins[0].field != 0 || span.origins[1].field != 0 ||
            span.origins[2].field != 0)
            note("scaler FIELD moved at a frame origin while Plus is progressive");
    }
    if (failures == 0 && report) std::printf("PASS %s\n", label);
    return failures;
}

int run() {
    Bench b;
    b.dut.reset = 1;
    b.run(64);
    b.dut.reset = 0;
    b.run(8);

    uint64_t n = 0;
    while (!*b.cpu_done()) {
        b.tick();
        if (++n > kReadyTimeout) fail("bench CPU did not become ready");
    }
    std::printf("setup: bench ready at cyc %llu, plus_mode=1 sync_filter=1 (normal)\n",
                (unsigned long long)b.cyc);

    // Small deterministic frame over the production bus (R9=6 even: settled
    // C9 parity aligns to frame parity, FR §19.5.5 p.214; odd-R9 balancing
    // stays with leaf t04j). 280 lines/field clears the crt_filter VSYNC
    // debounce on the normal filtered path.
    const char* setup = "setup";
    write_crtc(b, 0, 63, setup);
    write_crtc(b, 1, 40, setup);
    write_crtc(b, 2, 50, setup);
    write_crtc(b, 3, 0x22, setup);
    write_crtc(b, 4, 67, setup);
    write_crtc(b, 5, 0, setup);
    write_crtc(b, 6, 100, setup);
    write_crtc(b, 9, 6, setup);
    write_crtc(b, 7, 5, setup);
    write_crtc(b, 8, 0, setup);

    int failures = 0;

    // s1/s2: settled ordinary-R7 interlace via bus (R8=3 and R8=1).
    program_at_origin(b, 3, 5, "s1");
    failures += check_span(b, collect_two_settled_fields(b, "s1"),
                           "s1 Plus-R8=3/R7=5 settled", true, false, true);
    program_at_origin(b, 1, 5, "s2");
    failures += check_span(b, collect_two_settled_fields(b, "s2"),
                           "s2 Plus-R8=1/R7=5 settled", true, false, true);

    // z1/z2: R7=0 outgoing-parity exception (FR §19.7.3 p.219), both modes.
    // FIELD is NOT inverted: MID carries FIELD0, seam FIELD1 here.
    program_at_origin(b, 3, 0, "z1");
    failures += check_span(b, collect_two_settled_fields(b, "z1"),
                           "z1 Plus-R8=3/R7=0 outgoing-parity", true, true, true);
    program_at_origin(b, 1, 0, "z2");
    failures += check_span(b, collect_two_settled_fields(b, "z2"),
                           "z2 Plus-R8=1/R7=0 outgoing-parity", true, true, true);

    // t1: live entry at ODD raster. From progressive, wait for an odd raster
    // mid-line window, write R8=3 over the bus (RTL seeds ParityC9=C9.0=1
    // internally; never forced here), then watch the transition live.
    {
        const char* label = "t1 live-entry at odd raster";
        int local = 0;
        auto note = [&](const std::string& m) {
            std::printf("FAIL %s: %s\n", label, m.c_str());
            ++local;
        };
        program_at_origin(b, 0, 5, "t1-prog");
        // Reach a mid-frame odd raster away from the line-end edge.
        uint64_t guard = 0;
        while (!(*b.plus_row() == 3 && *b.plus_hcc() == 8)) {
            b.tick();
            if (++guard > kFieldTimeout) fail("t1: no odd-raster window");
        }
        const uint8_t field_before = b.dut.vsync_field;
        write_crtc(b, 8, 3, "t1-entry");
        // Landing line check: the two OUTs take ~2 chars; raster must still
        // be odd (documents the seeding the RTL just performed).
        if ((*b.plus_row() & 1) == 0)
            note("R8=3 entry did not land on an odd raster");
        // FIELD is combinational from frame parity + R8[0]: R8[0] went 0->1
        // mid-frame, so the pin may set here, but it must then hold stable
        // to the next origin (no mid-frame chatter).
        const uint8_t field_after = b.dut.vsync_field;
        // Hold watch starts at the odd-raster entry itself: check-then-tick
        // observes every interior state up to but excluding the origin edge.
        // The frame-origin parity transition may legitimately move FIELD
        // exactly at the boundary, so the arrived origin state is not
        // checked here; the settled span below pins the new pairing.
        uint64_t hold = 0;
        while (!at_plus_origin(b)) {
            if ((b.dut.vsync_field & 1) != (field_after & 1)) {
                note("FIELD moved mid-frame between entry and the next origin");
                break;
            }
            b.tick();
            ++hold;
            if (hold > kFieldTimeout) fail("t1: no origin after entry");
        }
        if (hold == 0)
            note("hold watch covered zero interior states (already at an origin at entry)");
        else
            std::printf("t1 hold: FIELD stable over %llu interior ticks to the next origin\n",
                        (unsigned long long)hold);
        (void)field_before;
        FieldSpan span = collect_two_settled_fields(b, "t1-settled");
        FieldSpan full;
        full.origins = span.origins;
        full.vsyncs = span.vsyncs;
        int sub = check_span(b, full, "t1 settled pairing after odd entry",
                             true, false, true);
        local += sub;
        if (local == 0) std::printf("PASS %s\n", label);
        failures += local;
    }

    // t2: live exit + production-consumer recovery. From settled R8=3,
    // write R8=0 mid-frame, then count three SELECTED (filtered) VSYNC
    // rises through the real consumer: history must flush to 0 and the
    // scandoubler must re-enable (documents the Amstrad.sv disable path).
    {
        const char* label = "t2 live-exit + consumer recovery";
        int local = 0;
        auto note = [&](const std::string& m) {
            std::printf("FAIL %s: %s\n", label, m.c_str());
            ++local;
        };
        program_at_origin(b, 3, 5, "t2-interlace");
        collect_two_settled_fields(b, "t2-settle"); // reach toggling state
        if (b.dut.scandoubler_o != 0)
            note("consumer enables the scandoubler while FIELD toggles");
        uint64_t guard = 0;
        while (!(*b.plus_row() == 2 && *b.plus_hcc() == 8)) {
            b.tick();
            if (++guard > kFieldTimeout) fail("t2: no mid-frame exit window");
        }
        write_crtc(b, 8, 0, "t2-exit");
        // FIELD must read 0 from the next raw VSYNC on (combinational gate).
        bool prev_raw = *b.plus_vsync_raw() != 0;
        guard = 0;
        bool seen_raw = false;
        while (!seen_raw) {
            b.tick();
            if (++guard > kFieldTimeout) fail("t2: no raw VSYNC after exit");
            bool raw = *b.plus_vsync_raw() != 0;
            if (raw && !prev_raw) {
                seen_raw = true;
                if ((b.dut.vsync_field & 1) != 0)
                    note("FIELD not 0 at the first raw VSYNC after R8=0 exit");
            }
            prev_raw = raw;
        }
        // Three selected (filtered) rises with FIELD=0 flush the consumer.
        bool prev_sel = b.dut.vsync_o != 0;
        unsigned rises = 0;
        guard = 0;
        while (rises < 3) {
            b.tick();
            if (++guard > kFieldTimeout) fail("t2: no 3 selected rises");
            bool sel = b.dut.vsync_o != 0;
            if (sel && !prev_sel) {
                ++rises;
                if ((b.dut.vsync_field & 1) != 0)
                    note("FIELD moved during progressive recovery");
            }
            prev_sel = sel;
        }
        if (b.dut.interlace_o != 0)
            note("consumer history not 0 after three progressive selected rises");
        if (b.dut.scandoubler_o != 1)
            note("consumer scandoubler not re-enabled after recovery");
        if (local == 0) std::printf("PASS %s\n", label);
        failures += local;
    }

    // p1/p2: progressive guards via bus.
    program_at_origin(b, 0, 5, "p1");
    failures += check_span(b, collect_two_settled_fields(b, "p1"),
                           "p1 Plus-R8=0 progressive", false, false, false);
    program_at_origin(b, 2, 5, "p2");
    failures += check_span(b, collect_two_settled_fields(b, "p2"),
                           "p2 Plus-R8=2 progressive guard", false, false, false);

    // n1: negative control. Plus R8=0 over the bus (also lands classic=0),
    // then poke the INACTIVE classic engine to 3. The pin must ignore it.
    program_at_origin(b, 0, 5, "n1");
    *b.classic_r8() = 3;
    b.tick();
    {
        // Span pins phase/length on the selected pin while perturbed; its
        // verdict is deferred (report=false) so this case prints one final
        // PASS/FAIL after the accept and consumer assertions below.
        FieldSpan span = collect_two_settled_fields(b, "n1");
        int sub = check_span(b, span, "n1 inactive-classic-R8=3 negative control",
                             false, false, false, false);
        // Count three ACTUAL selected filtered VSYNC rises while the classic
        // engine stays perturbed (8M explicit bound); the pin must read 0 at
        // every accept, and the consumer verdict below follows these
        // witnessed accepts rather than any preexisting state.
        bool prev_sel = b.dut.vsync_o != 0;
        unsigned sel_rises = 0;
        uint64_t m = 0;
        while (sel_rises < 3) {
            b.tick();
            if (++m > kN1AcceptTimeout) {
                std::printf("FAIL n1 inactive-classic-R8=3 negative control: "
                            "only %u/3 selected VSYNC rises while classic perturbed\n",
                            sel_rises);
                ++sub;
                break;
            }
            bool sel = b.dut.vsync_o != 0;
            if (sel && !prev_sel) {
                ++sel_rises;
                if ((b.dut.vsync_field & 1) != 0) {
                    std::printf("FAIL n1 inactive-classic-R8=3 negative control: "
                                "selected pin read 1 at accept %u while Plus R8=0\n",
                                sel_rises);
                    ++sub;
                }
            }
            prev_sel = sel;
        }
        if (sel_rises >= 3 &&
            (b.dut.interlace_o != 0 || b.dut.scandoubler_o != 1)) {
            std::printf("FAIL n1 inactive-classic-R8=3 negative control: "
                        "consumer moved on unselected toggling after %u witnessed accepts "
                        "(interlace_o=%u scandbl=%u)\n",
                        sel_rises, (unsigned)b.dut.interlace_o,
                        (unsigned)b.dut.scandoubler_o);
            ++sub;
        }
        if (sub == 0)
            std::printf("PASS n1 inactive-classic-R8=3 negative control "
                        "(%u selected accepts, consumer 0/enabled)\n", sel_rises);
        else
            std::printf("FAIL n1 inactive-classic-R8=3 negative control: "
                        "%d assertion(s) failed\n", sub);
        failures += sub;
    }
    *b.classic_r8() = 0;
    b.tick();

    if (failures != 0)
        fail("B8-2 FIELD ownership: " + std::to_string(failures) + " case(s) failed");
    std::printf("PASS B8-2: selected FIELD follows Plus R8/VSYNC phase over production I/O; "
                "consumer disables/recovers; inactive classic does not leak\n");
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    try {
        return run();
    } catch (const TestFailure& e) {
        std::printf("FAIL: %s\n", e.what());
        return 1;
    }
}
