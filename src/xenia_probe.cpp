// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics.hpp"
namespace x360 {
void run_xenia(Report& r) {
  r.begin("xenia_ppc");
  r.record("xenia_ppc",Status::untested,"Xenia core not linked in this diagnostic build; x86 test is not a PowerPC recompiler test");
}
}
