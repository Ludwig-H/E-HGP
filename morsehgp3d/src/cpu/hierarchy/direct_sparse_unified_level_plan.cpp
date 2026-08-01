#include "morsehgp3d/hierarchy/direct_sparse_unified_level_plan.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

struct TransientCofaceKey {
  std::array<PointId, 11U> point_ids{};
  std::size_t point_count{};

  friend bool operator==(
      const TransientCofaceKey&, const TransientCofaceKey&) = default;
};

struct BatchKey {
  exact::ExactLevel squared_level{};
  std::size_t order{};

  friend bool operator==(const BatchKey&, const BatchKey&) = default;
};

struct FamilyScratch {
  std::size_t source_role_record_index{};
  std::size_t source_family_index{};
  std::size_t source_event_index{};
};

struct DirectScratch {
  BatchKey batch_key{};
  std::size_t source_role_record_index{};
  std::size_t source_event_projection_index{};
  ExactDirectMorseH0Role role{ExactDirectMorseH0Role::birth};
  std::optional<std::size_t> source_family_index;
  std::optional<std::size_t> source_star_direct_coface_index;
  std::optional<ExactDirectSparseFacetKey> birth_facet_key;
  std::optional<TransientCofaceKey> saddle_coface_key;
};

struct StarCofaceScratch {
  BatchKey batch_key{};
  std::size_t source_star_coface_index{};
  TransientCofaceKey coface_key{};
  ExactDirectSparseSuccessiveIncidenceStarCofaceKind kind{
      ExactDirectSparseSuccessiveIncidenceStarCofaceKind::direct_family};
};

enum class FacetOccurrenceKind : std::uint8_t {
  direct_birth,
  coface_deletion,
};

struct FacetOccurrenceScratch {
  ExactDirectSparseFacetKey facet_key{};
  FacetOccurrenceKind kind{FacetOccurrenceKind::direct_birth};
  std::size_t source_index{};
};

struct CofaceFacetScratch {
  BatchKey batch_key{};
  std::size_t source_star_coface_index{};
  std::size_t removed_union_point_index{};
  PointId removed_point_id{};
  ExactDirectSparseFacetKey facet_key{};
};

