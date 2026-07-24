#include "morsehgp3d/gpu/morton_lbvh_build.hpp"

#include "../cuda/phase14_morton_lbvh_build_internal.hpp"

#include "morsehgp3d/spatial/point_cloud_aabb.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

constexpr std::size_t kAxisCount = 3U;
constexpr std::size_t kDeviceRangeByteSize =
    3U * sizeof(std::uint64_t);
constexpr std::size_t kDeviceControlByteSize =
    4U * sizeof(std::uint64_t);
constexpr std::uint64_t kMortonGridSize =
    std::uint64_t{1}
    << spatial::morton_lbvh_snapshot_morton_bits_per_axis;
constexpr std::uint64_t kMaximumMortonCoordinate =
    kMortonGridSize - UINT64_C(1);

[[nodiscard]] std::size_t checked_sum(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_node_count(std::size_t point_count) {
  if (point_count == 0U ||
      point_count >
          std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::length_error(
        "the Phase 14 Morton LBVH node count overflows size_t");
  }
  return point_count * 2U - 1U;
}

[[nodiscard]] spatial::PointId checked_point_id(
    std::size_t point_index) {
  if (!std::in_range<spatial::PointId>(point_index) ||
      static_cast<spatial::PointId>(point_index) >
          spatial::CanonicalPointCloud::max_point_id) {
    throw std::length_error(
        "a Phase 14 Morton LBVH point does not fit PointId");
  }
  return static_cast<spatial::PointId>(point_index);
}

[[nodiscard]] std::uint64_t exact_quantized_coordinate(
    const exact::ExactRational& coordinate,
    const exact::ExactRational& lower,
    const exact::ExactRational& upper) {
  if (lower == upper) {
    return 0U;
  }
  if (coordinate < lower || coordinate > upper) {
    throw std::logic_error(
        "a Phase 14 Morton coordinate lies outside the exact global AABB");
  }
  const exact::ExactRational ratio =
      (coordinate - lower) / (upper - lower);
  const exact::BigInt scaled_numerator =
      ratio.numerator() * kMortonGridSize;
  const exact::BigInt bin =
      scaled_numerator / ratio.denominator();
  if (bin < 0 || bin > kMortonGridSize) {
    throw std::logic_error(
        "a Phase 14 exact Morton bin lies outside its grid");
  }
  if (bin == kMortonGridSize) {
    return kMaximumMortonCoordinate;
  }
  return bin.convert_to<std::uint64_t>();
}

[[nodiscard]] std::uint64_t interleaved_morton_code(
    const std::array<std::uint64_t, 3>& coordinates) noexcept {
  std::uint64_t code = 0U;
  for (std::size_t bit = 0U;
       bit < spatial::morton_lbvh_snapshot_morton_bits_per_axis;
       ++bit) {
    const std::size_t output_bit = 3U * bit;
    code |= ((coordinates[0U] >> bit) & UINT64_C(1))
            << (output_bit + 2U);
    code |= ((coordinates[1U] >> bit) & UINT64_C(1))
            << (output_bit + 1U);
    code |= ((coordinates[2U] >> bit) & UINT64_C(1))
            << output_bit;
  }
  return code;
}

[[nodiscard]] bool leaf_less(
    const spatial::MortonLbvhSnapshotLeaf& left,
    const spatial::MortonLbvhSnapshotLeaf& right) noexcept {
  if (left.morton_code != right.morton_code) {
    return left.morton_code < right.morton_code;
  }
  return left.point_id < right.point_id;
}

[[nodiscard]] MortonLbvhDeviceBuildAudit initialize_audit(
    std::size_t maximum_point_count,
    std::size_t maximum_axis_count,
    std::size_t maximum_node_count,
    std::size_t point_count) {
  MortonLbvhDeviceBuildAudit audit;
  audit.maximum_point_count = maximum_point_count;
  audit.maximum_axis_count = maximum_axis_count;
  audit.maximum_node_count = maximum_node_count;
  audit.host_coordinate_word_capacity = maximum_axis_count;
  audit.host_encoded_bin_capacity = maximum_axis_count;
  audit.host_morton_code_capacity = maximum_point_count;
  audit.host_snapshot_leaf_capacity = maximum_point_count;
  audit.host_snapshot_node_capacity = maximum_node_count;

  const std::size_t maximum_leaf_bytes = checked_product(
      maximum_point_count,
      sizeof(spatial::MortonLbvhSnapshotLeaf),
      "the Phase 14 maximum snapshot leaf bytes overflow size_t");
  const std::size_t maximum_node_bytes = checked_product(
      maximum_node_count,
      sizeof(spatial::MortonLbvhSnapshotNode),
      "the Phase 14 maximum snapshot node bytes overflow size_t");
  audit.host_snapshot_byte_capacity = checked_sum(
      maximum_leaf_bytes,
      maximum_node_bytes,
      "the Phase 14 maximum snapshot bytes overflow size_t");

  audit.point_count = point_count;
  if (point_count != 0U) {
    audit.required_axis_count = checked_product(
        point_count,
        kAxisCount,
        "the Phase 14 required axis count overflows size_t");
    audit.required_node_count = checked_node_count(point_count);
    const std::size_t required_leaf_bytes = checked_product(
        point_count,
        sizeof(spatial::MortonLbvhSnapshotLeaf),
        "the Phase 14 required snapshot leaf bytes overflow size_t");
    const std::size_t required_node_bytes = checked_product(
        audit.required_node_count,
        sizeof(spatial::MortonLbvhSnapshotNode),
        "the Phase 14 required snapshot node bytes overflow size_t");
    audit.required_snapshot_byte_count = checked_sum(
        required_leaf_bytes,
        required_node_bytes,
        "the Phase 14 required snapshot bytes overflow size_t");
  }
  audit.strict_find_split_postorder_requested = true;
  audit.minimum_point_id_aabb_witness_rule_requested = true;
  return audit;
}

void validate_execution_metadata(
    detail::Phase14MortonLbvhExecutionKind execution_kind,
    bool cuda_path_qualified,
    std::size_t kernel_launch_count,
    std::size_t library_submission_count,
    std::size_t synchronization_count,
    const char* role) {
  switch (execution_kind) {
    case detail::Phase14MortonLbvhExecutionKind::host_fake:
      if (cuda_path_qualified || kernel_launch_count != 0U ||
          library_submission_count != 0U ||
          synchronization_count != 0U) {
        throw std::runtime_error(role);
      }
      return;
    case detail::Phase14MortonLbvhExecutionKind::cuda:
      if (!cuda_path_qualified ||
          (kernel_launch_count == 0U &&
           library_submission_count == 0U) ||
          synchronization_count == 0U) {
        throw std::runtime_error(role);
      }
      return;
  }
  throw std::runtime_error(role);
}

void validate_proposal_batch(
    const detail::Phase14MortonBinProposalBatch& batch,
    std::size_t point_count,
    std::size_t axis_count,
    std::size_t maximum_point_count,
    std::uint64_t previous_epoch) {
  const std::size_t expected_coordinate_bytes = checked_product(
      axis_count,
      sizeof(std::uint64_t),
      "the Phase 14 proposal coordinate bytes overflow size_t");
  const std::size_t expected_bin_bytes = checked_product(
      axis_count,
      sizeof(std::uint32_t),
      "the Phase 14 proposal bin bytes overflow size_t");
  if (point_count > maximum_point_count ||
      previous_epoch == std::numeric_limits<std::uint64_t>::max() ||
      batch.encoded_bins.size() != axis_count ||
      batch.axis_count != axis_count ||
      batch.host_to_device_coordinate_byte_count !=
          expected_coordinate_bytes ||
      batch.device_to_host_encoded_bin_byte_count !=
          expected_bin_bytes ||
      batch.buffer_epoch != previous_epoch + UINT64_C(1)) {
    throw std::runtime_error(
        "the Phase 14 Morton bin proposal returned invalid extents or epoch");
  }
  validate_execution_metadata(
      batch.execution_kind,
      batch.cuda_path_qualified,
      batch.kernel_launch_count,
      batch.library_submission_count,
      batch.synchronization_count,
      "the Phase 14 Morton bin proposal returned invalid execution metadata");
  if ((batch.execution_kind ==
           detail::Phase14MortonLbvhExecutionKind::cuda &&
       (batch.kernel_launch_count != 1U ||
        batch.library_submission_count != 0U ||
        batch.synchronization_count != 1U)) ||
      (batch.execution_kind ==
           detail::Phase14MortonLbvhExecutionKind::host_fake &&
       (batch.kernel_launch_count != 0U ||
        batch.library_submission_count != 0U ||
        batch.synchronization_count != 0U))) {
    throw std::runtime_error(
        "the Phase 14 Morton bin proposal returned wrong deterministic "
        "submission counts");
  }
}

void validate_snapshot_batch(
    const detail::Phase14MortonLbvhSnapshotBatch& batch,
    std::span<const std::uint64_t> certified_morton_codes,
    std::size_t point_count,
    std::size_t node_count,
    std::size_t maximum_point_count,
    detail::Phase14MortonLbvhExecutionKind expected_execution_kind,
    bool expected_cuda_path_qualified,
    std::uint64_t proposal_epoch) {
  const std::size_t expected_code_bytes = checked_product(
      point_count,
      sizeof(std::uint64_t),
      "the Phase 14 Morton code bytes overflow size_t");
  const std::size_t expected_leaf_bytes = checked_product(
      point_count,
      sizeof(spatial::MortonLbvhSnapshotLeaf),
      "the Phase 14 D2H leaf bytes overflow size_t");
  const std::size_t expected_node_bytes = checked_product(
      node_count,
      sizeof(spatial::MortonLbvhSnapshotNode),
      "the Phase 14 D2H node bytes overflow size_t");
  const std::size_t maximum_axis_count = checked_product(
      maximum_point_count,
      kAxisCount,
      "the Phase 14 maximum device axis count overflows size_t");
  const std::size_t maximum_node_count =
      checked_node_count(maximum_point_count);
  const std::size_t expected_device_coordinate_capacity =
      checked_product(
          maximum_axis_count,
          sizeof(std::uint64_t),
          "the Phase 14 device coordinate capacity overflows size_t");
  const std::size_t expected_device_bin_capacity = checked_product(
      maximum_axis_count,
      sizeof(std::uint32_t),
      "the Phase 14 device bin capacity overflows size_t");
  const std::size_t expected_device_code_capacity = checked_product(
      maximum_point_count,
      2U * sizeof(std::uint64_t),
      "the Phase 14 device Morton double-buffer capacity overflows size_t");
  const std::size_t expected_device_point_id_capacity =
      expected_device_code_capacity;
  const std::size_t expected_device_leaf_capacity = checked_product(
      maximum_point_count,
      sizeof(spatial::MortonLbvhSnapshotLeaf),
      "the Phase 14 device leaf capacity overflows size_t");
  const std::size_t expected_device_node_capacity = checked_product(
      maximum_node_count,
      sizeof(spatial::MortonLbvhSnapshotNode),
      "the Phase 14 device node capacity overflows size_t");
  const std::size_t expected_device_frontier_capacity =
      checked_product(
          maximum_point_count,
          2U * kDeviceRangeByteSize,
          "the Phase 14 device frontier capacity overflows size_t");
  const std::size_t expected_device_level_capacity = checked_product(
      maximum_node_count,
      sizeof(std::uint64_t),
      "the Phase 14 device level capacity overflows size_t");
  std::size_t expected_total_device_capacity = 0U;
  for (const std::size_t capacity :
       {expected_device_coordinate_capacity,
        expected_device_bin_capacity,
        expected_device_code_capacity,
        expected_device_point_id_capacity,
        expected_device_leaf_capacity,
        expected_device_node_capacity,
        expected_device_frontier_capacity,
        expected_device_level_capacity,
        kDeviceControlByteSize,
        batch.sort_temporary_byte_capacity}) {
    expected_total_device_capacity = checked_sum(
        expected_total_device_capacity,
        capacity,
        "the Phase 14 total device capacity overflows size_t");
  }
  if (point_count > maximum_point_count ||
      proposal_epoch == std::numeric_limits<std::uint64_t>::max() ||
      batch.point_count != static_cast<std::uint64_t>(point_count) ||
      batch.root_node_index != static_cast<std::uint64_t>(node_count - 1U) ||
      batch.leaves.size() != point_count ||
      batch.nodes.size() != node_count ||
      batch.proposed_counters.point_count !=
          static_cast<std::uint64_t>(point_count) ||
      batch.proposed_counters.node_count !=
          static_cast<std::uint64_t>(node_count) ||
      batch.host_to_device_morton_code_byte_count !=
          expected_code_bytes ||
      batch.device_to_host_leaf_byte_count != expected_leaf_bytes ||
      batch.device_to_host_node_byte_count != expected_node_bytes ||
      batch.device_coordinate_byte_capacity !=
          expected_device_coordinate_capacity ||
      batch.device_encoded_bin_byte_capacity !=
          expected_device_bin_capacity ||
      batch.device_morton_code_double_buffer_byte_capacity !=
          expected_device_code_capacity ||
      batch.device_point_id_double_buffer_byte_capacity !=
          expected_device_point_id_capacity ||
      batch.device_leaf_byte_capacity !=
          expected_device_leaf_capacity ||
      batch.device_node_byte_capacity !=
          expected_device_node_capacity ||
      batch.device_frontier_double_buffer_byte_capacity !=
          expected_device_frontier_capacity ||
      batch.device_level_schedule_byte_capacity !=
          expected_device_level_capacity ||
      batch.device_control_byte_capacity != kDeviceControlByteSize ||
      batch.total_fixed_device_byte_capacity !=
          expected_total_device_capacity ||
      (expected_cuda_path_qualified &&
       batch.sort_temporary_byte_capacity == 0U) ||
      (!expected_cuda_path_qualified &&
       batch.sort_temporary_byte_capacity != 0U) ||
      batch.buffer_epoch != proposal_epoch + UINT64_C(1) ||
      batch.execution_kind != expected_execution_kind ||
      batch.cuda_path_qualified != expected_cuda_path_qualified) {
    throw std::runtime_error(
        "the Phase 14 Morton LBVH snapshot returned invalid extents, "
        "execution kind, or epoch");
  }
  validate_execution_metadata(
      batch.execution_kind,
      batch.cuda_path_qualified,
      batch.kernel_launch_count,
      batch.library_submission_count,
      batch.synchronization_count,
      "the Phase 14 Morton LBVH snapshot returned invalid execution metadata");
  if (!std::in_range<std::size_t>(
          batch.proposed_counters.maximum_depth)) {
    throw std::runtime_error(
        "the Phase 14 Morton LBVH depth does not fit submission counters");
  }
  const std::size_t maximum_depth = static_cast<std::size_t>(
      batch.proposed_counters.maximum_depth);
  const std::size_t expected_snapshot_kernel_count = checked_sum(
      5U,
      checked_product(
          2U,
          maximum_depth,
          "the Phase 14 snapshot kernel count overflows size_t"),
      "the Phase 14 snapshot kernel count overflows size_t");
  const std::size_t expected_snapshot_synchronization_count =
      checked_sum(
          maximum_depth,
          2U,
          "the Phase 14 snapshot synchronization count overflows size_t");
  if ((batch.execution_kind ==
           detail::Phase14MortonLbvhExecutionKind::cuda &&
       (batch.kernel_launch_count != expected_snapshot_kernel_count ||
        batch.library_submission_count != 1U ||
        batch.synchronization_count !=
            expected_snapshot_synchronization_count)) ||
      (batch.execution_kind ==
           detail::Phase14MortonLbvhExecutionKind::host_fake &&
       (batch.kernel_launch_count != 0U ||
        batch.library_submission_count != 0U ||
        batch.synchronization_count != 0U))) {
    throw std::runtime_error(
        "the Phase 14 Morton LBVH snapshot returned wrong deterministic "
        "submission counts");
  }

  std::vector<unsigned char> point_seen(point_count, 0U);
  for (std::size_t position = 0U;
       position < batch.leaves.size();
       ++position) {
    const spatial::MortonLbvhSnapshotLeaf& leaf =
        batch.leaves[position];
    if (leaf.point_id >= static_cast<std::uint64_t>(point_count)) {
      throw std::runtime_error(
          "the Phase 14 Morton order contains an out-of-domain PointId");
    }
    const std::size_t point_index =
        static_cast<std::size_t>(leaf.point_id);
    if (point_seen[point_index] != 0U ||
        leaf.morton_code != certified_morton_codes[point_index] ||
        (position != 0U &&
         !leaf_less(batch.leaves[position - 1U], leaf))) {
      throw std::runtime_error(
          "the Phase 14 Morton order is not the stable "
          "(Morton code, PointId) permutation");
    }
    point_seen[point_index] = 1U;
  }
}

}  // namespace

