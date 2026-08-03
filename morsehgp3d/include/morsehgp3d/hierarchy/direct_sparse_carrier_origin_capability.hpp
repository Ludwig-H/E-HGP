#pragma once

#include "morsehgp3d/hierarchy/direct_normalized_h0_sparse_forest_projection.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_root_ledger.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_stable_facet_descent_closure.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_sparse_carrier_origin_capability_schema_version = 1U;
inline constexpr std::string_view
    direct_sparse_carrier_origin_capability_backend = "reference_cpu";
inline constexpr std::string_view
    direct_sparse_carrier_origin_capability_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_sparse_carrier_origin_capability_mode =
        "exact_sparse_carrier_origin_capability_v1";
inline constexpr std::string_view
    direct_sparse_carrier_origin_capability_deployment_status =
        "architecture_only_reference_cpu_not_massive_qualified";
inline constexpr std::string_view
    direct_sparse_carrier_origin_capability_public_status = "not_claimed";
inline constexpr std::string_view
    direct_sparse_carrier_origin_capability_proof_basis =
        "canonical_bound_seed_subsequence_stable_full_key_closure_terminal_"
        "root_active_root_probe_and_proof_bound_requested_coverage_"
        "materialization_v1";

// The seed index binds one source facet of the current batch to one entry of
// the closure's dense immutable seed projection.  Bindings form a canonical
// strictly-increasing subsequence of closure seed indices; gaps are allowed so
// a coordinator can omit equal-level roots after certifying them separately.
// Stable source facet token indices are also strictly increasing, which is the
// ordering required by the existing relative-batch builder.  This capability
// certifies exactly the bound subsequence and makes no statement about omitted
// closure projections.
// The binding supplies no root, rooted/latent kind, token or point coverage.
struct ExactDirectSparseCarrierOriginSeedBinding {
  std::size_t closure_seed_index{};
  std::size_t stable_source_facet_token_index{};

  friend bool operator==(
      const ExactDirectSparseCarrierOriginSeedBinding&,
      const ExactDirectSparseCarrierOriginSeedBinding&) = default;
};

// All proportional work introduced by this composition is bounded by either
// the two local caps or one of the existing proof/materialization budgets.
// The per-root probe budget is reused for every distinct terminal root.
struct ExactDirectSparseCarrierOriginCapabilityBudget {
  std::size_t maximum_carrier_count{};
  std::size_t maximum_distinct_terminal_root_count{};
  std::size_t maximum_output_point_reference_count{};
  ExactDirectSparseRootLedgerActiveRootProbeBudget active_root_probe{};
  ExactDirectSparseRootCoverageFromActiveRootProofsBudget coverage{};

  friend bool operator==(
      const ExactDirectSparseCarrierOriginCapabilityBudget&,
      const ExactDirectSparseCarrierOriginCapabilityBudget&) = default;
};

enum class ExactDirectSparseCarrierOriginKind : std::uint8_t {
  not_certified,
  latent,
  rooted,
};

// One record remains for every input carrier facet.  Several records may
// share one terminal root and token; the root proof and coverage are then
// performed and stored exactly once.  This producer canonically chooses
// R(r).token_id=r and L(h).token_id=h; downstream code must still treat the
// complete typed token as opaque and must never cast its numeric id back to a
// forest handle.
struct ExactDirectSparseCarrierOriginRecord {
  std::size_t closure_seed_index{};
  std::size_t stable_source_facet_token_index{};
  ExactDirectSparseStableFacetHandle terminal_forest_root_handle{};
  ExactFrozenIncidenceToken token{};
  std::optional<ExactFrozenIncidencePriorRootId> prior_root_id;
  ExactDirectSparseRootCoverageId coverage_id{};
  std::size_t typed_coverage_record_index{};
  ExactDirectSparseCarrierOriginKind kind{
      ExactDirectSparseCarrierOriginKind::not_certified};

  friend bool operator==(
      const ExactDirectSparseCarrierOriginRecord&,
      const ExactDirectSparseCarrierOriginRecord&) = default;
};

struct ExactDirectSparseCarrierOriginCapabilityCounters {
  std::size_t carrier_count{};
  std::size_t distinct_terminal_root_count{};
  std::size_t shared_terminal_reference_count{};
  std::size_t active_root_probe_count{};
  std::size_t rooted_terminal_root_count{};
  std::size_t latent_terminal_root_count{};
  std::size_t output_point_reference_count{};

