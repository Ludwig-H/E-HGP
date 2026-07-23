#pragma once

#include "morsehgp3d/hierarchy/direct_sparse_gateway_candidate_journal.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_sparse_gateway_clock_schema_version =
    1U;
inline constexpr std::string_view direct_sparse_gateway_clock_backend =
    "reference_cpu";
inline constexpr std::string_view direct_sparse_gateway_clock_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_sparse_gateway_clock_mode =
    "certified";
inline constexpr std::string_view
    direct_sparse_gateway_clock_refinement_status = "partial_refinement";
inline constexpr std::string_view direct_sparse_gateway_clock_public_status =
    "not_claimed";
inline constexpr std::string_view direct_sparse_gateway_clock_proof_basis =
    "fresh_gateway_candidate_and_locator_replay_bounded_scientific_identity_"
    "dense_source_batch_to_historical_locator_prefix_certificate_"
    "conditional_on_separate_external_clock_authority_replay_v1";

inline constexpr std::string_view
    direct_sparse_gateway_candidate_scientific_identity_domain =
        "MorseHGP3D/phase10/direct-sparse-gateway-candidate-journal/"
        "scientific-identity/v1/sha256/";
inline constexpr std::string_view
    direct_sparse_gateway_clock_certificate_digest_domain =
        "MorseHGP3D/phase10/direct-sparse-gateway-clock/certificate/"
        "v1/sha256/";

// The five population caps are checked before any arena record is inspected.
// maximum_exact_level_decimal_byte_count covers the cumulative numerator and
// denominator decimal bytes (not their u64 length prefixes).  The digest-byte
// cap covers the complete canonical payload outside the fixed domain string.
struct ExactDirectSparseGatewayCandidateScientificIdentityBudget {
  std::size_t maximum_deletion_projection_count{};
  std::size_t maximum_facet_token_count{};
  std::size_t maximum_gateway_candidate_count{};
  std::size_t maximum_batch_count{};
  std::size_t maximum_batch_facet_token_index_count{};
  std::size_t maximum_single_exact_level_integer_bit_count{};
  std::size_t maximum_exact_level_decimal_byte_count{};
  std::size_t maximum_digest_payload_byte_count{};

  friend bool operator==(
      const ExactDirectSparseGatewayCandidateScientificIdentityBudget&,
      const ExactDirectSparseGatewayCandidateScientificIdentityBudget&) =
      default;
};

enum class ExactDirectSparseGatewayCandidateScientificIdentityDecision
    : std::uint8_t {
  not_certified,
  no_identity_capacity_overflow,
  no_identity_budget_exhausted,
  no_identity_encoding_rejected,
  complete_certified_scientific_identity,
};

// Canonical encoding v1 uses big-endian u32/u64, one-byte explicit enum tags,
// and one byte (0 or 1) per bool.  Every arena has a one-byte tag followed by
// its u64 record count.  ExactLevel numerator and denominator are canonical
// decimal strings, each prefixed by its u64 byte length.  The identity includes
// the journal schema, an explicit LBVH-order tag, point/source-event counts,
// four upstream CanonicalIds and every field of the five scientific arenas,
// including the complete 10.6 first-incidence audits.  Operational budgets,
// derived facts, decision and scope are excluded.
struct ExactDirectSparseGatewayCandidateScientificIdentityResult {
  std::uint32_t schema_version{direct_sparse_gateway_clock_schema_version};
  ExactDirectSparseGatewayCandidateScientificIdentityBudget requested_budget{};
  std::size_t required_deletion_projection_count{};
  std::size_t required_facet_token_count{};
  std::size_t required_gateway_candidate_count{};
  std::size_t required_batch_count{};
  std::size_t required_batch_facet_token_index_count{};
  std::size_t required_maximum_single_exact_level_integer_bit_count{};
  std::size_t required_exact_level_decimal_byte_count{};
  std::size_t required_digest_payload_byte_count{};
  std::size_t arena_record_scan_count{};
  contract::CanonicalId scientific_identity_digest{};
  bool population_budget_preflight_certified{false};
  bool exact_level_integer_bits_within_budget{false};
  bool exact_level_decimal_bytes_within_budget{false};
  bool digest_payload_bytes_within_budget{false};
  bool canonical_encoding_freshly_replayed{false};
  bool all_five_scientific_arenas_bound{false};
  bool digest_present{false};
  ExactDirectSparseGatewayCandidateScientificIdentityDecision decision{
      ExactDirectSparseGatewayCandidateScientificIdentityDecision::
          not_certified};

  [[nodiscard]] bool certified_identity() const noexcept;

  friend bool operator==(
      const ExactDirectSparseGatewayCandidateScientificIdentityResult&,
      const ExactDirectSparseGatewayCandidateScientificIdentityResult&) =
      default;
};

// This anchor is deliberately not part of the certificate payload.  It names
// the caller-owned authority record whose replay must independently establish
// that the expected digest was committed at the claimed external time.
struct ExactDirectSparseGatewayExternalClockAnchor {
  std::uint64_t authority_id{};
  std::uint64_t replay_token{};
  contract::CanonicalId expected_certificate_digest{};

