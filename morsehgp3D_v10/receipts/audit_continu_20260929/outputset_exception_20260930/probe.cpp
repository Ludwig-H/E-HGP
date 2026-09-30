#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <new>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

#include "core/cli_output.hpp"

static bool fail_one_allocation = false;
static unsigned allocation_failures = 0;

void* operator new(std::size_t n) {
  if (fail_one_allocation) {
    fail_one_allocation = false;
    ++allocation_failures;
    throw std::bad_alloc();
  }
  if (void* p = std::malloc(n == 0 ? 1 : n)) return p;
  throw std::bad_alloc();
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

static int descriptors() {
  DIR* d = opendir("/proc/self/fd");
  if (!d) return -1;
  int n = 0;
  while (dirent* e = readdir(d))
    if (std::strcmp(e->d_name, ".") != 0 && std::strcmp(e->d_name, "..") != 0) ++n;
  if (closedir(d) != 0) return -1;
  return n;
}

static long size_of(const std::string& p) {
  struct stat s {};
  return stat(p.c_str(), &s) == 0 ? static_cast<long>(s.st_size) : -1;
}

static bool sentinel(const std::string& p) {
  std::FILE* f = std::fopen(p.c_str(), "wb");
  if (!f) return false;
  const char text[] = "PRIVATE_AUDIT_SENTINEL";
  const bool wrote = std::fwrite(text, 1, sizeof(text) - 1, f) == sizeof(text) - 1;
  return std::fclose(f) == 0 && wrote;
}

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  const std::string prefix(argv[1]);
  const std::string control = prefix + "/control.dat";
  const std::string oom = prefix + "/oom.dat";
  const std::string writer = prefix + "/writer.dat";
  const int baseline = descriptors();
  if (baseline < 0 || !sentinel(control) || !sentinel(oom) || !sentinel(writer)) return 3;
  bool control_ok = false;
  {
    mhgp10::cli::OutputSet output;
    control_ok = output.reserve(control).ok();
  }
  const int control_fds = descriptors();
  const long control_size = size_of(control);
  bool caught_oom = false;
  {
    mhgp10::cli::OutputSet output;
    fail_one_allocation = true;
    try { (void)output.reserve(oom); }
    catch (const std::bad_alloc&) { caught_oom = true; }
    fail_one_allocation = false;
  }
  const int oom_fds = descriptors();
  const long oom_size = size_of(oom);
  bool caught_writer = false;
  {
    mhgp10::cli::OutputSet output;
    try {
      (void)output.write(writer, [](std::FILE*) { throw std::runtime_error("private writer fixture"); });
    } catch (const std::runtime_error&) { caught_writer = true; }
  }
  const int writer_fds = descriptors();
  const long writer_size = size_of(writer);
  std::printf("{\"kind\":\"outputset_exception_observation\",\"baseline_fds\":%d,"
              "\"control_ok\":%s,\"control_fds\":%d,\"control_size\":%ld,"
              "\"caught_oom\":%s,\"allocation_failures\":%u,\"oom_fds\":%d,\"oom_size\":%ld,"
              "\"caught_writer\":%s,\"writer_fds\":%d,\"writer_size\":%ld}\n",
              baseline, control_ok ? "true" : "false", control_fds, control_size,
              caught_oom ? "true" : "false", allocation_failures, oom_fds, oom_size,
              caught_writer ? "true" : "false", writer_fds, writer_size);
  // Gate observes the specified fault, it does not qualify this helper as repaired.
  return control_ok && control_fds == baseline && control_size == -1 &&
                 caught_oom && allocation_failures == 1 && oom_fds == baseline + 1 && oom_size == 0 &&
                 caught_writer && writer_fds == baseline + 2 && writer_size == -1
             ? 0 : 1;
}
