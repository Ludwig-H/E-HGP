#pragma once

#include "morsehgp3d/hierarchy/direct_normalized_h0_source_plan.hpp"
#include "morsehgp3d/hierarchy/direct_frozen_unified_incidence_batch.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_closure.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

struct ExactDirectMorseUnifiedResidentInitializationResult;

enum class ExactDirectMorseUnifiedResidentSourceKind : std::uint8_t {
  successive_incidence_star,
  normalized_direct_h0_candidate_source,
};

// The certified mode is constructible only through the rank-window + sparse
// strict-facet closure initializer.  The enum is descriptive: callers cannot
// promote it, or a bool, into the private move-only session capability.
enum class ExactDirectNormalizedH0ResidentRetractionMode : std::uint8_t {
  not_applicable_successive_incidence_star,
  candidate_fail_open_without_h0_retraction_authority,
  certified_rank_window_and_sparse_strict_facet_closure,
};

enum class ExactDirectNormalizedH0CandidateFacetDisposition : std::uint8_t {
  equal_at_active_level,
  fail_open_strictly_earlier_without_retraction_authority,
  contradiction_strictly_later_than_active_level,
};

[[nodiscard]] ExactDirectNormalizedH0CandidateFacetDisposition
classify_exact_direct_normalized_h0_candidate_facet_birth(
    const exact::ExactLevel& facet_birth_squared_level,
    const exact::ExactLevel& active_squared_level) noexcept;

// Bounds the one initialization-only translation from the factorized
// normalized source into the sparse resident cursor format.  A full (K+1)
// coface exists only as fixed-size scratch while its K-facet deletions are
// emitted.  The resulting durable arenas contain no (K+1) key and no Star,
// Gamma or higher-order Delaunay authority.  This remains a bounded candidate
// adapter for audit/integration, not the massive-cloud deployment path: every
// temporary arena and its coexistence with the durable compatibility plan
// must fit these explicit caps before the first adapter allocation.
struct ExactDirectNormalizedH0ResidentAdapterBudget {
  std::size_t maximum_source_direct_birth_scan_count{};
  std::size_t maximum_source_coface_scan_count{};
  std::size_t maximum_source_batch_scan_count{};
  std::size_t maximum_coface_facet_reference_count{};
  std::size_t maximum_distinct_facet_count{};
  std::size_t maximum_facet_key_point_count{};
  std::size_t maximum_logical_storage_entry_count{};
  std::size_t maximum_coface_occurrence_offset_scratch_count{};
  std::size_t maximum_facet_occurrence_scratch_count{};
  std::size_t maximum_distinct_facet_scratch_count{};
  std::size_t maximum_batch_coface_index_scratch_count{};
  std::size_t maximum_simultaneous_adapter_entry_count{};

  friend bool operator==(
      const ExactDirectNormalizedH0ResidentAdapterBudget&,
      const ExactDirectNormalizedH0ResidentAdapterBudget&) = default;
};

inline constexpr std::uint32_t
    direct_morse_unified_resident_session_schema_version = 6U;
inline constexpr std::string_view
    direct_morse_unified_resident_session_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_unified_resident_session_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_unified_resident_session_mode =
        "certified_resident_unified_strict_prebatch_authority_and_atomic_"
        "sparse_delta_commit_with_normalized_candidate_fail_open_or_rank_"
        "window_sparse_strict_facet_closure_v6";
inline constexpr std::string_view
    direct_morse_unified_resident_session_public_status = "not_claimed";
inline constexpr std::string_view
    direct_morse_unified_resident_session_deployment_status =
        "architecture_only_not_strict_miss_integration_qualified_v6";
inline constexpr std::string_view
    direct_morse_unified_resident_session_proof_basis =
        "one_initial_unified_plan_verification_resident_sparse_positive_"
        "locator_exact_local_miniball_miss_certification_strict_pre_batch_"
        "frozen_authority_bundle_move_only_epoch_ticket_single_locator_"
        "transaction_with_rollbackable_pre_staged_sparse_delta_and_immutable_"
        "verified_plan_authority_single_batch_construction_single_quotient_"
        "action_streaming_verification_move_only_attestation_and_normalized_"
        "candidate_fail_open_or_fresh_rank_window_authority_one_sparse_"
        "strict_facet_closure_per_nonempty_strict_miss_set_v6";

