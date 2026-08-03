#pragma once

// Archived Phase 15 prototype; excluded from the active API and build.

#include "morsehgp3d/hierarchy/direct_morse_unified_resident_session.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

struct ExactDirectMorseUnifiedResidentVerticalBridgeInitializationResult;

inline constexpr std::uint32_t
    direct_morse_unified_resident_vertical_stream_bridge_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_unified_resident_vertical_stream_bridge_backend =
        "reference_cpu";
inline constexpr std::string_view
    direct_morse_unified_resident_vertical_stream_bridge_profile =
        "hgp_reduced";
inline constexpr std::string_view
    direct_morse_unified_resident_vertical_stream_bridge_mode =
        "conditional_exact_resident_prebatch_adjacent_order_vertical_stream";
inline constexpr std::string_view
    direct_morse_unified_resident_vertical_stream_bridge_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_morse_unified_resident_vertical_stream_bridge_proof_basis =
        "one_resident_prebatch_frozen_incidence_star_edge_per_common_lower_"
        "facet_same_closed_target_root_per_group_prior_carrier_reprobe_"
        "continuation_and_multifusion_propagation_external_k2_k1_seam_and_"
        "resident_commit_then_noexcept_vertical_publication_v1";

// K=2 is deliberately outside the resident lower-order locator.  This typed
// authority says only that a separate K1/Boruvka producer can answer every
// requested singleton carrier at an exact closed cut.  The bridge never mints
// or infers this authority.
struct ExactDirectMorseUnifiedResidentExternalK2K1SeamAuthority {
  std::uint32_t schema_version{
      direct_morse_unified_resident_vertical_stream_bridge_schema_version};
  std::uint64_t external_seam_authority_id{};
  contract::CanonicalId canonical_cloud_digest{};
  std::size_t point_count{};
  bool authority_freshly_verified{false};
  bool exact_k1_closed_cuts_certified{false};
  bool every_requested_singleton_has_one_closed_k1_root{false};
  bool horizontal_k1_successors_certified{false};
  bool global_membership_star_gamma_or_delaunay_materialized{false};
  bool public_status_claimed{false};

  [[nodiscard]] bool certified_external_seam() const noexcept;

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentExternalK2K1SeamAuthority&,
      const ExactDirectMorseUnifiedResidentExternalK2K1SeamAuthority&) =
      default;
};

struct ExactDirectMorseUnifiedResidentExternalK1Binding {
  std::size_t binding_index{};
  spatial::PointId point_id{};
  std::uint64_t closed_k1_root_id{};

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentExternalK1Binding&,
      const ExactDirectMorseUnifiedResidentExternalK1Binding&) = default;
};

// One caller-owned, batch-local view of the external K1 seam.  Bindings must
// be strictly point ordered.  They may be a bounded superset of the singleton
// points requested by the current resident batch, but no completeness beyond
// that request is inferred.
struct ExactDirectMorseUnifiedResidentExternalK2K1BatchReceipt {
  std::uint32_t schema_version{
      direct_morse_unified_resident_vertical_stream_bridge_schema_version};
  std::uint64_t external_seam_authority_id{};
  std::uint64_t resident_session_authority_id{};
  std::uint64_t resident_locator_instance_id{};
  contract::CanonicalId canonical_cloud_digest{};
  std::size_t point_count{};
  std::size_t source_batch_index{};
  exact::ExactLevel closed_squared_level{};
  std::vector<ExactDirectMorseUnifiedResidentExternalK1Binding> bindings;
  bool batch_receipt_freshly_verified{false};
  bool exact_closed_cut_certified{false};
  bool all_intermediate_k1_levels_accounted{false};
  bool bindings_complete_for_requested_singletons{false};
  bool roots_closed_at_exact_level{false};
  bool global_membership_star_gamma_or_delaunay_materialized{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentExternalK2K1BatchReceipt&,
      const ExactDirectMorseUnifiedResidentExternalK2K1BatchReceipt&) =
      default;
};

