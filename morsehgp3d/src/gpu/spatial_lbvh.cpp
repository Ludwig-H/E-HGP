#include "morsehgp3d/gpu/spatial_lbvh.hpp"

#include "phase4_spatial_bounds_internal.hpp"

#include "../cpu/spatial/exact_query.hpp"
#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/rational.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

class SpatialLbvhHostState final {
 public:
  std::shared_ptr<const void> cloud_identity;
  std::size_t point_count{0U};
  std::size_t root_index{0U};
  std::vector<SpatialLbvhNodeInputRecord> nodes;
  std::vector<spatial::PointId> leaf_point_ids;
};

}  // namespace morsehgp3d::gpu::detail

namespace morsehgp3d::gpu {
namespace {

using exact::ExactRational;
using exact::ExactRational3;

constexpr std::uint64_t kPositiveMaximumFiniteBits =
    UINT64_C(0x7fefffffffffffff);
constexpr std::uint64_t kPositiveInfinityBits =
    UINT64_C(0x7ff0000000000000);
constexpr std::uint64_t kSignBit = UINT64_C(0x8000000000000000);
constexpr std::uint64_t kExponentMask = UINT64_C(0x7ff0000000000000);
constexpr std::uint64_t kFractionMask = UINT64_C(0x000fffffffffffff);
constexpr std::uint64_t kFnvOffsetBasis = UINT64_C(14695981039346656037);
constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);
constexpr std::size_t kInvalidIndex = std::numeric_limits<std::size_t>::max();

struct DirectedEnclosure {
  std::uint64_t lower_bits{0U};
  std::uint64_t upper_bits{0U};
  DirectedEnclosureStatus status{DirectedEnclosureStatus::exact};
};

[[nodiscard]] ExactRational positive_binary64_rational(std::uint64_t bits) {
  if (bits > kPositiveMaximumFiniteBits) {
    throw std::logic_error(
        "an LBVH enclosure search produced a non-finite binary64 word");
  }
  return ExactRational::from_binary64_bits(bits);
}

[[nodiscard]] DirectedEnclosure enclose_nonnegative_rational(
    const ExactRational& value) {
  if (value.sign() < 0) {
    throw std::logic_error(
        "an LBVH nonnegative enclosure received a negative rational");
  }
  const ExactRational maximum =
      positive_binary64_rational(kPositiveMaximumFiniteBits);
  if (value > maximum) {
    return DirectedEnclosure{
        kPositiveMaximumFiniteBits,
        kPositiveMaximumFiniteBits,
        DirectedEnclosureStatus::unsupported_range};
  }

  std::uint64_t lower_bits = 0U;
  std::uint64_t upper_search_bits = kPositiveMaximumFiniteBits;
  while (lower_bits < upper_search_bits) {
    const std::uint64_t midpoint_bits =
        lower_bits + (upper_search_bits - lower_bits + 1U) / 2U;
    if (positive_binary64_rational(midpoint_bits) <= value) {
      lower_bits = midpoint_bits;
    } else {
      upper_search_bits = midpoint_bits - 1U;
    }
  }
  if (positive_binary64_rational(lower_bits) == value) {
    return DirectedEnclosure{
        lower_bits, lower_bits, DirectedEnclosureStatus::exact};
  }
  if (lower_bits == kPositiveMaximumFiniteBits) {
    throw std::logic_error(
        "a finite LBVH enclosure has no representable upper endpoint");
  }
  return DirectedEnclosure{
      lower_bits, lower_bits + 1U, DirectedEnclosureStatus::enclosed};
}

[[nodiscard]] DirectedEnclosure enclose_rational(
    const ExactRational& value) {
  if (value.sign() >= 0) {
    return enclose_nonnegative_rational(value);
  }
  const DirectedEnclosure magnitude = enclose_nonnegative_rational(-value);
  if (magnitude.status == DirectedEnclosureStatus::unsupported_range) {
    return DirectedEnclosure{
        kSignBit | kPositiveMaximumFiniteBits,
        kSignBit | kPositiveMaximumFiniteBits,
        DirectedEnclosureStatus::unsupported_range};
  }
  const std::uint64_t lower_bits =
      magnitude.upper_bits == 0U ? 0U : kSignBit | magnitude.upper_bits;
  const std::uint64_t upper_bits =
      magnitude.lower_bits == 0U ? 0U : kSignBit | magnitude.lower_bits;
  return DirectedEnclosure{lower_bits, upper_bits, magnitude.status};
}

[[nodiscard]] bool enclosure_supported(
    const std::array<DirectedEnclosure, 3>& query_enclosures,
    const DirectedEnclosure& cutoff_enclosure) noexcept {
  if (cutoff_enclosure.status == DirectedEnclosureStatus::unsupported_range) {
    return false;
  }
  return std::none_of(
      query_enclosures.begin(),
      query_enclosures.end(),
      [](const DirectedEnclosure& enclosure) {
        return enclosure.status == DirectedEnclosureStatus::unsupported_range;
      });
}

void set_enclosure_audit(
    SpatialLbvhAudit& audit,
    const std::array<DirectedEnclosure, 3>& query_enclosures,
    const DirectedEnclosure& cutoff_enclosure) {
  for (std::size_t axis = 0U; axis < query_enclosures.size(); ++axis) {
    audit.query_lower_bits[axis] = query_enclosures[axis].lower_bits;
    audit.query_upper_bits[axis] = query_enclosures[axis].upper_bits;
    audit.query_enclosure[axis] = query_enclosures[axis].status;
  }
  audit.cutoff_lower_bits = cutoff_enclosure.lower_bits;
  audit.cutoff_upper_bits = cutoff_enclosure.upper_bits;
  audit.cutoff_enclosure = cutoff_enclosure.status;
}

[[nodiscard]] bool is_nonnegative_interval_word(std::uint64_t bits) noexcept {
  if ((bits & kSignBit) != 0U) {
    return false;
  }
  if ((bits & kExponentMask) != kExponentMask) {
    return true;
  }
  return (bits & kFractionMask) == 0U;
}

[[nodiscard]] exact::ExactLevel exact_minimum_squared_distance(
    const detail::SpatialLbvhNodeInputRecord& node,
    const ExactRational3& query) {
  ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const ExactRational lower = ExactRational::from_binary64_bits(
        node.bounds.lower_bits[axis]);
    const ExactRational upper = ExactRational::from_binary64_bits(
        node.bounds.upper_bits[axis]);
    const ExactRational coordinate = query.coordinate(axis);
    ExactRational delta;
    if (coordinate < lower) {
      delta = lower - coordinate;
    } else if (coordinate > upper) {
      delta = coordinate - upper;
    }
    squared_distance = squared_distance + delta * delta;
  }
  return exact::ExactLevel{std::move(squared_distance)};
}

