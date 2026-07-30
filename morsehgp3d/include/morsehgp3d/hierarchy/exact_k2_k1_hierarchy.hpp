#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/hierarchy/k1_forest.hpp"
#include "morsehgp3d/hierarchy/reduced_gamma_history.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t exact_k2_k1_hierarchy_schema_version = 1U;
inline constexpr std::string_view exact_k2_k1_hierarchy_backend =
    "reference_cpu";
inline constexpr std::string_view exact_k2_k1_hierarchy_profile =
    "hgp_reduced";
inline constexpr std::string_view exact_k2_k1_hierarchy_mode =
    "bounded_exact_gamma2_to_emst_k1_n14";
inline constexpr std::string_view exact_k2_k1_hierarchy_public_status =
    "not_claimed";
inline constexpr std::string_view exact_k2_k1_hierarchy_deployment_status =
    "architecture_only_implemented";
inline constexpr std::string_view exact_k2_k1_hierarchy_proof_basis =
    "fresh_bounded_exhaustive_gamma2_history_and_fresh_complete_graph_emst_"
    "compact_k1_strict_open_closed_target_replay_v1";

// This budget covers only the bounded k=2 -> k=1 projection. The trusted
// source-history budget remains a separate input because no capacity echoed
// by an observed history may steer its fresh geometric replay.
struct ExactK2K1HierarchyBudget {
  static constexpr std::size_t maximum_supported_point_count = 14U;
  static constexpr std::size_t maximum_supported_source_batch_count = 455U;
  static constexpr std::size_t maximum_supported_source_group_count = 455U;
  static constexpr std::size_t maximum_supported_source_node_count = 364U;
  static constexpr std::size_t
      maximum_supported_source_child_reference_count = 363U;
  static constexpr std::size_t
      maximum_supported_source_root_reference_count = 363U;
  static constexpr std::size_t maximum_supported_checkpoint_count = 364U;
  // For n<=14 and k=2 there are at most F=91 facets, C=364 cofaces,
  // L_G<=F+C=455 Gamma levels and L_1<=n-1=13 K1 levels. The sweep visits
  // both strict and closed cuts of their union, and checkpoint construction
  // can replay every facet once more. The hard work caps dominate
  // 2*(L_G+L_1)*F+C*F facet visits and twice as many endpoint lookups.
  static constexpr std::size_t
      maximum_supported_source_facet_replay_count = 200000U;
  static constexpr std::size_t
      maximum_supported_vertical_endpoint_lookup_count = 400000U;
  static constexpr std::size_t
      maximum_supported_k1_parent_hop_count = 8000000U;
  static constexpr std::size_t
      maximum_supported_target_coverage_point_reference_count = 5000000U;
  static constexpr std::size_t
      maximum_supported_digest_logical_entry_count = 1000000U;
  static constexpr std::size_t
      maximum_supported_digest_exact_text_byte_count = 33554432U;

  std::size_t maximum_source_batch_count{};
  std::size_t maximum_source_group_count{};
  std::size_t maximum_source_node_count{};
  std::size_t maximum_source_child_reference_count{};
  std::size_t maximum_source_root_reference_count{};
  std::size_t maximum_checkpoint_count{};
  std::size_t maximum_source_facet_replay_count{};
  std::size_t maximum_vertical_endpoint_lookup_count{};
  std::size_t maximum_k1_parent_hop_count{};
  std::size_t maximum_target_coverage_point_reference_count{};
  std::size_t maximum_digest_logical_entry_count{};
  std::size_t maximum_digest_exact_text_byte_count{};

  friend bool operator==(
      const ExactK2K1HierarchyBudget&,
      const ExactK2K1HierarchyBudget&) = default;
};

enum class ExactK2K1VerticalCheckpointKind : std::uint8_t {
  birth,
  continuation,
  multifusion,
};

// Every non-deferred Gamma_2 group has exactly one checkpoint. Strict targets
// are ordered like the source group's prior roots and live in the open cut
// immediately before squared_level. The output target is the unique compact
// k=1 root in the closed cut at squared_level. Continuation checkpoints are
// retained even when their source coverage delta is empty.
struct ExactK2K1VerticalCheckpoint {
  std::size_t checkpoint_index{};
  std::size_t source_batch_index{};
  std::size_t source_group_record_index{};
  std::size_t source_batch_group_index{};
  exact::ExactLevel squared_level{};
  std::size_t source_root_node_id{};
  std::vector<K1NodeId> strict_prior_target_node_ids;
  K1NodeId closed_target_node_id{};
  ExactK2K1VerticalCheckpointKind kind{
      ExactK2K1VerticalCheckpointKind::birth};
  std::size_t prior_source_root_count{};
  std::size_t source_facet_count{};
  std::size_t source_covered_point_count{};
  std::size_t closed_target_covered_point_count{};
  std::size_t added_facet_count{};
  std::size_t added_point_count{};
  bool source_group_resolved_from_frozen_snapshot{false};
  bool every_prior_root_has_pre_batch_checkpoint{false};
  bool prior_targets_normalize_to_strict_open_targets{false};
  bool strict_open_targets_normalize_to_closed_target{false};
  bool all_source_facets_map_to_closed_target{false};
  bool source_coverage_contained_in_closed_target{false};

