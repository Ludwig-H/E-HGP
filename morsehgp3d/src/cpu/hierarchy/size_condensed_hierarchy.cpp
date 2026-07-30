#include "morsehgp3d/hierarchy/size_condensed_hierarchy.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using SourceRootId = std::uint64_t;

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value, const char* message) {
  if constexpr (sizeof(std::size_t) > sizeof(std::uint64_t)) {
    if (value > static_cast<std::size_t>(
                    std::numeric_limits<std::uint64_t>::max())) {
      throw std::length_error(message);
    }
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t checked_add(
    std::size_t left, std::size_t right, const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] SourceRootId source_root_id(std::size_t value) {
  return checked_u64(value, "a source root identifier exceeds uint64");
}

[[nodiscard]] bool same_k1_compact_forest(
    const K1CompactForest& left, const K1CompactForest& right) {
  return left.point_count == right.point_count && left.levels == right.levels &&
         left.selected_edges == right.selected_edges &&
         left.merge_nodes == right.merge_nodes &&
         left.child_ids == right.child_ids &&
         left.equal_level_batches == right.equal_level_batches &&
         left.root_node_id == right.root_node_id &&
         left.total_squared_weight == right.total_squared_weight &&
         left.total_hgp_weight == right.total_hgp_weight &&
         left.counters == right.counters;
}

[[nodiscard]] std::size_t visible_root_count_k1(
    const std::map<SourceRootId, std::pair<std::size_t, std::optional<std::size_t>>>&
        roots) {
  return static_cast<std::size_t>(std::count_if(
      roots.begin(), roots.end(), [](const auto& entry) {
        return entry.second.second.has_value();
      }));
}

struct GammaRootState {
  std::set<spatial::PointId> covered_point_ids;
  std::optional<std::size_t> visible_component_id;
};

[[nodiscard]] std::size_t visible_root_count_gamma(
    const std::map<SourceRootId, GammaRootState>& roots) {
  return static_cast<std::size_t>(std::count_if(
      roots.begin(), roots.end(), [](const auto& entry) {
        return entry.second.visible_component_id.has_value();
      }));
}

class DigestWriter {
 public:
  explicit DigestWriter(std::string_view domain) {
    text(domain);
  }

  void byte(std::uint8_t value) {
    const std::array<std::uint8_t, 1U> bytes{value};
    builder_.update(bytes);
  }

  void boolean(bool value) {
    byte(value ? std::uint8_t{1U} : std::uint8_t{0U});
  }

  void u64(std::uint64_t value) {
    std::array<std::uint8_t, 8U> bytes{};
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
      const std::size_t shift = (bytes.size() - 1U - index) * 8U;
      bytes[index] = static_cast<std::uint8_t>(value >> shift);
    }
    builder_.update(bytes);
  }

  void size(std::size_t value) {
    u64(checked_u64(value, "a digest size exceeds uint64"));
  }

  void text(std::string_view value) {
    size(value.size());
    builder_.update(value);
  }

  void identifier(const contract::CanonicalId& value) {
    builder_.update(value.bytes());
  }

  void level(const exact::ExactLevel& value) {
    text(value.numerator_string());
    text(value.denominator_string());
  }

  template <typename Enum>
  void enumeration(Enum value) {
    byte(static_cast<std::uint8_t>(value));
  }

  [[nodiscard]] contract::CanonicalId finalize() {
    return builder_.finalize();
  }

 private:
  contract::CanonicalSha256Builder builder_;
};

void append_k1_counters(
    DigestWriter& writer, const K1CompactForestCounters& counters) {
  writer.size(counters.point_count);
  writer.size(counters.input_edge_count);
  writer.size(counters.selected_edge_count);
  writer.size(counters.batched_selected_edge_count);
  writer.size(counters.distinct_level_count);
  writer.size(counters.equal_level_batch_count);
  writer.size(counters.merge_node_count);
  writer.size(counters.batched_merge_node_count);
  writer.size(counters.child_reference_count);
  writer.size(counters.total_node_count);
  writer.size(counters.merge_event_count);
  writer.size(counters.multifusion_count);
  writer.size(counters.max_merge_arity);
  writer.size(counters.root_coverage_size);
  writer.size(counters.stored_coverage_point_id_count);
  writer.size(counters.linear_storage_entry_count);
  writer.size(counters.linear_storage_entry_limit);
}

void append_gamma_counters(
    DigestWriter& writer,
    const ExactPersistentReducedGammaOrderHistoryCounters& counters) {
  writer.size(counters.preflight_count);
  writer.size(counters.diameter_pair_distance_evaluation_count);
  writer.size(counters.high_cut_gamma_build_count);
  writer.size(counters.activation_facet_level_reference_count);
  writer.size(counters.activation_coface_level_reference_count);
  writer.size(counters.activation_level_count);
  writer.size(counters.reduced_gamma_batch_build_count);
  writer.size(counters.reduced_gamma_group_count);
  writer.size(counters.history_group_record_count);
  writer.size(counters.deferred_group_count);
  writer.size(counters.birth_group_count);
  writer.size(counters.continuation_group_count);
  writer.size(counters.multifusion_group_count);
  writer.size(counters.fully_redundant_group_count);
  writer.size(counters.group_root_reference_count);
  writer.size(counters.group_newly_active_facet_count);
  writer.size(counters.group_equal_level_coface_count);
  writer.size(counters.created_node_count);
  writer.size(counters.child_reference_count);
  writer.size(counters.consumed_child_count);
  writer.size(counters.pre_batch_root_bijection_check_count);
  writer.size(counters.post_batch_root_bijection_check_count);
  writer.size(counters.coverage_delta_count);
  writer.size(counters.added_facet_count);
  writer.size(counters.added_point_reference_count);
  writer.size(counters.total_facet_work_count);
  writer.size(counters.total_coface_work_count);
  writer.size(counters.total_union_work_count);
  writer.size(counters.final_active_root_count);
}

void append_point_id_vector(
    DigestWriter& writer, std::span<const spatial::PointId> point_ids) {
  writer.size(point_ids.size());
  for (const spatial::PointId point_id : point_ids) {
    writer.u64(static_cast<std::uint64_t>(point_id));
  }
}

void append_facet_vector(
    DigestWriter& writer,
    const std::vector<std::vector<spatial::PointId>>& facets) {
  writer.size(facets.size());
  for (const auto& facet : facets) {
    append_point_id_vector(writer, facet);
  }
}

[[nodiscard]] contract::CanonicalId k1_source_projection_digest(
    const K1CompactForest& source) {
  DigestWriter writer{
      "MorseHGP3D/exact-size-condensation/k1-source-projection/v1"};
  writer.size(source.point_count);
  writer.size(source.levels.size());
  for (const exact::ExactLevel& level : source.levels) {
    writer.level(level);
  }
  writer.size(source.selected_edges.size());
  for (const ExactEmstEdge& edge : source.selected_edges) {
    writer.u64(static_cast<std::uint64_t>(edge.u));
    writer.u64(static_cast<std::uint64_t>(edge.v));
    writer.level(edge.squared_length);
    writer.level(edge.merge_level);
  }
  writer.size(source.merge_nodes.size());
  for (const K1CompactMergeNode& node : source.merge_nodes) {
    writer.u64(node.id);
    writer.size(node.level_index);
    writer.size(node.child_offset);
    writer.size(node.child_count);
    writer.u64(static_cast<std::uint64_t>(node.minimum_point_id));
    writer.size(node.coverage_size);
  }
  writer.size(source.child_ids.size());
  for (const K1NodeId child_id : source.child_ids) {
    writer.u64(child_id);
  }
  writer.size(source.equal_level_batches.size());
  for (const K1CompactEqualLevelBatch& batch : source.equal_level_batches) {
    writer.size(batch.level_index);
    writer.size(batch.selected_edge_offset);
    writer.size(batch.selected_edge_count);
    writer.size(batch.merge_node_offset);
    writer.size(batch.merge_node_count);
    writer.size(batch.pre_batch_component_count);
    writer.size(batch.post_batch_component_count);
  }
  writer.u64(source.root_node_id);
  writer.level(source.total_squared_weight);
  writer.level(source.total_hgp_weight);
  append_k1_counters(writer, source.counters);
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId gamma_source_projection_digest(
    const ExactPersistentReducedGammaOrderHistory& source) {
  DigestWriter writer{
      "MorseHGP3D/exact-size-condensation/reduced-gamma-source-projection/v1"};
  writer.text(ExactPersistentReducedGammaOrderHistory::proof_basis);
  writer.size(source.point_count);
  writer.size(source.order);
  writer.size(source.exhaustive_facet_count);
  writer.size(source.exhaustive_coface_count);
  writer.size(source.exhaustive_union_attempt_count);
  writer.size(source.activation_levels.size());
  for (const exact::ExactLevel& level : source.activation_levels) {
    writer.level(level);
  }
  writer.size(source.nodes.size());
  for (const ExactPersistentReducedGammaNode& node : source.nodes) {
    writer.size(node.node_id);
    writer.size(node.creation_batch_index);
    writer.size(node.creation_group_index);
    writer.level(node.squared_level);
    writer.enumeration(node.kind);
    writer.size(node.child_node_ids.size());
    for (const std::size_t child_id : node.child_node_ids) {
      writer.size(child_id);
    }
    writer.boolean(node.children_resolved_from_pre_batch_snapshot);
  }
  writer.size(source.group_records.size());
  for (const ExactPersistentReducedGammaHistoryGroupRecord& record :
       source.group_records) {
    writer.size(record.group_record_index);
    writer.size(record.batch_index);
    writer.size(record.batch_group_index);
    writer.level(record.squared_level);
    writer.enumeration(record.kind);
    append_point_id_vector(
        writer, record.canonical_representative_facet_point_ids);
    writer.size(record.prior_root_node_ids.size());
    for (const std::size_t root_id : record.prior_root_node_ids) {
      writer.size(root_id);
    }
    writer.boolean(record.resulting_root_node_id.has_value());
    if (record.resulting_root_node_id.has_value()) {
      writer.size(*record.resulting_root_node_id);
    }
    writer.boolean(record.created_node_id.has_value());
    if (record.created_node_id.has_value()) {
      writer.size(*record.created_node_id);
    }
    append_facet_vector(writer, record.newly_active_facet_point_ids);
    append_facet_vector(writer, record.equal_level_coface_point_ids);
    writer.boolean(record.coverage_delta.has_value());
    if (record.coverage_delta.has_value()) {
      append_facet_vector(
          writer, record.coverage_delta->added_facet_point_ids);
      append_point_id_vector(writer, record.coverage_delta->added_point_ids);
      writer.boolean(record.coverage_delta->fully_redundant);
    }
    writer.boolean(record.resolved_from_pre_batch_snapshot);
  }
  writer.size(source.batch_metadata.size());
  for (const ExactPersistentReducedGammaBatchMetadata& batch :
       source.batch_metadata) {
    writer.size(batch.batch_index);
    writer.size(batch.activation_level_index);
    writer.level(batch.squared_level);
    writer.size(batch.first_group_record_index);
    writer.size(batch.group_record_count);
    writer.boolean(batch.first_created_node_id.has_value());
    if (batch.first_created_node_id.has_value()) {
      writer.size(*batch.first_created_node_id);
    }
    writer.size(batch.created_node_count);
    writer.size(batch.active_root_count_before);
    writer.size(batch.active_root_count_after);
    writer.size(batch.strict_nontrivial_component_count);
    writer.size(batch.closed_nontrivial_component_count);
    writer.boolean(batch.pre_batch_root_bijection_certified);
    writer.boolean(batch.all_groups_resolved_before_mutation);
    writer.boolean(batch.post_batch_root_bijection_certified);
  }
  writer.size(source.final_active_roots.size());
  for (const ExactPersistentReducedGammaActiveRoot& root :
       source.final_active_roots) {
    writer.size(root.root_node_id);
    append_facet_vector(writer, root.facet_point_ids);
    append_point_id_vector(writer, root.covered_point_ids);
  }
  append_gamma_counters(writer, source.counters);
  writer.enumeration(source.decision);
  writer.enumeration(source.scope);
  writer.boolean(source.persistent_reduced_gamma_history_certified);
  writer.boolean(source.all_reduced_gamma_batches_fresh_replay_certified);
  writer.boolean(source.groups_resolved_against_pre_batch_snapshots);
  writer.boolean(source.mutations_applied_after_complete_batch_resolution);
  writer.boolean(source.coverage_deltas_accounted_exactly);
  return writer.finalize();
}

void append_source_counters(
    DigestWriter& writer, const ExactSizeCondensedSourceCounters& counters) {
  writer.size(counters.point_count);
  writer.size(counters.order);
  writer.size(counters.source_level_count);
  writer.size(counters.source_batch_count);
  writer.size(counters.source_transition_count);
  writer.size(counters.source_deferred_group_count);
  writer.size(counters.source_node_count);
  writer.size(counters.source_child_reference_count);
  writer.size(counters.source_selected_edge_count);
  writer.size(counters.source_facet_count);
  writer.size(counters.source_coface_count);
  writer.size(counters.source_coverage_delta_count);
  writer.boolean(counters.k1_compact_forest_counters.has_value());
  if (counters.k1_compact_forest_counters.has_value()) {
    append_k1_counters(writer, *counters.k1_compact_forest_counters);
  }
  writer.boolean(counters.reduced_gamma_history_counters.has_value());
  if (counters.reduced_gamma_history_counters.has_value()) {
    append_gamma_counters(writer, *counters.reduced_gamma_history_counters);
  }
}

void append_result_counters(
    DigestWriter& writer, const ExactSizeCondensedHierarchyCounters& counters) {
  writer.size(counters.processed_source_batch_count);
  writer.size(counters.processed_source_transition_count);
  writer.size(counters.processed_source_deferred_group_count);
  writer.size(counters.decision_record_count);
  writer.size(counters.hidden_decision_count);
  writer.size(counters.threshold_entry_count);
  writer.size(counters.continuation_count);
  writer.size(counters.multifusion_count);
  writer.size(counters.public_component_record_count);
  writer.size(counters.condensed_component_id_count);
  writer.size(counters.condensed_child_reference_count);
  writer.size(counters.maximum_visible_merge_arity);
  writer.size(counters.final_source_root_count);
  writer.size(counters.final_visible_root_count);
}

[[nodiscard]] contract::CanonicalId condensed_projection_digest(
    const ExactSizeCondensedHierarchy& result) {
  DigestWriter writer{
      "MorseHGP3D/exact-size-condensation/condensed-projection/v1"};
  writer.u64(result.schema_version);
  writer.size(result.min_cluster_size);
  writer.enumeration(result.coverage_relation);
  writer.enumeration(result.provenance.source_kind);
  writer.text(result.provenance.source_proof_basis);
  writer.identifier(result.provenance.source_projection_digest);
  append_source_counters(writer, result.provenance.source_counters);
  writer.boolean(result.provenance.source_complete_exact_history_certified);
  writer.boolean(result.provenance.source_fresh_replay_certified);
  writer.boolean(
      result.provenance.source_emst_minimality_reverified_by_condensation);
  writer.boolean(
      result.provenance.source_emst_certification_inherited_from_input_contract);
  writer.boolean(result.provenance.surrogate_v5_source_used);
  writer.size(result.decision_records.size());
  for (const ExactSizeCondensedComponentRecord& record :
       result.decision_records) {
    writer.size(record.decision_record_index);
    writer.size(record.batch_index);
    writer.boolean(record.source_batch_index.has_value());
    if (record.source_batch_index.has_value()) {
      writer.size(*record.source_batch_index);
    }
    writer.boolean(record.source_group_record_index.has_value());
    if (record.source_group_record_index.has_value()) {
      writer.size(*record.source_group_record_index);
    }
    writer.enumeration(record.source_transition_kind);
    writer.level(record.squared_level);
    writer.u64(record.source_resulting_root_node_id);
    writer.size(record.source_prior_root_node_ids.size());
    for (const SourceRootId root_id : record.source_prior_root_node_ids) {
      writer.u64(root_id);
    }
    writer.size(record.summed_prior_distinct_coverage_size);
    writer.size(record.distinct_prior_coverage_union_size);
    writer.size(record.distinct_coverage_size);
    writer.size(record.q_visible);
    writer.size(record.prior_visible_component_ids.size());
    for (const std::size_t component_id :
         record.prior_visible_component_ids) {
      writer.size(component_id);
    }
    writer.boolean(record.resulting_visible_component_id.has_value());
    if (record.resulting_visible_component_id.has_value()) {
      writer.size(*record.resulting_visible_component_id);
    }
    writer.boolean(record.created_visible_component_id.has_value());
    if (record.created_visible_component_id.has_value()) {
      writer.size(*record.created_visible_component_id);
    }
    writer.enumeration(record.decision);
    writer.boolean(record.decided_after_complete_source_batch);
  }
  writer.size(result.public_component_record_indices.size());
  for (const std::size_t record_index :
       result.public_component_record_indices) {
    writer.size(record_index);
  }
  writer.size(result.batch_metadata.size());
  for (const ExactSizeCondensedBatchMetadata& batch : result.batch_metadata) {
    writer.size(batch.batch_index);
    writer.boolean(batch.source_batch_index.has_value());
    if (batch.source_batch_index.has_value()) {
      writer.size(*batch.source_batch_index);
    }
    writer.level(batch.squared_level);
    writer.size(batch.first_decision_record_index);
    writer.size(batch.decision_record_count);
    writer.size(batch.source_transition_count);
    writer.size(batch.source_deferred_group_count);
    writer.size(batch.hidden_decision_count);
    writer.size(batch.threshold_entry_count);
    writer.size(batch.continuation_count);
    writer.size(batch.multifusion_count);
    writer.size(batch.visible_root_count_before);
    writer.size(batch.visible_root_count_after);
    writer.boolean(batch.source_batch_staged_completely_before_decisions);
    writer.boolean(batch.source_batch_mutated_only_after_all_decisions);
  }
  writer.size(result.final_visible_component_ids.size());
  for (const std::size_t component_id : result.final_visible_component_ids) {
    writer.size(component_id);
  }
  append_result_counters(writer, result.counters);
  writer.boolean(result.distinct_point_coverage_certified);
  writer.boolean(result.at_least_threshold_relation_certified);
  writer.boolean(result.all_source_batches_decided_atomically);
  writer.boolean(result.hidden_growth_preserved);
  writer.boolean(result.threshold_entries_are_not_morse_births);
  writer.boolean(result.every_source_transition_accounted_exactly_once);
  writer.boolean(result.condensed_genealogy_certified);
  writer.boolean(result.horizontal_single_order_view_certified);
  writer.boolean(result.vertical_maps_projected);
  writer.boolean(result.full_pi0_equivalence_claimed);
  writer.boolean(result.morsehgp3d_result_claimed);
  writer.boolean(result.public_status_claimed);
  writer.enumeration(result.decision);
  writer.enumeration(result.scope);
  // The digest value itself and the two certificates that depend on digest
  // verification are intentionally excluded. All non-derived result facts,
  // including the explicit horizontal-only/no-product claims, are signed.
  return writer.finalize();
}

struct DecisionOutcome {
  ExactSizeCondensedComponentDecision decision{
      ExactSizeCondensedComponentDecision::hidden};
  std::optional<std::size_t> resulting_visible_component_id;
  std::optional<std::size_t> created_visible_component_id;
};

[[nodiscard]] DecisionOutcome decide_component(
    std::size_t coverage_size,
    std::size_t min_cluster_size,
    std::span<const std::size_t> prior_visible_component_ids,
    std::size_t& next_visible_component_id) {
  if (coverage_size < min_cluster_size) {
    if (!prior_visible_component_ids.empty()) {
      throw std::logic_error(
          "a visible component cannot become hidden under monotone coverage");
    }
    return {};
  }
  if (prior_visible_component_ids.empty()) {
    const std::size_t created_id = next_visible_component_id;
    next_visible_component_id = checked_add(
        next_visible_component_id,
        1U,
        "the condensed component identifier exceeds size_t");
    return {
        ExactSizeCondensedComponentDecision::threshold_entry,
        created_id,
        created_id};
  }
  if (prior_visible_component_ids.size() == 1U) {
    return {
        ExactSizeCondensedComponentDecision::continuation,
        prior_visible_component_ids.front(),
        std::nullopt};
  }
  const std::size_t created_id = next_visible_component_id;
  next_visible_component_id = checked_add(
      next_visible_component_id,
      1U,
      "the condensed component identifier exceeds size_t");
  return {
      ExactSizeCondensedComponentDecision::multifusion,
      created_id,
      created_id};
}

void account_record(
    ExactSizeCondensedHierarchy& result,
    ExactSizeCondensedBatchMetadata& batch,
    ExactSizeCondensedComponentRecord record) {
  switch (record.decision) {
    case ExactSizeCondensedComponentDecision::hidden:
      ++batch.hidden_decision_count;
      ++result.counters.hidden_decision_count;
      break;
    case ExactSizeCondensedComponentDecision::threshold_entry:
      ++batch.threshold_entry_count;
      ++result.counters.threshold_entry_count;
      break;
    case ExactSizeCondensedComponentDecision::continuation:
      ++batch.continuation_count;
      ++result.counters.continuation_count;
      break;
    case ExactSizeCondensedComponentDecision::multifusion:
      ++batch.multifusion_count;
      ++result.counters.multifusion_count;
      result.counters.condensed_child_reference_count = checked_add(
          result.counters.condensed_child_reference_count,
          record.q_visible,
          "the condensed child-reference count overflows");
      result.counters.maximum_visible_merge_arity = std::max(
          result.counters.maximum_visible_merge_arity, record.q_visible);
      break;
  }
  if (record.decision != ExactSizeCondensedComponentDecision::hidden) {
    result.public_component_record_indices.push_back(
        record.decision_record_index);
  }
  if (record.created_visible_component_id.has_value()) {
    ++result.counters.condensed_component_id_count;
  }
  result.decision_records.push_back(std::move(record));
}

void finalize_result(
    ExactSizeCondensedHierarchy& result,
    std::size_t final_source_root_count,
    std::vector<std::size_t> final_visible_component_ids,
    ExactSizeCondensedHierarchyDecision decision) {
  std::sort(
      final_visible_component_ids.begin(), final_visible_component_ids.end());
  if (std::adjacent_find(
          final_visible_component_ids.begin(),
          final_visible_component_ids.end()) !=
      final_visible_component_ids.end()) {
    throw std::logic_error("final condensed component identifiers repeat");
  }
  result.final_visible_component_ids =
      std::move(final_visible_component_ids);
  result.counters.processed_source_batch_count =
      result.batch_metadata.size();
  result.counters.decision_record_count = result.decision_records.size();
  result.counters.public_component_record_count =
      result.public_component_record_indices.size();
  result.counters.final_source_root_count = final_source_root_count;
  result.counters.final_visible_root_count =
      result.final_visible_component_ids.size();
  result.distinct_point_coverage_certified = true;
  result.at_least_threshold_relation_certified = true;
  result.all_source_batches_decided_atomically = true;
  result.hidden_growth_preserved = true;
  result.threshold_entries_are_not_morse_births = true;
  result.every_source_transition_accounted_exactly_once = true;
  result.condensed_genealogy_certified = true;
  result.horizontal_single_order_view_certified = true;
  result.vertical_maps_projected = false;
  result.full_pi0_equivalence_claimed = false;
  result.morsehgp3d_result_claimed = false;
  result.public_status_claimed = false;
  result.decision = decision;
  result.scope = ExactSizeCondensedHierarchyScope::
      exact_k1_or_bounded_n14_reduced_gamma_distinct_point_coverage_view;
  result.condensed_projection_digest = condensed_projection_digest(result);
  result.output_digest_certified = true;
  result.size_condensed_hierarchy_certified = true;
}

[[nodiscard]] ExactSizeCondensedSourceTransitionKind gamma_transition_kind(
    ExactReducedGammaBatchGroupKind kind) {
  switch (kind) {
    case ExactReducedGammaBatchGroupKind::birth:
      return ExactSizeCondensedSourceTransitionKind::gamma_birth;
    case ExactReducedGammaBatchGroupKind::continuation:
      return ExactSizeCondensedSourceTransitionKind::gamma_continuation;
    case ExactReducedGammaBatchGroupKind::multifusion:
      return ExactSizeCondensedSourceTransitionKind::gamma_multifusion;
    case ExactReducedGammaBatchGroupKind::deferred_isolated_facet:
      break;
  }
  throw std::logic_error("a deferred Gamma group has no component transition");
}

[[nodiscard]] bool gamma_source_verification_closes(
    const ExactPersistentReducedGammaOrderHistoryVerification& verification) {
  return verification.requested_budget_certified &&
         verification.external_inputs_certified &&
         verification.derived_preflight_sizes_certified &&
         verification.high_cut_gamma_certified &&
         verification.activation_levels_certified &&
         verification.transient_reduced_gamma_batches_replayed_certified &&
         verification.nodes_certified && verification.group_records_certified &&
         verification.batch_metadata_certified &&
         verification.final_active_roots_certified &&
         verification.result_facts_certified &&
         verification.counters_certified && verification.decision_certified &&
         verification.scope_certified && verification.fresh_replay_certified &&
         verification.
             exact_persistent_reduced_gamma_order_history_decision_certified;
}

[[nodiscard]] ExactSizeCondensedHierarchy compute_k1(
    const spatial::CanonicalPointCloud& cloud,
    const K1CompactForest& source,
    std::size_t min_cluster_size) {
  if (min_cluster_size == 0U) {
    throw std::invalid_argument("min_cluster_size must be positive");
  }
  if (cloud.size() != source.point_count) {
    throw std::invalid_argument(
        "the K1CompactForest point count differs from its reference cloud");
  }
  if (cloud.size() >
      exact_size_condensed_k1_maximum_fresh_reference_point_count) {
    throw std::invalid_argument(
        "the fresh complete-graph K1 condensation oracle is capped at n<=64");
  }
  const K1EmstResult exact_emst = build_exact_complete_graph_emst(cloud);
  const K1CompactForest replayed = build_compact_k1_forest(exact_emst);
  if (!same_k1_compact_forest(replayed, source)) {
    throw std::invalid_argument(
        "the K1CompactForest source failed fresh exact EMST replay");
  }

  ExactSizeCondensedHierarchy result;
  result.min_cluster_size = min_cluster_size;
  result.provenance.source_kind =
      ExactSizeCondensedSourceKind::exact_k1_compact_forest;
  result.provenance.source_proof_basis =
      "bounded_n64_exact_complete_graph_emst_equal_level_compact_k1_forest_"
      "fresh_replay_v1";
  result.provenance.source_projection_digest =
      k1_source_projection_digest(source);
  result.provenance.source_counters = ExactSizeCondensedSourceCounters{
      source.point_count,
      1U,
      checked_add(
          source.levels.size(),
          1U,
          "the K1 source level count overflows"),
      checked_add(
          source.equal_level_batches.size(),
          1U,
          "the K1 source batch count overflows"),
      checked_add(
          source.point_count,
          source.merge_nodes.size(),
          "the K1 source transition count overflows"),
      0U,
      source.node_count(),
      source.child_ids.size(),
      source.selected_edges.size(),
      0U,
      0U,
      0U,
      source.counters,
      std::nullopt};
  result.provenance.source_complete_exact_history_certified = true;
  result.provenance.source_fresh_replay_certified = true;
  result.provenance.source_emst_minimality_reverified_by_condensation = true;
  result.provenance.
      source_emst_certification_inherited_from_input_contract = false;
  result.provenance.surrogate_v5_source_used = false;

  using K1State = std::pair<std::size_t, std::optional<std::size_t>>;
  std::map<SourceRootId, K1State> active_roots;
  std::size_t next_visible_component_id = 0U;

  ExactSizeCondensedBatchMetadata initial_batch;
  initial_batch.batch_index = 0U;
  initial_batch.source_batch_index = std::nullopt;
  initial_batch.squared_level = exact::ExactLevel{};
  initial_batch.first_decision_record_index = 0U;
  initial_batch.source_transition_count = source.point_count;
  initial_batch.visible_root_count_before = 0U;
  initial_batch.source_batch_staged_completely_before_decisions = true;
  std::vector<std::pair<SourceRootId, K1State>> initial_outputs;
  initial_outputs.reserve(source.point_count);
  for (std::size_t point_index = 0U; point_index < source.point_count;
       ++point_index) {
    const DecisionOutcome outcome = decide_component(
        1U,
        min_cluster_size,
        std::span<const std::size_t>{},
        next_visible_component_id);
    ExactSizeCondensedComponentRecord record;
    record.decision_record_index = result.decision_records.size();
    record.batch_index = initial_batch.batch_index;
    record.source_transition_kind =
        ExactSizeCondensedSourceTransitionKind::k1_initial_leaf;
    record.squared_level = exact::ExactLevel{};
    record.source_resulting_root_node_id = source_root_id(point_index);
    record.distinct_coverage_size = 1U;
    record.resulting_visible_component_id =
        outcome.resulting_visible_component_id;
    record.created_visible_component_id = outcome.created_visible_component_id;
    record.decision = outcome.decision;
    record.decided_after_complete_source_batch = true;
    initial_outputs.emplace_back(
        source_root_id(point_index),
        K1State{1U, outcome.resulting_visible_component_id});
    account_record(result, initial_batch, std::move(record));
  }
  for (const auto& output : initial_outputs) {
    if (!active_roots.emplace(output).second) {
      throw std::logic_error("the K1 initial batch repeats a leaf root");
    }
  }
  initial_batch.source_batch_mutated_only_after_all_decisions = true;
  initial_batch.decision_record_count = result.decision_records.size();
  initial_batch.visible_root_count_after = visible_root_count_k1(active_roots);
  result.counters.processed_source_transition_count = source.point_count;
  result.batch_metadata.push_back(std::move(initial_batch));

  struct PendingK1 {
    SourceRootId source_resulting_root_node_id{};
    std::vector<SourceRootId> source_prior_root_node_ids;
    std::size_t coverage_size{};
    std::vector<std::size_t> prior_visible_component_ids;
  };

  for (const K1CompactEqualLevelBatch& source_batch :
       source.equal_level_batches) {
    ExactSizeCondensedBatchMetadata batch;
    batch.batch_index = result.batch_metadata.size();
    batch.source_batch_index = source_batch.level_index;
    batch.squared_level = source.levels[source_batch.level_index];
    batch.first_decision_record_index = result.decision_records.size();
    batch.source_transition_count = source_batch.merge_node_count;
    batch.visible_root_count_before = visible_root_count_k1(active_roots);

    const auto snapshot = active_roots;
    std::set<SourceRootId> consumed_roots;
    std::vector<PendingK1> pending;
    pending.reserve(source_batch.merge_node_count);
    const std::size_t merge_end = checked_add(
        source_batch.merge_node_offset,
        source_batch.merge_node_count,
        "a compact K1 source batch range overflows");
    if (merge_end > source.merge_nodes.size()) {
      throw std::logic_error("a compact K1 source batch exceeds its arena");
    }
    for (std::size_t merge_index = source_batch.merge_node_offset;
         merge_index < merge_end;
         ++merge_index) {
      const K1CompactMergeNode& merge = source.merge_nodes[merge_index];
      PendingK1 staged;
      staged.source_resulting_root_node_id = merge.id;
      const std::span<const K1NodeId> children = source.children(merge.id);
      staged.source_prior_root_node_ids.reserve(children.size());
      staged.prior_visible_component_ids.reserve(children.size());
      for (const K1NodeId child_id : children) {
        const auto root = snapshot.find(child_id);
        if (root == snapshot.end() || !consumed_roots.insert(child_id).second) {
          throw std::logic_error(
              "a compact K1 batch does not consume frozen roots exactly once");
        }
        staged.source_prior_root_node_ids.push_back(child_id);
        staged.coverage_size = checked_add(
            staged.coverage_size,
            root->second.first,
            "a compact K1 coverage size overflows");
        if (root->second.second.has_value()) {
          staged.prior_visible_component_ids.push_back(*root->second.second);
        }
      }
      std::sort(
          staged.prior_visible_component_ids.begin(),
          staged.prior_visible_component_ids.end());
      if (std::adjacent_find(
              staged.prior_visible_component_ids.begin(),
              staged.prior_visible_component_ids.end()) !=
          staged.prior_visible_component_ids.end()) {
        throw std::logic_error(
            "a compact K1 batch aliases visible component identifiers");
      }
      if (staged.coverage_size != merge.coverage_size) {
        throw std::logic_error(
            "a compact K1 merge coverage does not equal its frozen children");
      }
      pending.push_back(std::move(staged));
    }
    batch.source_batch_staged_completely_before_decisions = true;

    std::vector<std::pair<SourceRootId, K1State>> outputs;
    outputs.reserve(pending.size());
    for (const PendingK1& staged : pending) {
      const DecisionOutcome outcome = decide_component(
          staged.coverage_size,
          min_cluster_size,
          staged.prior_visible_component_ids,
          next_visible_component_id);
      ExactSizeCondensedComponentRecord record;
      record.decision_record_index = result.decision_records.size();
      record.batch_index = batch.batch_index;
      record.source_batch_index = source_batch.level_index;
      record.source_group_record_index =
          static_cast<std::size_t>(staged.source_resulting_root_node_id) -
          source.point_count;
      record.source_transition_kind =
          ExactSizeCondensedSourceTransitionKind::k1_equal_level_merge;
      record.squared_level = batch.squared_level;
      record.source_resulting_root_node_id =
          staged.source_resulting_root_node_id;
      record.source_prior_root_node_ids = staged.source_prior_root_node_ids;
      record.summed_prior_distinct_coverage_size = staged.coverage_size;
      record.distinct_prior_coverage_union_size = staged.coverage_size;
      record.distinct_coverage_size = staged.coverage_size;
      record.q_visible = staged.prior_visible_component_ids.size();
      record.prior_visible_component_ids =
          staged.prior_visible_component_ids;
      record.resulting_visible_component_id =
          outcome.resulting_visible_component_id;
      record.created_visible_component_id =
          outcome.created_visible_component_id;
      record.decision = outcome.decision;
      record.decided_after_complete_source_batch = true;
      outputs.emplace_back(
          staged.source_resulting_root_node_id,
          K1State{
              staged.coverage_size,
              outcome.resulting_visible_component_id});
      account_record(result, batch, std::move(record));
    }

    for (const SourceRootId consumed_root : consumed_roots) {
      active_roots.erase(consumed_root);
    }
    for (const auto& output : outputs) {
      if (!active_roots.emplace(output).second) {
        throw std::logic_error("a compact K1 batch repeats an output root");
      }
    }
    batch.source_batch_mutated_only_after_all_decisions = true;
    batch.decision_record_count =
        result.decision_records.size() - batch.first_decision_record_index;
    batch.visible_root_count_after = visible_root_count_k1(active_roots);
    result.counters.processed_source_transition_count = checked_add(
        result.counters.processed_source_transition_count,
        batch.source_transition_count,
        "the processed K1 transition count overflows");
    result.batch_metadata.push_back(std::move(batch));
  }

  if (active_roots.size() != 1U ||
      active_roots.begin()->first != source.root_node_id ||
      active_roots.begin()->second.first != source.point_count) {
    throw std::logic_error("the condensed K1 replay did not reach its source root");
  }
  std::vector<std::size_t> final_visible;
  if (active_roots.begin()->second.second.has_value()) {
    final_visible.push_back(*active_roots.begin()->second.second);
  }
  finalize_result(
      result,
      active_roots.size(),
      std::move(final_visible),
      ExactSizeCondensedHierarchyDecision::complete_size_condensed_hierarchy);
  return result;
}

[[nodiscard]] ExactSizeCondensedHierarchy compute_gamma(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactPersistentReducedGammaOrderHistoryBudget budget,
    const ExactPersistentReducedGammaOrderHistory& source,
    std::size_t min_cluster_size) {
  if (min_cluster_size == 0U) {
    throw std::invalid_argument("min_cluster_size must be positive");
  }
  const ExactPersistentReducedGammaOrderHistoryVerification source_verification =
      verify_exact_persistent_reduced_gamma_order_history(
          cloud, order, budget, source);
  if (!gamma_source_verification_closes(source_verification)) {
    throw std::invalid_argument(
        "the reduced-Gamma source failed its fresh exact replay");
  }
  const bool complete_history =
      source.decision == ExactPersistentReducedGammaOrderHistoryDecision::
                             complete_persistent_reduced_gamma_history;
  const bool complete_empty_terminal =
      source.decision == ExactPersistentReducedGammaOrderHistoryDecision::
                             complete_empty_terminal_order;
  if (!complete_history && !complete_empty_terminal) {
    throw std::invalid_argument(
        "size condensation requires a complete exact source history");
  }

  ExactSizeCondensedHierarchy result;
  result.min_cluster_size = min_cluster_size;
  result.provenance.source_kind = ExactSizeCondensedSourceKind::
      exact_persistent_reduced_gamma_order_history;
  result.provenance.source_proof_basis =
      ExactPersistentReducedGammaOrderHistory::proof_basis;
  result.provenance.source_projection_digest =
      gamma_source_projection_digest(source);
  const std::size_t source_transition_count =
      source.counters.birth_group_count +
      source.counters.continuation_group_count +
      source.counters.multifusion_group_count;
  result.provenance.source_counters = ExactSizeCondensedSourceCounters{
      source.point_count,
      source.order,
      source.activation_levels.size(),
      source.batch_metadata.size(),
      source_transition_count,
      source.counters.deferred_group_count,
      source.nodes.size(),
      source.counters.child_reference_count,
      0U,
      source.exhaustive_facet_count,
      source.exhaustive_coface_count,
      source.counters.coverage_delta_count,
      std::nullopt,
      source.counters};
  result.provenance.source_complete_exact_history_certified = true;
  result.provenance.source_fresh_replay_certified = true;
  result.provenance.source_emst_minimality_reverified_by_condensation = false;
  result.provenance.
      source_emst_certification_inherited_from_input_contract = false;
  result.provenance.surrogate_v5_source_used = false;

  if (complete_empty_terminal) {
    finalize_result(
        result,
        0U,
        {},
        ExactSizeCondensedHierarchyDecision::complete_empty_terminal_order);
    return result;
  }

  std::map<SourceRootId, GammaRootState> active_roots;
  std::size_t next_visible_component_id = 0U;

  struct PendingGamma {
    const ExactPersistentReducedGammaHistoryGroupRecord* source_record{};
    SourceRootId source_resulting_root_node_id{};
    std::vector<SourceRootId> source_prior_root_node_ids;
    std::size_t summed_prior_coverage_size{};
    std::set<spatial::PointId> prior_coverage_union;
    std::set<spatial::PointId> resulting_coverage;
    std::vector<std::size_t> prior_visible_component_ids;
  };

  for (const ExactPersistentReducedGammaBatchMetadata& source_batch :
       source.batch_metadata) {
    ExactSizeCondensedBatchMetadata batch;
    batch.batch_index = result.batch_metadata.size();
    batch.source_batch_index = source_batch.batch_index;
    batch.squared_level = source_batch.squared_level;
    batch.first_decision_record_index = result.decision_records.size();
    batch.visible_root_count_before = visible_root_count_gamma(active_roots);

    const std::size_t group_end = checked_add(
        source_batch.first_group_record_index,
        source_batch.group_record_count,
        "a reduced-Gamma source batch range overflows");
    if (group_end > source.group_records.size()) {
      throw std::logic_error(
          "a reduced-Gamma source batch exceeds its group arena");
    }
    const auto snapshot = active_roots;
    std::set<SourceRootId> consumed_roots;
    std::vector<PendingGamma> pending;
    pending.reserve(source_batch.group_record_count);
    for (std::size_t group_index = source_batch.first_group_record_index;
         group_index < group_end;
         ++group_index) {
      const ExactPersistentReducedGammaHistoryGroupRecord& source_record =
          source.group_records[group_index];
      if (source_record.kind ==
          ExactReducedGammaBatchGroupKind::deferred_isolated_facet) {
        ++batch.source_deferred_group_count;
        continue;
      }
      if (!source_record.resulting_root_node_id.has_value() ||
          !source_record.coverage_delta.has_value()) {
        throw std::logic_error(
            "a non-deferred reduced-Gamma group lacks its exact output delta");
      }
      PendingGamma staged;
      staged.source_record = &source_record;
      staged.source_resulting_root_node_id =
          source_root_id(*source_record.resulting_root_node_id);
      staged.source_prior_root_node_ids.reserve(
          source_record.prior_root_node_ids.size());
      staged.prior_visible_component_ids.reserve(
          source_record.prior_root_node_ids.size());
      for (const std::size_t prior_root_index :
           source_record.prior_root_node_ids) {
        const SourceRootId prior_root_id = source_root_id(prior_root_index);
        const auto root = snapshot.find(prior_root_id);
        if (root == snapshot.end() ||
            !consumed_roots.insert(prior_root_id).second) {
          throw std::logic_error(
              "a reduced-Gamma batch does not consume frozen roots exactly once");
        }
        staged.source_prior_root_node_ids.push_back(prior_root_id);
        staged.summed_prior_coverage_size = checked_add(
            staged.summed_prior_coverage_size,
            root->second.covered_point_ids.size(),
            "the reduced-Gamma summed coverage size overflows");
        staged.prior_coverage_union.insert(
            root->second.covered_point_ids.begin(),
            root->second.covered_point_ids.end());
        if (root->second.visible_component_id.has_value()) {
          staged.prior_visible_component_ids.push_back(
              *root->second.visible_component_id);
        }
      }
      std::sort(
          staged.prior_visible_component_ids.begin(),
          staged.prior_visible_component_ids.end());
      if (std::adjacent_find(
              staged.prior_visible_component_ids.begin(),
              staged.prior_visible_component_ids.end()) !=
          staged.prior_visible_component_ids.end()) {
        throw std::logic_error(
            "a reduced-Gamma batch aliases visible component identifiers");
      }
      staged.resulting_coverage = staged.prior_coverage_union;
      staged.resulting_coverage.insert(
          source_record.coverage_delta->added_point_ids.begin(),
          source_record.coverage_delta->added_point_ids.end());
      pending.push_back(std::move(staged));
      ++batch.source_transition_count;
    }
    batch.source_batch_staged_completely_before_decisions = true;

    std::vector<std::pair<SourceRootId, GammaRootState>> outputs;
    outputs.reserve(pending.size());
    for (const PendingGamma& staged : pending) {
      const DecisionOutcome outcome = decide_component(
          staged.resulting_coverage.size(),
          min_cluster_size,
          staged.prior_visible_component_ids,
          next_visible_component_id);
      ExactSizeCondensedComponentRecord record;
      record.decision_record_index = result.decision_records.size();
      record.batch_index = batch.batch_index;
      record.source_batch_index = source_batch.batch_index;
      record.source_group_record_index =
          staged.source_record->group_record_index;
      record.source_transition_kind =
          gamma_transition_kind(staged.source_record->kind);
      record.squared_level = source_batch.squared_level;
      record.source_resulting_root_node_id =
          staged.source_resulting_root_node_id;
      record.source_prior_root_node_ids = staged.source_prior_root_node_ids;
      record.summed_prior_distinct_coverage_size =
          staged.summed_prior_coverage_size;
      record.distinct_prior_coverage_union_size =
          staged.prior_coverage_union.size();
      record.distinct_coverage_size = staged.resulting_coverage.size();
      record.q_visible = staged.prior_visible_component_ids.size();
      record.prior_visible_component_ids =
          staged.prior_visible_component_ids;
      record.resulting_visible_component_id =
          outcome.resulting_visible_component_id;
      record.created_visible_component_id =
          outcome.created_visible_component_id;
      record.decision = outcome.decision;
      record.decided_after_complete_source_batch = true;
      outputs.emplace_back(
          staged.source_resulting_root_node_id,
          GammaRootState{
              staged.resulting_coverage,
              outcome.resulting_visible_component_id});
      account_record(result, batch, std::move(record));
    }

    for (const SourceRootId consumed_root : consumed_roots) {
      active_roots.erase(consumed_root);
    }
    for (auto& output : outputs) {
      if (!active_roots.emplace(output.first, std::move(output.second)).second) {
        throw std::logic_error(
            "a reduced-Gamma batch repeats an output root");
      }
    }
    batch.source_batch_mutated_only_after_all_decisions = true;
    batch.decision_record_count =
        result.decision_records.size() - batch.first_decision_record_index;
    batch.visible_root_count_after = visible_root_count_gamma(active_roots);
    result.counters.processed_source_transition_count = checked_add(
        result.counters.processed_source_transition_count,
        batch.source_transition_count,
        "the processed reduced-Gamma transition count overflows");
    result.counters.processed_source_deferred_group_count = checked_add(
        result.counters.processed_source_deferred_group_count,
        batch.source_deferred_group_count,
        "the processed reduced-Gamma deferred count overflows");
    result.batch_metadata.push_back(std::move(batch));
  }

  if (result.counters.processed_source_transition_count !=
          source_transition_count ||
      result.counters.processed_source_deferred_group_count !=
          source.counters.deferred_group_count ||
      active_roots.size() != source.final_active_roots.size()) {
    throw std::logic_error(
        "the condensed reduced-Gamma replay does not close source counts");
  }
  for (const ExactPersistentReducedGammaActiveRoot& source_root :
       source.final_active_roots) {
    const auto replayed = active_roots.find(source_root_id(source_root.root_node_id));
    if (replayed == active_roots.end() ||
        replayed->second.covered_point_ids !=
            std::set<spatial::PointId>{
                source_root.covered_point_ids.begin(),
                source_root.covered_point_ids.end()}) {
      throw std::logic_error(
          "the condensed reduced-Gamma final coverage differs from its source");
    }
  }
  std::vector<std::size_t> final_visible;
  for (const auto& root : active_roots) {
    if (root.second.visible_component_id.has_value()) {
      final_visible.push_back(*root.second.visible_component_id);
    }
  }
  finalize_result(
      result,
      active_roots.size(),
      std::move(final_visible),
      ExactSizeCondensedHierarchyDecision::complete_size_condensed_hierarchy);
  return result;
}

[[nodiscard]] ExactSizeCondensedHierarchyVerification compare_results(
    const ExactSizeCondensedHierarchy& expected,
    const ExactSizeCondensedHierarchy& observed,
    bool source_certified) {
  ExactSizeCondensedHierarchyVerification verification;
  verification.source_certified = source_certified;
  verification.parameters_certified =
      observed.schema_version == expected.schema_version &&
      observed.min_cluster_size == expected.min_cluster_size &&
      observed.coverage_relation == expected.coverage_relation &&
      observed.scope == expected.scope;
  verification.provenance_certified =
      observed.provenance == expected.provenance;
  verification.source_counts_certified =
      observed.provenance.source_counters ==
      expected.provenance.source_counters;
  verification.batches_certified =
      observed.batch_metadata == expected.batch_metadata;
  verification.distinct_coverage_certified =
      observed.decision_records == expected.decision_records &&
      observed.distinct_point_coverage_certified &&
      observed.at_least_threshold_relation_certified;
  verification.decisions_certified =
      observed.decision_records == expected.decision_records &&
      observed.public_component_record_indices ==
          expected.public_component_record_indices &&
      observed.threshold_entries_are_not_morse_births;
  // The genealogy subcertificate deliberately rechecks its complete
  // source-backed journal instead of borrowing decisions/batches/counters
  // certificates. This makes an internal ancestry or batch-boundary mutation
  // visible here even when the final visible root identifiers are unchanged.
  verification.genealogy_certified =
      observed.decision_records == expected.decision_records &&
      observed.public_component_record_indices ==
          expected.public_component_record_indices &&
      observed.batch_metadata == expected.batch_metadata &&
      observed.final_visible_component_ids ==
          expected.final_visible_component_ids &&
      observed.counters == expected.counters &&
      observed.condensed_genealogy_certified ==
          expected.condensed_genealogy_certified &&
      observed.condensed_genealogy_certified;
  verification.counters_certified = observed.counters == expected.counters;
  const contract::CanonicalId expected_recomputed_digest =
      condensed_projection_digest(expected);
  const contract::CanonicalId observed_recomputed_digest =
      condensed_projection_digest(observed);
  verification.digests_certified =
      observed.condensed_projection_digest ==
          expected.condensed_projection_digest &&
      expected.condensed_projection_digest == expected_recomputed_digest &&
      observed.condensed_projection_digest == observed_recomputed_digest &&
      observed.output_digest_certified;
  verification.result_facts_certified =
      observed.distinct_point_coverage_certified ==
          expected.distinct_point_coverage_certified &&
      observed.at_least_threshold_relation_certified ==
          expected.at_least_threshold_relation_certified &&
      observed.all_source_batches_decided_atomically ==
          expected.all_source_batches_decided_atomically &&
      observed.hidden_growth_preserved == expected.hidden_growth_preserved &&
      observed.threshold_entries_are_not_morse_births ==
          expected.threshold_entries_are_not_morse_births &&
      observed.every_source_transition_accounted_exactly_once ==
          expected.every_source_transition_accounted_exactly_once &&
      observed.condensed_genealogy_certified ==
          expected.condensed_genealogy_certified &&
      observed.horizontal_single_order_view_certified ==
          expected.horizontal_single_order_view_certified &&
      observed.vertical_maps_projected == expected.vertical_maps_projected &&
      observed.full_pi0_equivalence_claimed ==
          expected.full_pi0_equivalence_claimed &&
      observed.morsehgp3d_result_claimed ==
          expected.morsehgp3d_result_claimed &&
      observed.public_status_claimed == expected.public_status_claimed &&
      observed.output_digest_certified == expected.output_digest_certified &&
      observed.size_condensed_hierarchy_certified ==
          expected.size_condensed_hierarchy_certified &&
      observed.decision == expected.decision;
  verification.fresh_replay_certified = observed == expected;
  verification.exact_size_condensed_hierarchy_certified =
      verification.source_certified &&
      verification.parameters_certified &&
      verification.provenance_certified &&
      verification.source_counts_certified &&
      verification.batches_certified &&
      verification.distinct_coverage_certified &&
      verification.decisions_certified &&
      verification.genealogy_certified &&
      verification.counters_certified &&
      verification.digests_certified &&
      verification.result_facts_certified &&
      verification.fresh_replay_certified;
  return verification;
}

}  // namespace

ExactSizeCondensedHierarchy build_exact_size_condensed_k1(
    const spatial::CanonicalPointCloud& cloud,
    const K1CompactForest& source,
    std::size_t min_cluster_size) {
  return compute_k1(cloud, source, min_cluster_size);
}

ExactSizeCondensedHierarchyVerification verify_exact_size_condensed_k1(
    const spatial::CanonicalPointCloud& cloud,
    const K1CompactForest& source,
    std::size_t min_cluster_size,
    const ExactSizeCondensedHierarchy& observed) {
  try {
    const ExactSizeCondensedHierarchy expected =
        compute_k1(cloud, source, min_cluster_size);
    return compare_results(expected, observed, true);
  } catch (const std::exception&) {
    return {};
  }
}

ExactSizeCondensedHierarchy build_exact_size_condensed_reduced_gamma(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactPersistentReducedGammaOrderHistoryBudget budget,
    const ExactPersistentReducedGammaOrderHistory& source,
    std::size_t min_cluster_size) {
  return compute_gamma(cloud, order, budget, source, min_cluster_size);
}

ExactSizeCondensedHierarchyVerification
verify_exact_size_condensed_reduced_gamma(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactPersistentReducedGammaOrderHistoryBudget budget,
    const ExactPersistentReducedGammaOrderHistory& source,
    std::size_t min_cluster_size,
    const ExactSizeCondensedHierarchy& observed) {
  try {
    const ExactSizeCondensedHierarchy expected =
        compute_gamma(cloud, order, budget, source, min_cluster_size);
    return compare_results(expected, observed, true);
  } catch (const std::exception&) {
    return {};
  }
}

}  // namespace morsehgp3d::hierarchy