struct ExactDirectMorseUnifiedResidentVerticalStreamBridgeBudget {
  ExactDirectSparsePositiveFacetProbeBudget target_probe_budget{};
  std::size_t maximum_resident_batch_count{};
  std::size_t maximum_persistent_source_root_carrier_count{};
  std::size_t maximum_batch_group_count{};
  std::size_t maximum_batch_hyperedge_count{};
  std::size_t maximum_batch_token_reference_count{};
  std::size_t maximum_batch_incidence_union_witness_count{};
  std::size_t maximum_batch_prior_carrier_reprobe_count{};
  std::size_t maximum_batch_target_probe_count{};
  std::size_t maximum_batch_target_probe_slot_visit_count{};
  std::size_t maximum_batch_target_probe_parent_hop_count{};
  std::size_t maximum_external_seam_binding_scan_count{};
  std::size_t maximum_key_point_scan_count{};
  std::size_t maximum_exact_level_comparison_count{};
  std::size_t maximum_scratch_entry_count{};
  std::size_t maximum_logical_batch_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentVerticalStreamBridgeBudget&,
      const ExactDirectMorseUnifiedResidentVerticalStreamBridgeBudget&) =
      default;
};

struct ExactDirectMorseUnifiedResidentVerticalStreamBridgeConfig {
  std::uint64_t bridge_authority_id{};
  std::uint64_t first_query_replay_token{};
  // The bridge cannot mint witnesses under the resident locator's external
  // authority.  This bit is an explicit caller certificate that the monotone
  // token suffix beginning above is reserved for read-only bridge probes.
  bool query_replay_token_suffix_reserved_by_resident_authority{false};
  ExactDirectMorseUnifiedResidentExternalK2K1SeamAuthority external_k2_k1{};

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentVerticalStreamBridgeConfig&,
      const ExactDirectMorseUnifiedResidentVerticalStreamBridgeConfig&) =
      default;
};

enum class ExactDirectMorseUnifiedResidentVerticalTargetKind : std::uint8_t {
  resident_adjacent_lower_order_root,
  external_k1_root,
};

// Exactly one durable carrier is retained per source root.  target_facet_key
// has source_order-1 points.  For K=2, target_binding_witness names the typed
// external seam receipt rather than a resident locator binding.
struct ExactDirectMorseUnifiedResidentVerticalRootCarrier {
  ExactFrozenIncidencePriorRootId source_root_id{};
  std::size_t source_order{};
  std::size_t target_order{};
  std::uint64_t closed_target_root_id{};
  ExactDirectSparseFacetKey target_facet_key{};
  ExactDirectSparseFacetWitness target_binding_witness{};
  std::size_t created_source_batch_index{};
  std::size_t last_verified_source_batch_index{};
  ExactDirectMorseUnifiedResidentVerticalTargetKind target_kind{
      ExactDirectMorseUnifiedResidentVerticalTargetKind::
          resident_adjacent_lower_order_root};

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentVerticalRootCarrier&,
      const ExactDirectMorseUnifiedResidentVerticalRootCarrier&) = default;
};

// Every factorized source coface is replayed as a canonical star of source
// K-facet adjacencies.  Each edge owns its exact K-1 intersection and the
// closed target root returned at the same cut.  These records are batch-local
// streaming output and are never retained by the bridge session.
struct ExactDirectMorseUnifiedResidentVerticalIncidenceUnionWitness {
  std::size_t witness_index{};
  std::size_t owner_group_index{};
  std::size_t source_hyperedge_index{};
  std::size_t left_source_facet_token_index{};
  std::size_t right_source_facet_token_index{};
  ExactDirectSparseFacetKey common_target_facet_key{};
  std::uint64_t closed_target_root_id{};
  ExactDirectSparseFacetWitness target_binding_witness{};
  ExactDirectMorseUnifiedResidentVerticalTargetKind target_kind{
      ExactDirectMorseUnifiedResidentVerticalTargetKind::
          resident_adjacent_lower_order_root};

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentVerticalIncidenceUnionWitness&,
      const ExactDirectMorseUnifiedResidentVerticalIncidenceUnionWitness&) =
      default;
};

struct ExactDirectMorseUnifiedResidentVerticalGroupImage {
  std::size_t group_image_index{};
  std::size_t source_group_index{};
  ExactFrozenIncidencePriorRootId resultant_source_root_id{};
  std::uint64_t closed_target_root_id{};
  std::size_t prior_source_root_offset{};
  std::size_t prior_source_root_count{};
  std::size_t incidence_union_witness_offset{};
  std::size_t incidence_union_witness_count{};
  ExactFrozenIncidenceHgpAction source_action{
      ExactFrozenIncidenceHgpAction::reduced_birth};
  bool all_prior_carriers_reverified{false};
  bool all_incidence_unions_have_same_target_root{false};

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentVerticalGroupImage&,
      const ExactDirectMorseUnifiedResidentVerticalGroupImage&) = default;
};