struct ExactDirectMorseUnifiedResidentSparseDeltaBudget {
  std::size_t maximum_component_patch_count{};
  std::size_t maximum_component_patch_latent_point_reference_count{};
  std::size_t maximum_root_replacement_count{};
  std::size_t maximum_new_root_count{};
  std::size_t maximum_root_patch_point_reference_count{};
  std::size_t maximum_group_append_count{};
  std::size_t maximum_group_child_reference_count{};
  std::size_t maximum_group_coverage_delta_point_reference_count{};
  std::size_t maximum_outstanding_ticket_count{};

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentSparseDeltaBudget&,
      const ExactDirectMorseUnifiedResidentSparseDeltaBudget&) = default;
};

// The public standalone frozen-batch API retains its fresh source-plan replay.
// The resident session instead holds an internal immutable authority created
// by its one initialization verification; no global source-plan replay is
// permitted on the per-batch path.
struct ExactDirectMorseUnifiedResidentSessionBudget {
  ExactDirectSparsePositiveFacetLocatorBudget locator{};
  ExactDirectSparsePositiveFacetProbeBudget probe{};
  ExactDirectFrozenUnifiedIncidenceBatchBudget frozen_batch{};
  std::size_t maximum_facet_resolution_count{};
  std::size_t maximum_prior_root_coverage_count{};
  std::size_t maximum_prior_root_coverage_point_reference_count{};
  std::size_t maximum_latent_carrier_coverage_count{};
  std::size_t maximum_latent_carrier_coverage_point_reference_count{};
  std::size_t maximum_fresh_facet_miniball_count{};
  std::size_t maximum_fresh_facet_miniball_support_enumeration_count{};
  // Certified-normalized pass-1 observations have their own cap.  The strict
  // cap counts the three concurrent strict-side vectors (observation indices,
  // canonical keys and compact carrier records).  The simultaneous cap also
  // includes pass-1 observations and the returned closure nodes, edges and
  // seed projections; closure-internal memo/step arenas retain their own
  // explicit ExactDirectSparseFacetDescentClosureBudget bounds.
  std::size_t maximum_normalized_facet_observation_scratch_count{};
  std::size_t maximum_normalized_strict_facet_scratch_count{};
  std::size_t
      maximum_normalized_strict_facet_simultaneous_scratch_entry_count{};
  std::size_t maximum_resident_root_count{};
  std::size_t maximum_resident_root_point_reference_count{};
  std::size_t maximum_resident_component_latent_point_reference_count{};
  std::size_t maximum_group_record_count{};
  std::size_t maximum_group_child_reference_count{};
  std::size_t maximum_group_coverage_delta_point_reference_count{};
  ExactDirectMorseUnifiedResidentSparseDeltaBudget sparse_delta{};

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentSessionBudget&,
      const ExactDirectMorseUnifiedResidentSessionBudget&) = default;
};

struct ExactDirectMorseUnifiedSnapshotIdentity {
  std::uint32_t schema_version{
      direct_morse_unified_resident_session_schema_version};
  std::uint64_t session_authority_id{};
  std::uint64_t locator_instance_id{};
  std::size_t epoch{};
  std::size_t batch_cursor{};
  contract::CanonicalId source_pair_canonical_cloud_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_stamp{};

  friend bool operator==(
      const ExactDirectMorseUnifiedSnapshotIdentity&,
      const ExactDirectMorseUnifiedSnapshotIdentity&) = default;
};

