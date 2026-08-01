#include "morsehgp3d/hierarchy/direct_frozen_unified_incidence_batch.hpp"

#include "direct_frozen_unified_incidence_batch_internal.hpp"
#include "direct_frozen_incidence_hgp_action_plan_internal.hpp"

#include <algorithm>
#include <bit>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

enum class VerifiedPlanAuthorityKind : std::uint8_t {
  freshly_verified_standalone_successive_star,
  immutable_verified_resident_successive_star,
  immutable_verified_resident_normalized_direct_source,
};

[[nodiscard]] std::optional<std::size_t> checked_add(
    std::size_t left,
    std::size_t right) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return std::nullopt;
  }
  return left + right;
}

[[nodiscard]] std::optional<std::size_t> checked_multiply(
    std::size_t left,
    std::size_t right) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return std::nullopt;
  }
  return left * right;
}

[[nodiscard]] bool add_to(
    std::size_t increment,
    std::size_t& target) noexcept {
  const auto sum = checked_add(target, increment);
  if (!sum.has_value()) {
    return false;
  }
  target = *sum;
  return true;
}

[[nodiscard]] bool valid_token_kind(
    ExactFrozenIncidenceTokenKind kind) noexcept {
  switch (kind) {
    case ExactFrozenIncidenceTokenKind::rooted_carrier:
    case ExactFrozenIncidenceTokenKind::latent_carrier:
    case ExactFrozenIncidenceTokenKind::equal_facet:
      return true;
  }
  return false;
}

[[nodiscard]] bool token_less(
    const ExactFrozenIncidenceToken& left,
    const ExactFrozenIncidenceToken& right) noexcept {
  if (left.kind != right.kind) {
    return static_cast<std::uint8_t>(left.kind) <
           static_cast<std::uint8_t>(right.kind);
  }
  return left.token_id < right.token_id;
}

[[nodiscard]] bool budget_is_sufficient(
    const ExactDirectFrozenUnifiedIncidenceBatchResult& result) noexcept {
  const auto& budget = result.requested_budget;
  return result.required_facet_resolution_capacity <=
             budget.maximum_facet_resolution_count &&
         result.required_prior_root_coverage_capacity <=
             budget.maximum_prior_root_coverage_count &&
         result.required_prior_root_coverage_point_reference_capacity <=
             budget.maximum_prior_root_coverage_point_reference_count &&
         result.required_latent_carrier_coverage_capacity <=
             budget.maximum_latent_carrier_coverage_count &&
         result.required_latent_carrier_coverage_point_reference_capacity <=
             budget.maximum_latent_carrier_coverage_point_reference_count &&
         result.required_batch_direct_reference_scan_capacity <=
             budget.maximum_batch_direct_reference_scan_count &&
         result.required_direct_saddle_hyperedge_capacity <=
             budget.maximum_direct_saddle_hyperedge_count &&
         result.required_residual_hyperedge_capacity <=
             budget.maximum_residual_hyperedge_count &&
         result.required_coface_facet_reference_scan_capacity <=
             budget.maximum_coface_facet_reference_scan_count &&
         result.required_source_point_reference_scan_capacity <=
             budget.maximum_source_point_reference_scan_count &&
         result.required_hyperedge_capacity <=
             budget.maximum_hyperedge_count &&
         result.required_token_reference_capacity <=
             budget.maximum_token_reference_count &&
         result.required_distinct_typed_token_capacity <=
             budget.maximum_distinct_typed_token_count &&
         result.required_root_attachment_capacity <=
             budget.maximum_root_attachment_count &&
         result.required_group_capacity <= budget.maximum_group_count &&
         result.required_residual_incidence_record_capacity <=
             budget.maximum_residual_incidence_record_count &&
         result.required_equal_facet_binding_record_capacity <=
             budget.maximum_equal_facet_binding_record_count &&
         result.required_coverage_delta_record_capacity <=
             budget.maximum_coverage_delta_record_count &&
         result.required_coverage_delta_facet_reference_capacity <=
             budget.maximum_coverage_delta_facet_reference_count &&
         result.required_coverage_delta_point_reference_capacity <=
             budget.maximum_coverage_delta_point_reference_count &&
         result.required_scratch_entry_capacity <=
             budget.maximum_scratch_entry_count &&
         result.required_logical_output_entry_capacity <=
             budget.maximum_logical_output_entry_count;
}

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchResult base_result(
    std::size_t batch_index,
    const ExactDirectFrozenUnifiedIncidenceBatchBudget& budget,
    VerifiedPlanAuthorityKind authority_kind =
        VerifiedPlanAuthorityKind::
            freshly_verified_standalone_successive_star) {
  ExactDirectFrozenUnifiedIncidenceBatchResult result;
  result.requested_budget = budget;
  result.source_batch_index = batch_index;
  const bool normalized =
      authority_kind == VerifiedPlanAuthorityKind::
                            immutable_verified_resident_normalized_direct_source;
  result.successive_star_source_authority = !normalized;
  result.normalized_direct_source_authority = normalized;
  result.scope = normalized
      ? ExactDirectFrozenUnifiedIncidenceBatchScope::
            exact_selected_batch_relative_to_verified_normalized_direct_source_and_external_facet_resolution_prior_root_and_latent_carrier_coverage_authorities_only
      : ExactDirectFrozenUnifiedIncidenceBatchScope::
            exact_selected_batch_relative_to_supplied_successive_star_and_external_facet_resolution_prior_root_and_latent_carrier_coverage_authorities_only;
  result.no_partial_scientific_payload_published = true;
  result.reducer_locator_forest_or_caller_state_mutated = false;
  result.global_facet_coface_or_gamma_catalog_materialized = false;
  result.supplied_star_global_completeness_claimed = false;
  result.public_status_claimed = false;
  return result;
}

void clear_payload(
    ExactDirectFrozenUnifiedIncidenceBatchResult& result) noexcept {
  result.quotient_hyperedges.clear();
  result.quotient_token_references.clear();
  result.incidence_facet_token_indices.clear();
  result.provenance.clear();
  result.root_attachments.clear();
  result.quotient = {};
  result.action_plan = {};
  result.residual_incidence_records.clear();
  result.equal_facet_binding_records.clear();
  result.coverage_deltas.clear();
  result.coverage_delta_facets.clear();
  result.coverage_delta_points.clear();
  result.counters = {};
  result.all_output_within_budget = false;
  result.no_partial_scientific_payload_published = true;
}

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchResult fail(
    ExactDirectFrozenUnifiedIncidenceBatchResult result,
    ExactDirectFrozenUnifiedIncidenceBatchDecision decision) noexcept {
  clear_payload(result);
  result.decision = decision;
  return result;
}

[[nodiscard]] bool range_is_valid(
    std::size_t offset,
    std::size_t count,
    std::size_t size) noexcept {
  return offset <= size && count <= size - offset;
}

[[nodiscard]] bool initialize_requirements(
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    const ExactDirectSparseUnifiedLevelPlanBatch& batch,
    std::span<const ExactDirectFrozenUnifiedFacetResolution>
        facet_resolutions,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
        prior_root_coverages,
    std::span<const PointId> coverage_points,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
        latent_carrier_coverages,
    std::span<const PointId> latent_coverage_points,
    ExactDirectFrozenUnifiedIncidenceBatchResult& result) noexcept {
  result.source_future_snapshot_index = batch.future_snapshot_index;
  result.squared_level = batch.squared_level;
  result.order = batch.order;
  result.required_facet_resolution_capacity = facet_resolutions.size();
  result.required_prior_root_coverage_capacity =
      prior_root_coverages.size();
  result.required_prior_root_coverage_point_reference_capacity =
      coverage_points.size();
  result.required_latent_carrier_coverage_capacity =
      latent_carrier_coverages.size();
  result.required_latent_carrier_coverage_point_reference_capacity =
      latent_coverage_points.size();
  result.required_batch_direct_reference_scan_capacity =
      batch.direct_reference_count;
  result.required_residual_hyperedge_capacity =
      batch.residual_reference_count;
  result.required_coface_facet_reference_scan_capacity =
      batch.coface_facet_reference_count;
  result.required_token_reference_capacity =
      batch.coface_facet_reference_count;
  result.required_distinct_typed_token_capacity =
      batch.coface_facet_reference_count;
  result.required_root_attachment_capacity = facet_resolutions.size();
  result.required_residual_incidence_record_capacity =
      batch.residual_reference_count;
  result.required_equal_facet_binding_record_capacity =
      facet_resolutions.size();
  result.required_coverage_delta_facet_reference_capacity =
      facet_resolutions.size();

  if (!range_is_valid(
          batch.direct_reference_offset,
          batch.direct_reference_count,
          source_plan.direct_references.size()) ||
      !range_is_valid(
          batch.residual_reference_offset,
          batch.residual_reference_count,
          source_plan.residual_references.size()) ||
      !range_is_valid(
          batch.coface_facet_reference_offset,
          batch.coface_facet_reference_count,
          source_plan.coface_facet_references.size())) {
    return false;
  }

  std::size_t direct_saddle_count = 0U;
  for (std::size_t local = 0U; local < batch.direct_reference_count;
       ++local) {
    const auto& reference = source_plan.direct_references
        [batch.direct_reference_offset + local];
    switch (reference.role) {
      case ExactDirectMorseH0Role::birth:
        break;
      case ExactDirectMorseH0Role::saddle:
        if (!add_to(1U, direct_saddle_count)) {
          return false;
        }
        break;
    }
  }
  result.required_direct_saddle_hyperedge_capacity =
      direct_saddle_count;
  const auto hyperedge_count = checked_add(
      direct_saddle_count, batch.residual_reference_count);
  if (!hyperedge_count.has_value()) {
    return false;
  }
  result.required_hyperedge_capacity = *hyperedge_count;
  result.required_group_capacity = *hyperedge_count;
  result.required_coverage_delta_record_capacity = *hyperedge_count;

  return true;
}

[[nodiscard]] ExactFrozenIncidenceQuotientBudget quotient_budget_from(
    const ExactDirectFrozenUnifiedIncidenceBatchBudget& budget) noexcept {
  return {
      budget.maximum_hyperedge_count,
      budget.maximum_token_reference_count,
      budget.maximum_distinct_typed_token_count,
      budget.maximum_group_count,
      budget.maximum_hyperedge_count,
      budget.maximum_distinct_typed_token_count,
      budget.maximum_distinct_typed_token_count,
      budget.maximum_scratch_entry_count,
      budget.maximum_logical_output_entry_count,
  };
}

[[nodiscard]] ExactFrozenIncidenceHgpActionPlanBudget action_budget_from(
    const ExactDirectFrozenUnifiedIncidenceBatchBudget& budget) noexcept {
  return {
      budget.maximum_hyperedge_count,
      budget.maximum_hyperedge_count,
      budget.maximum_root_attachment_count,
      budget.maximum_group_count,
      budget.maximum_hyperedge_count,
      budget.maximum_hyperedge_count,
      budget.maximum_root_attachment_count,
      budget.maximum_scratch_entry_count,
      budget.maximum_logical_output_entry_count,
  };
}

[[nodiscard]] const ExactDirectFrozenUnifiedFacetResolution*
find_resolution(
    std::span<const ExactDirectFrozenUnifiedFacetResolution> resolutions,
    std::size_t facet_token_index) noexcept {
  const auto found = std::lower_bound(
      resolutions.begin(),
      resolutions.end(),
      facet_token_index,
      [](const auto& resolution, std::size_t candidate) {
        return resolution.facet_token_index < candidate;
      });
  return found != resolutions.end() &&
                 found->facet_token_index == facet_token_index
             ? &*found
             : nullptr;
}

[[nodiscard]] const ExactDirectFrozenUnifiedPriorRootCoverage*
find_coverage(
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage> coverages,
    ExactFrozenIncidencePriorRootId prior_root_id) noexcept {
  const auto found = std::lower_bound(
      coverages.begin(),
      coverages.end(),
      prior_root_id,
      [](const auto& coverage, ExactFrozenIncidencePriorRootId candidate) {
        return coverage.prior_root_id < candidate;
      });
  return found != coverages.end() && found->prior_root_id == prior_root_id
             ? &*found
             : nullptr;
}

[[nodiscard]] const ExactDirectFrozenUnifiedLatentCarrierCoverage*
find_latent_coverage(
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage> coverages,
    ExactFrozenIncidenceTokenId latent_carrier_token_id) noexcept {
  const auto found = std::lower_bound(
      coverages.begin(),
      coverages.end(),
      latent_carrier_token_id,
      [](const auto& coverage, ExactFrozenIncidenceTokenId candidate) {
        return coverage.latent_carrier_token_id < candidate;
      });
  return found != coverages.end() &&
                 found->latent_carrier_token_id == latent_carrier_token_id
             ? &*found
             : nullptr;
}

[[nodiscard]] std::optional<std::size_t>
conservative_binary_search_point_probe_count(
    std::size_t sorted_point_count) noexcept {
  if (sorted_point_count == 0U) {
    return 0U;
  }
  // lower_bound probes at most bit_width(n) entries; binary_search may then
  // read the selected entry once more for equality.  Deliberately retain the
  // conservative extra probe instead of depending on one STL implementation.
  return checked_add(
      static_cast<std::size_t>(std::bit_width(sorted_point_count)), 1U);
}

