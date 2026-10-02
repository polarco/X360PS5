// SPDX-License-Identifier: GPL-3.0-or-later
#include "xenia/base/system.h"
#include <cstdio>
#include <stdexcept>
namespace xe {
void ShowSimpleMessageBox(SimpleMessageBoxType, std::string_view message) {
  // FatalError terminates after this; stdout and the diagnostic's last BEGIN
  // remain the available evidence if the core fails before returning.
  std::fwrite(message.data(),1,message.size(),stderr); std::fputc('\n',stderr); std::fflush(stderr);
}
void LaunchWebBrowser(std::string_view) { throw std::runtime_error("PS5 browser launch unsupported"); }
void LaunchFileExplorer(const std::filesystem::path&) { throw std::runtime_error("PS5 file explorer unsupported"); }
bool SetProcessPriorityClass(uint32_t) { return false; }
bool IsUseNexusForGameBarEnabled() { return false; }
}
