// Persistent C++ allocation failure is injected below the archive API, not by
// a product mutant. Compile with --wrap=openat,--wrap=fsync. The header override
// permits qualification of an isolated patch while the product stays frozen.
#include <atomic>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef MHGP7_ARCHIVE_HEADER
#define MHGP7_ARCHIVE_HEADER "../src/io/archive.hpp"
#endif
#include MHGP7_ARCHIVE_HEADER
#include "../src/pipeline/run.hpp"

using namespace mhgp7;

namespace allocation_fault {
std::atomic<bool> persistent{false}, counting{false};
std::atomic<long long> countdown{-1};
std::atomic<size_t> allocations{0}, denied{0};
void before() {
  if (counting.load()) allocations.fetch_add(1);
  const long long left = countdown.load();
  if (left >= 0 && countdown.fetch_sub(1) == 0) persistent.store(true);
  if (persistent.load()) { denied.fetch_add(1); throw std::bad_alloc(); }
}
void reset() noexcept {
  persistent.store(false);
  counting.store(false);
  countdown.store(-1);
}
[[gnu::noinline]] void* allocate(size_t size) {
  before();
  if (void* p = std::malloc(size == 0 ? 1 : size)) return p;
  throw std::bad_alloc();
}
[[gnu::noinline]] void* aligned(size_t size, size_t alignment) {
  before();
  void* p = nullptr;
  if (::posix_memalign(&p, alignment, size == 0 ? 1 : size) == 0) return p;
  throw std::bad_alloc();
}
[[gnu::noinline]] void release(void* p) noexcept { std::free(p); }
}  // namespace allocation_fault

