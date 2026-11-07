// Technical information sourced from the Amstrad CPC CRTC Compendium
// by Longshot (CC BY-NC-ND 4.0). FR §§27.1-27.2 p283, §27.3.2 p284.
#include "Vasic_ga_timing.h"
#include "Vasic_ga_timing___024root.h"
#include "verilated.h"
#include <cstdio>
#include <cstdint>
#include <string>

struct Bench {
 Vasic_ga_timing d;
 uint64_t ticks=0; unsigned h=0,line=0; bool rst=true;
 unsigned vs_start=8, vs_length=16; bool enable_vs=true;
 int first_irq_line=-1; unsigned first_irq_h=0; uint64_t first_irq_tick=0;
 bool last_int=true;
 Bench() {
  d.clk=0; d.fast=0; d.plus_unlocked=0; d.A=0; d.D=0;
  d.MREQ_N=1; d.M1_N=1; d.RD_N=1; d.IORQ_N=1; d.intack=0;
  d.pri=0; d.crtc_adj=0; d.HSYNC_I=0; d.VSYNC_I=0; d.crtc_line=0;
  d.SNA_LOAD=0; d.SNA_INKSEL=0; for(unsigned i=0;i<5;++i)d.SNA_PALETTE[i]=0; d.SNA_CONFIG=0;
  d.SNA_VSDELAY=0; d.SNA_INTCNT=0; d.SNA_INT=0; d.SNA_VS=0; d.SNA_HS=0;
 }
 unsigned count() { return d.rootp->asic_ga_timing__DOT__intcnt_reg; }
 bool clear() { return d.rootp->asic_ga_timing__DOT__intcntclr_4; }
 bool delayed() { return d.rootp->asic_ga_timing__DOT__intcnt_cnt; }
 bool tick() {
  d.clk=0; d.cen_16=(ticks%4==0); d.RESET_N=!rst; d.eval();
  bool ce=d.CCLK_EN_N; bool old_int=d.INT_N;
  d.clk=1; d.eval();
  // asic_video registers change on the SAME edge that samples old HS/VS.
  // Updating producer inputs after DUT evaluation reproduces that NBA ordering.
  if(rst) {h=0;line=0;d.HSYNC_I=0;d.VSYNC_I=0;d.crtc_line=0;}
  else if(ce) {
   if(++h==64) {h=0;++line;}
   if(h==46) d.HSYNC_I=1;
   if(h==50) d.HSYNC_I=0;
   if(h==0) d.VSYNC_I=enable_vs && line>=vs_start && line<vs_start+vs_length;
   d.crtc_line=line&511;
  }
  d.eval(); ++ticks;
  if(!rst && old_int && !d.INT_N && first_irq_line<0) {
   first_irq_line=line; first_irq_h=h; first_irq_tick=ticks;
   std::printf("IRQ tick=%llu line=%u character=%u count=%u\n",(unsigned long long)ticks,line,h,count());
  }
  last_int=d.INT_N;
  return ce;
 }
 void run(unsigned n) {while(n--)tick();}
 void boot() {
  // U204's published reset/restart bus combination, normal clock enables.
  d.M1_N=0;d.IORQ_N=0;d.RD_N=0;d.intack=1;run(256);
  d.M1_N=1;d.IORQ_N=1;d.RD_N=1;d.intack=0;rst=false;
 }
 void until(unsigned l,unsigned c) {for(unsigned guard=0;guard<2000000;++guard){if(line==l&&h==c)return;tick();}std::puts("producer timeout");std::exit(3);}
};
int errors=0;
void check(bool yes,const char* name,unsigned line,unsigned got,unsigned expected) {
 std::printf("%s %s line=%u got=%u expected=%u\n",yes?"PASS":"FAIL",name,line,got,expected);
 if(!yes)++errors;
}
void natural() {
 Bench b; b.boot();
 unsigned old_h=0; unsigned previous_fall=0; unsigned falls=0;
 while(b.line<63) {
  bool old_hs=b.d.HSYNC_I; b.tick();
  if(old_hs&&!b.d.HSYNC_I) {
   if(previous_fall) check(b.ticks-previous_fall==4096,"producer_line_ticks",b.line,b.ticks-previous_fall,4096);
   previous_fall=b.ticks;++falls;
  }
  if(b.h==52 && old_h!=52 && b.line>=9 && b.line<=61) {
   // FR §27.2 p283 / §27.3.2 p284: second fall absorbs its line;
   // precisely 52 SUBSEQUENT physical line ends produce the next wrap.
   unsigned expect=(b.line-9)%52;
   check(b.count()==expect,"VS_relative_R52",b.line,b.count(),expect);
  }
  old_h=b.h;
 }
 check(b.first_irq_line==61,"first_wrap_IRQ_physical_line",b.first_irq_line,b.first_irq_line,61);
 check(b.first_irq_h>=50 && b.first_irq_h<64,"first_wrap_IRQ_in_line",61,b.first_irq_h,50);
 check(falls==63,"producer_complete_lines",63,falls,63);
}
int main(int argc,char** argv){Verilated::commandArgs(argc,argv);natural();std::printf("proof: %s errors=%d\n",errors?"FAIL":"PASS",errors);return errors?1:0;}