struct ExactDirectMorseUnifiedResidentBatchCounters {
  std::size_t locator_probe_count{};
  std::size_t positive_locator_probe_count{};
  std::size_t unresolved_locator_probe_count{};
  std::size_t fresh_facet_miniball_build_count{};
  std::size_t fresh_facet_miniball_verification_count{};
  // Conservative work accounting: three complete enumerations per miss
  // cover construction, its internal certification pass, and the explicit
  // fresh verifier replay retained by this session.
  std::size_t fresh_facet_miniball_support_enumeration_count{};
  std::size_t rooted_resolution_count{};
  std::size_t latent_resolution_count{};
  std::size_t equal_resolution_count{};
  std::size_t normalized_strict_facet_seed_count{};
  std::size_t normalized_facet_observation_scratch_entry_peak{};
  std::size_t normalized_strict_facet_scratch_entry_peak{};
  std::size_t normalized_strict_facet_simultaneous_scratch_entry_peak{};
  std::size_t normalized_strict_facet_closure_build_count{};
  std::size_t normalized_strict_facet_closure_node_count{};
  std::size_t normalized_strict_facet_closure_edge_count{};
  std::size_t normalized_strict_facet_closure_seed_projection_count{};
  std::size_t normalized_strict_facet_closure_locator_snapshot_check_count{};
  std::size_t normalized_strict_facet_carrier_resolution_count{};
  std::size_t planned_component_union_count{};
  std::size_t planned_equal_binding_count{};
  std::size_t planned_birth_binding_count{};
  std::size_t planned_strict_facet_binding_count{};
  std::size_t planned_group_record_count{};
  std::size_t resident_state_full_copy_count{};
  std::size_t sparse_delta_component_patch_count{};
  std::size_t sparse_delta_component_latent_point_reference_count{};
  std::size_t sparse_delta_root_replacement_count{};
  std::size_t sparse_delta_new_root_count{};
  std::size_t sparse_delta_root_point_reference_count{};
  std::size_t sparse_delta_group_append_count{};
  std::size_t sparse_delta_group_child_reference_count{};
  std::size_t sparse_delta_group_coverage_delta_point_reference_count{};

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentBatchCounters&,
      const ExactDirectMorseUnifiedResidentBatchCounters&) = default;
};

// Public, structurally checkable receipt for the resident-only fast path.  The
// non-forgeable capability itself remains inside the move-only prepared ticket.
// Unlike the standalone verifier, this receipt makes no claim of an
// independently reconstructed expected batch.
struct ExactDirectMorseUnifiedResidentFrozenBatchReceipt {
  std::uint32_t schema_version{
      direct_morse_unified_resident_session_schema_version};
  std::size_t frozen_batch_construction_count{};
  std::size_t quotient_streaming_verification_count{};
  std::size_t action_plan_streaming_verification_count{};
  std::size_t structural_certification_count{};
  bool source_plan_immutable_authority_certified{false};
  bool frozen_batch_structurally_certified{false};
  bool internal_move_only_attestation_issued{false};
  bool independent_expected_batch_freshly_reconstructed{false};
  bool global_facet_coface_or_gamma_catalog_materialized{false};
  bool supplied_star_global_completeness_claimed{false};
  bool public_status_claimed{false};

  [[nodiscard]] bool certified_single_construction_receipt() const noexcept;

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentFrozenBatchReceipt&,
      const ExactDirectMorseUnifiedResidentFrozenBatchReceipt&) = default;
};

// All five authority arenas and the frozen result share exactly one identity.
// They are the strict pre-batch view retained by the ticket until commit.
struct ExactDirectMorseUnifiedResidentAuthorityBundle {
  ExactDirectMorseUnifiedSnapshotIdentity identity{};
  std::size_t source_batch_index{};
  std::size_t source_future_snapshot_index{};
  exact::ExactLevel squared_level{};
  std::size_t order{};
  std::vector<ExactDirectFrozenUnifiedFacetResolution> facet_resolutions;
  std::vector<ExactDirectFrozenUnifiedPriorRootCoverage>
      prior_root_coverages;
  std::vector<spatial::PointId> prior_root_coverage_point_references;
  std::vector<ExactDirectFrozenUnifiedLatentCarrierCoverage>
      latent_carrier_coverages;
  std::vector<spatial::PointId>
      latent_carrier_coverage_point_references;
  ExactDirectFrozenUnifiedIncidenceBatchResult frozen_batch{};
  ExactDirectMorseUnifiedResidentFrozenBatchReceipt frozen_batch_receipt{};
  ExactDirectMorseUnifiedResidentBatchCounters counters{};
  bool locator_snapshot_strictly_pre_batch{false};
  bool every_unresolved_facet_has_fresh_exact_equal_miniball{false};
  bool every_locator_miss_has_fresh_exact_miniball{false};
  bool every_strict_facet_miss_has_certified_relative_positive_closure{false};
  bool strict_facet_closure_bound_to_pre_batch_locator_snapshot{false};
  bool rank_window_saturated_h0_authority_certified{false};
  bool private_strict_facet_closure_attestation_issued{false};
  bool csr_authorities_share_identity_and_pre_batch_state{false};
  bool global_facet_coface_or_gamma_catalog_materialized{false};
  bool supplied_star_global_completeness_claimed{false};
  bool public_status_claimed{false};

