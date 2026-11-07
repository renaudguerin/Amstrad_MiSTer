// Reuses the same producer; adds real SNA_LOAD/RESET_N lifecycle operations.
// Technical information sourced from the Amstrad CPC CRTC Compendium
// by Longshot (CC BY-NC-ND 4.0). FR §27.2 p283, §27.3.2 p284.
#define main primary_proof_main
#include "asic_vs_rephase_test.cpp"
#undef main
void apply(Bench& b,unsigned delay,unsigned count,bool hs,bool vs) {
 b.d.HSYNC_I=hs;b.d.VSYNC_I=vs;
 b.d.SNA_HS=hs;b.d.SNA_VS=vs;b.d.SNA_VSDELAY=delay;
 b.d.SNA_INTCNT=count;b.d.SNA_INT=0;b.d.SNA_LOAD=1;
 b.tick();b.d.SNA_LOAD=0;b.run(2);
 check(b.count()==count,"import_B3_no_spurious_event",b.line,b.count(),count);
}
void restored_delay(bool active_hs) {
 Bench b;b.enable_vs=false;b.boot();b.until(5,active_hs?48:20);
 apply(b,1,17,active_hs,true);
 // B2=1 means one fall remains before reset; the next real end resets
 // its whole line. The following physical line must count exactly once.
 b.until(5,52);check(b.count()==0,"B2_one_reset_line",5,b.count(),0);
 b.until(6,52);check(b.count()==1,"B2_one_next_line",6,b.count(),1);
 b.until(57,52);check(b.count()==0,"B2_one_52nd_wrap",57,b.count(),0);
 check(b.first_irq_line==57,"B2_one_first_IRQ",b.first_irq_line,b.first_irq_line,57);
}
void pending_import(unsigned b2) {
 Bench b;b.boot();b.until(9,50);b.run(2);
 check(b.clear(),"import_inside_VS_clear",9,b.clear(),1);
 b.enable_vs=false;
 // Replace an already in-flight rephase by a real snapshot apply.
 // B2=0 must not preserve its old token; B2=1 must get a fresh reset.
 apply(b,b2,7,false,b2==1);
 b.until(9,52);check(b.count()==7,"import_discards_old_line",9,b.count(),7);
 b.until(10,52);check(b.count()==(b2?0:8),"import_next_line_owner",10,b.count(),b2?0:8);
 b.until(11,52);check(b.count()==(b2?1:9),"import_following_line_owner",11,b.count(),b2?1:9);
}
void expired_delay_active_hs() {
 Bench b;b.enable_vs=false;b.boot();b.until(5,48);
 apply(b,0,7,true,true);
 // B2=0 already consumed VS rephase; active restored HS must count only
 // its later real fall, never synthesize VS rephase a second time.
 b.until(5,52);check(b.count()==8,"B2_zero_real_fall",5,b.count(),8);
 b.until(6,52);check(b.count()==9,"B2_zero_next_line",6,b.count(),9);
}
void pending_reset() {
 Bench b;b.boot();b.until(9,50);b.run(2);
 check(b.clear(),"reset_inside_VS_clear",9,b.clear(),1);
 b.enable_vs=false;b.rst=true;
 b.d.M1_N=0;b.d.IORQ_N=0;b.d.RD_N=0;b.d.intack=1;b.run(256);
 b.d.M1_N=1;b.d.IORQ_N=1;b.d.RD_N=1;b.d.intack=0;b.rst=false;
 unsigned pc=99;bool pcl=false,pd=false;
 while(!(b.line==0 && b.h==52)) {
  b.tick();
  if(b.count()!=pc || b.clear()!=pcl || b.delayed()!=pd) {
   std::printf("RESET_DIAG tick=%llu line=%u h=%u count=%u vsclr=%u delayed=%u HS=%u VS=%u irqack=%u irqreset=%u\n",(unsigned long long)b.ticks,b.line,b.h,b.count(),b.clear(),b.delayed(),b.d.HSYNC_I,b.d.VSYNC_I,b.d.rootp->asic_ga_timing__DOT__irqack_rst,b.d.rootp->asic_ga_timing__DOT__irq_reset);
   pc=b.count();pcl=b.clear();pd=b.delayed();
  }
 }
 check(b.count()==1,"reset_first_real_fall",0,b.count(),1);
 b.until(1,52);check(b.count()==2,"reset_next_real_fall",1,b.count(),2);
}
int main(int argc,char**argv) {
 Verilated::commandArgs(argc,argv);
 restored_delay(false);restored_delay(true);expired_delay_active_hs();
 pending_import(0);pending_import(1);pending_reset();
 std::printf("lifecycle: %s errors=%d\n",errors?"FAIL":"PASS",errors);return errors?1:0;
}
