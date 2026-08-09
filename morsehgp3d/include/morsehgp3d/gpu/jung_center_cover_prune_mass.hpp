#pragma once

#include "morsehgp3d/gpu/morton_lbvh_build.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string_view>
#include <vector>

namespace morsehgp3d::gpu {
namespace detail {
class Phase15JungCenterCoverPruneMassTraversalAccess;
class Phase15JungCenterCoverPruneMassContextState;
}

inline constexpr std::uint32_t jung_center_cover_prune_mass_schema_version = 2U;
inline constexpr std::string_view jung_center_cover_prune_mass_phase =
    "P15-HOCUDA-P1a";
inline constexpr std::string_view jung_center_cover_prune_mass_backend =
    "cuda_g4_plus_reference_cpu_oracle";
inline constexpr std::string_view jung_center_cover_prune_mass_profile =
    "hgp_reduced";
inline constexpr std::string_view jung_center_cover_prune_mass_mode =
    "proposal_only_center_cover_prune_mass_falsifier_v1";
inline constexpr std::string_view jung_center_cover_prune_mass_deployment_status =
    "prototype_only";
inline constexpr std::string_view jung_center_cover_prune_mass_public_status =
    "not_claimed";
inline constexpr std::size_t jung_center_cover_patch_count = 64U;
inline constexpr std::size_t jung_center_cover_axis_count = 3U;
inline constexpr std::size_t jung_center_cover_support4_witness_threshold = 8U;
inline constexpr std::size_t jung_center_cover_histogram_bin_count = 32U;
inline constexpr std::size_t jung_center_cover_structural_lbvh_height_bound =
    3U * spatial::morton_lbvh_snapshot_morton_bits_per_axis +
    std::numeric_limits<std::size_t>::digits;
inline constexpr std::size_t jung_center_cover_minimum_stack_capacity =
    2U * jung_center_cover_structural_lbvh_height_bound + 1U;

struct JungCenterCoverPruneMassConfig final {
  std::size_t requested_shard_count{2048U};
  std::size_t endpoint_microtile_pair_mass{64U};
  std::size_t stack_capacity_per_shard{
      jung_center_cover_minimum_stack_capacity};
  bool collect_n32_qualification_receipts{false};

  friend bool operator==(
      const JungCenterCoverPruneMassConfig&,
      const JungCenterCoverPruneMassConfig&) = default;
};

enum class JungCenterCoverPruneMassStatus : std::uint8_t {
  complete = 0U,
  malformed_input = 1U,
  stack_capacity_exhausted = 2U,
  execution_failure = 3U,
};

enum class JungCenterCoverTerminalDisposition : std::uint8_t {
  pruned = 0U,
  microtile = 1U,
};

enum class JungCenterCoverEndpointBlockKind : std::uint8_t {
  diagonal = 0U,
  cross = 1U,
};

struct JungCenterCoverEndpointNode final {
  std::uint32_t node_index{};
  std::uint32_t leaf_begin{};
  std::uint32_t leaf_end{};

  friend bool operator==(
      const JungCenterCoverEndpointNode&,
      const JungCenterCoverEndpointNode&) = default;
};

struct JungCenterCoverEndpointBlock final {
  std::uint64_t unordered_pair_mass{};
  JungCenterCoverEndpointNode first{};
  JungCenterCoverEndpointNode second{};
  JungCenterCoverEndpointBlockKind kind{
      JungCenterCoverEndpointBlockKind::diagonal};
  std::array<std::uint8_t, 7U> reserved{};

