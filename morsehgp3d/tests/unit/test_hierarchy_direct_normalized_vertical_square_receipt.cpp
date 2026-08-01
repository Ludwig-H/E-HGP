#include "morsehgp3d/hierarchy/direct_normalized_vertical_square_receipt.hpp"

#include <array>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

namespace hierarchy = morsehgp3d::hierarchy;
namespace contract = morsehgp3d::contract;
namespace exact = morsehgp3d::exact;

using Key = hierarchy::ExactDirectNormalizedVerticalSimplexKey;

[[noreturn]] void fail(std::string_view message) {
  std::cerr << "direct normalized vertical square test failure: " << message
            << '\n';
  std::abort();
}

void require(bool condition, std::string_view message) {
  if (!condition) {
    fail(message);
  }
}

Key key(std::initializer_list<std::uint64_t> points) {
  Key result{};
  result.point_count = points.size();
  std::size_t index = 0U;
  for (const std::uint64_t point : points) {
    result.point_ids[index++] = point;
  }
  return result;
}

contract::CanonicalId cloud_digest() {
  std::array<std::uint8_t, contract::CanonicalId::byte_count> bytes{};
  bytes[0U] = 0x42U;
  return contract::CanonicalId{bytes};
}

hierarchy::ExactDirectNormalizedVerticalSourceComponentReceipt before_source() {
  hierarchy::ExactDirectNormalizedVerticalSourceComponentReceipt source{};
  source.normalized_source_authority_id = 71U;
  source.canonical_cloud_digest = cloud_digest();
  source.point_count = 5U;
  source.source_order = 3U;
  source.closed_squared_level = exact::ExactLevel{2};
  source.source_component_handle = 1001U;
  source.labels = {
      {.label_reference_index = 0U,
       .label_key = key({0U, 1U, 2U}),
       .squared_birth_level = exact::ExactLevel{1}},
      {.label_reference_index = 1U,
       .label_key = key({0U, 1U, 3U}),
       .squared_birth_level = exact::ExactLevel{1}},
  };
  source.spanning_tree_edges = {
      {.tree_edge_index = 0U,
       .left_label_reference_index = 0U,
       .right_label_reference_index = 1U,
       .source_coface_key = key({0U, 1U, 2U, 3U}),
       .source_coface_squared_level = exact::ExactLevel{2},
       .common_target_facet_key = key({0U, 1U})},
  };
  source.normalized_source_freshly_verified = true;
  source.closed_component_membership_exhaustive = true;
  source.source_adjacencies_freshly_verified = true;
  source.source_label_and_coface_levels_exact = true;
  return source;
}

hierarchy::ExactDirectNormalizedVerticalSourceComponentReceipt after_source() {
  hierarchy::ExactDirectNormalizedVerticalSourceComponentReceipt source{};
  source.normalized_source_authority_id = 71U;
  source.canonical_cloud_digest = cloud_digest();
  source.point_count = 5U;
  source.source_order = 3U;
  source.closed_squared_level = exact::ExactLevel{4};
  source.source_component_handle = 2001U;
  source.labels = {
      {.label_reference_index = 0U,
       .label_key = key({0U, 1U, 2U}),
       .squared_birth_level = exact::ExactLevel{1}},
      {.label_reference_index = 1U,
       .label_key = key({0U, 1U, 3U}),
       .squared_birth_level = exact::ExactLevel{1}},
      {.label_reference_index = 2U,
       .label_key = key({0U, 2U, 3U}),
       .squared_birth_level = exact::ExactLevel{2}},
  };
  // This tree intentionally does not use 01.  The old witness 01 is looked
  // up separately at the later cut, which is the actual commuting-square
  // check rather than a coincidental reuse by the new representative tree.
  source.spanning_tree_edges = {
      {.tree_edge_index = 0U,
       .left_label_reference_index = 0U,
       .right_label_reference_index = 2U,
       .source_coface_key = key({0U, 1U, 2U, 3U}),
       .source_coface_squared_level = exact::ExactLevel{2},
       .common_target_facet_key = key({0U, 2U})},
      {.tree_edge_index = 1U,
       .left_label_reference_index = 1U,
       .right_label_reference_index = 2U,
       .source_coface_key = key({0U, 1U, 2U, 3U}),
       .source_coface_squared_level = exact::ExactLevel{2},
       .common_target_facet_key = key({0U, 3U})},
  };
  source.normalized_source_freshly_verified = true;
  source.closed_component_membership_exhaustive = true;
  source.source_adjacencies_freshly_verified = true;
  source.source_label_and_coface_levels_exact = true;
  return source;
}