void hash_word(std::uint64_t& digest, std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
}

[[nodiscard]] bool sentinel_record(
    const detail::SpatialLbvhCoverRecord& record) noexcept {
  return record.node_index == detail::spatial_bounds_sentinel_code &&
         record.lower_squared_distance_bits ==
             detail::spatial_bounds_sentinel_code &&
         record.upper_squared_distance_bits ==
             detail::spatial_bounds_sentinel_code &&
         record.kind == detail::spatial_bounds_sentinel_code;
}

void require_interval_contains_exact_bound(
    const detail::SpatialLbvhCoverRecord& record,
    const exact::ExactLevel& exact_bound) {
  const ExactRational lower = ExactRational::from_binary64_bits(
      record.lower_squared_distance_bits);
  if (lower > exact_bound.rational()) {
    throw std::runtime_error(
        "the GPU spatial-LBVH interval lies above its exact AABB bound");
  }
  if (record.upper_squared_distance_bits != kPositiveInfinityBits) {
    const ExactRational upper = ExactRational::from_binary64_bits(
        record.upper_squared_distance_bits);
    if (upper < exact_bound.rational()) {
      throw std::runtime_error(
          "the GPU spatial-LBVH interval lies below its exact AABB bound");
    }
  }
}

[[nodiscard]] SpatialLbvhCoverResult unsupported_cover(
    const detail::SpatialLbvhHostState& host,
    const std::array<DirectedEnclosure, 3>& query_enclosures,
    const DirectedEnclosure& cutoff_enclosure) {
  SpatialLbvhCoverResult result;
  result.candidate_point_ids.reserve(host.point_count);
  for (std::size_t point_index = 0U;
       point_index < host.point_count;
       ++point_index) {
    result.candidate_point_ids.push_back(
        static_cast<spatial::PointId>(point_index));
  }
  result.audit.resident_node_count = host.nodes.size();
  result.audit.resident_point_count = host.point_count;
  result.audit.gpu_output_capacity = host.nodes.size();
  result.audit.candidate_point_count = host.point_count;
  result.audit.unsupported_range_fallback_count = host.point_count;
  result.audit.cover_antichain_complete = true;
  result.audit.point_partition_complete = true;
  result.audit.cpu_exact_recertification_complete = true;
  set_enclosure_audit(
      result.audit, query_enclosures, cutoff_enclosure);
  return result;
}

