#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/hierarchy/capped_distinct_point_coverage.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

class ExactDirectMorseResidentAtLeast20Adapter;
class ExactDirectMorseNormalizedH0ProductSession;

inline constexpr std::uint32_t
    direct_at_least20_stream_view_schema_version = 1U;
inline constexpr std::size_t direct_at_least20_stream_view_threshold = 20U;
inline constexpr std::size_t direct_at_least20_stream_view_maximum_order =
    10U;
inline constexpr std::string_view direct_at_least20_stream_view_backend =
    "reference_cpu";
inline constexpr std::string_view direct_at_least20_stream_view_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_at_least20_stream_view_mode =
    "certified_exact_downstream_at_least20_stream_view_relative_to_"
    "canonical_committed_batches_v1";
inline constexpr std::string_view
    direct_at_least20_stream_view_deployment_status =
        "bounded_checkpoint_ready_downstream_kernel";
inline constexpr std::string_view direct_at_least20_stream_view_public_status =
    "not_claimed";
inline constexpr std::string_view direct_at_least20_stream_view_proof_basis =
    "complete_equal_level_batch_then_cap20_distinct_point_union_"
    "associative_commutative_idempotent_root_successor_fold_v1";

using ExactDirectAtLeast20RootId = std::uint64_t;

enum class ExactDirectAtLeast20SourceAction : std::uint8_t {
  reduced_birth,
  continuation,
  multifusion,
};

enum class ExactDirectAtLeast20VisibleTransitionKind : std::uint8_t {
  threshold_entry,
  continuation,
  multifusion,
};

// This is the explicit public/standalone boundary.  The downstream kernel
// freshly canonicalizes and shape-checks it, but cannot prove that an upstream
// resident source actually committed it.  The integrated resident adapter and
// future product coordinator bypass this public seam through private move-only
// prepare/commit capabilities.  Public-input exactness therefore remains
// relative and never promotes a source or public hierarchy status.
struct ExactDirectAtLeast20CommittedGroupInput {
  std::size_t source_group_index{};
  ExactDirectAtLeast20SourceAction source_action{
      ExactDirectAtLeast20SourceAction::reduced_birth};
  std::size_t q_r{};
  ExactDirectAtLeast20RootId resultant_root_id{};
  std::vector<ExactDirectAtLeast20RootId> prior_root_ids;
  std::vector<spatial::PointId> coverage_delta_point_ids;
};

struct ExactDirectAtLeast20CommittedBatchInput {
  std::uint32_t schema_version{
      direct_at_least20_stream_view_schema_version};
  std::uint64_t source_session_authority_id{};
  std::size_t source_batch_cursor_before{};
  std::size_t source_batch_cursor_after{};
  std::size_t source_epoch_before{};
  std::size_t source_epoch_after{};
  std::size_t order{};
  exact::ExactLevel squared_level{};
  // Non-zero identity of the normalized scientific source (including the
  // cloud identity).  It binds the relative transcript but does not certify
  // that source: only a future non-forgeable resident adapter may do so.
  contract::CanonicalId source_scientific_digest{};
  contract::CanonicalId previous_source_commit_digest{};
  std::vector<ExactDirectAtLeast20CommittedGroupInput> groups;
  bool complete_equal_level_batch_committed{false};
  bool source_actions_qr_root_successors_and_coverage_delta_certified{false};
  bool source_pruned_by_min_cluster_size{false};
  bool source_public_status_claimed{false};
};

struct ExactDirectAtLeast20StreamViewBudget {
  std::size_t maximum_batch_group_count{};
  std::size_t maximum_batch_prior_root_reference_count{};
  std::size_t maximum_batch_coverage_delta_point_reference_count{};
  std::size_t maximum_batch_visible_transition_count{};
  std::size_t maximum_batch_visible_child_reference_count{};
  std::size_t maximum_root_state_count{};
  std::size_t maximum_active_root_count{};
  std::size_t maximum_retained_point_reference_count{};
  std::size_t maximum_staging_entry_count{};
  std::size_t maximum_checkpoint_root_state_count{};
  std::size_t maximum_checkpoint_retained_point_reference_count{};

  friend bool operator==(
      const ExactDirectAtLeast20StreamViewBudget&,
      const ExactDirectAtLeast20StreamViewBudget&) = default;
};

struct ExactDirectAtLeast20CommittedBatchCounters {
  std::size_t group_count{};
  std::size_t prior_root_reference_count{};
  std::size_t coverage_delta_point_reference_count{};
  std::size_t deduplicated_coverage_delta_point_reference_count{};
  std::size_t logical_input_entry_count{};

