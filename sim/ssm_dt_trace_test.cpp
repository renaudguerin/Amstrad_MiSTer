#include "Vssm_dt_trace_top.h"
#include "verilated.h"
#include <array>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
using U=unsigned __int128;
static void need(bool p,const char*s){if(!p)throw std::runtime_error(s);}
struct H {
 Vssm_dt_trace_top d; std::array<uint64_t,130>mem{};unsigned ticks=0;bool held=false;uint64_t hd=0;unsigned ha=0;
 void put(unsigned bit,unsigned n,uint64_t value){for(unsigned i=0;i<n;++i){unsigned k=bit+i;d.snapshot[k/32]=(d.snapshot[k/32]&~(1u<<(k%32)))|(((value>>i)&1u)<<(k%32));}}
 void cycle(){d.clk=0;d.eval();if(held)need(d.we&&d.addr==ha&&d.data==hd,"held DDR request changed");held=d.we&&d.busy;if(held){ha=d.addr;hd=d.data;}if(d.we&&!d.busy){need(d.addr>=0x6000000&&d.addr<0x6000082,"DDR range");mem[d.addr-0x6000000]=d.data;}d.clk=1;d.eval();ticks++;}
 void idle(unsigned n=20){while(n--)cycle();}
 H(){d.reset=1;d.trace_enable=1;d.qualified=1;d.m1_fetch=0;d.busy=0;for(unsigned i=0;i<4;++i)d.snapshot[i]=0;idle(4);d.reset=0;idle();}
 unsigned fetch(unsigned a,unsigned b,unsigned hold=5){d.fetch_addr=a;d.bus_data=b;d.m1_fetch=1;idle(hold);d.m1_fetch=0;unsigned t=ticks-4;cycle();idle(3);return t;}
 void marker(unsigned code,bool source=true){unsigned a=source?0xa385:0x9000;fetch(a,0xed);fetch(a+1,code&255);fetch(a+2,0xed);fetch(a+3,code>>8);if(source)fetch(a+4,0xc9);idle();}
 unsigned cut(unsigned a,unsigned row,unsigned r7,unsigned hold=5){put(8,7,row);put(63,8,r7);unsigned t=fetch(a,0xed,hold);fetch(a+1,0x49);return t;}
 void edge(unsigned row,unsigned active_ticks,bool visible=true){put(8,7,row);put(63,8,row);put(95,1,1);put(97,1,1);put(98,1,0);put(99,1,!visible);put(100,1,1);put(101,1,visible);idle(active_ticks);put(95,1,0);put(97,1,0);put(101,1,0);idle(2);}
 unsigned written(){return unsigned(mem[1]);}unsigned code(unsigned i){return mem[2+2*i]&65535;}uint64_t payload(unsigned i){return (mem[2+2*i]>>16)|((mem[3+2*i]>>48)<<48);}U sample(unsigned slot){return U(payload(2+2*slot))|(U(payload(3+2*slot))<<64);}uint32_t stamp(unsigned i){return mem[3+2*i]>>16;}
 void drain(){d.busy=0;idle(250);}
 unsigned sequence(unsigned i){return mem[3+2*i]&65535;}
 void save(const char*path){std::ofstream out(path,std::ios::binary);out.write((const char*)mem.data(),sizeof(mem));}
};
static uint64_t bits(U x,unsigned lo,unsigned n){return uint64_t(x>>lo)&((uint64_t(1)<<n)-1);}
static void status(H&h,unsigned errors=0){need(h.written()==17,"expected F8,F7 plus fourteen DT payloads and status");need(h.code(0)==0x2f8&&h.code(1)==0x2f7,"ordinary F8/F7 preserved");for(unsigned i=0;i<7;++i)need(h.code(2+2*i)==0xff20+i&&h.code(3+2*i)==0xff28+i,"ordered fixed DT slot tags");need(h.code(16)==0xff2f,"committed DT status");need((h.payload(16)>>32)==0x44545231&&((h.payload(16)>>8)&255)==1&&(h.payload(16)&255)==7,"DT magic/version/count");need(((h.payload(16)>>16)&255)==errors,"DT error exact");need(h.d.event_count==2&&h.d.dropped_count==0,"private records are not ordinary marker events");}
static void late_partial(){
 H h;h.marker(0x2f8);h.d.busy=1;h.marker(0x2f7);
 h.d.busy=0;bool at_payload=false;
 for(unsigned n=0;n<150;++n){h.d.clk=0;h.d.eval();if(h.d.we&&h.d.addr==0x6000007){at_payload=true;break;}h.cycle();}
 need(at_payload&&h.written()==2,"stall must be mid-first DT payload after genuineF7 header");h.d.busy=1;h.idle(10);need(h.d.frozen,"F7 must freeze partial even before bracket");
 std::array<uint32_t,28> before{};std::array<uint32_t,7> times{};
 for(unsigned i=0;i<28;++i)before[i]=h.d.samples[i];for(unsigned i=0;i<7;++i)times[i]=h.d.sample_ticks[i];
 auto immutable=[&](){for(unsigned i=0;i<28;++i)need(before[i]==h.d.samples[i],"frozen partial on-chip snapshot changed after late69F1");for(unsigned i=0;i<7;++i)need(times[i]==h.d.sample_ticks[i],"frozen partial on-chip timestamp changed after late69F1");};
 h.cut(0x69f1,55,14,33);immutable();h.drain();status(h,8);
 for(unsigned i=0;i<7;++i){U expected=0;for(unsigned k=0;k<4;++k)expected|=U(before[4*i+k])<<(32*k);need(h.sample(i)==expected,"committed partial packet changed afterfreeze");need(h.stamp(2+2*i)==times[i]&&h.stamp(3+2*i)==times[i],"committed partial timestamps changed afterfreeze");}
 auto committed=h.mem;h.cut(0x69f1,66,30);h.idle();immutable();need(h.mem==committed,"postflush latefetch changed committed partialpacket");
 std::puts("DT_LATE_PARTIAL PASS immutable prebracketF7 freeze, stalledpayload, lateM1, committedpacket, postflush");
}
static void run(){
 H h;h.marker(0x2f8);need(h.written()==1,"genuine F8 ordinary marker");h.d.busy=1;unsigned start=h.cut(0x69f1,0,30,33);h.edge(10,5);h.cut(0x69fc,10,10);h.edge(14,7);h.cut(0x6a07,20,14);h.edge(30,9,false);unsigned end=h.cut(0x69f1,0,30);h.edge(42,11);need(h.written()==1,"no private DDR before F7");h.marker(0x2f7);need(h.written()==1,"DDR stalled F7 cannot publish");
 h.d.busy=0;for(unsigned n=0;n<60;++n){h.d.clk=0;h.d.eval();if(h.d.we&&h.d.addr==0x6000001&&h.d.data==2)break;h.cycle();}h.d.busy=1;h.idle(40);need(h.written()==1,"unaccepted F7 header cannot release DT");h.drain();status(h);U z=h.sample(6);need(bits(z,0,6)==63&&bits(z,6,4)==3&&bits(z,11,4)==3,"complete mask and independent edge counts");need(bits(z,16,18)==5&&bits(z,34,18)==7&&bits(z,52,18)==0,"actual first-edge cycle ownership and DE-qualified black ticks");need(bits(z,70,3)==7&&bits(z,73,3)==3,"per-owner GAblank/DE seen");need(bits(z,79,24)==end-start,"fixed M1 bracket elapsed ticks");need(bits(z,125,1)==1&&bits(z,126,1)==0,"completed bracket independent of later edges");need(h.stamp(2)==start&&h.stamp(3)==start,"first ED completion timestamp");need(bits(h.sample(1),63,8)==10&&bits(h.sample(2),63,8)==14,"M1 snapshots observe pre-OUT R7, not accepted bus phases");
 std::ofstream out("transport-fixture.bin",std::ios::binary);out.write((const char*)h.mem.data(),sizeof(h.mem));out.close();
 H same;same.marker(0x2f8);same.d.fetch_addr=0x69f1;same.d.bus_data=0xed;same.d.m1_fetch=1;same.idle(5);same.d.m1_fetch=0;same.put(95,1,1);same.put(97,1,1);same.put(100,1,1);same.cycle();same.put(95,1,0);same.put(97,1,0);same.idle(3);same.fetch(0x69f2,0x49);same.cut(0x69fc,10,10);same.cut(0x6a07,20,14);same.cut(0x69f1,0,30);same.marker(0x2f7);same.drain();status(same);need(bits(same.sample(6),16,18)==1&&bits(same.sample(6),6,4)==1,"first bracket cycle raw edge belongs new owner0");
 H missing;missing.marker(0x2f8);missing.cut(0x69f1,0,30);missing.edge(10,2);missing.cut(0x69fc,10,10);missing.cut(0x6a07,20,14);missing.edge(30,3,false);missing.cut(0x69f1,0,30);missing.marker(0x2f7);missing.drain();status(missing);z=missing.sample(6);need(bits(z,0,6)==31&&bits(z,6,4)==2,"missing middle edge is honest ordinal count");need(bits(missing.sample(4),8,7)==30&&bits(missing.sample(5),8,7)==0,"row30 occupies second actual edge slot, not third");
 H noedge;noedge.marker(0x2f8);noedge.cut(0x69f1,0,30);noedge.cut(0x69fc,10,10);noedge.cut(0x6a07,20,14);noedge.cut(0x69f1,0,30);noedge.marker(0x2f7);noedge.drain();status(noedge);need(bits(noedge.sample(6),0,6)==7&&bits(noedge.sample(6),6,4)==0,"fixed bracket freezes with no raw edges");
 H partial;partial.marker(0x2f8);partial.cut(0x69f1,0,30);partial.d.qualified=0;partial.cut(0x69fc,10,10);partial.d.qualified=1;partial.marker(0x2f7);partial.drain();status(partial,9);need(bits(partial.sample(6),126,1)==1,"partial F7 freezes without waiting for edges");
 H wrong;wrong.marker(0x2f8);wrong.cut(0x69f1,0,30);wrong.fetch(0x69fc,0xed);wrong.fetch(0x69fd,0x71);wrong.marker(0x2f7);wrong.drain();status(wrong,10);need(!(bits(wrong.sample(6),0,6)&2),"bad ED tail does not validate prefix slot");
 H order;order.marker(0x2f8);order.cut(0x69f1,0,30);order.cut(0x6a07,20,14);order.marker(0x2f7);order.drain();status(order,12);
 H sat;sat.marker(0x2f8);sat.cut(0x69f1,0,30);sat.cut(0x69fc,10,10);sat.cut(0x6a07,20,14);sat.edge(10,262145);for(unsigned i=0;i<17;++i)sat.edge(20+i,1);sat.cut(0x69f1,0,30);sat.marker(0x2f7);sat.drain();status(sat,48);z=sat.sample(6);need(bits(z,6,4)==15&&bits(z,10,1)&&bits(z,11,4)==15&&bits(z,15,1),"saturating independent edge counts with overflow");need(bits(z,16,18)==262143&&bits(z,76,1),"active-black tick saturation");
 H ordinary;ordinary.marker(0x2f8);ordinary.cut(0x69f1,0,30);ordinary.cut(0x69fc,10,10);ordinary.cut(0x6a07,20,14);ordinary.cut(0x69f1,0,30);ordinary.marker(0x2f7);ordinary.marker(0x1234,false);ordinary.drain();need(ordinary.written()==18&&ordinary.d.event_count==3&&ordinary.d.dropped_count==0,"ordinary marker during private publication preserved");unsigned found=0;for(unsigned i=0;i<18;++i)found+=ordinary.code(i)==0x1234;need(found==1,"ordinary marker exactly once");
 unsigned ordinary_index=0,status_index=0;for(unsigned i=0;i<18;++i){if(ordinary.code(i)==0x1234)ordinary_index=i;if(ordinary.code(i)==0xff2f)status_index=i;}
 need(ordinary_index>2&&ordinary_index<status_index&&ordinary.sequence(ordinary_index)==2,"interleaved ordinary record stores event ordinal, not physical written index");
 ordinary.save("transport-interleaved.bin");
 ordinary.marker(0x02fa,false);ordinary.drain();need(ordinary.written()==19&&ordinary.d.event_count==4&&ordinary.code(18)==0x02fa&&ordinary.sequence(18)==3,"postflush FA record retains ordinary event ordinal");
 ordinary.marker(0x02f9,false);ordinary.drain();need(ordinary.written()==20&&ordinary.d.event_count==5&&ordinary.code(19)==0x02f9&&ordinary.sequence(19)==4,"postflush F9 header and ordinary event ordinal progress");
 ordinary.save("transport-postflush.bin");
 H closing;closing.marker(0x2f8);unsigned closing_start=closing.cut(0x69f1,0,30);closing.cut(0x69fc,10,10);closing.cut(0x6a07,20,14);unsigned closing_end=closing.fetch(0x69f1,0xed);closing.fetch(0x69f2,0x71);need(closing.d.frozen,"closing prefix freezes before wrong tail");
 std::array<uint32_t,24> frozen_prefix{};std::array<uint32_t,6> frozen_ticks{};for(unsigned i=0;i<24;++i)frozen_prefix[i]=closing.d.samples[i];for(unsigned i=0;i<6;++i)frozen_ticks[i]=closing.d.sample_ticks[i];
 closing.marker(0x2f7);closing.drain();status(closing,10);U closing_summary=closing.sample(6);need(bits(closing_summary,0,6)==7&&!bits(closing_summary,125,1),"wrong closing tail keeps valid M1 cuts but incomplete bracket");
 need(bits(closing_summary,79,24)==closing_end-closing_start&&closing.stamp(14)>closing_end,"partial elapsed stops at closing prefix while summary tick is F7");
 for(unsigned i=0;i<24;++i)need(frozen_prefix[i]==closing.d.samples[i],"closing-tail failure mutated frozen prefix samples");for(unsigned i=0;i<6;++i)need(frozen_ticks[i]==closing.d.sample_ticks[i],"closing-tail failure mutated frozen prefix timestamps");
 closing.save("transport-invalid-closing-tail.bin");
 std::puts("DT_DECODER_FIXTURES PASS actual interleaved/postflush event sequences and frozen invalid closing-tail partial");
 H off;off.d.trace_enable=0;off.marker(0x2f8);off.cut(0x69f1,0,30);off.cut(0x69f1,0,30);off.marker(0x2f7);off.drain();need(off.written()==2&&off.d.event_count==2,"disabled diagnostic ordinary ring unchanged");
 std::puts("DT_TRANSPORT PASS qualified M1/tails, held fetch, fixed bracket, ordinal missing/zero edges, first-cycle owner, partial/errors, saturation, DDR/F7header stalls, ordinary markers, disabled");
}
int main(int argc,char**argv){Verilated::commandArgs(argc,argv);try{if(argc==2&&std::string(argv[1])=="--late-partial-only")late_partial();else{run();late_partial();}return 0;}catch(const std::exception&e){std::fprintf(stderr,"DT_TRANSPORT FAIL %s\n",e.what());return 1;}}
