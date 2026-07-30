#pragma once

#include "morsehgp3d/gpu/morton_yao48_ranked_pair_tile_classifier.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    exact_closed_rank23_pair_terminal_catalog_schema_version = 1U;
inline constexpr std::size_t
    exact_closed_rank23_pair_terminal_catalog_maximum_closed_rank = 3U;
inline constexpr std::size_t
    exact_closed_rank23_pair_terminal_catalog_maximum_linear_capacity_factor =
        4096U;
inline constexpr std::string_view
    exact_closed_rank23_pair_terminal_catalog_backend =
        "cuda_g4_plus_host_fake_contract";
inline constexpr std::string_view
    exact_closed_rank23_pair_terminal_catalog_profile = "hgp_reduced";
inline constexpr std::string_view
    exact_closed_rank23_pair_terminal_catalog_mode =
        "resident_exact_closed_rank23_pair_terminal_catalog";
inline constexpr std::string_view
    exact_closed_rank23_pair_terminal_catalog_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    exact_closed_rank23_pair_terminal_catalog_public_status = "not_claimed";

enum class ExactClosedRank23PairTerminalCatalogStatus : std::uint8_t {
  complete_exact_closed_rank23,
  non_authoritative_host_fake,
  frontier_censored,
  capacity_exhausted,
  exact_predicate_unresolved,
};

// Every resident arena is bounded by point_count times one fixed factor.
// Factors larger than the compile-time ceiling are rejected, so callers
// cannot disguise a dense pair arena as a run-time capacity choice.
struct ExactClosedRank23PairTerminalCatalogConfig {
  std::size_t anchor_tile_capacity{4096U};
  std::size_t catalog_record_capacity_per_point{8U};
  std::size_t payload_point_id_capacity_per_point{32U};
  std::size_t exact_fallback_capacity_per_point{1U};

  friend bool operator==(
      const ExactClosedRank23PairTerminalCatalogConfig&,
      const ExactClosedRank23PairTerminalCatalogConfig&) = default;
};

struct ExactClosedRank23PairTerminalCatalogAudit {
  std::uint32_t schema_version{
      exact_closed_rank23_pair_terminal_catalog_schema_version};
  std::size_t point_count{};
  std::size_t maximum_closed_rank{};
  std::size_t anchor_tile_capacity{};
  std::size_t catalog_record_capacity_per_point{};
  std::size_t payload_point_id_capacity_per_point{};
  std::size_t exact_fallback_capacity_per_point{};
  std::size_t catalog_record_capacity{};
  std::size_t payload_point_id_capacity{};
  std::size_t exact_fallback_capacity{};
  std::size_t frontier_advance_count{};
  std::size_t classifier_commit_count{};
  std::size_t committed_chunk_count{};
  std::size_t catalog_record_count{};
  std::size_t strict_payload_point_id_count{};
  std::size_t shell_payload_point_id_count{};
  std::uint64_t candidate_count{};
  std::uint64_t above_window_count{};
  std::uint64_t unresolved_count{};
  std::uint64_t exact_fallback_count{};
  std::uint64_t certified_pruned_pair_mass{};
  std::uint64_t frontier_unresolved_pair_mass{};
  std::uint64_t unordered_pair_universe_count{};
  MortonYao48DeviceTiledPairFrontierStatus last_frontier_status{
      MortonYao48DeviceTiledPairFrontierStatus::censored};
  MortonYao48DeviceTiledPairFrontierStopReason frontier_stop_reason{
      MortonYao48DeviceTiledPairFrontierStopReason::none};
  MortonYao48RankedPairTileClassifierStatus last_classifier_status{
      MortonYao48RankedPairTileClassifierStatus::capacity_exhausted};
  MortonYao48RankedPairTileClassifierStopReason classifier_stop_reason{
      MortonYao48RankedPairTileClassifierStopReason::none};
  bool fixed_rank23_window_enforced{false};
  bool fixed_linear_capacities_validated{false};
  bool frontier_closed{false};
  bool frontier_mass_reconciled{false};
  bool classifier_mass_reconciled{false};
  bool zero_unresolved_certified{false};
  bool zero_exact_fallback_certified{false};
  bool closed_rank23_pair_catalog_complete{false};
  bool output_withheld{true};
  bool host_fake_lifecycle_exercised{false};
  bool cuda_execution_performed{false};
  bool candidate_device_to_host_performed{false};
  bool certified_prune_device_to_host_performed{false};
  bool callback_per_pair_performed{false};
  bool dense_pair_scan_performed{false};
  bool global_pair_matrix_materialized{false};
  bool ordinary_delaunay_materialized{false};
  bool higher_order_delaunay_mosaic_materialized{false};
  bool gamma2_complete_claimed{false};
  bool k2_complete_claimed{false};
  bool hierarchy_complete_claimed{false};
  bool latency_100ms_claimed{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactClosedRank23PairTerminalCatalogAudit&,
      const ExactClosedRank23PairTerminalCatalogAudit&) = default;
};

