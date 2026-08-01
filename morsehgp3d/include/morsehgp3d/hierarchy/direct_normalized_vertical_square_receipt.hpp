#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_normalized_vertical_square_receipt_schema_version = 1U;
inline constexpr std::string_view
    direct_normalized_vertical_square_receipt_backend = "reference_cpu";
inline constexpr std::string_view
    direct_normalized_vertical_square_receipt_profile = "hgp_reduced";
inline constexpr std::string_view direct_normalized_vertical_square_receipt_mode =
    "certified_conditional_normalized_common_facet_vertical_square";
inline constexpr std::string_view
    direct_normalized_vertical_square_receipt_public_status = "not_claimed";
inline constexpr std::string_view
    direct_normalized_vertical_square_receipt_proof_basis =
        "trusted_normalized_closed_component_spanning_tree_each_source_"
        "adjacency_exact_common_lower_order_facet_closed_target_carrier_"
        "agreement_persistent_common_facet_horizontal_propagation_and_one_"
        "commuting_square_v1";

// K_max is ten, so a normalized order-K source coface can contain K+1=11
// points.  This local receipt never builds a global facet or coface arena.
inline constexpr std::size_t
    direct_normalized_vertical_square_maximum_key_point_count = 11U;

struct ExactDirectNormalizedVerticalSimplexKey {
  std::array<
      spatial::PointId,
      direct_normalized_vertical_square_maximum_key_point_count>
      point_ids{};
  std::size_t point_count{};

  friend bool operator==(
      const ExactDirectNormalizedVerticalSimplexKey&,
      const ExactDirectNormalizedVerticalSimplexKey&) = default;
};

// Abstract, caller-owned horizontal source receipt for one non-trivial closed
// normalized H0 component.  labels is the exhaustive component membership at
// closed_squared_level.  spanning_tree_edges is a certified subset of source
// adjacencies and therefore has labels.size()-1 entries.
struct ExactDirectNormalizedVerticalSourceLabelReceipt {
  std::size_t label_reference_index{};
  ExactDirectNormalizedVerticalSimplexKey label_key{};
  exact::ExactLevel squared_birth_level{};

  friend bool operator==(
      const ExactDirectNormalizedVerticalSourceLabelReceipt&,
      const ExactDirectNormalizedVerticalSourceLabelReceipt&) = default;
};

struct ExactDirectNormalizedVerticalSourceTreeEdgeReceipt {
  std::size_t tree_edge_index{};
  std::size_t left_label_reference_index{};
  std::size_t right_label_reference_index{};
  ExactDirectNormalizedVerticalSimplexKey source_coface_key{};
  exact::ExactLevel source_coface_squared_level{};
  ExactDirectNormalizedVerticalSimplexKey common_target_facet_key{};

  friend bool operator==(
      const ExactDirectNormalizedVerticalSourceTreeEdgeReceipt&,
      const ExactDirectNormalizedVerticalSourceTreeEdgeReceipt&) = default;
};

struct ExactDirectNormalizedVerticalSourceComponentReceipt {
  std::uint64_t normalized_source_authority_id{};
  contract::CanonicalId canonical_cloud_digest{};
  std::size_t point_count{};
  std::size_t source_order{};
  exact::ExactLevel closed_squared_level{};
  std::uint64_t source_component_handle{};
  std::vector<ExactDirectNormalizedVerticalSourceLabelReceipt> labels;
  std::vector<ExactDirectNormalizedVerticalSourceTreeEdgeReceipt>
      spanning_tree_edges;

  bool normalized_source_freshly_verified{false};
  bool closed_component_membership_exhaustive{false};
  bool source_adjacencies_freshly_verified{false};
  bool source_label_and_coface_levels_exact{false};
  bool global_facet_coface_gamma_or_higher_delaunay_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactDirectNormalizedVerticalSourceComponentReceipt&,
      const ExactDirectNormalizedVerticalSourceComponentReceipt&) = default;
};

// Exact target-order carrier lookup at one closed cut.  Bindings are sorted by
// facet key and exhaustive for every facet requested by this local receipt.
struct ExactDirectNormalizedVerticalTargetFacetBindingReceipt {
  std::size_t binding_index{};
  ExactDirectNormalizedVerticalSimplexKey target_facet_key{};
  std::uint64_t closed_target_root_id{};

  friend bool operator==(
      const ExactDirectNormalizedVerticalTargetFacetBindingReceipt&,
      const ExactDirectNormalizedVerticalTargetFacetBindingReceipt&) =
      default;
};

struct ExactDirectNormalizedVerticalTargetCarrierReceipt {
  std::uint64_t target_carrier_authority_id{};
  contract::CanonicalId canonical_cloud_digest{};
  std::size_t point_count{};
  std::size_t target_order{};
  exact::ExactLevel closed_squared_level{};
  std::vector<ExactDirectNormalizedVerticalTargetFacetBindingReceipt>
      bindings;