hierarchy::ExactDirectNormalizedVerticalTargetCarrierReceipt before_target() {
  hierarchy::ExactDirectNormalizedVerticalTargetCarrierReceipt target{};
  target.target_carrier_authority_id = 81U;
  target.canonical_cloud_digest = cloud_digest();
  target.point_count = 5U;
  target.target_order = 2U;
  target.closed_squared_level = exact::ExactLevel{2};
  target.bindings = {
      {.binding_index = 0U,
       .target_facet_key = key({0U, 1U}),
       .closed_target_root_id = 5001U},
  };
  target.target_carrier_authority_freshly_verified = true;
  target.bindings_complete_for_requested_facets = true;
  target.roots_are_closed_at_exact_level = true;
  return target;
}

hierarchy::ExactDirectNormalizedVerticalTargetCarrierReceipt after_target() {
  hierarchy::ExactDirectNormalizedVerticalTargetCarrierReceipt target{};
  target.target_carrier_authority_id = 81U;
  target.canonical_cloud_digest = cloud_digest();
  target.point_count = 5U;
  target.target_order = 2U;
  target.closed_squared_level = exact::ExactLevel{4};
  target.bindings = {
      {.binding_index = 0U,
       .target_facet_key = key({0U, 1U}),
       .closed_target_root_id = 6001U},
      {.binding_index = 1U,
       .target_facet_key = key({0U, 2U}),
       .closed_target_root_id = 6001U},
      {.binding_index = 2U,
       .target_facet_key = key({0U, 3U}),
       .closed_target_root_id = 6001U},
  };
  target.target_carrier_authority_freshly_verified = true;
  target.bindings_complete_for_requested_facets = true;
  target.roots_are_closed_at_exact_level = true;
  return target;
}

hierarchy::ExactDirectNormalizedHorizontalCarrierInclusionReceipt horizontal() {
  hierarchy::ExactDirectNormalizedHorizontalCarrierInclusionReceipt receipt{};
  receipt.normalized_source_authority_id = 71U;
  receipt.inclusion_receipt_id = 91U;
  receipt.from_source_component_handle = 1001U;
  receipt.to_source_component_handle = 2001U;
  receipt.from_closed_squared_level = exact::ExactLevel{2};
  receipt.to_closed_squared_level = exact::ExactLevel{4};
  receipt.normalized_horizontal_inclusion_freshly_verified = true;
  receipt.source_successor_unique = true;
  receipt.all_intermediate_target_levels_accounted = true;
  return receipt;
}

hierarchy::ExactDirectNormalizedVerticalSquareBudget budget() {
  return {
      .maximum_source_label_scan_count = 5U,
      .maximum_source_tree_edge_scan_count = 3U,
      .maximum_target_binding_scan_count = 4U,
      .maximum_key_point_scan_count = 128U,
      .maximum_key_lookup_comparison_count = 32U,
      .maximum_source_subset_lookup_count = 2U,
      .maximum_source_subset_comparison_count = 8U,
      .maximum_tree_scratch_count = 3U,
      .maximum_exact_level_comparison_count = 32U,
      .maximum_single_exact_level_integer_bit_count = 64U,
      .maximum_logical_output_entry_count = 3U,
  };
}

}  // namespace

