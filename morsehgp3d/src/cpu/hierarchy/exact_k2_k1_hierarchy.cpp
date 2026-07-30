#include "morsehgp3d/hierarchy/exact_k2_k1_hierarchy.hpp"

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

struct DigestMetrics {
  std::size_t logical_entry_count{};
  std::size_t exact_text_byte_count{};
};

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

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left, std::size_t right, const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

void increment(std::size_t& value, const char* message) {
  value = checked_add(value, 1U, message);
}

class DigestWriter {
 public:
  DigestWriter(std::string_view domain, bool hash)
      : builder_(hash ? std::optional<contract::CanonicalSha256Builder>{
                            std::in_place}
                      : std::nullopt) {
    text(domain);
  }

  void boolean(bool value) {
    account_entry();
    raw_byte(value ? std::uint8_t{1U} : std::uint8_t{0U});
  }

  void u64(std::uint64_t value) {
    account_entry();
    raw_u64(value);
  }

  void size(std::size_t value) {
    account_entry();
    raw_u64(checked_u64(value, "a digest size exceeds uint64"));
  }

  void text(std::string_view value) {
    account_entry();
    raw_text(value);
  }

  void identifier(const contract::CanonicalId& value) {
    account_entry();
    if (builder_.has_value()) {
      builder_->update(value.bytes());
    }
  }

  void level(const exact::ExactLevel& value) {
    account_entry();
    const std::string numerator = value.numerator_string();
    const std::string denominator = value.denominator_string();
    metrics_.exact_text_byte_count = checked_add(
        metrics_.exact_text_byte_count,
        numerator.size(),
        "the digest exact-text byte count overflows");
    metrics_.exact_text_byte_count = checked_add(
        metrics_.exact_text_byte_count,
        denominator.size(),
        "the digest exact-text byte count overflows");
    raw_text(numerator);
    raw_text(denominator);
  }

  template <typename Enum>
  void enumeration(Enum value) {
    account_entry();
    raw_byte(static_cast<std::uint8_t>(value));
  }

  [[nodiscard]] const DigestMetrics& metrics() const noexcept {
    return metrics_;
  }

  [[nodiscard]] contract::CanonicalId finalize() {
    if (!builder_.has_value()) {
      throw std::logic_error("a digest meter cannot be finalized");
    }
    return builder_->finalize();
  }

 private:
  void account_entry() {
    increment(
        metrics_.logical_entry_count,
        "the digest logical-entry count overflows");
  }

  void raw_byte(std::uint8_t value) {
    if (builder_.has_value()) {
      const std::array<std::uint8_t, 1U> bytes{value};
      builder_->update(bytes);
    }
  }

  void raw_u64(std::uint64_t value) {
    if (!builder_.has_value()) {
      return;
    }
    std::array<std::uint8_t, 8U> bytes{};
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
      const std::size_t shift = (bytes.size() - 1U - index) * 8U;
      bytes[index] = static_cast<std::uint8_t>(value >> shift);
    }
    builder_->update(bytes);
  }

  void raw_text(std::string_view value) {
    raw_u64(checked_u64(value.size(), "a digest text size exceeds uint64"));
    if (builder_.has_value()) {
      builder_->update(value);
    }
  }

  std::optional<contract::CanonicalSha256Builder> builder_;
  DigestMetrics metrics_{};
};

void append_gamma_budget(
    DigestWriter& writer,
    const ExactPersistentReducedGammaOrderHistoryBudget& budget) {
  writer.size(budget.gamma_budget.maximum_enumerated_facet_count);
  writer.size(budget.gamma_budget.maximum_enumerated_coface_count);
  writer.size(budget.gamma_budget.maximum_union_attempt_count);
  writer.size(budget.maximum_activation_level_count);
  writer.size(budget.maximum_total_facet_work_count);
  writer.size(budget.maximum_total_coface_work_count);
  writer.size(budget.maximum_total_union_work_count);
  writer.size(budget.maximum_node_count);
  writer.size(budget.maximum_child_reference_count);
  writer.size(budget.maximum_group_root_reference_count);
  writer.size(budget.maximum_group_count);
  writer.size(budget.maximum_group_newly_active_facet_count);
  writer.size(budget.maximum_group_equal_level_coface_count);
  writer.size(budget.maximum_delta_facet_count);
  writer.size(budget.maximum_delta_point_reference_count);
}

void append_projection_budget(
    DigestWriter& writer, const ExactK2K1HierarchyBudget& budget) {
  writer.size(budget.maximum_source_batch_count);
  writer.size(budget.maximum_source_group_count);
  writer.size(budget.maximum_source_node_count);
  writer.size(budget.maximum_source_child_reference_count);
  writer.size(budget.maximum_source_root_reference_count);
  writer.size(budget.maximum_checkpoint_count);
  writer.size(budget.maximum_source_facet_replay_count);
  writer.size(budget.maximum_vertical_endpoint_lookup_count);
  writer.size(budget.maximum_k1_parent_hop_count);
  writer.size(budget.maximum_target_coverage_point_reference_count);
  writer.size(budget.maximum_digest_logical_entry_count);
  writer.size(budget.maximum_digest_exact_text_byte_count);
}

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

void append_point_ids(
    DigestWriter& writer, std::span<const spatial::PointId> point_ids) {
  writer.size(point_ids.size());
  for (const spatial::PointId point_id : point_ids) {
    writer.u64(static_cast<std::uint64_t>(point_id));
  }
}

void append_facets(
    DigestWriter& writer,
    const std::vector<std::vector<spatial::PointId>>& facets) {
  writer.size(facets.size());
  for (const auto& facet : facets) {
    append_point_ids(writer, facet);
  }
}

