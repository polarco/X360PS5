// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics.hpp"
#include <cerrno>
#include <limits>
#include <sys/mman.h>
#include <unistd.h>
#if X360_PS5
extern "C" {
int sceKernelMprotect(const void*,std::size_t,int);
}
#endif
namespace x360 {
Pages::Pages(std::size_t bytes,bool executable) {
  if (!bytes || bytes>std::numeric_limits<std::size_t>::max()-16383) { error=EINVAL; return; }
  size=(bytes+16383)&~std::size_t(16383);
#if X360_PS5
  ps5_exec_request request{};
  request.bytes=size;
  request.flags=executable?0:PS5_EXEC_TOGGLED;
  error=ps5_exec_alloc(&request,&region_);
  if(!error) { data=region_.base; size=region_.bytes; }
#else
  // Reserve extra space then trim to the PS5 page alignment without MAP_FIXED.
  void* raw=mmap(nullptr,size+16384,PROT_READ|PROT_WRITE|(executable?PROT_EXEC:0),MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  if(raw==MAP_FAILED) { error=errno; return; }
  auto start=reinterpret_cast<std::uintptr_t>(raw);
  auto aligned=(start+16383)&~std::uintptr_t(16383);
  auto prefix=aligned-start;
  if(prefix) munmap(raw,prefix);
  munmap(reinterpret_cast<void*>(aligned+size),16384-prefix);
  data=reinterpret_cast<void*>(aligned);
#endif
}
Pages::~Pages() {
#if X360_PS5
  ps5_exec_free(&region_);
#else
  if(data) munmap(data,size);
#endif
}
bool Pages::protect(int p) {
  if(!data) return false;
#if X360_PS5
  error=sceKernelMprotect(data,size,p);
  return error==0;
#else
  if(mprotect(data,size,p)==0) return true;
  error=errno; return false;
#endif
}
}
