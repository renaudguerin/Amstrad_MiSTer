// B23: independent implementations on one master-clock timeline. No trace
// realignment or mismatch masks. Diagnostic completion is NOT parity success.
#include "Vb23_diff_top.h"
#include "verilated.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

struct Bus {
    unsigned mreq, iorq, rd, wr, m1, halt, a, d;
    unsigned pins() const { return mreq | iorq<<1 | rd<<2 | wr<<3 | m1<<4; }
    bool ack() const { return !iorq && !m1; }
    bool read() const { return !mreq && !rd; }
    bool write() const { return !mreq && !wr; }
};
struct Model {
    std::array<unsigned char,65536> mem{};
    Bus prev{1,1,1,1,1,1,0,0};
    int reads=0, writes=0, io_reads=0, io_writes=0, acks=0, handler=0;
    bool io_bus_ok=true;
    int io_read_ticks=0, io_write_ticks=0, ack_ticks=0, read_ticks=0, stack_writes=0;
    unsigned char input(const Bus& b) const {
        // A real memory read wins over the uninitialized startup IORQ.
        if (b.read()) return mem[b.a];
        if (b.ack()) return 0xff;
        if (!b.iorq && !b.rd) return 0x5a;
        return 0xff;
    }
    void observe(const Bus& b) {
        if (!b.iorq && b.m1 && (!b.rd || !b.wr))
            io_bus_ok &= b.a==0x1234 && (b.wr || b.d==0x99);
        if (b.read() && b.a==0x8000) {
            ++read_ticks;
            if (!prev.read() || prev.a!=b.a) ++reads;
        }
        if (b.write() && b.a==0x8000 && (!prev.write() || prev.a!=b.a)) ++writes;
        if (b.write() && b.a>=0xeffe && b.a<=0xefff && !prev.write()) ++stack_writes;
        if (!b.iorq && b.m1 && !b.rd) {
            ++io_read_ticks;
            if (prev.iorq || prev.rd) ++io_reads;
        }
        if (!b.iorq && b.m1 && !b.wr) {
            ++io_write_ticks;
            if (prev.iorq || prev.wr) ++io_writes;
        }
        if (b.ack()) { ++ack_ticks; if (!prev.ack()) ++acks; }
        if (b.read() && !b.m1 && b.a==0x38 && !prev.read()) ++handler;
        prev=b;
    }
};
void require(bool ok,const std::string& message) { if (!ok) throw std::runtime_error(message); }
std::array<Bus,3> buses(const Vb23_diff_top& t) {
#define BUS(p) Bus{t.p##_mreq_n,t.p##_iorq_n,t.p##_rd_n,t.p##_wr_n,t.p##_m1_n,t.p##_halt_n,t.p##_a,t.p##_do}
    return {BUS(t80),BUS(tv80),BUS(tv80_noiowait)};
#undef BUS
}
struct Result { int mismatches=0, startup=0; std::array<int,5> pin_diffs{}; std::array<Model,3> models; };
Result run(const std::string& scenario,bool trace) {
    Vb23_diff_top t;
    Result r;
    bool irq=scenario=="irq", io=scenario=="io", wait=scenario=="wait";
    // Two NOPs initialize production IntCycleD_n (not reset by T80pa.vhd).
    // We still compare and report startup; there is no CPU-specific alignment.
    std::vector<unsigned char> code={0x00,0x00};
    const std::vector<unsigned char> body=irq
        ? std::vector<unsigned char>{0x31,0x00,0xf0,0xed,0x56,0xfb,0x00,0x00}
        : io ? std::vector<unsigned char>{0x01,0x34,0x12,0x3e,0x99,0xed,0x79,0xed,0x78,0x32,0x02,0x80,0x76}
             : std::vector<unsigned char>{0x3e,0x42,0x32,0x00,0x80,0x3e,0x00,0x3a,0x00,0x80,0x32,0x01,0x80,0x76};
    code.insert(code.end(),body.begin(),body.end());
    for (auto& m:r.models) {
        std::copy(code.begin(),code.end(),m.mem.begin());
        m.mem[0x38]=0x76; // IM1 handler: HALT, no unsupported TV80 RETI.
    }
    t.clk=0; t.reset_n=0; t.cen_p=0; t.cen_n=0;
    t.wait_n=t.int_n=t.nmi_n=t.busrq_n=1;
    t.out0=t.r800_mode=0;
    t.t80_di=t.tv80_di=t.tv80_noiowait_di=0xff;
    t.eval();
    for (int i=0;i<8;++i) { t.clk=0;t.eval();t.clk=1;t.eval(); }
    t.reset_n=1;
    int wait_until=-1, wait_start=-1;
    bool done=false;
    for (int tick=0;tick<4096;++tick) {
        auto before=buses(t);
        // One external WAIT pulse, triggered only by the production data read.
        // All DUTs receive exactly the same signal; no per-DUT time shifting.
        if (wait && wait_start<0 && before[0].read() && before[0].a==0x8000) {
            wait_start=tick; wait_until=tick+48;
        }
        t.wait_n=!(tick<wait_until);
        t.int_n=!(irq && tick>=640);
        t.cen_p=(tick%16==0); t.cen_n=(tick%16==8);
        t.clk=0;
        t.t80_di=r.models[0].input(before[0]);
        t.tv80_di=r.models[1].input(before[1]);
        t.tv80_noiowait_di=r.models[2].input(before[2]);
        t.eval(); t.clk=1; t.eval();
        auto now=buses(t);
        // RAM service is independent of the transaction-coverage window.
        for (int i=0;i<3;++i) if(now[i].write()) r.models[i].mem[now[i].a]=now[i].d;
        if (tick<128) require(((now[0].pins()^now[1].pins()) & ~2u)==0,scenario+": unexpected startup pin mismatch");
        // Startup pins remain compared, but are not real transaction coverage.
        if (tick>=128) for (int i=0;i<3;++i) r.models[i].observe(now[i]);
        if (trace) std::printf("TRACE %s %d wait=%d T80=%02x/%04x TV80=%02x/%04x mutant=%02x/%04x\n",scenario.c_str(),tick,t.wait_n,now[0].pins(),now[0].a,now[1].pins(),now[1].a,now[2].pins(),now[2].a);
        if (now[0].pins()!=now[1].pins()) {
            if (tick>=128) for (int bit=0;bit<5;++bit)
                r.pin_diffs[bit]+=((now[0].pins()^now[1].pins())>>bit)&1;
            int& n=tick<128?r.startup:r.mismatches;
            if (n++==0) std::printf("DIFF %s %s tick=%d xor=%02x T80=%02x TV80=%02x A=%04x/%04x (bits M1 WR RD IORQ MREQ)\n",scenario.c_str(),tick<128?"startup":"body",tick,now[0].pins()^now[1].pins(),now[0].pins(),now[1].pins(),now[0].a,now[1].a);
        }
        // HALT is reached independently; don't stop on the first CPU alone.
        // IRQ scenarios also demand real acknowledge and handler fetch coverage.
        if (tick>128 && !now[0].halt && !now[1].halt && !now[2].halt &&
            (!irq || (r.models[0].handler && r.models[1].handler && r.models[2].handler))) {
            done=true; break;
        }
    }
    require(done,scenario+": timeout before all CPUs completed");
    for (int i=0;i<3;++i) {
        const auto& m=r.models[i];
        std::printf("COVER %s %s mem=%d/%d io=%d/%d ack=%d handler=%d widths(read/ioR/ioW/ack)=%d/%d/%d/%d stack=%d\n",scenario.c_str(),i==0?"T80":i==1?"TV80":"IOWait0",m.reads,m.writes,m.io_reads,m.io_writes,m.acks,m.handler,m.read_ticks,m.io_read_ticks,m.io_write_ticks,m.ack_ticks,m.stack_writes);
        if (irq) require(m.acks==1 && m.handler>=1,scenario+": missing acknowledge/handler");
        else if (io) require(m.io_reads==1 && m.io_writes==1 && m.io_bus_ok && m.mem[0x8002]==0x5a,scenario+": missing IO, wrong port/OUT data or bad IN readback");
        else require(m.reads==1 && m.writes==1 && m.mem[0x8000]==0x42 && m.mem[0x8001]==0x42,scenario+": missing memory transaction or bad readback");
    }
    if (wait) require(wait_start>=0, "WAIT was never asserted");
    std::printf("RESULT %s startup_mismatches=%d body_mismatches=%d pin_diffs(MREQ/IORQ/RD/WR/M1)=%d/%d/%d/%d/%d\n",scenario.c_str(),r.startup,r.mismatches,r.pin_diffs[0],r.pin_diffs[1],r.pin_diffs[2],r.pin_diffs[3],r.pin_diffs[4]);
    return r;
}
int main(int argc,char** argv) {
    Verilated::commandArgs(argc,argv);
    bool strict=false, memory=false, trace=false;
    for (int i=1;i<argc;++i) {
        std::string a=argv[i];
        if(a=="--strict") strict=true;
        else if(a=="--require-memory-parity") memory=true;
        else if(a=="--trace") trace=true;
        else { std::fprintf(stderr,"Unknown option: %s\n",argv[i]); return 2; }
    }
    try {
        auto mem=run("memory",trace), io=run("io",trace), irq=run("irq",trace), wait=run("wait",trace);
        // Production implementation is the oracle, not a copied pulse-length rule.
        require(io.mismatches==0,"I/O pin parity failed");
        require(io.models[0].io_read_ticks==io.models[1].io_read_ticks && io.models[0].io_write_ticks==io.models[1].io_write_ticks,"automatic I/O wait parity lost");
        require(io.models[2].io_read_ticks<io.models[0].io_read_ticks && io.models[2].io_write_ticks<io.models[0].io_write_ticks,"IOWait(0) control did not discriminate");
        for(int i=0;i<2;++i) require(wait.models[i].read_ticks>mem.models[i].read_ticks,"WAIT did not stretch both data reads");
        if(memory) require(mem.mismatches==0 && wait.mismatches==0,"memory pin parity failed");
        if(strict) require(mem.mismatches+mem.startup+io.mismatches+io.startup+irq.mismatches+irq.startup+wait.mismatches+wait.startup==0,"strict pin parity failed");
        std::puts("B23 COVERAGE PASS: all transactions completed; IOWait control discriminates; WAIT stretched both CPUs. See RESULT for parity differences.");
        return 0;
    } catch(const std::exception& e) { std::fprintf(stderr,"B23 FAIL: %s\n",e.what()); return 1; }
}