  friend bool operator==(
      const JungCenterCoverEndpointBlock&,
      const JungCenterCoverEndpointBlock&) = default;
};

// This deliberately large record is legal only for the n<=32 differential.
// Each valid-mask bit authenticates the six binary64 endpoint words of the
// closed patch actually evaluated on device.  The record names witness
// subtrees, not endpoint pairs, and is never allocated by the 50k aggregate
// path.
struct JungCenterCoverQualificationReceipt final {
  JungCenterCoverEndpointBlock block{};
  std::uint64_t patch_bounds_valid_mask{};
  std::uint64_t infeasible_patch_mask{};
  std::uint64_t covered_patch_mask{};
  std::array<
      std::uint64_t,
      jung_center_cover_patch_count * jung_center_cover_axis_count>
      patch_lower_binary64_bits{};
  std::array<
      std::uint64_t,
      jung_center_cover_patch_count * jung_center_cover_axis_count>
      patch_upper_binary64_bits{};
  std::array<std::uint32_t, jung_center_cover_patch_count>
      witness_node_counts{};
  std::array<std::uint64_t, jung_center_cover_patch_count>
      witness_point_masses{};
  std::array<
      std::uint32_t,
      jung_center_cover_patch_count *
          jung_center_cover_support4_witness_threshold>
      witness_node_indices{};
  std::array<
      std::uint64_t,
      jung_center_cover_patch_count *
          jung_center_cover_support4_witness_threshold>
      witness_node_point_masses{};
  std::array<
      std::uint32_t,
      jung_center_cover_patch_count *
          jung_center_cover_support4_witness_threshold>
      witness_node_leaf_begins{};
  std::array<
      std::uint32_t,
      jung_center_cover_patch_count *
          jung_center_cover_support4_witness_threshold>
      witness_node_leaf_ends{};
  JungCenterCoverTerminalDisposition disposition{
      JungCenterCoverTerminalDisposition::microtile};
  std::array<std::uint8_t, 7U> reserved{};

  friend bool operator==(
      const JungCenterCoverQualificationReceipt&,
      const JungCenterCoverQualificationReceipt&) = default;
};

struct JungCenterCoverPruneMassAudit final {
  std::uint32_t schema_version{jung_center_cover_prune_mass_schema_version};
  std::uint64_t source_snapshot_epoch{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t requested_shard_count{};
  std::size_t generated_shard_count{};
  std::size_t worker_cta_count{};
  std::size_t worker_cta_with_work_count{};
  std::size_t stack_capacity_per_shard{};
  std::size_t maximum_stack_high_water{};
  std::size_t endpoint_microtile_pair_mass{};
  std::uint64_t unordered_pair_universe_mass{};
  std::uint64_t pruned_pair_mass{};
  std::uint64_t microtile_pair_mass{};
  std::uint64_t endpoint_block_visit_count{};
  std::uint64_t center_cover_attempt_count{};
  std::uint64_t pruned_block_count{};
  std::uint64_t microtile_block_count{};
  std::uint64_t diagonal_split_count{};
  std::uint64_t cross_split_count{};
  std::uint64_t patch_evaluation_count{};
  std::uint64_t infeasible_patch_count{};
  std::uint64_t covered_patch_count{};
  std::uint64_t unresolved_patch_count{};
  std::uint64_t witness_node_visit_count{};
  std::uint64_t accepted_witness_node_count{};
  std::uint64_t accepted_witness_point_mass{};
  std::uint64_t arithmetic_ambiguity_count{};
  std::uint64_t stack_capacity_failure_count{};
  std::uint64_t qualification_receipt_count{};
  std::uint64_t qualification_receipt_capacity{};
  std::uint64_t maximum_shard_block_visits{};
  std::uint64_t maximum_shard_witness_node_visits{};
  std::array<std::uint64_t, jung_center_cover_histogram_bin_count>
      shard_block_visit_histogram{};
  std::array<std::uint64_t, jung_center_cover_histogram_bin_count>
      shard_witness_visit_histogram{};
  std::size_t physical_device_arena_byte_count{};
  std::size_t source_device_byte_count{};
  std::size_t device_allocation_count{};
  std::size_t device_memset_count{};
  std::size_t cuda_kernel_launch_count{};
  std::size_t cuda_synchronization_count{};
  std::size_t host_to_device_byte_count{};
  std::size_t device_to_host_byte_count{};
  std::uint64_t shard_source_device_nanoseconds{};
  std::uint64_t center_cover_device_nanoseconds{};
  std::uint64_t finalize_device_nanoseconds{};
  std::uint64_t source_cover_device_nanoseconds{};
  std::uint64_t launcher_wall_nanoseconds{};
  bool source_identity_authenticated{false};
  bool source_epoch_authenticated{false};
  bool support4_only{true};
  bool fixed_64_closed_patches_used{true};
  bool directed_binary64_intervals_used{true};
  bool strict_prune_only{true};
  bool fail_open_on_ambiguity{true};
  bool multi_cta_scheduler_used{false};
  bool private_per_shard_dfs_used{false};
  bool exact_mass_identity_satisfied{false};
  bool one_terminal_synchronization_used{false};
  bool qualification_receipts_collected{false};
  bool qualification_patch_bounds_recorded{false};
  bool aggregate_only_50k_path{false};
  bool anchor_or_pair_payload_emitted{false};
  bool endpoint_pair_array_materialized{false};
  bool higher_order_delaunay_mosaic_materialized{false};
  bool scientific_decision_published{false};
  bool scientific_result_claimed{false};
  bool component_only{true};
  bool slo_eligible{false};
  bool warm_e2e_slo_claimed{false};
  bool anchor_completeness_claimed{false};
  bool host_fake_launcher_exercised{false};
  bool cuda_execution_performed{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const JungCenterCoverPruneMassAudit&,
      const JungCenterCoverPruneMassAudit&) = default;
};

struct JungCenterCoverPruneMassResult final {
  JungCenterCoverPruneMassStatus status{
      JungCenterCoverPruneMassStatus::execution_failure};
  JungCenterCoverPruneMassAudit audit{};
  std::vector<JungCenterCoverQualificationReceipt> qualification_receipts;
};

class JungCenterCoverPruneMassContext final {
 public:
  static constexpr std::string_view phase =
      jung_center_cover_prune_mass_phase;
  static constexpr std::string_view backend =
      jung_center_cover_prune_mass_backend;
  static constexpr std::string_view profile =
      jung_center_cover_prune_mass_profile;
  static constexpr std::string_view mode = jung_center_cover_prune_mass_mode;
  static constexpr std::string_view deployment_status =
      jung_center_cover_prune_mass_deployment_status;
  static constexpr std::string_view public_status =
      jung_center_cover_prune_mass_public_status;

