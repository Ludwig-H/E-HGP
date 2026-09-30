// Header-only allocation-boundary audit. No HGP/CLI engine is linked.
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include "cloud/u32le_input.hpp"
#include "core/cli_options.hpp"

namespace {
bool armed = false;
bool hit = false;
size_t refused_size = 0;

int descriptor_count() {
  DIR* d = ::opendir("/proc/self/fd");
  if (!d) return -1;
  int n = 0;
  while (const dirent* e = ::readdir(d))
    if (std::strcmp(e->d_name, ".") && std::strcmp(e->d_name, "..")) ++n;
  if (::closedir(d) != 0) return -1;
  return n;
}

int directory_count(const char* path) {
  DIR* d = ::opendir(path);
  if (!d) return -1;
  int n = 0;
  while (const dirent* e = ::readdir(d))
    if (std::strcmp(e->d_name, ".") && std::strcmp(e->d_name, "..")) ++n;
  if (::closedir(d) != 0) return -1;
  return n;
}

bool write_new(const char* path, const void* data, size_t bytes) {
  const int fd = ::open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
  if (fd < 0) return false;
  FILE* f = ::fdopen(fd, "wb");
  if (!f) { ::close(fd); return false; }
  const bool wrote = std::fwrite(data, 1, bytes, f) == bytes;
  const bool closed = std::fclose(f) == 0;
  return wrote && closed;
}

bool same_file(const char* path, const void* expected, size_t size) {
  unsigned char data[256];
  if (size >= sizeof data) return false;
  FILE* f = std::fopen(path, "rb");
  if (!f) return false;
  const size_t got = std::fread(data, 1, sizeof data, f);
  const bool clean = std::ferror(f) == 0 && std::feof(f);
  const bool closed = std::fclose(f) == 0;
  return clean && closed && got == size && std::memcmp(data, expected, size) == 0;
}

struct Answer { bool value_ok; mhgp10::Outcome outcome; };

template<class F>
bool run_case(const char* name, bool inject, F&& body, const char* dir,
              const char* cloud, const char* sentinel, const mhgp10::u32* point,
              const char* precious, size_t precious_size) {
  const int fds_before = descriptor_count();
  const int files_before = directory_count(dir);
  const bool unchanged_before = same_file(cloud, point, 12) && same_file(sentinel, precious, precious_size);
  bool escaped = false, unknown = false, returned = false, value_ok = false;
  mhgp10::Outcome outcome;
  hit = false;
  refused_size = 0;
  armed = inject;
  try {
    const Answer answer = body();
    returned = true;
    value_ok = answer.value_ok;
    outcome = answer.outcome;
  } catch (const std::bad_alloc&) {
    escaped = true;
  } catch (...) {
    unknown = true;
  }
  armed = false;
  const int fds_after = descriptor_count();
  const int files_after = directory_count(dir);
  const bool unchanged_after = same_file(cloud, point, 12) && same_file(sentinel, precious, precious_size);
  const auto status = mhgp10::status_name(outcome.status());
  const auto reason = mhgp10::reason_name(outcome.reason);
  std::printf("{\"case\":\"%s\",\"inject\":%s,\"fault_hit\":%s,\"refused_size\":%zu,"
              "\"returned\":%s,\"bad_alloc_escaped\":%s,\"other_escaped\":%s,"
              "\"value_ok\":%s,\"status\":\"%.*s\",\"reason\":\"%.*s\","
              "\"fds_before\":%d,\"fds_after\":%d,\"files_before\":%d,\"files_after\":%d,"
              "\"fixtures_before\":%s,\"fixtures_after\":%s}\n",
              name, inject ? "true" : "false", hit ? "true" : "false", refused_size,
              returned ? "true" : "false", escaped ? "true" : "false", unknown ? "true" : "false",
              value_ok ? "true" : "false", int(status.size()), status.data(), int(reason.size()), reason.data(),
              fds_before, fds_after, files_before, files_after,
              unchanged_before ? "true" : "false", unchanged_after ? "true" : "false");
  const bool resources = fds_before >= 0 && fds_before == fds_after && files_before == 2 && files_after == 2 &&
                         unchanged_before && unchanged_after;
  // A successful audit exit means the four specifically expected defects were reproduced.
  return resources && !unknown && (inject ? hit && refused_size > 0 && escaped && !returned && !value_ok
                                          : !hit && !escaped && returned && value_ok && outcome.ok());
}
} // namespace

void* operator new(size_t bytes) {
  if (armed) {
    armed = false;
    hit = true;
    refused_size = bytes;
    throw std::bad_alloc();
  }
  if (void* p = std::malloc(bytes ? bytes : 1)) return p;
  throw std::bad_alloc();
}
void* operator new[](size_t bytes) { return ::operator new(bytes); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, size_t) noexcept { std::free(p); }
void operator delete[](void* p, size_t) noexcept { std::free(p); }

int main(int argc, char** argv) {
  if (argc != 2) return 3;
  char cloud[4096], sentinel[4096];
  const int nc = std::snprintf(cloud, sizeof cloud, "%s/cloud-with-long-name-for-filesystem.u32le", argv[1]);
  const int ns = std::snprintf(sentinel, sizeof sentinel, "%s/sentinel", argv[1]);
  if (nc <= 0 || nc >= int(sizeof cloud) || ns <= 0 || ns >= int(sizeof sentinel)) return 3;
  const mhgp10::u32 point[3] = {1, 2, 3};
  const char precious[] = "unchanged sentinel payload\n";
  if (!write_new(cloud, point, sizeof point) || !write_new(sentinel, precious, sizeof precious - 1)) return 3;
  auto reader = [&]() {
    const auto r = mhgp10::read_u32le_cloud(cloud);
    const bool valid = r.ok() && r.value().x.size() == 1 && r.value().y.size() == 1 &&
                       r.value().z.size() == 1 && r.value().pid.size() == 1 &&
                       r.value().x[0] == 1 && r.value().y[0] == 2 && r.value().z[0] == 3 && r.value().pid[0] == 0;
    return Answer{valid, r.outcome()};
  };
  auto real = []() {
    const auto r = mhgp10::cli::parse_real("1.00000000000000000000", 0.0, 2.0);
    return Answer{r.ok() && r.value() == 1.0, r.outcome()};
  };
  auto list = []() {
    const auto r = mhgp10::cli::parse_integer_list<int>("1,2,3", 0, 10);
    return Answer{r.ok() && r.value().size() == 3 && r.value()[0] == 1 &&
                          r.value()[1] == 2 && r.value()[2] == 3, r.outcome()};
  };
  auto configs = []() {
    const auto r = mhgp10::cli::parse_head_configs("5 1.0 eom 0\n");
    return Answer{r.ok() && r.value().size() == 1 && r.value()[0].min_cluster_size == 5 &&
                  r.value()[0].z == 1.0 && !r.value()[0].leaf && !r.value()[0].allow_single, r.outcome()};
  };
  bool ok = true;
  for (bool inject : {false, true}) {
    ok = run_case("reader", inject, reader, argv[1], cloud, sentinel, point, precious, sizeof precious - 1) && ok;
    ok = run_case("real", inject, real, argv[1], cloud, sentinel, point, precious, sizeof precious - 1) && ok;
    ok = run_case("list", inject, list, argv[1], cloud, sentinel, point, precious, sizeof precious - 1) && ok;
    ok = run_case("configs", inject, configs, argv[1], cloud, sentinel, point, precious, sizeof precious - 1) && ok;
  }
  return std::fflush(stdout) == 0 && std::ferror(stdout) == 0 ? (ok ? 0 : 1) : 3;
}
