#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream_durable.hpp"
#include "morsehgp3d/spatial/brute_force.hpp"

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::ExactPairSupportAuthorityContext;
using morsehgp3d::hierarchy::ExactPairSupportCheckpoint;
using morsehgp3d::hierarchy::ExactPairSupportDurableConfig;
using morsehgp3d::hierarchy::ExactPairSupportDurablePublishDecision;
using morsehgp3d::hierarchy::ExactPairSupportDurablePublishOptions;
using morsehgp3d::hierarchy::ExactPairSupportDurablePublishStage;
using morsehgp3d::hierarchy::ExactPairSupportDurableSink;
using morsehgp3d::hierarchy::ExactPairSupportIncrementalVerifier;
using morsehgp3d::hierarchy::ExactPairSupportStreamBudget;
using morsehgp3d::hierarchy::ExactPairSupportStreamChunk;
using morsehgp3d::hierarchy::ExactPairSupportStreamCodecLimits;
using morsehgp3d::hierarchy::build_exact_pair_support_stream_chunk;
using morsehgp3d::hierarchy::decode_exact_pair_support_stream_chunk;
using morsehgp3d::hierarchy::encode_exact_pair_support_stream_chunk;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

constexpr std::string_view temporary_prefix = ".pair-support-";
constexpr std::string_view temporary_suffix = ".tmp";
constexpr std::string_view final_prefix = "pair-support-";
constexpr std::string_view final_suffix = ".p9c";

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const std::exception&) {
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

class TemporaryWorkspace {
 public:
  TemporaryWorkspace() {
    temporary_parent_ = std::filesystem::temp_directory_path();
    std::string pattern =
        (temporary_parent_ / "morsehgp3d-pair-durable-XXXXXX").string();
    std::vector<char> writable(pattern.begin(), pattern.end());
    writable.push_back('\0');
    char* const created = ::mkdtemp(writable.data());
    if (created == nullptr) {
      throw std::system_error(
          errno, std::generic_category(), "mkdtemp failed");
    }
    root_ = std::filesystem::path{created};
  }

  ~TemporaryWorkspace() {
    if (!root_.empty() && root_.parent_path() == temporary_parent_ &&
        root_.filename().string().starts_with(
            "morsehgp3d-pair-durable-")) {
      std::error_code ignored;
      static_cast<void>(std::filesystem::remove_all(root_, ignored));
    }
  }

  TemporaryWorkspace(const TemporaryWorkspace&) = delete;
  TemporaryWorkspace& operator=(const TemporaryWorkspace&) = delete;

  [[nodiscard]] std::filesystem::path make_directory(
      std::string_view name) const {
    const std::filesystem::path directory = root_ / name;
    if (!std::filesystem::create_directory(directory)) {
      throw std::runtime_error("cannot create a durable test directory");
    }
    return directory;
  }

  [[nodiscard]] const std::filesystem::path& root() const noexcept {
    return root_;
  }

 private:
  std::filesystem::path temporary_parent_;
  std::filesystem::path root_;
};

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y = 0.0,
    double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud triangle_cloud() {
  const std::array<CertifiedPoint3, 3U> points{
      point(-1.0, 0.0), point(0.0, 1.0), point(1.0, 0.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactPairSupportStreamBudget fixed_budget() {
  return ExactPairSupportStreamBudget{
      1U,
      32U,
      32U,
      1U,
      16U,
      1U,
      3U};
}

[[nodiscard]] ExactPairSupportStreamCodecLimits codec_limits() {
  return ExactPairSupportStreamCodecLimits{
      1U * 1024U * 1024U,
      128U,
      128U,
      32U,
      512U,
      4U * 1024U,
      64U * 1024U};
}

[[nodiscard]] ExactPairSupportDurableConfig durable_config() {
  return ExactPairSupportDurableConfig{
      fixed_budget(), codec_limits(), 128U, 32U * 1024U * 1024U};
}

struct PublishedSummary {
  ExactPairSupportCheckpoint terminal_checkpoint;
  std::size_t transition_count{};
  std::size_t encoded_byte_count{};
};

[[nodiscard]] PublishedSummary publish_to_terminal(
    ExactPairSupportDurableSink& sink,
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportDurableConfig& config) {
  std::size_t iteration_count = 0U;
  while (!sink.trusted_checkpoint().complete()) {
    if (iteration_count >= config.maximum_committed_transition_count) {
      throw std::runtime_error(
          "the durable triangle fixture exceeded its transition cap");
    }
    const ExactPairSupportCheckpoint source = sink.trusted_checkpoint();
    const ExactPairSupportStreamChunk chunk =
        build_exact_pair_support_stream_chunk(
            authority, config.fixed_chunk_budget, source);
    const auto result = sink.publish_next(chunk);
    if (result.decision !=
            ExactPairSupportDurablePublishDecision::durably_published ||
        !result.trusted_checkpoint_advanced) {
      throw std::runtime_error(
          "the durable triangle fixture rejected an exact transition");
    }
    ++iteration_count;
  }
  return PublishedSummary{
      sink.trusted_checkpoint(),
      sink.status().committed_transition_count,
      sink.status().total_encoded_byte_count};
}

struct DirectoryCounts {
  std::size_t temporary{};
  std::size_t final{};
};

[[nodiscard]] DirectoryCounts count_transition_files(
    const std::filesystem::path& directory) {
  DirectoryCounts counts;
  for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    const std::string name = entry.path().filename().string();
    if (name.starts_with(temporary_prefix) &&
        name.ends_with(temporary_suffix)) {
      ++counts.temporary;
    } else if (name.starts_with(final_prefix) &&
               name.ends_with(final_suffix)) {
      ++counts.final;
    }
  }
  return counts;
}

[[nodiscard]] std::filesystem::path first_final_path(
    const std::filesystem::path& directory) {
  return directory / "pair-support-00000000000000000000.p9c";
}

void seed_one_transition(
    const std::filesystem::path& directory,
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportDurableConfig& config) {
  ExactPairSupportDurableSink sink(directory, authority, config);
  const ExactPairSupportStreamChunk first =
      build_exact_pair_support_stream_chunk(
          authority, config.fixed_chunk_budget, sink.trusted_checkpoint());
  const auto result = sink.publish_next(first);
  if (result.decision !=
      ExactPairSupportDurablePublishDecision::durably_published) {
    throw std::runtime_error("cannot seed a durable transition fixture");
  }
}

void corrupt_first_byte(const std::filesystem::path& path) {
  const int descriptor = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
  if (descriptor < 0) {
    throw std::system_error(
        errno, std::generic_category(), "cannot open corruption fixture");
  }
  std::uint8_t byte{};
  if (::pread(descriptor, &byte, 1U, 0) != 1) {
    const int error_number = errno;
    static_cast<void>(::close(descriptor));
    throw std::system_error(
        error_number,
        std::generic_category(),
        "cannot read corruption fixture");
  }
  byte = static_cast<std::uint8_t>(byte ^ std::uint8_t{0x80U});
  if (::pwrite(descriptor, &byte, 1U, 0) != 1 ||
      ::fsync(descriptor) != 0) {
    const int error_number = errno;
    static_cast<void>(::close(descriptor));
    throw std::system_error(
        error_number,
        std::generic_category(),
        "cannot write corruption fixture");
  }
  if (::close(descriptor) != 0) {
    throw std::system_error(
        errno, std::generic_category(), "cannot close corruption fixture");
  }
}

void truncate_last_byte(const std::filesystem::path& path) {
  struct stat metadata {};
  if (::stat(path.c_str(), &metadata) != 0 || metadata.st_size <= 1) {
    throw std::runtime_error("cannot size truncation fixture");
  }
  if (::truncate(path.c_str(), metadata.st_size - 1) != 0) {
    throw std::system_error(
        errno, std::generic_category(), "cannot truncate fixture");
  }
}

void test_publication_recovery_and_lock() {
  const CanonicalPointCloud cloud = triangle_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportAuthorityContext authority(index, cloud, 1U);
  const ExactPairSupportDurableConfig config = durable_config();
  TemporaryWorkspace workspace;
  const std::filesystem::path directory = workspace.make_directory("run");

  PublishedSummary published;
  {
    ExactPairSupportDurableSink sink(directory, authority, config);
    check(
        sink.status().writer_lock_acquired &&
            sink.status().anchored_prefix_certified &&
            sink.status().committed_transition_count == 0U,
        "a new durable sink owns the writer lock and the anchored empty prefix");
    check_throws(
        [&] {
          ExactPairSupportDurableSink concurrent(
              directory, authority, config);
        },
        "a concurrent durable writer lock is refused");
    published = publish_to_terminal(sink, authority, config);
    check(
        published.transition_count > 1U &&
            published.terminal_checkpoint.complete() &&
            sink.status().anchored_run_certified &&
            sink.status().terminal_checkpoint_reached &&
            sink.status().retained_chunk_history_count == 0U &&
            sink.status().persistent_top_m_cell_count == 0U &&
            sink.status().global_gamma_coface_count == 0U &&
            sink.status().global_gamma_incidence_count == 0U &&
            sink.status().materialized_pair_arena_count == 0U,
        "bounded transitions reach an anchored terminal checkpoint without a retained chunk or global-cell arena");
  }

  {
    ExactPairSupportDurableSink recovered(directory, authority, config);
    check(
        recovered.status().recovered_transition_count ==
                published.transition_count &&
            recovered.status().committed_transition_count ==
                published.transition_count &&
            recovered.status().total_encoded_byte_count ==
                published.encoded_byte_count &&
            recovered.status().anchored_run_certified &&
            recovered.trusted_checkpoint().checkpoint_digest ==
                published.terminal_checkpoint.checkpoint_digest &&
            recovered.trusted_checkpoint().output_chain_digest ==
                published.terminal_checkpoint.output_chain_digest,
        "reopening replays the complete bounded prefix and recovers identical trusted digests");
  }
}

void test_hostile_files_and_symlinks() {
  const CanonicalPointCloud cloud = triangle_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportAuthorityContext authority(index, cloud, 1U);
  const ExactPairSupportDurableConfig config = durable_config();

  {
    TemporaryWorkspace workspace;
    const std::filesystem::path directory =
        workspace.make_directory("corrupt");
    seed_one_transition(directory, authority, config);
    corrupt_first_byte(first_final_path(directory));
    check_throws(
        [&] {
          ExactPairSupportDurableSink rejected(
              directory, authority, config);
        },
        "a corrupted committed transition is rejected during anchored recovery");
  }

  {
    TemporaryWorkspace workspace;
    const std::filesystem::path directory =
        workspace.make_directory("truncated");
    seed_one_transition(directory, authority, config);
    truncate_last_byte(first_final_path(directory));
    check_throws(
        [&] {
          ExactPairSupportDurableSink rejected(
              directory, authority, config);
        },
        "a truncated committed transition is rejected during bounded decoding");
  }

  {
    TemporaryWorkspace workspace;
    const std::filesystem::path directory = workspace.make_directory("gap");
    seed_one_transition(directory, authority, config);
    std::filesystem::rename(
        first_final_path(directory),
        directory / "pair-support-00000000000000000001.p9c");
    check_throws(
        [&] {
          ExactPairSupportDurableSink rejected(
              directory, authority, config);
        },
        "a committed transition sequence with a missing zero prefix is rejected before replay");
  }

  {
    TemporaryWorkspace workspace;
    const std::filesystem::path directory = workspace.make_directory("fifo");
    if (::mkfifo(first_final_path(directory).c_str(), 0600) != 0) {
      throw std::system_error(
          errno, std::generic_category(), "cannot create FIFO fixture");
    }
    const pid_t child = ::fork();
    if (child < 0) {
      throw std::system_error(
          errno, std::generic_category(), "fork failed for FIFO fixture");
    }
    if (child == 0) {
      static_cast<void>(::alarm(2U));
      try {
        ExactPairSupportDurableSink rejected(
            directory, authority, config);
      } catch (...) {
        std::_Exit(0);
      }
      std::_Exit(1);
    }
    int child_status = 0;
    pid_t waited = -1;
    do {
      waited = ::waitpid(child, &child_status, 0);
    } while (waited < 0 && errno == EINTR);
    check(
        waited == child && WIFEXITED(child_status) &&
            WEXITSTATUS(child_status) == 0,
        "a FIFO at a canonical final name is rejected without a blocking open");
  }

  {
    TemporaryWorkspace workspace;
    const std::filesystem::path actual = workspace.make_directory("actual");
    const std::filesystem::path link = workspace.root() / "directory-link";
    std::filesystem::create_directory_symlink(actual, link);
    check_throws(
        [&] {
          ExactPairSupportDurableSink rejected(link, authority, config);
        },
        "a symlink supplied as the dedicated directory is rejected");
  }

  {
    TemporaryWorkspace workspace;
    const std::filesystem::path directory =
        workspace.make_directory("final-link");
    const std::filesystem::path target = workspace.root() / "outside";
    {
      const int descriptor = ::open(
          target.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
      if (descriptor < 0 || ::close(descriptor) != 0) {
        throw std::runtime_error("cannot create the symlink target fixture");
      }
    }
    std::filesystem::create_symlink(target, first_final_path(directory));
    check_throws(
        [&] {
          ExactPairSupportDurableSink rejected(
              directory, authority, config);
        },
        "a symlink at a committed transition name is rejected without following it");
  }
}

[[nodiscard]] int crash_exit_code(
    ExactPairSupportDurablePublishStage stage) noexcept {
  return 40 + static_cast<int>(static_cast<std::uint8_t>(stage));
}

void exit_at_publish_stage(
    ExactPairSupportDurablePublishStage observed,
    void* state) noexcept {
  const auto requested =
      *static_cast<const ExactPairSupportDurablePublishStage*>(state);
  if (observed == requested) {
    std::_Exit(crash_exit_code(observed));
  }
}

[[nodiscard]] bool is_before_rename(
    ExactPairSupportDurablePublishStage stage) noexcept {
  return stage ==
             ExactPairSupportDurablePublishStage::temporary_file_written ||
         stage == ExactPairSupportDurablePublishStage::
                      temporary_file_synchronized;
}

void test_real_process_loss_at_each_publish_stage() {
  const CanonicalPointCloud cloud = triangle_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportAuthorityContext authority(index, cloud, 1U);
  const ExactPairSupportDurableConfig config = durable_config();
  const ExactPairSupportCheckpoint initial =
      morsehgp3d::hierarchy::make_initial_exact_pair_support_checkpoint(
          authority);
  const ExactPairSupportStreamChunk first =
      build_exact_pair_support_stream_chunk(
          authority, config.fixed_chunk_budget, initial);
  const std::vector<std::uint8_t> canonical_encoding =
      encode_exact_pair_support_stream_chunk(first, config.codec_limits);
  const std::array<ExactPairSupportDurablePublishStage, 4U> stages{
      ExactPairSupportDurablePublishStage::temporary_file_written,
      ExactPairSupportDurablePublishStage::temporary_file_synchronized,
      ExactPairSupportDurablePublishStage::transition_renamed,
      ExactPairSupportDurablePublishStage::directory_synchronized};

  for (const ExactPairSupportDurablePublishStage stage : stages) {
    TemporaryWorkspace workspace;
    const std::filesystem::path directory =
        workspace.make_directory("crash");
    const pid_t child = ::fork();
    if (child < 0) {
      throw std::system_error(
          errno, std::generic_category(), "fork failed");
    }
    if (child == 0) {
      try {
        ExactPairSupportDurableSink sink(directory, authority, config);
        ExactPairSupportDurablePublishStage requested = stage;
        const ExactPairSupportDurablePublishOptions options{
            exit_at_publish_stage, &requested};
        static_cast<void>(sink.publish_next(first, options));
        std::_Exit(100);
      } catch (...) {
        std::_Exit(101);
      }
    }

    int child_status = 0;
    pid_t waited = -1;
    do {
      waited = ::waitpid(child, &child_status, 0);
    } while (waited < 0 && errno == EINTR);
    check(
        waited == child && WIFEXITED(child_status) &&
            WEXITSTATUS(child_status) == crash_exit_code(stage),
        "the child process exits at the requested durable publication boundary");

    {
      ExactPairSupportDurableSink recovered(directory, authority, config);
      const bool before_rename = is_before_rename(stage);
      const DirectoryCounts after_recovery =
          count_transition_files(directory);
      check(
          after_recovery.temporary == 0U &&
              recovered.status().committed_transition_count ==
                  (before_rename ? 0U : 1U) &&
              recovered.status().removed_uncommitted_temporary_file_count ==
                  (before_rename ? 1U : 0U) &&
              recovered.trusted_checkpoint().checkpoint_digest ==
                  (before_rename ? initial.checkpoint_digest
                                 : first.next_checkpoint.checkpoint_digest),
          "recovery selects exactly the old state before rename and the new state after rename while removing only an uncommitted temporary");

      if (before_rename) {
        const ExactPairSupportStreamChunk retried =
            build_exact_pair_support_stream_chunk(
                authority,
                config.fixed_chunk_budget,
                recovered.trusted_checkpoint());
        check(
            retried == first &&
                encode_exact_pair_support_stream_chunk(
                    retried, config.codec_limits) == canonical_encoding,
            "an interrupted pre-rename transition is reproduced byte for byte from the unchanged trusted checkpoint");
        const auto retry_result = recovered.publish_next(retried);
        check(
            retry_result.decision ==
                    ExactPairSupportDurablePublishDecision::
                        durably_published &&
                retry_result.committed_transition_count == 1U,
            "a deterministic pre-rename retry durably publishes one transition");
      }
    }

    {
      ExactPairSupportDurableSink reopened(directory, authority, config);
      const DirectoryCounts final_counts =
          count_transition_files(directory);
      check(
          reopened.status().committed_transition_count == 1U &&
              reopened.trusted_checkpoint().checkpoint_digest ==
                  first.next_checkpoint.checkpoint_digest &&
              final_counts.temporary == 0U && final_counts.final == 1U,
          "a second recovery observes one and only one transition after crash recovery or deterministic retry");
    }
  }
}

void test_wire_integrity_is_not_scientific_provenance() {
  const CanonicalPointCloud cloud = triangle_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportAuthorityContext authority(index, cloud, 1U);
  const ExactPairSupportDurableConfig config = durable_config();
  ExactPairSupportIncrementalVerifier verifier(authority);
  ExactPairSupportStreamChunk mutated =
      build_exact_pair_support_stream_chunk(
          authority,
          config.fixed_chunk_budget,
          verifier.trusted_checkpoint());
  mutated.source_checkpoint_digest = {};
  const std::vector<std::uint8_t> checksummed_mutation =
      encode_exact_pair_support_stream_chunk(
          mutated, config.codec_limits);
  const auto decoded = decode_exact_pair_support_stream_chunk(
      checksummed_mutation, config.codec_limits);
  check(
      decoded.accepted() && decoded.chunk.has_value(),
      "a canonically rechecksummed semantic mutation remains a structurally valid wire bundle");
  check(
      decoded.chunk.has_value() &&
          !verifier
               .verify_next(
                   config.fixed_chunk_budget, *decoded.chunk)
               .chunk_transition_verified &&
          verifier.status().failed_closed,
      "anchored exact replay rejects a checksummed mutation because wire integrity is not provenance");
}

}  // namespace

int main() {
  try {
    test_publication_recovery_and_lock();
    test_hostile_files_and_symlinks();
    test_real_process_loss_at_each_publish_stage();
    test_wire_integrity_is_not_scientific_provenance();
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
  }
  if (failures != 0) {
    std::cerr << failures << " durable pair-support test(s) failed\n";
    return 1;
  }
  std::cout << "durable pair-support stream tests passed\n";
  return 0;
}
