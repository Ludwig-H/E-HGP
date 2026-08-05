#include "morsehgp3d/hierarchy/direct_morse_resident_event_crosswalk_journal.hpp"

#include <array>
#include <limits>
#include <new>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

enum class BuildFailure : std::uint8_t {
  capacity_overflow,
  allocation_failed,
  budget_exhausted,
  product_n_at_least_k_plus_two_rejected,
  source_forest_rejected,
  source_link_replay_rejected,
  live_sealed_bridge_rejected,
  normalized_incidence_complete_v7_capability,
  source_namespace_mismatch,
  resident_plan_structure_inconsistent,
  bridge_membership_inconsistent,
  missing_event_target,
  duplicate_event_target,
  exact_level_or_order_mismatch,
  digest_encoding_failed,
};

enum class CandidateKind : std::uint8_t {
  none,
  k2_birth,
  k2_saddle,
  higher_saddle,
};

struct Candidate {
  CandidateKind kind{CandidateKind::none};
  std::size_t batch_record_index{};
  std::size_t binding_index{};
};

struct UpperBirthCandidate {
  std::size_t plan_batch_index{};
  std::size_t direct_reference_index{};
  std::size_t source_role_record_index{};
  std::size_t facet_token_index{};
};

struct ForestBirthCandidate {
  std::size_t materialized_birth_record_index{};
  std::size_t forest_batch_index{};
};

struct ProjectionJoinSlot {
  Candidate target{};
  std::optional<std::size_t> link_index;
  std::optional<UpperBirthCandidate> upper_birth;
  std::optional<ForestBirthCandidate> forest_birth;
  bool lower_saddle_seen{false};
};

[[nodiscard]] bool try_add(
    std::size_t left,
    std::size_t right,
    std::size_t& sum) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  sum = left + right;
  return true;
}

void append_u64(
    contract::CanonicalSha256Builder& builder,
    std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    const std::size_t shift = (bytes.size() - 1U - index) * 8U;
    bytes[index] = static_cast<std::uint8_t>(value >> shift);
  }
  builder.update(bytes);
}

void append_size(
    contract::CanonicalSha256Builder& builder,
    std::size_t value) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  append_u64(builder, static_cast<std::uint64_t>(value));
}

void append_bool(
    contract::CanonicalSha256Builder& builder,
    bool value) {
  append_u64(builder, value ? UINT64_C(1) : UINT64_C(0));
}

void append_text(
    contract::CanonicalSha256Builder& builder,
    std::string_view value) {
  append_size(builder, value.size());
  builder.update(value);
}

void append_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

void append_level(
    contract::CanonicalSha256Builder& builder,
    const exact::ExactLevel& value) {
  append_text(builder, value.numerator_string());
  append_text(builder, value.denominator_string());
}

void append_locator_stamp(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& stamp) {
  append_u64(builder, stamp.schema_version);
  append_u64(builder, stamp.external_authority_id);
  append_size(builder, stamp.committed_batch_count);
  append_size(builder, stamp.inserted_key_count);
  append_size(builder, stamp.component_union_count);
  append_size(builder, stamp.binding_count);
  append_id(builder, stamp.committed_history_digest);
}

void append_facet_key(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseFacetKey& key) {
  append_size(builder, key.point_count);
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    append_u64(builder, static_cast<std::uint64_t>(key.point_ids[index]));
  }
}

void append_facet_witness(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseFacetWitness& witness) {
  append_u64(builder, witness.external_authority_id);
  append_u64(builder, witness.replay_token);
}

[[nodiscard]] contract::CanonicalId source_link_identity_digest(
    const ExactDirectMorseEventRankTowerLinkJournalResult& source) {
  contract::CanonicalSha256Builder builder;
  append_text(
      builder,
      "MorseHGP3D/phase15/direct-morse-resident-event-crosswalk/source-link/v1");
  append_u64(builder, source.schema_version);
  append_size(builder, source.point_count);
  append_size(builder, source.effective_maximum_order);
  append_size(builder, source.source_event_projection_count);
  append_id(builder, source.source_higher_canonical_cloud_digest);
  append_locator_stamp(builder, source.source_forest_final_locator_stamp);
  append_size(builder, source.links.size());
  for (const auto& link : source.links) {
    append_size(builder, link.link_index);
    append_size(builder, link.source_birth_record_index);
    append_size(builder, link.source_event_projection_index);
    append_size(builder, link.source_order);
    append_size(builder, link.target_order);
    append_level(builder, link.squared_level);
    append_size(builder, link.saddle_record_index);
    append_size(builder, link.source_saddle_family_index);
    append_size(builder, link.lower_batch_index);
    append_size(builder, link.atomic_group_index);
    append_u64(builder, link.target_resulting_root_node_id);
    append_size(builder, link.arm_terminal_offset);
    append_size(builder, link.arm_terminal_count);
    append_locator_stamp(builder, link.lower_strict_pre_batch_stamp);
    append_locator_stamp(builder, link.lower_committed_post_batch_stamp);
    append_id(builder, link.source_event_arm_identity_digest);
  }
  append_size(builder, source.arm_terminals.size());
  for (const auto& terminal : source.arm_terminals) {
    append_size(builder, terminal.arm_terminal_reference_index);
    append_size(builder, terminal.source_arm_root_binding_index);
    append_size(builder, terminal.terminal_birth_record_index);
    append_size(builder, terminal.frozen_carrier_component_handle);
    append_u64(
        builder,
        terminal.prior_reduced_root_node_id.has_value() ? UINT64_C(1)
                                                        : UINT64_C(0));
    if (terminal.prior_reduced_root_node_id.has_value()) {
      append_u64(builder, *terminal.prior_reduced_root_node_id);
    }
    append_u64(
        builder,
        static_cast<std::uint64_t>(terminal.removed_support_point_id));
    append_facet_key(builder, terminal.terminal_birth_facet_key);
    append_facet_witness(builder, terminal.terminal_birth_binding_witness);
    const auto center = terminal.terminal_birth_exact_center.to_record();
    append_text(builder, center.x_numerator);
    append_text(builder, center.y_numerator);
    append_text(builder, center.z_numerator);
    append_text(builder, center.denominator);
    append_level(builder, terminal.terminal_birth_exact_squared_level);
  }
  return builder.finalize();
}

void append_bridge_stamp(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseResidentAllOrdersVerticalBridgeStamp& stamp) {
  append_u64(builder, stamp.schema_version);
  append_u64(builder, stamp.bridge_session_instance_id);
  append_u64(builder, stamp.owned_k2_k1_bridge_session_instance_id);
  append_u64(builder, stamp.resident_session_authority_id);
  append_u64(builder, stamp.resident_locator_instance_id);
  append_size(builder, stamp.resident_batch_cursor);
  append_size(builder, stamp.resident_epoch);
  append_size(builder, stamp.committed_k2_batch_count);
  append_size(builder, stamp.committed_k2_group_count);
  append_size(
      builder, stamp.committed_k2_direct_saddle_group_binding_count);
  append_size(builder, stamp.committed_k2_direct_birth_k1_binding_count);
  append_size(builder, stamp.committed_higher_batch_count);
  append_size(builder, stamp.committed_higher_group_count);
  append_size(builder, stamp.committed_terminal_component_image_count);
  append_size(
      builder, stamp.committed_higher_direct_saddle_group_binding_count);
  append_size(builder, stamp.persistent_source_root_witness_count);
  append_u64(builder, stamp.next_query_replay_token);
  append_id(builder, stamp.canonical_cloud_digest);
  append_id(builder, stamp.resident_higher_canonical_cloud_digest);
}

