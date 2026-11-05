// Technical information sourced from the "Amstrad CPC CRTC Compendium"
// by Longshot (CC BY-NC-ND 4.0), French v1.11 §14.4 pp134-135.
// Ordinary R3l4->5 moves the rendered line LEFT8 M2 pixels. Full5/6 remain
// old-source regression anchors, not a new analogue monitor/settling oracle.
#include "Vclassic_mode_boundary_top.h"
#include "verilated.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
using namespace std;
namespace {
void need(bool ok,const string& why){if(!ok)throw runtime_error(why);}
uint8_t tag(unsigned a){return uint8_t((a*73u+(a>>7)*29u)^0xa6u);}
struct Program {
 vector<uint8_t> rom=vector<uint8_t>(16384,0xff);vector<unsigned> waits;unsigned idle=0;
 Program(const vector<unsigned>& widths,bool rapid,bool border){
  unsigned a=0x100;auto e=[&](unsigned v){rom.at(a++)=v;};
  auto io=[&](unsigned port,unsigned v){e(1);e(port);e(port>>8);e(0x3e);e(v);e(0xed);e(0x79);};
  rom[0]=0xc3;rom[1]=0;rom[2]=1;e(0xf3);e(0x31);e(0xf0);e(0xbf);
  const unsigned regs[]={63,40,50,widths[0],38,0,25,30,0,7,0,0,0x30,0};
  for(unsigned r=0;r<14;r++){io(0xbc00,r);io(0xbd00,regs[r]);}
  io(0x7f00,0);io(0x7f00,0x4b);io(0x7f00,1);io(0x7f00,0x4c);
  io(0x7f00,16);io(0x7f00,0x52);io(0x7f00,0x82);
  if(border){
   io(0x7f00,16);idle=a;e(0x3e);e(0x4b);e(0xed);e(0x79);
   e(0x3e);e(0x4c);e(0xed);e(0x79);e(0x18);e(uint8_t(int(idle)-int(a+1)));return;
  }
  if(rapid){
   io(0xf700,0x82);
   for(unsigned s=0;s<widths.size();s++){
    if(s){io(0xbc00,3);io(0xbd00,widths[s]);}
    waits.push_back(a);e(1);e(0);e(0xf5);e(0x16);e(s?1:4);
    unsigned low=a;e(0xed);e(0x78);e(0x1f);e(0x38);e(0xfb);
    e(0xed);e(0x78);e(0x1f);e(0x30);e(0xfb);e(0x15);e(0x20);e(uint8_t(int(low)-int(a+1)));
   }
  }
  idle=a;e(0x18);e(0xfe);
 }
};
struct Pixel {
 unsigned c0,c4,c9,seq,ce,ce4,hs,vs,de,blank,fhs,fvs,hb,vb,shs,shb,svb,shift;
 unsigned fetch,bs,addr,word,before,after,buf,ga,rgb,applied,mode,decode,ready;
};
uint64_t capture_hash(const vector<Pixel>& a,bool publicRGB){
 uint64_t h=14695981039346656037ull;
 for(const auto& p:a){
  const unsigned v[]={p.c0,p.c4,p.c9,p.seq,p.ce,p.ce4,p.hs,p.vs,p.de,p.blank,
   p.fhs,p.fvs,p.hb,p.vb,p.shs,p.shb,p.svb,p.shift,p.fetch,p.bs,p.addr,p.word,
   p.before,p.after,p.buf,publicRGB?p.rgb:p.ga,p.applied,p.mode,p.decode,p.ready};
  for(unsigned x:v)for(unsigned j=0;j<4;j++){h^=(x>>(8*j))&255;h*=1099511628211ull;}
 }
 return h;
}
// Canonical uint32-LE FNV-1a over the30 fields above, whole8192 clocks.
// Extracted from unchanged source captures; never generated from this candidate.
static constexpr uint64_t old_capture[2][3]={
 {0x2bec75f3250ff595ull,0x34e68c66cfb473adull,0xaa07a67c324faa55ull},
 {0x20dceefe221a89d5ull,0x73fc2596c0a128bdull,0xe8ad563d13c8d815ull},
};
struct Bench {
 Vclassic_mode_boundary_top d;Program program;vector<unsigned> widths;bool rapid,border;
 vector<uint8_t> ram=vector<uint8_t>(0x800000,0xff);array<unsigned,8> rawPast{};
 uint64_t ticks=0,modeEvents=0,readyLow=0;unsigned pc=0,frames=0,oldrow=0,writes=0;int stage=-1;
 bool armed=false,ins=false,wr=false,pal=false,cart=false,clean=false,lastEnd=false,lastHS=false,postRise=false,postFall=false;
 unsigned classifications=0,visible=0,ink11=0,ink12=0;bool watch=false,pendingDirect=false;
 enum Probe {None,Delayed,Direct} probe=None;
 Bench(unsigned type,vector<unsigned> w,bool r=false,bool b=false):program(w,r,b),widths(w),rapid(r),border(b){
  copy(program.rom.begin(),program.rom.end(),ram.begin());
  if(!border)for(unsigned a=0xc000;a<0x10000;a++)ram[0x20000+a]=tag(a);
  d.crtc_type=type;d.pause=0;d.filter_request=0;d.save_req=0;d.cart_dout=255;d.cart_ack=0;
  d.sna_download=0;d.load_wr=0;d.load_addr=0;d.load_data=0;d.reset_btn=1;
 }
 Pixel tick(){
  d.clk=0;d.eval();d.mem_din=ram.at(d.mem_addr);
  unsigned va=0x20000+unsigned(d.video_addr)*2;unsigned word=ram.at(va)|(unsigned(ram.at(va+1))<<8);d.video_word=word;
  if(!d.cart_req)cart=false;d.cart_ack=d.cart_req&&!cart;
  if(d.cart_ack){d.cart_dout=ram.at(d.cart_addr);cart=true;}d.eval();
  if(!d.owner_reset&&d.cpu_insn_start&&!ins){pc=d.cpu_addr;if(pc==program.idle||(stage>=0&&rapid&&pc==program.waits.at(stage)))armed=true;}ins=d.cpu_insn_start;
  if(!d.owner_reset&&d.crtc_data_write&&!wr&&d.crtc_reg==3){
   stage++;need(unsigned(stage)<widths.size()&&d.crtc_data==widths[stage],"actual CPU R3 order/data");
   if(rapid&&stage>0)need(!d.raw_hs&&d.c0<50,"ordinary R3 write must precede HS");
   writes++;frames=0;armed=false;
  }wr=d.crtc_data_write;
  if(armed&&oldrow!=0&&d.c4==0&&d.c9==0)frames++;oldrow=d.c4;
  if(watch&&(d.border_accept||d.ink_accept)&&!pal&&d.c0>=8&&d.c0<=31){
   need(d.border_accept&&!d.ink_accept&&d.palette_target==16&&(d.palette_data==11||d.palette_data==12)&&
        !d.raw_de&&!d.outgoing_de&&!d.hblank&&!d.vblank&&!d.force_blank,"visible accepted BORDER input");
   visible++;ink11+=d.palette_data==11;ink12+=d.palette_data==12;
  }pal=d.border_accept||d.ink_accept;
  Pixel p{d.c0,d.c4,d.c9,d.seq,d.ce16,d.ce4,d.raw_hs,d.raw_vs,d.raw_de,d.force_blank,
   d.filtered_hs,d.filtered_vs,d.hblank,d.vblank,d.selected_hs,d.selected_hb,d.selected_vb,d.shift,
   d.fetch,d.bs,d.video_addr,word,d.byte_out,0,0,0,0,0,0,0,0};
  bool clear=d.owner_reset||d.sna_load||d.filter_commit||d.applied_mode!=0;
  bool reset=d.owner_reset||d.sna_load;
  // Filter END is capture eligibility only. No RGB-sidecar equations/state
  // are copied: qualify a complete actual source pulse after the context.
  if(clear){rawPast.fill(0);clean=false;postRise=postFall=false;probe=pendingDirect?Direct:None;}
  else {
   if(d.ce16){for(unsigned j=7;j>0;j--)rawPast[j]=rawPast[j-1];rawPast[0]=d.classic_rgb;}
   if(!clean){clean=true;postRise=postFall=false;}
   else {
    if(!lastHS&&d.raw_hs){postRise=true;postFall=false;}
    if(lastHS&&!d.raw_hs&&postRise)postFall=true;
    if(d.end_gen!=lastEnd&&postFall){classifications++;postFall=false;}
   }
  }
  lastEnd=d.end_gen;lastHS=d.raw_hs;
  if(d.mem_wr&&!d.owner_reset&&!d.reset_btn)ram.at(d.mem_addr)=d.mem_dout;
  if(d.load_mem_wr)ram.at(d.load_mem_addr)=d.load_data;
  d.clk=1;d.eval();
  p.after=d.byte_out;p.buf=d.video_buf;p.ga=d.classic_rgb;p.rgb=d.public_rgb;
  p.applied=d.applied_mode;p.mode=d.requested_mode;p.decode=d.effective_decode;p.ready=d.ready;
  if(reset)need(p.rgb==0,"public reset/SNA must discard prior nonzero RGB");
  else if(p.applied!=0||probe==Direct)need(p.rgb==p.ga,"raw or restored6 public bypass");
  else if(probe==Delayed)need(p.rgb==rawPast[7],"public4 must follow prior raw pixels by8 M2");
  modeEvents+=d.mode_sync;readyLow+=!d.ready;d.clk=0;d.eval();ticks++;return p;
 }
 void run(unsigned n){while(n--)tick();}
 template<class F>void until(F f,unsigned cap,const string&why){while(!f()&&cap--)tick();need(f(),why);}
 void boot(){run(1024);d.reset_btn=0;}
 void source_class(){unsigned before=classifications;until([&]{return classifications>before;},3000000,"fresh complete source classification absent");}
 void setup(){need(d.r0_value==63&&d.r1_value==40&&d.r2_value==50&&d.r3_value==widths.at(stage)&&
  d.applied_mode==0&&d.requested_mode==2&&d.effective_decode==4&&!d.filter_nohs&&d.filter_size==256&&modeEvents>10,"real CPU/mode/Full acquisition guard");}
 vector<Pixel> capture(unsigned expectedStage=0){
  unsigned target=border?26:6;
  until([&]{return stage==int(expectedStage)&&armed&&frames>=(rapid&&stage>0?1u:3u)&&d.c4==target&&d.c9==0&&d.c0==0;},24000000,"fixed public window absent");setup();
  int capturedStage=stage;vector<Pixel> a;a.reserve(8192);watch=border;
  for(unsigned n=0;n<8192;n++){a.push_back(tick());need(stage==capturedStage,"next CPU width arrived before complete window");}watch=false;
  if(border)need(visible==12&&ink11&&ink12,"all fixed-window accepted border latches");return a;
 }
 // END can precede the first fully programmed field after a CPU/CRTC reset.
 // Qualify a natural frame after idle before requiring visible source pixels.
 void active(){until([&]{return armed&&frames>=1&&d.c4==6&&d.c9==0&&d.c0==20;},3000000,"fixed active source point absent");setup();}
 void nonzero(){need(d.c4==6&&d.c9==0&&d.c0==20,"predetermined nonzero opportunity");for(unsigned x:rawPast)need(x==32||x==42,"preceding programmed nonzero native pixels");}
 void mode(unsigned n){d.filter_request=n;until([&]{return d.applied_mode==n;},2000000,"public mode commit absent");run(8192);}
 vector<uint8_t> save(){
  d.save_req=1;tick();d.save_req=0;until([&]{return d.captured;},10000,"actual save capture absent");need(d.save_hold,"save must own coherent RAM/header");
  vector<uint8_t>s(0x20100);for(unsigned a=0;a<256;a++)for(unsigned j=0;j<8;j++)s[a]|=((d.saved_header[(a*8+j)/32]>>((a*8+j)%32))&1)<<j;
  copy(ram.begin()+0x20000,ram.begin()+0x40000,s.begin()+256);
  need((s[0x23]|unsigned(s[0x24])<<8)==program.idle&&s[0x46]==widths[0]&&s[0x6b]==128,"unedited natural snapshot PC/width/RAM");return s;
 }
 void restore(const vector<uint8_t>&s,unsigned target){
  until([&]{return d.raw_hs&&d.c0==51;},100000,"old active-HS context absent");need(d.r3_value==4,"old snapshot source must be4");
  unsigned before=classifications;pendingDirect=target==6;d.sna_download=1;run(4);
  for(unsigned a=0;a<s.size();a++){d.load_wr=1;d.load_addr=a;d.load_data=s[a];tick();d.load_wr=0;tick();}
  d.sna_download=0;until([&]{return d.sna_load;},1000,"public SNA apply absent");
  need(!d.raw_hs&&!d.owner_reset,"supported loader applies HS-low, not forged active HS");tick();need(d.r3_value==target,"captured targetR3 not applied");
  until([&]{return classifications>before;},3000000,"post-SNA complete source pulse absent");
  probe=target==4?Delayed:Direct;run(8192);
 }
};
unsigned compare_left8(const vector<Pixel>& a,const vector<Pixel>& b,unsigned lo,unsigned hi){
 unsigned count=0;for(unsigned i=0;i+32<a.size();i++)if(a[i].ce&&a[i].c0>=lo&&a[i].c0<=hi&&a[i+32].c0>=lo&&a[i+32].c0<=hi){
  need(b[i].rgb==a[i+32].rgb,"French14.4 public LEFT8 mismatch");count++;
 }return count;
}
void edges(const vector<Pixel>& a,const vector<Pixel>& b){
 for(unsigned raster:{0u,1u})for(bool entry:{true,false}){
  unsigned pos[2]={};
  for(unsigned w=0;w<2;w++){
   const auto& x=w?b:a;bool border=false,ink=false,found=false;
   for(unsigned i=0;i<x.size();i++)if(x[i].ce&&x[i].c9==raster&&x[i].c0>=(entry?0:38)&&x[i].c0<=(entry?2:42)){
    need(x[i].rgb==8||x[i].rgb==32||x[i].rgb==42,"unique border/pen edge content");border|=x[i].rgb==8;ink|=x[i].rgb!=8;
    if(!found&&(entry?x[i].rgb!=8:x[i].rgb==8)){pos[w]=i;found=true;}
   }need(border&&ink&&found,"complete DE border/ink edge");
  }need(pos[0]-pos[1]==32,"DE edge must move8 native dots");
 }
}

struct RelockProgram {
 vector<uint8_t> rom = vector<uint8_t>(16384, 0xff);
 unsigned r2PC = 0, r3PC = 0, loopPC = 0;
 RelockProgram(unsigned warm, unsigned target, unsigned r2) {
  unsigned a = 0x100;
  auto e = [&](unsigned x) { rom.at(a++) = uint8_t(x); };
  auto io = [&](unsigned p, unsigned v) {
   e(1); e(p); e(p >> 8); e(0x3e); e(v); unsigned pc = a; e(0xed); e(0x79); return pc;
  };
  rom[0] = 0xc3; rom[1] = 0; rom[2] = 1;
  e(0xf3); e(0x31); e(0xf0); e(0xbf);
  const unsigned regs[] = {63, 40, 48, warm, 38, 0, 25, 30, 0, 7, 0, 0, 0x30, 0};
  for (unsigned n = 0; n < 14; n++) { io(0xbc00, n); io(0xbd00, regs[n]); }
  io(0x7f00, 0); io(0x7f00, 0x4b); io(0x7f00, 1); io(0x7f00, 0x4c);
  io(0x7f00, 16); io(0x7f00, 0x52); io(0x7f00, 0x82); io(0xf700, 0x82);
  e(1); e(0); e(0xf5); e(0x16); e(4); unsigned low = a;
  e(0xed); e(0x78); e(0x1f); e(0x38); e(0xfb);
  e(0xed); e(0x78); e(0x1f); e(0x30); e(0xfb);
  e(0x15); e(0x20); e(uint8_t(int(low) - int(a + 1)));
  e(0xed); e(0x78); e(0x1f); e(0x38); e(0xfb); // following VSfall
  e(0x11); e(128); e(0); unsigned coarse = a;
  e(0x1b); e(0x7a); e(0xb3); e(0x20); e(uint8_t(int(coarse) - int(a + 1)));
  io(0xbc00, 2); r2PC = io(0xbd00, r2);
  io(0xbc00, 3); r3PC = io(0xbd00, target);
  io(0x7f00, 16); loopPC = a;
  e(0x3e); e(0x4b); e(0xed); e(0x79);
  e(0x3e); e(0x4c); e(0xed); e(0x79);
  e(0x18); e(uint8_t(int(loopPC) - int(a + 1)));
 }
};

struct RelockTimelineSample {
 uint8_t ga;
 bool hs, vs, de, ode, hb, vb, blank;
 uint8_t c0, c4, c9;
 bool operator==(const RelockTimelineSample& o) const {
  return ga == o.ga && hs == o.hs && vs == o.vs && de == o.de && ode == o.ode &&
         hb == o.hb && vb == o.vb && blank == o.blank &&
         c0 == o.c0 && c4 == o.c4 && c9 == o.c9;
 }
};

struct RelockAcceptedEvent {
 uint32_t relTick;
 uint8_t c0, c4, c9, target, data;
 bool operator==(const RelockAcceptedEvent& o) const {
  return relTick == o.relTick && c0 == o.c0 && c4 == o.c4 && c9 == o.c9 &&
         target == o.target && data == o.data;
 }
};

struct RelockResult {
 string name;
 unsigned warm = 0, target = 0, r2 = 0;
 unsigned visible = 0, accept11 = 0, accept12 = 0;
 unsigned rawEdges = 0, pubEdges = 0;
 unsigned mismatch = 0, historyBad = 0;
 unsigned complete = 0;
 uint64_t window = 0, transition = 0, firstComplete = 0;
 vector<RelockTimelineSample> timeline;
 vector<RelockAcceptedEvent> acceptedLedger;
};

struct RelockBench {
 Vclassic_mode_boundary_top d;
 RelockProgram prog;
 string name;
 unsigned warm, target, r2;
 vector<uint8_t> ram = vector<uint8_t>(0x800000, 0xff);
 array<unsigned, 8> history{};
 uint64_t tick = 0, r2Tick = 0, transition = 0, firstComplete = 0, rawStart = 0, window = 0;
 unsigned pc = 0, visible = 0, accept11 = 0, accept12 = 0;
 unsigned rawEdges = 0, pubEdges = 0, mismatch = 0, historyBad = 0;
 unsigned complete = 0, vsAfter = 0, warmVS = 0;
 uint64_t modeEvents = 0, readyLow = 0;
 bool ins = false, wr = false, oldHS = false, oldVS = false, oldPal = false;
 bool eligible = false, seenLoop = false, capturing = false, prevScore = false;
 unsigned prevGA = 0, prevPub = 0;
 vector<RelockTimelineSample> timeline;
 vector<RelockAcceptedEvent> acceptedLedger;