  [[nodiscard]] bool certified_strict_pre_batch_bundle() const noexcept;

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentAuthorityBundle&,
      const ExactDirectMorseUnifiedResidentAuthorityBundle&) = default;
};

struct ExactDirectMorseUnifiedResidentComponentState {
  std::size_t component_handle{};
  std::size_t parent_handle{};
  std::optional<ExactFrozenIncidencePriorRootId> root_id;
  std::vector<spatial::PointId> latent_point_coverage;
  bool active{false};

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentComponentState&,
      const ExactDirectMorseUnifiedResidentComponentState&) = default;
};

struct ExactDirectMorseUnifiedResidentRootCoverage {
  ExactFrozenIncidencePriorRootId root_id{};
  std::vector<spatial::PointId> point_ids;

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentRootCoverage&,
      const ExactDirectMorseUnifiedResidentRootCoverage&) = default;
};

struct ExactDirectMorseUnifiedResidentGroupRecord {
  std::size_t group_record_index{};
  std::size_t batch_index{};
  std::size_t owner_group_index{};
  exact::ExactLevel squared_level{};
  std::size_t order{};
  std::size_t q_r{};
  ExactFrozenIncidenceHgpAction action{
      ExactFrozenIncidenceHgpAction::reduced_birth};
  ExactFrozenIncidencePriorRootId resultant_root_id{};
  std::vector<ExactFrozenIncidencePriorRootId> child_root_ids;
  std::vector<spatial::PointId> coverage_delta_points;
  bool empty_coverage_delta{false};

  friend bool operator==(
      const ExactDirectMorseUnifiedResidentGroupRecord&,
      const ExactDirectMorseUnifiedResidentGroupRecord&) = default;
};

enum class ExactDirectMorseUnifiedResidentInitializationDecision
    : std::uint8_t {
  not_certified,
  no_source_plan_not_freshly_verified,
  no_session_budget_rejected,
  no_locator_initialization_rejected,
  no_allocation_failed,
  complete_certified_bounded_resident_session,
  no_normalized_adapter_budget_rejected,
  no_normalized_adapter_construction_rejected,
  no_rank_window_saturated_h0_authority_rejected,
  no_normalized_terminal_order_equal_point_count_unsupported,
  no_strict_facet_closure_configuration_rejected,
};

enum class ExactDirectMorseUnifiedResidentPreparationDecision
    : std::uint8_t {
  not_certified,
  no_session_not_initialized,
  no_plan_exhausted,
  no_authority_budget_exhausted,
  contradiction_locator_state_inconsistent,
  contradiction_unresolved_facet_birth_below_active_level,
  contradiction_unresolved_facet_birth_above_active_level,
  no_facet_miniball_certification_failed,
  no_frozen_batch_rejected,
  no_frozen_batch_verification_rejected,
  no_outstanding_ticket_budget_exhausted,
  no_sparse_delta_budget_exhausted,
  no_prepared_state_rejected,
  no_allocation_failed,
  complete_certified_prepared_batch,
  no_normalized_h0_retraction_authority_for_strictly_earlier_facet,
  no_strict_facet_closure_budget_exhausted,
  no_strict_facet_closure_unresolved,
  contradiction_strict_facet_closure_rejected,
};

enum class ExactDirectMorseUnifiedResidentCommitDecision : std::uint8_t {
  not_certified,
  no_ticket_already_consumed,
  no_foreign_ticket_rejected,
  no_stale_ticket_rejected,
  no_sparse_delta_staging_rejected,
  no_locator_transaction_rejected,
  complete_certified_atomic_batch_commit,
};

class ExactDirectMorseUnifiedResidentPreparedBatch {
 public:
  ExactDirectMorseUnifiedResidentPreparedBatch() noexcept;
  ~ExactDirectMorseUnifiedResidentPreparedBatch();
  ExactDirectMorseUnifiedResidentPreparedBatch(
      ExactDirectMorseUnifiedResidentPreparedBatch&&) noexcept;
  ExactDirectMorseUnifiedResidentPreparedBatch& operator=(
      ExactDirectMorseUnifiedResidentPreparedBatch&&) noexcept;
  ExactDirectMorseUnifiedResidentPreparedBatch(
      const ExactDirectMorseUnifiedResidentPreparedBatch&) = delete;
  ExactDirectMorseUnifiedResidentPreparedBatch& operator=(
      const ExactDirectMorseUnifiedResidentPreparedBatch&) = delete;