  friend bool operator==(
      const ExactDirectAtLeast20CommittedBatchCounters&,
      const ExactDirectAtLeast20CommittedBatchCounters&) = default;
};

enum class ExactDirectAtLeast20CommittedBatchDecision : std::uint8_t {
  not_sealed,
  no_batch_schema_rejected,
  no_batch_stamp_rejected,
  no_batch_order_rejected,
  no_batch_source_commit_premise_rejected,
  no_batch_group_budget_exhausted,
  no_batch_prior_root_budget_exhausted,
  no_batch_coverage_delta_budget_exhausted,
  no_batch_input_capacity_overflow,
  no_batch_group_shape_rejected,
  no_batch_cross_group_partition_rejected,
  no_batch_allocation_failed,
  complete_canonical_committed_batch_relative_transcript,
};

struct ExactDirectAtLeast20CommittedBatchBuildResult;

class ExactDirectAtLeast20CommittedBatch {
 public:
  ExactDirectAtLeast20CommittedBatch() noexcept;
  ~ExactDirectAtLeast20CommittedBatch();
  ExactDirectAtLeast20CommittedBatch(
      ExactDirectAtLeast20CommittedBatch&&) noexcept;
  ExactDirectAtLeast20CommittedBatch& operator=(
      ExactDirectAtLeast20CommittedBatch&&) noexcept;
  ExactDirectAtLeast20CommittedBatch(
      const ExactDirectAtLeast20CommittedBatch&) = delete;
  ExactDirectAtLeast20CommittedBatch& operator=(
      const ExactDirectAtLeast20CommittedBatch&) = delete;

  [[nodiscard]] bool valid_relative_transcript() const noexcept;
  [[nodiscard]] bool consumed() const noexcept;
  [[nodiscard]] std::uint64_t source_session_authority_id() const noexcept;
  [[nodiscard]] std::size_t source_batch_cursor_before() const noexcept;
  [[nodiscard]] std::size_t source_batch_cursor_after() const noexcept;
  [[nodiscard]] std::size_t source_epoch_before() const noexcept;
  [[nodiscard]] std::size_t source_epoch_after() const noexcept;
  [[nodiscard]] std::size_t order() const noexcept;
  [[nodiscard]] const exact::ExactLevel& squared_level() const noexcept;
  [[nodiscard]] const contract::CanonicalId& source_scientific_digest()
      const noexcept;
  [[nodiscard]] const contract::CanonicalId&
  previous_source_commit_digest() const noexcept;
  [[nodiscard]] const contract::CanonicalId& source_commit_digest()
      const noexcept;
  [[nodiscard]] const ExactDirectAtLeast20CommittedBatchCounters& counters()
      const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectAtLeast20CommittedBatch(
      std::unique_ptr<Impl>) noexcept;
  friend struct ExactDirectAtLeast20CommittedBatchBuildResult;
  friend ExactDirectAtLeast20CommittedBatchBuildResult
  seal_exact_direct_at_least20_committed_batch_relative_transcript(
      ExactDirectAtLeast20CommittedBatchInput&&,
      const ExactDirectAtLeast20StreamViewBudget&);
  friend class ExactDirectAtLeast20StreamViewSession;
};

struct ExactDirectAtLeast20CommittedBatchBuildResult {
  std::optional<ExactDirectAtLeast20CommittedBatch> batch;
  ExactDirectAtLeast20CommittedBatchCounters counters{};
  ExactDirectAtLeast20CommittedBatchDecision decision{
      ExactDirectAtLeast20CommittedBatchDecision::not_sealed};

  [[nodiscard]] bool certified_relative_transcript() const noexcept;
};

[[nodiscard]] ExactDirectAtLeast20CommittedBatchBuildResult
seal_exact_direct_at_least20_committed_batch_relative_transcript(
    ExactDirectAtLeast20CommittedBatchInput&& input,
    const ExactDirectAtLeast20StreamViewBudget& budget);

struct ExactDirectAtLeast20VisibleTransition {
  std::size_t source_group_index{};
  std::size_t order{};
  exact::ExactLevel squared_level{};
  ExactDirectAtLeast20SourceAction source_action{
      ExactDirectAtLeast20SourceAction::reduced_birth};
  std::size_t source_q_r{};
  std::size_t q_visible{};
  ExactDirectAtLeast20RootId resultant_root_id{};
  std::vector<ExactDirectAtLeast20RootId> prior_visible_root_ids;
  std::vector<spatial::PointId> retained_smallest_point_ids;
  ExactDirectAtLeast20VisibleTransitionKind kind{
      ExactDirectAtLeast20VisibleTransitionKind::threshold_entry};
  bool coverage_size_at_least_20{false};