struct ExactDirectMorseUnifiedResidentVerticalBatchCounters {
  std::size_t group_count{};
  std::size_t hyperedge_count{};
  std::size_t token_reference_count{};
  std::size_t incidence_union_witness_count{};
  std::size_t prior_carrier_reprobe_count{};
  std::size_t target_probe_count{};
  std::size_t target_probe_slot_visit_count{};
  std::size_t target_probe_parent_hop_count{};
  std::size_t external_seam_binding_scan_count{};
  std::size_t key_point_scan_count{};
  std::size_t exact_level_comparison_count{};
  std::size_t logical_scratch_entry_count{};
  std::size_t logical_output_entry_count{};

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentVerticalBatchCounters&,
      const ExactDirectMorseUnifiedResidentVerticalBatchCounters&) = default;
};

enum class ExactDirectMorseUnifiedResidentVerticalBatchScope
    : std::uint8_t {
  unspecified,
  one_verified_resident_prebatch_relative_adjacent_order_vertical_step_only,
};

struct ExactDirectMorseUnifiedResidentVerticalBatchReceipt {
  static constexpr std::string_view backend =
      direct_morse_unified_resident_vertical_stream_bridge_backend;
  static constexpr std::string_view profile =
      direct_morse_unified_resident_vertical_stream_bridge_profile;
  static constexpr std::string_view mode =
      direct_morse_unified_resident_vertical_stream_bridge_mode;
  static constexpr std::string_view public_status =
      direct_morse_unified_resident_vertical_stream_bridge_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_unified_resident_vertical_stream_bridge_proof_basis;

  std::uint32_t schema_version{
      direct_morse_unified_resident_vertical_stream_bridge_schema_version};
  std::uint64_t bridge_authority_id{};
  ExactDirectMorseUnifiedSnapshotIdentity source_prebatch_identity{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp source_postbatch_stamp{};
  contract::CanonicalId canonical_cloud_digest{};
  std::size_t source_batch_index{};
  exact::ExactLevel closed_squared_level{};
  std::size_t source_order{};
  std::size_t target_order{};
  std::vector<ExactFrozenIncidencePriorRootId> prior_source_root_ids;
  std::vector<ExactDirectMorseUnifiedResidentVerticalGroupImage> group_images;
  std::vector<ExactDirectMorseUnifiedResidentVerticalIncidenceUnionWitness>
      incidence_union_witnesses;
  ExactDirectMorseUnifiedResidentVerticalBatchCounters counters{};
  bool resident_prebatch_bundle_certified{false};
  bool merged_exact_level_order_prefix_certified{false};
  bool target_closed_cut_precedes_source_batch{false};
  bool external_k2_k1_seam_replayed{false};
  bool resident_lower_order_locator_replayed{false};
  bool every_source_coface_replayed_as_incidence_star{false};
  bool every_incidence_union_has_exact_common_target_facet{false};
  bool one_closed_target_root_per_source_group{false};
  bool continuations_propagated_from_prior_carrier{false};
  bool multifusions_propagated_from_all_prior_carriers{false};
  bool one_carrier_published_per_resultant_source_root{false};
  bool resident_batch_committed_before_vertical_publication{false};
  bool publication_suffix_noexcept{false};
  bool conditional_on_resident_source_and_external_k1_seam{true};
  bool incidence_complete_reduction_claimed{false};
  bool all_global_naturality_squares_claimed{false};
  bool vertical_maps_complete_claimed{false};
  bool global_membership_star_gamma_or_delaunay_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseUnifiedResidentVerticalBatchScope scope{
      ExactDirectMorseUnifiedResidentVerticalBatchScope::unspecified};

  [[nodiscard]] bool certified_relative_streamed_batch() const noexcept;

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentVerticalBatchReceipt&,
      const ExactDirectMorseUnifiedResidentVerticalBatchReceipt&) = default;
};