[[nodiscard]] contract::CanonicalId bridge_membership_digest(
    const ExactDirectMorseResidentAllOrdersVerticalBridgeStamp& stamp,
    std::span<const ExactDirectMorseResidentK2K1ClosedCutBatchRecord>
        k2_batches,
    std::span<const ExactDirectMorseResidentAllOrdersVerticalBatchRecord>
        higher_batches) {
  contract::CanonicalSha256Builder builder;
  append_text(
      builder,
      "MorseHGP3D/phase15/direct-morse-resident-event-crosswalk/live-bridge-membership/v1");
  append_bridge_stamp(builder, stamp);
  append_size(builder, k2_batches.size());
  for (const auto& batch : k2_batches) {
    append_size(builder, batch.source_batch_index);
    append_size(builder, batch.order);
    append_level(builder, batch.squared_level);
    append_id(builder, batch.o4_membership_digest);
  }
  append_size(builder, higher_batches.size());
  for (const auto& batch : higher_batches) {
    append_size(builder, batch.source_batch_index);
    append_size(builder, batch.source_order);
    append_size(builder, batch.target_order);
    append_level(builder, batch.squared_level);
    append_id(builder, batch.o4_membership_digest);
  }
  return builder.finalize();
}

void append_budget(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseResidentEventCrosswalkBudget& budget) {
  append_size(builder, budget.maximum_link_scan_count);
  append_size(builder, budget.maximum_resident_plan_batch_scan_count);
  append_size(builder, budget.maximum_k2_batch_scan_count);
  append_size(builder, budget.maximum_higher_batch_scan_count);
  append_size(builder, budget.maximum_k2_direct_birth_binding_scan_count);
  append_size(builder, budget.maximum_direct_saddle_group_binding_scan_count);
  append_size(builder, budget.maximum_forest_birth_record_scan_count);
  append_size(
      builder, budget.maximum_resident_plan_direct_reference_scan_count);
  append_size(
      builder, budget.maximum_resident_direct_reference_replay_count);
  append_size(builder, budget.maximum_event_projection_scratch_entry_count);
  append_size(builder, budget.maximum_record_count);
  append_size(builder, budget.maximum_logical_output_entry_count);
}

void append_counters(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseResidentEventCrosswalkCounters& counters) {
  append_size(builder, counters.link_scan_count);
  append_size(builder, counters.resident_plan_batch_scan_count);
  append_size(builder, counters.k2_batch_scan_count);
  append_size(builder, counters.higher_batch_scan_count);
  append_size(builder, counters.k2_direct_birth_binding_scan_count);
  append_size(builder, counters.direct_saddle_group_binding_scan_count);
  append_size(builder, counters.forest_birth_record_scan_count);
  append_size(builder, counters.resident_plan_direct_reference_scan_count);
  append_size(builder, counters.resident_direct_reference_replay_count);
  append_size(builder, counters.event_projection_scratch_entry_count);
  append_size(builder, counters.rank_two_k1_target_count);
  append_size(builder, counters.higher_rank_o4_target_count);
  append_size(builder, counters.record_count);
}

[[nodiscard]] contract::CanonicalId canonical_crosswalk_result_digest(
    const ExactDirectMorseResidentEventCrosswalkJournalResult& result)
    noexcept {
  try {
    contract::CanonicalSha256Builder builder;
    append_text(
        builder,
        "MorseHGP3D/phase15/direct-morse-resident-event-crosswalk/result/v1");
    append_u64(builder, result.schema_version);
    append_budget(builder, result.requested_budget);
    append_size(builder, result.point_count);
    append_size(builder, result.effective_maximum_order);
    append_size(builder, result.source_event_projection_count);
    append_size(builder, result.source_link_count);
    append_id(builder, result.source_higher_canonical_cloud_digest);
    append_id(builder, result.source_link_identity_digest);
    append_id(builder, result.source_bridge_membership_digest);
    append_locator_stamp(builder, result.source_forest_final_locator_stamp);
    append_bridge_stamp(builder, result.source_bridge_stamp);
    append_size(builder, result.records.size());
    for (const auto& record : result.records) {
      append_size(builder, record.record_index);
      append_size(builder, record.source_link_index);
      append_size(builder, record.source_birth_record_index);
      append_size(builder, record.source_event_projection_index);
      append_size(builder, record.source_order);
      append_size(builder, record.target_order);
      append_level(builder, record.squared_level);
      append_size(builder, record.source_upper_birth_direct_reference_index);
      append_size(builder, record.source_upper_birth_role_record_index);
      append_size(builder, record.source_upper_birth_facet_token_index);
      append_id(builder, record.source_event_arm_identity_digest);
      append_id(builder, record.source_bridge_batch_membership_digest);
      if (record.k1_birth_target.has_value()) {
        append_text(builder, "k1-closed-cut-node");
        const auto& target = *record.k1_birth_target;
        append_size(builder, target.k2_batch_record_index);
        append_size(builder, target.source_batch_index);
        append_size(builder, target.binding_index);
        append_size(builder, target.source_direct_reference_index);
        append_size(builder, target.source_role_record_index);
        append_size(builder, target.source_facet_token_index);
        append_u64(builder, target.closed_k1_node.value);
      } else if (record.o4_saddle_target.has_value()) {
        append_text(builder, "adjacent-resident-root");
        const auto& target = *record.o4_saddle_target;
        append_u64(builder, target.target_batch_is_k2 ? UINT64_C(1)
                                                     : UINT64_C(0));
        append_size(builder, target.bridge_batch_record_index);
        append_size(builder, target.source_batch_index);
        append_size(builder, target.binding_index);
        append_size(builder, target.source_direct_reference_index);
        append_size(builder, target.source_role_record_index);
        append_size(builder, target.source_incidence_family_index);
        append_size(builder, target.source_hyperedge_index);
        append_size(builder, target.owner_group_index);
        append_size(builder, target.group_image_index);
        append_u64(builder, target.resident_resultant_root.value);
      } else {
        append_text(builder, "missing-target");
      }
    }
    append_size(builder, result.logical_output_entry_count);
    append_counters(builder, result.counters);
    append_bool(builder, result.budget_preflight_certified);
    append_bool(builder, result.product_n_at_least_k_plus_two_replayed);
    append_bool(builder, result.source_conditional_forest_certificate_replayed);
    append_bool(
        builder, result.source_link_journal_freshly_replayed_relative_to_forest);
    append_bool(builder, result.live_bridge_final_seal_verified);
    append_bool(
        builder,
        result.normalized_incidence_complete_v7_live_capability_replayed);
    append_bool(builder, result.canonical_cloud_and_point_namespace_joined);
    append_bool(
        builder,
        result
            .resident_projection_indices_joined_conditionally_by_deterministic_canonical_source_order);
    append_bool(builder, result.common_source_event_semantic_digest_replayed);
    append_bool(
        builder,
        result.every_resident_direct_birth_replayed_against_exact_forest_birth);
    append_bool(builder, result.every_resident_direct_birth_has_one_source_link);
    append_bool(
        builder,
        result.every_resident_direct_saddle_below_k_has_one_source_link);
    append_bool(builder, result.every_source_link_bound_exactly_once);
    append_bool(
        builder, result.every_rank_two_link_bound_to_direct_birth_k1_target);
    append_bool(
        builder,
        result.every_higher_rank_link_bound_to_lower_direct_saddle_o4_group);
    append_bool(builder, result.exact_level_and_adjacent_order_replayed);
    append_bool(builder, result.bridge_membership_digests_freshly_replayed);
    append_bool(builder, result.target_identifier_namespaces_typed_and_disjoint);
    append_bool(builder, result.cross_domain_numeric_root_equality_used);
    append_bool(
        builder, result.o4_membership_complete_for_supplied_resident_batches);
    append_bool(builder, result.full_pi0_replayed);
    append_bool(builder, result.o7_bidirectional_completeness_replayed);
    append_bool(builder, result.global_morse_obligation_replayed);
    append_bool(builder, result.m1_replayed);
    append_bool(builder, result.gamma_cells_or_global_cofaces_materialized);
    append_bool(builder, result.ordinary_or_higher_order_delaunay_materialized);
    append_bool(builder, result.pair_matrix_materialized);
    append_bool(builder, result.public_status_claimed);
    append_bool(builder, result.no_partial_scientific_payload_published);
    append_u64(builder, static_cast<std::uint64_t>(result.decision));
    append_u64(builder, static_cast<std::uint64_t>(result.scope));
    return builder.finalize();
  } catch (...) {
    return {};
  }
}