 RelockBench(string n, unsigned w, unsigned t, unsigned r)
  : prog(w, t, r), name(n), warm(w), target(t), r2(r) {
  copy(prog.rom.begin(), prog.rom.end(), ram.begin());
  d.crtc_type = 1; d.pause = 0; d.filter_request = 0; d.save_req = 0;
  d.cart_ack = 0; d.cart_dout = 255; d.sna_download = 0; d.load_wr = 0;
  d.load_addr = 0; d.load_data = 0; d.reset_btn = 1;
 }

 void step() {
  d.clk = 0; d.eval();
  d.mem_din = ram.at(d.mem_addr);
  unsigned va = 0x20000 + unsigned(d.video_addr) * 2;
  d.video_word = ram.at(va) | (unsigned(ram.at(va + 1)) << 8);
  d.eval();

  unsigned c0 = d.c0, c4 = d.c4, c9 = d.c9;
  bool ce = d.ce16;

  if (!d.owner_reset && d.cpu_insn_start && !ins) {
   pc = d.cpu_addr;
   if (pc == prog.loopPC) seenLoop = true;
  }
  ins = d.cpu_insn_start;

  if (!d.owner_reset && d.crtc_data_write && !wr) {
   if (pc == prog.r2PC) {
    r2Tick = tick;
    need(d.crtc_reg == 2 && d.crtc_data == r2, "actual transition R2 order/data");
    need(!d.raw_vs && !d.raw_hs && c0 < 48 && d.filter_size == 256 && !d.filter_nohs, "ordinary R2 write after period training");
    need(d.r3_value == warm && warmVS >= 4, "warm CRTC R3 before R2 step");
   }
   if (pc == prog.r3PC) {
    transition = tick;
    need(r2Tick && d.crtc_reg == 3 && d.crtc_data == target && d.r2_value == r2, "actual transition R3 order/data");
    need(!d.raw_vs && d.filter_size == 256 && !d.filter_nohs, "R3 transition outside VS/training");
   }
  }
  wr = d.crtc_data_write;

  if (transition) {
   need(!d.owner_reset && !d.sna_download && !d.sna_load && !d.filter_commit, "no reset/SNA/filter/context clear across relock");
  }

  bool accept = d.border_accept || d.ink_accept;
  if (capturing && accept && !oldPal) {
   bool witness = (c0 >= 8 && c0 <= 31);
   if (witness) {
    need(d.border_accept && !d.ink_accept && d.palette_target == 16 &&
         (d.palette_data == 11 || d.palette_data == 12) &&
         !d.raw_de && !d.outgoing_de && !d.selected_hb && !d.selected_vb && !d.force_blank,
         "actual visible accepted BORDER input");
    if (d.palette_data == 11) accept11++;
    if (d.palette_data == 12) accept12++;
    acceptedLedger.push_back({uint32_t(tick - window), uint8_t(c0), uint8_t(c4), uint8_t(c9), uint8_t(d.palette_target), uint8_t(d.palette_data)});
   }
  }
  oldPal = accept;

  if (ce) {
   for (unsigned i = 7; i > 0; i--) history[i] = history[i - 1];
   history[0] = d.classic_rgb;
  }

  if (d.mem_wr && !d.owner_reset && !d.reset_btn) ram.at(d.mem_addr) = d.mem_dout;

  d.clk = 1; d.eval();

  if (!oldVS && d.raw_vs) {
   warmVS++;
   if (transition) vsAfter++;
  }
  oldVS = d.raw_vs;

  if (!oldHS && d.raw_hs) {
   rawStart = tick;
   eligible = transition && (tick > transition) && (d.r3_value == target);
  }
  if (oldHS && !d.raw_hs) {
   if (eligible) {
    need(tick - rawStart == target * 64, "first/subsequent complete post-write target raw pulse width");
    complete++;
    if (!firstComplete) firstComplete = tick;
   }
  }
  oldHS = d.raw_hs;

  if (capturing && ce) {
   timeline.push_back({
    uint8_t(d.classic_rgb),
    bool(d.raw_hs), bool(d.raw_vs),
    bool(d.raw_de), bool(d.outgoing_de),
    bool(d.selected_hb), bool(d.selected_vb),
    bool(d.force_blank),
    uint8_t(d.c0), uint8_t(d.c4), uint8_t(d.c9)
   });
  }

  bool score = capturing && ce && (c0 >= 8 && c0 <= 31);
  if (score) {
   need(firstComplete && tick > firstComplete && !vsAfter && !d.raw_vs &&
        d.r3_value == target && d.r2_value == r2 && d.applied_mode == 0 &&
        d.filter_size == 256 && !d.filter_nohs &&
        !d.raw_de && !d.outgoing_de && !d.selected_hb && !d.selected_vb && !d.force_blank,
        "coherent width/complete-pulse/BORDER/Full-visible tuple");
   need((d.classic_rgb == 32 || d.classic_rgb == 42) &&
        (d.public_rgb == 32 || d.public_rgb == 42),
        "programmed BORDER colors in raw/public outputs");
   visible++;
   mismatch += (d.public_rgb != d.classic_rgb);
   historyBad += (d.public_rgb != history[7]);
   if (prevScore) {
    rawEdges += (prevGA != d.classic_rgb);
    pubEdges += (prevPub != d.public_rgb);
   }
   prevGA = d.classic_rgb;
   prevPub = d.public_rgb;
  }
  if (ce) prevScore = score;

  modeEvents += d.mode_sync;
  readyLow += !d.ready;
  d.clk = 0; d.eval();
  tick++;
 }

