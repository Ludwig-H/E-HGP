#pragma once

#include "morsehgp3d/gpu/morton_yao48_pair_frontier.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    morton_yao48_tiled_pair_frontier_schema_version = 1U;
inline constexpr std::string_view
    morton_yao48_tiled_pair_frontier_backend = "reference_cpu";
inline constexpr std::string_view
    morton_yao48_tiled_pair_frontier_profile = "hgp_reduced";
inline constexpr std::string_view
    morton_yao48_tiled_pair_frontier_mode =
        "linear_budgeted_morton_yao48_anchor_tiles";
inline constexpr std::string_view
    morton_yao48_tiled_pair_frontier_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    morton_yao48_tiled_pair_frontier_public_status = "not_claimed";
inline constexpr std::string_view
    morton_yao48_tiled_pair_frontier_proof_basis =
        "strict_morton_prefix_pair_ownership_exact_yao48_leaf_and_radial_"
        "subtree_certificates_global_candidate_pruned_unresolved_mass_"
        "conservation_fixed_per_point_work_caps_no_dense_fallback_v1";
inline constexpr bool
    morton_yao48_tiled_pair_frontier_cuda_implementation_available = false;

enum class MortonYao48TiledPairFrontierStatus : std::uint8_t {
  frontier_complete,
  tile_complete,
  censored,
};

enum class MortonYao48TiledPairFrontierStopReason : std::uint8_t {
  none,
  anchor_tile_boundary,
  node_visit_budget,
  candidate_budget,
  witness_bank_receipt_budget,
  certified_prune_region_budget,
};

// All four work/output limits are fixed multiples of n for one context.
// Consequently, a context cannot silently spend quadratic work: a hostile
// cloud is censored with an explicit unresolved mass.  Raising a multiplier
// is an explicit caller policy decision, never an automatic dense fallback.
struct MortonYao48TiledPairFrontierConfig {
  std::size_t maximum_closed_rank{2U};
  std::size_t anchor_tile_capacity{4096U};
  std::size_t maximum_node_visits_per_point{2048U};
  // At K=10, if all 48 chambers fill before a cutoff becomes available, the
  // bank prefill alone may expose as many as 48 * 10 survivor pairs per
  // anchor.  This is an upper exposure bound, not a universal lower bound.
  std::size_t maximum_candidates_per_point{640U};
  std::size_t maximum_witness_bank_receipts_per_point{48U};
  std::size_t maximum_certified_prune_regions_per_point{64U};
  // A transaction is bounded independently of n so one tile can never ask a
  // vector to reserve the entire remaining global budget at massive scale.
  // Zero is a valid fail-closed policy and censors before the first matching
  // record or visit.
  std::size_t maximum_node_visits_per_transaction{4096U * 2048U};
  std::size_t maximum_candidates_per_transaction{4096U * 640U};
  std::size_t maximum_witness_bank_receipts_per_transaction{4096U * 48U};
  std::size_t maximum_certified_prune_regions_per_transaction{4096U * 256U};

  friend bool operator==(
      const MortonYao48TiledPairFrontierConfig&,
      const MortonYao48TiledPairFrontierConfig&) = default;
};

struct MortonYao48TiledPairFrontierAudit {
  std::uint32_t schema_version{
      morton_yao48_tiled_pair_frontier_schema_version};
  std::uint64_t advance_sequence{};
  std::uint64_t source_snapshot_epoch{};
  std::size_t point_count{};
  std::size_t maximum_closed_rank{};
  std::size_t anchor_tile_capacity{};
  std::size_t transaction_anchor_begin{};
  std::size_t transaction_anchor_end{};
  std::size_t transaction_completed_anchor_count{};
  std::uint64_t transaction_node_visit_count{};
  std::uint64_t transaction_candidate_pair_mass{};
  std::uint64_t transaction_certified_pruned_pair_mass{};
  std::size_t transaction_witness_bank_receipt_count{};
  std::size_t transaction_certified_prune_region_count{};
  std::uint64_t unordered_pair_universe_count{};
  std::uint64_t cumulative_candidate_pair_mass{};
  std::uint64_t cumulative_certified_pruned_pair_mass{};
  std::uint64_t unresolved_pair_mass{};
  std::size_t maximum_node_visit_count{};
  std::size_t maximum_candidate_count{};
  std::size_t maximum_witness_bank_receipt_count{};
  std::size_t maximum_certified_prune_region_count{};
  std::size_t maximum_transaction_node_visit_count{};
  std::size_t maximum_transaction_candidate_count{};
  std::size_t maximum_transaction_witness_bank_receipt_count{};
  std::size_t maximum_transaction_certified_prune_region_count{};
  std::uint64_t cumulative_node_visit_count{};
  std::uint64_t cumulative_witness_bank_receipt_count{};
  std::uint64_t cumulative_certified_prune_region_count{};
  bool source_build_authenticated{false};
  bool source_cloud_authenticated{false};
  bool tile_anchor_capacity_enforced{false};
  bool fixed_linear_work_budget_enforced{false};
  bool fixed_transaction_work_budget_enforced{false};
  bool candidate_pruned_unresolved_partition_validated{false};
  bool transaction_records_match_mass_deltas{false};
  // This closes pair coverage only.  Survivors are still proposals and must
  // pass the separate exact diametral-rank predicate.
  bool pair_coverage_partition_complete{false};
  bool terminally_censored{false};
  bool traversal_lease_owner_retained{false};
  bool host_fake_reference_execution{false};
  bool second_host_snapshot_retained{false};
  bool active_witness_bank_storage_bounded_by_48_times_k{false};
  bool radial_subtree_certificates_reused{false};
  bool unfiltered_owned_subtree_emitted{false};
  bool dense_pair_fallback_performed{false};
  bool global_pair_matrix_materialized{false};
  bool higher_order_structure_materialized{false};
  bool exact_diametral_rank_predicate_evaluated{false};
  bool cuda_execution_performed{false};
  bool scientific_pair_catalog_published{false};
  bool scientific_decision_published{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const MortonYao48TiledPairFrontierAudit&,
      const MortonYao48TiledPairFrontierAudit&) = default;
};