  friend bool operator==(
      const ExactDirectAtLeast20VisibleTransition&,
      const ExactDirectAtLeast20VisibleTransition&) = default;
};

struct ExactDirectAtLeast20StreamBatchCounters {
  std::size_t processed_group_count{};
  std::size_t processed_prior_root_reference_count{};
  std::size_t processed_coverage_delta_point_reference_count{};
  std::size_t hidden_result_count{};
  std::size_t threshold_entry_count{};
  std::size_t continuation_count{};
  std::size_t multifusion_count{};
  std::size_t active_root_count_before{};
  std::size_t active_root_count_after{};
  std::size_t retained_point_reference_count_before{};
  std::size_t retained_point_reference_count_after{};
  std::size_t required_staging_entry_count{};

  friend bool operator==(
      const ExactDirectAtLeast20StreamBatchCounters&,
      const ExactDirectAtLeast20StreamBatchCounters&) = default;
};

enum class ExactDirectAtLeast20StreamConsumeDecision : std::uint8_t {
  not_consumed,
  no_session_not_initialized,
  no_batch_not_sealed_or_already_consumed,
  no_foreign_source_session_rejected,
  no_foreign_source_science_rejected,
  no_stale_source_stamp_rejected,
  no_nonincreasing_order_level_rejected,
  no_unknown_or_inactive_prior_root_rejected,
  no_resultant_root_collision_rejected,
  no_batch_partition_rejected,
  no_root_state_budget_exhausted,
  no_active_root_budget_exhausted,
  no_retained_point_budget_exhausted,
  no_visible_transition_budget_exhausted,
  no_visible_child_reference_budget_exhausted,
  no_staging_budget_exhausted,
  no_allocation_failed,
  complete_certified_atomic_downstream_batch_fold,
};

struct ExactDirectAtLeast20StreamConsumeResult {
  std::size_t source_batch_cursor_before{};
  std::size_t source_batch_cursor_after{};
  std::size_t source_epoch_before{};
  std::size_t source_epoch_after{};
  std::size_t order{};
  exact::ExactLevel squared_level{};
  contract::CanonicalId source_commit_digest{};
  contract::CanonicalId view_digest_before{};
  contract::CanonicalId view_digest_after{};
  std::vector<ExactDirectAtLeast20VisibleTransition> visible_transitions;
  ExactDirectAtLeast20StreamBatchCounters counters{};
  bool complete_equal_level_batch_observed_before_visibility{false};
  bool hidden_roots_retained_in_source_fold{false};
  bool source_pruning_performed{false};
  bool scientific_state_mutated_on_failure{false};
  bool upstream_source_commit_capability_verified{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  bool exact_relative_downstream_fold_certified{false};
  ExactDirectAtLeast20StreamConsumeDecision decision{
      ExactDirectAtLeast20StreamConsumeDecision::not_consumed};

  [[nodiscard]] bool certified_atomic_downstream_batch_fold()
      const noexcept;
};

struct ExactDirectAtLeast20CheckpointRootRecord {
  std::size_t order{};
  ExactDirectAtLeast20RootId root_id{};
  std::vector<spatial::PointId> retained_smallest_point_ids;
  bool active{false};

  friend bool operator==(
      const ExactDirectAtLeast20CheckpointRootRecord&,
      const ExactDirectAtLeast20CheckpointRootRecord&) = default;
};

struct ExactDirectAtLeast20CheckpointOrderCursor {
  std::size_t order{};
  exact::ExactLevel last_squared_level{};

  friend bool operator==(
      const ExactDirectAtLeast20CheckpointOrderCursor&,
      const ExactDirectAtLeast20CheckpointOrderCursor&) = default;
};

struct ExactDirectAtLeast20StreamViewCheckpoint {
  std::uint32_t schema_version{
      direct_at_least20_stream_view_schema_version};
  std::uint64_t source_session_authority_id{};
  std::size_t next_source_batch_cursor{};
  std::size_t source_epoch{};
  contract::CanonicalId source_scientific_digest{};
  contract::CanonicalId source_commit_digest{};
  contract::CanonicalId view_digest{};
  std::vector<ExactDirectAtLeast20CheckpointRootRecord> roots;
  std::vector<ExactDirectAtLeast20CheckpointOrderCursor> order_cursors;
  std::size_t active_root_count{};
  std::size_t retained_point_reference_count{};
  contract::CanonicalId checkpoint_digest{};
  bool source_pruning_performed{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactDirectAtLeast20StreamViewCheckpoint&,
      const ExactDirectAtLeast20StreamViewCheckpoint&) = default;
};

enum class ExactDirectAtLeast20CheckpointDecision : std::uint8_t {
  not_built,
  no_session_not_initialized,
  no_checkpoint_budget_exhausted,
  no_checkpoint_shape_rejected,
  no_checkpoint_digest_rejected,
  no_allocation_failed,
  complete_certified_checkpoint,
};

struct ExactDirectAtLeast20CheckpointBuildResult {
  std::optional<ExactDirectAtLeast20StreamViewCheckpoint> checkpoint;
  ExactDirectAtLeast20CheckpointDecision decision{
      ExactDirectAtLeast20CheckpointDecision::not_built};

