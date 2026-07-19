#include "morsehgp3d/gpu/k1_boruvka.hpp"

#include "phase5_k1_boruvka_internal.hpp"

#include "morsehgp3d/exact/rational.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

class K1BoruvkaCandidateHostState final {
 public:
  std::shared_ptr<const void> cloud_identity;
  std::size_t point_count{};
  std::size_t root_index{};
  std::vector<K1BoruvkaNodeInputRecord> nodes;
  std::vector<std::uint64_t> coordinate_bits;
  std::vector<std::uint64_t> morton_point_ids;
  std::vector<std::size_t> morton_position_by_point_id;
  std::uint64_t last_buffer_epoch{};
  bool rope_topology_certified{false};
  bool lbvh_topology_and_exact_aabbs_certified{false};
};

}  // namespace morsehgp3d::gpu::detail

namespace morsehgp3d::gpu {
namespace {

constexpr std::uint64_t kPositiveMaximumFiniteBits =
    UINT64_C(0x7fefffffffffffff);
constexpr std::uint64_t kPositiveInfinityBits =
    UINT64_C(0x7ff0000000000000);
constexpr std::uint64_t kFnvOffsetBasis =
    UINT64_C(14695981039346656037);
constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);

[[nodiscard]] spatial::PointId checked_point_id(std::size_t index) {
  if (index > static_cast<std::size_t>(
                  spatial::CanonicalPointCloud::max_point_id)) {
    throw std::length_error("a Phase 5 K1 Boruvka point exceeds PointId");
  }
  return static_cast<spatial::PointId>(index);
}

[[nodiscard]] std::size_t checked_point_index(
    spatial::PointId point_id,
    std::size_t point_count) {
  if (!std::in_range<std::size_t>(point_id)) {
    throw std::runtime_error(
        "a Phase 5 K1 Boruvka PointId does not fit size_t");
  }
  const std::size_t index = static_cast<std::size_t>(point_id);
  if (index >= point_count) {
    throw std::runtime_error(
        "a Phase 5 K1 Boruvka PointId is outside the cloud");
  }
  return index;
}

[[nodiscard]] spatial::PointId checked_record_point_id(
    std::uint64_t point_id,
    std::size_t point_count) {
  if (!std::in_range<spatial::PointId>(point_id)) {
    throw std::runtime_error(
        "a Phase 5 K1 Boruvka candidate PointId does not fit PointId");
  }
  const spatial::PointId checked = static_cast<spatial::PointId>(point_id);
  (void)checked_point_index(checked, point_count);
  return checked;
}

[[nodiscard]] std::size_t checked_size_sum(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_size_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

constexpr std::size_t kCandidateRecordSizeBytes =
    sizeof(detail::K1BoruvkaCandidateRecord);
constexpr std::size_t kCandidatePayloadCopies = 2U;
static_assert(kCandidateRecordSizeBytes == 16U);
static_assert(
    kCandidatePayloadCopies * kCandidateRecordSizeBytes == 32U);

void validate_chunking_policy(K1BoruvkaChunkingPolicy policy) {
  if (policy.max_candidate_records_per_chunk == 0U) {
    throw std::invalid_argument(
        "a Phase 5 K1 Boruvka candidate chunk budget must be nonzero");
  }
  (void)checked_size_product(
      policy.max_candidate_records_per_chunk,
      kCandidatePayloadCopies * kCandidateRecordSizeBytes,
      "the Phase 5 K1 Boruvka candidate chunk byte budget overflows size_t");
}

void validate_morton_seed_policy(K1BoruvkaMortonSeedPolicy policy) {
  if (policy.window_radius == 0U) {
    throw std::invalid_argument(
        "a Phase 5 K1 Boruvka Morton seed window must be nonzero");
  }
  if (policy.window_radius >
      std::numeric_limits<std::size_t>::max() / 2U) {
    throw std::invalid_argument(
        "the Phase 5 K1 Boruvka Morton seed inspection bound overflows size_t");
  }
}

void validate_component_envelope_mode(
    K1BoruvkaComponentEnvelopeMode mode) {
  if (mode != K1BoruvkaComponentEnvelopeMode::frozen_initial &&
      mode !=
          K1BoruvkaComponentEnvelopeMode::sparse_witness_path_monotone) {
    throw std::invalid_argument(
        "a component-direct dual-tree envelope mode is unspecified");
  }
}

[[nodiscard]] exact::ExactLevel exact_squared_distance(
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId left_id,
    spatial::PointId right_id) {
  const exact::ExactRational3& left = cloud.point(left_id).exact();
  const exact::ExactRational3& right = cloud.point(right_id).exact();
  const exact::BigInt common_denominator =
      left.denominator() * right.denominator();
  exact::BigInt squared_numerator = 0;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::BigInt difference =
        left.numerator(axis) * right.denominator() -
        right.numerator(axis) * left.denominator();
    squared_numerator += difference * difference;
  }
  return exact::ExactLevel{
      std::move(squared_numerator),
      common_denominator * common_denominator};
}

[[nodiscard]] exact::ExactLevel merge_level(
    const exact::ExactLevel& squared_length) {
  return exact::ExactLevel{
      squared_length.numerator(),
      squared_length.denominator() * exact::BigInt{4}};
}

[[nodiscard]] hierarchy::ExactEmstEdge exact_edge(
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId left,
    spatial::PointId right) {
  if (left == right) {
    throw std::runtime_error("a Phase 5 K1 Boruvka candidate is a loop");
  }
  const spatial::PointId u = std::min(left, right);
  const spatial::PointId v = std::max(left, right);
  exact::ExactLevel squared_length = exact_squared_distance(cloud, u, v);
  if (squared_length == exact::ExactLevel{}) {
    throw std::runtime_error(
        "a Phase 5 K1 Boruvka candidate joins duplicate geometry");
  }
  exact::ExactLevel level = merge_level(squared_length);
  return hierarchy::ExactEmstEdge{
      u, v, std::move(squared_length), std::move(level)};
}

[[nodiscard]] bool edge_less(
    const hierarchy::ExactEmstEdge& left,
    const hierarchy::ExactEmstEdge& right) {
  if (left.squared_length != right.squared_length) {
    return left.squared_length < right.squared_length;
  }
  if (left.u != right.u) {
    return left.u < right.u;
  }
  return left.v < right.v;
}

[[nodiscard]] std::uint64_t exact_upper_binary64_bits(
    const exact::ExactLevel& value) {
  const exact::ExactRational maximum =
      exact::ExactRational::from_binary64_bits(kPositiveMaximumFiniteBits);
  if (value.rational() > maximum) {
    return kPositiveInfinityBits;
  }

  std::uint64_t lower_bits = 0U;
  std::uint64_t upper_search_bits = kPositiveMaximumFiniteBits;
  while (lower_bits < upper_search_bits) {
    const std::uint64_t midpoint_bits =
        lower_bits + (upper_search_bits - lower_bits + 1U) / 2U;
    if (exact::ExactRational::from_binary64_bits(midpoint_bits) <=
        value.rational()) {
      lower_bits = midpoint_bits;
    } else {
      upper_search_bits = midpoint_bits - 1U;
    }
  }
  if (exact::ExactRational::from_binary64_bits(lower_bits) ==
      value.rational()) {
    return lower_bits;
  }
  if (lower_bits == kPositiveMaximumFiniteBits) {
    return kPositiveInfinityBits;
  }
  return lower_bits + 1U;
}

[[nodiscard]] exact::ExactLevel exact_aabb_lower_bound(
    const detail::K1BoruvkaNodeInputRecord& node,
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId query_id) {
  const exact::ExactRational3& query = cloud.point(query_id).exact();
  exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational coordinate = query.coordinate(axis);
    const exact::ExactRational lower =
        exact::ExactRational::from_binary64_bits(node.lower_bits[axis]);
    const exact::ExactRational upper =
        exact::ExactRational::from_binary64_bits(node.upper_bits[axis]);
    exact::ExactRational delta;
    if (coordinate < lower) {
      delta = lower - coordinate;
    } else if (coordinate > upper) {
      delta = coordinate - upper;
    }
    squared_distance = squared_distance + delta * delta;
  }
  return exact::ExactLevel{std::move(squared_distance)};
}

[[nodiscard]] exact::ExactLevel exact_aabb_pair_lower_bound(
    const detail::K1BoruvkaNodeInputRecord& left,
    const detail::K1BoruvkaNodeInputRecord& right) {
  exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational left_lower =
        exact::ExactRational::from_binary64_bits(left.lower_bits[axis]);
    const exact::ExactRational left_upper =
        exact::ExactRational::from_binary64_bits(left.upper_bits[axis]);
    const exact::ExactRational right_lower =
        exact::ExactRational::from_binary64_bits(right.lower_bits[axis]);
    const exact::ExactRational right_upper =
        exact::ExactRational::from_binary64_bits(right.upper_bits[axis]);
    exact::ExactRational delta;
    if (left_upper < right_lower) {
      delta = right_lower - left_upper;
    } else if (right_upper < left_lower) {
      delta = left_lower - right_upper;
    }
    squared_distance = squared_distance + delta * delta;
  }
  return exact::ExactLevel{std::move(squared_distance)};
}

[[nodiscard]] exact::ExactLevel exact_aabb_squared_diagonal(
    const detail::K1BoruvkaNodeInputRecord& node) {
  exact::ExactRational squared_diagonal;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational lower =
        exact::ExactRational::from_binary64_bits(node.lower_bits[axis]);
    const exact::ExactRational upper =
        exact::ExactRational::from_binary64_bits(node.upper_bits[axis]);
    const exact::ExactRational extent = upper - lower;
    squared_diagonal = squared_diagonal + extent * extent;
  }
  return exact::ExactLevel{std::move(squared_diagonal)};
}

[[nodiscard]] std::size_t validate_frozen_labels(
    std::span<const spatial::PointId> labels,
    std::size_t point_count) {
  if (labels.size() != point_count) {
    throw std::invalid_argument(
        "a Phase 5 K1 Boruvka round needs one frozen label per point");
  }
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    if (!std::in_range<std::size_t>(labels[point_index]) ||
        static_cast<std::size_t>(labels[point_index]) >= point_count) {
      throw std::invalid_argument(
          "a frozen component label is outside the cloud");
    }
    const std::size_t label =
        static_cast<std::size_t>(labels[point_index]);
    if (label > point_index) {
      throw std::invalid_argument(
          "a frozen component label is not its least PointId");
    }
  }

  std::size_t component_count = 0U;
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    const std::size_t label = static_cast<std::size_t>(labels[point_index]);
    if (labels[label] != checked_point_id(label)) {
      throw std::invalid_argument(
          "a frozen component label is not a fixed representative");
    }
    if (label == point_index) {
      ++component_count;
    }
  }
  if (component_count == 0U) {
    throw std::invalid_argument("a frozen partition has no component");
  }
  return component_count;
}

[[nodiscard]] std::vector<std::uint64_t> build_component_tags(
    const detail::K1BoruvkaCandidateHostState& host,
    std::span<const spatial::PointId> labels,
    std::size_t& uniform_count,
    std::size_t& mixed_count) {
  std::vector<std::uint64_t> tags(
      host.nodes.size(), detail::k1_boruvka_mixed_component);
  uniform_count = 0U;
  mixed_count = 0U;
  for (std::size_t node_index = 0U;
       node_index < host.nodes.size();
       ++node_index) {
    const detail::K1BoruvkaNodeInputRecord& node = host.nodes[node_index];
    std::uint64_t tag = detail::k1_boruvka_mixed_component;
    if (node.leaf_point_id != detail::k1_boruvka_sentinel) {
      const std::size_t point_index = checked_point_index(
          static_cast<spatial::PointId>(node.leaf_point_id),
          host.point_count);
      tag = labels[point_index];
    } else {
      if (node.left_child >= node_index || node.right_child >= node_index) {
        throw std::logic_error(
            "the Phase 5 K1 Boruvka LBVH is not postorder");
      }
      const std::uint64_t left_tag =
          tags[static_cast<std::size_t>(node.left_child)];
      const std::uint64_t right_tag =
          tags[static_cast<std::size_t>(node.right_child)];
      if (left_tag != detail::k1_boruvka_mixed_component &&
          left_tag == right_tag) {
        tag = left_tag;
      }
    }
    tags[node_index] = tag;
    if (tag == detail::k1_boruvka_mixed_component) {
      ++mixed_count;
    } else {
      ++uniform_count;
    }
  }
  if (uniform_count + mixed_count != host.nodes.size()) {
    throw std::logic_error(
        "the Phase 5 K1 Boruvka tags do not cover the LBVH");
  }
  return tags;
}

struct SeedBundle {
  std::vector<K1BoruvkaSeed> public_seeds;
  std::vector<std::uint64_t> upper_bits;
  K1BoruvkaMortonSeedAudit morton_audit;
  K1BoruvkaSeedStatus status{K1BoruvkaSeedStatus::not_certified};
};

[[nodiscard]] SeedBundle build_canonical_fallback_seeds(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> labels) {
  const std::size_t point_count = cloud.size();
  std::size_t second_component_point = point_count;
  for (std::size_t point_index = 1U;
       point_index < point_count;
       ++point_index) {
    if (labels[point_index] != labels[0]) {
      second_component_point = point_index;
      break;
    }
  }
  if (second_component_point == point_count) {
    throw std::logic_error(
        "a nonterminal Phase 5 K1 Boruvka partition has one component");
  }

  SeedBundle bundle;
  bundle.public_seeds.reserve(point_count);
  bundle.upper_bits.reserve(point_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    const std::size_t target_index =
        labels[point_index] == labels[0] ? second_component_point : 0U;
    const spatial::PointId source_id = checked_point_id(point_index);
    const spatial::PointId target_id = checked_point_id(target_index);
    exact::ExactLevel cutoff =
        exact_squared_distance(cloud, source_id, target_id);
    if (cutoff == exact::ExactLevel{}) {
      throw std::logic_error(
          "a Phase 5 K1 Boruvka seed joins duplicate geometry");
    }
    bundle.upper_bits.push_back(exact_upper_binary64_bits(cutoff));
    bundle.public_seeds.push_back(
        K1BoruvkaSeed{source_id, target_id, std::move(cutoff)});
  }
  return bundle;
}

[[nodiscard]] hierarchy::ExactEmstEdge seed_edge(
    const K1BoruvkaSeed& seed) {
  const spatial::PointId u =
      std::min(seed.source_point_id, seed.target_point_id);
  const spatial::PointId v =
      std::max(seed.source_point_id, seed.target_point_id);
  return hierarchy::ExactEmstEdge{
      u,
      v,
      seed.exact_squared_cutoff,
      merge_level(seed.exact_squared_cutoff)};
}