[[nodiscard]] const ExactFrozenIncidenceTokenBinding* find_token_binding(
    const ExactFrozenIncidenceQuotientResult& quotient,
    const ExactFrozenIncidenceToken& token) noexcept {
  const auto found = std::lower_bound(
      quotient.token_bindings.begin(),
      quotient.token_bindings.end(),
      token,
      [](const auto& binding, const auto& candidate) {
        return token_less(binding.token, candidate);
      });
  return found != quotient.token_bindings.end() && found->token == token
             ? &*found
             : nullptr;
}

void canonicalize_points(std::vector<PointId>& points) {
  std::sort(points.begin(), points.end());
  points.erase(std::unique(points.begin(), points.end()), points.end());
}

void append_facet_points(
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    std::size_t facet_token_index,
    std::vector<PointId>& destination) {
  const auto& key = source_plan.facet_tokens[facet_token_index].facet_key;
  destination.insert(
      destination.end(),
      key.point_ids.begin(),
      key.point_ids.begin() +
          static_cast<std::ptrdiff_t>(key.point_count));
}

void append_root_coverage_points(
    ExactFrozenIncidencePriorRootId prior_root_id,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage> coverages,
    std::span<const PointId> coverage_points,
    std::vector<PointId>& destination) {
  const auto* coverage = find_coverage(coverages, prior_root_id);
  if (coverage == nullptr) {
    throw std::logic_error("a frozen unified root has no coverage record");
  }
  destination.insert(
      destination.end(),
      coverage_points.begin() +
          static_cast<std::ptrdiff_t>(coverage->point_reference_offset),
      coverage_points.begin() + static_cast<std::ptrdiff_t>(
          coverage->point_reference_offset +
          coverage->point_reference_count));
}

void append_latent_coverage_points(
    ExactFrozenIncidenceTokenId latent_carrier_token_id,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage> coverages,
    std::span<const PointId> coverage_points,
    std::vector<PointId>& destination) {
  const auto* coverage =
      find_latent_coverage(coverages, latent_carrier_token_id);
  if (coverage == nullptr) {
    throw std::logic_error(
        "a frozen unified latent carrier has no coverage record");
  }
  destination.insert(
      destination.end(),
      coverage_points.begin() +
          static_cast<std::ptrdiff_t>(coverage->point_reference_offset),
      coverage_points.begin() + static_cast<std::ptrdiff_t>(
          coverage->point_reference_offset +
          coverage->point_reference_count));
}

struct RootPair {
  ExactFrozenIncidenceTokenId carrier_id{};
  ExactFrozenIncidencePriorRootId prior_root_id{};
};

[[nodiscard]] bool validate_authorities(
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    const ExactDirectSparseUnifiedLevelPlanBatch& batch,
    std::span<const ExactDirectFrozenUnifiedFacetResolution> resolutions,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage> coverages,
    std::span<const PointId> coverage_points,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
        latent_coverages,
    std::span<const PointId> latent_coverage_points,
    bool& latent_authority_rejected,
    std::vector<ExactFrozenIncidenceRootAttachment>& attachments,
    std::vector<std::size_t>& touched_facet_tokens) {
  latent_authority_rejected = false;
  touched_facet_tokens.reserve(batch.coface_facet_reference_count);
  for (std::size_t local = 0U;
       local < batch.coface_facet_reference_count;
       ++local) {
    const auto& reference = source_plan.coface_facet_references
        [batch.coface_facet_reference_offset + local];
    touched_facet_tokens.push_back(reference.facet_token_index);
  }
  std::sort(touched_facet_tokens.begin(), touched_facet_tokens.end());
  touched_facet_tokens.erase(
      std::unique(
          touched_facet_tokens.begin(), touched_facet_tokens.end()),
      touched_facet_tokens.end());
  if (touched_facet_tokens.size() != resolutions.size()) {
    return false;
  }

  std::vector<RootPair> root_pairs;
  root_pairs.reserve(resolutions.size());
  std::vector<ExactFrozenIncidenceTokenId> equal_ids;
  equal_ids.reserve(resolutions.size());
  std::vector<ExactFrozenIncidenceTokenId> latent_ids;
  latent_ids.reserve(resolutions.size());
  for (std::size_t index = 0U; index < resolutions.size(); ++index) {
    const auto& resolution = resolutions[index];
    if (resolution.facet_token_index != touched_facet_tokens[index] ||
        (index != 0U &&
         resolutions[index - 1U].facet_token_index >=
             resolution.facet_token_index) ||
        resolution.facet_token_index >= source_plan.facet_tokens.size() ||
        !valid_token_kind(resolution.token.kind)) {
      return false;
    }
    if (resolution.token.kind ==
        ExactFrozenIncidenceTokenKind::rooted_carrier) {
      if (!resolution.prior_root_id.has_value()) {
        return false;
      }
      root_pairs.push_back(
          {resolution.token.token_id, *resolution.prior_root_id});
    } else {
      if (resolution.prior_root_id.has_value()) {
        return false;
      }
      if (resolution.token.kind ==
          ExactFrozenIncidenceTokenKind::equal_facet) {
        equal_ids.push_back(resolution.token.token_id);
      } else if (resolution.token.kind ==
                 ExactFrozenIncidenceTokenKind::latent_carrier) {
        latent_ids.push_back(resolution.token.token_id);
      }
    }
  }
  std::sort(equal_ids.begin(), equal_ids.end());
  if (std::adjacent_find(equal_ids.begin(), equal_ids.end()) !=
      equal_ids.end()) {
    return false;
  }
  std::sort(latent_ids.begin(), latent_ids.end());
  latent_ids.erase(
      std::unique(latent_ids.begin(), latent_ids.end()), latent_ids.end());
  if (latent_coverages.size() != latent_ids.size()) {
    latent_authority_rejected = true;
    return false;
  }

  std::sort(
      root_pairs.begin(),
      root_pairs.end(),
      [](const RootPair& left, const RootPair& right) {
        return left.carrier_id < right.carrier_id ||
               (left.carrier_id == right.carrier_id &&
                left.prior_root_id < right.prior_root_id);
      });
  std::vector<RootPair> canonical_roots;
  canonical_roots.reserve(root_pairs.size());
  for (const RootPair& pair : root_pairs) {
    if (!canonical_roots.empty() &&
        canonical_roots.back().carrier_id == pair.carrier_id) {
      if (canonical_roots.back().prior_root_id != pair.prior_root_id) {
        return false;
      }
      continue;
    }
    canonical_roots.push_back(pair);
  }
  std::vector<RootPair> roots_by_prior = canonical_roots;
  std::sort(
      roots_by_prior.begin(),
      roots_by_prior.end(),
      [](const RootPair& left, const RootPair& right) {
        return left.prior_root_id < right.prior_root_id;
      });
  if (std::adjacent_find(
          roots_by_prior.begin(),
          roots_by_prior.end(),
          [](const RootPair& left, const RootPair& right) {
            return left.prior_root_id == right.prior_root_id;
          }) != roots_by_prior.end() ||
      coverages.size() != roots_by_prior.size()) {
    return false;
  }

  std::size_t expected_offset = 0U;
  for (std::size_t index = 0U; index < coverages.size(); ++index) {
    const auto& coverage = coverages[index];
    if (coverage.prior_root_id != roots_by_prior[index].prior_root_id ||
        coverage.point_reference_offset != expected_offset ||
        !range_is_valid(
            coverage.point_reference_offset,
            coverage.point_reference_count,
            coverage_points.size())) {
      return false;
    }
    const auto begin = coverage_points.begin() +
        static_cast<std::ptrdiff_t>(coverage.point_reference_offset);
    const auto end = begin +
        static_cast<std::ptrdiff_t>(coverage.point_reference_count);
    for (auto cursor = begin; cursor != end; ++cursor) {
      if (static_cast<std::size_t>(*cursor) >= source_plan.point_count ||
          (cursor != begin && *(cursor - 1) >= *cursor)) {
        return false;
      }
    }
    expected_offset += coverage.point_reference_count;
  }
  if (expected_offset != coverage_points.size()) {
    return false;
  }

  expected_offset = 0U;
  for (std::size_t index = 0U; index < latent_coverages.size(); ++index) {
    const auto& coverage = latent_coverages[index];
    if (coverage.latent_carrier_token_id != latent_ids[index] ||
        coverage.point_reference_offset != expected_offset ||
        !range_is_valid(
            coverage.point_reference_offset,
            coverage.point_reference_count,
            latent_coverage_points.size())) {
      latent_authority_rejected = true;
      return false;
    }
    const auto begin = latent_coverage_points.begin() +
        static_cast<std::ptrdiff_t>(coverage.point_reference_offset);
    const auto end = begin +
        static_cast<std::ptrdiff_t>(coverage.point_reference_count);
    for (auto cursor = begin; cursor != end; ++cursor) {
      if (static_cast<std::size_t>(*cursor) >= source_plan.point_count ||
          (cursor != begin && *(cursor - 1) >= *cursor)) {
        latent_authority_rejected = true;
        return false;
      }
    }
    expected_offset += coverage.point_reference_count;
  }
  if (expected_offset != latent_coverage_points.size()) {
    latent_authority_rejected = true;
    return false;
  }

  // A facet resolved into a prior root must already be covered by that root.
  for (const auto& resolution : resolutions) {
    if (resolution.token.kind ==
        ExactFrozenIncidenceTokenKind::equal_facet) {
      continue;
    }
    const bool rooted =
        resolution.token.kind ==
        ExactFrozenIncidenceTokenKind::rooted_carrier;
    const auto* root_coverage = rooted
        ? find_coverage(coverages, *resolution.prior_root_id)
        : nullptr;
    const auto* latent_coverage = !rooted
        ? find_latent_coverage(
              latent_coverages, resolution.token.token_id)
        : nullptr;
    if ((rooted && root_coverage == nullptr) ||
        (!rooted && latent_coverage == nullptr)) {
      latent_authority_rejected = !rooted;
      return false;
    }
    const std::size_t point_offset = rooted
        ? root_coverage->point_reference_offset
        : latent_coverage->point_reference_offset;
    const std::size_t point_count = rooted
        ? root_coverage->point_reference_count
        : latent_coverage->point_reference_count;
    const auto points = rooted ? coverage_points : latent_coverage_points;
    const auto begin = points.begin() +
        static_cast<std::ptrdiff_t>(point_offset);
    const auto end = begin + static_cast<std::ptrdiff_t>(point_count);
    const auto& key =
        source_plan.facet_tokens[resolution.facet_token_index].facet_key;
    for (std::size_t point_index = 0U;
         point_index < key.point_count;
         ++point_index) {
      if (!std::binary_search(begin, end, key.point_ids[point_index])) {
        latent_authority_rejected = !rooted;
        return false;
      }
    }
  }

  attachments.reserve(canonical_roots.size());
  for (const RootPair& root : canonical_roots) {
    attachments.push_back({root.carrier_id, root.prior_root_id});
  }
  return true;
}