  [[nodiscard]] const ExactDirectMorseUnifiedResidentAuthorityBundle&
  authority_bundle() const noexcept;
  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] bool consumed() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectMorseUnifiedResidentPreparedBatch(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectMorseUnifiedResidentSession;
};

struct ExactDirectMorseUnifiedResidentPreparationResult {
  std::optional<ExactDirectMorseUnifiedResidentPreparedBatch> ticket;
  bool no_scientific_state_mutated{false};
  bool strict_pre_batch_bundle_certified{false};
  ExactDirectMorseUnifiedResidentPreparationDecision decision{
      ExactDirectMorseUnifiedResidentPreparationDecision::not_certified};

  [[nodiscard]] bool certified_prepared_batch() const noexcept;
};

struct ExactDirectMorseUnifiedResidentCommitResult {
  std::size_t committed_batch_index{};
  std::size_t committed_epoch{};
  ExactDirectSparsePositiveFacetBatchResult locator_batch{};
  bool ticket_consumed{false};
  bool exactly_one_locator_apply_batch_called{false};
  bool sparse_delta_staged_with_rollback_before_locator{false};
  bool staged_sparse_delta_released_after_locator_commit{false};
  bool cursor_and_epoch_advanced_once{false};
  bool no_scientific_state_mutated_on_failure{false};
  ExactDirectMorseUnifiedResidentCommitDecision decision{
      ExactDirectMorseUnifiedResidentCommitDecision::not_certified};

  [[nodiscard]] bool certified_committed_batch() const noexcept;
};

// This mutable session is deliberately not internally synchronized.  The
// caller must externally serialize every call on one session, including const
// accessors, and every call, move or destruction of tickets issued by it,
// against prepare/commit and each other.  Caller-owned source authorities must
// remain alive and immutable for the complete session lifetime.  References
// and iterators returned by accessors must not be used across a later prepare,
// commit, move or destruction of the session.
class ExactDirectMorseUnifiedResidentSession {
 public:
  static constexpr std::string_view backend =
      direct_morse_unified_resident_session_backend;
  static constexpr std::string_view profile =
      direct_morse_unified_resident_session_profile;
  static constexpr std::string_view mode =
      direct_morse_unified_resident_session_mode;
  static constexpr std::string_view public_status =
      direct_morse_unified_resident_session_public_status;
  static constexpr std::string_view deployment_status =
      direct_morse_unified_resident_session_deployment_status;
  static constexpr std::string_view proof_basis =
      direct_morse_unified_resident_session_proof_basis;

  ExactDirectMorseUnifiedResidentSession() noexcept;
  ~ExactDirectMorseUnifiedResidentSession();
  ExactDirectMorseUnifiedResidentSession(
      ExactDirectMorseUnifiedResidentSession&&) noexcept;
  ExactDirectMorseUnifiedResidentSession& operator=(
      ExactDirectMorseUnifiedResidentSession&&) noexcept;
  ExactDirectMorseUnifiedResidentSession(
      const ExactDirectMorseUnifiedResidentSession&) = delete;
  ExactDirectMorseUnifiedResidentSession& operator=(
      const ExactDirectMorseUnifiedResidentSession&) = delete;

