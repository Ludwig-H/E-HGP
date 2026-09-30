// Private header-only causal audit. No engine or CLI is compiled.
// GNU ld --wrap=rename fails exactly rename #2 only in the EIO scenario.
#include <dirent.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>
#include <vector>
#include "core/cli_output.hpp"
#include "core/cli_options.hpp"

namespace {
int rename_calls = 0, fail_rename_at = 0;
int fd_count() {
  DIR* d = ::opendir("/proc/self/fd");
  if (!d) return -1;
  int n = 0;
  while (const dirent* e = ::readdir(d))
    if (std::strcmp(e->d_name, ".") && std::strcmp(e->d_name, "..")) ++n;
  ::closedir(d);
  return n;
}
int temp_count(const std::string& path) {
  DIR* d = ::opendir(path.c_str());
  if (!d) return -1;
  int n = 0;
  while (const dirent* e = ::readdir(d))
    if (std::strstr(e->d_name, ".mhgp10-")) ++n;
  ::closedir(d);
  return n;
}
bool put(const std::string& path, const char* value) {
  std::FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return false;
  const bool written = std::fputs(value, f) >= 0;
  const bool closed = std::fclose(f) == 0;
  return written && closed;
}
std::string get(const std::string& path) {
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return "ABSENT";
  char buf[64];
  const size_t n = std::fread(buf, 1, sizeof buf, f);
  const bool bad = std::ferror(f) != 0;
  const bool closed = std::fclose(f) == 0;
  return bad || !closed ? "IO_ERROR" : std::string(buf, n);
}
}
extern "C" int __real_rename(const char*, const char*);
extern "C" int __wrap_rename(const char* from, const char* to) {
  ++rename_calls;
  if (rename_calls == fail_rename_at) { errno = EIO; return -1; }
  return __real_rename(from, to);
}

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  using namespace mhgp10;
  const char* names[] = {"success", "no_commit", "writer_bad_alloc", "rename2_eio", "idempotent"};
  bool all_ok = true;
  for (int mode = 0; mode < 5; ++mode) {
    std::string pattern = std::string(argv[1]) + "/fixture_" + names[mode] + "_XXXXXX";
    std::vector<char> path(pattern.begin(), pattern.end());
    path.push_back('\0');
    if (!::mkdtemp(path.data())) return 2;
    const std::string dir = path.data(), a = dir + "/a", b = dir + "/b";
    if (!put(a, "OLD_A") || !put(b, "OLD_B")) return 2;
    const int fds_before = fd_count();
    rename_calls = 0;
    fail_rename_at = mode == 3 ? 2 : 0;
    bool escaped = false, setup_ok = true, did_commit = false, second_commit_ok = false;
    Outcome result;
    {
      cli::OutputSet o;
      try {
        auto step = [&](Outcome r) {
          result = r;
          return r.ok();
        };
        setup_ok = step(o.declare(a)) && step(o.declare(b)) && step(o.reserve()) &&
                   step(o.write(a, [](std::FILE* f) { std::fputs("NEW_A", f); }));
        if (setup_ok) {
          result = o.write(b, [&](std::FILE* f) {
            std::fputs(mode == 2 ? "PARTIAL_B" : "NEW_B", f);
            if (mode == 2) throw std::bad_alloc();
          });
          if (result.ok() && mode != 1) {
            did_commit = true;
            result = o.commit();
            if (result.ok() && mode == 4) {
              result = o.commit();
              second_commit_ok = result.ok();
            }
          }
        }
      } catch (...) { escaped = true; }
    }
    const std::string va = get(a), vb = get(b);
    const int temps = temp_count(dir), fds_after = fd_count(), calls = rename_calls;
    const bool both_old = va == "OLD_A" && vb == "OLD_B";
    const bool both_new = va == "NEW_A" && vb == "NEW_B";
    const bool whole_set = (!did_commit || !result.ok()) ? both_old : both_new;
    bool expected = false;
    if (mode == 0) expected = result.ok() && calls == 2 && both_new;
    if (mode == 1) expected = result.ok() && calls == 0 && both_old && !did_commit;
    if (mode == 2) expected = result.reason == Reason::memory_budget && calls == 0 && both_old && !did_commit;
    if (mode == 3) expected = result.reason == Reason::output_unwritable && calls == 2 &&
                              va == "NEW_A" && vb == "OLD_B" && !whole_set;
    if (mode == 4) expected = result.ok() && calls == 2 && both_new && second_commit_ok;
    const bool gate_ok = setup_ok && !escaped && expected && temps == 0 &&
                         fds_before >= 0 && fds_before == fds_after;
    all_ok = all_ok && gate_ok;
    const auto status = status_name(result.status()), reason = reason_name(result.reason);
    std::printf("{\"case\":\"%s\",\"status\":\"%.*s\",\"reason\":\"%.*s\",\"a\":\"%s\",\"b\":\"%s\","
                "\"rename_calls\":%d,\"temp_count\":%d,\"fds_before\":%d,\"fds_after\":%d,"
                "\"did_commit\":%s,\"second_commit_ok\":%s,\"escaped\":%s,"
                "\"contract_all_or_nothing\":%s,\"observation_matches_expected\":%s}\n",
                names[mode], int(status.size()), status.data(), int(reason.size()), reason.data(),
                va.c_str(), vb.c_str(), calls, temps, fds_before, fds_after,
                did_commit ? "true" : "false", second_commit_ok ? "true" : "false",
                escaped ? "true" : "false", whole_set ? "true" : "false", gate_ok ? "true" : "false");
    if (::unlink(a.c_str()) != 0 || ::unlink(b.c_str()) != 0 || ::rmdir(dir.c_str()) != 0) return 2;
  }
  return all_ok ? 0 : 1;
}
