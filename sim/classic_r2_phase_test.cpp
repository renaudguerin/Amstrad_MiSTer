// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND 4.0), French v1.11 §14.4 p135.
// Public-output test: static R2+1 LEFT16 M2; alternating adjacent lines
// must synchronize strictly between independently measured endpoints.
#include "Vclassic_mode_boundary_top.h"
#include "verilated.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;
namespace {
void need(bool b,const string& s){if(!b)throw runtime_error(s);}
uint8_t tag(unsigned a){return uint8_t((a*73u+(a>>7)*29u)^0xa6u);}
void mix(uint64_t& h,unsigned x){h^=x;h*=1099511628211ull;}

struct Program {
 vector<uint8_t> rom=vector<uint8_t>(16384,0xff);unsigned outLow=0,outHigh=0;
 Program(unsigned low,unsigned high){
  unsigned a=0x100;auto e=[&](unsigned x){rom.at(a++)=x;};
  auto io=[&](unsigned p,unsigned x){e(1);e(p);e(p>>8);e(0x3e);e(x);e(0xed);e(0x79);};
  rom[0]=0xc3;rom[1]=0;rom[2]=1;e(0xf3);e(0x31);e(0xf0);e(0xbf);
  const unsigned r[]={63,40,low,6,38,0,25,30,0,7,0,0,0x30,0};
  for(unsigned n=0;n<14;n++){io(0xbc00,n);io(0xbd00,r[n]);}
  io(0x7f00,0);io(0x7f00,0x4b);io(0x7f00,1);io(0x7f00,0x4c);io(0x7f00,16);io(0x7f00,0x52);io(0x7f00,0x82);
  io(0xbc00,2);io(0xf700,0x82);e(1);e(0);e(0xf5);
  e(0xed);e(0x78);e(0x1f);e(0x38);e(0xfb); // low
  e(0xed);e(0x78);e(0x1f);e(0x30);e(0xfb); // rise
  e(1);e(0);e(0xbd);unsigned loop=a;
  e(0x3e);e(low);outLow=a;e(0xed);e(0x79);
  for(unsigned n=0;n<58;n++)e(0);
  e(0x3e);e(high);outHigh=a;e(0xed);e(0x79);
  for(unsigned n=0;n<55;n++)e(0);
  e(0xc3);e(loop);e(loop>>8);
 }
};

struct Result {
 string name;unsigned low,high;vector<unsigned> edge;
 uint64_t clocks=0,writes=0;unsigned deCount=0,deBad=0;
};

struct Bench {
 Vclassic_mode_boundary_top d;Program program;string name;unsigned low,high;
 vector<uint8_t> ram=vector<uint8_t>(0x800000,0xff);
 uint64_t tick=0,lastWrite=0,lastFhs=0,rawRise=0,modeEvents=0,readyLow=0;
 unsigned pc=0,oldrow=0,frames=0,writes=0,oldc0=0;
 bool ins=false,wr=false,armed=false,oldhs=false,oldfhs=false,found=false,recording=false;
 vector<unsigned> edges;
 bool deOld=false,deBeganInside=false;
 uint64_t deStart=0;unsigned deCount=0,deBad=0;

 Bench(unsigned l,unsigned h):program(l,h),name(to_string(l)+"-"+to_string(h)),low(l),high(h){
  copy(program.rom.begin(),program.rom.end(),ram.begin());
  for(unsigned a=0xc000;a<0x10000;a++)ram[0x20000+a]=tag(a);
  d.crtc_type=1;d.pause=0;d.filter_request=0;d.save_req=0;d.cart_ack=0;d.cart_dout=255;
  d.sna_download=0;d.load_wr=0;d.load_addr=0;d.load_data=0;d.reset_btn=1;
 }

 unsigned step(){
  d.clk=0;d.eval();d.mem_din=ram.at(d.mem_addr);
  unsigned a=0x20000+unsigned(d.video_addr)*2;
  d.video_word=ram.at(a)|(unsigned(ram.at(a+1))<<8);d.eval();
  unsigned c0=d.c0,c4=d.c4,c9=d.c9,ce=d.ce16;
  unsigned input=unsigned(d.ce4)|(unsigned(d.raw_hs)<<1)|(unsigned(d.raw_vs)<<2);
  if(!d.owner_reset&&d.cpu_insn_start&&!ins)pc=d.cpu_addr;ins=d.cpu_insn_start;
  if(!d.owner_reset&&d.crtc_data_write&&!wr){
   bool loop=pc==program.outLow||pc==program.outHigh;
   if(loop){
    need(d.crtc_reg==2&&d.crtc_data==(writes%2?high:low),"actual public R2 sequence/data");
    need(c0<46&&!d.raw_hs,"ordinary R2 write must precede both target HS positions");
    if(writes)need(tick-lastWrite==4096,"CPU loop is not exactly one public 64-character line per OUT");
    lastWrite=tick;writes++;armed=true;
   }
  }wr=d.crtc_data_write;
  if(armed&&oldrow!=0&&c4==0&&c9==0)frames++;oldrow=c4;
  if(oldc0!=c0&&c0==0)found=false;oldc0=c0;
  if(d.mem_wr&&!d.owner_reset&&!d.reset_btn)ram.at(d.mem_addr)=d.mem_dout;
  d.clk=1;d.eval();
  bool hs=d.raw_hs,fhs=d.selected_hs;
  bool deVisible=!d.selected_hb&&!d.selected_vb;
  if(!deOld&&deVisible){deStart=tick;deBeganInside=recording;}
  if(deOld&&!deVisible&&deBeganInside&&recording){
   unsigned width=unsigned(tick-deStart);deCount++;
   if(width!=3072)deBad++;
  }deOld=deVisible;
  if(!oldhs&&hs)rawRise=tick;
  if(oldhs&&!hs&&recording)need(tick-rawRise==384,"actual CRTC R3=6 width changed");
  if(!oldfhs&&fhs)lastFhs=tick;
  if(recording&&ce&&c4>=10&&c4<=21&&c0<=3&&!found&&(d.public_rgb==32||d.public_rgb==42)){
   need(!d.selected_hb&&!d.selected_vb,"bitmap DE-entry edge is outside actual Full view");
   need(d.public_rgb==d.classic_rgb,"R3=6 public/rawGA identity");
   edges.push_back(unsigned(tick-lastFhs));found=true;
  }
  oldhs=hs;oldfhs=fhs;modeEvents+=d.mode_sync;readyLow+=!d.ready;
  d.clk=0;d.eval();tick++;return input;
 }

