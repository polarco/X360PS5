// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics.hpp"
#include "xenia/base/memory.h"
#include <cstring>
namespace x360 {
void run_xenia_memory(Report& r) {
  r.begin("xenia_memory");
  using namespace xe::memory;
  constexpr size_t bytes=0x10000;
  auto handle=CreateFileMappingHandle({},bytes,PageAccess::kReadWrite,true);
  if(handle==kFileMappingHandleInvalid) { r.record("xenia_memory",Status::failed,"shared direct memory creation failed"); return; }
  void* a=MapFileView(handle,nullptr,bytes,PageAccess::kReadWrite,0);
  void* b=MapFileView(handle,nullptr,bytes,PageAccess::kReadWrite,0);
  bool ok=a&&b&&a!=b;
  if(ok) {
    std::memset(a,0x39,bytes);
    ok=std::memcmp(a,b,bytes)==0;
    static_cast<unsigned char*>(b)[17]=0xaa;
    ok &= static_cast<unsigned char*>(a)[17]==0xaa;
    PageAccess previous{};
    ok &= Protect(a,0x4000,PageAccess::kReadOnly,&previous) && previous==PageAccess::kReadWrite;
    size_t length=0; PageAccess queried{};
    ok &= QueryProtect(a,length,queried) && length==0x4000 && queried==PageAccess::kReadOnly;
    ok &= Protect(a,0x4000,PageAccess::kReadWrite);
    ok &= !Protect(reinterpret_cast<void*>(0x1234),1,PageAccess::kReadWrite);
  }
  if(b) ok &= UnmapFileView(handle,b,bytes);
  if(a) ok &= UnmapFileView(handle,a,bytes);
  CloseFileMappingHandle(handle,{});
  auto* p=AllocFixed(nullptr,bytes,AllocationType::kReserveCommit,PageAccess::kReadWrite);
  if(!p) ok=false;
  else {
    memset(p,0x5a,bytes);
    ok &= DeallocFixed(p,bytes,DeallocationType::kDecommit);
    if(AllocFixed(p,bytes,AllocationType::kCommit,PageAccess::kReadWrite)) {
      for(size_t i=0;i<bytes;++i) ok &= static_cast<unsigned char*>(p)[i]==0;
    } else ok=false;
    ok &= DeallocFixed(p,0,DeallocationType::kRelease);
  }
  r.record("xenia_memory",ok?Status::passed:Status::failed,"Xenia memory API: aliases, page protection query, decommit/zero/recommit, release; no full guest arena");
}
}
