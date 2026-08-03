#pragma once

#include "morsehgp3d/hierarchy/direct_normalized_h0_scientific_window_capability.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_projectable_contribution_window_schema_version = 1U;
inline constexpr std::string_view
    direct_projectable_contribution_window_backend = "reference_cpu";
inline constexpr std::string_view
    direct_projectable_contribution_window_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_projectable_contribution_window_mode =
        "exact_projectable_contribution_window_v1";
inline constexpr std::string_view
    direct_projectable_contribution_window_deployment_status =
        "architecture_only_one_authenticated_scientific_window_projection_"
        "no_global_catalog_no_aggregation_no_source_or_forest_commit";
inline constexpr std::string_view
    direct_projectable_contribution_window_public_status = "not_claimed";

// Every input traversal, transient arena and published arena is independently
// capped.  Facet-point scans include the complete fixed-width key storage so
// that validation of unused canonical zeros is budgeted too.
struct ExactDirectProjectableContributionWindowBudget {
  std::size_t maximum_window_facet_scan_count{};
  std::size_t maximum_window_facet_point_scan_count{};
  std::size_t maximum_direct_reference_scan_count{};
  std::size_t maximum_residual_reference_scan_count{};
  std::size_t maximum_coface_facet_reference_scan_count{};
  std::size_t maximum_touched_facet_definition_count{};
  std::size_t maximum_facet_point_reference_count{};
  std::size_t maximum_contribution_atom_count{};
  std::size_t maximum_scratch_entry_count{};

  friend bool operator==(
      const ExactDirectProjectableContributionWindowBudget&,
      const ExactDirectProjectableContributionWindowBudget&) = default;
};

enum class ExactDirectProjectableContributionSourceKind : std::uint8_t {
  direct_saddle,
  residual_incidence,
};

// Only facets actually named by a coface-deletion reference are published.
// Definitions are strictly ordered by stable source token and the points live
// in one immutable CSR arena owned by the result.
struct ExactDirectProjectableContributionFacetDefinition {
  std::size_t facet_definition_index{};
  std::size_t stable_source_facet_token_index{};
  std::size_t order{};
  std::size_t point_reference_offset{};
  std::size_t point_reference_count{};

  friend bool operator==(
      const ExactDirectProjectableContributionFacetDefinition&,
      const ExactDirectProjectableContributionFacetDefinition&) = default;
};

// One atom is emitted for every authenticated coface-facet deletion record.
// A residual incidence is retained even when it has no H0 node: contribution
// projection never consults a topological-node predicate.
struct ExactDirectProjectableContributionAtom {
  std::size_t contribution_atom_index{};
  ExactDirectProjectableContributionSourceKind source_kind{
      ExactDirectProjectableContributionSourceKind::direct_saddle};
  std::size_t source_coface_index{};
  std::size_t source_reference_index{};
  std::optional<std::size_t> source_role_record_index;
  std::optional<std::size_t> source_event_projection_index;
  std::optional<std::size_t> source_incidence_family_index;
  std::size_t source_deletion_reference_index{};
  std::size_t removed_union_point_index{};
  spatial::PointId removed_point_id{};
  std::size_t stable_source_facet_token_index{};
  std::size_t facet_definition_index{};
  std::size_t order{};
  exact::ExactLevel exact_squared_radius{};

  friend bool operator==(
      const ExactDirectProjectableContributionAtom&,
      const ExactDirectProjectableContributionAtom&) = default;
};

enum class ExactDirectProjectableContributionWindowDecision
    : std::uint8_t {
  not_built,
  no_scientific_source_stamp_rejected,
  no_scientific_window_rejected,
  no_private_source_or_window_identity_mismatch,
  no_public_source_or_batch_identity_mismatch,
  no_projection_capacity_overflow,
  no_projection_budget_exhausted,
  contradiction_authenticated_window_payload,
  contradiction_batch_or_chain_digest,
  no_projection_allocation_failed,
  complete_exhaustive_authenticated_window_projection,
};

enum class ExactDirectProjectableContributionWindowScope : std::uint8_t {
  unspecified,
  exhaustive_projection_of_one_already_authenticated_scientific_window_only,
};

struct ExactDirectProjectableContributionWindowReceipt {
  std::uint32_t schema_version{
      direct_projectable_contribution_window_schema_version};
  ExactDirectProjectableContributionWindowBudget requested_budget{};
  std::uint64_t scientific_authority_id{};
  contract::CanonicalId manifest_digest{};
  contract::CanonicalId source_identity_digest{};
  contract::CanonicalId horizontal_incidence_authority_digest{};
  contract::CanonicalId source_chain_digest{};
  contract::CanonicalId batch_identity_digest{};
  contract::CanonicalId successor_chain_digest{};
  contract::CanonicalId contribution_window_digest{};
  std::size_t source_batch_index{};
  std::size_t source_future_snapshot_index{};
  std::size_t order{};
  exact::ExactLevel exact_squared_radius{};

