#pragma once

#include "morsehgp3d/hierarchy/direct_saddle_arm_seed_journal.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_k1_forest_journal_schema_version = 1U;
inline constexpr std::string_view direct_k1_forest_journal_backend =
    "reference_cpu";
inline constexpr std::string_view direct_k1_forest_journal_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_k1_forest_journal_mode =
    "certified";
inline constexpr std::string_view direct_k1_forest_journal_refinement_status =
    "partial_refinement";
inline constexpr std::string_view direct_k1_forest_journal_public_status =
    "not_claimed";
inline constexpr std::string_view direct_k1_forest_journal_proof_basis =
    "terminal_rank_two_pair_arms_singleton_strict_roots_closed_diameter_"
    "strict_edge_descent_equal_level_atomic_quotient_k1_forest_v1";

using ExactDirectK1NodeId = std::uint64_t;

// Every cap is checked before the two O(n) DSU arrays or an output arena is
// allocated.  The equal-level scratch count is a logical bound over transient
// root, local-DSU and grouping entries, not allocator bytes.
struct ExactDirectK1ForestBudget {
  std::size_t maximum_source_replay_entry_count{};
  std::size_t maximum_point_scratch_entry_count{};
  std::size_t maximum_equal_level_scratch_entry_count{};
  std::size_t maximum_arm_root_binding_count{};
  std::size_t maximum_saddle_record_count{};
  std::size_t maximum_atomic_group_count{};
  std::size_t maximum_child_reference_count{};
  std::size_t maximum_batch_record_count{};

  friend bool operator==(
      const ExactDirectK1ForestBudget&,
      const ExactDirectK1ForestBudget&) = default;
};

struct ExactDirectK1ArmRootBinding {
  std::size_t binding_index{};
  std::size_t source_arm_seed_index{};
  std::size_t source_family_index{};
  spatial::PointId singleton_point_id{};
  ExactDirectK1NodeId pre_batch_root_node_id{};

  friend bool operator==(
      const ExactDirectK1ArmRootBinding&,
      const ExactDirectK1ArmRootBinding&) = default;
};

struct ExactDirectK1SaddleRecord {
  std::size_t saddle_record_index{};
  std::size_t source_family_index{};
  std::size_t source_event_index{};
  std::size_t source_journal_batch_index{};
  std::size_t binding_offset{};
  std::size_t binding_count{};
  std::size_t distinct_pre_batch_root_count{};
  std::size_t atomic_group_index{};

  friend bool operator==(
      const ExactDirectK1SaddleRecord&,
      const ExactDirectK1SaddleRecord&) = default;
};

// For q>=2, child_node_ids stores every distinct strict root and a new node is
// created.  For q=1, the child slice is empty and resulting_root_node_id is
// the preserved strict root; all contributing saddles remain explicit.
struct ExactDirectK1AtomicGroup {
  std::size_t atomic_group_index{};
  std::size_t batch_index{};
  std::size_t saddle_record_offset{};
  std::size_t saddle_record_count{};
  std::size_t child_offset{};
  std::size_t child_count{};
  std::optional<ExactDirectK1NodeId> created_node_id;
  ExactDirectK1NodeId resulting_root_node_id{};

  friend bool operator==(
      const ExactDirectK1AtomicGroup&,
      const ExactDirectK1AtomicGroup&) = default;
};

struct ExactDirectK1AtomicBatch {
  std::size_t batch_index{};
  std::size_t source_journal_batch_index{};
  exact::ExactLevel squared_level{};
  std::size_t saddle_record_offset{};
  std::size_t saddle_record_count{};
  std::size_t atomic_group_offset{};
  std::size_t atomic_group_count{};
  std::size_t strict_root_count{};
  std::size_t closed_root_count{};
  bool roots_resolved_from_frozen_pre_batch_snapshot{false};
  bool quotient_committed_atomically{false};

  friend bool operator==(
      const ExactDirectK1AtomicBatch&,
      const ExactDirectK1AtomicBatch&) = default;
};

struct ExactDirectK1ForestCounters {
  std::size_t order_one_family_count{};
  std::size_t arm_root_binding_count{};
  std::size_t saddle_record_count{};
  std::size_t atomic_group_count{};
  std::size_t continuation_group_count{};
  std::size_t created_node_count{};
  std::size_t multifusion_count{};
  std::size_t child_reference_count{};
  std::size_t batch_record_count{};
  std::size_t maximum_batch_saddle_count{};
  std::size_t maximum_merge_arity{};

  friend bool operator==(
      const ExactDirectK1ForestCounters&,
      const ExactDirectK1ForestCounters&) = default;
};

