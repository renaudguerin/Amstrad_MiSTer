#include "Vssm_s_trace_top.h"
#include "verilated.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
static void need(bool p,const char* s){if(!p)throw std::runtime_error(s);}
struct H {
 Vssm_s_trace_top d; std::array<uint64_t,130> mem{}; unsigned ticks=0;bool held=false;uint64_t held_data=0;unsigned held_addr=0;
 void cycle(){d.clk=0;d.eval();if(held)need(d.we&&d.addr==held_addr&&d.data==held_data,"Avalon request changed under stall");held=d.we&&d.busy;if(held){held_addr=d.addr;held_data=d.data;}if(d.we&&!d.busy){need(d.addr>=0x6000000&&d.addr<0x6000082,"DDR address");mem[d.addr-0x6000000]=d.data;}d.clk=1;d.eval();++ticks;}
 void idle(unsigned n=20){while(n--)cycle();}
 H(){d.reset=1;d.trace_enable=1;d.qualified=1;d.m1_fetch=0;d.busy=0;d.snapshot[0]=0;d.snapshot[1]=0;d.snapshot[2]=0;d.snapshot[3]=0;idle(4);d.reset=0;idle();}
 unsigned fetch(unsigned a,unsigned b,unsigned hold=5){d.fetch_addr=a;d.bus_data=b;d.m1_fetch=1;idle(hold);d.m1_fetch=0;unsigned t=ticks-4;cycle();idle(3);return t;}
 void marker(unsigned code){fetch(0x9000,0xED);fetch(0x9001,code&255);fetch(0x9002,0xED);fetch(0x9003,code>>8);idle();}
 unsigned written(){return mem[1]&0xffffffff;}
 uint16_t code(unsigned i){return mem[2+2*i]&65535;}
 uint64_t payload(unsigned i){return (mem[2+2*i]>>16)|((mem[3+2*i]>>48)<<48);}
 uint32_t stamp(unsigned i){return (mem[3+2*i]>>16)&0xffffffff;}
};
static const unsigned pc[]={0xA4CF,0xA4DE,0xA067,0xA06B,0x5BFB,0x5B0A,0x5B17};
static const unsigned op[]={0x06,0x11,0xED,0x11,0xED,0xED,0x29};
static void tail(H& h,unsigned i){
 if(i==2){h.fetch(0xA068,0x49);h.fetch(0xA069,0xED);h.fetch(0xA06A,0x71);}
 else if(i==4)h.fetch(0x5BFC,0x49);
 else if(i==5)h.fetch(0x5B0B,0x71);
 else h.fetch(0x9000,0x49);
}
static void run(){
 H h; h.fetch(0x5AF3,0xCD); std::array<unsigned,7> t{};
 h.d.busy=1;
 for(unsigned i=0;i<7;++i){h.d.snapshot[0]=0x10203040+i;h.d.snapshot[1]=0x50607080+i;h.d.snapshot[2]=0x90A0B0C0+i;h.d.snapshot[3]=0xD0E0F000+i;t[i]=h.fetch(pc[i],op[i],i+4);tail(h,i);if(i==0)h.fetch(pc[0],op[0],33);}
 need(h.written()==0,"capture must not publish before genuine S");
 h.marker(0x02D3); need(h.written()==0,"DDR stall must hold S and all private export");
 // Stall the *S header* after both ordinary payload words have committed.
 h.d.busy=0;
 for(unsigned n=0;n<40;++n){h.d.clk=0;h.d.eval();if(h.d.we&&h.d.addr==0x6000001&&h.d.data==1)break;h.cycle();}
 h.d.busy=1;h.idle(30);need(h.written()==0,"unaccepted S header cannot release private export");
 h.d.busy=0;h.idle(200);
 need(h.written()==16,"expected S plus fourteen payloads and status");need(h.code(0)==0x02D3,"ordinary S preserved");
 for(unsigned i=0;i<7;++i){need(h.code(1+2*i)==0xFF20+i&&h.code(2+2*i)==0xFF28+i,"ordered private tags");need(h.payload(1+2*i)==((uint64_t(0x50607080+i)<<32)|(0x10203040+i)),"first snapshot half");need(h.payload(2+2*i)==((uint64_t(0xD0E0F000+i)<<32)|(0x90A0B0C0+i)),"second snapshot half");need(h.stamp(1+2*i)==t[i]&&h.stamp(2+2*i)==t[i],"M1 completion timestamp");}
 need(h.code(15)==0xFF2F && (h.payload(15)&0xffff)==0x0107,"complete status v1/count7");need(h.d.event_count==1&&h.d.dropped_count==0,"private export is not marker recognition");
 // A genuine marker arriving during private transport keeps its own pending slot.
 H q;q.fetch(0x5AF3,0xCD);for(unsigned i=0;i<7;++i){q.fetch(pc[i],op[i]);tail(q,i);}q.marker(0x02D3);q.marker(0x1234);q.idle(250);need(q.written()==17&&q.d.event_count==2&&q.d.dropped_count==0,"ordinary marker during private flush");unsigned seen=0;for(unsigned i=0;i<17;++i)seen+=q.code(i)==0x1234;need(seen==1,"ordinary marker exactly once");
 for(unsigned bad=0;bad<3;++bad){H x;x.fetch(0x5AF3,0xCD);x.d.qualified=(bad!=0);x.fetch(pc[bad==2?1:0],bad==1?0:op[bad==2?1:0]);x.d.qualified=1;x.marker(0x02D3);x.idle(100);need(x.written()==2&&x.code(1)==0xFF2F,"invalid acquisition emits status only");need((x.payload(1)>>16)&255,"invalid source/mapping/order flag");}
 H wrong_tail;wrong_tail.fetch(0x5AF3,0xCD);
 for(unsigned i=0;i<3;++i){wrong_tail.fetch(pc[i],op[i]);if(i<2)tail(wrong_tail,i);}
 wrong_tail.fetch(0xA068,0x71);wrong_tail.marker(0x02D3);wrong_tail.idle(200);
 need(wrong_tail.written()==8&&wrong_tail.code(7)==0xFF2F&&((wrong_tail.payload(7)>>16)&2),"ED second opcode validation");
 H off;off.d.trace_enable=0;off.fetch(0x5AF3,0xCD);for(unsigned i=0;i<7;++i){off.fetch(pc[i],op[i]);tail(off,i);}off.marker(0x02D3);off.idle(200);need(off.written()==1,"disabled ordinary ring unchanged");
 std::puts("SSM_S_TRACE PASS held M1, guards, DDR/S-header stalls, fourteen payloads, ordinary markers, disabled control");
}
int main(int argc,char**argv){Verilated::commandArgs(argc,argv);try{run();return 0;}catch(const std::exception&e){std::fprintf(stderr,"SSM_S_TRACE FAIL %s\n",e.what());return 1;}}