  friend bool operator==(
      const ExactDirectSparseGatewayExternalClockAnchor&,
      const ExactDirectSparseGatewayExternalClockAnchor&) = default;
};

struct ExactDirectSparseGatewayClockBoundary {
  std::size_t source_batch_index{};
  std::size_t strict_pre_locator_prefix_count{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp historical_locator_stamp{};

  friend bool operator==(
      const ExactDirectSparseGatewayClockBoundary&,
      const ExactDirectSparseGatewayClockBoundary&) = default;
};

// The canonical payload outside its domain is exactly 136 + 92*S bytes:
// u32 schema, two u64 authority fields, a 32-byte source identity, one 76-byte
// final locator stamp, a u64 boundary count, then S records containing two u64
// values and one 76-byte historical stamp.  certificate_digest and
// digest_present are not encoded.  A zero digest is data, never a sentinel;
// digest_present is therefore mandatory for verification.
struct ExactDirectSparseGatewayClockCertificate {
  std::uint32_t schema_version{direct_sparse_gateway_clock_schema_version};
  std::uint64_t authority_id{};
  std::uint64_t replay_token{};
  contract::CanonicalId source_scientific_identity_digest{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp final_locator_stamp{};
  std::vector<ExactDirectSparseGatewayClockBoundary> boundaries;
  contract::CanonicalId certificate_digest{};
  bool digest_present{false};

  friend bool operator==(
      const ExactDirectSparseGatewayClockCertificate&,
      const ExactDirectSparseGatewayClockCertificate&) = default;
};

struct ExactDirectSparseGatewayClockCertificateDigestBudget {
  std::size_t maximum_boundary_count{};
  std::size_t maximum_digest_payload_byte_count{};

  friend bool operator==(
      const ExactDirectSparseGatewayClockCertificateDigestBudget&,
      const ExactDirectSparseGatewayClockCertificateDigestBudget&) = default;
};

enum class ExactDirectSparseGatewayClockCertificateDigestDecision
    : std::uint8_t {
  not_certified,
  no_digest_capacity_overflow,
  no_digest_budget_exhausted,
  complete_certified_certificate_digest,
};

struct ExactDirectSparseGatewayClockCertificateDigestResult {
  std::uint32_t schema_version{direct_sparse_gateway_clock_schema_version};
  ExactDirectSparseGatewayClockCertificateDigestBudget requested_budget{};
  std::size_t required_boundary_count{};
  std::size_t required_digest_payload_byte_count{};
  std::size_t boundary_scan_count{};
  contract::CanonicalId certificate_digest{};
  bool budget_preflight_certified{false};
  bool canonical_encoding_freshly_replayed{false};
  bool digest_present{false};
  ExactDirectSparseGatewayClockCertificateDigestDecision decision{
      ExactDirectSparseGatewayClockCertificateDigestDecision::not_certified};

  [[nodiscard]] bool certified_digest() const noexcept;

  friend bool operator==(
      const ExactDirectSparseGatewayClockCertificateDigestResult&,
      const ExactDirectSparseGatewayClockCertificateDigestResult&) = default;
};

// The boundary and top-level scratch sizes are preflighted before either
// top-level scratch vector
// is allocated.  Boundary scans cover one dense-shape pass and one sorted
// stamp comparison pass.  Sort comparisons are counted at each comparator
// invocation by the deterministic in-place heapsort.  Scratch bytes cover S
// (prefix,source-index) entries plus S extracted prefix values; the nested
// locator verifier and PSTAMP own and report their separate scratch budgets;
// their allocations are intentionally excluded from the top-level byte cap.
struct ExactDirectSparseGatewayClockVerificationBudget {
  std::size_t maximum_boundary_count{};
  std::size_t maximum_boundary_scan_count{};
  std::size_t maximum_sort_comparison_count{};
  std::size_t maximum_sort_scratch_entry_count{};
  std::size_t maximum_prefix_scratch_entry_count{};
  std::size_t maximum_temporary_scratch_byte_count{};
  ExactDirectSparseGatewayCandidateScientificIdentityBudget
      source_identity_budget{};
  ExactDirectSparseGatewayClockCertificateDigestBudget
      certificate_digest_budget{};
  ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget
      locator_structure_budget{};
  ExactDirectSparsePositiveFacetLocatorPrefixStampSweepBudget
      prefix_stamp_sweep_budget{};