  [[nodiscard]] bool certified_checkpoint() const noexcept;
};

struct ExactDirectAtLeast20StreamViewSessionInitializationResult;
struct ExactDirectAtLeast20StreamViewPreparationResult;

// This ticket is issued only by a view session after every validation,
// capacity reservation, visible-transition allocation and digest computation
// has completed.  Callers can move or destroy it but cannot construct one or
// ask the view to commit it.  The resident adapter and the future normalized
// product coordinator are the only cross-component friends; neither can mint
// it from a public committed-batch transcript.
class ExactDirectAtLeast20StreamViewPreparedBatch {
 public:
  ExactDirectAtLeast20StreamViewPreparedBatch() noexcept;
  ~ExactDirectAtLeast20StreamViewPreparedBatch();
  ExactDirectAtLeast20StreamViewPreparedBatch(
      ExactDirectAtLeast20StreamViewPreparedBatch&&) noexcept;
  ExactDirectAtLeast20StreamViewPreparedBatch& operator=(
      ExactDirectAtLeast20StreamViewPreparedBatch&&) noexcept;
  ExactDirectAtLeast20StreamViewPreparedBatch(
      const ExactDirectAtLeast20StreamViewPreparedBatch&) = delete;
  ExactDirectAtLeast20StreamViewPreparedBatch& operator=(
      const ExactDirectAtLeast20StreamViewPreparedBatch&) = delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] bool consumed() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectAtLeast20StreamViewPreparedBatch(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectAtLeast20StreamViewSession;
};

struct ExactDirectAtLeast20StreamViewPreparationResult {
  std::optional<ExactDirectAtLeast20StreamViewPreparedBatch> ticket;
  ExactDirectAtLeast20StreamConsumeResult result{};
  bool no_scientific_state_mutated{false};
  bool all_allocations_and_digests_completed{false};
  bool resident_commit_capability_bound{false};

  [[nodiscard]] bool certified_prepared_batch() const noexcept;
};

// The session owns only the downstream cap-20 summaries, root tombstones,
// exact per-order level cursors and digest chain.  It never owns or filters a
// facet, coface, incidence, source batch or full coverage_log.
class ExactDirectAtLeast20StreamViewSession {
 public:
  static constexpr std::string_view backend =
      direct_at_least20_stream_view_backend;
  static constexpr std::string_view profile =
      direct_at_least20_stream_view_profile;
  static constexpr std::string_view mode = direct_at_least20_stream_view_mode;
  static constexpr std::string_view deployment_status =
      direct_at_least20_stream_view_deployment_status;
  static constexpr std::string_view public_status =
      direct_at_least20_stream_view_public_status;
  static constexpr std::string_view proof_basis =
      direct_at_least20_stream_view_proof_basis;
  static constexpr std::size_t min_cluster_size =
      direct_at_least20_stream_view_threshold;
  static constexpr bool source_pruning_authority = false;
  static constexpr bool source_hierarchy_authority = false;
  static constexpr bool upstream_resident_adapter_certified = false;
  static constexpr bool public_exactness_claimed = false;

  ExactDirectAtLeast20StreamViewSession() noexcept;
  ~ExactDirectAtLeast20StreamViewSession();
  ExactDirectAtLeast20StreamViewSession(
      ExactDirectAtLeast20StreamViewSession&&) noexcept;
  ExactDirectAtLeast20StreamViewSession& operator=(
      ExactDirectAtLeast20StreamViewSession&&) noexcept;
  ExactDirectAtLeast20StreamViewSession(
      const ExactDirectAtLeast20StreamViewSession&) = delete;
  ExactDirectAtLeast20StreamViewSession& operator=(
      const ExactDirectAtLeast20StreamViewSession&) = delete;