void* operator new(size_t size) { return allocation_fault::allocate(size); }
void* operator new[](size_t size) { return allocation_fault::allocate(size); }
void* operator new(size_t size, std::align_val_t alignment) {
  return allocation_fault::aligned(size, static_cast<size_t>(alignment));
}
void* operator new[](size_t size, std::align_val_t alignment) {
  return allocation_fault::aligned(size, static_cast<size_t>(alignment));
}
void* operator new(size_t size, const std::nothrow_t&) noexcept {
  try { return allocation_fault::allocate(size); } catch (...) { return nullptr; }
}
void* operator new[](size_t size, const std::nothrow_t&) noexcept {
  try { return allocation_fault::allocate(size); } catch (...) { return nullptr; }
}
void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
  try { return allocation_fault::aligned(size, static_cast<size_t>(alignment)); } catch (...) { return nullptr; }
}
void* operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
  try { return allocation_fault::aligned(size, static_cast<size_t>(alignment)); } catch (...) { return nullptr; }
}
void operator delete(void* p) noexcept { allocation_fault::release(p); }
void operator delete[](void* p) noexcept { allocation_fault::release(p); }
void operator delete(void* p, size_t) noexcept { allocation_fault::release(p); }
void operator delete[](void* p, size_t) noexcept { allocation_fault::release(p); }
void operator delete(void* p, std::align_val_t) noexcept { allocation_fault::release(p); }
void operator delete[](void* p, std::align_val_t) noexcept { allocation_fault::release(p); }
void operator delete(void* p, size_t, std::align_val_t) noexcept { allocation_fault::release(p); }
void operator delete[](void* p, size_t, std::align_val_t) noexcept { allocation_fault::release(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { allocation_fault::release(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { allocation_fault::release(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { allocation_fault::release(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { allocation_fault::release(p); }

namespace {
bool fail_staging_open = false, fail_parent_sync = false;
unsigned staging_open_faults = 0, directory_syncs = 0, callback_mask = 0;
size_t constructor_faults = 0, commit_faults = 0;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

struct Temporary {
  std::filesystem::path path;
  Temporary() {
    char name[] = "/tmp/mhgp7-archive-cleanup-XXXXXX";
    if (!::mkdtemp(name)) throw std::runtime_error("temporary directory unavailable");
    path = name;
  }
  ~Temporary() {
    allocation_fault::reset();
    std::error_code ec;
    std::filesystem::remove_all(path, ec);  // Test-owned root; never fault-injected.
  }
};

void empty(const Temporary& temp) {
  require(std::filesystem::is_empty(temp.path), "archive or provisional directory leaked");
}

std::filesystem::path provisional(const Temporary& temp) {
  std::filesystem::path found;
  for (const auto& entry : std::filesystem::directory_iterator(temp.path)) {
    require(found.empty(), "multiple provisional directories");
    found = entry.path();
    require(found.filename().string().starts_with(".mhgp7-provisional-"), "unexpected visible archive");
  }
  require(!found.empty(), "provisional directory absent before injection");
  return found;
}

ForestResult small_forest() {
  ForestResult r;
  for (PointId id : {0u, 1u}) {
    FacetKey key;
    key.k = 1;
    key.p[0] = id;
    r.facet_keys.push_back(key);
  }
  r.final_canon_fid = {0, 1};
  r.facets = 2;
  return r;
}

void confirm_persistent_failure() {
  const size_t before = allocation_fault::denied.load();
  allocation_fault::persistent.store(true);
  bool caught = false;
  try {
    void* p = ::operator new(1);
    ::operator delete(p);
  } catch (const std::bad_alloc&) { caught = true; }
  if (!caught || allocation_fault::denied.load() != before + 1) std::_Exit(96);
}

void persistent_destructor(Temporary& temp, const std::vector<InputPoint>& points) {
  const auto destination = temp.path / "persistent";
  {
    ForestArchive archive(destination);
    archive.input(points);
    confirm_persistent_failure();
  }  // The failure remains enabled for every destructor allocation attempt.
  allocation_fault::reset();
  empty(temp);
}

void complete_suffix_cleanup(Temporary& temp, const std::vector<InputPoint>& points, const ForestResult& forest) {
  const auto destination = temp.path / "all_orders";
  {
    ForestArchive archive(destination);
    archive.input(points);
    for (u64 k = 1; k <= 10; ++k) archive.forest(k, forest);
    const auto staging = provisional(temp);
    require(std::distance(std::filesystem::directory_iterator(staging), std::filesystem::directory_iterator{}) == 11,
            "all ten order files not materialized before cleanup");
    confirm_persistent_failure();
  }
  allocation_fault::reset();
  empty(temp);
}

void constructor_sweep(Temporary& temp) {
  const auto destination = temp.path / "constructor-long-name-to-force-allocation";
  allocation_fault::allocations.store(0);
  allocation_fault::counting.store(true);
  { ForestArchive archive(destination); }
  allocation_fault::reset();
  const size_t count = allocation_fault::allocations.load();
  require(count > 0, "constructor allocation sweep vacuous");
  for (size_t i = 0; i < count; ++i) {
    bool caught = false;
    allocation_fault::countdown.store(static_cast<long long>(i));
    try { ForestArchive archive(destination); }
    catch (const std::bad_alloc&) { caught = true; }
    allocation_fault::reset();
    require(caught, "constructor allocation failure not reached");
    ++constructor_faults;
    empty(temp);
  }
  // The constructor must also unwind safely after mkdir, before its retained
  // staging fd opens, even when constructing the OS error itself cannot allocate.
  fail_staging_open = true;
  bool caught = false;
  try { ForestArchive archive(destination); }
  catch (const std::bad_alloc&) { caught = true; }
  allocation_fault::reset();
  fail_staging_open = false;
  require(caught && staging_open_faults == 1, "post-mkdir constructor fault not reached");
  empty(temp);
}

void commit_sweep(Temporary& temp, const std::vector<InputPoint>& points, const ForestResult& forest) {
  const auto destination = temp.path / "commit";
  const std::string digest(64, 'a');
  {
    ForestArchive archive(destination);
    archive.input(points);
    archive.forest(1, forest);
    allocation_fault::allocations.store(0);
    allocation_fault::counting.store(true);
    archive.commit(1, "verified_events_only", digest);
    allocation_fault::reset();
  }
  const size_t count = allocation_fault::allocations.load();
  require(count > 0 && std::filesystem::exists(destination / "manifest.json"), "commit sweep vacuous");
  std::filesystem::remove_all(destination);
  for (size_t i = 0; i < count; ++i) {
    bool caught = false;
    {
      ForestArchive archive(destination);
      archive.input(points);
      archive.forest(1, forest);
      allocation_fault::countdown.store(static_cast<long long>(i));
      try { archive.commit(1, "verified_events_only", digest); }
      catch (const std::bad_alloc&) { caught = true; }
    }
    allocation_fault::reset();
    require(caught, "commit allocation failure not reached");
    ++commit_faults;
    empty(temp);
  }
  // fsync(parent) is after the publication point. Its failure and a persistent
  // allocation failure must not turn a visible completed archive into an abort.
  directory_syncs = 0;
  bool confirmed = true;
  {
    ForestArchive archive(destination);
    archive.input(points);
    archive.forest(1, forest);
    fail_parent_sync = true;
    archive.commit(1, "verified_events_only", digest);
    confirmed = archive.parent_sync_confirmed();
  }
  allocation_fault::reset();
  fail_parent_sync = false;
  require(!confirmed && directory_syncs == 2, "post-publication sync failure not reached");
  require(std::filesystem::exists(destination / "manifest.json"), "published archive was removed");
  require(std::distance(std::filesystem::directory_iterator(temp.path), std::filesystem::directory_iterator{}) == 1,
          "post-publication provisional directory leaked");
  std::filesystem::remove_all(destination);
  empty(temp);
}

void callback_failure(Temporary& temp) {
  const std::vector<InputPoint> input{{0, {0, 0, 7}}, {1, {0, 9, 6}}, {2, {1, 4, 0}},
                                      {3, {0, 0, 1}}, {4, {4, 1, 2}}};
  const auto destination = temp.path / "callback";
  RunResult result;
  {
    ForestArchive archive(destination);
    archive.input(input);
    RunOptions options;
    options.complete_silent_incidence = true;
    options.digest = true;
    options.fold_join_before_next_k = true;
    options.fold_inflight = 1;
    options.on_forest = [&](u64 k, const auto&, const ForestResult& forest) {
      archive.forest(k, forest);
      callback_mask |= 1u << k;
      if (k == 2) {
        confirm_persistent_failure();
        throw std::bad_alloc();
      }
    };
    result = run_pipeline(input, options);
  }
  allocation_fault::reset();
  require(callback_mask == ((1u << 1) | (1u << 2)), "K1/K2 callback prefix not reached exactly");
  require(result.status == PipelineStatus::kResourceExhausted && result.stage_reached == kRunStageFold,
          "late allocation refusal status/stage");
  require(result.digest_all.empty() && result.digest_forest.empty() && result.total_events == 0,
          "refused pipeline retained provisional payload");
  empty(temp);
}

void os_cleanup_failure(Temporary& temp, const std::vector<InputPoint>& points) {
  const auto destination = temp.path / "os_failure";
  int pipe_fds[2];
  require(::pipe2(pipe_fds, O_CLOEXEC) == 0, "diagnostic pipe failed");
  const int saved_stderr = ::dup(STDERR_FILENO);
  require(saved_stderr >= 0, "stderr duplication failed");
  std::filesystem::path staging, blocked;
  {
    ForestArchive archive(destination);
    archive.input(points);
    staging = provisional(temp);
    blocked = staging / "input.u16";
    require(::unlink(blocked.c_str()) == 0 && ::mkdir(blocked.c_str(), 0700) == 0, "OS unlink fault setup failed");
    require(::dup2(pipe_fds[1], STDERR_FILENO) >= 0, "diagnostic capture failed");
    ::close(pipe_fds[1]);
    confirm_persistent_failure();
  }
  allocation_fault::reset();
  require(::dup2(saved_stderr, STDERR_FILENO) >= 0, "stderr restoration failed");
  ::close(saved_stderr);
  char message[256]{};
  const ssize_t length = ::read(pipe_fds[0], message, sizeof(message));
  ::close(pipe_fds[0]);
  require(length > 0 && length < 192 && message[length - 1] == '\n', "OS cleanup diagnostic absent/unbounded");
  require(std::strstr(message, "archive_cleanup=failed provisional=.mhgp7-provisional-") == message &&
          std::strstr(message, " operations_failed=2 first_errno=") != nullptr,
          "OS cleanup diagnostic missing exact failed operations");
  require(!std::filesystem::exists(destination) && std::filesystem::is_directory(blocked),
          "OS cleanup failure was silently misrepresented");
  require(::rmdir(blocked.c_str()) == 0 && ::rmdir(staging.c_str()) == 0, "fault fixture cleanup failed");
  empty(temp);
}
}  // namespace

extern "C" int __real_openat(int dirfd, const char* path, int flags, ...);
extern "C" int __wrap_openat(int dirfd, const char* path, int flags, ...) {
  mode_t mode = 0;
  if ((flags & O_CREAT) != 0) {
    va_list args;
    va_start(args, flags);
    mode = static_cast<mode_t>(va_arg(args, int));
    va_end(args);
  }
  if (fail_staging_open && (flags & O_DIRECTORY) != 0 && std::strncmp(path, ".mhgp7-provisional-", 19) == 0) {
    ++staging_open_faults;
    allocation_fault::persistent.store(true);
    errno = EMFILE;
    return -1;
  }
  return __real_openat(dirfd, path, flags, mode);
}

extern "C" int __real_fsync(int fd);
extern "C" int __wrap_fsync(int fd) {
  struct stat info{};
  if (::fstat(fd, &info) != 0) return -1;
  if (S_ISDIR(info.st_mode) && ++directory_syncs == 2 && fail_parent_sync) {
    allocation_fault::persistent.store(true);
    errno = EIO;
    return -1;
  }
  return __real_fsync(fd);
}

int main() {
  std::set_terminate([] { std::_Exit(97); });
  try {
    Temporary temp;
    const std::vector<InputPoint> input{{0, {1, 2, 3}}, {1, {4, 5, 6}}};
    const ForestResult forest = small_forest();
    persistent_destructor(temp, input);
    complete_suffix_cleanup(temp, input, forest);
    constructor_sweep(temp);
    commit_sweep(temp, input, forest);
    callback_failure(temp);
    os_cleanup_failure(temp, input);
    require(allocation_fault::denied.load() >= constructor_faults + commit_faults + 4,
            "persistent allocation refusal witness floor");
    std::printf("archive_cleanup_gate=passed persistent=1 all_order_files=10 constructor_faults=%zu post_mkdir_faults=%u commit_faults=%zu "
                "callback_mask=%u parent_sync_fault=1 os_cleanup_fault=1 denied_allocations=%zu\n",
                constructor_faults, staging_open_faults, commit_faults, callback_mask, allocation_fault::denied.load());
    return 0;
  } catch (const std::exception& error) {
    allocation_fault::reset();
    std::fprintf(stderr, "archive cleanup gate: %s\n", error.what());
    return 1;
  }
}
