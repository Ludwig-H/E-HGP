#pragma once

#include "morsehgp3d/hierarchy/direct_normalized_h0_relative_frozen_incidence_batch.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_stable_facet_forest.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_normalized_h0_sparse_forest_projection_schema_version = 1U;
inline constexpr std::string_view
    direct_normalized_h0_sparse_forest_projection_backend = "reference_cpu";
inline constexpr std::string_view
    direct_normalized_h0_sparse_forest_projection_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_normalized_h0_sparse_forest_projection_mode =
        "scientific_window_external_carrier_relative_sparse_forest_"
        "projection_v1";
inline constexpr std::string_view
    direct_normalized_h0_sparse_forest_projection_deployment_status =
        "architecture_only_prepared_forest_ticket_no_source_commit_no_"
        "root_ledger_no_vertical_no_durable_restart";
inline constexpr std::string_view
    direct_normalized_h0_sparse_forest_projection_public_status =
        "not_claimed";

// Token identities remain the complete typed (kind, token_id) pair.  The
// numeric token id is never interpreted as a stable forest handle.  Entries
// must be strictly ordered by typed token and exhaust exactly the rooted and
// latent carriers of the supplied frozen quotient.
struct ExactDirectNormalizedH0SparseForestCarrierAttachment {
  ExactFrozenIncidenceToken token{};
  ExactDirectSparseStableFacetHandle pre_batch_root_handle{};
  std::optional<ExactFrozenIncidencePriorRootId> prior_root_id;

  friend bool operator==(
      const ExactDirectNormalizedH0SparseForestCarrierAttachment&,
      const ExactDirectNormalizedH0SparseForestCarrierAttachment&) = default;
};

// All potentially window-sized scans and scratch arenas are independently
// capped.  Binding and group union capacities are computed before allocation;
// the canonical union capacity is their conservative sum before self-edge and
// duplicate elimination.
struct ExactDirectNormalizedH0SparseForestProjectionBudget {
  std::size_t maximum_window_facet_scan_count{};
  std::size_t maximum_incidence_reference_scan_count{};
  std::size_t maximum_direct_reference_scan_count{};
  std::size_t maximum_group_token_scan_count{};
  std::size_t maximum_carrier_attachment_count{};
  std::size_t maximum_binding_union_count{};
  std::size_t maximum_group_union_count{};
  std::size_t maximum_canonical_union_count{};
  std::size_t maximum_scratch_entry_count{};

  friend bool operator==(
      const ExactDirectNormalizedH0SparseForestProjectionBudget&,
      const ExactDirectNormalizedH0SparseForestProjectionBudget&) = default;
};

enum class ExactDirectNormalizedH0SparseForestProjectionDecision
    : std::uint8_t {
  not_prepared,
  no_scientific_source_stamp_rejected,
  no_scientific_window_rejected,
  no_relative_frozen_batch_rejected,
  no_scientific_identity_or_batch_binding_rejected,
  no_sparse_forest_rejected,
  no_projection_capacity_overflow,
  no_projection_budget_exhausted,
  no_carrier_attachment_order_or_kind_rejected,
  no_carrier_attachment_exhaustiveness_rejected,
  contradiction_carrier_attachment_root_or_prior_root,
  contradiction_window_facet_projection,
  no_sparse_forest_preparation_rejected,
  no_projection_allocation_failed,
  complete_external_attachment_relative_sparse_forest_preparation,
};

enum class ExactDirectNormalizedH0SparseForestProjectionScope
    : std::uint8_t {
  unspecified,
  exact_scientific_window_relative_to_publicly_supplied_external_carrier_attachments_only,
};

struct ExactDirectNormalizedH0SparseForestProjectionReceipt {
  std::uint32_t schema_version{
      direct_normalized_h0_sparse_forest_projection_schema_version};
  ExactDirectNormalizedH0SparseForestProjectionBudget requested_budget{};
  std::uint64_t scientific_authority_id{};
  contract::CanonicalId manifest_digest{};
  contract::CanonicalId source_identity_digest{};
  contract::CanonicalId horizontal_incidence_authority_digest{};
  contract::CanonicalId source_chain_digest{};
  contract::CanonicalId batch_identity_digest{};
  contract::CanonicalId successor_chain_digest{};
  // SHA-256 over exactly the source/batch identity and canonical external
  // attachment premise (typed token, root handle, prior-root presence/value).
  // It is intentionally not a digest of the complete frozen batch.
  contract::CanonicalId external_carrier_attachment_digest{};
  std::size_t source_batch_index{};
  ExactDirectSparseStableFacetForestStamp forest_pre_stamp{};
  contract::CanonicalId forest_prepared_batch_digest{};

  std::size_t required_window_facet_scan_count{};
  std::size_t required_incidence_reference_scan_count{};
  std::size_t required_direct_reference_scan_count{};
  std::size_t required_group_token_scan_count{};
  std::size_t required_carrier_attachment_count{};
  std::size_t required_binding_union_count{};
  std::size_t required_group_union_count{};
  std::size_t required_canonical_union_capacity{};
  std::size_t required_scratch_entry_capacity{};
  std::size_t direct_birth_facet_count{};
  std::size_t equal_facet_count{};
  std::size_t rooted_carrier_attachment_count{};
  std::size_t latent_carrier_attachment_count{};
  std::size_t insertion_request_count{};
  std::size_t canonical_union_request_count{};
  std::size_t q_r_zero_group_count{};
  std::size_t q_r_one_group_count{};
  std::size_t q_r_multiple_group_count{};
  // Exactly the prepared-window and relative-batch private capability seams.
  // Projection never invokes either payload's public full-validation path.
  std::size_t private_capability_check_count{};
  std::size_t payload_full_revalidation_count{};