  friend bool operator==(
      const ExactK2K1VerticalCheckpoint&,
      const ExactK2K1VerticalCheckpoint&) = default;
};

struct ExactK2K1HierarchyCounters {
  std::size_t source_batch_count{};
  std::size_t source_group_count{};
  std::size_t source_node_count{};
  std::size_t source_child_reference_count{};
  std::size_t source_root_reference_count{};
  std::size_t deferred_group_count{};
  std::size_t birth_checkpoint_count{};
  std::size_t continuation_checkpoint_count{};
  std::size_t multifusion_checkpoint_count{};
  std::size_t fully_redundant_checkpoint_count{};
  std::size_t checkpoint_count{};
  std::size_t source_facet_replay_count{};
  std::size_t vertical_endpoint_lookup_count{};
  std::size_t k1_parent_hop_count{};
  std::size_t exact_level_comparison_count{};
  std::size_t target_coverage_point_reference_count{};
  std::size_t strict_open_target_reference_count{};
  std::size_t strict_open_normalization_check_count{};
  std::size_t strict_to_closed_naturality_check_count{};
  std::size_t k1_target_normalization_update_count{};
  std::size_t prior_target_naturality_check_count{};
  std::size_t consumed_source_root_count{};
  std::size_t produced_source_root_count{};
  std::size_t final_source_root_count{};
  std::size_t digest_logical_entry_count{};
  std::size_t digest_exact_text_byte_count{};

  friend bool operator==(
      const ExactK2K1HierarchyCounters&,
      const ExactK2K1HierarchyCounters&) = default;
};

struct ExactK2K1HierarchyProvenance {
  std::string source_horizontal_proof_basis;
  std::string k1_target_proof_basis;
  contract::CanonicalId canonical_cloud_digest{};
  contract::CanonicalId source_horizontal_projection_digest{};
  contract::CanonicalId k1_compact_projection_digest{};
  ExactPersistentReducedGammaOrderHistoryCounters source_history_counters{};
  K1CompactForestCounters k1_forest_counters{};
  bool source_history_fresh_replay_certified{false};
  bool source_horizontal_projection_complete{false};
  bool k1_complete_graph_emst_fresh_replay_certified{false};
  bool k1_compact_forest_fresh_replay_certified{false};
  bool conditional_direct_morse_authority_not_used{false};

  friend bool operator==(
      const ExactK2K1HierarchyProvenance&,
      const ExactK2K1HierarchyProvenance&) = default;
};

enum class ExactK2K1HierarchyDecision : std::uint8_t {
  not_certified,
  no_source_history_rejected,
  no_k1_forest_rejected,
  no_projection_preflight_budget_insufficient,
  complete_empty_terminal_order,
  complete_bounded_exact_k2_k1_hierarchy,
};

enum class ExactK2K1HierarchyScope : std::uint8_t {
  unspecified,
  bounded_n14_exact_exhaustive_gamma2_to_complete_graph_emst_k1_only,
};

// The horizontal hierarchy remains owned by the freshly replayed source. This
// result persists a complete canonical digest and all source counters instead
// of retaining a second copy of its potentially large arenas. The bounded
// verifier does transiently reconstruct source-root states. It is not durable,
// is not a scalable provider, does not certify M.1 beyond this adjacent-order
// family, and cannot be promoted to a 50k or public product result.
struct ExactK2K1HierarchyResult {
  static constexpr std::string_view backend = exact_k2_k1_hierarchy_backend;
  static constexpr std::string_view profile = exact_k2_k1_hierarchy_profile;
  static constexpr std::string_view mode = exact_k2_k1_hierarchy_mode;
  static constexpr std::string_view public_status =
      exact_k2_k1_hierarchy_public_status;
  static constexpr std::string_view deployment_status =
      exact_k2_k1_hierarchy_deployment_status;
  static constexpr std::string_view proof_basis =
      exact_k2_k1_hierarchy_proof_basis;

