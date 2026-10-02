// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics.hpp"
#include "demo_renderer.hpp"
#include <array>
#include <cstring>
#include <sys/stat.h>
#include <sys/resource.h>
#include <ps5platform/heap.h>
#include <ps5platform/shm.h>
extern "C" {
int sceUserServiceInitialize(void*);
int sceUserServiceGetForegroundUser(int*);
int sceUserServiceGetInitialUser(int*);
int scePadInit();
int scePadOpen(int,int,int,const void*);
int scePadGetHandle(int,int,int);
int scePadReadState(int,void*);
int sceSystemServiceLoadExec(const char*,char* const[]);
}
namespace {
x360::Report* report;
int pad=-1; unsigned last_buttons=0,selection=0;
bool stress=false; std::uint64_t stress_end=0; unsigned cycles=0;
std::uint64_t next_usage=0;
void memory_usage() {
  struct ps5_heap_stats heap{}; ps5_heap_stats(&heap);
  report->note("title heap mapped_bytes="+std::to_string(heap.mapped_bytes)+" peak_bytes="+std::to_string(heap.peak_bytes)+" libc_fallbacks="+std::to_string(heap.libc_fallbacks));
  struct ps5_shm_stats shared{}; ps5_shm_live(&shared);
  report->note("shared memory object_bytes="+std::to_string(shared.object_bytes)+" view_bytes="+std::to_string(shared.view_bytes)+" range_bytes="+std::to_string(shared.range_bytes));
  rusage usage{};
  if(getrusage(RUSAGE_SELF,&usage)==0)
    report->note("memory metric ru_maxrss="+std::to_string(usage.ru_maxrss)+" KiB (process high water; does not include all GPU/direct allocations)");
  else report->note("memory metric getrusage unavailable; consumption not validated");
}
const char* entries[]={"MEMORY", "THREADS", "X86 JIT", "EXCEPTIONS", "XENIA PPC", "VULKAN COMPUTE", "ALL TESTS X3", "STRESS 10 MIN", "EXIT"};
void execute(unsigned selected) {
  using namespace x360;
  switch(selected) {
    case 0:run_memory(*report);run_xenia_memory(*report);break;
    case 1:run_threads(*report);break;
    case 2:run_jit(*report);break;
    case 3:run_exception(*report);break;
    case 4:run_xenia(*report);break;
    case 5:run_gpu(*report);break;
    case 6:for(int i=0;i<3;++i) { report->note("SUITE "+std::to_string(i+1)); run_suite(*report); if(report->failed()) break; }break;
    case 7:stress=true; cycles=0; stress_end=monotonic_us()+600000000; next_usage=0; report->begin("stability");memory_usage();break;
    case 8:report->note("EXIT requested via system service");
      report->note("LoadExec return="+std::to_string(sceSystemServiceLoadExec("exit",nullptr)));break;
  }
}
void draw(ps5::demo::Canvas& c) noexcept {
  using ps5::demo::Color; using namespace x360;
  alignas(8) std::array<unsigned char,120> sample{};
  if(pad<0) {
    int user=-1;
    if(sceUserServiceGetForegroundUser(&user)<0) sceUserServiceGetInitialUser(&user);
    if(user>=0) { pad=scePadOpen(user,0,0,nullptr); if(pad<0) pad=scePadGetHandle(user,0,0); }
  }
  unsigned pressed=0;
  if(pad>=0 && scePadReadState(pad,sample.data())>=0) {
    unsigned buttons=0; int connected=0; memcpy(&buttons,sample.data(),4); memcpy(&connected,sample.data()+0x4c,4);
    if(connected && !(buttons&0x80000000u)) { pressed=buttons&~last_buttons; last_buttons=buttons; }
    else last_buttons=0;
  }
  if(stress) {
    if(monotonic_us()>=next_usage) { memory_usage(); next_usage=monotonic_us()+30000000; }
    if(pressed&0x2000) { stress=false; report->record("stability",Status::untested,"cancelled by tester"); }
    else if(monotonic_us()>=stress_end) { stress=false; report->record("stability",Status::passed,"600 seconds controlled memory/thread load; cycles="+std::to_string(cycles)+"; not emulator/GPU soak"); }
    else {
      run_memory(*report);run_threads(*report);++cycles;
      for(const auto& r:report->results) if((r.id=="memory"||r.id=="threads")&&r.status==Status::failed) {stress=false;report->record("stability",Status::failed,"memory/thread failure");break;}
    }
  } else {
    if(pressed&0x10) selection=(selection+8)%9;
    if(pressed&0x40) selection=(selection+1)%9;
    if(pressed&0x4000) execute(selection);
  }
  c.clear(Color::background);
  c.text(70,60,"X360PS5 0 1 0",8,Color::white);
  c.text(70,145,"DIAGNOSTIC BUILD   PS5 13 60 RELAPSE TARGET",3,Color::cyan);
  c.text(70,205,"UP DOWN SELECT   CROSS RUN   CIRCLE CANCEL STRESS",3,Color::white);
  for(unsigned i=0;i<9;++i) {
    if(i==selection) c.rectangle(60,278+i*64,620,54,Color::panel);
    c.text(80,290+i*64,entries[i],4,i==selection?Color::cyan:Color::white);
  }
  unsigned row=0;
  for(const auto& r:report->results) {
    std::string id=r.id; for(auto& ch:id) { if(ch>='a'&&ch<='z') ch-=32; if(ch=='_')ch=' '; }
    c.text(740,285+row*52,id,3,Color::white);
    c.text(1230,285+row*52,name(r.status),3,r.status==Status::passed?Color::cyan:r.status==Status::failed?Color::magenta:Color::yellow);++row;
  }
  c.text(70,950,report->persisted()?"LOGS IN DATA X360PS5 LOGS":"LOG NOT SAVED   CHECK APP FILESYSTEM PERMISSION",3,report->persisted()?Color::white:Color::magenta);
  if(stress) c.text(70,1000,"STRESS RUNNING   CIRCLE CANCELS",3,Color::yellow);
}
}
int main() {
  mkdir("/data/x360ps5",0700);
  x360::Report session("/data/x360ps5/logs"); report=&session;
  sceUserServiceInitialize(nullptr); scePadInit();
  session.note("UI uses CPU canvas + VideoOut; Vulkan presentation remains untested");
  ps5::demo::run(draw,"X360PS5 diagnostic ready");
}
