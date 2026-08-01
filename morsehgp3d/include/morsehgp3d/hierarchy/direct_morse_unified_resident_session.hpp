#pragma once

#include "morsehgp3d/hierarchy/direct_frozen_unified_incidence_batch.hpp"
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

inline constexpr std::uint32_t
    direct_morse_unified_resident_session_schema_version = 3U;
inline constexpr std::string_view
    direct_morse_unified_resident_session_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_unified_resident_session_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_unified_resident_session_mode =
        "certified_resident_unified_strict_prebatch_authority_and_atomic_"
        "sparse_delta_commit_with_immutable_verified_plan_authority_v3";
inline constexpr std::string_view
    direct_morse_unified_resident_session_public_status = "not_claimed";
inline constexpr std::string_view
    direct_morse_unified_resident_session_deployment_status =
        "bounded_sparse_resident_delta_without_per_batch_plan_replay_v3";
inline constexpr std::string_view
    direct_morse_unified_resident_session_proof_basis =
        "one_initial_unified_plan_verification_resident_sparse_positive_"
        "locator_exact_local_miniball_miss_certification_strict_pre_batch_"
        "frozen_authority_bundle_move_only_epoch_ticket_single_locator_"
        "transaction_with_rollbackable_pre_staged_sparse_delta_and_immutable_"
        "verified_plan_authority_v3";

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
  std::size_t planned_component_union_count{};
  std::size_t planned_equal_binding_count{};
  std::size_t planned_birth_binding_count{};
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
  ExactDirectFrozenUnifiedIncidenceBatchVerification frozen_verification{};
  ExactDirectMorseUnifiedResidentBatchCounters counters{};
  bool locator_snapshot_strictly_pre_batch{false};
  bool every_unresolved_facet_has_fresh_exact_equal_miniball{false};
  bool csr_authorities_share_identity_and_pre_batch_state{false};
  bool frozen_batch_freshly_reconstructed_from_immutable_plan_authority{
      false};
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
  [[nodiscard]] std::size_t batch_cursor() const noexcept;
  [[nodiscard]] std::size_t epoch() const noexcept;
  [[nodiscard]] std::size_t source_plan_initial_verification_count()
      const noexcept;
  [[nodiscard]] std::size_t frozen_batch_source_replay_count()
      const noexcept;
  [[nodiscard]] std::size_t frozen_batch_reconstruction_count()
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
};

struct ExactDirectMorseUnifiedResidentInitializationResult {
  std::optional<ExactDirectMorseUnifiedResidentSession> session;
  std::size_t source_plan_initial_verification_count{};
  bool source_plan_freshly_verified_once{false};
  bool source_plan_owned_by_session{false};
  bool locator_and_component_state_initialized{false};
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

}  // namespace morsehgp3d::hierarchy
