#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/hierarchy/critical_catalog_typed_gamma_arm_root_path_overlay.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace morsehgp3d::hierarchy {

// The nested 6.21 budget remains authoritative.  The four local capacities
// bound only canonical event projections, durable arm tuples and their compact
// event-local aggregation; no path witness is copied.
struct ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget {
  static constexpr std::size_t maximum_supported_event_key_record_count =
      1456U;
  static constexpr std::size_t maximum_supported_arm_key_record_count =
      5824U;
  static constexpr std::size_t
      maximum_supported_event_arm_key_reference_count = 5824U;
  static constexpr std::size_t
      maximum_supported_event_projection_point_id_reference_count = 21840U;

  ExactCriticalCatalogTypedGammaArmRootPathOverlayBudget path_overlay_budget{};
  std::size_t maximum_event_key_record_count{};
  std::size_t maximum_arm_key_record_count{};
  std::size_t maximum_event_arm_key_reference_count{};
  std::size_t maximum_event_projection_point_id_reference_count{};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget&,
      const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget&) =
      default;
};

enum class ExactCriticalCatalogTypedGammaDurableArmKeyCatalogDecision :
    std::uint8_t {
  not_certified,
  no_durable_arm_key_catalog_external_budget_seam_mismatch,
  no_durable_arm_key_catalog_preflight_budget_insufficient,
  no_durable_arm_key_catalog_source_path_overlay_rejected,
  no_durable_arm_key_catalog_source_path_overlay_incomplete,
  complete_exhaustive_single_order_durable_arm_key_catalog,
};

enum class ExactCriticalCatalogTypedGammaDurableArmKeyCatalogScope :
    std::uint8_t {
  unspecified,
  bounded_n14_k10_single_order_v2_critical_event_ids_and_canonical_arm_identity_tuples_from_recertified_internal_replayable_paths_only,
};

// This is exactly the schema-version-free scientific projection hashed by a
// public v2 CriticalEvent.event_id.  It is retained so equal hashes can still
// be compared by complete semantics and any collision fails closed.
struct ExactCriticalEventV2IdentityProjection {
  std::vector<spatial::PointId> interior_point_ids;
  std::vector<spatial::PointId> shell_point_ids;
  std::vector<spatial::PointId> minimal_support_point_ids;
  exact::ExactCenter3 center_witness_homogeneous;
  exact::ExactLevel squared_level_exact;

  friend bool operator==(
      const ExactCriticalEventV2IdentityProjection&,
      const ExactCriticalEventV2IdentityProjection&) = default;
};

// This tuple is the stable semantic key of one future attachment, but it is
// intentionally not hashed in the Attachment domain at this milestone.  This
// keeps that public type domain reserved for a later complete Attachment
// certification while retaining its exact future identity projection here.
struct ExactCriticalArmDurableKey {
  contract::CanonicalId event_id;
  std::size_t order{};
  spatial::PointId removed_shell_point_id{};

  friend bool operator==(
      const ExactCriticalArmDurableKey&,
      const ExactCriticalArmDurableKey&) = default;
  friend std::strong_ordering operator<=>(
      const ExactCriticalArmDurableKey&,
      const ExactCriticalArmDurableKey&) = default;
};

struct ExactCriticalCatalogTypedGammaDurableEventKeyRecord {
  std::size_t event_key_record_index{};
  std::size_t source_catalog_event_index{};
  contract::CanonicalId event_id;
  ExactCriticalEventV2IdentityProjection identity_projection;
  std::vector<std::size_t> arm_key_record_indices;

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaDurableEventKeyRecord&,
      const ExactCriticalCatalogTypedGammaDurableEventKeyRecord&) = default;
};

struct ExactCriticalCatalogTypedGammaDurableArmKeyRecord {
  std::size_t arm_key_record_index{};
  ExactCriticalArmDurableKey durable_key;
  std::size_t event_key_record_index{};
  std::size_t source_path_record_index{};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaDurableArmKeyRecord&,
      const ExactCriticalCatalogTypedGammaDurableArmKeyRecord&) = default;
};

struct ExactCriticalCatalogTypedGammaDurableArmKeyCatalogCounters {
  std::size_t preflight_count{};
  std::size_t source_path_overlay_verification_count{};
  std::size_t critical_catalog_build_count{};
  std::size_t saddle_event_reconciliation_count{};
  std::size_t event_projection_count{};
  std::size_t event_id_hash_count{};
  std::size_t event_hash_semantic_comparison_count{};
  std::size_t arm_path_key_reconciliation_count{};
  std::size_t event_key_record_count{};
  std::size_t arm_key_record_count{};
  std::size_t event_arm_key_reference_count{};
  std::size_t event_projection_point_id_reference_count{};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogCounters&,
      const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogCounters&) =
      default;
};