[[nodiscard]] SpatialLbvhCoverResult validate_and_recertify_cover(
    const detail::SpatialLbvhCoverBatch& batch,
    const detail::SpatialLbvhHostState& host,
    const ExactRational3& query,
    const exact::ExactLevel& squared_cutoff,
    const std::array<DirectedEnclosure, 3>& query_enclosures,
    const DirectedEnclosure& cutoff_enclosure) {
  SpatialLbvhCoverResult result;
  SpatialLbvhAudit& audit = result.audit;
  audit.resident_node_count = host.nodes.size();
  audit.resident_point_count = host.point_count;
  audit.gpu_output_capacity = batch.records.size();
  audit.gpu_output_cover_record_count = batch.record_count;
  audit.gpu_launch_count = 1U;
  audit.gpu_kernel_launch_count = batch.kernel_launch_count;
  audit.gpu_traversal_round_count = batch.traversal_round_count;
  audit.gpu_parallel_round_count = batch.parallel_round_count;
  audit.gpu_peak_frontier_count = batch.peak_frontier_count;
  audit.gpu_processed_node_count = batch.processed_node_count;
  audit.buffer_epoch = batch.buffer_epoch;
  set_enclosure_audit(audit, query_enclosures, cutoff_enclosure);

  if (batch.records.size() != host.nodes.size() || batch.record_count == 0U ||
      batch.record_count > batch.records.size() ||
      batch.kernel_launch_count == 0U ||
      batch.kernel_launch_count != batch.traversal_round_count ||
      batch.traversal_round_count > batch.processed_node_count ||
      batch.processed_node_count > host.nodes.size() ||
      batch.peak_frontier_count == 0U ||
      batch.peak_frontier_count > batch.processed_node_count ||
      batch.parallel_round_count > batch.traversal_round_count ||
      ((batch.peak_frontier_count > 1U) !=
       (batch.parallel_round_count > 0U))) {
    throw std::runtime_error(
        "the GPU spatial-LBVH cover returned invalid frontier metadata");
  }
  for (std::size_t index = batch.record_count;
       index < batch.records.size();
       ++index) {
    if (!sentinel_record(batch.records[index])) {
      throw std::runtime_error(
          "the GPU spatial-LBVH cover left a stale record beyond its prefix");
    }
  }

  std::vector<std::size_t> record_position_by_node(
      host.nodes.size(), kInvalidIndex);
  for (std::size_t position = 0U; position < batch.record_count; ++position) {
    const detail::SpatialLbvhCoverRecord& record = batch.records[position];
    if (!std::in_range<std::size_t>(record.node_index) ||
        static_cast<std::size_t>(record.node_index) >= host.nodes.size()) {
      throw std::runtime_error(
          "the GPU spatial-LBVH cover returned an out-of-range node index");
    }
    const std::size_t node_index = static_cast<std::size_t>(record.node_index);
    if (record_position_by_node[node_index] != kInvalidIndex) {
      throw std::runtime_error(
          "the GPU spatial-LBVH cover repeated a node index");
    }
    record_position_by_node[node_index] = position;
    if (!is_nonnegative_interval_word(
            record.lower_squared_distance_bits) ||
        !is_nonnegative_interval_word(
            record.upper_squared_distance_bits) ||
        record.lower_squared_distance_bits == kPositiveInfinityBits ||
        (record.upper_squared_distance_bits != kPositiveInfinityBits &&
         record.lower_squared_distance_bits >
             record.upper_squared_distance_bits)) {
      throw std::runtime_error(
          "the GPU spatial-LBVH cover returned an invalid distance interval");
    }
    switch (record.kind) {
      case detail::spatial_lbvh_cover_prune_code:
        if (record.upper_squared_distance_bits == kPositiveInfinityBits ||
            record.lower_squared_distance_bits <=
                cutoff_enclosure.upper_bits) {
          throw std::runtime_error(
              "the GPU spatial-LBVH prune lacks a strict FP64 margin");
        }
        ++audit.gpu_prune_proposal_count;
        break;
      case detail::spatial_lbvh_cover_leaf_code:
        ++audit.gpu_candidate_leaf_count;
        break;
      default:
        throw std::runtime_error(
            "the GPU spatial-LBVH cover returned an invalid record kind");
    }
  }
  if (audit.gpu_prune_proposal_count + audit.gpu_candidate_leaf_count !=
      batch.record_count) {
    throw std::logic_error(
        "the GPU spatial-LBVH cover counters do not close");
  }

  std::uint64_t digest = kFnvOffsetBasis;
  for (std::size_t node_index = 0U;
       node_index < record_position_by_node.size();
       ++node_index) {
    const std::size_t position = record_position_by_node[node_index];
    if (position == kInvalidIndex) {
      continue;
    }
    const detail::SpatialLbvhCoverRecord& record = batch.records[position];
    hash_word(digest, static_cast<std::uint64_t>(node_index));
    hash_word(digest, record.lower_squared_distance_bits);
    hash_word(digest, record.upper_squared_distance_bits);
    hash_word(digest, record.kind);
  }
  audit.proposal_digest_fnv1a = digest;

  std::vector<unsigned char> point_class(host.point_count, 0U);
  std::vector<unsigned char> record_used(batch.record_count, 0U);
  std::vector<std::pair<std::size_t, std::size_t>> traversal_stack{
      {host.root_index, 0U}};
  std::vector<std::size_t> traversal_width_by_depth;
  result.candidate_point_ids.reserve(host.point_count);
  result.certified_exterior_point_ids.reserve(host.point_count);
  while (!traversal_stack.empty()) {
    const auto [node_index, depth] = traversal_stack.back();
    traversal_stack.pop_back();
    if (node_index >= host.nodes.size() || depth >= host.nodes.size()) {
      throw std::logic_error(
          "the host spatial-LBVH traversal reached an invalid node");
    }
    if (traversal_width_by_depth.size() == depth) {
      traversal_width_by_depth.push_back(0U);
    }
    if (traversal_width_by_depth.size() <= depth) {
      throw std::logic_error(
          "the host spatial-LBVH traversal skipped one frontier depth");
    }
    ++traversal_width_by_depth[depth];
    ++audit.traversed_node_count;
    const detail::SpatialLbvhNodeInputRecord& node = host.nodes[node_index];
    const exact::ExactLevel exact_bound =
        exact_minimum_squared_distance(node, query);
    ++audit.cpu_exact_aabb_bound_evaluation_count;
    const std::size_t record_position = record_position_by_node[node_index];
    if (record_position != kInvalidIndex) {
      if (record_used[record_position] != 0U) {
        throw std::runtime_error(
            "the GPU spatial-LBVH cover reused a terminal record");
      }
      record_used[record_position] = 1U;
      const detail::SpatialLbvhCoverRecord& record =
          batch.records[record_position];
      require_interval_contains_exact_bound(record, exact_bound);
      if (record.kind == detail::spatial_lbvh_cover_prune_code) {
        ++audit.cpu_exact_prune_recertification_count;
        if (exact_bound <= squared_cutoff) {
          throw std::runtime_error(
              "the GPU spatial-LBVH cover attempted a false strict prune");
        }
        const exact::ExactLevel margin{
            exact_bound.rational() - squared_cutoff.rational()};
        if (!audit.minimum_certified_strict_margin.has_value() ||
            margin < *audit.minimum_certified_strict_margin) {
          audit.minimum_certified_strict_margin = margin;
        }
        ++audit.certified_pruned_subtree_count;
        for (std::uint64_t leaf_position = node.leaf_begin;
             leaf_position < node.leaf_end;
             ++leaf_position) {
          if (!std::in_range<std::size_t>(leaf_position) ||
              static_cast<std::size_t>(leaf_position) >=
                  host.leaf_point_ids.size()) {
            throw std::runtime_error(
                "a pruned spatial-LBVH range exceeds its leaf array");
          }
          const spatial::PointId point_id =
              host.leaf_point_ids[static_cast<std::size_t>(leaf_position)];
          const std::size_t point_index = static_cast<std::size_t>(point_id);
          if (point_index >= point_class.size() || point_class[point_index] != 0U) {
            throw std::runtime_error(
                "the spatial-LBVH cover repeated a pruned PointId");
          }
          point_class[point_index] = 2U;
          result.certified_exterior_point_ids.push_back(point_id);
        }
        continue;
      }

      const bool leaf =
          node.left_child == detail::spatial_bounds_sentinel_code &&
          node.right_child == detail::spatial_bounds_sentinel_code &&
          node.leaf_end == node.leaf_begin + 1U;
      if (!leaf || !std::in_range<std::size_t>(node.leaf_begin) ||
          static_cast<std::size_t>(node.leaf_begin) >=
              host.leaf_point_ids.size()) {
        throw std::runtime_error(
            "the GPU spatial-LBVH candidate record does not name one leaf");
      }
      const spatial::PointId point_id =
          host.leaf_point_ids[static_cast<std::size_t>(node.leaf_begin)];
      const std::size_t point_index = static_cast<std::size_t>(point_id);
      if (point_index >= point_class.size() || point_class[point_index] != 0U) {
        throw std::runtime_error(
            "the spatial-LBVH cover repeated a candidate PointId");
      }
      point_class[point_index] = 1U;
      result.candidate_point_ids.push_back(point_id);
      continue;
    }

    if (node.left_child == detail::spatial_bounds_sentinel_code ||
        node.right_child == detail::spatial_bounds_sentinel_code ||
        !std::in_range<std::size_t>(node.left_child) ||
        !std::in_range<std::size_t>(node.right_child)) {
      throw std::runtime_error(
          "the GPU spatial-LBVH cover omitted a leaf or exposed invalid children");
    }
    const std::size_t left_child =
        static_cast<std::size_t>(node.left_child);
    const std::size_t right_child =
        static_cast<std::size_t>(node.right_child);
    if (left_child >= node_index || right_child >= node_index ||
        left_child == right_child) {
      throw std::runtime_error(
          "the GPU spatial-LBVH cover exposed a cyclic topology");
    }
    ++audit.internal_node_expansion_count;
    traversal_stack.emplace_back(right_child, depth + 1U);
    traversal_stack.emplace_back(left_child, depth + 1U);
  }

  const std::size_t exact_peak_frontier_count = *std::max_element(
      traversal_width_by_depth.begin(), traversal_width_by_depth.end());
  const std::size_t exact_parallel_round_count =
      static_cast<std::size_t>(std::count_if(
          traversal_width_by_depth.begin(),
          traversal_width_by_depth.end(),
          [](std::size_t width) { return width > 1U; }));
  if (std::find(record_used.begin(), record_used.end(), 0U) !=
          record_used.end() ||
      std::find(point_class.begin(), point_class.end(), 0U) !=
          point_class.end() ||
      audit.traversed_node_count !=
          1U + 2U * audit.internal_node_expansion_count ||
      audit.cpu_exact_aabb_bound_evaluation_count !=
          audit.traversed_node_count ||
      audit.cpu_exact_prune_recertification_count !=
          audit.certified_pruned_subtree_count ||
      audit.certified_pruned_subtree_count !=
          audit.gpu_prune_proposal_count ||
      audit.gpu_processed_node_count != audit.traversed_node_count ||
      audit.gpu_traversal_round_count != traversal_width_by_depth.size() ||
      audit.gpu_peak_frontier_count != exact_peak_frontier_count ||
      audit.gpu_parallel_round_count != exact_parallel_round_count) {
    throw std::runtime_error(
        "the GPU spatial-LBVH records do not form one complete antichain cover");
  }
  std::sort(
      result.candidate_point_ids.begin(), result.candidate_point_ids.end());
  std::sort(
      result.certified_exterior_point_ids.begin(),
      result.certified_exterior_point_ids.end());
  audit.certified_pruned_point_count =
      result.certified_exterior_point_ids.size();
  audit.candidate_point_count = result.candidate_point_ids.size();
  if (audit.certified_pruned_point_count + audit.candidate_point_count !=
          host.point_count ||
      (audit.certified_pruned_subtree_count == 0U) !=
          !audit.minimum_certified_strict_margin.has_value()) {
    throw std::logic_error(
        "the certified spatial-LBVH cover does not partition the point cloud");
  }
  audit.cover_antichain_complete = true;
  audit.point_partition_complete = true;
  audit.cpu_exact_recertification_complete = true;
  return result;
}

}  // namespace