[[nodiscard]] SeedBundle build_morton_refined_seeds(
    const detail::K1BoruvkaCandidateHostState& host,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> labels,
    K1BoruvkaMortonSeedPolicy policy,
    const detail::K1BoruvkaMortonSeedProposalBatch& batch) {
  validate_morton_seed_policy(policy);
  const std::size_t point_count = host.point_count;
  if (host.morton_point_ids.size() != point_count ||
      host.morton_position_by_point_id.size() != point_count ||
      batch.records.size() != point_count ||
      batch.window_radius != policy.window_radius ||
      batch.kernel_launch_count != 1U ||
      batch.synchronization_count != 1U ||
      !batch.complete_source_coverage || !batch.bounded_window) {
    throw std::runtime_error(
        "the GPU Morton seed proposal has invalid global metadata");
  }

  SeedBundle bundle = build_canonical_fallback_seeds(cloud, labels);
  K1BoruvkaMortonSeedAudit& audit = bundle.morton_audit;
  audit.source_count = point_count;
  audit.window_radius = policy.window_radius;
  audit.neighbor_inspection_budget_per_source = std::min(
      point_count - 1U, 2U * policy.window_radius);
  audit.exact_seed_distance_evaluation_count = point_count;
  audit.gpu_kernel_launch_count = batch.kernel_launch_count;
  audit.gpu_synchronization_count = batch.synchronization_count;

  std::size_t inspected_total = 0U;
  std::size_t external_total = 0U;
  std::size_t proposed_total = 0U;
  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    const detail::K1BoruvkaMortonSeedProposalRecord& record =
        batch.records[source_index];
    const std::size_t morton_position =
        host.morton_position_by_point_id[source_index];
    if (morton_position >= point_count) {
      throw std::logic_error(
          "the Phase 5 K1 Boruvka Morton inverse permutation is invalid");
    }
    const std::size_t expected_left =
        std::min(policy.window_radius, morton_position);
    const std::size_t expected_right = std::min(
        policy.window_radius, point_count - 1U - morton_position);
    const std::size_t expected_inspected =
        expected_left + expected_right;
    if (record.failure_code != 0U ||
        record.inspected_neighbor_count != expected_inspected ||
        record.external_neighbor_count >
            record.inspected_neighbor_count) {
      throw std::runtime_error(
          "a GPU Morton seed record violates its bounded source window");
    }
    inspected_total = checked_size_sum(
        inspected_total,
        static_cast<std::size_t>(record.inspected_neighbor_count),
        "the Phase 5 K1 Boruvka Morton inspection total overflowed");
    external_total = checked_size_sum(
        external_total,
        static_cast<std::size_t>(record.external_neighbor_count),
        "the Phase 5 K1 Boruvka Morton external-neighbor total overflowed");
    audit.maximum_inspected_neighbor_count_per_source = std::max(
        audit.maximum_inspected_neighbor_count_per_source,
        static_cast<std::size_t>(record.inspected_neighbor_count));

    const bool has_proposal =
        record.target_point_id != detail::k1_boruvka_sentinel;
    if (has_proposal != (record.external_neighbor_count != 0U)) {
      throw std::runtime_error(
          "a GPU Morton seed record disagrees with its external-neighbor count");
    }
    if (!has_proposal) {
      continue;
    }
    proposed_total = checked_size_sum(
        proposed_total,
        1U,
        "the Phase 5 K1 Boruvka Morton proposal total overflowed");
    const spatial::PointId target_id = checked_record_point_id(
        record.target_point_id, point_count);
    const std::size_t target_index =
        checked_point_index(target_id, point_count);
    const std::size_t target_morton_position =
        host.morton_position_by_point_id[target_index];
    const std::size_t morton_separation =
        morton_position > target_morton_position
            ? morton_position - target_morton_position
            : target_morton_position - morton_position;
    if (labels[target_index] == labels[source_index] ||
        morton_separation == 0U ||
        morton_separation > policy.window_radius) {
      throw std::runtime_error(
          "a GPU Morton seed target is internal or outside its trusted window");
    }

    K1BoruvkaSeed& fallback = bundle.public_seeds[source_index];
    if (target_id == fallback.target_point_id) {
      continue;
    }
    hierarchy::ExactEmstEdge proposed_edge = exact_edge(
        cloud, checked_point_id(source_index), target_id);
    audit.exact_seed_distance_evaluation_count = checked_size_sum(
        audit.exact_seed_distance_evaluation_count,
        1U,
        "the Phase 5 K1 Boruvka exact seed evaluation count overflowed");
    const hierarchy::ExactEmstEdge fallback_edge = seed_edge(fallback);
    if (edge_less(proposed_edge, fallback_edge)) {
      const bool strict_improvement =
          proposed_edge.squared_length < fallback_edge.squared_length;
      fallback.target_point_id = target_id;
      fallback.exact_squared_cutoff = proposed_edge.squared_length;
      bundle.upper_bits[source_index] =
          exact_upper_binary64_bits(fallback.exact_squared_cutoff);
      ++audit.exact_selected_proposal_count;
      if (strict_improvement) {
        ++audit.exact_strict_improvement_count;
      }
    }
  }

  if (inspected_total != batch.inspected_neighbor_count ||
      external_total != batch.external_neighbor_count ||
      proposed_total != batch.proposed_seed_count ||
      audit.maximum_inspected_neighbor_count_per_source >
          audit.neighbor_inspection_budget_per_source) {
    throw std::runtime_error(
        "the GPU Morton seed proposal counters do not close");
  }
  audit.inspected_neighbor_count = inspected_total;
  audit.external_neighbor_count = external_total;
  audit.floating_proposal_count = proposed_total;
  audit.exact_fallback_count =
      point_count - audit.exact_selected_proposal_count;
  audit.complete_source_coverage_certified = true;
  audit.bounded_window_certified = true;
  audit.external_targets_recertified = true;
  audit.exact_monotone_cutoff_certified = true;
  bundle.status = K1BoruvkaSeedStatus::
      bounded_morton_window_external_exact_monotone_certified;
  return bundle;
}

void hash_word(std::uint64_t& digest, std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
}

[[nodiscard]] std::uint64_t proposal_digest(
    std::span<const std::size_t> offsets,
    std::span<const K1BoruvkaCandidate> candidates) {
  std::uint64_t digest = kFnvOffsetBasis;
  for (const std::size_t offset : offsets) {
    hash_word(digest, static_cast<std::uint64_t>(offset));
  }
  for (const K1BoruvkaCandidate& candidate : candidates) {
    hash_word(digest, candidate.source_point_id);
    hash_word(digest, candidate.target_point_id);
  }
  return digest;
}

[[nodiscard]] std::uint64_t terminal_proposal_digest(
    std::size_t point_count) noexcept {
  std::uint64_t digest = kFnvOffsetBasis;
  for (std::size_t point_index = 0U;
       point_index <= point_count;
       ++point_index) {
    hash_word(digest, 0U);
  }
  return digest;
}

struct ResolvedCandidate {
  hierarchy::ExactEmstEdge edge;
  spatial::PointId source_point_id{};
};

[[nodiscard]] bool resolved_less(
    const ResolvedCandidate& left,
    const ResolvedCandidate& right) {
  if (edge_less(left.edge, right.edge)) {
    return true;
  }
  if (edge_less(right.edge, left.edge)) {
    return false;
  }
  return left.source_point_id < right.source_point_id;
}

struct ExactSearchQueueEntry {
  exact::ExactLevel lower_bound;
  std::size_t node_index{};
};

struct ExactSearchNearestFirst {
  [[nodiscard]] bool operator()(
      const ExactSearchQueueEntry& left,
      const ExactSearchQueueEntry& right) const {
    if (left.lower_bound != right.lower_bound) {
      return left.lower_bound > right.lower_bound;
    }
    return left.node_index > right.node_index;
  }
};

struct DualTreeFrontierEntry {
  exact::ExactLevel lower_bound;
  std::size_t left_node_index{};
  std::size_t right_node_index{};
};

struct DualTreeTraversalMetadataView {
  std::span<const exact::ExactLevel> node_squared_diagonals;
  std::span<const std::size_t> node_leaf_counts;
  std::span<const std::size_t> node_leaf_begins;
  std::span<const std::size_t> node_leaf_ends;
  std::size_t lbvh_maximum_depth{};
};

template <typename Audit, typename RelaxLeafPair>
void traverse_exact_dual_tree_frontier(
    const detail::K1BoruvkaCandidateHostState& host,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> labels,
    const std::vector<std::uint64_t>& tags,
    const DualTreeTraversalMetadataView& metadata,
    std::span<const exact::ExactLevel> node_cutoff_upper_bounds,
    Audit& audit,
    RelaxLeafPair&& relax_leaf_pair) {
  const std::size_t point_count = host.point_count;
  const std::size_t node_count = host.nodes.size();
  if (point_count <= 1U || labels.size() != point_count ||
      tags.size() != node_count ||
      metadata.node_squared_diagonals.size() != node_count ||
      metadata.node_leaf_counts.size() != node_count ||
      metadata.node_leaf_begins.size() != node_count ||
      metadata.node_leaf_ends.size() != node_count ||
      metadata.lbvh_maximum_depth == 0U ||
      node_cutoff_upper_bounds.size() != node_count) {
    throw std::logic_error(
        "the exact dual-tree DFS metadata is incomplete");
  }

  std::size_t pair_factor_left = point_count;
  std::size_t pair_factor_right = point_count - 1U;
  if ((pair_factor_left & 1U) == 0U) {
    pair_factor_left /= 2U;
  } else {
    pair_factor_right /= 2U;
  }
  audit.unordered_point_pair_count = checked_size_product(
      pair_factor_left,
      pair_factor_right,
      "the exact dual-tree unordered point-pair bound overflowed size_t");
  audit.lbvh_maximum_depth = metadata.lbvh_maximum_depth;
  audit.certified_depth_first_frontier_bound = checked_size_sum(
      checked_size_product(
          metadata.lbvh_maximum_depth,
          2U,
          "the exact dual-tree DFS frontier bound overflowed size_t"),
      1U,
      "the exact dual-tree DFS frontier bound overflowed size_t");
  const std::size_t maximum_terminal_count = checked_size_sum(
      audit.unordered_point_pair_count,
      point_count,
      "the exact dual-tree terminal-count bound overflowed size_t");
  audit.certified_node_pair_visit_bound = checked_size_sum(
      checked_size_product(
          maximum_terminal_count - 1U,
          2U,
          "the exact dual-tree visit bound overflowed size_t"),
      1U,
      "the exact dual-tree visit bound overflowed size_t");

  const auto close_node_pair = [&](std::size_t first, std::size_t second) {
    std::size_t covered_count = 0U;
    if (first == second) {
      std::size_t first_factor = metadata.node_leaf_counts[first];
      std::size_t second_factor = first_factor - 1U;
      if ((first_factor & 1U) == 0U) {
        first_factor /= 2U;
      } else {
        second_factor /= 2U;
      }
      covered_count = checked_size_product(
          first_factor,
          second_factor,
          "the exact dual-tree self-pair coverage overflowed size_t");
    } else {
      covered_count = checked_size_product(
          metadata.node_leaf_counts[first],
          metadata.node_leaf_counts[second],
          "the exact dual-tree cross-pair coverage overflowed size_t");
    }
    audit.covered_unordered_point_pair_count = checked_size_sum(
        audit.covered_unordered_point_pair_count,
        covered_count,
        "the exact dual-tree total pair coverage overflowed size_t");
  };

  std::vector<DualTreeFrontierEntry> frontier;
  frontier.reserve(audit.certified_depth_first_frontier_bound);
  const auto make_pair_entry = [&](std::size_t first, std::size_t second) {
    if (first >= node_count || second >= node_count) {
      throw std::logic_error(
          "an exact dual-tree node pair is outside the LBVH");
    }
    if (second < first) {
      std::swap(first, second);
    }
    const bool disjoint_ranges =
        metadata.node_leaf_ends[first] <=
            metadata.node_leaf_begins[second] ||
        metadata.node_leaf_ends[second] <=
            metadata.node_leaf_begins[first];
    if (first != second && !disjoint_ranges) {
      throw std::logic_error(
          "an exact dual-tree pair contains overlapping leaf ranges");
    }
    exact::ExactLevel lower_bound = exact_aabb_pair_lower_bound(
        host.nodes[first], host.nodes[second]);
    audit.cpu_exact_aabb_pair_bound_evaluation_count = checked_size_sum(
        audit.cpu_exact_aabb_pair_bound_evaluation_count,
        1U,
        "the exact dual-tree AABB-pair count overflowed size_t");
    return DualTreeFrontierEntry{
        std::move(lower_bound), first, second};
  };
  const auto push_pairs_near_first =
      [&make_pair_entry, &frontier, &audit](
          std::initializer_list<std::pair<std::size_t, std::size_t>> pairs) {
    std::vector<DualTreeFrontierEntry> entries;
    entries.reserve(pairs.size());
    for (const auto& [first, second] : pairs) {
      entries.push_back(make_pair_entry(first, second));
    }
    std::sort(
        entries.begin(),
        entries.end(),
        [](const DualTreeFrontierEntry& left,
           const DualTreeFrontierEntry& right) {
          if (left.lower_bound != right.lower_bound) {
            return left.lower_bound > right.lower_bound;
          }
          if (left.left_node_index != right.left_node_index) {
            return left.left_node_index > right.left_node_index;
          }
          return left.right_node_index > right.right_node_index;
        });
    if (frontier.size() >
            audit.certified_depth_first_frontier_bound ||
        entries.size() >
            audit.certified_depth_first_frontier_bound -
                frontier.size()) {
      throw std::logic_error(
          "the exact dual-tree DFS frontier exceeded its depth bound");
    }
    frontier.insert(
        frontier.end(),
        std::make_move_iterator(entries.begin()),
        std::make_move_iterator(entries.end()));
    audit.maximum_cpu_frontier_size = std::max(
        audit.maximum_cpu_frontier_size, frontier.size());
  };
  push_pairs_near_first({{host.root_index, host.root_index}});

  while (!frontier.empty()) {
    DualTreeFrontierEntry entry = std::move(frontier.back());
    frontier.pop_back();
    audit.cpu_node_pair_visit_count = checked_size_sum(
        audit.cpu_node_pair_visit_count,
        1U,
        "the exact dual-tree node-pair visit count overflowed size_t");

    const std::uint64_t left_tag = tags[entry.left_node_index];
    const std::uint64_t right_tag = tags[entry.right_node_index];
    if (left_tag != detail::k1_boruvka_mixed_component &&
        left_tag == right_tag) {
      close_node_pair(entry.left_node_index, entry.right_node_index);
      audit.cpu_uniform_same_component_pair_prune_count = checked_size_sum(
          audit.cpu_uniform_same_component_pair_prune_count,
          1U,
          "the exact dual-tree uniform prune count overflowed size_t");
      continue;
    }

    const exact::ExactLevel& left_cutoff =
        node_cutoff_upper_bounds[entry.left_node_index];
    const exact::ExactLevel& right_cutoff =
        node_cutoff_upper_bounds[entry.right_node_index];
    const exact::ExactLevel& pair_cutoff =
        left_cutoff < right_cutoff ? right_cutoff : left_cutoff;
    if (entry.lower_bound > pair_cutoff) {
      close_node_pair(entry.left_node_index, entry.right_node_index);
      audit.cpu_strict_aabb_pair_prune_count = checked_size_sum(
          audit.cpu_strict_aabb_pair_prune_count,
          1U,
          "the exact dual-tree strict prune count overflowed size_t");
      continue;
    }

    const detail::K1BoruvkaNodeInputRecord& left_node =
        host.nodes[entry.left_node_index];
    const detail::K1BoruvkaNodeInputRecord& right_node =
        host.nodes[entry.right_node_index];
    const bool left_is_leaf =
        left_node.leaf_point_id != detail::k1_boruvka_sentinel;
    const bool right_is_leaf =
        right_node.leaf_point_id != detail::k1_boruvka_sentinel;
    if (left_is_leaf && right_is_leaf) {
      const spatial::PointId left_id = checked_record_point_id(
          left_node.leaf_point_id, point_count);
      const spatial::PointId right_id = checked_record_point_id(
          right_node.leaf_point_id, point_count);
      if (left_id == right_id ||
          labels[checked_point_index(left_id, point_count)] ==
              labels[checked_point_index(right_id, point_count)]) {
        throw std::logic_error(
            "an exact dual-tree internal leaf pair escaped uniform pruning");
      }
      hierarchy::ExactEmstEdge edge = exact_edge(
          cloud, left_id, right_id);
      close_node_pair(entry.left_node_index, entry.right_node_index);
      audit.cpu_exact_point_pair_distance_evaluation_count = checked_size_sum(
          audit.cpu_exact_point_pair_distance_evaluation_count,
          1U,
          "the exact dual-tree point-pair count overflowed size_t");
      relax_leaf_pair(std::move(edge), left_id, right_id);
      continue;
    }

    audit.cpu_node_pair_expansion_count = checked_size_sum(
        audit.cpu_node_pair_expansion_count,
        1U,
        "the exact dual-tree node-pair expansion count overflowed size_t");
    if (entry.left_node_index == entry.right_node_index) {
      if (left_is_leaf) {
        throw std::logic_error(
            "an exact dual-tree leaf self-pair escaped uniform pruning");
      }
      const std::size_t left_child =
          static_cast<std::size_t>(left_node.left_child);
      const std::size_t right_child =
          static_cast<std::size_t>(left_node.right_child);
      push_pairs_near_first({
          {left_child, left_child},
          {left_child, right_child},
          {right_child, right_child}});
      continue;
    }

    bool split_left = false;
    if (!left_is_leaf && right_is_leaf) {
      split_left = true;
    } else if (left_is_leaf && !right_is_leaf) {
      split_left = false;
    } else if (!left_is_leaf && !right_is_leaf) {
      const exact::ExactLevel& left_diagonal =
          metadata.node_squared_diagonals[entry.left_node_index];
      const exact::ExactLevel& right_diagonal =
          metadata.node_squared_diagonals[entry.right_node_index];
      if (left_diagonal != right_diagonal) {
        split_left = left_diagonal > right_diagonal;
      } else if (metadata.node_leaf_counts[entry.left_node_index] !=
                 metadata.node_leaf_counts[entry.right_node_index]) {
        split_left =
            metadata.node_leaf_counts[entry.left_node_index] >
            metadata.node_leaf_counts[entry.right_node_index];
      } else {
        split_left = entry.left_node_index > entry.right_node_index;
      }
    } else {
      throw std::logic_error(
          "an exact dual-tree external leaf pair was not evaluated");
    }

    if (split_left) {
      push_pairs_near_first({
          {static_cast<std::size_t>(left_node.left_child),
           entry.right_node_index},
          {static_cast<std::size_t>(left_node.right_child),
           entry.right_node_index}});
    } else {
      push_pairs_near_first({
          {entry.left_node_index,
           static_cast<std::size_t>(right_node.left_child)},
          {entry.left_node_index,
           static_cast<std::size_t>(right_node.right_child)}});
    }
  }

  const std::size_t terminal_count = checked_size_sum(
      audit.cpu_uniform_same_component_pair_prune_count,
      checked_size_sum(
          audit.cpu_strict_aabb_pair_prune_count,
          audit.cpu_exact_point_pair_distance_evaluation_count,
          "the exact dual-tree terminal partition overflowed size_t"),
      "the exact dual-tree terminal partition overflowed size_t");
  const std::size_t partitioned_visit_count = checked_size_sum(
      audit.cpu_node_pair_expansion_count,
      terminal_count,
      "the exact dual-tree visited-pair partition overflowed size_t");
  bool expansion_arity_closed = false;
  if (audit.cpu_node_pair_visit_count >= 2U) {
    const std::size_t edge_count = audit.cpu_node_pair_visit_count - 1U;
    const std::size_t minimum_expansion_count =
        edge_count / 3U + (edge_count % 3U == 0U ? 0U : 1U);
    const std::size_t maximum_expansion_count =
        (audit.cpu_node_pair_visit_count - 2U) / 2U;
    expansion_arity_closed =
        minimum_expansion_count <= audit.cpu_node_pair_expansion_count &&
        audit.cpu_node_pair_expansion_count <= maximum_expansion_count;
  }
  if (audit.cpu_exact_aabb_pair_bound_evaluation_count !=
          audit.cpu_node_pair_visit_count ||
      partitioned_visit_count != audit.cpu_node_pair_visit_count ||
      !expansion_arity_closed ||
      audit.cpu_exact_point_pair_distance_evaluation_count >
          audit.unordered_point_pair_count ||
      audit.covered_unordered_point_pair_count !=
          audit.unordered_point_pair_count ||
      audit.maximum_cpu_frontier_size >
          audit.certified_depth_first_frontier_bound ||
      audit.cpu_node_pair_visit_count >
          audit.certified_node_pair_visit_bound) {
    throw std::logic_error(
        "the exact dual-tree DFS did not close its certified bounds");
  }
  audit.canonical_unordered_pair_partition_certified = true;
  audit.uniform_component_pair_prunes_certified = true;
  audit.strict_only_aabb_pair_pruning_certified = true;
  audit.depth_first_frontier_bound_certified = true;
  audit.node_pair_visit_bound_certified = true;
  audit.complete_frontier_exhaustion_certified = true;
}

