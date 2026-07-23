#include "morsehgp3d/hierarchy/direct_sparse_gateway_temporal_resolution.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <new>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

struct PrefixTokenProjection {
  std::size_t prefix{};
  std::size_t token{};
  std::size_t projection{};
};

struct BuildArtifacts {
  std::vector<ExactDirectSparsePositiveFacetPrefixQuery> queries;
  ExactDirectSparsePositiveFacetPrefixSweepResult prefix_sweep;
};

enum class BuildFailure : std::uint8_t {
  capacity_overflow,
  budget_exhausted,
  allocation_failed,
  source_rejected,
  prefix_sweep_rejected,
  append_only_contradiction,
};

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] bool checked_multiply(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  result = left * right;
  return true;
}

[[nodiscard]] bool increment(std::size_t& value) noexcept {
  return checked_add(value, 1U, value);
}

[[nodiscard]] bool non_scope_is_honest(
    const ExactDirectSparseGatewayTemporalResolutionResult& result)
    noexcept {
  return !result.nested_verifiers_replayed &&
         !result.external_clock_authority_replayed &&
         !result.external_binding_authority_replayed &&
         result.conditional_on_caller_external_binding_authority_replay &&
         result.conditional_on_caller_strict_pre_lot_orchestration &&
         !result.external_freeze_synchronization_replayed &&
         result.conditional_on_caller_external_freeze_synchronization &&
         result.in_memory_replay_only && !result.crash_durable &&
         !result.missing_facet_means_isolated &&
         !result.singleton_component_created &&
         !result.quotient_root_union_or_forest_mutated &&
         !result.gateway_attach_published &&
         !result.gamma_cells_or_higher_order_delaunay_materialized &&
         !result.forbidden_global_structure_materialized &&
         !result.public_status_claimed && result.partial_refinement_only &&
         result.scope ==
             ExactDirectSparseGatewayTemporalResolutionScope::
                 source_batch_strict_pre_locator_prefixes_cross_localized_deletion_facets_only;
}

void initialize_scope(
    ExactDirectSparseGatewayTemporalResolutionResult& result) noexcept {
  result.scope = ExactDirectSparseGatewayTemporalResolutionScope::
      source_batch_strict_pre_locator_prefixes_cross_localized_deletion_facets_only;
  result.no_partial_scientific_payload_published = true;
}

void clear_payload(
    ExactDirectSparseGatewayTemporalResolutionResult& result) noexcept {
  result.projection_to_resolution.clear();
  result.temporal_resolutions.clear();
  result.logical_output_entry_count = 0U;
  result.no_partial_scientific_payload_published = true;
}

[[nodiscard]] ExactDirectSparseGatewayTemporalResolutionDecision
decision_for(BuildFailure failure) noexcept {
  switch (failure) {
    case BuildFailure::capacity_overflow:
      return ExactDirectSparseGatewayTemporalResolutionDecision::
          no_temporal_resolution_capacity_overflow;
    case BuildFailure::budget_exhausted:
      return ExactDirectSparseGatewayTemporalResolutionDecision::
          no_temporal_resolution_budget_exhausted;
    case BuildFailure::allocation_failed:
      return ExactDirectSparseGatewayTemporalResolutionDecision::
          no_temporal_resolution_allocation_failed;
    case BuildFailure::source_rejected:
      return ExactDirectSparseGatewayTemporalResolutionDecision::
          no_temporal_resolution_source_rejected;
    case BuildFailure::prefix_sweep_rejected:
      return ExactDirectSparseGatewayTemporalResolutionDecision::
          no_temporal_resolution_prefix_sweep_rejected;
    case BuildFailure::append_only_contradiction:
      return ExactDirectSparseGatewayTemporalResolutionDecision::
          no_temporal_resolution_append_only_contradiction;
  }
  return ExactDirectSparseGatewayTemporalResolutionDecision::not_certified;
}

void check_snapshot(
    const ExactDirectSparsePositiveFacetLocator& locator,
    ExactDirectSparseGatewayTemporalResolutionResult& result) {
  if (!increment(result.counters.locator_snapshot_check_count)) {
    throw std::overflow_error(
        "a gateway temporal-resolution snapshot counter overflowed");
  }
  if (locator.snapshot_stamp() != result.locator_snapshot_stamp) {
    clear_payload(result);
    throw std::runtime_error(
        "the positive-facet locator changed during temporal resolution");
  }
}

[[nodiscard]] ExactDirectSparseGatewayTemporalResolutionResult fail(
    ExactDirectSparseGatewayTemporalResolutionResult result,
    BuildFailure failure,
    const ExactDirectSparsePositiveFacetLocator& locator) {
  clear_payload(result);
  result.decision = decision_for(failure);
  check_snapshot(locator, result);
  result.common_frozen_locator_snapshot_certified = true;
  return result;
}