MortonLbvhDeviceBuildResult::MortonLbvhDeviceBuildResult(
    MortonLbvhDeviceBuildDecision decision,
    MortonLbvhDeviceBuildStopReason stop_reason,
    MortonLbvhDeviceBuildAudit audit,
    std::optional<spatial::MortonLbvhIndex> certified_index)
    : decision_(decision),
      stop_reason_(stop_reason),
      audit_(std::move(audit)),
      certified_index_(std::move(certified_index)) {}

bool MortonLbvhDeviceBuildResult::complete_certified_build() const noexcept {
  return schema_version_ == morton_lbvh_device_build_schema_version &&
         decision_ == MortonLbvhDeviceBuildDecision::complete &&
         stop_reason_ == MortonLbvhDeviceBuildStopReason::none &&
         certified_index_.has_value() &&
         certified_index_->ready() &&
         audit_.fixed_capacity_preflight_satisfied &&
         audit_.every_ambiguous_axis_exactly_resolved &&
         audit_.encoded_bin_extent_validated &&
         audit_.stable_morton_point_id_order_validated &&
         audit_.strict_find_split_postorder_requested &&
         audit_.minimum_point_id_aabb_witness_rule_requested &&
         audit_.snapshot_import_certified &&
         !audit_.cpu_reference_build_invoked &&
         !audit_.higher_order_delaunay_mosaic_materialized &&
         !audit_.global_cell_or_coface_arena_materialized &&
         !audit_.public_status_claimed;
}

