#pragma once

#include "morsehgp3d/hierarchy/direct_morse_chunk_run.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_reducer.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_durable_live_commit_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_durable_live_commit_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_durable_live_commit_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_durable_live_commit_mode = "budgeted";
inline constexpr std::string_view
    direct_morse_durable_live_commit_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_morse_durable_live_commit_public_status = "not_claimed";
inline constexpr std::string_view
    direct_morse_durable_live_commit_proof_basis =
        "single_batch_live_delta_equals_fresh_durable_replay_recertified_"
        "reversible_files_then_atomic_reducer_cursor_commit_before_HEAD_v1";

enum class ExactDirectMorseDurableLiveCommitDecision : std::uint8_t {
  not_committed,
  no_commit_publication_rejected_retryable,
  no_commit_live_participant_rejected_retryable,
  no_commit_indeterminate_reopen_required,
  complete_durable_live_single_batch_commit,
};

struct ExactDirectMorseDurableLiveCommitResult {
  static constexpr std::string_view backend =
      direct_morse_durable_live_commit_backend;
  static constexpr std::string_view profile =
      direct_morse_durable_live_commit_profile;
  static constexpr std::string_view mode =
      direct_morse_durable_live_commit_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_durable_live_commit_deployment_status;
  static constexpr std::string_view public_status =
      direct_morse_durable_live_commit_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_durable_live_commit_proof_basis;

  std::uint32_t schema_version{
      direct_morse_durable_live_commit_schema_version};
  std::size_t source_batch_index{};
  bool one_batch_chunk_bound_to_live_delta{false};
  bool publication_recertified_before_process_local_commit{false};
  bool process_local_commit_preceded_HEAD{false};
  bool authoritative_HEAD_and_live_cursors_aligned{false};
  bool retryable_without_reopen{false};
  bool reopen_required{false};
  bool coordinator_poisoned{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  AtomicLinearRunPublishResult publication{};
  std::optional<ExactDirectMorseForestLiveCommitResult> live_commit;
  ExactDirectMorseDurableLiveCommitDecision decision{
      ExactDirectMorseDurableLiveCommitDecision::not_committed};

  [[nodiscard]] bool certified_complete_commit() const noexcept;
  [[nodiscard]] bool certified_retryable_rejection() const noexcept;
  [[nodiscard]] bool certified_reopen_required() const noexcept;
};

// Serialized Phase-15E seam.  While this coordinator is live, callers must
// not access its store, executor or reducer through another path.  A process-
// local commit happens only after the transition and candidate HEAD have been
// written, synchronized, reread and scientifically recertified.  HEAD remains
// the durable recovery authority: any later gate or I/O uncertainty poisons
// this coordinator and requires all three live objects to be discarded and
// rebuilt from HEAD.
class ExactDirectMorseDurableLiveCommitCoordinator {
 public:
  ExactDirectMorseDurableLiveCommitCoordinator(
      const ExactDirectMorseChunkRunContext& chunk_context,
      AtomicLinearRunStore& store,
      ExactDirectSparseFacetDescentAnchoredBatchExecutor& executor,
      ExactDirectMorseForestReducer& reducer);

  ExactDirectMorseDurableLiveCommitCoordinator(
      const ExactDirectMorseDurableLiveCommitCoordinator&) = delete;
  ExactDirectMorseDurableLiveCommitCoordinator& operator=(
      const ExactDirectMorseDurableLiveCommitCoordinator&) = delete;
  ExactDirectMorseDurableLiveCommitCoordinator(
      ExactDirectMorseDurableLiveCommitCoordinator&&) = delete;
  ExactDirectMorseDurableLiveCommitCoordinator& operator=(
      ExactDirectMorseDurableLiveCommitCoordinator&&) = delete;

  [[nodiscard]] ExactDirectMorseDurableLiveCommitResult
  commit_next_single_batch_chunk(
      ExactDirectSparseFacetDescentAnchoredBatchExecutor::
          PreparedTopKProposalBatch& prepared,
      AtomicLinearRunPublishOptions options = {});

  [[nodiscard]] bool poisoned() const noexcept {
    return poisoned_;
  }

 private:
  const ExactDirectMorseChunkRunContext* chunk_context_{};
  AtomicLinearRunStore* store_{};
  ExactDirectSparseFacetDescentAnchoredBatchExecutor* executor_{};
  ExactDirectMorseForestReducer* reducer_{};
  bool active_{false};
  bool poisoned_{false};
};

}  // namespace morsehgp3d::hierarchy