class BoundedComparator {
 public:
  explicit BoundedComparator(std::size_t maximum) noexcept
      : maximum_(maximum) {}

  [[nodiscard]] bool less(
      const PrefixTokenProjection& left,
      const PrefixTokenProjection& right,
      bool& result) noexcept {
    if (comparison_count_ == maximum_) {
      exhausted_ = true;
      return false;
    }
    ++comparison_count_;
    if (left.prefix != right.prefix) {
      result = left.prefix < right.prefix;
    } else if (left.token != right.token) {
      result = left.token < right.token;
    } else {
      result = left.projection < right.projection;
    }
    return true;
  }

  [[nodiscard]] std::size_t comparison_count() const noexcept {
    return comparison_count_;
  }

 private:
  std::size_t maximum_{};
  std::size_t comparison_count_{};
  bool exhausted_{false};
};

[[nodiscard]] bool sift_down(
    std::span<PrefixTokenProjection> entries,
    std::size_t root,
    std::size_t heap_size,
    BoundedComparator& comparator) noexcept {
  while (root < heap_size / 2U) {
    const std::size_t left = root * 2U + 1U;
    std::size_t greatest = left;
    const std::size_t right = left + 1U;
    if (right < heap_size) {
      bool left_less = false;
      if (!comparator.less(entries[left], entries[right], left_less)) {
        return false;
      }
      if (left_less) {
        greatest = right;
      }
    }
    bool root_less = false;
    if (!comparator.less(entries[root], entries[greatest], root_less)) {
      return false;
    }
    if (!root_less) {
      return true;
    }
    std::swap(entries[root], entries[greatest]);
    root = greatest;
  }
  return true;
}

