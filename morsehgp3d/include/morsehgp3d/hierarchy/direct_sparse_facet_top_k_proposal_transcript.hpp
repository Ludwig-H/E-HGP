#pragma once

#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_facet_top_k_proposal_transcript_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_facet_top_k_proposal_transcript_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_facet_top_k_proposal_transcript_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_facet_top_k_proposal_transcript_mode = "proposal_only";
inline constexpr std::string_view
    direct_sparse_facet_top_k_proposal_transcript_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    direct_sparse_facet_top_k_proposal_transcript_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_sparse_facet_top_k_proposal_transcript_proof_basis =
        "externally_frozen_input_bounded_atomic_structural_validation_"
        "strict_full_key_order_unique_"
        "distinct_incumbent_proposals_domain_and_exclusion_revalidation_"
        "deferred_to_exact_point_of_use_v1";

inline constexpr std::size_t
    direct_sparse_facet_top_k_proposal_maximum_candidate_count =
        direct_sparse_positive_facet_maximum_point_count;

// The fixed-width tail must be zero after candidate_point_count.  Candidate
// order is deliberately not scientific: a producer may retain its preferred
// incumbent order, while the validator checks pairwise distinctness.  The
// domain of every candidate and every point-of-use exclusion remain unchecked
// here and must be revalidated before an exact top-k call.
struct ExactDirectSparseFacetTopKProposalRecord {
  ExactDirectSparseFacetKey source_facet_key{};
  std::array<
      spatial::PointId,
      direct_sparse_facet_top_k_proposal_maximum_candidate_count>
      candidate_point_ids{};
  std::size_t candidate_point_count{};

  friend bool operator==(
      const ExactDirectSparseFacetTopKProposalRecord&,
      const ExactDirectSparseFacetTopKProposalRecord&) = default;
};

struct ExactDirectSparseFacetTopKProposalTranscriptMetadata {
  std::size_t source_batch_index{};
  exact::ExactLevel closed_batch_squared_level{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_snapshot_stamp{};

  friend bool operator==(
      const ExactDirectSparseFacetTopKProposalTranscriptMetadata&,
      const ExactDirectSparseFacetTopKProposalTranscriptMetadata&) = default;
};

// The record cap bounds the complete validation scan.  The byte cap bounds
// the physical fixed-width record arena, while the remaining caps expose its
// logical full-key and candidate populations.  No payload allocation occurs
// before every cap and every input record have been validated.
struct ExactDirectSparseFacetTopKProposalTranscriptBudget {
  std::size_t maximum_proposal_record_count{};
  std::size_t maximum_facet_key_point_reference_count{};
  std::size_t maximum_candidate_point_reference_count{};
  std::size_t maximum_payload_byte_count{};
  std::size_t maximum_logical_storage_entry_count{};

  friend bool operator==(
      const ExactDirectSparseFacetTopKProposalTranscriptBudget&,
      const ExactDirectSparseFacetTopKProposalTranscriptBudget&) = default;
};

enum class ExactDirectSparseFacetTopKProposalTranscriptDecision
    : std::uint8_t {
  not_validated,
  complete_empty_proposal_transcript,
  complete_validated_proposal_transcript,
  no_transcript_metadata_rejected,
  no_transcript_input_shape_rejected,
  no_transcript_capacity_overflow,
  no_transcript_budget_exhausted,
};

enum class ExactDirectSparseFacetTopKProposalTranscriptScope
    : std::uint8_t {
  unspecified,
  bounded_structurally_validated_incumbent_proposals_by_full_facet_key_only,
};

struct ExactDirectSparseFacetTopKProposalTranscriptResult {
  static constexpr std::string_view backend =
      direct_sparse_facet_top_k_proposal_transcript_backend;
  static constexpr std::string_view profile =
      direct_sparse_facet_top_k_proposal_transcript_profile;
  static constexpr std::string_view mode =
      direct_sparse_facet_top_k_proposal_transcript_mode;
  static constexpr std::string_view deployment_status =
      direct_sparse_facet_top_k_proposal_transcript_deployment_status;
  static constexpr std::string_view public_status =
      direct_sparse_facet_top_k_proposal_transcript_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_facet_top_k_proposal_transcript_proof_basis;

  std::uint32_t schema_version{
      direct_sparse_facet_top_k_proposal_transcript_schema_version};
  ExactDirectSparseFacetTopKProposalTranscriptBudget requested_budget{};
  ExactDirectSparseFacetTopKProposalTranscriptMetadata metadata{};
  std::size_t input_proposal_record_count{};
  std::size_t required_facet_key_point_reference_count{};
  std::size_t required_candidate_point_reference_count{};
  std::size_t required_payload_byte_count{};
  std::size_t required_logical_storage_entry_count{};
  std::size_t facet_cardinality{};
  std::size_t published_payload_byte_count{};
  std::size_t published_logical_storage_entry_count{};
  std::vector<ExactDirectSparseFacetTopKProposalRecord> proposal_records;

  bool metadata_shape_validated{false};
  bool budget_preflight_completed{false};
  bool budget_preflight_satisfied{false};
  bool every_full_key_validated{false};
  bool homogeneous_facet_cardinality_validated{false};
  bool records_strictly_sorted_by_full_key{false};
  bool full_keys_unique{false};
  bool candidate_counts_within_k{false};
  bool candidate_point_ids_distinct{false};
  bool unused_candidate_slots_zero{false};
  bool input_validation_atomic{false};
  bool payload_published_only_after_full_validation{false};
  bool no_partial_proposal_payload_published{false};

  // These remain false by construction.  A downstream exact query must
  // validate both properties against its own cloud and exclusion set.
  bool candidate_point_domain_validated{false};
  bool candidate_exclusions_validated{false};
  bool exact_top_k_partition_certified{false};
  bool scientific_decision_published{false};
  bool locator_state_mutated{false};
  bool hierarchy_reduction_or_attachment_published{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};
  bool proposal_only{false};

  ExactDirectSparseFacetTopKProposalTranscriptDecision decision{
      ExactDirectSparseFacetTopKProposalTranscriptDecision::not_validated};
  ExactDirectSparseFacetTopKProposalTranscriptScope scope{
      ExactDirectSparseFacetTopKProposalTranscriptScope::unspecified};

  [[nodiscard]] bool complete_proposal_transcript() const noexcept;
  [[nodiscard]] bool certified_atomic_failure() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;

  friend bool operator==(
      const ExactDirectSparseFacetTopKProposalTranscriptResult&,
      const ExactDirectSparseFacetTopKProposalTranscriptResult&) = default;
};

// This validates and owns only a producer transcript.  In particular it does
// not accept a point cloud or an exclusion set and therefore cannot promote a
// proposed candidate to an exact incumbent or a scientific top-k result.  The
// caller must keep the input span immutable and externally synchronized for
// the whole call; validation followed by copy is not a synchronization
// primitive.
[[nodiscard]] ExactDirectSparseFacetTopKProposalTranscriptResult
build_exact_direct_sparse_facet_top_k_proposal_transcript(
    const ExactDirectSparseFacetTopKProposalTranscriptMetadata& metadata,
    std::span<const ExactDirectSparseFacetTopKProposalRecord>
        proposal_records,
    const ExactDirectSparseFacetTopKProposalTranscriptBudget& budget);

}  // namespace morsehgp3d::hierarchy
