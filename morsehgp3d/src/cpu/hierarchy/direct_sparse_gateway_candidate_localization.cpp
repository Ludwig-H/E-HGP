#include "morsehgp3d/hierarchy/direct_sparse_gateway_candidate_localization.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

struct DeletionScratchRecord {
  ExactDirectSparseFacetKey facet_key{};
  std::size_t gateway_candidate_index{};
  std::size_t source_batch_index{};
  std::size_t removed_union_point_index{};
  std::size_t localized_facet_token_index{};
};

enum class BuildFailure : std::uint8_t {
  capacity_overflow,
  budget_exhausted,
  source_not_certified,
  locator_probe_budget_exhausted,
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

[[nodiscard]] bool checked_accumulate(
    std::size_t increment,
    std::size_t& counter) noexcept {
  std::size_t sum = 0U;
  if (!try_add_size(counter, increment, sum)) {
    return false;
  }
  counter = sum;
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
      "a sparse gateway localization traversal order is invalid");
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

[[nodiscard]] bool canonical_facet_key(
    const ExactDirectSparseFacetKey& key) noexcept {
  if (key.point_count == 0U ||
      key.point_count > direct_sparse_positive_facet_maximum_point_count) {
    return false;
  }
  for (std::size_t point_index = 0U;
       point_index < key.point_count;
       ++point_index) {
    if (point_index != 0U &&
        key.point_ids[point_index - 1U] >= key.point_ids[point_index]) {
      return false;
    }
  }
  for (std::size_t point_index = key.point_count;
       point_index < key.point_ids.size();
       ++point_index) {
    if (key.point_ids[point_index] != 0U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool non_scope_is_honest(
    const ExactDirectSparseGatewayCandidateLocalizationResult& result)
    noexcept {
  return !result.locator_state_mutated &&
         !result.locator_batch_committed &&
         !result.external_binding_authority_replayed &&
         !result.locator_snapshot_batch_level_alignment_claimed &&
         !result.missing_facet_means_isolated &&
         !result.singleton_component_created &&
         !result.root_union_or_forest_mutated &&
         !result.gateway_attach_published &&
         !result.eleven_point_coface_keys_materialized &&
         !result.gamma_cells_or_higher_order_delaunay_materialized &&
         !result.forbidden_global_structure_materialized &&
         !result.public_status_claimed && result.partial_refinement_only &&
         result.scope ==
             ExactDirectSparseGatewayCandidateLocalizationScope::
                 candidate_deletion_facets_relative_to_frozen_positive_locator_only;
}

void initialize_scope(
    ExactDirectSparseGatewayCandidateLocalizationResult& result) noexcept {
  result.scope = ExactDirectSparseGatewayCandidateLocalizationScope::
      candidate_deletion_facets_relative_to_frozen_positive_locator_only;
  result.no_partial_scientific_payload_published = true;
  result.locator_state_mutated = false;
  result.locator_batch_committed = false;
  result.external_binding_authority_replayed = false;
  result.locator_snapshot_batch_level_alignment_claimed = false;
  result.missing_facet_means_isolated = false;
  result.singleton_component_created = false;
  result.root_union_or_forest_mutated = false;
  result.gateway_attach_published = false;
  result.eleven_point_coface_keys_materialized = false;
  result.gamma_cells_or_higher_order_delaunay_materialized = false;
  result.forbidden_global_structure_materialized = false;
  result.public_status_claimed = false;
  result.partial_refinement_only = true;
}

void clear_scientific_payload(
    ExactDirectSparseGatewayCandidateLocalizationResult& result) noexcept {
  result.deletion_projections.clear();
  result.localized_facet_tokens.clear();
  result.logical_storage_entry_count = 0U;
  result.no_partial_scientific_payload_published = true;
}

[[nodiscard]] ExactDirectSparseGatewayCandidateLocalizationDecision
decision_for(BuildFailure failure) noexcept {
  switch (failure) {
    case BuildFailure::capacity_overflow:
      return ExactDirectSparseGatewayCandidateLocalizationDecision::
          no_localization_capacity_overflow;
    case BuildFailure::budget_exhausted:
      return ExactDirectSparseGatewayCandidateLocalizationDecision::
          no_localization_budget_exhausted;
    case BuildFailure::source_not_certified:
      return ExactDirectSparseGatewayCandidateLocalizationDecision::
          no_localization_source_not_certified;
    case BuildFailure::locator_probe_budget_exhausted:
      return ExactDirectSparseGatewayCandidateLocalizationDecision::
          no_localization_locator_probe_budget_exhausted;
  }
  return ExactDirectSparseGatewayCandidateLocalizationDecision::not_certified;
}

void check_locator_snapshot(
    const ExactDirectSparsePositiveFacetLocator& locator,
    ExactDirectSparseGatewayCandidateLocalizationResult& result) {
  if (!checked_accumulate(
          1U, result.counters.locator_snapshot_check_count)) {
    throw std::overflow_error(
        "a sparse gateway localization snapshot counter overflowed");
  }
  if (locator.snapshot_stamp() != result.locator_snapshot_stamp) {
    clear_scientific_payload(result);
    throw std::runtime_error(
        "the positive-facet locator changed during gateway localization");
  }
}

[[nodiscard]] ExactDirectSparseGatewayCandidateLocalizationResult fail(
    ExactDirectSparseGatewayCandidateLocalizationResult result,
    BuildFailure failure,
    const ExactDirectSparsePositiveFacetLocator& locator) {
  clear_scientific_payload(result);
  result.decision = decision_for(failure);
  check_locator_snapshot(locator, result);
  result.common_frozen_locator_snapshot_certified = true;
  return result;
}

[[nodiscard]] bool observed_scientific_storage_within_budget(
    const ExactDirectSparseGatewayCandidateLocalizationResult& observed,
    const ExactDirectSparseGatewayCandidateLocalizationBudget& budget)
    noexcept {
  if (observed.deletion_projections.size() >
          budget.maximum_deletion_reference_count ||
      observed.localized_facet_tokens.size() >
          budget.maximum_distinct_facet_count) {
    return false;
  }
  std::size_t key_point_count = 0U;
  for (const ExactDirectSparseGatewayLocalizedFacetToken& token :
       observed.localized_facet_tokens) {
    if (token.facet_key.point_count >
            direct_sparse_positive_facet_maximum_point_count ||
        !try_add_size(
            key_point_count,
            token.facet_key.point_count,
            key_point_count)) {
      return false;
    }
  }
  std::size_t logical_storage_entry_count = 0U;
  if (key_point_count > budget.maximum_facet_key_point_count ||
      !try_add_size(
          observed.deletion_projections.size(),
          observed.localized_facet_tokens.size(),
          logical_storage_entry_count) ||
      !try_add_size(
          logical_storage_entry_count,
          key_point_count,
          logical_storage_entry_count)) {
    return false;
  }
  return logical_storage_entry_count ==
             observed.logical_storage_entry_count &&
         logical_storage_entry_count <=
             budget.maximum_logical_storage_entry_count;
}

[[nodiscard]] bool complete_payload_shape(
    const ExactDirectSparseGatewayCandidateLocalizationResult& result)
    noexcept {
  std::size_t expected_snapshot_check_count = 0U;
  if (!try_add_size(
          result.required_locator_probe_count,
          2U,
          expected_snapshot_check_count)) {
    return false;
  }
  std::size_t localized_facet_count = 0U;
  if (!try_add_size(
          result.counters.relative_positive_facet_count,
          result.counters.latent_unresolved_facet_count,
          localized_facet_count)) {
    return false;
  }
  if (result.deletion_projections.size() !=
          result.required_deletion_reference_count ||
      result.localized_facet_tokens.size() !=
          result.required_distinct_facet_count ||
      result.required_locator_probe_count !=
          result.required_distinct_facet_count ||
      result.counters.source_candidate_scan_count !=
          result.required_source_candidate_scan_count ||
      result.counters.deletion_reference_count !=
          result.required_deletion_reference_count ||
      result.counters.distinct_facet_count !=
          result.required_distinct_facet_count ||
      result.counters.facet_key_point_count !=
          result.required_facet_key_point_count ||
      result.counters.locator_probe_count !=
          result.required_locator_probe_count ||
      localized_facet_count != result.required_distinct_facet_count ||
      result.counters.locator_snapshot_check_count !=
          expected_snapshot_check_count) {
    return false;
  }

  for (std::size_t projection_index = 0U;
       projection_index < result.deletion_projections.size();
       ++projection_index) {
    const auto& projection = result.deletion_projections[projection_index];
    bool canonical_candidate_partition = false;
    if (projection_index == 0U) {
      canonical_candidate_partition =
          projection.gateway_candidate_index == 0U &&
          projection.removed_union_point_index == 0U;
    } else {
      const auto& previous =
          result.deletion_projections[projection_index - 1U];
      if (projection.gateway_candidate_index ==
          previous.gateway_candidate_index) {
        canonical_candidate_partition =
            projection.source_batch_index == previous.source_batch_index &&
            previous.removed_union_point_index < 10U &&
            projection.removed_union_point_index ==
                previous.removed_union_point_index + 1U;
      } else {
        canonical_candidate_partition =
            previous.gateway_candidate_index <
                std::numeric_limits<std::size_t>::max() &&
            projection.gateway_candidate_index ==
                previous.gateway_candidate_index + 1U &&
            projection.removed_union_point_index == 0U;
      }
    }
    if (projection.deletion_projection_index != projection_index ||
        projection.gateway_candidate_index >=
            result.source_gateway_candidate_count ||
        projection.removed_union_point_index > 10U ||
        projection.localized_facet_token_index >=
            result.localized_facet_tokens.size() ||
        !canonical_candidate_partition) {
      return false;
    }
  }
  if (result.deletion_projections.empty() !=
          (result.source_gateway_candidate_count == 0U) ||
      (!result.deletion_projections.empty() &&
       result.deletion_projections.back().gateway_candidate_index + 1U !=
           result.source_gateway_candidate_count)) {
    return false;
  }

  std::size_t key_point_count = 0U;
  std::size_t positive_count = 0U;
  std::size_t latent_count = 0U;
  for (std::size_t token_index = 0U;
       token_index < result.localized_facet_tokens.size();
       ++token_index) {
    const auto& token = result.localized_facet_tokens[token_index];
    if (token.localized_facet_token_index != token_index ||
        !canonical_facet_key(token.facet_key) ||
        (token_index != 0U &&
         !facet_key_less(
             result.localized_facet_tokens[token_index - 1U].facet_key,
             token.facet_key)) ||
        !try_add_size(
            key_point_count,
            token.facet_key.point_count,
            key_point_count)) {
      return false;
    }
    switch (token.disposition) {
      case ExactDirectSparseGatewayFacetLocalizationDisposition::
          relative_positive:
        if (!token.component_handle_present ||
            !token.source_binding_witness_present ||
            token.source_binding_witness.external_authority_id == 0U ||
            token.source_binding_witness.replay_token == 0U) {
          return false;
        }
        if (!checked_accumulate(1U, positive_count)) {
          return false;
        }
        break;
      case ExactDirectSparseGatewayFacetLocalizationDisposition::
          latent_unresolved:
        if (token.component_handle_present ||
            token.source_binding_witness_present ||
            token.component_handle != ExactDirectSparseComponentHandle{} ||
            token.source_binding_witness !=
                ExactDirectSparseFacetWitness{}) {
          return false;
        }
        if (!checked_accumulate(1U, latent_count)) {
          return false;
        }
        break;
      case ExactDirectSparseGatewayFacetLocalizationDisposition::
          not_certified:
        return false;
    }
  }
  std::size_t logical_storage_entry_count = 0U;
  return key_point_count == result.required_facet_key_point_count &&
         positive_count == result.counters.relative_positive_facet_count &&
         latent_count == result.counters.latent_unresolved_facet_count &&
         try_add_size(
             result.deletion_projections.size(),
             result.localized_facet_tokens.size(),
             logical_storage_entry_count) &&
         try_add_size(
             logical_storage_entry_count,
             key_point_count,
             logical_storage_entry_count) &&
         logical_storage_entry_count == result.logical_storage_entry_count;
}

[[nodiscard]] bool add_probe_counters(
    const ExactDirectSparsePositiveFacetProbeResult& probe,
    ExactDirectSparseGatewayCandidateLocalizationCounters& counters)
    noexcept {
  auto updated = counters;
  if (!(checked_accumulate(1U, updated.locator_probe_count) &&
         checked_accumulate(
             probe.slot_visit_count, updated.slot_visit_count) &&
         checked_accumulate(
             probe.component_parent_hop_count,
             updated.component_parent_hop_count) &&
         checked_accumulate(
             probe.full_key_comparison_count,
             updated.full_key_comparison_count) &&
         checked_accumulate(
             probe.equal_fingerprint_distinct_key_count,
             updated.equal_fingerprint_distinct_key_count))) {
    return false;
  }
  counters = updated;
  return true;
}

}  // namespace

bool ExactDirectSparseGatewayCandidateLocalizationResult::
    certified_partial_refinement() const noexcept {
  std::size_t maximum_deletion_reference_count = 0U;
  if (!try_multiply_size(
          11U,
          required_source_candidate_scan_count,
          maximum_deletion_reference_count)) {
    return false;
  }
  return schema_version ==
             direct_sparse_gateway_candidate_localization_schema_version &&
         decision == ExactDirectSparseGatewayCandidateLocalizationDecision::
                         complete_certified_relative_gateway_candidate_localizations &&
         source_gateway_candidate_journal_freshly_replayed &&
         budget_preflight_certified &&
         every_candidate_deletion_reconstructed &&
         projections_canonical_and_partition_candidates &&
         distinct_full_keys_globally_deduplicated &&
         one_locator_probe_per_distinct_full_key &&
         every_locator_probe_complete &&
         relative_positive_and_latent_outcomes_separated &&
         every_positive_token_has_existing_handle_and_binding_witness &&
         every_latent_token_has_no_positive_payload &&
         common_frozen_locator_snapshot_certified &&
         logical_storage_within_budget &&
         no_partial_scientific_payload_published &&
         non_scope_is_honest(*this) && complete_payload_shape(*this) &&
         locator_query_witness.external_authority_id != 0U &&
         locator_query_witness.replay_token != 0U &&
         locator_snapshot_stamp.schema_version ==
             direct_sparse_positive_facet_locator_schema_version &&
         locator_snapshot_stamp.external_authority_id ==
             locator_query_witness.external_authority_id &&
         required_source_candidate_scan_count ==
             source_gateway_candidate_count &&
         required_source_candidate_scan_count <=
             requested_budget.maximum_source_candidate_scan_count &&
         required_deletion_reference_count <=
             requested_budget.maximum_deletion_reference_count &&
         required_deletion_reference_count <=
             maximum_deletion_reference_count &&
         required_distinct_facet_count <=
             requested_budget.maximum_distinct_facet_count &&
         required_distinct_facet_count <=
             required_deletion_reference_count &&
         required_facet_key_point_count <=
             requested_budget.maximum_facet_key_point_count &&
         counters.slot_visit_count <=
             requested_budget.maximum_aggregate_slot_visit_count &&
         counters.component_parent_hop_count <=
             requested_budget
                 .maximum_aggregate_component_parent_hop_count &&
         logical_storage_entry_count <=
             requested_budget.maximum_logical_storage_entry_count;
}

bool ExactDirectSparseGatewayCandidateLocalizationResult::
    certified_atomic_failure() const noexcept {
  const bool recognized_failure =
      decision == ExactDirectSparseGatewayCandidateLocalizationDecision::
                      no_localization_capacity_overflow ||
      decision == ExactDirectSparseGatewayCandidateLocalizationDecision::
                      no_localization_budget_exhausted ||
      decision == ExactDirectSparseGatewayCandidateLocalizationDecision::
                      no_localization_source_not_certified ||
      decision == ExactDirectSparseGatewayCandidateLocalizationDecision::
                      no_localization_locator_probe_budget_exhausted;
  return schema_version ==
             direct_sparse_gateway_candidate_localization_schema_version &&
         recognized_failure && deletion_projections.empty() &&
         localized_facet_tokens.empty() &&
         logical_storage_entry_count == 0U &&
         no_partial_scientific_payload_published &&
         common_frozen_locator_snapshot_certified &&
         locator_query_witness.external_authority_id != 0U &&
         locator_query_witness.replay_token != 0U &&
         locator_snapshot_stamp.schema_version ==
             direct_sparse_positive_facet_locator_schema_version &&
         locator_snapshot_stamp.external_authority_id ==
             locator_query_witness.external_authority_id &&
         counters.locator_snapshot_check_count >= 2U &&
         (decision !=
                  ExactDirectSparseGatewayCandidateLocalizationDecision::
                      no_localization_source_not_certified ||
          (!source_gateway_candidate_journal_freshly_replayed &&
           counters.locator_probe_count == 0U)) &&
         (decision !=
                  ExactDirectSparseGatewayCandidateLocalizationDecision::
                      no_localization_locator_probe_budget_exhausted ||
          (source_gateway_candidate_journal_freshly_replayed &&
           budget_preflight_certified)) &&
         (source_gateway_candidate_journal_freshly_replayed ||
          (decision ==
               ExactDirectSparseGatewayCandidateLocalizationDecision::
                   no_localization_source_not_certified) ||
          (decision ==
               ExactDirectSparseGatewayCandidateLocalizationDecision::
                   no_localization_budget_exhausted &&
           source_gateway_candidate_count >
               requested_budget.maximum_source_candidate_scan_count)) &&
         non_scope_is_honest(*this);
}

bool ExactDirectSparseGatewayCandidateLocalizationResult::certified_outcome()
    const noexcept {
  return certified_partial_refinement() || certified_atomic_failure();
}

ExactDirectSparseGatewayCandidateLocalizationResult
build_exact_direct_sparse_gateway_candidate_localization(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    const ExactDirectSparseGatewayCandidateJournalResult&
        source_gateway_journal,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayCandidateLocalizationBudget& budget,
    spatial::LbvhTraversalOrder traversal_order) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the sparse gateway localization LBVH has a different authority");
  }
  require_valid_traversal_order(traversal_order);
  if (!locator.certified_positive_locator()) {
    throw std::invalid_argument(
        "a sparse gateway localization requires a certified locator");
  }
  if (locator_query_witness.external_authority_id == 0U ||
      locator_query_witness.external_authority_id !=
          locator.config().external_authority_id ||
      locator_query_witness.replay_token == 0U) {
    throw std::invalid_argument(
        "a sparse gateway localization requires one matching locator witness");
  }

  ExactDirectSparseGatewayCandidateLocalizationResult result;
  result.requested_budget = budget;
  result.traversal_order = traversal_order;
  result.point_count = cloud.size();
  result.source_gateway_candidate_count =
      source_gateway_journal.gateway_candidates.size();
  result.required_source_candidate_scan_count =
      result.source_gateway_candidate_count;
  result.locator_query_witness = locator_query_witness;
  result.locator_snapshot_stamp = locator.snapshot_stamp();
  result.counters.locator_snapshot_check_count = 1U;
  initialize_scope(result);

  if (result.source_gateway_candidate_count >
      budget.maximum_source_candidate_scan_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted, locator);
  }

