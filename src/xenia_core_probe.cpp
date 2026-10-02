// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics.hpp"
#include "xenia/base/byte_order.h"
#include "xenia/cpu/backend/x64/x64_backend.h"
#include "xenia/cpu/processor.h"
#include "xenia/cpu/raw_module.h"
#include "xenia/cpu/thread_state.h"
#include "xenia/memory.h"
#include <exception>
namespace x360 {
static void run_xenia_impl(Report& r) {
  r.begin("xenia_ppc");
  if(!xe::memory::IsWritableExecutableMemorySupported()) {
    r.record("xenia_ppc",Status::failed,"executable memory unavailable; backend not started"); return;
  }
  xe::Memory memory;
  if(!memory.Initialize()) { r.record("xenia_ppc",Status::failed,"Xenia guest memory initialization failed"); return; }
  xe::cpu::Processor processor(&memory,nullptr);
  if(!processor.Setup(std::make_unique<xe::cpu::backend::x64::X64Backend>())) { r.record("xenia_ppc",Status::failed,"Xenia backend initialization failed"); return; }
  constexpr std::uint32_t base=0x80000000;
  if(!memory.LookupHeap(base)->AllocFixed(base,0x10000,0x10000,xe::kMemoryAllocationReserve|xe::kMemoryAllocationCommit,xe::kMemoryProtectRead|xe::kMemoryProtectWrite)) {
    r.record("xenia_ppc",Status::failed,"guest code allocation failed"); return;
  }
  // Three separate guest functions: positive sum, signed addition, wrap to zero.
  const std::uint32_t words[][4]={
    {0x38600013,0x38800017,0x7c632214,0x4e800020},
    {0x3860ffed,0x38800017,0x7c632214,0x4e800020},
    {0x3860ffff,0x38800001,0x7c632214,0x4e800020}};
  const std::uint64_t expected[]={42,4,0};
  for(unsigned n=0;n<3;++n) for(unsigned i=0;i<4;++i)
    xe::store_and_swap<std::uint32_t>(memory.TranslateVirtual(base+n*16+i*4),words[n][i]);
  auto module=std::make_unique<xe::cpu::RawModule>(&processor);
  module->SetAddressRange(base,sizeof(words)); module->set_name("X360PS5 owned PPC arithmetic probe"); module->set_executable(true);
  processor.AddModule(std::move(module));
  processor.backend()->CommitExecutableRange(base,base+0x10000);
  auto stack=memory.SystemHeapAlloc(0x10000);
  if(!stack) { r.record("xenia_ppc",Status::failed,"guest stack allocation failed"); return; }
  bool ok=true;
  {
    xe::cpu::ThreadState state(&processor,0x100,stack+0x10000);
    auto* context=state.context();
    for(unsigned n=0;n<3;++n) {
      context->lr=0xbcbcbcbc; context->r[3]=0xdeadbeef;
      auto* function=processor.ResolveFunction(base+n*16);
      bool executed=false;
      if(function) { processor.backend()->SetGuestRoundingMode(context,0); executed=function->Call(&state,std::uint32_t(context->lr)); }
      ok &= executed && context->r[3]==expected[n];
      r.note("PPC case="+std::to_string(n)+" r3="+std::to_string(context->r[3])+" expected="+std::to_string(expected[n]));
      if(!ok) break;
    }
    r.record("xenia_ppc",ok?Status::passed:Status::failed,"Xenia PPC frontend + x64 backend: three arithmetic programs; no Xbox kernel or GPU");
  }
  memory.SystemHeapFree(stack);
}
void run_xenia(Report& r) {
  try { run_xenia_impl(r); }
  catch(const std::exception& e) { r.record("xenia_ppc",Status::failed,std::string("Xenia exception: ")+e.what()); }
}
}
