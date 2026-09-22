// B18 acceptance 2+3: executed classic program -> published SNA -> real owners.
// Expected bytes come from the program below and docs/references/formats/Snapshot
// (.SNA) file format.md, not from capture ports or decoder outputs. RAM uses
// an independent bank/page/offset pattern plus the program's specified writes.
#include "Vb18_snapshot_top.h"
#include "verilated.h"
#include <array>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>

static void require(bool b, const std::string& why) { if (!b) throw std::runtime_error(why); }
static std::string hx(unsigned v) { std::ostringstream s; s << std::hex << v; return s.str(); }
template<class W> static unsigned bits(const W& w, unsigned lo, unsigned n=8) {
    unsigned v=0; for(unsigned i=0;i<n;++i) v |= ((w[(lo+i)/32]>>((lo+i)%32))&1u)<<i; return v;
}
static uint8_t pattern(unsigned bank,unsigned offset) {
    return uint8_t((bank*71) ^ (offset*13) ^ (offset>>8) ^ ((offset>>14)*37));
}
struct Program {
    std::vector<uint8_t> bytes;
    std::array<uint8_t,256> header{};
    unsigned request_pc=0, capture_pc=0;
    void emit(std::initializer_list<uint8_t> b) {bytes.insert(bytes.end(),b);}
    void out(unsigned port,unsigned value) {emit({0x01,uint8_t(port),uint8_t(port>>8),0x3e,uint8_t(value),0xed,0x79});}
    void word(unsigned at,unsigned value) {header[at]=value;header[at+1]=value>>8;}
    Program(unsigned model,unsigned type) {
        std::copy_n("MV - SNA",8,header.begin());header[0x10]=3;
        emit({0xf3,0x31,0xf0,0xbf}); // DI; SP in base RAM bank 2
        out(0x7f00,0x9e); // mode 2, both ROMs disabled, clear GA interrupt count
        // Type 1 settles at zero in one-character/one-line frames. Type 0
        // starts at C0=C4=C9=R0=R4=R9=0: its second C0=0 increments C4
        // once and enters adjustment, then freezes. French ACCC v1.11
        // sections 13.2.1 pp.105-106 and 13.2.6 p.110. Sync is unreachable.
        const std::array<uint8_t,16> crtc={0,0,255,0x24,0,0,127,127,0,0,0x12,0x09,0x23,0x45,0x16,0x78};
        for(unsigned r=0;r<16;++r) {out(0xbc00,r);out(0xbd00,crtc[r]);header[0x43+r]=crtc[r];}
        out(0xbc00,13);header[0x42]=13;
        for(unsigned p=0;p<16;++p) {unsigned c=(p*7+3)&31;out(0x7f00,p);out(0x7f00,0x40|c);header[0x2f+p]=c;}
        out(0x7f00,16);out(0x7f00,0x5b);header[0x3f]=27;
        out(0x7f00,7);header[0x2e]=7;
        out(0x7f00,0xc4);header[0x41]=model==0?4:0; // page 12 at 4000 on 6128
        out(0xdf00,0x2d);header[0x55]=0x2d;
        out(0xef00,0x96);header[0xa1]=0x96; // printer output (unrestored)
        out(0xf700,0x82);header[0x59]=0x82; // A/C outputs, B input
        // AY-3-8912 implemented register widths: do not make raw unused-bit
        // retention a snapshot contract (tone coarse/envelope shape=4,
        // noise/amplitudes=5 bits). Distinct values remain in each register.
        const std::array<uint8_t,16> psg_mask={255,15,255,15,255,15,31,255,31,31,31,255,255,15,255,255};
        for(unsigned r=0;r<16;++r) {
            unsigned v=(r*11+5)&psg_mask[r];
            if(r==7) v=0x3f; // silent, keyboard port in input mode
            out(0xf400,r);out(0xf600,0xc0);out(0xf600,0);
            out(0xf400,v);out(0xf600,0x80);out(0xf600,0);
            header[0x5b+r]=v;
        }
        out(0xf400,14);out(0xf600,0xc0);out(0xf600,0x49);
        // HID row 9 maps joystick bit 0 to matrix bit 3, active low: F7.
        header[0x5a]=14;header[0x56]=0xf7;header[0x57]=0xf4;header[0x58]=0x49;
        // Distinct writes in remapped and base RAM, independently patched below.
        emit({0x3e,0xa9,0x32,0x00,0x40,0x3e,0x6c,0x32,0x00,0xa1});
        // Both AF banks loaded by PUSH/POP (flag value is specified, not ALU-derived).
        emit({0x21,0xa5,0x5a,0xe5,0xf1,0x08}); // alternate AF=5AA5
        emit({0x01,0x34,0x12,0x11,0x78,0x56,0x21,0xbc,0x9a,0xd9});
        emit({0x21,0x53,0xa6,0xe5,0xf1}); // main AF=A653
        emit({0x01,0x57,0x13,0x11,0x68,0x24,0x21,0x79,0x35,
              0xdd,0x21,0x8a,0x46,0xfd,0x21,0x9b,0x57});
        emit({0x3e,0x6d,0xed,0x47,0xed,0x5e}); // I=6D, IM2
        emit({uint8_t(model==2?0xf3:0xfb),0x00}); // DI or EI; NOP consumes EI deferral
        out(0x7f00,0x9e); // clear any interrupt accumulated during CRTC setup
        // OUT changed A/BC: restore the independent architectural values.
        emit({0x01,0x57,0x13,0x3e,0xd3,0xed,0x4f}); // R=D3
        request_pc=bytes.size();emit({0x3e,0xa6}); // M1 increments R to D4
        capture_pc=bytes.size();emit({0x00,0x3a,0x00,0x40,0x32,0x02,0xa1,0x76});
        header[0x11]=0x53;header[0x12]=0xa6;word(0x13,0x1357);word(0x15,0x2468);word(0x17,0x3579);
        header[0x19]=0xd4;header[0x1a]=0x6d;header[0x1b]=header[0x1c]=model==2?0:1;
        word(0x1d,0x468a);word(0x1f,0x579b);word(0x21,0xbff0);word(0x23,capture_pc);header[0x25]=2;
        header[0x26]=0xa5;header[0x27]=0x5a;word(0x28,0x1234);word(0x2a,0x5678);word(0x2c,0x9abc);
        header[0x40]=0x8e;header[0x6b]=model==0?128:64;header[0x6d]=2-model;header[0xa4]=type;
        header[0xb2]=2; // reset hcnt encoded 00 -> two lines until VSYNC
        if(type==0) {header[0xab]=1;header[0xb0]=0x80;}
        // Type 1 starts VSYNC with reset R7=0. Writing R7=127 prevents
        // subsequent starts; its fixed 16-line pulse expires with elapsed
        // count 15 retained (SNA v3 AF, not the inactive pulse's countdown).
        else header[0xaf]=15;
    }
};
struct Bench {
    Vb18_snapshot_top d;
    std::vector<uint8_t> memory=std::vector<uint8_t>(4*0x800000,0xff);
    std::vector<uint8_t> slot=std::vector<uint8_t>(16+0x20100,0xa7);
    uint64_t ticks=0,stalls=0,writes=0;
    bool cart_seen=false,stalled=false;
    uint64_t last_data=0;unsigned last_addr=0;
    unsigned idx(unsigned bank,unsigned addr) {require(bank<4 && addr<0x800000,"memory bounds");return bank*0x800000+addr;}
    void putword(unsigned off,uint64_t v) {for(unsigned i=0;i<8;++i) slot.at(off+i)=uint8_t(v>>(8*i));}
    uint64_t word(unsigned off) {uint64_t v=0;for(unsigned i=0;i<8;++i)v|=uint64_t(slot.at(off+i))<<(8*i);return v;}
    void tick() {
        d.clk=0;d.eval();
        d.mem_din=memory[idx(d.mem_bank,d.mem_addr)];
        d.cart_ack=0;
        if(!d.cart_req)cart_seen=false;
        if(d.cart_req&&!cart_seen&&ticks%5==0) {
            d.cart_dout=memory[idx(d.cart_bank,d.cart_addr)];d.cart_ack=1;cart_seen=true;
        }
        d.ddram_busy=(ticks%19<7)||(ticks%113<13);d.eval();
        if(stalled)require(d.ddram_we && d.ddram_addr==last_addr && d.ddram_din==last_data,"DDR transaction changed under stall");
        stalled=d.ddram_we&&d.ddram_busy;
        if(stalled) {++stalls;last_addr=d.ddram_addr;last_data=d.ddram_din;}
        if(d.ddram_we&&!d.ddram_busy) {require(d.ddram_addr>=0x7c00000,"DDR base");putword((d.ddram_addr-0x7c00000)*8,d.ddram_din);++writes;}
        if(d.mem_wr&&!d.owner_reset)memory[idx(d.mem_bank,d.mem_addr)]=d.mem_dout;
        if(d.load_mem_wr)memory[idx(d.load_mem_bank,d.load_mem_addr)]=d.load_data;
        d.clk=1;d.eval();++ticks;d.clk=0;d.eval();
    }
    void run(unsigned n) {while(n--)tick();}
    template<class F> void until(F f,unsigned limit,const std::string& why) {while(!f()&&limit--)tick();require(f(),why+" pc="+hx(d.cpu_addr));}
};
static void bytecheck(unsigned at,unsigned got,unsigned expected,const std::string& stage) {
    require(got==expected,stage+" byte "+hx(at)+" expected="+hx(expected)+" got="+hx(got));
}
static void run_case(unsigned model,unsigned type) {
    Bench b;Program p(model,type);auto& d=b.d;
    const unsigned ram_size=model==0?0x20000:0x10000;
    std::vector<uint8_t> expected(ram_size);
    for(unsigned bank=0;bank<3;++bank)for(unsigned a=0;a<0x20000;++a)b.memory[b.idx(bank,0x20000+a)]=pattern(bank,a);
    for(unsigned a=0;a<ram_size;++a)expected[a]=pattern(model,a);
    for(unsigned a=0;a<p.bytes.size();++a) {b.memory[b.idx(model,a)]=p.bytes[a];b.memory[b.idx(model,0x20000+a)]=p.bytes[a];expected[a]=p.bytes[a];}
    expected[model==0?0x10000:0x4000]=0xa9;expected[0xa100]=0x6c;
    expected[0xbfee]=0x53;expected[0xbfef]=0xa6; // second PUSH HL overwrites first
    d.model=model;d.crtc_type=type;d.reset_btn=1;b.run(1024);d.reset_btn=0;
    b.until([&]{return d.cpu_insn_start&&d.cpu_addr==p.request_pc;},1000000,"program never reached capture request");
    // The request occurs while INSN_START is high; the controller must wait
    // for the following instruction edge. That next instruction is the NOP.
    b.putword(0,77);b.putword(8,(ram_size+256)/4);
    auto before=b.slot;
    d.save_req=1;b.tick();d.save_req=0;
    b.until([&]{return d.captured;},10000,"save not captured");
    require(d.save_hold,"capture did not hold CPU");
    for(unsigned i=0;i<256;++i)bytecheck(i,bits(d.saved_header,i*8),p.header[i],"capture");
    b.until([&]{return b.writes>20;},100000,"no partial publication");
    require(b.word(0)==UINT64_MAX,"host must see invalid generation during stream");
    // Capture a host read begun under the old header and copying mid-publication.
    auto torn=b.slot;std::copy_n(before.begin(),16,torn.begin());
    b.until([&]{return d.save_done;},5000000,"save did not finish");
    require(b.stalls>100,"DDR stall stimulus did not exercise writer");
    require(b.word(0)==1 && b.word(8)==(ram_size+256)/4,"final publication words");
    require(!std::equal(before.begin(),before.begin()+16,b.slot.begin()),"overlapping host read was not invalidated");
    for(unsigned i=0;i<256;++i)bytecheck(i,b.slot[16+i],p.header[i],"published header");
    for(unsigned i=0;i<ram_size;++i)bytecheck(i,b.slot[272+i],expected[i],"published RAM");
    // Preserve real RTL publication samples for the production Python host reader.
    const std::string stem="obj_dir/b18_snapshot/model"+std::to_string(model)+"-type"+std::to_string(type);
    auto dump=[&](const std::string& suffix,const std::vector<uint8_t>& v) {std::ofstream f(stem+suffix,std::ios::binary);f.write(reinterpret_cast<const char*>(v.data()),v.size());require(bool(f),"write host samples");};
    dump("-before.bin",before);dump("-torn.bin",torn);dump("-final.bin",b.slot);
    // Destroy every RAM byte first: a no-op reload must fail the comparison.
    for(unsigned a=0;a<ram_size;++a)b.memory[b.idx(model,0x20000+a)]=uint8_t(expected[a]^0xff);
    d.model=(model+1)%3; // header selection must override the initial menu bank
    d.sna_download=1;b.run(4);
    for(unsigned i=0;i<ram_size+256;++i) {d.load_wr=1;d.load_addr=i;d.load_data=b.slot[16+i];b.tick();d.load_wr=0;b.tick();}
    d.sna_download=0;
    b.until([&]{return d.sna_load;},100,"no apply pulse");
    require(!d.owner_reset&&d.sna_hold,"production apply must release reset and hold execution");b.tick();
    // SNA field -> architectural REG mapping from T80pa interface; every bit.
    std::array<uint32_t,7> cpu{};
    auto put=[&](unsigned lo,unsigned n,unsigned value){for(unsigned i=0;i<n;++i)cpu[(lo+i)/32]|=((value>>i)&1u)<<((lo+i)%32);};
    const std::array<unsigned,29> regbits={8,0,80,88,96,104,112,120,40,32,210,211,128,136,192,200,48,56,64,72,208,24,16,144,152,160,168,176,184};
    for(unsigned i=0;i<29;++i)put(regbits[i],i==10||i==11?1:i==20?2:8,p.header[0x11+i]);
    for(unsigned i=0;i<212;++i)require(bits(d.cpu_reg,i,1)==bits(cpu,i,1),"restored CPU bit "+std::to_string(i));
    // Classic owners consume GA/CRTC registers, MMU and PPI/PSG, but not
    // printer/FDC/CRTC counters/GA IRQ phase. Compare only consumed fields.
    for(unsigned i=0x2e;i<=0x6a;++i)bytecheck(i,bits(d.hw_header,(i-0x2e)*8),p.header[i],"restored owner");
    bytecheck(0x56,d.ppi_a_latch,0xf7,"restored PPI A input, not old output latch 0x0e");
    bytecheck(0x57,d.ppi_b_latch,0xf4,"restored PPI B input");
    require(d.active_model==model,"restored classic model bank");
    for(unsigned i=0;i<ram_size;++i)bytecheck(i,b.memory[b.idx(model,0x20000+i)],expected[i],"restored RAM");
    b.until([&]{return !d.cpu_halt_n;},10000,"restored CPU did not execute continuation");
    bytecheck(0xa102,b.memory[b.idx(model,0x2a102)],0xa9,"continuation read through restored RAM map");
    std::cout<<"PASS B18 model="<<model<<" CRTC="<<type<<" header=256 RAM="<<ram_size<<" stalls="<<b.stalls<<"\n";
}
int main(int argc,char** argv) {
    Verilated::commandArgs(argc,argv);
    try {for(unsigned model=0;model<3;++model)for(unsigned type=0;type<2;++type)run_case(model,type);}
    catch(const std::exception& e) {std::cerr<<"FAIL B18: "<<e.what()<<"\n";return 1;}
    std::cout<<"B18 snapshot: PASS 6 classic capture/restore cases\n";return 0;
}