  JungCenterCoverPruneMassContext(
      MortonLbvhDeviceTraversalLease&& traversal_lease,
      JungCenterCoverPruneMassConfig config = {});
  ~JungCenterCoverPruneMassContext() noexcept;
  JungCenterCoverPruneMassContext(
      JungCenterCoverPruneMassContext&&) noexcept;
  JungCenterCoverPruneMassContext& operator=(
      JungCenterCoverPruneMassContext&&) noexcept;
  JungCenterCoverPruneMassContext(
      const JungCenterCoverPruneMassContext&) = delete;
  JungCenterCoverPruneMassContext& operator=(
      const JungCenterCoverPruneMassContext&) = delete;

  [[nodiscard]] JungCenterCoverPruneMassResult profile_once();
  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] bool poisoned() const noexcept;
  [[nodiscard]] const JungCenterCoverPruneMassConfig& config() const noexcept {
    return config_;
  }

 private:
  JungCenterCoverPruneMassConfig config_{};
  MortonLbvhDeviceTraversalLeaseAudit source_audit_{};
  std::shared_ptr<detail::Phase15JungCenterCoverPruneMassContextState> state_;
  std::shared_ptr<void> source_owner_;
  std::shared_ptr<const void> source_cloud_identity_;
  std::shared_ptr<const void> source_index_identity_;
  const std::uint64_t* device_coordinate_bits_{};
  const std::uint64_t* device_morton_point_ids_{};
  const void* device_nodes_{};
  int cuda_device_{-1};
  bool host_fake_{false};
  bool consumed_{false};
};

}  // namespace morsehgp3d::gpu
