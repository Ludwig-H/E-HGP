#pragma once

#include "morsehgp3d/hierarchy/reduced_gamma_batch.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

// This budget covers one complete, bounded, single-order history.  The work
// bounds deliberately charge one logical candidate-space traversal for the
// high cut and one for every possible facet/coface activation level.  They do
// not count the internal strict/closed replays or fresh-verifier executions
// performed by 6.8, 6.10 and 6.13.  They describe a correctness-oriented
// reference implementation and are not scalable production-work limits.
struct ExactPersistentReducedGammaOrderHistoryBudget {
  static constexpr std::size_t maximum_supported_activation_level_count =
      6435U;
  static constexpr std::size_t maximum_supported_total_facet_work_count =
      22088352U;
  static constexpr std::size_t maximum_supported_total_coface_work_count =
      22088352U;
  static constexpr std::size_t maximum_supported_total_union_work_count =
      135291156U;
  static constexpr std::size_t maximum_supported_node_count = 3432U;
  static constexpr std::size_t maximum_supported_child_reference_count =
      3431U;
  static constexpr std::size_t
      maximum_supported_group_root_reference_count = 3431U;
  static constexpr std::size_t maximum_supported_group_count = 6435U;
  static constexpr std::size_t
      maximum_supported_group_newly_active_facet_count = 3432U;
  static constexpr std::size_t
      maximum_supported_group_equal_level_coface_count = 3432U;
  static constexpr std::size_t maximum_supported_delta_facet_count = 3432U;
  static constexpr std::size_t
      maximum_supported_delta_point_reference_count = 24024U;
  static constexpr std::size_t maximum_supported_diameter_pair_count = 91U;

  ExactStrictGammaBudget gamma_budget{};
  std::size_t maximum_activation_level_count{};
  std::size_t maximum_total_facet_work_count{};
  std::size_t maximum_total_coface_work_count{};
  std::size_t maximum_total_union_work_count{};
  std::size_t maximum_node_count{};
  std::size_t maximum_child_reference_count{};
  std::size_t maximum_group_root_reference_count{};
  std::size_t maximum_group_count{};
  std::size_t maximum_group_newly_active_facet_count{};
  std::size_t maximum_group_equal_level_coface_count{};
  std::size_t maximum_delta_facet_count{};
  std::size_t maximum_delta_point_reference_count{};

  friend bool operator==(
      const ExactPersistentReducedGammaOrderHistoryBudget&,
      const ExactPersistentReducedGammaOrderHistoryBudget&) = default;
};

enum class ExactPersistentReducedGammaOrderHistoryDecision :
    std::uint8_t {
  not_certified,
  no_history_preflight_budget_insufficient,
  complete_persistent_reduced_gamma_history,
  complete_empty_terminal_order,
};

enum class ExactPersistentReducedGammaOrderHistoryScope : std::uint8_t {
  unspecified,
  bounded_n14_k10_single_order_persistent_hgp_reduced_gamma_history_including_empty_terminal_only,
};

enum class ExactPersistentReducedGammaNodeKind : std::uint8_t {
  birth,
  multifusion,
};

// A merge-tree edge is oriented from an older child to the newer parent node.
// Births have no children. Continuations retain an existing identifier and
// create no node. These identifiers are bounded local replay aids, never
// public or durable hierarchy IDs.
struct ExactPersistentReducedGammaNode {
  std::size_t node_id{};
  std::size_t creation_batch_index{};
  std::size_t creation_group_index{};
  exact::ExactLevel squared_level;
  ExactPersistentReducedGammaNodeKind kind{
      ExactPersistentReducedGammaNodeKind::birth};
  std::vector<std::size_t> child_node_ids;
  bool children_resolved_from_pre_batch_snapshot{false};

  friend bool operator==(
      const ExactPersistentReducedGammaNode&,
      const ExactPersistentReducedGammaNode&) = default;
};

// There is one compact record for every transient 6.13 group, including
// deferred facets and fully redundant resolved groups. A missing resulting
// root denotes only a deferred isolated facet. Across the complete journal,
// newly-active facet labels and equal-level coface labels have exact totals F
// and C; coverage deltas have at most F facet labels and kF point IDs.
struct ExactPersistentReducedGammaHistoryGroupRecord {
  std::size_t group_record_index{};
  std::size_t batch_index{};
  std::size_t batch_group_index{};
  exact::ExactLevel squared_level;
  ExactReducedGammaBatchGroupKind kind{
      ExactReducedGammaBatchGroupKind::deferred_isolated_facet};
  std::vector<spatial::PointId>
      canonical_representative_facet_point_ids;
  std::vector<std::size_t> prior_root_node_ids;
  std::optional<std::size_t> resulting_root_node_id;
  std::optional<std::size_t> created_node_id;
  std::vector<std::vector<spatial::PointId>>
      newly_active_facet_point_ids;
  std::vector<std::vector<spatial::PointId>>
      equal_level_coface_point_ids;
  std::optional<ExactReducedGammaCoverageDelta> coverage_delta;
  bool resolved_from_pre_batch_snapshot{false};