 Result run(){
  for(unsigned i=0;i<1024;i++)step();d.reset_btn=0;
  unsigned cap=10000000;
  auto reachedEntry=[&](){
   return armed&&frames>=3&&d.c4==10&&d.c9==0&&d.c0==0;
  };
  while(cap>0&&!reachedEntry()){--cap;step();}
  need(reachedEntry(),"public settled entry not reached");
  need(d.r0_value==63&&d.r1_value==40&&d.r3_value==6&&d.applied_mode==0&&d.requested_mode==2&&
       d.effective_decode==4&&!d.filter_nohs&&d.filter_size==256&&modeEvents>10&&readyLow>0,
       "real CPU/mode/Full acquisition setup");
  unsigned preWrites=writes;
  array<uint64_t,2> hash{};
  recording=true;
  for(unsigned f=0;f<2;f++){
   uint64_t h=14695981039346656037ull;
   for(unsigned i=0;i<312u*4096;i++)mix(h,step());
   hash[f]=h;
   need(d.c4==10&&d.c9==0&&d.c0==0,"capture not an exact natural 312-line frame");
  }
  need(writes-preWrites==624,"exactly 624 accepted loop writes in two-frame measurement window");
  need(hash[0]==hash[1],"actual CE4/rawHS/VS full-frame inputs do not repeat");
  need(edges.size()==192,"fixed 12 rows x 8 rasters x 2 frames DE entries missing");
  need(deCount>0&&deBad==0,"all complete visible DE spans must be 3072 master clocks");
  return{name,low,high,edges,tick,writes,deCount,deBad};
 }
};

} // namespace

int main(int argc,char**argv){
 Verilated::commandArgs(argc,argv);cout<<unitbuf;
 try {
  vector<Result> r;
  for(auto pair:{array<unsigned,2>{46,46},{47,47},{48,48},{46,47},{46,48}}){
   Bench b(pair[0],pair[1]);r.push_back(b.run());
   cout<<"CASE "<<r.back().name<<" SETUP_PASS writes="<<r.back().writes
       <<" clocks="<<r.back().clocks<<" edge_min="<<*min_element(r.back().edge.begin(),r.back().edge.end())
       <<" edge_max="<<*max_element(r.back().edge.begin(),r.back().edge.end())
       <<" de_spans="<<r.back().deCount<<" de_bad="<<r.back().deBad<<'\n';
  }
  auto total=[](const vector<unsigned>&v){uint64_t n=0;for(auto x:v)n+=x;return n;};
  need(total(r[0].edge)-total(r[1].edge)==192u*64u,"French static R2+1 LEFT16 control");
  need(total(r[0].edge)-total(r[2].edge)==192u*128u,"French static R2+2 LEFT32 control");

  unsigned meanFailures=0;
  for(unsigned i=3;i<r.size();i++){
   unsigned endpoint=i-2;
   uint64_t sum=total(r[i].edge);
   bool interior=(sum<total(r[0].edge)&&sum>total(r[endpoint].edge));
   if(!interior)meanFailures++;
   cout<<"ALTERNATING "<<r[i].name<<" mean_master="<<double(sum)/192
       <<" strict_static_bounds=("<<double(total(r[endpoint].edge))/192<<','
       <<double(total(r[0].edge))/192<<") "<<(interior?"PASS":"SOURCE_RED")<<'\n';
  }
  cout<<"classic-r2-phase: "<<(meanFailures?"QUALIFIED_RED":"PASS")
      <<" static_controls=PASS de_controls=PASS alternating_bounds="<<meanFailures<<"/2\n";
  return meanFailures?1:0;
 }catch(const exception&e){
  cerr<<"classic-r2-phase: SETUP_OR_CONTROL_FAIL "<<e.what()<<'\n';
  return 2;
 }
}