  std::uint32_t schema_version{exact_k2_k1_hierarchy_schema_version};
  ExactK2K1HierarchyBudget requested_budget{};
  std::size_t point_count{};
  std::size_t source_order{2U};
  std::size_t target_order{1U};
  std::size_t required_source_batch_capacity{};
  std::size_t required_source_group_capacity{};
  std::size_t required_source_node_capacity{};
  std::size_t required_source_child_reference_capacity{};
  std::size_t required_source_root_reference_capacity{};
  std::size_t required_checkpoint_capacity{};
  // These four values are conservative arithmetic upper bounds certified
  // before the union sweep. Unlike the structural and digest capacities,
  // they need not equal the smaller observed work counters.
  std::size_t preflight_source_facet_replay_upper_bound{};
  std::size_t preflight_vertical_endpoint_lookup_upper_bound{};
  std::size_t preflight_k1_parent_hop_upper_bound{};
  std::size_t preflight_target_coverage_point_reference_upper_bound{};
  std::size_t required_digest_logical_entry_capacity{};
  std::size_t required_digest_exact_text_byte_capacity{};
  ExactK2K1HierarchyProvenance provenance{};
  std::vector<ExactK2K1VerticalCheckpoint> vertical_checkpoints;
  // The verifier recomputes this projection from the observed payload. The
  // digest field and the two derived digest/result self-certifications are
  // excluded. All other semantic attestations are included, and the fresh
  // verifier compares the two excluded booleans independently.
  contract::CanonicalId hierarchy_projection_digest{};
  ExactK2K1HierarchyCounters counters{};
  bool source_and_target_preflight_sizes_derived_from_fresh_replay{false};
  bool projection_budget_preflight_certified{false};
  bool all_source_batches_staged_before_root_mutation{false};
  bool every_non_deferred_group_has_one_checkpoint{false};
  bool every_checkpoint_uses_strict_open_and_closed_exact_k1_targets{false};
  bool source_facets_and_point_coverage_replayed_exactly{false};
  bool strict_open_closed_vertical_naturality_certified{false};
  bool k1_only_interlevel_target_evolution_certified{false};
  bool vertical_map_complete_for_bounded_hgp_reduced_k2_to_k1_source{false};
  bool all_order_vertical_maps_complete{false};
  bool horizontal_source_persisted_in_result{false};
  bool bounded_exhaustive_gamma_source_materialized{false};
  bool product_architecture_claimed{false};
  bool scalable_50k_claimed{false};
  bool global_m1_claimed{false};
  bool public_status_claimed{false};
  bool output_digest_certified{false};
  bool exact_k2_k1_hierarchy_certified{false};
  bool failure_payload_empty{false};
  bool atomic_failure{false};
  ExactK2K1HierarchyDecision decision{
      ExactK2K1HierarchyDecision::not_certified};
  ExactK2K1HierarchyScope scope{ExactK2K1HierarchyScope::unspecified};

  // These predicates only check the self-contained receipt (including its
  // success digest). Scientific authority belongs exclusively to the fresh
  // verifier below, which rebuilds both horizontal inputs from the cloud.
  [[nodiscard]] bool well_formed_success_receipt() const noexcept;
  [[nodiscard]] bool well_formed_fail_closed_receipt() const noexcept;

  friend bool operator==(
      const ExactK2K1HierarchyResult&,
      const ExactK2K1HierarchyResult&) = default;
};

struct ExactK2K1HierarchyVerification {
  bool trusted_inputs_accepted{false};
  bool source_history_freshly_replayed{false};
  bool k1_emst_and_compact_forest_freshly_replayed{false};
  bool parameters_certified{false};
  bool required_capacities_certified{false};
  bool provenance_certified{false};
  bool checkpoints_certified{false};
  bool counters_certified{false};
  bool result_facts_certified{false};
  bool success_digest_recomputed{false};
  // Atomic failures deliberately carry no scientific digest or payload.
  bool failure_digest_absence_certified{false};
  bool failure_has_no_partial_scientific_payload{false};
  bool fresh_replay_certified{false};
  bool exact_k2_k1_hierarchy_decision_certified{false};

  friend bool operator==(
      const ExactK2K1HierarchyVerification&,
      const ExactK2K1HierarchyVerification&) = default;
};

[[nodiscard]] ExactK2K1HierarchyResult build_exact_k2_k1_hierarchy(
    const spatial::CanonicalPointCloud& cloud,
    ExactPersistentReducedGammaOrderHistoryBudget trusted_source_budget,
    const ExactPersistentReducedGammaOrderHistory& source_history,
    const K1CompactForest& target_k1_forest,
    const ExactK2K1HierarchyBudget& budget);

[[nodiscard]] ExactK2K1HierarchyVerification verify_exact_k2_k1_hierarchy(
    const spatial::CanonicalPointCloud& cloud,
    ExactPersistentReducedGammaOrderHistoryBudget trusted_source_budget,
    const ExactPersistentReducedGammaOrderHistory& source_history,
    const K1CompactForest& target_k1_forest,
    const ExactK2K1HierarchyBudget& trusted_budget,
    const ExactK2K1HierarchyResult& observed);

}  // namespace morsehgp3d::hierarchy