[[nodiscard]] K1BoruvkaSeededExactRoundResolution
resolve_seeded_exact_external_1nn(
    const detail::K1BoruvkaCandidateHostState& host,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> labels,
    const std::vector<std::uint64_t>& tags,
    const SeedBundle& seeds,
    std::size_t component_count,
    std::size_t uniform_node_count,
    std::size_t mixed_node_count) {
  const std::size_t point_count = host.point_count;
  if (component_count <= 1U || seeds.public_seeds.size() != point_count ||
      tags.size() != host.nodes.size() ||
      seeds.status != K1BoruvkaSeedStatus::
          bounded_morton_window_external_exact_monotone_certified ||
      !seeds.morton_audit.complete_source_coverage_certified ||
      !seeds.morton_audit.bounded_window_certified ||
      !seeds.morton_audit.external_targets_recertified ||
      !seeds.morton_audit.exact_monotone_cutoff_certified) {
    throw std::logic_error(
        "the exact external-1NN search requires a certified nonterminal Morton seed bundle");
  }

  K1BoruvkaSeededExactRoundResolution resolution;
  resolution.point_minima.reserve(point_count);
  std::vector<std::optional<ResolvedCandidate>> component_minima(
      point_count);
  K1BoruvkaExactSearchAudit& audit = resolution.search_audit;
  audit.resident_point_count = point_count;
  audit.resident_node_count = host.nodes.size();
  audit.frozen_component_count = component_count;
  audit.uniform_lbvh_node_count = uniform_node_count;
  audit.mixed_lbvh_node_count = mixed_node_count;

  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    const spatial::PointId source_id = checked_point_id(source_index);
    const spatial::PointId source_component = labels[source_index];
    const K1BoruvkaSeed& seed = seeds.public_seeds[source_index];
    const std::size_t seed_target_index = checked_point_index(
        seed.target_point_id, point_count);
    if (seed.source_point_id != source_id ||
        labels[seed_target_index] == source_component ||
        seed.exact_squared_cutoff == exact::ExactLevel{}) {
      throw std::logic_error(
          "a certified Morton incumbent is not an exact external edge");
    }

    ResolvedCandidate best{seed_edge(seed), source_id};
    ++audit.seed_incumbent_count;
    ++audit.point_query_count;
    const std::size_t node_visit_count_before_query =
        audit.cpu_node_visit_count;
    const std::size_t exact_point_distance_count_before_query =
        audit.cpu_exact_point_distance_evaluation_count;
    std::priority_queue<
        ExactSearchQueueEntry,
        std::vector<ExactSearchQueueEntry>,
        ExactSearchNearestFirst>
        frontier;
    exact::ExactLevel root_bound = exact_aabb_lower_bound(
        host.nodes[host.root_index], cloud, source_id);
    ++audit.cpu_exact_aabb_bound_evaluation_count;
    frontier.push(ExactSearchQueueEntry{
        std::move(root_bound), host.root_index});
    std::size_t maximum_frontier_size = frontier.size();

    while (!frontier.empty()) {
      ExactSearchQueueEntry entry = frontier.top();
      frontier.pop();
      ++audit.cpu_node_visit_count;
      if (tags[entry.node_index] == source_component) {
        ++audit.cpu_uniform_component_prune_count;
        continue;
      }
      if (entry.lower_bound > best.edge.squared_length) {
        ++audit.cpu_strict_aabb_prune_count;
        continue;
      }

      const detail::K1BoruvkaNodeInputRecord& node =
          host.nodes[entry.node_index];
      if (node.leaf_point_id != detail::k1_boruvka_sentinel) {
        const spatial::PointId target_id = checked_record_point_id(
            node.leaf_point_id, point_count);
        if (target_id == seed.target_point_id) {
          ++audit.cpu_seed_leaf_distance_reuse_count;
          continue;
        }
        hierarchy::ExactEmstEdge edge = exact_edge(
            cloud, source_id, target_id);
        ++audit.cpu_exact_point_distance_evaluation_count;
        ResolvedCandidate candidate{std::move(edge), source_id};
        if (resolved_less(candidate, best)) {
          best = std::move(candidate);
        }
        continue;
      }

      ++audit.cpu_internal_node_expansion_count;
      for (const std::uint64_t child_word : {
               node.left_child, node.right_child}) {
        if (!std::in_range<std::size_t>(child_word)) {
          throw std::logic_error(
              "an exact external-1NN child index does not fit size_t");
        }
        const std::size_t child = static_cast<std::size_t>(child_word);
        if (child >= host.nodes.size()) {
          throw std::logic_error(
              "an exact external-1NN child is outside the LBVH");
        }
        if (tags[child] == source_component) {
          ++audit.cpu_uniform_component_prune_count;
          continue;
        }
        exact::ExactLevel child_bound = exact_aabb_lower_bound(
            host.nodes[child], cloud, source_id);
        ++audit.cpu_exact_aabb_bound_evaluation_count;
        frontier.push(ExactSearchQueueEntry{
            std::move(child_bound), child});
        maximum_frontier_size = std::max(
            maximum_frontier_size, frontier.size());
      }
    }

    audit.maximum_cpu_node_visit_count_per_source = std::max(
        audit.maximum_cpu_node_visit_count_per_source,
        audit.cpu_node_visit_count - node_visit_count_before_query);
    audit.maximum_cpu_exact_point_distance_evaluation_count_per_source =
        std::max(
            audit
                .maximum_cpu_exact_point_distance_evaluation_count_per_source,
            audit.cpu_exact_point_distance_evaluation_count -
                exact_point_distance_count_before_query);
    audit.maximum_cpu_frontier_size_per_source = std::max(
        audit.maximum_cpu_frontier_size_per_source,
        maximum_frontier_size);

    resolution.point_minima.push_back(
        K1BoruvkaPointMinimum{source_id, best.edge});
    const std::size_t component_index = checked_point_index(
        source_component, point_count);
    std::optional<ResolvedCandidate>& component_minimum =
        component_minima[component_index];
    if (!component_minimum.has_value() ||
        resolved_less(best, *component_minimum)) {
      component_minimum = std::move(best);
    }
  }

  resolution.component_minima.reserve(component_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    if (labels[point_index] != checked_point_id(point_index)) {
      continue;
    }
    if (!component_minima[point_index].has_value()) {
      throw std::logic_error(
          "an exact external-1NN search lost a frozen component");
    }
    const ResolvedCandidate& minimum = *component_minima[point_index];
    resolution.component_minima.push_back(
        hierarchy::K1BoruvkaComponentMinimum{
            checked_point_id(point_index),
            minimum.source_point_id,
            minimum.edge});
  }
  if (resolution.point_minima.size() != point_count ||
      resolution.component_minima.size() != component_count ||
      audit.point_query_count != point_count ||
      audit.seed_incumbent_count != point_count) {
    throw std::logic_error(
        "the exact external-1NN result does not close its source/component counts");
  }

  audit.point_minimum_count = resolution.point_minima.size();
  audit.component_minimum_count = resolution.component_minima.size();
  audit.frozen_labels_certified = true;
  audit.lbvh_topology_and_exact_aabbs_certified =
      host.lbvh_topology_and_exact_aabbs_certified;
  audit.complete_source_seed_coverage_certified = true;
  audit.external_seed_targets_recertified = true;
  audit.exact_seed_cutoffs_recertified = true;
  audit.uniform_component_prunes_certified = true;
  audit.strict_only_aabb_pruning_certified = true;
  audit.complete_frontier_exhaustion_certified = true;
  audit.canonical_kappa_resolution_certified = true;
  audit.point_minima_complete = true;
  audit.component_minima_complete = true;
  resolution.morton_seed_audit = seeds.morton_audit;
  resolution.seed_status = seeds.status;
  resolution.search_status = K1BoruvkaExactSearchStatus::
      exact_external_1nn_branch_and_bound_certified;
  return resolution;
}

