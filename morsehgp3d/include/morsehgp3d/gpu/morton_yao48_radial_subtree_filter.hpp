#pragma once

#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {

inline constexpr std::uint32_t
    morton_yao48_radial_subtree_filter_schema_version = 1U;
inline constexpr std::size_t
    morton_yao48_radial_subtree_filter_cone_count = 48U;
inline constexpr std::size_t
    morton_yao48_radial_subtree_filter_maximum_witness_count = 10U;
inline constexpr std::string_view
    morton_yao48_radial_subtree_filter_backend =
        "cuda_g4_plus_reference_cpu";
inline constexpr std::string_view
    morton_yao48_radial_subtree_filter_profile = "hgp_reduced";
inline constexpr std::string_view
    morton_yao48_radial_subtree_filter_mode =
        "directed_binary64_radial_subtree_proposal_exact_cpu_replay";
inline constexpr std::string_view
    morton_yao48_radial_subtree_filter_deployment_status =
        "architecture_only";
inline constexpr std::string_view
    morton_yao48_radial_subtree_filter_public_status = "not_claimed";

enum class MortonYao48RadialSubtreeDecision : std::uint8_t {
  descend,
  certified_prune,
};

// The 48 banks are supplied by the future tiled frontier.  Every used slot
// is a PointId; unused tail slots are ignored.  This component never visits a
// leaf and never emits a pair.
struct MortonYao48RadialSubtreeQuery {
  std::uint64_t replay_id{};
  std::uint64_t anchor_morton_position{};
  std::uint64_t node_index{};
  std::size_t maximum_closed_rank{2U};
  std::array<std::array<spatial::PointId,
                        morton_yao48_radial_subtree_filter_maximum_witness_count>,
             morton_yao48_radial_subtree_filter_cone_count>
      bank_witness_point_ids{};
  std::array<std::uint8_t,
             morton_yao48_radial_subtree_filter_cone_count>
      bank_witness_counts{};
  // Supplied by the upstream bank receipt.  Each word must conservatively
  // bound that chamber's exact maximum witness distance squared.  The GPU
  // consumes the words directly; only CUDA prune proposals trigger exact CPU
  // replay of the witnesses and verification of these upper bounds.
  std::array<std::uint64_t,
             morton_yao48_radial_subtree_filter_cone_count>
      bank_squared_radius_upper_bits{};

  friend bool operator==(
      const MortonYao48RadialSubtreeQuery&,
      const MortonYao48RadialSubtreeQuery&) = default;
};

struct MortonYao48RadialSubtreeExactReceipt {
  std::uint64_t replay_id{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t anchor_morton_position{};
  spatial::PointId anchor_point_id{};
  std::uint64_t node_index{};
  std::uint64_t leaf_begin{};
  std::uint64_t leaf_end{};
  // Local authenticated width only.  Receipts may overlap and are not
  // additive without a future compositor-owned disjoint partition.
  std::uint64_t authenticated_subtree_leaf_count{};
  std::size_t maximum_closed_rank{};
  exact::ExactLevel exact_aabb_minimum_squared_distance{};
  exact::ExactLevel exact_three_times_maximum_bank_radius{};

  friend bool operator==(
      const MortonYao48RadialSubtreeExactReceipt&,
      const MortonYao48RadialSubtreeExactReceipt&) = default;
};

struct MortonYao48RadialSubtreeDecisionRecord {
  MortonYao48RadialSubtreeQuery query{};
  MortonYao48RadialSubtreeDecision decision{
      MortonYao48RadialSubtreeDecision::descend};
  bool cuda_proposed_prune{};
  bool exact_replay_accepted{};
  std::unique_ptr<MortonYao48RadialSubtreeExactReceipt> exact_receipt;

