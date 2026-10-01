#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include "core/cli_output.hpp"

namespace {
bool want_injection = false, armed = false;
unsigned creations = 0, injections = 0;
int created_fd = -1;
char created_path[PATH_MAX]{};
constexpr bool mutant = MP10_EXPECT_MUTANT != 0;

int descriptors() {
  DIR* d = ::opendir("/proc/self/fd");
  if (!d) return -1000;
  int n = 0;
  while (dirent* e = ::readdir(d))
    if (std::strcmp(e->d_name, ".") && std::strcmp(e->d_name, "..")) ++n;
  ::closedir(d);
  return n;
}
int temporaries(const char* path) {
  DIR* d = ::opendir(path);
  if (!d) return -1000;
  int n = 0;
  while (dirent* e = ::readdir(d))
    if (std::strstr(e->d_name, ".mhgp10-") != nullptr) ++n;
  ::closedir(d);
  return n;
}
bool sentinel(const char* path) {
  char b[64]{};
  FILE* f = std::fopen(path, "rb");
  if (!f) return false;
  const size_t n = std::fread(b, 1, sizeof b, f);
  const bool closed = std::fclose(f) == 0;
  return closed && n == 13 && std::memcmp(b, "SENTINEL_MP10", 13) == 0;
}
}

void* operator new(std::size_t n) {
  if (armed) {
    armed = false;
    ++injections;
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

extern "C" int __real_open(const char*, int, ...);
extern "C" int __wrap_open(const char* path, int flags, ...) {
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list args;
    va_start(args, flags);
    mode = static_cast<mode_t>(va_arg(args, int));
    va_end(args);
  }
  const int fd = flags & O_CREAT ? __real_open(path, flags, mode) : __real_open(path, flags);
  if (fd >= 0 && (flags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL) &&
      std::strstr(path, ".mhgp10-") != nullptr) {
    ++creations;
    created_fd = fd;
    std::snprintf(created_path, sizeof created_path, "%s", path);
    if (want_injection) armed = true;
  }
  return fd;
}

int main(int argc, char** argv) {
  if (argc != 3 || (std::strcmp(argv[1], "plain") && std::strcmp(argv[1], "inject"))) return 2;
  want_injection = std::strcmp(argv[1], "inject") == 0;
  char destination[PATH_MAX]{};
  if (std::snprintf(destination, sizeof destination, "%s/destination_preexistante_sentinelle.txt", argv[2]) >= PATH_MAX)
    return 2;
  FILE* f = std::fopen(destination, "wb");
  if (!f || std::fwrite("SENTINEL_MP10", 1, 13, f) != 13 || std::fclose(f) != 0) return 2;
  const int before_fd = descriptors(), before_temp = temporaries(argv[2]);
  bool returned = false, outer_exception = false, declaration_ok = false;
  mhgp10::Outcome result = mhgp10::fail(mhgp10::Reason::output_unwritable);
  {
    mhgp10::cli::OutputSet output;
    declaration_ok = output.declare(destination).ok();
    if (declaration_ok) {
      try {
        result = output.reserve();
        returned = true;
      } catch (...) {
        outer_exception = true;
      }
    }
  }  // destructor must run before observing the resources
  const bool pending = armed;
  armed = false;  // inspections/reporting may never consume the pending test injection
  const int fd_delta = descriptors() - before_fd;
  const int temp_delta = temporaries(argv[2]) - before_temp;
  const bool fd_live = created_fd >= 0 && ::fcntl(created_fd, F_GETFD) != -1;
  struct stat st{};
  const bool temp_live = created_path[0] != '\0' && ::lstat(created_path, &st) == 0;
  const bool destination_intact = sentinel(destination);
  // Explicitly clean only the fd/path whose successful creation the wrapper observed.
  const bool clean_fd = !fd_live || ::close(created_fd) == 0;
  const bool clean_temp = !temp_live || ::unlink(created_path) == 0;
  const int post_fd_delta = descriptors() - before_fd;
  const int post_temp_delta = temporaries(argv[2]) - before_temp;
  const bool leak_expected = mutant && want_injection;
  const bool reason_ok = returned && result.reason == (leak_expected ? mhgp10::Reason::memory_budget : mhgp10::Reason::none);
  const bool ok = declaration_ok && before_fd >= 0 && before_temp == 0 && returned && !outer_exception && reason_ok &&
                  creations == 1 && injections == unsigned(leak_expected) && pending == (want_injection && !mutant) &&
                  fd_delta == int(leak_expected) && temp_delta == int(leak_expected) &&
                  fd_live == leak_expected && temp_live == leak_expected && destination_intact &&
                  clean_fd && clean_temp && post_fd_delta == 0 && post_temp_delta == 0;
  const auto reason = mhgp10::reason_name(result.reason);
  std::printf("{\"variant\":\"%s\",\"case\":\"%s\",\"declaration_ok\":%d,\"reserve_returned\":%d,"
              "\"outer_exception\":%d,\"reason\":\"%.*s\",\"creations\":%u,\"injections\":%u,"
              "\"pending_before_disarm\":%d,\"fd_delta\":%d,\"temp_delta\":%d,\"created_fd_live\":%d,"
              "\"created_temp_live\":%d,\"destination_intact\":%d,\"cleanup_fd_ok\":%d,\"cleanup_temp_ok\":%d,"
              "\"post_cleanup_fd_delta\":%d,\"post_cleanup_temp_delta\":%d,\"expected_observation\":%d}\n",
              mutant ? "mutant" : "baseline", argv[1], declaration_ok, returned, outer_exception,
              int(reason.size()), reason.data(), creations, injections, pending, fd_delta, temp_delta, fd_live,
              temp_live, destination_intact, clean_fd, clean_temp, post_fd_delta, post_temp_delta, ok);
  return ok ? 0 : 1;
}