[[nodiscard]] ExactDirectMorseResidentEventCrosswalkDecision decision_for(
    BuildFailure failure) noexcept {
  switch (failure) {
    case BuildFailure::capacity_overflow:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_capacity_overflow;
    case BuildFailure::allocation_failed:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_allocation_failed;
    case BuildFailure::budget_exhausted:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_budget_exhausted;
    case BuildFailure::product_n_at_least_k_plus_two_rejected:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_product_n_at_least_k_plus_two_rejected;
    case BuildFailure::source_forest_rejected:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_source_forest_rejected;
    case BuildFailure::source_link_replay_rejected:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_source_link_replay_rejected;
    case BuildFailure::live_sealed_bridge_rejected:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_live_sealed_bridge_rejected;
    case BuildFailure::normalized_incidence_complete_v7_capability:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_normalized_incidence_complete_v7_capability;
    case BuildFailure::source_namespace_mismatch:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_source_namespace_mismatch;
    case BuildFailure::resident_plan_structure_inconsistent:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_resident_plan_structure_inconsistent;
    case BuildFailure::bridge_membership_inconsistent:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_bridge_membership_inconsistent;
    case BuildFailure::missing_event_target:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_missing_event_target;
    case BuildFailure::duplicate_event_target:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_duplicate_event_target;
    case BuildFailure::exact_level_or_order_mismatch:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_exact_level_or_order_mismatch;
    case BuildFailure::digest_encoding_failed:
      return ExactDirectMorseResidentEventCrosswalkDecision::
          no_crosswalk_digest_encoding_failed;
  }
  return ExactDirectMorseResidentEventCrosswalkDecision::not_certified;
}

void initialize_scope(
    ExactDirectMorseResidentEventCrosswalkJournalResult& result) noexcept {
  result.scope = ExactDirectMorseResidentEventCrosswalkScope::
      all_forest_rank_at_least_two_adjacent_event_links_to_live_sealed_resident_targets_under_n_at_least_k_plus_two;
  result.cross_domain_numeric_root_equality_used = false;
  result.full_pi0_replayed = false;
  result.o7_bidirectional_completeness_replayed = false;
  result.global_morse_obligation_replayed = false;
  result.m1_replayed = false;
  result.gamma_cells_or_global_cofaces_materialized = false;
  result.ordinary_or_higher_order_delaunay_materialized = false;
  result.pair_matrix_materialized = false;
  result.public_status_claimed = false;
}

[[nodiscard]] ExactDirectMorseResidentEventCrosswalkJournalResult fail(
    ExactDirectMorseResidentEventCrosswalkJournalResult&& result,
    BuildFailure failure) noexcept {
  result.point_count = 0U;
  result.effective_maximum_order = 0U;
  result.source_event_projection_count = 0U;
  result.source_link_count = 0U;
  result.source_higher_canonical_cloud_digest = {};
  result.source_link_identity_digest = {};
  result.source_bridge_membership_digest = {};
  result.crosswalk_digest = {};
  result.source_forest_final_locator_stamp = {};
  result.source_bridge_stamp = {};
  result.records.clear();
  result.logical_output_entry_count = 0U;
  result.counters = {};
  result.budget_preflight_certified = false;
  result.product_n_at_least_k_plus_two_replayed = false;
  result.source_conditional_forest_certificate_replayed = false;
  result.source_link_journal_freshly_replayed_relative_to_forest = false;
  result.live_bridge_final_seal_verified = false;
  result.normalized_incidence_complete_v7_live_capability_replayed = false;
  result.canonical_cloud_and_point_namespace_joined = false;
  result
      .resident_projection_indices_joined_conditionally_by_deterministic_canonical_source_order =
      false;
  result.common_source_event_semantic_digest_replayed = false;
  result.every_resident_direct_birth_replayed_against_exact_forest_birth =
      false;
  result.every_resident_direct_birth_has_one_source_link = false;
  result.every_resident_direct_saddle_below_k_has_one_source_link = false;
  result.every_source_link_bound_exactly_once = false;
  result.every_rank_two_link_bound_to_direct_birth_k1_target = false;
  result.every_higher_rank_link_bound_to_lower_direct_saddle_o4_group = false;
  result.exact_level_and_adjacent_order_replayed = false;
  result.bridge_membership_digests_freshly_replayed = false;
  result.target_identifier_namespaces_typed_and_disjoint = false;
  result.o4_membership_complete_for_supplied_resident_batches = false;
  result.no_partial_scientific_payload_published = true;
  result.decision = decision_for(failure);
  initialize_scope(result);
  result.crosswalk_digest = canonical_crosswalk_result_digest(result);
  return std::move(result);
}

[[nodiscard]] bool batch_direct_slice_contains(
    const ExactDirectSparseUnifiedLevelPlanBatch& batch,
    std::size_t direct_reference_index) noexcept {
  return batch.direct_reference_offset <= direct_reference_index &&
         direct_reference_index - batch.direct_reference_offset <
             batch.direct_reference_count;
}

[[nodiscard]] bool plan_batch_shape(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    std::size_t batch_index) noexcept {
  if (batch_index >= plan.batches.size()) {
    return false;
  }
  const auto& batch = plan.batches[batch_index];
  return batch.batch_index == batch_index &&
         batch.direct_reference_offset <= plan.direct_references.size() &&
         batch.direct_reference_count <=
             plan.direct_references.size() - batch.direct_reference_offset &&
         batch.residual_reference_offset <= plan.residual_references.size() &&
         batch.residual_reference_count <=
             plan.residual_references.size() -
                 batch.residual_reference_offset &&
         batch.coface_facet_reference_offset <=
             plan.coface_facet_references.size() &&
         batch.coface_facet_reference_count <=
             plan.coface_facet_references.size() -
                 batch.coface_facet_reference_offset;
}

[[nodiscard]] bool add_candidate(
    std::vector<ProjectionJoinSlot>& slots,
    std::size_t event_projection_index,
    Candidate candidate) noexcept {
  if (event_projection_index >= slots.size() ||
      slots[event_projection_index].target.kind != CandidateKind::none) {
    return false;
  }
  slots[event_projection_index].target = candidate;
  return true;
}

