// Lifecycle and failure injection below the archive API; the fsync wrapper is
// linked only into this test and cannot be selected through the product CLI.
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/stat.h>

#include "../src/io/archive.hpp"
#include "../src/pipeline/digest.hpp"

using namespace mhgp7;

namespace {
int file_syncs = 0, directory_syncs = 0;
int fail_file_sync = 0, fail_directory_sync = 0;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

template <typename F>
void refuses(F&& call, const char* expected) {
  try {
    call();
  } catch (const std::runtime_error& e) {
    require(std::string(e.what()) == expected, "wrong refusal cause");
    return;
  }
  throw std::runtime_error("missing refusal");
}

void reset_faults() {
  file_syncs = directory_syncs = fail_file_sync = fail_directory_sync = 0;
}

struct Temporary {
  std::filesystem::path path;
  Temporary() {
    auto pattern = (std::filesystem::temp_directory_path() / "mhgp7-archive-api-XXXXXX").string();
    std::vector<char> name(pattern.begin(), pattern.end());
    name.push_back('\0');
    char* created = ::mkdtemp(name.data());
    if (!created) throw std::runtime_error("temporary directory unavailable");
    path = created;
  }
  ~Temporary() {
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
  }
};

ForestResult forest() {
  ForestResult r;
  for (const PointId id : {7u, 42u, 99u}) {
    FacetKey key;
    key.k = 1;
    key.p[0] = id;
    r.facet_keys.push_back(key);
  }
  r.final_canon_fid = {0, 1, 2};
  r.facets = r.facet_keys.size();
  return r;
}

std::string digest(const ForestResult& r) {
  DigestAll all;
  all.add(digest_forest_v4(1, r));
  return all.hex();
}

}  // namespace

extern "C" int __real_fsync(int fd);
extern "C" int __wrap_fsync(int fd) {
  struct stat info{};
  if (::fstat(fd, &info) != 0) return -1;
  if (S_ISDIR(info.st_mode)) {
    if (++directory_syncs == fail_directory_sync) { errno = EIO; return -1; }
  } else {
    if (++file_syncs == fail_file_sync) { errno = EIO; return -1; }
  }
  return __real_fsync(fd);
}

int main() {
  try {
    Temporary temp;
    const std::vector<InputPoint> input{{99, {3, 2, 1}}, {7, {6, 5, 4}}, {42, {9, 8, 7}}};
    const ForestResult r = forest();
    const std::string d = digest(r);
    for (const bool fail_parent : {false, true}) {
      reset_faults();
      if (fail_parent) fail_directory_sync = 2;  // staging sync precedes parent sync
      const auto destination = temp.path / (fail_parent ? "parent_sync_failed" : "normal");
      ForestArchive archive(destination);
      archive.input(input);
      archive.forest(1, r);
      archive.commit(1, "verified_events_only", d);
      require(std::filesystem::is_regular_file(destination / "manifest.json"), "committed archive absent");
      require(archive.parent_sync_confirmed() != fail_parent, "parent sync diagnostic");
      require(directory_syncs == 2, "parent failure injection not exercised");
      refuses([&] { archive.input(input); }, "archive_already_committed");
      refuses([&] { archive.forest(2, r); }, "archive_already_committed");
      refuses([&] { archive.commit(1, "verified_events_only", d); }, "archive_already_committed");
    }
    reset_faults();
    {
      ForestArchive archive(temp.path / "zero_orders");
      archive.input(input);
      refuses([&] { archive.commit(0, "verified_events_only", d); }, "archive_incomplete");
    }
    for (const i64 invalid : {-1, 65536}) {
      ForestArchive archive(temp.path / (invalid < 0 ? "negative" : "wide"));
      auto malformed = input;
      malformed.back().position.z = invalid;  // first records have already been written
      refuses([&] { archive.input(malformed); }, "archive_input_outside_u16");
      refuses([&] { archive.forest(1, r); }, "archive_abandoned");
    }
    {
      ForestArchive archive(temp.path / "empty");
      refuses([&] { archive.input({}); }, "archive_input_point_count");
    }
    // A written but failed K2 cannot be published by committing the old K1
    // prefix. No fake archive hook: fsync itself returns EIO through the linker.
    reset_faults();
    {
      ForestArchive archive(temp.path / "failed_suffix");
      archive.input(input);
      archive.forest(1, r);
      fail_file_sync = 3;
      refuses([&] { archive.forest(2, r); }, "archive_sync_file_failed");
      require(file_syncs == 3, "file failure injection not exercised");
      refuses([&] { archive.commit(1, "verified_events_only", d); }, "archive_abandoned");
    }
    reset_faults();
    for (const bool normalized : {false, true}) {
      ForestArchive archive(temp.path / (normalized ? "wrong_normalized" : "wrong_legacy"));
      archive.input(input);
      auto output = r;
      output.normalized_reduced = normalized;
      archive.forest(1, output);
      refuses([&] { archive.commit(1, normalized ? "verified_events_only" : "normalized_horizontal_h0_candidate", d); },
              "archive_forest_semantics_mismatch");
    }
    {
      ForestArchive archive(temp.path / "mixed_semantics");
      archive.input(input);
      archive.forest(1, r);
      auto output = r;
      output.normalized_reduced = true;
      refuses([&] { archive.forest(2, output); }, "archive_mixed_forest_semantics");
      refuses([&] { archive.commit(1, "verified_events_only", d); }, "archive_abandoned");
    }
    for (const auto& entry : std::filesystem::directory_iterator(temp.path)) {
      const auto name = entry.path().filename().string();
      require(name == "normal" || name == "parent_sync_failed", "refused or provisional archive leaked");
    }
    std::puts("archive_api_gate=passed completed=2 parent_sync_fault=1 file_sync_fault=1 closed_lifecycle=6 invalid_inputs=3 semantics_refusals=3");
    return 0;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "archive API gate: %s\n", e.what());
    return 1;
  }
}
