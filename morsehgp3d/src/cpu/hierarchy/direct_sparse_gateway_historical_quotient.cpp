#include "morsehgp3d/hierarchy/direct_sparse_gateway_historical_quotient.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::size_t absent_index =
    std::numeric_limits<std::size_t>::max();

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

[[nodiscard]] bool accumulate(
    std::size_t increment,
    std::size_t& value) noexcept {
  return checked_add(value, increment, value);
}

enum class TypedVertexKind : std::uint8_t {
  historical_root,
  latent_temporal_resolution,
};

struct IncidenceScratch {
  std::size_t source_batch_index{};
  std::size_t gateway_candidate_index{};
  std::size_t historical_component_index{absent_index};
  TypedVertexKind kind{TypedVertexKind::historical_root};
  std::size_t identity{};
};

struct CandidateScratch {
  std::size_t source_batch_index{absent_index};
  std::size_t strict_pre_locator_prefix_count{absent_index};
  std::size_t deletion_projection_offset{absent_index};
  std::size_t deletion_projection_count{};
  std::size_t historical_positive_projection_count{};
  std::size_t historical_latent_projection_count{};
};

struct BatchScratch {
  std::size_t strict_pre_locator_prefix_count{absent_index};
  std::size_t candidate_count{};
};

class CandidateDisjointSet {
 public:
  enum class UnionResult : std::uint8_t {
    budget_exhausted,
    already_joined,
    joined,
  };

  CandidateDisjointSet(
      std::size_t count,
      std::size_t maximum_parent_hop_count)
      : parent_(count),
        maximum_parent_hop_count_(maximum_parent_hop_count) {
    std::iota(parent_.begin(), parent_.end(), std::size_t{0U});
  }

  [[nodiscard]] bool find(
      std::size_t value,
      std::size_t& root) noexcept {
    root = value;
    while (parent_[root] != root) {
      if (!consume_parent_hop()) {
        return false;
      }
      root = parent_[root];
    }
    while (parent_[value] != value) {
      if (!consume_parent_hop()) {
        return false;
      }
      const std::size_t next = parent_[value];
      parent_[value] = root;
      value = next;
    }
    return true;
  }

  [[nodiscard]] UnionResult unite(
      std::size_t left,
      std::size_t right) noexcept {
    std::size_t left_root = 0U;
    std::size_t right_root = 0U;
    if (!find(left, left_root) || !find(right, right_root)) {
      return UnionResult::budget_exhausted;
    }
    left = left_root;
    right = right_root;
    if (left == right) {
      return UnionResult::already_joined;
    }
    parent_[std::max(left, right)] = std::min(left, right);
    return UnionResult::joined;
  }

  [[nodiscard]] std::size_t parent_hop_count() const noexcept {
    return parent_hop_count_;
  }

 private:
  [[nodiscard]] bool consume_parent_hop() noexcept {
    if (parent_hop_count_ == maximum_parent_hop_count_) {
      return false;
    }
    ++parent_hop_count_;
    return true;
  }

  std::vector<std::size_t> parent_;
  std::size_t maximum_parent_hop_count_{};
  std::size_t parent_hop_count_{};
};

class ComparisonBudget {
 public:
  explicit ComparisonBudget(std::size_t maximum) : maximum_(maximum) {}

  template <class Value, class Less>
  [[nodiscard]] bool compare(
      const Value& left,
      const Value& right,
      const Less& less,
      bool& result) noexcept {
    if (count_ == maximum_) {
      return false;
    }
    ++count_;
    result = less(left, right);
    return true;
  }

  [[nodiscard]] std::size_t count() const noexcept { return count_; }

 private:
  std::size_t maximum_{};
  std::size_t count_{};
};

template <class Value, class Less>
[[nodiscard]] bool bounded_sift_down(
    std::vector<Value>& values,
    std::size_t root,
    std::size_t end,
    const Less& less,
    ComparisonBudget& comparisons) noexcept {
  while (root < end / 2U) {
    std::size_t child = root * 2U + 1U;
    if (child + 1U < end) {
      bool left_is_less = false;
      if (!comparisons.compare(
              values[child],
              values[child + 1U],
              less,
              left_is_less)) {
        return false;
      }
      if (left_is_less) {
        ++child;
      }
    }
    bool root_is_less = false;
    if (!comparisons.compare(
            values[root], values[child], less, root_is_less)) {
      return false;
    }
    if (!root_is_less) {
      return true;
    }
    std::swap(values[root], values[child]);
    root = child;
  }
  return true;
}