// Completes every budget requirement without allocating or reading any
// supplied PointId entry.  Canonical record ordering lets the valid path use
// logarithmic lookups; malformed authorities are only classified here and
// are rejected after the complete budget gate.  Therefore no source-point
// validation, sort, factorized arena, quotient or delta arena can precede the
// cap check.
[[nodiscard]] bool preflight_requirements_from_raw_authorities(
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    const ExactDirectSparseUnifiedLevelPlanBatch& batch,
    std::span<const ExactDirectFrozenUnifiedFacetResolution> resolutions,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
        prior_root_coverages,
    std::span<const PointId> prior_root_coverage_points,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
        latent_coverages,
    std::span<const PointId> latent_coverage_points,
    ExactDirectFrozenUnifiedIncidenceBatchResult& result,
    std::size_t& closed_point_occurrence_capacity,
    bool& facet_authority_rejected,
    bool& latent_authority_rejected) noexcept {
  facet_authority_rejected = false;
  latent_authority_rejected = false;

  const bool resolutions_strictly_ordered = std::adjacent_find(
      resolutions.begin(), resolutions.end(), [](const auto& left,
                                                  const auto& right) {
        return left.facet_token_index >= right.facet_token_index;
      }) == resolutions.end();
  const bool roots_strictly_ordered = std::adjacent_find(
      prior_root_coverages.begin(),
      prior_root_coverages.end(),
      [](const auto& left, const auto& right) {
        return left.prior_root_id >= right.prior_root_id;
      }) == prior_root_coverages.end();
  const bool latents_strictly_ordered = std::adjacent_find(
      latent_coverages.begin(),
      latent_coverages.end(),
      [](const auto& left, const auto& right) {
        return left.latent_carrier_token_id >=
               right.latent_carrier_token_id;
      }) == latent_coverages.end();
  facet_authority_rejected =
      !resolutions_strictly_ordered || !roots_strictly_ordered;
  latent_authority_rejected = !latents_strictly_ordered;

  std::size_t distinct_facet_point_count = 0U;
  std::size_t containment_point_probe_count = 0U;
  for (const auto& resolution : resolutions) {
    if (resolution.facet_token_index >= source_plan.facet_tokens.size()) {
      facet_authority_rejected = true;
      continue;
    }
    const std::size_t point_count =
        source_plan.facet_tokens[resolution.facet_token_index]
            .facet_key.point_count;
    if (!add_to(point_count, distinct_facet_point_count)) {
      return false;
    }
    if (!valid_token_kind(resolution.token.kind)) {
      facet_authority_rejected = true;
      continue;
    }
    if (resolution.token.kind ==
        ExactFrozenIncidenceTokenKind::equal_facet) {
      if (resolution.prior_root_id.has_value()) {
        facet_authority_rejected = true;
      }
      continue;
    }

    std::size_t coverage_point_count = 0U;
    if (resolution.token.kind ==
        ExactFrozenIncidenceTokenKind::rooted_carrier) {
      if (!resolution.prior_root_id.has_value() ||
          !roots_strictly_ordered) {
        facet_authority_rejected = true;
        continue;
      }
      const auto* coverage =
          find_coverage(prior_root_coverages, *resolution.prior_root_id);
      if (coverage == nullptr ||
          !range_is_valid(
              coverage->point_reference_offset,
              coverage->point_reference_count,
              prior_root_coverage_points.size())) {
        facet_authority_rejected = true;
        continue;
      }
      coverage_point_count = coverage->point_reference_count;
    } else {
      if (resolution.prior_root_id.has_value()) {
        facet_authority_rejected = true;
      }
      if (!latents_strictly_ordered) {
        latent_authority_rejected = true;
        continue;
      }
      const auto* coverage = find_latent_coverage(
          latent_coverages, resolution.token.token_id);
      if (coverage == nullptr ||
          !range_is_valid(
              coverage->point_reference_offset,
              coverage->point_reference_count,
              latent_coverage_points.size())) {
        latent_authority_rejected = true;
        continue;
      }
      coverage_point_count = coverage->point_reference_count;
    }
    const auto probes_per_point =
        conservative_binary_search_point_probe_count(
            coverage_point_count);
    const auto probes = probes_per_point.has_value()
        ? checked_multiply(point_count, *probes_per_point)
        : std::nullopt;
    if (!probes.has_value() ||
        !add_to(*probes, containment_point_probe_count)) {
      return false;
    }
  }

  std::size_t occurrence_facet_point_scan_count = 0U;
  std::size_t occurrence_latent_point_scan_count = 0U;
  const std::size_t references_begin = batch.coface_facet_reference_offset;
  const std::size_t references_end =
      references_begin + batch.coface_facet_reference_count;
  std::size_t coface_slice_begin = references_begin;
  for (std::size_t reference_index = references_begin;
       reference_index < references_end;
       ++reference_index) {
    const auto& reference =
        source_plan.coface_facet_references[reference_index];
    if (reference_index != references_begin &&
        source_plan.coface_facet_references[reference_index - 1U]
                .source_star_coface_index !=
            reference.source_star_coface_index) {
      coface_slice_begin = reference_index;
    }
    if (reference.facet_token_index >= source_plan.facet_tokens.size()) {
      facet_authority_rejected = true;
      continue;
    }
    if (!add_to(
            source_plan.facet_tokens[reference.facet_token_index]
                .facet_key.point_count,
            occurrence_facet_point_scan_count)) {
      return false;
    }
    if (!resolutions_strictly_ordered) {
      facet_authority_rejected = true;
      continue;
    }
    const auto* resolution =
        find_resolution(resolutions, reference.facet_token_index);
    if (resolution == nullptr) {
      facet_authority_rejected = true;
      continue;
    }
    if (resolution->token.kind !=
        ExactFrozenIncidenceTokenKind::latent_carrier) {
      continue;
    }
    bool first_in_hyperedge = true;
    for (std::size_t previous_index = coface_slice_begin;
         previous_index < reference_index;
         ++previous_index) {
      const auto& previous_reference =
          source_plan.coface_facet_references[previous_index];
      const auto* previous_resolution = find_resolution(
          resolutions, previous_reference.facet_token_index);
      if (previous_resolution != nullptr &&
          previous_resolution->token == resolution->token) {
        first_in_hyperedge = false;
        break;
      }
    }
    if (!first_in_hyperedge) {
      continue;
    }
    if (!latents_strictly_ordered) {
      latent_authority_rejected = true;
      continue;
    }
    const auto* coverage = find_latent_coverage(
        latent_coverages, resolution->token.token_id);
    if (coverage == nullptr ||
        !range_is_valid(
            coverage->point_reference_offset,
            coverage->point_reference_count,
            latent_coverage_points.size())) {
      latent_authority_rejected = true;
      continue;
    }
    if (!add_to(
            coverage->point_reference_count,
            occurrence_latent_point_scan_count)) {
      return false;
    }
  }
  const auto closed_capacity = checked_add(
      occurrence_facet_point_scan_count,
      occurrence_latent_point_scan_count);
  if (!closed_capacity.has_value()) {
    return false;
  }
  closed_point_occurrence_capacity = *closed_capacity;

  std::size_t point_scan_count = 0U;
  if (!add_to(prior_root_coverage_points.size(), point_scan_count) ||
      // prepare_group_coverages copies every prior-root slice once more.
      !add_to(prior_root_coverage_points.size(), point_scan_count) ||
      !add_to(latent_coverage_points.size(), point_scan_count) ||
      !add_to(containment_point_probe_count, point_scan_count) ||
      !add_to(occurrence_facet_point_scan_count, point_scan_count) ||
      !add_to(occurrence_latent_point_scan_count, point_scan_count)) {
    return false;
  }
  result.required_source_point_reference_scan_capacity = point_scan_count;

  const auto delta_point_capacity = checked_add(
      distinct_facet_point_count, latent_coverage_points.size());
  if (!delta_point_capacity.has_value()) {
    return false;
  }
  result.required_coverage_delta_point_reference_capacity =
      *delta_point_capacity;

  // Conservative simultaneous scratch: local sorting/indexing, quotient DSU,
  // action analysis, the flat facet/point occurrence tables, one reusable
  // closed-point buffer, and both external coverage CSR analyses.
  std::size_t scratch = 0U;
  const auto eight_tokens = checked_multiply(
      8U, batch.coface_facet_reference_count);
  const auto eight_hyperedges = checked_multiply(
      8U, result.required_hyperedge_capacity);
  const auto four_resolutions = checked_multiply(4U, resolutions.size());
  const auto double_root_points = checked_multiply(
      2U, prior_root_coverage_points.size());
  const auto double_roots = checked_multiply(
      2U, result.required_prior_root_coverage_capacity);
  const auto double_latent_points = checked_multiply(
      2U, latent_coverage_points.size());
  const auto double_latents = checked_multiply(
      2U, latent_coverages.size());
  if (!eight_tokens.has_value() || !eight_hyperedges.has_value() ||
      !four_resolutions.has_value() || !double_root_points.has_value() ||
      !double_roots.has_value() || !double_latent_points.has_value() ||
      !double_latents.has_value() ||
      !add_to(*eight_tokens, scratch) ||
      !add_to(*eight_hyperedges, scratch) ||
      !add_to(*four_resolutions, scratch) ||
      !add_to(*double_root_points, scratch) ||
      !add_to(*double_roots, scratch) ||
      !add_to(*double_latent_points, scratch) ||
      !add_to(*double_latents, scratch) ||
      !add_to(distinct_facet_point_count, scratch) ||
      !add_to(closed_point_occurrence_capacity, scratch)) {
    return false;
  }
  result.required_scratch_entry_capacity = scratch;

  const auto double_tokens = checked_multiply(
      2U, batch.coface_facet_reference_count);
  const auto double_hyperedges = checked_multiply(
      2U, result.required_hyperedge_capacity);
  const auto quotient_output =
      double_tokens.has_value() && double_hyperedges.has_value()
          ? checked_add(*double_tokens, *double_hyperedges)
          : std::nullopt;
  const auto action_output = double_hyperedges.has_value()
      ? checked_add(*double_hyperedges, resolutions.size())
      : std::nullopt;
  if (!double_tokens.has_value() || !double_hyperedges.has_value() ||
      !quotient_output.has_value() || !action_output.has_value()) {
    return false;
  }
  std::size_t output = 0U;
  const std::size_t increments[] = {
      result.required_hyperedge_capacity,
      batch.coface_facet_reference_count,
      batch.coface_facet_reference_count,
      result.required_hyperedge_capacity,
      resolutions.size(),
      *quotient_output,
      *action_output,
      batch.residual_reference_count,
      resolutions.size(),
      result.required_hyperedge_capacity,
      resolutions.size(),
      *delta_point_capacity,
  };
  for (const std::size_t increment : increments) {
    if (!add_to(increment, output)) {
      return false;
    }
  }
  result.required_logical_output_entry_capacity = output;
  return true;
}

struct CofaceSlice {
  std::size_t source_star_coface_index{};
  std::size_t source_reference_offset{};
  std::size_t reference_count{};
};

struct HyperedgeMeta {
  ExactFrozenIncidenceHyperedgeProvenanceKind kind{
      ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family};
  std::size_t source_reference_index{};
  std::size_t source_star_coface_index{};
};

[[nodiscard]] const CofaceSlice* find_coface_slice(
    const std::vector<CofaceSlice>& slices,
    std::size_t source_star_coface_index) noexcept {
  const auto found = std::lower_bound(
      slices.begin(),
      slices.end(),
      source_star_coface_index,
      [](const CofaceSlice& slice, std::size_t candidate) {
        return slice.source_star_coface_index < candidate;
      });
  return found != slices.end() &&
                 found->source_star_coface_index ==
                     source_star_coface_index
             ? &*found
             : nullptr;
}

[[nodiscard]] bool build_factorized_authorities(
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    const ExactDirectSparseUnifiedLevelPlanBatch& batch,
    std::span<const ExactDirectFrozenUnifiedFacetResolution> resolutions,
    std::vector<ExactFrozenIncidenceHyperedge>& hyperedges,
    std::vector<ExactFrozenIncidenceToken>& tokens,
    std::vector<std::size_t>& facet_token_indices,
    std::vector<HyperedgeMeta>& metadata,
    std::size_t required_hyperedge_capacity,
    std::size_t& deferred_birth_count) {
  std::vector<CofaceSlice> slices;
  // Direct births are scan-only and never own a coface slice.  Reserving from
  // batch.direct_reference_count would therefore allocate O(B) scratch that
  // the certified hyperedge scratch requirement deliberately does not count.
  slices.reserve(required_hyperedge_capacity);
  const std::size_t references_begin =
      batch.coface_facet_reference_offset;
  const std::size_t references_end = references_begin +
      batch.coface_facet_reference_count;
  std::size_t cursor = references_begin;
  while (cursor < references_end) {
    const std::size_t source_coface =
        source_plan.coface_facet_references[cursor]
            .source_star_coface_index;
    const std::size_t slice_begin = cursor;
    while (cursor < references_end &&
           source_plan.coface_facet_references[cursor]
                   .source_star_coface_index == source_coface) {
      ++cursor;
    }
    if (!slices.empty() &&
        slices.back().source_star_coface_index >= source_coface) {
      return false;
    }
    slices.push_back({source_coface, slice_begin, cursor - slice_begin});
  }

  metadata.reserve(required_hyperedge_capacity);
  for (std::size_t local = 0U; local < batch.direct_reference_count;
       ++local) {
    const auto& direct = source_plan.direct_references
        [batch.direct_reference_offset + local];
    switch (direct.role) {
      case ExactDirectMorseH0Role::birth:
        ++deferred_birth_count;
        if (direct.source_star_direct_coface_index.has_value()) {
          return false;
        }
        break;
      case ExactDirectMorseH0Role::saddle:
        if (!direct.source_star_direct_coface_index.has_value()) {
          return false;
        }
        metadata.push_back(
            {ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family,
             direct.direct_reference_index,
             *direct.source_star_direct_coface_index});
        break;
    }
  }
  for (std::size_t local = 0U; local < batch.residual_reference_count;
       ++local) {
    const auto& residual = source_plan.residual_references
        [batch.residual_reference_offset + local];
    metadata.push_back(
        {ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence,
         residual.residual_reference_index,
         residual.source_star_coface_index});
  }
  if (metadata.size() != required_hyperedge_capacity ||
      metadata.size() != slices.size()) {
    return false;
  }

  std::vector<bool> slice_used(slices.size(), false);
  hyperedges.reserve(metadata.size());
  tokens.reserve(batch.coface_facet_reference_count);
  facet_token_indices.reserve(batch.coface_facet_reference_count);
  for (std::size_t hyperedge_index = 0U;
       hyperedge_index < metadata.size();
       ++hyperedge_index) {
    const auto* slice = find_coface_slice(
        slices, metadata[hyperedge_index].source_star_coface_index);
    if (slice == nullptr || slice->reference_count != batch.order + 1U) {
      return false;
    }
    const std::size_t slice_index =
        static_cast<std::size_t>(slice - slices.data());
    if (slice_used[slice_index]) {
      return false;
    }
    slice_used[slice_index] = true;
    const std::size_t token_offset = tokens.size();
    for (std::size_t local = 0U; local < slice->reference_count;
         ++local) {
      const auto& facet_reference =
          source_plan.coface_facet_references
              [slice->source_reference_offset + local];
      const auto* resolution = find_resolution(
          resolutions, facet_reference.facet_token_index);
      if (resolution == nullptr ||
          facet_reference.removed_union_point_index != local) {
        return false;
      }
      tokens.push_back(resolution->token);
      facet_token_indices.push_back(facet_reference.facet_token_index);
    }
    hyperedges.push_back(
        {hyperedge_index, token_offset, slice->reference_count});
  }
  return std::all_of(
             slice_used.begin(), slice_used.end(), [](bool used) {
               return used;
             }) &&
         tokens.size() == batch.coface_facet_reference_count &&
         tokens.size() == facet_token_indices.size();
}

