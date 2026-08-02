#pragma once

#include "morsehgp3d/hierarchy/direct_morse_resident_k2_k1_closed_cut_bridge.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_resident_all_orders_vertical_bridge_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_resident_all_orders_vertical_bridge_backend =
        "reference_cpu";
inline constexpr std::string_view
    direct_morse_resident_all_orders_vertical_bridge_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_resident_all_orders_vertical_bridge_mode =
        "certified_conditional_inductive_resident_all_adjacent_orders_"
        "vertical_group_images_v1";
inline constexpr std::string_view
    direct_morse_resident_all_orders_vertical_bridge_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_morse_resident_all_orders_vertical_bridge_proof_basis =
        "owned_k2_k1_bridge_private_move_only_outer_ticket_exact_product_"
        "order_prebatch_locator_reprobe_one_persistent_canonical_target_"
        "facet_witness_per_source_root_all_nonroot_source_key_codimension_"
        "one_deletions_live_root_agreement_terminal_exhausted_locator_"
        "witness_sweep_composes_monotone_target_unions_owned_normalized_"
        "source_capability_and_preallocated_post_resident_commit_"
        "publication_v1";

// Every cap applies to one live all-orders bridge.  Target deletions are
// generated in a fixed ExactDirectSparseFacetKey scratch object and are never
// retained globally.  The only persistent vertical geometric payload is one
// target-facet witness per source root plus one compact record per committed
// order-k batch (k >= 3).
struct ExactDirectMorseResidentAllOrdersVerticalBridgeBudget {
  std::size_t maximum_committed_higher_batch_count{};
  std::size_t maximum_committed_higher_group_count{};
  std::size_t maximum_prepared_higher_group_count{};
  std::size_t maximum_persistent_source_root_witness_count{};
  std::size_t maximum_prior_root_witness_probe_count{};
  std::size_t maximum_final_root_witness_probe_count{};
  std::size_t maximum_source_facet_resolution_scan_count{};
  std::size_t maximum_projected_target_facet_probe_count{};
  std::size_t maximum_expected_group_child_root_reference_count{};
  std::size_t maximum_expected_group_coverage_delta_point_reference_count{};
  std::uint64_t maximum_query_replay_token{};
  ExactDirectSparsePositiveFacetProbeBudget target_probe{};

  friend bool operator==(
      const ExactDirectMorseResidentAllOrdersVerticalBridgeBudget&,
      const ExactDirectMorseResidentAllOrdersVerticalBridgeBudget&) = default;
};

struct ExactDirectMorseResidentAllOrdersVerticalBridgeRequirements {
  std::size_t resident_batch_count{};
  std::size_t resident_k2_batch_count{};
  std::size_t resident_higher_batch_count{};
  std::size_t prepared_higher_group_count{};
  std::size_t persistent_source_root_witness_count_before{};
  std::size_t persistent_source_root_witness_count_after{};
  std::size_t prior_root_witness_probe_count{};
  std::size_t source_facet_resolution_scan_count{};
  std::size_t projected_target_facet_probe_count{};
  std::size_t expected_group_child_root_reference_count{};
  std::size_t expected_group_coverage_delta_point_reference_count{};

  friend bool operator==(
      const ExactDirectMorseResidentAllOrdersVerticalBridgeRequirements&,
      const ExactDirectMorseResidentAllOrdersVerticalBridgeRequirements&) =
      default;
};

struct ExactDirectMorseResidentAllOrdersVerticalBridgeStamp {
  std::uint32_t schema_version{
      direct_morse_resident_all_orders_vertical_bridge_schema_version};
  std::uint64_t bridge_session_instance_id{};
  std::uint64_t owned_k2_k1_bridge_session_instance_id{};
  std::uint64_t resident_session_authority_id{};
  std::uint64_t resident_locator_instance_id{};
  std::size_t resident_batch_cursor{};
  std::size_t resident_epoch{};
  std::size_t committed_k2_batch_count{};
  std::size_t committed_k2_group_count{};
  std::size_t committed_higher_batch_count{};
  std::size_t committed_higher_group_count{};
  std::size_t persistent_source_root_witness_count{};
  std::uint64_t next_query_replay_token{};
  contract::CanonicalId canonical_cloud_digest{};
  contract::CanonicalId resident_higher_canonical_cloud_digest{};