  [[nodiscard]] bool certified_resident_session() const noexcept;
  [[nodiscard]] bool complete() const noexcept;
  [[nodiscard]] bool source_cursor_exhausted() const noexcept;
  [[nodiscard]] std::size_t batch_cursor() const noexcept;
  [[nodiscard]] std::size_t epoch() const noexcept;
  [[nodiscard]] std::size_t source_plan_initial_verification_count()
      const noexcept;
  [[nodiscard]] std::size_t frozen_batch_source_replay_count()
      const noexcept;
  [[nodiscard]] std::size_t frozen_batch_reconstruction_count()
      const noexcept;
  [[nodiscard]] ExactDirectMorseUnifiedResidentSourceKind source_kind()
      const noexcept;
  [[nodiscard]] bool normalized_direct_source_session() const noexcept;
  [[nodiscard]] ExactDirectNormalizedH0ResidentRetractionMode
  normalized_h0_retraction_mode() const noexcept;
  [[nodiscard]] bool normalized_h0_retraction_authority_certified()
      const noexcept;
  [[nodiscard]] const ExactDirectSparseUnifiedLevelPlanResult& plan()
      const noexcept;
  [[nodiscard]] const ExactDirectSparsePositiveFacetLocator& locator()
      const noexcept;
  [[nodiscard]] const std::vector<ExactDirectMorseUnifiedResidentComponentState>&
  component_states() const noexcept;
  [[nodiscard]] const std::vector<ExactDirectMorseUnifiedResidentRootCoverage>&
  root_coverages() const noexcept;
  [[nodiscard]] const std::vector<ExactDirectMorseUnifiedResidentGroupRecord>&
  group_records() const noexcept;

  [[nodiscard]] ExactDirectMorseUnifiedResidentPreparationResult
  prepare_next();
  [[nodiscard]] ExactDirectMorseUnifiedResidentCommitResult commit(
      ExactDirectMorseUnifiedResidentPreparedBatch&& ticket) noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectMorseUnifiedResidentSession(
      std::unique_ptr<Impl>) noexcept;
  friend struct ExactDirectMorseUnifiedResidentInitializationResult;
  friend ExactDirectMorseUnifiedResidentInitializationResult
  initialize_exact_direct_morse_unified_resident_session(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      const ExactDirectSupportTerminalFacade&,
      const ExactDirectMorseEventJournalResult&,
      const ExactDirectSaddleArmSeedBudget&,
      const ExactDirectSaddleArmSeedJournalResult&,
      const ExactDirectClosedSaddleIncidenceBudget&,
      const ExactDirectClosedSaddleIncidenceJournalResult&,
      const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&,
      spatial::LbvhTraversalOrder,
      const ExactDirectSparseSuccessiveIncidenceStarJournalResult&,
      const ExactDirectSparseUnifiedLevelPlanBudget&,
      const ExactDirectSparseUnifiedLevelPlanResult&,
      std::uint64_t,
      const ExactDirectMorseUnifiedResidentSessionBudget&);
  friend ExactDirectMorseUnifiedResidentInitializationResult
  initialize_exact_direct_normalized_h0_resident_session(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      const ExactDirectSupportTerminalFacade&,
      const ExactDirectMorseEventJournalResult&,
      const ExactDirectSaddleArmSeedBudget&,
      const ExactDirectSaddleArmSeedJournalResult&,
      const ExactDirectClosedSaddleIncidenceBudget&,
      const ExactDirectClosedSaddleIncidenceJournalResult&,
      const ExactDirectSparseGatewayCandidateBudget&,
      spatial::LbvhTraversalOrder,
      const ExactDirectSparseGatewayCandidateJournalResult&,
      const ExactDirectNormalizedH0SourcePlanBudget&,
      const ExactDirectNormalizedH0SourcePlanResult&,
      const ExactDirectNormalizedH0ResidentAdapterBudget&,
      std::uint64_t,
      const ExactDirectMorseUnifiedResidentSessionBudget&);
  friend ExactDirectMorseUnifiedResidentInitializationResult
  initialize_exact_direct_normalized_h0_certified_resident_session(
      const spatial::MortonLbvhIndex&,
      const spatial::CanonicalPointCloud&,
      const ExactDirectSupportTerminalFacade&,
      const ExactDirectMorseEventJournalResult&,
      const ExactDirectSaddleArmSeedBudget&,
      const ExactDirectSaddleArmSeedJournalResult&,
      const ExactDirectClosedSaddleIncidenceBudget&,
      const ExactDirectClosedSaddleIncidenceJournalResult&,
      const ExactDirectSparseGatewayCandidateBudget&,
      spatial::LbvhTraversalOrder,
      const ExactDirectSparseGatewayCandidateJournalResult&,
      const ExactDirectNormalizedH0SourcePlanBudget&,
      const ExactDirectNormalizedH0SourcePlanResult&,
      const ExactDirectRankWindowSaturatedH0Authority&,
      const ExactDirectSparseFacetDescentClosureBudget&,
      const ExactDirectSparseFacetDescentClosureConfig&,
      spatial::LbvhTraversalOrder,
      const ExactDirectNormalizedH0ResidentAdapterBudget&,
      std::uint64_t,
      const ExactDirectMorseUnifiedResidentSessionBudget&);
};