[[nodiscard]] bool result_storage_within_budget(
    const ExactDirectMorseResidentEventCrosswalkJournalResult& result,
    const ExactDirectMorseResidentEventCrosswalkBudget& budget) noexcept {
  return result.counters.link_scan_count <= budget.maximum_link_scan_count &&
         result.counters.resident_plan_batch_scan_count <=
             budget.maximum_resident_plan_batch_scan_count &&
         result.counters.k2_batch_scan_count <=
             budget.maximum_k2_batch_scan_count &&
         result.counters.higher_batch_scan_count <=
             budget.maximum_higher_batch_scan_count &&
         result.counters.k2_direct_birth_binding_scan_count <=
             budget.maximum_k2_direct_birth_binding_scan_count &&
         result.counters.direct_saddle_group_binding_scan_count <=
             budget.maximum_direct_saddle_group_binding_scan_count &&
         result.counters.forest_birth_record_scan_count <=
             budget.maximum_forest_birth_record_scan_count &&
         result.counters.resident_plan_direct_reference_scan_count <=
             budget.maximum_resident_plan_direct_reference_scan_count &&
         result.counters.resident_direct_reference_replay_count <=
             budget.maximum_resident_direct_reference_replay_count &&
         result.counters.event_projection_scratch_entry_count <=
             budget.maximum_event_projection_scratch_entry_count &&
         result.records.size() <= budget.maximum_record_count &&
         result.logical_output_entry_count == result.records.size() &&
         result.logical_output_entry_count <=
             budget.maximum_logical_output_entry_count;
}

[[nodiscard]] bool payload_shape(
    const ExactDirectMorseResidentEventCrosswalkJournalResult& result)
    noexcept {
  if (result.schema_version !=
          direct_morse_resident_event_crosswalk_journal_schema_version ||
      result.point_count < result.effective_maximum_order ||
      result.point_count - result.effective_maximum_order < 2U ||
      result.source_link_count != result.records.size() ||
      result.logical_output_entry_count != result.records.size() ||
      result.counters.link_scan_count != result.source_link_count ||
      result.counters.record_count != result.records.size() ||
      result.counters.event_projection_scratch_entry_count !=
          result.source_event_projection_count ||
      result.source_higher_canonical_cloud_digest ==
          contract::CanonicalId{} ||
      result.source_link_identity_digest == contract::CanonicalId{} ||
      result.source_bridge_membership_digest == contract::CanonicalId{} ||
      result.crosswalk_digest == contract::CanonicalId{} ||
      canonical_crosswalk_result_digest(result) != result.crosswalk_digest ||
      result.source_bridge_stamp.bridge_session_instance_id == 0U) {
    return false;
  }
  std::size_t rank_two_count = 0U;
  std::size_t higher_count = 0U;
  for (std::size_t index = 0U; index < result.records.size(); ++index) {
    const auto& record = result.records[index];
    if (record.record_index != index || record.source_link_index != index ||
        record.source_event_projection_index >=
            result.source_event_projection_count ||
        record.source_order < 2U ||
        record.target_order + 1U != record.source_order ||
        record.source_order > result.effective_maximum_order ||
        record.source_event_arm_identity_digest == contract::CanonicalId{} ||
        record.source_bridge_batch_membership_digest ==
            contract::CanonicalId{} ||
        record.k1_birth_target.has_value() ==
            record.o4_saddle_target.has_value()) {
      return false;
    }
    if (record.source_order == 2U) {
      if (!record.k1_birth_target.has_value() ||
          record.source_upper_birth_direct_reference_index !=
              record.k1_birth_target->source_direct_reference_index ||
          record.source_upper_birth_role_record_index !=
              record.k1_birth_target->source_role_record_index ||
          record.source_upper_birth_facet_token_index !=
              record.k1_birth_target->source_facet_token_index) {
        return false;
      }
      ++rank_two_count;
    } else {
      if (!record.o4_saddle_target.has_value()) {
        return false;
      }
      const auto& target = *record.o4_saddle_target;
      if (target.target_batch_is_k2 != (record.target_order == 2U)) {
        return false;
      }
      ++higher_count;
    }
  }
  return rank_two_count == result.counters.rank_two_k1_target_count &&
         higher_count == result.counters.higher_rank_o4_target_count;
}

}  // namespace

bool ExactDirectMorseResidentEventCrosswalkJournalResult::
    certified_conditional_resident_event_crosswalk() const noexcept {
  return payload_shape(*this) && result_storage_within_budget(*this, requested_budget) &&
         budget_preflight_certified &&
         product_n_at_least_k_plus_two_replayed &&
         source_conditional_forest_certificate_replayed &&
         source_link_journal_freshly_replayed_relative_to_forest &&
         live_bridge_final_seal_verified &&
         normalized_incidence_complete_v7_live_capability_replayed &&
         canonical_cloud_and_point_namespace_joined &&
         resident_projection_indices_joined_conditionally_by_deterministic_canonical_source_order &&
         !common_source_event_semantic_digest_replayed &&
         every_resident_direct_birth_replayed_against_exact_forest_birth &&
         every_resident_direct_birth_has_one_source_link &&
         every_resident_direct_saddle_below_k_has_one_source_link &&
         every_source_link_bound_exactly_once &&
         every_rank_two_link_bound_to_direct_birth_k1_target &&
         every_higher_rank_link_bound_to_lower_direct_saddle_o4_group &&
         exact_level_and_adjacent_order_replayed &&
         bridge_membership_digests_freshly_replayed &&
         target_identifier_namespaces_typed_and_disjoint &&
         !cross_domain_numeric_root_equality_used &&
         o4_membership_complete_for_supplied_resident_batches &&
         !full_pi0_replayed && !o7_bidirectional_completeness_replayed &&
         !global_morse_obligation_replayed && !m1_replayed &&
         !gamma_cells_or_global_cofaces_materialized &&
         !ordinary_or_higher_order_delaunay_materialized &&
         !pair_matrix_materialized && !public_status_claimed &&
         no_partial_scientific_payload_published &&
         decision == ExactDirectMorseResidentEventCrosswalkDecision::
                         complete_conditional_resident_event_crosswalk &&
         scope == ExactDirectMorseResidentEventCrosswalkScope::
                      all_forest_rank_at_least_two_adjacent_event_links_to_live_sealed_resident_targets_under_n_at_least_k_plus_two;
}

