#include "morsehgp3d/hierarchy/direct_sparse_gateway_candidate_journal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

struct DeletionScratchRecord {
  ExactDirectSparseFacetKey facet_key{};
  std::size_t source_family_index{};
  ExactDirectSparseGatewayDeletionSource source{
      ExactDirectSparseGatewayDeletionSource::unspecified};
  std::size_t source_deletion_index{};
  std::size_t source_event_index{};
  std::size_t source_order{};
  PointId removed_point_id{};
  exact::ExactLevel saddle_squared_level{};
};

enum class BuildFailure : std::uint8_t {
  none,
  capacity_overflow,
  budget_exhausted,
  source_join_inconsistent,
  first_incidence_budget_exhausted,
  no_coface_contradiction,
  level_contradiction,
};

[[nodiscard]] bool try_add_size(
    std::size_t left,
    std::size_t right,
    std::size_t& sum) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  sum = left + right;
  return true;
}

[[nodiscard]] bool try_multiply_size(
    std::size_t left,
    std::size_t right,
    std::size_t& product) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  product = left * right;
  return true;
}

void require_valid_traversal_order(
    spatial::LbvhTraversalOrder traversal_order) {
  switch (traversal_order) {
    case spatial::LbvhTraversalOrder::near_first:
    case spatial::LbvhTraversalOrder::far_first:
      return;
  }
  throw std::invalid_argument(
      "a sparse gateway-candidate traversal order is invalid");
}

[[nodiscard]] bool facet_key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  if (left.point_count != right.point_count) {
    return left.point_count < right.point_count;
  }
  return std::lexicographical_compare(
      left.point_ids.begin(),
      left.point_ids.begin() +
          static_cast<std::ptrdiff_t>(left.point_count),
      right.point_ids.begin(),
      right.point_ids.begin() +
          static_cast<std::ptrdiff_t>(right.point_count));
}