void append_gamma_source_projection(
    DigestWriter& writer,
    const ExactPersistentReducedGammaOrderHistory& source) {
  writer.text(ExactPersistentReducedGammaOrderHistory::proof_basis);
  append_gamma_budget(writer, source.requested_budget);
  writer.size(source.point_count);
  writer.size(source.order);
  writer.size(source.exhaustive_facet_count);
  writer.size(source.exhaustive_coface_count);
  writer.size(source.exhaustive_union_attempt_count);
  writer.size(source.required_activation_level_capacity);
  writer.size(source.required_total_facet_work_capacity);
  writer.size(source.required_total_coface_work_capacity);
  writer.size(source.required_total_union_work_capacity);
  writer.size(source.required_node_capacity);
  writer.size(source.required_child_reference_capacity);
  writer.size(source.required_group_root_reference_capacity);
  writer.size(source.required_group_capacity);
  writer.size(source.required_group_newly_active_facet_capacity);
  writer.size(source.required_group_equal_level_coface_capacity);
  writer.size(source.required_delta_facet_capacity);
  writer.size(source.required_delta_point_reference_capacity);
  writer.boolean(source.exact_diameter_squared.has_value());
  if (source.exact_diameter_squared.has_value()) {
    writer.level(*source.exact_diameter_squared);
  }
  writer.boolean(source.high_cut_squared_level.has_value());
  if (source.high_cut_squared_level.has_value()) {
    writer.level(*source.high_cut_squared_level);
  }
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
    append_point_ids(
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
    append_facets(writer, record.newly_active_facet_point_ids);
    append_facets(writer, record.equal_level_coface_point_ids);
    writer.boolean(record.coverage_delta.has_value());
    if (record.coverage_delta.has_value()) {
      append_facets(writer, record.coverage_delta->added_facet_point_ids);
      append_point_ids(writer, record.coverage_delta->added_point_ids);
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
    append_facets(writer, root.facet_point_ids);
    append_point_ids(writer, root.covered_point_ids);
  }
  writer.boolean(source.candidate_space_size_certified);
  writer.boolean(source.preflight_budget_sufficient);
  writer.boolean(source.geometry_started_after_successful_preflight);
  writer.boolean(source.high_cut_equals_twice_exact_squared_diameter);
  writer.boolean(source.high_cut_strictly_above_all_activation_levels);
  writer.boolean(source.high_cut_catalog_exhaustive);
  writer.boolean(source.activation_levels_canonical_and_complete);
  writer.boolean(source.all_reduced_gamma_batches_fresh_replay_certified);
  writer.boolean(source.groups_resolved_against_pre_batch_snapshots);
  writer.boolean(source.mutations_applied_after_complete_batch_resolution);
  writer.boolean(
      source.active_roots_match_nontrivial_components_after_every_batch);
  writer.boolean(source.node_ids_dense_and_deterministic);
  writer.boolean(source.children_precede_parent);
  writer.boolean(source.each_child_consumed_at_most_once);
  writer.boolean(source.every_exhaustive_coface_affected_exactly_once);
  writer.boolean(source.group_kinds_have_exact_persistent_effects);
  writer.boolean(
      source.every_non_deferred_group_has_exactly_one_coverage_delta);
  writer.boolean(source.fully_redundant_groups_preserved);
  writer.boolean(source.coverage_deltas_accounted_exactly);
  writer.boolean(source.final_single_root_covers_all_facets_and_points);
  writer.boolean(source.terminal_order_complete_empty);
  writer.boolean(source.persistent_reduced_gamma_history_certified);
  append_gamma_counters(writer, source.counters);
  writer.enumeration(source.decision);
  writer.enumeration(source.scope);
}

void append_k1_projection(
    DigestWriter& writer, const K1CompactForest& source) {
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
}

void append_result_counters(
    DigestWriter& writer, const ExactK2K1HierarchyCounters& counters) {
  writer.size(counters.source_batch_count);
  writer.size(counters.source_group_count);
  writer.size(counters.source_node_count);
  writer.size(counters.source_child_reference_count);
  writer.size(counters.source_root_reference_count);
  writer.size(counters.deferred_group_count);
  writer.size(counters.birth_checkpoint_count);
  writer.size(counters.continuation_checkpoint_count);
  writer.size(counters.multifusion_checkpoint_count);
  writer.size(counters.fully_redundant_checkpoint_count);
  writer.size(counters.checkpoint_count);
  writer.size(counters.source_facet_replay_count);
  writer.size(counters.vertical_endpoint_lookup_count);
  writer.size(counters.k1_parent_hop_count);
  writer.size(counters.exact_level_comparison_count);
  writer.size(counters.target_coverage_point_reference_count);
  writer.size(counters.strict_open_target_reference_count);
  writer.size(counters.strict_open_normalization_check_count);
  writer.size(counters.strict_to_closed_naturality_check_count);
  writer.size(counters.k1_target_normalization_update_count);
  writer.size(counters.prior_target_naturality_check_count);
  writer.size(counters.consumed_source_root_count);
  writer.size(counters.produced_source_root_count);
  writer.size(counters.final_source_root_count);
  writer.size(counters.digest_logical_entry_count);
  writer.size(counters.digest_exact_text_byte_count);
}

void append_provenance(
    DigestWriter& writer, const ExactK2K1HierarchyProvenance& provenance) {
  writer.text(provenance.source_horizontal_proof_basis);
  writer.text(provenance.k1_target_proof_basis);
  writer.identifier(provenance.canonical_cloud_digest);
  writer.identifier(provenance.source_horizontal_projection_digest);
  writer.identifier(provenance.k1_compact_projection_digest);
  append_gamma_counters(writer, provenance.source_history_counters);
  append_k1_counters(writer, provenance.k1_forest_counters);
  writer.boolean(provenance.source_history_fresh_replay_certified);
  writer.boolean(provenance.source_horizontal_projection_complete);
  writer.boolean(provenance.k1_complete_graph_emst_fresh_replay_certified);
  writer.boolean(provenance.k1_compact_forest_fresh_replay_certified);
  writer.boolean(provenance.conditional_direct_morse_authority_not_used);
}

void append_checkpoint(
    DigestWriter& writer, const ExactK2K1VerticalCheckpoint& checkpoint) {
  writer.size(checkpoint.checkpoint_index);
  writer.size(checkpoint.source_batch_index);
  writer.size(checkpoint.source_group_record_index);
  writer.size(checkpoint.source_batch_group_index);
  writer.level(checkpoint.squared_level);
  writer.size(checkpoint.source_root_node_id);
  writer.size(checkpoint.strict_prior_target_node_ids.size());
  for (const K1NodeId target : checkpoint.strict_prior_target_node_ids) {
    writer.u64(target);
  }
  writer.u64(checkpoint.closed_target_node_id);
  writer.enumeration(checkpoint.kind);
  writer.size(checkpoint.prior_source_root_count);
  writer.size(checkpoint.source_facet_count);
  writer.size(checkpoint.source_covered_point_count);
  writer.size(checkpoint.closed_target_covered_point_count);
  writer.size(checkpoint.added_facet_count);
  writer.size(checkpoint.added_point_count);
  writer.boolean(checkpoint.source_group_resolved_from_frozen_snapshot);
  writer.boolean(checkpoint.every_prior_root_has_pre_batch_checkpoint);
  writer.boolean(
      checkpoint.prior_targets_normalize_to_strict_open_targets);
  writer.boolean(
      checkpoint.strict_open_targets_normalize_to_closed_target);
  writer.boolean(checkpoint.all_source_facets_map_to_closed_target);
  writer.boolean(checkpoint.source_coverage_contained_in_closed_target);
}

void append_hierarchy_projection(
    DigestWriter& writer, const ExactK2K1HierarchyResult& result) {
  writer.text(exact_k2_k1_hierarchy_backend);
  writer.text(exact_k2_k1_hierarchy_profile);
  writer.text(exact_k2_k1_hierarchy_mode);
  writer.text(exact_k2_k1_hierarchy_deployment_status);
  writer.text(exact_k2_k1_hierarchy_public_status);
  writer.text(exact_k2_k1_hierarchy_proof_basis);
  writer.u64(result.schema_version);
  append_projection_budget(writer, result.requested_budget);
  writer.size(result.point_count);
  writer.size(result.source_order);
  writer.size(result.target_order);
  writer.size(result.required_source_batch_capacity);
  writer.size(result.required_source_group_capacity);
  writer.size(result.required_source_node_capacity);
  writer.size(result.required_source_child_reference_capacity);
  writer.size(result.required_source_root_reference_capacity);
  writer.size(result.required_checkpoint_capacity);
  writer.size(result.preflight_source_facet_replay_upper_bound);
  writer.size(result.preflight_vertical_endpoint_lookup_upper_bound);
  writer.size(result.preflight_k1_parent_hop_upper_bound);
  writer.size(result.preflight_target_coverage_point_reference_upper_bound);
  writer.size(result.required_digest_logical_entry_capacity);
  writer.size(result.required_digest_exact_text_byte_capacity);
  append_provenance(writer, result.provenance);
  writer.size(result.vertical_checkpoints.size());
  for (const ExactK2K1VerticalCheckpoint& checkpoint :
       result.vertical_checkpoints) {
    append_checkpoint(writer, checkpoint);
  }
  append_result_counters(writer, result.counters);
  writer.boolean(
      result.source_and_target_preflight_sizes_derived_from_fresh_replay);
  writer.boolean(result.projection_budget_preflight_certified);
  writer.boolean(result.all_source_batches_staged_before_root_mutation);
  writer.boolean(result.every_non_deferred_group_has_one_checkpoint);
  writer.boolean(
      result.every_checkpoint_uses_strict_open_and_closed_exact_k1_targets);
  writer.boolean(result.source_facets_and_point_coverage_replayed_exactly);
  writer.boolean(result.strict_open_closed_vertical_naturality_certified);
  writer.boolean(result.k1_only_interlevel_target_evolution_certified);
  writer.boolean(
      result
          .vertical_map_complete_for_bounded_hgp_reduced_k2_to_k1_source);
  writer.boolean(result.all_order_vertical_maps_complete);
  writer.boolean(result.horizontal_source_persisted_in_result);
  writer.boolean(result.bounded_exhaustive_gamma_source_materialized);
  writer.boolean(result.product_architecture_claimed);
  writer.boolean(result.scalable_50k_claimed);
  writer.boolean(result.global_m1_claimed);
  writer.boolean(result.public_status_claimed);
  // Derived digest/self-certification booleans are deliberately excluded.
  writer.boolean(result.failure_payload_empty);
  writer.boolean(result.atomic_failure);
  writer.enumeration(result.decision);
  writer.enumeration(result.scope);
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
         verification
             .exact_persistent_reduced_gamma_order_history_decision_certified;
}

[[nodiscard]] bool budget_within_supported_bounds(
    const ExactK2K1HierarchyBudget& budget) noexcept {
  return budget.maximum_source_batch_count <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_source_batch_count &&
         budget.maximum_source_group_count <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_source_group_count &&
         budget.maximum_source_node_count <=
             ExactK2K1HierarchyBudget::maximum_supported_source_node_count &&
         budget.maximum_source_child_reference_count <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_source_child_reference_count &&
         budget.maximum_source_root_reference_count <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_source_root_reference_count &&
         budget.maximum_checkpoint_count <=
             ExactK2K1HierarchyBudget::maximum_supported_checkpoint_count &&
         budget.maximum_source_facet_replay_count <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_source_facet_replay_count &&
         budget.maximum_vertical_endpoint_lookup_count <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_vertical_endpoint_lookup_count &&
         budget.maximum_k1_parent_hop_count <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_k1_parent_hop_count &&
         budget.maximum_target_coverage_point_reference_count <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_target_coverage_point_reference_count &&
         budget.maximum_digest_logical_entry_count <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_digest_logical_entry_count &&
         budget.maximum_digest_exact_text_byte_count <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_digest_exact_text_byte_count;
}

[[nodiscard]] bool structural_budget_sufficient(
    const ExactK2K1HierarchyResult& result,
    const ExactK2K1HierarchyBudget& budget) noexcept {
  return result.required_source_batch_capacity <=
             budget.maximum_source_batch_count &&
         result.required_source_group_capacity <=
             budget.maximum_source_group_count &&
         result.required_source_node_capacity <=
             budget.maximum_source_node_count &&
         result.required_source_child_reference_capacity <=
             budget.maximum_source_child_reference_count &&
         result.required_source_root_reference_capacity <=
             budget.maximum_source_root_reference_count &&
         result.required_checkpoint_capacity <=
             budget.maximum_checkpoint_count;
}

[[nodiscard]] bool dynamic_budget_sufficient(
    const ExactK2K1HierarchyResult& result,
    const ExactK2K1HierarchyBudget& budget) noexcept {
  return result.preflight_source_facet_replay_upper_bound <=
             budget.maximum_source_facet_replay_count &&
         result.preflight_vertical_endpoint_lookup_upper_bound <=
             budget.maximum_vertical_endpoint_lookup_count &&
         result.preflight_k1_parent_hop_upper_bound <=
             budget.maximum_k1_parent_hop_count &&
         result.preflight_target_coverage_point_reference_upper_bound <=
             budget.maximum_target_coverage_point_reference_count &&
         result.required_digest_logical_entry_capacity <=
             budget.maximum_digest_logical_entry_count &&
         result.required_digest_exact_text_byte_capacity <=
             budget.maximum_digest_exact_text_byte_count;
}

[[nodiscard]] bool dynamic_requirements_within_supported_bounds(
    const ExactK2K1HierarchyResult& result) noexcept {
  return result.preflight_source_facet_replay_upper_bound <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_source_facet_replay_count &&
         result.preflight_vertical_endpoint_lookup_upper_bound <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_vertical_endpoint_lookup_count &&
         result.preflight_k1_parent_hop_upper_bound <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_k1_parent_hop_count &&
         result.preflight_target_coverage_point_reference_upper_bound <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_target_coverage_point_reference_count &&
         result.required_digest_logical_entry_capacity <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_digest_logical_entry_count &&
         result.required_digest_exact_text_byte_capacity <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_digest_exact_text_byte_count;
}

[[nodiscard]] bool structural_requirements_within_supported_bounds(
    const ExactK2K1HierarchyResult& result) noexcept {
  return result.required_source_batch_capacity <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_source_batch_count &&
         result.required_source_group_capacity <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_source_group_count &&
         result.required_source_node_capacity <=
             ExactK2K1HierarchyBudget::maximum_supported_source_node_count &&
         result.required_source_child_reference_capacity <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_source_child_reference_count &&
         result.required_source_root_reference_capacity <=
             ExactK2K1HierarchyBudget::
                 maximum_supported_source_root_reference_count &&
         result.required_checkpoint_capacity <=
             ExactK2K1HierarchyBudget::maximum_supported_checkpoint_count;
}

[[nodiscard]] bool all_requirements_zero(
    const ExactK2K1HierarchyResult& result) noexcept {
  return result.required_source_batch_capacity == 0U &&
         result.required_source_group_capacity == 0U &&
         result.required_source_node_capacity == 0U &&
         result.required_source_child_reference_capacity == 0U &&
         result.required_source_root_reference_capacity == 0U &&
         result.required_checkpoint_capacity == 0U &&
         result.preflight_source_facet_replay_upper_bound == 0U &&
         result.preflight_vertical_endpoint_lookup_upper_bound == 0U &&
         result.preflight_k1_parent_hop_upper_bound == 0U &&
         result.preflight_target_coverage_point_reference_upper_bound == 0U &&
         result.required_digest_logical_entry_capacity == 0U &&
         result.required_digest_exact_text_byte_capacity == 0U;
}

enum class CutBoundary : std::uint8_t {
  strict_open,
  closed,
};

struct K1ParentIndex {
  static constexpr K1NodeId no_parent =
      std::numeric_limits<K1NodeId>::max();

  std::vector<K1NodeId> parent_by_node;
};

[[nodiscard]] K1ParentIndex build_k1_parent_index(
    const K1CompactForest& forest) {
  K1ParentIndex index;
  index.parent_by_node.assign(forest.node_count(), K1ParentIndex::no_parent);
  for (const K1CompactMergeNode& node : forest.merge_nodes) {
    if (node.id >= index.parent_by_node.size()) {
      throw std::logic_error("a compact K1 node identifier exceeds its arena");
    }
    for (const K1NodeId child : forest.children(node.id)) {
      if (child >= index.parent_by_node.size() || child == node.id ||
          index.parent_by_node[child] != K1ParentIndex::no_parent) {
        throw std::logic_error(
            "the compact K1 child-to-parent relation is not a forest");
      }
      index.parent_by_node[child] = node.id;
    }
  }
  if (forest.root_node_id >= index.parent_by_node.size() ||
      index.parent_by_node[forest.root_node_id] != K1ParentIndex::no_parent) {
    throw std::logic_error("the compact K1 root has an invalid parent");
  }
  for (K1NodeId node_id = 0U; node_id < index.parent_by_node.size();
       ++node_id) {
    if (node_id != forest.root_node_id &&
        index.parent_by_node[node_id] == K1ParentIndex::no_parent) {
      throw std::logic_error("the compact K1 forest is disconnected");
    }
  }
  return index;
}

[[nodiscard]] bool parent_visible_at_cut(
    const exact::ExactLevel& parent_level,
    const exact::ExactLevel& cut_level,
    CutBoundary boundary,
    ExactK2K1HierarchyCounters& counters) {
  increment(
      counters.exact_level_comparison_count,
      "the exact-level comparison count overflows");
  if (boundary == CutBoundary::strict_open) {
    return parent_level < cut_level;
  }
  return parent_level <= cut_level;
}

[[nodiscard]] K1NodeId normalize_k1_target(
    const K1CompactForest& forest,
    const K1ParentIndex& parent_index,
    K1NodeId node_id,
    const exact::ExactLevel& cut_level,
    CutBoundary boundary,
    ExactK2K1HierarchyCounters& counters) {
  if (node_id >= parent_index.parent_by_node.size()) {
    throw std::logic_error("a K1 target identifier exceeds its arena");
  }
  while (parent_index.parent_by_node[node_id] != K1ParentIndex::no_parent) {
    const K1NodeId parent = parent_index.parent_by_node[node_id];
    if (!parent_visible_at_cut(
            forest.node_level(parent), cut_level, boundary, counters)) {
      break;
    }
    node_id = parent;
    increment(
        counters.k1_parent_hop_count,
        "the K1 parent-hop count overflows");
  }
  return node_id;
}

using Facet = std::vector<spatial::PointId>;
using FacetSet = std::set<Facet>;
using PointSet = std::set<spatial::PointId>;

struct SourceRootState {
  FacetSet facets;
  PointSet covered_points;
  K1NodeId target_node_id{};
  std::optional<std::size_t> checkpoint_index;
};

[[nodiscard]] bool is_subset(
    const PointSet& subset,
    std::span<const spatial::PointId> sorted_superset) {
  return std::includes(
      sorted_superset.begin(),
      sorted_superset.end(),
      subset.begin(),
      subset.end());
}

struct FacetCutAnalysis {
  K1NodeId target_node_id{};
  PointSet covered_points;
};

[[nodiscard]] FacetCutAnalysis analyze_facets_at_cut(
    const FacetSet& facets,
    const K1CompactForest& forest,
    const K1ParentIndex& parent_index,
    const exact::ExactLevel& level,
    CutBoundary boundary,
    ExactK2K1HierarchyCounters& counters) {
  std::optional<K1NodeId> target;
  PointSet covered_points;
  for (const Facet& facet : facets) {
    if (facet.size() != 2U || facet[0] >= facet[1]) {
      throw std::logic_error(
          "the order-two source replay encountered a noncanonical facet");
    }
    increment(
        counters.source_facet_replay_count,
        "the source-facet replay count overflows");
    for (const spatial::PointId point_id : facet) {
      covered_points.insert(point_id);
      increment(
          counters.vertical_endpoint_lookup_count,
          "the vertical endpoint-lookup count overflows");
      const K1NodeId endpoint_target = normalize_k1_target(
          forest,
          parent_index,
          static_cast<K1NodeId>(point_id),
          level,
          boundary,
          counters);
      if (!target.has_value()) {
        target = endpoint_target;
      } else if (*target != endpoint_target) {
        throw std::logic_error(
            "one reduced-Gamma component maps to multiple K1 cut roots");
      }
    }
  }
  if (!target.has_value()) {
    throw std::logic_error("a nontrivial reduced-Gamma root has no facets");
  }
  return FacetCutAnalysis{*target, std::move(covered_points)};
}

[[nodiscard]] std::size_t certify_coverage_containment(
    const SourceRootState& state,
    const K1CompactForest& forest,
    ExactK2K1HierarchyCounters& counters) {
  const K1HierarchyNode target = forest.materialize_node(state.target_node_id);
  counters.target_coverage_point_reference_count = checked_add(
      counters.target_coverage_point_reference_count,
      target.point_ids.size(),
      "the target-coverage point-reference count overflows");
  if (!is_subset(state.covered_points, target.point_ids)) {
    throw std::logic_error(
        "a reduced-Gamma point coverage escapes its K1 target");
  }
  return target.point_ids.size();
}

void audit_vertical_square(
    const std::map<std::size_t, SourceRootState>& active_roots,
    const K1CompactForest& forest,
    const K1ParentIndex& parent_index,
    const exact::ExactLevel& level,
    CutBoundary boundary,
    ExactK2K1HierarchyCounters& counters) {
  for (const auto& [root_id, state] : active_roots) {
    static_cast<void>(root_id);
    FacetCutAnalysis analysis = analyze_facets_at_cut(
        state.facets, forest, parent_index, level, boundary, counters);
    if (analysis.target_node_id != state.target_node_id ||
        analysis.covered_points != state.covered_points) {
      throw std::logic_error(
          "a strict/closed vertical square failed exact source replay");
    }
    static_cast<void>(certify_coverage_containment(state, forest, counters));
  }
}

[[nodiscard]] ExactK2K1VerticalCheckpointKind checkpoint_kind(
    ExactReducedGammaBatchGroupKind kind) {
  switch (kind) {
    case ExactReducedGammaBatchGroupKind::birth:
      return ExactK2K1VerticalCheckpointKind::birth;
    case ExactReducedGammaBatchGroupKind::continuation:
      return ExactK2K1VerticalCheckpointKind::continuation;
    case ExactReducedGammaBatchGroupKind::multifusion:
      return ExactK2K1VerticalCheckpointKind::multifusion;
    case ExactReducedGammaBatchGroupKind::deferred_isolated_facet:
      break;
  }
  throw std::logic_error("a deferred source group has no checkpoint kind");
}

struct PendingSourceOutput {
  std::size_t root_node_id{};
  SourceRootState state;
  ExactK2K1VerticalCheckpoint checkpoint;
};

void normalize_active_roots(
    std::map<std::size_t, SourceRootState>& active_roots,
    const K1CompactForest& forest,
    const K1ParentIndex& parent_index,
    const exact::ExactLevel& level,
    CutBoundary boundary,
    ExactK2K1HierarchyCounters& counters) {
  for (auto& [root_id, state] : active_roots) {
    static_cast<void>(root_id);
    const K1NodeId prior_target = state.target_node_id;
    state.target_node_id = normalize_k1_target(
        forest,
        parent_index,
        prior_target,
        level,
        boundary,
        counters);
    if (boundary == CutBoundary::strict_open) {
      increment(
          counters.strict_open_normalization_check_count,
          "the strict-open normalization-check count overflows");
    }
    if (state.target_node_id != prior_target) {
      increment(
          counters.k1_target_normalization_update_count,
          "the K1 target-normalization update count overflows");
    }
  }
}

[[nodiscard]] PointSet point_set(
    std::span<const spatial::PointId> point_ids) {
  return PointSet{point_ids.begin(), point_ids.end()};
}

[[nodiscard]] std::vector<PendingSourceOutput> stage_source_batch(
    const ExactPersistentReducedGammaOrderHistory& source,
    const ExactPersistentReducedGammaBatchMetadata& batch,
    const std::map<std::size_t, SourceRootState>& strict_snapshot,
    const K1CompactForest& forest,
    const K1ParentIndex& parent_index,
    ExactK2K1HierarchyResult& result,
    std::set<std::size_t>& consumed_roots) {
  const std::size_t group_end = checked_add(
      batch.first_group_record_index,
      batch.group_record_count,
      "a Gamma2 batch range overflows");
  if (group_end > source.group_records.size()) {
    throw std::logic_error("a Gamma2 batch exceeds its group-record arena");
  }

  std::vector<PendingSourceOutput> outputs;
  outputs.reserve(batch.group_record_count);
  for (std::size_t group_index = batch.first_group_record_index;
       group_index < group_end;
       ++group_index) {
    const ExactPersistentReducedGammaHistoryGroupRecord& record =
        source.group_records[group_index];
    if (record.group_record_index != group_index ||
        record.batch_index != batch.batch_index ||
        record.batch_group_index !=
            group_index - batch.first_group_record_index ||
        record.squared_level != batch.squared_level) {
      throw std::logic_error(
          "a Gamma2 group is not canonically indexed within its batch");
    }
    if (record.kind ==
        ExactReducedGammaBatchGroupKind::deferred_isolated_facet) {
      continue;
    }
    if (!record.resulting_root_node_id.has_value() ||
        !record.coverage_delta.has_value()) {
      throw std::logic_error(
          "a non-deferred Gamma2 group lacks its output or coverage delta");
    }

    PendingSourceOutput pending;
    pending.root_node_id = *record.resulting_root_node_id;
    PointSet prior_covered_points;
    pending.checkpoint.strict_prior_target_node_ids.reserve(
        record.prior_root_node_ids.size());
    bool every_prior_has_checkpoint = true;
    for (const std::size_t prior_root_id : record.prior_root_node_ids) {
      const auto prior = strict_snapshot.find(prior_root_id);
      if (prior == strict_snapshot.end() ||
          !consumed_roots.insert(prior_root_id).second) {
        throw std::logic_error(
            "a Gamma2 batch does not consume frozen roots exactly once");
      }
      pending.state.facets.insert(
          prior->second.facets.begin(), prior->second.facets.end());
      prior_covered_points.insert(
          prior->second.covered_points.begin(),
          prior->second.covered_points.end());
      pending.checkpoint.strict_prior_target_node_ids.push_back(
          prior->second.target_node_id);
      every_prior_has_checkpoint =
          every_prior_has_checkpoint &&
          prior->second.checkpoint_index.has_value();
      increment(
          result.counters.strict_open_target_reference_count,
          "the strict-open target-reference count overflows");
    }

    for (const Facet& facet :
         record.coverage_delta->added_facet_point_ids) {
      if (!pending.state.facets.insert(facet).second) {
        throw std::logic_error(
            "a Gamma2 coverage delta repeats an existing source facet");
      }
    }
    FacetCutAnalysis closed_analysis = analyze_facets_at_cut(
        pending.state.facets,
        forest,
        parent_index,
        batch.squared_level,
        CutBoundary::closed,
        result.counters);
    pending.state.covered_points = std::move(closed_analysis.covered_points);
    PointSet expected_covered_points = prior_covered_points;
    expected_covered_points.insert(
        record.coverage_delta->added_point_ids.begin(),
        record.coverage_delta->added_point_ids.end());
    if (pending.state.covered_points != expected_covered_points) {
      throw std::logic_error(
          "a Gamma2 coverage delta does not reproduce exact point coverage");
    }

    pending.state.target_node_id = closed_analysis.target_node_id;
    const std::size_t closed_target_coverage_size =
        certify_coverage_containment(
            pending.state, forest, result.counters);

    bool strict_targets_close_to_output = true;
    for (const K1NodeId strict_target :
         pending.checkpoint.strict_prior_target_node_ids) {
      const K1NodeId closed_target = normalize_k1_target(
          forest,
          parent_index,
          strict_target,
          batch.squared_level,
          CutBoundary::closed,
          result.counters);
      strict_targets_close_to_output =
          strict_targets_close_to_output &&
          closed_target == pending.state.target_node_id;
      increment(
          result.counters.strict_to_closed_naturality_check_count,
          "the strict-to-closed naturality-check count overflows");
      increment(
          result.counters.prior_target_naturality_check_count,
          "the prior-target naturality-check count overflows");
    }
    if (!every_prior_has_checkpoint || !strict_targets_close_to_output) {
      throw std::logic_error(
          "a Gamma2 prior target does not close naturally in K1");
    }

    pending.checkpoint.checkpoint_index =
        result.vertical_checkpoints.size() + outputs.size();
    pending.checkpoint.source_batch_index = batch.batch_index;
    pending.checkpoint.source_group_record_index =
        record.group_record_index;
    pending.checkpoint.source_batch_group_index = record.batch_group_index;
    pending.checkpoint.squared_level = record.squared_level;
    pending.checkpoint.source_root_node_id = pending.root_node_id;
    pending.checkpoint.closed_target_node_id =
        pending.state.target_node_id;
    pending.checkpoint.kind = checkpoint_kind(record.kind);
    pending.checkpoint.prior_source_root_count =
        record.prior_root_node_ids.size();
    pending.checkpoint.source_facet_count = pending.state.facets.size();
    pending.checkpoint.source_covered_point_count =
        pending.state.covered_points.size();
    pending.checkpoint.closed_target_covered_point_count =
        closed_target_coverage_size;
    pending.checkpoint.added_facet_count =
        record.coverage_delta->added_facet_point_ids.size();
    pending.checkpoint.added_point_count =
        record.coverage_delta->added_point_ids.size();
    pending.checkpoint.source_group_resolved_from_frozen_snapshot =
        record.resolved_from_pre_batch_snapshot &&
        batch.pre_batch_root_bijection_certified &&
        batch.all_groups_resolved_before_mutation;
    pending.checkpoint.every_prior_root_has_pre_batch_checkpoint =
        every_prior_has_checkpoint;
    pending.checkpoint.prior_targets_normalize_to_strict_open_targets = true;
    pending.checkpoint.strict_open_targets_normalize_to_closed_target =
        strict_targets_close_to_output;
    pending.checkpoint.all_source_facets_map_to_closed_target = true;
    pending.checkpoint.source_coverage_contained_in_closed_target = true;
    pending.state.checkpoint_index = pending.checkpoint.checkpoint_index;

    switch (pending.checkpoint.kind) {
      case ExactK2K1VerticalCheckpointKind::birth:
        increment(
            result.counters.birth_checkpoint_count,
            "the birth-checkpoint count overflows");
        break;
      case ExactK2K1VerticalCheckpointKind::continuation:
        increment(
            result.counters.continuation_checkpoint_count,
            "the continuation-checkpoint count overflows");
        break;
      case ExactK2K1VerticalCheckpointKind::multifusion:
        increment(
            result.counters.multifusion_checkpoint_count,
            "the multifusion-checkpoint count overflows");
        break;
    }
    if (record.coverage_delta->fully_redundant) {
      increment(
          result.counters.fully_redundant_checkpoint_count,
          "the fully-redundant checkpoint count overflows");
    }
    outputs.push_back(std::move(pending));
  }
  return outputs;
}

void replay_union_of_exact_levels(
    const ExactPersistentReducedGammaOrderHistory& source,
    const K1CompactForest& forest,
    ExactK2K1HierarchyResult& result) {
  const K1ParentIndex parent_index = build_k1_parent_index(forest);
  std::set<exact::ExactLevel> critical_levels{
      source.activation_levels.begin(), source.activation_levels.end()};
  critical_levels.insert(forest.levels.begin(), forest.levels.end());

  std::map<std::size_t, SourceRootState> active_roots;
  std::size_t next_source_batch_index = 0U;
  for (const exact::ExactLevel& level : critical_levels) {
    normalize_active_roots(
        active_roots,
        forest,
        parent_index,
        level,
        CutBoundary::strict_open,
        result.counters);
    audit_vertical_square(
        active_roots,
        forest,
        parent_index,
        level,
        CutBoundary::strict_open,
        result.counters);

    const ExactPersistentReducedGammaBatchMetadata* source_batch = nullptr;
    if (next_source_batch_index < source.batch_metadata.size() &&
        source.batch_metadata[next_source_batch_index].squared_level == level) {
      source_batch = &source.batch_metadata[next_source_batch_index];
      if (source_batch->batch_index != next_source_batch_index) {
        throw std::logic_error("the Gamma2 batch identifiers are not dense");
      }
      ++next_source_batch_index;
    }

    const auto strict_snapshot = active_roots;
    std::map<std::size_t, SourceRootState> closed_roots = strict_snapshot;
    normalize_active_roots(
        closed_roots,
        forest,
        parent_index,
        level,
        CutBoundary::closed,
        result.counters);
    if (source_batch != nullptr) {
      std::set<std::size_t> consumed_roots;
      std::vector<PendingSourceOutput> outputs = stage_source_batch(
          source,
          *source_batch,
          strict_snapshot,
          forest,
          parent_index,
          result,
          consumed_roots);
      for (const std::size_t root_id : consumed_roots) {
        if (closed_roots.erase(root_id) != 1U) {
          throw std::logic_error(
              "a staged Gamma2 source root disappeared before mutation");
        }
      }
      for (PendingSourceOutput& output : outputs) {
        if (!closed_roots
                 .emplace(output.root_node_id, std::move(output.state))
                 .second) {
          throw std::logic_error("a Gamma2 batch repeats an output root");
        }
        result.vertical_checkpoints.push_back(
            std::move(output.checkpoint));
      }
      result.counters.consumed_source_root_count = checked_add(
          result.counters.consumed_source_root_count,
          consumed_roots.size(),
          "the consumed source-root count overflows");
      result.counters.produced_source_root_count = checked_add(
          result.counters.produced_source_root_count,
          outputs.size(),
          "the produced source-root count overflows");
    }
    active_roots = std::move(closed_roots);
    audit_vertical_square(
        active_roots,
        forest,
        parent_index,
        level,
        CutBoundary::closed,
        result.counters);
  }

  if (next_source_batch_index != source.batch_metadata.size() ||
      active_roots.size() != source.final_active_roots.size()) {
    throw std::logic_error(
        "the strict/closed union sweep did not close the Gamma2 history");
  }
  for (const ExactPersistentReducedGammaActiveRoot& source_root :
       source.final_active_roots) {
    const auto replayed = active_roots.find(source_root.root_node_id);
    if (replayed == active_roots.end() ||
        replayed->second.facets !=
            FacetSet{
                source_root.facet_point_ids.begin(),
                source_root.facet_point_ids.end()} ||
        replayed->second.covered_points !=
            point_set(source_root.covered_point_ids) ||
        replayed->second.target_node_id != forest.root_node_id) {
      throw std::logic_error(
          "the final Gamma2 root or its K1 target differs from fresh replay");
    }
  }
  result.counters.final_source_root_count = active_roots.size();
  result.counters.checkpoint_count = result.vertical_checkpoints.size();
}

void append_canonical_cloud_projection(
    DigestWriter& writer, const spatial::CanonicalPointCloud& cloud) {
  writer.size(cloud.size());
  for (std::size_t point_index = 0U; point_index < cloud.size();
       ++point_index) {
    const auto bits =
        cloud.point(static_cast<spatial::PointId>(point_index))
            .canonical_input_bits();
    for (const std::uint64_t word : bits) {
      writer.u64(word);
    }
  }
}

[[nodiscard]] DigestMetrics measure_canonical_cloud_projection(
    const spatial::CanonicalPointCloud& cloud) {
  DigestWriter writer{
      "MorseHGP3D/phase15-tg2a/canonical-cloud/v1", false};
  append_canonical_cloud_projection(writer, cloud);
  return writer.metrics();
}

[[nodiscard]] contract::CanonicalId canonical_cloud_projection_digest(
    const spatial::CanonicalPointCloud& cloud) {
  DigestWriter writer{
      "MorseHGP3D/phase15-tg2a/canonical-cloud/v1", true};
  append_canonical_cloud_projection(writer, cloud);
  return writer.finalize();
}

[[nodiscard]] DigestMetrics measure_gamma_source_projection(
    const ExactPersistentReducedGammaOrderHistory& source) {
  DigestWriter writer{
      "MorseHGP3D/phase15-tg2a/gamma2-horizontal-projection/v1", false};
  append_gamma_source_projection(writer, source);
  return writer.metrics();
}

[[nodiscard]] contract::CanonicalId gamma_source_projection_digest(
    const ExactPersistentReducedGammaOrderHistory& source) {
  DigestWriter writer{
      "MorseHGP3D/phase15-tg2a/gamma2-horizontal-projection/v1", true};
  append_gamma_source_projection(writer, source);
  return writer.finalize();
}

[[nodiscard]] DigestMetrics measure_k1_projection(
    const K1CompactForest& forest) {
  DigestWriter writer{
      "MorseHGP3D/phase15-tg2a/exact-emst-compact-k1-projection/v1",
      false};
  append_k1_projection(writer, forest);
  return writer.metrics();
}

[[nodiscard]] contract::CanonicalId k1_projection_digest(
    const K1CompactForest& forest) {
  DigestWriter writer{
      "MorseHGP3D/phase15-tg2a/exact-emst-compact-k1-projection/v1",
      true};
  append_k1_projection(writer, forest);
  return writer.finalize();
}

[[nodiscard]] DigestMetrics measure_hierarchy_projection(
    const ExactK2K1HierarchyResult& result) {
  DigestWriter writer{
      "MorseHGP3D/phase15-tg2a/exact-k2-k1-hierarchy/v1", false};
  append_hierarchy_projection(writer, result);
  return writer.metrics();
}

[[nodiscard]] contract::CanonicalId compute_hierarchy_projection_digest(
    const ExactK2K1HierarchyResult& result) {
  DigestWriter writer{
      "MorseHGP3D/phase15-tg2a/exact-k2-k1-hierarchy/v1", true};
  append_hierarchy_projection(writer, result);
  return writer.finalize();
}

[[nodiscard]] DigestMetrics add_digest_metrics(
    DigestMetrics left, const DigestMetrics& right) {
  left.logical_entry_count = checked_add(
      left.logical_entry_count,
      right.logical_entry_count,
      "the aggregate digest logical-entry count overflows");
  left.exact_text_byte_count = checked_add(
      left.exact_text_byte_count,
      right.exact_text_byte_count,
      "the aggregate digest exact-text byte count overflows");
  return left;
}

[[nodiscard]] ExactK2K1HierarchyResult base_result(
    std::size_t point_count, const ExactK2K1HierarchyBudget& budget) {
  ExactK2K1HierarchyResult result;
  result.requested_budget = budget;
  result.point_count = point_count;
  result.scope = ExactK2K1HierarchyScope::
      bounded_n14_exact_exhaustive_gamma2_to_complete_graph_emst_k1_only;
  return result;
}

[[nodiscard]] ExactK2K1HierarchyResult atomic_failure_result(
    const ExactK2K1HierarchyResult& requirements,
    ExactK2K1HierarchyDecision decision) {
  ExactK2K1HierarchyResult failure =
      base_result(requirements.point_count, requirements.requested_budget);
  failure.required_source_batch_capacity =
      requirements.required_source_batch_capacity;
  failure.required_source_group_capacity =
      requirements.required_source_group_capacity;
  failure.required_source_node_capacity =
      requirements.required_source_node_capacity;
  failure.required_source_child_reference_capacity =
      requirements.required_source_child_reference_capacity;
  failure.required_source_root_reference_capacity =
      requirements.required_source_root_reference_capacity;
  failure.required_checkpoint_capacity =
      requirements.required_checkpoint_capacity;
  failure.preflight_source_facet_replay_upper_bound =
      requirements.preflight_source_facet_replay_upper_bound;
  failure.preflight_vertical_endpoint_lookup_upper_bound =
      requirements.preflight_vertical_endpoint_lookup_upper_bound;
  failure.preflight_k1_parent_hop_upper_bound =
      requirements.preflight_k1_parent_hop_upper_bound;
  failure.preflight_target_coverage_point_reference_upper_bound =
      requirements.preflight_target_coverage_point_reference_upper_bound;
  failure.required_digest_logical_entry_capacity =
      requirements.required_digest_logical_entry_capacity;
  failure.required_digest_exact_text_byte_capacity =
      requirements.required_digest_exact_text_byte_capacity;
  failure.failure_payload_empty = true;
  failure.atomic_failure = true;
  failure.decision = decision;
  return failure;
}

[[nodiscard]] bool complete_source_decision(
    const ExactPersistentReducedGammaOrderHistory& source) {
  return source.decision ==
             ExactPersistentReducedGammaOrderHistoryDecision::
                 complete_persistent_reduced_gamma_history ||
         source.decision ==
             ExactPersistentReducedGammaOrderHistoryDecision::
                 complete_empty_terminal_order;
}

void derive_projection_preflight_requirements(
    const spatial::CanonicalPointCloud& cloud,
    const ExactPersistentReducedGammaOrderHistory& source,
    const K1CompactForest& forest,
    ExactK2K1HierarchyResult& result) {
  std::set<exact::ExactLevel> critical_levels{
      source.activation_levels.begin(), source.activation_levels.end()};
  critical_levels.insert(forest.levels.begin(), forest.levels.end());
  const std::size_t two_cut_count = checked_multiply(
      2U,
      critical_levels.size(),
      "the strict/closed cut count overflows");
  const std::size_t square_facet_visits = checked_multiply(
      two_cut_count,
      source.exhaustive_facet_count,
      "the square facet-visit bound overflows");
  const std::size_t checkpoint_facet_visits = checked_multiply(
      result.required_checkpoint_capacity,
      source.exhaustive_facet_count,
      "the checkpoint facet-visit bound overflows");
  result.preflight_source_facet_replay_upper_bound = checked_add(
      square_facet_visits,
      checkpoint_facet_visits,
      "the aggregate facet-replay bound overflows");
  result.preflight_vertical_endpoint_lookup_upper_bound = checked_multiply(
      2U,
      result.preflight_source_facet_replay_upper_bound,
      "the endpoint-lookup bound overflows");

  const std::size_t active_root_normalization_count = checked_multiply(
      two_cut_count,
      source.nodes.size(),
      "the active-root normalization bound overflows");
  const std::size_t total_normalization_count = checked_add(
      checked_add(
          result.preflight_vertical_endpoint_lookup_upper_bound,
          active_root_normalization_count,
          "the K1 normalization bound overflows"),
      source.counters.group_root_reference_count,
      "the K1 normalization bound overflows");
  result.preflight_k1_parent_hop_upper_bound = checked_multiply(
      total_normalization_count,
      forest.merge_nodes.size(),
      "the K1 parent-hop bound overflows");

  const std::size_t square_target_materialization_count = checked_multiply(
      two_cut_count,
      source.nodes.size(),
      "the square target-materialization bound overflows");
  const std::size_t target_materialization_count = checked_add(
      square_target_materialization_count,
      result.required_checkpoint_capacity,
      "the target-materialization bound overflows");
  result.preflight_target_coverage_point_reference_upper_bound =
      checked_multiply(
          target_materialization_count,
          cloud.size(),
          "the target-coverage point-reference bound overflows");

  ExactK2K1HierarchyResult digest_shape = result;
  digest_shape.vertical_checkpoints.reserve(
      result.required_checkpoint_capacity);
  for (const ExactPersistentReducedGammaHistoryGroupRecord& record :
       source.group_records) {
    if (record.kind ==
        ExactReducedGammaBatchGroupKind::deferred_isolated_facet) {
      continue;
    }
    ExactK2K1VerticalCheckpoint checkpoint;
    checkpoint.squared_level = record.squared_level;
    checkpoint.strict_prior_target_node_ids.resize(
        record.prior_root_node_ids.size());
    digest_shape.vertical_checkpoints.push_back(std::move(checkpoint));
  }
  if (digest_shape.vertical_checkpoints.size() !=
      result.required_checkpoint_capacity) {
    throw std::logic_error(
        "the digest-shape checkpoint count differs from source counters");
  }
  DigestMetrics aggregate = measure_canonical_cloud_projection(cloud);
  aggregate =
      add_digest_metrics(aggregate, measure_gamma_source_projection(source));
  aggregate =
      add_digest_metrics(aggregate, measure_k1_projection(forest));
  aggregate =
      add_digest_metrics(aggregate, measure_hierarchy_projection(digest_shape));
  result.required_digest_logical_entry_capacity =
      aggregate.logical_entry_count;
  result.required_digest_exact_text_byte_capacity =
      aggregate.exact_text_byte_count;
}

[[nodiscard]] ExactK2K1HierarchyResult compute_exact_k2_k1_hierarchy(
    const spatial::CanonicalPointCloud& cloud,
    ExactPersistentReducedGammaOrderHistoryBudget trusted_source_budget,
    const ExactPersistentReducedGammaOrderHistory& source_history,
    const K1CompactForest& target_k1_forest,
    const ExactK2K1HierarchyBudget& budget) {
  if (cloud.size() <
          ExactPersistentReducedGammaOrderHistory::
              minimum_supported_point_count ||
      cloud.size() >
          ExactK2K1HierarchyBudget::maximum_supported_point_count) {
    throw std::invalid_argument(
        "the exact Gamma2-to-K1 hierarchy requires 2<=n<=14");
  }
  if (!budget_within_supported_bounds(budget)) {
    throw std::invalid_argument(
        "an exact Gamma2-to-K1 budget exceeds its supported hard cap");
  }

  ExactK2K1HierarchyResult result = base_result(cloud.size(), budget);
  const ExactPersistentReducedGammaOrderHistoryVerification
      source_verification =
          verify_exact_persistent_reduced_gamma_order_history(
              cloud, 2U, trusted_source_budget, source_history);
  if (!gamma_source_verification_closes(source_verification) ||
      source_history.point_count != cloud.size() ||
      source_history.order != 2U || !complete_source_decision(source_history)) {
    return atomic_failure_result(
        result,
        ExactK2K1HierarchyDecision::no_source_history_rejected);
  }

  const K1CompactForest fresh_k1 =
      build_compact_k1_forest(build_exact_complete_graph_emst(cloud));
  if (!same_k1_compact_forest(fresh_k1, target_k1_forest)) {
    return atomic_failure_result(
        result, ExactK2K1HierarchyDecision::no_k1_forest_rejected);
  }

  result.required_source_batch_capacity = source_history.batch_metadata.size();
  result.required_source_group_capacity = source_history.group_records.size();
  result.required_source_node_capacity = source_history.nodes.size();
  result.required_source_child_reference_capacity =
      source_history.counters.child_reference_count;
  result.required_source_root_reference_capacity =
      source_history.counters.group_root_reference_count;
  result.required_checkpoint_capacity = checked_add(
      checked_add(
          source_history.counters.birth_group_count,
          source_history.counters.continuation_group_count,
          "the required checkpoint count overflows"),
      source_history.counters.multifusion_group_count,
      "the required checkpoint count overflows");
  if (!structural_requirements_within_supported_bounds(result)) {
    throw std::length_error(
        "the bounded source exceeded a proved structural hard cap");
  }
  if (!structural_budget_sufficient(result, budget)) {
    return atomic_failure_result(
        result,
        ExactK2K1HierarchyDecision::
            no_projection_preflight_budget_insufficient);
  }

  derive_projection_preflight_requirements(
      cloud, source_history, fresh_k1, result);
  if (!dynamic_requirements_within_supported_bounds(result)) {
    throw std::length_error(
        "the bounded exact vertical preflight exceeded a proved hard cap");
  }
  if (!dynamic_budget_sufficient(result, budget)) {
    return atomic_failure_result(
        result,
        ExactK2K1HierarchyDecision::
            no_projection_preflight_budget_insufficient);
  }

  result.counters.source_batch_count = source_history.batch_metadata.size();
  result.counters.source_group_count = source_history.group_records.size();
  result.counters.source_node_count = source_history.nodes.size();
  result.counters.source_child_reference_count =
      source_history.counters.child_reference_count;
  result.counters.source_root_reference_count =
      source_history.counters.group_root_reference_count;
  result.counters.deferred_group_count =
      source_history.counters.deferred_group_count;
  result.vertical_checkpoints.reserve(result.required_checkpoint_capacity);

  replay_union_of_exact_levels(source_history, fresh_k1, result);
  if (result.counters.birth_checkpoint_count !=
          source_history.counters.birth_group_count ||
      result.counters.continuation_checkpoint_count !=
          source_history.counters.continuation_group_count ||
      result.counters.multifusion_checkpoint_count !=
          source_history.counters.multifusion_group_count ||
      result.counters.fully_redundant_checkpoint_count !=
          source_history.counters.fully_redundant_group_count ||
      result.counters.checkpoint_count !=
          result.required_checkpoint_capacity ||
      result.counters.consumed_source_root_count !=
          source_history.counters.group_root_reference_count ||
      result.counters.produced_source_root_count !=
          result.required_checkpoint_capacity ||
      result.counters.final_source_root_count !=
          source_history.final_active_roots.size()) {
    throw std::logic_error(
        "the exact Gamma2-to-K1 checkpoint counters do not close");
  }
  if (result.counters.source_facet_replay_count >
          result.preflight_source_facet_replay_upper_bound ||
      result.counters.vertical_endpoint_lookup_count >
          result.preflight_vertical_endpoint_lookup_upper_bound ||
      result.counters.k1_parent_hop_count >
          result.preflight_k1_parent_hop_upper_bound ||
      result.counters.target_coverage_point_reference_count >
          result.preflight_target_coverage_point_reference_upper_bound) {
    throw std::logic_error(
        "the exact vertical sweep exceeded its arithmetic preflight");
  }

  result.provenance.source_horizontal_proof_basis =
      ExactPersistentReducedGammaOrderHistory::proof_basis;
  result.provenance.k1_target_proof_basis =
      "bounded_n14_fresh_exact_complete_graph_emst_equal_level_compact_k1_"
      "forest_v1";
  result.provenance.canonical_cloud_digest =
      canonical_cloud_projection_digest(cloud);
  result.provenance.source_horizontal_projection_digest =
      gamma_source_projection_digest(source_history);
  result.provenance.k1_compact_projection_digest =
      k1_projection_digest(fresh_k1);
  result.provenance.source_history_counters = source_history.counters;
  result.provenance.k1_forest_counters = fresh_k1.counters;
  result.provenance.source_history_fresh_replay_certified = true;
  result.provenance.source_horizontal_projection_complete = true;
  result.provenance.k1_complete_graph_emst_fresh_replay_certified = true;
  result.provenance.k1_compact_forest_fresh_replay_certified = true;
  result.provenance.conditional_direct_morse_authority_not_used = true;

  result.source_and_target_preflight_sizes_derived_from_fresh_replay = true;
  result.projection_budget_preflight_certified = true;
  result.all_source_batches_staged_before_root_mutation = true;
  result.every_non_deferred_group_has_one_checkpoint = true;
  result.every_checkpoint_uses_strict_open_and_closed_exact_k1_targets =
      true;
  result.source_facets_and_point_coverage_replayed_exactly = true;
  result.strict_open_closed_vertical_naturality_certified = true;
  result.k1_only_interlevel_target_evolution_certified = true;
  result
      .vertical_map_complete_for_bounded_hgp_reduced_k2_to_k1_source = true;
  result.all_order_vertical_maps_complete = false;
  result.horizontal_source_persisted_in_result = false;
  result.bounded_exhaustive_gamma_source_materialized =
      source_history.decision ==
      ExactPersistentReducedGammaOrderHistoryDecision::
          complete_persistent_reduced_gamma_history;
  result.product_architecture_claimed = false;
  result.scalable_50k_claimed = false;
  result.global_m1_claimed = false;
  result.public_status_claimed = false;
  result.output_digest_certified = true;
  result.exact_k2_k1_hierarchy_certified = true;
  result.failure_payload_empty = false;
  result.atomic_failure = false;
  result.decision = source_history.decision ==
                            ExactPersistentReducedGammaOrderHistoryDecision::
                                complete_empty_terminal_order
                        ? ExactK2K1HierarchyDecision::
                              complete_empty_terminal_order
                        : ExactK2K1HierarchyDecision::
                              complete_bounded_exact_k2_k1_hierarchy;

  result.counters.digest_logical_entry_count =
      result.required_digest_logical_entry_capacity;
  result.counters.digest_exact_text_byte_count =
      result.required_digest_exact_text_byte_capacity;
  DigestMetrics observed_digest_work =
      measure_canonical_cloud_projection(cloud);
  observed_digest_work = add_digest_metrics(
      observed_digest_work, measure_gamma_source_projection(source_history));
  observed_digest_work = add_digest_metrics(
      observed_digest_work, measure_k1_projection(fresh_k1));
  observed_digest_work = add_digest_metrics(
      observed_digest_work, measure_hierarchy_projection(result));
  if (observed_digest_work.logical_entry_count !=
          result.required_digest_logical_entry_capacity ||
      observed_digest_work.exact_text_byte_count !=
          result.required_digest_exact_text_byte_capacity) {
    throw std::logic_error(
        "the digest projection exceeded its exact arithmetic preflight");
  }
  result.hierarchy_projection_digest = compute_hierarchy_projection_digest(result);
  return result;
}

[[nodiscard]] bool zero_identifier(const contract::CanonicalId& value) {
  return value == contract::CanonicalId{};
}

[[nodiscard]] bool same_required_capacities(
    const ExactK2K1HierarchyResult& left,
    const ExactK2K1HierarchyResult& right) {
  return left.required_source_batch_capacity ==
             right.required_source_batch_capacity &&
         left.required_source_group_capacity ==
             right.required_source_group_capacity &&
         left.required_source_node_capacity ==
             right.required_source_node_capacity &&
         left.required_source_child_reference_capacity ==
             right.required_source_child_reference_capacity &&
         left.required_source_root_reference_capacity ==
             right.required_source_root_reference_capacity &&
         left.required_checkpoint_capacity ==
             right.required_checkpoint_capacity &&
         left.preflight_source_facet_replay_upper_bound ==
             right.preflight_source_facet_replay_upper_bound &&
         left.preflight_vertical_endpoint_lookup_upper_bound ==
             right.preflight_vertical_endpoint_lookup_upper_bound &&
         left.preflight_k1_parent_hop_upper_bound ==
             right.preflight_k1_parent_hop_upper_bound &&
         left.preflight_target_coverage_point_reference_upper_bound ==
             right.preflight_target_coverage_point_reference_upper_bound &&
         left.required_digest_logical_entry_capacity ==
             right.required_digest_logical_entry_capacity &&
         left.required_digest_exact_text_byte_capacity ==
             right.required_digest_exact_text_byte_capacity;
}

[[nodiscard]] bool same_result_facts(
    const ExactK2K1HierarchyResult& left,
    const ExactK2K1HierarchyResult& right) {
  return left.source_and_target_preflight_sizes_derived_from_fresh_replay ==
             right
                 .source_and_target_preflight_sizes_derived_from_fresh_replay &&
         left.projection_budget_preflight_certified ==
             right.projection_budget_preflight_certified &&
         left.all_source_batches_staged_before_root_mutation ==
             right.all_source_batches_staged_before_root_mutation &&
         left.every_non_deferred_group_has_one_checkpoint ==
             right.every_non_deferred_group_has_one_checkpoint &&
         left.every_checkpoint_uses_strict_open_and_closed_exact_k1_targets ==
             right
                 .every_checkpoint_uses_strict_open_and_closed_exact_k1_targets &&
         left.source_facets_and_point_coverage_replayed_exactly ==
             right.source_facets_and_point_coverage_replayed_exactly &&
         left.strict_open_closed_vertical_naturality_certified ==
             right.strict_open_closed_vertical_naturality_certified &&
         left.k1_only_interlevel_target_evolution_certified ==
             right.k1_only_interlevel_target_evolution_certified &&
         left
                 .vertical_map_complete_for_bounded_hgp_reduced_k2_to_k1_source ==
             right
                 .vertical_map_complete_for_bounded_hgp_reduced_k2_to_k1_source &&
         left.all_order_vertical_maps_complete ==
             right.all_order_vertical_maps_complete &&
         left.horizontal_source_persisted_in_result ==
             right.horizontal_source_persisted_in_result &&
         left.bounded_exhaustive_gamma_source_materialized ==
             right.bounded_exhaustive_gamma_source_materialized &&
         left.product_architecture_claimed ==
             right.product_architecture_claimed &&
         left.scalable_50k_claimed == right.scalable_50k_claimed &&
         left.global_m1_claimed == right.global_m1_claimed &&
         left.public_status_claimed == right.public_status_claimed &&
         left.output_digest_certified == right.output_digest_certified &&
         left.exact_k2_k1_hierarchy_certified ==
             right.exact_k2_k1_hierarchy_certified &&
         left.failure_payload_empty == right.failure_payload_empty &&
         left.atomic_failure == right.atomic_failure &&
         left.decision == right.decision && left.scope == right.scope;
}

}  // namespace