  friend bool operator==(
      const ExactDirectSparseCarrierOriginCapabilityCounters&,
      const ExactDirectSparseCarrierOriginCapabilityCounters&) = default;
};

enum class ExactDirectSparseCarrierOriginCapabilityDisposition
    : std::uint8_t {
  not_certified,
  complete,
  rejected,
  budget_exhausted,
  contradiction,
};

enum class ExactDirectSparseCarrierOriginCapabilityDecision : std::uint8_t {
  not_certified,
  no_forest_rejected,
  no_closure_rejected,
  no_ledger_rejected,
  no_forest_ledger_stamp_alignment_rejected,
  no_carrier_budget_exhausted,
  no_input_shape_rejected,
  no_distinct_terminal_root_budget_exhausted,
  no_active_root_probe_budget_exhausted,
  no_active_root_unobserved_rejected,
  contradiction_active_root_probe,
  no_coverage_budget_exhausted,
  no_coverage_rejected,
  contradiction_coverage,
  no_output_point_budget_exhausted,
  no_token_namespace_rejected,
  contradiction_stamp_changed,
  no_allocation_failed,
  complete_empty_carrier_set,
  complete_exact_rooted_latent_carrier_origins_and_coverages,
};

class ExactDirectSparseCarrierOriginCapabilityResult final {
 public:
  static constexpr std::string_view backend =
      direct_sparse_carrier_origin_capability_backend;
  static constexpr std::string_view profile =
      direct_sparse_carrier_origin_capability_profile;
  static constexpr std::string_view mode =
      direct_sparse_carrier_origin_capability_mode;
  static constexpr std::string_view deployment_status =
      direct_sparse_carrier_origin_capability_deployment_status;
  static constexpr std::string_view public_status =
      direct_sparse_carrier_origin_capability_public_status;
  static constexpr std::string_view proof_basis =
      direct_sparse_carrier_origin_capability_proof_basis;

  ExactDirectSparseCarrierOriginCapabilityResult() noexcept;
  ~ExactDirectSparseCarrierOriginCapabilityResult();
  ExactDirectSparseCarrierOriginCapabilityResult(
      ExactDirectSparseCarrierOriginCapabilityResult&&) noexcept;
  ExactDirectSparseCarrierOriginCapabilityResult& operator=(
      ExactDirectSparseCarrierOriginCapabilityResult&&) noexcept;
  ExactDirectSparseCarrierOriginCapabilityResult(
      const ExactDirectSparseCarrierOriginCapabilityResult&) = delete;
  ExactDirectSparseCarrierOriginCapabilityResult& operator=(
      const ExactDirectSparseCarrierOriginCapabilityResult&) = delete;

  [[nodiscard]] std::uint32_t schema_version() const noexcept;
  [[nodiscard]] const ExactDirectSparseCarrierOriginCapabilityBudget&
  requested_budget() const & noexcept;
  [[nodiscard]] const ExactDirectSparseCarrierOriginCapabilityBudget&
  requested_budget() const && = delete;
  [[nodiscard]] const ExactDirectSparseStableFacetForestStamp& forest_stamp()
      const & noexcept;
  [[nodiscard]] const ExactDirectSparseStableFacetForestStamp& forest_stamp()
      const && = delete;
  [[nodiscard]] const ExactDirectSparseRootLedgerStamp& ledger_stamp()
      const & noexcept;
  [[nodiscard]] const ExactDirectSparseRootLedgerStamp& ledger_stamp()
      const && = delete;
  [[nodiscard]] const ExactDirectSparseCarrierOriginCapabilityCounters&
  counters() const & noexcept;
  [[nodiscard]] const ExactDirectSparseCarrierOriginCapabilityCounters&
  counters() const && = delete;

