#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_closure_adapter.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_index.hpp"
#include "morsehgp3d/hierarchy/direct_morse_vertical_target_proposal_adapter.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_forest_carrier_cut_replay_session_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_replay_session_backend =
        "reference_cpu";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_replay_session_profile =
        "hgp_reduced";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_replay_session_mode =
        "certified_monotone_forest_relative_closed_exact_cut_replay";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_replay_session_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_morse_forest_carrier_cut_replay_session_proof_basis =
        "accepted_self_contained_exact_direct_morse_forest_journal_global_"
        "locator_prefix_and_all_order_carrier_state_monotone_replay_groups_"
        "then_births_with_complete_pre_post_stamp_equality_and_synchronous_"
        "nonowning_cut_view_v1";

// The nested carrier budget bounds the dense all-order forest state and every
// journal scan.  The locator budget is trusted independently and must equal
// the construction budget recorded by the accepted source forest.  The five
// additional caps make the monotone global-prefix replay and its reusable
// per-batch scratch explicit.
struct ExactDirectMorseForestCarrierCutReplaySessionBudget {
  ExactDirectMorseForestCarrierCutIndexBudget carrier_state_budget{};
  ExactDirectSparsePositiveFacetLocatorBudget locator_budget{};
  std::size_t maximum_replayed_global_batch_count{};
  std::size_t maximum_replayed_locator_union_count{};
  std::size_t maximum_replayed_locator_binding_count{};
  std::size_t maximum_batch_group_plan_count{};
  std::size_t maximum_batch_group_carrier_reference_count{};
  std::size_t maximum_batch_group_prior_root_reference_count{};
  std::size_t maximum_batch_locator_union_scratch_count{};
  std::size_t maximum_batch_locator_binding_scratch_count{};

  friend bool operator==(
      const ExactDirectMorseForestCarrierCutReplaySessionBudget&,
      const ExactDirectMorseForestCarrierCutReplaySessionBudget&) = default;
};

struct ExactDirectMorseForestCarrierCutReplaySessionCounters {
  std::size_t forest_birth_record_scan_count{};
  std::size_t forest_node_scan_count{};
  std::size_t forest_batch_preflight_scan_count{};
  std::size_t replayed_global_batch_count{};
  std::size_t replayed_target_batch_count{};
  std::size_t replayed_atomic_group_count{};
  std::size_t replayed_saddle_count{};
  std::size_t replayed_arm_binding_count{};
  std::size_t replayed_child_reference_count{};
  std::size_t replayed_locator_union_count{};
  std::size_t replayed_locator_binding_count{};
  std::size_t locator_stamp_equality_check_count{};
  std::size_t component_parent_hop_count{};
  std::size_t exact_level_comparison_count{};
  std::size_t maximum_observed_exact_level_integer_bit_count{};
  std::size_t target_carrier_count{};
  std::size_t synchronous_view_visit_count{};

  friend bool operator==(
      const ExactDirectMorseForestCarrierCutReplaySessionCounters&,
      const ExactDirectMorseForestCarrierCutReplaySessionCounters&) =
      default;
};

enum class ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision
    : std::uint8_t {
  not_certified,
  no_replay_session_capacity_overflow,
  no_replay_session_allocation_failed,
  no_replay_session_budget_exhausted,
  no_replay_session_source_forest_rejected,
  no_replay_session_invalid_target_order,
  no_replay_session_locator_budget_mismatch,
  no_replay_session_locator_initialization_rejected,
  no_replay_session_source_forest_contradiction,
  no_replay_session_initial_locator_stamp_contradiction,
  complete_certified_replay_session_ready,
};

enum class ExactDirectMorseForestCarrierCutReplayAdvanceDecision
    : std::uint8_t {
  not_certified,
  no_replay_session_not_ready,
  no_replay_invalid_closed_squared_level,
  no_replay_nonmonotone_closed_cut,
  no_replay_capacity_overflow,
  no_replay_allocation_failed,
  no_replay_budget_exhausted,
  no_replay_source_forest_contradiction,
  no_replay_locator_commit_rejected,
  no_replay_locator_stamp_contradiction,
  complete_certified_forest_relative_closed_cut,
};

enum class ExactDirectMorseForestCarrierCutReplayScope : std::uint8_t {
  unspecified,
  one_target_order_live_locator_and_carrier_to_optional_reduced_root_view_at_monotone_closed_exact_cuts_only,
};