 RelockResult run() {
  for (unsigned n = 0; n < 1024; n++) step();
  d.reset_btn = 0;
  unsigned timeout = 9000000;
  while (!(transition && seenLoop && d.c4 == 34 && d.c9 == 2 && d.c0 == 0)) {
   need(timeout > 0, "fixed post-transition natural capture boundary absent");
   timeout--;
   step();
  }
  need(firstComplete && complete >= 4 && !vsAfter && d.filter_size == 256 &&
       d.applied_mode == 0 && d.requested_mode == 2 && d.effective_decode == 4 &&
       modeEvents > 10 && readyLow > 0,
       "fixed capture input/mode/nonvacuity setup");
  window = tick;
  timeline.reserve(2048);
  capturing = true;
  for (unsigned n = 0; n < 8192; n++) step();
  capturing = false;
  need(visible == 768 && accept11 > 0 && accept12 > 0 && rawEdges > 0 && pubEdges > 0 &&
       !vsAfter && complete >= 6,
       "fixed visible BORDER nonvacuity and no context/VS cancellation");
  return {name, warm, target, r2, visible, accept11, accept12, rawEdges, pubEdges,
          mismatch, historyBad, complete, window, transition, firstComplete,
          std::move(timeline), std::move(acceptedLedger)};
 }
};

}
int main(int argc,char**argv){
 Verilated::commandArgs(argc,argv);cout<<unitbuf;
 try {
  Bench border4(1,{4},false,true),border5(1,{5},false,true);border4.boot();auto a=border4.capture();border5.boot();auto b=border5.capture();
  need(compare_left8(a,b,8,31)==752,"entire border sample window");
  unsigned staticRuns=0,anchors=0;
  for(unsigned type:{0u,1u}){
   array<vector<Pixel>,3> p;
   for(unsigned w=4;w<=6;w++){
    Bench h(type,{w});h.boot();p[w-4]=h.capture();
    need(capture_hash(p[w-4],false)==old_capture[type][w-4],"whole old VRAM/rawGA/acquisition regression");
    if(w!=4){need(capture_hash(p[w-4],true)==old_capture[type][w-4],"whole old public5/6 regression");anchors++;}staticRuns++;
   }
   need(compare_left8(p[0],p[1],1,48)==1520,"whole Full-visible line");
   need(compare_left8(p[0],p[1],0,2)==80&&compare_left8(p[0],p[1],38,42)==144,"DE edge windows");edges(p[0],p[1]);
  }
  unsigned rapidRelations=0;
  for(auto widths:{vector<unsigned>{4,5,4,5,6},vector<unsigned>{5,4,5,4,6}}){
   Bench h(1,widths,true);h.boot();vector<vector<Pixel>> p;
   for(unsigned s=0;s<widths.size();s++){
    p.push_back(h.capture(s));need(capture_hash(p.back(),false)==old_capture[1][widths[s]-4],"rapid old rawGA/VRAM identity");
    if(widths[s]!=4){need(capture_hash(p.back(),true)==old_capture[1][widths[s]-4],"rapid old5/6 anchor");anchors++;}
    if(s&&widths[s]!=6){unsigned lo=widths[s-1]==4?s-1:s,hi=widths[s-1]==4?s:s-1;
     need(compare_left8(p[lo],p[hi],1,48)==1520,"first-following-frame visible relation");rapidRelations++;}
   }
  }
  // Two actual normal callers produce unedited save headers and coherent RAM.
  Bench producer6(1,{6});producer6.boot();producer6.until([&]{return producer6.armed&&producer6.frames>=3&&producer6.d.c4==6&&producer6.d.c9==0&&producer6.d.c0==20;},5000000,"normal6 capture source");producer6.setup();auto six=producer6.save();
  Bench h(1,{4});h.boot();h.until([&]{return h.armed&&h.frames>=3&&h.d.c4==6&&h.d.c9==0&&h.d.c0==20;},5000000,"normal4 lifecycle source");h.setup();h.nonzero();h.probe=Bench::Delayed;
  h.mode(1);h.mode(2);h.mode(0);h.source_class();h.probe=Bench::Delayed;h.active();h.nonzero();
  h.d.reset_btn=1;h.run(1024);h.stage=-1;h.writes=0;h.armed=false;h.frames=0;h.d.reset_btn=0;
  h.source_class();h.probe=Bench::Delayed;h.active();h.nonzero();auto four=h.save();h.restore(four,4);h.restore(six,6);
  need(staticRuns==6&&rapidRelations==6&&anchors==10,"fixed owner coverage count");
  cout<<"classic-r3-position: PASS12 callers; BORDER0/752; both types visible0/1520 DE0/80+0/144; 16 old rawGA/VRAM streams; 10 public5/6 anchors; 6 rapid relations; public nonzero reset/Full-rearm; loader-reset SNA4to4+4to6\n";

  // Combined R2 relocation and R3 width transition regression:
  // Full width6 publicRGB must follow rawGA after complete new width6 raw pulses.
  struct RelockCase { string name; unsigned warm, target, r2; };
  const RelockCase relockCases[] = {
   {"positive4_fixed", 4, 4, 48},
   {"bypass6_fixed", 4, 6, 48},
   {"relocate6", 6, 6, 49},
   {"relocate4", 4, 4, 49},
   {"MAIN_4to6_relocate", 4, 6, 49}
  };
  vector<RelockResult> relockResults;
  relockResults.reserve(5);
  for (const auto& c : relockCases) {
   RelockBench b(c.name, c.warm, c.target, c.r2);
   relockResults.push_back(b.run());
  }

  const auto& pos4 = relockResults[0];
  const auto& bypass6 = relockResults[1];
  const auto& relocate6 = relockResults[2];
  const auto& relocate4 = relockResults[3];
  const auto& mainCase = relockResults[4];

  need(pos4.acceptedLedger.size() == 14, "14 visible accepted BORDER latches expected");
  need(pos4.accept11 == 8 && pos4.accept12 == 6, "exact accepted BORDER 11/12 count");
  for (size_t i = 1; i < relockResults.size(); i++) {
   need(relockResults[i].acceptedLedger == pos4.acceptedLedger,
        "visible accepted-input ledger mismatch for caller " + relockResults[i].name);
  }

  need(mainCase.timeline.size() == 2048 && relocate6.timeline.size() == 2048, "complete 2048-sample capture window");
  need(mainCase.timeline == relocate6.timeline,
       "MAIN and relocation control rawGA/visibility timeline mismatch");

  need(pos4.historyBad == 0 && pos4.mismatch > 0,
       "positive4 control must follow 8-dot history (historyBad=0, mismatch>0)");
  need(pos4.rawEdges > 0 && pos4.pubEdges > 0,
       "positive4 control must exercise actual edge transitions, not uniform color");

  need(bypass6.mismatch == 0, "bypass6_fixed control must pass zero public/rawGA mismatch");
  need(bypass6.historyBad > 0, "bypass6_fixed control history8 mismatch");

  need(relocate6.mismatch == 0, "relocate6 control must pass zero public/rawGA mismatch");
  need(relocate6.historyBad > 0, "relocate6 control history8 mismatch");

  need(relocate4.historyBad == 0 && relocate4.mismatch > 0,
       "relocate4 control must follow 8-dot history (historyBad=0, mismatch>0)");
  need(relocate4.rawEdges > 0 && relocate4.pubEdges > 0,
       "relocate4 control must exercise actual edge transitions, not uniform color");

  cout << "classic-r3-position: relock controls PASS ("
       << "positive4 mismatch=" << pos4.mismatch << "/768 historyBad=" << pos4.historyBad
       << ", bypass6 mismatch=" << bypass6.mismatch << "/768 historyBad=" << bypass6.historyBad
       << ", relocate6 mismatch=" << relocate6.mismatch << "/768 historyBad=" << relocate6.historyBad
       << ", relocate4 mismatch=" << relocate4.mismatch << "/768 historyBad=" << relocate4.historyBad
       << "); MAIN mismatch=" << mainCase.mismatch << "/768\n";

  need(mainCase.mismatch == 0,
       "MAIN width6 public/rawGA identity mismatch: " + to_string(mainCase.mismatch) + "/768 visible samples");
  cout << "classic-r3-position: PASS 17 callers (12 existing + 5 R2/R3 relock)\n";
 }catch(const exception&e){cerr<<"classic-r3-position: FAIL "<<e.what()<<'\n';return 1;}
}