  [[nodiscard]] std::span<const ExactDirectSparseCarrierOriginRecord>
  origins() const & noexcept;
  [[nodiscard]] std::span<const ExactDirectSparseCarrierOriginRecord>
  origins() const && = delete;
  [[nodiscard]] std::span<
      const ExactDirectNormalizedH0StableFacetResolution>
  // Carrier-only resolutions.  They are directly compatible with the
  // relative builder's element type but are not a complete batch resolution:
  // equal_facet records must come from a separate scientific authority and be
  // merged canonically by the future coordinator.
  carrier_stable_facet_resolutions() const & noexcept;
  [[nodiscard]] std::span<
      const ExactDirectNormalizedH0StableFacetResolution>
  carrier_stable_facet_resolutions() const && = delete;
  [[nodiscard]] std::span<
      const ExactDirectNormalizedH0SparseForestCarrierAttachment>
  carrier_attachments() const & noexcept;
  [[nodiscard]] std::span<
      const ExactDirectNormalizedH0SparseForestCarrierAttachment>
  carrier_attachments() const && = delete;
  [[nodiscard]] std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
  prior_root_coverages() const & noexcept;
  [[nodiscard]] std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
  prior_root_coverages() const && = delete;
  [[nodiscard]] std::span<const spatial::PointId>
  prior_root_coverage_point_references() const & noexcept;
  [[nodiscard]] std::span<const spatial::PointId>
  prior_root_coverage_point_references() const && = delete;
  [[nodiscard]] std::span<
      const ExactDirectFrozenUnifiedLatentCarrierCoverage>
  latent_carrier_coverages() const & noexcept;
  [[nodiscard]] std::span<
      const ExactDirectFrozenUnifiedLatentCarrierCoverage>
  latent_carrier_coverages() const && = delete;
  [[nodiscard]] std::span<const spatial::PointId>
  latent_carrier_coverage_point_references() const & noexcept;
  [[nodiscard]] std::span<const spatial::PointId>
  latent_carrier_coverage_point_references() const && = delete;

  [[nodiscard]] ExactDirectSparseCarrierOriginCapabilityDisposition
  disposition() const noexcept;
  [[nodiscard]] ExactDirectSparseCarrierOriginCapabilityDecision decision()
      const noexcept;
  [[nodiscard]] bool no_external_root_or_coverage_span_consumed()
      const noexcept;
  [[nodiscard]] bool source_exactness_claimed() const noexcept;
  [[nodiscard]] bool public_status_claimed() const noexcept;
  [[nodiscard]] bool certified_exact_carrier_origin_capability()
      const noexcept;
  [[nodiscard]] bool certified_budget_exhaustion() const noexcept;
  [[nodiscard]] bool certified_rejection() const noexcept;
  [[nodiscard]] bool certified_fail_closed_contradiction() const noexcept;
  [[nodiscard]] bool certified_outcome() const noexcept;
  [[nodiscard]] bool certified_for(
      const ExactDirectSparseStableFacetForestStamp&,
      const ExactDirectSparseRootLedgerStamp&) const noexcept;

 private:
  struct Impl;
  std::unique_ptr<const Impl> impl_;

  explicit ExactDirectSparseCarrierOriginCapabilityResult(
      std::unique_ptr<const Impl>) noexcept;

  friend ExactDirectSparseCarrierOriginCapabilityResult
  derive_exact_direct_sparse_carrier_origin_capability(
      const ExactDirectSparseStableFacetDescentClosureResult&,
      std::span<const ExactDirectSparseCarrierOriginSeedBinding>,
      const ExactDirectSparseStableFacetForest&,
      const ExactDirectSparseRootLedger&,
      const ExactDirectSparseCarrierOriginCapabilityBudget&);
};

// This synchronous composition is not a concurrency primitive.  The caller
// must freeze forest and ledger writers for the complete call.  The function
// consumes no caller-supplied root, rooted/latent classification, token or
// coverage span: all carrier facts are derived from the closure terminal and
// the aligned root ledger.  Equal facets are deliberately outside this
// carrier-only capability, so this output alone is not an exhaustive batch
// resolution.  It does not advance a source cursor or commit either state.
[[nodiscard]] ExactDirectSparseCarrierOriginCapabilityResult
derive_exact_direct_sparse_carrier_origin_capability(
    const ExactDirectSparseStableFacetDescentClosureResult& closure,
    std::span<const ExactDirectSparseCarrierOriginSeedBinding> seed_bindings,
    const ExactDirectSparseStableFacetForest& forest,
    const ExactDirectSparseRootLedger& ledger,
    const ExactDirectSparseCarrierOriginCapabilityBudget& budget);

}  // namespace morsehgp3d::hierarchy