SpatialLbvhContext::SpatialLbvhContext(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud)
    : state_(std::make_shared<detail::SpatialLbvhContextState>()),
      host_(std::make_unique<detail::SpatialLbvhHostState>()) {
  if (!index.validated_for(cloud) || cloud.identity_ == nullptr ||
      cloud.size() == 0U) {
    throw std::invalid_argument(
        "a GPU spatial-LBVH context requires a validated nonempty index");
  }
  host_->cloud_identity = cloud.identity_;
  host_->point_count = cloud.size();
  host_->root_index = index.root_index_;
  host_->leaf_point_ids.reserve(index.leaves_.size());
  for (const spatial::MortonLeafRecord& leaf : index.leaves_) {
    host_->leaf_point_ids.push_back(leaf.point_id);
  }
  host_->nodes.reserve(index.nodes_.size());
  for (const spatial::MortonLbvhIndex::Node& node : index.nodes_) {
    detail::SpatialLbvhNodeInputRecord packed;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      packed.bounds.lower_bits[axis] =
          cloud.point(node.lower_point_ids[axis])
              .canonical_input_bits()[axis];
      packed.bounds.upper_bits[axis] =
          cloud.point(node.upper_point_ids[axis])
              .canonical_input_bits()[axis];
    }
    if (!std::in_range<std::uint64_t>(node.leaf_begin) ||
        !std::in_range<std::uint64_t>(node.leaf_end)) {
      throw std::length_error(
          "a spatial-LBVH leaf range is not representable on the GPU");
    }
    packed.leaf_begin = static_cast<std::uint64_t>(node.leaf_begin);
    packed.leaf_end = static_cast<std::uint64_t>(node.leaf_end);
    if (!node.is_leaf()) {
      if (!std::in_range<std::uint64_t>(node.left_child) ||
          !std::in_range<std::uint64_t>(node.right_child)) {
        throw std::length_error(
            "a spatial-LBVH child index is not representable on the GPU");
      }
      packed.left_child = static_cast<std::uint64_t>(node.left_child);
      packed.right_child = static_cast<std::uint64_t>(node.right_child);
    }
    host_->nodes.push_back(packed);
  }
  if (host_->nodes.empty() || host_->root_index >= host_->nodes.size() ||
      host_->leaf_point_ids.size() != host_->point_count ||
      host_->nodes.size() != 2U * host_->point_count - 1U) {
    throw std::logic_error(
        "the GPU spatial-LBVH snapshot did not preserve the validated tree");
  }
}