[[nodiscard]] bool valid_facet_key(
    const ExactDirectSparseFacetKey& key,
    std::size_t point_count) noexcept {
  if (key.point_count == 0U ||
      key.point_count > key.point_ids.size() ||
      key.point_count > point_count) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (static_cast<std::size_t>(key.point_ids[index]) >= point_count ||
        (index != 0U && key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  for (std::size_t index = key.point_count;
       index < key.point_ids.size();
       ++index) {
    if (key.point_ids[index] != 0U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ExactDirectSparseFacetKey facet_key_from(
    const ExactDirectSaddleArmFacet& facet) {
  if (facet.point_count == 0U ||
      facet.point_count > facet.point_ids.size()) {
    throw std::invalid_argument(
        "a reconstructed direct deletion has an invalid cardinality");
  }
  ExactDirectSparseFacetKey key;
  key.point_count = facet.point_count;
  std::copy_n(
      facet.point_ids.begin(), facet.point_count, key.point_ids.begin());
  return key;
}

[[nodiscard]] bool deletion_scratch_less(
    const DeletionScratchRecord& left,
    const DeletionScratchRecord& right) noexcept {
  if (facet_key_less(left.facet_key, right.facet_key)) {
    return true;
  }
  if (facet_key_less(right.facet_key, left.facet_key)) {
    return false;
  }
  if (left.source_family_index != right.source_family_index) {
    return left.source_family_index < right.source_family_index;
  }
  if (left.removed_point_id != right.removed_point_id) {
    return left.removed_point_id < right.removed_point_id;
  }
  if (left.source != right.source) {
    return static_cast<std::uint8_t>(left.source) <
           static_cast<std::uint8_t>(right.source);
  }
  return left.source_deletion_index < right.source_deletion_index;
}

[[nodiscard]] bool same_key(
    const DeletionScratchRecord& left,
    const DeletionScratchRecord& right) noexcept {
  return left.facet_key == right.facet_key;
}

void clear_scientific_payload(
    ExactDirectSparseGatewayCandidateJournalResult& result) {
  result.deletion_projections.clear();
  result.facet_tokens.clear();
  result.gateway_candidates.clear();
  result.batches.clear();
  result.batch_facet_token_indices.clear();
  result.logical_storage_entry_count = 0U;
  result.no_partial_scientific_payload_published = true;
}

[[nodiscard]] ExactDirectSparseGatewayCandidateDecision decision_for(
    BuildFailure failure) {
  switch (failure) {
    case BuildFailure::capacity_overflow:
      return ExactDirectSparseGatewayCandidateDecision::
          no_gateway_candidate_capacity_overflow;
    case BuildFailure::budget_exhausted:
      return ExactDirectSparseGatewayCandidateDecision::
          no_gateway_candidate_budget_exhausted;
    case BuildFailure::source_join_inconsistent:
      return ExactDirectSparseGatewayCandidateDecision::
          no_gateway_candidate_source_join_inconsistent;
    case BuildFailure::first_incidence_budget_exhausted:
      return ExactDirectSparseGatewayCandidateDecision::
          no_gateway_candidate_first_incidence_budget_exhausted;
    case BuildFailure::no_coface_contradiction:
      return ExactDirectSparseGatewayCandidateDecision::
          no_gateway_candidate_no_coface_contradiction;
    case BuildFailure::level_contradiction:
      return ExactDirectSparseGatewayCandidateDecision::
          no_gateway_candidate_level_contradiction;
    case BuildFailure::none:
      break;
  }
  throw std::logic_error(
      "a sparse gateway-candidate failure has no decision");
}

[[nodiscard]] ExactDirectSparseGatewayCandidateJournalResult fail(
    ExactDirectSparseGatewayCandidateJournalResult result,
    BuildFailure failure) {
  clear_scientific_payload(result);
  result.decision = decision_for(failure);
  return result;
}

[[nodiscard]] bool source_verification_closes(
    const ExactDirectClosedSaddleIncidenceStreamingVerification&
        verification) noexcept {
  return verification.source_arm_journal_certified &&
         verification.requirements_certified &&
         verification.family_records_certified &&
         verification.equal_level_facet_seed_records_certified &&
         verification.deletion_partition_certified &&
         verification.factorized_facets_certified &&
         verification.result_facts_certified &&
         verification.decision_and_scope_certified &&
         verification.constant_auxiliary_record_storage_certified &&
         verification.fresh_streaming_replay_certified &&
         verification.result_certified;
}

[[nodiscard]] bool add_logical_entries(
    std::size_t increment,
    const ExactDirectSparseGatewayCandidateBudget& budget,
    std::size_t& logical_storage_entry_count,
    BuildFailure& failure) noexcept {
  std::size_t sum = 0U;
  if (!try_add_size(
          logical_storage_entry_count, increment, sum)) {
    failure = BuildFailure::capacity_overflow;
    return false;
  }
  logical_storage_entry_count = sum;
  if (logical_storage_entry_count >
      budget.maximum_logical_storage_entry_count) {
    failure = BuildFailure::budget_exhausted;
    return false;
  }
  return true;
}

[[nodiscard]] bool gateway_candidate_added_point_less(
    const ExactDirectSparseFirstIncidenceMinimizer& minimizer,
    PointId point_id) noexcept {
  return minimizer.added_point_id < point_id;
}

[[nodiscard]] bool observed_scientific_storage_within_budget(
    const ExactDirectSparseGatewayCandidateJournalResult& observed,
    const ExactDirectSparseGatewayCandidateBudget& budget) noexcept {
  if (observed.deletion_projections.size() >
          budget.maximum_deletion_reference_count ||
      observed.facet_tokens.size() > budget.maximum_distinct_facet_count ||
      observed.gateway_candidates.size() >
          budget.maximum_gateway_candidate_count ||
      observed.batches.size() > budget.maximum_batch_count ||
      observed.batch_facet_token_indices.size() >
          budget.maximum_batch_facet_reference_count ||
      observed.facet_tokens.size() > observed.deletion_projections.size() ||
      observed.batch_facet_token_indices.size() !=
          observed.facet_tokens.size()) {
    return false;
  }

  std::size_t logical_storage_entry_count = 0U;
  std::size_t facet_key_point_count = 0U;
  for (const std::size_t arena_size : {
           observed.deletion_projections.size(),
           observed.facet_tokens.size(),
           observed.gateway_candidates.size(),
           observed.batches.size(),
           observed.batch_facet_token_indices.size()}) {
    if (!try_add_size(
            logical_storage_entry_count,
            arena_size,
            logical_storage_entry_count)) {
      return false;
    }
  }
  for (const ExactDirectSparseGatewayFacetToken& token :
       observed.facet_tokens) {
    if (token.source_facet_key.point_count >
            token.source_facet_key.point_ids.size() ||
        !try_add_size(
            facet_key_point_count,
            token.source_facet_key.point_count,
            facet_key_point_count) ||
        !try_add_size(
            logical_storage_entry_count,
            token.source_facet_key.point_count,
            logical_storage_entry_count)) {
      return false;
    }
  }
  if (facet_key_point_count > budget.maximum_facet_key_point_count) {
    return false;
  }
  for (const ExactDirectSparseGatewayCandidateRecord& candidate :
       observed.gateway_candidates) {
    if (candidate.positive_support_point_count >
            candidate.positive_support_point_ids.size() ||
        !try_add_size(
            logical_storage_entry_count,
            candidate.positive_support_point_count,
            logical_storage_entry_count)) {
      return false;
    }
  }
  return logical_storage_entry_count <=
             budget.maximum_logical_storage_entry_count &&
         observed.logical_storage_entry_count ==
             logical_storage_entry_count;
}

[[nodiscard]] bool first_incidence_audit_within_budget(
    const ExactDirectSparseFirstIncidenceAudit& audit,
    const ExactDirectSparseFirstIncidenceBudget& budget) noexcept {
  return audit.source_support_enumeration_count <=
             budget.maximum_source_support_enumeration_count &&
         audit.node_visit_count <= budget.maximum_node_visit_count &&
         audit.internal_node_expansion_count <=
             budget.maximum_internal_node_expansion_count &&
         audit.exact_aabb_bound_evaluation_count <=
             budget.maximum_exact_aabb_bound_evaluation_count &&
         audit.exact_point_evaluation_count <=
             budget.maximum_exact_point_evaluation_count &&
         audit.coface_support_enumeration_count <=
             budget.maximum_coface_support_enumeration_count &&
         audit.candidate_point_classification_count <=
             budget.maximum_candidate_point_classification_count &&
         audit.peak_frontier_entry_count <=
             budget.maximum_frontier_entry_count &&
         audit.peak_cominimizer_entry_count <=
             budget.maximum_cominimizer_count;
}

[[nodiscard]] bool source_result_facts_match(
    const ExactDirectSparseGatewayCandidateJournalResult& expected,
    const ExactDirectSparseGatewayCandidateJournalResult& observed) {
  return observed.schema_version == expected.schema_version &&
         observed.requested_budget == expected.requested_budget &&
         observed.traversal_order == expected.traversal_order &&
         observed.point_count == expected.point_count &&
         observed.source_direct_event_count ==
             expected.source_direct_event_count &&
         observed.required_source_family_scan_count ==
             expected.required_source_family_scan_count &&
         observed.required_deletion_reference_count ==
             expected.required_deletion_reference_count &&
         observed.required_distinct_facet_count ==
             expected.required_distinct_facet_count &&
         observed.required_facet_key_point_count ==
             expected.required_facet_key_point_count &&
         observed.required_first_incidence_call_count ==
             expected.required_first_incidence_call_count &&
         observed.required_gateway_candidate_count ==
             expected.required_gateway_candidate_count &&
         observed.required_batch_count == expected.required_batch_count &&
         observed.required_batch_facet_reference_count ==
             expected.required_batch_facet_reference_count &&
         observed.logical_storage_entry_count ==
             expected.logical_storage_entry_count &&
         observed.source_pair_canonical_cloud_digest ==
             expected.source_pair_canonical_cloud_digest &&
         observed.source_higher_canonical_cloud_digest ==
             expected.source_higher_canonical_cloud_digest &&
         observed.source_pair_semantic_digest ==
             expected.source_pair_semantic_digest &&
         observed.source_higher_semantic_digest ==
             expected.source_higher_semantic_digest &&
         observed.budget_preflight_certified ==
             expected.budget_preflight_certified &&
         observed.source_incidence_journal_freshly_replayed ==
             expected.source_incidence_journal_freshly_replayed &&
         observed.every_strict_and_equal_deletion_reconstructed ==
             expected.every_strict_and_equal_deletion_reconstructed &&
         observed.deletion_references_sorted_by_full_key ==
             expected.deletion_references_sorted_by_full_key &&
         observed.distinct_full_keys_deduplicated ==
             expected.distinct_full_keys_deduplicated &&
         observed.one_first_incidence_call_per_distinct_facet ==
             expected.one_first_incidence_call_per_distinct_facet &&
         observed.every_first_incidence_complete ==
             expected.every_first_incidence_complete &&
         observed.every_first_incidence_at_or_below_each_saddle ==
             expected.every_first_incidence_at_or_below_each_saddle &&
         observed.strict_and_equal_level_relations_classified ==
             expected.strict_and_equal_level_relations_classified &&
         observed.all_positive_support_candidates_retained_atomically ==
             expected.all_positive_support_candidates_retained_atomically &&
         observed.batches_canonical_and_partition_tokens ==
             expected.batches_canonical_and_partition_tokens &&
         observed.logical_storage_within_budget ==
             expected.logical_storage_within_budget &&
         observed.output_linear_in_references_key_points_and_candidates ==
             expected.output_linear_in_references_key_points_and_candidates &&
         observed.no_partial_scientific_payload_published ==
             expected.no_partial_scientific_payload_published &&
         observed.no_forbidden_global_structure_materialized ==
             expected.no_forbidden_global_structure_materialized &&
         observed.eleven_point_coface_keys_materialized ==
             expected.eleven_point_coface_keys_materialized &&
         observed.locator_or_quotient_consulted ==
             expected.locator_or_quotient_consulted &&
         observed.root_union_or_forest_mutated ==
             expected.root_union_or_forest_mutated &&
         observed.gateway_attach_published ==
             expected.gateway_attach_published &&
         observed.gamma_cells_or_higher_order_delaunay_materialized ==
             expected.gamma_cells_or_higher_order_delaunay_materialized &&
         observed.public_status_claimed == expected.public_status_claimed &&
         observed.partial_refinement_only ==
             expected.partial_refinement_only &&
         observed.decision == expected.decision &&
         observed.scope == expected.scope;
}

[[nodiscard]] bool non_scope_is_honest(
    const ExactDirectSparseGatewayCandidateJournalResult& result) noexcept {
  return result.no_forbidden_global_structure_materialized &&
         !result.eleven_point_coface_keys_materialized &&
         !result.locator_or_quotient_consulted &&
         !result.root_union_or_forest_mutated &&
         !result.gateway_attach_published &&
         !result.gamma_cells_or_higher_order_delaunay_materialized &&
         !result.public_status_claimed && result.partial_refinement_only;
}

}  // namespace

bool ExactDirectSparseGatewayCandidateJournalResult::
    certified_partial_refinement() const {
  if (schema_version !=
          direct_sparse_gateway_candidate_journal_schema_version ||
      decision != ExactDirectSparseGatewayCandidateDecision::
                      complete_certified_sparse_gateway_candidates ||
      scope != ExactDirectSparseGatewayCandidateScope::
                   direct_saddle_deletion_facets_first_incidence_candidates_only ||
      !budget_preflight_certified ||
      !source_incidence_journal_freshly_replayed ||
      !every_strict_and_equal_deletion_reconstructed ||
      !deletion_references_sorted_by_full_key ||
      !distinct_full_keys_deduplicated ||
      !one_first_incidence_call_per_distinct_facet ||
      !every_first_incidence_complete ||
      !every_first_incidence_at_or_below_each_saddle ||
      !strict_and_equal_level_relations_classified ||
      !all_positive_support_candidates_retained_atomically ||
      !batches_canonical_and_partition_tokens ||
      !logical_storage_within_budget ||
      !output_linear_in_references_key_points_and_candidates ||
      !no_partial_scientific_payload_published || !non_scope_is_honest(*this) ||
      deletion_projections.size() != required_deletion_reference_count ||
      facet_tokens.size() != required_distinct_facet_count ||
      gateway_candidates.size() != required_gateway_candidate_count ||
      batches.size() != required_batch_count ||
      batch_facet_token_indices.size() !=
          required_batch_facet_reference_count ||
      required_first_incidence_call_count !=
          required_distinct_facet_count ||
      required_batch_facet_reference_count !=
          required_distinct_facet_count ||
      required_source_family_scan_count >
          requested_budget.maximum_source_family_scan_count ||
      required_deletion_reference_count >
          requested_budget.maximum_deletion_reference_count ||
      required_distinct_facet_count >
          requested_budget.maximum_distinct_facet_count ||
      required_facet_key_point_count >
          requested_budget.maximum_facet_key_point_count ||
      required_gateway_candidate_count >
          requested_budget.maximum_gateway_candidate_count ||
      required_batch_count > requested_budget.maximum_batch_count ||
      required_batch_facet_reference_count >
          requested_budget.maximum_batch_facet_reference_count ||
      logical_storage_entry_count >
          requested_budget.maximum_logical_storage_entry_count) {
    return false;
  }
  std::size_t maximum_deletion_reference_count = 0U;
  if (!try_multiply_size(
          11U,
          required_source_family_scan_count,
          maximum_deletion_reference_count) ||
      required_deletion_reference_count > maximum_deletion_reference_count ||
      required_distinct_facet_count > required_deletion_reference_count) {
    return false;
  }

  std::size_t expected_projection_offset = 0U;
  std::size_t expected_candidate_offset = 0U;
  std::size_t observed_facet_key_point_count = 0U;
  std::size_t observed_support_point_count = 0U;
  for (std::size_t token_index = 0U;
       token_index < facet_tokens.size();
       ++token_index) {
    const ExactDirectSparseGatewayFacetToken& token =
        facet_tokens[token_index];
    if (token.facet_token_index != token_index ||
        !valid_facet_key(token.source_facet_key, point_count) ||
        (token_index != 0U &&
         !facet_key_less(
             facet_tokens[token_index - 1U].source_facet_key,
             token.source_facet_key)) ||
        token.source_miniball_squared_level >
            token.first_incidence_squared_level ||
        token.deletion_projection_offset != expected_projection_offset ||
        token.deletion_projection_count == 0U ||
        token.deletion_projection_count >
            deletion_projections.size() - expected_projection_offset ||
        token.gateway_candidate_offset != expected_candidate_offset ||
        token.gateway_candidate_count == 0U ||
        token.gateway_candidate_count >
            gateway_candidates.size() - expected_candidate_offset ||
        token.batch_index >= batches.size() ||
        token.first_incidence_audit.excluded_facet_point_count !=
            token.source_facet_key.point_count ||
        token.first_incidence_audit.eligible_coface_point_count !=
            point_count - token.source_facet_key.point_count ||
        !token.first_incidence_audit.traversal_complete ||
        !first_incidence_audit_within_budget(
            token.first_incidence_audit,
            requested_budget.first_incidence_budget) ||
        token.first_incidence_audit.peak_cominimizer_entry_count <
            token.gateway_candidate_count ||
        !try_add_size(
            observed_facet_key_point_count,
            token.source_facet_key.point_count,
            observed_facet_key_point_count)) {
      return false;
    }

    PointId previous_added_point_id = 0U;
    bool first_candidate = true;
    for (std::size_t local_candidate_index = 0U;
         local_candidate_index < token.gateway_candidate_count;
         ++local_candidate_index) {
      const std::size_t candidate_index =
          token.gateway_candidate_offset + local_candidate_index;
      const ExactDirectSparseGatewayCandidateRecord& candidate =
          gateway_candidates[candidate_index];
      if (candidate.gateway_candidate_index != candidate_index ||
          candidate.facet_token_index != token_index ||
          static_cast<std::size_t>(candidate.added_point_id) >= point_count ||
          std::binary_search(
              token.source_facet_key.point_ids.begin(),
              token.source_facet_key.point_ids.begin() +
                  static_cast<std::ptrdiff_t>(
                      token.source_facet_key.point_count),
              candidate.added_point_id) ||
          (!first_candidate &&
           candidate.added_point_id <= previous_added_point_id) ||
          candidate.positive_support_point_count == 0U ||
          candidate.positive_support_point_count >
              candidate.positive_support_point_ids.size() ||
          candidate.added_point_in_source_closed_ball ==
              candidate.added_point_in_selected_positive_support ||
          !try_add_size(
              observed_support_point_count,
              candidate.positive_support_point_count,
              observed_support_point_count)) {
        return false;
      }
      bool support_contains_added_point = false;
      for (std::size_t support_index = 0U;
           support_index < candidate.positive_support_point_ids.size();
           ++support_index) {
        const PointId support_point_id =
            candidate.positive_support_point_ids[support_index];
        if (support_index >= candidate.positive_support_point_count) {
          if (support_point_id != 0U) {
            return false;
          }
          continue;
        }
        if (static_cast<std::size_t>(support_point_id) >= point_count ||
            (support_index != 0U &&
             candidate.positive_support_point_ids[support_index - 1U] >=
                 support_point_id)) {
          return false;
        }
        if (support_point_id == candidate.added_point_id) {
          support_contains_added_point = true;
        } else if (!std::binary_search(
                       token.source_facet_key.point_ids.begin(),
                       token.source_facet_key.point_ids.begin() +
                           static_cast<std::ptrdiff_t>(
                               token.source_facet_key.point_count),
                       support_point_id)) {
          return false;
        }
      }
      if (support_contains_added_point !=
          candidate.added_point_in_selected_positive_support) {
        return false;
      }
      previous_added_point_id = candidate.added_point_id;
      first_candidate = false;
    }

    for (std::size_t local_projection_index = 0U;
         local_projection_index < token.deletion_projection_count;
         ++local_projection_index) {
      const std::size_t projection_index =
          token.deletion_projection_offset + local_projection_index;
      const ExactDirectSparseGatewayDeletionProjection& projection =
          deletion_projections[projection_index];
      const bool equal_level =
          token.first_incidence_squared_level ==
          projection.saddle_squared_level;
      if (projection.deletion_projection_index != projection_index ||
          projection.source_family_index >=
              required_source_family_scan_count ||
          projection.source_event_index >= source_direct_event_count ||
          projection.source_order != token.source_facet_key.point_count ||
          static_cast<std::size_t>(projection.removed_point_id) >=
              point_count ||
          std::binary_search(
              token.source_facet_key.point_ids.begin(),
              token.source_facet_key.point_ids.begin() +
                  static_cast<std::ptrdiff_t>(
                      token.source_facet_key.point_count),
              projection.removed_point_id) ||
          projection.facet_token_index != token_index ||
          token.first_incidence_squared_level >
              projection.saddle_squared_level ||
          (projection.source ==
                   ExactDirectSparseGatewayDeletionSource::strict_arm_seed
               ? !(token.source_miniball_squared_level <
                   projection.saddle_squared_level)
               : projection.source ==
                         ExactDirectSparseGatewayDeletionSource::
                             equal_level_facet_seed
                     ? token.source_miniball_squared_level !=
                           projection.saddle_squared_level
                     : true) ||
          projection.level_relation !=
              (equal_level
                   ? ExactDirectSparseGatewayLevelRelation::
                         first_incidence_equal_to_saddle
                   : ExactDirectSparseGatewayLevelRelation::
                         first_incidence_strictly_below_saddle) ||
          projection.removed_point_is_first_incidence_cominimizer !=
              equal_level) {
        return false;
      }
      const auto candidate_begin =
          gateway_candidates.begin() +
          static_cast<std::ptrdiff_t>(token.gateway_candidate_offset);
      const auto candidate_end = candidate_begin +
          static_cast<std::ptrdiff_t>(token.gateway_candidate_count);
      const auto matching_candidate = std::lower_bound(
          candidate_begin,
          candidate_end,
          projection.removed_point_id,
          [](const ExactDirectSparseGatewayCandidateRecord& candidate,
             PointId point_id) {
            return candidate.added_point_id < point_id;
          });
      const bool removed_point_is_candidate =
          matching_candidate != candidate_end &&
          matching_candidate->added_point_id == projection.removed_point_id;
      if (removed_point_is_candidate != equal_level) {
        return false;
      }
    }
    expected_projection_offset += token.deletion_projection_count;
    expected_candidate_offset += token.gateway_candidate_count;
  }
  if (expected_projection_offset != deletion_projections.size() ||
      expected_candidate_offset != gateway_candidates.size() ||
      observed_facet_key_point_count != required_facet_key_point_count) {
    return false;
  }

  std::vector<bool> token_seen(facet_tokens.size(), false);
  std::size_t expected_batch_token_offset = 0U;
  for (std::size_t batch_index = 0U;
       batch_index < batches.size();
       ++batch_index) {
    const ExactDirectSparseGatewayCandidateBatch& batch = batches[batch_index];
    if (batch.batch_index != batch_index ||
        batch.facet_cardinality == 0U ||
        batch.facet_cardinality > 10U ||
        batch.facet_token_index_offset != expected_batch_token_offset ||
        batch.facet_token_index_count == 0U ||
        batch.facet_token_index_count >
            batch_facet_token_indices.size() - expected_batch_token_offset ||
        (batch_index != 0U &&
         !(batches[batch_index - 1U].facet_cardinality <
               batch.facet_cardinality ||
           (batches[batch_index - 1U].facet_cardinality ==
                batch.facet_cardinality &&
            batches[batch_index - 1U].first_incidence_squared_level <
                batch.first_incidence_squared_level)))) {
      return false;
    }
    std::size_t previous_token_index = 0U;
    bool first_token = true;
    for (std::size_t local_token_index = 0U;
         local_token_index < batch.facet_token_index_count;
         ++local_token_index) {
      const std::size_t token_index = batch_facet_token_indices[
          batch.facet_token_index_offset + local_token_index];
      if (token_index >= facet_tokens.size() || token_seen[token_index]) {
        return false;
      }
      const ExactDirectSparseGatewayFacetToken& token =
          facet_tokens[token_index];
      if (token.batch_index != batch_index ||
          token.source_facet_key.point_count != batch.facet_cardinality ||
          token.first_incidence_squared_level !=
              batch.first_incidence_squared_level ||
          (!first_token &&
           !facet_key_less(
               facet_tokens[previous_token_index].source_facet_key,
               token.source_facet_key))) {
        return false;
      }
      token_seen[token_index] = true;
      previous_token_index = token_index;
      first_token = false;
    }
    expected_batch_token_offset += batch.facet_token_index_count;
  }
  if (expected_batch_token_offset != batch_facet_token_indices.size() ||
      !std::all_of(token_seen.begin(), token_seen.end(), [](bool seen) {
        return seen;
      })) {
    return false;
  }

  std::size_t recomputed_logical_storage_entry_count = 0U;
  for (const std::size_t increment : {
           deletion_projections.size(),
           facet_tokens.size(),
           observed_facet_key_point_count,
           gateway_candidates.size(),
           observed_support_point_count,
           batches.size(),
           batch_facet_token_indices.size()}) {
    if (!try_add_size(
            recomputed_logical_storage_entry_count,
            increment,
            recomputed_logical_storage_entry_count)) {
      return false;
    }
  }
  return recomputed_logical_storage_entry_count ==
         logical_storage_entry_count;
}

ExactDirectSparseGatewayCandidateJournalResult
build_exact_direct_sparse_gateway_candidate_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& budget,
    spatial::LbvhTraversalOrder traversal_order) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the sparse gateway-candidate LBVH has a different point authority");
  }
  require_valid_traversal_order(traversal_order);

  ExactDirectSparseGatewayCandidateJournalResult result;
  result.requested_budget = budget;
  result.traversal_order = traversal_order;
  result.point_count = cloud.size();
  result.source_direct_event_count = source_facade.events.size();
  result.scope = ExactDirectSparseGatewayCandidateScope::
      direct_saddle_deletion_facets_first_incidence_candidates_only;
  result.source_pair_canonical_cloud_digest =
      source_incidence_journal.source_pair_canonical_cloud_digest;
  result.source_higher_canonical_cloud_digest =
      source_incidence_journal.source_higher_canonical_cloud_digest;
  result.source_pair_semantic_digest =
      source_incidence_journal.source_pair_semantic_digest;
  result.source_higher_semantic_digest =
      source_incidence_journal.source_higher_semantic_digest;
  result.no_partial_scientific_payload_published = true;
  result.no_forbidden_global_structure_materialized = true;
  result.eleven_point_coface_keys_materialized = false;
  result.locator_or_quotient_consulted = false;
  result.root_union_or_forest_mutated = false;
  result.gateway_attach_published = false;
  result.gamma_cells_or_higher_order_delaunay_materialized = false;
  result.public_status_claimed = false;
  result.partial_refinement_only = true;

  const auto source_verification =
      verify_exact_direct_closed_saddle_incidence_journal_streaming(
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal,
          source_incidence_budget,
          source_incidence_journal);
  if (!source_verification_closes(source_verification) ||
      !source_incidence_journal.certified_partial_refinement()) {
    result.decision = ExactDirectSparseGatewayCandidateDecision::
        no_gateway_candidate_source_not_certified;
    return result;
  }
  result.source_incidence_journal_freshly_replayed = true;
  result.required_source_family_scan_count =
      source_incidence_journal.families.size();
  if (result.required_source_family_scan_count >
      budget.maximum_source_family_scan_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }

  std::size_t deletion_reference_count = 0U;
  if (!try_add_size(
          source_arm_journal.arm_seeds.size(),
          source_incidence_journal.equal_level_facet_seeds.size(),
          deletion_reference_count)) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  result.required_deletion_reference_count = deletion_reference_count;
  if (deletion_reference_count >
      budget.maximum_deletion_reference_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }
  std::size_t eleven_times_family_count = 0U;
  if (!try_multiply_size(
          11U,
          result.required_source_family_scan_count,
          eleven_times_family_count)) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  if (deletion_reference_count > eleven_times_family_count) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }

  std::vector<DeletionScratchRecord> scratch;
  scratch.reserve(deletion_reference_count);
  for (const ExactDirectClosedSaddleIncidenceFamilyRecord& family :
       source_incidence_journal.families) {
    if (family.family_index >= source_incidence_journal.families.size() ||
        family.source_arm_family_index >= source_arm_journal.families.size() ||
        family.source_event_index >= source_facade.events.size()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    for (std::size_t local = 0U;
         local < family.strict_arm_seed_count;
         ++local) {
      std::size_t seed_index = 0U;
      if (!try_add_size(
              family.strict_arm_seed_offset, local, seed_index) ||
          seed_index >= source_arm_journal.arm_seeds.size()) {
        return fail(std::move(result), BuildFailure::source_join_inconsistent);
      }
      const ExactDirectSaddleArmSeedRecord& seed =
          source_arm_journal.arm_seeds[seed_index];
      scratch.push_back(DeletionScratchRecord{
          facet_key_from(reconstruct_exact_direct_saddle_arm_facet(
              source_facade, source_arm_journal, seed_index)),
          family.family_index,
          ExactDirectSparseGatewayDeletionSource::strict_arm_seed,
          seed_index,
          family.source_event_index,
          family.order,
          seed.removed_support_point_id,
          family.critical_squared_level});
    }
    for (std::size_t local = 0U;
         local < family.equal_level_facet_seed_count;
         ++local) {
      std::size_t seed_index = 0U;
      if (!try_add_size(
              family.equal_level_facet_seed_offset, local, seed_index) ||
          seed_index >=
              source_incidence_journal.equal_level_facet_seeds.size()) {
        return fail(std::move(result), BuildFailure::source_join_inconsistent);
      }
      const ExactDirectEqualLevelFacetSeedRecord& seed =
          source_incidence_journal.equal_level_facet_seeds[seed_index];
      scratch.push_back(DeletionScratchRecord{
          facet_key_from(
              reconstruct_exact_direct_equal_level_saddle_facet(
                  source_facade,
                  source_arm_journal,
                  source_incidence_journal,
                  seed_index)),
          family.family_index,
          ExactDirectSparseGatewayDeletionSource::equal_level_facet_seed,
          seed_index,
          family.source_event_index,
          family.order,
          seed.removed_interior_point_id,
          family.critical_squared_level});
    }
  }
  if (scratch.size() != deletion_reference_count) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }
  std::sort(scratch.begin(), scratch.end(), deletion_scratch_less);
  result.every_strict_and_equal_deletion_reconstructed = true;
  result.deletion_references_sorted_by_full_key = true;

  std::size_t distinct_facet_count = 0U;
  std::size_t facet_key_point_count = 0U;
  for (std::size_t begin = 0U; begin < scratch.size();) {
    std::size_t end = begin + 1U;
    while (end < scratch.size() && same_key(scratch[begin], scratch[end])) {
      ++end;
    }
    ++distinct_facet_count;
    if (!try_add_size(
            facet_key_point_count,
            scratch[begin].facet_key.point_count,
            facet_key_point_count)) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    begin = end;
  }
  result.required_distinct_facet_count = distinct_facet_count;
  result.required_facet_key_point_count = facet_key_point_count;
  result.required_first_incidence_call_count = distinct_facet_count;
  result.required_batch_facet_reference_count = distinct_facet_count;
  if (distinct_facet_count > budget.maximum_distinct_facet_count ||
      facet_key_point_count > budget.maximum_facet_key_point_count ||
      distinct_facet_count >
          budget.maximum_batch_facet_reference_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }
  result.distinct_full_keys_deduplicated = true;

  BuildFailure storage_failure = BuildFailure::none;
  std::size_t logical_storage_entry_count = 0U;
  for (const std::size_t base_increment : {
           deletion_reference_count,
           distinct_facet_count,
           facet_key_point_count,
           distinct_facet_count}) {
    if (!add_logical_entries(
            base_increment,
            budget,
            logical_storage_entry_count,
            storage_failure)) {
      result.logical_storage_entry_count = logical_storage_entry_count;
      return fail(std::move(result), storage_failure);
    }
  }
  result.budget_preflight_certified = true;

  std::vector<ExactDirectSparseGatewayDeletionProjection> projections;
  std::vector<ExactDirectSparseGatewayFacetToken> tokens;
  std::vector<ExactDirectSparseGatewayCandidateRecord> candidates;
  projections.reserve(deletion_reference_count);
  tokens.reserve(distinct_facet_count);
  candidates.reserve(std::min(
      budget.maximum_gateway_candidate_count, std::size_t{64U}));

  std::size_t token_index = 0U;
  for (std::size_t begin = 0U; begin < scratch.size();) {
    std::size_t end = begin + 1U;
    while (end < scratch.size() && same_key(scratch[begin], scratch[end])) {
      ++end;
    }
    if (candidates.size() >= budget.maximum_gateway_candidate_count) {
      if (budget.maximum_gateway_candidate_count ==
          std::numeric_limits<std::size_t>::max()) {
        return fail(std::move(result), BuildFailure::capacity_overflow);
      }
      result.required_gateway_candidate_count =
          budget.maximum_gateway_candidate_count + 1U;
      result.logical_storage_entry_count = logical_storage_entry_count;
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }
    const std::size_t remaining_gateway_candidate_count =
        budget.maximum_gateway_candidate_count - candidates.size();
    ExactDirectSparseFirstIncidenceBudget first_incidence_budget =
        budget.first_incidence_budget;
    const bool global_candidate_cap_is_tighter =
        remaining_gateway_candidate_count <
        first_incidence_budget.maximum_cominimizer_count;
    first_incidence_budget.maximum_cominimizer_count = std::min(
        first_incidence_budget.maximum_cominimizer_count,
        remaining_gateway_candidate_count);
    const ExactDirectSparseFirstIncidenceResult first =
        build_exact_direct_sparse_first_incidence(
            index,
            cloud,
            scratch[begin].facet_key,
            first_incidence_budget,
            traversal_order);
    if (first.certified_budget_exhaustion()) {
      result.logical_storage_entry_count = logical_storage_entry_count;
      if (global_candidate_cap_is_tighter &&
          first.stop_reason ==
              ExactDirectSparseFirstIncidenceStopReason::
                  cominimizer_entry_limit) {
        if (budget.maximum_gateway_candidate_count ==
            std::numeric_limits<std::size_t>::max()) {
          return fail(std::move(result), BuildFailure::capacity_overflow);
        }
        result.required_gateway_candidate_count =
            budget.maximum_gateway_candidate_count + 1U;
        return fail(std::move(result), BuildFailure::budget_exhausted);
      }
      return fail(
          std::move(result),
          BuildFailure::first_incidence_budget_exhausted);
    }
    if (first.certified_complete_no_coface()) {
      result.logical_storage_entry_count = logical_storage_entry_count;
      return fail(
          std::move(result), BuildFailure::no_coface_contradiction);
    }
    if (!first.certified_complete_first_incidence() ||
        !first.source_facet_miniball.has_value() ||
        !first.first_incidence_squared_level.has_value()) {
      result.logical_storage_entry_count = logical_storage_entry_count;
      return fail(
          std::move(result), BuildFailure::source_join_inconsistent);
    }
    std::size_t candidate_count_after_query = 0U;
    if (!try_add_size(
            candidates.size(),
            first.cominimizers.size(),
            candidate_count_after_query)) {
      result.logical_storage_entry_count = logical_storage_entry_count;
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    if (candidate_count_after_query >
        budget.maximum_gateway_candidate_count) {
      result.required_gateway_candidate_count = candidate_count_after_query;
      result.logical_storage_entry_count = logical_storage_entry_count;
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }

    const std::size_t candidate_offset = candidates.size();
    for (const ExactDirectSparseFirstIncidenceMinimizer& minimizer :
         first.cominimizers) {
      std::size_t candidate_logical_entries = 0U;
      if (!try_add_size(
              1U,
              minimizer.support_point_count,
              candidate_logical_entries)) {
        result.logical_storage_entry_count = logical_storage_entry_count;
        return fail(std::move(result), BuildFailure::capacity_overflow);
      }
      if (!add_logical_entries(
              candidate_logical_entries,
              budget,
              logical_storage_entry_count,
              storage_failure)) {
        result.logical_storage_entry_count = logical_storage_entry_count;
        return fail(std::move(result), storage_failure);
      }
      candidates.push_back(ExactDirectSparseGatewayCandidateRecord{
          candidates.size(),
          token_index,
          minimizer.added_point_id,
          minimizer.support_point_ids,
          minimizer.support_point_count,
          minimizer.added_point_in_source_closed_ball,
          minimizer.added_point_in_selected_positive_support});
    }

    const exact::ExactLevel& source_level =
        first.source_facet_miniball->squared_radius;
    const exact::ExactLevel& incidence_level =
        *first.first_incidence_squared_level;
    const std::size_t projection_offset = projections.size();
    for (std::size_t source_index = begin;
         source_index < end;
         ++source_index) {
      const DeletionScratchRecord& deletion = scratch[source_index];
      if (incidence_level > deletion.saddle_squared_level) {
        result.logical_storage_entry_count = logical_storage_entry_count;
        return fail(std::move(result), BuildFailure::level_contradiction);
      }
      if ((deletion.source ==
               ExactDirectSparseGatewayDeletionSource::strict_arm_seed &&
           !(source_level < deletion.saddle_squared_level)) ||
          (deletion.source == ExactDirectSparseGatewayDeletionSource::
                                  equal_level_facet_seed &&
           source_level != deletion.saddle_squared_level)) {
        result.logical_storage_entry_count = logical_storage_entry_count;
        return fail(
            std::move(result), BuildFailure::source_join_inconsistent);
      }
      const bool equal = incidence_level == deletion.saddle_squared_level;
      const auto matching_minimizer = std::lower_bound(
          first.cominimizers.begin(),
          first.cominimizers.end(),
          deletion.removed_point_id,
          gateway_candidate_added_point_less);
      const bool removed_point_is_cominimizer =
          matching_minimizer != first.cominimizers.end() &&
          matching_minimizer->added_point_id == deletion.removed_point_id;
      if (removed_point_is_cominimizer != equal) {
        result.logical_storage_entry_count = logical_storage_entry_count;
        return fail(std::move(result), BuildFailure::level_contradiction);
      }
      projections.push_back(ExactDirectSparseGatewayDeletionProjection{
          projections.size(),
          deletion.source_family_index,
          deletion.source,
          deletion.source_deletion_index,
          deletion.source_event_index,
          deletion.source_order,
          deletion.removed_point_id,
          token_index,
          deletion.saddle_squared_level,
          equal
              ? ExactDirectSparseGatewayLevelRelation::
                    first_incidence_equal_to_saddle
              : ExactDirectSparseGatewayLevelRelation::
                    first_incidence_strictly_below_saddle,
          removed_point_is_cominimizer});
    }
    tokens.push_back(ExactDirectSparseGatewayFacetToken{
        token_index,
        scratch[begin].facet_key,
        source_level,
        incidence_level,
        first.audit,
        projection_offset,
        end - begin,
        candidate_offset,
        first.cominimizers.size(),
        0U});
    ++token_index;
    begin = end;
  }
  result.required_gateway_candidate_count = candidates.size();
  result.one_first_incidence_call_per_distinct_facet =
      tokens.size() == distinct_facet_count;
  result.every_first_incidence_complete = true;
  result.every_first_incidence_at_or_below_each_saddle = true;
  result.strict_and_equal_level_relations_classified = true;

  std::vector<std::size_t> batch_token_indices(tokens.size());
  std::iota(
      batch_token_indices.begin(), batch_token_indices.end(), 0U);
  std::sort(
      batch_token_indices.begin(),
      batch_token_indices.end(),
      [&tokens](std::size_t left_index, std::size_t right_index) {
        const ExactDirectSparseGatewayFacetToken& left = tokens[left_index];
        const ExactDirectSparseGatewayFacetToken& right = tokens[right_index];
        if (left.source_facet_key.point_count !=
            right.source_facet_key.point_count) {
          return left.source_facet_key.point_count <
                 right.source_facet_key.point_count;
        }
        if (left.first_incidence_squared_level !=
            right.first_incidence_squared_level) {
          return left.first_incidence_squared_level <
                 right.first_incidence_squared_level;
        }
        return facet_key_less(
            left.source_facet_key, right.source_facet_key);
      });
  std::vector<ExactDirectSparseGatewayCandidateBatch> batches;
  batches.reserve(std::min(
      tokens.size(), budget.maximum_batch_count));
  for (std::size_t begin = 0U; begin < batch_token_indices.size();) {
    std::size_t end = begin + 1U;
    const ExactDirectSparseGatewayFacetToken& first =
        tokens[batch_token_indices[begin]];
    while (end < batch_token_indices.size()) {
      const ExactDirectSparseGatewayFacetToken& next =
          tokens[batch_token_indices[end]];
      if (next.source_facet_key.point_count !=
              first.source_facet_key.point_count ||
          next.first_incidence_squared_level !=
              first.first_incidence_squared_level) {
        break;
      }
      ++end;
    }
    if (batches.size() >= budget.maximum_batch_count) {
      result.required_batch_count = batches.size() + 1U;
      result.logical_storage_entry_count = logical_storage_entry_count;
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }
    if (!add_logical_entries(
            1U,
            budget,
            logical_storage_entry_count,
            storage_failure)) {
      result.logical_storage_entry_count = logical_storage_entry_count;
      return fail(std::move(result), storage_failure);
    }
    const std::size_t batch_index = batches.size();
    batches.push_back(ExactDirectSparseGatewayCandidateBatch{
        batch_index,
        first.source_facet_key.point_count,
        first.first_incidence_squared_level,
        begin,
        end - begin});
    for (std::size_t index_in_batch = begin;
         index_in_batch < end;
         ++index_in_batch) {
      tokens[batch_token_indices[index_in_batch]].batch_index = batch_index;
    }
    begin = end;
  }
  result.required_batch_count = batches.size();
  result.logical_storage_entry_count = logical_storage_entry_count;
  result.logical_storage_within_budget =
      logical_storage_entry_count <=
      budget.maximum_logical_storage_entry_count;
  result.all_positive_support_candidates_retained_atomically = true;
  result.batches_canonical_and_partition_tokens = true;
  result.output_linear_in_references_key_points_and_candidates = true;

  result.deletion_projections = std::move(projections);
  result.facet_tokens = std::move(tokens);
  result.gateway_candidates = std::move(candidates);
  result.batches = std::move(batches);
  result.batch_facet_token_indices = std::move(batch_token_indices);
  result.decision = ExactDirectSparseGatewayCandidateDecision::
      complete_certified_sparse_gateway_candidates;
  if (!result.certified_partial_refinement()) {
    throw std::logic_error(
        "a complete sparse gateway-candidate journal failed its contract");
  }
  return result;
}

ExactDirectSparseFacetKey
reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet(
    const ExactDirectSparseGatewayCandidateJournalResult& journal,
    std::size_t gateway_candidate_index,
    std::size_t removed_union_point_index) {
  if (!journal.certified_partial_refinement()) {
    throw std::invalid_argument(
        "a gateway-candidate deletion requires a certified journal");
  }
  if (gateway_candidate_index >= journal.gateway_candidates.size()) {
    throw std::out_of_range(
        "a gateway candidate index is outside the journal");
  }
  const ExactDirectSparseGatewayCandidateRecord& candidate =
      journal.gateway_candidates[gateway_candidate_index];
  if (candidate.gateway_candidate_index != gateway_candidate_index ||
      candidate.facet_token_index >= journal.facet_tokens.size()) {
    throw std::invalid_argument(
        "a gateway candidate has inconsistent indexing");
  }
  const ExactDirectSparseFacetKey& source =
      journal.facet_tokens[candidate.facet_token_index].source_facet_key;
  const std::size_t union_point_count = source.point_count + 1U;
  if (removed_union_point_index >= union_point_count ||
      std::binary_search(
          source.point_ids.begin(),
          source.point_ids.begin() +
              static_cast<std::ptrdiff_t>(source.point_count),
          candidate.added_point_id)) {
    throw std::invalid_argument(
        "a gateway candidate deletion has an invalid union identity");
  }

  std::array<PointId, 11U> union_point_ids{};
  auto output = union_point_ids.begin();
  bool added = false;
  for (std::size_t index = 0U; index < source.point_count; ++index) {
    while (!added && candidate.added_point_id < source.point_ids[index]) {
      *output = candidate.added_point_id;
      ++output;
      added = true;
    }
    *output = source.point_ids[index];
    ++output;
  }
  if (!added) {
    *output = candidate.added_point_id;
    ++output;
  }
  if (output != union_point_ids.begin() +
                    static_cast<std::ptrdiff_t>(union_point_count)) {
    throw std::logic_error(
        "a gateway candidate union has the wrong cardinality");
  }

  ExactDirectSparseFacetKey deletion;
  deletion.point_count = source.point_count;
  std::size_t write_index = 0U;
  for (std::size_t read_index = 0U;
       read_index < union_point_count;
       ++read_index) {
    if (read_index != removed_union_point_index) {
      deletion.point_ids[write_index] = union_point_ids[read_index];
      ++write_index;
    }
  }
  if (write_index != deletion.point_count) {
    throw std::logic_error(
        "a gateway candidate deletion has the wrong cardinality");
  }
  return deletion;
}

ExactDirectSparseGatewayCandidateVerification
verify_exact_direct_sparse_gateway_candidate_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& trusted_budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& observed) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the sparse gateway-candidate verifier has a foreign LBVH");
  }
  require_valid_traversal_order(traversal_order);

  ExactDirectSparseGatewayCandidateVerification verification;
  verification.observed_storage_within_budget =
      observed_scientific_storage_within_budget(observed, trusted_budget);
  if (!verification.observed_storage_within_budget) {
    return verification;
  }

  const auto source_verification =
      verify_exact_direct_closed_saddle_incidence_journal_streaming(
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal,
          source_incidence_budget,
          source_incidence_journal);
  verification.source_incidence_journal_freshly_replayed =
      source_verification_closes(source_verification) &&
      source_incidence_journal.certified_partial_refinement();
  if (!verification.source_incidence_journal_freshly_replayed) {
    return verification;
  }

  const ExactDirectSparseGatewayCandidateJournalResult expected =
      build_exact_direct_sparse_gateway_candidate_journal(
          index,
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal,
          source_incidence_budget,
          source_incidence_journal,
          trusted_budget,
          traversal_order);
  verification.deletion_projections_freshly_replayed =
      observed.deletion_projections == expected.deletion_projections;
  verification.facet_tokens_freshly_replayed =
      observed.facet_tokens == expected.facet_tokens;
  verification.gateway_candidates_freshly_replayed =
      observed.gateway_candidates == expected.gateway_candidates;
  verification.batches_freshly_replayed =
      observed.batches == expected.batches &&
      observed.batch_facet_token_indices ==
          expected.batch_facet_token_indices;
  verification.counters_and_result_facts_freshly_replayed =
      source_result_facts_match(expected, observed);
  verification.no_forbidden_global_structure_materialized =
      non_scope_is_honest(observed) && non_scope_is_honest(expected);
  verification.fresh_replay_certified =
      verification.observed_storage_within_budget &&
      verification.source_incidence_journal_freshly_replayed &&
      verification.deletion_projections_freshly_replayed &&
      verification.facet_tokens_freshly_replayed &&
      verification.gateway_candidates_freshly_replayed &&
      verification.batches_freshly_replayed &&
      verification.counters_and_result_facts_freshly_replayed &&
      verification.no_forbidden_global_structure_materialized;
  verification.result_certified = verification.fresh_replay_certified;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