  friend bool operator==(
      const ExactDirectMorseResidentAllOrdersVerticalBridgeStamp&,
      const ExactDirectMorseResidentAllOrdersVerticalBridgeStamp&) = default;
};

// The binding witness is the live locator's original binding witness for the
// retained target key.  It is never accepted as standalone authority: every
// later source-root use re-probes the complete key and requires the same
// binding witness in the exact current locator snapshot.
struct ExactDirectMorseResidentAllOrdersVerticalRootWitness {
  ExactFrozenIncidencePriorRootId source_root_id{};
  std::size_t source_order{};
  std::size_t target_order{};
  ExactDirectSparseFacetKey canonical_target_facet_key{};
  ExactDirectSparseFacetWitness target_source_binding_witness{};
  ExactFrozenIncidencePriorRootId last_resolved_target_root_id{};
  std::size_t first_source_batch_index{};
  std::size_t last_verified_source_batch_index{};
  std::size_t live_reprobe_count{};
  bool canonical_target_facet_selected{false};
  bool source_binding_witness_live_reprobed{false};
  bool target_root_was_live_and_rooted{false};
  bool external_target_authority_replayed{false};

  [[nodiscard]] bool certified_conditional_root_witness() const noexcept;

  friend bool operator==(
      const ExactDirectMorseResidentAllOrdersVerticalRootWitness&,
      const ExactDirectMorseResidentAllOrdersVerticalRootWitness&) = default;
};

struct ExactDirectMorseResidentAllOrdersVerticalGroupImage {
  std::size_t owner_group_index{};
  std::size_t q_r{};
  ExactFrozenIncidenceHgpAction action{
      ExactFrozenIncidenceHgpAction::reduced_birth};
  std::size_t prior_root_witness_probe_count{};
  std::size_t nonroot_source_facet_resolution_count{};
  std::size_t projected_target_facet_probe_count{};
  ExactFrozenIncidencePriorRootId resident_resultant_source_root_id{};
  ExactFrozenIncidencePriorRootId resolved_target_root_id{};
  ExactDirectSparseFacetKey canonical_target_facet_key{};
  ExactDirectSparseFacetWitness target_source_binding_witness{};
  bool every_prior_source_root_witness_present_and_live_reprobed{false};
  bool every_latent_or_equal_source_key_projected_to_all_deletions{false};
  bool every_target_deletion_live_positive_and_rooted{false};
  bool one_live_target_root_for_complete_group{false};
  bool resultant_source_root_bound_after_resident_commit{false};

  [[nodiscard]] bool certified_conditional_group_image() const noexcept;

  friend bool operator==(
      const ExactDirectMorseResidentAllOrdersVerticalGroupImage&,
      const ExactDirectMorseResidentAllOrdersVerticalGroupImage&) = default;
};