  std::size_t required_window_facet_scan_count{};
  std::size_t required_window_facet_point_scan_count{};
  std::size_t required_direct_reference_scan_count{};
  std::size_t required_residual_reference_scan_count{};
  std::size_t required_coface_facet_reference_scan_count{};
  std::size_t required_touched_facet_definition_count{};
  std::size_t required_facet_point_reference_count{};
  std::size_t required_contribution_atom_count{};
  std::size_t required_scratch_entry_count{};
  std::size_t source_direct_coface_count{};
  std::size_t source_residual_coface_count{};
  std::size_t direct_contribution_atom_count{};
  std::size_t residual_contribution_atom_count{};
  std::size_t private_capability_check_count{};
  std::size_t authenticated_payload_full_replay_count{};

  bool scientific_source_stamp_bound{false};
  bool scientific_window_private_identity_bound{false};
  bool manifest_source_batch_and_chain_bound{false};
  bool complete_authenticated_window_payload_replayed{false};
  bool every_coface_facet_reference_projected_once{false};
  bool direct_and_residual_cofaces_retained{false};
  bool source_records_not_filtered_by_h0_node_presence{false};
  bool touched_facet_definitions_canonical_and_deduplicated{false};
  bool facet_point_csr_canonical{false};
  bool every_atom_retains_exact_batch_squared_radius{false};
  bool source_session_window_forest_or_ledger_mutated{false};
  bool global_facet_coface_incidence_or_gamma_catalog_materialized{false};
  bool ordinary_or_higher_order_delaunay_materialized{false};
  bool contribution_aggregation_performed{false};
  bool source_exactness_claimed{false};
  bool vertical_maps_complete{false};
  bool durable_restart_supported{false};
  bool public_status_claimed{false};
  ExactDirectProjectableContributionWindowDecision decision{
      ExactDirectProjectableContributionWindowDecision::not_built};
  ExactDirectProjectableContributionWindowScope scope{
      ExactDirectProjectableContributionWindowScope::unspecified};

  friend bool operator==(
      const ExactDirectProjectableContributionWindowReceipt&,
      const ExactDirectProjectableContributionWindowReceipt&) = default;
};

class ExactDirectProjectableContributionWindowResult {
 public:
  static constexpr std::string_view backend =
      direct_projectable_contribution_window_backend;
  static constexpr std::string_view profile =
      direct_projectable_contribution_window_profile;
  static constexpr std::string_view mode =
      direct_projectable_contribution_window_mode;
  static constexpr std::string_view deployment_status =
      direct_projectable_contribution_window_deployment_status;
  static constexpr std::string_view public_status =
      direct_projectable_contribution_window_public_status;

  ExactDirectProjectableContributionWindowResult() noexcept;
  ~ExactDirectProjectableContributionWindowResult();
  ExactDirectProjectableContributionWindowResult(
      ExactDirectProjectableContributionWindowResult&&) noexcept;
  ExactDirectProjectableContributionWindowResult& operator=(
      ExactDirectProjectableContributionWindowResult&&) noexcept;
  ExactDirectProjectableContributionWindowResult(
      const ExactDirectProjectableContributionWindowResult&) = delete;
  ExactDirectProjectableContributionWindowResult& operator=(
      const ExactDirectProjectableContributionWindowResult&) = delete;

  [[nodiscard]] bool certified_exhaustive_projection() const noexcept;
  [[nodiscard]] const ExactDirectProjectableContributionWindowReceipt&
  receipt() const noexcept;
  [[nodiscard]]
  std::span<const ExactDirectProjectableContributionFacetDefinition>
  facet_definitions() const noexcept;
  [[nodiscard]] std::span<const spatial::PointId> facet_point_references()
      const noexcept;
  [[nodiscard]] std::span<const ExactDirectProjectableContributionAtom>
  contribution_atoms() const noexcept;

 private:
  struct Impl;
  ExactDirectProjectableContributionWindowReceipt receipt_{};
  std::unique_ptr<const Impl> impl_;

  friend class ExactDirectProjectableContributionWindow;
};

class ExactDirectProjectableContributionWindow final {
 public:
  static constexpr std::string_view backend =
      direct_projectable_contribution_window_backend;
  static constexpr std::string_view profile =
      direct_projectable_contribution_window_profile;
  static constexpr std::string_view mode =
      direct_projectable_contribution_window_mode;
  static constexpr std::string_view deployment_status =
      direct_projectable_contribution_window_deployment_status;
  static constexpr std::string_view public_status =
      direct_projectable_contribution_window_public_status;

  [[nodiscard]] static ExactDirectProjectableContributionWindowResult build(
      const ExactDirectNormalizedH0ScientificSourceStamp& source_stamp,
      const ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&
          scientific_window,
      const ExactDirectProjectableContributionWindowBudget& budget) noexcept;
};

}  // namespace morsehgp3d::hierarchy
