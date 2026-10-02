// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics.hpp"
int main(int argc,char** argv) {
  x360::Report report(argc>1?argv[1]:"build/host-logs");
  for(unsigned n=0;n<3;++n) {
    report.note("Xenia lifecycle iteration="+std::to_string(n+1));
    x360::run_xenia(report);
    if(report.failed()) return 1;
  }
  for(const auto& r:report.results) if(r.id=="xenia_ppc") return r.status==x360::Status::passed?0:1;
  return 2;
}