bool ExactK2K1HierarchyResult::well_formed_success_receipt() const noexcept {
  const bool terminal =
      decision ==
      ExactK2K1HierarchyDecision::complete_empty_terminal_order;
  const bool complete =
      decision ==
      ExactK2K1HierarchyDecision::complete_bounded_exact_k2_k1_hierarchy;
  bool self_digest_matches = false;
  try {
    self_digest_matches =
        compute_hierarchy_projection_digest(*this) == hierarchy_projection_digest;
  } catch (...) {
    return false;
  }
  bool checkpoint_records_close =
      vertical_checkpoints.size() == required_checkpoint_capacity;
  for (std::size_t index = 0U;
       checkpoint_records_close && index < vertical_checkpoints.size();
       ++index) {
    const ExactK2K1VerticalCheckpoint& checkpoint =
        vertical_checkpoints[index];
    checkpoint_records_close =
        checkpoint.checkpoint_index == index &&
        checkpoint.strict_prior_target_node_ids.size() ==
            checkpoint.prior_source_root_count &&
        checkpoint.source_group_resolved_from_frozen_snapshot &&
        checkpoint.every_prior_root_has_pre_batch_checkpoint &&
        checkpoint.prior_targets_normalize_to_strict_open_targets &&
        checkpoint.strict_open_targets_normalize_to_closed_target &&
        checkpoint.all_source_facets_map_to_closed_target &&
        checkpoint.source_coverage_contained_in_closed_target;
  }
  const bool counters_close =
      counters.source_batch_count == required_source_batch_capacity &&
      counters.source_group_count == required_source_group_capacity &&
      counters.source_node_count == required_source_node_capacity &&
      counters.source_child_reference_count ==
          required_source_child_reference_capacity &&
      counters.source_root_reference_count ==
          required_source_root_reference_capacity &&
      counters.checkpoint_count == required_checkpoint_capacity &&
      counters.source_facet_replay_count <=
          preflight_source_facet_replay_upper_bound &&
      counters.vertical_endpoint_lookup_count <=
          preflight_vertical_endpoint_lookup_upper_bound &&
      counters.k1_parent_hop_count <= preflight_k1_parent_hop_upper_bound &&
      counters.target_coverage_point_reference_count <=
          preflight_target_coverage_point_reference_upper_bound &&
      counters.digest_logical_entry_count ==
          required_digest_logical_entry_capacity &&
      counters.digest_exact_text_byte_count ==
          required_digest_exact_text_byte_capacity &&
      counters.deferred_group_count <= counters.source_group_count &&
      counters.checkpoint_count <= counters.source_group_count &&
      counters.deferred_group_count ==
          counters.source_group_count - counters.checkpoint_count &&
      counters.birth_checkpoint_count <= counters.checkpoint_count &&
      counters.continuation_checkpoint_count <=
          counters.checkpoint_count - counters.birth_checkpoint_count &&
      counters.multifusion_checkpoint_count ==
          counters.checkpoint_count - counters.birth_checkpoint_count -
              counters.continuation_checkpoint_count &&
      counters.birth_checkpoint_count <= counters.source_node_count &&
      counters.multifusion_checkpoint_count ==
          counters.source_node_count - counters.birth_checkpoint_count &&
      counters.consumed_source_root_count ==
          counters.source_root_reference_count &&
      counters.produced_source_root_count == counters.checkpoint_count &&
      counters.final_source_root_count == (complete ? 1U : 0U);
  const bool provenance_counters_close =
      provenance.source_history_counters.history_group_record_count ==
          counters.source_group_count &&
      provenance.source_history_counters.created_node_count ==
          counters.source_node_count &&
      provenance.source_history_counters.child_reference_count ==
          counters.source_child_reference_count &&
      provenance.source_history_counters.group_root_reference_count ==
          counters.source_root_reference_count &&
      provenance.source_history_counters.deferred_group_count ==
          counters.deferred_group_count &&
      provenance.source_history_counters.birth_group_count ==
          counters.birth_checkpoint_count &&
      provenance.source_history_counters.continuation_group_count ==
          counters.continuation_checkpoint_count &&
      provenance.source_history_counters.multifusion_group_count ==
          counters.multifusion_checkpoint_count &&
      provenance.source_history_counters.fully_redundant_group_count ==
          counters.fully_redundant_checkpoint_count &&
      provenance.k1_forest_counters.point_count == point_count;
  return schema_version == exact_k2_k1_hierarchy_schema_version &&
         point_count >= 2U &&
         point_count <=
             ExactK2K1HierarchyBudget::maximum_supported_point_count &&
         source_order == 2U && target_order == 1U &&
         budget_within_supported_bounds(requested_budget) &&
         structural_requirements_within_supported_bounds(*this) &&
         dynamic_requirements_within_supported_bounds(*this) &&
         structural_budget_sufficient(*this, requested_budget) &&
         dynamic_budget_sufficient(*this, requested_budget) &&
         counters_close && provenance_counters_close &&
         (terminal || complete) && terminal == (point_count == 2U) &&
         scope == ExactK2K1HierarchyScope::
                      bounded_n14_exact_exhaustive_gamma2_to_complete_graph_emst_k1_only &&
         source_and_target_preflight_sizes_derived_from_fresh_replay &&
         projection_budget_preflight_certified &&
         all_source_batches_staged_before_root_mutation &&
         every_non_deferred_group_has_one_checkpoint &&
         every_checkpoint_uses_strict_open_and_closed_exact_k1_targets &&
         source_facets_and_point_coverage_replayed_exactly &&
         strict_open_closed_vertical_naturality_certified &&
         k1_only_interlevel_target_evolution_certified &&
         vertical_map_complete_for_bounded_hgp_reduced_k2_to_k1_source &&
         !all_order_vertical_maps_complete &&
         !horizontal_source_persisted_in_result &&
         bounded_exhaustive_gamma_source_materialized == complete &&
         !product_architecture_claimed && !scalable_50k_claimed &&
         !global_m1_claimed && !public_status_claimed &&
         output_digest_certified && exact_k2_k1_hierarchy_certified &&
         !failure_payload_empty && !atomic_failure &&
         provenance.source_history_fresh_replay_certified &&
         provenance.source_horizontal_projection_complete &&
         provenance.k1_complete_graph_emst_fresh_replay_certified &&
         provenance.k1_compact_forest_fresh_replay_certified &&
         provenance.conditional_direct_morse_authority_not_used &&
         !provenance.source_horizontal_proof_basis.empty() &&
         !provenance.k1_target_proof_basis.empty() && self_digest_matches &&
         checkpoint_records_close &&
         counters.checkpoint_count == vertical_checkpoints.size();
}