  friend bool operator==(
      const ExactPersistentReducedGammaHistoryGroupRecord&,
      const ExactPersistentReducedGammaHistoryGroupRecord&) = default;
};

struct ExactPersistentReducedGammaBatchMetadata {
  std::size_t batch_index{};
  std::size_t activation_level_index{};
  exact::ExactLevel squared_level;
  std::size_t first_group_record_index{};
  std::size_t group_record_count{};
  std::optional<std::size_t> first_created_node_id;
  std::size_t created_node_count{};
  std::size_t active_root_count_before{};
  std::size_t active_root_count_after{};
  std::size_t strict_nontrivial_component_count{};
  std::size_t closed_nontrivial_component_count{};
  bool pre_batch_root_bijection_certified{false};
  bool all_groups_resolved_before_mutation{false};
  bool post_batch_root_bijection_certified{false};

  friend bool operator==(
      const ExactPersistentReducedGammaBatchMetadata&,
      const ExactPersistentReducedGammaBatchMetadata&) = default;
};

struct ExactPersistentReducedGammaActiveRoot {
  std::size_t root_node_id{};
  std::vector<std::vector<spatial::PointId>> facet_point_ids;
  std::vector<spatial::PointId> covered_point_ids;

  friend bool operator==(
      const ExactPersistentReducedGammaActiveRoot&,
      const ExactPersistentReducedGammaActiveRoot&) = default;
};

struct ExactPersistentReducedGammaOrderHistoryCounters {
  std::size_t preflight_count{};
  std::size_t diameter_pair_distance_evaluation_count{};
  std::size_t high_cut_gamma_build_count{};
  std::size_t activation_facet_level_reference_count{};
  std::size_t activation_coface_level_reference_count{};
  std::size_t activation_level_count{};
  std::size_t reduced_gamma_batch_build_count{};
  std::size_t reduced_gamma_group_count{};
  std::size_t history_group_record_count{};
  std::size_t deferred_group_count{};
  std::size_t birth_group_count{};
  std::size_t continuation_group_count{};
  std::size_t multifusion_group_count{};
  std::size_t fully_redundant_group_count{};
  std::size_t group_root_reference_count{};
  std::size_t group_newly_active_facet_count{};
  std::size_t group_equal_level_coface_count{};
  std::size_t created_node_count{};
  std::size_t child_reference_count{};
  std::size_t consumed_child_count{};
  std::size_t pre_batch_root_bijection_check_count{};
  std::size_t post_batch_root_bijection_check_count{};
  std::size_t coverage_delta_count{};
  std::size_t added_facet_count{};
  std::size_t added_point_reference_count{};
  std::size_t total_facet_work_count{};
  std::size_t total_coface_work_count{};
  std::size_t total_union_work_count{};
  std::size_t final_active_root_count{};

  friend bool operator==(
      const ExactPersistentReducedGammaOrderHistoryCounters&,
      const ExactPersistentReducedGammaOrderHistoryCounters&) = default;
};

// Compact-output bounds for k<n are: one 6.8 high cut under gamma_budget;
// at most L=F+C levels and metadata records; at most L group records holding
// exactly F newly-active facet labels, C equal-level coface labels, at most
// C-1 prior-root references, F delta-facet labels and kF delta point IDs; at most C
// nodes and C-1 child references; and one final root with F facets and n
// covered points. The transient 6.13 batches themselves are never retained.
// Despite its mathematical name, this object is not durable persistence,
// catalogue provenance, a public-ID/SHA hierarchy, vertical/M.1/full-pi0
// output, a CUDA result, or a public-status decision.
struct ExactPersistentReducedGammaOrderHistory {
  static constexpr std::size_t minimum_supported_point_count = 2U;
  static constexpr std::size_t maximum_supported_point_count = 14U;
  static constexpr std::size_t minimum_supported_order = 2U;
  static constexpr std::size_t maximum_supported_order = 10U;
  static constexpr const char* proof_basis =
      "exact_bounded_exhaustive_gamma_all_exact_levels_persistent_"
      "reduced_root_genealogy_v1";