enum class ExactDirectMorseUnifiedResidentVerticalBridgeInitializationDecision
    : std::uint8_t {
  not_certified,
  no_bridge_resident_not_at_empty_prefix,
  no_bridge_external_seam_rejected,
  no_bridge_budget_rejected,
  no_bridge_allocation_failed,
  complete_certified_empty_relative_vertical_bridge,
};

enum class ExactDirectMorseUnifiedResidentVerticalPreparationDecision
    : std::uint8_t {
  not_certified,
  no_bridge_not_ready,
  no_bridge_outstanding_ticket,
  no_bridge_resident_or_ticket_rejected,
  no_bridge_stale_resident_prefix,
  no_bridge_batch_order_rejected,
  no_bridge_target_cut_not_closed,
  no_bridge_external_seam_rejected,
  no_bridge_source_incidence_rejected,
  no_bridge_common_target_facet_rejected,
  no_bridge_target_carrier_not_rooted,
  no_bridge_prior_carrier_missing,
  no_bridge_target_root_conflict,
  no_bridge_capacity_overflow,
  no_bridge_budget_exhausted,
  no_bridge_allocation_failed,
  complete_certified_prepared_relative_vertical_batch,
};

enum class ExactDirectMorseUnifiedResidentVerticalCommitDecision
    : std::uint8_t {
  not_certified,
  no_bridge_prepared_ticket_rejected,
  no_bridge_resident_commit_rejected_without_vertical_mutation,
  complete_certified_resident_and_vertical_atomic_commit,
};

class ExactDirectMorseUnifiedResidentPreparedVerticalBatch {
 public:
  ExactDirectMorseUnifiedResidentPreparedVerticalBatch() noexcept;
  ~ExactDirectMorseUnifiedResidentPreparedVerticalBatch();
  ExactDirectMorseUnifiedResidentPreparedVerticalBatch(
      ExactDirectMorseUnifiedResidentPreparedVerticalBatch&&) noexcept;
  ExactDirectMorseUnifiedResidentPreparedVerticalBatch& operator=(
      ExactDirectMorseUnifiedResidentPreparedVerticalBatch&&) noexcept;
  ExactDirectMorseUnifiedResidentPreparedVerticalBatch(
      const ExactDirectMorseUnifiedResidentPreparedVerticalBatch&) = delete;
  ExactDirectMorseUnifiedResidentPreparedVerticalBatch& operator=(
      const ExactDirectMorseUnifiedResidentPreparedVerticalBatch&) = delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] bool consumed() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectMorseUnifiedResidentPreparedVerticalBatch(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectMorseUnifiedResidentVerticalStreamBridge;
};

struct ExactDirectMorseUnifiedResidentVerticalPreparationResult {
  std::optional<ExactDirectMorseUnifiedResidentPreparedVerticalBatch> ticket;
  bool bridge_and_resident_state_unmodified{false};
  ExactDirectMorseUnifiedResidentVerticalPreparationDecision decision{
      ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
          not_certified};

  [[nodiscard]] bool certified_prepared_vertical_batch() const noexcept;
};

struct ExactDirectMorseUnifiedResidentVerticalCommitResult {
  ExactDirectMorseUnifiedResidentCommitResult resident_commit{};
  std::optional<ExactDirectMorseUnifiedResidentVerticalBatchReceipt> receipt;
  bool prepared_vertical_ticket_consumed{false};
  bool vertical_state_mutated{false};
  bool no_vertical_state_mutated_on_resident_failure{false};
  bool resident_committed_before_vertical_publication{false};
  bool postcommit_suffix_noexcept_or_fail_stop{false};
  ExactDirectMorseUnifiedResidentVerticalCommitDecision decision{
      ExactDirectMorseUnifiedResidentVerticalCommitDecision::not_certified};

  [[nodiscard]] bool certified_atomic_vertical_commit() const noexcept;
};

class ExactDirectMorseUnifiedResidentVerticalStreamBridge {
 public:
  static constexpr std::string_view backend =
      direct_morse_unified_resident_vertical_stream_bridge_backend;
  static constexpr std::string_view profile =
      direct_morse_unified_resident_vertical_stream_bridge_profile;
  static constexpr std::string_view mode =
      direct_morse_unified_resident_vertical_stream_bridge_mode;
  static constexpr std::string_view public_status =
      direct_morse_unified_resident_vertical_stream_bridge_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_unified_resident_vertical_stream_bridge_proof_basis;

