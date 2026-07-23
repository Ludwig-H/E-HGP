#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_gateway_clock.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_gateway_clock_authority_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_gateway_clock_authority_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_gateway_clock_authority_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_gateway_clock_authority_mode = "certified";
inline constexpr std::string_view
    direct_sparse_gateway_clock_authority_refinement_status =
        "partial_refinement";
inline constexpr std::string_view
    direct_sparse_gateway_clock_authority_public_status = "not_claimed";
inline constexpr std::string_view
    direct_sparse_gateway_clock_authority_proof_basis =
        "in_memory_append_only_dense_chronology_domain_separated_sha256_"
        "authority_replay_composed_with_fresh_10_11_clock_verification_v1";

inline constexpr std::string_view
    direct_sparse_gateway_clock_authority_initial_digest_domain =
        "MorseHGP3D/phase10/direct-sparse-gateway-clock-authority/"
        "initial/v1/sha256/";
inline constexpr std::string_view
    direct_sparse_gateway_clock_authority_capture_digest_domain =
        "MorseHGP3D/phase10/direct-sparse-gateway-clock-authority/"
        "capture/v1/sha256/";
inline constexpr std::string_view
    direct_sparse_gateway_clock_authority_seal_digest_domain =
        "MorseHGP3D/phase10/direct-sparse-gateway-clock-authority/"
        "seal/v1/sha256/";

// This milestone is replayable only while this owning object remains alive
// and immutable.  It deliberately defines no filesystem codec, fsync
// protocol, crash recovery, anti-rollback mechanism, or crash-durable claim.
// The trusted orchestrator owns lifecycle uniqueness of the non-zero
// (authority_id, session_id) pair; this in-memory kernel prevents branching by
// deleting copies and invalidating moved-from objects, but it is not a
// cross-process session registry.
struct ExactDirectSparseGatewayClockAuthorityJournalBudget {
  std::size_t maximum_source_batch_count{};
  std::size_t maximum_capture_record_count{};
  std::size_t maximum_source_chronology_entry_count{};
  std::size_t maximum_initial_arena_byte_count{};

  friend bool operator==(
      const ExactDirectSparseGatewayClockAuthorityJournalBudget&,
      const ExactDirectSparseGatewayClockAuthorityJournalBudget&) = default;
};

enum class ExactDirectSparseGatewayClockAuthorityInitializationDecision
    : std::uint8_t {
  not_certified,
  no_authority_identifier_rejected,
  no_authority_source_identity_rejected,
  no_authority_locator_rejected,
  no_authority_capacity_overflow,
  no_authority_budget_exhausted,
  no_authority_allocation_failed,
  complete_certified_empty_authority_journal,
};

struct ExactDirectSparseGatewayClockAuthorityCaptureRecord {
  std::size_t chronological_index{};
  std::size_t source_batch_index{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_snapshot_stamp{};
  contract::CanonicalId previous_chain_digest{};
  contract::CanonicalId chain_digest{};

  friend bool operator==(
      const ExactDirectSparseGatewayClockAuthorityCaptureRecord&,
      const ExactDirectSparseGatewayClockAuthorityCaptureRecord&) = default;
};

enum class ExactDirectSparseGatewayClockAuthorityCaptureDecision
    : std::uint8_t {
  not_certified,
  no_capture_authority_not_initialized,
  no_capture_authority_already_sealed,
  no_capture_source_batch_out_of_range,
  no_capture_source_batch_duplicate,
  no_capture_capacity_exhausted,
  no_capture_locator_not_certified,
  no_capture_stamp_encoding_rejected,
  no_capture_locator_chronology_rejected,
  no_capture_frozen_locator_rejected,
  complete_certified_source_batch_snapshot_capture,
};

struct ExactDirectSparseGatewayClockAuthorityCaptureResult {
  std::uint32_t schema_version{
      direct_sparse_gateway_clock_authority_schema_version};
  std::size_t requested_source_batch_index{};
  std::size_t required_chronological_index{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_stamp_at_entry{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_stamp_at_exit{};
  ExactDirectSparseGatewayClockAuthorityCaptureRecord committed_record{};
  bool authority_initialized{false};
  bool source_batch_in_range_and_previously_absent{false};
  bool capture_capacity_preflight_certified{false};
  bool locator_certified_and_entry_exit_stamp_equal{false};
  bool locator_prefix_chronology_certified{false};
  bool canonical_chain_transition_freshly_computed{false};
  bool chronological_index_dense{false};
  bool source_batch_snapshot_committed{false};
  bool authority_state_mutated{false};
  ExactDirectSparseGatewayClockAuthorityCaptureDecision decision{
      ExactDirectSparseGatewayClockAuthorityCaptureDecision::not_certified};

  [[nodiscard]] bool certified_capture() const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectSparseGatewayClockAuthorityCaptureResult&,
      const ExactDirectSparseGatewayClockAuthorityCaptureResult&) = default;
};

struct ExactDirectSparseGatewayClockAuthoritySealBudget {
  std::size_t maximum_source_batch_count{};
  std::size_t maximum_capture_record_scan_count{};
  std::size_t maximum_certificate_boundary_count{};
  std::size_t maximum_temporary_scratch_byte_count{};
  ExactDirectSparseGatewayCandidateScientificIdentityBudget
      source_identity_budget{};
  ExactDirectSparseGatewayClockCertificateDigestBudget
      certificate_digest_budget{};