struct ExactDirectMorseForestCarrierCutReplayAdvanceResult {
  // Ephemeral in-session receipt only.  It authenticates the live replay
  // transition and frozen stamp shape, but no fresh verifier binds an
  // arbitrary deserialized level back to the committed forest prefix.
  std::uint32_t schema_version{
      direct_morse_forest_carrier_cut_replay_session_schema_version};
  std::size_t target_order{};
  exact::ExactLevel requested_closed_squared_level{};
  std::size_t committed_global_batch_prefix_count{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp frozen_locator_stamp{};
  ExactDirectMorseForestCarrierCutReplaySessionCounters cumulative_counters{};
  bool source_forest_prefix_freshly_replayed{false};
  bool every_strict_pre_and_committed_stamp_matched{false};
  bool groups_before_same_level_births_replayed{false};
  bool locator_unions_then_bindings_committed_atomically{false};
  bool carrier_state_matches_locator_canonical_parents{false};
  bool synchronous_view_invoked{false};
  bool locator_mutated_during_advance{false};
  bool session_poisoned{false};
  bool forest_relative_only{true};
  bool gamma_cells_or_global_cofaces_materialized{false};
  bool higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseForestCarrierCutReplayAdvanceDecision decision{
      ExactDirectMorseForestCarrierCutReplayAdvanceDecision::not_certified};
  ExactDirectMorseForestCarrierCutReplayScope scope{
      ExactDirectMorseForestCarrierCutReplayScope::unspecified};

  [[nodiscard]] bool certified_forest_relative_closed_cut() const noexcept;
  [[nodiscard]] bool certified_nonmutating_rejection() const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
};

class ExactDirectMorseForestCarrierCutReplaySession;
struct ExactDirectMorseForestCarrierCutReplaySessionInitialization;

// This object can only be constructed by an active session visit.  It cannot
// be copied or moved, and every operation is valid only for the dynamic extent
// of the consumer call.  The locator itself never escapes this epoch-checked
// facade.
class ExactDirectMorseForestCarrierCutReplayView {
 public:
  ExactDirectMorseForestCarrierCutReplayView(
      const ExactDirectMorseForestCarrierCutReplayView&) = delete;
  ExactDirectMorseForestCarrierCutReplayView& operator=(
      const ExactDirectMorseForestCarrierCutReplayView&) = delete;
  ExactDirectMorseForestCarrierCutReplayView(
      ExactDirectMorseForestCarrierCutReplayView&&) = delete;
  ExactDirectMorseForestCarrierCutReplayView& operator=(
      ExactDirectMorseForestCarrierCutReplayView&&) = delete;
  ~ExactDirectMorseForestCarrierCutReplayView() = default;

  [[nodiscard]] std::size_t target_order() const noexcept;
  [[nodiscard]] exact::ExactLevel closed_squared_level() const;
  [[nodiscard]] std::size_t committed_global_batch_prefix_count() const
      noexcept;
  [[nodiscard]] std::size_t target_carrier_count() const noexcept;
  [[nodiscard]] ExactDirectSparsePositiveFacetLocatorSnapshotStamp
  locator_snapshot_stamp() const noexcept;
  [[nodiscard]] ExactDirectSparsePositiveFacetProbeResult
  probe_positive_facet(
      const ExactDirectSparseFacetKey& key,
      const ExactDirectSparseFacetWitness& witness,
      const ExactDirectSparsePositiveFacetProbeBudget& budget) const
      noexcept;
  [[nodiscard]] ExactDirectMorseForestCarrierCutEntry entry_at(
      std::size_t target_entry_index) const;
  [[nodiscard]] std::optional<ExactDirectMorseForestCarrierCutEntry>
  find_entry(ExactDirectSparseComponentHandle component_handle) const;
  // index and cloud must be the immutable canonical PointId namespace that
  // produced the borrowed forest.  This adapter checks the LBVH/cloud pair and
  // the source point count; the forest journal does not retain a coordinate
  // digest capable of re-establishing a foreign cloud's identity.
  [[nodiscard]] ExactDirectMorseForestCarrierCutClosureAdapterResult
  build_closure_summary_from_canonical_distinct_keys(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::span<const ExactDirectSparseFacetKey>
          canonical_distinct_keys,
      const ExactDirectSparseFacetWitness& locator_query_witness,
      const ExactDirectMorseForestCarrierCutClosureAdapterBudget& budget,
      const ExactDirectSparseFacetDescentClosureConfig& closure_config = {},
      spatial::LbvhTraversalOrder traversal_order =
          spatial::LbvhTraversalOrder::near_first) const;
  [[nodiscard]] ExactDirectMorseVerticalTargetProposalAdapterResult
  build_vertical_target_proposals_from_group_plan(
      const ExactDirectMorseForestJournalResult& source_forest,
      const ExactDirectMorseVerticalTargetFacetPlanResult& plan,
      const ExactDirectMorseForestCarrierCutClosureAdapterResult&
          closure_summary,
      const ExactDirectMorseVerticalTargetProposalAdapterBudget& budget)
      const;