  [[nodiscard]] bool initialized() const noexcept;
  [[nodiscard]] std::uint64_t source_session_authority_id() const noexcept;
  [[nodiscard]] std::size_t next_source_batch_cursor() const noexcept;
  [[nodiscard]] std::size_t source_epoch() const noexcept;
  [[nodiscard]] std::size_t root_state_count() const noexcept;
  [[nodiscard]] std::size_t active_root_count() const noexcept;
  [[nodiscard]] std::size_t retained_point_reference_count() const noexcept;
  [[nodiscard]] const contract::CanonicalId& source_scientific_digest()
      const noexcept;
  [[nodiscard]] const contract::CanonicalId& source_commit_digest()
      const noexcept;
  [[nodiscard]] const contract::CanonicalId& view_digest() const noexcept;

  [[nodiscard]] ExactDirectAtLeast20StreamConsumeResult consume(
      ExactDirectAtLeast20CommittedBatch&& batch) noexcept;
  [[nodiscard]] ExactDirectAtLeast20CheckpointBuildResult make_checkpoint()
      const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectAtLeast20StreamViewSession(
      std::unique_ptr<Impl>) noexcept;

  [[nodiscard]] ExactDirectAtLeast20StreamViewPreparationResult
  prepare_relative_batch(
      ExactDirectAtLeast20CommittedBatch&& batch) noexcept;
  [[nodiscard]] ExactDirectAtLeast20StreamViewPreparationResult
  prepare_resident_batch(
      ExactDirectAtLeast20CommittedBatchInput&& input) noexcept;
  [[nodiscard]] ExactDirectAtLeast20StreamConsumeResult
  commit_prepared_batch(
      ExactDirectAtLeast20StreamViewPreparedBatch&& ticket,
      bool resident_commit_succeeded) noexcept;

  friend ExactDirectAtLeast20StreamViewSessionInitializationResult
  initialize_exact_direct_at_least20_stream_view_session(
      std::uint64_t,
      std::size_t,
      std::size_t,
      const contract::CanonicalId&,
      const contract::CanonicalId&,
      const ExactDirectAtLeast20StreamViewBudget&);
  friend ExactDirectAtLeast20StreamViewSessionInitializationResult
  restore_exact_direct_at_least20_stream_view_session(
      ExactDirectAtLeast20StreamViewCheckpoint&&,
      std::uint64_t,
      const ExactDirectAtLeast20StreamViewBudget&);
  friend class ExactDirectMorseResidentAtLeast20Adapter;
  // Reserved for the single owner that will compose resident horizontal,
  // K2->K1 vertical and downstream view commits without duplicating the
  // resident session.  This friendship exposes no implementation today.
  friend class ExactDirectMorseNormalizedH0ProductSession;
};

enum class ExactDirectAtLeast20SessionInitializationDecision
    : std::uint8_t {
  not_initialized,
  no_source_session_authority_id_rejected,
  no_source_scientific_digest_rejected,
  no_session_stamp_rejected,
  no_session_budget_rejected,
  no_checkpoint_shape_rejected,
  no_checkpoint_digest_rejected,
  no_allocation_failed,
  complete_initialized_empty_session,
  complete_restored_certified_session,
};

struct ExactDirectAtLeast20StreamViewSessionInitializationResult {
  std::optional<ExactDirectAtLeast20StreamViewSession> session;
  bool budget_preflight_certified{false};
  bool checkpoint_freshly_verified{false};
  bool source_pruning_enabled{false};
  bool upstream_resident_adapter_certified{false};
  ExactDirectAtLeast20SessionInitializationDecision decision{
      ExactDirectAtLeast20SessionInitializationDecision::not_initialized};

  [[nodiscard]] bool certified_initialized_session() const noexcept;
};

[[nodiscard]] ExactDirectAtLeast20StreamViewSessionInitializationResult
initialize_exact_direct_at_least20_stream_view_session(
    std::uint64_t source_session_authority_id,
    std::size_t next_source_batch_cursor,
    std::size_t source_epoch,
    const contract::CanonicalId& source_scientific_digest,
    const contract::CanonicalId& source_commit_digest,
    const ExactDirectAtLeast20StreamViewBudget& budget);

[[nodiscard]] ExactDirectAtLeast20StreamViewSessionInitializationResult
restore_exact_direct_at_least20_stream_view_session(
    ExactDirectAtLeast20StreamViewCheckpoint&& checkpoint,
    std::uint64_t reminted_source_session_authority_id,
    const ExactDirectAtLeast20StreamViewBudget& budget);

}  // namespace morsehgp3d::hierarchy