  ExactDirectMorseUnifiedResidentVerticalStreamBridge() noexcept;
  ~ExactDirectMorseUnifiedResidentVerticalStreamBridge();
  ExactDirectMorseUnifiedResidentVerticalStreamBridge(
      ExactDirectMorseUnifiedResidentVerticalStreamBridge&&) noexcept;
  ExactDirectMorseUnifiedResidentVerticalStreamBridge& operator=(
      ExactDirectMorseUnifiedResidentVerticalStreamBridge&&) noexcept;
  ExactDirectMorseUnifiedResidentVerticalStreamBridge(
      const ExactDirectMorseUnifiedResidentVerticalStreamBridge&) = delete;
  ExactDirectMorseUnifiedResidentVerticalStreamBridge& operator=(
      const ExactDirectMorseUnifiedResidentVerticalStreamBridge&) = delete;

  [[nodiscard]] bool certified_relative_bridge() const noexcept;
  [[nodiscard]] std::size_t resident_batch_cursor() const noexcept;
  [[nodiscard]] std::size_t resident_epoch() const noexcept;
  [[nodiscard]] const std::vector<
      ExactDirectMorseUnifiedResidentVerticalRootCarrier>&
  root_carriers() const noexcept;

  [[nodiscard]] ExactDirectMorseUnifiedResidentVerticalPreparationResult
  prepare_next(
      const ExactDirectMorseUnifiedResidentSession& resident,
      const ExactDirectMorseUnifiedResidentPreparedBatch& resident_ticket,
      const ExactDirectMorseUnifiedResidentExternalK2K1BatchReceipt*
          external_k2_k1_batch);

  // This is the only supported commit path for a bridged batch.  All fallible
  // vertical work is complete before the resident commit.  A normal resident
  // rejection leaves the vertical carrier state unchanged.  Once the resident
  // commit succeeds, any impossible postcondition is fail-stop because the
  // resident mutation can no longer be rolled back honestly.
  [[nodiscard]] ExactDirectMorseUnifiedResidentVerticalCommitResult commit(
      ExactDirectMorseUnifiedResidentSession& resident,
      ExactDirectMorseUnifiedResidentPreparedBatch&& resident_ticket,
      ExactDirectMorseUnifiedResidentPreparedVerticalBatch&& vertical_ticket)
      noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectMorseUnifiedResidentVerticalStreamBridge(
      std::unique_ptr<Impl>) noexcept;
  friend struct
      ExactDirectMorseUnifiedResidentVerticalBridgeInitializationResult;
  friend ExactDirectMorseUnifiedResidentVerticalBridgeInitializationResult
  initialize_exact_direct_morse_unified_resident_vertical_stream_bridge(
      const ExactDirectMorseUnifiedResidentSession&,
      const ExactDirectMorseUnifiedResidentVerticalStreamBridgeBudget&,
      const ExactDirectMorseUnifiedResidentVerticalStreamBridgeConfig&);
};

struct ExactDirectMorseUnifiedResidentVerticalBridgeInitializationResult {
  std::optional<ExactDirectMorseUnifiedResidentVerticalStreamBridge> bridge;
  bool resident_empty_prefix_certified{false};
  bool external_k2_k1_seam_certified{false};
  bool persistent_carrier_capacity_preallocated{false};
  bool no_global_membership_star_gamma_or_delaunay_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseUnifiedResidentVerticalBridgeInitializationDecision decision{
      ExactDirectMorseUnifiedResidentVerticalBridgeInitializationDecision::
          not_certified};

  [[nodiscard]] bool certified_initialized_bridge() const noexcept;
};

[[nodiscard]]
ExactDirectMorseUnifiedResidentVerticalBridgeInitializationResult
initialize_exact_direct_morse_unified_resident_vertical_stream_bridge(
    const ExactDirectMorseUnifiedResidentSession& resident,
    const ExactDirectMorseUnifiedResidentVerticalStreamBridgeBudget& budget,
    const ExactDirectMorseUnifiedResidentVerticalStreamBridgeConfig& config);

}  // namespace morsehgp3d::hierarchy