struct ExactDirectMorseResidentAllOrdersVerticalBatchRecord {
  std::uint32_t schema_version{
      direct_morse_resident_all_orders_vertical_bridge_schema_version};
  ExactDirectMorseUnifiedSnapshotIdentity resident_pre_batch_identity{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp
      live_post_resident_locator_stamp{};
  std::size_t source_batch_index{};
  exact::ExactLevel squared_level{};
  std::size_t source_order{};
  std::size_t target_order{};
  std::size_t prior_root_witness_probe_count{};
  std::size_t source_facet_resolution_scan_count{};
  std::size_t projected_target_facet_probe_count{};
  std::vector<ExactDirectMorseResidentAllOrdersVerticalGroupImage>
      group_images;
  bool exact_product_order_target_batch_already_committed{false};
  bool pre_batch_locator_snapshot_immutable_during_all_probes{false};
  bool every_higher_group_has_one_inductive_target_image{false};
  bool actual_resident_group_suffix_compared_field_by_field{false};
  bool resident_batch_committed{false};
  bool post_resident_commit_publication_allocation_free{false};
  bool complete_campaign_exhaustion_observed{false};
  bool all_naturality_squares_replayed{false};
  bool vertical_maps_complete{false};
  bool global_morse_obligation_replayed{false};
  bool global_facet_coface_or_gamma_catalog_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool pair_matrix_materialized{false};
  bool public_status_claimed{false};

  [[nodiscard]] bool certified_conditional_higher_batch() const noexcept;

  friend bool operator==(
      const ExactDirectMorseResidentAllOrdersVerticalBatchRecord&,
      const ExactDirectMorseResidentAllOrdersVerticalBatchRecord&) = default;
};

struct ExactDirectMorseResidentAllOrdersVerticalFinalSeal {
  std::uint32_t schema_version{
      direct_morse_resident_all_orders_vertical_bridge_schema_version};
  ExactDirectMorseResidentAllOrdersVerticalBridgeStamp sealed_stamp{};
  std::size_t required_resident_batch_count{};
  std::size_t required_k2_batch_count{};
  std::size_t required_higher_batch_count{};
  std::size_t required_resident_group_count{};
  std::size_t sealed_k2_group_count{};
  std::size_t sealed_higher_group_count{};
  std::size_t sealed_nonroot_source_facet_resolution_count{};
  std::size_t sealed_expected_projected_target_facet_probe_count{};
  std::size_t sealed_projected_target_facet_probe_count{};
  std::size_t sealed_persistent_source_root_witness_count{};
  std::size_t sealed_final_root_witness_probe_count{};
  std::size_t sealed_terminal_locator_union_count{};
  std::size_t sealed_terminal_locator_batch_count{};
  std::uint64_t final_root_witness_query_replay_token_begin{};
  std::uint64_t final_root_witness_query_replay_token_end{};
  std::uint64_t terminal_k1_session_instance_id{};
  std::size_t pre_terminal_k1_level_cursor{};
  std::size_t terminal_k1_level_cursor{};
  std::size_t distinct_k1_level_count{};
  std::size_t sealed_target_only_k1_level_count{};
  contract::CanonicalId k1_source_forest_digest{};
  contract::CanonicalId terminal_k1_history_digest{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp terminal_locator_stamp{};
  ExactDirectMorseUnifiedResidentSourceKind resident_source_kind{
      ExactDirectMorseUnifiedResidentSourceKind::successive_incidence_star};
  ExactDirectNormalizedH0ResidentRetractionMode
      resident_normalized_h0_retraction_mode{
          ExactDirectNormalizedH0ResidentRetractionMode::
              not_applicable_successive_incidence_star};
  bool resident_normalized_direct_source_session{false};
  bool resident_normalized_horizontal_incidence_reduction_certified{false};
  bool normalized_incidence_complete_v7_source_capability{false};
  bool owned_k1_terminal_advance_receipt_live_verified{false};
  bool every_target_only_k1_level_consumed{false};
  bool owned_k1_terminal_cursor_complete{false};
  bool resident_cursor_exhausted{false};
  bool no_outstanding_prepared_ticket{false};
  bool every_required_plan_batch_replayed_in_exact_product_order{false};
  bool every_k2_batch_and_group_replayed_to_sealed_k1{false};
  bool every_higher_group_bound_to_one_live_adjacent_order_root{false};
  bool every_nonroot_source_key_projected_to_all_codimension_one_deletions{
      false};
  bool every_resident_group_covered_exactly_once{false};
  bool every_persistent_target_binding_reprobed_at_terminal_locator{false};
  bool terminal_locator_monotone_union_history_owned_and_exhausted{false};
  bool every_intermediate_target_fusion_composed_by_terminal_root_reprobe{
      false};
  bool all_naturality_squares_replayed{false};
  bool vertical_maps_complete{false};
  bool global_morse_obligation_replayed{false};
  bool m1_replayed{false};
  bool global_facet_coface_or_gamma_catalog_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool pair_matrix_materialized{false};
  bool public_status_claimed{false};

  [[nodiscard]] bool certified_final_vertical_seal() const noexcept;

  friend bool operator==(
      const ExactDirectMorseResidentAllOrdersVerticalFinalSeal&,
      const ExactDirectMorseResidentAllOrdersVerticalFinalSeal&) = default;
};

enum class ExactDirectMorseResidentAllOrdersVerticalInitializationDecision
    : std::uint8_t {
  not_certified,
  no_owned_k2_k1_bridge_rejected,
  no_initial_resident_cursor_required,
  no_budget_rejected,
  no_session_instance_id_exhausted,
  no_allocation_failed,
  complete_certified_bounded_conditional_bridge,
};

enum class ExactDirectMorseResidentAllOrdersVerticalPreparationDecision
    : std::uint8_t {
  not_certified,
  no_bridge_not_ready,
  no_owned_bridge_preparation_rejected,
  no_resident_pre_batch_identity_rejected,
  no_preparation_budget_exhausted,
  no_prior_source_root_witness,
  no_target_facet_locator_probe_rejected,
  no_target_component_not_rooted,
  no_group_target_root_conflict,
  no_frozen_group_shape_rejected,
  no_allocation_failed,
  complete_prepared_nonhigher_transit_batch,
  complete_prepared_conditional_higher_vertical_batch,
};

enum class ExactDirectMorseResidentAllOrdersVerticalCommitDecision
    : std::uint8_t {
  not_certified,
  no_ticket_already_consumed,
  no_foreign_ticket_rejected,
  no_owned_bridge_commit_rejected_without_outer_vertical_mutation,
  complete_committed_nonhigher_transit_batch,
  complete_committed_conditional_higher_vertical_batch,
};

enum class ExactDirectMorseResidentAllOrdersVerticalSealDecision
    : std::uint8_t {
  not_certified,
  no_bridge_not_ready,
  no_resident_not_exhausted,
  no_outstanding_prepared_ticket,
  no_plan_batch_replay_mismatch,
  no_group_or_deletion_coverage_mismatch,
  no_terminal_target_witness_reprobe_rejected,
  no_owned_k1_terminal_target_history_rejected,
  complete_certified_final_vertical_seal,
};

struct ExactDirectMorseResidentAllOrdersVerticalSealResult {
  std::optional<ExactDirectMorseResidentAllOrdersVerticalFinalSeal> seal;
  bool final_seal_published{false};
  bool existing_final_seal_returned_without_mutation{false};
  bool no_partial_seal_published_on_failure{false};
  ExactDirectMorseResidentAllOrdersVerticalSealDecision decision{
      ExactDirectMorseResidentAllOrdersVerticalSealDecision::not_certified};