bool MortonLbvhDeviceBuildResult::cuda_qualified_build() const noexcept {
  return complete_certified_build() &&
         audit_.gpu_execution_performed &&
         audit_.cuda_builder_qualified &&
         !audit_.fake_launcher_executed;
}

const spatial::MortonLbvhIndex&
MortonLbvhDeviceBuildResult::certified_index() const & {
  if (!complete_certified_build()) {
    throw std::logic_error(
        "an incomplete Phase 14 Morton LBVH result has no certified index");
  }
  return *certified_index_;
}

MortonLbvhBuildContext::MortonLbvhBuildContext(
    std::size_t maximum_point_count)
    : state_(
          std::make_shared<
              detail::Phase14MortonLbvhBuildContextState>()),
      maximum_point_count_(maximum_point_count) {
  if (maximum_point_count == 0U ||
      maximum_point_count >
          static_cast<std::size_t>(
              spatial::CanonicalPointCloud::max_point_count) ||
      maximum_point_count > static_cast<std::size_t>(INT_MAX)) {
    throw std::invalid_argument(
        "a Phase 14 Morton LBVH context requires a nonzero PointId-sized "
        "capacity within the CUB int item-count limit");
  }
  maximum_axis_count_ = checked_product(
      maximum_point_count,
      kAxisCount,
      "the Phase 14 Morton LBVH axis capacity overflows size_t");
  maximum_node_count_ = checked_node_count(maximum_point_count);
  (void)initialize_audit(
      maximum_point_count_,
      maximum_axis_count_,
      maximum_node_count_,
      0U);
}