  friend bool operator==(
      const ExactDirectSparseGatewayClockAuthoritySealBudget&,
      const ExactDirectSparseGatewayClockAuthoritySealBudget&) = default;
};

enum class ExactDirectSparseGatewayClockAuthoritySealDecision
    : std::uint8_t {
  not_certified,
  no_seal_authority_not_initialized,
  no_seal_already_sealed,
  no_seal_incomplete_or_foreign_source_batches,
  no_seal_capacity_overflow,
  no_seal_budget_exhausted,
  no_seal_allocation_failed,
  no_seal_source_identity_rejected,
  no_seal_authority_history_rejected,
  no_seal_certificate_digest_rejected,
  no_seal_frozen_locator_rejected,
  complete_certified_single_clock_certificate_seal,
};

struct ExactDirectSparseGatewayClockAuthoritySealResult {
  std::uint32_t schema_version{
      direct_sparse_gateway_clock_authority_schema_version};
  ExactDirectSparseGatewayClockAuthoritySealBudget requested_budget{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_stamp_at_entry{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_stamp_at_exit{};
  std::size_t required_source_batch_count{};
  std::size_t required_capture_record_scan_count{};
  std::size_t required_certificate_boundary_count{};
  std::size_t required_temporary_scratch_byte_count{};
  std::size_t capture_record_scan_count{};
  ExactDirectSparseGatewayCandidateScientificIdentityResult
      source_identity_result{};
  ExactDirectSparseGatewayClockCertificateDigestResult
      certificate_digest_result{};
  contract::CanonicalId seal_digest{};
  bool seal_budget_preflight_certified{false};
  bool every_source_batch_present_exactly_once{false};
  bool source_identity_freshly_computed{false};
  bool boundaries_reindexed_by_source_from_chronological_records{false};
  bool final_locator_extends_capture_chronology{false};
  bool certificate_digest_freshly_computed{false};
  bool locator_certified_and_entry_exit_stamp_equal{false};
  bool exactly_one_certificate_committed{false};
  bool authority_state_mutated{false};
  ExactDirectSparseGatewayClockAuthoritySealDecision decision{
      ExactDirectSparseGatewayClockAuthoritySealDecision::not_certified};

  [[nodiscard]] bool certified_seal() const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectSparseGatewayClockAuthoritySealResult&,
      const ExactDirectSparseGatewayClockAuthoritySealResult&) = default;
};

class ExactDirectSparseGatewayClockAuthorityJournal {
 public:
  static constexpr std::string_view backend =
      direct_sparse_gateway_clock_authority_backend;
  static constexpr std::string_view profile =
      direct_sparse_gateway_clock_authority_profile;
  static constexpr std::string_view mode =
      direct_sparse_gateway_clock_authority_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_gateway_clock_authority_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_gateway_clock_authority_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_gateway_clock_authority_proof_basis;