  [[nodiscard]] bool certified_final_vertical_seal() const noexcept;
};

class ExactDirectMorseResidentAllOrdersVerticalPreparedBatch {
 public:
  ExactDirectMorseResidentAllOrdersVerticalPreparedBatch() noexcept;
  ~ExactDirectMorseResidentAllOrdersVerticalPreparedBatch();
  ExactDirectMorseResidentAllOrdersVerticalPreparedBatch(
      ExactDirectMorseResidentAllOrdersVerticalPreparedBatch&&) noexcept;
  ExactDirectMorseResidentAllOrdersVerticalPreparedBatch& operator=(
      ExactDirectMorseResidentAllOrdersVerticalPreparedBatch&&) noexcept;
  ExactDirectMorseResidentAllOrdersVerticalPreparedBatch(
      const ExactDirectMorseResidentAllOrdersVerticalPreparedBatch&) = delete;
  ExactDirectMorseResidentAllOrdersVerticalPreparedBatch& operator=(
      const ExactDirectMorseResidentAllOrdersVerticalPreparedBatch&) = delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] bool higher_order_vertical_batch() const noexcept;
  [[nodiscard]] const ExactDirectMorseUnifiedSnapshotIdentity&
  resident_pre_batch_identity() const noexcept;
  [[nodiscard]] const ExactDirectMorseUnifiedResidentAuthorityBundle&
  resident_authority_bundle() const noexcept;
  [[nodiscard]] const ExactDirectMorseResidentAllOrdersVerticalBatchRecord*
  conditional_batch_record() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectMorseResidentAllOrdersVerticalPreparedBatch(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectMorseResidentAllOrdersVerticalBridge;
};

static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseResidentAllOrdersVerticalPreparedBatch>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectMorseResidentAllOrdersVerticalPreparedBatch>);

struct ExactDirectMorseResidentAllOrdersVerticalPreparationResult {
  std::optional<ExactDirectMorseResidentAllOrdersVerticalPreparedBatch> ticket;
  ExactDirectMorseResidentAllOrdersVerticalBridgeRequirements requirements{};
  bool no_resident_or_outer_vertical_scientific_state_mutated_on_failure{false};
  bool owned_k1_lower_cut_may_have_advanced_without_outer_publication{false};
  bool nonhigher_transit_only{false};
  bool conditional_higher_group_images_prepared{false};
  ExactDirectMorseResidentAllOrdersVerticalPreparationDecision decision{
      ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
          not_certified};

