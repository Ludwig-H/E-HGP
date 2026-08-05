#pragma once

#include "morsehgp3d/hierarchy/direct_morse_resident_all_orders_vertical_bridge.hpp"
#include "morsehgp3d/hierarchy/direct_morse_vertical_journal.hpp"
#include "morsehgp3d/hierarchy/direct_normalized_h0_incidence_reduction_authority.hpp"
#include "morsehgp3d/hierarchy/direct_normalized_h0_source_plan.hpp"
#include "morsehgp3d/hierarchy/direct_normalized_vertical_square_receipt.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_gateway_candidate_journal.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_gateway_clock.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_resident_naturality_square_campaign_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_resident_naturality_square_campaign_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_resident_naturality_square_campaign_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_resident_naturality_square_campaign_mode =
        "certified_conditional_every_adjacent_order_pair_every_consecutive_"
        "closed_cut_normalized_naturality_square_replay_campaign_v1";
inline constexpr std::string_view
    direct_morse_resident_naturality_square_campaign_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_morse_resident_naturality_square_campaign_proof_basis =
        "sealed_all_orders_vertical_bridge_v4_final_seal_cross_verified_"
        "against_live_bridge_live_normalized_incidence_complete_v7_source_"
        "capability_certified_candidate_source_plan_and_horizontal_incidence_"
        "reduction_authority_per_component_transient_membership_and_spanning_"
        "tree_derivation_released_after_use_five_trusted_receipts_per_square_"
        "fresh_build_and_fresh_verify_of_one_commuting_square_per_nontrivial_"
        "closed_component_per_adjacent_order_pair_per_consecutive_closed_cut_"
        "pair_with_exhaustive_coverage_cross_check_against_the_final_seal_v1";

// Every cap applies to one campaign run.  The campaign never materializes a
// global facet, coface, Gamma or Delaunay arena: per-component memberships,
// spanning trees and per-cut target bindings are derived incrementally into
// bounded transient scratch arenas that are cleared after each square.  The
// only retained scientific payload is one compact attestation per replayed
// square.  per_square_budget is the trusted budget handed unchanged to every
// inner build_exact_direct_normalized_vertical_square_receipt call and to its
// fresh verifier.
struct ExactDirectMorseResidentNaturalitySquareCampaignBudget {
  std::size_t maximum_adjacent_order_pair_count{};
  std::size_t maximum_consecutive_closed_cut_pair_count{};
  std::size_t maximum_replayed_square_count{};
  std::size_t maximum_retained_square_attestation_count{};
  std::size_t maximum_component_scan_count{};
  std::size_t maximum_component_label_count{};
  std::size_t maximum_component_tree_edge_count{};
  std::size_t maximum_target_binding_count{};
  std::size_t maximum_source_batch_scan_count{};
  std::size_t maximum_group_record_scan_count{};
  std::size_t maximum_component_state_scan_count{};
  std::size_t maximum_root_coverage_point_scan_count{};
  std::size_t maximum_root_witness_scan_count{};
  std::size_t maximum_source_plan_birth_reference_scan_count{};
  std::size_t maximum_source_plan_coface_scan_count{};
  std::size_t maximum_coface_reconstruction_count{};
  std::size_t maximum_key_point_scan_count{};
  std::size_t maximum_key_lookup_comparison_count{};
  std::size_t maximum_exact_level_comparison_count{};
  std::size_t maximum_single_exact_level_integer_bit_count{};
  std::size_t maximum_transient_scratch_entry_count{};
  std::size_t maximum_logical_output_entry_count{};
  ExactDirectNormalizedVerticalSquareBudget per_square_budget{};

  friend bool operator==(
      const ExactDirectMorseResidentNaturalitySquareCampaignBudget&,
      const ExactDirectMorseResidentNaturalitySquareCampaignBudget&) = default;
};

