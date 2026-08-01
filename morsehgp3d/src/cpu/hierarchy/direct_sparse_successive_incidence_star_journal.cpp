#include "morsehgp3d/hierarchy/direct_sparse_successive_incidence_star_journal.hpp"

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

struct DeletionScratch {
  ExactDirectSparseFacetKey facet_key{};
};

struct TransientCofaceKey {
  std::array<PointId, 11U> point_ids{};
  std::size_t point_count{};

  friend bool operator==(
      const TransientCofaceKey&, const TransientCofaceKey&) = default;
};

struct CofaceOccurrenceScratch {
  TransientCofaceKey coface_key{};
  std::size_t facet_token_index{};
  ExactDirectSparseSuccessiveIncidenceMinimizer minimizer{};
};

enum class BuildFailure : std::uint8_t {
  none,
  capacity_overflow,
  allocation_failed,
  budget_exhausted,
  source_not_certified,
  source_join_inconsistent,
  successive_incidence_budget_exhausted,
  successive_incidence_contradiction,
  direct_family_join_contradiction,
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

[[nodiscard]] bool child_work_cap_sum(
    const ExactDirectSparseSuccessiveIncidenceBudget& budget,
    std::size_t& sum) noexcept {
  sum = 0U;
  for (const std::size_t cap : {
           budget.maximum_source_support_enumeration_count,
           budget.maximum_node_visit_count,
           budget.maximum_internal_node_expansion_count,
           budget.maximum_exact_aabb_bound_evaluation_count,
           budget.maximum_exact_point_evaluation_count,
           budget.maximum_coface_support_enumeration_count,
           budget.maximum_candidate_point_classification_count}) {
    if (!try_add_size(sum, cap, sum)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool add_child_work(
    const ExactDirectSparseSuccessiveIncidenceAudit& audit,
    std::size_t aggregate_limit,
    std::size_t& aggregate_count) noexcept {
  for (const std::size_t count : {
           audit.source_support_enumeration_count,
           audit.node_visit_count,
           audit.internal_node_expansion_count,
           audit.exact_aabb_bound_evaluation_count,
           audit.exact_point_evaluation_count,
           audit.coface_support_enumeration_count,
           audit.candidate_point_classification_count}) {
    if (!try_add_size(aggregate_count, count, aggregate_count) ||
        aggregate_count > aggregate_limit) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool child_audit_within_budget(
    const ExactDirectSparseSuccessiveIncidenceAudit& audit,
    const ExactDirectSparseSuccessiveIncidenceBudget& budget) noexcept {
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
             budget.maximum_cominimizer_count &&
         audit.traversal_complete;
}

void require_valid_traversal_order(
    spatial::LbvhTraversalOrder traversal_order) {
  switch (traversal_order) {
    case spatial::LbvhTraversalOrder::near_first:
    case spatial::LbvhTraversalOrder::far_first:
      return;
  }
  throw std::invalid_argument(
      "a successive-incidence star traversal order is invalid");
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

[[nodiscard]] bool same_transient_key(
    const TransientCofaceKey& left,
    const TransientCofaceKey& right) noexcept {
  return left == right;
}

[[nodiscard]] ExactDirectSparseFacetKey facet_key_from(
    const ExactDirectSaddleArmFacet& facet) {
  if (facet.point_count == 0U ||
      facet.point_count > facet.point_ids.size()) {
    throw std::invalid_argument(
        "a successive-incidence star deletion has an invalid cardinality");
  }
  ExactDirectSparseFacetKey key;
  key.point_count = facet.point_count;
  std::copy_n(
      facet.point_ids.begin(), facet.point_count, key.point_ids.begin());
  return key;
}

[[nodiscard]] TransientCofaceKey merge_facet_and_point(
    const ExactDirectSparseFacetKey& facet,
    PointId added_point) {
  if (facet.point_count == 0U ||
      facet.point_count >= TransientCofaceKey{}.point_ids.size() ||
      std::binary_search(
          facet.point_ids.begin(),
          facet.point_ids.begin() +
              static_cast<std::ptrdiff_t>(facet.point_count),
          added_point)) {
    throw std::invalid_argument(
        "a factorized successive-incidence coface is inconsistent");
  }
  TransientCofaceKey key;
  key.point_count = facet.point_count + 1U;
  const auto facet_end =
      facet.point_ids.begin() + static_cast<std::ptrdiff_t>(facet.point_count);
  const auto insertion =
      std::lower_bound(facet.point_ids.begin(), facet_end, added_point);
  const std::size_t insertion_index =
      static_cast<std::size_t>(insertion - facet.point_ids.begin());
  std::copy_n(
      facet.point_ids.begin(), insertion_index, key.point_ids.begin());
  key.point_ids[insertion_index] = added_point;
  std::copy(
      insertion,
      facet_end,
      key.point_ids.begin() +
          static_cast<std::ptrdiff_t>(insertion_index + 1U));
  return key;
}

[[nodiscard]] TransientCofaceKey direct_family_key(
    const ExactDirectSupportEvent& event,
    std::size_t expected_order) {
  if (event.saddle_order != std::optional<std::size_t>{expected_order} ||
      event.closed_rank != expected_order + 1U ||
      event.closed_rank > 11U) {
    throw std::invalid_argument(
        "a direct saddle family has an inconsistent coface rank");
  }
  std::vector<PointId> ids;
  ids.reserve(event.closed_rank);
  ids.insert(
      ids.end(),
      event.support_ids.begin(),
      event.support_ids.begin() +
          static_cast<std::ptrdiff_t>(event.support_size));
  ids.insert(ids.end(), event.interior_ids.begin(), event.interior_ids.end());
  std::sort(ids.begin(), ids.end());
  if (ids.size() != event.closed_rank ||
      std::adjacent_find(ids.begin(), ids.end()) != ids.end()) {
    throw std::invalid_argument(
        "a direct saddle family coface is not a canonical set");
  }
  TransientCofaceKey key;
  key.point_count = ids.size();
  std::copy(ids.begin(), ids.end(), key.point_ids.begin());
  return key;
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

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStarJournalDecision
decision_for(BuildFailure failure) {
  switch (failure) {
    case BuildFailure::capacity_overflow:
      return ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
          no_star_capacity_overflow;
    case BuildFailure::allocation_failed:
      return ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
          no_star_allocation_failed;
    case BuildFailure::budget_exhausted:
      return ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
          no_star_budget_exhausted;
    case BuildFailure::source_not_certified:
      return ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
          no_star_source_not_certified;
    case BuildFailure::source_join_inconsistent:
      return ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
          no_star_source_join_inconsistent;
    case BuildFailure::successive_incidence_budget_exhausted:
      return ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
          no_star_successive_incidence_budget_exhausted;
    case BuildFailure::successive_incidence_contradiction:
      return ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
          no_star_successive_incidence_contradiction;
    case BuildFailure::direct_family_join_contradiction:
      return ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
          no_star_direct_family_join_contradiction;
    case BuildFailure::none:
      break;
  }
  throw std::logic_error("a star-journal failure has no decision");
}

void clear_scientific_payload(
    ExactDirectSparseSuccessiveIncidenceStarJournalResult& result) {
  result.facet_tokens.clear();
  result.shells.clear();
  result.cofaces.clear();
  result.residual_batches.clear();
  result.residual_batch_coface_indices.clear();
  result.logical_storage_entry_count = 0U;
  result.no_partial_scientific_payload_published = true;
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStarJournalResult fail(
    ExactDirectSparseSuccessiveIncidenceStarJournalResult result,
    BuildFailure failure) {
  clear_scientific_payload(result);
  result.decision = decision_for(failure);
  return result;
}

[[nodiscard]] bool add_required_count(
    std::size_t increment,
    std::size_t cap,
    std::size_t& count,
    BuildFailure& failure) noexcept {
  std::size_t sum = 0U;
  if (!try_add_size(count, increment, sum)) {
    failure = BuildFailure::capacity_overflow;
    return false;
  }
  count = sum;
  if (count > cap) {
    failure = BuildFailure::budget_exhausted;
    return false;
  }
  return true;
}

[[nodiscard]] bool add_logical_count(
    std::size_t increment,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& budget,
    std::size_t& count,
    BuildFailure& failure) noexcept {
  return add_required_count(
      increment,
      budget.maximum_logical_storage_entry_count,
      count,
      failure);
}

[[nodiscard]] bool scientific_storage_within_budget(
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& observed,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& budget)
    noexcept {
  if (observed.facet_tokens.size() > budget.maximum_distinct_facet_count ||
      observed.shells.size() > budget.maximum_incidence_shell_count ||
      observed.cofaces.size() > budget.maximum_distinct_coface_count ||
      observed.residual_batches.size() >
          budget.maximum_residual_batch_count ||
      observed.residual_batch_coface_indices.size() >
          budget.maximum_residual_batch_reference_count ||
      observed.residual_batch_coface_indices.size() > observed.cofaces.size()) {
    return false;
  }
  std::size_t key_points = 0U;
  std::size_t support_points = 0U;
  for (const auto& token : observed.facet_tokens) {
    if (token.source_facet_key.point_count == 0U ||
        token.source_facet_key.point_count >
            token.source_facet_key.point_ids.size() ||
        !try_add_size(
            key_points, token.source_facet_key.point_count, key_points)) {
      return false;
    }
  }
  for (const auto& coface : observed.cofaces) {
    if (coface.positive_support_point_count == 0U ||
        coface.positive_support_point_count >
            coface.positive_support_point_ids.size() ||
        !try_add_size(
            support_points,
            coface.positive_support_point_count,
            support_points)) {
      return false;
    }
  }
  if (key_points > budget.maximum_facet_key_point_count) {
    return false;
  }
  std::size_t logical = 0U;
  for (const std::size_t increment : {
           observed.facet_tokens.size(),
           key_points,
           observed.shells.size(),
           observed.cofaces.size(),
           support_points,
           observed.residual_batches.size(),
           observed.residual_batch_coface_indices.size()}) {
    if (!try_add_size(logical, increment, logical)) {
      return false;
    }
  }
  return logical <= budget.maximum_logical_storage_entry_count;
}

[[nodiscard]] bool canonical_facet_key(
    const ExactDirectSparseFacetKey& key,
    std::size_t point_count) noexcept {
  if (key.point_count == 0U || key.point_count > key.point_ids.size() ||
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
    const ExactDirectSparseSuccessiveIncidenceStarCoface& coface,
    std::size_t point_count) noexcept {
  if (coface.positive_support_point_count == 0U ||
      coface.positive_support_point_count >
          coface.positive_support_point_ids.size()) {
    return false;
  }
  for (std::size_t index = 0U;
       index < coface.positive_support_point_count;
       ++index) {
    if (static_cast<std::size_t>(
            coface.positive_support_point_ids[index]) >= point_count ||
        (index != 0U &&
         coface.positive_support_point_ids[index - 1U] >=
             coface.positive_support_point_ids[index])) {
      return false;
    }
  }
  for (std::size_t index = coface.positive_support_point_count;
       index < coface.positive_support_point_ids.size();
       ++index) {
    if (coface.positive_support_point_ids[index] != 0U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool scientific_payload_structurally_certified(
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& result)
    noexcept {
  const auto& budget = result.requested_budget;
  std::size_t expected_call_count = 0U;
  if (!try_add_size(
          result.shells.size(),
          result.facet_tokens.size(),
          expected_call_count)) {
    return false;
  }
  if (!scientific_storage_within_budget(result, budget) ||
      result.required_source_family_scan_count >
          budget.maximum_source_family_scan_count ||
      result.required_higher_order_family_count >
          result.required_source_family_scan_count ||
      result.excluded_order_one_family_count >
          result.required_source_family_scan_count ||
      result.required_higher_order_family_count !=
          result.required_source_family_scan_count -
              result.excluded_order_one_family_count ||
      result.required_deletion_reference_count >
          budget.maximum_deletion_reference_count ||
      result.required_distinct_facet_count != result.facet_tokens.size() ||
      result.required_facet_key_point_count >
          budget.maximum_facet_key_point_count ||
      result.required_successive_incidence_call_count >
          budget.maximum_successive_incidence_call_count ||
      result.required_incidence_shell_count != result.shells.size() ||
      result.required_coface_occurrence_count >
          budget.maximum_coface_occurrence_count ||
      result.required_distinct_coface_count != result.cofaces.size() ||
      result.required_direct_coface_count >
          budget.maximum_direct_coface_count ||
      result.required_residual_coface_count >
          budget.maximum_residual_coface_count ||
      result.required_residual_batch_count !=
          result.residual_batches.size() ||
      result.required_residual_batch_reference_count !=
          result.residual_batch_coface_indices.size() ||
      result.required_source_family_scan_count >
          result.source_direct_event_count ||
      result.logical_storage_entry_count >
          budget.maximum_logical_storage_entry_count ||
      result.aggregate_successive_work_entry_count >
          result.aggregate_successive_work_entry_limit ||
      result.required_successive_incidence_call_count !=
          expected_call_count) {
    return false;
  }

  std::size_t expected_shell_offset = 0U;
  std::size_t deletion_reference_sum = 0U;
  std::size_t facet_key_point_sum = 0U;
  std::size_t expected_occurrence_count = 0U;
  std::size_t owner_count_sum = 0U;
  std::size_t shell_minimizer_sum = 0U;
  std::size_t aggregate_work = 0U;
  for (std::size_t token_index = 0U;
       token_index < result.facet_tokens.size();
       ++token_index) {
    const auto& token = result.facet_tokens[token_index];
    if (token.facet_token_index != token_index ||
        !canonical_facet_key(token.source_facet_key, result.point_count) ||
        token.source_facet_key.point_count < 2U ||
        (token_index != 0U &&
         !facet_key_less(
             result.facet_tokens[token_index - 1U].source_facet_key,
             token.source_facet_key)) ||
        token.source_deletion_reference_count == 0U ||
        token.shell_offset != expected_shell_offset ||
        token.shell_count > result.shells.size() - expected_shell_offset ||
        !token.complete_no_later_coface ||
        !child_audit_within_budget(
            token.terminal_query_audit,
            budget.successive_incidence_budget) ||
        token.terminal_query_audit.eligible_coface_point_count !=
            result.point_count - token.source_facet_key.point_count ||
        !add_child_work(
            token.terminal_query_audit,
            result.aggregate_successive_work_entry_limit,
            aggregate_work) ||
        !try_add_size(
            deletion_reference_sum,
            token.source_deletion_reference_count,
            deletion_reference_sum) ||
        !try_add_size(
            facet_key_point_sum,
            token.source_facet_key.point_count,
            facet_key_point_sum) ||
        !try_add_size(
            expected_occurrence_count,
            result.point_count - token.source_facet_key.point_count,
            expected_occurrence_count) ||
        !try_add_size(
            owner_count_sum,
            token.canonical_owner_coface_count,
            owner_count_sum)) {
      return false;
    }
    std::optional<exact::ExactLevel> prior_level;
    for (std::size_t local = 0U; local < token.shell_count; ++local) {
      const std::size_t shell_index = token.shell_offset + local;
      const auto& shell = result.shells[shell_index];
      if (shell.shell_index != shell_index ||
          shell.facet_token_index != token_index ||
          shell.successive_query_index != local ||
          shell.minimizer_count == 0U ||
          (prior_level.has_value() &&
           !(*prior_level < shell.squared_level)) ||
          !child_audit_within_budget(
              shell.query_audit, budget.successive_incidence_budget) ||
          shell.query_audit.eligible_coface_point_count !=
              result.point_count - token.source_facet_key.point_count ||
          !add_child_work(
              shell.query_audit,
              result.aggregate_successive_work_entry_limit,
              aggregate_work) ||
          !try_add_size(
              shell_minimizer_sum,
              shell.minimizer_count,
              shell_minimizer_sum)) {
        return false;
      }
      prior_level = shell.squared_level;
    }
    if (!try_add_size(
            expected_shell_offset,
            token.shell_count,
            expected_shell_offset)) {
      return false;
    }
  }
  if (expected_shell_offset != result.shells.size() ||
      deletion_reference_sum != result.required_deletion_reference_count ||
      facet_key_point_sum != result.required_facet_key_point_count ||
      expected_occurrence_count !=
          result.required_coface_occurrence_count ||
      shell_minimizer_sum != result.required_coface_occurrence_count ||
      owner_count_sum != result.cofaces.size() ||
      aggregate_work != result.aggregate_successive_work_entry_count) {
    return false;
  }

  std::size_t support_point_sum = 0U;
  std::size_t occurrence_sum = 0U;
  std::size_t direct_count = 0U;
  std::size_t residual_count = 0U;
  std::optional<TransientCofaceKey> prior_coface_key;
  for (std::size_t coface_index = 0U;
       coface_index < result.cofaces.size();
       ++coface_index) {
    const auto& coface = result.cofaces[coface_index];
    if (coface.coface_index != coface_index ||
        coface.owner_facet_token_index >= result.facet_tokens.size() ||
        static_cast<std::size_t>(coface.added_point_id) >=
            result.point_count ||
        !canonical_support(coface, result.point_count) ||
        coface.supplied_facet_occurrence_count == 0U ||
        !try_add_size(
            occurrence_sum,
            coface.supplied_facet_occurrence_count,
            occurrence_sum) ||
        !try_add_size(
            support_point_sum,
            coface.positive_support_point_count,
            support_point_sum)) {
      return false;
    }
    TransientCofaceKey reconstructed;
    try {
      reconstructed = merge_facet_and_point(
          result.facet_tokens[coface.owner_facet_token_index]
              .source_facet_key,
          coface.added_point_id);
    } catch (...) {
      return false;
    }
    std::size_t deleting_facet_count = 0U;
    std::optional<std::size_t> canonical_owner;
    for (const auto& token : result.facet_tokens) {
      const bool deletes =
          token.source_facet_key.point_count + 1U ==
              reconstructed.point_count &&
          std::includes(
              reconstructed.point_ids.begin(),
              reconstructed.point_ids.begin() +
                  static_cast<std::ptrdiff_t>(reconstructed.point_count),
              token.source_facet_key.point_ids.begin(),
              token.source_facet_key.point_ids.begin() +
                  static_cast<std::ptrdiff_t>(
                      token.source_facet_key.point_count));
      if (deletes) {
        ++deleting_facet_count;
        if (!canonical_owner.has_value()) {
          canonical_owner = token.facet_token_index;
        }
      }
    }
    if ((prior_coface_key.has_value() &&
         !transient_key_less(*prior_coface_key, reconstructed)) ||
        !canonical_owner.has_value() ||
        *canonical_owner != coface.owner_facet_token_index ||
        deleting_facet_count != coface.supplied_facet_occurrence_count ||
        !std::includes(
            reconstructed.point_ids.begin(),
            reconstructed.point_ids.begin() +
                static_cast<std::ptrdiff_t>(reconstructed.point_count),
            coface.positive_support_point_ids.begin(),
            coface.positive_support_point_ids.begin() +
                static_cast<std::ptrdiff_t>(
                    coface.positive_support_point_count)) ||
        std::binary_search(
            coface.positive_support_point_ids.begin(),
            coface.positive_support_point_ids.begin() +
                static_cast<std::ptrdiff_t>(
                    coface.positive_support_point_count),
            coface.added_point_id) !=
            coface.added_point_in_selected_positive_support) {
      return false;
    }
    const auto& owner_token =
        result.facet_tokens[coface.owner_facet_token_index];
    bool owner_shell_level_found = false;
    for (std::size_t local = 0U; local < owner_token.shell_count; ++local) {
      owner_shell_level_found =
          owner_shell_level_found ||
          result.shells[owner_token.shell_offset + local].squared_level ==
              coface.squared_level;
    }
    if (!owner_shell_level_found) {
      return false;
    }
    prior_coface_key = reconstructed;
    if (coface.kind ==
        ExactDirectSparseSuccessiveIncidenceStarCofaceKind::direct_family) {
      if (coface.direct_family_match_count != 1U) {
        return false;
      }
      ++direct_count;
    } else if (
        coface.kind ==
        ExactDirectSparseSuccessiveIncidenceStarCofaceKind::residual) {
      if (coface.direct_family_match_count != 0U) {
        return false;
      }
      ++residual_count;
    } else {
      return false;
    }
  }
  std::size_t classified_count = 0U;
  if (!try_add_size(direct_count, residual_count, classified_count) ||
      occurrence_sum != result.required_coface_occurrence_count ||
      direct_count != result.required_direct_coface_count ||
      direct_count != result.required_higher_order_family_count ||
      residual_count != result.required_residual_coface_count ||
      classified_count != result.cofaces.size()) {
    return false;
  }

  std::size_t batch_reference_offset = 0U;
  std::size_t residual_references_seen = 0U;
  for (std::size_t batch_index = 0U;
       batch_index < result.residual_batches.size();
       ++batch_index) {
    const auto& batch = result.residual_batches[batch_index];
    if (batch.batch_index != batch_index || batch.order == 0U ||
        batch.residual_coface_index_offset != batch_reference_offset ||
        batch.residual_coface_index_count == 0U ||
        batch.residual_coface_index_count >
            result.residual_batch_coface_indices.size() -
                batch_reference_offset ||
        (batch_index != 0U &&
         !(result.residual_batches[batch_index - 1U].order < batch.order ||
           (result.residual_batches[batch_index - 1U].order == batch.order &&
            result.residual_batches[batch_index - 1U].squared_level <
                batch.squared_level)))) {
      return false;
    }
    std::optional<std::size_t> prior_index;
    for (std::size_t local = 0U;
         local < batch.residual_coface_index_count;
         ++local) {
      const std::size_t reference_index = batch_reference_offset + local;
      const std::size_t coface_index =
          result.residual_batch_coface_indices[reference_index];
      if (coface_index >= result.cofaces.size() ||
          (prior_index.has_value() && !(*prior_index < coface_index))) {
        return false;
      }
      const auto& coface = result.cofaces[coface_index];
      if (coface.kind !=
              ExactDirectSparseSuccessiveIncidenceStarCofaceKind::residual ||
          result.facet_tokens[coface.owner_facet_token_index]
                  .source_facet_key.point_count != batch.order ||
          coface.squared_level != batch.squared_level) {
        return false;
      }
      prior_index = coface_index;
      ++residual_references_seen;
    }
    if (!try_add_size(
            batch_reference_offset,
            batch.residual_coface_index_count,
            batch_reference_offset)) {
      return false;
    }
  }
  if (batch_reference_offset !=
          result.residual_batch_coface_indices.size() ||
      residual_references_seen != residual_count) {
    return false;
  }
  for (const auto& coface : result.cofaces) {
    if (coface.kind !=
        ExactDirectSparseSuccessiveIncidenceStarCofaceKind::residual) {
      continue;
    }
    if (std::count(
            result.residual_batch_coface_indices.begin(),
            result.residual_batch_coface_indices.end(),
            coface.coface_index) != 1) {
      return false;
    }
  }

  std::size_t logical = 0U;
  for (const std::size_t increment : {
           result.facet_tokens.size(),
           facet_key_point_sum,
           result.shells.size(),
           result.cofaces.size(),
           support_point_sum,
           result.residual_batches.size(),
           result.residual_batch_coface_indices.size()}) {
    if (!try_add_size(logical, increment, logical)) {
      return false;
    }
  }
  return logical == result.logical_storage_entry_count;
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStarJournalResult
build_impl(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& budget,
    spatial::LbvhTraversalOrder traversal_order) {
  ExactDirectSparseSuccessiveIncidenceStarJournalResult result;
  result.requested_budget = budget;
  result.traversal_order = traversal_order;
  result.point_count = cloud.size();
  result.source_direct_event_count = source_facade.events.size();
  result.scope = ExactDirectSparseSuccessiveIncidenceStarJournalScope::
      bounded_successive_incidence_star_of_supplied_direct_higher_order_facets_only;
  result.source_pair_canonical_cloud_digest =
      source_incidence_journal.source_pair_canonical_cloud_digest;
  result.source_higher_canonical_cloud_digest =
      source_incidence_journal.source_higher_canonical_cloud_digest;
  result.source_pair_semantic_digest =
      source_incidence_journal.source_pair_semantic_digest;
  result.source_higher_semantic_digest =
      source_incidence_journal.source_higher_semantic_digest;
  result.no_partial_scientific_payload_published = true;
  result.no_global_facet_or_coface_catalog_materialized = true;
  result.transient_union_keys_released_before_publication = true;
  result.persistent_k_plus_one_keys_materialized = false;
  result.gamma_cells_or_higher_order_delaunay_materialized = false;
  result.hierarchy_or_forest_mutated = false;
  result.global_gamma_completeness_claimed = false;
  result.product_sparse_silent_source_complete = false;
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
    return fail(std::move(result), BuildFailure::source_not_certified);
  }
  result.source_incidence_journal_freshly_replayed = true;
  result.required_source_family_scan_count =
      source_incidence_journal.families.size();
  if (result.required_source_family_scan_count >
      budget.maximum_source_family_scan_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }

  for (const auto& family : source_incidence_journal.families) {
    if (family.order == 1U) {
      ++result.excluded_order_one_family_count;
      continue;
    }
    if (family.order < 2U) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    ++result.required_higher_order_family_count;
    std::size_t family_deletion_count = 0U;
    if (!try_add_size(
            family.strict_arm_seed_count,
            family.equal_level_facet_seed_count,
            family_deletion_count) ||
        !try_add_size(
            result.required_deletion_reference_count,
            family_deletion_count,
            result.required_deletion_reference_count)) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
  }
  if (result.required_higher_order_family_count !=
      result.required_source_family_scan_count -
          result.excluded_order_one_family_count) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }
  result.order_one_families_excluded_to_preserve_boruvka_authority = true;
  if (result.required_deletion_reference_count >
      budget.maximum_deletion_reference_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }

  std::vector<DeletionScratch> deletions;
  deletions.reserve(result.required_deletion_reference_count);
  std::vector<TransientCofaceKey> direct_family_keys;
  direct_family_keys.reserve(result.required_higher_order_family_count);
  for (const auto& family : source_incidence_journal.families) {
    if (family.order == 1U) {
      continue;
    }
    if (family.family_index >= source_incidence_journal.families.size() ||
        family.source_event_index >= source_facade.events.size() ||
        family.source_arm_family_index >= source_arm_journal.families.size()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    direct_family_keys.push_back(direct_family_key(
        source_facade.events[family.source_event_index], family.order));
    for (std::size_t local = 0U; local < family.strict_arm_seed_count;
         ++local) {
      std::size_t seed_index = 0U;
      if (!try_add_size(family.strict_arm_seed_offset, local, seed_index) ||
          seed_index >= source_arm_journal.arm_seeds.size()) {
        return fail(std::move(result), BuildFailure::source_join_inconsistent);
      }
      deletions.push_back(DeletionScratch{facet_key_from(
          reconstruct_exact_direct_saddle_arm_facet(
              source_facade, source_arm_journal, seed_index))});
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
      deletions.push_back(DeletionScratch{facet_key_from(
          reconstruct_exact_direct_equal_level_saddle_facet(
              source_facade,
              source_arm_journal,
              source_incidence_journal,
              seed_index))});
    }
  }
  if (deletions.size() != result.required_deletion_reference_count ||
      direct_family_keys.size() !=
          result.required_higher_order_family_count) {
    return fail(std::move(result), BuildFailure::source_join_inconsistent);
  }
  std::sort(
      deletions.begin(),
      deletions.end(),
      [](const DeletionScratch& left, const DeletionScratch& right) {
        return facet_key_less(left.facet_key, right.facet_key);
      });
  std::sort(
      direct_family_keys.begin(),
      direct_family_keys.end(),
      transient_key_less);
  if (std::adjacent_find(
          direct_family_keys.begin(), direct_family_keys.end()) !=
      direct_family_keys.end()) {
    return fail(
        std::move(result), BuildFailure::direct_family_join_contradiction);
  }
  result.every_strict_and_equal_deletion_reconstructed = true;

  for (std::size_t begin = 0U; begin < deletions.size();) {
    std::size_t end = begin + 1U;
    while (end < deletions.size() &&
           deletions[begin].facet_key == deletions[end].facet_key) {
      ++end;
    }
    if (!try_add_size(
            result.required_facet_key_point_count,
            deletions[begin].facet_key.point_count,
            result.required_facet_key_point_count)) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
    ++result.required_distinct_facet_count;
    begin = end;
  }
  if (result.required_distinct_facet_count >
          budget.maximum_distinct_facet_count ||
      result.required_facet_key_point_count >
          budget.maximum_facet_key_point_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }
  result.distinct_source_facets_deduplicated = true;
  std::size_t per_call_work_limit = 0U;
  if (!child_work_cap_sum(
          budget.successive_incidence_budget, per_call_work_limit) ||
      !try_multiply_size(
          budget.maximum_successive_incidence_call_count,
          per_call_work_limit,
          result.aggregate_successive_work_entry_limit)) {
    return fail(std::move(result), BuildFailure::capacity_overflow);
  }
  result.budget_preflight_certified = true;

  result.facet_tokens.reserve(result.required_distinct_facet_count);
  result.shells.reserve(std::min(
      budget.maximum_incidence_shell_count, std::size_t{64U}));
  std::vector<CofaceOccurrenceScratch> occurrences;
  occurrences.reserve(std::min(
      budget.maximum_coface_occurrence_count, std::size_t{128U}));

  BuildFailure counting_failure = BuildFailure::none;
  std::size_t deletion_begin = 0U;
  while (deletion_begin < deletions.size()) {
    std::size_t deletion_end = deletion_begin + 1U;
    while (deletion_end < deletions.size() &&
           deletions[deletion_begin].facet_key ==
               deletions[deletion_end].facet_key) {
      ++deletion_end;
    }
    const std::size_t token_index = result.facet_tokens.size();
    ExactDirectSparseSuccessiveIncidenceStarFacetToken token;
    token.facet_token_index = token_index;
    token.source_facet_key = deletions[deletion_begin].facet_key;
    token.source_deletion_reference_count = deletion_end - deletion_begin;
    token.shell_offset = result.shells.size();

    std::optional<exact::ExactLevel> threshold;
    std::optional<exact::ExactLevel> source_miniball_level;
    std::size_t query_index = 0U;
    for (;;) {
      if (!add_required_count(
              1U,
              budget.maximum_successive_incidence_call_count,
              result.required_successive_incidence_call_count,
              counting_failure)) {
        return fail(std::move(result), counting_failure);
      }
      const auto next = build_exact_direct_sparse_successive_incidence(
          index,
          cloud,
          token.source_facet_key,
          threshold,
          budget.successive_incidence_budget,
          traversal_order);
      if (!add_child_work(
              next.audit,
              result.aggregate_successive_work_entry_limit,
              result.aggregate_successive_work_entry_count)) {
        return fail(std::move(result), BuildFailure::budget_exhausted);
      }
      if (next.certified_budget_exhaustion()) {
        return fail(
            std::move(result),
            BuildFailure::successive_incidence_budget_exhausted);
      }
      if (!next.source_facet_miniball.has_value()) {
        return fail(
            std::move(result),
            BuildFailure::successive_incidence_contradiction);
      }
      if (!source_miniball_level.has_value()) {
        source_miniball_level =
            next.source_facet_miniball->squared_radius;
      } else if (*source_miniball_level !=
                 next.source_facet_miniball->squared_radius) {
        return fail(
            std::move(result),
            BuildFailure::successive_incidence_contradiction);
      }
      if (next.certified_complete_no_strictly_higher_coface()) {
        if (next.next_incidence_squared_level.has_value() ||
            !next.cominimizers.empty()) {
          return fail(
              std::move(result),
              BuildFailure::successive_incidence_contradiction);
        }
        token.terminal_query_audit = next.audit;
        token.complete_no_later_coface = true;
        break;
      }
      if (!next.certified_complete_next_incidence() ||
          !next.next_incidence_squared_level.has_value() ||
          next.cominimizers.empty() ||
          (threshold.has_value() &&
           !(*threshold < *next.next_incidence_squared_level))) {
        return fail(
            std::move(result),
            BuildFailure::successive_incidence_contradiction);
      }
      if (!add_required_count(
              1U,
              budget.maximum_incidence_shell_count,
              result.required_incidence_shell_count,
              counting_failure) ||
          !add_required_count(
              next.cominimizers.size(),
              budget.maximum_coface_occurrence_count,
              result.required_coface_occurrence_count,
              counting_failure)) {
        return fail(std::move(result), counting_failure);
      }
      result.shells.push_back(
          ExactDirectSparseSuccessiveIncidenceStarShell{
              result.shells.size(),
              token_index,
              query_index,
              *next.next_incidence_squared_level,
              next.cominimizers.size(),
              next.audit});
      for (const auto& minimizer : next.cominimizers) {
        if (minimizer.squared_level !=
            *next.next_incidence_squared_level) {
          return fail(
              std::move(result),
              BuildFailure::successive_incidence_contradiction);
        }
        occurrences.push_back(CofaceOccurrenceScratch{
            merge_facet_and_point(
                token.source_facet_key, minimizer.added_point_id),
            token_index,
            minimizer});
      }
      threshold = next.next_incidence_squared_level;
      ++query_index;
    }
    if (!source_miniball_level.has_value()) {
      return fail(
          std::move(result),
          BuildFailure::successive_incidence_contradiction);
    }
    token.source_miniball_squared_level = *source_miniball_level;
    token.shell_count = result.shells.size() - token.shell_offset;
    result.facet_tokens.push_back(std::move(token));
    deletion_begin = deletion_end;
  }
  result.every_facet_queried_through_complete_no_later_coface = true;
  result.aggregate_successive_work_within_derived_limit = true;
  result.successive_shell_levels_strictly_increasing = true;
  result.all_equal_level_minimizers_retained_atomically = true;

  std::sort(
      occurrences.begin(),
      occurrences.end(),
      [](const CofaceOccurrenceScratch& left,
         const CofaceOccurrenceScratch& right) {
        if (transient_key_less(left.coface_key, right.coface_key)) {
          return true;
        }
        if (transient_key_less(right.coface_key, left.coface_key)) {
          return false;
        }
        if (left.facet_token_index != right.facet_token_index) {
          return left.facet_token_index < right.facet_token_index;
        }
        return left.minimizer.added_point_id <
               right.minimizer.added_point_id;
      });
  result.cofaces.reserve(std::min(
      budget.maximum_distinct_coface_count, occurrences.size()));

  for (std::size_t begin = 0U; begin < occurrences.size();) {
    std::size_t end = begin + 1U;
    while (end < occurrences.size() &&
           same_transient_key(
               occurrences[begin].coface_key,
               occurrences[end].coface_key)) {
      if (occurrences[end].minimizer.squared_level !=
              occurrences[begin].minimizer.squared_level ||
          occurrences[end].facet_token_index ==
              occurrences[end - 1U].facet_token_index) {
        return fail(
            std::move(result),
            BuildFailure::successive_incidence_contradiction);
      }
      ++end;
    }
    if (!add_required_count(
            1U,
            budget.maximum_distinct_coface_count,
            result.required_distinct_coface_count,
            counting_failure)) {
      return fail(std::move(result), counting_failure);
    }
    const auto direct_begin = std::lower_bound(
        direct_family_keys.begin(),
        direct_family_keys.end(),
        occurrences[begin].coface_key,
        transient_key_less);
    const auto direct_end = std::upper_bound(
        direct_begin,
        direct_family_keys.end(),
        occurrences[begin].coface_key,
        [](const TransientCofaceKey& key,
           const TransientCofaceKey& candidate) {
          return transient_key_less(key, candidate);
        });
    const std::size_t direct_match_count =
        static_cast<std::size_t>(direct_end - direct_begin);
    const bool is_direct = direct_match_count != 0U;
    std::size_t& kind_count =
        is_direct ? result.required_direct_coface_count
                  : result.required_residual_coface_count;
    const std::size_t kind_cap =
        is_direct ? budget.maximum_direct_coface_count
                  : budget.maximum_residual_coface_count;
    if (!add_required_count(
            1U, kind_cap, kind_count, counting_failure)) {
      return fail(std::move(result), counting_failure);
    }
    const CofaceOccurrenceScratch& owner = occurrences[begin];
    if (owner.facet_token_index >= result.facet_tokens.size()) {
      return fail(std::move(result), BuildFailure::source_join_inconsistent);
    }
    ++result.facet_tokens[owner.facet_token_index]
          .canonical_owner_coface_count;
    result.cofaces.push_back(
        ExactDirectSparseSuccessiveIncidenceStarCoface{
            result.cofaces.size(),
            owner.facet_token_index,
            owner.minimizer.added_point_id,
            owner.minimizer.support_point_ids,
            owner.minimizer.support_point_count,
            owner.minimizer.squared_level,
            end - begin,
            direct_match_count,
            is_direct
                ? ExactDirectSparseSuccessiveIncidenceStarCofaceKind::
                      direct_family
                : ExactDirectSparseSuccessiveIncidenceStarCofaceKind::
                      residual,
            owner.minimizer.added_point_in_source_closed_ball,
            owner.minimizer.added_point_in_selected_positive_support});
    begin = end;
  }
  result.every_factorized_occurrence_deduplicated_once = true;
  result.canonical_owner_is_lexicographically_first_supplied_facet = true;
  if (result.required_direct_coface_count != direct_family_keys.size()) {
    return fail(
        std::move(result), BuildFailure::direct_family_join_contradiction);
  }
  result.every_coface_joined_exactly_to_direct_families = true;

  std::vector<std::size_t> residual_indices;
  residual_indices.reserve(result.required_residual_coface_count);
  for (const auto& coface : result.cofaces) {
    if (coface.kind ==
        ExactDirectSparseSuccessiveIncidenceStarCofaceKind::residual) {
      residual_indices.push_back(coface.coface_index);
    }
  }
  std::sort(
      residual_indices.begin(),
      residual_indices.end(),
      [&result](std::size_t left_index, std::size_t right_index) {
        const auto& left = result.cofaces[left_index];
        const auto& right = result.cofaces[right_index];
        const std::size_t left_order =
            result.facet_tokens[left.owner_facet_token_index]
                .source_facet_key.point_count;
        const std::size_t right_order =
            result.facet_tokens[right.owner_facet_token_index]
                .source_facet_key.point_count;
        if (left_order != right_order) {
          return left_order < right_order;
        }
        if (left.squared_level != right.squared_level) {
          return left.squared_level < right.squared_level;
        }
        return left.coface_index < right.coface_index;
      });
  result.required_residual_batch_reference_count = residual_indices.size();
  if (result.required_residual_batch_reference_count >
      budget.maximum_residual_batch_reference_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted);
  }
  for (std::size_t begin = 0U; begin < residual_indices.size();) {
    const auto& first = result.cofaces[residual_indices[begin]];
    const std::size_t order =
        result.facet_tokens[first.owner_facet_token_index]
            .source_facet_key.point_count;
    std::size_t end = begin + 1U;
    while (end < residual_indices.size()) {
      const auto& candidate = result.cofaces[residual_indices[end]];
      const std::size_t candidate_order =
          result.facet_tokens[candidate.owner_facet_token_index]
              .source_facet_key.point_count;
      if (candidate_order != order ||
          candidate.squared_level != first.squared_level) {
        break;
      }
      ++end;
    }
    if (!add_required_count(
            1U,
            budget.maximum_residual_batch_count,
            result.required_residual_batch_count,
            counting_failure)) {
      return fail(std::move(result), counting_failure);
    }
    result.residual_batches.push_back(
        ExactDirectSparseSuccessiveIncidenceStarResidualBatch{
            result.residual_batches.size(),
            order,
            first.squared_level,
            result.residual_batch_coface_indices.size(),
            end - begin});
    result.residual_batch_coface_indices.insert(
        result.residual_batch_coface_indices.end(),
        residual_indices.begin() + static_cast<std::ptrdiff_t>(begin),
        residual_indices.begin() + static_cast<std::ptrdiff_t>(end));
    begin = end;
  }
  result.residual_batches_canonical_and_complete_for_this_star = true;

  std::size_t support_point_count = 0U;
  for (const auto& coface : result.cofaces) {
    if (!try_add_size(
            support_point_count,
            coface.positive_support_point_count,
            support_point_count)) {
      return fail(std::move(result), BuildFailure::capacity_overflow);
    }
  }
  for (const std::size_t increment : {
           result.facet_tokens.size(),
           result.required_facet_key_point_count,
           result.shells.size(),
           result.cofaces.size(),
           support_point_count,
           result.residual_batches.size(),
           result.residual_batch_coface_indices.size()}) {
    if (!add_logical_count(
            increment,
            budget,
            result.logical_storage_entry_count,
            counting_failure)) {
      return fail(std::move(result), counting_failure);
    }
  }
  result.logical_storage_within_budget = true;
  result.decision = ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
      complete_bounded_successive_incidence_star_journal;
  return result;
}

}  // namespace

bool ExactDirectSparseSuccessiveIncidenceStarJournalResult::
    certified_bounded_star() const noexcept {
  return schema_version ==
             direct_sparse_successive_incidence_star_journal_schema_version &&
         budget_preflight_certified &&
         source_incidence_journal_freshly_replayed &&
         order_one_families_excluded_to_preserve_boruvka_authority &&
         every_strict_and_equal_deletion_reconstructed &&
         distinct_source_facets_deduplicated &&
         every_facet_queried_through_complete_no_later_coface &&
         aggregate_successive_work_within_derived_limit &&
         successive_shell_levels_strictly_increasing &&
         all_equal_level_minimizers_retained_atomically &&
         every_factorized_occurrence_deduplicated_once &&
         canonical_owner_is_lexicographically_first_supplied_facet &&
         every_coface_joined_exactly_to_direct_families &&
         residual_batches_canonical_and_complete_for_this_star &&
         logical_storage_within_budget &&
         no_partial_scientific_payload_published &&
         scientific_payload_structurally_certified(*this) &&
         no_global_facet_or_coface_catalog_materialized &&
         transient_union_keys_released_before_publication &&
         !persistent_k_plus_one_keys_materialized &&
         !gamma_cells_or_higher_order_delaunay_materialized &&
         !hierarchy_or_forest_mutated && !global_gamma_completeness_claimed &&
         !product_sparse_silent_source_complete && !public_status_claimed &&
         partial_refinement_only &&
         decision ==
             ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
                 complete_bounded_successive_incidence_star_journal &&
         scope == ExactDirectSparseSuccessiveIncidenceStarJournalScope::
                      bounded_successive_incidence_star_of_supplied_direct_higher_order_facets_only;
}

ExactDirectSparseSuccessiveIncidenceStarJournalResult
build_exact_direct_sparse_successive_incidence_star_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& budget,
    spatial::LbvhTraversalOrder traversal_order) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the successive-incidence star LBVH has a different point authority");
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
        source_incidence_journal,
        budget,
        traversal_order);
  } catch (const std::bad_alloc&) {
    ExactDirectSparseSuccessiveIncidenceStarJournalResult result;
    result.requested_budget = budget;
    result.traversal_order = traversal_order;
    result.point_count = cloud.size();
    result.source_direct_event_count = source_facade.events.size();
    result.scope = ExactDirectSparseSuccessiveIncidenceStarJournalScope::
        bounded_successive_incidence_star_of_supplied_direct_higher_order_facets_only;
    result.no_partial_scientific_payload_published = true;
    result.no_global_facet_or_coface_catalog_materialized = true;
    result.transient_union_keys_released_before_publication = true;
    result.persistent_k_plus_one_keys_materialized = false;
    result.gamma_cells_or_higher_order_delaunay_materialized = false;
    result.hierarchy_or_forest_mutated = false;
    result.global_gamma_completeness_claimed = false;
    result.product_sparse_silent_source_complete = false;
    result.public_status_claimed = false;
    result.partial_refinement_only = true;
    result.decision =
        ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
            no_star_allocation_failed;
    return result;
  }
}

ExactDirectSparseSuccessiveIncidenceStarReconstructedCoface
reconstruct_exact_direct_sparse_successive_incidence_star_coface(
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& journal,
    std::size_t coface_index) {
  if (coface_index >= journal.cofaces.size()) {
    throw std::out_of_range(
        "a star-journal coface index is outside the journal");
  }
  const auto& coface = journal.cofaces[coface_index];
  if (coface.coface_index != coface_index ||
      coface.owner_facet_token_index >= journal.facet_tokens.size()) {
    throw std::invalid_argument(
        "a star-journal coface has inconsistent factorized indexing");
  }
  const auto transient = merge_facet_and_point(
      journal.facet_tokens[coface.owner_facet_token_index].source_facet_key,
      coface.added_point_id);
  ExactDirectSparseSuccessiveIncidenceStarReconstructedCoface reconstructed;
  reconstructed.point_count = transient.point_count;
  std::copy_n(
      transient.point_ids.begin(),
      transient.point_count,
      reconstructed.point_ids.begin());
  return reconstructed;
}

ExactDirectSparseSuccessiveIncidenceStarJournalVerification
verify_exact_direct_sparse_successive_incidence_star_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& observed) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the successive-incidence star verifier has a different point authority");
  }
  require_valid_traversal_order(traversal_order);
  ExactDirectSparseSuccessiveIncidenceStarJournalVerification verification;
  verification.observed_storage_within_budget =
      scientific_storage_within_budget(observed, budget);
  if (!verification.observed_storage_within_budget) {
    return verification;
  }
  const auto expected =
      build_exact_direct_sparse_successive_incidence_star_journal(
          index,
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal,
          source_incidence_budget,
          source_incidence_journal,
          budget,
          traversal_order);
  verification.source_incidence_journal_freshly_replayed =
      expected.source_incidence_journal_freshly_replayed;
  verification.expected_result_freshly_reconstructed =
      expected.certified_bounded_star() ||
      expected.decision !=
          ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
              not_certified;
  verification.observed_recursively_equal = observed == expected;
  verification.no_forbidden_global_structure_materialized =
      observed.no_global_facet_or_coface_catalog_materialized &&
      observed.transient_union_keys_released_before_publication &&
      !observed.persistent_k_plus_one_keys_materialized &&
      !observed.gamma_cells_or_higher_order_delaunay_materialized &&
      !observed.hierarchy_or_forest_mutated &&
      !observed.global_gamma_completeness_claimed &&
      !observed.product_sparse_silent_source_complete &&
      !observed.public_status_claimed;
  verification.fresh_replay_certified =
      verification.observed_storage_within_budget &&
      verification.source_incidence_journal_freshly_replayed &&
      verification.expected_result_freshly_reconstructed &&
      verification.observed_recursively_equal &&
      verification.no_forbidden_global_structure_materialized;
  verification.result_certified =
      verification.fresh_replay_certified &&
      expected.certified_bounded_star();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