  [[nodiscard]] bool certified_prepared_batch() const noexcept;
};

struct ExactDirectMorseResidentAllOrdersVerticalCommitResult {
  ExactDirectMorseResidentK2K1ClosedCutCommitResult owned_bridge_commit{};
  std::size_t committed_resident_batch_cursor{};
  std::size_t committed_higher_batch_count{};
  std::size_t committed_higher_group_count{};
  std::size_t persistent_source_root_witness_count{};
  bool ticket_consumed{false};
  bool outer_vertical_state_mutated{false};
  bool no_outer_vertical_state_mutated_on_owned_bridge_rejection{false};
  bool post_resident_commit_publication_allocation_free{false};
  ExactDirectMorseResidentAllOrdersVerticalCommitDecision decision{
      ExactDirectMorseResidentAllOrdersVerticalCommitDecision::not_certified};

  [[nodiscard]] bool certified_committed_batch() const noexcept;
};

class ExactDirectMorseResidentAllOrdersVerticalBridge;
struct ExactDirectMorseResidentAllOrdersVerticalInitialization;

// Single-threaded by contract.  The owned K2/K1 bridge is never exposed
// mutably.  Prepared tickets carry only owned values and a private shared seal;
// no session address, public bool or transcript can authorize commit.
class ExactDirectMorseResidentAllOrdersVerticalBridge {
 public:
  struct Impl;

  static constexpr std::string_view backend =
      direct_morse_resident_all_orders_vertical_bridge_backend;
  static constexpr std::string_view profile =
      direct_morse_resident_all_orders_vertical_bridge_profile;
  static constexpr std::string_view mode =
      direct_morse_resident_all_orders_vertical_bridge_mode;
  static constexpr std::string_view public_status =
      direct_morse_resident_all_orders_vertical_bridge_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_resident_all_orders_vertical_bridge_proof_basis;

  ExactDirectMorseResidentAllOrdersVerticalBridge() noexcept;
  ~ExactDirectMorseResidentAllOrdersVerticalBridge();
  ExactDirectMorseResidentAllOrdersVerticalBridge(
      ExactDirectMorseResidentAllOrdersVerticalBridge&&) noexcept;
  ExactDirectMorseResidentAllOrdersVerticalBridge& operator=(
      ExactDirectMorseResidentAllOrdersVerticalBridge&&) noexcept;
  ExactDirectMorseResidentAllOrdersVerticalBridge(
      const ExactDirectMorseResidentAllOrdersVerticalBridge&) = delete;
  ExactDirectMorseResidentAllOrdersVerticalBridge& operator=(
      const ExactDirectMorseResidentAllOrdersVerticalBridge&) = delete;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool resident_complete() const noexcept;
  [[nodiscard]] bool conditional_all_adjacent_order_group_images_complete()
      const noexcept;
  [[nodiscard]] ExactDirectMorseUnifiedResidentSourceKind resident_source_kind()
      const noexcept;
  [[nodiscard]] bool resident_normalized_direct_source_session() const noexcept;
  [[nodiscard]] ExactDirectNormalizedH0ResidentRetractionMode
  resident_normalized_h0_retraction_mode() const noexcept;
  [[nodiscard]] bool
  resident_normalized_horizontal_incidence_reduction_certified()
      const noexcept;
  // Product integration must use this live owned capability (and require it),
  // not infer normalized incidence completeness from a copied seal record.
  [[nodiscard]] bool normalized_incidence_complete_v7_source_capability()
      const noexcept;
  [[nodiscard]] ExactDirectK1BoruvkaClosedCutSessionStamp
  owned_k1_current_stamp() const;
  [[nodiscard]] contract::CanonicalId owned_k1_source_forest_digest()
      const noexcept;
  [[nodiscard]] bool verify_owned_k1_source_forest_digest(
      const contract::CanonicalId&) const noexcept;
  [[nodiscard]] std::size_t owned_k1_distinct_level_count() const noexcept;
  [[nodiscard]] bool owned_k1_terminal_complete() const noexcept;
  [[nodiscard]] ExactDirectMorseResidentAllOrdersVerticalBridgeStamp
  current_stamp() const;
  [[nodiscard]] bool verify_stamp(
      const ExactDirectMorseResidentAllOrdersVerticalBridgeStamp&) const
      noexcept;
  [[nodiscard]] bool final_vertical_sealed() const noexcept;
  [[nodiscard]] const ExactDirectMorseResidentAllOrdersVerticalFinalSeal*
  final_vertical_seal() const noexcept;
  [[nodiscard]] bool verify_final_vertical_seal(
      const ExactDirectMorseResidentAllOrdersVerticalFinalSeal&) const
      noexcept;
  [[nodiscard]] const std::vector<
      ExactDirectMorseResidentAllOrdersVerticalRootWitness>&
  source_root_target_witnesses() const noexcept;
  [[nodiscard]] const std::vector<
      ExactDirectMorseResidentAllOrdersVerticalBatchRecord>&
  committed_higher_batches() const noexcept;
  [[nodiscard]] const std::vector<
      ExactDirectMorseResidentK2K1ClosedCutBatchRecord>&
  committed_k2_batches() const noexcept;
  [[nodiscard]] const ExactDirectSparseUnifiedLevelPlanResult& resident_plan()
      const noexcept;
  [[nodiscard]] const ExactDirectSparsePositiveFacetLocator& resident_locator()
      const noexcept;
  [[nodiscard]] const std::vector<
      ExactDirectMorseUnifiedResidentComponentState>&
  resident_component_states() const noexcept;
  [[nodiscard]] const std::vector<
      ExactDirectMorseUnifiedResidentRootCoverage>&
  resident_root_coverages() const noexcept;
  [[nodiscard]] const std::vector<ExactDirectMorseUnifiedResidentGroupRecord>&
  resident_group_records() const noexcept;