SpatialLbvhContext::~SpatialLbvhContext() noexcept = default;
SpatialLbvhContext::SpatialLbvhContext(SpatialLbvhContext&&) noexcept =
    default;
SpatialLbvhContext& SpatialLbvhContext::operator=(
    SpatialLbvhContext&&) noexcept = default;

void SpatialLbvhContext::require_matching_cloud(
    const spatial::CanonicalPointCloud& cloud) const {
  if (state_ == nullptr || host_ == nullptr ||
      host_->cloud_identity == nullptr || host_->point_count == 0U) {
    throw std::invalid_argument(
        "a moved-from GPU spatial-LBVH context is not queryable");
  }
  if (cloud.identity_ != host_->cloud_identity ||
      cloud.size() != host_->point_count) {
    throw std::invalid_argument(
        "the GPU spatial-LBVH context belongs to another PointId namespace");
  }
}

std::size_t SpatialLbvhContext::node_count() const noexcept {
  return host_ == nullptr ? 0U : host_->nodes.size();
}

std::size_t SpatialLbvhContext::point_count() const noexcept {
  return host_ == nullptr ? 0U : host_->point_count;
}

SpatialLbvhCoverResult SpatialLbvhContext::cover_strict_exterior(
    const spatial::CanonicalPointCloud& cloud,
    const ExactRational3& query,
    const exact::ExactLevel& squared_cutoff) {
  require_matching_cloud(cloud);
  const ExactRational3 canonical_query =
      spatial::detail::validated_query(query);
  const exact::ExactLevel canonical_cutoff =
      spatial::detail::validated_squared_radius(squared_cutoff);

  std::array<DirectedEnclosure, 3> query_enclosures{};
  std::array<std::uint64_t, 3> query_lower_bits{};
  std::array<std::uint64_t, 3> query_upper_bits{};
  for (std::size_t axis = 0U; axis < query_enclosures.size(); ++axis) {
    query_enclosures[axis] =
        enclose_rational(canonical_query.coordinate(axis));
    query_lower_bits[axis] = query_enclosures[axis].lower_bits;
    query_upper_bits[axis] = query_enclosures[axis].upper_bits;
  }
  const DirectedEnclosure cutoff_enclosure =
      enclose_nonnegative_rational(canonical_cutoff.rational());
  return state_->with_gpu_section([&] {
    if (!enclosure_supported(query_enclosures, cutoff_enclosure)) {
      return unsupported_cover(
          *host_, query_enclosures, cutoff_enclosure);
    }
    const detail::SpatialLbvhCoverBatch batch =
        detail::propose_strict_lbvh_cover_on_gpu(
            *state_,
            host_->nodes,
            host_->root_index,
            query_lower_bits,
            query_upper_bits,
            cutoff_enclosure.lower_bits,
            cutoff_enclosure.upper_bits);
    return validate_and_recertify_cover(
        batch,
        *host_,
        canonical_query,
        canonical_cutoff,
        query_enclosures,
        cutoff_enclosure);
  });
}