[[nodiscard]] K1BoruvkaSeededDualTreeRoundResolution
resolve_seeded_exact_external_1nn_dual_tree(
    const detail::K1BoruvkaCandidateHostState& host,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> labels,
    const std::vector<std::uint64_t>& tags,
    const SeedBundle& seeds,
    std::size_t component_count,
    std::size_t uniform_node_count,
    std::size_t mixed_node_count) {
  const std::size_t point_count = host.point_count;
  if (component_count <= 1U || seeds.public_seeds.size() != point_count ||
      tags.size() != host.nodes.size() ||
      seeds.status != K1BoruvkaSeedStatus::
          bounded_morton_window_external_exact_monotone_certified ||
      !seeds.morton_audit.complete_source_coverage_certified ||
      !seeds.morton_audit.bounded_window_certified ||
      !seeds.morton_audit.external_targets_recertified ||
      !seeds.morton_audit.exact_monotone_cutoff_certified) {
    throw std::logic_error(
        "the exact dual-tree search requires a certified nonterminal Morton seed bundle");
  }

  K1BoruvkaSeededDualTreeRoundResolution resolution;
  K1BoruvkaDualTreeSearchAudit& audit = resolution.search_audit;
  audit.resident_point_count = point_count;
  audit.resident_node_count = host.nodes.size();
  audit.frozen_component_count = component_count;
  audit.uniform_lbvh_node_count = uniform_node_count;
  audit.mixed_lbvh_node_count = mixed_node_count;

  std::vector<ResolvedCandidate> point_best;
  point_best.reserve(point_count);
  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    const spatial::PointId source_id = checked_point_id(source_index);
    const spatial::PointId source_component = labels[source_index];
    const K1BoruvkaSeed& seed = seeds.public_seeds[source_index];
    const std::size_t seed_target_index = checked_point_index(
        seed.target_point_id, point_count);
    if (seed.source_point_id != source_id ||
        labels[seed_target_index] == source_component ||
        seed.exact_squared_cutoff == exact::ExactLevel{}) {
      throw std::logic_error(
          "a certified dual-tree incumbent is not an exact external edge");
    }
    point_best.push_back(ResolvedCandidate{seed_edge(seed), source_id});
    ++audit.seed_incumbent_count;
  }

  const std::size_t invalid_node_index = host.nodes.size();
  std::vector<exact::ExactLevel> node_incumbent_cutoffs(host.nodes.size());
  std::vector<exact::ExactLevel> node_squared_diagonals(host.nodes.size());
  std::vector<std::size_t> node_leaf_counts(host.nodes.size(), 0U);
  std::vector<std::size_t> node_heights(host.nodes.size(), 0U);
  std::vector<std::size_t> node_leaf_begins(
      host.nodes.size(), point_count);
  std::vector<std::size_t> node_leaf_ends(host.nodes.size(), 0U);
  std::vector<std::size_t> parent_by_node(
      host.nodes.size(), invalid_node_index);
  std::vector<std::size_t> leaf_node_by_point(
      point_count, invalid_node_index);
  for (std::size_t node_index = 0U;
       node_index < host.nodes.size();
       ++node_index) {
    const detail::K1BoruvkaNodeInputRecord& node = host.nodes[node_index];
    node_squared_diagonals[node_index] = exact_aabb_squared_diagonal(node);
    if (node.leaf_point_id != detail::k1_boruvka_sentinel) {
      const spatial::PointId point_id = checked_record_point_id(
          node.leaf_point_id, point_count);
      const std::size_t point_index = checked_point_index(
          point_id, point_count);
      const std::size_t morton_position =
          host.morton_position_by_point_id[point_index];
      if (morton_position >= point_count ||
          leaf_node_by_point[point_index] != invalid_node_index) {
        throw std::logic_error(
            "the exact dual-tree leaf permutation is invalid");
      }
      node_incumbent_cutoffs[node_index] =
          seeds.public_seeds[point_index].exact_squared_cutoff;
      node_leaf_counts[node_index] = 1U;
      node_leaf_begins[node_index] = morton_position;
      node_leaf_ends[node_index] = morton_position + 1U;
      leaf_node_by_point[point_index] = node_index;
      continue;
    }
    if (!std::in_range<std::size_t>(node.left_child) ||
        !std::in_range<std::size_t>(node.right_child)) {
      throw std::logic_error(
          "an exact dual-tree child index does not fit size_t");
    }
    const std::size_t left_child =
        static_cast<std::size_t>(node.left_child);
    const std::size_t right_child =
        static_cast<std::size_t>(node.right_child);
    if (left_child >= node_index || right_child >= node_index) {
      throw std::logic_error(
          "the exact dual-tree LBVH is not in certified postorder");
    }
    if (parent_by_node[left_child] != invalid_node_index ||
        parent_by_node[right_child] != invalid_node_index ||
        !((node_leaf_ends[left_child] == node_leaf_begins[right_child]) ||
          (node_leaf_ends[right_child] == node_leaf_begins[left_child]))) {
      throw std::logic_error(
          "the exact dual-tree child ranges do not form a partition");
    }
    parent_by_node[left_child] = node_index;
    parent_by_node[right_child] = node_index;
    node_incumbent_cutoffs[node_index] =
        node_incumbent_cutoffs[left_child] <
                node_incumbent_cutoffs[right_child]
            ? node_incumbent_cutoffs[right_child]
            : node_incumbent_cutoffs[left_child];
    node_leaf_counts[node_index] = checked_size_sum(
        node_leaf_counts[left_child],
        node_leaf_counts[right_child],
        "the exact dual-tree leaf count overflowed size_t");
    node_heights[node_index] = checked_size_sum(
        std::max(node_heights[left_child], node_heights[right_child]),
        1U,
        "the exact dual-tree height overflowed size_t");
    node_leaf_begins[node_index] = std::min(
        node_leaf_begins[left_child], node_leaf_begins[right_child]);
    node_leaf_ends[node_index] = std::max(
        node_leaf_ends[left_child], node_leaf_ends[right_child]);
  }
  if (node_leaf_counts[host.root_index] != point_count ||
      node_leaf_begins[host.root_index] != 0U ||
      node_leaf_ends[host.root_index] != point_count ||
      parent_by_node[host.root_index] != invalid_node_index ||
      std::find(
          leaf_node_by_point.begin(),
          leaf_node_by_point.end(),
          invalid_node_index) != leaf_node_by_point.end()) {
    throw std::logic_error(
        "the exact dual-tree metadata does not cover every point");
  }
  audit.dynamic_incumbent_node_count = node_incumbent_cutoffs.size();

  const auto decrease_point_incumbent = [&](
      std::size_t point_index,
      const exact::ExactLevel& squared_length) {
    std::size_t node_index = leaf_node_by_point[point_index];
    if (!(squared_length < node_incumbent_cutoffs[node_index])) {
      return;
    }
    node_incumbent_cutoffs[node_index] = squared_length;
    audit.cpu_strict_incumbent_decrease_count = checked_size_sum(
        audit.cpu_strict_incumbent_decrease_count,
        1U,
        "the exact dual-tree incumbent decrease count overflowed size_t");
    while (parent_by_node[node_index] != invalid_node_index) {
      const std::size_t parent = parent_by_node[node_index];
      const detail::K1BoruvkaNodeInputRecord& parent_node =
          host.nodes[parent];
      const std::size_t left_child =
          static_cast<std::size_t>(parent_node.left_child);
      const std::size_t right_child =
          static_cast<std::size_t>(parent_node.right_child);
      const exact::ExactLevel& refreshed =
          node_incumbent_cutoffs[left_child] <
                  node_incumbent_cutoffs[right_child]
              ? node_incumbent_cutoffs[right_child]
              : node_incumbent_cutoffs[left_child];
      if (node_incumbent_cutoffs[parent] == refreshed) {
        break;
      }
      node_incumbent_cutoffs[parent] = refreshed;
      audit.cpu_incumbent_ancestor_update_count = checked_size_sum(
          audit.cpu_incumbent_ancestor_update_count,
          1U,
          "the exact dual-tree incumbent ancestor update count overflowed size_t");
      node_index = parent;
    }
  };

  const DualTreeTraversalMetadataView traversal_metadata{
      node_squared_diagonals,
      node_leaf_counts,
      node_leaf_begins,
      node_leaf_ends,
      node_heights[host.root_index]};
  traverse_exact_dual_tree_frontier(
      host,
      cloud,
      labels,
      tags,
      traversal_metadata,
      node_incumbent_cutoffs,
      audit,
      [&](hierarchy::ExactEmstEdge edge,
          spatial::PointId left_id,
          spatial::PointId right_id) {
        const std::size_t left_point_index = checked_point_index(
            left_id, point_count);
        const std::size_t right_point_index = checked_point_index(
            right_id, point_count);
        const exact::ExactLevel candidate_squared_length =
            edge.squared_length;
        ResolvedCandidate left_candidate{edge, left_id};
        if (resolved_less(
                left_candidate, point_best[left_point_index])) {
          point_best[left_point_index] = std::move(left_candidate);
          decrease_point_incumbent(
              left_point_index, candidate_squared_length);
        }
        ResolvedCandidate right_candidate{std::move(edge), right_id};
        if (resolved_less(
                right_candidate, point_best[right_point_index])) {
          point_best[right_point_index] = std::move(right_candidate);
          decrease_point_incumbent(
              right_point_index, candidate_squared_length);
        }
      });
  std::vector<exact::ExactLevel> verified_incumbent_cutoffs(
      host.nodes.size());
  for (std::size_t node_index = 0U;
       node_index < host.nodes.size();
       ++node_index) {
    const detail::K1BoruvkaNodeInputRecord& node = host.nodes[node_index];
    if (node.leaf_point_id != detail::k1_boruvka_sentinel) {
      const spatial::PointId point_id = checked_record_point_id(
          node.leaf_point_id, point_count);
      verified_incumbent_cutoffs[node_index] =
          point_best[checked_point_index(point_id, point_count)]
              .edge.squared_length;
    } else {
      const std::size_t left_child =
          static_cast<std::size_t>(node.left_child);
      const std::size_t right_child =
          static_cast<std::size_t>(node.right_child);
      verified_incumbent_cutoffs[node_index] =
          verified_incumbent_cutoffs[left_child] <
                  verified_incumbent_cutoffs[right_child]
              ? verified_incumbent_cutoffs[right_child]
              : verified_incumbent_cutoffs[left_child];
    }
    if (verified_incumbent_cutoffs[node_index] !=
        node_incumbent_cutoffs[node_index]) {
      throw std::logic_error(
          "the exact dual-tree dynamic incumbent tree is stale");
    }
  }

  resolution.point_minima.reserve(point_count);
  std::vector<std::optional<ResolvedCandidate>> component_minima(
      point_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    const spatial::PointId point_id = checked_point_id(point_index);
    const ResolvedCandidate& minimum = point_best[point_index];
    resolution.point_minima.push_back(
        K1BoruvkaPointMinimum{point_id, minimum.edge});
    const std::size_t component_index = checked_point_index(
        labels[point_index], point_count);
    std::optional<ResolvedCandidate>& component_minimum =
        component_minima[component_index];
    if (!component_minimum.has_value() ||
        resolved_less(minimum, *component_minimum)) {
      component_minimum = minimum;
    }
  }

  resolution.component_minima.reserve(component_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    if (labels[point_index] != checked_point_id(point_index)) {
      continue;
    }
    if (!component_minima[point_index].has_value()) {
      throw std::logic_error(
          "the exact dual-tree search lost a frozen component");
    }
    const ResolvedCandidate& minimum = *component_minima[point_index];
    resolution.component_minima.push_back(
        hierarchy::K1BoruvkaComponentMinimum{
            checked_point_id(point_index),
            minimum.source_point_id,
            minimum.edge});
  }
  if (resolution.point_minima.size() != point_count ||
      resolution.component_minima.size() != component_count ||
      audit.seed_incumbent_count != point_count ||
      audit.dynamic_incumbent_node_count != host.nodes.size()) {
    throw std::logic_error(
        "the exact dual-tree result does not close its certified counts");
  }

  audit.point_minimum_count = resolution.point_minima.size();
  audit.component_minimum_count = resolution.component_minima.size();
  audit.frozen_labels_certified = true;
  audit.lbvh_topology_and_exact_aabbs_certified =
      host.lbvh_topology_and_exact_aabbs_certified;
  audit.complete_source_seed_coverage_certified = true;
  audit.external_seed_targets_recertified = true;
  audit.exact_seed_cutoffs_recertified = true;
  audit.dynamic_incumbent_tree_certified = true;
  audit.canonical_unordered_pair_partition_certified = true;
  audit.uniform_component_pair_prunes_certified = true;
  audit.strict_only_aabb_pair_pruning_certified = true;
  audit.depth_first_frontier_bound_certified = true;
  audit.complete_frontier_exhaustion_certified = true;
  audit.canonical_kappa_resolution_certified = true;
  audit.point_minima_complete = true;
  audit.component_minima_complete = true;
  resolution.morton_seed_audit = seeds.morton_audit;
  resolution.seed_status = seeds.status;
  resolution.search_status = K1BoruvkaDualTreeSearchStatus::
      exact_external_1nn_shared_lbvh_dual_tree_certified;
  return resolution;
}