template <class Value, class Less>
[[nodiscard]] bool bounded_heapsort(
    std::vector<Value>& values,
    const Less& less,
    ComparisonBudget& comparisons) noexcept {
  for (std::size_t start = values.size() / 2U; start > 0U; --start) {
    if (!bounded_sift_down(
            values, start - 1U, values.size(), less, comparisons)) {
      return false;
    }
  }
  for (std::size_t end = values.size(); end > 1U; --end) {
    std::swap(values[0U], values[end - 1U]);
    if (!bounded_sift_down(
            values, 0U, end - 1U, less, comparisons)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool incidence_vertex_less(
    const IncidenceScratch& left,
    const IncidenceScratch& right) noexcept {
  if (left.source_batch_index != right.source_batch_index) {
    return left.source_batch_index < right.source_batch_index;
  }
  if (left.kind != right.kind) {
    return left.kind < right.kind;
  }
  if (left.identity != right.identity) {
    return left.identity < right.identity;
  }
  return left.gateway_candidate_index < right.gateway_candidate_index;
}

[[nodiscard]] bool same_typed_vertex(
    const IncidenceScratch& left,
    const IncidenceScratch& right) noexcept {
  return left.source_batch_index == right.source_batch_index &&
         left.kind == right.kind && left.identity == right.identity;
}

[[nodiscard]] bool incidence_component_less(
    const IncidenceScratch& left,
    const IncidenceScratch& right) noexcept {
  if (left.historical_component_index !=
      right.historical_component_index) {
    return left.historical_component_index <
           right.historical_component_index;
  }
  if (left.kind != right.kind) {
    return left.kind < right.kind;
  }
  if (left.identity != right.identity) {
    return left.identity < right.identity;
  }
  return left.gateway_candidate_index < right.gateway_candidate_index;
}

[[nodiscard]] bool same_projected_vertex(
    const IncidenceScratch& left,
    const IncidenceScratch& right) noexcept {
  return left.historical_component_index ==
             right.historical_component_index &&
         left.kind == right.kind && left.identity == right.identity;
}

void initialize_scope(
    ExactDirectSparseGatewayHistoricalQuotientResult& result) noexcept {
  result.scope = ExactDirectSparseGatewayHistoricalQuotientScope::
      supplied_gateway_candidates_batch_local_typed_incidence_known_root_projection_only;
  result.no_partial_scientific_payload_published = true;
  result.nested_verifiers_replayed = false;
  result.external_clock_authority_replayed = false;
  result.external_binding_authority_replayed = false;
  result.conditional_on_caller_external_binding_authority_replay = true;
  result.conditional_on_caller_strict_pre_lot_orchestration = true;
  result.external_freeze_synchronization_replayed = false;
  result.conditional_on_caller_external_freeze_synchronization = true;
  result.in_memory_replay_only = true;
  result.crash_durable = false;
  result.global_gateway_completeness_claimed = false;
  result.latent_facet_means_isolated = false;
  result.root_birth_continuation_or_multifusion_claimed = false;
  result.locator_root_union_or_forest_mutated = false;
  result.gateway_attach_published = false;
  result.gamma_cells_or_higher_order_delaunay_materialized = false;
  result.forbidden_global_structure_materialized = false;
  result.public_status_claimed = false;
  result.partial_refinement_only = true;
}

[[nodiscard]] bool non_scope_is_honest(
    const ExactDirectSparseGatewayHistoricalQuotientResult& result)
    noexcept {
  return !result.nested_verifiers_replayed &&
         !result.external_clock_authority_replayed &&
         !result.external_binding_authority_replayed &&
         result.conditional_on_caller_external_binding_authority_replay &&
         result.conditional_on_caller_strict_pre_lot_orchestration &&
         !result.external_freeze_synchronization_replayed &&
         result.conditional_on_caller_external_freeze_synchronization &&
         result.in_memory_replay_only && !result.crash_durable &&
         !result.global_gateway_completeness_claimed &&
         !result.latent_facet_means_isolated &&
         !result.root_birth_continuation_or_multifusion_claimed &&
         !result.locator_root_union_or_forest_mutated &&
         !result.gateway_attach_published &&
         !result.gamma_cells_or_higher_order_delaunay_materialized &&
         !result.forbidden_global_structure_materialized &&
         !result.public_status_claimed && result.partial_refinement_only;
}

[[nodiscard]] ExactDirectSparseGatewayHistoricalQuotientResult fail(
    ExactDirectSparseGatewayHistoricalQuotientResult result,
    ExactDirectSparseGatewayHistoricalQuotientDecision decision) noexcept {
  result.batches.clear();
  result.candidate_to_component.clear();
  result.components.clear();
  result.component_root_ids.clear();
  result.component_latent_resolution_indices.clear();
  result.logical_output_entry_count = 0U;
  result.no_partial_scientific_payload_published = true;
  result.decision = decision;
  return result;
}

[[nodiscard]] bool requirements_fit_budget(
    const ExactDirectSparseGatewayHistoricalQuotientResult& result) noexcept {
  const auto& budget = result.requested_budget;
  return result.required_batch_count <= budget.maximum_batch_count &&
         result.required_candidate_count <= budget.maximum_candidate_count &&
         result.required_projection_count <=
             budget.maximum_projection_count &&
         result.required_temporal_resolution_count <=
             budget.maximum_temporal_resolution_count &&
         result.required_incidence_scan_count <=
             budget.maximum_incidence_scan_count &&
         result.required_sort_scratch_entry_count <=
             budget.maximum_sort_scratch_entry_count &&
         result.required_candidate_dsu_entry_count <=
             budget.maximum_candidate_dsu_entry_count &&
         result.required_component_output_capacity <=
             budget.maximum_component_output_count &&
         result.required_root_output_capacity <=
             budget.maximum_root_output_count &&
         result.required_latent_output_capacity <=
             budget.maximum_latent_output_count &&
         result.required_logical_output_capacity <=
             budget.maximum_logical_output_entry_count &&
         result.required_temporary_scratch_byte_count <=
             budget.maximum_temporary_scratch_byte_count;
}

[[nodiscard]] bool add_scratch_bytes(
    std::size_t count,
    std::size_t element_size,
    std::size_t& total) noexcept {
  std::size_t bytes = 0U;
  return checked_multiply(count, element_size, bytes) &&
         checked_add(total, bytes, total);
}

[[nodiscard]] bool complete_payload_well_formed(
    const ExactDirectSparseGatewayHistoricalQuotientResult& result)
    noexcept {
  if (result.batches.size() != result.required_batch_count ||
      result.candidate_to_component.size() !=
          result.required_candidate_count ||
      result.components.size() != result.counters.component_count ||
      result.component_root_ids.size() !=
          result.counters.distinct_root_vertex_count ||
      result.component_latent_resolution_indices.size() !=
          result.counters.distinct_latent_vertex_count) {
    return false;
  }

  std::size_t expected_projection_offset = 0U;
  for (std::size_t index = 0U;
       index < result.candidate_to_component.size();
       ++index) {
    const auto& candidate = result.candidate_to_component[index];
    if (candidate.gateway_candidate_index != index ||
        candidate.source_batch_index >= result.batches.size() ||
        candidate.historical_component_index >= result.components.size() ||
        result.components[candidate.historical_component_index]
                .source_batch_index != candidate.source_batch_index ||
        candidate.deletion_projection_offset != expected_projection_offset ||
        candidate.deletion_projection_count == 0U ||
        candidate.historical_positive_projection_count >
            candidate.deletion_projection_count ||
        candidate.historical_latent_projection_count !=
            candidate.deletion_projection_count -
                candidate.historical_positive_projection_count ||
        !checked_add(
            expected_projection_offset,
            candidate.deletion_projection_count,
            expected_projection_offset)) {
      return false;
    }
  }
  if (expected_projection_offset != result.required_projection_count) {
    return false;
  }

  std::size_t expected_root_offset = 0U;
  std::size_t expected_latent_offset = 0U;
  std::size_t rooted_count = 0U;
  std::size_t latent_only_count = 0U;
  std::size_t one_root_count = 0U;
  std::size_t multiple_root_count = 0U;
  std::size_t declared_component_candidate_count = 0U;
  for (std::size_t index = 0U; index < result.components.size(); ++index) {
    const auto& component = result.components[index];
    if (component.historical_component_index != index ||
        component.source_batch_index >= result.batches.size() ||
        component.representative_gateway_candidate_index >=
            result.candidate_to_component.size() ||
        result.candidate_to_component
                [component.representative_gateway_candidate_index]
                    .historical_component_index != index ||
        component.candidate_count == 0U ||
        component.root_offset != expected_root_offset ||
        component.latent_resolution_offset != expected_latent_offset ||
        component.root_count >
            result.component_root_ids.size() - expected_root_offset ||
        component.latent_resolution_count >
            result.component_latent_resolution_indices.size() -
                expected_latent_offset ||
        (index != 0U &&
         !(result.components[index - 1U].source_batch_index <
               component.source_batch_index ||
           (result.components[index - 1U].source_batch_index ==
                component.source_batch_index &&
            result.components[index - 1U]
                    .representative_gateway_candidate_index <
                component.representative_gateway_candidate_index))) ||
        !accumulate(
            component.candidate_count,
            declared_component_candidate_count)) {
      return false;
    }
    for (std::size_t local = 1U; local < component.root_count; ++local) {
      if (!(result.component_root_ids[component.root_offset + local - 1U] <
            result.component_root_ids[component.root_offset + local])) {
        return false;
      }
    }
    for (std::size_t local = 1U;
         local < component.latent_resolution_count;
         ++local) {
      if (!(result.component_latent_resolution_indices
                [component.latent_resolution_offset + local - 1U] <
            result.component_latent_resolution_indices
                [component.latent_resolution_offset + local])) {
        return false;
      }
    }
    if (component.root_count == 0U) {
      if (component.latent_resolution_count == 0U ||
          component.disposition !=
              ExactDirectSparseGatewayHistoricalQuotientDisposition::
                  latent_only_unresolved ||
          !accumulate(1U, latent_only_count)) {
        return false;
      }
    } else {
      if (!accumulate(1U, rooted_count)) {
        return false;
      }
      if (component.root_count == 1U) {
        if (component.disposition !=
                ExactDirectSparseGatewayHistoricalQuotientDisposition::
                    one_known_root_class ||
            !accumulate(1U, one_root_count)) {
          return false;
        }
      } else if (
          component.disposition !=
              ExactDirectSparseGatewayHistoricalQuotientDisposition::
                  multiple_known_root_class ||
          !accumulate(1U, multiple_root_count)) {
        return false;
      }
    }
    if (!checked_add(
            expected_root_offset,
            component.root_count,
            expected_root_offset) ||
        !checked_add(
            expected_latent_offset,
            component.latent_resolution_count,
            expected_latent_offset)) {
      return false;
    }
  }
  if (expected_root_offset != result.component_root_ids.size() ||
      expected_latent_offset !=
          result.component_latent_resolution_indices.size()) {
    return false;
  }

  std::size_t expected_component_offset = 0U;
  std::size_t batch_rooted = 0U;
  std::size_t batch_latent_only = 0U;
  std::size_t declared_batch_candidate_count = 0U;
  for (std::size_t batch_index = 0U;
       batch_index < result.batches.size();
       ++batch_index) {
    const auto& batch = result.batches[batch_index];
    if (batch.batch_index != batch_index ||
        batch.candidate_count == 0U ||
        batch.component_offset != expected_component_offset ||
        batch.component_count >
            result.components.size() - expected_component_offset ||
        batch.rooted_component_count >
            batch.component_count ||
        batch.latent_only_component_count !=
            batch.component_count - batch.rooted_component_count ||
        !accumulate(
            batch.candidate_count,
            declared_batch_candidate_count)) {
      return false;
    }
    for (std::size_t local = 0U; local < batch.component_count; ++local) {
      const auto& component =
          result.components[batch.component_offset + local];
      if (component.source_batch_index != batch_index ||
          component.strict_pre_locator_prefix_count !=
              batch.strict_pre_locator_prefix_count) {
        return false;
      }
    }
    if (!checked_add(
            expected_component_offset,
            batch.component_count,
            expected_component_offset) ||
        !accumulate(batch.rooted_component_count, batch_rooted) ||
        !accumulate(
            batch.latent_only_component_count, batch_latent_only)) {
      return false;
    }
  }

  return expected_component_offset == result.components.size() &&
         declared_component_candidate_count ==
             result.candidate_to_component.size() &&
         declared_batch_candidate_count ==
             result.candidate_to_component.size() &&
         rooted_count == result.counters.rooted_component_count &&
         latent_only_count ==
             result.counters.latent_only_component_count &&
         one_root_count ==
             result.counters.one_known_root_component_count &&
         multiple_root_count ==
             result.counters.multiple_known_root_component_count &&
         batch_rooted == rooted_count &&
         batch_latent_only == latent_only_count;
}

[[nodiscard]] bool observed_storage_within_budget(
    const ExactDirectSparseGatewayHistoricalQuotientResult& observed,
    const ExactDirectSparseGatewayHistoricalQuotientBudget& budget)
    noexcept {
  std::size_t logical = 0U;
  return observed.batches.size() <= budget.maximum_batch_count &&
         observed.candidate_to_component.size() <=
             budget.maximum_candidate_count &&
         observed.components.size() <=
             budget.maximum_component_output_count &&
         observed.component_root_ids.size() <=
             budget.maximum_root_output_count &&
         observed.component_latent_resolution_indices.size() <=
             budget.maximum_latent_output_count &&
         checked_add(
             observed.batches.size(),
             observed.candidate_to_component.size(),
             logical) &&
         checked_add(logical, observed.components.size(), logical) &&
         checked_add(
             logical, observed.component_root_ids.size(), logical) &&
         checked_add(
             logical,
             observed.component_latent_resolution_indices.size(),
             logical) &&
         logical <= budget.maximum_logical_output_entry_count;
}

}  // namespace

bool ExactDirectSparseGatewayHistoricalQuotientResult::
    certified_partial_refinement() const noexcept {
  std::size_t expected_logical = 0U;
  return schema_version ==
             direct_sparse_gateway_historical_quotient_schema_version &&
         decision ==
             ExactDirectSparseGatewayHistoricalQuotientDecision::
                 complete_certified_batch_local_historical_root_quotient_proposal &&
         source_and_temporal_resolution_prevalidated &&
         budget_preflight_certified &&
         every_candidate_partitioned_by_its_deletion_projections &&
         every_projection_mapped_to_one_typed_vertex &&
         typed_root_and_latent_namespaces_disjoint &&
         closure_strictly_batch_local &&
         shared_latent_facets_join_candidate_hyperedges &&
         candidate_hyperedge_duplicates_are_idempotent &&
         components_canonical_and_partition_candidates &&
         known_root_projection_exact_for_supplied_candidates &&
         latent_only_components_preserved_as_unresolved &&
         roots_and_latent_resolution_slices_canonical &&
         no_partial_scientific_payload_published &&
         non_scope_is_honest(*this) &&
         complete_payload_well_formed(*this) &&
         counters.batch_scan_count == required_batch_count &&
         counters.candidate_scan_count == required_candidate_count &&
         counters.projection_scan_count == required_projection_count &&
         counters.incidence_scan_count == required_incidence_scan_count &&
         counters.sort_comparison_count <=
             requested_budget.maximum_sort_comparison_count &&
         counters.candidate_dsu_parent_hop_count <=
             requested_budget.maximum_candidate_dsu_parent_hop_count &&
         counters.component_count <= required_component_output_capacity &&
         counters.distinct_root_vertex_count <=
             required_root_output_capacity &&
         counters.distinct_latent_vertex_count <=
             required_latent_output_capacity &&
         requirements_fit_budget(*this) &&
         checked_add(batches.size(), candidate_to_component.size(), expected_logical) &&
         checked_add(expected_logical, components.size(), expected_logical) &&
         checked_add(
             expected_logical, component_root_ids.size(), expected_logical) &&
         checked_add(
             expected_logical,
             component_latent_resolution_indices.size(),
             expected_logical) &&
         expected_logical == logical_output_entry_count &&
         logical_output_entry_count <= required_logical_output_capacity;
}

bool ExactDirectSparseGatewayHistoricalQuotientResult::
    certified_atomic_failure() const noexcept {
  const bool recognized =
      decision ==
          ExactDirectSparseGatewayHistoricalQuotientDecision::
              no_historical_quotient_capacity_overflow ||
      decision ==
          ExactDirectSparseGatewayHistoricalQuotientDecision::
              no_historical_quotient_budget_exhausted ||
      decision ==
          ExactDirectSparseGatewayHistoricalQuotientDecision::
              no_historical_quotient_allocation_failed ||
      decision ==
          ExactDirectSparseGatewayHistoricalQuotientDecision::
              no_historical_quotient_source_rejected;
  return schema_version ==
             direct_sparse_gateway_historical_quotient_schema_version &&
         recognized && batches.empty() && candidate_to_component.empty() &&
         components.empty() && component_root_ids.empty() &&
         component_latent_resolution_indices.empty() &&
         logical_output_entry_count == 0U &&
         no_partial_scientific_payload_published &&
         non_scope_is_honest(*this);
}

bool ExactDirectSparseGatewayHistoricalQuotientResult::certified_outcome()
    const noexcept {
  return certified_partial_refinement() || certified_atomic_failure();
}

ExactDirectSparseGatewayHistoricalQuotientResult
build_exact_direct_sparse_gateway_historical_quotient(
    const ExactDirectSparseGatewayCandidateJournalResult& source,
    const ExactDirectSparseGatewayCandidateLocalizationResult& localization,
    const ExactDirectSparseGatewayTemporalResolutionResult&
        temporal_resolution,
    const ExactDirectSparseGatewayHistoricalQuotientBudget& budget) {
  ExactDirectSparseGatewayHistoricalQuotientResult result;
  result.requested_budget = budget;
  result.locator_snapshot_stamp =
      temporal_resolution.locator_snapshot_stamp;
  result.required_batch_count = source.batches.size();
  result.required_candidate_count = source.gateway_candidates.size();
  result.required_projection_count =
      localization.deletion_projections.size();
  result.required_temporal_resolution_count =
      temporal_resolution.temporal_resolutions.size();
  result.required_sort_scratch_entry_count =
      std::max(
          result.required_projection_count,
          result.required_candidate_count);
  result.required_candidate_dsu_entry_count =
      result.required_candidate_count;
  result.required_component_output_capacity =
      result.required_candidate_count;
  result.required_root_output_capacity = result.required_projection_count;
  result.required_latent_output_capacity =
      result.required_projection_count;
  initialize_scope(result);

  std::size_t three_projection_count = 0U;
  if (!checked_multiply(
          result.required_projection_count,
          3U,
          three_projection_count)) {
    return fail(
        std::move(result),
        ExactDirectSparseGatewayHistoricalQuotientDecision::
            no_historical_quotient_capacity_overflow);
  }
  result.required_incidence_scan_count = three_projection_count;

  std::size_t triple_candidate_count = 0U;
  std::size_t double_projection_count = 0U;
  if (!checked_multiply(
          result.required_candidate_count,
          3U,
          triple_candidate_count) ||
      !checked_multiply(
          result.required_projection_count,
          2U,
          double_projection_count) ||
      !checked_add(
          result.required_batch_count,
          result.required_candidate_count,
          result.required_logical_output_capacity) ||
      !checked_add(
          result.required_logical_output_capacity,
          result.required_candidate_count,
          result.required_logical_output_capacity) ||
      !checked_add(
          result.required_logical_output_capacity,
          double_projection_count,
          result.required_logical_output_capacity)) {
    return fail(
        std::move(result),
        ExactDirectSparseGatewayHistoricalQuotientDecision::
            no_historical_quotient_capacity_overflow);
  }

  std::size_t scratch_bytes = 0U;
  if (!add_scratch_bytes(
          result.required_projection_count,
          sizeof(IncidenceScratch),
          scratch_bytes) ||
      !add_scratch_bytes(
          result.required_candidate_count,
          sizeof(CandidateScratch),
          scratch_bytes) ||
      !add_scratch_bytes(
          result.required_batch_count,
          sizeof(BatchScratch),
          scratch_bytes) ||
      !add_scratch_bytes(
          triple_candidate_count,
          sizeof(std::size_t),
          scratch_bytes)) {
    return fail(
        std::move(result),
        ExactDirectSparseGatewayHistoricalQuotientDecision::
            no_historical_quotient_capacity_overflow);
  }
  result.required_temporary_scratch_byte_count = scratch_bytes;

  std::size_t eleven_candidate_count = 0U;
  if (!checked_multiply(
          result.required_candidate_count,
          11U,
          eleven_candidate_count)) {
    return fail(
        std::move(result),
        ExactDirectSparseGatewayHistoricalQuotientDecision::
            no_historical_quotient_capacity_overflow);
  }
  if (result.required_projection_count > eleven_candidate_count ||
      (result.required_candidate_count == 0U) !=
          (result.required_projection_count == 0U)) {
    return fail(
        std::move(result),
        ExactDirectSparseGatewayHistoricalQuotientDecision::
            no_historical_quotient_source_rejected);
  }
  if (!requirements_fit_budget(result)) {
    return fail(
        std::move(result),
        ExactDirectSparseGatewayHistoricalQuotientDecision::
            no_historical_quotient_budget_exhausted);
  }
  result.budget_preflight_certified = true;

  if (!source.certified_partial_refinement() ||
      !localization.certified_partial_refinement() ||
      !temporal_resolution.certified_partial_refinement() ||
      localization.source_gateway_candidate_count !=
          source.gateway_candidates.size() ||
      localization.deletion_projections.size() !=
          temporal_resolution.projection_to_resolution.size() ||
      temporal_resolution.required_source_batch_count !=
          source.batches.size() ||
      temporal_resolution.required_projection_count !=
          localization.deletion_projections.size() ||
      temporal_resolution.required_localized_token_count !=
          localization.localized_facet_tokens.size() ||
      localization.locator_snapshot_stamp !=
          temporal_resolution.locator_snapshot_stamp) {
    return fail(
        std::move(result),
        ExactDirectSparseGatewayHistoricalQuotientDecision::
            no_historical_quotient_source_rejected);
  }
  result.source_and_temporal_resolution_prevalidated = true;

  try {
    std::vector<CandidateScratch> candidate_scratch(
        result.required_candidate_count);
    std::vector<BatchScratch> batch_scratch(result.required_batch_count);
    std::vector<IncidenceScratch> incidences;
    incidences.reserve(result.required_projection_count);

    for (std::size_t projection_index = 0U;
         projection_index < localization.deletion_projections.size();
         ++projection_index) {
      const auto& projection =
          localization.deletion_projections[projection_index];
      const auto& temporal_reference =
          temporal_resolution.projection_to_resolution[projection_index];
      if (projection.deletion_projection_index != projection_index ||
          temporal_reference.projection_index != projection_index ||
          projection.gateway_candidate_index >=
              source.gateway_candidates.size() ||
          projection.source_batch_index >= source.batches.size() ||
          projection.localized_facet_token_index >=
              localization.localized_facet_tokens.size() ||
          temporal_reference.temporal_resolution_index >=
              temporal_resolution.temporal_resolutions.size()) {
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_source_rejected);
      }
      const auto& candidate =
          source.gateway_candidates[projection.gateway_candidate_index];
      if (candidate.gateway_candidate_index !=
              projection.gateway_candidate_index ||
          candidate.facet_token_index >= source.facet_tokens.size() ||
          source.facet_tokens[candidate.facet_token_index].batch_index !=
              projection.source_batch_index) {
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_source_rejected);
      }
      const auto& resolution =
          temporal_resolution.temporal_resolutions
              [temporal_reference.temporal_resolution_index];
      if (resolution.temporal_resolution_index !=
              temporal_reference.temporal_resolution_index ||
          resolution.localized_facet_token_index !=
              projection.localized_facet_token_index) {
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_source_rejected);
      }

      auto& candidate_facts =
          candidate_scratch[projection.gateway_candidate_index];
      if (candidate_facts.deletion_projection_count == 0U) {
        candidate_facts.source_batch_index = projection.source_batch_index;
        candidate_facts.strict_pre_locator_prefix_count =
            resolution.strict_pre_locator_prefix_count;
        candidate_facts.deletion_projection_offset = projection_index;
      } else if (
          candidate_facts.source_batch_index !=
              projection.source_batch_index ||
          candidate_facts.strict_pre_locator_prefix_count !=
              resolution.strict_pre_locator_prefix_count ||
          candidate_facts.deletion_projection_offset +
                  candidate_facts.deletion_projection_count !=
              projection_index) {
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_source_rejected);
      }
      auto& batch_facts = batch_scratch[projection.source_batch_index];
      if (batch_facts.strict_pre_locator_prefix_count == absent_index) {
        batch_facts.strict_pre_locator_prefix_count =
            resolution.strict_pre_locator_prefix_count;
      } else if (
          batch_facts.strict_pre_locator_prefix_count !=
          resolution.strict_pre_locator_prefix_count) {
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_source_rejected);
      }

      IncidenceScratch incidence;
      incidence.source_batch_index = projection.source_batch_index;
      incidence.gateway_candidate_index =
          projection.gateway_candidate_index;
      switch (resolution.disposition) {
        case ExactDirectSparseGatewayTemporalDisposition::relative_positive:
          if (!resolution.positive_payload_present) {
            return fail(
                std::move(result),
                ExactDirectSparseGatewayHistoricalQuotientDecision::
                    no_historical_quotient_source_rejected);
          }
          incidence.kind = TypedVertexKind::historical_root;
          incidence.identity = resolution.historical_component_root;
          if (!accumulate(
                  1U,
                  candidate_facts
                      .historical_positive_projection_count)) {
            return fail(
                std::move(result),
                ExactDirectSparseGatewayHistoricalQuotientDecision::
                    no_historical_quotient_capacity_overflow);
          }
          break;
        case ExactDirectSparseGatewayTemporalDisposition::latent_unresolved:
          if (resolution.positive_payload_present) {
            return fail(
                std::move(result),
                ExactDirectSparseGatewayHistoricalQuotientDecision::
                    no_historical_quotient_source_rejected);
          }
          incidence.kind =
              TypedVertexKind::latent_temporal_resolution;
          incidence.identity =
              temporal_reference.temporal_resolution_index;
          if (!accumulate(
                  1U,
                  candidate_facts.historical_latent_projection_count)) {
            return fail(
                std::move(result),
                ExactDirectSparseGatewayHistoricalQuotientDecision::
                    no_historical_quotient_capacity_overflow);
          }
          break;
        case ExactDirectSparseGatewayTemporalDisposition::not_certified:
          return fail(
              std::move(result),
              ExactDirectSparseGatewayHistoricalQuotientDecision::
                  no_historical_quotient_source_rejected);
      }
      if (!accumulate(
              1U, candidate_facts.deletion_projection_count)) {
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_capacity_overflow);
      }
      incidences.push_back(incidence);
      if (!accumulate(1U, result.counters.projection_scan_count) ||
          !accumulate(1U, result.counters.incidence_scan_count)) {
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_capacity_overflow);
      }
    }
    result.every_projection_mapped_to_one_typed_vertex = true;
    result.typed_root_and_latent_namespaces_disjoint = true;

    std::size_t expected_projection_offset = 0U;
    for (std::size_t candidate_index = 0U;
         candidate_index < candidate_scratch.size();
         ++candidate_index) {
      const auto& facts = candidate_scratch[candidate_index];
      if (facts.source_batch_index >= source.batches.size() ||
          facts.deletion_projection_count == 0U ||
          facts.deletion_projection_offset != expected_projection_offset ||
          facts.historical_positive_projection_count >
              facts.deletion_projection_count ||
          facts.historical_latent_projection_count !=
              facts.deletion_projection_count -
                  facts.historical_positive_projection_count ||
          !checked_add(
              expected_projection_offset,
              facts.deletion_projection_count,
              expected_projection_offset) ||
          !accumulate(
              1U, batch_scratch[facts.source_batch_index].candidate_count)) {
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_source_rejected);
      }
    }
    if (expected_projection_offset != incidences.size()) {
      return fail(
          std::move(result),
          ExactDirectSparseGatewayHistoricalQuotientDecision::
              no_historical_quotient_source_rejected);
    }
    for (const auto& batch : batch_scratch) {
      if (batch.candidate_count == 0U ||
          batch.strict_pre_locator_prefix_count == absent_index) {
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_source_rejected);
      }
    }
    result.every_candidate_partitioned_by_its_deletion_projections = true;

    ComparisonBudget comparisons(budget.maximum_sort_comparison_count);
    if (!bounded_heapsort(
            incidences, incidence_vertex_less, comparisons)) {
      result.counters.sort_comparison_count = comparisons.count();
      return fail(
          std::move(result),
          ExactDirectSparseGatewayHistoricalQuotientDecision::
              no_historical_quotient_budget_exhausted);
    }

    CandidateDisjointSet candidate_components(
        result.required_candidate_count,
        budget.maximum_candidate_dsu_parent_hop_count);
    for (std::size_t begin = 0U; begin < incidences.size();) {
      std::size_t end = begin + 1U;
      while (end < incidences.size() &&
             same_typed_vertex(incidences[begin], incidences[end])) {
        const CandidateDisjointSet::UnionResult union_result =
            candidate_components.unite(
                incidences[begin].gateway_candidate_index,
                incidences[end].gateway_candidate_index);
        if (union_result ==
            CandidateDisjointSet::UnionResult::budget_exhausted) {
          result.counters.candidate_dsu_parent_hop_count =
              candidate_components.parent_hop_count();
          return fail(
              std::move(result),
              ExactDirectSparseGatewayHistoricalQuotientDecision::
                  no_historical_quotient_budget_exhausted);
        }
        if (union_result == CandidateDisjointSet::UnionResult::joined &&
            !accumulate(1U, result.counters.candidate_union_count)) {
          return fail(
              std::move(result),
              ExactDirectSparseGatewayHistoricalQuotientDecision::
                  no_historical_quotient_capacity_overflow);
        }
        ++end;
      }
      begin = end;
    }
    if (!accumulate(
            incidences.size(), result.counters.incidence_scan_count)) {
      return fail(
          std::move(result),
          ExactDirectSparseGatewayHistoricalQuotientDecision::
              no_historical_quotient_capacity_overflow);
    }
    result.closure_strictly_batch_local = true;
    result.shared_latent_facets_join_candidate_hyperedges = true;
    result.candidate_hyperedge_duplicates_are_idempotent = true;

    std::vector<std::size_t> representatives;
    representatives.reserve(result.required_candidate_count);
    for (std::size_t candidate_index = 0U;
         candidate_index < result.required_candidate_count;
         ++candidate_index) {
      std::size_t representative = 0U;
      if (!candidate_components.find(candidate_index, representative)) {
        result.counters.candidate_dsu_parent_hop_count =
            candidate_components.parent_hop_count();
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_budget_exhausted);
      }
      if (representative == candidate_index) {
        representatives.push_back(candidate_index);
      }
    }
    const auto representative_less =
        [&candidate_scratch](
            std::size_t left, std::size_t right) noexcept {
          if (candidate_scratch[left].source_batch_index !=
              candidate_scratch[right].source_batch_index) {
            return candidate_scratch[left].source_batch_index <
                   candidate_scratch[right].source_batch_index;
          }
          return left < right;
        };
    if (!bounded_heapsort(
            representatives, representative_less, comparisons)) {
      result.counters.sort_comparison_count = comparisons.count();
      return fail(
          std::move(result),
          ExactDirectSparseGatewayHistoricalQuotientDecision::
              no_historical_quotient_budget_exhausted);
    }

    std::vector<std::size_t> component_index_by_representative(
        result.required_candidate_count, absent_index);
    std::vector<ExactDirectSparseGatewayHistoricalComponent> components;
    components.reserve(result.required_component_output_capacity);
    for (const std::size_t representative : representatives) {
      const std::size_t component_index = components.size();
      component_index_by_representative[representative] = component_index;
      const auto& facts = candidate_scratch[representative];
      components.push_back(
          {component_index,
           facts.source_batch_index,
           facts.strict_pre_locator_prefix_count,
           representative});
    }

    std::vector<
        ExactDirectSparseGatewayCandidateToHistoricalComponent>
        candidate_map;
    candidate_map.reserve(result.required_candidate_count);
    for (std::size_t candidate_index = 0U;
         candidate_index < result.required_candidate_count;
         ++candidate_index) {
      std::size_t representative = 0U;
      if (!candidate_components.find(candidate_index, representative)) {
        result.counters.candidate_dsu_parent_hop_count =
            candidate_components.parent_hop_count();
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_budget_exhausted);
      }
      if (representative >= component_index_by_representative.size() ||
          component_index_by_representative[representative] ==
              absent_index) {
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_source_rejected);
      }
      const std::size_t component_index =
          component_index_by_representative[representative];
      const auto& facts = candidate_scratch[candidate_index];
      if (components[component_index].source_batch_index !=
              facts.source_batch_index ||
          !accumulate(
              1U, components[component_index].candidate_count)) {
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_source_rejected);
      }
      candidate_map.push_back(
          {candidate_index,
           facts.source_batch_index,
           component_index,
           facts.deletion_projection_offset,
           facts.deletion_projection_count,
           facts.historical_positive_projection_count,
           facts.historical_latent_projection_count});
      if (!accumulate(1U, result.counters.candidate_scan_count)) {
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_capacity_overflow);
      }
    }
    result.counters.candidate_dsu_parent_hop_count =
        candidate_components.parent_hop_count();
    result.components_canonical_and_partition_candidates = true;

    for (auto& incidence : incidences) {
      incidence.historical_component_index =
          candidate_map[incidence.gateway_candidate_index]
              .historical_component_index;
    }
    if (!bounded_heapsort(
            incidences, incidence_component_less, comparisons)) {
      result.counters.sort_comparison_count = comparisons.count();
      return fail(
          std::move(result),
          ExactDirectSparseGatewayHistoricalQuotientDecision::
              no_historical_quotient_budget_exhausted);
    }
    result.counters.sort_comparison_count = comparisons.count();

    std::vector<ExactDirectSparseComponentHandle> root_ids;
    std::vector<std::size_t> latent_indices;
    root_ids.reserve(result.required_root_output_capacity);
    latent_indices.reserve(result.required_latent_output_capacity);
    std::size_t cursor = 0U;
    for (std::size_t component_index = 0U;
         component_index < components.size();
         ++component_index) {
      auto& component = components[component_index];
      component.root_offset = root_ids.size();
      component.latent_resolution_offset = latent_indices.size();
      while (cursor < incidences.size() &&
             incidences[cursor].historical_component_index ==
                 component_index) {
        const IncidenceScratch first = incidences[cursor];
        std::size_t end = cursor + 1U;
        while (end < incidences.size() &&
               same_projected_vertex(first, incidences[end])) {
          ++end;
        }
        if (first.kind == TypedVertexKind::historical_root) {
          root_ids.push_back(
              static_cast<ExactDirectSparseComponentHandle>(
                  first.identity));
          ++component.root_count;
        } else {
          if (first.identity >=
              temporal_resolution.temporal_resolutions.size()) {
            return fail(
                std::move(result),
                ExactDirectSparseGatewayHistoricalQuotientDecision::
                    no_historical_quotient_source_rejected);
          }
          latent_indices.push_back(first.identity);
          ++component.latent_resolution_count;
        }
        cursor = end;
      }
      if (component.root_count == 0U) {
        if (component.latent_resolution_count == 0U) {
          return fail(
              std::move(result),
              ExactDirectSparseGatewayHistoricalQuotientDecision::
                  no_historical_quotient_source_rejected);
        }
        component.disposition =
            ExactDirectSparseGatewayHistoricalQuotientDisposition::
                latent_only_unresolved;
        ++result.counters.latent_only_component_count;
      } else if (component.root_count == 1U) {
        component.disposition =
            ExactDirectSparseGatewayHistoricalQuotientDisposition::
                one_known_root_class;
        ++result.counters.rooted_component_count;
        ++result.counters.one_known_root_component_count;
      } else {
        component.disposition =
            ExactDirectSparseGatewayHistoricalQuotientDisposition::
                multiple_known_root_class;
        ++result.counters.rooted_component_count;
        ++result.counters.multiple_known_root_component_count;
      }
    }
    if (cursor != incidences.size() ||
        !accumulate(
            incidences.size(), result.counters.incidence_scan_count)) {
      return fail(
          std::move(result),
          ExactDirectSparseGatewayHistoricalQuotientDecision::
              no_historical_quotient_capacity_overflow);
    }

    std::vector<ExactDirectSparseGatewayHistoricalQuotientBatch> batches;
    batches.reserve(result.required_batch_count);
    std::size_t component_cursor = 0U;
    for (std::size_t batch_index = 0U;
         batch_index < batch_scratch.size();
         ++batch_index) {
      const std::size_t component_offset = component_cursor;
      std::size_t rooted = 0U;
      std::size_t latent_only = 0U;
      while (component_cursor < components.size() &&
             components[component_cursor].source_batch_index ==
                 batch_index) {
        if (components[component_cursor].root_count == 0U) {
          ++latent_only;
        } else {
          ++rooted;
        }
        ++component_cursor;
      }
      batches.push_back(
          {batch_index,
           batch_scratch[batch_index].strict_pre_locator_prefix_count,
           batch_scratch[batch_index].candidate_count,
           component_offset,
           component_cursor - component_offset,
           rooted,
           latent_only});
      if (!accumulate(1U, result.counters.batch_scan_count)) {
        return fail(
            std::move(result),
            ExactDirectSparseGatewayHistoricalQuotientDecision::
                no_historical_quotient_capacity_overflow);
      }
    }
    if (component_cursor != components.size()) {
      return fail(
          std::move(result),
          ExactDirectSparseGatewayHistoricalQuotientDecision::
              no_historical_quotient_source_rejected);
    }

    result.counters.component_count = components.size();
    result.counters.distinct_root_vertex_count = root_ids.size();
    result.counters.distinct_latent_vertex_count = latent_indices.size();
    result.known_root_projection_exact_for_supplied_candidates = true;
    result.latent_only_components_preserved_as_unresolved = true;
    result.roots_and_latent_resolution_slices_canonical = true;
    if (!checked_add(
            batches.size(), candidate_map.size(), result.logical_output_entry_count) ||
        !checked_add(
            result.logical_output_entry_count,
            components.size(),
            result.logical_output_entry_count) ||
        !checked_add(
            result.logical_output_entry_count,
            root_ids.size(),
            result.logical_output_entry_count) ||
        !checked_add(
            result.logical_output_entry_count,
            latent_indices.size(),
            result.logical_output_entry_count) ||
        result.logical_output_entry_count >
            result.required_logical_output_capacity) {
      return fail(
          std::move(result),
          ExactDirectSparseGatewayHistoricalQuotientDecision::
              no_historical_quotient_capacity_overflow);
    }

    result.batches = std::move(batches);
    result.candidate_to_component = std::move(candidate_map);
    result.components = std::move(components);
    result.component_root_ids = std::move(root_ids);
    result.component_latent_resolution_indices =
        std::move(latent_indices);
    result.decision =
        ExactDirectSparseGatewayHistoricalQuotientDecision::
            complete_certified_batch_local_historical_root_quotient_proposal;
    if (!result.certified_partial_refinement()) {
      throw std::logic_error(
          "a complete historical gateway quotient failed its contract");
    }
    return result;
  } catch (const std::length_error&) {
    return fail(
        std::move(result),
        ExactDirectSparseGatewayHistoricalQuotientDecision::
            no_historical_quotient_capacity_overflow);
  } catch (const std::bad_alloc&) {
    return fail(
        std::move(result),
        ExactDirectSparseGatewayHistoricalQuotientDecision::
            no_historical_quotient_allocation_failed);
  }
}

