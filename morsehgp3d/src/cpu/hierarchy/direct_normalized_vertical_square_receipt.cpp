#include "morsehgp3d/hierarchy/direct_normalized_vertical_square_receipt.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <numeric>
#include <utility>
#include <vector>

#include <boost/multiprecision/integer.hpp>

namespace morsehgp3d::hierarchy {
namespace {

using Key = ExactDirectNormalizedVerticalSimplexKey;
using Result = ExactDirectNormalizedVerticalSquareResult;
using Decision = ExactDirectNormalizedVerticalSquareDecision;

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] std::size_t integer_bit_count(
    const exact::BigInt& value) noexcept {
  // ExactLevel numerators are nonnegative and denominators are positive.
  if (value == 0) {
    return 1U;
  }
  return static_cast<std::size_t>(boost::multiprecision::msb(value)) +
         1U;
}

[[nodiscard]] bool key_less(const Key& left, const Key& right) noexcept {
  if (left.point_count != right.point_count) {
    return left.point_count < right.point_count;
  }
  for (std::size_t index = 0U; index < left.point_count; ++index) {
    if (left.point_ids[index] != right.point_ids[index]) {
      return left.point_ids[index] < right.point_ids[index];
    }
  }
  return false;
}

void clear_payload(Result& result) noexcept {
  result.component_images.clear();
  result.squares.clear();
  result.counters = {};
}

[[nodiscard]] Result fail(Result result, Decision decision) noexcept {
  clear_payload(result);
  result.trusted_normalized_source_receipts_accepted = false;
  result.trusted_target_carrier_receipts_accepted = false;
  result.trusted_horizontal_inclusion_receipt_accepted = false;
  result.canonical_point_namespace_identity_certified = false;
  result.adjacent_orders_certified = false;
  result.exact_closed_levels_certified = false;
  result.source_component_memberships_replayed = false;
  result.source_spanning_trees_replayed = false;
  result.every_source_adjacency_has_exact_common_target_facet = false;
  result.every_common_target_facet_resolved_at_same_closed_cut = false;
  result.representative_independence_proved = false;
  result.source_horizontal_inclusion_replayed = false;
  result.persistent_common_facet_target_propagation_replayed = false;
  result.commuting_horizontal_vertical_square_replayed = false;
  result.no_partial_scientific_payload_published_on_failure = true;
  result.decision = decision;
  result.scope = ExactDirectNormalizedVerticalSquareScope::unspecified;
  return result;
}

class Accounting {
 public:
  Accounting(
      const ExactDirectNormalizedVerticalSquareBudget& budget,
      ExactDirectNormalizedVerticalSquareCounters& counters) noexcept
      : budget_(budget), counters_(counters) {}

  [[nodiscard]] bool measure_level(const exact::ExactLevel& level) noexcept {
    const std::size_t observed = std::max(
        integer_bit_count(level.numerator()),
        integer_bit_count(level.denominator()));
    counters_.maximum_observed_exact_level_integer_bit_count = std::max(
        counters_.maximum_observed_exact_level_integer_bit_count,
        observed);
    if (observed > budget_.maximum_single_exact_level_integer_bit_count) {
      exhausted_ = true;
      return false;
    }
    return true;
  }

  [[nodiscard]] bool level_equal(
      const exact::ExactLevel& left,
      const exact::ExactLevel& right) {
    if (!charge(
            counters_.exact_level_comparison_count,
            budget_.maximum_exact_level_comparison_count)) {
      return false;
    }
    relation_ = left == right ? 0 : (left < right ? -1 : 1);
    return true;
  }

  [[nodiscard]] bool level_less(
      const exact::ExactLevel& left,
      const exact::ExactLevel& right) {
    if (!charge(
            counters_.exact_level_comparison_count,
            budget_.maximum_exact_level_comparison_count)) {
      return false;
    }
    relation_ = left < right ? -1 : (right < left ? 1 : 0);
    return true;
  }

  [[nodiscard]] bool level_less_equal(
      const exact::ExactLevel& left,
      const exact::ExactLevel& right) {
    if (!charge(
            counters_.exact_level_comparison_count,
            budget_.maximum_exact_level_comparison_count)) {
      return false;
    }
    relation_ = left < right ? -1 : (right < left ? 1 : 0);
    return true;
  }

  [[nodiscard]] int relation() const noexcept { return relation_; }