[[nodiscard]] K1BoruvkaSeededComponentDualTreeRoundResolution
resolve_seeded_exact_component_minima_dual_tree(
    const detail::K1BoruvkaCandidateHostState& host,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> labels,
    const std::vector<std::uint64_t>& tags,
    const SeedBundle& seeds,
    std::size_t component_count,
    std::size_t uniform_node_count,
    std::size_t mixed_node_count,
    K1BoruvkaComponentEnvelopeMode envelope_mode) {
  const std::size_t point_count = host.point_count;
  if (component_count <= 1U || seeds.public_seeds.size() != point_count ||
      tags.size() != host.nodes.size() ||
      seeds.status != K1BoruvkaSeedStatus::
          bounded_morton_window_external_exact_monotone_certified ||
      !seeds.morton_audit.complete_source_coverage_certified ||
      !seeds.morton_audit.bounded_window_certified ||
      !seeds.morton_audit.external_targets_recertified ||
      !seeds.morton_audit.exact_monotone_cutoff_certified) {
    throw std::logic_error(
        "the component-direct dual-tree search requires a certified nonterminal Morton seed bundle");
  }

  K1BoruvkaSeededComponentDualTreeRoundResolution resolution;
  K1BoruvkaComponentDualTreeSearchAudit& audit =
      resolution.search_audit;
  audit.component_envelope_mode = envelope_mode;
  audit.resident_point_count = point_count;
  audit.resident_node_count = host.nodes.size();
  audit.frozen_component_count = component_count;
  audit.uniform_lbvh_node_count = uniform_node_count;
  audit.mixed_lbvh_node_count = mixed_node_count;

  const std::size_t invalid_component_slot = component_count;
  std::vector<std::size_t> component_slot_by_label(
      point_count, invalid_component_slot);
  std::vector<spatial::PointId> component_labels;
  component_labels.reserve(component_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    const spatial::PointId point_id = checked_point_id(point_index);
    if (labels[point_index] != point_id) {
      continue;
    }
    component_slot_by_label[point_index] = component_labels.size();
    component_labels.push_back(point_id);
  }
  if (component_labels.size() != component_count) {
    throw std::logic_error(
        "the component-direct dual-tree labels lost a representative");
  }

  const auto component_slot = [&](spatial::PointId component_id) {
    const std::size_t component_index = checked_point_index(
        component_id, point_count);
    const std::size_t slot = component_slot_by_label[component_index];
    if (slot >= component_count || component_labels[slot] != component_id) {
      throw std::logic_error(
          "the component-direct dual-tree component slot is invalid");
    }
    return slot;
  };

  std::vector<std::optional<ResolvedCandidate>> component_best(
      component_count);
  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    const spatial::PointId source_id = checked_point_id(source_index);
    const spatial::PointId source_component = labels[source_index];
    const K1BoruvkaSeed& seed = seeds.public_seeds[source_index];
    const std::size_t seed_target_index = checked_point_index(
        seed.target_point_id, point_count);
    if (seed.source_point_id != source_id ||
        labels[seed_target_index] == source_component ||
        seed.exact_squared_cutoff == exact::ExactLevel{}) {
      throw std::logic_error(
          "a component-direct seed is not an exact external edge");
    }
    const std::size_t slot = component_slot(source_component);
    ResolvedCandidate candidate{seed_edge(seed), source_id};
    if (!component_best[slot].has_value() ||
        resolved_less(candidate, *component_best[slot])) {
      component_best[slot] = std::move(candidate);
    }
    audit.point_seed_count = checked_size_sum(
        audit.point_seed_count,
        1U,
        "the component-direct point-seed count overflowed size_t");
  }

  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    const K1BoruvkaSeed& seed = seeds.public_seeds[source_index];
    const std::size_t target_index = checked_point_index(
        seed.target_point_id, point_count);
    const std::size_t target_slot = component_slot(labels[target_index]);
    if (!component_best[target_slot].has_value()) {
      throw std::logic_error(
          "the component-direct target component has no source seed incumbent");
    }
    ResolvedCandidate target_candidate{
        seed_edge(seed), seed.target_point_id};
    audit.target_component_seed_offer_count = checked_size_sum(
        audit.target_component_seed_offer_count,
        1U,
        "the component-direct target-component seed-offer count overflowed size_t");
    if (!resolved_less(
            target_candidate, *component_best[target_slot])) {
      continue;
    }
    if (target_candidate.edge.squared_length <
        component_best[target_slot]->edge.squared_length) {
      audit.target_component_seed_strict_cutoff_decrease_count =
          checked_size_sum(
              audit.target_component_seed_strict_cutoff_decrease_count,
              1U,
              "the component-direct target-component strict-decrease count overflowed size_t");
    }
    component_best[target_slot] = std::move(target_candidate);
    audit.target_component_seed_kappa_update_count = checked_size_sum(
        audit.target_component_seed_kappa_update_count,
        1U,
        "the component-direct target-component kappa-update count overflowed size_t");
  }

  std::vector<exact::ExactLevel> initial_component_cutoffs;
  initial_component_cutoffs.reserve(component_count);
  for (const std::optional<ResolvedCandidate>& minimum : component_best) {
    if (!minimum.has_value()) {
      throw std::logic_error(
          "a component-direct seed reduction lost a component");
    }
    initial_component_cutoffs.push_back(minimum->edge.squared_length);
    audit.component_seed_incumbent_count = checked_size_sum(
        audit.component_seed_incumbent_count,
        1U,
        "the component-direct seed-incumbent count overflowed size_t");
  }

  const std::size_t invalid_node_index = host.nodes.size();
  std::vector<exact::ExactLevel> node_cutoff_upper_bounds(
      host.nodes.size());
  std::vector<exact::ExactLevel> node_squared_diagonals(host.nodes.size());
  std::vector<std::size_t> node_leaf_counts(host.nodes.size(), 0U);
  std::vector<std::size_t> node_heights(host.nodes.size(), 0U);
  std::vector<std::size_t> node_leaf_begins(
      host.nodes.size(), point_count);
  std::vector<std::size_t> node_leaf_ends(host.nodes.size(), 0U);
  std::vector<std::size_t> parent_by_node(
      host.nodes.size(), invalid_node_index);
  std::vector<std::size_t> leaf_node_by_point(
      point_count, invalid_node_index);
  for (std::size_t node_index = 0U;
       node_index < host.nodes.size();
       ++node_index) {
    const detail::K1BoruvkaNodeInputRecord& node = host.nodes[node_index];
    node_squared_diagonals[node_index] = exact_aabb_squared_diagonal(node);
    if (node.leaf_point_id != detail::k1_boruvka_sentinel) {
      const spatial::PointId point_id = checked_record_point_id(
          node.leaf_point_id, point_count);
      const std::size_t point_index = checked_point_index(
          point_id, point_count);
      const std::size_t morton_position =
          host.morton_position_by_point_id[point_index];
      if (morton_position >= point_count ||
          leaf_node_by_point[point_index] != invalid_node_index) {
        throw std::logic_error(
            "the component-direct dual-tree leaf permutation is invalid");
      }
      const std::size_t slot = component_slot(labels[point_index]);
      node_cutoff_upper_bounds[node_index] =
          initial_component_cutoffs[slot];
      node_leaf_counts[node_index] = 1U;
      node_leaf_begins[node_index] = morton_position;
      node_leaf_ends[node_index] = morton_position + 1U;
      leaf_node_by_point[point_index] = node_index;
      continue;
    }
    if (!std::in_range<std::size_t>(node.left_child) ||
        !std::in_range<std::size_t>(node.right_child)) {
      throw std::logic_error(
          "a component-direct dual-tree child index does not fit size_t");
    }
    const std::size_t left_child =
        static_cast<std::size_t>(node.left_child);
    const std::size_t right_child =
        static_cast<std::size_t>(node.right_child);
    if (left_child >= node_index || right_child >= node_index) {
      throw std::logic_error(
          "the component-direct dual-tree LBVH is not in certified postorder");
    }
    if (parent_by_node[left_child] != invalid_node_index ||
        parent_by_node[right_child] != invalid_node_index ||
        !((node_leaf_ends[left_child] == node_leaf_begins[right_child]) ||
          (node_leaf_ends[right_child] == node_leaf_begins[left_child]))) {
      throw std::logic_error(
          "the component-direct dual-tree child ranges do not partition the leaves");
    }
    parent_by_node[left_child] = node_index;
    parent_by_node[right_child] = node_index;
    node_cutoff_upper_bounds[node_index] =
        node_cutoff_upper_bounds[left_child] <
                node_cutoff_upper_bounds[right_child]
            ? node_cutoff_upper_bounds[right_child]
            : node_cutoff_upper_bounds[left_child];
    node_leaf_counts[node_index] = checked_size_sum(
        node_leaf_counts[left_child],
        node_leaf_counts[right_child],
        "the component-direct dual-tree leaf count overflowed size_t");
    node_heights[node_index] = checked_size_sum(
        std::max(node_heights[left_child], node_heights[right_child]),
        1U,
        "the component-direct dual-tree height overflowed size_t");
    node_leaf_begins[node_index] = std::min(
        node_leaf_begins[left_child], node_leaf_begins[right_child]);
    node_leaf_ends[node_index] = std::max(
        node_leaf_ends[left_child], node_leaf_ends[right_child]);
  }
  if (node_leaf_counts[host.root_index] != point_count ||
      node_leaf_begins[host.root_index] != 0U ||
      node_leaf_ends[host.root_index] != point_count ||
      parent_by_node[host.root_index] != invalid_node_index ||
      std::find(
          leaf_node_by_point.begin(),
          leaf_node_by_point.end(),
          invalid_node_index) != leaf_node_by_point.end()) {
    throw std::logic_error(
        "the component-direct dual-tree metadata does not cover every point");
  }
  audit.component_cutoff_upper_envelope_node_count =
      node_cutoff_upper_bounds.size();

  const auto decrease_component_witness_path = [&](
      std::size_t point_index,
      const exact::ExactLevel& squared_length) {
    if (envelope_mode == K1BoruvkaComponentEnvelopeMode::frozen_initial) {
      return;
    }
    std::size_t node_index = leaf_node_by_point[point_index];
    if (!(squared_length < node_cutoff_upper_bounds[node_index])) {
      throw std::logic_error(
          "a sparse component witness leaf did not decrease strictly");
    }
    node_cutoff_upper_bounds[node_index] = squared_length;
    audit.cpu_component_witness_leaf_update_count = checked_size_sum(
        audit.cpu_component_witness_leaf_update_count,
        1U,
        "the component witness-leaf update count overflowed size_t");
    while (parent_by_node[node_index] != invalid_node_index) {
      const std::size_t parent = parent_by_node[node_index];
      const detail::K1BoruvkaNodeInputRecord& parent_node =
          host.nodes[parent];
      const std::size_t left_child =
          static_cast<std::size_t>(parent_node.left_child);
      const std::size_t right_child =
          static_cast<std::size_t>(parent_node.right_child);
      const exact::ExactLevel& refreshed =
          node_cutoff_upper_bounds[left_child] <
                  node_cutoff_upper_bounds[right_child]
              ? node_cutoff_upper_bounds[right_child]
              : node_cutoff_upper_bounds[left_child];
      if (node_cutoff_upper_bounds[parent] < refreshed) {
        throw std::logic_error(
            "a sparse component witness path increased its upper envelope");
      }
      if (node_cutoff_upper_bounds[parent] == refreshed) {
        break;
      }
      node_cutoff_upper_bounds[parent] = refreshed;
      audit.cpu_component_witness_ancestor_update_count = checked_size_sum(
          audit.cpu_component_witness_ancestor_update_count,
          1U,
          "the component witness ancestor-update count overflowed size_t");
      node_index = parent;
    }
  };

  const DualTreeTraversalMetadataView traversal_metadata{
      node_squared_diagonals,
      node_leaf_counts,
      node_leaf_begins,
      node_leaf_ends,
      node_heights[host.root_index]};
  traverse_exact_dual_tree_frontier(
      host,
      cloud,
      labels,
      tags,
      traversal_metadata,
      node_cutoff_upper_bounds,
      audit,
      [&](hierarchy::ExactEmstEdge edge,
          spatial::PointId left_id,
          spatial::PointId right_id) {
        const std::size_t left_point_index = checked_point_index(
            left_id, point_count);
        const std::size_t right_point_index = checked_point_index(
            right_id, point_count);
        const std::size_t left_slot = component_slot(
            labels[left_point_index]);
        const std::size_t right_slot = component_slot(
            labels[right_point_index]);
        if (left_slot == right_slot ||
            !component_best[left_slot].has_value() ||
            !component_best[right_slot].has_value()) {
          throw std::logic_error(
              "a component-direct external pair has invalid component slots");
        }
        const exact::ExactLevel candidate_squared_length =
            edge.squared_length;
        ResolvedCandidate left_candidate{edge, left_id};
        if (resolved_less(left_candidate, *component_best[left_slot])) {
          if (candidate_squared_length <
              component_best[left_slot]->edge.squared_length) {
            audit.cpu_strict_component_cutoff_decrease_count =
                checked_size_sum(
                    audit.cpu_strict_component_cutoff_decrease_count,
                    1U,
                    "the component-direct strict-decrease count overflowed size_t");
            decrease_component_witness_path(
                left_point_index, candidate_squared_length);
          }
          component_best[left_slot] = std::move(left_candidate);
          audit.cpu_component_kappa_update_count = checked_size_sum(
              audit.cpu_component_kappa_update_count,
              1U,
              "the component-direct kappa-update count overflowed size_t");
        }
        ResolvedCandidate right_candidate{std::move(edge), right_id};
        if (resolved_less(right_candidate, *component_best[right_slot])) {
          if (candidate_squared_length <
              component_best[right_slot]->edge.squared_length) {
            audit.cpu_strict_component_cutoff_decrease_count =
                checked_size_sum(
                    audit.cpu_strict_component_cutoff_decrease_count,
                    1U,
                    "the component-direct strict-decrease count overflowed size_t");
            decrease_component_witness_path(
                right_point_index, candidate_squared_length);
          }
          component_best[right_slot] = std::move(right_candidate);
          audit.cpu_component_kappa_update_count = checked_size_sum(
              audit.cpu_component_kappa_update_count,
              1U,
              "the component-direct kappa-update count overflowed size_t");
        }
      });

  std::vector<exact::ExactLevel> verified_live_upper_envelope(
      host.nodes.size());
  std::vector<exact::ExactLevel> verified_frozen_upper_envelope(
      host.nodes.size());
  for (std::size_t node_index = 0U;
       node_index < host.nodes.size();
       ++node_index) {
    const detail::K1BoruvkaNodeInputRecord& node = host.nodes[node_index];
    if (node.leaf_point_id != detail::k1_boruvka_sentinel) {
      const spatial::PointId point_id = checked_record_point_id(
          node.leaf_point_id, point_count);
      const std::size_t point_index = checked_point_index(
          point_id, point_count);
      const std::size_t slot = component_slot(labels[point_index]);
      const exact::ExactLevel& live_component_cutoff =
          component_best[slot]->edge.squared_length;
      const exact::ExactLevel& live_leaf_upper_bound =
          node_cutoff_upper_bounds[node_index];
      const exact::ExactLevel& frozen_leaf_upper_bound =
          initial_component_cutoffs[slot];
      if (live_leaf_upper_bound < live_component_cutoff ||
          frozen_leaf_upper_bound < live_leaf_upper_bound) {
        throw std::logic_error(
            "a component witness leaf is not a live cutoff upper bound");
      }
      verified_live_upper_envelope[node_index] = live_leaf_upper_bound;
      verified_frozen_upper_envelope[node_index] =
          frozen_leaf_upper_bound;
    } else {
      const std::size_t left_child =
          static_cast<std::size_t>(node.left_child);
      const std::size_t right_child =
          static_cast<std::size_t>(node.right_child);
      verified_live_upper_envelope[node_index] =
          verified_live_upper_envelope[left_child] <
                  verified_live_upper_envelope[right_child]
              ? verified_live_upper_envelope[right_child]
              : verified_live_upper_envelope[left_child];
      verified_frozen_upper_envelope[node_index] =
          verified_frozen_upper_envelope[left_child] <
                  verified_frozen_upper_envelope[right_child]
              ? verified_frozen_upper_envelope[right_child]
              : verified_frozen_upper_envelope[left_child];
    }
    if (verified_live_upper_envelope[node_index] !=
            node_cutoff_upper_bounds[node_index] ||
        verified_frozen_upper_envelope[node_index] <
            verified_live_upper_envelope[node_index]) {
      throw std::logic_error(
          "the component-direct live cutoff envelope lost its certified bounds");
    }
  }

  resolution.component_minima.reserve(component_count);
  for (std::size_t slot = 0U; slot < component_count; ++slot) {
    if (!component_best[slot].has_value()) {
      throw std::logic_error(
          "the component-direct traversal lost a component minimum");
    }
    const ResolvedCandidate& minimum = *component_best[slot];
    const std::size_t source_index = checked_point_index(
        minimum.source_point_id, point_count);
    if (labels[source_index] != component_labels[slot]) {
      throw std::logic_error(
          "a component-direct minimum has the wrong oriented source");
    }
    resolution.component_minima.push_back(
        hierarchy::K1BoruvkaComponentMinimum{
            component_labels[slot],
            minimum.source_point_id,
            minimum.edge});
  }

  std::size_t maximum_component_updates =
      audit.cpu_exact_point_pair_distance_evaluation_count;
  maximum_component_updates = checked_size_sum(
      maximum_component_updates,
      audit.cpu_exact_point_pair_distance_evaluation_count,
      "the component-direct update bound overflowed size_t");
  const std::size_t maximum_witness_ancestor_updates = checked_size_product(
      audit.cpu_component_witness_leaf_update_count,
      audit.lbvh_maximum_depth,
      "the component witness ancestor-update bound overflowed size_t");
  const bool envelope_update_counts_closed =
      (envelope_mode == K1BoruvkaComponentEnvelopeMode::frozen_initial &&
       audit.cpu_component_witness_leaf_update_count == 0U &&
       audit.cpu_component_witness_ancestor_update_count == 0U) ||
      (envelope_mode ==
           K1BoruvkaComponentEnvelopeMode::sparse_witness_path_monotone &&
       audit.cpu_component_witness_leaf_update_count ==
           audit.cpu_strict_component_cutoff_decrease_count &&
       audit.cpu_component_witness_ancestor_update_count <=
           maximum_witness_ancestor_updates);
  if (audit.point_seed_count != point_count ||
      audit.component_seed_incumbent_count != component_count ||
      audit.target_component_seed_offer_count != point_count ||
      audit.target_component_seed_kappa_update_count > point_count ||
      audit.target_component_seed_strict_cutoff_decrease_count >
          audit.target_component_seed_kappa_update_count ||
      audit.component_cutoff_upper_envelope_node_count !=
          host.nodes.size() ||
      resolution.component_minima.size() != component_count ||
      audit.cpu_component_kappa_update_count > maximum_component_updates ||
      audit.cpu_strict_component_cutoff_decrease_count >
          audit.cpu_component_kappa_update_count ||
      !envelope_update_counts_closed) {
    throw std::logic_error(
        "the component-direct result does not close its certified counts");
  }

  audit.component_minimum_count = resolution.component_minima.size();
  audit.frozen_labels_certified = true;
  audit.lbvh_topology_and_exact_aabbs_certified =
      host.lbvh_topology_and_exact_aabbs_certified;
  audit.complete_source_seed_coverage_certified = true;
  audit.external_seed_targets_recertified = true;
  audit.exact_seed_cutoffs_recertified = true;
  audit.component_seed_reduction_certified = true;
  audit.bidirectional_component_seed_reduction_certified = true;
  audit.component_cutoff_upper_envelope_certified = true;
  audit.live_component_cutoff_upper_bound_certified = true;
  audit.pointwise_at_most_frozen_envelope_certified = true;
  audit.canonical_kappa_resolution_certified = true;
  audit.component_minima_complete = true;
  resolution.morton_seed_audit = seeds.morton_audit;
  resolution.seed_status = seeds.status;
  resolution.search_status = K1BoruvkaComponentDualTreeSearchStatus::
      exact_component_minima_shared_lbvh_dual_tree_certified;
  return resolution;
}