enum class BuildFailure : std::uint8_t {
  none,
  capacity_overflow,
  allocation_failed,
  budget_exhausted,
  source_star_not_certified,
  source_join_inconsistent,
  direct_star_crosscheck_contradiction,
  batch_partition_contradiction,
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

void require_valid_traversal_order(
    spatial::LbvhTraversalOrder traversal_order) {
  switch (traversal_order) {
    case spatial::LbvhTraversalOrder::near_first:
    case spatial::LbvhTraversalOrder::far_first:
      return;
  }
  throw std::invalid_argument(
      "a unified-level-plan traversal order is invalid");
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

[[nodiscard]] bool transient_key_less(
    const TransientCofaceKey& left,
    const TransientCofaceKey& right) noexcept {
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

[[nodiscard]] bool batch_key_less(
    const BatchKey& left,
    const BatchKey& right) noexcept {
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  return left.order < right.order;
}

[[nodiscard]] bool direct_scratch_less(
    const DirectScratch& left,
    const DirectScratch& right) noexcept {
  if (batch_key_less(left.batch_key, right.batch_key)) {
    return true;
  }
  if (batch_key_less(right.batch_key, left.batch_key)) {
    return false;
  }
  if (left.role != right.role) {
    return static_cast<std::uint8_t>(left.role) <
           static_cast<std::uint8_t>(right.role);
  }
  return left.source_role_record_index < right.source_role_record_index;
}

[[nodiscard]] bool star_scratch_less(
    const StarCofaceScratch& left,
    const StarCofaceScratch& right) noexcept {
  if (batch_key_less(left.batch_key, right.batch_key)) {
    return true;
  }
  if (batch_key_less(right.batch_key, left.batch_key)) {
    return false;
  }
  return left.source_star_coface_index < right.source_star_coface_index;
}

[[nodiscard]] bool coface_facet_scratch_less(
    const CofaceFacetScratch& left,
    const CofaceFacetScratch& right) noexcept {
  if (batch_key_less(left.batch_key, right.batch_key)) {
    return true;
  }
  if (batch_key_less(right.batch_key, left.batch_key)) {
    return false;
  }
  if (left.source_star_coface_index != right.source_star_coface_index) {
    return left.source_star_coface_index <
           right.source_star_coface_index;
  }
  return left.removed_union_point_index < right.removed_union_point_index;
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

[[nodiscard]] TransientCofaceKey event_closed_set_key(
    const ExactDirectSupportEvent& event) {
  const std::size_t support_size =
      static_cast<std::size_t>(event.support_size);
  if (support_size < 2U || support_size > event.support_ids.size() ||
      event.closed_rank > 11U) {
    throw std::invalid_argument(
        "a unified-level direct event has an invalid closed rank");
  }
  TransientCofaceKey key;
  key.point_count = support_size + event.interior_ids.size();
  if (key.point_count != event.closed_rank || key.point_count > 11U) {
    throw std::invalid_argument(
        "a unified-level direct event closed set is inconsistent");
  }
  std::copy_n(
      event.support_ids.begin(), support_size, key.point_ids.begin());
  std::copy(
      event.interior_ids.begin(),
      event.interior_ids.end(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(support_size));
  std::sort(
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count));
  if (std::adjacent_find(
          key.point_ids.begin(),
          key.point_ids.begin() +
              static_cast<std::ptrdiff_t>(key.point_count)) !=
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count)) {
    throw std::invalid_argument(
        "a unified-level direct event closed set repeats a point");
  }
  return key;
}

[[nodiscard]] TransientCofaceKey star_coface_key(
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& star,
    std::size_t coface_index) {
  const auto source =
      reconstruct_exact_direct_sparse_successive_incidence_star_coface(
          star, coface_index);
  if (source.point_count < 3U || source.point_count > 11U) {
    throw std::invalid_argument(
        "a unified-level star coface has an invalid higher-order rank");
  }
  TransientCofaceKey key;
  key.point_count = source.point_count;
  std::copy_n(
      source.point_ids.begin(), source.point_count, key.point_ids.begin());
  return key;
}

[[nodiscard]] ExactDirectSparseFacetKey facet_from_closed_set(
    const TransientCofaceKey& key) {
  if (key.point_count < 2U || key.point_count > 10U) {
    throw std::invalid_argument(
        "a unified-level direct birth is not a supported facet");
  }
  ExactDirectSparseFacetKey facet;
  facet.point_count = key.point_count;
  std::copy_n(key.point_ids.begin(), key.point_count, facet.point_ids.begin());
  return facet;
}

[[nodiscard]] ExactDirectSparseFacetKey delete_closed_set_point(
    const TransientCofaceKey& key,
    std::size_t removed_index) {
  if (key.point_count < 3U || key.point_count > 11U ||
      removed_index >= key.point_count) {
    throw std::invalid_argument(
        "a unified-level coface deletion is invalid");
  }
  ExactDirectSparseFacetKey facet;
  facet.point_count = key.point_count - 1U;
  std::copy_n(key.point_ids.begin(), removed_index, facet.point_ids.begin());
  std::copy(
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(removed_index + 1U),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count),
      facet.point_ids.begin() + static_cast<std::ptrdiff_t>(removed_index));
  return facet;
}

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanDecision decision_for(
    BuildFailure failure) {
  switch (failure) {
    case BuildFailure::capacity_overflow:
      return ExactDirectSparseUnifiedLevelPlanDecision::
          no_plan_capacity_overflow;
    case BuildFailure::allocation_failed:
      return ExactDirectSparseUnifiedLevelPlanDecision::
          no_plan_allocation_failed;
    case BuildFailure::budget_exhausted:
      return ExactDirectSparseUnifiedLevelPlanDecision::
          no_plan_budget_exhausted;
    case BuildFailure::source_star_not_certified:
      return ExactDirectSparseUnifiedLevelPlanDecision::
          no_plan_source_star_not_certified;
    case BuildFailure::source_join_inconsistent:
      return ExactDirectSparseUnifiedLevelPlanDecision::
          no_plan_source_join_inconsistent;
    case BuildFailure::direct_star_crosscheck_contradiction:
      return ExactDirectSparseUnifiedLevelPlanDecision::
          no_plan_direct_star_crosscheck_contradiction;
    case BuildFailure::batch_partition_contradiction:
      return ExactDirectSparseUnifiedLevelPlanDecision::
          no_plan_batch_partition_contradiction;
    case BuildFailure::none:
      break;
  }
  throw std::logic_error("a unified-level-plan failure has no decision");
}

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanResult base_result(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectClosedSaddleIncidenceJournalResult& incidence,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& star_budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& star,
    const ExactDirectSparseUnifiedLevelPlanBudget& budget) {
  ExactDirectSparseUnifiedLevelPlanResult result;
  result.requested_budget = budget;
  result.source_star_budget = star_budget;
  result.source_star_traversal_order = traversal_order;
  result.point_count = cloud.size();
  result.source_event_projection_count = source_journal.event_projection_count;
  result.source_role_record_count = source_journal.role_record_count;
  result.source_incidence_family_count = incidence.families.size();
  result.source_star_facet_token_count = star.facet_tokens.size();
  result.source_star_coface_count = star.cofaces.size();
  result.source_pair_canonical_cloud_digest =
      star.source_pair_canonical_cloud_digest;
  result.source_higher_canonical_cloud_digest =
      star.source_higher_canonical_cloud_digest;
  result.source_pair_semantic_digest = star.source_pair_semantic_digest;
  result.source_higher_semantic_digest = star.source_higher_semantic_digest;
  result.scope = ExactDirectSparseUnifiedLevelPlanScope::
      bounded_unified_direct_and_residual_level_plan_of_supplied_higher_order_star_only;
  result.no_partial_scientific_payload_published = true;
  result.no_k_plus_one_coface_key_persisted = true;
  result.no_global_facet_or_coface_catalog_materialized = true;
  result.reducer_snapshot_or_forest_mutated = false;
  result.bounded_star_global_completeness_claimed = false;
  result.public_status_claimed = false;
  result.partial_refinement_only = true;
  return result;
}

void clear_payload(ExactDirectSparseUnifiedLevelPlanResult& result) {
  result.facet_tokens.clear();
  result.direct_references.clear();
  result.residual_references.clear();
  result.coface_facet_references.clear();
  result.batches.clear();
  result.logical_storage_entry_count = 0U;
  result.no_partial_scientific_payload_published = true;
}

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanResult fail(
    ExactDirectSparseUnifiedLevelPlanResult result,
    BuildFailure failure) {
  clear_payload(result);
  result.decision = decision_for(failure);
  return result;
}

[[nodiscard]] bool budget_sufficient(
    const ExactDirectSparseUnifiedLevelPlanResult& result,
    const ExactDirectSparseUnifiedLevelPlanBudget& budget) noexcept {
  return result.required_source_role_scan_count <=
             budget.maximum_source_role_scan_count &&
         result.required_higher_order_direct_role_count <=
             budget.maximum_higher_order_direct_role_count &&
         result.required_source_family_scan_count <=
             budget.maximum_source_family_scan_count &&
         result.required_higher_order_saddle_family_count <=
             budget.maximum_higher_order_saddle_family_count &&
         result.required_star_coface_scan_count <=
             budget.maximum_star_coface_scan_count &&
         result.required_star_direct_coface_count <=
             budget.maximum_star_direct_coface_count &&
         result.required_star_residual_coface_count <=
             budget.maximum_star_residual_coface_count &&
         result.required_star_coface_point_scan_count <=
             budget.maximum_star_coface_point_scan_count &&
         result.required_coface_deletion_reference_count <=
             budget.maximum_coface_deletion_reference_count &&
         result.required_direct_reference_count <=
             budget.maximum_direct_reference_count &&
         result.required_residual_reference_count <=
             budget.maximum_residual_reference_count;
}

[[nodiscard]] bool add_logical(
    std::size_t increment,
    const ExactDirectSparseUnifiedLevelPlanBudget& budget,
    std::size_t& count) noexcept {
  return try_add_size(count, increment, count) &&
         count <= budget.maximum_logical_storage_entry_count;
}

[[nodiscard]] bool plan_storage_within_budget(
    const ExactDirectSparseUnifiedLevelPlanResult& observed,
    const ExactDirectSparseUnifiedLevelPlanBudget& budget) noexcept {
  if (observed.facet_tokens.size() >
          budget.maximum_distinct_facet_token_count ||
      observed.direct_references.size() >
          budget.maximum_direct_reference_count ||
      observed.residual_references.size() >
          budget.maximum_residual_reference_count ||
      observed.coface_facet_references.size() >
          budget.maximum_coface_deletion_reference_count ||
      observed.batches.size() > budget.maximum_batch_count) {
    return false;
  }
  std::size_t key_point_count = 0U;
  for (const auto& token : observed.facet_tokens) {
    if (!canonical_facet_key(token.facet_key, observed.point_count) ||
        !try_add_size(
            key_point_count, token.facet_key.point_count, key_point_count)) {
      return false;
    }
  }
  std::size_t logical = 0U;
  for (const std::size_t increment : {
           observed.facet_tokens.size(),
           key_point_count,
           observed.direct_references.size(),
           observed.residual_references.size(),
           observed.coface_facet_references.size(),
           observed.batches.size()}) {
    if (!try_add_size(logical, increment, logical)) {
      return false;
    }
  }
  return key_point_count <= budget.maximum_facet_key_point_count &&
         logical <= budget.maximum_logical_storage_entry_count;
}

[[nodiscard]] bool structural_plan_receipt(
    const ExactDirectSparseUnifiedLevelPlanResult& result) noexcept {
  std::size_t direct_role_total = 0U;
  std::size_t star_coface_total = 0U;
  if (!try_add_size(
          result.required_direct_birth_reference_count,
          result.required_direct_saddle_reference_count,
          direct_role_total) ||
      !try_add_size(
          result.required_star_direct_coface_count,
          result.required_star_residual_coface_count,
          star_coface_total)) {
    return false;
  }
  if (!plan_storage_within_budget(result, result.requested_budget) ||
      !budget_sufficient(result, result.requested_budget) ||
      result.required_distinct_facet_token_count !=
          result.facet_tokens.size() ||
      result.required_batch_count != result.batches.size() ||
      result.required_direct_reference_count !=
          result.direct_references.size() ||
      result.required_residual_reference_count !=
          result.residual_references.size() ||
      result.required_coface_deletion_reference_count !=
          result.coface_facet_references.size() ||
      result.required_direct_reference_count !=
          result.required_higher_order_direct_role_count ||
      direct_role_total != result.required_direct_reference_count ||
      result.required_direct_saddle_reference_count !=
          result.required_higher_order_saddle_family_count ||
      result.required_direct_star_crosscheck_count !=
          result.required_star_direct_coface_count ||
      result.required_direct_star_crosscheck_count !=
          result.required_direct_saddle_reference_count ||
      result.required_residual_reference_count !=
          result.required_star_residual_coface_count ||
      star_coface_total != result.required_star_coface_scan_count ||
      result.required_star_coface_point_scan_count !=
          result.required_coface_deletion_reference_count ||
      result.excluded_order_one_role_count >
          result.required_source_role_scan_count ||
      result.excluded_order_one_family_count >
          result.required_source_family_scan_count ||
      result.required_higher_order_direct_role_count !=
          result.required_source_role_scan_count -
              result.excluded_order_one_role_count ||
      result.required_higher_order_saddle_family_count !=
          result.required_source_family_scan_count -
              result.excluded_order_one_family_count) {
    return false;
  }

  std::size_t key_point_count = 0U;
  std::size_t birth_token_reference_count = 0U;
  std::size_t deletion_token_reference_count = 0U;
  for (std::size_t index = 0U; index < result.facet_tokens.size(); ++index) {
    const auto& token = result.facet_tokens[index];
    if (token.facet_token_index != index ||
        !canonical_facet_key(token.facet_key, result.point_count) ||
        (index != 0U &&
         !facet_key_less(
             result.facet_tokens[index - 1U].facet_key,
             token.facet_key)) ||
        (token.source_star_facet_token_index.has_value() &&
         *token.source_star_facet_token_index >=
             result.source_star_facet_token_count) ||
        !try_add_size(
            key_point_count, token.facet_key.point_count, key_point_count) ||
        !try_add_size(
            birth_token_reference_count,
            token.direct_birth_reference_count,
            birth_token_reference_count) ||
        !try_add_size(
            deletion_token_reference_count,
            token.coface_deletion_reference_count,
            deletion_token_reference_count)) {
      return false;
    }
  }
  if (key_point_count != result.required_facet_key_point_count ||
      birth_token_reference_count !=
          result.required_direct_birth_reference_count ||
      deletion_token_reference_count !=
          result.required_coface_deletion_reference_count) {
    return false;
  }

  std::size_t observed_birth_count = 0U;
  std::size_t observed_saddle_count = 0U;
  for (std::size_t index = 0U; index < result.direct_references.size();
       ++index) {
    const auto& reference = result.direct_references[index];
    if (reference.direct_reference_index != index ||
        reference.source_role_record_index >= result.source_role_record_count ||
        reference.source_event_projection_index >=
            result.source_event_projection_count) {
      return false;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      if (result.direct_references[prior].source_role_record_index ==
          reference.source_role_record_index) {
        return false;
      }
    }
    if (reference.role == ExactDirectMorseH0Role::birth) {
      if (reference.source_incidence_family_index.has_value() ||
          reference.source_star_direct_coface_index.has_value() ||
          !reference.direct_birth_facet_token_index.has_value() ||
          *reference.direct_birth_facet_token_index >=
              result.facet_tokens.size()) {
        return false;
      }
      ++observed_birth_count;
    } else if (reference.role == ExactDirectMorseH0Role::saddle) {
      if (!reference.source_incidence_family_index.has_value() ||
          *reference.source_incidence_family_index >=
              result.source_incidence_family_count ||
          !reference.source_star_direct_coface_index.has_value() ||
          *reference.source_star_direct_coface_index >=
              result.source_star_coface_count ||
          reference.direct_birth_facet_token_index.has_value()) {
        return false;
      }
      ++observed_saddle_count;
    } else {
      return false;
    }
  }
  if (observed_birth_count != result.required_direct_birth_reference_count ||
      observed_saddle_count !=
          result.required_direct_saddle_reference_count) {
    return false;
  }

  for (std::size_t index = 0U; index < result.residual_references.size();
       ++index) {
    const auto& reference = result.residual_references[index];
    if (reference.residual_reference_index != index ||
        reference.source_star_coface_index >=
            result.source_star_coface_count) {
      return false;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      if (result.residual_references[prior].source_star_coface_index ==
          reference.source_star_coface_index) {
        return false;
      }
    }
    for (const auto& direct : result.direct_references) {
      if (direct.source_star_direct_coface_index ==
          reference.source_star_coface_index) {
        return false;
      }
    }
  }

  for (std::size_t index = 0U;
       index < result.coface_facet_references.size();
       ++index) {
    const auto& reference = result.coface_facet_references[index];
    if (reference.coface_facet_reference_index != index ||
        reference.source_star_coface_index >=
            result.source_star_coface_count ||
        reference.removed_union_point_index > 10U ||
        static_cast<std::size_t>(reference.removed_point_id) >=
            result.point_count ||
        reference.facet_token_index >= result.facet_tokens.size() ||
        std::binary_search(
            result.facet_tokens[reference.facet_token_index]
                .facet_key.point_ids.begin(),
            result.facet_tokens[reference.facet_token_index]
                    .facet_key.point_ids.begin() +
                static_cast<std::ptrdiff_t>(
                    result.facet_tokens[reference.facet_token_index]
                        .facet_key.point_count),
            reference.removed_point_id)) {
      return false;
    }
    for (std::size_t prior = 0U; prior < index; ++prior) {
      const auto& previous = result.coface_facet_references[prior];
      if (previous.source_star_coface_index ==
              reference.source_star_coface_index &&
          previous.removed_union_point_index ==
              reference.removed_union_point_index) {
        return false;
      }
    }
  }

  std::size_t direct_offset = 0U;
  std::size_t residual_offset = 0U;
  std::size_t deletion_offset = 0U;
  for (std::size_t index = 0U; index < result.batches.size(); ++index) {
    const auto& batch = result.batches[index];
    if (batch.batch_index != index || batch.future_snapshot_index != index ||
        batch.order < 2U || batch.direct_reference_offset != direct_offset ||
        batch.residual_reference_offset != residual_offset ||
        batch.coface_facet_reference_offset != deletion_offset ||
        batch.direct_reference_count >
            result.direct_references.size() - direct_offset ||
        batch.residual_reference_count >
            result.residual_references.size() - residual_offset ||
        batch.coface_facet_reference_count >
            result.coface_facet_references.size() - deletion_offset ||
        (batch.direct_reference_count == 0U &&
         batch.residual_reference_count == 0U) ||
        (index != 0U &&
         !batch_key_less(
             BatchKey{result.batches[index - 1U].squared_level,
                      result.batches[index - 1U].order},
             BatchKey{batch.squared_level, batch.order})) ||
        !try_add_size(
            direct_offset,
            batch.direct_reference_count,
            direct_offset) ||
        !try_add_size(
            residual_offset,
            batch.residual_reference_count,
            residual_offset) ||
        !try_add_size(
            deletion_offset,
            batch.coface_facet_reference_count,
            deletion_offset)) {
      return false;
    }
  }
  if (direct_offset != result.direct_references.size() ||
      residual_offset != result.residual_references.size() ||
      deletion_offset != result.coface_facet_references.size()) {
    return false;
  }

  std::size_t logical = 0U;
  for (const std::size_t increment : {
           result.facet_tokens.size(),
           key_point_count,
           result.direct_references.size(),
           result.residual_references.size(),
           result.coface_facet_references.size(),
           result.batches.size()}) {
    if (!try_add_size(logical, increment, logical)) {
      return false;
    }
  }
  return logical == result.logical_storage_entry_count;
}

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanResult build_impl(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult& incidence,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& star_budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& star,
    const ExactDirectSparseUnifiedLevelPlanBudget& budget) {
  ExactDirectSparseUnifiedLevelPlanResult result = base_result(
      cloud,
      source_journal,
      incidence,
      star_budget,
      traversal_order,
      star,
      budget);

  const auto star_verification =
      verify_exact_direct_sparse_successive_incidence_star_journal(
          index,
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal,
          source_incidence_budget,
          incidence,
          star_budget,
          traversal_order,
          star);
  if (!star_verification.result_certified ||
      !star.certified_bounded_star()) {
    return fail(std::move(result), BuildFailure::source_star_not_certified);
  }
  result.source_star_freshly_verified = true;

  const ExactDirectMorseEventJournalView source_view{source_journal};
  result.required_source_role_scan_count =
      source_view.materialized_direct_role_records().size();
  result.required_source_family_scan_count = incidence.families.size();
  result.required_star_coface_scan_count = star.cofaces.size();
  if (result.required_source_role_scan_count >
          budget.maximum_source_role_scan_count ||
      result.required_source_family_scan_count >
          budget.maximum_source_family_scan_count ||
      result.required_star_coface_scan_count >
          budget.maximum_star_coface_scan_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }

  std::vector<FamilyScratch> family_scratch;
  family_scratch.reserve(incidence.families.size());
  for (const auto& family : incidence.families) {
    if (family.order == 1U) {
      ++result.excluded_order_one_family_count;
      continue;
    }
    if (family.order < 2U ||
        family.source_arm_family_index >= source_arm_journal.families.size()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    const auto& arm_family =
        source_arm_journal.families[family.source_arm_family_index];
    family_scratch.push_back(FamilyScratch{
        arm_family.journal_role_record_index,
        family.family_index,
        family.source_event_index});
    ++result.required_higher_order_saddle_family_count;
  }
  std::sort(
      family_scratch.begin(),
      family_scratch.end(),
      [](const FamilyScratch& left, const FamilyScratch& right) {
        return left.source_role_record_index < right.source_role_record_index;
      });
  if (std::adjacent_find(
          family_scratch.begin(),
          family_scratch.end(),
          [](const FamilyScratch& left, const FamilyScratch& right) {
            return left.source_role_record_index ==
                   right.source_role_record_index;
          }) != family_scratch.end()) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }

  std::vector<DirectScratch> direct_scratch;
  direct_scratch.reserve(result.required_source_role_scan_count);
  for (const auto& role : source_view.materialized_direct_role_records()) {
    if (role.batch_index >= source_journal.batches.size()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    const auto& batch = source_journal.batches[role.batch_index];
    if (batch.order == 1U) {
      ++result.excluded_order_one_role_count;
      continue;
    }
    if (batch.order < 2U ||
        role.event_projection_index >= source_view.event_projection_count()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    const auto& projection =
        source_view.materialized_direct_event_projection_at(
            role.event_projection_index);
    if (projection.source_index >= source_facade.events.size()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    const auto& event = source_facade.events[projection.source_index];
    const TransientCofaceKey closed_set = event_closed_set_key(event);
    DirectScratch direct;
    direct.batch_key = BatchKey{batch.squared_level, batch.order};
    direct.source_role_record_index = role.role_record_index;
    direct.source_event_projection_index = role.event_projection_index;
    direct.role = role.role;
    if (role.role == ExactDirectMorseH0Role::birth) {
      if (event.birth_order != std::optional<std::size_t>{batch.order} ||
          closed_set.point_count != batch.order) {
        return fail(std::move(result), BuildFailure::source_join_inconsistent);
      }
      direct.birth_facet_key = facet_from_closed_set(closed_set);
      ++result.required_direct_birth_reference_count;
    } else if (role.role == ExactDirectMorseH0Role::saddle) {
      if (event.saddle_order != std::optional<std::size_t>{batch.order} ||
          closed_set.point_count != batch.order + 1U) {
        return fail(std::move(result), BuildFailure::source_join_inconsistent);
      }
      const auto family = std::lower_bound(
          family_scratch.begin(),
          family_scratch.end(),
          role.role_record_index,
          [](const FamilyScratch& candidate, std::size_t role_index) {
            return candidate.source_role_record_index < role_index;
          });
      if (family == family_scratch.end() ||
          family->source_role_record_index != role.role_record_index ||
          family->source_event_index != projection.source_index) {
        return fail(std::move(result), BuildFailure::source_join_inconsistent);
      }
      direct.source_family_index = family->source_family_index;
      direct.saddle_coface_key = closed_set;
      ++result.required_direct_saddle_reference_count;
    } else {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    direct_scratch.push_back(std::move(direct));
    ++result.required_higher_order_direct_role_count;
  }
  if (result.required_higher_order_direct_role_count !=
          result.required_source_role_scan_count -
              result.excluded_order_one_role_count ||
      result.required_higher_order_saddle_family_count !=
          result.required_source_family_scan_count -
              result.excluded_order_one_family_count ||
      result.required_direct_saddle_reference_count !=
          result.required_higher_order_saddle_family_count) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }
  result.order_one_roles_and_families_excluded_to_preserve_boruvka_authority =
      true;

  std::vector<StarCofaceScratch> star_scratch;
  star_scratch.reserve(star.cofaces.size());
  for (const auto& coface : star.cofaces) {
    if (coface.coface_index >= star.cofaces.size() ||
        coface.owner_facet_token_index >= star.facet_tokens.size()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    const TransientCofaceKey key =
        star_coface_key(star, coface.coface_index);
    const std::size_t order = key.point_count - 1U;
    if (star.facet_tokens[coface.owner_facet_token_index]
            .source_facet_key.point_count != order) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    if (!try_add_size(
            result.required_star_coface_point_scan_count,
            key.point_count,
            result.required_star_coface_point_scan_count) ||
        !try_add_size(
            result.required_coface_deletion_reference_count,
            key.point_count,
            result.required_coface_deletion_reference_count)) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    if (coface.kind ==
        ExactDirectSparseSuccessiveIncidenceStarCofaceKind::direct_family) {
      ++result.required_star_direct_coface_count;
    } else if (
        coface.kind ==
        ExactDirectSparseSuccessiveIncidenceStarCofaceKind::residual) {
      ++result.required_star_residual_coface_count;
    } else {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    star_scratch.push_back(StarCofaceScratch{
        BatchKey{coface.squared_level, order},
        coface.coface_index,
        key,
        coface.kind});
  }
  result.required_direct_reference_count =
      result.required_higher_order_direct_role_count;
  result.required_residual_reference_count =
      result.required_star_residual_coface_count;
  if (!budget_sufficient(result, budget)) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }

  std::vector<std::size_t> saddle_direct_indices;
  std::vector<std::size_t> star_direct_indices;
  for (std::size_t index_value = 0U; index_value < direct_scratch.size();
       ++index_value) {
    if (direct_scratch[index_value].role == ExactDirectMorseH0Role::saddle) {
      saddle_direct_indices.push_back(index_value);
    }
  }
  for (std::size_t index_value = 0U; index_value < star_scratch.size();
       ++index_value) {
    if (star_scratch[index_value].kind ==
        ExactDirectSparseSuccessiveIncidenceStarCofaceKind::direct_family) {
      star_direct_indices.push_back(index_value);
    }
  }
  const auto saddle_key_less = [&direct_scratch](
                                   std::size_t left,
                                   std::size_t right) {
    return transient_key_less(
        *direct_scratch[left].saddle_coface_key,
        *direct_scratch[right].saddle_coface_key);
  };
  const auto star_key_less = [&star_scratch](
                                 std::size_t left,
                                 std::size_t right) {
    return transient_key_less(
        star_scratch[left].coface_key, star_scratch[right].coface_key);
  };
  std::sort(
      saddle_direct_indices.begin(),
      saddle_direct_indices.end(),
      saddle_key_less);
  std::sort(
      star_direct_indices.begin(), star_direct_indices.end(), star_key_less);
  if (saddle_direct_indices.size() != star_direct_indices.size() ||
      saddle_direct_indices.size() !=
          result.required_higher_order_saddle_family_count) {
    return fail(
        std::move(result), BuildFailure::direct_star_crosscheck_contradiction);
  }
  for (std::size_t join_index = 0U;
       join_index < saddle_direct_indices.size();
       ++join_index) {
    DirectScratch& direct = direct_scratch[saddle_direct_indices[join_index]];
    const StarCofaceScratch& star_coface =
        star_scratch[star_direct_indices[join_index]];
    if (*direct.saddle_coface_key != star_coface.coface_key ||
        direct.batch_key != star_coface.batch_key ||
        (join_index != 0U &&
         *direct_scratch[saddle_direct_indices[join_index - 1U]]
                  .saddle_coface_key == *direct.saddle_coface_key)) {
      return fail(
          std::move(result),
          BuildFailure::direct_star_crosscheck_contradiction);
    }
    direct.source_star_direct_coface_index =
        star_coface.source_star_coface_index;
    ++result.required_direct_star_crosscheck_count;
  }
  result.direct_star_cofaces_crosschecked_bijectively = true;
  result.star_direct_cofaces_are_crosscheck_only_not_residual_references =
      true;

  std::vector<FacetOccurrenceScratch> facet_occurrences;
  std::size_t facet_occurrence_capacity = 0U;
  if (!try_add_size(
          result.required_direct_birth_reference_count,
          result.required_coface_deletion_reference_count,
          facet_occurrence_capacity)) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  facet_occurrences.reserve(facet_occurrence_capacity);
  for (std::size_t index_value = 0U; index_value < direct_scratch.size();
       ++index_value) {
    const auto& direct = direct_scratch[index_value];
    if (direct.birth_facet_key.has_value()) {
      facet_occurrences.push_back(FacetOccurrenceScratch{
          *direct.birth_facet_key,
          FacetOccurrenceKind::direct_birth,
          index_value});
    }
  }
  std::vector<CofaceFacetScratch> deletion_scratch;
  deletion_scratch.reserve(result.required_coface_deletion_reference_count);
  for (const auto& coface : star_scratch) {
    for (std::size_t removed_index = 0U;
         removed_index < coface.coface_key.point_count;
         ++removed_index) {
      const ExactDirectSparseFacetKey facet =
          delete_closed_set_point(coface.coface_key, removed_index);
      const std::size_t deletion_index = deletion_scratch.size();
      deletion_scratch.push_back(CofaceFacetScratch{
          coface.batch_key,
          coface.source_star_coface_index,
          removed_index,
          coface.coface_key.point_ids[removed_index],
          facet});
      facet_occurrences.push_back(FacetOccurrenceScratch{
          facet,
          FacetOccurrenceKind::coface_deletion,
          deletion_index});
    }
  }
  if (deletion_scratch.size() !=
      result.required_coface_deletion_reference_count) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }
  std::sort(
      facet_occurrences.begin(),
      facet_occurrences.end(),
      [](const FacetOccurrenceScratch& left,
         const FacetOccurrenceScratch& right) {
        if (facet_key_less(left.facet_key, right.facet_key)) {
          return true;
        }
        if (facet_key_less(right.facet_key, left.facet_key)) {
          return false;
        }
        if (left.kind != right.kind) {
          return static_cast<std::uint8_t>(left.kind) <
                 static_cast<std::uint8_t>(right.kind);
        }
        return left.source_index < right.source_index;
      });

  std::vector<ExactDirectSparseUnifiedLevelPlanFacetToken> tokens;
  for (std::size_t begin = 0U; begin < facet_occurrences.size();) {
    std::size_t end = begin + 1U;
    while (end < facet_occurrences.size() &&
           facet_occurrences[begin].facet_key ==
               facet_occurrences[end].facet_key) {
      ++end;
    }
    if (tokens.size() >= budget.maximum_distinct_facet_token_count) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }
    std::optional<std::size_t> source_star_token;
    const auto star_token = std::lower_bound(
        star.facet_tokens.begin(),
        star.facet_tokens.end(),
        facet_occurrences[begin].facet_key,
        [](const auto& token, const ExactDirectSparseFacetKey& key) {
          return facet_key_less(token.source_facet_key, key);
        });
    if (star_token != star.facet_tokens.end() &&
        star_token->source_facet_key == facet_occurrences[begin].facet_key) {
      source_star_token = star_token->facet_token_index;
    }
    ExactDirectSparseUnifiedLevelPlanFacetToken token;
    token.facet_token_index = tokens.size();
    token.facet_key = facet_occurrences[begin].facet_key;
    token.source_star_facet_token_index = source_star_token;
    for (std::size_t occurrence_index = begin;
         occurrence_index < end;
         ++occurrence_index) {
      if (facet_occurrences[occurrence_index].kind ==
          FacetOccurrenceKind::direct_birth) {
        ++token.direct_birth_reference_count;
      } else {
        ++token.coface_deletion_reference_count;
      }
    }
    if (!try_add_size(
            result.required_facet_key_point_count,
            token.facet_key.point_count,
            result.required_facet_key_point_count)) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    tokens.push_back(std::move(token));
    begin = end;
  }
  result.required_distinct_facet_token_count = tokens.size();
  if (result.required_distinct_facet_token_count >
          budget.maximum_distinct_facet_token_count ||
      result.required_facet_key_point_count >
          budget.maximum_facet_key_point_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }

  const auto find_token = [&tokens](
                              const ExactDirectSparseFacetKey& key)
      -> std::optional<std::size_t> {
    const auto found = std::lower_bound(
        tokens.begin(),
        tokens.end(),
        key,
        [](const auto& token, const ExactDirectSparseFacetKey& candidate) {
          return facet_key_less(token.facet_key, candidate);
        });
    if (found == tokens.end() || found->facet_key != key) {
      return std::nullopt;
    }
    return found->facet_token_index;
  };

  std::sort(direct_scratch.begin(), direct_scratch.end(), direct_scratch_less);
  std::sort(star_scratch.begin(), star_scratch.end(), star_scratch_less);
  std::sort(
      deletion_scratch.begin(),
      deletion_scratch.end(),
      coface_facet_scratch_less);
  std::vector<BatchKey> batch_keys;
  std::size_t batch_key_capacity = 0U;
  if (!try_add_size(
          direct_scratch.size(),
          result.required_star_residual_coface_count,
          batch_key_capacity)) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  batch_keys.reserve(batch_key_capacity);
  for (const auto& direct : direct_scratch) {
    batch_keys.push_back(direct.batch_key);
  }
  for (const auto& coface : star_scratch) {
    if (coface.kind ==
        ExactDirectSparseSuccessiveIncidenceStarCofaceKind::residual) {
      batch_keys.push_back(coface.batch_key);
    }
  }
  std::sort(batch_keys.begin(), batch_keys.end(), batch_key_less);
  batch_keys.erase(
      std::unique(batch_keys.begin(), batch_keys.end()), batch_keys.end());
  result.required_batch_count = batch_keys.size();
  if (batch_keys.size() > budget.maximum_batch_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }

  std::vector<ExactDirectSparseUnifiedLevelPlanDirectReference> direct_refs;
  std::vector<ExactDirectSparseUnifiedLevelPlanResidualReference>
      residual_refs;
  std::vector<ExactDirectSparseUnifiedLevelPlanCofaceFacetReference>
      deletion_refs;
  std::vector<ExactDirectSparseUnifiedLevelPlanBatch> batches;
  direct_refs.reserve(result.required_direct_reference_count);
  residual_refs.reserve(result.required_residual_reference_count);
  deletion_refs.reserve(result.required_coface_deletion_reference_count);
  batches.reserve(result.required_batch_count);

  std::size_t direct_cursor = 0U;
  std::size_t star_cursor = 0U;
  std::size_t deletion_cursor = 0U;
  for (const BatchKey& key : batch_keys) {
    const std::size_t direct_offset = direct_refs.size();
    while (direct_cursor < direct_scratch.size() &&
           direct_scratch[direct_cursor].batch_key == key) {
      const DirectScratch& direct = direct_scratch[direct_cursor];
      std::optional<std::size_t> birth_token_index;
      if (direct.birth_facet_key.has_value()) {
        birth_token_index = find_token(*direct.birth_facet_key);
        if (!birth_token_index.has_value()) {
          return fail(
              std::move(result), BuildFailure::batch_partition_contradiction);
        }
      }
      direct_refs.push_back(
          ExactDirectSparseUnifiedLevelPlanDirectReference{
              direct_refs.size(),
              direct.source_role_record_index,
              direct.source_event_projection_index,
              direct.role,
              direct.source_family_index,
              direct.source_star_direct_coface_index,
              birth_token_index});
      ++direct_cursor;
    }

    const std::size_t residual_offset = residual_refs.size();
    while (star_cursor < star_scratch.size() &&
           batch_key_less(star_scratch[star_cursor].batch_key, key)) {
      ++star_cursor;
    }
    std::size_t local_star_cursor = star_cursor;
    while (local_star_cursor < star_scratch.size() &&
           star_scratch[local_star_cursor].batch_key == key) {
      if (star_scratch[local_star_cursor].kind ==
          ExactDirectSparseSuccessiveIncidenceStarCofaceKind::residual) {
        residual_refs.push_back(
            ExactDirectSparseUnifiedLevelPlanResidualReference{
                residual_refs.size(),
                star_scratch[local_star_cursor]
                    .source_star_coface_index});
      }
      ++local_star_cursor;
    }
    star_cursor = local_star_cursor;

    const std::size_t deletion_offset = deletion_refs.size();
    while (deletion_cursor < deletion_scratch.size() &&
           deletion_scratch[deletion_cursor].batch_key == key) {
      const CofaceFacetScratch& deletion =
          deletion_scratch[deletion_cursor];
      const auto token_index = find_token(deletion.facet_key);
      if (!token_index.has_value()) {
        return fail(
            std::move(result), BuildFailure::batch_partition_contradiction);
      }
      deletion_refs.push_back(
          ExactDirectSparseUnifiedLevelPlanCofaceFacetReference{
              deletion_refs.size(),
              deletion.source_star_coface_index,
              deletion.removed_union_point_index,
              deletion.removed_point_id,
              *token_index});
      ++deletion_cursor;
    }
    if (direct_refs.size() == direct_offset &&
        residual_refs.size() == residual_offset) {
      return fail(
          std::move(result), BuildFailure::batch_partition_contradiction);
    }
    batches.push_back(ExactDirectSparseUnifiedLevelPlanBatch{
        batches.size(),
        batches.size(),
        key.squared_level,
        key.order,
        direct_offset,
        direct_refs.size() - direct_offset,
        residual_offset,
        residual_refs.size() - residual_offset,
        deletion_offset,
        deletion_refs.size() - deletion_offset});
  }
  if (direct_cursor != direct_scratch.size() ||
      star_cursor != star_scratch.size() ||
      deletion_cursor != deletion_scratch.size() ||
      direct_refs.size() != result.required_direct_reference_count ||
      residual_refs.size() != result.required_residual_reference_count ||
      deletion_refs.size() !=
          result.required_coface_deletion_reference_count) {
    return fail(
        std::move(result), BuildFailure::batch_partition_contradiction);
  }

  std::size_t logical = 0U;
  for (const std::size_t increment : {
           tokens.size(),
           result.required_facet_key_point_count,
           direct_refs.size(),
           residual_refs.size(),
           deletion_refs.size(),
           batches.size()}) {
    if (!add_logical(increment, budget, logical)) {
      return fail(std::move(result), BuildFailure::budget_exhausted);
    }
  }
  result.logical_storage_entry_count = logical;
  result.facet_tokens = std::move(tokens);
  result.direct_references = std::move(direct_refs);
  result.residual_references = std::move(residual_refs);
  result.coface_facet_references = std::move(deletion_refs);
  result.batches = std::move(batches);
  result.every_higher_order_direct_role_projected_once = true;
  result.every_higher_order_saddle_role_joined_to_one_family = true;
  result.every_star_residual_coface_referenced_once = true;
  result.every_star_coface_contributes_all_factorized_deletions = true;
  result.facet_tokens_canonical_and_deduplicated = true;
  result.unique_batch_per_exact_level_and_order = true;
  result.direct_and_residual_same_level_order_share_one_future_snapshot = true;
  result.batches_sorted_by_exact_level_then_order = true;
  result.logical_storage_within_budget = true;
  result.decision = ExactDirectSparseUnifiedLevelPlanDecision::
      complete_bounded_unified_level_plan;
  return result;
}

}  // namespace

bool ExactDirectSparseUnifiedLevelPlanResult::certified_bounded_plan()
    const noexcept {
  return schema_version == direct_sparse_unified_level_plan_schema_version &&
         source_star_freshly_verified &&
         order_one_roles_and_families_excluded_to_preserve_boruvka_authority &&
         every_higher_order_direct_role_projected_once &&
         every_higher_order_saddle_role_joined_to_one_family &&
         direct_star_cofaces_crosschecked_bijectively &&
         star_direct_cofaces_are_crosscheck_only_not_residual_references &&
         every_star_residual_coface_referenced_once &&
         every_star_coface_contributes_all_factorized_deletions &&
         facet_tokens_canonical_and_deduplicated &&
         unique_batch_per_exact_level_and_order &&
         direct_and_residual_same_level_order_share_one_future_snapshot &&
         batches_sorted_by_exact_level_then_order &&
         logical_storage_within_budget &&
         no_partial_scientific_payload_published &&
         no_k_plus_one_coface_key_persisted &&
         no_global_facet_or_coface_catalog_materialized &&
         !reducer_snapshot_or_forest_mutated &&
         !bounded_star_global_completeness_claimed && !public_status_claimed &&
         partial_refinement_only && structural_plan_receipt(*this) &&
         decision == ExactDirectSparseUnifiedLevelPlanDecision::
                         complete_bounded_unified_level_plan &&
         scope == ExactDirectSparseUnifiedLevelPlanScope::
                      bounded_unified_direct_and_residual_level_plan_of_supplied_higher_order_star_only;
}

ExactDirectSparseUnifiedLevelPlanResult
build_exact_direct_sparse_unified_level_plan(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult& incidence,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& star_budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& star,
    const ExactDirectSparseUnifiedLevelPlanBudget& budget) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the unified-level-plan LBVH has a different point authority");
  }
  require_valid_traversal_order(traversal_order);
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
        star_budget,
        traversal_order,
        star,
        budget);
  } catch (const std::bad_alloc&) {
    auto result = base_result(
        cloud,
        source_journal,
        incidence,
        star_budget,
        traversal_order,
        star,
        budget);
    result.decision =
        ExactDirectSparseUnifiedLevelPlanDecision::no_plan_allocation_failed;
    return result;
  } catch (const std::length_error&) {
    auto result = base_result(
        cloud,
        source_journal,
        incidence,
        star_budget,
        traversal_order,
        star,
        budget);
    result.decision =
        ExactDirectSparseUnifiedLevelPlanDecision::no_plan_allocation_failed;
    return result;
  }
}