bool ExactK2K1HierarchyResult::well_formed_fail_closed_receipt() const noexcept {
  const bool failure_decision =
      decision ==
          ExactK2K1HierarchyDecision::no_source_history_rejected ||
      decision == ExactK2K1HierarchyDecision::no_k1_forest_rejected ||
      decision == ExactK2K1HierarchyDecision::
                      no_projection_preflight_budget_insufficient;
  const bool rejection_has_no_requirements =
      (decision !=
           ExactK2K1HierarchyDecision::no_source_history_rejected &&
       decision != ExactK2K1HierarchyDecision::no_k1_forest_rejected) ||
      all_requirements_zero(*this);
  const bool budget_failure_is_insufficient =
      decision != ExactK2K1HierarchyDecision::
                      no_projection_preflight_budget_insufficient ||
      !structural_budget_sufficient(*this, requested_budget) ||
      !dynamic_budget_sufficient(*this, requested_budget);
  return schema_version == exact_k2_k1_hierarchy_schema_version &&
         point_count >= 2U &&
         point_count <=
             ExactK2K1HierarchyBudget::maximum_supported_point_count &&
         source_order == 2U && target_order == 1U && failure_decision &&
         budget_within_supported_bounds(requested_budget) &&
         structural_requirements_within_supported_bounds(*this) &&
         dynamic_requirements_within_supported_bounds(*this) &&
         rejection_has_no_requirements && budget_failure_is_insufficient &&
         scope == ExactK2K1HierarchyScope::
                      bounded_n14_exact_exhaustive_gamma2_to_complete_graph_emst_k1_only &&
         failure_payload_empty && atomic_failure &&
         provenance == ExactK2K1HierarchyProvenance{} &&
         vertical_checkpoints.empty() &&
         zero_identifier(hierarchy_projection_digest) &&
         counters == ExactK2K1HierarchyCounters{} &&
         !source_and_target_preflight_sizes_derived_from_fresh_replay &&
         !projection_budget_preflight_certified &&
         !all_source_batches_staged_before_root_mutation &&
         !every_non_deferred_group_has_one_checkpoint &&
         !every_checkpoint_uses_strict_open_and_closed_exact_k1_targets &&
         !source_facets_and_point_coverage_replayed_exactly &&
         !strict_open_closed_vertical_naturality_certified &&
         !k1_only_interlevel_target_evolution_certified &&
         !vertical_map_complete_for_bounded_hgp_reduced_k2_to_k1_source &&
         !all_order_vertical_maps_complete &&
         !horizontal_source_persisted_in_result &&
         !bounded_exhaustive_gamma_source_materialized &&
         !product_architecture_claimed && !scalable_50k_claimed &&
         !global_m1_claimed && !public_status_claimed &&
         !output_digest_certified && !exact_k2_k1_hierarchy_certified;
}

