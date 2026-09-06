// Explicit u16 input and a create-only, transactional Linux file export.
// An archive is committed only after the terminal pipeline status succeeds.
// No checkpoint/restart or power-loss guarantee is implied by this format.
#pragma once

#include <array>
#include <charconv>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "../core/caps.hpp"
#include "../core/sha256.hpp"
#include "../forest/fold.hpp"

namespace mhgp7 {

inline std::vector<InputPoint> read_u16_text(const std::string& path) {
  const auto close_file = [](FILE* file) { std::fclose(file); };
  std::unique_ptr<FILE, decltype(close_file)> f(std::fopen(path.c_str(), "rb"), close_file);
  if (!f) throw std::runtime_error("input_open_failed");
  std::vector<InputPoint> points;
  std::array<char, 256> line{};
  size_t line_number = 0;
  for (;;) {
    size_t length = 0;
    int c = 0;
    while ((c = std::fgetc(f.get())) != EOF) {
      if (length == line.size()) throw std::runtime_error("input_line_too_long:" + std::to_string(line_number + 1));
      if (c == 0) throw std::runtime_error("input_nul_byte");
      line[length++] = static_cast<char>(c);
      if (c == '\n') break;
    }
    if (length == 0 && c == EOF) break;
    ++line_number;
    const char* p = line.data();
    const char* end = p + length;
    const auto space = [&]() { while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) ++p; };
    space();
    if (p == end || *p == '#') continue;
    u64 values[4]{};
    for (int i = 0; i < 4; ++i) {
      const auto parsed = std::from_chars(p, end, values[i]);
      if (parsed.ec != std::errc{} || parsed.ptr == p ||
          (parsed.ptr != end && *parsed.ptr != ' ' && *parsed.ptr != '\t' && *parsed.ptr != '\r' && *parsed.ptr != '\n'))
        throw std::runtime_error("input_invalid_integer:" + std::to_string(line_number));
      p = parsed.ptr;
      space();
    }
    if (p != end || values[0] > UINT32_MAX || values[1] > 65535 || values[2] > 65535 || values[3] > 65535)
      throw std::runtime_error("input_outside_u16_contract:" + std::to_string(line_number));
    if (points.size() >= kMaxTreePositions) throw std::runtime_error("input_point_limit");
    points.push_back({static_cast<PointId>(values[0]),
                      {static_cast<i64>(values[1]), static_cast<i64>(values[2]), static_cast<i64>(values[3])}});
  }
  if (std::ferror(f.get())) throw std::runtime_error("input_read_failed");
  if (points.size() < 2) throw std::runtime_error("input_requires_two_points");
  return points;
}