bool ExactDirectMorseResidentEventCrosswalkJournalResult::
    certified_atomic_failure() const noexcept {
  const bool failure_decision =
      decision != ExactDirectMorseResidentEventCrosswalkDecision::not_certified &&
      decision != ExactDirectMorseResidentEventCrosswalkDecision::
                      complete_conditional_resident_event_crosswalk;
  return schema_version ==
             direct_morse_resident_event_crosswalk_journal_schema_version &&
         failure_decision &&
         scope == ExactDirectMorseResidentEventCrosswalkScope::
                      all_forest_rank_at_least_two_adjacent_event_links_to_live_sealed_resident_targets_under_n_at_least_k_plus_two &&
         point_count == 0U &&
         effective_maximum_order == 0U &&
         source_event_projection_count == 0U && source_link_count == 0U &&
         source_higher_canonical_cloud_digest == contract::CanonicalId{} &&
         source_link_identity_digest == contract::CanonicalId{} &&
         source_bridge_membership_digest == contract::CanonicalId{} &&
         crosswalk_digest != contract::CanonicalId{} &&
         canonical_crosswalk_result_digest(*this) == crosswalk_digest &&
         records.empty() &&
         source_forest_final_locator_stamp ==
             ExactDirectSparsePositiveFacetLocatorSnapshotStamp{} &&
         source_bridge_stamp ==
             ExactDirectMorseResidentAllOrdersVerticalBridgeStamp{} &&
         logical_output_entry_count == 0U && counters ==
             ExactDirectMorseResidentEventCrosswalkCounters{} &&
         !budget_preflight_certified &&
         !product_n_at_least_k_plus_two_replayed &&
         !source_conditional_forest_certificate_replayed &&
         !source_link_journal_freshly_replayed_relative_to_forest &&
         !live_bridge_final_seal_verified &&
         !normalized_incidence_complete_v7_live_capability_replayed &&
         !canonical_cloud_and_point_namespace_joined &&
         !resident_projection_indices_joined_conditionally_by_deterministic_canonical_source_order &&
         !common_source_event_semantic_digest_replayed &&
         !every_resident_direct_birth_replayed_against_exact_forest_birth &&
         !every_resident_direct_birth_has_one_source_link &&
         !every_resident_direct_saddle_below_k_has_one_source_link &&
         !every_source_link_bound_exactly_once &&
         !every_rank_two_link_bound_to_direct_birth_k1_target &&
         !every_higher_rank_link_bound_to_lower_direct_saddle_o4_group &&
         !exact_level_and_adjacent_order_replayed &&
         !bridge_membership_digests_freshly_replayed &&
         !target_identifier_namespaces_typed_and_disjoint &&
         !o4_membership_complete_for_supplied_resident_batches &&
         no_partial_scientific_payload_published &&
         !cross_domain_numeric_root_equality_used && !full_pi0_replayed &&
         !o7_bidirectional_completeness_replayed &&
         !global_morse_obligation_replayed && !m1_replayed &&
         !gamma_cells_or_global_cofaces_materialized &&
         !ordinary_or_higher_order_delaunay_materialized &&
         !pair_matrix_materialized &&
         !public_status_claimed;
}

bool ExactDirectMorseResidentEventCrosswalkJournalResult::certified_outcome()
    const noexcept {
  return certified_conditional_resident_event_crosswalk() ||
         certified_atomic_failure();
}

