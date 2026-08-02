#pragma once

#include "morsehgp3d/hierarchy/direct_frozen_unified_incidence_batch.hpp"
#include "morsehgp3d/hierarchy/direct_normalized_h0_scientific_window_capability.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_normalized_h0_relative_frozen_incidence_batch_schema_version = 1U;
inline constexpr std::string_view
    direct_normalized_h0_relative_frozen_incidence_batch_backend =
        "reference_cpu";
inline constexpr std::string_view
    direct_normalized_h0_relative_frozen_incidence_batch_profile =
        "hgp_reduced";
inline constexpr std::string_view
    direct_normalized_h0_relative_frozen_incidence_batch_mode =
        "scientific_window_relative_frozen_quotient_action_v1";
inline constexpr std::string_view
    direct_normalized_h0_relative_frozen_incidence_batch_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_normalized_h0_relative_frozen_incidence_batch_deployment_status =
        "architecture_only_no_forest_commit_no_vertical_no_durable_restart";

// The caller names stable source handles.  The builder privately translates
// them through the scientifically authenticated window before entering the
// existing frozen quotient/action core, whose indices remain window-local.
struct ExactDirectNormalizedH0StableFacetResolution {
  std::size_t stable_source_facet_token_index{};
  ExactFrozenIncidenceToken token{};
  std::optional<ExactFrozenIncidencePriorRootId> prior_root_id;

  friend bool operator==(
      const ExactDirectNormalizedH0StableFacetResolution&,
      const ExactDirectNormalizedH0StableFacetResolution&) = default;
};

struct ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBudget {
  std::size_t maximum_window_facet_translation_count{};
  std::size_t maximum_stable_resolution_translation_count{};
  ExactDirectFrozenUnifiedIncidenceBatchBudget frozen_batch{};

  friend bool operator==(
      const ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBudget&,
      const ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBudget&) =
      default;
};

enum class ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision
    : std::uint8_t {
  not_built,
  no_scientific_window_capability_rejected,
  no_translation_budget_rejected,
  no_stable_resolution_order_rejected,
  no_stable_resolution_outside_window_rejected,
  no_frozen_batch_rejected,
  no_allocation_failed,
  complete_scientific_window_relative_frozen_batch,
};

class ExactDirectNormalizedH0RelativeFrozenIncidenceBatch {
 public:
  ExactDirectNormalizedH0RelativeFrozenIncidenceBatch() noexcept;
  ~ExactDirectNormalizedH0RelativeFrozenIncidenceBatch();
  ExactDirectNormalizedH0RelativeFrozenIncidenceBatch(
      ExactDirectNormalizedH0RelativeFrozenIncidenceBatch&&) noexcept;
  ExactDirectNormalizedH0RelativeFrozenIncidenceBatch& operator=(
      ExactDirectNormalizedH0RelativeFrozenIncidenceBatch&&) noexcept;
  ExactDirectNormalizedH0RelativeFrozenIncidenceBatch(
      const ExactDirectNormalizedH0RelativeFrozenIncidenceBatch&) = delete;
  ExactDirectNormalizedH0RelativeFrozenIncidenceBatch& operator=(
      const ExactDirectNormalizedH0RelativeFrozenIncidenceBatch&) = delete;

  [[nodiscard]] bool valid() const noexcept;
  [[nodiscard]] std::size_t source_batch_index() const noexcept;
  [[nodiscard]] const contract::CanonicalId& manifest_digest() const noexcept;
  [[nodiscard]] const contract::CanonicalId& batch_identity_digest()
      const noexcept;
  // The subordinate frozen batch keeps window-local facet indices.  This
  // retained bijection makes those indices interpretable after the prepared
  // scientific-window ticket is committed or destroyed.
  [[nodiscard]] std::span<const std::size_t>
  local_to_stable_facet_token_indices() const noexcept;
  [[nodiscard]] const ExactDirectFrozenUnifiedIncidenceBatchResult&
  frozen_batch() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  explicit ExactDirectNormalizedH0RelativeFrozenIncidenceBatch(
      std::unique_ptr<Impl>) noexcept;
  friend class ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder;
};

struct ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuildResult {
  std::optional<ExactDirectNormalizedH0RelativeFrozenIncidenceBatch> batch;
  std::size_t window_facet_translation_count{};
  std::size_t stable_resolution_scan_count{};
  std::size_t stable_to_local_translation_count{};
  bool scientific_window_capability_consumed{false};
  bool required_stable_resolutions_translated_injectively{false};
  bool exactly_one_frozen_batch_construction{false};
  bool quotient_freshly_streaming_verified{false};
  bool action_plan_freshly_streaming_verified{false};
  bool complete_source_plan_materialized{false};
  bool global_facet_coface_incidence_or_gamma_catalog_materialized{false};
  bool resident_forest_or_caller_state_mutated{false};
  bool vertical_maps_complete{false};
  bool public_status_claimed{false};
  ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision decision{
      ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::not_built};

  [[nodiscard]] bool certified_scientific_relative_build() const noexcept;
};

class ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder final {
 public:
  static constexpr std::string_view backend =
      direct_normalized_h0_relative_frozen_incidence_batch_backend;
  static constexpr std::string_view profile =
      direct_normalized_h0_relative_frozen_incidence_batch_profile;
  static constexpr std::string_view mode =
      direct_normalized_h0_relative_frozen_incidence_batch_mode;
  static constexpr std::string_view public_status =
      direct_normalized_h0_relative_frozen_incidence_batch_public_status;
  static constexpr std::string_view deployment_status =
      direct_normalized_h0_relative_frozen_incidence_batch_deployment_status;

  [[nodiscard]] static
      ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuildResult
      build(
          const ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&
              scientific_window,
          std::span<const ExactDirectNormalizedH0StableFacetResolution>
              stable_facet_resolutions,
          std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
              prior_root_coverages,
          std::span<const spatial::PointId>
              prior_root_coverage_point_references,
          std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
              latent_carrier_coverages,
          std::span<const spatial::PointId>
              latent_carrier_coverage_point_references,
          const ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBudget&
              budget);
};

}  // namespace morsehgp3d::hierarchy