struct ExactDirectMorseResidentNaturalitySquareCampaignCounters {
  std::size_t adjacent_order_pair_count{};
  std::size_t consecutive_closed_cut_pair_count{};
  std::size_t replayed_square_count{};
  std::size_t retained_square_attestation_count{};
  std::size_t component_scan_count{};
  std::size_t singleton_component_skip_count{};
  std::size_t terminal_component_square_count{};
  std::size_t peak_component_label_count{};
  std::size_t peak_component_tree_edge_count{};
  std::size_t peak_target_binding_count{};
  std::size_t source_batch_scan_count{};
  std::size_t group_record_scan_count{};
  std::size_t component_state_scan_count{};
  std::size_t root_coverage_point_scan_count{};
  std::size_t root_witness_scan_count{};
  std::size_t source_plan_birth_reference_scan_count{};
  std::size_t source_plan_coface_scan_count{};
  std::size_t coface_reconstruction_count{};
  std::size_t key_point_scan_count{};
  std::size_t key_lookup_comparison_count{};
  std::size_t exact_level_comparison_count{};
  std::size_t maximum_observed_exact_level_integer_bit_count{};
  std::size_t peak_transient_scratch_entry_count{};
  std::size_t released_transient_arena_count{};
  std::size_t logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseResidentNaturalitySquareCampaignCounters&,
      const ExactDirectMorseResidentNaturalitySquareCampaignCounters&) =
      default;
};

enum class ExactDirectMorseResidentNaturalitySquareCampaignDecision
    : std::uint8_t {
  not_certified,
  no_campaign_capacity_overflow,
  no_campaign_budget_exhausted,
  no_campaign_allocation_failed,
  no_campaign_bridge_not_sealed,
  no_campaign_final_seal_mismatch,
  no_campaign_normalized_v7_source_capability_rejected,
  no_campaign_normalized_source_surface_rejected,
  no_campaign_point_namespace_identity_rejected,
  no_campaign_adjacent_order_schedule_rejected,
  no_campaign_component_membership_rejected,
  no_campaign_spanning_tree_rejected,
  no_campaign_target_carrier_rejected,
  no_campaign_horizontal_inclusion_rejected,
  no_campaign_square_receipt_rejected,
  no_campaign_square_verification_rejected,
  no_campaign_square_coverage_incomplete,
  complete_conditional_all_naturality_squares_replayed,
};

enum class ExactDirectMorseResidentNaturalitySquareCampaignScope
    : std::uint8_t {
  unspecified,
  conditional_on_sealed_bridge_and_normalized_source,
};

// Compact per-square attestation.  It retains no label list, no tree, no
// binding list and no receipt payload: the fresh campaign verifier re-derives
// the five receipts and rebuilds the square from the same sealed bridge and
// normalized source surface.  The persistent common target facet key is the
// only geometric datum retained and is fixed size.
struct ExactDirectMorseResidentNaturalitySquareAttestation {
  std::size_t square_attestation_index{};
  std::size_t source_order{};
  std::size_t target_order{};
  exact::ExactLevel from_closed_squared_level{};
  exact::ExactLevel to_closed_squared_level{};
  std::uint64_t before_source_component_handle{};
  std::uint64_t after_source_component_handle{};
  std::uint64_t horizontal_inclusion_receipt_id{};
  std::size_t before_component_label_count{};
  std::size_t after_component_label_count{};
  std::size_t before_spanning_tree_edge_count{};
  std::size_t after_spanning_tree_edge_count{};
  std::size_t before_target_binding_count{};
  std::size_t after_target_binding_count{};
  ExactDirectNormalizedVerticalSimplexKey
      persistent_common_target_facet_key{};
  std::uint64_t from_closed_target_root_id{};
  std::uint64_t propagated_closed_target_root_id{};
  std::uint64_t to_closed_target_root_id{};
  ExactDirectNormalizedVerticalSquareDecision square_decision{
      ExactDirectNormalizedVerticalSquareDecision::not_certified};
  bool square_receipt_freshly_built{false};
  bool square_receipt_freshly_verified{false};
  bool square_positively_certified{false};

  [[nodiscard]] bool certified_conditional_square_attestation() const noexcept;

  friend bool operator==(
      const ExactDirectMorseResidentNaturalitySquareAttestation&,
      const ExactDirectMorseResidentNaturalitySquareAttestation&) = default;
};

