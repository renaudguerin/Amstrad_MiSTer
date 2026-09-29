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
struct Fetch { unsigned addr, data, clocks, waits; bool rom; };
struct Harness {
    Vssm_provider_top d;
    std::array<unsigned char,65536> rom{}, ram{};
    std::vector<Fetch> fetches;
    Fetch pending{};
    bool fetching=false, ack=false;
    unsigned acks=0, io_reads=0, tick=0, hold=0, ppi_byte=0;
    bool stretch=false;
    Harness(bool plus=false) {
        d.plus_mode=plus; d.reset=1; d.irq=0; d.extra_wait=0;
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
            bool f=d.ssm_m1_fetch;
            if(f && !fetching) pending={d.cpu_addr,0,0,0,bool(d.romen)};
            if(stretch && f && d.cpu_addr==4) require(d.event_count==0,"marker published before held HH fetch completed");
            if(f) {pending.data=d.ssm_bus_data; ++pending.clocks; pending.waits+=!d.wait_n;}
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
static void marker(bool plus, bool stretch) {
    Harness h(plus); h.stretch=stretch;
    h.load(0,{0xf3,0xed,0xfe,0xed,0xff,0x76});
    h.run();
    h.sequence({0,1,2,3,4,5},{0xf3,0xed,0xfe,0xed,0xff,0x76});
    require(h.d.event_count==1 && h.d.last_code==0xfffe,"ROM marker missing or duplicated");
    for(auto f:h.fetches) require(f.rom,"ROM fetch selected RAM");
    require(h.fetches[4].waits>0,"GA/extra WAIT not exercised");
    if(stretch) require(h.hold==200 && h.fetches[4].clocks>=200,"HH fetch was not stretched");
    std::printf("PASS %s ROM marker stretch=%d HH clocks=%u WAIT=%u\n",plus?"Plus":"classic",stretch,h.fetches[4].clocks,h.fetches[4].waits);
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
int main(int argc,char**argv) {
    Verilated::commandArgs(argc,argv);
    try {marker(false,false); marker(true,true); ram_and_io(); interrupt(true); interrupt(false);}
    catch(const std::exception&e) {std::fprintf(stderr,"FAIL ssm-provider: %s\n",e.what()); return 1;}
    std::puts("ssm-provider: PASS 5 production motherboard/T80 cases");
}
