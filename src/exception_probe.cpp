// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics.hpp"
#include <csignal>
#include <cstring>
#include <ucontext.h>
#include <unistd.h>
#if X360_PS5
#include <ps5platform/context.h>
#endif
namespace x360 {
namespace {
volatile sig_atomic_t observed=0;
std::uintptr_t expected=0;
void handle(int signal, siginfo_t*,void* context) {
  auto* uc=static_cast<ucontext_t*>(context);
#if X360_PS5
  auto& ip=uc->uc_mcontext.mc_rip;
#else
  auto& ip=uc->uc_mcontext.gregs[REG_RIP];
#endif
  if(signal==SIGILL && static_cast<std::uintptr_t>(ip)==expected) { observed=1; ip+=2; return; }
  // Never resume arbitrary faults. This handler owns only our exact UD2 instruction.
  _exit(128+signal);
}
}
void run_exception(Report& r) {
  r.begin("exceptions");
  Pages p(16384,true);
  if(!p.data) { r.record("exceptions",Status::untested,"requires executable memory"); return; }
  const unsigned char program[]={0x0f,0x0b,0xc3}; memcpy(p.data,program,sizeof(program));
  struct sigaction action{},old{}; action.sa_sigaction=handle; action.sa_flags=SA_SIGINFO;
  sigemptyset(&action.sa_mask); observed=0; expected=reinterpret_cast<std::uintptr_t>(p.data);
  if(sigaction(SIGILL,&action,&old)) { r.record("exceptions",Status::failed,"sigaction refused"); return; }
  reinterpret_cast<void(*)()>(p.data)();
  bool restored=sigaction(SIGILL,&old,nullptr)==0; expected=0;
  r.record("exceptions",observed && restored?Status::passed:Status::failed,"exact UD2 instruction recovered via signal context; handler restored");
}
}
