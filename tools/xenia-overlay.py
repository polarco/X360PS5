#!/usr/bin/env python3
"""Apply the platform patch only to an exported header, retaining upstream source."""
from pathlib import Path
import subprocess
import sys
import os
root=Path(__file__).resolve().parents[1]
out=Path(sys.argv[1]).resolve()
previous={p:(p.read_bytes(),p.stat().st_atime_ns,p.stat().st_mtime_ns) for p in out.rglob('*') if p.is_file()} if out.exists() else {}
target=out/'src/xenia/base/platform.h'
target.parent.mkdir(parents=True,exist_ok=True)
data=subprocess.check_output(['git','-C',str(root/'.deps/xenia'),'show','b083312b8b18e07e6e410b82104191f126722794:src/xenia/base/platform.h'])
target.write_bytes(data)
subprocess.run(['git','apply','--unsafe-paths','--directory='+str(out),str(root/'patches/xenia/platform.patch')],check=True,cwd=root)
source=(root/'.deps/xenia/src/xenia/base/exception_handler_posix.cc').read_text()
source=source.replace('#include "xenia/base/exception_handler.h"','#include "xenia/base/exception_handler.h"\n#include <ps5platform/context.h>')
registers=['RAX','RCX','RDX','RBX','RSP','RBP','RSI','RDI','R8','R9','R10','R11','R12','R13','R14','R15','RIP','EFL','ERR']
for register in registers:
    field='rflags' if register=='EFL' else register.lower()
    source=source.replace(f'mcontext.gregs[REG_{register}]',f'mcontext.mc_{field}')
source=source.replace('greg_t(', '__register_t(')
source=source.replace('mcontext.fpregs->_xmm','reinterpret_cast<vec128_t*>(reinterpret_cast<unsigned char*>(mcontext.mc_fpstate) + 160)')
old='''static constexpr size_t kIntRegisterMap[] = {
          REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP,
          REG_RSI, REG_RDI, REG_R8,  REG_R9,  REG_R10, REG_R11,
          REG_R12, REG_R13, REG_R14, REG_R15,
      };'''
new='__register_t* kIntRegisterMap[] = {'+', '.join('&mcontext.mc_'+r.lower() for r in registers[:16])+'};'
if old not in source: raise RuntimeError('pinned Xenia context table changed')
source=source.replace(old,new).replace('mcontext.gregs[kIntRegisterMap[modified_register_index]]','*kIntRegisterMap[modified_register_index]')
# Parenthesize the XMM pointer before indexing to preserve operator precedence.
source=source.replace('&reinterpret_cast<vec128_t*>(reinterpret_cast<unsigned char*>(mcontext.mc_fpstate) + 160)[modified_register_index]', '&(reinterpret_cast<vec128_t*>(reinterpret_cast<unsigned char*>(mcontext.mc_fpstate) + 160))[modified_register_index]')
(out/'exception_handler_ps5.cc').write_text(source)
clock=(root/'.deps/xenia/src/xenia/base/clock_posix.cc').read_text()
clock=clock.replace('CLOCK_MONOTONIC_RAW','CLOCK_MONOTONIC')
clock=clock.replace('return 1000000000ull / res.tv_nsec;', 'return 1000000000ull; // tick_count returns nanoseconds, independent of resolution')
(out/'clock_ps5.cc').write_text(clock)
thread=(root/'.deps/xenia/src/xenia/base/threading_posix.cc').read_text()
thread=thread.replace('#include <pthread.h>', '#include <pthread.h>\n#include <pthread_np.h>\n#include <stdexcept>\nusing cpu_set_t = cpuset_t;\nstatic int pthread_getname_np(pthread_t t, char* s, size_t n) { pthread_get_name_np(t,s,n); return 0; }\nstatic void pthread_setname_np(pthread_t t, const char* s) { pthread_set_name_np(t,s); }')
thread=thread.replace('syscall(SYS_gettid)', 'pthread_getthreadid_np()')
thread=thread.replace('static_cast<uint32_t>(thread_)', 'static_cast<uint32_t>(tid_)')
thread=thread.replace('pthread_mutex_consistent(native_mutex);', 'pthread_mutex_unlock(native_mutex); throw std::runtime_error("PS5: robust mutex recovery unsupported");')
thread=thread.replace('pthread_sigqueue(thread_, GetSystemSignal(SignalType::kThreadUserCallback),\n                     value);', 'throw std::runtime_error("PS5: queued thread callbacks are not implemented");')
(out/'threading_ps5.cc').write_text(thread)
filesystem=(root/'.deps/xenia/src/xenia/base/filesystem_posix.cc').read_text()
start=filesystem.index('std::filesystem::path GetExecutablePath() {')
end=filesystem.index('\nstd::filesystem::path GetExecutableFolder()',start)
filesystem=filesystem[:start]+'std::filesystem::path GetExecutablePath() { return "/app0/eboot.bin"; }\n'+filesystem[end:]
start=filesystem.index('std::filesystem::path GetUserFolder() {')
end=filesystem.index('\nFILE* OpenFile(',start)
filesystem=filesystem[:start]+'std::filesystem::path GetUserFolder() { return "/data/x360ps5"; }\n'+filesystem[end:]
(out/'filesystem_ps5.cc').write_text(filesystem)
for path,(content,atime,mtime) in previous.items():
    if path.exists() and path.read_bytes()==content: os.utime(path,ns=(atime,mtime))