enum class ExactDirectK1ForestDecision : std::uint8_t {
  not_certified,
  no_k1_capacity_overflow,
  no_k1_budget_exhausted,
  no_k1_source_seed_journal_rejected,
  no_k1_rank_two_source_inconsistent,
  no_k1_quotient_did_not_close,
  complete_certified_direct_k1_forest_journal,
};

enum class ExactDirectK1ForestScope : std::uint8_t {
  unspecified,
  exact_order_one_forest_and_atomic_continuations_only,
};

struct ExactDirectK1ForestJournalResult {
  static constexpr std::string_view backend = direct_k1_forest_journal_backend;
  static constexpr std::string_view profile = direct_k1_forest_journal_profile;
  static constexpr std::string_view mode = direct_k1_forest_journal_mode;
  static constexpr std::string_view refinement_status =
      direct_k1_forest_journal_refinement_status;
  static constexpr std::string_view public_status =
      direct_k1_forest_journal_public_status;
  static constexpr std::string_view proof_basis =
      direct_k1_forest_journal_proof_basis;

  std::uint32_t schema_version{direct_k1_forest_journal_schema_version};
  ExactDirectK1ForestBudget requested_budget{};
  std::size_t point_count{};
  std::size_t source_direct_event_count{};
  std::size_t required_source_replay_entry_count{};
  std::size_t required_point_scratch_entry_count{};
  std::size_t required_equal_level_scratch_entry_count{};
  std::size_t required_arm_root_binding_count{};
  std::size_t required_saddle_record_count{};
  std::size_t required_atomic_group_capacity{};
  std::size_t required_child_reference_capacity{};
  std::size_t required_batch_record_capacity{};
  std::size_t logical_added_storage_entry_count{};
  std::size_t logical_added_storage_entry_limit{};
  std::size_t combined_logical_storage_entry_count{};
  std::size_t combined_logical_storage_entry_limit{};
  contract::CanonicalId source_pair_canonical_cloud_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};
  std::vector<ExactDirectK1ArmRootBinding> arm_root_bindings;
  std::vector<ExactDirectK1SaddleRecord> saddle_records;
  std::vector<ExactDirectK1AtomicGroup> atomic_groups;
  std::vector<ExactDirectK1NodeId> child_node_ids;
  std::vector<ExactDirectK1AtomicBatch> batches;
  ExactDirectK1NodeId root_node_id{};
  std::size_t node_count{};
  ExactDirectK1ForestCounters counters{};
  bool budget_preflight_certified{false};
  bool source_seed_journal_streaming_replayed{false};
  bool every_order_one_saddle_is_rank_two{false};
  bool every_order_one_arm_is_singleton{false};
  bool every_arm_bound_to_strict_pre_batch_root{false};
  bool equal_level_quotients_resolved_before_mutation{false};
  bool continuation_groups_preserved_without_new_node{false};
  bool closed_diameter_strict_edge_descent_theorem_applies{false};
  bool exact_order_one_forest_reduction_performed{false};
  bool output_linear_in_points_and_order_one_saddles{false};
  bool no_forbidden_global_structure_materialized{false};
  bool forbidden_global_geometry_computed{false};
  bool higher_order_root_localization_claimed{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};
  ExactDirectK1ForestDecision decision{
      ExactDirectK1ForestDecision::not_certified};
  ExactDirectK1ForestScope scope{ExactDirectK1ForestScope::unspecified};

  [[nodiscard]] bool certified_order_one_forest() const;

  friend bool operator==(
      const ExactDirectK1ForestJournalResult&,
      const ExactDirectK1ForestJournalResult&) = default;
};

[[nodiscard]] ExactDirectK1ForestJournalResult
build_exact_direct_k1_forest_journal(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectK1ForestBudget& budget);

struct ExactDirectK1ForestStreamingVerification {
  bool source_seed_journal_certified{false};
  bool requirements_certified{false};
  bool arm_root_bindings_certified{false};
  bool saddle_records_certified{false};
  bool atomic_groups_certified{false};
  bool child_references_certified{false};
  bool batches_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_and_scope_certified{false};
  bool no_second_persistent_output_arena_certified{false};
  bool fresh_streaming_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectK1ForestStreamingVerification&,
      const ExactDirectK1ForestStreamingVerification&) = default;
};

[[nodiscard]] ExactDirectK1ForestStreamingVerification
verify_exact_direct_k1_forest_journal_streaming(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectK1ForestBudget& trusted_budget,
    const ExactDirectK1ForestJournalResult& observed);

}  // namespace morsehgp3d::hierarchy