ExactK2K1HierarchyResult build_exact_k2_k1_hierarchy(
    const spatial::CanonicalPointCloud& cloud,
    ExactPersistentReducedGammaOrderHistoryBudget trusted_source_budget,
    const ExactPersistentReducedGammaOrderHistory& source_history,
    const K1CompactForest& target_k1_forest,
    const ExactK2K1HierarchyBudget& budget) {
  ExactK2K1HierarchyResult result = compute_exact_k2_k1_hierarchy(
      cloud,
      trusted_source_budget,
      source_history,
      target_k1_forest,
      budget);
  const ExactK2K1HierarchyVerification verification =
      verify_exact_k2_k1_hierarchy(
          cloud,
          trusted_source_budget,
          source_history,
          target_k1_forest,
          budget,
          result);
  if (!verification.exact_k2_k1_hierarchy_decision_certified) {
    throw std::logic_error(
        "the exact Gamma2-to-K1 builder failed its fresh verifier");
  }
  return result;
}

ExactK2K1HierarchyVerification verify_exact_k2_k1_hierarchy(
    const spatial::CanonicalPointCloud& cloud,
    ExactPersistentReducedGammaOrderHistoryBudget trusted_source_budget,
    const ExactPersistentReducedGammaOrderHistory& source_history,
    const K1CompactForest& target_k1_forest,
    const ExactK2K1HierarchyBudget& trusted_budget,
    const ExactK2K1HierarchyResult& observed) {
  ExactK2K1HierarchyVerification verification;
  try {
    const ExactK2K1HierarchyResult expected = compute_exact_k2_k1_hierarchy(
        cloud,
        std::move(trusted_source_budget),
        source_history,
        target_k1_forest,
        trusted_budget);
    const bool source_rejected =
        expected.decision ==
        ExactK2K1HierarchyDecision::no_source_history_rejected;
    const bool k1_rejected =
        expected.decision ==
        ExactK2K1HierarchyDecision::no_k1_forest_rejected;
    verification.trusted_inputs_accepted = !source_rejected && !k1_rejected;
    verification.source_history_freshly_replayed = !source_rejected;
    verification.k1_emst_and_compact_forest_freshly_replayed =
        !source_rejected && !k1_rejected;
    verification.parameters_certified =
        observed.schema_version == expected.schema_version &&
        observed.requested_budget == expected.requested_budget &&
        observed.point_count == expected.point_count &&
        observed.source_order == expected.source_order &&
        observed.target_order == expected.target_order &&
        observed.scope == expected.scope;
    verification.required_capacities_certified =
        same_required_capacities(observed, expected);
    verification.provenance_certified =
        observed.provenance == expected.provenance;
    verification.checkpoints_certified =
        observed.vertical_checkpoints == expected.vertical_checkpoints;
    verification.counters_certified =
        observed.counters == expected.counters;
    verification.result_facts_certified =
        same_result_facts(observed, expected);

    const bool expected_success = expected.well_formed_success_receipt();
    const bool expected_failure = expected.well_formed_fail_closed_receipt();
    bool observed_digest_recomputed = false;
    if (expected_success && verification.parameters_certified &&
        verification.required_capacities_certified &&
        verification.provenance_certified &&
        verification.checkpoints_certified &&
        verification.counters_certified &&
        verification.result_facts_certified) {
      observed_digest_recomputed =
          observed.hierarchy_projection_digest ==
          compute_hierarchy_projection_digest(observed);
    }
    verification.success_digest_recomputed =
        expected_success && observed_digest_recomputed &&
        observed.hierarchy_projection_digest ==
            expected.hierarchy_projection_digest;
    verification.failure_digest_absence_certified =
        expected_failure &&
        zero_identifier(observed.hierarchy_projection_digest) &&
        zero_identifier(expected.hierarchy_projection_digest);
    verification.failure_has_no_partial_scientific_payload =
        expected_failure && observed.well_formed_fail_closed_receipt();
    const bool decision_payload_certified =
        (expected_success && verification.success_digest_recomputed) ||
        (expected_failure &&
         verification.failure_digest_absence_certified &&
         verification.failure_has_no_partial_scientific_payload);
    verification.fresh_replay_certified =
        verification.parameters_certified &&
        verification.required_capacities_certified &&
        verification.provenance_certified &&
        verification.checkpoints_certified &&
        verification.counters_certified &&
        verification.result_facts_certified && decision_payload_certified;
    verification.exact_k2_k1_hierarchy_decision_certified =
        verification.fresh_replay_certified &&
        ((expected_success && observed.well_formed_success_receipt()) ||
         (expected_failure && observed.well_formed_fail_closed_receipt()));
  } catch (const std::exception&) {
    return {};
  }
  return verification;
}

}  // namespace morsehgp3d::hierarchy