SpatialLbvhClosedBallResult SpatialLbvhContext::closed_ball(
    const spatial::CanonicalPointCloud& cloud,
    const ExactRational3& query,
    const exact::ExactLevel& squared_radius) {
  require_matching_cloud(cloud);
  const ExactRational3 canonical_query =
      spatial::detail::validated_query(query);
  const exact::ExactLevel canonical_radius =
      spatial::detail::validated_squared_radius(squared_radius);
  SpatialLbvhCoverResult cover = cover_strict_exterior(
      cloud, canonical_query, canonical_radius);

  std::vector<spatial::PointId> interior_ids;
  std::vector<spatial::PointId> shell_ids;
  std::vector<spatial::PointId> exterior_ids =
      std::move(cover.certified_exterior_point_ids);
  interior_ids.reserve(cover.candidate_point_ids.size());
  shell_ids.reserve(cover.candidate_point_ids.size());
  for (const spatial::PointId point_id : cover.candidate_point_ids) {
    const exact::ExactLevel distance = spatial::detail::exact_squared_distance(
        canonical_query, cloud.point(point_id));
    if (distance < canonical_radius) {
      interior_ids.push_back(point_id);
    } else if (distance == canonical_radius) {
      shell_ids.push_back(point_id);
    } else {
      exterior_ids.push_back(point_id);
    }
  }
  std::sort(exterior_ids.begin(), exterior_ids.end());

  spatial::SpatialQueryCounters counters;
  if (cover.audit.gpu_launch_count == 0U) {
    counters.method = spatial::SpatialQueryMethod::brute_force;
  } else {
    counters.method = spatial::SpatialQueryMethod::morton_lbvh;
    counters.node_visit_count = cover.audit.traversed_node_count;
    counters.internal_node_expansion_count =
        cover.audit.internal_node_expansion_count;
    counters.exact_aabb_bound_evaluation_count =
        cover.audit.cpu_exact_aabb_bound_evaluation_count;
    counters.pruned_subtree_count =
        cover.audit.certified_pruned_subtree_count;
    counters.pruned_eligible_point_count =
        cover.audit.certified_pruned_point_count;
    counters.minimum_strict_pruning_margin =
        cover.audit.minimum_certified_strict_margin;
  }
  counters.exact_point_distance_evaluation_count =
      cover.candidate_point_ids.size();
  spatial::ClosedBallPartition partition{
      cloud,
      canonical_radius,
      std::move(interior_ids),
      std::move(shell_ids),
      std::move(exterior_ids),
      std::move(counters)};
  cover.audit.cpu_exact_candidate_distance_evaluation_count =
      cover.candidate_point_ids.size();
  cover.audit.exact_partition_complete = true;
  return SpatialLbvhClosedBallResult{
      std::move(partition), std::move(cover.audit)};
}