  [[nodiscard]] ExactDirectMorseResidentAllOrdersVerticalPreparationResult
  prepare_next();
  [[nodiscard]] ExactDirectMorseResidentAllOrdersVerticalCommitResult commit(
      ExactDirectMorseResidentAllOrdersVerticalPreparedBatch&&) noexcept;
  [[nodiscard]] ExactDirectMorseResidentAllOrdersVerticalSealResult seal()
      noexcept;

 private:
  explicit ExactDirectMorseResidentAllOrdersVerticalBridge(
      std::unique_ptr<Impl>) noexcept;

  friend struct ExactDirectMorseResidentAllOrdersVerticalInitialization;
  friend ExactDirectMorseResidentAllOrdersVerticalInitialization
  initialize_exact_direct_morse_resident_all_orders_vertical_bridge(
      ExactDirectMorseResidentK2K1ClosedCutBridge&&,
      const ExactDirectMorseResidentAllOrdersVerticalBridgeBudget&);

  std::unique_ptr<Impl> impl_;
};

static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseResidentAllOrdersVerticalBridge>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectMorseResidentAllOrdersVerticalBridge>);

struct ExactDirectMorseResidentAllOrdersVerticalInitialization {
  std::uint32_t schema_version{
      direct_morse_resident_all_orders_vertical_bridge_schema_version};
  ExactDirectMorseResidentAllOrdersVerticalBridgeBudget requested_budget{};
  ExactDirectMorseResidentAllOrdersVerticalBridgeRequirements requirements{};
  contract::CanonicalId canonical_cloud_digest{};
  contract::CanonicalId resident_higher_canonical_cloud_digest{};
  bool owned_k2_k1_bridge_consumed{false};
  bool process_local_bridge_capability_issued{false};
  bool exact_initial_resident_cursor_required{false};
  bool persistent_batch_and_root_witness_arenas_preallocated{false};
  bool incidence_complete_reduction{false};
  bool all_naturality_squares_replayed{false};
  bool vertical_maps_complete{false};
  bool global_facet_coface_or_gamma_catalog_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool pair_matrix_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseResidentAllOrdersVerticalInitializationDecision decision{
      ExactDirectMorseResidentAllOrdersVerticalInitializationDecision::
          not_certified};
  ExactDirectMorseResidentAllOrdersVerticalBridge bridge{};

  [[nodiscard]] bool certified_ready_bridge() const noexcept;
};

[[nodiscard]] ExactDirectMorseResidentAllOrdersVerticalInitialization
initialize_exact_direct_morse_resident_all_orders_vertical_bridge(
    ExactDirectMorseResidentK2K1ClosedCutBridge&& owned_bridge,
    const ExactDirectMorseResidentAllOrdersVerticalBridgeBudget& budget);

}  // namespace morsehgp3d::hierarchy