ExactDirectSparseUnifiedLevelPlanVerification
verify_exact_direct_sparse_unified_level_plan(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult& incidence,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& star_budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& star,
    const ExactDirectSparseUnifiedLevelPlanBudget& budget,
    const ExactDirectSparseUnifiedLevelPlanResult& observed) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the unified-level-plan verifier has a different point authority");
  }
  require_valid_traversal_order(traversal_order);
  ExactDirectSparseUnifiedLevelPlanVerification verification;
  verification.observed_storage_within_budget =
      plan_storage_within_budget(observed, budget);
  if (!verification.observed_storage_within_budget) {
    return verification;
  }
  const auto expected = build_exact_direct_sparse_unified_level_plan(
      index,
      cloud,
      source_facade,
      source_journal,
      source_arm_budget,
      source_arm_journal,
      source_incidence_budget,
      incidence,
      star_budget,
      traversal_order,
      star,
      budget);
  verification.source_star_freshly_verified =
      expected.source_star_freshly_verified;
  verification.expected_result_freshly_reconstructed =
      expected.certified_bounded_plan() ||
      expected.decision != ExactDirectSparseUnifiedLevelPlanDecision::
                               not_certified;
  verification.observed_recursively_equal = observed == expected;
  verification.no_forbidden_global_structure_materialized =
      observed.no_partial_scientific_payload_published &&
      observed.no_k_plus_one_coface_key_persisted &&
      observed.no_global_facet_or_coface_catalog_materialized &&
      !observed.reducer_snapshot_or_forest_mutated &&
      !observed.bounded_star_global_completeness_claimed &&
      !observed.public_status_claimed;
  verification.fresh_replay_certified =
      verification.observed_storage_within_budget &&
      verification.source_star_freshly_verified &&
      verification.expected_result_freshly_reconstructed &&
      verification.observed_recursively_equal &&
      verification.no_forbidden_global_structure_materialized;
  verification.result_certified =
      verification.fresh_replay_certified &&
      expected.certified_bounded_plan();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