  const auto source_verification =
      verify_exact_direct_sparse_gateway_candidate_journal(
          index,
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal,
          source_incidence_budget,
          source_incidence_journal,
          source_gateway_budget,
          traversal_order,
          source_gateway_journal);
  if (!source_verification.result_certified ||
      !source_gateway_journal.certified_partial_refinement()) {
    return fail(
        std::move(result), BuildFailure::source_not_certified, locator);
  }
  result.source_gateway_candidate_journal_freshly_replayed = true;

  std::size_t deletion_reference_count = 0U;
  for (std::size_t candidate_index = 0U;
       candidate_index < source_gateway_journal.gateway_candidates.size();
       ++candidate_index) {
    const auto& candidate =
        source_gateway_journal.gateway_candidates[candidate_index];
    if (candidate.gateway_candidate_index != candidate_index ||
        candidate.facet_token_index >=
            source_gateway_journal.facet_tokens.size()) {
      return fail(
          std::move(result), BuildFailure::source_not_certified, locator);
    }
    const std::size_t deletion_count =
        source_gateway_journal.facet_tokens[candidate.facet_token_index]
            .source_facet_key.point_count +
        1U;
    if (!try_add_size(
            deletion_reference_count,
            deletion_count,
            deletion_reference_count)) {
      return fail(
          std::move(result), BuildFailure::capacity_overflow, locator);
    }
    if (!checked_accumulate(
            1U, result.counters.source_candidate_scan_count)) {
      return fail(
          std::move(result), BuildFailure::capacity_overflow, locator);
    }
  }
  result.required_deletion_reference_count = deletion_reference_count;
  result.counters.deletion_reference_count = deletion_reference_count;
  if (deletion_reference_count >
      budget.maximum_deletion_reference_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted, locator);
  }

  std::vector<DeletionScratchRecord> deletions;
  deletions.reserve(deletion_reference_count);
  for (std::size_t candidate_index = 0U;
       candidate_index < source_gateway_journal.gateway_candidates.size();
       ++candidate_index) {
    const auto& candidate =
        source_gateway_journal.gateway_candidates[candidate_index];
    const auto& source_token =
        source_gateway_journal.facet_tokens[candidate.facet_token_index];
    const std::size_t deletion_count =
        source_token.source_facet_key.point_count + 1U;
    for (std::size_t removed_index = 0U;
         removed_index < deletion_count;
         ++removed_index) {
      deletions.push_back(
          {reconstruct_exact_direct_sparse_gateway_candidate_deletion_facet(
               source_gateway_journal,
               candidate_index,
               removed_index),
           candidate_index,
           source_token.batch_index,
           removed_index,
           0U});
    }
  }
  if (deletions.size() != deletion_reference_count) {
    throw std::logic_error(
        "sparse gateway localization reconstructed the wrong deletion count");
  }
  result.every_candidate_deletion_reconstructed = true;

  std::vector<std::size_t> deletion_order(deletions.size());
  std::iota(deletion_order.begin(), deletion_order.end(), std::size_t{0U});
  std::sort(
      deletion_order.begin(),
      deletion_order.end(),
      [&deletions](std::size_t left_index, std::size_t right_index) {
        const auto& left = deletions[left_index];
        const auto& right = deletions[right_index];
        if (facet_key_less(left.facet_key, right.facet_key)) {
          return true;
        }
        if (facet_key_less(right.facet_key, left.facet_key)) {
          return false;
        }
        if (left.gateway_candidate_index !=
            right.gateway_candidate_index) {
          return left.gateway_candidate_index <
                 right.gateway_candidate_index;
        }
        return left.removed_union_point_index <
               right.removed_union_point_index;
      });

  std::size_t distinct_facet_count = 0U;
  std::size_t facet_key_point_count = 0U;
  const ExactDirectSparseFacetKey* previous_key = nullptr;
  for (const std::size_t deletion_index : deletion_order) {
    const auto& key = deletions[deletion_index].facet_key;
    if (previous_key == nullptr || *previous_key != key) {
      if (!checked_accumulate(1U, distinct_facet_count)) {
        return fail(
            std::move(result), BuildFailure::capacity_overflow, locator);
      }
      if (!try_add_size(
              facet_key_point_count,
              key.point_count,
              facet_key_point_count)) {
        return fail(
            std::move(result), BuildFailure::capacity_overflow, locator);
      }
      previous_key = &key;
    }
  }
  result.required_distinct_facet_count = distinct_facet_count;
  result.required_facet_key_point_count = facet_key_point_count;
  result.required_locator_probe_count = distinct_facet_count;
  result.counters.distinct_facet_count = distinct_facet_count;
  result.counters.facet_key_point_count = facet_key_point_count;
  if (distinct_facet_count > budget.maximum_distinct_facet_count ||
      facet_key_point_count > budget.maximum_facet_key_point_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted, locator);
  }

  std::size_t logical_storage_entry_count = 0U;
  if (!try_add_size(
          deletion_reference_count,
          distinct_facet_count,
          logical_storage_entry_count) ||
      !try_add_size(
          logical_storage_entry_count,
          facet_key_point_count,
          logical_storage_entry_count)) {
    return fail(
        std::move(result), BuildFailure::capacity_overflow, locator);
  }
  if (logical_storage_entry_count >
      budget.maximum_logical_storage_entry_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted, locator);
  }
  result.budget_preflight_certified = true;

  std::vector<ExactDirectSparseGatewayLocalizedFacetToken> localized_tokens;
  localized_tokens.reserve(distinct_facet_count);
  previous_key = nullptr;
  std::size_t current_token_index = 0U;
  for (const std::size_t deletion_index : deletion_order) {
    auto& deletion = deletions[deletion_index];
    if (previous_key == nullptr || *previous_key != deletion.facet_key) {
      current_token_index = localized_tokens.size();
      localized_tokens.push_back(
          {current_token_index, deletion.facet_key});
      previous_key = &deletion.facet_key;
    }
    deletion.localized_facet_token_index = current_token_index;
  }
  result.distinct_full_keys_globally_deduplicated = true;

  std::vector<ExactDirectSparseGatewayCandidateDeletionProjection>
      projections;
  projections.reserve(deletions.size());
  for (std::size_t deletion_index = 0U;
       deletion_index < deletions.size();
       ++deletion_index) {
    const auto& deletion = deletions[deletion_index];
    projections.push_back(
        {deletion_index,
         deletion.gateway_candidate_index,
         deletion.source_batch_index,
         deletion.removed_union_point_index,
         deletion.localized_facet_token_index});
  }
  result.projections_canonical_and_partition_candidates = true;

  for (auto& token : localized_tokens) {
    if (result.counters.slot_visit_count >
            budget.maximum_aggregate_slot_visit_count ||
        result.counters.component_parent_hop_count >
            budget.maximum_aggregate_component_parent_hop_count) {
      return fail(
          std::move(result), BuildFailure::capacity_overflow, locator);
    }
    const ExactDirectSparsePositiveFacetProbeBudget effective_probe_budget{
        std::min(
            budget.facet_probe_budget.maximum_slot_visit_count,
            budget.maximum_aggregate_slot_visit_count -
                result.counters.slot_visit_count),
        std::min(
            budget.facet_probe_budget.maximum_component_parent_hop_count,
            budget.maximum_aggregate_component_parent_hop_count -
                result.counters.component_parent_hop_count)};
    const auto probe = locator.probe_positive_facet(
        token.facet_key,
        locator_query_witness,
        effective_probe_budget);
    if (!add_probe_counters(probe, result.counters)) {
      return fail(
          std::move(result), BuildFailure::capacity_overflow, locator);
    }
    if (result.counters.slot_visit_count >
            budget.maximum_aggregate_slot_visit_count ||
        result.counters.component_parent_hop_count >
            budget.maximum_aggregate_component_parent_hop_count) {
      return fail(
          std::move(result), BuildFailure::budget_exhausted, locator);
    }

    switch (probe.disposition) {
      case ExactDirectSparsePositiveFacetProbeDisposition::positive:
        if (!probe.certified_positive_hit()) {
          throw std::logic_error(
              "a positive gateway localization probe failed its contract");
        }
        token.component_handle = probe.component_handle;
        token.source_binding_witness = probe.source_binding_witness;
        token.component_handle_present = true;
        token.source_binding_witness_present = true;
        token.disposition =
            ExactDirectSparseGatewayFacetLocalizationDisposition::
                relative_positive;
        if (!checked_accumulate(
                1U, result.counters.relative_positive_facet_count)) {
          return fail(
              std::move(result), BuildFailure::capacity_overflow, locator);
        }
        break;
      case ExactDirectSparsePositiveFacetProbeDisposition::unresolved:
        if (!probe.certified_unresolved_miss()) {
          throw std::logic_error(
              "an unresolved gateway localization probe failed its contract");
        }
        token.disposition =
            ExactDirectSparseGatewayFacetLocalizationDisposition::
                latent_unresolved;
        if (!checked_accumulate(
                1U, result.counters.latent_unresolved_facet_count)) {
          return fail(
              std::move(result), BuildFailure::capacity_overflow, locator);
        }
        break;
      case ExactDirectSparsePositiveFacetProbeDisposition::budget_exhausted:
        if (!probe.certified_budget_exhaustion()) {
          throw std::logic_error(
              "a budgeted gateway localization probe failed its contract");
        }
        if ((probe.slot_visit_budget_exhausted &&
             effective_probe_budget.maximum_slot_visit_count <
                 budget.facet_probe_budget.maximum_slot_visit_count) ||
            (probe.component_parent_hop_budget_exhausted &&
             effective_probe_budget.maximum_component_parent_hop_count <
                 budget.facet_probe_budget
                     .maximum_component_parent_hop_count)) {
          return fail(
              std::move(result), BuildFailure::budget_exhausted, locator);
        }
        return fail(
            std::move(result),
            BuildFailure::locator_probe_budget_exhausted,
            locator);
      case ExactDirectSparsePositiveFacetProbeDisposition::not_certified:
        throw std::logic_error(
            "a certified locator produced an uncertified gateway probe");
    }
    check_locator_snapshot(locator, result);
  }
  result.one_locator_probe_per_distinct_full_key = true;
  result.every_locator_probe_complete = true;
  result.relative_positive_and_latent_outcomes_separated = true;
  result.every_positive_token_has_existing_handle_and_binding_witness = true;
  result.every_latent_token_has_no_positive_payload = true;

  check_locator_snapshot(locator, result);
  result.common_frozen_locator_snapshot_certified = true;
  result.logical_storage_entry_count = logical_storage_entry_count;
  result.logical_storage_within_budget = true;
  result.deletion_projections = std::move(projections);
  result.localized_facet_tokens = std::move(localized_tokens);
  result.decision = ExactDirectSparseGatewayCandidateLocalizationDecision::
      complete_certified_relative_gateway_candidate_localizations;
  if (!result.certified_partial_refinement()) {
    throw std::logic_error(
        "a complete sparse gateway localization failed its contract");
  }
  return result;
}

