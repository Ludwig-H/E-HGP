#include "morsehgp3d/hierarchy/direct_normalized_h0_source_plan.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <new>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

struct CofaceKey {
  std::array<PointId, 11U> point_ids{};
  std::size_t point_count{};

  friend bool operator==(const CofaceKey&, const CofaceKey&) = default;
};

struct CofaceKeyLess {
  [[nodiscard]] bool operator()(
      const CofaceKey& left,
      const CofaceKey& right) const noexcept {
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
};

struct BatchKey {
  exact::ExactLevel squared_level{};
  std::size_t order{};

  friend bool operator==(const BatchKey&, const BatchKey&) = default;
};

struct BatchKeyLess {
  [[nodiscard]] bool operator()(
      const BatchKey& left,
      const BatchKey& right) const noexcept {
    if (left.squared_level != right.squared_level) {
      return left.squared_level < right.squared_level;
    }
    return left.order < right.order;
  }
};

struct DirectBirthScratch {
  BatchKey batch_key{};
  std::size_t source_role_record_index{};
  std::size_t source_event_projection_index{};
  ExactDirectSparseFacetKey birth_facet_key{};
};

struct CofaceScratch {
  exact::ExactLevel squared_level{};
  std::optional<std::size_t> source_role_record_index;
  std::optional<std::size_t> source_event_projection_index;
  std::optional<std::size_t> source_incidence_family_index;
  std::array<PointId, 4U> support_point_ids{};
  std::size_t support_point_count{};
  std::size_t gateway_candidate_occurrence_count{};
  std::size_t direct_family_match_count{};
  std::size_t published_coface_index{std::numeric_limits<std::size_t>::max()};
};

struct GatewayCandidateScratch {
  std::size_t source_gateway_candidate_index{};
  CofaceScratch* coface{};
};

enum class BuildFailure : std::uint8_t {
  none,
  capacity_overflow,
  allocation_failed,
  budget_exhausted,
  gateway_authority_rejected,
  source_join_inconsistent,
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

[[nodiscard]] bool canonical_facet_key(
    const ExactDirectSparseFacetKey& key,
    std::size_t point_count) noexcept {
  if (key.point_count < 2U || key.point_count > key.point_ids.size() ||
      key.point_count > point_count) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (static_cast<std::size_t>(key.point_ids[index]) >= point_count ||
        (index != 0U && key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  for (std::size_t index = key.point_count; index < key.point_ids.size();
       ++index) {
    if (key.point_ids[index] != 0U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool canonical_support(
    const std::array<PointId, 4U>& support,
    std::size_t support_count,
    std::size_t point_count) noexcept {
  if (support_count < 2U || support_count > support.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < support_count; ++index) {
    if (static_cast<std::size_t>(support[index]) >= point_count ||
        (index != 0U && support[index - 1U] >= support[index])) {
      return false;
    }
  }
  for (std::size_t index = support_count; index < support.size(); ++index) {
    if (support[index] != 0U) {
      return false;
    }
  }
  return true;
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

[[nodiscard]] ExactDirectSparseFacetKey delete_coface_point(
    const CofaceKey& coface,
    std::size_t removed_index) {
  ExactDirectSparseFacetKey facet;
  if (coface.point_count < 3U ||
      coface.point_count > coface.point_ids.size() ||
      coface.point_count - 1U > facet.point_ids.size() ||
      removed_index >= coface.point_count) {
    throw std::invalid_argument(
        "a normalized-H0 coface deletion is invalid");
  }
  facet.point_count = coface.point_count - 1U;
  std::copy_n(
      coface.point_ids.begin(), removed_index, facet.point_ids.begin());
  std::copy(
      coface.point_ids.begin() +
          static_cast<std::ptrdiff_t>(removed_index + 1U),
      coface.point_ids.begin() +
          static_cast<std::ptrdiff_t>(coface.point_count),
      facet.point_ids.begin() + static_cast<std::ptrdiff_t>(removed_index));
  return facet;
}

[[nodiscard]] CofaceKey merge_facet_and_point(
    const ExactDirectSparseFacetKey& facet,
    PointId added_point_id) {
  CofaceKey key;
  if (facet.point_count < 2U || facet.point_count > facet.point_ids.size() ||
      facet.point_count + 1U > key.point_ids.size() ||
      std::binary_search(
          facet.point_ids.begin(),
          facet.point_ids.begin() +
              static_cast<std::ptrdiff_t>(facet.point_count),
          added_point_id)) {
    throw std::invalid_argument(
        "a normalized-H0 factorized coface is invalid");
  }
  key.point_count = facet.point_count + 1U;
  std::copy_n(facet.point_ids.begin(), facet.point_count, key.point_ids.begin());
  key.point_ids[facet.point_count] = added_point_id;
  std::sort(
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count));
  return key;
}

[[nodiscard]] CofaceKey event_closed_set_key(
    const ExactDirectSupportEvent& event) {
  const std::size_t support_count =
      static_cast<std::size_t>(event.support_size);
  CofaceKey key;
  if (support_count < 2U || support_count > event.support_ids.size() ||
      event.closed_rank != support_count + event.interior_ids.size() ||
      event.closed_rank > key.point_ids.size()) {
    throw std::invalid_argument(
        "a normalized-H0 direct event has an invalid closed set");
  }
  key.point_count = event.closed_rank;
  std::copy_n(
      event.support_ids.begin(), support_count, key.point_ids.begin());
  std::copy(
      event.interior_ids.begin(),
      event.interior_ids.end(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(support_count));
  std::sort(
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count));
  if (std::adjacent_find(
          key.point_ids.begin(),
          key.point_ids.begin() +
              static_cast<std::ptrdiff_t>(key.point_count)) !=
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count)) {
    throw std::invalid_argument(
        "a normalized-H0 direct event closed set repeats a point");
  }
  return key;
}

[[nodiscard]] ExactDirectSparseFacetKey facet_from_birth(
    const CofaceKey& key) {
  ExactDirectSparseFacetKey facet;
  if (key.point_count < 2U || key.point_count > facet.point_ids.size()) {
    throw std::invalid_argument(
        "a normalized-H0 direct birth has an unsupported cardinality");
  }
  facet.point_count = key.point_count;
  std::copy_n(key.point_ids.begin(), key.point_count, facet.point_ids.begin());
  return facet;
}

[[nodiscard]] std::array<PointId, 4U> canonical_event_support(
    const ExactDirectSupportEvent& event) {
  std::array<PointId, 4U> support{};
  const std::size_t count = static_cast<std::size_t>(event.support_size);
  if (count < 2U || count > support.size()) {
    throw std::invalid_argument(
        "a normalized-H0 direct support has invalid arity");
  }
  std::copy_n(event.support_ids.begin(), count, support.begin());
  std::sort(
      support.begin(), support.begin() + static_cast<std::ptrdiff_t>(count));
  return support;
}

[[nodiscard]] bool facet_is_deletion_of(
    const ExactDirectSparseFacetKey& facet,
    const CofaceKey& coface) noexcept {
  return facet.point_count + 1U == coface.point_count &&
         std::includes(
             coface.point_ids.begin(),
             coface.point_ids.begin() +
                 static_cast<std::ptrdiff_t>(coface.point_count),
             facet.point_ids.begin(),
             facet.point_ids.begin() +
                 static_cast<std::ptrdiff_t>(facet.point_count));
}

[[nodiscard]] std::optional<PointId> factor_added_point(
    const ExactDirectSparseFacetKey& facet,
    const CofaceKey& coface) noexcept {
  if (!facet_is_deletion_of(facet, coface)) {
    return std::nullopt;
  }
  for (std::size_t index = 0U; index < coface.point_count; ++index) {
    if (!std::binary_search(
            facet.point_ids.begin(),
            facet.point_ids.begin() +
                static_cast<std::ptrdiff_t>(facet.point_count),
            coface.point_ids[index])) {
      return coface.point_ids[index];
    }
  }
  return std::nullopt;
}

[[nodiscard]] ExactDirectNormalizedH0SourcePlanDecision decision_for(
    BuildFailure failure) {
  switch (failure) {
    case BuildFailure::capacity_overflow:
      return ExactDirectNormalizedH0SourcePlanDecision::
          no_source_capacity_overflow;
    case BuildFailure::allocation_failed:
      return ExactDirectNormalizedH0SourcePlanDecision::
          no_source_allocation_failed;
    case BuildFailure::budget_exhausted:
      return ExactDirectNormalizedH0SourcePlanDecision::
          no_source_budget_exhausted;
    case BuildFailure::gateway_authority_rejected:
      return ExactDirectNormalizedH0SourcePlanDecision::
          no_source_gateway_authority_rejected;
    case BuildFailure::source_join_inconsistent:
      return ExactDirectNormalizedH0SourcePlanDecision::
          no_source_join_inconsistent;
    case BuildFailure::none:
      break;
  }
  throw std::logic_error("a normalized-H0 source failure has no decision");
}

[[nodiscard]] ExactDirectNormalizedH0SourcePlanResult base_result(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectClosedSaddleIncidenceJournalResult& incidence,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& budget) {
  ExactDirectNormalizedH0SourcePlanResult result;
  result.requested_budget = budget;
  result.source_gateway_budget = source_gateway_budget;
  result.source_gateway_traversal_order = source_gateway_traversal_order;
  result.point_count = cloud.size();
  result.source_direct_event_count = source_facade.events.size();
  result.source_event_projection_count = source_journal.event_projection_count;
  result.source_role_record_count = source_journal.role_record_count;
  result.source_incidence_family_count = incidence.families.size();
  result.source_gateway_token_count = source_gateway.facet_tokens.size();
  result.source_gateway_candidate_count =
      source_gateway.gateway_candidates.size();
  result.source_pair_canonical_cloud_digest =
      source_gateway.source_pair_canonical_cloud_digest;
  result.source_higher_canonical_cloud_digest =
      source_gateway.source_higher_canonical_cloud_digest;
  result.source_pair_semantic_digest =
      source_gateway.source_pair_semantic_digest;
  result.source_higher_semantic_digest =
      source_gateway.source_higher_semantic_digest;
  result.no_partial_scientific_payload_published = true;
  result.gateway_core_keys_and_audits_reused_without_copy = true;
  result.no_successive_incidence_star_materialized = true;
  result.no_global_star_facet_or_coface_catalog_materialized = true;
  result.no_gamma_or_higher_order_delaunay_materialized = true;
  result.transient_k_plus_one_keys_released_before_publication = true;
  result.persistent_k_plus_one_keys_materialized = false;
  result.hierarchy_or_forest_mutated = false;
  result.global_regularity_authority_certified = false;
  result.omitted_late_cofaces_qr1_noop_certified = false;
  result.normalized_noop_level_receipts_emitted = false;
  result.incidence_complete_reduction_proved = false;
  result.resident_atomic_fold_performed = false;
  result.contract_v2_identity_compatible = false;
  result.campaign_v3_single_gateway_per_core_facet_satisfied = false;
  result.campaign_v3_product_source_claimed = false;
  result.public_status_claimed = false;
  result.scope = ExactDirectNormalizedH0SourcePlanScope::
      complete_direct_events_and_complete_first_incidence_of_direct_core_facets_only;
  return result;
}

void clear_payload(ExactDirectNormalizedH0SourcePlanResult& result) {
  result.gateway_candidate_references.clear();
  result.cofaces.clear();
  result.direct_birth_references.clear();
  result.batch_coface_references.clear();
  result.batches.clear();
  result.logical_storage_entry_count = 0U;
  result.no_partial_scientific_payload_published = true;
}

[[nodiscard]] ExactDirectNormalizedH0SourcePlanResult fail(
    ExactDirectNormalizedH0SourcePlanResult result,
    BuildFailure failure) {
  clear_payload(result);
  result.decision = decision_for(failure);
  return result;
}

[[nodiscard]] bool storage_within_budget(
    const ExactDirectNormalizedH0SourcePlanResult& result,
    const ExactDirectNormalizedH0SourcePlanBudget& budget) noexcept {
  if (result.gateway_candidate_references.size() >
          budget.maximum_source_gateway_candidate_scan_count ||
      result.cofaces.size() > budget.maximum_distinct_coface_count ||
      result.direct_birth_references.size() >
          budget.maximum_direct_birth_reference_count ||
      result.batch_coface_references.size() >
          budget.maximum_batch_coface_reference_count ||
      result.batches.size() > budget.maximum_batch_count) {
    return false;
  }
  std::size_t support_points = 0U;
  for (const auto& coface : result.cofaces) {
    if (!canonical_support(
            coface.positive_support_point_ids,
            coface.positive_support_point_count,
            result.point_count) ||
        !try_add_size(
            support_points,
            coface.positive_support_point_count,
            support_points)) {
      return false;
    }
  }
  std::size_t logical = 0U;
  for (const std::size_t increment : {
           result.gateway_candidate_references.size(),
           result.cofaces.size(),
           support_points,
           result.direct_birth_references.size(),
           result.batch_coface_references.size(),
           result.batches.size()}) {
    if (!try_add_size(logical, increment, logical)) {
      return false;
    }
  }
  return logical <= budget.maximum_logical_storage_entry_count;
}

[[nodiscard]] bool structural_receipt(
    const ExactDirectNormalizedH0SourcePlanResult& result) noexcept {
  if (!storage_within_budget(result, result.requested_budget) ||
      result.required_source_role_scan_count >
          result.requested_budget.maximum_source_role_scan_count ||
      result.required_source_gateway_token_scan_count >
          result.requested_budget.maximum_source_gateway_token_scan_count ||
      result.required_source_gateway_candidate_scan_count >
          result.requested_budget.maximum_source_gateway_candidate_scan_count ||
      result.required_source_gateway_token_scan_count !=
          result.source_gateway_token_count ||
      result.required_source_gateway_candidate_scan_count !=
          result.source_gateway_candidate_count ||
      result.gateway_candidate_references.size() !=
          result.required_higher_order_gateway_candidate_count ||
      result.excluded_order_one_gateway_candidate_count +
              result.required_higher_order_gateway_candidate_count !=
          result.source_gateway_candidate_count ||
      result.required_distinct_coface_count != result.cofaces.size() ||
      result.required_direct_coface_count >
          result.requested_budget.maximum_direct_coface_count ||
      result.required_residual_coface_count >
          result.requested_budget.maximum_residual_coface_count ||
      result.required_direct_coface_count +
              result.required_residual_coface_count !=
          result.cofaces.size() ||
      result.required_batch_count != result.batches.size() ||
      result.required_direct_birth_reference_count !=
          result.direct_birth_references.size() ||
      result.required_batch_coface_reference_count !=
          result.batch_coface_references.size() ||
      result.required_batch_coface_reference_count != result.cofaces.size()) {
    return false;
  }

  for (std::size_t index = 0U;
       index < result.gateway_candidate_references.size();
       ++index) {
    const auto& reference = result.gateway_candidate_references[index];
    if (reference.gateway_candidate_reference_index != index ||
        reference.source_gateway_candidate_index >=
            result.source_gateway_candidate_count ||
        reference.source_coface_index >= result.cofaces.size() ||
        !result.cofaces[reference.source_coface_index]
             .observed_by_first_incidence_gateway) {
      return false;
    }
    if (index != 0U) {
      const auto& previous = result.gateway_candidate_references[index - 1U];
      if (previous.source_coface_index > reference.source_coface_index ||
          (previous.source_coface_index == reference.source_coface_index &&
           previous.source_gateway_candidate_index >=
               reference.source_gateway_candidate_index)) {
        return false;
      }
    }
  }

  std::size_t direct_count = 0U;
  std::size_t residual_count = 0U;
  std::size_t support_point_count = 0U;
  std::size_t gateway_reference_offset = 0U;
  for (std::size_t index = 0U; index < result.cofaces.size(); ++index) {
    const auto& coface = result.cofaces[index];
    if (coface.coface_index != index || coface.order < 2U ||
        coface.owner_source_gateway_token_index >=
            result.source_gateway_token_count ||
        !canonical_support(
            coface.positive_support_point_ids,
            coface.positive_support_point_count,
            result.point_count) ||
        coface.supplied_core_facet_occurrence_count == 0U ||
        !try_add_size(
            support_point_count,
            coface.positive_support_point_count,
            support_point_count)) {
      return false;
    }
    if (coface.gateway_candidate_reference_offset !=
            gateway_reference_offset ||
        coface.gateway_candidate_reference_count >
            result.gateway_candidate_references.size() -
                gateway_reference_offset ||
        (coface.gateway_candidate_reference_count != 0U) !=
            coface.observed_by_first_incidence_gateway) {
      return false;
    }
    for (std::size_t local = 0U;
         local < coface.gateway_candidate_reference_count;
         ++local) {
      if (result.gateway_candidate_references[gateway_reference_offset + local]
              .source_coface_index != index) {
        return false;
      }
    }
    gateway_reference_offset += coface.gateway_candidate_reference_count;
    if (coface.kind ==
        ExactDirectNormalizedH0SourceCofaceKind::direct_saddle) {
      if (coface.direct_family_match_count != 1U ||
          !coface.source_role_record_index.has_value() ||
          *coface.source_role_record_index >= result.source_role_record_count ||
          !coface.source_event_projection_index.has_value() ||
          *coface.source_event_projection_index >=
              result.source_event_projection_count ||
          !coface.source_incidence_family_index.has_value() ||
          *coface.source_incidence_family_index >=
              result.source_incidence_family_count) {
        return false;
      }
      ++direct_count;
    } else if (
        coface.kind == ExactDirectNormalizedH0SourceCofaceKind::
                           first_incidence_residual) {
      if (coface.direct_family_match_count != 0U ||
          !coface.observed_by_first_incidence_gateway ||
          coface.source_role_record_index.has_value() ||
          coface.source_event_projection_index.has_value() ||
          coface.source_incidence_family_index.has_value()) {
        return false;
      }
      ++residual_count;
    } else {
      return false;
    }
  }
  if (direct_count != result.required_direct_coface_count ||
      residual_count != result.required_residual_coface_count ||
      gateway_reference_offset !=
          result.gateway_candidate_references.size()) {
    return false;
  }

  for (std::size_t index = 0U;
       index < result.direct_birth_references.size();
       ++index) {
    const auto& reference = result.direct_birth_references[index];
    if (reference.direct_birth_reference_index != index ||
        reference.source_role_record_index >= result.source_role_record_count ||
        reference.source_event_projection_index >=
            result.source_event_projection_count ||
        !canonical_facet_key(reference.birth_facet_key, result.point_count)) {
      return false;
    }
  }
  for (std::size_t index = 0U;
       index < result.batch_coface_references.size();
       ++index) {
    const auto& reference = result.batch_coface_references[index];
    if (reference.batch_coface_reference_index != index ||
        reference.source_coface_index >= result.cofaces.size() ||
        result.cofaces[reference.source_coface_index]
                .batch_coface_reference_index != index) {
      return false;
    }
  }
  for (std::size_t index = 0U; index < result.cofaces.size(); ++index) {
    const auto& coface = result.cofaces[index];
    if (coface.batch_coface_reference_index >=
            result.batch_coface_references.size() ||
        result.batch_coface_references[coface.batch_coface_reference_index]
                .source_coface_index != index) {
      return false;
    }
  }

  std::size_t birth_offset = 0U;
  std::size_t coface_offset = 0U;
  for (std::size_t index = 0U; index < result.batches.size(); ++index) {
    const auto& batch = result.batches[index];
    if (batch.batch_index != index || batch.future_snapshot_index != index ||
        batch.order < 2U ||
        batch.direct_birth_reference_offset != birth_offset ||
        batch.coface_reference_offset != coface_offset ||
        batch.direct_birth_reference_count >
            result.direct_birth_references.size() - birth_offset ||
        batch.coface_reference_count >
            result.batch_coface_references.size() - coface_offset ||
        (batch.direct_birth_reference_count == 0U &&
         batch.coface_reference_count == 0U) ||
        (index != 0U &&
         !BatchKeyLess{}(
             BatchKey{result.batches[index - 1U].squared_level,
                      result.batches[index - 1U].order},
             BatchKey{batch.squared_level, batch.order}))) {
      return false;
    }
    for (std::size_t local = 0U;
         local < batch.direct_birth_reference_count;
         ++local) {
      if (result.direct_birth_references[birth_offset + local]
              .birth_facet_key.point_count != batch.order) {
        return false;
      }
    }
    for (std::size_t local = 0U; local < batch.coface_reference_count;
         ++local) {
      const auto& coface = result.cofaces[
          result.batch_coface_references[coface_offset + local]
              .source_coface_index];
      if (coface.order != batch.order ||
          coface.squared_level != batch.squared_level) {
        return false;
      }
    }
    birth_offset += batch.direct_birth_reference_count;
    coface_offset += batch.coface_reference_count;
  }
  if (birth_offset != result.direct_birth_references.size() ||
      coface_offset != result.batch_coface_references.size()) {
    return false;
  }

  std::size_t logical = 0U;
  for (const std::size_t increment : {
           result.gateway_candidate_references.size(),
           result.cofaces.size(),
           support_point_count,
           result.direct_birth_references.size(),
           result.batch_coface_references.size(),
           result.batches.size()}) {
    if (!try_add_size(logical, increment, logical)) {
      return false;
    }
  }
  return logical == result.logical_storage_entry_count;
}

[[nodiscard]] ExactDirectNormalizedH0SourcePlanResult build_impl(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult& incidence,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& budget) {
  auto result = base_result(
      cloud,
      source_facade,
      source_journal,
      incidence,
      source_gateway_budget,
      source_gateway_traversal_order,
      source_gateway,
      budget);

  const auto gateway_verification =
      verify_exact_direct_sparse_gateway_candidate_journal(
          index,
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal,
          source_incidence_budget,
          incidence,
          source_gateway_budget,
          source_gateway_traversal_order,
          source_gateway);
  if (!gateway_verification.result_certified ||
      !source_gateway.certified_partial_refinement()) {
    return fail(std::move(result), BuildFailure::gateway_authority_rejected);
  }
  result.source_gateway_journal_freshly_verified = true;
  const auto gateway_identity =
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          source_gateway, budget.source_gateway_identity_budget);
  if (!gateway_identity.certified_identity()) {
    if (gateway_identity.decision ==
        ExactDirectSparseGatewayCandidateScientificIdentityDecision::
            no_identity_capacity_overflow) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    if (gateway_identity.decision ==
        ExactDirectSparseGatewayCandidateScientificIdentityDecision::
            no_identity_budget_exhausted) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }
    return fail(std::move(result), BuildFailure::gateway_authority_rejected);
  }
  result.source_gateway_scientific_identity_digest =
      gateway_identity.scientific_identity_digest;
  result.source_gateway_scientific_identity_certified = true;

  const ExactDirectMorseEventJournalView source_view{source_journal};
  result.required_source_role_scan_count =
      source_view.materialized_direct_role_records().size();
  result.required_source_gateway_token_scan_count =
      source_gateway.facet_tokens.size();
  result.required_source_gateway_candidate_scan_count =
      source_gateway.gateway_candidates.size();
  if (result.required_source_role_scan_count >
          budget.maximum_source_role_scan_count ||
      result.required_source_gateway_token_scan_count >
          budget.maximum_source_gateway_token_scan_count ||
      result.required_source_gateway_candidate_scan_count >
          budget.maximum_source_gateway_candidate_scan_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }

  std::map<std::size_t, std::size_t> saddle_family_by_event;
  for (const auto& family : incidence.families) {
    if (family.order == 1U) {
      continue;
    }
    if (!saddle_family_by_event
             .emplace(family.source_event_index, family.family_index)
             .second) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
  }

  std::map<CofaceKey, CofaceScratch, CofaceKeyLess> coface_scratch;
  std::vector<DirectBirthScratch> birth_scratch;
  std::size_t direct_coface_scratch_count = 0U;
  std::size_t residual_coface_scratch_count = 0U;
  for (const auto& role : source_view.materialized_direct_role_records()) {
    if (role.batch_index >= source_journal.batches.size() ||
        role.event_projection_index >= source_view.event_projection_count()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    const auto& source_batch = source_journal.batches[role.batch_index];
    if (source_batch.order == 1U) {
      ++result.excluded_order_one_role_count;
      continue;
    }
    const auto& projection =
        source_view.materialized_direct_event_projection_at(
            role.event_projection_index);
    if (projection.source !=
            ExactDirectMorseEventSource::direct_support_terminal_event ||
        projection.source_index >= source_facade.events.size()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    const auto& event = source_facade.events[projection.source_index];
    const CofaceKey closed_set = event_closed_set_key(event);
    if (role.role == ExactDirectMorseH0Role::birth) {
      if (event.birth_order !=
              std::optional<std::size_t>{source_batch.order} ||
          closed_set.point_count != source_batch.order) {
        return fail(std::move(result), BuildFailure::source_join_inconsistent);
      }
      if (birth_scratch.size() >=
              budget.maximum_direct_birth_reference_count ||
          birth_scratch.size() >=
              budget.maximum_logical_storage_entry_count) {
        return fail(std::move(result), BuildFailure::budget_exhausted);
      }
      birth_scratch.push_back(DirectBirthScratch{
          BatchKey{source_batch.squared_level, source_batch.order},
          role.role_record_index,
          role.event_projection_index,
          facet_from_birth(closed_set)});
      continue;
    }
    if (role.role != ExactDirectMorseH0Role::saddle ||
        event.saddle_order !=
            std::optional<std::size_t>{source_batch.order} ||
        closed_set.point_count != source_batch.order + 1U ||
        !saddle_family_by_event.contains(projection.source_index)) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    const auto support = canonical_event_support(event);
    const std::size_t family_index =
        saddle_family_by_event.at(projection.source_index);
    auto position = coface_scratch.find(closed_set);
    bool inserted = false;
    if (position == coface_scratch.end()) {
      if (coface_scratch.size() >= budget.maximum_distinct_coface_count ||
          coface_scratch.size() >=
              budget.maximum_logical_storage_entry_count ||
          direct_coface_scratch_count >=
              budget.maximum_direct_coface_count) {
        return fail(std::move(result), BuildFailure::budget_exhausted);
      }
      std::tie(position, inserted) = coface_scratch.emplace(
          closed_set,
          CofaceScratch{
              event.squared_level,
              role.role_record_index,
              role.event_projection_index,
              family_index,
              support,
              static_cast<std::size_t>(event.support_size),
              0U,
              0U,
              std::numeric_limits<std::size_t>::max()});
      ++direct_coface_scratch_count;
    }
    if (!inserted &&
        (position->second.squared_level != event.squared_level ||
         position->second.source_role_record_index !=
             std::optional<std::size_t>{role.role_record_index} ||
         position->second.source_event_projection_index !=
             std::optional<std::size_t>{role.event_projection_index} ||
         position->second.source_incidence_family_index !=
             std::optional<std::size_t>{family_index} ||
         position->second.support_point_ids != support ||
         position->second.support_point_count !=
             static_cast<std::size_t>(event.support_size))) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    ++position->second.direct_family_match_count;
    if (position->second.direct_family_match_count > 1U) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
  }

  std::vector<GatewayCandidateScratch> gateway_candidate_scratch;
  for (const auto& candidate : source_gateway.gateway_candidates) {
    if (candidate.gateway_candidate_index >=
            source_gateway.gateway_candidates.size() ||
        candidate.facet_token_index >= source_gateway.facet_tokens.size()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    const auto& token =
        source_gateway.facet_tokens[candidate.facet_token_index];
    if (token.source_facet_key.point_count == 1U) {
      ++result.excluded_order_one_gateway_candidate_count;
      continue;
    }
    if (token.source_facet_key.point_count < 2U) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    if (candidate.positive_support_point_count < 2U ||
        candidate.positive_support_point_count >
            candidate.positive_support_point_ids.size()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    const CofaceKey key = merge_facet_and_point(
        token.source_facet_key, candidate.added_point_id);
    std::array<PointId, 4U> support = candidate.positive_support_point_ids;
    std::sort(
        support.begin(),
        support.begin() + static_cast<std::ptrdiff_t>(
                              candidate.positive_support_point_count));
    auto position = coface_scratch.find(key);
    bool inserted = false;
    if (position == coface_scratch.end()) {
      if (coface_scratch.size() >= budget.maximum_distinct_coface_count ||
          coface_scratch.size() >=
              budget.maximum_logical_storage_entry_count ||
          residual_coface_scratch_count >=
              budget.maximum_residual_coface_count) {
        return fail(std::move(result), BuildFailure::budget_exhausted);
      }
      std::tie(position, inserted) = coface_scratch.emplace(
          key,
          CofaceScratch{
              token.first_incidence_squared_level,
              std::nullopt,
              std::nullopt,
              std::nullopt,
              support,
              candidate.positive_support_point_count,
              0U,
              0U,
              std::numeric_limits<std::size_t>::max()});
      ++residual_coface_scratch_count;
    }
    if (!inserted &&
        (position->second.squared_level !=
             token.first_incidence_squared_level ||
         position->second.support_point_ids != support ||
         position->second.support_point_count !=
             candidate.positive_support_point_count)) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    ++position->second.gateway_candidate_occurrence_count;
    if (gateway_candidate_scratch.size() >=
        budget.maximum_logical_storage_entry_count) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }
    gateway_candidate_scratch.push_back(
        GatewayCandidateScratch{
            candidate.gateway_candidate_index, &position->second});
    ++result.required_higher_order_gateway_candidate_count;
  }

  result.required_direct_birth_reference_count = birth_scratch.size();
  result.required_direct_coface_count = direct_coface_scratch_count;
  result.required_residual_coface_count = residual_coface_scratch_count;
  result.required_distinct_coface_count = coface_scratch.size();
  result.required_batch_coface_reference_count = coface_scratch.size();
  if (coface_scratch.size() >
      budget.maximum_batch_coface_reference_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }

  std::sort(
      birth_scratch.begin(),
      birth_scratch.end(),
      [](const DirectBirthScratch& left, const DirectBirthScratch& right) {
        if (BatchKeyLess{}(left.batch_key, right.batch_key)) {
          return true;
        }
        if (BatchKeyLess{}(right.batch_key, left.batch_key)) {
          return false;
        }
        return left.source_role_record_index < right.source_role_record_index;
      });
  std::vector<std::pair<BatchKey, std::size_t>> coface_order;
  coface_order.reserve(coface_scratch.size());
  std::size_t support_point_count = 0U;
  std::size_t coface_index = 0U;
  for (auto& [key, scratch] : coface_scratch) {
    if (scratch.direct_family_match_count > 1U ||
        !try_add_size(
            support_point_count,
            scratch.support_point_count,
            support_point_count)) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    scratch.published_coface_index = coface_index;
    coface_order.emplace_back(
        BatchKey{scratch.squared_level, key.point_count - 1U}, coface_index);
    ++coface_index;
  }
  std::sort(
      coface_order.begin(),
      coface_order.end(),
      [](const auto& left, const auto& right) {
        if (BatchKeyLess{}(left.first, right.first)) {
          return true;
        }
        if (BatchKeyLess{}(right.first, left.first)) {
          return false;
        }
        return left.second < right.second;
      });

  std::size_t birth_probe = 0U;
  std::size_t coface_probe = 0U;
  std::size_t batch_count = 0U;
  while (birth_probe < birth_scratch.size() ||
         coface_probe < coface_order.size()) {
    const bool birth_first =
        coface_probe == coface_order.size() ||
        (birth_probe < birth_scratch.size() &&
         BatchKeyLess{}(
             birth_scratch[birth_probe].batch_key,
             coface_order[coface_probe].first));
    const BatchKey key = birth_first
                             ? birth_scratch[birth_probe].batch_key
                             : coface_order[coface_probe].first;
    ++batch_count;
    if (batch_count > budget.maximum_batch_count) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }
    while (birth_probe < birth_scratch.size() &&
           birth_scratch[birth_probe].batch_key == key) {
      ++birth_probe;
    }
    while (coface_probe < coface_order.size() &&
           coface_order[coface_probe].first == key) {
      ++coface_probe;
    }
  }
  result.required_batch_count = batch_count;

  std::size_t logical = 0U;
  for (const std::size_t increment : {
           gateway_candidate_scratch.size(),
           coface_scratch.size(),
           support_point_count,
           birth_scratch.size(),
           coface_scratch.size(),
           batch_count}) {
    if (!try_add_size(logical, increment, logical)) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    if (logical > budget.maximum_logical_storage_entry_count) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }
  }

  std::vector<ExactDirectNormalizedH0GatewayCandidateReference>
      gateway_candidate_references;
  gateway_candidate_references.reserve(gateway_candidate_scratch.size());
  for (const auto& candidate : gateway_candidate_scratch) {
    if (candidate.coface == nullptr ||
        candidate.coface->published_coface_index >= coface_scratch.size()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    gateway_candidate_references.push_back(
        ExactDirectNormalizedH0GatewayCandidateReference{
            0U,
            candidate.source_gateway_candidate_index,
            candidate.coface->published_coface_index});
  }
  std::sort(
      gateway_candidate_references.begin(),
      gateway_candidate_references.end(),
      [](const auto& left, const auto& right) {
        if (left.source_coface_index != right.source_coface_index) {
          return left.source_coface_index < right.source_coface_index;
        }
        return left.source_gateway_candidate_index <
               right.source_gateway_candidate_index;
      });
  for (std::size_t index = 0U;
       index < gateway_candidate_references.size();
       ++index) {
    gateway_candidate_references[index].gateway_candidate_reference_index =
        index;
  }

  std::vector<ExactDirectNormalizedH0SourceCoface> cofaces;
  cofaces.reserve(coface_scratch.size());
  std::size_t gateway_reference_cursor = 0U;
  for (const auto& [key, scratch] : coface_scratch) {
    std::optional<std::size_t> owner;
    std::size_t core_occurrence_count = 0U;
    for (std::size_t removed = 0U; removed < key.point_count; ++removed) {
      const ExactDirectSparseFacetKey facet =
          delete_coface_point(key, removed);
      const auto found = std::lower_bound(
          source_gateway.facet_tokens.begin(),
          source_gateway.facet_tokens.end(),
          facet,
          [](const auto& token, const auto& candidate_facet) {
            return facet_key_less(
                token.source_facet_key, candidate_facet);
          });
      if (found != source_gateway.facet_tokens.end() &&
          found->source_facet_key == facet) {
        ++core_occurrence_count;
        if (!owner.has_value() || found->facet_token_index < *owner) {
          owner = found->facet_token_index;
        }
      }
    }
    if (!owner.has_value()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    const auto& owner_facet =
        source_gateway.facet_tokens[*owner].source_facet_key;
    const auto added_point = factor_added_point(owner_facet, key);
    if (!added_point.has_value()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    const std::size_t reference_offset = gateway_reference_cursor;
    while (gateway_reference_cursor <
               gateway_candidate_references.size() &&
           gateway_candidate_references[gateway_reference_cursor]
                   .source_coface_index == scratch.published_coface_index) {
      ++gateway_reference_cursor;
    }
    const std::size_t reference_count =
        gateway_reference_cursor - reference_offset;
    if (reference_count != scratch.gateway_candidate_occurrence_count) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    const bool direct = scratch.direct_family_match_count == 1U;
    cofaces.push_back(ExactDirectNormalizedH0SourceCoface{
        scratch.published_coface_index,
        *owner,
        *added_point,
        key.point_count - 1U,
        scratch.source_role_record_index,
        scratch.source_event_projection_index,
        scratch.source_incidence_family_index,
        scratch.support_point_ids,
        scratch.support_point_count,
        scratch.squared_level,
        core_occurrence_count,
        reference_offset,
        reference_count,
        std::numeric_limits<std::size_t>::max(),
        scratch.direct_family_match_count,
        direct ? ExactDirectNormalizedH0SourceCofaceKind::direct_saddle
               : ExactDirectNormalizedH0SourceCofaceKind::
                     first_incidence_residual,
        reference_count != 0U});
  }
  if (gateway_reference_cursor != gateway_candidate_references.size()) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }

  std::vector<ExactDirectNormalizedH0SourceDirectBirthReference> birth_refs;
  std::vector<ExactDirectNormalizedH0SourceBatchCofaceReference> coface_refs;
  std::vector<ExactDirectNormalizedH0SourceBatch> batches;
  birth_refs.reserve(birth_scratch.size());
  coface_refs.reserve(cofaces.size());
  batches.reserve(batch_count);
  std::size_t birth_cursor = 0U;
  std::size_t coface_cursor = 0U;
  while (birth_cursor < birth_scratch.size() ||
         coface_cursor < coface_order.size()) {
    const bool birth_first =
        coface_cursor == coface_order.size() ||
        (birth_cursor < birth_scratch.size() &&
         BatchKeyLess{}(
             birth_scratch[birth_cursor].batch_key,
             coface_order[coface_cursor].first));
    const BatchKey key = birth_first
                             ? birth_scratch[birth_cursor].batch_key
                             : coface_order[coface_cursor].first;
    const std::size_t birth_offset = birth_refs.size();
    while (birth_cursor < birth_scratch.size() &&
           birth_scratch[birth_cursor].batch_key == key) {
      const auto& birth = birth_scratch[birth_cursor];
      birth_refs.push_back(
          ExactDirectNormalizedH0SourceDirectBirthReference{
              birth_refs.size(),
              birth.source_role_record_index,
              birth.source_event_projection_index,
              birth.birth_facet_key});
      ++birth_cursor;
    }
    const std::size_t coface_offset = coface_refs.size();
    while (coface_cursor < coface_order.size() &&
           coface_order[coface_cursor].first == key) {
      const std::size_t source_coface_index =
          coface_order[coface_cursor].second;
      if (source_coface_index >= cofaces.size() ||
          cofaces[source_coface_index].batch_coface_reference_index !=
              std::numeric_limits<std::size_t>::max()) {
        return fail(std::move(result), BuildFailure::source_join_inconsistent);
      }
      cofaces[source_coface_index].batch_coface_reference_index =
          coface_refs.size();
      coface_refs.push_back(
          ExactDirectNormalizedH0SourceBatchCofaceReference{
              coface_refs.size(), source_coface_index});
      ++coface_cursor;
    }
    batches.push_back(ExactDirectNormalizedH0SourceBatch{
        batches.size(),
        batches.size(),
        key.squared_level,
        key.order,
        birth_offset,
        birth_refs.size() - birth_offset,
        coface_offset,
        coface_refs.size() - coface_offset});
  }
  if (birth_cursor != birth_scratch.size() ||
      coface_cursor != coface_order.size()) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }
  if (batches.size() != batch_count ||
      coface_refs.size() != cofaces.size()) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }

  result.logical_storage_entry_count = logical;
  result.gateway_candidate_references =
      std::move(gateway_candidate_references);
  result.cofaces = std::move(cofaces);
  result.direct_birth_references = std::move(birth_refs);
  result.batch_coface_references = std::move(coface_refs);
  result.batches = std::move(batches);
  result.order_one_roles_excluded_to_preserve_boruvka_authority = true;
  result.every_higher_order_direct_birth_projected_once = true;
  result.every_higher_order_direct_saddle_joined_once = true;
  result.order_one_gateway_candidates_excluded_to_preserve_boruvka_authority =
      true;
  result.every_higher_order_source_gateway_candidate_mapped_once = true;
  result.direct_union_complete_first_incidence_cominimizers_deduplicated =
      true;
  result.canonical_owner_is_lexicographically_first_core_facet = true;
  result.unique_batch_per_exact_level_and_order = true;
  result.batches_sorted_by_exact_level_then_order = true;
  result.exact_rational_levels_retained = true;
  result.logical_storage_within_budget = true;
  result.decision = ExactDirectNormalizedH0SourcePlanDecision::
      complete_certified_direct_plus_first_incidence_source_plan;
  return result;
}

[[nodiscard]] bool source_association_matches(
    const ExactDirectSparseGatewayCandidateJournalResult& gateway,
    const ExactDirectSparseGatewayCandidateScientificIdentityResult& identity,
    const ExactDirectNormalizedH0SourcePlanResult& source) noexcept {
  return identity.certified_identity() &&
         identity.requested_budget ==
             source.requested_budget.source_gateway_identity_budget &&
         identity.required_deletion_projection_count ==
             gateway.deletion_projections.size() &&
         identity.required_facet_token_count == gateway.facet_tokens.size() &&
         identity.required_gateway_candidate_count ==
             gateway.gateway_candidates.size() &&
         identity.required_batch_count == gateway.batches.size() &&
         identity.required_batch_facet_token_index_count ==
             gateway.batch_facet_token_indices.size() &&
         source.source_gateway_scientific_identity_certified &&
         identity.scientific_identity_digest ==
             source.source_gateway_scientific_identity_digest &&
         source.point_count == gateway.point_count &&
         source.source_gateway_budget == gateway.requested_budget &&
         source.source_gateway_traversal_order == gateway.traversal_order &&
         source.source_direct_event_count ==
             gateway.source_direct_event_count &&
         source.source_gateway_token_count == gateway.facet_tokens.size() &&
         source.source_gateway_candidate_count ==
             gateway.gateway_candidates.size() &&
         source.source_pair_canonical_cloud_digest ==
             gateway.source_pair_canonical_cloud_digest &&
         source.source_higher_canonical_cloud_digest ==
             gateway.source_higher_canonical_cloud_digest &&
         source.source_pair_semantic_digest ==
             gateway.source_pair_semantic_digest &&
         source.source_higher_semantic_digest ==
             gateway.source_higher_semantic_digest;
}

}  // namespace

bool ExactDirectNormalizedH0SourcePlanResult::
    certified_complete_candidate_source_plan() const noexcept {
  return schema_version == direct_normalized_h0_source_plan_schema_version &&
         source_gateway_journal_freshly_verified &&
         source_gateway_scientific_identity_certified &&
         order_one_roles_excluded_to_preserve_boruvka_authority &&
         every_higher_order_direct_birth_projected_once &&
         every_higher_order_direct_saddle_joined_once &&
         order_one_gateway_candidates_excluded_to_preserve_boruvka_authority &&
         every_higher_order_source_gateway_candidate_mapped_once &&
         direct_union_complete_first_incidence_cominimizers_deduplicated &&
         canonical_owner_is_lexicographically_first_core_facet &&
         unique_batch_per_exact_level_and_order &&
         batches_sorted_by_exact_level_then_order &&
         exact_rational_levels_retained && logical_storage_within_budget &&
         no_partial_scientific_payload_published &&
         gateway_core_keys_and_audits_reused_without_copy &&
         no_successive_incidence_star_materialized &&
         no_global_star_facet_or_coface_catalog_materialized &&
         no_gamma_or_higher_order_delaunay_materialized &&
         transient_k_plus_one_keys_released_before_publication &&
         !persistent_k_plus_one_keys_materialized &&
         !hierarchy_or_forest_mutated &&
         !global_regularity_authority_certified &&
         !omitted_late_cofaces_qr1_noop_certified &&
         !normalized_noop_level_receipts_emitted &&
         !incidence_complete_reduction_proved &&
         !resident_atomic_fold_performed && !contract_v2_identity_compatible &&
         !campaign_v3_single_gateway_per_core_facet_satisfied &&
         !campaign_v3_product_source_claimed &&
         !public_status_claimed && structural_receipt(*this) &&
         decision == ExactDirectNormalizedH0SourcePlanDecision::
                         complete_certified_direct_plus_first_incidence_source_plan &&
         scope == ExactDirectNormalizedH0SourcePlanScope::
                      complete_direct_events_and_complete_first_incidence_of_direct_core_facets_only;
}

ExactDirectNormalizedH0ReconstructedCoface
reconstruct_exact_direct_normalized_h0_source_coface(
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectSparseGatewayCandidateScientificIdentityResult&
        source_gateway_identity,
    const ExactDirectNormalizedH0SourcePlanResult& source,
    std::size_t coface_index) {
  if (!source_association_matches(
          source_gateway, source_gateway_identity, source) ||
      coface_index >= source.cofaces.size()) {
    throw std::invalid_argument(
        "a normalized-H0 coface has a different source authority or index");
  }
  const auto& coface = source.cofaces[coface_index];
  if (coface.coface_index != coface_index ||
      coface.owner_source_gateway_token_index >=
          source_gateway.facet_tokens.size()) {
    throw std::invalid_argument(
        "a normalized-H0 coface has inconsistent factorized indexing");
  }
  const CofaceKey key = merge_facet_and_point(
      source_gateway
          .facet_tokens[coface.owner_source_gateway_token_index]
          .source_facet_key,
      coface.added_point_id);
  if (key.point_count != coface.order + 1U) {
    throw std::invalid_argument(
        "a normalized-H0 coface order differs from its factorization");
  }
  ExactDirectNormalizedH0ReconstructedCoface result;
  result.point_count = key.point_count;
  std::copy_n(key.point_ids.begin(), key.point_count, result.point_ids.begin());
  return result;
}

ExactDirectNormalizedH0SourcePlanResult
build_exact_direct_normalized_h0_source_plan(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult& incidence,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& budget) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the normalized-H0 source LBVH has a different point authority");
  }
  try {
    return build_impl(
        index,
        cloud,
        source_facade,
        source_journal,
        source_arm_budget,
        source_arm_journal,
        source_incidence_budget,
        incidence,
        source_gateway_budget,
        source_gateway_traversal_order,
        source_gateway,
        budget);
  } catch (const std::bad_alloc&) {
    auto result = base_result(
        cloud,
        source_facade,
        source_journal,
        incidence,
        source_gateway_budget,
        source_gateway_traversal_order,
        source_gateway,
        budget);
    result.decision =
        ExactDirectNormalizedH0SourcePlanDecision::no_source_allocation_failed;
    return result;
  } catch (const std::length_error&) {
    auto result = base_result(
        cloud,
        source_facade,
        source_journal,
        incidence,
        source_gateway_budget,
        source_gateway_traversal_order,
        source_gateway,
        budget);
    result.decision =
        ExactDirectNormalizedH0SourcePlanDecision::no_source_allocation_failed;
    return result;
  }
}

ExactDirectNormalizedH0SourcePlanVerification
verify_exact_direct_normalized_h0_source_plan(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult& incidence,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& budget,
    const ExactDirectNormalizedH0SourcePlanResult& observed) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the normalized-H0 source verifier has a different point authority");
  }
  ExactDirectNormalizedH0SourcePlanVerification verification;
  verification.observed_storage_within_budget =
      storage_within_budget(observed, budget);
  if (!verification.observed_storage_within_budget) {
    return verification;
  }
  const auto expected = build_exact_direct_normalized_h0_source_plan(
      index,
      cloud,
      source_facade,
      source_journal,
      source_arm_budget,
      source_arm_journal,
      source_incidence_budget,
      incidence,
      source_gateway_budget,
      source_gateway_traversal_order,
      source_gateway,
      budget);
  verification.source_gateway_journal_freshly_verified =
      expected.source_gateway_journal_freshly_verified;
  verification.expected_result_freshly_reconstructed =
      expected.certified_complete_candidate_source_plan() ||
      expected.decision !=
          ExactDirectNormalizedH0SourcePlanDecision::not_certified;
  verification.observed_recursively_equal = observed == expected;
  verification.no_forbidden_global_structure_materialized =
      observed.gateway_core_keys_and_audits_reused_without_copy &&
      observed.no_successive_incidence_star_materialized &&
      observed.no_global_star_facet_or_coface_catalog_materialized &&
      observed.no_gamma_or_higher_order_delaunay_materialized &&
      observed.transient_k_plus_one_keys_released_before_publication &&
      !observed.persistent_k_plus_one_keys_materialized &&
      !observed.hierarchy_or_forest_mutated;
  verification.unsupported_noop_and_resident_claims_remain_false =
      !observed.global_regularity_authority_certified &&
      !observed.omitted_late_cofaces_qr1_noop_certified &&
      !observed.normalized_noop_level_receipts_emitted &&
      !observed.incidence_complete_reduction_proved &&
      !observed.resident_atomic_fold_performed &&
      !observed.contract_v2_identity_compatible &&
      !observed.campaign_v3_single_gateway_per_core_facet_satisfied &&
      !observed.campaign_v3_product_source_claimed &&
      !observed.public_status_claimed;
  verification.fresh_replay_certified =
      verification.observed_storage_within_budget &&
      verification.source_gateway_journal_freshly_verified &&
      verification.expected_result_freshly_reconstructed &&
      verification.observed_recursively_equal &&
      verification.no_forbidden_global_structure_materialized &&
      verification.unsupported_noop_and_resident_claims_remain_false;
  verification.result_certified =
      verification.fresh_replay_certified &&
      expected.certified_complete_candidate_source_plan();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
