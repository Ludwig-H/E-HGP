#include "morsehgp3d/gpu/binary64_lbvh_top_k.hpp"

#include "phase14_binary64_lbvh_top_k_internal.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu::detail {

class Phase14Binary64LbvhTopKHostState final {
 public:
  std::shared_ptr<const void> cloud_identity;
  std::shared_ptr<void> adopted_owner;
  const std::uint64_t* device_coordinate_bits{};
  const std::uint64_t* device_morton_point_ids{};
  const void* device_nodes{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t persistent_input_device_byte_capacity{};
  std::uint64_t source_snapshot_epoch{};
  int cuda_device{-1};
  bool poisoned{};
  std::mutex mutex;
};

}  // namespace morsehgp3d::gpu::detail

namespace morsehgp3d::gpu {
namespace {

[[nodiscard]] std::size_t checked_node_count(std::size_t point_count) {
  if (point_count == 0U ||
      point_count > std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::length_error(
        "the binary64 LBVH top-k node count overflows size_t");
  }
  return point_count * 2U - 1U;
}

[[nodiscard]] std::size_t checked_neighbor_count(
    std::size_t point_count,
    std::size_t maximum_order) {
  if (maximum_order == 0U ||
      point_count >
          std::numeric_limits<std::size_t>::max() / maximum_order) {
    throw std::length_error(
        "the binary64 LBVH top-k transcript extent overflows size_t");
  }
  return point_count * maximum_order;
}

}  // namespace

bool Binary64LbvhTopKResult::validated_complete_binary64_transcript()
    const noexcept {
  if (schema_version != binary64_lbvh_top_k_schema_version ||
      audit.maximum_order == 0U || audit.maximum_order > 10U ||
      audit.point_count <= audit.maximum_order ||
      audit.point_count >
          std::numeric_limits<std::size_t>::max() / 2U + 1U ||
      audit.certified_node_count != audit.point_count * 2U - 1U ||
      audit.point_count >
          std::numeric_limits<std::size_t>::max() /
              audit.maximum_order) {
    return false;
  }
  const std::size_t expected = audit.point_count * audit.maximum_order;
  return source_point_ids_by_morton_position.size() == audit.point_count &&
         neighbor_point_ids.size() == expected &&
         squared_distances.size() == expected &&
         audit.neighbor_record_count == expected &&
         audit.completed_query_count == audit.point_count &&
         audit.failed_query_count == 0U &&
         audit.traversal_lease_adopted &&
         audit.fixed_round_to_nearest_distance_recipe_requested &&
         audit.directed_round_down_aabb_recipe_requested &&
         audit.strict_prune_requested &&
         audit.stackless_postorder_traversal_requested &&
         audit.complete_query_coverage && audit.no_candidate_truncation &&
         audit.source_snapshot_epoch != 0U &&
         audit.source_morton_permutation_validated &&
         audit.host_transcript_structure_validated &&
         !audit.higher_order_delaunay_mosaic_materialized &&
         !audit.global_pair_matrix_materialized &&
         !audit.exact_morse_hierarchy_claimed &&
         !audit.public_status_claimed;
}

bool Binary64LbvhCsrNeighborRankResult::
    validated_complete_binary64_rank_prefix() const noexcept {
  if (schema_version != binary64_lbvh_csr_neighbor_rank_schema_version ||
      audit.point_count == 0U ||
      audit.point_count <= audit.maximum_rank ||
      audit.maximum_rank < 2U ||
      audit.maximum_rank > binary64_lbvh_csr_neighbor_rank_maximum ||
      audit.overflow_rank != audit.maximum_rank + 1U ||
      audit.overflow_rank >
          static_cast<std::size_t>(
              std::numeric_limits<std::uint16_t>::max()) ||
      audit.point_count >
          std::numeric_limits<std::size_t>::max() / 2U + 1U ||
      audit.certified_node_count != audit.point_count * 2U - 1U ||
      audit.query_batch_size == 0U ||
      audit.query_batch_size > audit.point_count ||
      audit.query_batch_count !=
          1U + (audit.point_count - 1U) / audit.query_batch_size ||
      audit.kernel_local_size_bytes_per_thread <
          binary64_lbvh_csr_neighbor_rank_maximum *
              (sizeof(double) + sizeof(spatial::PointId)) ||
      directed_csr_ranks.size() != audit.directed_csr_arc_count ||
      audit.ranked_arc_count + audit.overflow_arc_count !=
          audit.directed_csr_arc_count ||
      audit.completed_query_count != audit.point_count ||
      audit.failed_query_count != 0U) {
    return false;
  }
  return audit.traversal_lease_adopted &&
         audit.fixed_round_to_nearest_distance_recipe_requested &&
         audit.directed_round_down_aabb_recipe_requested &&
         audit.strict_prune_requested && audit.equality_descends_requested &&
         audit.stackless_postorder_traversal_requested &&
         audit.complete_query_coverage &&
         audit.no_search_work_cap &&
         audit.exact_binary64_prefix_complete &&
         audit.ranks_above_maximum_censored &&
         audit.source_mapping_permutation_validated &&
         audit.csr_structure_validated &&
         audit.host_rank_transcript_validated &&
         !audit.n_by_maximum_rank_table_materialized &&
         !audit.higher_order_delaunay_mosaic_materialized &&
         !audit.global_pair_matrix_materialized &&
         !audit.exact_morse_hierarchy_claimed &&
         !audit.public_status_claimed && audit.source_snapshot_epoch != 0U;
}

Binary64LbvhTopKContext::Binary64LbvhTopKContext(
    const spatial::CanonicalPointCloud& cloud,
    MortonLbvhDeviceTraversalLease&& traversal_lease)
    : host_(
          std::make_unique<detail::Phase14Binary64LbvhTopKHostState>()) {
  if (!traversal_lease.ready() || !traversal_lease.cuda_resident()) {
    throw std::invalid_argument(
        "a binary64 LBVH top-k context requires a resident CUDA traversal lease");
  }
  const MortonLbvhDeviceTraversalLeaseAudit& audit =
      traversal_lease.audit();
  const std::size_t point_count = cloud.size();
  const std::size_t node_count = checked_node_count(point_count);
  const std::size_t coordinate_word_count = checked_neighbor_count(
      point_count, 3U);
  if (cloud.identity_ == nullptr ||
      traversal_lease.source_cloud_identity_ == nullptr ||
      cloud.identity_.get() !=
          traversal_lease.source_cloud_identity_.get() ||
      audit.point_count != point_count ||
      audit.certified_node_count != node_count ||
      audit.retained_coordinate_word_capacity < coordinate_word_count ||
      audit.retained_morton_point_id_capacity < point_count ||
      audit.retained_node_capacity < node_count ||
      audit.persistent_device_byte_capacity == 0U ||
      audit.retained_host_snapshot_byte_count != 0U ||
      audit.second_host_snapshot_retained ||
      audit.higher_order_delaunay_mosaic_materialized ||
      audit.global_cell_or_coface_arena_materialized ||
      audit.public_status_claimed ||
      traversal_lease.device_coordinate_bits_ == nullptr ||
      traversal_lease.device_morton_point_ids_ == nullptr ||
      traversal_lease.device_nodes_ == nullptr ||
      traversal_lease.cuda_device_ < 0) {
    throw std::invalid_argument(
        "a binary64 LBVH top-k context received an incompatible traversal lease");
  }

  // Move the unique owner only after every potentially throwing validation.
  host_->cloud_identity = traversal_lease.source_cloud_identity_;
  host_->point_count = point_count;
  host_->certified_node_count = node_count;
  host_->persistent_input_device_byte_capacity =
      audit.persistent_device_byte_capacity;
  host_->source_snapshot_epoch = audit.source_snapshot_epoch;
  host_->device_coordinate_bits =
      traversal_lease.device_coordinate_bits_;
  host_->device_morton_point_ids =
      traversal_lease.device_morton_point_ids_;
  host_->device_nodes = traversal_lease.device_nodes_;
  host_->cuda_device = traversal_lease.cuda_device_;
  host_->adopted_owner =
      std::move(traversal_lease.retained_resources_);
  traversal_lease.source_cloud_identity_.reset();
  traversal_lease.device_coordinate_bits_ = nullptr;
  traversal_lease.device_morton_point_ids_ = nullptr;
  traversal_lease.device_nodes_ = nullptr;
  traversal_lease.cuda_device_ = -1;
}

Binary64LbvhTopKContext::~Binary64LbvhTopKContext() noexcept = default;
Binary64LbvhTopKContext::Binary64LbvhTopKContext(
    Binary64LbvhTopKContext&&) noexcept = default;
Binary64LbvhTopKContext& Binary64LbvhTopKContext::operator=(
    Binary64LbvhTopKContext&&) noexcept = default;

void Binary64LbvhTopKContext::require_matching_cloud(
    const spatial::CanonicalPointCloud& cloud) const {
  if (!host_ || host_->poisoned) {
    throw std::logic_error(
        "a moved-from or poisoned binary64 LBVH top-k context cannot be used");
  }
  if (cloud.identity_ == nullptr || host_->cloud_identity == nullptr ||
      cloud.identity_.get() != host_->cloud_identity.get() ||
      cloud.size() != host_->point_count) {
    throw std::invalid_argument(
        "a binary64 LBVH top-k context was called with another PointId namespace");
  }
}

Binary64LbvhTopKResult Binary64LbvhTopKContext::query_all(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t maximum_order,
    std::size_t seed_window_radius) {
  if (!host_) {
    throw std::logic_error(
        "a moved-from binary64 LBVH top-k context cannot be used");
  }
  std::scoped_lock lock{host_->mutex};
  require_matching_cloud(cloud);
  if (maximum_order == 0U || maximum_order > 10U ||
      host_->point_count <= maximum_order ||
      seed_window_radius < maximum_order) {
    throw std::invalid_argument(
        "binary64 LBVH top-k requires 1 <= K <= 10, n > K and seed W >= K");
  }
  static_cast<void>(checked_neighbor_count(
      host_->point_count, maximum_order));
  const std::size_t effective_seed_window =
      seed_window_radius < host_->point_count
          ? seed_window_radius
          : host_->point_count - 1U;
  try {
    Binary64LbvhTopKResult result =
        detail::run_phase14_binary64_lbvh_top_k_on_gpu(
            host_->device_coordinate_bits,
            host_->device_morton_point_ids,
            host_->device_nodes,
            host_->point_count,
            host_->certified_node_count,
            host_->persistent_input_device_byte_capacity,
            host_->source_snapshot_epoch,
            maximum_order,
            effective_seed_window,
            host_->cuda_device);
    std::vector<unsigned char> source_seen(host_->point_count, 0U);
    for (std::size_t position = 0U;
         position < host_->point_count;
         ++position) {
      const spatial::PointId source =
          result.source_point_ids_by_morton_position[position];
      if (!std::in_range<std::size_t>(source) ||
          static_cast<std::size_t>(source) >= host_->point_count ||
          source_seen[static_cast<std::size_t>(source)] != 0U) {
        throw std::runtime_error(
            "the binary64 LBVH source Morton order is not a permutation");
      }
      source_seen[static_cast<std::size_t>(source)] = 1U;
      const std::size_t row = position * maximum_order;
      for (std::size_t rank = 0U; rank < maximum_order; ++rank) {
        const spatial::PointId candidate =
            result.neighbor_point_ids[row + rank];
        const double distance = result.squared_distances[row + rank];
        if (!std::in_range<std::size_t>(candidate) ||
            static_cast<std::size_t>(candidate) >= host_->point_count ||
            candidate == source || distance != distance || distance < 0.0) {
          throw std::runtime_error(
              "the binary64 LBVH transcript contains an invalid neighbor");
        }
        for (std::size_t previous = 0U; previous < rank; ++previous) {
          if (result.neighbor_point_ids[row + previous] == candidate) {
            throw std::runtime_error(
                "the binary64 LBVH transcript repeats a neighbor");
          }
        }
        if (rank != 0U) {
          const double previous_distance =
              result.squared_distances[row + rank - 1U];
          const spatial::PointId previous_id =
              result.neighbor_point_ids[row + rank - 1U];
          if (distance < previous_distance ||
              (distance == previous_distance && candidate < previous_id)) {
            throw std::runtime_error(
                "the binary64 LBVH transcript is not lexicographically sorted");
          }
        }
      }
    }
    result.audit.source_morton_permutation_validated = true;
    result.audit.host_transcript_structure_validated = true;
    if (!result.validated_complete_binary64_transcript()) {
      throw std::runtime_error(
          "the binary64 LBVH top-k launcher returned an invalid transcript");
    }
    return result;
  } catch (...) {
    host_->poisoned = true;
    throw;
  }
}

Binary64LbvhCsrNeighborRankResult
Binary64LbvhTopKContext::query_csr_neighbor_ranks(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const std::uint64_t> csr_offsets,
    std::span<const spatial::PointId> csr_neighbors,
    std::span<const spatial::PointId> source_point_id_by_canonical_id,
    std::size_t maximum_rank,
    std::size_t seed_window_radius,
    std::size_t query_batch_size) {
  if (!host_) {
    throw std::logic_error(
        "a moved-from binary64 LBVH top-k context cannot be used");
  }
  std::scoped_lock lock{host_->mutex};
  require_matching_cloud(cloud);
  if (maximum_rank < 2U ||
      maximum_rank > binary64_lbvh_csr_neighbor_rank_maximum ||
      host_->point_count <= maximum_rank ||
      seed_window_radius < maximum_rank || query_batch_size == 0U) {
    throw std::invalid_argument(
        "binary64 LBVH CSR ranks require 2 <= M <= 256, n > M, "
        "seed W >= M and a nonzero query batch");
  }
  if (csr_offsets.size() != host_->point_count + 1U ||
      csr_offsets.front() != 0U ||
      csr_offsets.back() != csr_neighbors.size() ||
      source_point_id_by_canonical_id.size() != host_->point_count) {
    throw std::invalid_argument(
        "binary64 LBVH CSR ranks received incompatible CSR extents");
  }
  std::vector<unsigned char> source_seen(host_->point_count, 0U);
  for (std::size_t canonical = 0U; canonical < host_->point_count;
       ++canonical) {
    const spatial::PointId source =
        source_point_id_by_canonical_id[canonical];
    if (!std::in_range<std::size_t>(source) ||
        static_cast<std::size_t>(source) >= host_->point_count ||
        source_seen[static_cast<std::size_t>(source)] != 0U) {
      throw std::invalid_argument(
          "binary64 LBVH CSR source mapping is not a permutation");
    }
    source_seen[static_cast<std::size_t>(source)] = 1U;
  }
  for (std::size_t source = 0U; source < host_->point_count; ++source) {
    const std::uint64_t begin_u64 = csr_offsets[source];
    const std::uint64_t end_u64 = csr_offsets[source + 1U];
    if (end_u64 < begin_u64 || end_u64 > csr_neighbors.size()) {
      throw std::invalid_argument(
          "binary64 LBVH CSR offsets are not monotone");
    }
    const std::size_t begin = static_cast<std::size_t>(begin_u64);
    const std::size_t end = static_cast<std::size_t>(end_u64);
    for (std::size_t position = begin; position < end; ++position) {
      const spatial::PointId target = csr_neighbors[position];
      if (!std::in_range<std::size_t>(target) ||
          static_cast<std::size_t>(target) >= host_->point_count ||
          static_cast<std::size_t>(target) == source ||
          (position != begin && !(csr_neighbors[position - 1U] < target))) {
        throw std::invalid_argument(
            "binary64 LBVH CSR rows must contain strictly increasing valid "
            "non-self PointIds");
      }
    }
  }

  const std::size_t effective_seed_window =
      std::min(seed_window_radius, host_->point_count - 1U);
  const std::size_t effective_query_batch =
      std::min(query_batch_size, host_->point_count);
  try {
    Binary64LbvhCsrNeighborRankResult result =
        detail::run_phase15_binary64_lbvh_csr_neighbor_rank_on_gpu(
            host_->device_coordinate_bits,
            host_->device_morton_point_ids,
            host_->device_nodes,
            host_->point_count,
            host_->certified_node_count,
            host_->persistent_input_device_byte_capacity,
            host_->source_snapshot_epoch,
            csr_offsets,
            csr_neighbors,
            source_point_id_by_canonical_id,
            maximum_rank,
            effective_seed_window,
            effective_query_batch,
            host_->cuda_device);
    std::size_t ranked_count{};
    std::size_t overflow_count{};
    const auto overflow_rank = static_cast<std::uint16_t>(maximum_rank + 1U);
    for (const std::uint16_t rank : result.directed_csr_ranks) {
      if (rank >= 1U && rank <= maximum_rank) {
        ++ranked_count;
      } else if (rank == overflow_rank) {
        ++overflow_count;
      } else {
        throw std::runtime_error(
            "binary64 LBVH CSR rank transcript contains an invalid rank");
      }
    }
    result.audit.ranked_arc_count = ranked_count;
    result.audit.overflow_arc_count = overflow_count;
    result.audit.source_mapping_permutation_validated = true;
    result.audit.csr_structure_validated = true;
    result.audit.host_rank_transcript_validated = true;
    if (!result.validated_complete_binary64_rank_prefix()) {
      throw std::runtime_error(
          "binary64 LBVH CSR rank launcher returned an invalid transcript");
    }
    return result;
  } catch (...) {
    host_->poisoned = true;
    throw;
  }
}

std::size_t Binary64LbvhTopKContext::point_count() const noexcept {
  return host_ ? host_->point_count : 0U;
}

std::size_t Binary64LbvhTopKContext::certified_node_count() const noexcept {
  return host_ ? host_->certified_node_count : 0U;
}

}  // namespace morsehgp3d::gpu
