// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#if X360_PS5
#include <ps5platform/exec.h>
#endif

namespace x360 {
enum class Status { untested, passed, failed };
const char* name(Status status);
struct Result { std::string id; Status status = Status::untested; std::string detail; };
class Report {
 public:
  explicit Report(const char* directory);
  ~Report();
  void begin(const char* id);
  void record(const char* id, Status status, const std::string& detail);
  void note(const std::string& message);
  bool persisted() const { return file_ != nullptr; }
  bool failed() const { for(const auto& r:results) if(r.status==Status::failed) return true; return false; }
  const std::string& path() const { return path_; }
  std::vector<Result> results;
 private:
  FILE* file_ = nullptr;
  std::string path_;
};
class Pages {
 public:
  explicit Pages(std::size_t size, bool executable = false);
  ~Pages();
  Pages(const Pages&) = delete;
  Pages& operator=(const Pages&) = delete;
  bool protect(int protection);
  void* data = nullptr;
  std::size_t size = 0;
  int error = 0;
 private:
#if X360_PS5
  ps5_exec_region region_{};
#endif
};
std::uint64_t monotonic_us();
void run_memory(Report& report);
void run_threads(Report& report);
void run_jit(Report& report);
void run_exception(Report& report);
void run_xenia(Report& report);
void run_xenia_memory(Report& report);
void run_gpu(Report& report);
void run_suite(Report& report);
}