MortonLbvhBuildContext::~MortonLbvhBuildContext() noexcept = default;

MortonLbvhBuildContext::MortonLbvhBuildContext(
    MortonLbvhBuildContext&&) noexcept = default;

MortonLbvhBuildContext& MortonLbvhBuildContext::operator=(
    MortonLbvhBuildContext&&) noexcept = default;

MortonLbvhDeviceBuildResult MortonLbvhBuildContext::build(
    const spatial::CanonicalPointCloud& cloud) {
  if (!state_) {
    throw std::logic_error(
        "a moved-from Phase 14 Morton LBVH context cannot be used");
  }
  const std::size_t point_count = cloud.size();
  if (point_count == 0U) {
    throw std::invalid_argument(
        "a Phase 14 Morton LBVH requires a nonempty canonical cloud");
  }
  if (point_count >
      static_cast<std::size_t>(
          spatial::CanonicalPointCloud::max_point_count)) {
    throw std::length_error(
        "a Phase 14 Morton LBVH cloud exceeds the PointId domain");
  }

  MortonLbvhDeviceBuildAudit audit = initialize_audit(
      maximum_point_count_,
      maximum_axis_count_,
      maximum_node_count_,
      point_count);
  if (point_count > maximum_point_count_) {
    return MortonLbvhDeviceBuildResult{
        MortonLbvhDeviceBuildDecision::capacity_exhausted,
        MortonLbvhDeviceBuildStopReason::point_capacity,
        std::move(audit),
        std::nullopt};
  }
  audit.fixed_capacity_preflight_satisfied = true;

  std::vector<std::uint64_t> coordinate_bits(
      audit.required_axis_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    const std::array<std::uint64_t, 3> point_bits =
        cloud.point(checked_point_id(point_index))
            .canonical_input_bits();
    for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
      coordinate_bits[axis * point_count + point_index] =
          point_bits[axis];
    }
  }

  const spatial::ExactPointCloudAabb3 exact_bounds =
      spatial::build_exact_point_cloud_aabb(cloud);
  audit.cpu_exact_point_cloud_aabb_scan_count = point_count;
  if (!std::in_range<std::size_t>(
          exact_bounds.audit.exact_extremum_comparison_count)) {
    throw std::length_error(
        "the Phase 14 exact AABB comparison count does not fit size_t");
  }
  audit.cpu_exact_extremum_comparison_count =
      static_cast<std::size_t>(
          exact_bounds.audit.exact_extremum_comparison_count);

  std::array<exact::ExactRational, 3> exact_lower{};
  std::array<exact::ExactRational, 3> exact_upper{};
  for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
    exact_lower[axis] = exact::ExactRational::from_binary64_bits(
        exact_bounds.bounds.lower_binary64_bits[axis]);
    exact_upper[axis] = exact::ExactRational::from_binary64_bits(
        exact_bounds.bounds.upper_binary64_bits[axis]);
  }

  std::optional<spatial::MortonLbvhIndex> certified_index =
      state_->with_gpu_section([&]() {
        const detail::Phase14MortonBinProposalBatch proposal =
            detail::propose_phase14_morton_bins_on_gpu(
                *state_,
                coordinate_bits,
                point_count,
                exact_bounds.bounds.lower_binary64_bits,
                exact_bounds.bounds.upper_binary64_bits,
                maximum_point_count_);
        validate_proposal_batch(
            proposal,
            point_count,
            audit.required_axis_count,
            maximum_point_count_,
            last_buffer_epoch_);

        audit.device_bin_proposal_count = proposal.axis_count;
        audit.host_to_device_coordinate_byte_count =
            proposal.host_to_device_coordinate_byte_count;
        audit.device_to_host_encoded_bin_byte_count =
            proposal.device_to_host_encoded_bin_byte_count;
        audit.proposal_buffer_epoch = proposal.buffer_epoch;
        audit.device_kernel_launch_count =
            proposal.kernel_launch_count;
        audit.device_library_submission_count =
            proposal.library_submission_count;
        audit.device_synchronization_count =
            proposal.synchronization_count;
        audit.encoded_bin_extent_validated = true;

        std::vector<std::uint64_t> morton_codes(point_count, 0U);
        for (std::size_t point_index = 0U;
             point_index < point_count;
             ++point_index) {
          std::array<std::uint64_t, 3> bins{};
          for (std::size_t axis = 0U; axis < kAxisCount; ++axis) {
            const std::size_t proposal_index =
                axis * point_count + point_index;
            const std::uint32_t encoded =
                proposal.encoded_bins[proposal_index];
            if ((encoded & ~detail::phase14_morton_bin_allowed_mask) !=
                0U) {
              throw std::runtime_error(
                  "a Phase 14 encoded Morton bin uses reserved bits");
            }
            if ((encoded &
                 detail::phase14_morton_bin_ambiguous_bit) != 0U) {
              if (encoded !=
                  detail::phase14_morton_bin_ambiguous_bit) {
                throw std::runtime_error(
                    "an ambiguous Phase 14 Morton bin has a stale payload");
              }
              ++audit.device_ambiguous_axis_count;
              ++audit.cpu_exact_fallback_axis_count;
              bins[axis] = exact_quantized_coordinate(
                  cloud.point(checked_point_id(point_index))
                      .exact()
                      .coordinate(axis),
                  exact_lower[axis],
                  exact_upper[axis]);
            } else {
              ++audit.device_unambiguous_axis_count;
              bins[axis] = static_cast<std::uint64_t>(
                  encoded &
                  detail::phase14_morton_bin_value_mask);
            }
          }
          morton_codes[point_index] =
              interleaved_morton_code(bins);
        }
        if (audit.device_unambiguous_axis_count +
                    audit.device_ambiguous_axis_count !=
                audit.required_axis_count ||
            audit.cpu_exact_fallback_axis_count !=
                audit.device_ambiguous_axis_count) {
          throw std::logic_error(
              "the Phase 14 Morton ambiguity accounting did not close");
        }
        audit.every_ambiguous_axis_exactly_resolved = true;

        detail::Phase14MortonLbvhSnapshotBatch snapshot_batch =
            detail::build_phase14_morton_lbvh_snapshot_on_gpu(
                *state_,
                coordinate_bits,
                point_count,
                morton_codes,
                maximum_point_count_);
        validate_snapshot_batch(
            snapshot_batch,
            morton_codes,
            point_count,
            audit.required_node_count,
            maximum_point_count_,
            proposal.execution_kind,
            proposal.cuda_path_qualified,
            proposal.buffer_epoch);

        audit.host_to_device_morton_code_byte_count =
            snapshot_batch.host_to_device_morton_code_byte_count;
        audit.device_to_host_leaf_byte_count =
            snapshot_batch.device_to_host_leaf_byte_count;
        audit.device_to_host_node_byte_count =
            snapshot_batch.device_to_host_node_byte_count;
        audit.device_coordinate_byte_capacity =
            snapshot_batch.device_coordinate_byte_capacity;
        audit.device_encoded_bin_byte_capacity =
            snapshot_batch.device_encoded_bin_byte_capacity;
        audit.device_morton_code_double_buffer_byte_capacity =
            snapshot_batch
                .device_morton_code_double_buffer_byte_capacity;
        audit.device_point_id_double_buffer_byte_capacity =
            snapshot_batch
                .device_point_id_double_buffer_byte_capacity;
        audit.device_leaf_byte_capacity =
            snapshot_batch.device_leaf_byte_capacity;
        audit.device_node_byte_capacity =
            snapshot_batch.device_node_byte_capacity;
        audit.device_frontier_double_buffer_byte_capacity =
            snapshot_batch
                .device_frontier_double_buffer_byte_capacity;
        audit.device_level_schedule_byte_capacity =
            snapshot_batch.device_level_schedule_byte_capacity;
        audit.device_control_byte_capacity =
            snapshot_batch.device_control_byte_capacity;
        audit.device_sort_temporary_byte_capacity =
            snapshot_batch.sort_temporary_byte_capacity;
        audit.total_fixed_device_byte_capacity =
            snapshot_batch.total_fixed_device_byte_capacity;
        audit.device_kernel_launch_count = checked_sum(
            audit.device_kernel_launch_count,
            snapshot_batch.kernel_launch_count,
            "the Phase 14 kernel launch count overflowed");
        audit.device_library_submission_count = checked_sum(
            audit.device_library_submission_count,
            snapshot_batch.library_submission_count,
            "the Phase 14 library submission count overflowed");
        audit.device_synchronization_count = checked_sum(
            audit.device_synchronization_count,
            snapshot_batch.synchronization_count,
            "the Phase 14 synchronization count overflowed");
        audit.snapshot_buffer_epoch =
            snapshot_batch.buffer_epoch;
        audit.stable_morton_point_id_order_validated = true;
        audit.gpu_execution_performed =
            snapshot_batch.execution_kind ==
            detail::Phase14MortonLbvhExecutionKind::cuda;
        audit.cuda_builder_qualified =
            snapshot_batch.cuda_path_qualified &&
            audit.gpu_execution_performed;
        audit.fake_launcher_executed =
            snapshot_batch.execution_kind ==
            detail::Phase14MortonLbvhExecutionKind::host_fake;

        spatial::MortonLbvhSnapshot snapshot;
        snapshot.schema_version =
            spatial::morton_lbvh_snapshot_schema_version;
        snapshot.morton_bits_per_axis =
            spatial::morton_lbvh_snapshot_morton_bits_per_axis;
        snapshot.point_count =
            snapshot_batch.point_count;
        snapshot.root_node_index =
            snapshot_batch.root_node_index;
        snapshot.root_aabb = exact_bounds.bounds;
        snapshot.proposed_counters =
            snapshot_batch.proposed_counters;
        snapshot.leaves = std::move(snapshot_batch.leaves);
        snapshot.nodes = std::move(snapshot_batch.nodes);

        spatial::MortonLbvhIndex imported =
            spatial::MortonLbvhIndex::import_certified_snapshot(
                cloud, snapshot);
        audit.cpu_import_exact_morton_axis_recertification_count =
            audit.required_axis_count;
        audit.snapshot_import_certified =
            imported.validated_for(cloud);
        if (!audit.snapshot_import_certified) {
          throw std::logic_error(
              "the Phase 14 certified snapshot import returned an "
              "unvalidated Morton LBVH");
        }
        last_buffer_epoch_ = snapshot_batch.buffer_epoch;
        return std::optional<spatial::MortonLbvhIndex>{
            std::move(imported)};
      });

  MortonLbvhDeviceBuildResult result{
      MortonLbvhDeviceBuildDecision::complete,
      MortonLbvhDeviceBuildStopReason::none,
      std::move(audit),
      std::move(certified_index)};
  if (!result.complete_certified_build()) {
    throw std::logic_error(
        "the Phase 14 Morton LBVH result did not close its certified "
        "snapshot-import contract");
  }
  return result;
}

std::size_t MortonLbvhBuildContext::maximum_point_count() const noexcept {
  return state_ == nullptr ? 0U : maximum_point_count_;
}

std::size_t MortonLbvhBuildContext::maximum_node_count() const noexcept {
  return state_ == nullptr ? 0U : maximum_node_count_;
}

}  // namespace morsehgp3d::gpu