  bool target_carrier_authority_freshly_verified{false};
  bool bindings_complete_for_requested_facets{false};
  bool roots_are_closed_at_exact_level{false};
  bool global_facet_coface_gamma_or_higher_delaunay_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactDirectNormalizedVerticalTargetCarrierReceipt&,
      const ExactDirectNormalizedVerticalTargetCarrierReceipt&) = default;
};

// This receipt binds the two component snapshots to one genuine horizontal
// successor relation.  all_intermediate_target_levels_accounted means that
// the supplied cuts are consecutive in the merged adjacent-order schedule,
// or that the trusted authority has already composed every omitted target-only
// step.  The local verifier never infers that fact from timings or IDs.
struct ExactDirectNormalizedHorizontalCarrierInclusionReceipt {
  std::uint64_t normalized_source_authority_id{};
  std::uint64_t inclusion_receipt_id{};
  std::uint64_t from_source_component_handle{};
  std::uint64_t to_source_component_handle{};
  exact::ExactLevel from_closed_squared_level{};
  exact::ExactLevel to_closed_squared_level{};

  bool normalized_horizontal_inclusion_freshly_verified{false};
  bool source_successor_unique{false};
  bool all_intermediate_target_levels_accounted{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactDirectNormalizedHorizontalCarrierInclusionReceipt&,
      const ExactDirectNormalizedHorizontalCarrierInclusionReceipt&) =
      default;
};

struct ExactDirectNormalizedVerticalSquareBudget {
  std::size_t maximum_source_label_scan_count{};
  std::size_t maximum_source_tree_edge_scan_count{};
  std::size_t maximum_target_binding_scan_count{};
  std::size_t maximum_key_point_scan_count{};
  std::size_t maximum_key_lookup_comparison_count{};
  std::size_t maximum_source_subset_lookup_count{};
  std::size_t maximum_source_subset_comparison_count{};
  std::size_t maximum_tree_scratch_count{};
  std::size_t maximum_exact_level_comparison_count{};
  std::size_t maximum_single_exact_level_integer_bit_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectNormalizedVerticalSquareBudget&,
      const ExactDirectNormalizedVerticalSquareBudget&) = default;
};

enum class ExactDirectNormalizedVerticalComponentRole : std::uint8_t {
  before_horizontal_step,
  after_horizontal_step,
};

struct ExactDirectNormalizedVerticalComponentImageReceipt {
  std::size_t component_image_index{};
  ExactDirectNormalizedVerticalComponentRole role{
      ExactDirectNormalizedVerticalComponentRole::before_horizontal_step};
  std::uint64_t source_component_handle{};
  exact::ExactLevel closed_squared_level{};
  std::size_t canonical_representative_label_reference_index{};
  std::size_t canonical_witness_tree_edge_index{};
  std::size_t canonical_witness_target_binding_index{};
  ExactDirectNormalizedVerticalSimplexKey
      canonical_common_target_facet_key{};
  std::uint64_t closed_target_root_id{};

  friend bool operator==(
      const ExactDirectNormalizedVerticalComponentImageReceipt&,
      const ExactDirectNormalizedVerticalComponentImageReceipt&) = default;
};

struct ExactDirectNormalizedVerticalCommutingSquareReceipt {
  std::size_t square_index{};
  std::size_t from_component_image_index{};
  std::size_t to_component_image_index{};
  std::uint64_t horizontal_inclusion_receipt_id{};
  ExactDirectNormalizedVerticalSimplexKey
      persistent_common_target_facet_key{};
  std::size_t persistent_target_binding_at_to_cut_index{};
  std::uint64_t from_closed_target_root_id{};
  std::uint64_t propagated_closed_target_root_id{};
  std::uint64_t to_closed_target_root_id{};

  friend bool operator==(
      const ExactDirectNormalizedVerticalCommutingSquareReceipt&,
      const ExactDirectNormalizedVerticalCommutingSquareReceipt&) = default;
};

struct ExactDirectNormalizedVerticalSquareCounters {
  std::size_t source_label_scan_count{};
  std::size_t source_tree_edge_scan_count{};
  std::size_t target_binding_scan_count{};
  std::size_t key_point_scan_count{};
  std::size_t key_lookup_comparison_count{};
  std::size_t source_subset_lookup_count{};
  std::size_t source_subset_comparison_count{};
  std::size_t tree_union_count{};
  std::size_t exact_level_comparison_count{};
  std::size_t maximum_observed_exact_level_integer_bit_count{};
  std::size_t logical_scratch_entry_count{};
  std::size_t logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectNormalizedVerticalSquareCounters&,
      const ExactDirectNormalizedVerticalSquareCounters&) = default;
};

enum class ExactDirectNormalizedVerticalSquareDecision : std::uint8_t {
  not_certified,
  no_square_capacity_overflow,
  no_square_budget_exhausted,
  no_square_allocation_failed,
  no_square_source_authority_rejected,
  no_square_target_authority_rejected,
  no_square_horizontal_authority_rejected,
  no_square_source_component_rejected,
  no_square_common_facet_contradiction,
  no_square_target_representative_conflict,
  no_square_horizontal_inclusion_contradiction,
  no_square_commutation_contradiction,
  complete_conditional_normalized_common_facet_vertical_square,
};