// Move-only conditional final authority for the naturality-square campaign.
// all_naturality_squares_replayed may become true ONLY here, ONLY under scope
// conditional_on_sealed_bridge_and_normalized_source, and public_status stays
// not_claimed.  The sealed bridge keeps that fact false in every batch record
// and asserts it only inside its own final seal; this result references the
// seal (sealed_bridge_stamp copy) instead of mutating any upstream receipt.
// Failure is atomic: attestations and counters are purged, facts reset, and
// no_partial_scientific_payload_published_on_failure is set.
struct ExactDirectMorseResidentNaturalitySquareCampaignResult {
  static constexpr std::string_view backend =
      direct_morse_resident_naturality_square_campaign_backend;
  static constexpr std::string_view profile =
      direct_morse_resident_naturality_square_campaign_profile;
  static constexpr std::string_view mode =
      direct_morse_resident_naturality_square_campaign_mode;
  static constexpr std::string_view public_status =
      direct_morse_resident_naturality_square_campaign_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_resident_naturality_square_campaign_proof_basis;

  ExactDirectMorseResidentNaturalitySquareCampaignResult() = default;
  ~ExactDirectMorseResidentNaturalitySquareCampaignResult() = default;
  ExactDirectMorseResidentNaturalitySquareCampaignResult(
      ExactDirectMorseResidentNaturalitySquareCampaignResult&&) noexcept =
      default;
  ExactDirectMorseResidentNaturalitySquareCampaignResult& operator=(
      ExactDirectMorseResidentNaturalitySquareCampaignResult&&) noexcept =
      default;
  ExactDirectMorseResidentNaturalitySquareCampaignResult(
      const ExactDirectMorseResidentNaturalitySquareCampaignResult&) = delete;
  ExactDirectMorseResidentNaturalitySquareCampaignResult& operator=(
      const ExactDirectMorseResidentNaturalitySquareCampaignResult&) = delete;

  std::uint32_t schema_version{
      direct_morse_resident_naturality_square_campaign_schema_version};
  ExactDirectMorseResidentNaturalitySquareCampaignBudget requested_budget{};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};
  std::uint64_t normalized_source_authority_id{};
  std::uint64_t target_carrier_authority_id{};
  contract::CanonicalId canonical_cloud_digest{};
  ExactDirectMorseResidentAllOrdersVerticalBridgeStamp sealed_bridge_stamp{};
  std::vector<ExactDirectMorseResidentNaturalitySquareAttestation>
      square_attestations;
  ExactDirectMorseResidentNaturalitySquareCampaignCounters counters{};
  std::optional<std::size_t> rejected_adjacent_order_pair_index;
  std::optional<std::size_t> rejected_consecutive_closed_cut_pair_index;
  std::optional<std::uint64_t> rejected_source_component_handle;
  std::optional<ExactDirectNormalizedVerticalSquareDecision>
      rejected_square_decision;

  bool sealed_bridge_live_verified{false};
  bool final_seal_cross_verified_against_live_bridge{false};
  bool normalized_v7_source_capability_live_required{false};
  bool normalized_source_surface_freshly_verified{false};
  bool canonical_point_namespace_identity_certified{false};
  bool exact_product_order_adjacent_schedule_reconstructed{false};
  bool every_component_membership_derived_incrementally_and_released{false};
  bool every_spanning_tree_derived_from_certified_source_adjacencies{false};
  bool every_target_carrier_binding_derived_from_sealed_bridge_images{false};
  bool every_horizontal_inclusion_bound_to_consecutive_closed_cuts{false};
  bool every_square_receipt_freshly_built{false};
  bool every_square_receipt_freshly_verified{false};
  bool every_singleton_component_skip_justified{false};
  bool terminal_component_square_replayed{false};
  bool all_naturality_squares_replayed{false};
  bool no_partial_scientific_payload_published_on_failure{false};
  bool conditional_on_sealed_bridge_and_normalized_source{true};
  bool external_target_authority_replayed{false};
  bool global_morse_obligation_replayed{false};
  bool full_pi0_membership_claimed{false};
  bool m1_replayed{false};
  bool vertical_maps_complete{false};
  bool global_facet_coface_gamma_or_higher_delaunay_materialized{false};
  bool pair_matrix_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseResidentNaturalitySquareCampaignDecision decision{
      ExactDirectMorseResidentNaturalitySquareCampaignDecision::not_certified};
  ExactDirectMorseResidentNaturalitySquareCampaignScope scope{
      ExactDirectMorseResidentNaturalitySquareCampaignScope::unspecified};

  [[nodiscard]] bool certified_conditional_all_naturality_squares_replayed()
      const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectMorseResidentNaturalitySquareCampaignResult&,
      const ExactDirectMorseResidentNaturalitySquareCampaignResult&) = default;
};