[[nodiscard]] K1BoruvkaRoundProposal validate_and_resolve(
    detail::K1BoruvkaCandidateHostState& host,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> labels,
    const std::vector<std::uint64_t>& tags,
    SeedBundle seeds,
    const detail::K1BoruvkaCandidateBatch& batch,
    std::size_t component_count,
    std::size_t uniform_node_count,
    std::size_t mixed_node_count) {
  const std::size_t point_count = host.point_count;
  if (batch.candidate_offsets.size() != point_count + 1U ||
      batch.candidate_offsets.front() != 0U ||
      batch.output_capacity != batch.records.size() ||
      batch.kernel_launch_count != 2U ||
      batch.synchronization_count != 2U ||
      batch.count_pass_node_visit_count !=
          batch.emit_pass_node_visit_count ||
      batch.buffer_epoch == 0U ||
      batch.buffer_epoch <= host.last_buffer_epoch ||
      !batch.exact_capacity || !batch.no_truncation) {
    throw std::runtime_error(
        "the GPU K1 Boruvka candidate batch has invalid metadata");
  }
  if (!std::in_range<std::uint64_t>(batch.records.size()) ||
      batch.candidate_offsets.back() !=
          static_cast<std::uint64_t>(batch.records.size())) {
    throw std::runtime_error(
        "the GPU K1 Boruvka candidate offsets do not close");
  }

  K1BoruvkaRoundProposal proposal;
  proposal.frozen_component_labels.assign(labels.begin(), labels.end());
  proposal.seeds = std::move(seeds.public_seeds);
  proposal.candidate_offsets.reserve(point_count + 1U);
  for (std::size_t point_index = 0U;
       point_index < point_count + 1U;
       ++point_index) {
    const std::uint64_t offset_word = batch.candidate_offsets[point_index];
    if (!std::in_range<std::size_t>(offset_word)) {
      throw std::runtime_error(
          "a GPU K1 Boruvka candidate offset does not fit size_t");
    }
    const std::size_t offset = static_cast<std::size_t>(offset_word);
    if (offset > batch.records.size() ||
        (!proposal.candidate_offsets.empty() &&
         offset < proposal.candidate_offsets.back())) {
      throw std::runtime_error(
          "the GPU K1 Boruvka candidate offsets are not monotone");
    }
    proposal.candidate_offsets.push_back(offset);
  }

  proposal.candidates.reserve(batch.records.size());
  std::vector<unsigned char> target_seen(point_count, 0U);
  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    const spatial::PointId source_id = checked_point_id(source_index);
    const std::size_t begin = proposal.candidate_offsets[source_index];
    const std::size_t end = proposal.candidate_offsets[source_index + 1U];
    if (begin == end) {
      throw std::runtime_error(
          "a nonterminal GPU K1 Boruvka source has no candidate");
    }
    for (std::size_t position = begin; position < end; ++position) {
      const detail::K1BoruvkaCandidateRecord& record =
          batch.records[position];
      if (record.source_point_id != source_id) {
        throw std::runtime_error(
            "a GPU K1 Boruvka candidate is in the wrong source segment");
      }
      const std::size_t target_index = checked_point_index(
          static_cast<spatial::PointId>(record.target_point_id), point_count);
      if (labels[target_index] == labels[source_index] ||
          target_seen[target_index] != 0U) {
        throw std::runtime_error(
            "a GPU K1 Boruvka candidate is internal or duplicated");
      }
      target_seen[target_index] = 1U;
      proposal.candidates.push_back(K1BoruvkaCandidate{
          source_id, static_cast<spatial::PointId>(record.target_point_id)});
    }
    for (std::size_t position = begin; position < end; ++position) {
      const std::size_t target_index = checked_point_index(
          proposal.candidates[position].target_point_id, point_count);
      target_seen[target_index] = 0U;
    }
  }

  K1BoruvkaCandidateAudit& audit = proposal.audit;
  audit.resident_point_count = point_count;
  audit.resident_node_count = host.nodes.size();
  audit.frozen_component_count = component_count;
  audit.uniform_lbvh_node_count = uniform_node_count;
  audit.mixed_lbvh_node_count = mixed_node_count;
  audit.exact_seed_count = proposal.seeds.size();
  audit.gpu_candidate_count = proposal.candidates.size();
  audit.gpu_output_capacity = batch.output_capacity;
  audit.gpu_kernel_launch_count = batch.kernel_launch_count;
  audit.gpu_synchronization_count = batch.synchronization_count;
  audit.gpu_count_pass_node_visit_count =
      batch.count_pass_node_visit_count;
  audit.gpu_emit_pass_node_visit_count = batch.emit_pass_node_visit_count;
  audit.gpu_uniform_component_prune_count =
      batch.uniform_component_prune_count;
  audit.gpu_strict_aabb_prune_count = batch.strict_aabb_prune_count;
  audit.gpu_invalid_bound_descent_count =
      batch.invalid_bound_descent_count;
  audit.buffer_epoch = batch.buffer_epoch;
  audit.frozen_labels_certified = true;
  audit.rope_topology_certified = host.rope_topology_certified;
  audit.exact_capacity_certified = true;
  audit.no_truncation_certified = true;

  // Rebuild exactly the fixed-seed set A_q. At a leaf, the exact AABB bound is
  // the point distance, so this replay requires every outgoing target with
  // d^2(q,p) <= R_q and therefore every exact outgoing minimizer.
  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    const std::size_t begin = proposal.candidate_offsets[source_index];
    const std::size_t end = proposal.candidate_offsets[source_index + 1U];
    for (std::size_t position = begin; position < end; ++position) {
      target_seen[checked_point_index(
          proposal.candidates[position].target_point_id, point_count)] = 1U;
    }

    const spatial::PointId source_id = checked_point_id(source_index);
    std::uint64_t node_word = static_cast<std::uint64_t>(host.root_index);
    std::size_t step_count = 0U;
    while (node_word != detail::k1_boruvka_sentinel) {
      if (!std::in_range<std::size_t>(node_word) ||
          static_cast<std::size_t>(node_word) >= host.nodes.size() ||
          step_count >= host.nodes.size()) {
        throw std::runtime_error(
            "the Phase 5 K1 Boruvka rope traversal is cyclic or invalid");
      }
      ++step_count;
      const std::size_t node_index = static_cast<std::size_t>(node_word);
      const detail::K1BoruvkaNodeInputRecord& node = host.nodes[node_index];
      if (tags[node_index] == labels[source_index]) {
        node_word = node.escape;
        continue;
      }
      const exact::ExactLevel lower_bound =
          exact_aabb_lower_bound(node, cloud, source_id);
      ++audit.cpu_exact_aabb_bound_evaluation_count;
      if (lower_bound > proposal.seeds[source_index].exact_squared_cutoff) {
        node_word = node.escape;
        continue;
      }
      if (node.leaf_point_id != detail::k1_boruvka_sentinel) {
        const std::size_t target_index = checked_point_index(
            static_cast<spatial::PointId>(node.leaf_point_id), point_count);
        if (target_seen[target_index] == 0U) {
          throw std::runtime_error(
              "the GPU K1 Boruvka proposal omitted a required candidate");
        }
        ++audit.cpu_required_candidate_count;
        node_word = node.escape;
      } else {
        node_word = node.left_child;
      }
    }

    for (std::size_t position = begin; position < end; ++position) {
      target_seen[checked_point_index(
          proposal.candidates[position].target_point_id, point_count)] = 0U;
    }
  }
  audit.candidate_superset_certified = true;

  std::vector<std::optional<ResolvedCandidate>> component_minima(point_count);
  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    const spatial::PointId source_id = checked_point_id(source_index);
    std::optional<ResolvedCandidate> point_minimum;
    const std::size_t begin = proposal.candidate_offsets[source_index];
    const std::size_t end = proposal.candidate_offsets[source_index + 1U];
    for (std::size_t position = begin; position < end; ++position) {
      hierarchy::ExactEmstEdge edge = exact_edge(
          cloud,
          source_id,
          proposal.candidates[position].target_point_id);
      ++audit.cpu_exact_candidate_distance_evaluation_count;
      if (edge.squared_length >
          proposal.seeds[source_index].exact_squared_cutoff) {
        continue;
      }
      ResolvedCandidate candidate{std::move(edge), source_id};
      if (!point_minimum.has_value() ||
          resolved_less(candidate, *point_minimum)) {
        point_minimum = std::move(candidate);
      }
    }
    if (!point_minimum.has_value()) {
      throw std::runtime_error(
          "the certified GPU K1 Boruvka superset has no seed-bounded candidate");
    }
    const std::size_t component_index = checked_point_index(
        labels[source_index], point_count);
    std::optional<ResolvedCandidate>& component_minimum =
        component_minima[component_index];
    if (!component_minimum.has_value() ||
        resolved_less(*point_minimum, *component_minimum)) {
      component_minimum = std::move(point_minimum);
    }
  }

  proposal.cpu_exact_component_minima.reserve(component_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    if (labels[point_index] != checked_point_id(point_index)) {
      continue;
    }
    if (!component_minima[point_index].has_value()) {
      throw std::runtime_error(
          "a certified GPU K1 Boruvka component has no exact minimum");
    }
    const ResolvedCandidate& minimum = *component_minima[point_index];
    proposal.cpu_exact_component_minima.push_back(
        hierarchy::K1BoruvkaComponentMinimum{
            checked_point_id(point_index),
            minimum.source_point_id,
            minimum.edge});
  }
  if (proposal.cpu_exact_component_minima.size() != component_count) {
    throw std::runtime_error(
        "the CPU K1 Boruvka resolution lost a frozen component");
  }
  audit.cpu_exact_resolution_complete = true;
  audit.proposal_digest_fnv1a = proposal_digest(
      proposal.candidate_offsets, proposal.candidates);
  host.last_buffer_epoch = batch.buffer_epoch;
  return proposal;
}

class ChunkedRoundAccumulator final {
 public:
  ChunkedRoundAccumulator(
      detail::K1BoruvkaCandidateHostState& host,
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> labels,
      const std::vector<std::uint64_t>& tags,
      const SeedBundle& seeds,
      std::size_t component_count,
      std::size_t uniform_node_count,
      std::size_t mixed_node_count,
      K1BoruvkaChunkingPolicy policy)
      : host_(host),
        cloud_(cloud),
        labels_(labels),
        tags_(tags),
        seeds_(seeds),
        component_count_(component_count),
        policy_(policy),
        component_minima_(host.point_count),
        target_seen_(host.point_count, 0U) {
    K1BoruvkaCandidateAudit& audit = proposal_audit_;
    audit.resident_point_count = host.point_count;
    audit.resident_node_count = host.nodes.size();
    audit.frozen_component_count = component_count;
    audit.uniform_lbvh_node_count = uniform_node_count;
    audit.mixed_lbvh_node_count = mixed_node_count;
    audit.exact_seed_count = seeds.public_seeds.size();
    audit.frozen_labels_certified = true;
    audit.rope_topology_certified = host.rope_topology_certified;
  }

  void consume(const detail::K1BoruvkaCandidateChunkView& chunk) {
    const std::size_t point_count = host_.point_count;
    if (chunk.source_begin != next_source_begin_ ||
        chunk.source_end <= chunk.source_begin ||
        chunk.source_end > point_count) {
      throw std::runtime_error(
          "a chunked GPU K1 Boruvka source range is incomplete or unordered");
    }
    const std::size_t source_count =
        chunk.source_end - chunk.source_begin;
    if (chunk.candidate_offsets.size() != source_count + 1U ||
        chunk.candidate_offsets.front() != 0U ||
        chunk.records.size() >
            policy_.max_candidate_records_per_chunk ||
        !std::in_range<std::uint64_t>(chunk.records.size()) ||
        chunk.candidate_offsets.back() !=
            static_cast<std::uint64_t>(chunk.records.size())) {
      throw std::runtime_error(
          "a chunked GPU K1 Boruvka payload does not close its source range");
    }
    std::size_t prior_offset = 0U;
    for (const std::uint64_t offset_word : chunk.candidate_offsets) {
      if (!std::in_range<std::size_t>(offset_word)) {
        throw std::runtime_error(
            "a chunked GPU K1 Boruvka relative offset does not fit size_t");
      }
      const std::size_t offset = static_cast<std::size_t>(offset_word);
      if (offset < prior_offset || offset > chunk.records.size()) {
        throw std::runtime_error(
            "chunked GPU K1 Boruvka relative offsets are not monotone");
      }
      prior_offset = offset;
    }

    logical_candidate_count_ = checked_size_sum(
        logical_candidate_count_,
        chunk.records.size(),
        "the chunked Phase 5 K1 Boruvka logical candidate count overflowed");
    source_chunk_count_ = checked_size_sum(
        source_chunk_count_,
        1U,
        "the chunked Phase 5 K1 Boruvka source chunk count overflowed");
    peak_chunk_source_count_ =
        std::max(peak_chunk_source_count_, source_count);
    peak_chunk_candidate_count_ =
        std::max(peak_chunk_candidate_count_, chunk.records.size());

    for (std::size_t local_source_index = 0U;
         local_source_index < source_count;
         ++local_source_index) {
      const std::size_t source_index =
          chunk.source_begin + local_source_index;
      const spatial::PointId source_id = checked_point_id(source_index);
      const std::size_t begin = static_cast<std::size_t>(
          chunk.candidate_offsets[local_source_index]);
      const std::size_t end = static_cast<std::size_t>(
          chunk.candidate_offsets[local_source_index + 1U]);
      if (begin == end) {
        throw std::runtime_error(
            "a nonterminal chunked GPU K1 Boruvka source has no candidate");
      }
      max_source_candidate_count_ = std::max(
          max_source_candidate_count_, end - begin);

      for (std::size_t position = begin; position < end; ++position) {
        const detail::K1BoruvkaCandidateRecord& record =
            chunk.records[position];
        const spatial::PointId record_source = checked_record_point_id(
            record.source_point_id, point_count);
        const spatial::PointId target_id = checked_record_point_id(
            record.target_point_id, point_count);
        const std::size_t target_index =
            checked_point_index(target_id, point_count);
        if (record_source != source_id ||
            labels_[target_index] == labels_[source_index] ||
            target_seen_[target_index] != 0U) {
          throw std::runtime_error(
              "a chunked GPU K1 Boruvka candidate is misplaced, internal or duplicated");
        }
        target_seen_[target_index] = 1U;
      }

      certify_required_candidates(source_index, source_id);
      resolve_source_minimum(
          source_index,
          source_id,
          chunk.records.subspan(begin, end - begin));

      for (std::size_t position = begin; position < end; ++position) {
        const spatial::PointId target_id = checked_record_point_id(
            chunk.records[position].target_point_id, point_count);
        target_seen_[checked_point_index(target_id, point_count)] = 0U;
      }
    }
    next_source_begin_ = chunk.source_end;
  }

