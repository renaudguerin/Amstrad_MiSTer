#include "Vssm_provider_top.h"
#include "verilated.h"
#include <array>
#include <vector>
#include <cstdio>
#include <stdexcept>
#include <algorithm>

static void require(bool ok, const char* why) {
    if (!ok) throw std::runtime_error(why);
}
struct Fetch { unsigned addr, data, clocks, waits; bool rom, observed; };
struct Harness {
    Vssm_provider_top d;
    std::array<unsigned char,65536> rom{}, ram{};
    std::vector<Fetch> fetches;
    Fetch pending{};
    bool fetching=false, ack=false;
    unsigned acks=0, io_reads=0, tick=0, hold=0, ppi_byte=0;
    bool stretch=false;
    Harness(bool plus=false) {
        d.plus_mode=plus; d.reset=1; d.irq=0; d.nmi=0; d.extra_wait=0;
        for (int i=0;i<128;++i) cycle();
        d.reset=0;
    }
    void load(unsigned addr, std::initializer_list<unsigned char> bytes, bool in_rom=true) {
        std::copy(bytes.begin(),bytes.end(),(in_rom?rom:ram).begin()+addr);
    }
    void settle() {
        d.clk=0; d.eval();
        // Fixture's external memory responder. ROM selection comes from the
        // real motherboard GA/MMU; peripheral reads leave the bus neutral.
        d.cpu_din=d.mem_rd ? (d.romen?rom:ram)[d.cpu_addr] : 0xff;
        if (stretch && d.ssm_m1_fetch && d.cpu_addr==4 && hold<200) {
            d.extra_wait=1; d.cpu_din=0x00; ++hold;
        } else d.extra_wait=0;
        d.eval();
    }
    void cycle() {
        settle();
        if (!d.reset) {
            // Trace completed physical CPU reads independently. The detector
            // remains connected only to the motherboard's qualified provider.
            // NMI wake can release HALT_n partway through a physical read.
            bool f=d.m1 && d.mreq && d.rd;
            if(f && !fetching) pending={d.cpu_addr,0,0,0,bool(d.romen),false};
            if(stretch && f && d.cpu_addr==4) require(d.event_count==0,"marker published before held HH fetch completed");
            if(f) {pending.data=d.ssm_bus_data; ++pending.clocks; pending.waits+=!d.wait_n; pending.observed|=d.ssm_m1_fetch;}
            if(!f && fetching) fetches.push_back(pending);
            fetching=f;
            // T80pa IntCycleD_n is not reset: its initial M1 may assert
            // IORQ alongside MREQ. An acknowledge has MREQ inactive.
            bool a=d.m1 && d.iorq && !d.mreq;
            if(a && !ack) ++acks;
            ack=a;
            if(d.iorq && d.rd && !d.mreq) {
                ++io_reads; ppi_byte=d.ssm_bus_data;
                // Motherboard ppi_ipb = {tape_in, 2'b11, jumpers, VS}.
                // Fixture tape=1 and jumpers=1010 give F4/F5; VS is live.
                require((ppi_byte & 0xfe)==0xf4,"PPI bus responder missing");
                require(!f,"I/O read advertised as opcode fetch");
            }
            if(d.mem_wr) ram[d.cpu_addr]=d.cpu_dout;
        }
        d.clk=1; d.eval(); ++tick;
    }
    void run() {
        for(unsigned n=0;n<100000;++n) {
            cycle();
            if(!d.cpu_halt_n) {for(int j=0;j<8;++j) cycle(); return;}
        }
        throw std::runtime_error("CPU failed to HALT");
    }
    void sequence(const std::vector<unsigned>& addr, const std::vector<unsigned>& bytes) {
        if(fetches.size()!=addr.size()) for(auto f:fetches) std::fprintf(stderr,"fetch %04x:%02x clocks=%u\n",f.addr,f.data,f.clocks);
        require(fetches.size()==addr.size(),"unexpected completed opcode-fetch count");
        for(unsigned i=0;i<addr.size();++i) {
            require(fetches[i].addr==addr[i],"opcode fetch address/order mismatch");
            require(fetches[i].data==bytes[i],"opcode fetch bus data mismatch");
        }
    }
};
static void marker(bool plus, bool stretch, unsigned char low=0xfe, unsigned char high=0xff) {
    Harness h(plus); h.stretch=stretch;
    h.load(0,{0xf3,0xed,low,0xed,high,0x76});
    h.run();
    h.sequence({0,1,2,3,4,5},{0xf3,0xed,low,0xed,high,0x76});
    require(h.d.event_count==1 && h.d.last_code==((unsigned(high)<<8)|low),"ROM marker missing or duplicated");
    for(auto f:h.fetches) require(f.rom,"ROM fetch selected RAM");
    require(h.fetches[4].waits>0,"GA/extra WAIT not exercised");
    if(stretch) require(h.hold==200 && h.fetches[4].clocks>=200,"HH fetch was not stretched");
    std::printf("PASS %s ROM marker code=%04x stretch=%d HH clocks=%u WAIT=%u\n",plus?"Plus":"classic",unsigned(h.d.last_code),stretch,h.fetches[4].clocks,h.fetches[4].waits);
}
static void ram_and_io() {
    Harness h;
    // Disable ROM overlays by GA configuration, jump into RAM. Read PPI B
    // through the resolved motherboard bus, then execute the marker there.
    h.load(0,{0xf3,0x01,0x00,0x7f,0x3e,0x8c,0xed,0x79,0xc3,0x00,0x40});
    // Jump operands must be visible in RAM once the GA disables low ROM.
    h.ram=h.rom;
    h.load(0x4000,{0x01,0x00,0xf5,0xed,0x78,0x32,0x00,0x50,0xed,0xfe,0xed,0xff,0x76},false);
    h.run();
    require(h.io_reads>0,"PPI read not executed");
    require(h.ram[0x5000]==h.ppi_byte,"CPU did not consume resolved PPI bus byte");
    require(h.d.event_count==1 && h.d.last_code==0xfffe,"RAM marker missing");
    for(auto f:h.fetches) if(f.addr>=0x4000) require(!f.rom,"RAM fetch selected ROM");
    h.sequence({0,1,4,6,7,8,0x4000,0x4003,0x4004,0x4005,0x4008,0x4009,0x400a,0x400b,0x400c},
               {0xf3,0x01,0x3e,0xed,0x79,0xc3,0x01,0xed,0x78,0x32,0xed,0xfe,0xed,0xff,0x76});
    std::puts("PASS ROM-to-RAM mapping, operands excluded, PPI resolved data consumed / I/O excluded");
}
static void interrupt(bool enabled) {
    Harness h;
    h.load(0,{0x31,0x00,0x80,0xed,0x56,static_cast<unsigned char>(enabled?0xfb:0xf3),0xed,0xfe,0xed,0xff,0x76});
    h.load(0x38,{0x76}); h.d.irq=1;
    h.run();
    if(enabled) {
        h.sequence({0,3,4,5,6,7,0x38},{0x31,0xed,0x56,0xfb,0xed,0xfe,0x76});
        require(h.acks==1,"IM1 acknowledge missing/duplicated");
        require(h.d.event_count==0,"interrupt failed to break marker");
        require(h.ram[0x7ffe]==8 && h.ram[0x7fff]==0,"IM1 pushed wrong return address");
        // T80 increments PC at M1/T2 before decoding HALT, then sets
        // Halt_FF at T_Res. This live successor is not the fetch address.
        std::printf("PASS IM1: ACK=%u return=0008 handler-fetch=0038:76 HALT-address=%04x\n",h.acks,h.d.cpu_addr);
    } else {
        require(h.acks==0 && h.d.event_count==1 && h.d.last_code==0xfffe,"DI control failed");
        std::puts("PASS DI control: uninterrupted ED FE ED FF marker");
    }
}
static void halt_then_interrupt() {
    Harness h;
    // LD SP,8000; IM 1; EI; ED FE; HALT. HALT must cancel that partial
    // marker before the handler's ED FF. A full marker follows HALT in ROM,
    // to be executed only after the interrupt returns.
    h.load(0,{0x31,0x00,0x80,0xed,0x56,0xfb,0xed,0xfe,0x76,
              0xed,0xfe,0xed,0xff,0x3e,0x5a,0x32,0x00,0x50,0x76});
    h.load(0x38,{0xed,0xff,0xc9});
    h.run();
    h.sequence({0,3,4,5,6,7,8},{0x31,0xed,0x56,0xfb,0xed,0xfe,0x76});
    require(h.fetches.back().observed,"initial HALT opcode missing from SSM cancellation stream");

    // T80.vhd's M1/T2 holds PC while Halt_FF is set and substitutes NOP
    // into IR, while T_Res still drives A from PC. The raw bus therefore
    // repeats 0009:ED, although the CPU executes none of the successor bytes.
    // Count completed physical reads, rather than time spent at a live PC.
    const unsigned initial_fetches=h.fetches.size();
    for(unsigned n=0;h.fetches.size()<initial_fetches+12 && n<100000;++n) {
        h.cycle();
        require(!h.d.cpu_halt_n,"CPU left HALT before IRQ");
    }
    require(h.fetches.size()==initial_fetches+12,"repeated HALT M1 reads missing");
    // Allow the detector's fetch-end / recognition / event pipeline to drain.
    for(unsigned n=0;n<8;++n) h.cycle();
    const unsigned halted_events=h.d.event_count;
    const unsigned halted_code=h.d.last_code;

    h.d.irq=1;
    for(unsigned n=0;h.acks==0 && n<100000;++n) h.cycle();
    require(h.acks==1,"HALT did not wake through IM1 acknowledge");
    const unsigned before_handler=h.fetches.size();
    // No real opcode fetch can overlap the acknowledge. Any fetch completing
    // before this bus boundary must still be the held successor read.
    require(!h.d.ssm_m1_fetch,"IM1 acknowledge contributed an opcode byte");
    h.d.irq=0;
    for(unsigned n=0;!h.d.cpu_halt_n && n<100000;++n) h.cycle();
    require(h.d.cpu_halt_n,"CPU stayed halted after IM1 acknowledge");
    h.run();

    std::vector<unsigned> addresses={0,3,4,5,6,7,8};
    std::vector<unsigned> bytes={0x31,0xed,0x56,0xfb,0xed,0xfe,0x76};
    for(unsigned n=initial_fetches;n<before_handler;++n) {
        addresses.push_back(9); bytes.push_back(0xed);
    }
    const std::vector<unsigned> tail_addresses={0x38,0x39,0x3a,9,10,11,12,13,15,18};
    const std::vector<unsigned> tail_bytes={0xed,0xff,0xc9,0xed,0xfe,0xed,0xff,0x3e,0x32,0x76};
    addresses.insert(addresses.end(),tail_addresses.begin(),tail_addresses.end());
    bytes.insert(bytes.end(),tail_bytes.begin(),tail_bytes.end());
    h.sequence(addresses,bytes);
    require(h.acks==1,"HALT wake acknowledged more than once");
    require(h.ram[0x7ffe]==9 && h.ram[0x7fff]==0,"HALT wake pushed wrong return address");
    require(h.ram[0x5000]==0x5a,"CPU did not execute the returned successor program");
    std::printf("HALT/IM1: repeated 0009:ED reads=%u halted-events=%u halted-code=%04x "
                "ACK=%u return=0009 resumed-events=%u final-code=%04x\n",
                before_handler-initial_fetches,halted_events,halted_code,h.acks,
                unsigned(h.d.event_count)-halted_events,unsigned(h.d.last_code));
    require(h.d.event_count==halted_events+1 && h.d.last_code==0xfffe,"HALT cancellation or resumed marker count failed");
    require(halted_events==0,"repeated HALT bus reads published spurious SSM marker");
    for(unsigned n=initial_fetches;n<before_handler;++n)
        require(!h.fetches[n].observed,"physical HALT read entered SSM provider");
    std::puts("PASS repeated HALT M1, IM1 wake/return, one executed successor marker");
}
static void halt_then_nmi() {
    Harness h;
    h.load(0,{0x31,0x00,0x80,0xed,0xfe,0x76,
              0xed,0xfe,0xed,0xff,0x3e,0x5a,0x32,0x01,0x50,0x76});
    h.load(0x66,{0xed,0xff,0xed,0x45}); // partial pair, then RETN
    h.run();
    h.sequence({0,3,4,5},{0x31,0xed,0xfe,0x76});
    require(h.fetches.back().observed,"initial HALT opcode missing before NMI");
    const unsigned initial_fetches=h.fetches.size();
    for(unsigned n=0;h.fetches.size()<initial_fetches+12 && n<100000;++n) h.cycle();
    require(h.fetches.size()==initial_fetches+12 && !h.d.cpu_halt_n,"HALT reads missing before NMI");
    h.d.nmi=1;
    for(unsigned n=0;!h.d.cpu_halt_n && n<100000;++n) h.cycle();
    require(h.d.cpu_halt_n,"NMI did not release HALT");
    h.d.nmi=0;
    h.run();

    // An NMI acknowledge is a physical M1 memory read, unlike IM1's I/O
    // acknowledge. T80 forces IR=NOP and leaves PC=0006 during that read.
    // HALT_n rises before its data phase ends, so qualify the entire read.
    unsigned handler=initial_fetches;
    while(handler<h.fetches.size() && h.fetches[handler].addr==6) {
        require(h.fetches[handler].data==0xed,"unexpected NMI wake dummy bus byte");
        ++handler;
    }
    std::vector<unsigned> addresses={0,3,4,5};
    std::vector<unsigned> bytes={0x31,0xed,0xfe,0x76};
    for(unsigned n=initial_fetches;n<handler;++n) {
        addresses.push_back(6); bytes.push_back(0xed);
    }
    const std::vector<unsigned> tail_addresses={0x66,0x67,0x68,0x69,6,7,8,9,10,12,15};
    const std::vector<unsigned> tail_bytes={0xed,0xff,0xed,0x45,0xed,0xfe,0xed,0xff,0x3e,0x32,0x76};
    addresses.insert(addresses.end(),tail_addresses.begin(),tail_addresses.end());
    bytes.insert(bytes.end(),tail_bytes.begin(),tail_bytes.end());
    h.sequence(addresses,bytes);
    require(handler-initial_fetches>=13,"physical NMI acknowledge read missing");
    require(h.acks==0,"NMI incorrectly used maskable interrupt acknowledge");
    require(h.ram[0x7ffe]==6 && h.ram[0x7fff]==0,"NMI pushed wrong HALT return address");
    require(h.ram[0x5001]==0x5a,"RETN did not execute the resumed program");
    unsigned leaked=0;
    for(unsigned n=initial_fetches;n<handler;++n) leaked+=h.fetches[n].observed;
    std::printf("HALT/NMI: physical successor reads=%u leaked-provider-reads=%u return=0006 events=%u code=%04x\n",
                handler-initial_fetches,leaked,unsigned(h.d.event_count),unsigned(h.d.last_code));
    require(leaked==0,"NMI wake exposed an unexecuted HALT successor byte");
    require(h.d.event_count==1 && h.d.last_code==0xfffe,"NMI cancellation or resumed marker count failed");
    std::puts("PASS HALT NMI dummy-read exclusion, RETN, one executed successor marker");
}
int main(int argc,char**argv) {
    Verilated::commandArgs(argc,argv);
    try {marker(false,false); marker(true,true); marker(false,false,0xed,0xed); ram_and_io(); interrupt(true); interrupt(false); halt_then_interrupt(); halt_then_nmi();}
    catch(const std::exception&e) {std::fprintf(stderr,"FAIL ssm-provider: %s\n",e.what()); return 1;}
    std::puts("ssm-provider: PASS 8 production motherboard/T80 cases");
}
