#include "morsehgp3d/gpu/k1_boruvka.hpp"

#include "phase5_k1_boruvka_internal.hpp"

#include "morsehgp3d/exact/rational.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
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
  std::uint64_t last_buffer_epoch{};
  bool rope_topology_certified{false};
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
};

[[nodiscard]] SeedBundle build_seeds(
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
        !summary.count_emit_identity_certified ||
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
    emission.count_emit_identity_certified = true;
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

  SeedBundle seeds = build_seeds(cloud, frozen_component_labels);
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
  require_matching_cloud(cloud);
  validate_chunking_policy(policy);
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
      emission.count_emit_identity_certified = true;
      emission.candidate_payload_physical_bound_certified = true;
      resolution.emission_status = K1BoruvkaEmissionStatus::
          complete_source_ranges_candidate_payload_bound_certified;
      return resolution;
    });
  }

  SeedBundle seeds = build_seeds(cloud, frozen_component_labels);
  std::vector<std::uint64_t> labels(
      frozen_component_labels.begin(), frozen_component_labels.end());
  return state_->with_gpu_section([&]() {
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
    return accumulator.finish(summary);
  });
}

std::size_t K1BoruvkaCandidateContext::point_count() const noexcept {
  return host_ ? host_->point_count : 0U;
}

std::size_t K1BoruvkaCandidateContext::node_count() const noexcept {
  return host_ ? host_->nodes.size() : 0U;
}

}  // namespace morsehgp3d::gpu
