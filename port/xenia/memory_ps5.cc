// SPDX-License-Identifier: GPL-3.0-or-later
// Implements Xenia's memory API through the pinned PS5 SDK, never /proc.
// Tracks only allocations owned by this adapter. Unknown ranges fail closed.
#include "xenia/base/memory.h"
#include <ps5platform/exec.h>
#include <ps5platform/shm.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <mutex>
#include <vector>
extern "C" int sceKernelMprotect(const void*,size_t,int);
namespace xe::memory {
namespace {
constexpr size_t page=0x4000;
struct View { size_t size; int handle; bool owned_handle; bool owned_range; std::vector<PageAccess> access; };
std::recursive_mutex guard;
std::map<uintptr_t,size_t> ranges;
std::map<uintptr_t,View> views;
std::map<int,ps5_shm> handles;
int next_handle=1;
bool aligned(uintptr_t x) { return x%page==0; }
bool valid(uintptr_t base,size_t length) { return length && aligned(base) && aligned(length) && length<=std::numeric_limits<uintptr_t>::max()-base; }
bool contains(uintptr_t outer,size_t bytes,uintptr_t inner,size_t length) { return inner>=outer && length<=bytes && inner-outer<=bytes-length; }
auto view_at(uintptr_t at) {
  auto it=views.upper_bound(at); if(it==views.begin()) return views.end(); --it;
  return contains(it->first,it->second.size,at,1)?it:views.end();
}
bool overlap(uintptr_t at,size_t length) {
  for(const auto& [base,v]:views) if(at<base+v.size && base<at+length) return true;
  return false;
}
bool reserve(size_t length,void* hint,void** out) {
  if(ps5_vrange_reserve(length,hint,0x10000,out)) return false;
  if(hint && *out!=hint) { ps5_vrange_release(*out,length); *out=nullptr; return false; }
  ranges.emplace(reinterpret_cast<uintptr_t>(*out),length); return true;
}
bool protect(void* address,size_t length,PageAccess access,PageAccess* old) {
  uintptr_t raw=reinterpret_cast<uintptr_t>(address);
  if(!length || length>std::numeric_limits<uintptr_t>::max()-raw-page) return false;
  uintptr_t at=raw&~uintptr_t(page-1); size_t bytes=((raw+length+page-1)&~uintptr_t(page-1))-at;
  auto it=view_at(at); if(it==views.end() || !contains(it->first,it->second.size,at,bytes)) return false;
  size_t first=(at-it->first)/page;
  if(sceKernelMprotect(reinterpret_cast<void*>(at),bytes,int(access))) return false;
  if(old) *old=it->second.access[first];
  std::fill_n(it->second.access.begin()+first,bytes/page,access); return true;
}
}
size_t page_size() { return page; }
size_t allocation_granularity() { return 0x10000; }
bool IsWritableExecutableMemorySupported() {
  ps5_exec_request q{}; q.bytes=0x10000; ps5_exec_region region{};
  if(ps5_exec_alloc(&q,&region)) return false;
  ps5_exec_free(&region); return true;
}
FileMappingHandle CreateFileMappingHandle(const std::filesystem::path&,size_t bytes,PageAccess,bool) {
  std::lock_guard lock(guard);
  if(!valid(0,bytes) || next_handle==std::numeric_limits<int>::max()) return kFileMappingHandleInvalid;
  ps5_shm object{}; if(ps5_shm_create(bytes,&object)) return kFileMappingHandleInvalid;
  int handle=next_handle++; handles.emplace(handle,object); return handle;
}
void CloseFileMappingHandle(FileMappingHandle handle,const std::filesystem::path&) {
  std::lock_guard lock(guard);
  // The caller must unmap all aliases before releasing the physical allocation.
  for(const auto& [base,v]:views) if(v.handle==handle) return;
  auto it=handles.find(handle); if(it!=handles.end()) { ps5_shm_destroy(&it->second); handles.erase(it); }
}
void* MapFileView(FileMappingHandle handle,void* address,size_t length,PageAccess access,size_t offset) {
  std::lock_guard lock(guard);
  uintptr_t at=reinterpret_cast<uintptr_t>(address);
  auto h=handles.find(handle);
  if(h==handles.end() || !valid(at,length) || !aligned(offset) || !contains(0,h->second.bytes,offset,length) || overlap(at,length)) return nullptr;
  bool own=true;
  if(address) for(const auto& [base,bytes]:ranges) if(contains(base,bytes,at,length)) {own=false;break;}
  void* reserved=address;
  if(own && !reserve(length,address,&reserved)) return nullptr;
  void* mapped=nullptr;
  int rc=ps5_shm_map(&h->second,offset,length,reserved,int(access),PS5_SHM_FIXED,&mapped);
  if(rc) { if(own) {ps5_vrange_release(reserved,length);ranges.erase(reinterpret_cast<uintptr_t>(reserved));} return nullptr; }
  views.emplace(reinterpret_cast<uintptr_t>(mapped),View{length,handle,false,own,std::vector<PageAccess>(length/page,access)});
  return mapped;
}
bool UnmapFileView(FileMappingHandle handle,void* address,size_t length) {
  std::lock_guard lock(guard);
  auto at=reinterpret_cast<uintptr_t>(address); auto it=views.find(at);
  if(it==views.end() || it->second.handle!=handle || it->second.size!=length) return false;
  bool own=it->second.owned_range;
  if(ps5_shm_unmap(address,length,PS5_SHM_KEEP_RESERVED)) return false;
  views.erase(it);
  if(own) { if(ps5_vrange_release(address,length)) return false; ranges.erase(at); }
  return true;
}
void* AllocFixed(void* address,size_t length,AllocationType allocation,PageAccess access) {
  std::lock_guard lock(guard);
  uintptr_t at=reinterpret_cast<uintptr_t>(address);
  if(!valid(at,length)) return nullptr;
  if(allocation==AllocationType::kReserve) { void* out=nullptr; return reserve(length,address,&out)?out:nullptr; }
  auto existing=view_at(at);
  if(existing!=views.end()) {
    if(allocation!=AllocationType::kCommit || !contains(existing->first,existing->second.size,at,length)) return nullptr;
    return protect(address,length,access,nullptr)?address:nullptr;
  }
  if(allocation==AllocationType::kCommit) {
    bool owned=false; for(const auto& [base,bytes]:ranges) owned |= contains(base,bytes,at,length);
    if(!address || !owned) return nullptr;
  }
  auto handle=CreateFileMappingHandle({},length,access,true);
  if(handle==kFileMappingHandleInvalid) return nullptr;
  void* out=MapFileView(handle,address,length,access,0);
  if(!out) CloseFileMappingHandle(handle,{});
  else views.at(reinterpret_cast<uintptr_t>(out)).owned_handle=true;
  return out;
}
bool DeallocFixed(void* address,size_t length,DeallocationType type) {
  std::lock_guard lock(guard); auto at=reinterpret_cast<uintptr_t>(address);
  auto it=view_at(at);
  if(type==DeallocationType::kDecommit) {
    if(!valid(at,length) || it==views.end() || !contains(it->first,it->second.size,at,length)) return false;
    // Clear before making inaccessible; a subsequent commit observes zero pages.
    if(!protect(address,length,PageAccess::kReadWrite,nullptr)) return false;
    memset(address,0,length); return protect(address,length,PageAccess::kNoAccess,nullptr);
  }
  if(length!=0) return false;
  if(it!=views.end() && it->first==at) {
    auto info=it->second;
    if(!UnmapFileView(info.handle,address,info.size)) return false;
    if(info.owned_handle) CloseFileMappingHandle(info.handle,{});
    return true;
  }
  auto range=ranges.find(at); if(range==ranges.end() || overlap(at,range->second)) return false;
  if(ps5_vrange_release(address,range->second)) return false;
  ranges.erase(range); return true;
}
bool Protect(void* address,size_t length,PageAccess access,PageAccess* old) {
  std::lock_guard lock(guard); return protect(address,length,access,old);
}
bool QueryProtect(void* address,size_t& length,PageAccess& access) {
  std::lock_guard lock(guard); uintptr_t at=reinterpret_cast<uintptr_t>(address);
  auto it=view_at(at); if(it==views.end()) return false;
  size_t index=(at-it->first)/page; access=it->second.access[index];
  size_t end=index+1; while(end<it->second.access.size() && it->second.access[end]==access) ++end;
  length=(end-index)*page; return true;
}
}