  ExactDirectSparseGatewayClockAuthorityJournal() = default;
  ExactDirectSparseGatewayClockAuthorityJournal(
      const ExactDirectSparseGatewayClockAuthorityJournal&) = delete;
  ExactDirectSparseGatewayClockAuthorityJournal& operator=(
      const ExactDirectSparseGatewayClockAuthorityJournal&) = delete;
  ExactDirectSparseGatewayClockAuthorityJournal(
      ExactDirectSparseGatewayClockAuthorityJournal&& other) noexcept;
  ExactDirectSparseGatewayClockAuthorityJournal& operator=(
      ExactDirectSparseGatewayClockAuthorityJournal&& other) noexcept;

  [[nodiscard]] bool certified_initialized_authority() const noexcept;
  [[nodiscard]] bool certified_sealed_once() const noexcept;

  // The locator must remain externally frozen for the complete call.  The
  // first capture is required to equal the opening stamp; later prefixes are
  // nondecreasing and equal prefixes require equal complete stamps.  The
  // entry/exit stamps detect sequential mutation; they are not synchronization.
  [[nodiscard]] ExactDirectSparseGatewayClockAuthorityCaptureResult
  capture_source_batch(
      std::size_t source_batch_index,
      const ExactDirectSparsePositiveFacetLocator& locator);

  // Sealing is one-shot.  Failure leaves captures and seal state unchanged.
  [[nodiscard]] ExactDirectSparseGatewayClockAuthoritySealResult
  seal_clock_certificate(
      const ExactDirectSparseGatewayCandidateJournalResult& source_journal,
      const ExactDirectSparsePositiveFacetLocator& locator,
      const ExactDirectSparseGatewayClockAuthoritySealBudget& budget);

  [[nodiscard]] std::uint32_t schema_version() const noexcept {
    return schema_version_;
  }
  [[nodiscard]] std::uint64_t authority_id() const noexcept {
    return authority_id_;
  }
  [[nodiscard]] std::uint64_t session_id() const noexcept {
    return session_id_;
  }
  [[nodiscard]] std::uint64_t expected_locator_authority_id() const noexcept {
    return expected_locator_authority_id_;
  }
  [[nodiscard]] const ExactDirectSparsePositiveFacetLocatorSnapshotStamp&
  opening_locator_stamp() const noexcept {
    return opening_locator_stamp_;
  }
  [[nodiscard]] std::size_t source_batch_count() const noexcept {
    return source_batch_count_;
  }
  [[nodiscard]] const contract::CanonicalId&
  source_scientific_identity_digest() const noexcept {
    return source_scientific_identity_digest_;
  }
  [[nodiscard]] const ExactDirectSparseGatewayClockAuthorityJournalBudget&
  budget() const noexcept {
    return budget_;
  }
  [[nodiscard]] const std::vector<
      ExactDirectSparseGatewayClockAuthorityCaptureRecord>&
  capture_records() const noexcept {
    return capture_records_;
  }
  [[nodiscard]] const std::vector<std::size_t>& source_chronology_indices()
      const noexcept {
    return source_chronology_indices_;
  }
  [[nodiscard]] const contract::CanonicalId& initial_chain_digest()
      const noexcept {
    return initial_chain_digest_;
  }
  [[nodiscard]] const contract::CanonicalId& current_capture_chain_digest()
      const noexcept {
    return current_capture_chain_digest_;
  }
  [[nodiscard]] const contract::CanonicalId& seal_digest() const noexcept {
    return seal_digest_;
  }
  [[nodiscard]] bool certificate_present() const noexcept {
    return certificate_present_;
  }
  [[nodiscard]] const ExactDirectSparseGatewayClockCertificate&
  sealed_certificate() const noexcept {
    return sealed_certificate_;
  }
  [[nodiscard]] std::size_t committed_certificate_count() const noexcept {
    return committed_certificate_count_;
  }
  [[nodiscard]] ExactDirectSparseGatewayClockAuthorityInitializationDecision
  initialization_decision() const noexcept {
    return initialization_decision_;
  }

  friend bool operator==(
      const ExactDirectSparseGatewayClockAuthorityJournal&,
      const ExactDirectSparseGatewayClockAuthorityJournal&) = default;

 private:
  friend ExactDirectSparseGatewayClockAuthorityJournal
  build_exact_direct_sparse_gateway_clock_authority_journal(
      std::uint64_t,
      std::uint64_t,
      const ExactDirectSparseGatewayCandidateScientificIdentityResult&,
      const ExactDirectSparsePositiveFacetLocator&,
      const ExactDirectSparseGatewayClockAuthorityJournalBudget&);