struct ExactDirectMorseUnifiedResidentInitializationResult {
  std::optional<ExactDirectMorseUnifiedResidentSession> session;
  std::size_t source_plan_initial_verification_count{};
  bool source_plan_freshly_verified_once{false};
  bool source_plan_owned_by_session{false};
  bool locator_and_component_state_initialized{false};
  ExactDirectMorseUnifiedResidentSourceKind source_kind{
      ExactDirectMorseUnifiedResidentSourceKind::successive_incidence_star};
  bool normalized_source_plan_consumed_directly{false};
  bool normalized_sparse_compatibility_plan_certified{false};
  bool every_normalized_coface_reconstructed_transiently{false};
  ExactDirectNormalizedH0ResidentRetractionMode
      normalized_h0_retraction_mode{
          ExactDirectNormalizedH0ResidentRetractionMode::
              not_applicable_successive_incidence_star};
  bool normalized_h0_retraction_authority_certified{false};
  bool normalized_candidate_fails_open_on_strictly_earlier_facet{false};
  bool rank_window_saturated_h0_authority_freshly_verified{false};
  bool strict_facet_closure_session_capability_issued{false};
  bool successive_incidence_star_materialized_by_adapter{false};
  bool global_regularity_authority_certified{false};
  bool omitted_late_cofaces_qr1_noop_certified{false};
  bool normalized_verticality_certified{false};
  bool no_global_facet_coface_or_gamma_catalog_materialized{false};
  bool public_status_claimed{false};
  ExactDirectMorseUnifiedResidentInitializationDecision decision{
      ExactDirectMorseUnifiedResidentInitializationDecision::not_certified};

  [[nodiscard]] bool certified_initialized_session() const noexcept;
};

// Source objects remain caller-owned and must outlive the session.  The
// verified unified plan itself is copied into the session and becomes its
// immutable cursor authority.
[[nodiscard]] ExactDirectMorseUnifiedResidentInitializationResult
initialize_exact_direct_morse_unified_resident_session(
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
    std::uint64_t session_authority_id,
    const ExactDirectMorseUnifiedResidentSessionBudget& budget);

// Initializes the same sparse locator/frozen quotient/reducer session without
// accepting or constructing a SuccessiveStar.  The normalized source is
// freshly replayed exactly once before the compatibility cursor and locator
// become observable.  Unsupported global-regularity, omitted-no-op,
// verticality and public-exact claims remain false.
[[nodiscard]] ExactDirectMorseUnifiedResidentInitializationResult
initialize_exact_direct_normalized_h0_resident_session(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget,
    const ExactDirectNormalizedH0SourcePlanResult& source_plan,
    const ExactDirectNormalizedH0ResidentAdapterBudget& adapter_budget,
    std::uint64_t session_authority_id,
    const ExactDirectMorseUnifiedResidentSessionBudget& budget);

// Certified normalized H0 carrier mode.  The supplied rank-window authority
// is freshly reconstructed against the already-certified terminal facade.
// Each nonempty set of strict locator misses is then resolved by exactly one
// sparse functional-forest closure against the immutable pre-batch locator
// snapshot.  K=n remains deliberately unsupported.  The closure graph is
// transient; only a private move-only compact attestation survives in the
// prepared ticket, and no Star, Gamma or higher-order Delaunay mosaic is
// materialized.
[[nodiscard]] ExactDirectMorseUnifiedResidentInitializationResult
initialize_exact_direct_normalized_h0_certified_resident_session(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget,
    const ExactDirectNormalizedH0SourcePlanResult& source_plan,
    const ExactDirectRankWindowSaturatedH0Authority&
        source_rank_window_authority,
    const ExactDirectSparseFacetDescentClosureBudget&
        strict_facet_closure_budget,
    const ExactDirectSparseFacetDescentClosureConfig&
        strict_facet_closure_config,
    spatial::LbvhTraversalOrder strict_facet_closure_traversal_order,
    const ExactDirectNormalizedH0ResidentAdapterBudget& adapter_budget,
    std::uint64_t session_authority_id,
    const ExactDirectMorseUnifiedResidentSessionBudget& budget);

}  // namespace morsehgp3d::hierarchy