enum class ExactDirectNormalizedVerticalSquareScope : std::uint8_t {
  unspecified,
  one_trusted_normalized_adjacent_order_closed_component_horizontal_step_only,
};

struct ExactDirectNormalizedVerticalSquareResult {
  static constexpr std::string_view backend =
      direct_normalized_vertical_square_receipt_backend;
  static constexpr std::string_view profile =
      direct_normalized_vertical_square_receipt_profile;
  static constexpr std::string_view mode =
      direct_normalized_vertical_square_receipt_mode;
  static constexpr std::string_view public_status =
      direct_normalized_vertical_square_receipt_public_status;
  static constexpr std::string_view proof_basis =
      direct_normalized_vertical_square_receipt_proof_basis;

  std::uint32_t schema_version{
      direct_normalized_vertical_square_receipt_schema_version};
  ExactDirectNormalizedVerticalSquareBudget requested_budget{};
  std::size_t point_count{};
  std::size_t source_order{};
  std::size_t target_order{};
  std::uint64_t normalized_source_authority_id{};
  std::uint64_t target_carrier_authority_id{};
  contract::CanonicalId canonical_cloud_digest{};
  exact::ExactLevel from_closed_squared_level{};
  exact::ExactLevel to_closed_squared_level{};
  std::vector<ExactDirectNormalizedVerticalComponentImageReceipt>
      component_images;
  std::vector<ExactDirectNormalizedVerticalCommutingSquareReceipt> squares;
  ExactDirectNormalizedVerticalSquareCounters counters{};

  bool trusted_normalized_source_receipts_accepted{false};
  bool trusted_target_carrier_receipts_accepted{false};
  bool trusted_horizontal_inclusion_receipt_accepted{false};
  bool canonical_point_namespace_identity_certified{false};
  bool adjacent_orders_certified{false};
  bool exact_closed_levels_certified{false};
  bool source_component_memberships_replayed{false};
  bool source_spanning_trees_replayed{false};
  bool every_source_adjacency_has_exact_common_target_facet{false};
  bool every_common_target_facet_resolved_at_same_closed_cut{false};
  bool representative_independence_proved{false};
  bool source_horizontal_inclusion_replayed{false};
  bool persistent_common_facet_target_propagation_replayed{false};
  bool commuting_horizontal_vertical_square_replayed{false};
  bool no_partial_scientific_payload_published_on_failure{false};
  bool conditional_on_trusted_normalized_source_and_carrier_receipts{true};
  bool global_regularity_authority_certified{false};
  bool incidence_complete_reduction_proved{false};
  bool all_naturality_squares_replayed{false};
  bool vertical_maps_complete{false};
  bool global_facet_coface_gamma_or_higher_delaunay_materialized{false};
  bool public_status_claimed{false};
  ExactDirectNormalizedVerticalSquareDecision decision{
      ExactDirectNormalizedVerticalSquareDecision::not_certified};
  ExactDirectNormalizedVerticalSquareScope scope{
      ExactDirectNormalizedVerticalSquareScope::unspecified};

  [[nodiscard]] bool certified_conditional_vertical_square() const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectNormalizedVerticalSquareResult&,
      const ExactDirectNormalizedVerticalSquareResult&) = default;
};

[[nodiscard]] ExactDirectNormalizedVerticalSquareResult
build_exact_direct_normalized_vertical_square_receipt(
    const ExactDirectNormalizedVerticalSourceComponentReceipt& before_source,
    const ExactDirectNormalizedVerticalTargetCarrierReceipt& before_target,
    const ExactDirectNormalizedVerticalSourceComponentReceipt& after_source,
    const ExactDirectNormalizedVerticalTargetCarrierReceipt& after_target,
    const ExactDirectNormalizedHorizontalCarrierInclusionReceipt& horizontal,
    const ExactDirectNormalizedVerticalSquareBudget& budget);

struct ExactDirectNormalizedVerticalSquareVerification {
  bool trusted_authorities_accepted{false};
  bool observed_storage_within_budget{false};
  bool expected_result_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool unsupported_global_claims_remain_false{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectNormalizedVerticalSquareVerification&,
      const ExactDirectNormalizedVerticalSquareVerification&) = default;
};

[[nodiscard]] ExactDirectNormalizedVerticalSquareVerification
verify_exact_direct_normalized_vertical_square_receipt(
    const ExactDirectNormalizedVerticalSourceComponentReceipt& before_source,
    const ExactDirectNormalizedVerticalTargetCarrierReceipt& before_target,
    const ExactDirectNormalizedVerticalSourceComponentReceipt& after_source,
    const ExactDirectNormalizedVerticalTargetCarrierReceipt& after_target,
    const ExactDirectNormalizedHorizontalCarrierInclusionReceipt& horizontal,
    const ExactDirectNormalizedVerticalSquareBudget& trusted_budget,
    const ExactDirectNormalizedVerticalSquareResult& observed);

}  // namespace morsehgp3d::hierarchy