  friend bool operator==(
      const ExactDirectSparseGatewayClockVerificationBudget&,
      const ExactDirectSparseGatewayClockVerificationBudget&) = default;
};

enum class ExactDirectSparseGatewayClockVerificationDecision
    : std::uint8_t {
  not_certified,
  no_clock_capacity_overflow,
  no_clock_budget_exhausted,
  no_clock_anchor_rejected,
  no_clock_source_journal_rejected,
  no_clock_locator_structure_rejected,
  no_clock_certificate_shape_rejected,
  no_clock_source_identity_rejected,
  no_clock_certificate_digest_rejected,
  no_clock_sort_budget_exhausted,
  no_clock_prefix_stamp_replay_rejected,
  no_clock_frozen_snapshot_rejected,
  complete_conditional_source_batch_locator_clock_certificate,
};

enum class ExactDirectSparseGatewayClockScope : std::uint8_t {
  unspecified,
  source_gateway_candidate_batches_to_strict_pre_locator_commit_prefixes_relative_to_caller_clock_authority_only,
};

struct ExactDirectSparseGatewayClockVerification {
  static constexpr std::string_view backend =
      direct_sparse_gateway_clock_backend;
  static constexpr std::string_view profile =
      direct_sparse_gateway_clock_profile;
  static constexpr std::string_view mode = direct_sparse_gateway_clock_mode;
  static constexpr std::string_view refinement_status =
      direct_sparse_gateway_clock_refinement_status;
  static constexpr std::string_view public_status =
      direct_sparse_gateway_clock_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_gateway_clock_proof_basis;

  std::uint32_t schema_version{direct_sparse_gateway_clock_schema_version};
  ExactDirectSparseGatewayClockVerificationBudget requested_budget{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_stamp_at_entry{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_stamp_at_exit{};
  std::size_t required_boundary_count{};
  std::size_t required_boundary_scan_count{};
  std::size_t required_sort_scratch_entry_count{};
  std::size_t required_prefix_scratch_entry_count{};
  std::size_t required_temporary_scratch_byte_count{};
  std::size_t boundary_scan_count{};
  std::size_t sort_comparison_count{};
  ExactDirectSparseGatewayCandidateVerification source_journal_verification{};
  ExactDirectSparseGatewayCandidateScientificIdentityResult
      source_identity_result{};
  ExactDirectSparsePositiveFacetLocatorStructuralVerification
      locator_structure_verification{};
  ExactDirectSparseGatewayClockCertificateDigestResult
      certificate_digest_result{};
  ExactDirectSparsePositiveFacetLocatorPrefixStampSweepCounters
      prefix_stamp_sweep_counters{};
  bool boundary_and_scratch_budget_preflight_certified{false};
  bool external_anchor_non_null_and_payload_matched{false};
  bool source_journal_freshly_replayed{false};
  bool locator_structure_freshly_replayed{false};
  bool source_scientific_identity_freshly_replayed{false};
  bool certificate_digest_freshly_replayed{false};
  bool boundaries_dense_and_prefixes_in_history{false};
  bool boundaries_sorted_by_prefix_without_source_monotonicity_assumption{
      false};
  bool every_historical_stamp_freshly_replayed{false};
  bool final_locator_stamp_matches_entry_and_exit{false};
  // This is an absence-of-write fact about this const kernel, not a detector
  // for an unsynchronized concurrent source-journal writer.
  bool source_and_locator_inputs_mutated{false};
  bool external_clock_authority_replayed{false};
  bool conditional_on_caller_clock_authority_replay{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  bool partial_refinement_only{false};
  ExactDirectSparseGatewayClockVerificationDecision decision{
      ExactDirectSparseGatewayClockVerificationDecision::not_certified};
  ExactDirectSparseGatewayClockScope scope{
      ExactDirectSparseGatewayClockScope::unspecified};
  bool result_certified{false};

  [[nodiscard]] bool certified_conditional_clock_binding() const noexcept;

  friend bool operator==(
      const ExactDirectSparseGatewayClockVerification&,
      const ExactDirectSparseGatewayClockVerification&) = default;
};

[[nodiscard]] ExactDirectSparseGatewayCandidateScientificIdentityResult
compute_exact_direct_sparse_gateway_candidate_scientific_identity(
    const ExactDirectSparseGatewayCandidateJournalResult& journal,
    const ExactDirectSparseGatewayCandidateScientificIdentityBudget& budget);

[[nodiscard]] ExactDirectSparseGatewayClockCertificateDigestResult
compute_exact_direct_sparse_gateway_clock_certificate_digest(
    const ExactDirectSparseGatewayClockCertificate& certificate,
    const ExactDirectSparseGatewayClockCertificateDigestBudget& budget);

// Fresh verification intentionally repeats the complete 10.7 and locator
// structural replay.  The final success remains conditional: this kernel can
// match a separate anchor record but cannot replay the caller-owned authority
// named by that record.  The caller must externally freeze both scientific
// inputs for the whole call; stamp comparisons are not synchronization.
[[nodiscard]] ExactDirectSparseGatewayClockVerification
verify_exact_direct_sparse_gateway_clock_certificate(
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
    const ExactDirectSparseGatewayExternalClockAnchor& external_anchor,
    const ExactDirectSparseGatewayClockCertificate& certificate,
    const ExactDirectSparseGatewayClockVerificationBudget& budget);

}  // namespace morsehgp3d::hierarchy
