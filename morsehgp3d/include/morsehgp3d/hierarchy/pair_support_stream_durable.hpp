#pragma once

#include "morsehgp3d/hierarchy/pair_support_stream_codec.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace morsehgp3d::hierarchy {

// The durable protocol is deliberately limited to a dedicated directory on a
// local Unix filesystem providing flock, fdatasync, atomic rename and
// directory fsync.  It certifies recovery after process loss; it does not
// claim power-loss behavior for an unverified filesystem or block device, nor
// detect rollback to an older otherwise valid file prefix without an external
// authoritative head.
inline constexpr std::uint32_t pair_support_durable_schema_version = 1U;

struct ExactPairSupportDurableConfig {
  // One immutable external authority for every transition in this sink.  A
  // decoded file never gets to choose the replay budget used by the verifier.
  ExactPairSupportStreamBudget fixed_chunk_budget{};
  ExactPairSupportStreamCodecLimits codec_limits{};
  std::size_t maximum_committed_transition_count{};
  std::size_t maximum_total_encoded_byte_count{};

  friend bool operator==(
      const ExactPairSupportDurableConfig&,
      const ExactPairSupportDurableConfig&) = default;
};

enum class ExactPairSupportDurablePublishStage : std::uint8_t {
  temporary_file_written,
  temporary_file_synchronized,
  transition_renamed,
  directory_synchronized,
};

using ExactPairSupportDurablePublishObserver = void (*)(
    ExactPairSupportDurablePublishStage,
    void*) noexcept;

struct ExactPairSupportDurablePublishOptions {
  // Tests may terminate a child process at one of the explicit durability
  // boundaries.  Production callers normally leave this callback null.
  ExactPairSupportDurablePublishObserver observer{};
  void* observer_state{};
};

enum class ExactPairSupportDurablePublishDecision : std::uint8_t {
  durably_published,
  terminal_checkpoint_already_reached,
  codec_limit_rejected,
  transition_rejected_failed_closed,
  retryable_io_failure,
  indeterminate_io_failure_reopen_required,
};

struct ExactPairSupportDurablePublishResult {
  ExactPairSupportDurablePublishDecision decision{
      ExactPairSupportDurablePublishDecision::
          transition_rejected_failed_closed};
  ExactPairSupportStreamChunkVerification transition_verification{};
  std::size_t committed_transition_count{};
  std::size_t total_encoded_byte_count{};
  int system_error_number{};
  bool trusted_checkpoint_advanced{false};
};

struct ExactPairSupportDurableStatus {
  std::size_t recovered_transition_count{};
  std::size_t committed_transition_count{};
  std::size_t total_encoded_byte_count{};
  std::size_t maximum_simultaneously_decoded_chunk_count{};
  std::size_t removed_uncommitted_temporary_file_count{};
  bool writer_lock_acquired{false};
  bool anchored_prefix_certified{false};
  bool anchored_run_certified{false};
  bool terminal_checkpoint_reached{false};
  bool failed_closed{false};
  // Explicit architectural witnesses: durable replay retains one decoded
  // transition and never creates a higher-order mosaic or global incidence
  // arena.
  std::size_t retained_chunk_history_count{};
  std::size_t persistent_top_m_cell_count{};
  std::size_t global_gamma_coface_count{};
  std::size_t global_gamma_incidence_count{};
  std::size_t materialized_pair_arena_count{};
};

class ExactPairSupportDurableSink {
 public:
  // The directory must already exist, be dedicated to this run and not be a
  // symlink.  Construction acquires a nonblocking exclusive writer lock,
  // reconstructs the initial checkpoint and replays every contiguous final
  // transition.  Any malformed, noncontiguous or hostile entry fails closed by
  // throwing; it is never skipped or removed.
  ExactPairSupportDurableSink(
      const std::filesystem::path& dedicated_directory,
      const ExactPairSupportAuthorityContext& authority,
      ExactPairSupportDurableConfig config);
  ~ExactPairSupportDurableSink();

  ExactPairSupportDurableSink(const ExactPairSupportDurableSink&) = delete;
  ExactPairSupportDurableSink& operator=(
      const ExactPairSupportDurableSink&) = delete;
  ExactPairSupportDurableSink(ExactPairSupportDurableSink&&) = delete;
  ExactPairSupportDurableSink& operator=(
      ExactPairSupportDurableSink&&) = delete;

  [[nodiscard]] ExactPairSupportDurablePublishResult publish_next(
      const ExactPairSupportStreamChunk& observed,
      ExactPairSupportDurablePublishOptions options = {});

  [[nodiscard]] const ExactPairSupportCheckpoint& trusted_checkpoint()
      const noexcept;
  [[nodiscard]] const ExactPairSupportDurableStatus& status() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace morsehgp3d::hierarchy
