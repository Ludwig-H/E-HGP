#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/hierarchy/k1_forest.hpp"
#include "morsehgp3d/hierarchy/reduced_gamma_history.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t exact_size_condensed_hierarchy_schema_version =
    1U;
inline constexpr const char* exact_size_condensed_hierarchy_proof_basis =
    "fresh_exact_source_replay_distinct_point_coverage_at_least_threshold_"
    "post_equal_level_batch_condensation_v1";
inline constexpr std::size_t
    exact_size_condensed_k1_maximum_fresh_reference_point_count = 64U;

enum class ExactSizeCondensedSourceKind : std::uint8_t {
  exact_k1_compact_forest,
  exact_persistent_reduced_gamma_order_history,
};

enum class ExactSizeCondensedCoverageRelation : std::uint8_t {
  at_least,
};

// threshold_entry is a visibility event introduced by condensation. It is
// deliberately not called a birth: the exact source component may have existed
// and grown through arbitrarily many hidden source batches before this event.
enum class ExactSizeCondensedComponentDecision : std::uint8_t {
  hidden,
  threshold_entry,
  continuation,
  multifusion,
};

enum class ExactSizeCondensedSourceTransitionKind : std::uint8_t {
  k1_initial_leaf,
  k1_equal_level_merge,
  gamma_birth,
  gamma_continuation,
  gamma_multifusion,
};

enum class ExactSizeCondensedHierarchyDecision : std::uint8_t {
  not_certified,
  complete_size_condensed_hierarchy,
  complete_empty_terminal_order,
};

enum class ExactSizeCondensedHierarchyScope : std::uint8_t {
  unspecified,
  exact_k1_or_bounded_n14_reduced_gamma_distinct_point_coverage_view,
};

struct ExactSizeCondensedSourceCounters {
  std::size_t point_count{};
  std::size_t order{};
  std::size_t source_level_count{};
  std::size_t source_batch_count{};
  std::size_t source_transition_count{};
  std::size_t source_deferred_group_count{};
  std::size_t source_node_count{};
  std::size_t source_child_reference_count{};
  std::size_t source_selected_edge_count{};
  std::size_t source_facet_count{};
  std::size_t source_coface_count{};
  std::size_t source_coverage_delta_count{};
  std::optional<K1CompactForestCounters> k1_compact_forest_counters;
  std::optional<ExactPersistentReducedGammaOrderHistoryCounters>
      reduced_gamma_history_counters;

  friend bool operator==(
      const ExactSizeCondensedSourceCounters&,
      const ExactSizeCondensedSourceCounters&) = default;
};

struct ExactSizeCondensedSourceProvenance {
  ExactSizeCondensedSourceKind source_kind{
      ExactSizeCondensedSourceKind::exact_k1_compact_forest};
  std::string source_proof_basis;
  contract::CanonicalId source_projection_digest{};
  ExactSizeCondensedSourceCounters source_counters{};
  bool source_complete_exact_history_certified{false};
  bool source_fresh_replay_certified{false};
  bool source_emst_minimality_reverified_by_condensation{false};
  bool source_emst_certification_inherited_from_input_contract{false};
  bool surrogate_v5_source_used{false};

  friend bool operator==(
      const ExactSizeCondensedSourceProvenance&,
      const ExactSizeCondensedSourceProvenance&) = default;
};

// Every non-deferred exact source transition receives one decision. Hidden
// records are retained only as a bounded certificate/audit projection; the
// public_component_record_indices arena identifies the actual condensed output.
struct ExactSizeCondensedComponentRecord {
  std::size_t decision_record_index{};
  std::size_t batch_index{};
  std::optional<std::size_t> source_batch_index;
  std::optional<std::size_t> source_group_record_index;
  ExactSizeCondensedSourceTransitionKind source_transition_kind{
      ExactSizeCondensedSourceTransitionKind::k1_equal_level_merge};
  exact::ExactLevel squared_level{};
  std::uint64_t source_resulting_root_node_id{};
  std::vector<std::uint64_t> source_prior_root_node_ids;
  std::size_t summed_prior_distinct_coverage_size{};
  std::size_t distinct_prior_coverage_union_size{};
  std::size_t distinct_coverage_size{};
  std::size_t q_visible{};
  std::vector<std::size_t> prior_visible_component_ids;
  std::optional<std::size_t> resulting_visible_component_id;
  std::optional<std::size_t> created_visible_component_id;
  ExactSizeCondensedComponentDecision decision{
      ExactSizeCondensedComponentDecision::hidden};
  bool decided_after_complete_source_batch{false};

  friend bool operator==(
      const ExactSizeCondensedComponentRecord&,
      const ExactSizeCondensedComponentRecord&) = default;
};

struct ExactSizeCondensedBatchMetadata {
  std::size_t batch_index{};
  std::optional<std::size_t> source_batch_index;
  exact::ExactLevel squared_level{};
  std::size_t first_decision_record_index{};
  std::size_t decision_record_count{};
  std::size_t source_transition_count{};
  std::size_t source_deferred_group_count{};
  std::size_t hidden_decision_count{};
  std::size_t threshold_entry_count{};
  std::size_t continuation_count{};
  std::size_t multifusion_count{};
  std::size_t visible_root_count_before{};
  std::size_t visible_root_count_after{};
  bool source_batch_staged_completely_before_decisions{false};
  bool source_batch_mutated_only_after_all_decisions{false};