struct ExactClosedRank23PairTerminalCatalogResult;

// Move-only terminal authority over the resident rank <= 3 pair catalog.
// This is deliberately not a Gamma_2, K=2, hierarchy, or latency authority.
class ExactClosedRank23PairTerminalCatalog final {
 public:
  ~ExactClosedRank23PairTerminalCatalog() noexcept = default;
  ExactClosedRank23PairTerminalCatalog(
      ExactClosedRank23PairTerminalCatalog&&) noexcept = default;
  ExactClosedRank23PairTerminalCatalog& operator=(
      ExactClosedRank23PairTerminalCatalog&&) noexcept = default;
  ExactClosedRank23PairTerminalCatalog(
      const ExactClosedRank23PairTerminalCatalog&) = delete;
  ExactClosedRank23PairTerminalCatalog& operator=(
      const ExactClosedRank23PairTerminalCatalog&) = delete;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool cuda_resident() const noexcept;
  [[nodiscard]] bool host_fake() const noexcept;
  [[nodiscard]] const ExactClosedRank23PairTerminalCatalogAudit& audit()
      const noexcept {
    return audit_;
  }
  [[nodiscard]] const MortonYao48RankedPairTileCatalogLease&
  resident_catalog() const & noexcept {
    return resident_catalog_;
  }
  [[nodiscard]] const MortonYao48RankedPairTileCatalogLease&
  resident_catalog() const && = delete;

 private:
  ExactClosedRank23PairTerminalCatalog(
      ExactClosedRank23PairTerminalCatalogAudit audit,
      MortonYao48RankedPairTileCatalogLease&& resident_catalog);

  ExactClosedRank23PairTerminalCatalogAudit audit_{};
  MortonYao48RankedPairTileCatalogLease resident_catalog_;

  friend ExactClosedRank23PairTerminalCatalogResult
  build_exact_closed_rank23_pair_terminal_catalog(
      MortonLbvhDeviceTraversalLease&&,
      ExactClosedRank23PairTerminalCatalogConfig);
};

struct ExactClosedRank23PairTerminalCatalogResult {
  ExactClosedRank23PairTerminalCatalogStatus status{
      ExactClosedRank23PairTerminalCatalogStatus::frontier_censored};
  ExactClosedRank23PairTerminalCatalogAudit audit{};
  std::optional<ExactClosedRank23PairTerminalCatalog> catalog;

  ExactClosedRank23PairTerminalCatalogResult() = default;
  ExactClosedRank23PairTerminalCatalogResult(
      ExactClosedRank23PairTerminalCatalogResult&&) noexcept = default;
  ExactClosedRank23PairTerminalCatalogResult& operator=(
      ExactClosedRank23PairTerminalCatalogResult&&) noexcept = default;
  ExactClosedRank23PairTerminalCatalogResult(
      const ExactClosedRank23PairTerminalCatalogResult&) = delete;
  ExactClosedRank23PairTerminalCatalogResult& operator=(
      const ExactClosedRank23PairTerminalCatalogResult&) = delete;

  [[nodiscard]] bool complete() const noexcept {
    return status == ExactClosedRank23PairTerminalCatalogStatus::
                         complete_exact_closed_rank23 &&
           catalog.has_value() && catalog->ready();
  }
};

[[nodiscard]] ExactClosedRank23PairTerminalCatalogResult
build_exact_closed_rank23_pair_terminal_catalog(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    ExactClosedRank23PairTerminalCatalogConfig config = {});

}  // namespace morsehgp3d::gpu