  std::uint32_t schema_version_{
      direct_sparse_gateway_clock_authority_schema_version};
  std::uint64_t authority_id_{};
  std::uint64_t session_id_{};
  std::uint64_t expected_locator_authority_id_{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp opening_locator_stamp_{};
  std::size_t source_batch_count_{};
  contract::CanonicalId source_scientific_identity_digest_{};
  ExactDirectSparseGatewayClockAuthorityJournalBudget budget_{};
  std::size_t required_initial_arena_byte_count_{};
  std::vector<ExactDirectSparseGatewayClockAuthorityCaptureRecord>
      capture_records_;
  std::vector<std::size_t> source_chronology_indices_;
  contract::CanonicalId initial_chain_digest_{};
  contract::CanonicalId current_capture_chain_digest_{};
  contract::CanonicalId seal_digest_{};
  ExactDirectSparseGatewayClockCertificate sealed_certificate_{};
  std::size_t committed_certificate_count_{};
  bool initialization_budget_preflight_certified_{false};
  bool dense_source_index_initialized_{false};
  bool opening_locator_stamp_certified_{false};
  bool certificate_present_{false};
  ExactDirectSparseGatewayClockAuthorityInitializationDecision
      initialization_decision_{
          ExactDirectSparseGatewayClockAuthorityInitializationDecision::
              not_certified};

  void invalidate_after_move() noexcept;
};

[[nodiscard]] ExactDirectSparseGatewayClockAuthorityJournal
build_exact_direct_sparse_gateway_clock_authority_journal(
    std::uint64_t authority_id,
    std::uint64_t session_id,
    const ExactDirectSparseGatewayCandidateScientificIdentityResult&
        source_identity,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayClockAuthorityJournalBudget& budget);

struct ExactDirectSparseGatewayClockAuthorityVerificationBudget {
  std::size_t maximum_capture_record_count{};
  std::size_t maximum_capture_record_scan_count{};
  std::size_t maximum_source_presence_entry_count{};
  std::size_t maximum_source_presence_scan_count{};
  std::size_t maximum_temporary_scratch_byte_count{};
  ExactDirectSparseGatewayClockVerificationBudget clock_verification_budget{};

  friend bool operator==(
      const ExactDirectSparseGatewayClockAuthorityVerificationBudget&,
      const ExactDirectSparseGatewayClockAuthorityVerificationBudget&) =
      default;
};

// Independent caller-owned authority for the one sealed in-memory session.
// Replaying the journal is accepted only when its final seal digest matches
// this expected value.  The anchor does not discharge the separate obligation
// to call capture_source_batch at the real scientific pre-lot boundary.
struct ExactDirectSparseGatewayClockAuthorityExternalSealAnchor {
  std::uint64_t authority_id{};
  std::uint64_t session_id{};
  contract::CanonicalId expected_seal_digest{};