  ExactPersistentReducedGammaOrderHistoryBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  std::size_t exhaustive_facet_count{};
  std::size_t exhaustive_coface_count{};
  std::size_t exhaustive_union_attempt_count{};
  std::size_t required_activation_level_capacity{};
  std::size_t required_total_facet_work_capacity{};
  std::size_t required_total_coface_work_capacity{};
  std::size_t required_total_union_work_capacity{};
  std::size_t required_node_capacity{};
  std::size_t required_child_reference_capacity{};
  std::size_t required_group_root_reference_capacity{};
  std::size_t required_group_capacity{};
  std::size_t required_group_newly_active_facet_capacity{};
  std::size_t required_group_equal_level_coface_capacity{};
  std::size_t required_delta_facet_capacity{};
  std::size_t required_delta_point_reference_capacity{};
  std::optional<exact::ExactLevel> exact_diameter_squared;
  std::optional<exact::ExactLevel> high_cut_squared_level;
  std::optional<ExactStrictGammaResult> high_cut_gamma;
  std::vector<exact::ExactLevel> activation_levels;
  std::vector<ExactPersistentReducedGammaNode> nodes;
  std::vector<ExactPersistentReducedGammaHistoryGroupRecord>
      group_records;
  std::vector<ExactPersistentReducedGammaBatchMetadata> batch_metadata;
  std::vector<ExactPersistentReducedGammaActiveRoot> final_active_roots;
  bool candidate_space_size_certified{false};
  bool preflight_budget_sufficient{false};
  bool geometry_started_after_successful_preflight{false};
  bool high_cut_equals_twice_exact_squared_diameter{false};
  bool high_cut_strictly_above_all_activation_levels{false};
  bool high_cut_catalog_exhaustive{false};
  bool activation_levels_canonical_and_complete{false};
  bool all_reduced_gamma_batches_fresh_replay_certified{false};
  bool groups_resolved_against_pre_batch_snapshots{false};
  bool mutations_applied_after_complete_batch_resolution{false};
  bool active_roots_match_nontrivial_components_after_every_batch{false};
  bool node_ids_dense_and_deterministic{false};
  bool children_precede_parent{false};
  bool each_child_consumed_at_most_once{false};
  bool every_exhaustive_coface_affected_exactly_once{false};
  bool group_kinds_have_exact_persistent_effects{false};
  bool every_non_deferred_group_has_exactly_one_coverage_delta{false};
  bool fully_redundant_groups_preserved{false};
  bool coverage_deltas_accounted_exactly{false};
  bool final_single_root_covers_all_facets_and_points{false};
  bool terminal_order_complete_empty{false};
  bool persistent_reduced_gamma_history_certified{false};
  ExactPersistentReducedGammaOrderHistoryCounters counters{};
  ExactPersistentReducedGammaOrderHistoryDecision decision{
      ExactPersistentReducedGammaOrderHistoryDecision::not_certified};
  ExactPersistentReducedGammaOrderHistoryScope scope{
      ExactPersistentReducedGammaOrderHistoryScope::unspecified};

  friend bool operator==(
      const ExactPersistentReducedGammaOrderHistory&,
      const ExactPersistentReducedGammaOrderHistory&) = default;
};

struct ExactPersistentReducedGammaOrderHistoryVerification {
  bool requested_budget_certified{false};
  bool external_inputs_certified{false};
  bool derived_preflight_sizes_certified{false};
  bool high_cut_gamma_certified{false};
  bool activation_levels_certified{false};
  bool transient_reduced_gamma_batches_replayed_certified{false};
  bool nodes_certified{false};
  bool group_records_certified{false};
  bool batch_metadata_certified{false};
  bool final_active_roots_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_persistent_reduced_gamma_order_history_decision_certified{
      false};
};

[[nodiscard]] ExactPersistentReducedGammaOrderHistory
build_exact_persistent_reduced_gamma_order_history(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactPersistentReducedGammaOrderHistoryBudget budget);

// Reconstructs the high cut and transiently replays every 6.13 batch from only
// the external cloud, order and trusted budget. Stored node IDs or journal
// layers never steer this replay. This bounded compact journal is not durable
// persistence, a 6.12 catalogue provenance, a public-ID/SHA layer, a vertical
// or M.1 artifact, a full-pi0 product, a CUDA backend, or a public-status
// decision.
[[nodiscard]] ExactPersistentReducedGammaOrderHistoryVerification
verify_exact_persistent_reduced_gamma_order_history(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactPersistentReducedGammaOrderHistoryBudget budget,
    const ExactPersistentReducedGammaOrderHistory& history);

}  // namespace morsehgp3d::hierarchy