[[nodiscard]] bool bounded_heapsort(
    std::span<PrefixTokenProjection> entries,
    BoundedComparator& comparator) noexcept {
  if (entries.size() < 2U) {
    return true;
  }
  for (std::size_t index = entries.size() / 2U; index > 0U; --index) {
    if (!sift_down(entries, index - 1U, entries.size(), comparator)) {
      return false;
    }
  }
  for (std::size_t size = entries.size(); size > 1U; --size) {
    std::swap(entries[0U], entries[size - 1U]);
    if (!sift_down(entries, 0U, size - 1U, comparator)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool same_pair(
    const PrefixTokenProjection& left,
    const PrefixTokenProjection& right) noexcept {
  return left.prefix == right.prefix && left.token == right.token;
}

[[nodiscard]] bool observed_storage_within_budget(
    const ExactDirectSparseGatewayTemporalResolutionResult& observed,
    const ExactDirectSparseGatewayTemporalResolutionBudget& budget)
    noexcept {
  std::size_t logical = 0U;
  return observed.projection_to_resolution.size() <=
             budget.maximum_projection_output_count &&
         observed.temporal_resolutions.size() <=
             budget.maximum_temporal_resolution_output_count &&
         checked_add(
             observed.projection_to_resolution.size(),
             observed.temporal_resolutions.size(),
             logical) &&
         logical == observed.logical_output_entry_count &&
         logical <= budget.maximum_logical_output_entry_count;
}

[[nodiscard]] bool scientific_payload_is_well_formed(
    const ExactDirectSparseGatewayTemporalResolutionResult& result)
    noexcept {
  if (result.projection_to_resolution.size() !=
          result.required_projection_count ||
      result.temporal_resolutions.size() !=
          result.required_resolution_scan_count) {
    return false;
  }

  for (std::size_t index = 0U;
       index < result.projection_to_resolution.size();
       ++index) {
    const auto& projection = result.projection_to_resolution[index];
    if (projection.projection_index != index ||
        projection.temporal_resolution_index >=
            result.temporal_resolutions.size()) {
      return false;
    }
  }

  std::size_t projection_reference_count = 0U;
  std::size_t historical_positive_count = 0U;
  std::size_t historical_latent_count = 0U;
  std::size_t final_prefix_comparison_count = 0U;
  for (std::size_t index = 0U;
       index < result.temporal_resolutions.size();
       ++index) {
    const auto& resolution = result.temporal_resolutions[index];
    if (resolution.temporal_resolution_index != index ||
        resolution.strict_pre_locator_prefix_count >
            result.locator_snapshot_stamp.committed_batch_count ||
        resolution.localized_facet_token_index >=
            result.required_localized_token_count ||
        resolution.projection_reference_count == 0U ||
        !checked_add(
            projection_reference_count,
            resolution.projection_reference_count,
            projection_reference_count)) {
      return false;
    }
    if (index > 0U) {
      const auto& previous = result.temporal_resolutions[index - 1U];
      if (previous.strict_pre_locator_prefix_count >
              resolution.strict_pre_locator_prefix_count ||
          (previous.strict_pre_locator_prefix_count ==
               resolution.strict_pre_locator_prefix_count &&
           previous.localized_facet_token_index >=
               resolution.localized_facet_token_index)) {
        return false;
      }
    }
    if (resolution.strict_pre_locator_prefix_count ==
            result.locator_snapshot_stamp.committed_batch_count &&
        !increment(final_prefix_comparison_count)) {
      return false;
    }
    switch (resolution.disposition) {
      case ExactDirectSparseGatewayTemporalDisposition::relative_positive:
        if (!resolution.positive_payload_present ||
            resolution.source_binding_witness.external_authority_id !=
                result.locator_snapshot_stamp.external_authority_id ||
            resolution.source_binding_witness.replay_token == 0U ||
            !increment(historical_positive_count)) {
          return false;
        }
        break;
      case ExactDirectSparseGatewayTemporalDisposition::latent_unresolved:
        if (resolution.positive_payload_present ||
            resolution.historical_component_root !=
                ExactDirectSparseComponentHandle{} ||
            resolution.source_binding_witness !=
                ExactDirectSparseFacetWitness{} ||
            !increment(historical_latent_count)) {
          return false;
        }
        break;
      case ExactDirectSparseGatewayTemporalDisposition::not_certified:
        return false;
    }
  }
  return projection_reference_count == result.required_projection_count &&
         historical_positive_count ==
             result.counters.historical_positive_count &&
         historical_latent_count ==
             result.counters.historical_latent_count &&
         final_prefix_comparison_count ==
             result.counters.final_prefix_comparison_count;
}

[[nodiscard]] ExactDirectSparseGatewayTemporalResolutionResult build_impl(
    const ExactDirectSparseGatewayCandidateJournalResult& source,
    const ExactDirectSparseGatewayCandidateLocalizationResult& localization,
    const ExactDirectSparseGatewayClockAuthorityJournal& sealed_authority,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayTemporalResolutionBudget& budget,
    const ExactDirectSparsePositiveFacetPrefixSweepBudget&
        prefix_sweep_budget,
    BuildArtifacts* artifacts) {
  if (!locator.certified_positive_locator()) {
    throw std::invalid_argument(
        "gateway temporal resolution requires a certified locator");
  }
  if (artifacts != nullptr) {
    artifacts->queries.clear();
    artifacts->prefix_sweep = {};
  }

  ExactDirectSparseGatewayTemporalResolutionResult result;
  result.requested_budget = budget;
  result.requested_prefix_sweep_budget = prefix_sweep_budget;
  result.locator_snapshot_stamp = locator.snapshot_stamp();
  result.counters.locator_snapshot_check_count = 1U;
  result.required_source_batch_count = source.batches.size();
  result.required_projection_count =
      localization.deletion_projections.size();
  result.required_localized_token_count =
      localization.localized_facet_tokens.size();
  result.required_boundary_scan_count = source.batches.size();
  result.required_localized_token_scan_count =
      localization.localized_facet_tokens.size();
  result.required_sort_scratch_entry_count =
      localization.deletion_projections.size();
  initialize_scope(result);

  if (!checked_multiply(
          result.required_sort_scratch_entry_count,
          sizeof(PrefixTokenProjection),
          result.required_temporary_scratch_byte_count) ||
      !checked_multiply(
          localization.deletion_projections.size(),
          2U,
          result.required_projection_scan_count)) {
    return fail(std::move(result), BuildFailure::capacity_overflow, locator);
  }
  if (source.batches.size() > budget.maximum_source_batch_count ||
      localization.deletion_projections.size() >
          budget.maximum_projection_count ||
      localization.localized_facet_tokens.size() >
          budget.maximum_localized_token_count ||
      result.required_boundary_scan_count >
          budget.maximum_boundary_scan_count ||
      result.required_localized_token_scan_count >
          budget.maximum_localized_token_scan_count ||
      result.required_projection_scan_count >
          budget.maximum_projection_scan_count ||
      result.required_sort_scratch_entry_count >
          budget.maximum_sort_scratch_entry_count ||
      result.required_temporary_scratch_byte_count >
          budget.maximum_temporary_scratch_byte_count ||
      result.required_projection_count >
          budget.maximum_projection_output_count) {
    return fail(std::move(result), BuildFailure::budget_exhausted, locator);
  }

  const auto& certificate = sealed_authority.sealed_certificate();
  if (!source.certified_partial_refinement() ||
      !localization.certified_partial_refinement() ||
      !sealed_authority.certified_sealed_once() ||
      localization.point_count != source.point_count ||
      localization.source_gateway_candidate_count !=
          source.gateway_candidates.size() ||
      certificate.boundaries.size() != source.batches.size() ||
      sealed_authority.source_batch_count() != source.batches.size() ||
      localization.locator_snapshot_stamp != result.locator_snapshot_stamp ||
      certificate.final_locator_stamp != result.locator_snapshot_stamp ||
      localization.locator_query_witness.external_authority_id !=
          locator.config().external_authority_id ||
      localization.locator_query_witness.replay_token == 0U) {
    return fail(std::move(result), BuildFailure::source_rejected, locator);
  }

  for (std::size_t index = 0U; index < source.batches.size(); ++index) {
    const auto& boundary = certificate.boundaries[index];
    if (source.batches[index].batch_index != index ||
        boundary.source_batch_index != index ||
        boundary.strict_pre_locator_prefix_count >
            result.locator_snapshot_stamp.committed_batch_count ||
        boundary.historical_locator_stamp.committed_batch_count !=
            boundary.strict_pre_locator_prefix_count ||
        boundary.historical_locator_stamp.external_authority_id !=
            locator.config().external_authority_id ||
        !increment(result.counters.boundary_scan_count)) {
      return fail(std::move(result), BuildFailure::source_rejected, locator);
    }
  }
  result.source_boundaries_dense_and_in_locator_history = true;

  for (std::size_t token_index = 0U;
       token_index < localization.localized_facet_tokens.size();
       ++token_index) {
    const auto& token =
        localization.localized_facet_tokens[token_index];
    if (!increment(result.counters.localized_token_scan_count)) {
      return fail(
          std::move(result), BuildFailure::capacity_overflow, locator);
    }
    const bool positive =
        token.disposition ==
        ExactDirectSparseGatewayFacetLocalizationDisposition::
            relative_positive;
    const bool latent =
        token.disposition ==
        ExactDirectSparseGatewayFacetLocalizationDisposition::
            latent_unresolved;
    if (token.localized_facet_token_index != token_index ||
        (!positive && !latent) ||
        (positive &&
         (!token.component_handle_present ||
          !token.source_binding_witness_present)) ||
        (latent &&
         (token.component_handle_present ||
          token.source_binding_witness_present))) {
      return fail(std::move(result), BuildFailure::source_rejected, locator);
    }
  }
  result.source_localization_and_sealed_authority_prevalidated = true;

  try {
    std::vector<PrefixTokenProjection> sorted;
    sorted.reserve(localization.deletion_projections.size());
    for (std::size_t index = 0U;
         index < localization.deletion_projections.size();
         ++index) {
      const auto& projection = localization.deletion_projections[index];
      if (projection.deletion_projection_index != index ||
          projection.source_batch_index >= certificate.boundaries.size() ||
          projection.localized_facet_token_index >=
              localization.localized_facet_tokens.size()) {
        return fail(
            std::move(result), BuildFailure::source_rejected, locator);
      }
      sorted.push_back(
          {certificate.boundaries[projection.source_batch_index]
               .strict_pre_locator_prefix_count,
           projection.localized_facet_token_index,
           index});
      if (!increment(result.counters.projection_scan_count)) {
        return fail(
            std::move(result), BuildFailure::capacity_overflow, locator);
      }
    }
    result.every_projection_bound_to_exact_prefix_token_pair = true;

    BoundedComparator comparator(budget.maximum_sort_comparison_count);
    if (!bounded_heapsort(sorted, comparator)) {
      result.counters.sort_comparison_count =
          comparator.comparison_count();
      return fail(
          std::move(result), BuildFailure::budget_exhausted, locator);
    }
    result.counters.sort_comparison_count = comparator.comparison_count();
    result.pairs_heapsorted_by_prefix_token_projection = true;

    std::size_t unique_count = 0U;
    std::size_t query_key_point_count = 0U;
    for (std::size_t index = 0U; index < sorted.size(); ++index) {
      if (index == 0U || !same_pair(sorted[index - 1U], sorted[index])) {
        if (!increment(unique_count) ||
            !checked_add(
                query_key_point_count,
                localization
                    .localized_facet_tokens[sorted[index].token]
                    .facet_key.point_count,
                query_key_point_count)) {
          return fail(
              std::move(result), BuildFailure::capacity_overflow, locator);
        }
      }
    }
    result.required_resolution_scan_count = unique_count;
    result.required_prefix_query_scratch_entry_count = unique_count;
    result.required_prefix_resolution_scratch_entry_count = unique_count;
    result.counters.distinct_prefix_token_pair_count = unique_count;

    std::size_t query_bytes = 0U;
    std::size_t prefix_resolution_bytes = 0U;
    if (!checked_multiply(
            unique_count,
            sizeof(ExactDirectSparsePositiveFacetPrefixQuery),
            query_bytes) ||
        !checked_multiply(
            unique_count,
            sizeof(ExactDirectSparsePositiveFacetPrefixResolution),
            prefix_resolution_bytes) ||
        !checked_add(
            result.required_temporary_scratch_byte_count,
            query_bytes,
            result.required_temporary_scratch_byte_count) ||
        !checked_add(
            result.required_temporary_scratch_byte_count,
            prefix_resolution_bytes,
            result.required_temporary_scratch_byte_count) ||
        !checked_add(
            sorted.size(),
            unique_count,
            result.logical_output_entry_count)) {
      return fail(
          std::move(result), BuildFailure::capacity_overflow, locator);
    }
    if (unique_count > budget.maximum_resolution_scan_count ||
        unique_count >
            budget.maximum_prefix_query_scratch_entry_count ||
        unique_count >
            budget.maximum_prefix_resolution_scratch_entry_count ||
        unique_count >
            budget.maximum_temporal_resolution_output_count ||
        result.required_temporary_scratch_byte_count >
            budget.maximum_temporary_scratch_byte_count ||
        result.logical_output_entry_count >
            budget.maximum_logical_output_entry_count ||
        unique_count > prefix_sweep_budget.maximum_query_count ||
        query_key_point_count >
            prefix_sweep_budget.maximum_query_key_point_count) {
      return fail(
          std::move(result), BuildFailure::budget_exhausted, locator);
    }
    result.budget_preflight_certified = true;

    std::vector<ExactDirectSparsePositiveFacetPrefixQuery> queries;
    std::vector<ExactDirectSparseGatewayProjectionToTemporalResolution>
        projection_map(localization.deletion_projections.size());
    queries.reserve(unique_count);
    std::size_t current_resolution = 0U;
    for (std::size_t index = 0U; index < sorted.size(); ++index) {
      if (index == 0U || !same_pair(sorted[index - 1U], sorted[index])) {
        current_resolution = queries.size();
        queries.push_back(
            {current_resolution,
             sorted[index].prefix,
             localization.localized_facet_tokens[sorted[index].token]
                 .facet_key});
      }
      projection_map[sorted[index].projection] =
          {sorted[index].projection, current_resolution};
      if (!increment(result.counters.projection_scan_count)) {
        return fail(
            std::move(result), BuildFailure::capacity_overflow, locator);
      }
    }
    result.pairs_deduplicated_only_by_prefix_and_token = true;
    result.one_prefix_query_per_distinct_pair =
        queries.size() == unique_count;

    auto prefix_sweep =
        build_exact_direct_sparse_positive_facet_prefix_sweep(
            queries,
            localization.locator_query_witness,
            locator,
            prefix_sweep_budget);
    if (!prefix_sweep.certified_partial_refinement() ||
        prefix_sweep.resolutions.size() != unique_count ||
        prefix_sweep.locator_snapshot_stamp !=
            result.locator_snapshot_stamp) {
      return fail(
          std::move(result),
          BuildFailure::prefix_sweep_rejected,
          locator);
    }
    result.prefix_sweep_completed = true;

    std::vector<ExactDirectSparseGatewayTemporalResolution> resolutions;
    resolutions.reserve(unique_count);
    std::size_t sorted_cursor = 0U;
    for (std::size_t index = 0U; index < unique_count; ++index) {
      const auto& query = queries[index];
      const auto& historical = prefix_sweep.resolutions[index];
      const std::size_t token_index = sorted[sorted_cursor].token;
      const PrefixTokenProjection unique_pair = sorted[sorted_cursor];
      const std::size_t group_begin = sorted_cursor;
      do {
        ++sorted_cursor;
      } while (
          sorted_cursor < sorted.size() &&
          same_pair(unique_pair, sorted[sorted_cursor]));
      const auto& token =
          localization.localized_facet_tokens[token_index];
      ExactDirectSparseGatewayTemporalResolution resolution;
      resolution.temporal_resolution_index = index;
      resolution.strict_pre_locator_prefix_count =
          query.committed_batch_prefix_count;
      resolution.localized_facet_token_index = token_index;
      resolution.projection_reference_count =
          sorted_cursor - group_begin;
      if (historical.query_index != index ||
          historical.committed_batch_prefix_count !=
              query.committed_batch_prefix_count) {
        return fail(
            std::move(result),
            BuildFailure::append_only_contradiction,
            locator);
      }
      switch (historical.disposition) {
        case ExactDirectSparsePositiveFacetPrefixDisposition::
            relative_positive:
          if (!historical.component_handle_present ||
              !historical.source_binding_witness_present ||
              token.disposition !=
                  ExactDirectSparseGatewayFacetLocalizationDisposition::
                      relative_positive ||
              historical.source_binding_witness !=
                  token.source_binding_witness) {
            return fail(
                std::move(result),
                BuildFailure::append_only_contradiction,
                locator);
          }
          resolution.disposition =
              ExactDirectSparseGatewayTemporalDisposition::
                  relative_positive;
          resolution.historical_component_root =
              historical.component_handle;
          resolution.source_binding_witness =
              historical.source_binding_witness;
          resolution.positive_payload_present = true;
          if (!increment(result.counters.historical_positive_count)) {
            return fail(
                std::move(result),
                BuildFailure::capacity_overflow,
                locator);
          }
          break;
        case ExactDirectSparsePositiveFacetPrefixDisposition::
            latent_unresolved:
          if (historical.component_handle_present ||
              historical.source_binding_witness_present ||
              historical.component_handle !=
                  ExactDirectSparseComponentHandle{} ||
              historical.source_binding_witness !=
                  ExactDirectSparseFacetWitness{}) {
            return fail(
                std::move(result),
                BuildFailure::append_only_contradiction,
                locator);
          }
          resolution.disposition =
              ExactDirectSparseGatewayTemporalDisposition::
                  latent_unresolved;
          if (!increment(result.counters.historical_latent_count)) {
            return fail(
                std::move(result),
                BuildFailure::capacity_overflow,
                locator);
          }
          break;
        case ExactDirectSparsePositiveFacetPrefixDisposition::
            not_certified:
          return fail(
              std::move(result),
              BuildFailure::append_only_contradiction,
              locator);
      }
      if (token.disposition ==
              ExactDirectSparseGatewayFacetLocalizationDisposition::
                  latent_unresolved &&
          resolution.disposition !=
              ExactDirectSparseGatewayTemporalDisposition::
                  latent_unresolved) {
        return fail(
            std::move(result),
            BuildFailure::append_only_contradiction,
            locator);
      }
      if (query.committed_batch_prefix_count ==
          result.locator_snapshot_stamp.committed_batch_count) {
        if (!increment(result.counters.final_prefix_comparison_count) ||
            (token.disposition ==
                 ExactDirectSparseGatewayFacetLocalizationDisposition::
                     relative_positive &&
             (resolution.disposition !=
                  ExactDirectSparseGatewayTemporalDisposition::
                      relative_positive ||
              resolution.historical_component_root !=
                  token.component_handle ||
              resolution.source_binding_witness !=
                  token.source_binding_witness)) ||
            (token.disposition ==
                 ExactDirectSparseGatewayFacetLocalizationDisposition::
                     latent_unresolved &&
             resolution.disposition !=
                 ExactDirectSparseGatewayTemporalDisposition::
                     latent_unresolved)) {
          return fail(
              std::move(result),
              BuildFailure::append_only_contradiction,
              locator);
        }
      }
      resolutions.push_back(resolution);
      if (!increment(result.counters.resolution_scan_count)) {
        return fail(
            std::move(result), BuildFailure::capacity_overflow, locator);
      }
    }

    result.historical_positive_implies_final_positive_with_same_witness =
        true;
    result.final_latent_implies_historical_latent = true;
    result.final_prefix_payload_identical = true;
    result.every_latent_has_no_positive_payload = true;
    result.two_scientific_output_arenas_without_copied_facet_keys = true;
    check_snapshot(locator, result);
    result.common_frozen_locator_snapshot_certified = true;
    result.projection_to_resolution = std::move(projection_map);
    result.temporal_resolutions = std::move(resolutions);
    result.decision = ExactDirectSparseGatewayTemporalResolutionDecision::
        complete_certified_gateway_temporal_resolutions;
    if (artifacts != nullptr) {
      artifacts->queries = std::move(queries);
      artifacts->prefix_sweep = std::move(prefix_sweep);
    }
    if (!result.certified_partial_refinement()) {
      throw std::logic_error(
          "a complete gateway temporal resolution failed its contract");
    }
    return result;
  } catch (const std::length_error&) {
    return fail(std::move(result), BuildFailure::capacity_overflow, locator);
  } catch (const std::bad_alloc&) {
    return fail(std::move(result), BuildFailure::allocation_failed, locator);
  }
}

[[nodiscard]] bool bundle_complete(
    const ExactDirectSparseGatewayTemporalResolutionAuthorityBundle&
        bundle) noexcept {
  return bundle.index != nullptr && bundle.cloud != nullptr &&
         bundle.source_facade != nullptr &&
         bundle.source_journal != nullptr &&
         bundle.source_arm_budget != nullptr &&
         bundle.source_arm_journal != nullptr &&
         bundle.source_incidence_budget != nullptr &&
         bundle.source_incidence_journal != nullptr &&
         bundle.source_gateway_budget != nullptr &&
         bundle.source_gateway_journal != nullptr &&
         bundle.locator_query_witness != nullptr &&
         bundle.locator_budget != nullptr &&
         bundle.locator_config != nullptr && bundle.locator != nullptr &&
         bundle.localization_budget != nullptr &&
         bundle.observed_localization != nullptr &&
         bundle.external_seal_anchor != nullptr &&
         bundle.authority_journal_budget != nullptr &&
         bundle.observed_authority != nullptr &&
         bundle.authority_verification_budget != nullptr &&
         bundle.prefix_locator_structure_budget != nullptr;
}

}  // namespace

bool ExactDirectSparseGatewayTemporalResolutionResult::
    certified_partial_refinement() const noexcept {
  std::size_t expected_logical = 0U;
  return schema_version ==
             direct_sparse_gateway_temporal_resolution_schema_version &&
         decision == ExactDirectSparseGatewayTemporalResolutionDecision::
                         complete_certified_gateway_temporal_resolutions &&
         source_localization_and_sealed_authority_prevalidated &&
         budget_preflight_certified &&
         source_boundaries_dense_and_in_locator_history &&
         every_projection_bound_to_exact_prefix_token_pair &&
         pairs_heapsorted_by_prefix_token_projection &&
         pairs_deduplicated_only_by_prefix_and_token &&
         one_prefix_query_per_distinct_pair && prefix_sweep_completed &&
         historical_positive_implies_final_positive_with_same_witness &&
         final_latent_implies_historical_latent &&
         final_prefix_payload_identical &&
         every_latent_has_no_positive_payload &&
         two_scientific_output_arenas_without_copied_facet_keys &&
         common_frozen_locator_snapshot_certified &&
         no_partial_scientific_payload_published &&
         non_scope_is_honest(*this) &&
         scientific_payload_is_well_formed(*this) &&
         counters.boundary_scan_count == required_boundary_scan_count &&
         counters.localized_token_scan_count ==
             required_localized_token_scan_count &&
         counters.projection_scan_count ==
             required_projection_scan_count &&
         counters.resolution_scan_count ==
             required_resolution_scan_count &&
         counters.distinct_prefix_token_pair_count ==
             required_resolution_scan_count &&
         counters.sort_comparison_count <=
             requested_budget.maximum_sort_comparison_count &&
         required_source_batch_count <=
             requested_budget.maximum_source_batch_count &&
         required_projection_count <=
             requested_budget.maximum_projection_count &&
         required_localized_token_count <=
             requested_budget.maximum_localized_token_count &&
         required_temporary_scratch_byte_count <=
             requested_budget.maximum_temporary_scratch_byte_count &&
         checked_add(
             projection_to_resolution.size(),
             temporal_resolutions.size(),
             expected_logical) &&
         expected_logical == logical_output_entry_count &&
         expected_logical <=
             requested_budget.maximum_logical_output_entry_count;
}

bool ExactDirectSparseGatewayTemporalResolutionResult::
    certified_atomic_failure() const noexcept {
  const bool recognized =
      decision ==
          ExactDirectSparseGatewayTemporalResolutionDecision::
              no_temporal_resolution_capacity_overflow ||
      decision ==
          ExactDirectSparseGatewayTemporalResolutionDecision::
              no_temporal_resolution_budget_exhausted ||
      decision ==
          ExactDirectSparseGatewayTemporalResolutionDecision::
              no_temporal_resolution_allocation_failed ||
      decision ==
          ExactDirectSparseGatewayTemporalResolutionDecision::
              no_temporal_resolution_source_rejected ||
      decision ==
          ExactDirectSparseGatewayTemporalResolutionDecision::
              no_temporal_resolution_prefix_sweep_rejected ||
      decision ==
          ExactDirectSparseGatewayTemporalResolutionDecision::
              no_temporal_resolution_append_only_contradiction;
  return schema_version ==
             direct_sparse_gateway_temporal_resolution_schema_version &&
         recognized && projection_to_resolution.empty() &&
         temporal_resolutions.empty() &&
         logical_output_entry_count == 0U &&
         no_partial_scientific_payload_published &&
         common_frozen_locator_snapshot_certified &&
         counters.locator_snapshot_check_count >= 2U &&
         non_scope_is_honest(*this);
}

bool ExactDirectSparseGatewayTemporalResolutionResult::certified_outcome()
    const noexcept {
  return certified_partial_refinement() || certified_atomic_failure();
}

ExactDirectSparseGatewayTemporalResolutionResult
build_exact_direct_sparse_gateway_temporal_resolution(
    const ExactDirectSparseGatewayCandidateJournalResult& source,
    const ExactDirectSparseGatewayCandidateLocalizationResult& localization,
    const ExactDirectSparseGatewayClockAuthorityJournal& sealed_authority,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayTemporalResolutionBudget& budget,
    const ExactDirectSparsePositiveFacetPrefixSweepBudget&
        prefix_sweep_budget) {
  return build_impl(
      source,
      localization,
      sealed_authority,
      locator,
      budget,
      prefix_sweep_budget,
      nullptr);
}

ExactDirectSparseGatewayTemporalResolutionVerification
verify_exact_direct_sparse_gateway_temporal_resolution(
    const ExactDirectSparseGatewayTemporalResolutionAuthorityBundle&
        authorities,
    const ExactDirectSparseGatewayTemporalResolutionBudget& budget,
    const ExactDirectSparsePositiveFacetPrefixSweepBudget&
        prefix_sweep_budget,
    const ExactDirectSparseGatewayTemporalResolutionResult& observed) {
  ExactDirectSparseGatewayTemporalResolutionVerification verification;
  verification.authority_bundle_complete = bundle_complete(authorities);
  if (!verification.authority_bundle_complete) {
    return verification;
  }
  verification.observed_storage_within_budget =
      observed_storage_within_budget(observed, budget);
  if (!verification.observed_storage_within_budget) {
    return verification;
  }

  verification.localization_verification =
      verify_exact_direct_sparse_gateway_candidate_localization(
          *authorities.index,
          *authorities.cloud,
          *authorities.source_facade,
          *authorities.source_journal,
          *authorities.source_arm_budget,
          *authorities.source_arm_journal,
          *authorities.source_incidence_budget,
          *authorities.source_incidence_journal,
          *authorities.source_gateway_budget,
          *authorities.source_gateway_journal,
          *authorities.locator_query_witness,
          *authorities.locator,
          *authorities.localization_budget,
          authorities.traversal_order,
          *authorities.observed_localization);
  verification.localization_freshly_replayed =
      verification.localization_verification.result_certified;
  if (!verification.localization_freshly_replayed) {
    return verification;
  }

  verification.clock_authority_verification =
      verify_exact_direct_sparse_gateway_clock_authority_journal(
          *authorities.index,
          *authorities.cloud,
          *authorities.source_facade,
          *authorities.source_journal,
          *authorities.source_arm_budget,
          *authorities.source_arm_journal,
          *authorities.source_incidence_budget,
          *authorities.source_incidence_journal,
          *authorities.source_gateway_budget,
          authorities.traversal_order,
          *authorities.source_gateway_journal,
          authorities.trusted_component_handle_count,
          *authorities.locator_budget,
          *authorities.locator_config,
          *authorities.locator,
          *authorities.external_seal_anchor,
          *authorities.authority_journal_budget,
          *authorities.observed_authority,
          *authorities.authority_verification_budget);
  verification.clock_authority_freshly_replayed =
      verification.clock_authority_verification
          .certified_external_clock_binding();
  if (!verification.clock_authority_freshly_replayed) {
    return verification;
  }

  BuildArtifacts artifacts;
  const auto expected = build_impl(
      *authorities.source_gateway_journal,
      *authorities.observed_localization,
      *authorities.observed_authority,
      *authorities.locator,
      budget,
      prefix_sweep_budget,
      &artifacts);
  verification.expected_result_freshly_reconstructed =
      expected.certified_partial_refinement();
  if (!verification.expected_result_freshly_reconstructed) {
    return verification;
  }

  verification.prefix_sweep_verification =
      verify_exact_direct_sparse_positive_facet_prefix_sweep(
          artifacts.queries,
          *authorities.locator_query_witness,
          *authorities.locator,
          prefix_sweep_budget,
          *authorities.prefix_locator_structure_budget,
          artifacts.prefix_sweep);
  verification.prefix_sweep_freshly_replayed =
      verification.prefix_sweep_verification.result_certified;
  verification.observed_result_recursively_equal = observed == expected;
  verification.nested_verifiers_replayed =
      verification.localization_freshly_replayed &&
      verification.clock_authority_freshly_replayed &&
      verification.prefix_sweep_freshly_replayed;
  verification.external_clock_authority_replayed =
      verification.clock_authority_verification
          .external_clock_authority_replayed;
  verification.external_binding_authority_replayed =
      verification.localization_verification
          .external_binding_authority_replayed;
  verification.conditional_on_caller_external_binding_authority_replay =
      !verification.external_binding_authority_replayed;
  verification.no_forbidden_global_structure_materialized =
      !observed.forbidden_global_structure_materialized &&
      !expected.forbidden_global_structure_materialized &&
      !observed.gamma_cells_or_higher_order_delaunay_materialized &&
      !expected.gamma_cells_or_higher_order_delaunay_materialized;
  verification.fresh_replay_certified =
      verification.nested_verifiers_replayed &&
      verification.observed_result_recursively_equal;
  verification.result_certified =
      verification.authority_bundle_complete &&
      verification.observed_storage_within_budget &&
      verification.expected_result_freshly_reconstructed &&
      verification.fresh_replay_certified &&
      verification.external_clock_authority_replayed &&
      !verification.external_binding_authority_replayed &&
      verification
          .conditional_on_caller_external_binding_authority_replay &&
      verification.conditional_on_caller_strict_pre_lot_orchestration &&
      !verification.external_freeze_synchronization_replayed &&
      verification
          .conditional_on_caller_external_freeze_synchronization &&
      verification.in_memory_replay_only &&
      !verification.crash_durable &&
      verification.no_forbidden_global_structure_materialized &&
      observed.certified_partial_refinement();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