ExactDirectMorseResidentEventCrosswalkJournalResult
build_exact_direct_morse_resident_event_crosswalk_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseEventRankTowerLinkBudget& trusted_link_budget,
    const ExactDirectMorseEventRankTowerLinkJournalResult& source_link,
    const ExactDirectMorseResidentAllOrdersVerticalBridge& live_bridge,
    const ExactDirectMorseResidentEventCrosswalkBudget& budget) {
  ExactDirectMorseResidentEventCrosswalkJournalResult result;
  result.requested_budget = budget;
  initialize_scope(result);

  try {
    if (!source_forest.certified_conditional_h0_candidate()) {
      return fail(std::move(result), BuildFailure::source_forest_rejected);
    }
    if (source_forest.point_count < source_forest.effective_maximum_order ||
        source_forest.point_count - source_forest.effective_maximum_order <
            2U) {
      return fail(
          std::move(result),
          BuildFailure::product_n_at_least_k_plus_two_rejected);
    }
    const auto link_replay =
        verify_exact_direct_morse_event_rank_tower_link_journal(
            source_forest, trusted_link_budget, source_link);
    if (!link_replay.result_certified) {
      return fail(std::move(result), BuildFailure::source_link_replay_rejected);
    }
    if (!live_bridge.ready() || !live_bridge.resident_complete() ||
        !live_bridge.conditional_all_adjacent_order_group_images_complete() ||
        !live_bridge.final_vertical_sealed() ||
        live_bridge.final_vertical_seal() == nullptr ||
        !live_bridge.verify_final_vertical_seal(
            *live_bridge.final_vertical_seal())) {
      return fail(
          std::move(result), BuildFailure::live_sealed_bridge_rejected);
    }
    if (!live_bridge.resident_normalized_direct_source_session() ||
        !live_bridge
             .resident_normalized_horizontal_incidence_reduction_certified() ||
        !live_bridge.normalized_incidence_complete_v7_source_capability() ||
        !live_bridge.final_vertical_seal()
             ->normalized_incidence_complete_v7_source_capability) {
      return fail(
          std::move(result),
          BuildFailure::normalized_incidence_complete_v7_capability);
    }

    const auto& plan = live_bridge.resident_plan();
    const auto& k2_batches = live_bridge.committed_k2_batches();
    const auto& higher_batches = live_bridge.committed_higher_batches();
    const auto bridge_stamp = live_bridge.current_stamp();
    if (!live_bridge.verify_stamp(bridge_stamp) ||
        bridge_stamp != live_bridge.final_vertical_seal()->sealed_stamp ||
        source_link.point_count != source_forest.point_count ||
        source_link.effective_maximum_order !=
            source_forest.effective_maximum_order ||
        source_link.source_event_projection_count !=
            source_forest.source_event_projection_count ||
        plan.point_count != source_forest.point_count ||
        plan.source_event_projection_count !=
            source_forest.source_event_projection_count ||
        plan.source_higher_canonical_cloud_digest !=
            source_forest.source_higher_canonical_cloud_digest ||
        plan.source_pair_canonical_cloud_digest !=
            bridge_stamp.canonical_cloud_digest ||
        bridge_stamp.resident_higher_canonical_cloud_digest !=
            source_forest.source_higher_canonical_cloud_digest ||
        bridge_stamp.resident_batch_cursor != plan.batches.size()) {
      return fail(std::move(result), BuildFailure::source_namespace_mismatch);
    }

    std::size_t birth_binding_count = 0U;
    std::size_t saddle_binding_count = 0U;
    for (const auto& batch : k2_batches) {
      if (!try_add(
              birth_binding_count,
              batch.direct_birth_k1_bindings.size(),
              birth_binding_count) ||
          !try_add(
              saddle_binding_count,
              batch.direct_saddle_group_bindings.size(),
              saddle_binding_count)) {
        return fail(std::move(result), BuildFailure::capacity_overflow);
      }
    }
    for (const auto& batch : higher_batches) {
      if (!try_add(
              saddle_binding_count,
              batch.direct_saddle_group_bindings.size(),
              saddle_binding_count)) {
        return fail(std::move(result), BuildFailure::capacity_overflow);
      }
    }
    std::size_t direct_replay_count = 0U;
    if (!try_add(
            birth_binding_count,
            saddle_binding_count,
            direct_replay_count)) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    const std::size_t record_count = source_link.links.size();
    const std::size_t forest_birth_scan_count =
        source_forest.birth_records.size();
    const std::size_t plan_direct_reference_scan_count =
        plan.direct_references.size();
    if (record_count > budget.maximum_link_scan_count ||
        plan.batches.size() >
            budget.maximum_resident_plan_batch_scan_count ||
        k2_batches.size() > budget.maximum_k2_batch_scan_count ||
        higher_batches.size() > budget.maximum_higher_batch_scan_count ||
        birth_binding_count >
            budget.maximum_k2_direct_birth_binding_scan_count ||
        saddle_binding_count >
            budget.maximum_direct_saddle_group_binding_scan_count ||
        forest_birth_scan_count >
            budget.maximum_forest_birth_record_scan_count ||
        plan_direct_reference_scan_count >
            budget.maximum_resident_plan_direct_reference_scan_count ||
        direct_replay_count >
            budget.maximum_resident_direct_reference_replay_count ||
        plan.source_event_projection_count >
            budget.maximum_event_projection_scratch_entry_count ||
        record_count > budget.maximum_record_count ||
        record_count > budget.maximum_logical_output_entry_count) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }

    for (std::size_t batch_index = 0U;
         batch_index < plan.batches.size();
         ++batch_index) {
      if (!plan_batch_shape(plan, batch_index) ||
          plan.batches[batch_index].order < 2U ||
          plan.batches[batch_index].order >
              source_forest.effective_maximum_order) {
        return fail(
            std::move(result),
            BuildFailure::resident_plan_structure_inconsistent);
      }
    }

    std::vector<ProjectionJoinSlot> slots(
        plan.source_event_projection_count);
    result.records.reserve(record_count);

    for (std::size_t physical_index = 0U;
         physical_index < source_forest.birth_records.size();
         ++physical_index) {
      const auto& birth = source_forest.birth_records[physical_index];
      if (birth.order < 2U) {
        continue;
      }
      if (birth.order > source_forest.effective_maximum_order ||
          birth.source_event_projection_index >= slots.size() ||
          birth.source_journal_batch_index >= source_forest.batches.size()) {
        return fail(
            std::move(result), BuildFailure::source_namespace_mismatch);
      }
      const auto& forest_batch =
          source_forest.batches[birth.source_journal_batch_index];
      auto& slot = slots[birth.source_event_projection_index];
      if (forest_batch.batch_index != birth.source_journal_batch_index ||
          forest_batch.order != birth.order || slot.forest_birth.has_value()) {
        return fail(
            std::move(result), BuildFailure::duplicate_event_target);
      }
      slot.forest_birth =
          ForestBirthCandidate{physical_index, birth.source_journal_batch_index};
    }

    for (std::size_t batch_index = 0U;
         batch_index < plan.batches.size();
         ++batch_index) {
      const auto& batch = plan.batches[batch_index];
      for (std::size_t local = 0U; local < batch.direct_reference_count;
           ++local) {
        const std::size_t direct_index = batch.direct_reference_offset + local;
        const auto& direct = plan.direct_references[direct_index];
        if (direct.direct_reference_index != direct_index ||
            direct.source_event_projection_index >= slots.size()) {
          return fail(
              std::move(result),
              BuildFailure::resident_plan_structure_inconsistent);
        }
        auto& slot = slots[direct.source_event_projection_index];
        if (direct.role == ExactDirectMorseH0Role::birth) {
          if (!direct.direct_birth_facet_token_index.has_value() ||
              *direct.direct_birth_facet_token_index >=
                  plan.facet_tokens.size() ||
              slot.upper_birth.has_value()) {
            return fail(
                std::move(result), BuildFailure::duplicate_event_target);
          }
          slot.upper_birth = UpperBirthCandidate{
              batch_index,
              direct_index,
              direct.source_role_record_index,
              *direct.direct_birth_facet_token_index};
        } else if (batch.order < source_forest.effective_maximum_order) {
          if (!direct.source_incidence_family_index.has_value() ||
              slot.lower_saddle_seen) {
            return fail(
                std::move(result), BuildFailure::duplicate_event_target);
          }
          slot.lower_saddle_seen = true;
        }
      }
    }

    for (std::size_t record_index = 0U;
         record_index < k2_batches.size();
         ++record_index) {
      const auto& batch = k2_batches[record_index];
      if (!batch.certified_conditional_k2_to_k1_batch() ||
          batch.order != 2U || !plan_batch_shape(plan, batch.source_batch_index) ||
          plan.batches[batch.source_batch_index].order != batch.order ||
          plan.batches[batch.source_batch_index].squared_level !=
              batch.squared_level ||
          batch.o4_membership_digest == contract::CanonicalId{} ||
          canonical_exact_direct_morse_resident_k2_o4_membership_digest(
              batch) != batch.o4_membership_digest) {
        return fail(
            std::move(result), BuildFailure::bridge_membership_inconsistent);
      }
      const auto& plan_batch = plan.batches[batch.source_batch_index];
      for (std::size_t binding_index = 0U;
           binding_index < batch.direct_birth_k1_bindings.size();
           ++binding_index) {
        const auto& binding = batch.direct_birth_k1_bindings[binding_index];
        if (!binding.certified_conditional_birth_k1_binding() ||
            binding.binding_index != binding_index ||
            binding.source_direct_reference_index >=
                plan.direct_references.size() ||
            !batch_direct_slice_contains(
                plan_batch, binding.source_direct_reference_index)) {
          return fail(
              std::move(result),
              BuildFailure::bridge_membership_inconsistent);
        }
        const auto& direct =
            plan.direct_references[binding.source_direct_reference_index];
        if (direct.direct_reference_index !=
                binding.source_direct_reference_index ||
            direct.role != ExactDirectMorseH0Role::birth ||
            direct.source_role_record_index !=
                binding.source_role_record_index ||
            direct.source_event_projection_index !=
                binding.source_event_projection_index ||
            !direct.direct_birth_facet_token_index.has_value() ||
            *direct.direct_birth_facet_token_index !=
                binding.source_facet_token_index) {
          return fail(
              std::move(result),
              BuildFailure::bridge_membership_inconsistent);
        }
        if (!add_candidate(
                slots,
                binding.source_event_projection_index,
                {CandidateKind::k2_birth, record_index, binding_index})) {
          return fail(
              std::move(result), BuildFailure::duplicate_event_target);
        }
      }
      for (std::size_t binding_index = 0U;
           binding_index < batch.direct_saddle_group_bindings.size();
           ++binding_index) {
        const auto& binding =
            batch.direct_saddle_group_bindings[binding_index];
        if (!binding.certified_conditional_saddle_group_binding() ||
            binding.binding_index != binding_index ||
            binding.source_direct_reference_index >=
                plan.direct_references.size() ||
            !batch_direct_slice_contains(
                plan_batch, binding.source_direct_reference_index)) {
          return fail(
              std::move(result),
              BuildFailure::bridge_membership_inconsistent);
        }
        const auto& direct =
            plan.direct_references[binding.source_direct_reference_index];
        if (direct.direct_reference_index !=
                binding.source_direct_reference_index ||
            direct.role != ExactDirectMorseH0Role::saddle ||
            direct.source_role_record_index !=
                binding.source_role_record_index ||
            direct.source_event_projection_index !=
                binding.source_event_projection_index ||
            !direct.source_incidence_family_index.has_value() ||
            *direct.source_incidence_family_index !=
                binding.source_incidence_family_index) {
          return fail(
              std::move(result),
              BuildFailure::bridge_membership_inconsistent);
        }
        if (plan_batch.order < source_forest.effective_maximum_order &&
            !add_candidate(
                slots,
                binding.source_event_projection_index,
                {CandidateKind::k2_saddle, record_index, binding_index})) {
          return fail(
              std::move(result), BuildFailure::duplicate_event_target);
        }
      }
    }

    for (std::size_t record_index = 0U;
         record_index < higher_batches.size();
         ++record_index) {
      const auto& batch = higher_batches[record_index];
      if (!batch.certified_conditional_higher_batch() ||
          batch.source_order < 3U ||
          !plan_batch_shape(plan, batch.source_batch_index) ||
          plan.batches[batch.source_batch_index].order != batch.source_order ||
          plan.batches[batch.source_batch_index].squared_level !=
              batch.squared_level ||
          batch.o4_membership_digest == contract::CanonicalId{} ||
          canonical_exact_direct_morse_resident_higher_o4_membership_digest(
              batch) != batch.o4_membership_digest) {
        return fail(
            std::move(result), BuildFailure::bridge_membership_inconsistent);
      }
      const auto& plan_batch = plan.batches[batch.source_batch_index];
      for (std::size_t binding_index = 0U;
           binding_index < batch.direct_saddle_group_bindings.size();
           ++binding_index) {
        const auto& binding =
            batch.direct_saddle_group_bindings[binding_index];
        if (!binding.certified_conditional_saddle_group_binding() ||
            binding.binding_index != binding_index ||
            binding.source_direct_reference_index >=
                plan.direct_references.size() ||
            !batch_direct_slice_contains(
                plan_batch, binding.source_direct_reference_index)) {
          return fail(
              std::move(result),
              BuildFailure::bridge_membership_inconsistent);
        }
        const auto& direct =
            plan.direct_references[binding.source_direct_reference_index];
        if (direct.direct_reference_index !=
                binding.source_direct_reference_index ||
            direct.role != ExactDirectMorseH0Role::saddle ||
            direct.source_role_record_index !=
                binding.source_role_record_index ||
            direct.source_event_projection_index !=
                binding.source_event_projection_index ||
            !direct.source_incidence_family_index.has_value() ||
            *direct.source_incidence_family_index !=
                binding.source_incidence_family_index) {
          return fail(
              std::move(result),
              BuildFailure::bridge_membership_inconsistent);
        }
        if (plan_batch.order < source_forest.effective_maximum_order &&
            !add_candidate(
                slots,
                binding.source_event_projection_index,
                {CandidateKind::higher_saddle,
                 record_index,
                 binding_index})) {
          return fail(
              std::move(result), BuildFailure::duplicate_event_target);
        }
      }
    }

    std::size_t rank_two_count = 0U;
    std::size_t higher_count = 0U;
    for (const auto& link : source_link.links) {
      if (link.link_index >= source_link.links.size() ||
          link.source_event_projection_index >= slots.size()) {
        return fail(
            std::move(result), BuildFailure::source_namespace_mismatch);
      }
      auto& slot = slots[link.source_event_projection_index];
      if (slot.target.kind == CandidateKind::none ||
          !slot.upper_birth.has_value() || !slot.forest_birth.has_value()) {
        return fail(std::move(result), BuildFailure::missing_event_target);
      }
      if (slot.link_index.has_value()) {
        return fail(std::move(result), BuildFailure::duplicate_event_target);
      }
      const Candidate candidate = slot.target;
      const auto& upper = *slot.upper_birth;
      const auto& forest_candidate = *slot.forest_birth;
      if (upper.plan_batch_index >= plan.batches.size() ||
          upper.direct_reference_index >= plan.direct_references.size() ||
          upper.facet_token_index >= plan.facet_tokens.size() ||
          forest_candidate.materialized_birth_record_index >=
              source_forest.birth_records.size() ||
          forest_candidate.forest_batch_index >= source_forest.batches.size()) {
        return fail(
            std::move(result), BuildFailure::source_namespace_mismatch);
      }
      const auto& upper_batch = plan.batches[upper.plan_batch_index];
      const auto& upper_direct =
          plan.direct_references[upper.direct_reference_index];
      const auto& upper_token = plan.facet_tokens[upper.facet_token_index];
      const auto& forest_birth = source_forest.birth_records[
          forest_candidate.materialized_birth_record_index];
      const auto& forest_batch =
          source_forest.batches[forest_candidate.forest_batch_index];
      if (link.source_birth_record_index != forest_birth.birth_record_index ||
          upper_direct.role != ExactDirectMorseH0Role::birth ||
          upper_direct.direct_reference_index !=
              upper.direct_reference_index ||
          upper_direct.source_role_record_index !=
              upper.source_role_record_index ||
          upper_direct.source_event_projection_index !=
              link.source_event_projection_index ||
          !upper_direct.direct_birth_facet_token_index.has_value() ||
          *upper_direct.direct_birth_facet_token_index !=
              upper.facet_token_index ||
          upper_token.facet_token_index != upper.facet_token_index ||
          forest_birth.source_event_projection_index !=
              link.source_event_projection_index ||
          forest_birth.source_journal_batch_index !=
              forest_candidate.forest_batch_index ||
          forest_birth.order != link.source_order ||
          forest_batch.order != link.source_order ||
          upper_batch.order != link.source_order ||
          forest_batch.squared_level != link.squared_level ||
          upper_batch.squared_level != link.squared_level ||
          forest_birth.facet_key != upper_token.facet_key) {
        return fail(
            std::move(result), BuildFailure::exact_level_or_order_mismatch);
      }
      slot.link_index = link.link_index;
      ExactDirectMorseResidentEventCrosswalkRecord record;
      record.record_index = result.records.size();
      record.source_link_index = link.link_index;
      record.source_birth_record_index = link.source_birth_record_index;
      record.source_event_projection_index =
          link.source_event_projection_index;
      record.source_order = link.source_order;
      record.target_order = link.target_order;
      record.squared_level = link.squared_level;
      record.source_upper_birth_direct_reference_index =
          upper.direct_reference_index;
      record.source_upper_birth_role_record_index =
          upper.source_role_record_index;
      record.source_upper_birth_facet_token_index = upper.facet_token_index;
      record.source_event_arm_identity_digest =
          link.source_event_arm_identity_digest;

      if (link.source_order == 2U) {
        if (candidate.kind != CandidateKind::k2_birth ||
            candidate.batch_record_index >= k2_batches.size()) {
          return fail(
              std::move(result),
              BuildFailure::exact_level_or_order_mismatch);
        }
        const auto& batch = k2_batches[candidate.batch_record_index];
        if (candidate.binding_index >=
                batch.direct_birth_k1_bindings.size() ||
            batch.order != link.source_order ||
            batch.squared_level != link.squared_level) {
          return fail(
              std::move(result),
              BuildFailure::exact_level_or_order_mismatch);
        }
        const auto& binding =
            batch.direct_birth_k1_bindings[candidate.binding_index];
        record.source_bridge_batch_membership_digest =
            batch.o4_membership_digest;
        record.k1_birth_target =
            ExactDirectMorseResidentEventCrosswalkK1BirthTarget{
                candidate.batch_record_index,
                batch.source_batch_index,
                binding.binding_index,
                binding.source_direct_reference_index,
                binding.source_role_record_index,
                binding.source_facet_token_index,
                {binding.closed_k1_root_node_id}};
        ++rank_two_count;
      } else {
        const ExactDirectMorseResidentDirectSaddleGroupBinding* binding =
            nullptr;
        std::size_t source_batch_index = 0U;
        contract::CanonicalId membership_digest{};
        const bool target_is_k2 = candidate.kind == CandidateKind::k2_saddle;
        if (target_is_k2) {
          if (candidate.batch_record_index >= k2_batches.size()) {
            return fail(
                std::move(result),
                BuildFailure::bridge_membership_inconsistent);
          }
          const auto& batch = k2_batches[candidate.batch_record_index];
          if (candidate.binding_index >=
              batch.direct_saddle_group_bindings.size()) {
            return fail(
                std::move(result),
                BuildFailure::bridge_membership_inconsistent);
          }
          binding =
              &batch.direct_saddle_group_bindings[candidate.binding_index];
          source_batch_index = batch.source_batch_index;
          membership_digest = batch.o4_membership_digest;
          if (batch.order != link.target_order ||
              batch.squared_level != link.squared_level ||
              link.target_order != 2U) {
            return fail(
                std::move(result),
                BuildFailure::exact_level_or_order_mismatch);
          }
        } else if (candidate.kind == CandidateKind::higher_saddle) {
          if (candidate.batch_record_index >= higher_batches.size()) {
            return fail(
                std::move(result),
                BuildFailure::bridge_membership_inconsistent);
          }
          const auto& batch = higher_batches[candidate.batch_record_index];
          if (candidate.binding_index >=
              batch.direct_saddle_group_bindings.size()) {
            return fail(
                std::move(result),
                BuildFailure::bridge_membership_inconsistent);
          }
          binding =
              &batch.direct_saddle_group_bindings[candidate.binding_index];
          source_batch_index = batch.source_batch_index;
          membership_digest = batch.o4_membership_digest;
          if (batch.source_order != link.target_order ||
              batch.squared_level != link.squared_level ||
              link.target_order < 3U) {
            return fail(
                std::move(result),
                BuildFailure::exact_level_or_order_mismatch);
          }
        } else {
          return fail(
              std::move(result),
              BuildFailure::exact_level_or_order_mismatch);
        }
        record.source_bridge_batch_membership_digest = membership_digest;
        record.o4_saddle_target =
            ExactDirectMorseResidentEventCrosswalkO4SaddleTarget{
                target_is_k2,
                candidate.batch_record_index,
                source_batch_index,
                binding->binding_index,
                binding->source_direct_reference_index,
                binding->source_role_record_index,
                binding->source_incidence_family_index,
                binding->source_hyperedge_index,
                binding->owner_group_index,
                binding->group_image_index,
                {binding->resident_resultant_root_id}};
        ++higher_count;
      }
      result.records.push_back(std::move(record));
    }

    for (const auto& slot : slots) {
      const bool has_upper_birth = slot.upper_birth.has_value();
      const bool has_forest_birth = slot.forest_birth.has_value();
      const bool has_link = slot.link_index.has_value();
      if ((has_upper_birth || has_forest_birth) &&
          !(has_upper_birth && has_forest_birth && has_link)) {
        return fail(std::move(result), BuildFailure::missing_event_target);
      }
      if (slot.lower_saddle_seen &&
          (!has_link || slot.target.kind == CandidateKind::none)) {
        return fail(std::move(result), BuildFailure::missing_event_target);
      }
    }

    result.point_count = source_forest.point_count;
    result.effective_maximum_order = source_forest.effective_maximum_order;
    result.source_event_projection_count =
        source_forest.source_event_projection_count;
    result.source_link_count = source_link.links.size();
    result.source_higher_canonical_cloud_digest =
        source_forest.source_higher_canonical_cloud_digest;
    result.source_forest_final_locator_stamp = source_forest.final_locator_stamp;
    result.source_bridge_stamp = bridge_stamp;
    result.source_link_identity_digest = source_link_identity_digest(source_link);
    result.source_bridge_membership_digest = bridge_membership_digest(
        bridge_stamp, k2_batches, higher_batches);
    if (result.source_link_identity_digest == contract::CanonicalId{} ||
        result.source_bridge_membership_digest == contract::CanonicalId{}) {
      return fail(
          std::move(result), BuildFailure::digest_encoding_failed);
    }
    result.logical_output_entry_count = result.records.size();
    result.counters.link_scan_count = source_link.links.size();
    result.counters.resident_plan_batch_scan_count = plan.batches.size();
    result.counters.k2_batch_scan_count = k2_batches.size();
    result.counters.higher_batch_scan_count = higher_batches.size();
    result.counters.k2_direct_birth_binding_scan_count = birth_binding_count;
    result.counters.direct_saddle_group_binding_scan_count =
        saddle_binding_count;
    result.counters.forest_birth_record_scan_count = forest_birth_scan_count;
    result.counters.resident_plan_direct_reference_scan_count =
        plan_direct_reference_scan_count;
    result.counters.resident_direct_reference_replay_count =
        direct_replay_count;
    result.counters.event_projection_scratch_entry_count = slots.size();
    result.counters.rank_two_k1_target_count = rank_two_count;
    result.counters.higher_rank_o4_target_count = higher_count;
    result.counters.record_count = result.records.size();
    result.budget_preflight_certified = true;
    result.product_n_at_least_k_plus_two_replayed = true;
    result.source_conditional_forest_certificate_replayed = true;
    result.source_link_journal_freshly_replayed_relative_to_forest = true;
    result.live_bridge_final_seal_verified = true;
    result.normalized_incidence_complete_v7_live_capability_replayed = true;
    result.canonical_cloud_and_point_namespace_joined = true;
    result
        .resident_projection_indices_joined_conditionally_by_deterministic_canonical_source_order =
        true;
    result.common_source_event_semantic_digest_replayed = false;
    result.every_resident_direct_birth_replayed_against_exact_forest_birth =
        true;
    result.every_resident_direct_birth_has_one_source_link = true;
    result.every_resident_direct_saddle_below_k_has_one_source_link = true;
    result.every_source_link_bound_exactly_once = true;
    result.every_rank_two_link_bound_to_direct_birth_k1_target = true;
    result.every_higher_rank_link_bound_to_lower_direct_saddle_o4_group = true;
    result.exact_level_and_adjacent_order_replayed = true;
    result.bridge_membership_digests_freshly_replayed = true;
    result.target_identifier_namespaces_typed_and_disjoint = true;
    result.o4_membership_complete_for_supplied_resident_batches = true;
    result.no_partial_scientific_payload_published = true;
    result.decision = ExactDirectMorseResidentEventCrosswalkDecision::
        complete_conditional_resident_event_crosswalk;
    result.crosswalk_digest = canonical_crosswalk_result_digest(result);
    if (result.crosswalk_digest == contract::CanonicalId{}) {
      return fail(
          std::move(result), BuildFailure::digest_encoding_failed);
    }
    if (!result.certified_conditional_resident_event_crosswalk()) {
      return fail(
          std::move(result), BuildFailure::bridge_membership_inconsistent);
    }
    return result;
  } catch (const std::bad_alloc&) {
    return fail(std::move(result), BuildFailure::allocation_failed);
  } catch (const std::length_error&) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
}