 private:
  friend class ExactDirectMorseForestCarrierCutReplaySession;

  explicit ExactDirectMorseForestCarrierCutReplayView(
      const ExactDirectMorseForestCarrierCutReplaySession& session,
      std::uint64_t visit_epoch) noexcept;

  const ExactDirectMorseForestCarrierCutReplaySession* session_{};
  std::uint64_t visit_epoch_{};
};

class ExactDirectMorseForestCarrierCutReplaySession {
 public:
  // Public only so the out-of-line implementation can name the opaque state
  // in allocation-free helper functions.  No definition is exposed here.
  struct Impl;

  static constexpr std::string_view backend =
      direct_morse_forest_carrier_cut_replay_session_backend;
  static constexpr std::string_view profile =
      direct_morse_forest_carrier_cut_replay_session_profile;
  static constexpr std::string_view mode =
      direct_morse_forest_carrier_cut_replay_session_mode;
  static constexpr std::string_view public_status =
      direct_morse_forest_carrier_cut_replay_session_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_forest_carrier_cut_replay_session_proof_basis;

  ~ExactDirectMorseForestCarrierCutReplaySession();
  ExactDirectMorseForestCarrierCutReplaySession(
      ExactDirectMorseForestCarrierCutReplaySession&&) = delete;
  ExactDirectMorseForestCarrierCutReplaySession& operator=(
      ExactDirectMorseForestCarrierCutReplaySession&&) = delete;
  ExactDirectMorseForestCarrierCutReplaySession(
      const ExactDirectMorseForestCarrierCutReplaySession&) = delete;
  ExactDirectMorseForestCarrierCutReplaySession& operator=(
      const ExactDirectMorseForestCarrierCutReplaySession&) = delete;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool poisoned() const noexcept;
  [[nodiscard]] bool has_frozen_cut() const noexcept;
  [[nodiscard]] std::size_t target_order() const noexcept;

  template <typename Consumer>
    requires std::is_object_v<std::remove_reference_t<Consumer>> &&
             std::invocable<
                 Consumer&,
                 const ExactDirectMorseForestCarrierCutReplayView&> &&
             std::same_as<
                 std::invoke_result_t<
                     Consumer&,
                     const ExactDirectMorseForestCarrierCutReplayView&>,
                 void>
  [[nodiscard]] ExactDirectMorseForestCarrierCutReplayAdvanceResult
  advance_to_closed_cut(
      const exact::ExactLevel& closed_squared_level,
      Consumer&& consumer) & {
    using ConsumerType = std::remove_reference_t<Consumer>;
    void* const erased_consumer = const_cast<void*>(
        static_cast<const void*>(std::addressof(consumer)));
    return advance_to_closed_cut_erased(
        closed_squared_level,
        erased_consumer,
        [](void* erased,
           const ExactDirectMorseForestCarrierCutReplayView& view) {
          std::invoke(*static_cast<ConsumerType*>(erased), view);
        });
  }

 private:
  using ErasedConsumer = void (*)(
      void*, const ExactDirectMorseForestCarrierCutReplayView&);
  enum class FrozenLocatorViewDecision : std::uint8_t {
    complete_stable_snapshot,
    session_not_ready,
    session_poisoned,
    stale_view_epoch,
    locator_snapshot_changed,
  };
  using FrozenLocatorViewConsumer = bool (*)(
      void*,
      std::size_t,
      std::size_t,
      const exact::ExactLevel&,
      const ExactDirectSparsePositiveFacetLocatorSnapshotStamp&,
      const ExactDirectSparsePositiveFacetLocator&);

  explicit ExactDirectMorseForestCarrierCutReplaySession(
      std::unique_ptr<Impl> impl) noexcept;

  [[nodiscard]] ExactDirectMorseForestCarrierCutReplayAdvanceResult
  advance_to_closed_cut_erased(
      const exact::ExactLevel& closed_squared_level,
      void* erased_consumer,
      ErasedConsumer consumer) &;