int main() {
  const auto source_before = before_source();
  const auto target_before = before_target();
  const auto source_after = after_source();
  const auto target_after = after_target();
  const auto horizontal_receipt = horizontal();
  const auto trusted_budget = budget();

  const auto positive =
      hierarchy::build_exact_direct_normalized_vertical_square_receipt(
          source_before,
          target_before,
          source_after,
          target_after,
          horizontal_receipt,
          trusted_budget);
  require(
      positive.certified_conditional_vertical_square(),
      "positive receipt must certify the local vertical square");
  require(
      positive.component_images.size() == 2U && positive.squares.size() == 1U,
      "positive payload shape");
  require(
      positive.component_images[0U].closed_target_root_id == 5001U &&
          positive.component_images[1U].closed_target_root_id == 6001U,
      "component images must use the closed target roots");
  require(
      positive.squares[0U].persistent_common_target_facet_key ==
          key({0U, 1U}) &&
          positive.squares[0U].propagated_closed_target_root_id == 6001U,
      "the old common facet must witness target propagation");
  require(
      !positive.global_regularity_authority_certified &&
          !positive.incidence_complete_reduction_proved &&
          !positive.all_naturality_squares_replayed &&
          !positive.vertical_maps_complete &&
          !positive.public_status_claimed,
      "unsupported global claims must remain false");

  const auto verification =
      hierarchy::verify_exact_direct_normalized_vertical_square_receipt(
          source_before,
          target_before,
          source_after,
          target_after,
          horizontal_receipt,
          trusted_budget,
          positive);
  require(verification.result_certified, "fresh verifier must accept positive");

  auto representative_conflict_target = target_after;
  representative_conflict_target.bindings[2U].closed_target_root_id = 6002U;
  const auto representative_conflict =
      hierarchy::build_exact_direct_normalized_vertical_square_receipt(
          source_before,
          target_before,
          source_after,
          representative_conflict_target,
          horizontal_receipt,
          trusted_budget);
  require(
      representative_conflict.decision ==
              hierarchy::ExactDirectNormalizedVerticalSquareDecision::
                  no_square_target_representative_conflict &&
          representative_conflict.certified_atomic_failure(),
      "two target roots in one source spanning tree must fail atomically");

  auto non_commuting_target = target_after;
  non_commuting_target.bindings[0U].closed_target_root_id = 6002U;
  const auto non_commuting =
      hierarchy::build_exact_direct_normalized_vertical_square_receipt(
          source_before,
          target_before,
          source_after,
          non_commuting_target,
          horizontal_receipt,
          trusted_budget);
  require(
      non_commuting.decision ==
              hierarchy::ExactDirectNormalizedVerticalSquareDecision::
                  no_square_commutation_contradiction &&
          non_commuting.certified_atomic_failure(),
      "a persistent facet reaching another later root must reject the square");

  auto forged_common_facet_source = source_before;
  forged_common_facet_source.spanning_tree_edges[0U]
      .common_target_facet_key = key({0U, 2U});
  const auto forged_common_facet =
      hierarchy::build_exact_direct_normalized_vertical_square_receipt(
          forged_common_facet_source,
          target_before,
          source_after,
          target_after,
          horizontal_receipt,
          trusted_budget);
  require(
      forged_common_facet.decision ==
              hierarchy::ExactDirectNormalizedVerticalSquareDecision::
                  no_square_common_facet_contradiction &&
          forged_common_facet.certified_atomic_failure(),
      "a forged common facet must fail before target lookup");

  auto insufficient_budget = trusted_budget;
  --insufficient_budget.maximum_source_label_scan_count;
  const auto exhausted =
      hierarchy::build_exact_direct_normalized_vertical_square_receipt(
          source_before,
          target_before,
          source_after,
          target_after,
          horizontal_receipt,
          insufficient_budget);
  require(
      exhausted.decision ==
              hierarchy::ExactDirectNormalizedVerticalSquareDecision::
                  no_square_budget_exhausted &&
          exhausted.certified_atomic_failure(),
      "cap minus one must fail without a scientific prefix");

  auto mutated_observed = positive;
  mutated_observed.squares[0U].propagated_closed_target_root_id = 9999U;
  const auto rejected_mutation =
      hierarchy::verify_exact_direct_normalized_vertical_square_receipt(
          source_before,
          target_before,
          source_after,
          target_after,
          horizontal_receipt,
          trusted_budget,
          mutated_observed);
  require(
      !rejected_mutation.result_certified,
      "fresh verifier must reject a mutated square");

  return 0;
}