  friend bool operator==(
      const ExactDirectSparseGatewayClockAuthorityExternalSealAnchor&,
      const ExactDirectSparseGatewayClockAuthorityExternalSealAnchor&) =
      default;
};

enum class ExactDirectSparseGatewayClockAuthorityVerificationDecision
    : std::uint8_t {
  not_certified,
  no_authority_replay_identifier_rejected,
  no_authority_replay_capacity_overflow,
  no_authority_replay_budget_exhausted,
  no_authority_replay_allocation_failed,
  no_authority_replay_initialization_rejected,
  no_authority_replay_chain_rejected,
  no_authority_replay_source_coverage_rejected,
  no_authority_replay_certificate_rejected,
  no_authority_replay_clock_rejected,
  no_authority_replay_frozen_snapshot_rejected,
  complete_external_clock_authority_replayed,
};

enum class ExactDirectSparseGatewayClockAuthorityVerificationScope
    : std::uint8_t {
  unspecified,
  in_memory_authority_captured_source_batches_to_frozen_locator_prefixes_and_single_10_11_clock_certificate_only,
};

struct ExactDirectSparseGatewayClockAuthorityVerification {
  static constexpr std::string_view backend =
      direct_sparse_gateway_clock_authority_backend;
  static constexpr std::string_view profile =
      direct_sparse_gateway_clock_authority_profile;
  static constexpr std::string_view mode =
      direct_sparse_gateway_clock_authority_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_gateway_clock_authority_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_gateway_clock_authority_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_gateway_clock_authority_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_gateway_clock_authority_schema_version};
  ExactDirectSparseGatewayClockAuthorityVerificationBudget requested_budget{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_stamp_at_entry{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_stamp_at_exit{};
  std::size_t required_capture_record_count{};
  std::size_t required_capture_record_scan_count{};
  std::size_t required_source_presence_entry_count{};
  std::size_t required_source_presence_scan_count{};
  std::size_t required_temporary_scratch_byte_count{};
  std::size_t capture_record_scan_count{};
  std::size_t source_presence_scan_count{};
  contract::CanonicalId replayed_initial_chain_digest{};
  contract::CanonicalId replayed_capture_chain_digest{};
  contract::CanonicalId replayed_seal_digest{};
  ExactDirectSparseGatewayClockCertificateDigestResult
      authority_certificate_digest_result{};
  ExactDirectSparseGatewayClockVerification clock_verification{};
  bool replay_budget_preflight_certified{false};
  bool external_seal_anchor_matched{false};
  bool initialization_state_freshly_replayed{false};
  bool opening_locator_stamp_bound_and_freshly_replayed{false};
  bool chronological_indices_dense{false};
  bool locator_prefix_chronology_nondecreasing_and_equal_prefix_stamp_stable{
      false};
  bool source_batches_present_exactly_once_in_arbitrary_capture_order{false};
  bool every_capture_chain_transition_freshly_replayed{false};
  bool every_boundary_matches_captured_stamp_by_source{false};
  bool final_locator_stamp_freshly_matched{false};
  bool certificate_digest_freshly_matched{false};
  bool single_seal_digest_freshly_replayed{false};
  bool conditional_clock_certificate_freshly_verified{false};
  // This only reports equal entry/exit observations and absence of a write
  // path in this const kernel.  It is not evidence that a synchronization
  // primitive was acquired or replayed.
  bool journal_and_locator_inputs_mutated{false};
  bool in_memory_replay_only{true};
  bool crash_durable{false};
  bool external_clock_authority_replayed{false};
  bool conditional_on_caller_clock_authority_replay{true};
  bool conditional_on_caller_strict_pre_lot_orchestration{true};
  bool external_freeze_synchronization_replayed{false};
  bool conditional_on_caller_external_freeze_synchronization{true};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{true};
  ExactDirectSparseGatewayClockAuthorityVerificationDecision decision{
      ExactDirectSparseGatewayClockAuthorityVerificationDecision::
          not_certified};
  ExactDirectSparseGatewayClockAuthorityVerificationScope scope{
      ExactDirectSparseGatewayClockAuthorityVerificationScope::unspecified};
  bool result_certified{false};

  [[nodiscard]] bool certified_external_clock_binding() const noexcept;

  friend bool operator==(
      const ExactDirectSparseGatewayClockAuthorityVerification&,
      const ExactDirectSparseGatewayClockAuthorityVerification&) = default;
};

// Fresh replay of the authority chain followed by the complete 10.11-CLOCK
// verifier.  The independent seal anchor supplies the trusted authority and
// session identities and the expected final authority digest; session_id
// becomes CLOCK's replay_token.  Only this outer result may set
// external_clock_authority_replayed=true and clear the CLOCK-authority
// conditional flag.  Strict pre-lot orchestration and external freeze
// synchronization remain explicit caller premises.
[[nodiscard]] ExactDirectSparseGatewayClockAuthorityVerification
verify_exact_direct_sparse_gateway_clock_authority_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& trusted_source_budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& observed_source,
    std::size_t trusted_component_handle_count,
    const ExactDirectSparsePositiveFacetLocatorBudget& trusted_locator_budget,
    const ExactDirectSparsePositiveFacetLocatorConfig& trusted_locator_config,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayClockAuthorityExternalSealAnchor&
        external_seal_anchor,
    const ExactDirectSparseGatewayClockAuthorityJournalBudget&
        trusted_authority_budget,
    const ExactDirectSparseGatewayClockAuthorityJournal& observed_authority,
    const ExactDirectSparseGatewayClockAuthorityVerificationBudget& budget);

}  // namespace morsehgp3d::hierarchy