ExactDirectMorseResidentEventCrosswalkVerification
verify_exact_direct_morse_resident_event_crosswalk_journal(
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseEventRankTowerLinkBudget& trusted_link_budget,
    const ExactDirectMorseEventRankTowerLinkJournalResult& source_link,
    const ExactDirectMorseResidentAllOrdersVerticalBridge& live_bridge,
    const ExactDirectMorseResidentEventCrosswalkBudget& trusted_budget,
    const ExactDirectMorseResidentEventCrosswalkJournalResult& observed) {
  ExactDirectMorseResidentEventCrosswalkVerification verification;
  const auto link_replay =
      verify_exact_direct_morse_event_rank_tower_link_journal(
          source_forest, trusted_link_budget, source_link);
  verification.source_link_freshly_replayed_relative_to_forest =
      link_replay.result_certified;
  verification.live_bridge_final_seal_verified =
      live_bridge.final_vertical_sealed() &&
      live_bridge.final_vertical_seal() != nullptr &&
      live_bridge.verify_final_vertical_seal(*live_bridge.final_vertical_seal());
  verification.observed_storage_within_budget =
      result_storage_within_budget(observed, trusted_budget);
  const auto expected =
      build_exact_direct_morse_resident_event_crosswalk_journal(
          source_forest,
          trusted_link_budget,
          source_link,
          live_bridge,
          trusted_budget);
  verification.expected_journal_freshly_rebuilt =
      expected.certified_outcome();
  verification.observed_recursively_equal =
      verification.observed_storage_within_budget &&
      verification.expected_journal_freshly_rebuilt && observed == expected;
  verification.observed_structure_certified =
      verification.observed_recursively_equal && observed.certified_outcome();
  verification.result_certified =
      verification.observed_storage_within_budget &&
      verification.expected_journal_freshly_rebuilt &&
      verification.observed_structure_certified &&
      verification.observed_recursively_equal;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