  [[nodiscard]] K1BoruvkaChunkedRoundResolution finish(
      const detail::K1BoruvkaChunkedCandidateSummary& summary) {
    const std::size_t point_count = host_.point_count;
    if (next_source_begin_ != point_count ||
        source_chunk_count_ != summary.source_chunk_count ||
        logical_candidate_count_ != summary.logical_candidate_count ||
        peak_chunk_source_count_ != summary.peak_chunk_source_count ||
        peak_chunk_candidate_count_ !=
            summary.peak_chunk_candidate_count ||
        max_source_candidate_count_ !=
            summary.max_source_candidate_count ||
        summary.candidate_record_budget !=
            policy_.max_candidate_records_per_chunk ||
        summary.peak_chunk_candidate_count >
            summary.candidate_record_budget ||
        summary.device_candidate_capacity_high_water >
            summary.candidate_record_budget ||
        summary.host_candidate_capacity_high_water >
            summary.candidate_record_budget ||
        summary.device_candidate_capacity_high_water <
            summary.peak_chunk_candidate_count ||
        summary.host_candidate_capacity_high_water <
            summary.peak_chunk_candidate_count ||
        summary.kernel_launch_count == 0U ||
        summary.kernel_launch_count != summary.source_chunk_count + 1U ||
        summary.synchronization_count != summary.kernel_launch_count ||
        summary.count_pass_node_visit_count !=
            summary.emit_pass_node_visit_count ||
        summary.buffer_epoch == 0U ||
        summary.buffer_epoch <= host_.last_buffer_epoch ||
        !summary.complete_source_partition_certified ||
        !summary.count_emit_cardinality_and_visit_count_certified ||
        !summary.exact_capacity ||
        !summary.no_truncation) {
      throw std::runtime_error(
          "the chunked GPU K1 Boruvka summary does not certify a complete bounded emission");
    }

    const std::size_t physical_capacity = checked_size_sum(
        summary.device_candidate_capacity_high_water,
        summary.host_candidate_capacity_high_water,
        "the chunked Phase 5 K1 Boruvka physical capacity overflowed");
    const std::size_t payload_peak_bytes = checked_size_product(
        physical_capacity,
        kCandidateRecordSizeBytes,
        "the chunked Phase 5 K1 Boruvka payload byte count overflowed");
    const std::size_t payload_budget_bytes = checked_size_product(
        policy_.max_candidate_records_per_chunk,
        kCandidatePayloadCopies * kCandidateRecordSizeBytes,
        "the chunked Phase 5 K1 Boruvka payload budget overflowed");
    if (payload_peak_bytes > payload_budget_bytes) {
      throw std::runtime_error(
          "the chunked GPU K1 Boruvka candidate payload exceeded its physical bound");
    }

    K1BoruvkaChunkedRoundResolution resolution;
    resolution.cpu_exact_component_minima.reserve(component_count_);
    for (std::size_t point_index = 0U;
         point_index < point_count;
         ++point_index) {
      if (labels_[point_index] != checked_point_id(point_index)) {
        continue;
      }
      if (!component_minima_[point_index].has_value()) {
        throw std::runtime_error(
            "a chunked certified GPU K1 Boruvka component has no exact minimum");
      }
      const ResolvedCandidate& minimum = *component_minima_[point_index];
      resolution.cpu_exact_component_minima.push_back(
          hierarchy::K1BoruvkaComponentMinimum{
              checked_point_id(point_index),
              minimum.source_point_id,
              minimum.edge});
    }
    if (resolution.cpu_exact_component_minima.size() != component_count_) {
      throw std::runtime_error(
          "the chunked CPU K1 Boruvka resolution lost a frozen component");
    }

    proposal_audit_.gpu_candidate_count = summary.logical_candidate_count;
    proposal_audit_.gpu_output_capacity = summary.logical_candidate_count;
    proposal_audit_.gpu_kernel_launch_count = summary.kernel_launch_count;
    proposal_audit_.gpu_synchronization_count =
        summary.synchronization_count;
    proposal_audit_.gpu_count_pass_node_visit_count =
        summary.count_pass_node_visit_count;
    proposal_audit_.gpu_emit_pass_node_visit_count =
        summary.emit_pass_node_visit_count;
    proposal_audit_.gpu_uniform_component_prune_count =
        summary.uniform_component_prune_count;
    proposal_audit_.gpu_strict_aabb_prune_count =
        summary.strict_aabb_prune_count;
    proposal_audit_.gpu_invalid_bound_descent_count =
        summary.invalid_bound_descent_count;
    proposal_audit_.buffer_epoch = summary.buffer_epoch;
    proposal_audit_.proposal_digest_fnv1a = summary.proposal_digest_fnv1a;
    proposal_audit_.exact_capacity_certified = true;
    proposal_audit_.no_truncation_certified = true;
    proposal_audit_.candidate_superset_certified = true;
    proposal_audit_.cpu_exact_resolution_complete = true;
    resolution.proposal_audit = proposal_audit_;

    K1BoruvkaChunkedEmissionAudit& emission = resolution.emission_audit;
    emission.logical_candidate_count = summary.logical_candidate_count;
    emission.source_chunk_count = summary.source_chunk_count;
    emission.peak_chunk_source_count = summary.peak_chunk_source_count;
    emission.peak_chunk_candidate_count =
        summary.peak_chunk_candidate_count;
    emission.max_source_candidate_count =
        summary.max_source_candidate_count;
    emission.candidate_record_budget = summary.candidate_record_budget;
    emission.device_candidate_capacity_high_water =
        summary.device_candidate_capacity_high_water;
    emission.host_candidate_capacity_high_water =
        summary.host_candidate_capacity_high_water;
    emission.candidate_record_size_bytes = kCandidateRecordSizeBytes;
    emission.candidate_payload_peak_bytes = payload_peak_bytes;
    emission.count_kernel_launch_count = 1U;
    emission.emit_kernel_launch_count = summary.source_chunk_count;
    emission.synchronization_count = summary.synchronization_count;
    emission.complete_source_partition_certified = true;
    emission.count_emit_cardinality_and_visit_count_certified = true;
    emission.candidate_payload_physical_bound_certified = true;
    resolution.emission_status = K1BoruvkaEmissionStatus::
        complete_source_ranges_candidate_payload_bound_certified;
    host_.last_buffer_epoch = summary.buffer_epoch;
    return resolution;
  }

 private:
  void certify_required_candidates(
      std::size_t source_index,
      spatial::PointId source_id) {
    std::uint64_t node_word = static_cast<std::uint64_t>(host_.root_index);
    std::size_t step_count = 0U;
    while (node_word != detail::k1_boruvka_sentinel) {
      if (!std::in_range<std::size_t>(node_word) ||
          static_cast<std::size_t>(node_word) >= host_.nodes.size() ||
          step_count >= host_.nodes.size()) {
        throw std::runtime_error(
            "the chunked Phase 5 K1 Boruvka rope traversal is cyclic or invalid");
      }
      ++step_count;
      const std::size_t node_index = static_cast<std::size_t>(node_word);
      const detail::K1BoruvkaNodeInputRecord& node = host_.nodes[node_index];
      if (tags_[node_index] == labels_[source_index]) {
        node_word = node.escape;
        continue;
      }
      const exact::ExactLevel lower_bound =
          exact_aabb_lower_bound(node, cloud_, source_id);
      ++proposal_audit_.cpu_exact_aabb_bound_evaluation_count;
      if (lower_bound >
          seeds_.public_seeds[source_index].exact_squared_cutoff) {
        node_word = node.escape;
        continue;
      }
      if (node.leaf_point_id != detail::k1_boruvka_sentinel) {
        const spatial::PointId target_id = checked_record_point_id(
            node.leaf_point_id, host_.point_count);
        const std::size_t target_index =
            checked_point_index(target_id, host_.point_count);
        if (target_seen_[target_index] == 0U) {
          throw std::runtime_error(
              "the chunked GPU K1 Boruvka proposal omitted a required candidate");
        }
        ++proposal_audit_.cpu_required_candidate_count;
        node_word = node.escape;
      } else {
        node_word = node.left_child;
      }
    }
  }

  void resolve_source_minimum(
      std::size_t source_index,
      spatial::PointId source_id,
      std::span<const detail::K1BoruvkaCandidateRecord> records) {
    std::optional<ResolvedCandidate> point_minimum;
    for (const detail::K1BoruvkaCandidateRecord& record : records) {
      const spatial::PointId target_id = checked_record_point_id(
          record.target_point_id, host_.point_count);
      hierarchy::ExactEmstEdge edge =
          exact_edge(cloud_, source_id, target_id);
      ++proposal_audit_.cpu_exact_candidate_distance_evaluation_count;
      if (edge.squared_length >
          seeds_.public_seeds[source_index].exact_squared_cutoff) {
        continue;
      }
      ResolvedCandidate candidate{std::move(edge), source_id};
      if (!point_minimum.has_value() ||
          resolved_less(candidate, *point_minimum)) {
        point_minimum = std::move(candidate);
      }
    }
    if (!point_minimum.has_value()) {
      throw std::runtime_error(
          "the chunked certified GPU K1 Boruvka superset has no seed-bounded candidate");
    }
    const std::size_t component_index =
        checked_point_index(labels_[source_index], host_.point_count);
    std::optional<ResolvedCandidate>& component_minimum =
        component_minima_[component_index];
    if (!component_minimum.has_value() ||
        resolved_less(*point_minimum, *component_minimum)) {
      component_minimum = std::move(point_minimum);
    }
  }

  detail::K1BoruvkaCandidateHostState& host_;
  const spatial::CanonicalPointCloud& cloud_;
  std::span<const spatial::PointId> labels_;
  const std::vector<std::uint64_t>& tags_;
  const SeedBundle& seeds_;
  std::size_t component_count_{};
  K1BoruvkaChunkingPolicy policy_{};
  std::vector<std::optional<ResolvedCandidate>> component_minima_;
  std::vector<unsigned char> target_seen_;
  K1BoruvkaCandidateAudit proposal_audit_;
  std::size_t next_source_begin_{};
  std::size_t logical_candidate_count_{};
  std::size_t source_chunk_count_{};
  std::size_t peak_chunk_source_count_{};
  std::size_t peak_chunk_candidate_count_{};
  std::size_t max_source_candidate_count_{};
};

}  // namespace

K1BoruvkaCandidateContext::K1BoruvkaCandidateContext(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud)
    : state_(std::make_shared<detail::K1BoruvkaCandidateContextState>()),
      host_(std::make_unique<detail::K1BoruvkaCandidateHostState>()) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the Phase 5 K1 Boruvka GPU context requires a matching ready LBVH");
  }
  host_->cloud_identity = cloud.identity_;
  host_->point_count = cloud.size();
  host_->root_index = index.root_index_;
  host_->nodes.reserve(index.nodes_.size());
  host_->coordinate_bits.resize(host_->point_count * 3U);
  host_->morton_point_ids.reserve(host_->point_count);
  host_->morton_position_by_point_id.assign(
      host_->point_count, host_->point_count);
  for (std::size_t point_index = 0U;
       point_index < host_->point_count;
       ++point_index) {
    const std::array<std::uint64_t, 3> bits =
        cloud.point(checked_point_id(point_index)).canonical_input_bits();
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      host_->coordinate_bits[axis * host_->point_count + point_index] =
          bits[axis];
    }
  }
  for (std::size_t morton_position = 0U;
       morton_position < index.leaves_.size();
       ++morton_position) {
    const spatial::PointId point_id =
        index.leaves_[morton_position].point_id;
    const std::size_t point_index =
        checked_point_index(point_id, host_->point_count);
    if (host_->morton_position_by_point_id[point_index] !=
        host_->point_count) {
      throw std::logic_error(
          "the Phase 5 K1 Boruvka Morton order repeats a PointId");
    }
    host_->morton_point_ids.push_back(
        static_cast<std::uint64_t>(point_id));
    host_->morton_position_by_point_id[point_index] = morton_position;
  }
  if (host_->morton_point_ids.size() != host_->point_count ||
      std::find(
          host_->morton_position_by_point_id.begin(),
          host_->morton_position_by_point_id.end(),
          host_->point_count) !=
          host_->morton_position_by_point_id.end()) {
    throw std::logic_error(
        "the Phase 5 K1 Boruvka Morton order is not a complete permutation");
  }

  for (const auto& node : index.nodes_) {
    detail::K1BoruvkaNodeInputRecord record;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      record.lower_bits[axis] =
          cloud.point(node.lower_point_ids[axis]).canonical_input_bits()[axis];
      record.upper_bits[axis] =
          cloud.point(node.upper_point_ids[axis]).canonical_input_bits()[axis];
    }
    if (node.is_leaf()) {
      if (node.right_child != spatial::MortonLbvhIndex::invalid_node_index ||
          node.leaf_begin >= index.leaves_.size() ||
          node.leaf_end - node.leaf_begin != 1U) {
        throw std::logic_error(
            "the Phase 5 K1 Boruvka context received a malformed LBVH leaf");
      }
      record.leaf_point_id = index.leaves_[node.leaf_begin].point_id;
    } else {
      if (node.right_child == spatial::MortonLbvhIndex::invalid_node_index ||
          node.left_child >= index.nodes_.size() ||
          node.right_child >= index.nodes_.size()) {
        throw std::logic_error(
            "the Phase 5 K1 Boruvka context received malformed LBVH children");
      }
      record.left_child = static_cast<std::uint64_t>(node.left_child);
      record.right_child = static_cast<std::uint64_t>(node.right_child);
    }
    host_->nodes.push_back(record);
  }
  if (host_->nodes.empty() || host_->root_index >= host_->nodes.size()) {
    throw std::logic_error(
        "the Phase 5 K1 Boruvka context received an incomplete LBVH");
  }

  std::vector<unsigned char> node_seen(host_->nodes.size(), 0U);
  std::vector<std::pair<std::size_t, std::uint64_t>> pending;
  pending.emplace_back(host_->root_index, detail::k1_boruvka_sentinel);
  std::size_t visited_count = 0U;
  while (!pending.empty()) {
    const auto [node_index, escape] = pending.back();
    pending.pop_back();
    if (node_index >= host_->nodes.size() || node_seen[node_index] != 0U) {
      throw std::logic_error(
          "the Phase 5 K1 Boruvka LBVH is cyclic or repeats a node");
    }
    node_seen[node_index] = 1U;
    ++visited_count;
    detail::K1BoruvkaNodeInputRecord& node = host_->nodes[node_index];
    node.escape = escape;
    if (node.leaf_point_id == detail::k1_boruvka_sentinel) {
      const std::size_t left = static_cast<std::size_t>(node.left_child);
      const std::size_t right = static_cast<std::size_t>(node.right_child);
      pending.emplace_back(right, escape);
      pending.emplace_back(left, node.right_child);
    }
  }
  if (visited_count != host_->nodes.size()) {
    throw std::logic_error(
        "the Phase 5 K1 Boruvka rope does not cover every LBVH node");
  }
  host_->rope_topology_certified = true;
  host_->lbvh_topology_and_exact_aabbs_certified = true;
}

