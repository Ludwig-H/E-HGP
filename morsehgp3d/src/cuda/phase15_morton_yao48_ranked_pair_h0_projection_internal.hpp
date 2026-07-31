#pragma once

#include "morsehgp3d/gpu/morton_yao48_ranked_pair_h0_projection.hpp"

#include "phase15_exact_pair_level_fixed.cuh"
#include "phase15_morton_yao48_ranked_pair_tile_classifier_internal.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace morsehgp3d::gpu::detail {

enum class Phase15MortonYao48RankedPairH0RecordKind : std::uint8_t {
  regular_pair_candidate = 0U,
  diagnostic_or_carrier_candidate = 1U,
};

struct alignas(16) Phase15MortonYao48RankedPairH0Record {
  exact_pair_level_fixed::CanonicalLevel256 squared_level{};
  std::uint64_t support_u{};
  std::uint64_t support_v{};
  std::uint64_t strict_begin{};
  std::uint64_t strict_end{};
  std::uint64_t shell_begin{};
  std::uint64_t shell_end{};
  std::uint8_t closed_rank{};
  Phase15MortonYao48RankedPairH0RecordKind kind{
      Phase15MortonYao48RankedPairH0RecordKind::
          diagnostic_or_carrier_candidate};
  std::uint16_t reserved_zero16{};
  std::uint32_t reserved_zero32{};
};

static_assert(
    std::is_trivially_copyable_v<
        Phase15MortonYao48RankedPairH0Record>);
static_assert(
    std::is_standard_layout_v<Phase15MortonYao48RankedPairH0Record>);

struct Phase15MortonYao48RankedPairH0ProjectionPrivateViews {
  std::shared_ptr<void> output_owner;
  std::shared_ptr<MortonYao48RankedPairTileCatalogLease> source_catalog;
  const Phase15MortonYao48RankedPairH0Record* device_records{};
  const std::uint64_t* device_coordinate_bits{};
  const std::uint64_t* device_strict_point_ids{};
  const std::uint64_t* device_shell_point_ids{};
  std::size_t point_count{};
  std::size_t record_count{};
  std::size_t strict_point_id_count{};
  std::size_t shell_point_id_count{};
  int cuda_device{-1};
  bool host_fake{false};
  bool ready{false};
};

// The only raw-view capability for a future resident radix adapter.  Public
// callers cannot copy records or borrowed payloads to the host.
class Phase15MortonYao48RankedPairH0ProjectionPrivateViewAccess final {
 public:
  [[nodiscard]] static
      Phase15MortonYao48RankedPairH0ProjectionPrivateViews inspect(
          const MortonYao48RankedPairH0ProjectionLease& lease) noexcept;
};

enum class Phase15MortonYao48RankedPairH0ProjectionExecutionKind
    : std::uint8_t {
  host_fake = 0U,
  cuda = 1U,
};

enum class Phase15MortonYao48RankedPairH0ProjectionFailureCode
    : std::uint8_t {
  none = 0U,
  exact_level_unresolved = 1U,
  malformed_source = 2U,
  internal_invariant = 3U,
};

struct Phase15MortonYao48RankedPairH0ProjectionRequest {
  MortonYao48RankedPairH0ProjectionConfig config{};
  const std::uint64_t* coordinate_bits{};
  const std::uint64_t* support_u{};
  const std::uint64_t* support_v{};
  const std::uint8_t* closed_rank{};
  const std::uint64_t* strict_offsets{};
  const std::uint64_t* strict_point_ids{};
  const std::uint64_t* shell_offsets{};
  const std::uint64_t* shell_point_ids{};
  std::size_t point_count{};
  std::size_t maximum_closed_rank{};
  std::size_t record_count{};
  std::size_t strict_point_id_count{};
  std::size_t shell_point_id_count{};
  std::size_t record_capacity{};
  int cuda_device{-1};
  bool host_fake{false};
};

static_assert(
    std::is_trivially_copyable_v<
        Phase15MortonYao48RankedPairH0ProjectionRequest>);

struct Phase15MortonYao48RankedPairH0ProjectionReceipt {
  std::shared_ptr<void> output_owner;
  const Phase15MortonYao48RankedPairH0Record* records{};
  std::size_t point_count{};
  std::size_t maximum_closed_rank{};
  std::size_t record_capacity{};
  std::size_t source_record_count{};
  std::size_t projected_record_count{};
  std::size_t regular_pair_candidate_count{};
  std::size_t diagnostic_or_carrier_candidate_count{};
  std::size_t strict_point_id_count{};
  std::size_t shell_point_id_count{};
  std::size_t exact_level_unresolved_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::size_t scalar_control_device_to_host_count{};
  int cuda_device{-1};
  Phase15MortonYao48RankedPairH0ProjectionExecutionKind execution_kind{
      Phase15MortonYao48RankedPairH0ProjectionExecutionKind::host_fake};
  Phase15MortonYao48RankedPairH0ProjectionFailureCode failure_code{
      Phase15MortonYao48RankedPairH0ProjectionFailureCode::none};
  bool fixed_linear_capacity_validated{false};
  bool one_record_per_source_validated{false};
  bool exact_level_payload_constructed{false};
  bool exact_levels_computed_from_source_coordinates{false};
  bool strict_and_shell_payload_ranges_validated{false};
  bool regular_pair_closed_rank_identity_validated{false};
  bool scientific_birth_and_saddle_roles_withheld{false};
  bool intermediate_record_device_to_host_performed{false};
  bool payload_device_to_host_performed{false};
  bool callback_per_pair_performed{false};
  bool dense_pair_scan_performed{false};
  bool global_pair_matrix_materialized{false};
  bool ordinary_delaunay_materialized{false};
  bool higher_order_delaunay_mosaic_materialized{false};
  bool scientific_decision_published{false};
  bool public_status_claimed{false};
};

[[nodiscard]] Phase15MortonYao48RankedPairH0ProjectionReceipt
phase15_launch_morton_yao48_ranked_pair_h0_projection(
    const Phase15MortonYao48RankedPairH0ProjectionRequest& request);

}  // namespace morsehgp3d::gpu::detail