  bool scientific_source_stamp_bound{false};
  bool scientific_window_and_relative_batch_bound{false};
  bool manifest_source_identity_and_batch_bound{false};
  bool external_carrier_attachment_premise_digest_bound{false};
  bool forest_pre_state_bound{false};
  // The forest stamp binds the exact structure-only pre-state and its source
  // namespace.  It is not evidence of a scientific cursor, batch chain or
  // reduced-root ledger alignment; those remain external attachment facts.
  bool forest_pre_stamp_is_not_scientific_cursor_chain_or_root_ledger_alignment{
      false};
  bool carrier_attachments_canonical_exhaustive_and_rooted{false};
  bool opaque_token_ids_kept_distinct_from_stable_handles{false};
  bool exact_relative_to_external_carrier_attachments{false};
  bool all_fallible_projection_work_completed_before_commit{false};
  bool forest_logical_state_mutated{false};
  bool forest_physical_capacity_may_have_changed{false};
  bool source_session_or_window_committed{false};
  bool source_exactness_claimed{false};
  bool vertical_maps_complete{false};
  bool durable_restart_supported{false};
  bool global_facet_coface_incidence_or_gamma_catalog_materialized{false};
  bool public_status_claimed{false};
  ExactDirectSparseStableFacetForestPreparationDecision forest_decision{
      ExactDirectSparseStableFacetForestPreparationDecision::not_prepared};
  ExactDirectNormalizedH0SparseForestProjectionDecision decision{
      ExactDirectNormalizedH0SparseForestProjectionDecision::not_prepared};
  ExactDirectNormalizedH0SparseForestProjectionScope scope{
      ExactDirectNormalizedH0SparseForestProjectionScope::unspecified};

  friend bool operator==(
      const ExactDirectNormalizedH0SparseForestProjectionReceipt&,
      const ExactDirectNormalizedH0SparseForestProjectionReceipt&) = default;
};

class ExactDirectNormalizedH0SparseForestProjection;

// The receipt fields stay inspectable, but certification also requires a
// private immutable snapshot minted by prepare and the privately held forest
// ticket.  A default object or a publicly rewritten receipt is never enough.
class ExactDirectNormalizedH0SparseForestProjectionResult
    : public ExactDirectNormalizedH0SparseForestProjectionReceipt {
 public:
  static constexpr std::string_view backend =
      direct_normalized_h0_sparse_forest_projection_backend;
  static constexpr std::string_view profile =
      direct_normalized_h0_sparse_forest_projection_profile;
  static constexpr std::string_view mode =
      direct_normalized_h0_sparse_forest_projection_mode;
  static constexpr std::string_view deployment_status =
      direct_normalized_h0_sparse_forest_projection_deployment_status;
  static constexpr std::string_view public_status =
      direct_normalized_h0_sparse_forest_projection_public_status;

  ExactDirectNormalizedH0SparseForestProjectionResult() noexcept;
  ~ExactDirectNormalizedH0SparseForestProjectionResult();
  ExactDirectNormalizedH0SparseForestProjectionResult(
      ExactDirectNormalizedH0SparseForestProjectionResult&&) noexcept;
  ExactDirectNormalizedH0SparseForestProjectionResult& operator=(
      ExactDirectNormalizedH0SparseForestProjectionResult&&) noexcept;
  ExactDirectNormalizedH0SparseForestProjectionResult(
      const ExactDirectNormalizedH0SparseForestProjectionResult&) = delete;
  ExactDirectNormalizedH0SparseForestProjectionResult& operator=(
      const ExactDirectNormalizedH0SparseForestProjectionResult&) = delete;

  [[nodiscard]] bool certified_prepared_projection() const noexcept;
  [[nodiscard]] bool has_forest_ticket() const noexcept;
  [[nodiscard]] const ExactDirectSparseStableFacetForestPreparedBatch*
  forest_ticket() const noexcept;
  [[nodiscard]] std::optional<ExactDirectSparseStableFacetForestPreparedBatch>
  take_forest_ticket() noexcept;

 private:
  struct PrivateAttestation;
  std::unique_ptr<const PrivateAttestation> private_attestation_;
  std::optional<ExactDirectSparseStableFacetForestPreparedBatch>
      forest_ticket_;

  friend class ExactDirectNormalizedH0SparseForestProjection;
};

class ExactDirectNormalizedH0SparseForestProjection final {
 public:
  static constexpr std::string_view backend =
      direct_normalized_h0_sparse_forest_projection_backend;
  static constexpr std::string_view profile =
      direct_normalized_h0_sparse_forest_projection_profile;
  static constexpr std::string_view mode =
      direct_normalized_h0_sparse_forest_projection_mode;
  static constexpr std::string_view deployment_status =
      direct_normalized_h0_sparse_forest_projection_deployment_status;
  static constexpr std::string_view public_status =
      direct_normalized_h0_sparse_forest_projection_public_status;

  [[nodiscard]] static
      ExactDirectNormalizedH0SparseForestProjectionResult
      prepare(
          const ExactDirectNormalizedH0ScientificSourceStamp& source_stamp,
          const ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&
              scientific_window,
          const ExactDirectNormalizedH0RelativeFrozenIncidenceBatch&
              relative_batch,
          ExactDirectSparseStableFacetForest& forest,
          std::span<const
                        ExactDirectNormalizedH0SparseForestCarrierAttachment>
              carrier_attachments,
          const ExactDirectNormalizedH0SparseForestProjectionBudget& budget)
          noexcept;
};

}  // namespace morsehgp3d::hierarchy
