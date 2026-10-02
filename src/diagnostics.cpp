// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics.hpp"
#include "build_info.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

namespace x360 {
const char* name(Status s) {
  switch (s) { case Status::passed: return "APROVADO";
    case Status::failed: return "FALHOU"; default: return "NAO TESTADO"; }
}
std::uint64_t monotonic_us() {
  timespec t{};
  if (clock_gettime(CLOCK_MONOTONIC, &t)) return 0;
  return std::uint64_t(t.tv_sec) * 1000000 + t.tv_nsec / 1000;
}
Report::Report(const char* dir) {
  mkdir(dir, 0700);
  for (unsigned attempt=0; attempt<100; ++attempt) {
    path_ = std::string(dir) + "/run-" + std::to_string(getpid()) + "-" +
            std::to_string(monotonic_us()) + "-" + std::to_string(attempt) + ".log";
    int fd = open(path_.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
    if (fd >= 0) { file_ = fdopen(fd, "w"); if (!file_) close(fd); break; }
    if (errno != EEXIST) break;
  }
  note(std::string("X360PS5 ") + X360_VERSION + " environment=" + X360_ENVIRONMENT);
  note(std::string("dependencies=") + X360_DEPENDENCIES);
  note("target firmware=13.60 Relapse (user supplied; not automatically verified)");
  note("runtime model/firmware/HEN: tester must include these in the report");
  for (auto id : {"filesystem", "memory", "threads", "jit_x86", "exceptions",
                  "xenia_ppc", "vulkan", "compute", "gpu_readback", "presentation", "stability"})
    results.push_back({id, Status::untested, "not executed"});
  bool readback=false;
  if(persisted()) {
    if(FILE* input=fopen(path_.c_str(),"r")) {
      char line[256]{}; readback=fgets(line,sizeof(line),input) && strstr(line,"X360PS5"); fclose(input);
    }
  }
  record("filesystem", persisted() && readback ? Status::passed : Status::failed,
         persisted() && readback ? "log write/flush/reopen/read: "+path_ : "log persistence/readback failed; check app permissions");
}
Report::~Report() { if (file_) fclose(file_); }
void Report::note(const std::string& s) {
  printf("%llu %s\n", static_cast<unsigned long long>(monotonic_us()), s.c_str());
  fflush(stdout);
  if (file_) {
    if (fprintf(file_, "%llu %s\n", static_cast<unsigned long long>(monotonic_us()), s.c_str()) < 0 || fflush(file_) != 0) {
      fclose(file_); file_ = nullptr;
      for (auto& r : results) if (r.id == "filesystem") { r.status=Status::failed; r.detail="log write/flush failed"; }
    }
  }
}
void Report::begin(const char* id) {
  for (auto& r : results) if (r.id == id) { r.status=Status::untested; r.detail="started; no result yet"; }
  note(std::string("BEGIN ") + id + " (missing END means interrupted, never passed)");
}
void Report::record(const char* id, Status s, const std::string& detail) {
  bool found=false;
  for (auto& r : results) if (r.id == id) { r.status=s; r.detail=detail; found=true; break; }
  if (!found) results.push_back({id,s,detail});
  note(std::string("END ") + id + " " + name(s) + " " + detail);
}
void run_memory(Report& r) {
  r.begin("memory");
  bool ok=true;
  for (int n=0; n<16; ++n) {
    Pages p(65536);
    if (!p.data) { r.record("memory",Status::failed,"allocate error="+std::to_string(p.error)); return; }
    auto* words=static_cast<std::uint32_t*>(p.data);
    for (std::size_t i=0;i<p.size/4;++i) words[i]=std::uint32_t(i)^0x5a39a5c3u;
    ok &= (reinterpret_cast<std::uintptr_t>(p.data) % 16384 == 0);
    ok &= p.protect(1);
    for (std::size_t i=0;i<p.size/4;++i) ok &= words[i]==(std::uint32_t(i)^0x5a39a5c3u);
    if (!p.protect(3)) { ok=false; break; }
  }
  r.record("memory",ok?Status::passed:Status::failed,"16 allocate/read/write/protect/free cycles; 16KiB alignment; does not prove Xenia guest address layout");
}
namespace {
struct ThreadData { pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER; unsigned counter=0; };
void* count(void* p) { auto& d=*static_cast<ThreadData*>(p); for(int i=0;i<10000;++i) { pthread_mutex_lock(&d.lock); ++d.counter; pthread_mutex_unlock(&d.lock); } return nullptr; }
}
void run_threads(Report& r) {
  r.begin("threads"); ThreadData d; pthread_t threads[2]; unsigned started=0;
  auto before=monotonic_us();
  for (;started<2;++started) if(pthread_create(&threads[started],nullptr,count,&d)) break;
  bool ok=started==2;
  for(unsigned i=0;i<started;++i) ok &= pthread_join(threads[i],nullptr)==0;
  ok &= d.counter==20000 && monotonic_us()>=before;
  pthread_mutex_destroy(&d.lock);
  r.record("threads",ok?Status::passed:Status::failed,"two pthread workers + mutex; count="+std::to_string(d.counter));
}
void run_jit(Report& r) {
  r.begin("jit_x86"); Pages p(16384,true);
  if(!p.data) { r.record("jit_x86",Status::failed,"executable allocation denied error="+std::to_string(p.error)+"; check HEN JIT permission"); return; }
  const unsigned char program[]={0xb8,0x2a,0,0,0,0xc3};
  memcpy(p.data,program,sizeof(program));
  __builtin___clear_cache(static_cast<char*>(p.data),static_cast<char*>(p.data)+sizeof(program));
  int answer=reinterpret_cast<int(*)()>(p.data)();
  r.record("jit_x86",answer==42?Status::passed:Status::failed,"hand-written x86 return="+std::to_string(answer)+"; NOT Xenia/PowerPC validation");
}
void run_suite(Report& r) {
#if X360_PS5
  if(!r.failed()) run_xenia_memory(r);
#endif
  for(auto test : {run_memory, run_threads, run_jit, run_exception, run_xenia, run_gpu}) {
    if(r.failed()) { r.note("Suite stopped after failure; inspect log before retrying individual tests"); return; }
    test(r);
  }
}
}