static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseResidentNaturalitySquareCampaignResult>);
static_assert(!std::is_copy_assignable_v<
              ExactDirectMorseResidentNaturalitySquareCampaignResult>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectMorseResidentNaturalitySquareCampaignResult>);
static_assert(std::is_nothrow_move_assignable_v<
              ExactDirectMorseResidentNaturalitySquareCampaignResult>);

// The sealed bridge is trusted and caller-frozen: the caller must not commit,
// seal or move it between build and verify.  trusted_final_seal is a caller
// copy cross-verified against the live bridge via verify_final_vertical_seal
// and verify_stamp.  The normalized source surface (candidate source plan,
// gateway journal, gateway identity, horizontal incidence reduction
// authority) supplies exhaustive per-component memberships and certified
// source adjacencies; the live capability
// sealed_bridge.normalized_incidence_complete_v7_source_capability() is
// required and never inferred from a copied seal record.
[[nodiscard]] ExactDirectMorseResidentNaturalitySquareCampaignResult
build_exact_direct_morse_resident_naturality_square_campaign(
    const ExactDirectMorseResidentAllOrdersVerticalBridge& sealed_bridge,
    const ExactDirectMorseResidentAllOrdersVerticalFinalSeal&
        trusted_final_seal,
    const ExactDirectNormalizedH0SourcePlanResult&
        trusted_normalized_source_plan,
    const ExactDirectSparseGatewayCandidateJournalResult&
        trusted_source_gateway,
    const ExactDirectSparseGatewayCandidateScientificIdentityResult&
        trusted_source_gateway_identity,
    const ExactDirectNormalizedH0IncidenceReductionAuthority&
        trusted_incidence_reduction_authority,
    const ExactDirectMorseResidentNaturalitySquareCampaignBudget& budget);

struct ExactDirectMorseResidentNaturalitySquareCampaignVerification {
  bool sealed_bridge_accepted{false};
  bool trusted_normalized_source_surface_accepted{false};
  bool observed_storage_within_budget{false};
  bool expected_result_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool unsupported_global_claims_remain_false{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseResidentNaturalitySquareCampaignVerification&,
      const ExactDirectMorseResidentNaturalitySquareCampaignVerification&) =
      default;
};

// Fresh verifier: rebuilds the complete campaign from the same sealed bridge
// and normalized source surface with the trusted budget, then requires
// recursive equality with the observed result.  A faithfully transported
// atomic failure is also verifiable; downstream consumers that need the
// positive fact must additionally require
// observed.certified_conditional_all_naturality_squares_replayed().
[[nodiscard]] ExactDirectMorseResidentNaturalitySquareCampaignVerification
verify_exact_direct_morse_resident_naturality_square_campaign(
    const ExactDirectMorseResidentAllOrdersVerticalBridge& sealed_bridge,
    const ExactDirectMorseResidentAllOrdersVerticalFinalSeal&
        trusted_final_seal,
    const ExactDirectNormalizedH0SourcePlanResult&
        trusted_normalized_source_plan,
    const ExactDirectSparseGatewayCandidateJournalResult&
        trusted_source_gateway,
    const ExactDirectSparseGatewayCandidateScientificIdentityResult&
        trusted_source_gateway_identity,
    const ExactDirectNormalizedH0IncidenceReductionAuthority&
        trusted_incidence_reduction_authority,
    const ExactDirectMorseResidentNaturalitySquareCampaignBudget&
        trusted_budget,
    const ExactDirectMorseResidentNaturalitySquareCampaignResult& observed);