  [[nodiscard]] bool charge_key_points(std::size_t count) noexcept {
    if (count >
        budget_.maximum_key_point_scan_count -
            std::min(
                budget_.maximum_key_point_scan_count,
                counters_.key_point_scan_count)) {
      exhausted_ = true;
      return false;
    }
    counters_.key_point_scan_count += count;
    return true;
  }

  [[nodiscard]] bool charge_key_lookup_comparison() noexcept {
    return charge(
        counters_.key_lookup_comparison_count,
        budget_.maximum_key_lookup_comparison_count);
  }

  [[nodiscard]] bool charge_subset_lookup() noexcept {
    return charge(
        counters_.source_subset_lookup_count,
        budget_.maximum_source_subset_lookup_count);
  }

  [[nodiscard]] bool charge_subset_comparison() noexcept {
    return charge(
        counters_.source_subset_comparison_count,
        budget_.maximum_source_subset_comparison_count);
  }

  [[nodiscard]] bool exhausted() const noexcept { return exhausted_; }

 private:
  [[nodiscard]] bool charge(
      std::size_t& counter,
      std::size_t maximum) noexcept {
    if (counter >= maximum) {
      exhausted_ = true;
      return false;
    }
    ++counter;
    return true;
  }

  const ExactDirectNormalizedVerticalSquareBudget& budget_;
  ExactDirectNormalizedVerticalSquareCounters& counters_;
  int relation_{};
  bool exhausted_{false};
};

[[nodiscard]] bool key_valid(
    const Key& key,
    std::size_t point_count,
    std::size_t expected_cardinality,
    Accounting& accounting) noexcept {
  if (key.point_count != expected_cardinality || key.point_count == 0U ||
      key.point_count > key.point_ids.size() ||
      !accounting.charge_key_points(key.point_count)) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (key.point_ids[index] >= point_count ||
        (index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index])) {
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

[[nodiscard]] bool union_and_intersection_match(
    const Key& left,
    const Key& right,
    const Key& expected_union,
    const Key& expected_intersection,
    Accounting& accounting) noexcept {
  if (!accounting.charge_key_points(left.point_count) ||
      !accounting.charge_key_points(right.point_count)) {
    return false;
  }

  Key observed_union{};
  Key observed_intersection{};
  std::size_t left_index = 0U;
  std::size_t right_index = 0U;
  while (left_index < left.point_count || right_index < right.point_count) {
    if (right_index == right.point_count ||
        (left_index < left.point_count &&
         left.point_ids[left_index] < right.point_ids[right_index])) {
      if (observed_union.point_count == observed_union.point_ids.size()) {
        return false;
      }
      observed_union.point_ids[observed_union.point_count++] =
          left.point_ids[left_index++];
      continue;
    }
    if (left_index == left.point_count ||
        right.point_ids[right_index] < left.point_ids[left_index]) {
      if (observed_union.point_count == observed_union.point_ids.size()) {
        return false;
      }
      observed_union.point_ids[observed_union.point_count++] =
          right.point_ids[right_index++];
      continue;
    }
    const spatial::PointId shared = left.point_ids[left_index];
    if (observed_union.point_count == observed_union.point_ids.size() ||
        observed_intersection.point_count ==
            observed_intersection.point_ids.size()) {
      return false;
    }
    observed_union.point_ids[observed_union.point_count++] = shared;
    observed_intersection.point_ids[observed_intersection.point_count++] =
        shared;
    ++left_index;
    ++right_index;
  }
  return observed_union == expected_union &&
         observed_intersection == expected_intersection;
}

class DisjointSet {
 public:
  explicit DisjointSet(std::size_t count) : parents_(count) {
    std::iota(parents_.begin(), parents_.end(), 0U);
  }

  [[nodiscard]] std::size_t find(std::size_t value) noexcept {
    std::size_t root = value;
    while (parents_[root] != root) {
      root = parents_[root];
    }
    while (parents_[value] != value) {
      const std::size_t next = parents_[value];
      parents_[value] = root;
      value = next;
    }
    return root;
  }

  [[nodiscard]] bool unite(std::size_t left, std::size_t right) noexcept {
    left = find(left);
    right = find(right);
    if (left == right) {
      return false;
    }
    if (right < left) {
      std::swap(left, right);
    }
    parents_[right] = left;
    return true;
  }

 private:
  std::vector<std::size_t> parents_;
};

[[nodiscard]] bool source_authority_flags_valid(
    const ExactDirectNormalizedVerticalSourceComponentReceipt& source)
    noexcept {
  return source.normalized_source_authority_id != 0U &&
         source.normalized_source_freshly_verified &&
         source.closed_component_membership_exhaustive &&
         source.source_adjacencies_freshly_verified &&
         source.source_label_and_coface_levels_exact &&
         !source.global_facet_coface_gamma_or_higher_delaunay_materialized &&
         !source.public_status_claimed;
}

[[nodiscard]] bool target_authority_flags_valid(
    const ExactDirectNormalizedVerticalTargetCarrierReceipt& target)
    noexcept {
  return target.target_carrier_authority_id != 0U &&
         target.target_carrier_authority_freshly_verified &&
         target.bindings_complete_for_requested_facets &&
         target.roots_are_closed_at_exact_level &&
         !target.global_facet_coface_gamma_or_higher_delaunay_materialized &&
         !target.public_status_claimed;
}

[[nodiscard]] bool horizontal_authority_flags_valid(
    const ExactDirectNormalizedHorizontalCarrierInclusionReceipt& horizontal)
    noexcept {
  return horizontal.normalized_source_authority_id != 0U &&
         horizontal.inclusion_receipt_id != 0U &&
         horizontal.normalized_horizontal_inclusion_freshly_verified &&
         horizontal.source_successor_unique &&
         horizontal.all_intermediate_target_levels_accounted &&
         !horizontal.public_status_claimed;
}

[[nodiscard]] bool validate_source_component(
    const ExactDirectNormalizedVerticalSourceComponentReceipt& source,
    Accounting& accounting,
    ExactDirectNormalizedVerticalSquareCounters& counters) {
  if (source.labels.size() < 2U ||
      source.spanning_tree_edges.size() != source.labels.size() - 1U) {
    return false;
  }
  counters.logical_scratch_entry_count = std::max(
      counters.logical_scratch_entry_count, source.labels.size());

  for (std::size_t index = 0U; index < source.labels.size(); ++index) {
    ++counters.source_label_scan_count;
    const auto& label = source.labels[index];
    if (label.label_reference_index != index ||
        !key_valid(
            label.label_key,
            source.point_count,
            source.source_order,
            accounting) ||
        !accounting.measure_level(label.squared_birth_level) ||
        !accounting.level_less_equal(
            label.squared_birth_level, source.closed_squared_level) ||
        accounting.relation() > 0 ||
        (index != 0U &&
         !key_less(source.labels[index - 1U].label_key, label.label_key))) {
      return false;
    }
  }

  DisjointSet tree(source.labels.size());
  for (std::size_t index = 0U; index < source.spanning_tree_edges.size();
       ++index) {
    ++counters.source_tree_edge_scan_count;
    const auto& edge = source.spanning_tree_edges[index];
    if (edge.tree_edge_index != index ||
        edge.left_label_reference_index >= source.labels.size() ||
        edge.right_label_reference_index >= source.labels.size() ||
        edge.left_label_reference_index == edge.right_label_reference_index ||
        !key_valid(
            edge.source_coface_key,
            source.point_count,
            source.source_order + 1U,
            accounting) ||
        !key_valid(
            edge.common_target_facet_key,
            source.point_count,
            source.source_order - 1U,
            accounting) ||
        !accounting.measure_level(edge.source_coface_squared_level) ||
        !accounting.level_less_equal(
            edge.source_coface_squared_level, source.closed_squared_level) ||
        accounting.relation() > 0 ||
        !accounting.level_less_equal(
            source.labels[edge.left_label_reference_index]
                .squared_birth_level,
            edge.source_coface_squared_level) ||
        accounting.relation() > 0 ||
        !accounting.level_less_equal(
            source.labels[edge.right_label_reference_index]
                .squared_birth_level,
            edge.source_coface_squared_level) ||
        accounting.relation() > 0) {
      return false;
    }
    const auto& left =
        source.labels[edge.left_label_reference_index].label_key;
    const auto& right =
        source.labels[edge.right_label_reference_index].label_key;
    if (!union_and_intersection_match(
            left,
            right,
            edge.source_coface_key,
            edge.common_target_facet_key,
            accounting) ||
        !tree.unite(
            edge.left_label_reference_index,
            edge.right_label_reference_index)) {
      return false;
    }
    ++counters.tree_union_count;
  }

  const std::size_t tree_root = tree.find(0U);
  for (std::size_t index = 1U; index < source.labels.size(); ++index) {
    if (tree.find(index) != tree_root) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool validate_target_receipt(
    const ExactDirectNormalizedVerticalTargetCarrierReceipt& target,
    Accounting& accounting,
    ExactDirectNormalizedVerticalSquareCounters& counters) noexcept {
  for (std::size_t index = 0U; index < target.bindings.size(); ++index) {
    ++counters.target_binding_scan_count;
    const auto& binding = target.bindings[index];
    if (binding.binding_index != index ||
        !key_valid(
            binding.target_facet_key,
            target.point_count,
            target.target_order,
            accounting) ||
        (index != 0U &&
         !key_less(
             target.bindings[index - 1U].target_facet_key,
             binding.target_facet_key))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] const ExactDirectNormalizedVerticalTargetFacetBindingReceipt*
find_target_binding(
    const ExactDirectNormalizedVerticalTargetCarrierReceipt& target,
    const Key& key,
    Accounting& accounting) noexcept {
  std::size_t first = 0U;
  std::size_t count = target.bindings.size();
  while (count != 0U) {
    if (!accounting.charge_key_lookup_comparison()) {
      return nullptr;
    }
    const std::size_t step = count / 2U;
    const std::size_t middle = first + step;
    if (key_less(target.bindings[middle].target_facet_key, key)) {
      first = middle + 1U;
      count -= step + 1U;
    } else {
      count = step;
    }
  }
  if (first >= target.bindings.size()) {
    return nullptr;
  }
  if (!accounting.charge_key_lookup_comparison() ||
      key_less(key, target.bindings[first].target_facet_key) ||
      key_less(target.bindings[first].target_facet_key, key)) {
    return nullptr;
  }
  return &target.bindings[first];
}

[[nodiscard]] bool build_component_image(
    const ExactDirectNormalizedVerticalSourceComponentReceipt& source,
    const ExactDirectNormalizedVerticalTargetCarrierReceipt& target,
    ExactDirectNormalizedVerticalComponentRole role,
    std::size_t image_index,
    Accounting& accounting,
    ExactDirectNormalizedVerticalComponentImageReceipt& output) noexcept {
  bool root_initialized = false;
  std::uint64_t common_root = 0U;
  const ExactDirectNormalizedVerticalSourceTreeEdgeReceipt* witness_edge =
      nullptr;
  const ExactDirectNormalizedVerticalTargetFacetBindingReceipt*
      witness_binding = nullptr;

  for (const auto& edge : source.spanning_tree_edges) {
    const auto* binding =
        find_target_binding(target, edge.common_target_facet_key, accounting);
    if (binding == nullptr) {
      return false;
    }
    if (!root_initialized) {
      common_root = binding->closed_target_root_id;
      root_initialized = true;
    } else if (binding->closed_target_root_id != common_root) {
      return false;
    }
    if (witness_edge == nullptr ||
        key_less(
            edge.common_target_facet_key,
            witness_edge->common_target_facet_key) ||
        (edge.common_target_facet_key ==
             witness_edge->common_target_facet_key &&
         edge.tree_edge_index < witness_edge->tree_edge_index)) {
      witness_edge = &edge;
      witness_binding = binding;
    }
  }
  if (!root_initialized || witness_edge == nullptr ||
      witness_binding == nullptr) {
    return false;
  }

  output.component_image_index = image_index;
  output.role = role;
  output.source_component_handle = source.source_component_handle;
  output.closed_squared_level = source.closed_squared_level;
  // labels are strictly canonical, so the first one is the representative.
  output.canonical_representative_label_reference_index = 0U;
  output.canonical_witness_tree_edge_index = witness_edge->tree_edge_index;
  output.canonical_witness_target_binding_index =
      witness_binding->binding_index;
  output.canonical_common_target_facet_key =
      witness_edge->common_target_facet_key;
  output.closed_target_root_id = common_root;
  return true;
}

[[nodiscard]] bool source_labels_are_subset(
    const ExactDirectNormalizedVerticalSourceComponentReceipt& before,
    const ExactDirectNormalizedVerticalSourceComponentReceipt& after,
    Accounting& accounting) noexcept {
  for (const auto& label : before.labels) {
    if (!accounting.charge_subset_lookup()) {
      return false;
    }
    std::size_t first = 0U;
    std::size_t count = after.labels.size();
    while (count != 0U) {
      if (!accounting.charge_subset_comparison()) {
        return false;
      }
      const std::size_t step = count / 2U;
      const std::size_t middle = first + step;
      if (key_less(after.labels[middle].label_key, label.label_key)) {
        first = middle + 1U;
        count -= step + 1U;
      } else {
        count = step;
      }
    }
    if (first >= after.labels.size() ||
        !accounting.charge_subset_comparison() ||
        key_less(label.label_key, after.labels[first].label_key) ||
        key_less(after.labels[first].label_key, label.label_key)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool unsupported_claims_false(const Result& result) noexcept {
  return result.conditional_on_trusted_normalized_source_and_carrier_receipts &&
         !result.global_regularity_authority_certified &&
         !result.incidence_complete_reduction_proved &&
         !result.all_naturality_squares_replayed &&
         !result.vertical_maps_complete &&
         !result.global_facet_coface_gamma_or_higher_delaunay_materialized &&
         !result.public_status_claimed;
}

}  // namespace

bool ExactDirectNormalizedVerticalSquareResult::
    certified_conditional_vertical_square() const noexcept {
  return schema_version ==
             direct_normalized_vertical_square_receipt_schema_version &&
         component_images.size() == 2U && squares.size() == 1U &&
         counters.logical_output_entry_count == 3U &&
         trusted_normalized_source_receipts_accepted &&
         trusted_target_carrier_receipts_accepted &&
         trusted_horizontal_inclusion_receipt_accepted &&
         canonical_point_namespace_identity_certified &&
         adjacent_orders_certified && exact_closed_levels_certified &&
         source_component_memberships_replayed &&
         source_spanning_trees_replayed &&
         every_source_adjacency_has_exact_common_target_facet &&
         every_common_target_facet_resolved_at_same_closed_cut &&
         representative_independence_proved &&
         source_horizontal_inclusion_replayed &&
         persistent_common_facet_target_propagation_replayed &&
         commuting_horizontal_vertical_square_replayed &&
         !no_partial_scientific_payload_published_on_failure &&
         unsupported_claims_false(*this) &&
         decision == ExactDirectNormalizedVerticalSquareDecision::
                         complete_conditional_normalized_common_facet_vertical_square &&
         scope == ExactDirectNormalizedVerticalSquareScope::
                      one_trusted_normalized_adjacent_order_closed_component_horizontal_step_only &&
         source_order >= 2U && target_order + 1U == source_order &&
         squares.front().from_component_image_index == 0U &&
         squares.front().to_component_image_index == 1U &&
         squares.front().from_closed_target_root_id ==
             component_images[0U].closed_target_root_id &&
         squares.front().propagated_closed_target_root_id ==
             squares.front().to_closed_target_root_id &&
         squares.front().to_closed_target_root_id ==
             component_images[1U].closed_target_root_id &&
         counters.source_label_scan_count <=
             requested_budget.maximum_source_label_scan_count &&
         counters.source_tree_edge_scan_count <=
             requested_budget.maximum_source_tree_edge_scan_count &&
         counters.target_binding_scan_count <=
             requested_budget.maximum_target_binding_scan_count &&
         counters.key_point_scan_count <=
             requested_budget.maximum_key_point_scan_count &&
         counters.key_lookup_comparison_count <=
             requested_budget.maximum_key_lookup_comparison_count &&
         counters.source_subset_lookup_count <=
             requested_budget.maximum_source_subset_lookup_count &&
         counters.source_subset_comparison_count <=
             requested_budget.maximum_source_subset_comparison_count &&
         counters.logical_scratch_entry_count <=
             requested_budget.maximum_tree_scratch_count &&
         counters.exact_level_comparison_count <=
             requested_budget.maximum_exact_level_comparison_count &&
         counters.maximum_observed_exact_level_integer_bit_count <=
             requested_budget.maximum_single_exact_level_integer_bit_count &&
         counters.logical_output_entry_count <=
             requested_budget.maximum_logical_output_entry_count;
}

bool ExactDirectNormalizedVerticalSquareResult::certified_atomic_failure()
    const noexcept {
  const bool failure_decision =
      decision != ExactDirectNormalizedVerticalSquareDecision::not_certified &&
      decision != ExactDirectNormalizedVerticalSquareDecision::
                      complete_conditional_normalized_common_facet_vertical_square;
  return schema_version ==
             direct_normalized_vertical_square_receipt_schema_version &&
         failure_decision && component_images.empty() && squares.empty() &&
         counters == ExactDirectNormalizedVerticalSquareCounters{} &&
         no_partial_scientific_payload_published_on_failure &&
         unsupported_claims_false(*this) &&
         scope == ExactDirectNormalizedVerticalSquareScope::unspecified;
}

bool ExactDirectNormalizedVerticalSquareResult::certified_outcome()
    const noexcept {
  return certified_conditional_vertical_square() || certified_atomic_failure();
}

ExactDirectNormalizedVerticalSquareResult
build_exact_direct_normalized_vertical_square_receipt(
    const ExactDirectNormalizedVerticalSourceComponentReceipt& before_source,
    const ExactDirectNormalizedVerticalTargetCarrierReceipt& before_target,
    const ExactDirectNormalizedVerticalSourceComponentReceipt& after_source,
    const ExactDirectNormalizedVerticalTargetCarrierReceipt& after_target,
    const ExactDirectNormalizedHorizontalCarrierInclusionReceipt& horizontal,
    const ExactDirectNormalizedVerticalSquareBudget& budget) {
  Result result{};
  result.requested_budget = budget;

  try {
    result.point_count = before_source.point_count;
    result.source_order = before_source.source_order;
    result.target_order = before_target.target_order;
    result.normalized_source_authority_id =
        before_source.normalized_source_authority_id;
    result.target_carrier_authority_id =
        before_target.target_carrier_authority_id;
    result.canonical_cloud_digest = before_source.canonical_cloud_digest;
    result.from_closed_squared_level = before_source.closed_squared_level;
    result.to_closed_squared_level = after_source.closed_squared_level;

    std::size_t required_source_labels = 0U;
    std::size_t required_source_edges = 0U;
    std::size_t required_target_bindings = 0U;
    if (!checked_add(
            before_source.labels.size(),
            after_source.labels.size(),
            required_source_labels) ||
        !checked_add(
            before_source.spanning_tree_edges.size(),
            after_source.spanning_tree_edges.size(),
            required_source_edges) ||
        !checked_add(
            before_target.bindings.size(),
            after_target.bindings.size(),
            required_target_bindings)) {
      return fail(
          std::move(result),
          Decision::no_square_capacity_overflow);
    }
    const std::size_t required_tree_scratch =
        std::max(before_source.labels.size(), after_source.labels.size());
    if (required_source_labels > budget.maximum_source_label_scan_count ||
        required_source_edges >
            budget.maximum_source_tree_edge_scan_count ||
        required_target_bindings >
            budget.maximum_target_binding_scan_count ||
        before_source.labels.size() >
            budget.maximum_source_subset_lookup_count ||
        required_tree_scratch > budget.maximum_tree_scratch_count ||
        3U > budget.maximum_logical_output_entry_count) {
      return fail(
          std::move(result),
          Decision::no_square_budget_exhausted);
    }

    if (!source_authority_flags_valid(before_source) ||
        !source_authority_flags_valid(after_source) ||
        before_source.normalized_source_authority_id !=
            after_source.normalized_source_authority_id ||
        before_source.point_count != after_source.point_count ||
        before_source.source_order != after_source.source_order ||
        before_source.source_order < 2U ||
        before_source.source_order >=
            direct_normalized_vertical_square_maximum_key_point_count ||
        before_source.point_count < before_source.source_order + 1U ||
        before_source.canonical_cloud_digest !=
            after_source.canonical_cloud_digest) {
      return fail(
          std::move(result),
          Decision::no_square_source_authority_rejected);
    }
    if (!target_authority_flags_valid(before_target) ||
        !target_authority_flags_valid(after_target) ||
        before_target.target_carrier_authority_id !=
            after_target.target_carrier_authority_id ||
        before_target.point_count != after_target.point_count ||
        before_target.target_order != after_target.target_order ||
        before_target.target_order + 1U != before_source.source_order ||
        before_target.point_count != before_source.point_count ||
        before_target.canonical_cloud_digest !=
            before_source.canonical_cloud_digest ||
        after_target.canonical_cloud_digest !=
            before_source.canonical_cloud_digest) {
      return fail(
          std::move(result),
          Decision::no_square_target_authority_rejected);
    }
    if (!horizontal_authority_flags_valid(horizontal) ||
        horizontal.normalized_source_authority_id !=
            before_source.normalized_source_authority_id ||
        horizontal.from_source_component_handle !=
            before_source.source_component_handle ||
        horizontal.to_source_component_handle !=
            after_source.source_component_handle) {
      return fail(
          std::move(result),
          Decision::no_square_horizontal_authority_rejected);
    }

    Accounting accounting{budget, result.counters};
    const exact::ExactLevel* levels[] = {
        &before_source.closed_squared_level,
        &after_source.closed_squared_level,
        &before_target.closed_squared_level,
        &after_target.closed_squared_level,
        &horizontal.from_closed_squared_level,
        &horizontal.to_closed_squared_level,
    };
    for (const exact::ExactLevel* level : levels) {
      if (!accounting.measure_level(*level)) {
        return fail(
            std::move(result),
            Decision::no_square_budget_exhausted);
      }
    }
    if (!accounting.level_equal(
            before_source.closed_squared_level,
            before_target.closed_squared_level) ||
        accounting.relation() != 0 ||
        !accounting.level_equal(
            after_source.closed_squared_level,
            after_target.closed_squared_level) ||
        accounting.relation() != 0 ||
        !accounting.level_equal(
            horizontal.from_closed_squared_level,
            before_source.closed_squared_level) ||
        accounting.relation() != 0 ||
        !accounting.level_equal(
            horizontal.to_closed_squared_level,
            after_source.closed_squared_level) ||
        accounting.relation() != 0 ||
        !accounting.level_less(
            before_source.closed_squared_level,
            after_source.closed_squared_level) ||
        accounting.relation() >= 0) {
      return fail(
          std::move(result),
          accounting.exhausted()
              ? Decision::no_square_budget_exhausted
              : Decision::no_square_horizontal_authority_rejected);
    }

    if (!validate_source_component(
            before_source, accounting, result.counters) ||
        !validate_source_component(
            after_source, accounting, result.counters)) {
      return fail(
          std::move(result),
          accounting.exhausted()
              ? Decision::no_square_budget_exhausted
              : Decision::no_square_common_facet_contradiction);
    }
    if (!validate_target_receipt(
            before_target, accounting, result.counters) ||
        !validate_target_receipt(
            after_target, accounting, result.counters)) {
      return fail(
          std::move(result),
          accounting.exhausted()
              ? Decision::no_square_budget_exhausted
              : Decision::no_square_target_authority_rejected);
    }

    result.component_images.resize(2U);
    if (!build_component_image(
            before_source,
            before_target,
            ExactDirectNormalizedVerticalComponentRole::
                before_horizontal_step,
            0U,
            accounting,
            result.component_images[0U]) ||
        !build_component_image(
            after_source,
            after_target,
            ExactDirectNormalizedVerticalComponentRole::after_horizontal_step,
            1U,
            accounting,
            result.component_images[1U])) {
      return fail(
          std::move(result),
          accounting.exhausted()
              ? Decision::no_square_budget_exhausted
              : Decision::no_square_target_representative_conflict);
    }

    if (!source_labels_are_subset(before_source, after_source, accounting)) {
      return fail(
          std::move(result),
          accounting.exhausted()
              ? Decision::no_square_budget_exhausted
              : Decision::no_square_horizontal_inclusion_contradiction);
    }

    const Key& persistent_facet =
        result.component_images[0U].canonical_common_target_facet_key;
    const auto* propagated_binding =
        find_target_binding(after_target, persistent_facet, accounting);
    if (propagated_binding == nullptr ||
        propagated_binding->closed_target_root_id !=
            result.component_images[1U].closed_target_root_id) {
      return fail(
          std::move(result),
          accounting.exhausted()
              ? Decision::no_square_budget_exhausted
              : Decision::no_square_commutation_contradiction);
    }

    result.squares.push_back(
        ExactDirectNormalizedVerticalCommutingSquareReceipt{
            .square_index = 0U,
            .from_component_image_index = 0U,
            .to_component_image_index = 1U,
            .horizontal_inclusion_receipt_id =
                horizontal.inclusion_receipt_id,
            .persistent_common_target_facet_key = persistent_facet,
            .persistent_target_binding_at_to_cut_index =
                propagated_binding->binding_index,
            .from_closed_target_root_id =
                result.component_images[0U].closed_target_root_id,
            .propagated_closed_target_root_id =
                propagated_binding->closed_target_root_id,
            .to_closed_target_root_id =
                result.component_images[1U].closed_target_root_id,
        });

    result.counters.logical_output_entry_count = 3U;
    result.trusted_normalized_source_receipts_accepted = true;
    result.trusted_target_carrier_receipts_accepted = true;
    result.trusted_horizontal_inclusion_receipt_accepted = true;
    result.canonical_point_namespace_identity_certified = true;
    result.adjacent_orders_certified = true;
    result.exact_closed_levels_certified = true;
    result.source_component_memberships_replayed = true;
    result.source_spanning_trees_replayed = true;
    result.every_source_adjacency_has_exact_common_target_facet = true;
    result.every_common_target_facet_resolved_at_same_closed_cut = true;
    result.representative_independence_proved = true;
    result.source_horizontal_inclusion_replayed = true;
    result.persistent_common_facet_target_propagation_replayed = true;
    result.commuting_horizontal_vertical_square_replayed = true;
    result.no_partial_scientific_payload_published_on_failure = false;
    result.decision = Decision::
        complete_conditional_normalized_common_facet_vertical_square;
    result.scope = ExactDirectNormalizedVerticalSquareScope::
        one_trusted_normalized_adjacent_order_closed_component_horizontal_step_only;
    if (!result.certified_conditional_vertical_square()) {
      return fail(
          std::move(result),
          Decision::no_square_commutation_contradiction);
    }
    return result;
  } catch (const std::bad_alloc&) {
    return fail(
        std::move(result), Decision::no_square_allocation_failed);
  }
}

ExactDirectNormalizedVerticalSquareVerification
verify_exact_direct_normalized_vertical_square_receipt(
    const ExactDirectNormalizedVerticalSourceComponentReceipt& before_source,
    const ExactDirectNormalizedVerticalTargetCarrierReceipt& before_target,
    const ExactDirectNormalizedVerticalSourceComponentReceipt& after_source,
    const ExactDirectNormalizedVerticalTargetCarrierReceipt& after_target,
    const ExactDirectNormalizedHorizontalCarrierInclusionReceipt& horizontal,
    const ExactDirectNormalizedVerticalSquareBudget& trusted_budget,
    const ExactDirectNormalizedVerticalSquareResult& observed) {
  ExactDirectNormalizedVerticalSquareVerification verification{};
  verification.observed_storage_within_budget =
      observed.component_images.size() <= 2U && observed.squares.size() <= 1U &&
      observed.counters.source_label_scan_count <=
          trusted_budget.maximum_source_label_scan_count &&
      observed.counters.source_tree_edge_scan_count <=
          trusted_budget.maximum_source_tree_edge_scan_count &&
      observed.counters.target_binding_scan_count <=
          trusted_budget.maximum_target_binding_scan_count &&
      observed.counters.key_point_scan_count <=
          trusted_budget.maximum_key_point_scan_count &&
      observed.counters.key_lookup_comparison_count <=
          trusted_budget.maximum_key_lookup_comparison_count &&
      observed.counters.source_subset_lookup_count <=
          trusted_budget.maximum_source_subset_lookup_count &&
      observed.counters.source_subset_comparison_count <=
          trusted_budget.maximum_source_subset_comparison_count &&
      observed.counters.logical_scratch_entry_count <=
          trusted_budget.maximum_tree_scratch_count &&
      observed.counters.exact_level_comparison_count <=
          trusted_budget.maximum_exact_level_comparison_count &&
      observed.counters.maximum_observed_exact_level_integer_bit_count <=
          trusted_budget.maximum_single_exact_level_integer_bit_count &&
      observed.counters.logical_output_entry_count <=
          trusted_budget.maximum_logical_output_entry_count;

  const Result expected =
      build_exact_direct_normalized_vertical_square_receipt(
          before_source,
          before_target,
          after_source,
          after_target,
          horizontal,
          trusted_budget);
  verification.expected_result_freshly_reconstructed = true;
  verification.trusted_authorities_accepted =
      expected.certified_conditional_vertical_square();
  verification.observed_recursively_equal = observed == expected;
  verification.unsupported_global_claims_remain_false =
      unsupported_claims_false(observed);
  verification.result_certified =
      verification.observed_storage_within_budget &&
      verification.expected_result_freshly_reconstructed &&
      verification.observed_recursively_equal &&
      verification.unsupported_global_claims_remain_false &&
      observed.certified_outcome();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