ExactDirectSparseGatewayCandidateLocalizationVerification
verify_exact_direct_sparse_gateway_candidate_localization(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    const ExactDirectSparseGatewayCandidateJournalResult&
        source_gateway_journal,
    const ExactDirectSparseFacetWitness& locator_query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayCandidateLocalizationBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseGatewayCandidateLocalizationResult& observed) {
  require_valid_traversal_order(traversal_order);
  ExactDirectSparseGatewayCandidateLocalizationVerification verification;
  verification.trusted_inputs_certified =
      index.validated_for(cloud) && locator.certified_positive_locator() &&
      locator_query_witness.external_authority_id != 0U &&
      locator_query_witness.external_authority_id ==
          locator.config().external_authority_id &&
      locator_query_witness.replay_token != 0U;
  if (!verification.trusted_inputs_certified) {
    return verification;
  }
  verification.observed_storage_within_budget =
      observed_scientific_storage_within_budget(observed, budget);
  if (!verification.observed_storage_within_budget) {
    return verification;
  }
  verification.locator_snapshot_matches_observed_build =
      locator.snapshot_stamp() == observed.locator_snapshot_stamp;
  if (!verification.locator_snapshot_matches_observed_build) {
    return verification;
  }

  const auto expected =
      build_exact_direct_sparse_gateway_candidate_localization(
          index,
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal,
          source_incidence_budget,
          source_incidence_journal,
          source_gateway_budget,
          source_gateway_journal,
          locator_query_witness,
          locator,
          budget,
          traversal_order);
  verification.source_gateway_candidate_journal_freshly_replayed =
      expected.source_gateway_candidate_journal_freshly_replayed;
  const bool source_replay_bypassed_by_trusted_preflight =
      expected.decision ==
          ExactDirectSparseGatewayCandidateLocalizationDecision::
              no_localization_budget_exhausted &&
      source_gateway_journal.gateway_candidates.size() >
          budget.maximum_source_candidate_scan_count;
  verification.observed_outcome_well_formed = observed.certified_outcome();
  verification.candidate_deletions_freshly_replayed =
      observed.deletion_projections == expected.deletion_projections;
  verification.distinct_full_keys_freshly_replayed =
      observed.localized_facet_tokens.size() ==
          expected.localized_facet_tokens.size() &&
      std::equal(
          observed.localized_facet_tokens.begin(),
          observed.localized_facet_tokens.end(),
          expected.localized_facet_tokens.begin(),
          [](const auto& left, const auto& right) {
            return left.localized_facet_token_index ==
                       right.localized_facet_token_index &&
                   left.facet_key == right.facet_key;
          });
  verification.locator_probes_freshly_replayed =
      observed.localized_facet_tokens == expected.localized_facet_tokens &&
      observed.counters.locator_probe_count ==
          expected.counters.locator_probe_count &&
      observed.counters.slot_visit_count ==
          expected.counters.slot_visit_count &&
      observed.counters.component_parent_hop_count ==
          expected.counters.component_parent_hop_count &&
      observed.counters.full_key_comparison_count ==
          expected.counters.full_key_comparison_count &&
      observed.counters.equal_fingerprint_distinct_key_count ==
          expected.counters.equal_fingerprint_distinct_key_count;
  verification.counters_and_result_facts_freshly_replayed =
      observed == expected;
  verification.no_locator_mutation_or_batch_commit =
      !observed.locator_state_mutated &&
      !observed.locator_batch_committed &&
      !expected.locator_state_mutated &&
      !expected.locator_batch_committed;
  verification.external_binding_authority_replayed = false;
  verification.no_isolation_singleton_union_forest_or_attachment_invented =
      !observed.missing_facet_means_isolated &&
      !observed.singleton_component_created &&
      !observed.root_union_or_forest_mutated &&
      !observed.gateway_attach_published &&
      !observed.locator_snapshot_batch_level_alignment_claimed &&
      !expected.missing_facet_means_isolated &&
      !expected.singleton_component_created &&
      !expected.root_union_or_forest_mutated &&
      !expected.gateway_attach_published &&
      !expected.locator_snapshot_batch_level_alignment_claimed;
  verification.no_forbidden_global_structure_materialized =
      non_scope_is_honest(observed) && non_scope_is_honest(expected);
  verification.fresh_replay_certified = observed == expected;
  verification.result_certified =
      verification.trusted_inputs_certified &&
      verification.observed_storage_within_budget &&
      verification.locator_snapshot_matches_observed_build &&
      (verification.source_gateway_candidate_journal_freshly_replayed ||
       source_replay_bypassed_by_trusted_preflight) &&
      verification.observed_outcome_well_formed &&
      verification.candidate_deletions_freshly_replayed &&
      verification.distinct_full_keys_freshly_replayed &&
      verification.locator_probes_freshly_replayed &&
      verification.counters_and_result_facts_freshly_replayed &&
      verification.no_locator_mutation_or_batch_commit &&
      !verification.external_binding_authority_replayed &&
      verification
          .no_isolation_singleton_union_forest_or_attachment_invented &&
      verification.no_forbidden_global_structure_materialized &&
      verification.fresh_replay_certified && expected.certified_outcome();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