struct PreparedGroupCoverage {
  std::vector<ExactFrozenIncidencePriorRootId> prior_root_ids;
  std::vector<PointId> prior_points;
};

[[nodiscard]] bool prepare_group_coverages(
    const std::vector<ExactFrozenIncidenceRootAttachment>& attachments,
    const ExactFrozenIncidenceQuotientResult& quotient,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage> coverages,
    std::span<const PointId> coverage_points,
    std::vector<PreparedGroupCoverage>& prepared) {
  prepared.resize(quotient.groups.size());
  for (const auto& attachment : attachments) {
    const auto* binding = find_token_binding(
        quotient,
        {ExactFrozenIncidenceTokenKind::rooted_carrier,
         attachment.rooted_carrier_token_id});
    if (binding == nullptr) {
      throw std::logic_error(
          "a frozen unified root attachment has no quotient token");
    }
    if (binding->group_index >= prepared.size()) {
      return false;
    }
    auto& group = prepared[binding->group_index];
    group.prior_root_ids.push_back(attachment.prior_root_id);
    append_root_coverage_points(
        attachment.prior_root_id,
        coverages,
        coverage_points,
        group.prior_points);
  }
  for (std::size_t group_index = 0U;
       group_index < prepared.size();
       ++group_index) {
    auto& group = prepared[group_index];
    std::sort(group.prior_root_ids.begin(), group.prior_root_ids.end());
    canonicalize_points(group.prior_points);
    if (group.prior_root_ids.size() !=
        quotient.groups[group_index].rooted_carrier_count) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool resolution_is_rooted(
    std::span<const ExactDirectFrozenUnifiedFacetResolution> resolutions,
    std::size_t facet_token_index) {
  const auto* resolution = find_resolution(resolutions, facet_token_index);
  if (resolution == nullptr) {
    throw std::logic_error(
        "a frozen unified incidence has no facet resolution");
  }
  return resolution->token.kind ==
         ExactFrozenIncidenceTokenKind::rooted_carrier;
}

struct FacetOccurrence {
  std::size_t group_index{};
  std::size_t facet_token_index{};
  std::size_t source_hyperedge_index{};
};

struct PointOccurrence {
  std::size_t group_index{};
  PointId point_id{};
  std::size_t source_hyperedge_index{};
};

// Expands every source facet occurrence and every distinct latent carrier in
// one hyperedge exactly once.  One reusable pair of buffers replaces the two
// formerly allocated closed-point vectors per hyperedge; the retained flat
// occurrence arenas are the scratch subsequently consumed by delta building.
[[nodiscard]] bool build_provenance_and_occurrences(
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    std::span<const ExactDirectFrozenUnifiedFacetResolution> resolutions,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
        latent_coverages,
    std::span<const PointId> latent_coverage_points,
    const std::vector<ExactFrozenIncidenceHyperedge>& hyperedges,
    const std::vector<ExactFrozenIncidenceToken>& tokens,
    const std::vector<std::size_t>& facet_token_indices,
    const std::vector<HyperedgeMeta>& metadata,
    const ExactFrozenIncidenceQuotientResult& quotient,
    const std::vector<PreparedGroupCoverage>& prepared_coverages,
    std::size_t closed_point_occurrence_capacity,
    std::vector<ExactFrozenIncidenceHyperedgeProvenance>& provenance,
    std::vector<FacetOccurrence>& facet_occurrences,
    std::vector<PointOccurrence>& point_occurrences) {
  if (metadata.size() != hyperedges.size() ||
      quotient.hyperedge_bindings.size() != hyperedges.size()) {
    return false;
  }
  provenance.reserve(hyperedges.size());
  facet_occurrences.reserve(facet_token_indices.size());
  point_occurrences.reserve(closed_point_occurrence_capacity);
  std::vector<PointId> closed_points;
  std::vector<ExactFrozenIncidenceTokenId> latent_token_ids;
  const auto maximum_reference_count = std::max_element(
      hyperedges.begin(),
      hyperedges.end(),
      [](const auto& left, const auto& right) {
        return left.token_reference_count < right.token_reference_count;
      });
  if (maximum_reference_count != hyperedges.end()) {
    latent_token_ids.reserve(maximum_reference_count->token_reference_count);
  }
  for (std::size_t hyperedge_index = 0U;
       hyperedge_index < hyperedges.size();
       ++hyperedge_index) {
    const auto& hyperedge = hyperedges[hyperedge_index];
    const std::size_t group_index =
        quotient.hyperedge_bindings[hyperedge_index].group_index;
    if (group_index >= prepared_coverages.size()) {
      return false;
    }
    closed_points.clear();
    latent_token_ids.clear();
    for (std::size_t local = 0U;
         local < hyperedge.token_reference_count;
         ++local) {
      const std::size_t reference_index =
          hyperedge.token_reference_offset + local;
      if (reference_index >= facet_token_indices.size() ||
          reference_index >= tokens.size()) {
        return false;
      }
      const std::size_t facet_token_index =
          facet_token_indices[reference_index];
      append_facet_points(source_plan, facet_token_index, closed_points);
      facet_occurrences.push_back(
          {group_index, facet_token_index, hyperedge_index});
      if (tokens[reference_index].kind ==
          ExactFrozenIncidenceTokenKind::latent_carrier) {
        latent_token_ids.push_back(tokens[reference_index].token_id);
      }
    }
    std::sort(latent_token_ids.begin(), latent_token_ids.end());
    latent_token_ids.erase(
        std::unique(latent_token_ids.begin(), latent_token_ids.end()),
        latent_token_ids.end());
    for (const ExactFrozenIncidenceTokenId token_id : latent_token_ids) {
      append_latent_coverage_points(
          token_id,
          latent_coverages,
          latent_coverage_points,
          closed_points);
    }
    canonicalize_points(closed_points);
    const auto& prior_points =
        prepared_coverages[group_index].prior_points;
    const std::size_t added_point_count =
        static_cast<std::size_t>(std::count_if(
            closed_points.begin(),
            closed_points.end(),
            [&prior_points](PointId point) {
              return !std::binary_search(
                  prior_points.begin(), prior_points.end(), point);
            }));
    bool every_facet_already_rooted = true;
    for (std::size_t local = 0U;
         local < hyperedge.token_reference_count;
         ++local) {
      every_facet_already_rooted =
          every_facet_already_rooted &&
          resolution_is_rooted(
              resolutions,
              facet_token_indices
                  [hyperedge.token_reference_offset + local]);
    }
    const bool residual =
        metadata[hyperedge_index].kind ==
        ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence;
    provenance.push_back(
        {hyperedge_index,
         metadata[hyperedge_index].kind,
         added_point_count,
         added_point_count == 0U,
         residual && every_facet_already_rooted &&
             added_point_count == 0U});
    for (const PointId point : closed_points) {
      point_occurrences.push_back(
          {group_index, point, hyperedge_index});
    }
  }
  return provenance.size() == hyperedges.size() &&
         facet_occurrences.size() == facet_token_indices.size();
}

[[nodiscard]] bool build_delta_records(
    std::span<const ExactDirectFrozenUnifiedFacetResolution> resolutions,
    const ExactFrozenIncidenceQuotientResult& quotient,
    const ExactFrozenIncidenceHgpActionPlanResult& action_plan,
    const std::vector<PreparedGroupCoverage>& prepared_coverages,
    std::vector<FacetOccurrence>& facet_occurrences,
    std::vector<PointOccurrence>& point_occurrences,
    std::vector<ExactDirectFrozenUnifiedCoverageDeltaRecord>& deltas,
    std::vector<ExactDirectFrozenUnifiedCoverageDeltaFacetReference>&
        delta_facets,
    std::vector<ExactDirectFrozenUnifiedCoverageDeltaPointReference>&
        delta_points) {
  std::sort(
      facet_occurrences.begin(),
      facet_occurrences.end(),
      [](const FacetOccurrence& left, const FacetOccurrence& right) {
        if (left.group_index != right.group_index) {
          return left.group_index < right.group_index;
        }
        if (left.facet_token_index != right.facet_token_index) {
          return left.facet_token_index < right.facet_token_index;
        }
        return left.source_hyperedge_index <
               right.source_hyperedge_index;
      });
  facet_occurrences.erase(
      std::unique(
          facet_occurrences.begin(),
          facet_occurrences.end(),
          [](const FacetOccurrence& left, const FacetOccurrence& right) {
            return left.group_index == right.group_index &&
                   left.facet_token_index == right.facet_token_index;
          }),
      facet_occurrences.end());
  std::sort(
      point_occurrences.begin(),
      point_occurrences.end(),
      [](const PointOccurrence& left, const PointOccurrence& right) {
        if (left.group_index != right.group_index) {
          return left.group_index < right.group_index;
        }
        if (left.point_id != right.point_id) {
          return left.point_id < right.point_id;
        }
        return left.source_hyperedge_index <
               right.source_hyperedge_index;
      });
  point_occurrences.erase(
      std::unique(
          point_occurrences.begin(),
          point_occurrences.end(),
          [](const PointOccurrence& left, const PointOccurrence& right) {
            return left.group_index == right.group_index &&
                   left.point_id == right.point_id;
          }),
      point_occurrences.end());

  deltas.reserve(action_plan.groups.size());
  std::size_t facet_cursor = 0U;
  std::size_t point_cursor = 0U;
  for (std::size_t group_index = 0U;
       group_index < action_plan.groups.size();
       ++group_index) {
    const auto& action_group = action_plan.groups[group_index];
    if (action_group.group_index != group_index ||
        group_index >= quotient.groups.size() ||
        group_index >= prepared_coverages.size()) {
      return false;
    }
    const std::size_t facet_offset = delta_facets.size();
    while (facet_cursor < facet_occurrences.size() &&
           facet_occurrences[facet_cursor].group_index == group_index) {
      const auto& occurrence = facet_occurrences[facet_cursor];
      if (!resolution_is_rooted(
              resolutions, occurrence.facet_token_index)) {
        delta_facets.push_back(
            {group_index,
             occurrence.source_hyperedge_index,
             occurrence.facet_token_index});
      }
      ++facet_cursor;
    }
    const std::size_t point_offset = delta_points.size();
    const auto& prior_points = prepared_coverages[group_index].prior_points;
    while (point_cursor < point_occurrences.size() &&
           point_occurrences[point_cursor].group_index == group_index) {
      const auto& occurrence = point_occurrences[point_cursor];
      if (!std::binary_search(
              prior_points.begin(),
              prior_points.end(),
              occurrence.point_id)) {
        delta_points.push_back(
            {group_index,
             occurrence.source_hyperedge_index,
             occurrence.point_id});
      }
      ++point_cursor;
    }
    const bool fully_redundant =
        delta_facets.size() == facet_offset &&
        delta_points.size() == point_offset;
    if (action_group.purely_residual &&
        action_group.fully_redundant_residual_group !=
            fully_redundant) {
      return false;
    }
    deltas.push_back(
        {deltas.size(),
         action_group.group_index,
         facet_offset,
         delta_facets.size() - facet_offset,
         point_offset,
         delta_points.size() - point_offset,
         action_group.q_r,
         action_group.action,
         fully_redundant});
  }
  return deltas.size() == action_plan.groups.size() &&
         facet_cursor == facet_occurrences.size() &&
         point_cursor == point_occurrences.size();
}

[[nodiscard]] bool build_residual_records(
    const std::vector<ExactFrozenIncidenceHyperedge>& hyperedges,
    const std::vector<HyperedgeMeta>& metadata,
    const ExactFrozenIncidenceQuotientResult& quotient,
    const std::vector<ExactFrozenIncidenceHyperedgeProvenance>& provenance,
    const std::vector<
        ExactDirectFrozenUnifiedCoverageDeltaFacetReference>& delta_facets,
    const std::vector<
        ExactDirectFrozenUnifiedCoverageDeltaPointReference>& delta_points,
    std::size_t required_residual_record_capacity,
    std::vector<ExactDirectFrozenUnifiedResidualIncidenceRecord>& records) {
  // Direct-saddle metadata coexists here but never produces a residual record.
  records.reserve(required_residual_record_capacity);
  std::vector<std::size_t> owned_facet_counts(hyperedges.size(), 0U);
  std::vector<std::size_t> owned_point_counts(hyperedges.size(), 0U);
  for (const auto& reference : delta_facets) {
    if (reference.first_source_hyperedge_index >= hyperedges.size() ||
        quotient.hyperedge_bindings
                [reference.first_source_hyperedge_index]
                    .group_index != reference.owner_group_index) {
      return false;
    }
    ++owned_facet_counts[reference.first_source_hyperedge_index];
  }
  for (const auto& reference : delta_points) {
    if (reference.first_source_hyperedge_index >= hyperedges.size() ||
        quotient.hyperedge_bindings
                [reference.first_source_hyperedge_index]
                    .group_index != reference.owner_group_index) {
      return false;
    }
    ++owned_point_counts[reference.first_source_hyperedge_index];
  }
  for (std::size_t hyperedge_index = 0U;
       hyperedge_index < metadata.size();
       ++hyperedge_index) {
    if (metadata[hyperedge_index].kind !=
        ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence) {
      continue;
    }
    if (hyperedge_index >= hyperedges.size() ||
        hyperedge_index >= provenance.size() ||
        provenance[hyperedge_index].source_hyperedge_index !=
            hyperedge_index ||
        provenance[hyperedge_index].kind !=
            ExactFrozenIncidenceHyperedgeProvenanceKind::
                residual_incidence) {
      return false;
    }
    const auto& hyperedge = hyperedges[hyperedge_index];
    const std::size_t owned_facet_count =
        owned_facet_counts[hyperedge_index];
    const std::size_t owned_point_count =
        owned_point_counts[hyperedge_index];
    const auto& local = provenance[hyperedge_index];
    records.push_back(
        {records.size(),
         metadata[hyperedge_index].source_reference_index,
         metadata[hyperedge_index].source_star_coface_index,
         hyperedge_index,
         quotient.hyperedge_bindings[hyperedge_index].group_index,
         hyperedge.token_reference_offset,
         hyperedge.token_reference_count,
         owned_facet_count,
         owned_point_count,
         local.added_point_count,
         local.zero_point_delta_certified,
         local.fully_redundant,
         owned_facet_count == 0U && owned_point_count == 0U});
  }
  return records.size() == required_residual_record_capacity;
}

[[nodiscard]] bool build_equal_bindings(
    std::span<const ExactDirectFrozenUnifiedFacetResolution> resolutions,
    const ExactFrozenIncidenceQuotientResult& quotient,
    const std::vector<ExactDirectFrozenUnifiedCoverageDeltaFacetReference>&
        delta_facets,
    std::vector<ExactDirectFrozenUnifiedEqualFacetBindingRecord>& records) {
  records.reserve(resolutions.size());
  for (const auto& resolution : resolutions) {
    if (resolution.token.kind !=
        ExactFrozenIncidenceTokenKind::equal_facet) {
      continue;
    }
    const auto* binding = find_token_binding(quotient, resolution.token);
    if (binding == nullptr) {
      return false;
    }
    const auto key =
        std::pair<std::size_t, std::size_t>{
            binding->group_index, resolution.facet_token_index};
    const auto facet = std::lower_bound(
        delta_facets.begin(),
        delta_facets.end(),
        key,
        [](const auto& reference, const auto& candidate) {
          return reference.owner_group_index < candidate.first ||
                 (reference.owner_group_index == candidate.first &&
                  reference.facet_token_index < candidate.second);
        });
    if (facet == delta_facets.end() ||
        facet->owner_group_index != binding->group_index ||
        facet->facet_token_index != resolution.facet_token_index) {
      return false;
    }
    records.push_back(
        {records.size(),
         resolution.facet_token_index,
         resolution.token.token_id,
         binding->group_index,
         static_cast<std::size_t>(facet - delta_facets.begin())});
  }
  return true;
}

[[nodiscard]] std::size_t logical_output_count(
    const ExactDirectFrozenUnifiedIncidenceBatchResult& result) {
  std::size_t count = 0U;
  const std::size_t increments[] = {
      result.quotient_hyperedges.size(),
      result.quotient_token_references.size(),
      result.incidence_facet_token_indices.size(),
      result.provenance.size(),
      result.root_attachments.size(),
      result.quotient.counters.logical_output_entry_count,
      result.action_plan.counters.logical_output_entry_count,
      result.residual_incidence_records.size(),
      result.equal_facet_binding_records.size(),
      result.coverage_deltas.size(),
      result.coverage_delta_facets.size(),
      result.coverage_delta_points.size(),
  };
  for (const std::size_t increment : increments) {
    if (!add_to(increment, count)) {
      throw std::overflow_error(
          "a frozen unified logical output count overflowed");
    }
  }
  return count;
}

[[nodiscard]] bool delta_arenas_are_canonical_partitions(
    const ExactDirectFrozenUnifiedIncidenceBatchResult& result) noexcept;

[[nodiscard]] const ExactDirectFrozenUnifiedCoverageDeltaFacetReference*
find_delta_facet_reference(
    const ExactDirectFrozenUnifiedIncidenceBatchResult& result,
    std::size_t group_index,
    std::size_t facet_token_index) noexcept {
  if (group_index >= result.coverage_deltas.size()) {
    return nullptr;
  }
  const auto& delta = result.coverage_deltas[group_index];
  if (!range_is_valid(
          delta.facet_reference_offset,
          delta.facet_reference_count,
          result.coverage_delta_facets.size())) {
    return nullptr;
  }
  const auto begin = result.coverage_delta_facets.begin() +
      static_cast<std::ptrdiff_t>(delta.facet_reference_offset);
  const auto end = begin +
      static_cast<std::ptrdiff_t>(delta.facet_reference_count);
  const auto found = std::lower_bound(
      begin,
      end,
      facet_token_index,
      [](const auto& reference, std::size_t candidate) {
        return reference.facet_token_index < candidate;
      });
  return found != end && found->facet_token_index == facet_token_index
      ? &*found
      : nullptr;
}

[[nodiscard]] bool hyperedge_contains_facet(
    const ExactDirectFrozenUnifiedIncidenceBatchResult& result,
    std::size_t hyperedge_index,
    std::size_t facet_token_index,
    const ExactFrozenIncidenceToken* expected_token = nullptr) noexcept {
  if (hyperedge_index >= result.quotient_hyperedges.size()) {
    return false;
  }
  const auto& hyperedge = result.quotient_hyperedges[hyperedge_index];
  if (hyperedge.hyperedge_index != hyperedge_index ||
      !range_is_valid(
          hyperedge.token_reference_offset,
          hyperedge.token_reference_count,
          result.incidence_facet_token_indices.size()) ||
      !range_is_valid(
          hyperedge.token_reference_offset,
          hyperedge.token_reference_count,
          result.quotient_token_references.size())) {
    return false;
  }
  for (std::size_t local = 0U;
       local < hyperedge.token_reference_count;
       ++local) {
    const std::size_t reference_index =
        hyperedge.token_reference_offset + local;
    if (result.incidence_facet_token_indices[reference_index] ==
            facet_token_index &&
        (expected_token == nullptr ||
         result.quotient_token_references[reference_index] ==
             *expected_token)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool facts_match_payload(
    const ExactDirectFrozenUnifiedIncidenceBatchResult& result) noexcept {
  if (!delta_arenas_are_canonical_partitions(result) ||
      result.coverage_deltas.size() != result.action_plan.groups.size() ||
      result.quotient_hyperedges.size() !=
          result.quotient.hyperedge_bindings.size() ||
      result.provenance.size() != result.quotient_hyperedges.size()) {
    return false;
  }

  std::size_t direct_hyperedge_count = 0U;
  std::size_t residual_hyperedge_count = 0U;
  for (std::size_t hyperedge_index = 0U;
       hyperedge_index < result.provenance.size();
       ++hyperedge_index) {
    const auto& provenance = result.provenance[hyperedge_index];
    if (provenance.source_hyperedge_index != hyperedge_index ||
        provenance.zero_point_delta_certified !=
            (provenance.added_point_count == 0U) ||
        (provenance.fully_redundant &&
         (provenance.kind !=
              ExactFrozenIncidenceHyperedgeProvenanceKind::
                  residual_incidence ||
          !provenance.zero_point_delta_certified))) {
      return false;
    }
    switch (provenance.kind) {
      case ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family:
        ++direct_hyperedge_count;
        break;
      case ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence:
        ++residual_hyperedge_count;
        break;
    }
  }

  std::size_t local_zero_count = 0U;
  std::size_t canonical_ownerless_count = 0U;
  std::size_t recorded_residual_facet_ownership = 0U;
  std::size_t recorded_residual_point_ownership = 0U;
  std::size_t previous_source_hyperedge_index = 0U;
  for (std::size_t record_index = 0U;
       record_index < result.residual_incidence_records.size();
       ++record_index) {
    const auto& record = result.residual_incidence_records[record_index];
    if (record.residual_incidence_record_index != record_index ||
        record.source_hyperedge_index >= result.quotient_hyperedges.size() ||
        record.source_hyperedge_index >= result.provenance.size() ||
        record.source_hyperedge_index >=
            result.quotient.hyperedge_bindings.size() ||
        (record_index != 0U &&
         previous_source_hyperedge_index >= record.source_hyperedge_index)) {
      return false;
    }
    previous_source_hyperedge_index = record.source_hyperedge_index;
    const auto& hyperedge =
        result.quotient_hyperedges[record.source_hyperedge_index];
    const auto& provenance =
        result.provenance[record.source_hyperedge_index];
    if (record.owner_group_index !=
            result.quotient.hyperedge_bindings
                [record.source_hyperedge_index]
                    .group_index ||
        record.facet_reference_offset !=
            hyperedge.token_reference_offset ||
        record.facet_reference_count !=
            hyperedge.token_reference_count ||
        provenance.kind !=
            ExactFrozenIncidenceHyperedgeProvenanceKind::
                residual_incidence ||
        record.local_added_point_count != provenance.added_point_count ||
        record.local_zero_point_delta_certified !=
            provenance.zero_point_delta_certified ||
        record.local_fully_redundant_certified !=
            provenance.fully_redundant ||
        record.local_zero_point_delta_certified !=
            (record.local_added_point_count == 0U) ||
        record.canonically_owns_no_coverage_delta !=
            (record.canonically_owned_facet_delta_count == 0U &&
             record.canonically_owned_point_delta_count == 0U)) {
      return false;
    }
    if (!add_to(
            record.canonically_owned_facet_delta_count,
            recorded_residual_facet_ownership) ||
        !add_to(
            record.canonically_owned_point_delta_count,
            recorded_residual_point_ownership)) {
      return false;
    }
    if (record.local_zero_point_delta_certified) {
      ++local_zero_count;
    }
    if (record.canonically_owns_no_coverage_delta) {
      ++canonical_ownerless_count;
    }
  }

  std::size_t observed_residual_facet_ownership = 0U;
  for (const auto& reference : result.coverage_delta_facets) {
    if (!hyperedge_contains_facet(
            result,
            reference.first_source_hyperedge_index,
            reference.facet_token_index)) {
      return false;
    }
    if (result.provenance[reference.first_source_hyperedge_index].kind ==
        ExactFrozenIncidenceHyperedgeProvenanceKind::
            residual_incidence) {
      ++observed_residual_facet_ownership;
    }
  }
  std::size_t observed_residual_point_ownership = 0U;
  for (const auto& reference : result.coverage_delta_points) {
    if (reference.first_source_hyperedge_index >=
        result.provenance.size()) {
      return false;
    }
    if (result.provenance[reference.first_source_hyperedge_index].kind ==
        ExactFrozenIncidenceHyperedgeProvenanceKind::
            residual_incidence) {
      ++observed_residual_point_ownership;
    }
  }
  if (recorded_residual_facet_ownership !=
          observed_residual_facet_ownership ||
      recorded_residual_point_ownership !=
          observed_residual_point_ownership) {
    return false;
  }

  std::size_t previous_equal_facet_token_index = 0U;
  for (std::size_t record_index = 0U;
       record_index < result.equal_facet_binding_records.size();
       ++record_index) {
    const auto& record = result.equal_facet_binding_records[record_index];
    if (record.equal_facet_binding_record_index != record_index ||
        (record_index != 0U &&
         previous_equal_facet_token_index >= record.facet_token_index) ||
        record.coverage_delta_facet_reference_index >=
            result.coverage_delta_facets.size()) {
      return false;
    }
    previous_equal_facet_token_index = record.facet_token_index;
    const auto& delta_reference = result.coverage_delta_facets
        [record.coverage_delta_facet_reference_index];
    const ExactFrozenIncidenceToken token{
        ExactFrozenIncidenceTokenKind::equal_facet,
        record.equal_facet_token_id};
    const auto* binding = find_token_binding(result.quotient, token);
    if (binding == nullptr ||
        binding->group_index != record.owner_group_index ||
        delta_reference.owner_group_index != record.owner_group_index ||
        delta_reference.facet_token_index != record.facet_token_index ||
        !hyperedge_contains_facet(
            result,
            delta_reference.first_source_hyperedge_index,
            record.facet_token_index,
            &token)) {
      return false;
    }
  }

  std::size_t q_r_zero_count = 0U;
  std::size_t q_r_one_count = 0U;
  std::size_t q_r_multiple_count = 0U;
  std::size_t fully_redundant_delta_count = 0U;
  for (std::size_t group_index = 0U;
       group_index < result.coverage_deltas.size();
       ++group_index) {
    const auto& delta = result.coverage_deltas[group_index];
    const auto& action_group = result.action_plan.groups[group_index];
    if (action_group.group_index != group_index ||
        delta.q_r != action_group.q_r ||
        delta.action != action_group.action ||
        delta.fully_redundant !=
            (delta.facet_reference_count == 0U &&
             delta.point_reference_count == 0U) ||
        (action_group.purely_residual &&
         delta.fully_redundant !=
             action_group.fully_redundant_residual_group)) {
      return false;
    }
    if (delta.q_r == 0U) {
      ++q_r_zero_count;
    } else if (delta.q_r == 1U) {
      ++q_r_one_count;
    } else {
      ++q_r_multiple_count;
    }
    if (delta.fully_redundant) {
      ++fully_redundant_delta_count;
    }
  }

  // Facet occurrences are retained, unlike the expanded point occurrences.
  // Replaying them here proves both membership and the true first hyperedge
  // owner without reconstructing any geometric carrier coverage.
  for (std::size_t hyperedge_index = 0U;
       hyperedge_index < result.quotient_hyperedges.size();
       ++hyperedge_index) {
    const auto& hyperedge = result.quotient_hyperedges[hyperedge_index];
    if (!range_is_valid(
            hyperedge.token_reference_offset,
            hyperedge.token_reference_count,
            result.quotient_token_references.size()) ||
        !range_is_valid(
            hyperedge.token_reference_offset,
            hyperedge.token_reference_count,
            result.incidence_facet_token_indices.size())) {
      return false;
    }
    const std::size_t group_index =
        result.quotient.hyperedge_bindings[hyperedge_index].group_index;
    for (std::size_t local = 0U;
         local < hyperedge.token_reference_count;
         ++local) {
      const std::size_t reference_index =
          hyperedge.token_reference_offset + local;
      const auto* delta_reference = find_delta_facet_reference(
          result,
          group_index,
          result.incidence_facet_token_indices[reference_index]);
      const bool rooted =
          result.quotient_token_references[reference_index].kind ==
          ExactFrozenIncidenceTokenKind::rooted_carrier;
      if ((rooted && delta_reference != nullptr) ||
          (!rooted &&
           (delta_reference == nullptr ||
            delta_reference->first_source_hyperedge_index >
                hyperedge_index))) {
        return false;
      }
    }
  }

  return result.counters.hyperedge_count ==
             result.quotient_hyperedges.size() &&
         result.counters.token_reference_count ==
             result.quotient_token_references.size() &&
         result.incidence_facet_token_indices.size() ==
             result.quotient_token_references.size() &&
         result.provenance.size() == result.quotient_hyperedges.size() &&
         result.counters.root_attachment_count ==
             result.root_attachments.size() &&
         result.counters.group_count == result.action_plan.groups.size() &&
         result.counters.direct_saddle_hyperedge_count ==
             direct_hyperedge_count &&
         result.counters.residual_hyperedge_count ==
             residual_hyperedge_count &&
         result.counters.direct_saddle_hyperedge_count ==
             result.action_plan.counters
                 .direct_hyperedge_reference_count &&
         result.counters.residual_hyperedge_count ==
             result.action_plan.counters
                 .residual_hyperedge_reference_count &&
         result.counters.q_r_zero_group_count == q_r_zero_count &&
         result.counters.q_r_one_group_count == q_r_one_count &&
         result.counters.q_r_multiple_group_count == q_r_multiple_count &&
         result.counters.q_r_zero_group_count ==
             result.quotient.counters.q_r_zero_group_count &&
         result.counters.q_r_one_group_count ==
             result.quotient.counters.q_r_one_group_count &&
         result.counters.q_r_multiple_group_count ==
             result.quotient.counters.q_r_multiple_group_count &&
         result.counters.residual_incidence_record_count ==
             result.residual_incidence_records.size() &&
         result.counters.equal_facet_binding_record_count ==
             result.equal_facet_binding_records.size() &&
         result.counters.coverage_delta_record_count ==
             result.coverage_deltas.size() &&
         result.counters.coverage_delta_facet_reference_count ==
             result.coverage_delta_facets.size() &&
         result.counters.coverage_delta_point_reference_count ==
             result.coverage_delta_points.size() &&
         result.counters.fully_redundant_coverage_delta_count ==
             fully_redundant_delta_count &&
         result.counters.residual_local_zero_point_delta_count ==
             local_zero_count &&
         result.counters.residual_canonical_ownerless_delta_count ==
             canonical_ownerless_count;
}

[[nodiscard]] bool delta_arenas_are_canonical_partitions(
    const ExactDirectFrozenUnifiedIncidenceBatchResult& result) noexcept {
  if (result.coverage_deltas.size() != result.action_plan.groups.size() ||
      result.coverage_deltas.size() != result.quotient.groups.size() ||
      result.quotient.hyperedge_bindings.size() !=
          result.quotient_hyperedges.size()) {
    return false;
  }
  std::size_t facet_cursor = 0U;
  std::size_t point_cursor = 0U;
  for (std::size_t group_index = 0U;
       group_index < result.coverage_deltas.size();
       ++group_index) {
    const auto& delta = result.coverage_deltas[group_index];
    const auto& action_group = result.action_plan.groups[group_index];
    if (delta.coverage_delta_record_index != group_index ||
        delta.owner_group_index != group_index ||
        action_group.group_index != group_index ||
        delta.q_r != action_group.q_r ||
        delta.action != action_group.action ||
        delta.fully_redundant !=
            (delta.facet_reference_count == 0U &&
             delta.point_reference_count == 0U) ||
        (action_group.purely_residual &&
         delta.fully_redundant !=
             action_group.fully_redundant_residual_group) ||
        delta.facet_reference_offset != facet_cursor ||
        delta.point_reference_offset != point_cursor ||
        !range_is_valid(
            facet_cursor,
            delta.facet_reference_count,
            result.coverage_delta_facets.size()) ||
        !range_is_valid(
            point_cursor,
            delta.point_reference_count,
            result.coverage_delta_points.size())) {
      return false;
    }
    for (std::size_t local = 0U;
         local < delta.facet_reference_count;
         ++local) {
      const auto& reference =
          result.coverage_delta_facets[facet_cursor + local];
      if (reference.owner_group_index != group_index ||
          reference.first_source_hyperedge_index >=
              result.quotient_hyperedges.size() ||
          reference.first_source_hyperedge_index >=
              result.quotient.hyperedge_bindings.size() ||
          result.quotient.hyperedge_bindings
                  [reference.first_source_hyperedge_index]
                      .group_index != group_index ||
          (local != 0U &&
           result.coverage_delta_facets[facet_cursor + local - 1U]
                   .facet_token_index >= reference.facet_token_index)) {
        return false;
      }
    }
    for (std::size_t local = 0U;
         local < delta.point_reference_count;
         ++local) {
      const auto& reference =
          result.coverage_delta_points[point_cursor + local];
      if (reference.owner_group_index != group_index ||
          reference.first_source_hyperedge_index >=
              result.quotient_hyperedges.size() ||
          reference.first_source_hyperedge_index >=
              result.quotient.hyperedge_bindings.size() ||
          result.quotient.hyperedge_bindings
                  [reference.first_source_hyperedge_index]
                      .group_index != group_index ||
          (local != 0U &&
           result.coverage_delta_points[point_cursor + local - 1U]
                   .point_id >= reference.point_id)) {
        return false;
      }
    }
    facet_cursor += delta.facet_reference_count;
    point_cursor += delta.point_reference_count;
  }
  return facet_cursor == result.coverage_delta_facets.size() &&
         point_cursor == result.coverage_delta_points.size();
}

}  // namespace

bool ExactDirectFrozenUnifiedIncidenceBatchResult::
    certified_frozen_unified_incidence_batch() const noexcept {
  const bool source_scope_certified =
      successive_star_source_authority !=
          normalized_direct_source_authority &&
      ((successive_star_source_authority &&
        scope == ExactDirectFrozenUnifiedIncidenceBatchScope::
                     exact_selected_batch_relative_to_supplied_successive_star_and_external_facet_resolution_prior_root_and_latent_carrier_coverage_authorities_only) ||
       (normalized_direct_source_authority &&
        scope == ExactDirectFrozenUnifiedIncidenceBatchScope::
                     exact_selected_batch_relative_to_verified_normalized_direct_source_and_external_facet_resolution_prior_root_and_latent_carrier_coverage_authorities_only));
  return schema_version ==
             direct_frozen_unified_incidence_batch_schema_version &&
         (source_plan_freshly_verified !=
          source_plan_verified_once_by_immutable_resident_authority) &&
         selected_batch_partition_freshly_replayed &&
         direct_births_excluded_and_deferred &&
         one_hyperedge_per_direct_saddle_and_residual &&
         every_hyperedge_uses_all_factorized_deletions &&
         facet_resolution_authority_canonical_and_exhaustive &&
         typed_tokens_deduplicated_by_frozen_quotient &&
         prior_root_coverage_csr_canonical_and_exhaustive &&
         rooted_facets_covered_by_their_prior_roots &&
         latent_carrier_coverage_csr_canonical_and_exhaustive &&
         latent_facets_covered_by_their_carriers &&
         frozen_quotient_freshly_streaming_verified &&
         frozen_hgp_action_plan_freshly_streaming_verified &&
         residual_incidence_records_canonical_and_exhaustive &&
         equal_facet_bindings_canonical_and_exhaustive &&
         coverage_deltas_are_exact_facet_and_point_set_differences &&
         flat_delta_arenas_have_canonical_group_owners &&
         all_output_within_budget &&
         no_partial_scientific_payload_published &&
         !reducer_locator_forest_or_caller_state_mutated &&
         !global_facet_coface_or_gamma_catalog_materialized &&
         !supplied_star_global_completeness_claimed &&
         !public_status_claimed && budget_is_sufficient(*this) &&
         required_facet_resolution_capacity ==
             counters.facet_resolution_count &&
         required_prior_root_coverage_capacity ==
             counters.prior_root_coverage_count &&
         required_prior_root_coverage_point_reference_capacity ==
             counters.prior_root_coverage_point_reference_count &&
         required_latent_carrier_coverage_capacity ==
             counters.latent_carrier_coverage_count &&
         required_latent_carrier_coverage_point_reference_capacity ==
             counters.latent_carrier_coverage_point_reference_count &&
         required_batch_direct_reference_scan_capacity ==
             counters.batch_direct_reference_scan_count &&
         required_direct_saddle_hyperedge_capacity ==
             counters.direct_saddle_hyperedge_count &&
         required_residual_hyperedge_capacity ==
             counters.residual_hyperedge_count &&
         required_coface_facet_reference_scan_capacity ==
             counters.coface_facet_reference_scan_count &&
         required_source_point_reference_scan_capacity ==
             counters.source_point_reference_scan_count &&
         required_hyperedge_capacity == counters.hyperedge_count &&
         required_token_reference_capacity ==
             counters.token_reference_count &&
         counters.distinct_typed_token_count <=
             required_distinct_typed_token_capacity &&
         counters.root_attachment_count <=
             required_root_attachment_capacity &&
         counters.group_count <= required_group_capacity &&
         counters.residual_incidence_record_count <=
             required_residual_incidence_record_capacity &&
         counters.equal_facet_binding_record_count <=
             required_equal_facet_binding_record_capacity &&
         counters.coverage_delta_record_count <=
             required_coverage_delta_record_capacity &&
         counters.coverage_delta_facet_reference_count <=
             required_coverage_delta_facet_reference_capacity &&
         counters.coverage_delta_point_reference_count <=
             required_coverage_delta_point_reference_capacity &&
         counters.logical_scratch_entry_count <=
             required_scratch_entry_capacity &&
         counters.logical_output_entry_count <=
             required_logical_output_entry_capacity &&
         facts_match_payload(*this) &&
         quotient.certified_frozen_incidence_quotient() &&
         action_plan.certified_frozen_incidence_hgp_action_plan() &&
         decision == ExactDirectFrozenUnifiedIncidenceBatchDecision::
                         complete_certified_frozen_unified_incidence_batch &&
         source_scope_certified;
}

bool ExactDirectFrozenUnifiedIncidenceBatchResult::atomic_empty_failure()
    const noexcept {
  return schema_version ==
             direct_frozen_unified_incidence_batch_schema_version &&
         decision !=
             ExactDirectFrozenUnifiedIncidenceBatchDecision::not_certified &&
         decision != ExactDirectFrozenUnifiedIncidenceBatchDecision::
                         complete_certified_frozen_unified_incidence_batch &&
         quotient_hyperedges.empty() &&
         quotient_token_references.empty() &&
         incidence_facet_token_indices.empty() && provenance.empty() &&
         root_attachments.empty() &&
         quotient == ExactFrozenIncidenceQuotientResult{} &&
         action_plan == ExactFrozenIncidenceHgpActionPlanResult{} &&
         residual_incidence_records.empty() &&
         equal_facet_binding_records.empty() && coverage_deltas.empty() &&
         coverage_delta_facets.empty() && coverage_delta_points.empty() &&
         counters == ExactDirectFrozenUnifiedIncidenceBatchCounters{} &&
         !all_output_within_budget &&
         no_partial_scientific_payload_published &&
         !reducer_locator_forest_or_caller_state_mutated &&
         !global_facet_coface_or_gamma_catalog_materialized &&
         !supplied_star_global_completeness_claimed &&
         !public_status_claimed &&
         (successive_star_source_authority !=
          normalized_direct_source_authority) &&
         ((successive_star_source_authority &&
           scope == ExactDirectFrozenUnifiedIncidenceBatchScope::
                        exact_selected_batch_relative_to_supplied_successive_star_and_external_facet_resolution_prior_root_and_latent_carrier_coverage_authorities_only) ||
          (normalized_direct_source_authority &&
           scope == ExactDirectFrozenUnifiedIncidenceBatchScope::
                        exact_selected_batch_relative_to_verified_normalized_direct_source_and_external_facet_resolution_prior_root_and_latent_carrier_coverage_authorities_only));
}

namespace {

struct VerifiedPlanBuildAudit {
  std::size_t batch_construction_count{};
  std::size_t quotient_streaming_verification_count{};
  std::size_t action_plan_streaming_verification_count{};
  std::size_t structural_certification_count{};
};

// Both callers enter after certification of this exact plan.  Standalone
// callers perform that verification freshly for the current call; the
// resident path holds a non-forgeable internal lease over its const plan.
// The batch itself is rebuilt on every entry in both modes.
[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchResult
build_exact_direct_frozen_unified_incidence_batch_from_verified_plan_authority(
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    VerifiedPlanAuthorityKind authority_kind,
    VerifiedPlanBuildAudit* audit,
    std::size_t batch_index,
    std::span<const ExactDirectFrozenUnifiedFacetResolution>
        facet_resolutions,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
        prior_root_coverages,
    std::span<const PointId> prior_root_coverage_point_references,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
        latent_carrier_coverages,
    std::span<const PointId> latent_carrier_coverage_point_references,
    const ExactDirectFrozenUnifiedIncidenceBatchBudget& budget) {
  ExactDirectFrozenUnifiedIncidenceBatchResult result =
      base_result(batch_index, budget, authority_kind);
  if (audit != nullptr) {
    ++audit->batch_construction_count;
  }
  try {
    result.source_plan_freshly_verified =
        authority_kind ==
        VerifiedPlanAuthorityKind::
            freshly_verified_standalone_successive_star;
    result.source_plan_verified_once_by_immutable_resident_authority =
        authority_kind != VerifiedPlanAuthorityKind::
                              freshly_verified_standalone_successive_star;
    if (batch_index >= source_plan.batches.size() ||
        source_plan.batches[batch_index].batch_index != batch_index) {
      return fail(
          std::move(result),
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              no_batch_index_rejected);
    }
    const auto& batch = source_plan.batches[batch_index];
    if (!initialize_requirements(
            source_plan,
            batch,
            facet_resolutions,
            prior_root_coverages,
            prior_root_coverage_point_references,
            latent_carrier_coverages,
            latent_carrier_coverage_point_references,
            result)) {
      return fail(
          std::move(result),
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              no_batch_capacity_overflow);
    }
    if (!budget_is_sufficient(result)) {
      return fail(
          std::move(result),
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              no_batch_budget_exhausted);
    }

    std::size_t closed_point_occurrence_capacity = 0U;
    bool preflight_facet_authority_rejected = false;
    bool preflight_latent_authority_rejected = false;
    if (!preflight_requirements_from_raw_authorities(
            source_plan,
            batch,
            facet_resolutions,
            prior_root_coverages,
            prior_root_coverage_point_references,
            latent_carrier_coverages,
            latent_carrier_coverage_point_references,
            result,
            closed_point_occurrence_capacity,
            preflight_facet_authority_rejected,
            preflight_latent_authority_rejected)) {
      return fail(
          std::move(result),
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              no_batch_capacity_overflow);
    }
    if (!budget_is_sufficient(result)) {
      return fail(
          std::move(result),
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              no_batch_budget_exhausted);
    }
    if (preflight_facet_authority_rejected ||
        preflight_latent_authority_rejected) {
      return fail(
          std::move(result),
          preflight_latent_authority_rejected
              ? ExactDirectFrozenUnifiedIncidenceBatchDecision::
                    no_batch_latent_carrier_coverage_authority_rejected
              : ExactDirectFrozenUnifiedIncidenceBatchDecision::
                    no_batch_facet_resolution_authority_rejected);
    }

    std::vector<std::size_t> touched_facet_tokens;
    std::vector<ExactFrozenIncidenceRootAttachment> attachments;
    bool latent_authority_rejected = false;
    if (!validate_authorities(
            source_plan,
            batch,
            facet_resolutions,
            prior_root_coverages,
            prior_root_coverage_point_references,
            latent_carrier_coverages,
            latent_carrier_coverage_point_references,
            latent_authority_rejected,
            attachments,
            touched_facet_tokens)) {
      return fail(
          std::move(result),
          latent_authority_rejected
              ? ExactDirectFrozenUnifiedIncidenceBatchDecision::
                    no_batch_latent_carrier_coverage_authority_rejected
              : ExactDirectFrozenUnifiedIncidenceBatchDecision::
                    no_batch_facet_resolution_authority_rejected);
    }
    result.facet_resolution_authority_canonical_and_exhaustive = true;
    result.prior_root_coverage_csr_canonical_and_exhaustive = true;
    result.rooted_facets_covered_by_their_prior_roots = true;
    result.latent_carrier_coverage_csr_canonical_and_exhaustive = true;
    result.latent_facets_covered_by_their_carriers = true;

    std::vector<ExactFrozenIncidenceHyperedge> hyperedges;
    std::vector<ExactFrozenIncidenceToken> tokens;
    std::vector<std::size_t> facet_token_indices;
    std::vector<HyperedgeMeta> metadata;
    std::size_t deferred_birth_count = 0U;
    if (!build_factorized_authorities(
            source_plan,
            batch,
            facet_resolutions,
            hyperedges,
            tokens,
            facet_token_indices,
            metadata,
            result.required_hyperedge_capacity,
            deferred_birth_count)) {
      return fail(
          std::move(result),
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              no_batch_factorized_hyperedge_reconstruction_rejected);
    }
    result.selected_batch_partition_freshly_replayed = true;
    result.direct_births_excluded_and_deferred = true;
    result.one_hyperedge_per_direct_saddle_and_residual =
        hyperedges.size() ==
        result.required_hyperedge_capacity;
    result.every_hyperedge_uses_all_factorized_deletions =
        tokens.size() == batch.coface_facet_reference_count;
    if (!result.one_hyperedge_per_direct_saddle_and_residual ||
        !result.every_hyperedge_uses_all_factorized_deletions) {
      return fail(
          std::move(result),
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              no_batch_factorized_hyperedge_reconstruction_rejected);
    }

    const auto quotient_budget = quotient_budget_from(budget);
    auto quotient = build_exact_direct_frozen_incidence_quotient(
        hyperedges, tokens, quotient_budget);
    if (audit != nullptr) {
      ++audit->quotient_streaming_verification_count;
    }
    auto verified_quotient_authority =
        internal::ExactFrozenIncidenceVerifiedQuotientAuthorityFactory::
            verify_once(hyperedges, tokens, quotient_budget, quotient);
    if (!verified_quotient_authority.has_value()) {
      return fail(
          std::move(result),
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              no_batch_frozen_incidence_quotient_rejected);
    }
    result.typed_tokens_deduplicated_by_frozen_quotient = true;
    result.frozen_quotient_freshly_streaming_verified = true;

    std::vector<PreparedGroupCoverage> prepared_coverages;
    std::vector<ExactFrozenIncidenceHyperedgeProvenance> provenance;
    std::vector<FacetOccurrence> facet_occurrences;
    std::vector<PointOccurrence> point_occurrences;
    if (!prepare_group_coverages(
            attachments,
            quotient,
            prior_root_coverages,
            prior_root_coverage_point_references,
            prepared_coverages) ||
        !build_provenance_and_occurrences(
            source_plan,
            facet_resolutions,
            latent_carrier_coverages,
            latent_carrier_coverage_point_references,
            hyperedges,
            tokens,
            facet_token_indices,
            metadata,
            quotient,
            prepared_coverages,
            closed_point_occurrence_capacity,
            provenance,
            facet_occurrences,
            point_occurrences)) {
      return fail(
          std::move(result),
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              no_batch_prior_root_coverage_authority_rejected);
    }

    const auto action_budget = action_budget_from(budget);
    const bool resident_authority =
        authority_kind != VerifiedPlanAuthorityKind::
                              freshly_verified_standalone_successive_star;
    auto action_plan = [&]() {
      if (resident_authority) {
        return internal::
            build_exact_direct_frozen_incidence_hgp_action_plan_from_verified_quotient(
                *verified_quotient_authority,
                provenance,
                attachments,
                action_budget);
      }
      return build_exact_direct_frozen_incidence_hgp_action_plan(
          hyperedges,
          tokens,
          quotient_budget,
          quotient,
          provenance,
          attachments,
          action_budget);
    }();
    if (audit != nullptr) {
      ++audit->action_plan_streaming_verification_count;
    }
    const auto action_verification = [&]() {
      if (resident_authority) {
        return internal::
            verify_exact_direct_frozen_incidence_hgp_action_plan_streaming_from_verified_quotient(
                *verified_quotient_authority,
                provenance,
                attachments,
                action_budget,
                action_plan);
      }
      return verify_exact_direct_frozen_incidence_hgp_action_plan_streaming(
          hyperedges,
          tokens,
          quotient_budget,
          quotient,
          provenance,
          attachments,
          action_budget,
          action_plan);
    }();
    if (!action_verification.result_certified) {
      return fail(
          std::move(result),
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              no_batch_frozen_hgp_action_plan_rejected);
    }
    result.frozen_hgp_action_plan_freshly_streaming_verified = true;

    std::vector<ExactDirectFrozenUnifiedResidualIncidenceRecord>
        residual_records;
    std::vector<ExactDirectFrozenUnifiedEqualFacetBindingRecord>
        equal_bindings;
    std::vector<ExactDirectFrozenUnifiedCoverageDeltaRecord> deltas;
    std::vector<ExactDirectFrozenUnifiedCoverageDeltaFacetReference>
        delta_facets;
    std::vector<ExactDirectFrozenUnifiedCoverageDeltaPointReference>
        delta_points;
    if (!build_delta_records(
            facet_resolutions,
            quotient,
            action_plan,
            prepared_coverages,
            facet_occurrences,
            point_occurrences,
            deltas,
            delta_facets,
            delta_points) ||
        !build_residual_records(
            hyperedges,
            metadata,
            quotient,
            provenance,
            delta_facets,
            delta_points,
            result.required_residual_incidence_record_capacity,
            residual_records) ||
        !build_equal_bindings(
            facet_resolutions,
            quotient,
            delta_facets,
            equal_bindings)) {
      return fail(
          std::move(result),
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              no_batch_coverage_delta_scope_rejected);
    }

    result.quotient_hyperedges = std::move(hyperedges);
    result.quotient_token_references = std::move(tokens);
    result.incidence_facet_token_indices =
        std::move(facet_token_indices);
    result.provenance = std::move(provenance);
    result.root_attachments = std::move(attachments);
    result.quotient = std::move(quotient);
    result.action_plan = std::move(action_plan);
    result.residual_incidence_records = std::move(residual_records);
    result.equal_facet_binding_records = std::move(equal_bindings);
    result.coverage_deltas = std::move(deltas);
    result.coverage_delta_facets = std::move(delta_facets);
    result.coverage_delta_points = std::move(delta_points);

    result.counters.facet_resolution_count = facet_resolutions.size();
    result.counters.prior_root_coverage_count =
        prior_root_coverages.size();
    result.counters.prior_root_coverage_point_reference_count =
        prior_root_coverage_point_references.size();
    result.counters.latent_carrier_coverage_count =
        latent_carrier_coverages.size();
    result.counters.latent_carrier_coverage_point_reference_count =
        latent_carrier_coverage_point_references.size();
    result.counters.batch_direct_reference_scan_count =
        batch.direct_reference_count;
    result.counters.deferred_direct_birth_count = deferred_birth_count;
    result.counters.direct_saddle_hyperedge_count =
        result.action_plan.counters.direct_hyperedge_reference_count;
    result.counters.residual_hyperedge_count =
        result.action_plan.counters.residual_hyperedge_reference_count;
    result.counters.coface_facet_reference_scan_count =
        batch.coface_facet_reference_count;
    result.counters.source_point_reference_scan_count =
        result.required_source_point_reference_scan_capacity;
    result.counters.hyperedge_count = result.quotient_hyperedges.size();
    result.counters.token_reference_count =
        result.quotient_token_references.size();
    result.counters.distinct_typed_token_count =
        result.quotient.counters.distinct_token_count;
    result.counters.root_attachment_count =
        result.root_attachments.size();
    result.counters.group_count = result.action_plan.groups.size();
    result.counters.q_r_zero_group_count =
        result.quotient.counters.q_r_zero_group_count;
    result.counters.q_r_one_group_count =
        result.quotient.counters.q_r_one_group_count;
    result.counters.q_r_multiple_group_count =
        result.quotient.counters.q_r_multiple_group_count;
    result.counters.residual_incidence_record_count =
        result.residual_incidence_records.size();
    result.counters.equal_facet_binding_record_count =
        result.equal_facet_binding_records.size();
    result.counters.coverage_delta_record_count =
        result.coverage_deltas.size();
    result.counters.coverage_delta_facet_reference_count =
        result.coverage_delta_facets.size();
    result.counters.coverage_delta_point_reference_count =
        result.coverage_delta_points.size();
    result.counters.fully_redundant_coverage_delta_count =
        static_cast<std::size_t>(std::count_if(
            result.coverage_deltas.begin(),
            result.coverage_deltas.end(),
            [](const auto& delta) { return delta.fully_redundant; }));
    result.counters.residual_local_zero_point_delta_count =
        static_cast<std::size_t>(std::count_if(
            result.residual_incidence_records.begin(),
            result.residual_incidence_records.end(),
            [](const auto& record) {
              return record.local_zero_point_delta_certified;
            }));
    result.counters.residual_canonical_ownerless_delta_count =
        static_cast<std::size_t>(std::count_if(
            result.residual_incidence_records.begin(),
            result.residual_incidence_records.end(),
            [](const auto& record) {
              return record.canonically_owns_no_coverage_delta;
            }));
    result.counters.logical_scratch_entry_count =
        result.required_scratch_entry_capacity;
    result.counters.logical_output_entry_count =
        logical_output_count(result);

    result.residual_incidence_records_canonical_and_exhaustive =
        result.residual_incidence_records.size() ==
        batch.residual_reference_count;
    result.equal_facet_bindings_canonical_and_exhaustive =
        result.equal_facet_binding_records.size() ==
        static_cast<std::size_t>(std::count_if(
            facet_resolutions.begin(),
            facet_resolutions.end(),
            [](const auto& resolution) {
              return resolution.token.kind ==
                     ExactFrozenIncidenceTokenKind::equal_facet;
            }));
    result.coverage_deltas_are_exact_facet_and_point_set_differences =
        result.coverage_deltas.size() == result.action_plan.groups.size();
    result.flat_delta_arenas_have_canonical_group_owners =
        delta_arenas_are_canonical_partitions(result);
    result.all_output_within_budget =
        result.counters.logical_output_entry_count <=
            result.required_logical_output_entry_capacity &&
        facts_match_payload(result);
    result.no_partial_scientific_payload_published = true;
    result.decision = ExactDirectFrozenUnifiedIncidenceBatchDecision::
        complete_certified_frozen_unified_incidence_batch;
    if (audit != nullptr) {
      ++audit->structural_certification_count;
    }
    if (!result.certified_frozen_unified_incidence_batch()) {
      return fail(
          std::move(result),
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              no_batch_coverage_delta_scope_rejected);
    }
    return result;
  } catch (const std::bad_alloc&) {
    return fail(
        std::move(result),
        ExactDirectFrozenUnifiedIncidenceBatchDecision::
            no_batch_allocation_failed);
  } catch (const std::length_error&) {
    return fail(
        std::move(result),
        ExactDirectFrozenUnifiedIncidenceBatchDecision::
            no_batch_capacity_overflow);
  }
}

}  // namespace

ExactDirectFrozenUnifiedIncidenceBatchResult
build_exact_direct_frozen_unified_incidence_batch(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&
        source_star_budget,
    spatial::LbvhTraversalOrder source_star_traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& source_star,
    const ExactDirectSparseUnifiedLevelPlanBudget& source_plan_budget,
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    std::size_t batch_index,
    std::span<const ExactDirectFrozenUnifiedFacetResolution>
        facet_resolutions,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
        prior_root_coverages,
    std::span<const PointId> prior_root_coverage_point_references,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
        latent_carrier_coverages,
    std::span<const PointId> latent_carrier_coverage_point_references,
    const ExactDirectFrozenUnifiedIncidenceBatchBudget& budget) {
  ExactDirectFrozenUnifiedIncidenceBatchResult result =
      base_result(batch_index, budget);
  try {
    const auto source_verification =
        verify_exact_direct_sparse_unified_level_plan(
            index,
            cloud,
            source_facade,
            source_journal,
            source_arm_budget,
            source_arm_journal,
            source_incidence_budget,
            source_incidence_journal,
            source_star_budget,
            source_star_traversal_order,
            source_star,
            source_plan_budget,
            source_plan);
    if (!source_verification.result_certified) {
      return fail(
          std::move(result),
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              no_batch_source_plan_not_freshly_verified);
    }
    return
        build_exact_direct_frozen_unified_incidence_batch_from_verified_plan_authority(
            source_plan,
            VerifiedPlanAuthorityKind::
                freshly_verified_standalone_successive_star,
            nullptr,
            batch_index,
            facet_resolutions,
            prior_root_coverages,
            prior_root_coverage_point_references,
            latent_carrier_coverages,
            latent_carrier_coverage_point_references,
            budget);
  } catch (const std::bad_alloc&) {
    return fail(
        std::move(result),
        ExactDirectFrozenUnifiedIncidenceBatchDecision::
            no_batch_allocation_failed);
  } catch (const std::length_error&) {
    return fail(
        std::move(result),
        ExactDirectFrozenUnifiedIncidenceBatchDecision::
            no_batch_capacity_overflow);
  }
}

ExactDirectFrozenUnifiedIncidenceBatchVerification
verify_exact_direct_frozen_unified_incidence_batch(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&
        source_star_budget,
    spatial::LbvhTraversalOrder source_star_traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& source_star,
    const ExactDirectSparseUnifiedLevelPlanBudget& source_plan_budget,
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    std::size_t batch_index,
    std::span<const ExactDirectFrozenUnifiedFacetResolution>
        facet_resolutions,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
        prior_root_coverages,
    std::span<const PointId> prior_root_coverage_point_references,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
        latent_carrier_coverages,
    std::span<const PointId> latent_carrier_coverage_point_references,
    const ExactDirectFrozenUnifiedIncidenceBatchBudget& trusted_budget,
    const ExactDirectFrozenUnifiedIncidenceBatchResult& observed) {
  ExactDirectFrozenUnifiedIncidenceBatchVerification verification;
  verification.requested_budget_certified =
      observed.requested_budget == trusted_budget;
  const auto source_verification =
      verify_exact_direct_sparse_unified_level_plan(
          index,
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal,
          source_incidence_budget,
          source_incidence_journal,
          source_star_budget,
          source_star_traversal_order,
          source_star,
          source_plan_budget,
          source_plan);
  verification.source_plan_freshly_verified =
      source_verification.result_certified;
  ExactDirectFrozenUnifiedIncidenceBatchResult expected;
  if (source_verification.result_certified) {
    expected =
        build_exact_direct_frozen_unified_incidence_batch_from_verified_plan_authority(
            source_plan,
            VerifiedPlanAuthorityKind::
                freshly_verified_standalone_successive_star,
            nullptr,
            batch_index,
            facet_resolutions,
            prior_root_coverages,
            prior_root_coverage_point_references,
            latent_carrier_coverages,
            latent_carrier_coverage_point_references,
            trusted_budget);
  } else {
    expected = fail(
        base_result(batch_index, trusted_budget),
        ExactDirectFrozenUnifiedIncidenceBatchDecision::
            no_batch_source_plan_not_freshly_verified);
  }
  verification.expected_result_freshly_reconstructed =
      expected.certified_frozen_unified_incidence_batch();
  verification.supplied_latent_carrier_coverage_freshly_replayed =
      verification.expected_result_freshly_reconstructed &&
      expected.latent_carrier_coverage_csr_canonical_and_exhaustive &&
      expected.latent_facets_covered_by_their_carriers &&
      expected.counters.latent_carrier_coverage_count ==
          latent_carrier_coverages.size() &&
      expected.counters.latent_carrier_coverage_point_reference_count ==
          latent_carrier_coverage_point_references.size();
  if (observed.quotient.requested_budget ==
      quotient_budget_from(trusted_budget)) {
    const auto quotient_verification =
        verify_exact_direct_frozen_incidence_quotient_streaming(
            observed.quotient_hyperedges,
            observed.quotient_token_references,
            observed.quotient.requested_budget,
            observed.quotient);
    verification.quotient_freshly_streaming_verified =
        quotient_verification.result_certified;
  }
  if (verification.quotient_freshly_streaming_verified &&
      observed.action_plan.requested_budget ==
          action_budget_from(trusted_budget)) {
    const auto action_verification =
        verify_exact_direct_frozen_incidence_hgp_action_plan_streaming(
            observed.quotient_hyperedges,
            observed.quotient_token_references,
            observed.quotient.requested_budget,
            observed.quotient,
            observed.provenance,
            observed.root_attachments,
            observed.action_plan.requested_budget,
            observed.action_plan);
    verification.action_plan_freshly_streaming_verified =
        action_verification.result_certified;
  }
  verification.observed_recursively_equal = observed == expected;
  verification.result_facts_and_scope_certified =
      observed.certified_frozen_unified_incidence_batch();
  verification.no_forbidden_global_structure_or_mutation =
      !observed.reducer_locator_forest_or_caller_state_mutated &&
      !observed.global_facet_coface_or_gamma_catalog_materialized &&
      !observed.supplied_star_global_completeness_claimed &&
      !observed.public_status_claimed;
  verification.fresh_replay_certified =
      verification.source_plan_freshly_verified &&
      verification.expected_result_freshly_reconstructed &&
      verification.supplied_latent_carrier_coverage_freshly_replayed &&
      verification.quotient_freshly_streaming_verified &&
      verification.action_plan_freshly_streaming_verified &&
      verification.observed_recursively_equal;
  verification.result_certified =
      verification.requested_budget_certified &&
      verification.result_facts_and_scope_certified &&
      verification.no_forbidden_global_structure_or_mutation &&
      verification.fresh_replay_certified;
  return verification;
}

namespace internal {

ExactDirectFrozenUnifiedImmutablePlanAuthorityInitialization
ExactDirectFrozenUnifiedImmutablePlanAuthorityFactory::create(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&
        source_star_budget,
    spatial::LbvhTraversalOrder source_star_traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& source_star,
    const ExactDirectSparseUnifiedLevelPlanBudget& source_plan_budget,
    const ExactDirectSparseUnifiedLevelPlanResult& immutable_plan) {
  ExactDirectFrozenUnifiedImmutablePlanAuthorityInitialization output;
  output.source_plan_verification_count = 1U;
  const auto verification = verify_exact_direct_sparse_unified_level_plan(
      index,
      cloud,
      source_facade,
      source_journal,
      source_arm_budget,
      source_arm_journal,
      source_incidence_budget,
      source_incidence_journal,
      source_star_budget,
      source_star_traversal_order,
      source_star,
      source_plan_budget,
      immutable_plan);
  if (!verification.result_certified) {
    return output;
  }
  output.authority =
      ExactDirectFrozenUnifiedImmutablePlanAuthority{
          &immutable_plan,
          ExactDirectFrozenUnifiedImmutablePlanAuthorityKind::
              successive_incidence_star};
  output.source_plan_freshly_verified_once = true;
  return output;
}

std::optional<ExactDirectFrozenUnifiedResidentBatchAttestation>
ExactDirectFrozenUnifiedResidentBatchAttestedBuilder::build_once(
    const ExactDirectFrozenUnifiedImmutablePlanAuthority& authority,
    std::size_t batch_index,
    std::span<const ExactDirectFrozenUnifiedFacetResolution>
        facet_resolutions,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
        prior_root_coverages,
    std::span<const spatial::PointId> prior_root_coverage_point_references,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
        latent_carrier_coverages,
    std::span<const spatial::PointId> latent_carrier_coverage_point_references,
    const ExactDirectFrozenUnifiedIncidenceBatchBudget& budget,
    ExactDirectFrozenUnifiedIncidenceBatchResult& destination) {
  if (!authority.valid()) {
    destination = fail(
        base_result(
            batch_index,
            budget,
            authority.kind() ==
                    ExactDirectFrozenUnifiedImmutablePlanAuthorityKind::
                        normalized_direct_h0_candidate_source
                ? VerifiedPlanAuthorityKind::
                      immutable_verified_resident_normalized_direct_source
                : VerifiedPlanAuthorityKind::
                      immutable_verified_resident_successive_star),
        ExactDirectFrozenUnifiedIncidenceBatchDecision::
            no_batch_source_plan_not_freshly_verified);
    return std::nullopt;
  }
  VerifiedPlanBuildAudit audit;
  destination =
      build_exact_direct_frozen_unified_incidence_batch_from_verified_plan_authority(
          authority.plan(),
          authority.kind() ==
                  ExactDirectFrozenUnifiedImmutablePlanAuthorityKind::
                      normalized_direct_h0_candidate_source
              ? VerifiedPlanAuthorityKind::
                    immutable_verified_resident_normalized_direct_source
              : VerifiedPlanAuthorityKind::
                    immutable_verified_resident_successive_star,
          &audit,
          batch_index,
          facet_resolutions,
          prior_root_coverages,
          prior_root_coverage_point_references,
          latent_carrier_coverages,
          latent_carrier_coverage_point_references,
          budget);
  if (destination.decision !=
          ExactDirectFrozenUnifiedIncidenceBatchDecision::
              complete_certified_frozen_unified_incidence_batch ||
      destination.source_plan_freshly_verified ||
      !destination
           .source_plan_verified_once_by_immutable_resident_authority ||
      !destination.frozen_quotient_freshly_streaming_verified ||
      !destination.frozen_hgp_action_plan_freshly_streaming_verified ||
      audit.batch_construction_count != 1U ||
      audit.quotient_streaming_verification_count != 1U ||
      audit.action_plan_streaming_verification_count != 1U ||
      audit.structural_certification_count != 1U) {
    return std::nullopt;
  }
  return ExactDirectFrozenUnifiedResidentBatchAttestation{&destination};
}

}  // namespace internal

}  // namespace morsehgp3d::hierarchy
