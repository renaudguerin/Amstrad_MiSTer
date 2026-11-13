// Bounded checkpoint-INSTRUCTION replay, not original SHAKER acquisition.
// Reuse production motherboard/T80/READY. Test-only jumps select source blocks;
// no native media is modified and no expected hidden counter values are invented.
#include "Vssm_provider_top.h"
#include "verilated.h"
#include <array>
#include <cstdio>
#include <stdexcept>
#include <algorithm>
static void need(bool p,const char*s){if(!p)throw std::runtime_error(s);}
static void mix(uint64_t& h,uint64_t x){h^=x;h*=1099511628211ULL;}
struct H {
 Vssm_provider_top d;
 std::array<unsigned char,65536> rom{},ram{};
 std::array<uint64_t,130> ddr{};
 std::array<uint64_t,4> hash{{1469598103934665603ULL,1469598103934665603ULL,1469598103934665603ULL,1469598103934665603ULL}};
 std::array<std::array<uint32_t,4>,7> cuts{};
 std::array<unsigned,7> cut_ticks{};
 unsigned tick=0,completed=0,cut_count=0,fetch_addr=0;bool was_fetch=false;
 void put(unsigned a,std::initializer_list<unsigned char> b){std::copy(b.begin(),b.end(),ram.begin()+a);}
 H(bool on){
  d.plus_mode=0;d.irq=0;d.nmi=0;d.extra_wait=0;d.reset=1;d.s_trace_enable=on;d.s_trace_busy=0;
  // ROM disable operands mirrored into RAM, as in the existing provider.
  put(0,{0xF3,0x31,0xFE,0xFF,0x01,0,0x7F,0x3E,0x8C,0xED,0x79,0xC3,0xF3,0x5A});rom=ram;
  put(0x5AF3,{0xCD,0x72,0xA0}); // Original CALL A072; trampoline is fixture-only.
  put(0xA072,{0xC3,0xCF,0xA4});
  put(0xA4CF,{0x06,0xF5,0xC3,0xDE,0xA4});
  put(0xA4DE,{0x11,0xF5,0x4D,0x01,0x08,0xBC,0xED,0x49,0x01,0x03,0xBD,0xC3,0x67,0xA0});
  put(0xA067,{0xED,0x49,0xED,0x71,0x11,0xE3,0x4D,0xC3,0x00,0xA1});
  put(0xA100,{0x01,0x05,0xBC,0xED,0x49,0x01,0x08,0xBD,0xC3,0xFB,0x5B});
  put(0x5BFB,{0xED,0x49,0x01,0x07,0xBC,0xED,0x49,0x04,0xC3,0x0A,0x5B});
  put(0x5B0A,{0xED,0x71,0xC3,0x17,0x5B});
  put(0x5B17,{0x29,0x29,0x29,0x29,0xED,0xD3,0xED,0x02,0x76});
  for(unsigned i=0;i<128;++i)cycle();d.reset=0;
 }
 void cycle(){
  d.clk=0;d.eval();d.cpu_din=d.mem_rd?(d.romen?rom:ram)[d.cpu_addr]:255;
  // Same externally supplied DDR stall train in both cases, independent of DUT.
  d.s_trace_busy=(tick%43)<31;d.eval();
  if(!d.reset){
   if(d.mem_wr)ram[d.cpu_addr]=d.cpu_dout;
   if(d.ssm_m1_fetch)fetch_addr=d.cpu_addr;
   static const unsigned pc[]={0xA4CF,0xA4DE,0xA067,0xA06B,0x5BFB,0x5B0A,0x5B17};
   if(was_fetch&&!d.ssm_m1_fetch&&cut_count<7&&fetch_addr==pc[cut_count]){
    for(unsigned i=0;i<4;++i)cuts[cut_count][i]=d.s_trace_machine[i];
    cut_ticks[cut_count]=tick-128;++cut_count;
   }
   if(d.ssm_m1_fetch)mix(hash[0],(uint64_t(d.cpu_addr)<<8)|d.ssm_bus_data);
   if(was_fetch&&!d.ssm_m1_fetch)++completed;was_fetch=d.ssm_m1_fetch;
   mix(hash[1],(uint64_t(d.iorq)<<33)|(uint64_t(d.rd)<<32)|(uint64_t(d.cpu_addr)<<8)|d.cpu_dout);
   mix(hash[1],d.ssm_bus_data);
   for(unsigned i=0;i<4;++i)mix(hash[2],d.s_trace_machine[i]);
   mix(hash[3],d.s_trace_rgb);
   if(d.s_trace_ddr_we&&!d.s_trace_busy){need(d.s_trace_ddr_addr>=0x6000000&&d.s_trace_ddr_addr<0x6000082,"DDR range");ddr[d.s_trace_ddr_addr-0x6000000]=d.s_trace_ddr_data;}
  }
  d.clk=1;d.eval();++tick;
 }
 void run(){for(unsigned n=0;n<50000;++n)cycle();need(!d.cpu_halt_n,"replay HALT");need(d.event_count==1&&d.last_code==0x02D3,"genuine source S marker");}
};
int main(int argc,char**argv){Verilated::commandArgs(argc,argv);try{
 H off(false),on(true);off.run();on.run();need(off.hash==on.hash&&off.completed==on.completed,"observer perturbs production CPU/I/O/CRTC/RGB replay");
 need((off.ddr[1]&0xffffffff)==1&&(on.ddr[1]&0xffffffff)==16,"production completion transport");
 need(on.cut_count==7&&off.cuts==on.cuts,"seven direct live state cuts");
 for(unsigned i=0;i<7;++i)for(unsigned half=0;half<2;++half){
  unsigned slot=1+2*i+half;uint64_t a=on.ddr[2+2*slot],b=on.ddr[3+2*slot];
  uint64_t payload=(a>>16)|((b>>48)<<48);
  uint64_t direct=on.cuts[i][2*half]|(uint64_t(on.cuts[i][2*half+1])<<32);
  need(payload==direct&&((b>>16)&0xffffffff)==on.cut_ticks[i],"direct live cut/transport equality");
 }
 need(off.ddr[2]==on.ddr[2]&&off.ddr[3]==on.ddr[3],"ordinary S timestamp/position/observation changed");
 for(unsigned i=0;i<4;++i)std::printf("replay_hash[%u]=%016llx ",i,(unsigned long long)on.hash[i]);
 std::printf("\nSSM_S_TRACE_PROVIDER PASS production T80/READY replay equality, %u completed M1; not original live state\n",on.completed);
 return 0;
}catch(const std::exception&e){std::fprintf(stderr,"SSM_S_TRACE_PROVIDER FAIL %s\n",e.what());return 1;}}
