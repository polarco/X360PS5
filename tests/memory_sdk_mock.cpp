// SPDX-License-Identifier: GPL-3.0-or-later
// Host-only model of the SDK boundary. Passing this does not validate PS5 syscalls.
#include <ps5platform/shm.h>
#include <ps5platform/exec.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
extern "C" {
int sceKernelMprotect(const void* p,size_t n,int prot) { return mprotect(const_cast<void*>(p),n,prot); }
int ps5_shm_create(size_t n,ps5_shm* s) {
  int fd=memfd_create("x360ps5-test",0); if(fd<0)return errno;
  if(ftruncate(fd,n)) {int e=errno;close(fd);return e;}
  s->direct_start=fd;s->bytes=n;return 0;
}
void ps5_shm_destroy(ps5_shm* s) {close(int(s->direct_start));s->direct_start=-1;}
int ps5_vrange_reserve(size_t n,void* hint,size_t,void** out) {
  if(hint) {
    *out=mmap(hint,n,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
  } else {
    auto* raw=static_cast<unsigned char*>(mmap(nullptr,n+65536,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
    if(raw==MAP_FAILED)return errno;
    auto aligned=(reinterpret_cast<uintptr_t>(raw)+65535)&~uintptr_t(65535);
    size_t prefix=aligned-reinterpret_cast<uintptr_t>(raw);
    if(prefix)munmap(raw,prefix);
    munmap(reinterpret_cast<void*>(aligned+n),65536-prefix);*out=reinterpret_cast<void*>(aligned);
  }
  return *out==MAP_FAILED?errno:0;
}
int ps5_vrange_release(void* p,size_t n) {return munmap(p,n);}
int ps5_shm_map(const ps5_shm* s,size_t offset,size_t n,void* at,int protection,unsigned flags,void** out) {
  *out=mmap(at,n,protection,MAP_SHARED|((flags&PS5_SHM_FIXED)?MAP_FIXED:0),int(s->direct_start),offset);
  return *out==MAP_FAILED?errno:0;
}
int ps5_shm_unmap(void* p,size_t n,unsigned flags) {
  if(flags&PS5_SHM_KEEP_RESERVED) return mmap(p,n,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==MAP_FAILED?errno:0;
  return munmap(p,n);
}
int ps5_exec_alloc(const ps5_exec_request* q,ps5_exec_region* r) {
  r->base=mmap(nullptr,q->bytes,7,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);r->bytes=q->bytes;
  return r->base==MAP_FAILED?errno:0;
}
void ps5_exec_free(ps5_exec_region* r) {if(r->base)munmap(r->base,r->bytes);}
}