// ---------------------------------------------------------------------------
// External target authority binding for ExactDirectMorseVerticalJournal.
//
// The sealed bridge's facet witnesses only NAME an external authority
// ({external_authority_id, replay_token}); no in-tree component ever sets
// external_target_authority_replayed=true, and
// certified_conditional_root_witness() requires it false.  This binding
// therefore certifies the NAMING only: it extracts the nonzero authority id
// sealed in terminal_locator_stamp, checks it against the live bridge, and
// exposes the exact ExactDirectMorseVerticalConfig expected by
// build_exact_direct_morse_vertical_journal.  The REPLAYING authority (the
// all-orders analogue of ExactDirectMorseK2K1TargetAuthorityResult, which
// re-derives every named record against an independent fresh oracle) is a
// separate follow-up component and is the only place that may ever raise
// external_target_authority_replayed.
// ---------------------------------------------------------------------------

inline constexpr std::uint32_t
    direct_morse_resident_vertical_external_target_authority_binding_schema_version =
        1U;
inline constexpr std::string_view
    direct_morse_resident_vertical_external_target_authority_binding_mode =
        "named_sealed_resident_locator_external_target_authority_binding_"
        "without_replay_v1";
inline constexpr std::string_view
    direct_morse_resident_vertical_external_target_authority_binding_public_status =
        "not_claimed";

enum class ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingDecision
    : std::uint8_t {
  not_certified,
  no_binding_bridge_not_sealed,
  no_binding_final_seal_mismatch,
  no_binding_authority_id_zero,
  no_binding_authority_id_mismatch,
  no_binding_replay_token_range_rejected,
  complete_named_external_target_authority_binding,
};

struct ExactDirectMorseResidentVerticalExternalTargetAuthorityBinding {
  std::uint32_t schema_version{
      direct_morse_resident_vertical_external_target_authority_binding_schema_version};
  std::uint64_t external_target_authority_id{};
  std::uint64_t resident_session_authority_id{};
  std::uint64_t resident_locator_instance_id{};
  std::size_t persistent_source_root_witness_count{};
  std::uint64_t final_root_witness_query_replay_token_begin{};
  std::uint64_t final_root_witness_query_replay_token_end{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp terminal_locator_stamp{};

  bool sealed_bridge_live_verified{false};
  bool final_seal_cross_verified_against_live_bridge{false};
  bool authority_id_nonzero{false};
  bool authority_id_matches_terminal_locator_stamp{false};
  bool replay_token_range_contiguous{false};
  bool external_target_authority_named_only{false};
  bool external_target_authority_replayed{false};
  bool all_naturality_squares_replayed{false};
  bool public_status_claimed{false};
  ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingDecision
      decision{
          ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingDecision::
              not_certified};

  [[nodiscard]] ExactDirectMorseVerticalConfig vertical_journal_config()
      const noexcept;
  [[nodiscard]] bool certified_named_external_target_authority_binding()
      const noexcept;

  friend bool operator==(
      const ExactDirectMorseResidentVerticalExternalTargetAuthorityBinding&,
      const ExactDirectMorseResidentVerticalExternalTargetAuthorityBinding&) =
      default;
};

[[nodiscard]] ExactDirectMorseResidentVerticalExternalTargetAuthorityBinding
build_exact_direct_morse_resident_vertical_external_target_authority_binding(
    const ExactDirectMorseResidentAllOrdersVerticalBridge& sealed_bridge,
    const ExactDirectMorseResidentAllOrdersVerticalFinalSeal&
        trusted_final_seal) noexcept;

struct
    ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingVerification {
  bool sealed_bridge_accepted{false};
  bool expected_binding_freshly_reconstructed{false};
  bool observed_recursively_equal{false};
  bool replay_never_claimed{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingVerification&,
      const ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingVerification&) =
      default;
};

[[nodiscard]]
ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingVerification
verify_exact_direct_morse_resident_vertical_external_target_authority_binding(
    const ExactDirectMorseResidentAllOrdersVerticalBridge& sealed_bridge,
    const ExactDirectMorseResidentAllOrdersVerticalFinalSeal&
        trusted_final_seal,
    const ExactDirectMorseResidentVerticalExternalTargetAuthorityBinding&
        observed) noexcept;

}  // namespace morsehgp3d::hierarchy