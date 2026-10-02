// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics.hpp"
#include <cstring>
int main(int argc,char** argv) {
  const char* directory=argc>1?argv[1]:"build/host-logs";
  x360::Report report(directory);
  x360::run_suite(report);
  bool failed=false;
  for(const auto& r:report.results) failed |= r.status==x360::Status::failed;
  return failed?1:0;
}