ExactDirectSparseGatewayHistoricalQuotientVerification
verify_exact_direct_sparse_gateway_historical_quotient(
    const ExactDirectSparseGatewayTemporalResolutionAuthorityBundle&
        authorities,
    const ExactDirectSparseGatewayTemporalResolutionBudget& temporal_budget,
    const ExactDirectSparsePositiveFacetPrefixSweepBudget&
        prefix_sweep_budget,
    const ExactDirectSparseGatewayTemporalResolutionResult&
        observed_temporal_resolution,
    const ExactDirectSparseGatewayHistoricalQuotientBudget& budget,
    const ExactDirectSparseGatewayHistoricalQuotientResult& observed) {
  ExactDirectSparseGatewayHistoricalQuotientVerification verification;
  verification.authority_bundle_complete =
      authorities.source_gateway_journal != nullptr &&
      authorities.observed_localization != nullptr;
  verification.observed_storage_within_budget =
      observed_storage_within_budget(observed, budget);
  if (!verification.authority_bundle_complete ||
      !verification.observed_storage_within_budget) {
    return verification;
  }

  try {
    verification.temporal_resolution_verification =
        verify_exact_direct_sparse_gateway_temporal_resolution(
            authorities,
            temporal_budget,
            prefix_sweep_budget,
            observed_temporal_resolution);
  } catch (const std::bad_alloc&) {
    return verification;
  } catch (const std::length_error&) {
    return verification;
  } catch (const std::exception&) {
    return verification;
  }
  verification.authority_bundle_complete =
      verification.temporal_resolution_verification
          .authority_bundle_complete;
  verification.temporal_resolution_freshly_replayed =
      verification.temporal_resolution_verification.result_certified;
  if (!verification.authority_bundle_complete ||
      !verification.temporal_resolution_freshly_replayed) {
    return verification;
  }

  const auto expected =
      build_exact_direct_sparse_gateway_historical_quotient(
          *authorities.source_gateway_journal,
          *authorities.observed_localization,
          observed_temporal_resolution,
          budget);
  verification.expected_result_freshly_reconstructed =
      expected.certified_partial_refinement();
  if (!verification.expected_result_freshly_reconstructed) {
    return verification;
  }
  verification.observed_result_recursively_equal = observed == expected;
  verification.batch_local_typed_closure_freshly_replayed =
      verification.observed_result_recursively_equal &&
      expected.known_root_projection_exact_for_supplied_candidates &&
      expected.shared_latent_facets_join_candidate_hyperedges;
  verification.external_clock_authority_replayed =
      verification.temporal_resolution_verification
          .external_clock_authority_replayed;
  verification.external_binding_authority_replayed =
      verification.temporal_resolution_verification
          .external_binding_authority_replayed;
  verification.conditional_on_caller_external_binding_authority_replay =
      verification.temporal_resolution_verification
          .conditional_on_caller_external_binding_authority_replay;
  verification.conditional_on_caller_strict_pre_lot_orchestration =
      verification.temporal_resolution_verification
          .conditional_on_caller_strict_pre_lot_orchestration;
  verification.external_freeze_synchronization_replayed =
      verification.temporal_resolution_verification
          .external_freeze_synchronization_replayed;
  verification.conditional_on_caller_external_freeze_synchronization =
      verification.temporal_resolution_verification
          .conditional_on_caller_external_freeze_synchronization;
  verification.in_memory_replay_only =
      verification.temporal_resolution_verification.in_memory_replay_only;
  verification.crash_durable =
      verification.temporal_resolution_verification.crash_durable;
  verification.no_hgp_action_or_forbidden_global_structure_materialized =
      !observed.root_birth_continuation_or_multifusion_claimed &&
      !expected.root_birth_continuation_or_multifusion_claimed &&
      !observed.locator_root_union_or_forest_mutated &&
      !expected.locator_root_union_or_forest_mutated &&
      !observed.gateway_attach_published &&
      !expected.gateway_attach_published &&
      !observed.forbidden_global_structure_materialized &&
      !expected.forbidden_global_structure_materialized &&
      !observed.gamma_cells_or_higher_order_delaunay_materialized &&
      !expected.gamma_cells_or_higher_order_delaunay_materialized;
  verification.fresh_replay_certified =
      verification.temporal_resolution_freshly_replayed &&
      verification.batch_local_typed_closure_freshly_replayed;
  verification.result_certified =
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
      verification
          .no_hgp_action_or_forbidden_global_structure_materialized &&
      observed.certified_partial_refinement();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