K1BoruvkaCandidateContext::~K1BoruvkaCandidateContext() noexcept = default;

K1BoruvkaCandidateContext::K1BoruvkaCandidateContext(
    K1BoruvkaCandidateContext&&) noexcept = default;

K1BoruvkaCandidateContext& K1BoruvkaCandidateContext::operator=(
    K1BoruvkaCandidateContext&&) noexcept = default;

void K1BoruvkaCandidateContext::require_matching_cloud(
    const spatial::CanonicalPointCloud& cloud) const {
  if (!state_ || !host_) {
    throw std::logic_error(
        "a moved-from Phase 5 K1 Boruvka GPU context cannot be used");
  }
  if (cloud.identity_ == nullptr ||
      host_->cloud_identity.get() != cloud.identity_.get() ||
      cloud.size() != host_->point_count) {
    throw std::invalid_argument(
        "the Phase 5 K1 Boruvka GPU context was called with another cloud");
  }
}

K1BoruvkaRoundProposal K1BoruvkaCandidateContext::propose_round(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> frozen_component_labels) {
  require_matching_cloud(cloud);
  const std::size_t component_count = validate_frozen_labels(
      frozen_component_labels, host_->point_count);

  std::size_t uniform_node_count = 0U;
  std::size_t mixed_node_count = 0U;
  const std::vector<std::uint64_t> tags = build_component_tags(
      *host_,
      frozen_component_labels,
      uniform_node_count,
      mixed_node_count);

  if (component_count == 1U) {
    return state_->with_healthy_section([&]() {
      K1BoruvkaRoundProposal proposal;
      proposal.frozen_component_labels.assign(
          frozen_component_labels.begin(), frozen_component_labels.end());
      proposal.candidate_offsets.assign(host_->point_count + 1U, 0U);
      proposal.audit.resident_point_count = host_->point_count;
      proposal.audit.resident_node_count = host_->nodes.size();
      proposal.audit.frozen_component_count = 1U;
      proposal.audit.uniform_lbvh_node_count = uniform_node_count;
      proposal.audit.mixed_lbvh_node_count = mixed_node_count;
      proposal.audit.frozen_labels_certified = true;
      proposal.audit.rope_topology_certified =
          host_->rope_topology_certified;
      proposal.audit.exact_capacity_certified = true;
      proposal.audit.no_truncation_certified = true;
      proposal.audit.candidate_superset_certified = true;
      proposal.audit.cpu_exact_resolution_complete = true;
      proposal.audit.proposal_digest_fnv1a = proposal_digest(
          proposal.candidate_offsets, proposal.candidates);
      return proposal;
    });
  }

  SeedBundle seeds = build_canonical_fallback_seeds(
      cloud, frozen_component_labels);
  std::vector<std::uint64_t> labels(
      frozen_component_labels.begin(), frozen_component_labels.end());
  return state_->with_gpu_section([&]() {
    const detail::K1BoruvkaCandidateBatch batch =
        detail::propose_k1_boruvka_candidates_on_gpu(
            *state_,
            host_->nodes,
            host_->root_index,
            host_->coordinate_bits,
            host_->point_count,
            labels,
            tags,
            seeds.upper_bits);
    return validate_and_resolve(
        *host_,
        cloud,
        frozen_component_labels,
        tags,
        std::move(seeds),
        batch,
        component_count,
        uniform_node_count,
        mixed_node_count);
  });
}

K1BoruvkaChunkedRoundResolution
K1BoruvkaCandidateContext::propose_round_chunked(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> frozen_component_labels,
    K1BoruvkaChunkingPolicy policy) {
  return propose_round_chunked_impl(
      cloud, frozen_component_labels, policy, nullptr);
}

K1BoruvkaChunkedRoundResolution
K1BoruvkaCandidateContext::propose_round_chunked(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> frozen_component_labels,
    K1BoruvkaChunkingPolicy chunking_policy,
    K1BoruvkaMortonSeedPolicy seed_policy) {
  validate_morton_seed_policy(seed_policy);
  return propose_round_chunked_impl(
      cloud,
      frozen_component_labels,
      chunking_policy,
      &seed_policy);
}

K1BoruvkaSeededExactRoundResolution
K1BoruvkaCandidateContext::resolve_round_exact_external_1nn(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> frozen_component_labels,
    K1BoruvkaMortonSeedPolicy seed_policy) {
  require_matching_cloud(cloud);
  validate_morton_seed_policy(seed_policy);
  const std::size_t component_count = validate_frozen_labels(
      frozen_component_labels, host_->point_count);
  if (component_count <= 1U) {
    throw std::invalid_argument(
        "the exact external-1NN resolver requires a nonterminal frozen partition");
  }

  std::size_t uniform_node_count = 0U;
  std::size_t mixed_node_count = 0U;
  const std::vector<std::uint64_t> tags = build_component_tags(
      *host_,
      frozen_component_labels,
      uniform_node_count,
      mixed_node_count);
  std::vector<std::uint64_t> labels(
      frozen_component_labels.begin(), frozen_component_labels.end());
  return state_->with_gpu_section([&]() {
    const detail::K1BoruvkaMortonSeedProposalBatch seed_proposals =
        detail::propose_k1_boruvka_morton_seeds_on_gpu(
            *state_,
            host_->nodes,
            host_->root_index,
            host_->coordinate_bits,
            host_->point_count,
            host_->morton_point_ids,
            labels,
            seed_policy.window_radius);
    const SeedBundle seeds = build_morton_refined_seeds(
        *host_,
        cloud,
        frozen_component_labels,
        seed_policy,
        seed_proposals);
    return resolve_seeded_exact_external_1nn(
        *host_,
        cloud,
        frozen_component_labels,
        tags,
        seeds,
        component_count,
        uniform_node_count,
        mixed_node_count);
  });
}

K1BoruvkaSeededDualTreeRoundResolution
K1BoruvkaCandidateContext::resolve_round_exact_external_1nn_dual_tree(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> frozen_component_labels,
    K1BoruvkaMortonSeedPolicy seed_policy) {
  require_matching_cloud(cloud);
  validate_morton_seed_policy(seed_policy);
  const std::size_t component_count = validate_frozen_labels(
      frozen_component_labels, host_->point_count);
  if (component_count <= 1U) {
    throw std::invalid_argument(
        "the exact dual-tree resolver requires a nonterminal frozen partition");
  }

  std::size_t uniform_node_count = 0U;
  std::size_t mixed_node_count = 0U;
  const std::vector<std::uint64_t> tags = build_component_tags(
      *host_,
      frozen_component_labels,
      uniform_node_count,
      mixed_node_count);
  std::vector<std::uint64_t> labels(
      frozen_component_labels.begin(), frozen_component_labels.end());
  return state_->with_gpu_section([&]() {
    const detail::K1BoruvkaMortonSeedProposalBatch seed_proposals =
        detail::propose_k1_boruvka_morton_seeds_on_gpu(
            *state_,
            host_->nodes,
            host_->root_index,
            host_->coordinate_bits,
            host_->point_count,
            host_->morton_point_ids,
            labels,
            seed_policy.window_radius);
    const SeedBundle seeds = build_morton_refined_seeds(
        *host_,
        cloud,
        frozen_component_labels,
        seed_policy,
        seed_proposals);
    return resolve_seeded_exact_external_1nn_dual_tree(
        *host_,
        cloud,
        frozen_component_labels,
        tags,
        seeds,
        component_count,
        uniform_node_count,
        mixed_node_count);
  });
}

K1BoruvkaSeededComponentDualTreeRoundResolution
K1BoruvkaCandidateContext::resolve_round_exact_component_minima_dual_tree(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> frozen_component_labels,
    K1BoruvkaMortonSeedPolicy seed_policy,
    K1BoruvkaComponentEnvelopeMode envelope_mode) {
  require_matching_cloud(cloud);
  validate_morton_seed_policy(seed_policy);
  validate_component_envelope_mode(envelope_mode);
  const std::size_t component_count = validate_frozen_labels(
      frozen_component_labels, host_->point_count);
  if (component_count <= 1U) {
    throw std::invalid_argument(
        "the component-direct dual-tree resolver requires a nonterminal frozen partition");
  }

  std::size_t uniform_node_count = 0U;
  std::size_t mixed_node_count = 0U;
  const std::vector<std::uint64_t> tags = build_component_tags(
      *host_,
      frozen_component_labels,
      uniform_node_count,
      mixed_node_count);
  std::vector<std::uint64_t> labels(
      frozen_component_labels.begin(), frozen_component_labels.end());
  return state_->with_gpu_section([&]() {
    const detail::K1BoruvkaMortonSeedProposalBatch seed_proposals =
        detail::propose_k1_boruvka_morton_seeds_on_gpu(
            *state_,
            host_->nodes,
            host_->root_index,
            host_->coordinate_bits,
            host_->point_count,
            host_->morton_point_ids,
            labels,
            seed_policy.window_radius);
    const SeedBundle seeds = build_morton_refined_seeds(
        *host_,
        cloud,
        frozen_component_labels,
        seed_policy,
        seed_proposals);
    return resolve_seeded_exact_component_minima_dual_tree(
        *host_,
        cloud,
        frozen_component_labels,
        tags,
        seeds,
        component_count,
        uniform_node_count,
        mixed_node_count,
        envelope_mode);
  });
}

K1BoruvkaChunkedRoundResolution
K1BoruvkaCandidateContext::propose_round_chunked_impl(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const spatial::PointId> frozen_component_labels,
    K1BoruvkaChunkingPolicy policy,
    const K1BoruvkaMortonSeedPolicy* seed_policy) {
  require_matching_cloud(cloud);
  validate_chunking_policy(policy);
  if (seed_policy != nullptr) {
    validate_morton_seed_policy(*seed_policy);
  }
  const std::size_t component_count = validate_frozen_labels(
      frozen_component_labels, host_->point_count);

  std::size_t uniform_node_count = 0U;
  std::size_t mixed_node_count = 0U;
  const std::vector<std::uint64_t> tags = build_component_tags(
      *host_,
      frozen_component_labels,
      uniform_node_count,
      mixed_node_count);

  if (component_count == 1U) {
    return state_->with_gpu_section([&]() {
      const std::size_t device_candidate_capacity =
          detail::enforce_k1_boruvka_candidate_budget_on_gpu(
              *state_, policy.max_candidate_records_per_chunk);
      if (device_candidate_capacity >
          policy.max_candidate_records_per_chunk) {
        throw std::runtime_error(
            "the terminal chunked GPU K1 Boruvka workspace exceeds its candidate budget");
      }
      K1BoruvkaChunkedRoundResolution resolution;
      K1BoruvkaCandidateAudit& proposal = resolution.proposal_audit;
      proposal.resident_point_count = host_->point_count;
      proposal.resident_node_count = host_->nodes.size();
      proposal.frozen_component_count = 1U;
      proposal.uniform_lbvh_node_count = uniform_node_count;
      proposal.mixed_lbvh_node_count = mixed_node_count;
      proposal.proposal_digest_fnv1a =
          terminal_proposal_digest(host_->point_count);
      proposal.frozen_labels_certified = true;
      proposal.rope_topology_certified =
          host_->rope_topology_certified;
      proposal.exact_capacity_certified = true;
      proposal.no_truncation_certified = true;
      proposal.candidate_superset_certified = true;
      proposal.cpu_exact_resolution_complete = true;

      K1BoruvkaChunkedEmissionAudit& emission =
          resolution.emission_audit;
      emission.candidate_record_budget =
          policy.max_candidate_records_per_chunk;
      emission.device_candidate_capacity_high_water =
          device_candidate_capacity;
      emission.candidate_record_size_bytes = kCandidateRecordSizeBytes;
      emission.candidate_payload_peak_bytes = checked_size_product(
          device_candidate_capacity,
          kCandidateRecordSizeBytes,
          "the terminal chunked Phase 5 K1 Boruvka payload byte count overflowed");
      emission.complete_source_partition_certified = true;
      emission.count_emit_cardinality_and_visit_count_certified = true;
      emission.candidate_payload_physical_bound_certified = true;
      resolution.emission_status = K1BoruvkaEmissionStatus::
          complete_source_ranges_candidate_payload_bound_certified;
      return resolution;
    });
  }

  std::vector<std::uint64_t> labels(
      frozen_component_labels.begin(), frozen_component_labels.end());
  return state_->with_gpu_section([&]() {
    SeedBundle seeds;
    if (seed_policy == nullptr) {
      seeds = build_canonical_fallback_seeds(
          cloud, frozen_component_labels);
    } else {
      const detail::K1BoruvkaMortonSeedProposalBatch seed_proposals =
          detail::propose_k1_boruvka_morton_seeds_on_gpu(
              *state_,
              host_->nodes,
              host_->root_index,
              host_->coordinate_bits,
              host_->point_count,
              host_->morton_point_ids,
              labels,
              seed_policy->window_radius);
      seeds = build_morton_refined_seeds(
          *host_,
          cloud,
          frozen_component_labels,
          *seed_policy,
          seed_proposals);
    }
    ChunkedRoundAccumulator accumulator{
        *host_,
        cloud,
        frozen_component_labels,
        tags,
        seeds,
        component_count,
        uniform_node_count,
        mixed_node_count,
        policy};
    const detail::K1BoruvkaCandidateChunkConsumer consume_chunk =
        [&](const detail::K1BoruvkaCandidateChunkView& chunk) {
          accumulator.consume(chunk);
        };
    const detail::K1BoruvkaChunkedCandidateSummary summary =
        detail::propose_k1_boruvka_candidates_chunked_on_gpu(
            *state_,
            host_->nodes,
            host_->root_index,
            host_->coordinate_bits,
            host_->point_count,
            labels,
            tags,
            seeds.upper_bits,
            policy.max_candidate_records_per_chunk,
            consume_chunk);
    K1BoruvkaChunkedRoundResolution resolution =
        accumulator.finish(summary);
    resolution.morton_seed_audit = seeds.morton_audit;
    resolution.seed_status = seeds.status;
    return resolution;
  });
}

std::size_t K1BoruvkaCandidateContext::point_count() const noexcept {
  return host_ ? host_->point_count : 0U;
}

std::size_t K1BoruvkaCandidateContext::node_count() const noexcept {
  return host_ ? host_->nodes.size() : 0U;
}

}  // namespace morsehgp3d::gpu