struct MortonYao48TiledPairFrontierAdvance {
  MortonYao48TiledPairFrontierStatus status{
      MortonYao48TiledPairFrontierStatus::censored};
  MortonYao48TiledPairFrontierStopReason stop_reason{
      MortonYao48TiledPairFrontierStopReason::node_visit_budget};
  std::vector<MortonYao48PairCandidate> candidates;
  std::vector<MortonYao48WitnessBankReceipt> witness_bank_receipts;
  std::vector<MortonYao48CertifiedPruneRegion> certified_prune_regions;
  MortonYao48TiledPairFrontierAudit audit{};
};

// `frontier_complete` certifies that every unordered pair is represented by
// exactly one survivor or one certified prune region.  It does not accept the
// survivors scientifically and never means that an exact pair catalog has
// been published.

// Serial host/fake executable specification of the tiled compositor.  The
// production CUDA implementation may execute a tile's anchors in parallel,
// but it must preserve this Morton owner, certificate and mass contract.
class MortonYao48TiledPairFrontierContext final {
 public:
  static constexpr std::string_view backend =
      morton_yao48_tiled_pair_frontier_backend;
  static constexpr std::string_view profile =
      morton_yao48_tiled_pair_frontier_profile;
  static constexpr std::string_view mode =
      morton_yao48_tiled_pair_frontier_mode;
  static constexpr std::string_view deployment_status =
      morton_yao48_tiled_pair_frontier_deployment_status;
  static constexpr std::string_view public_status =
      morton_yao48_tiled_pair_frontier_public_status;
  static constexpr std::string_view proof_basis =
      morton_yao48_tiled_pair_frontier_proof_basis;

  MortonYao48TiledPairFrontierContext(
      MortonLbvhDeviceTraversalLease&& traversal_lease,
      MortonYao48TiledPairFrontierConfig config = {});
  ~MortonYao48TiledPairFrontierContext() noexcept = default;

  MortonYao48TiledPairFrontierContext(
      MortonYao48TiledPairFrontierContext&&) noexcept = default;
  MortonYao48TiledPairFrontierContext& operator=(
      MortonYao48TiledPairFrontierContext&&) noexcept = default;
  MortonYao48TiledPairFrontierContext(
      const MortonYao48TiledPairFrontierContext&) = delete;
  MortonYao48TiledPairFrontierContext& operator=(
      const MortonYao48TiledPairFrontierContext&) = delete;

  [[nodiscard]] MortonYao48TiledPairFrontierAdvance advance(
      const MortonLbvhDeviceBuildResult& certified_build,
      const spatial::CanonicalPointCloud& cloud);

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool terminally_censored() const noexcept {
    return terminally_censored_;
  }
  [[nodiscard]] bool poisoned() const noexcept {
    return poisoned_;
  }
  [[nodiscard]] const MortonYao48TiledPairFrontierConfig& config()
      const noexcept {
    return config_;
  }

 private:
  [[nodiscard]] MortonYao48TiledPairFrontierAudit make_audit(
      const MortonYao48PairFrontierAudit& before,
      const MortonYao48PairFrontierAudit& after,
      std::uint64_t transaction_candidate_mass,
      std::uint64_t transaction_pruned_mass,
      std::size_t transaction_receipt_count,
      std::size_t transaction_prune_region_count) const;

  MortonYao48TiledPairFrontierConfig config_{};
  std::size_t point_count_{};
  std::size_t maximum_node_visit_count_{};
  std::size_t maximum_candidate_count_{};
  std::size_t maximum_witness_bank_receipt_count_{};
  std::size_t maximum_certified_prune_region_count_{};
  MortonYao48PairFrontierContext frontier_;
  std::optional<MortonYao48PairFrontierAudit> last_frontier_audit_;
  std::uint64_t advance_sequence_{};
  bool terminally_censored_{false};
  bool poisoned_{false};
  MortonYao48TiledPairFrontierStopReason terminal_stop_reason_{
      MortonYao48TiledPairFrontierStopReason::none};
};

}  // namespace morsehgp3d::gpu