  MortonYao48RadialSubtreeDecisionRecord() = default;
  MortonYao48RadialSubtreeDecisionRecord(
      MortonYao48RadialSubtreeDecisionRecord&&) noexcept = default;
  MortonYao48RadialSubtreeDecisionRecord& operator=(
      MortonYao48RadialSubtreeDecisionRecord&&) noexcept = default;
  MortonYao48RadialSubtreeDecisionRecord(
      const MortonYao48RadialSubtreeDecisionRecord&) = delete;
  MortonYao48RadialSubtreeDecisionRecord& operator=(
      const MortonYao48RadialSubtreeDecisionRecord&) = delete;
};

struct MortonYao48RadialSubtreeFilterAudit {
  std::uint32_t schema_version{
      morton_yao48_radial_subtree_filter_schema_version};
  std::size_t input_count{};
  std::size_t maximum_query_count{};
  std::size_t device_eligible_input_count{};
  std::size_t device_fail_open_input_count{};
  std::size_t cuda_prune_proposal_count{};
  std::size_t exact_replay_count{};
  std::size_t exact_accepted_prune_count{};
  std::size_t hostile_or_false_proposal_rejected_count{};
  std::size_t exact_witness_distance_evaluation_count{};
  std::size_t exact_cone_recertification_count{};
  std::size_t launcher_call_count{};
  std::size_t cuda_kernel_launch_count{};
  std::size_t cuda_synchronization_count{};
  int cuda_device{-1};
  bool host_fake_launcher_exercised{};
  bool cuda_execution_performed{};
  bool directed_round_down_aabb_recipe_requested{};
  bool directed_round_up_three_radius_recipe_requested{};
  bool equality_proposes_prune{};
  bool all_48_banks_required_full{};
  bool exact_cpu_replay_required_for_every_prune{};
  bool fail_open_on_nonfinite_overflow_or_ambiguity{};
  bool subtree_pruning_implemented{};
  bool leaf_traversal_performed{};
  bool pair_record_emitted{};
  bool product_path_enabled{};
  bool dense_pair_fallback_performed{};
  bool global_pair_matrix_materialized{};
  bool ordinary_delaunay_materialized{};
  bool higher_order_delaunay_mosaic_materialized{};
  bool scientific_frontier_complete_claimed{};
  bool public_status_claimed{};

  friend bool operator==(
      const MortonYao48RadialSubtreeFilterAudit&,
      const MortonYao48RadialSubtreeFilterAudit&) = default;
};

struct MortonYao48RadialSubtreeFilterResult {
  std::vector<MortonYao48RadialSubtreeDecisionRecord> decisions;
  MortonYao48RadialSubtreeFilterAudit audit{};
};

namespace detail {
class Phase15MortonYao48RadialSubtreeFilterState;
}

// Standalone bounded filter for future composition inside the resident
// Morton/Yao48 traversal.  It owns only O(maximum_query_count) scratch.  A
// CUDA proposal is never returned as authority without exact CPU replay.
class MortonYao48RadialSubtreeFilterContext final {
 public:
  explicit MortonYao48RadialSubtreeFilterContext(
      std::size_t maximum_query_count);
  ~MortonYao48RadialSubtreeFilterContext() noexcept;

  MortonYao48RadialSubtreeFilterContext(
      MortonYao48RadialSubtreeFilterContext&&) noexcept;
  MortonYao48RadialSubtreeFilterContext& operator=(
      MortonYao48RadialSubtreeFilterContext&&) noexcept;
  MortonYao48RadialSubtreeFilterContext(
      const MortonYao48RadialSubtreeFilterContext&) = delete;
  MortonYao48RadialSubtreeFilterContext& operator=(
      const MortonYao48RadialSubtreeFilterContext&) = delete;

  [[nodiscard]] MortonYao48RadialSubtreeFilterResult decide(
      const spatial::CanonicalPointCloud& cloud,
      const MortonLbvhDeviceBuildResult& certified_build,
      std::span<const MortonYao48RadialSubtreeQuery> queries);

  [[nodiscard]] std::size_t maximum_query_count() const noexcept {
    return maximum_query_count_;
  }

 private:
  std::shared_ptr<detail::Phase15MortonYao48RadialSubtreeFilterState> state_;
  std::size_t maximum_query_count_{};
};

}  // namespace morsehgp3d::gpu