struct ExactCriticalCatalogTypedGammaDurableArmKeyCatalogResult {
  static constexpr std::size_t minimum_supported_point_count = 3U;
  static constexpr std::size_t maximum_supported_point_count = 14U;
  static constexpr std::size_t minimum_supported_order = 2U;
  static constexpr std::size_t maximum_supported_order = 10U;
  static constexpr const char* proof_basis =
      "v2_domain_separated_sha256_critical_event_keys_with_full_"
      "projection_collision_checks_and_exhaustive_single_order_arm_"
      "identity_tuple_catalog_v1";

  ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget requested_budget{};
  std::size_t point_count{};
  std::size_t order{};
  std::size_t critical_event_support_bound{};
  std::size_t critical_arm_bound{};
  std::size_t required_event_key_record_capacity{};
  std::size_t required_arm_key_record_capacity{};
  std::size_t required_event_arm_key_reference_capacity{};
  std::size_t required_event_projection_point_id_reference_capacity{};
  ExactCriticalCatalogTypedGammaArmRootPathOverlayDecision
      source_path_overlay_decision{
          ExactCriticalCatalogTypedGammaArmRootPathOverlayDecision::
              not_certified};
  std::vector<ExactCriticalCatalogTypedGammaDurableEventKeyRecord>
      event_key_records;
  std::vector<ExactCriticalCatalogTypedGammaDurableArmKeyRecord>
      arm_key_records;
  bool durable_key_conservative_preflight_bounds_certified{false};
  bool durable_key_preflight_budget_sufficient{false};
  bool all_four_external_budget_seams_certified{false};
  bool source_path_overlay_is_external_and_not_retained{false};
  bool source_path_overlay_fresh_replay_certified{false};
  bool reconstruction_started_only_after_complete_source_path_overlay{false};
  bool transient_critical_catalog_fresh_replay_certified{false};
  bool event_identity_projections_are_complete_schema_version_free_v2_keys{
      false};
  bool critical_event_ids_are_domain_separated_sha256_v2{false};
  bool event_hash_collisions_checked_against_complete_projections{false};
  bool every_requested_order_saddle_has_one_durable_event_key{false};
  bool every_complete_shell_has_exactly_one_sorted_durable_arm_tuple_per_point{
      false};
  bool arm_tuples_biject_replayable_source_paths{false};
  bool event_to_arm_aggregation_is_complete_and_canonical{false};
  bool identities_exclude_paths_targets_reduced_roots_and_local_indices{false};
  bool records_are_internal_keys_and_not_public_attachments_or_equal_level_batches{
      false};
  bool diagnostic_outcomes_have_no_key_payload{false};
  bool critical_catalog_typed_gamma_durable_arm_key_catalog_certified{false};
  ExactCriticalCatalogTypedGammaDurableArmKeyCatalogCounters counters{};
  ExactCriticalCatalogTypedGammaDurableArmKeyCatalogDecision decision{
      ExactCriticalCatalogTypedGammaDurableArmKeyCatalogDecision::
          not_certified};
  ExactCriticalCatalogTypedGammaDurableArmKeyCatalogScope scope{
      ExactCriticalCatalogTypedGammaDurableArmKeyCatalogScope::unspecified};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogResult&,
      const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogResult&) =
      default;
};

struct ExactCriticalCatalogTypedGammaDurableArmKeyCatalogVerification {
  bool requested_budget_certified{false};
  bool external_inputs_certified{false};
  bool derived_preflight_sizes_certified{false};
  bool source_path_overlay_decision_certified{false};
  bool source_path_overlay_fresh_replay_certified{false};
  bool event_key_records_certified{false};
  bool arm_key_records_certified{false};
  bool result_facts_certified{false};
  bool counters_certified{false};
  bool decision_certified{false};
  bool scope_certified{false};
  bool fresh_replay_certified{false};
  bool exact_critical_catalog_typed_gamma_durable_arm_key_catalog_decision_certified{
      false};

  friend bool operator==(
      const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogVerification&,
      const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogVerification&) =
      default;
};

void validate_exact_critical_catalog_typed_gamma_durable_arm_key_catalog_budget_caps(
    const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget& budget);

[[nodiscard]] ExactCriticalCatalogTypedGammaDurableArmKeyCatalogResult
build_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget budget,
    const ExactCriticalCatalogTypedGammaJournalResult& source_journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& source_root_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionResult&
        source_composition,
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayResult&
        source_path_overlay);

// Every external layer and every observed key is untrusted.  Verification
// reconstructs the complete result from the canonical cloud and trusted
// budget; no observed digest, tuple or local index steers that replay.
[[nodiscard]] ExactCriticalCatalogTypedGammaDurableArmKeyCatalogVerification
verify_exact_critical_catalog_typed_gamma_durable_arm_key_catalog(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t order,
    ExactCriticalCatalogTypedGammaDurableArmKeyCatalogBudget budget,
    const ExactCriticalCatalogTypedGammaJournalResult& source_journal,
    const ExactCriticalCatalogTypedGammaRootOverlayResult& source_root_overlay,
    const ExactCriticalCatalogTypedGammaArmRootCompositionResult&
        source_composition,
    const ExactCriticalCatalogTypedGammaArmRootPathOverlayResult&
        source_path_overlay,
    const ExactCriticalCatalogTypedGammaDurableArmKeyCatalogResult& result);

}  // namespace morsehgp3d::hierarchy