  friend bool operator==(
      const ExactSizeCondensedBatchMetadata&,
      const ExactSizeCondensedBatchMetadata&) = default;
};

struct ExactSizeCondensedHierarchyCounters {
  std::size_t processed_source_batch_count{};
  std::size_t processed_source_transition_count{};
  std::size_t processed_source_deferred_group_count{};
  std::size_t decision_record_count{};
  std::size_t hidden_decision_count{};
  std::size_t threshold_entry_count{};
  std::size_t continuation_count{};
  std::size_t multifusion_count{};
  std::size_t public_component_record_count{};
  std::size_t condensed_component_id_count{};
  std::size_t condensed_child_reference_count{};
  std::size_t maximum_visible_merge_arity{};
  std::size_t final_source_root_count{};
  std::size_t final_visible_root_count{};

  friend bool operator==(
      const ExactSizeCondensedHierarchyCounters&,
      const ExactSizeCondensedHierarchyCounters&) = default;
};

struct ExactSizeCondensedHierarchy {
  static constexpr const char* proof_basis =
      exact_size_condensed_hierarchy_proof_basis;

  std::uint32_t schema_version{
      exact_size_condensed_hierarchy_schema_version};
  std::size_t min_cluster_size{20U};
  ExactSizeCondensedCoverageRelation coverage_relation{
      ExactSizeCondensedCoverageRelation::at_least};
  ExactSizeCondensedSourceProvenance provenance{};
  std::vector<ExactSizeCondensedComponentRecord> decision_records;
  std::vector<std::size_t> public_component_record_indices;
  std::vector<ExactSizeCondensedBatchMetadata> batch_metadata;
  std::vector<std::size_t> final_visible_component_ids;
  contract::CanonicalId condensed_projection_digest{};
  ExactSizeCondensedHierarchyCounters counters{};
  bool distinct_point_coverage_certified{false};
  bool at_least_threshold_relation_certified{false};
  bool all_source_batches_decided_atomically{false};
  bool hidden_growth_preserved{false};
  bool threshold_entries_are_not_morse_births{false};
  bool every_source_transition_accounted_exactly_once{false};
  bool condensed_genealogy_certified{false};
  // This adapter is a horizontal view of one exact order. For k>=2 its source
  // is hgp_reduced. It projects no vertical map and claims no full-pi0
  // equivalence (including when min_cluster_size<=order). Consequently this
  // object is never, by itself, a MorseHGP3DResult or a public-status result.
  bool horizontal_single_order_view_certified{false};
  bool vertical_maps_projected{false};
  bool full_pi0_equivalence_claimed{false};
  bool morsehgp3d_result_claimed{false};
  bool public_status_claimed{false};
  bool output_digest_certified{false};
  bool size_condensed_hierarchy_certified{false};
  ExactSizeCondensedHierarchyDecision decision{
      ExactSizeCondensedHierarchyDecision::not_certified};
  ExactSizeCondensedHierarchyScope scope{
      ExactSizeCondensedHierarchyScope::unspecified};

  friend bool operator==(
      const ExactSizeCondensedHierarchy&,
      const ExactSizeCondensedHierarchy&) = default;
};

struct ExactSizeCondensedHierarchyVerification {
  bool source_certified{false};
  bool parameters_certified{false};
  bool provenance_certified{false};
  bool source_counts_certified{false};
  bool batches_certified{false};
  bool distinct_coverage_certified{false};
  bool decisions_certified{false};
  bool genealogy_certified{false};
  bool counters_certified{false};
  bool digests_certified{false};
  bool result_facts_certified{false};
  bool fresh_replay_certified{false};
  bool exact_size_condensed_hierarchy_certified{false};

  friend bool operator==(
      const ExactSizeCondensedHierarchyVerification&,
      const ExactSizeCondensedHierarchyVerification&) = default;
};

// This deliberately bounded reference entry point does not inherit EMST trust.
// It rebuilds the exact complete-graph EMST from the cloud, reconstructs the
// compact forest, and compares all five compact arenas, levels, batches,
// weights and counters before condensation. The n<=64 cap prevents this oracle
// from becoming a scalable product architecture under another name.
[[nodiscard]] ExactSizeCondensedHierarchy build_exact_size_condensed_k1(
    const spatial::CanonicalPointCloud& cloud,
    const K1CompactForest& source,
    std::size_t min_cluster_size = 20U);

[[nodiscard]] ExactSizeCondensedHierarchyVerification
verify_exact_size_condensed_k1(
    const spatial::CanonicalPointCloud& cloud,
    const K1CompactForest& source,
    std::size_t min_cluster_size,
    const ExactSizeCondensedHierarchy& observed);

// The bounded n<=14 Gamma adapter does not trust stored certification flags.
// It freshly replays the official exact source verifier from cloud, order and
// budget before reading any source genealogy or coverage delta.
[[nodiscard]] ExactSizeCondensedHierarchy
build_exact_size_condensed_reduced_gamma(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactPersistentReducedGammaOrderHistoryBudget budget,
    const ExactPersistentReducedGammaOrderHistory& source,
    std::size_t min_cluster_size = 20U);

[[nodiscard]] ExactSizeCondensedHierarchyVerification
verify_exact_size_condensed_reduced_gamma(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactPersistentReducedGammaOrderHistoryBudget budget,
    const ExactPersistentReducedGammaOrderHistory& source,
    std::size_t min_cluster_size,
    const ExactSizeCondensedHierarchy& observed);

}  // namespace morsehgp3d::hierarchy
