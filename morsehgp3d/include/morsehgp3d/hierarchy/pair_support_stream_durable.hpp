#pragma once

#include "morsehgp3d/hierarchy/pair_support_stream_codec.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>

namespace morsehgp3d::hierarchy {

// The durable protocol is deliberately limited to a dedicated directory on a
// local Unix filesystem providing flock, fdatasync, atomic hard-link
// publication, atomic HEAD replacement and directory fsync.  It certifies
// recovery after process loss; it does not
// claim power-loss behavior for an unverified filesystem or block device, nor
// detect rollback to an older otherwise valid file prefix without an external
// monotone prefix anchor kept outside the restore domain.
inline constexpr std::uint32_t pair_support_durable_schema_version = 2U;

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

// A caller may persist this compact prefix witness in an independent,
// monotone store.  Supplying it on reopen proves only that the local run still
// contains this exact certified prefix; a locally stored HEAD cannot by itself
// detect a coordinated rollback of both HEAD and transition files.
struct ExactPairSupportDurableExternalPrefixAnchor {
  std::uint64_t committed_transition_count{};
  contract::CanonicalId checkpoint_digest{};

  friend bool operator==(
      const ExactPairSupportDurableExternalPrefixAnchor&,
      const ExactPairSupportDurableExternalPrefixAnchor&) = default;
};

enum class ExactPairSupportDurablePublishStage : std::uint8_t {
  transition_temporary_file_written,
  transition_temporary_file_synchronized,
  transition_final_link_created,
  transition_temporary_link_removed,
  transition_directory_synchronized,
  head_temporary_file_written,
  head_temporary_file_synchronized,
  head_replaced,
  head_directory_synchronized,
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
  ExactPairSupportDurableExternalPrefixAnchor current_prefix_anchor{};
  int system_error_number{};
  bool trusted_checkpoint_advanced{false};
};

struct ExactPairSupportDurableStatus {
  std::size_t recovered_transition_count{};
  std::size_t committed_transition_count{};
  std::size_t total_encoded_byte_count{};
  std::size_t maximum_simultaneously_decoded_chunk_count{};
  std::size_t removed_uncommitted_temporary_file_count{};
  std::size_t removed_uncommitted_final_file_count{};
  ExactPairSupportDurableExternalPrefixAnchor current_prefix_anchor{};
  bool writer_lock_acquired{false};
  bool authoritative_head_certified{false};
  bool external_prefix_anchor_supplied{false};
  bool external_prefix_anchor_verified{false};
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
  // Both entry points require a preexisting dedicated directory and acquire a
  // nonblocking cooperative writer lock.  create_new refuses an existing
  // authoritative namespace; open_existing never interprets a missing HEAD as
  // an empty run.
  [[nodiscard]] static ExactPairSupportDurableSink create_new(
      const std::filesystem::path& dedicated_directory,
      const ExactPairSupportAuthorityContext& authority,
      ExactPairSupportDurableConfig config);

  [[nodiscard]] static ExactPairSupportDurableSink open_existing(
      const std::filesystem::path& dedicated_directory,
      const ExactPairSupportAuthorityContext& authority,
      ExactPairSupportDurableConfig config,
      std::optional<ExactPairSupportDurableExternalPrefixAnchor>
          expected_prefix_anchor = std::nullopt);
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
  enum class OpenMode : std::uint8_t {
    create_new,
    open_existing,
  };

  ExactPairSupportDurableSink(
      OpenMode mode,
      const std::filesystem::path& dedicated_directory,
      const ExactPairSupportAuthorityContext& authority,
      ExactPairSupportDurableConfig config,
      std::optional<ExactPairSupportDurableExternalPrefixAnchor>
          expected_prefix_anchor);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace morsehgp3d::hierarchy