  [[nodiscard]] FrozenLocatorViewDecision view_with_frozen_locator(
      std::uint64_t epoch,
      void* erased_consumer,
      FrozenLocatorViewConsumer consumer) const;

  [[nodiscard]] bool view_epoch_is_live(std::uint64_t epoch) const noexcept;
  [[nodiscard]] bool view_borrows_source_forest(
      std::uint64_t epoch,
      const ExactDirectMorseForestJournalResult* source_forest) const
      noexcept;
  [[nodiscard]] exact::ExactLevel view_closed_squared_level(
      std::uint64_t epoch) const;
  [[nodiscard]] std::size_t view_committed_batch_prefix_count(
      std::uint64_t epoch) const noexcept;
  [[nodiscard]] std::size_t view_target_carrier_count(
      std::uint64_t epoch) const noexcept;
  [[nodiscard]] ExactDirectSparsePositiveFacetLocatorSnapshotStamp
  view_locator_snapshot_stamp(std::uint64_t epoch) const noexcept;
  [[nodiscard]] ExactDirectSparsePositiveFacetProbeResult
  view_probe_positive_facet(
      std::uint64_t epoch,
      const ExactDirectSparseFacetKey& key,
      const ExactDirectSparseFacetWitness& witness,
      const ExactDirectSparsePositiveFacetProbeBudget& budget) const
      noexcept;
  [[nodiscard]] ExactDirectMorseForestCarrierCutEntry view_entry_at(
      std::uint64_t epoch, std::size_t target_entry_index) const;
  [[nodiscard]] std::optional<ExactDirectMorseForestCarrierCutEntry>
  view_find_entry(
      std::uint64_t epoch,
      ExactDirectSparseComponentHandle component_handle) const;

  friend class ExactDirectMorseForestCarrierCutReplayView;
  friend struct ExactDirectMorseForestCarrierCutReplaySessionInitialization;
  friend ExactDirectMorseForestCarrierCutReplaySessionInitialization
  build_exact_direct_morse_forest_carrier_cut_replay_session(
      const ExactDirectMorseForestJournalResult&,
      std::size_t,
      const ExactDirectMorseForestCarrierCutReplaySessionBudget&);

  std::unique_ptr<Impl> impl_;
};

struct ExactDirectMorseForestCarrierCutReplaySessionInitialization {
  std::uint32_t schema_version{
      direct_morse_forest_carrier_cut_replay_session_schema_version};
  ExactDirectMorseForestCarrierCutReplaySessionBudget requested_budget{};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  std::size_t target_order{};
  ExactDirectMorseForestCarrierCutReplaySessionCounters counters{};
  bool source_forest_certified_outcome_accepted{false};
  bool budget_preflight_certified{false};
  bool global_batches_strictly_ordered{false};
  bool dense_component_and_node_state_initialized{false};
  bool target_carrier_partition_reconstructed{false};
  bool locator_initialized_from_trusted_recorded_budget{false};
  bool initial_locator_stamp_matched{false};
  bool source_forest_must_remain_immutable{true};
  bool forest_relative_only{true};
  bool gamma_cells_or_global_cofaces_materialized{false};
  bool higher_order_delaunay_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision
      decision{ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                   not_certified};
  ExactDirectMorseForestCarrierCutReplayScope scope{
      ExactDirectMorseForestCarrierCutReplayScope::unspecified};
  std::unique_ptr<ExactDirectMorseForestCarrierCutReplaySession> session;

  [[nodiscard]] bool certified_ready_session() const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
};

// The source forest is borrowed.  It must remain alive, unmoved and immutable
// until the returned session is destroyed.
[[nodiscard]] ExactDirectMorseForestCarrierCutReplaySessionInitialization
build_exact_direct_morse_forest_carrier_cut_replay_session(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::size_t target_order,
    const ExactDirectMorseForestCarrierCutReplaySessionBudget& budget);

ExactDirectMorseForestCarrierCutReplaySessionInitialization
build_exact_direct_morse_forest_carrier_cut_replay_session(
    ExactDirectMorseForestJournalResult&&,
    std::size_t,
    const ExactDirectMorseForestCarrierCutReplaySessionBudget&) = delete;

ExactDirectMorseForestCarrierCutReplaySessionInitialization
build_exact_direct_morse_forest_carrier_cut_replay_session(
    const ExactDirectMorseForestJournalResult&&,
    std::size_t,
    const ExactDirectMorseForestCarrierCutReplaySessionBudget&) = delete;

}  // namespace morsehgp3d::hierarchy