SpatialLbvhTopKResult SpatialLbvhContext::top_k(
    const spatial::CanonicalPointCloud& cloud,
    const ExactRational3& query,
    std::size_t requested_rank,
    const spatial::ExclusionSet& exclusions) {
  require_matching_cloud(cloud);
  if (!exclusions.validated_for(cloud)) {
    throw std::invalid_argument(
        "the exclusion set belongs to another PointId namespace");
  }
  const std::size_t eligible_point_count =
      cloud.size() - exclusions.ids().size();
  if (requested_rank == 0U || requested_rank > eligible_point_count) {
    throw std::out_of_range(
        "the requested rank is outside the eligible point set");
  }
  const ExactRational3 canonical_query =
      spatial::detail::validated_query(query);

  std::vector<spatial::ExactNeighbor> seed_neighbors;
  seed_neighbors.reserve(requested_rank);
  std::optional<exact::ExactLevel> seed_cutoff;
  for (std::size_t point_index = 0U;
       point_index < cloud.size() && seed_neighbors.size() < requested_rank;
       ++point_index) {
    const spatial::PointId point_id =
        static_cast<spatial::PointId>(point_index);
    if (exclusions.contains(point_id)) {
      continue;
    }
    exact::ExactLevel distance = spatial::detail::exact_squared_distance(
        canonical_query, cloud.point(point_id));
    if (!seed_cutoff.has_value() || distance > *seed_cutoff) {
      seed_cutoff = distance;
    }
    seed_neighbors.push_back(
        spatial::ExactNeighbor{point_id, std::move(distance)});
  }
  if (seed_neighbors.size() != requested_rank || !seed_cutoff.has_value()) {
    throw std::logic_error(
        "the GPU spatial-LBVH top-k seed did not contain k eligible points");
  }

  SpatialLbvhCoverResult cover = cover_strict_exterior(
      cloud, canonical_query, *seed_cutoff);
  std::vector<spatial::ExactNeighbor> evaluated_neighbors;
  evaluated_neighbors.reserve(cover.candidate_point_ids.size());
  std::size_t reused_seed_count = 0U;
  std::size_t excluded_candidate_count = 0U;
  for (const spatial::PointId point_id : cover.candidate_point_ids) {
    if (exclusions.contains(point_id)) {
      ++excluded_candidate_count;
      continue;
    }
    const auto seed = std::find_if(
        seed_neighbors.begin(),
        seed_neighbors.end(),
        [point_id](const spatial::ExactNeighbor& neighbor) {
          return neighbor.point_id == point_id;
        });
    if (seed != seed_neighbors.end()) {
      evaluated_neighbors.push_back(*seed);
      ++reused_seed_count;
    } else {
      evaluated_neighbors.push_back(spatial::ExactNeighbor{
          point_id,
          spatial::detail::exact_squared_distance(
              canonical_query, cloud.point(point_id))});
    }
  }
  if (reused_seed_count != requested_rank) {
    throw std::logic_error(
        "a certified spatial-LBVH cover lost one of its top-k seed points");
  }
  if (evaluated_neighbors.size() + excluded_candidate_count !=
      cover.candidate_point_ids.size()) {
    throw std::logic_error(
        "the spatial-LBVH top-k candidates do not close over exclusions");
  }

  std::size_t pruned_eligible_point_count = 0U;
  for (const spatial::PointId point_id :
       cover.certified_exterior_point_ids) {
    if (!exclusions.contains(point_id)) {
      ++pruned_eligible_point_count;
    }
  }
  if (evaluated_neighbors.size() + pruned_eligible_point_count !=
      eligible_point_count) {
    throw std::logic_error(
        "the spatial-LBVH top-k cover does not partition eligible points");
  }

  spatial::SpatialQueryCounters counters;
  if (cover.audit.gpu_launch_count == 0U) {
    counters.method = spatial::SpatialQueryMethod::brute_force;
  } else {
    counters.method = spatial::SpatialQueryMethod::morton_lbvh;
    counters.node_visit_count = cover.audit.traversed_node_count;
    counters.internal_node_expansion_count =
        cover.audit.internal_node_expansion_count;
    counters.exact_aabb_bound_evaluation_count =
        cover.audit.cpu_exact_aabb_bound_evaluation_count;
    counters.pruned_subtree_count =
        cover.audit.certified_pruned_subtree_count;
    counters.pruned_eligible_point_count = pruned_eligible_point_count;
    counters.minimum_strict_pruning_margin =
        cover.audit.minimum_certified_strict_margin;
  }
  counters.excluded_point_count = exclusions.ids().size();
  counters.exact_point_distance_evaluation_count =
      evaluated_neighbors.size();
  spatial::TopKPartition partition =
      spatial::TopKPartition::from_evaluated_neighbors(
          cloud,
          requested_rank,
          std::move(evaluated_neighbors),
          eligible_point_count,
          std::move(counters));
  if (partition.cutoff_squared_distance() > *seed_cutoff) {
    throw std::logic_error(
        "the final top-k cutoff exceeds its exact seed upper bound");
  }

  cover.audit.cpu_exact_seed_distance_evaluation_count = requested_rank;
  cover.audit.cpu_exact_candidate_distance_evaluation_count =
      partition.distance_evaluation_count();
  cover.audit.excluded_candidate_point_count = excluded_candidate_count;
  cover.audit.certified_pruned_eligible_point_count =
      pruned_eligible_point_count;
  cover.audit.top_k_seed_squared_cutoff = *seed_cutoff;
  cover.audit.exact_partition_complete = true;
  return SpatialLbvhTopKResult{
      std::move(partition), std::move(cover.audit)};
}

SpatialLbvhTopKResult SpatialLbvhContext::nearest(
    const spatial::CanonicalPointCloud& cloud,
    const ExactRational3& query,
    const spatial::ExclusionSet& exclusions) {
  return top_k(cloud, query, 1U, exclusions);
}

}  // namespace morsehgp3d::gpu
