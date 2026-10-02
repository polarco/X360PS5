// SPDX-License-Identifier: GPL-3.0-or-later
#include "xenia/base/memory.h"
#include <cstdio>
#include <cstring>
#include <limits>
int main() {
  using namespace xe::memory;
  auto check=[](bool ok,const char* what){if(!ok)std::fprintf(stderr,"FAILED: %s\n",what);return ok;};
  bool ok=true;
  ok &= check(!AllocFixed(nullptr,0,AllocationType::kReserve,PageAccess::kNoAccess),"zero size rejected");
  ok &= check(!AllocFixed(reinterpret_cast<void*>(0x1234),65536,AllocationType::kReserve,PageAccess::kNoAccess),"unaligned address rejected");
  auto* area=AllocFixed(nullptr,131072,AllocationType::kReserve,PageAccess::kNoAccess);
  if(!check(area!=nullptr,"reserve"))return 1;
  auto h=CreateFileMappingHandle({},65536,PageAccess::kReadWrite,false);
  auto* a=static_cast<unsigned char*>(MapFileView(h,area,65536,PageAccess::kReadWrite,0));
  auto* b=static_cast<unsigned char*>(MapFileView(h,static_cast<char*>(area)+65536,65536,PageAccess::kReadWrite,0));
  if(!check(a&&b,"two aliases"))return 1;
  a[37]=91;ok &= check(b[37]==91,"physical alias identity");
  ok &= check(!MapFileView(h,a,65536,PageAccess::kReadWrite,0),"live view never overwritten");
  PageAccess previous{},access{};size_t length=0;
  ok &= check(Protect(a+16384,16384,PageAccess::kReadOnly,&previous)&&previous==PageAccess::kReadWrite,"partial protection");
  ok &= check(QueryProtect(a,length,access)&&length==16384&&access==PageAccess::kReadWrite,"query stops at protection boundary");
  ok &= check(QueryProtect(a+16384,length,access)&&length==16384&&access==PageAccess::kReadOnly,"query read-only page");
  ok &= check(!Protect(a+131072,16384,PageAccess::kReadWrite),"unknown memory rejected");
  ok &= check(DeallocFixed(area,0,DeallocationType::kRelease),"release first view");
  ok &= check(b[37]==91,"second alias survives release of first view");
  ok &= check(!DeallocFixed(area,0,DeallocationType::kRelease),"reservation cannot release while second alias is live");
  if(QueryProtect(a,length,access)) ok &= check(UnmapFileView(h,a,65536),"unmap first");
  ok &= check(UnmapFileView(h,b,65536),"unmap second");
  CloseFileMappingHandle(h,{});
  ok &= check(DeallocFixed(area,0,DeallocationType::kRelease),"release empty reservation");
  auto* p=AllocFixed(nullptr,65536,AllocationType::kReserveCommit,PageAccess::kReadWrite);
  if(!check(p!=nullptr,"reserve commit"))return 1;
  memset(p,0xab,65536);
  ok &= check(DeallocFixed(p,65536,DeallocationType::kDecommit),"decommit");
  ok &= check(AllocFixed(p,65536,AllocationType::kCommit,PageAccess::kReadWrite)==p,"recommit");
  for(size_t i=0;i<65536;++i) if(static_cast<unsigned char*>(p)[i]) {ok=false;break;}
  ok &= check(DeallocFixed(p,0,DeallocationType::kRelease),"release");
  return ok?0:1;
}