namespace archive_detail {

// Own the namespace before any temporary-directory mutation. Member teardown
// runs even if ForestArchive's constructor throws. Cleanup uses only retained
// descriptors, fixed filenames and stack storage, including its diagnostic:
// std::filesystem::remove_all(error_code) can allocate and is not suitable in
// a noexcept destructor during a persistent std::bad_alloc.
class StagingDirectory {
 public:
  StagingDirectory() = default;
  StagingDirectory(const StagingDirectory&) = delete;
  StagingDirectory& operator=(const StagingDirectory&) = delete;
  ~StagingDirectory() noexcept {
    if (name_[0] != '\0' && !published_) cleanup();
    if (staging_fd_ >= 0) ::close(staging_fd_);
    if (parent_fd_ >= 0) ::close(parent_fd_);
  }
  void create(const std::filesystem::path& parent, const char* destination) {
    parent_fd_ = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (parent_fd_ < 0) throw std::runtime_error("archive_open_directory_failed");
    struct stat info{};
    if (::fstatat(parent_fd_, destination, &info, AT_SYMLINK_NOFOLLOW) == 0)
      throw std::runtime_error("archive_destination_exists");
    if (errno != ENOENT) throw std::runtime_error("archive_destination_check_failed");
    // Linux /proc binds mkdtemp to the already-open parent, even if its path
    // changes. Everything after mkdir succeeds is non-allocating until the
    // directory is owned by this member (also on the openat failure path).
    char pattern[96] = "/proc/self/fd/";
    char* end = std::to_chars(pattern + 14, pattern + 35, parent_fd_).ptr;
    constexpr char suffix[] = "/.mhgp7-provisional-XXXXXX";
    std::memcpy(end, suffix, sizeof(suffix));
    char* created = ::mkdtemp(pattern);
    if (!created) throw std::runtime_error("archive_create_directory_failed");
    std::memcpy(name_.data(), end + 1, sizeof(suffix) - 1);
    staging_fd_ = ::openat(parent_fd_, name_.data(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (staging_fd_ < 0) throw std::runtime_error("archive_open_directory_failed");
  }
  int fd() const noexcept { return staging_fd_; }
  bool open() const noexcept { return name_[0] != '\0' && !published_; }
  void sync() {
    if (::fsync(staging_fd_) != 0) throw std::runtime_error("archive_sync_directory_failed");
  }
  bool publish(const char* destination) {
    if (::syscall(SYS_renameat2, parent_fd_, name_.data(), parent_fd_, destination, 1U) != 0)
      throw std::runtime_error("archive_commit_create_only_failed");
    // Publication has happened. No allocation or throwing operation follows.
    published_ = true;
    return ::fsync(parent_fd_) == 0;
  }
 private:
  void cleanup() noexcept {
    unsigned errors = 0;
    int first_errno = 0;
    const auto failure = [&]() noexcept {
      if (errors++ == 0) first_errno = errno;
    };
    if (staging_fd_ >= 0) {
      static constexpr const char* files[] = {
          "input.u16", "forest_K1.bin", "forest_K2.bin", "forest_K3.bin",
          "forest_K4.bin", "forest_K5.bin", "forest_K6.bin", "forest_K7.bin",
          "forest_K8.bin", "forest_K9.bin", "forest_K10.bin", "manifest.json"};
      for (const char* file : files)
        if (::unlinkat(staging_fd_, file, 0) != 0 && errno != ENOENT) failure();
      // Do not remove a different directory substituted at the temporary name.
      struct stat owned{}, named{};
      if (::fstat(staging_fd_, &owned) != 0) failure();
      else if (::fstatat(parent_fd_, name_.data(), &named, AT_SYMLINK_NOFOLLOW) != 0) {
        if (errno != ENOENT) failure();
      } else if (owned.st_dev != named.st_dev || owned.st_ino != named.st_ino) {
        errno = ESTALE;
        failure();
      } else if (::unlinkat(parent_fd_, name_.data(), AT_REMOVEDIR) != 0 && errno != ENOENT) failure();
    } else if (::unlinkat(parent_fd_, name_.data(), AT_REMOVEDIR) != 0 && errno != ENOENT) failure();
    if (errors != 0) warn(errors, first_errno);
  }
  void warn(unsigned errors, int first_errno) const noexcept {
    char message[192];
    char* out = message;
    const auto append = [&](const char* value) noexcept {
      while (*value != '\0' && out < message + sizeof(message) - 1) *out++ = *value++;
    };
    const auto number = [&](int value) noexcept {
      char digits[32]{};
      const auto result = std::to_chars(digits, digits + sizeof(digits) - 1, value);
      if (result.ec == std::errc{}) append(digits);
      else append("?");
    };
    append("archive_cleanup=failed provisional=");
    append(name_.data());
    append(" operations_failed=");
    number(static_cast<int>(errors));
    append(" first_errno=");
    number(first_errno);
    *out++ = '\n';
    const char* cursor = message;
    // Bounded best effort, including EINTR/partial writes. No recursive cleanup
    // or allocation retry can mask an OS refusal or prevent stack unwinding.
    for (unsigned attempt = 0; cursor < out && attempt < 3; ++attempt) {
      const ssize_t written = ::write(STDERR_FILENO, cursor, static_cast<size_t>(out - cursor));
      if (written > 0) cursor += written;
      else if (written < 0 && errno != EINTR) break;
    }
  }
  int parent_fd_ = -1, staging_fd_ = -1;
  std::array<char, 32> name_{};
  bool published_ = false;
};

class File {
 public:
  File(int directory_fd, const char* name) {
    const int fd = ::openat(directory_fd, name, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) throw std::runtime_error("archive_create_file_failed");
    f_ = ::fdopen(fd, "wb");
    if (!f_) { ::close(fd); throw std::runtime_error("archive_fdopen_failed"); }
  }
  File(const File&) = delete;
  File& operator=(const File&) = delete;
  ~File() { if (f_) std::fclose(f_); }
  void put(const void* p, size_t size) {
    if (std::fwrite(p, 1, size, f_) != size) throw std::runtime_error("archive_write_failed");
    hash_.update(p, size);
    bytes_ += size;
  }
  void text(std::string_view value) { put(value.data(), value.size()); }
  void uint(u64 value, size_t bytes) {
    u8 b[8];
    for (size_t i = 0; i < bytes; ++i) b[i] = static_cast<u8>(value >> (8 * i));
    put(b, bytes);
  }
  void facet(const FacetKey& key) {
    uint(key.k, 1);
    for (PointId id : key.p) uint(id, 4);
  }
  void level(const ExactLevel& value) {
    for (u64 limb : value.num) uint(limb, 8);
    uint(static_cast<u64>(value.den), 8);
    uint(static_cast<u64>(static_cast<u128>(value.den) >> 64), 8);
  }
  std::string finish() {
    if (std::fflush(f_) != 0 || ::fsync(::fileno(f_)) != 0) throw std::runtime_error("archive_sync_file_failed");
    FILE* closing = f_;
    f_ = nullptr;
    if (std::fclose(closing) != 0) throw std::runtime_error("archive_close_file_failed");
    return hash_.hex();
  }
  u64 bytes() const { return bytes_; }
 private:
  FILE* f_ = nullptr;
  Sha256 hash_;
  u64 bytes_ = 0;
};

}  // namespace archive_detail

class ForestArchive {
 public:
  explicit ForestArchive(const std::filesystem::path& destination) {
    if (destination.empty() || destination.filename().empty() || destination.filename() == "." || destination.filename() == "..")
      throw std::runtime_error("archive_invalid_destination");
    auto parent = destination.parent_path();
    if (parent.empty()) parent = ".";
    destination_name_ = destination.filename().string();
    directory_.create(parent, destination_name_.c_str());
  }
  ForestArchive(const ForestArchive&) = delete;
  ForestArchive& operator=(const ForestArchive&) = delete;
  ~ForestArchive() = default;
  void input(const std::vector<InputPoint>& points) {
    require_open();
    WriteGuard guard{failed_};
    if (points.empty() || points.size() > kMaxTreePositions)
      throw std::runtime_error("archive_input_point_count");
    archive_detail::File f(directory_.fd(), "input.u16");
    f.text("mhgp7-input-u16-v1\n");
    f.uint(points.size(), 8);
    for (const auto& point : points) {
      if (!p3_in_profile(point.position)) throw std::runtime_error("archive_input_outside_u16");
      f.uint(point.id, 4);
      f.uint(static_cast<u64>(point.position.x), 2);
      f.uint(static_cast<u64>(point.position.y), 2);
      f.uint(static_cast<u64>(point.position.z), 2);
    }
    input_hash_ = f.finish();
    input_count_ = points.size();
    entries_.push_back({"input.u16", input_hash_, f.bytes()});
    guard.done = true;
  }
  void forest(u64 k, const ForestResult& r) {
    require_open();
    WriteGuard guard{failed_};
    if (k == 0 || k > 10 || k != next_k_ || !r.refusal.empty())
      throw std::runtime_error("archive_forest_order_or_refusal");
    if (k == 1) normalized_reduced_ = r.normalized_reduced;
    else if (normalized_reduced_ != r.normalized_reduced)
      throw std::runtime_error("archive_mixed_forest_semantics");
    const std::string name = "forest_K" + std::to_string(k) + ".bin";
    archive_detail::File f(directory_.fd(), name.c_str());
    f.text("mhgp7-forest-file-v1\n");
    f.uint(k, 4);
    f.uint(r.facet_keys.size(), 8);
    for (const auto& key : r.facet_keys) f.facet(key);
    f.uint(r.final_canon_fid.size(), 8);
    for (u32 fid : r.final_canon_fid) f.uint(fid, 4);
    f.uint(r.delta_count(), 8);
    r.for_each_delta([&](const ComponentDeltaView& delta) {
      f.uint(delta.batch, 8);
      f.level(delta.level);
      f.facet(delta.output);
      f.uint(delta.parents.size(), 8);
      for (const auto& key : delta.parents) f.facet(key);
      f.uint(delta.born.size(), 8);
      for (const auto& key : delta.born) f.facet(key);
    });
    entries_.push_back({name, f.finish(), f.bytes()});
    ++next_k_;
    guard.done = true;
  }
  void commit(u64 kmax, std::string_view semantics, std::string_view digest_all) {
    require_open();
    WriteGuard guard{failed_};
    if (kmax == 0 || kmax > 10 || next_k_ != kmax + 1 || input_hash_.empty())
      throw std::runtime_error("archive_incomplete");
    // All variable string values below are closed vocabulary or SHA-256 hex.
    if (semantics != "verified_events_only" && semantics != "normalized_horizontal_h0_candidate")
      throw std::runtime_error("archive_unknown_semantics");
    if (normalized_reduced_ != (semantics == "normalized_horizontal_h0_candidate"))
      throw std::runtime_error("archive_forest_semantics_mismatch");
    if (digest_all.size() != 64 || digest_all.find_first_not_of("0123456789abcdef") != std::string_view::npos)
      throw std::runtime_error("archive_invalid_digest");
    archive_detail::File manifest(directory_.fd(), "manifest.json");
    manifest.text("{\n  \"schema\": \"mhgp7-archive-v1\",\n  \"status\": \"completed\",\n  \"public_status\": \"not_claimed\",\n  \"require_exact\": false,\n  \"vertical_maps\": \"none\",\n  \"forest_semantics\": \"");
    manifest.text(semantics);
    manifest.text("\",\n  \"points\": " + std::to_string(input_count_) + ",\n  \"kmax\": " + std::to_string(kmax) + ",\n  \"digest_all\": \"");
    manifest.text(digest_all);
    manifest.text("\",\n  \"files\": [\n");
    for (size_t i = 0; i < entries_.size(); ++i) {
      const auto& e = entries_[i];
      manifest.text("    {\"name\": \"" + e.name + "\", \"sha256\": \"" + e.sha256 + "\", \"bytes\": " + std::to_string(e.bytes) + "}");
      manifest.text(i + 1 == entries_.size() ? "\n" : ",\n");
    }
    manifest.text("  ]\n}\n");
    manifest.finish();
    directory_.sync();
    // RENAME_NOREPLACE closes the race after the initial destination check.
    parent_sync_confirmed_ = directory_.publish(destination_name_.c_str());
    // Rename is the publication point. No later failure may report an abort
    // while leaving a completed archive visible. Parent fsync concerns crash
    // durability only; its outcome is an explicit, non-throwing diagnostic.
    committed_ = true;
    guard.done = true;
  }
  bool parent_sync_confirmed() const noexcept { return parent_sync_confirmed_; }
 private:
  struct WriteGuard {
    bool& failed;
    bool done = false;
    ~WriteGuard() { if (!done) failed = true; }
  };
  void require_open() const {
    if (committed_ || !directory_.open()) throw std::runtime_error("archive_already_committed");
    if (failed_) throw std::runtime_error("archive_abandoned");
  }
  struct Entry { std::string name, sha256; u64 bytes; };
  archive_detail::StagingDirectory directory_;
  std::string destination_name_;
  std::vector<Entry> entries_;
  std::string input_hash_;
  u64 input_count_ = 0, next_k_ = 1;
  bool committed_ = false, parent_sync_confirmed_ = false, failed_ = false;
  bool normalized_reduced_ = false;
};

}  // namespace mhgp7
