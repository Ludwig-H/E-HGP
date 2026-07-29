#include "morsehgp3d/gpu/morton_yao48_pair_frontier.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu {
namespace {

constexpr std::array<std::array<std::uint8_t, 3>, 6>
    axis_permutations{{
        {{0U, 1U, 2U}},
        {{0U, 2U, 1U}},
        {{1U, 0U, 2U}},
        {{1U, 2U, 0U}},
        {{2U, 0U, 1U}},
        {{2U, 1U, 0U}},
    }};

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    const char* message) {
  if constexpr (
      std::numeric_limits<std::size_t>::max() >
      std::numeric_limits<std::uint64_t>::max()) {
    if (value > std::numeric_limits<std::uint64_t>::max()) {
      throw std::length_error(message);
    }
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::uint64_t checked_sum(
    std::uint64_t left,
    std::uint64_t right,
    const char* message) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    throw std::overflow_error(message);
  }
  return left + right;
}

void checked_accumulate(
    std::uint64_t& destination,
    std::uint64_t increment,
    const char* message) {
  destination = checked_sum(destination, increment, message);
}

[[nodiscard]] std::size_t checked_full_binary_node_count(
    std::size_t leaf_count,
    const char* message) {
  if (leaf_count == 0U ||
      leaf_count >
          (std::numeric_limits<std::size_t>::max() / 2U) + 1U) {
    throw std::length_error(message);
  }
  return 2U * leaf_count - 1U;
}

[[nodiscard]] std::uint64_t checked_unordered_pair_count(
    std::size_t point_count) {
  if (point_count == 0U) {
    throw std::invalid_argument(
        "the Morton/Yao48 frontier requires a nonempty cloud");
  }
  const std::uint64_t count = checked_u64(
      point_count,
      "the Morton/Yao48 point count does not fit uint64");
  const std::uint64_t predecessor = count - UINT64_C(1);
  const std::uint64_t left =
      count % UINT64_C(2) == 0U ? count / UINT64_C(2) : count;
  const std::uint64_t right =
      count % UINT64_C(2) == 0U ? predecessor
                                : predecessor / UINT64_C(2);
  if (left != 0U &&
      right > std::numeric_limits<std::uint64_t>::max() / left) {
    throw std::length_error(
        "the Morton/Yao48 unordered-pair universe overflows uint64");
  }
  return left * right;
}

[[nodiscard]] bool leaf_key_less(
    const spatial::MortonLeafRecord& left,
    const spatial::MortonLeafRecord& right) noexcept {
  if (left.morton_code != right.morton_code) {
    return left.morton_code < right.morton_code;
  }
  return left.point_id < right.point_id;
}

[[nodiscard]] exact::ExactLevel exact_squared_distance(
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId left_id,
    spatial::PointId right_id) {
  const exact::ExactRational3& left = cloud.point(left_id).exact();
  const exact::ExactRational3& right = cloud.point(right_id).exact();
  exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational delta =
        left.coordinate(axis) - right.coordinate(axis);
    squared_distance = squared_distance + delta * delta;
  }
  exact::ExactLevel level{std::move(squared_distance)};
  if (!(level > exact::ExactLevel{})) {
    throw std::logic_error(
        "a Morton/Yao48 witness must be geometrically distinct");
  }
  return level;
}

[[nodiscard]] exact::ExactLevel triple_level(
    const exact::ExactLevel& level) {
  return exact::ExactLevel{
      exact::BigInt{3} * level.numerator(), level.denominator()};
}

[[nodiscard]] exact::ExactLevel exact_aabb_lower_bound(
    const std::array<spatial::PointId, 3>& lower_point_ids,
    const std::array<spatial::PointId, 3>& upper_point_ids,
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId anchor_id) {
  const exact::ExactRational3& anchor = cloud.point(anchor_id).exact();
  exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational coordinate = anchor.coordinate(axis);
    const exact::ExactRational lower =
        cloud.point(lower_point_ids[axis]).coordinate(axis);
    const exact::ExactRational upper =
        cloud.point(upper_point_ids[axis]).coordinate(axis);
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

struct NonnegativeInterval {
  exact::ExactRational lower{};
  exact::ExactRational upper{};
  bool nonempty{false};
};

[[nodiscard]] NonnegativeInterval oriented_nonnegative_interval(
    const exact::ExactRational& displacement_lower,
    const exact::ExactRational& displacement_upper,
    bool negative_direction) {
  NonnegativeInterval interval;
  if (negative_direction) {
    interval.lower = -displacement_upper;
    interval.upper = -displacement_lower;
  } else {
    interval.lower = displacement_lower;
    interval.upper = displacement_upper;
  }
  if (interval.upper < exact::ExactRational{}) {
    return interval;
  }
  if (interval.lower < exact::ExactRational{}) {
    interval.lower = exact::ExactRational{};
  }
  interval.nonempty = interval.lower <= interval.upper;
  return interval;
}

[[nodiscard]] bool ordered_intervals_feasible(
    const std::array<NonnegativeInterval, 3>& intervals,
    const std::array<std::uint8_t, 3>& descending_axes) {
  const std::size_t smallest_axis =
      static_cast<std::size_t>(descending_axes[2]);
  const std::size_t middle_axis =
      static_cast<std::size_t>(descending_axes[1]);
  const std::size_t largest_axis =
      static_cast<std::size_t>(descending_axes[0]);
  exact::ExactRational selected = intervals[smallest_axis].lower;
  if (selected > intervals[smallest_axis].upper) {
    return false;
  }
  if (selected < intervals[middle_axis].lower) {
    selected = intervals[middle_axis].lower;
  }
  if (selected > intervals[middle_axis].upper) {
    return false;
  }
  if (selected < intervals[largest_axis].lower) {
    selected = intervals[largest_axis].lower;
  }
  return selected <= intervals[largest_axis].upper;
}

struct ClosedConeMaskEvaluation {
  std::uint64_t mask{};
  std::uint64_t test_count{};
};

// The half-open leaf chambers are contained in these exact closed chambers.
// Any equality or interval overlap stays in the mask and therefore descends
// unless the conservative radial certificate closes every possible chamber.
[[nodiscard]] ClosedConeMaskEvaluation exact_closed_cone_aabb_mask(
    const std::array<spatial::PointId, 3>& lower_point_ids,
    const std::array<spatial::PointId, 3>& upper_point_ids,
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId anchor_id) {
  const exact::ExactRational3& anchor = cloud.point(anchor_id).exact();
  std::array<exact::ExactRational, 3> displacement_lower{};
  std::array<exact::ExactRational, 3> displacement_upper{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    displacement_lower[axis] =
        cloud.point(lower_point_ids[axis]).coordinate(axis) -
        anchor.coordinate(axis);
    displacement_upper[axis] =
        cloud.point(upper_point_ids[axis]).coordinate(axis) -
        anchor.coordinate(axis);
  }

  ClosedConeMaskEvaluation evaluation;
  for (std::uint8_t negative_mask = 0U;
       negative_mask < 8U;
       ++negative_mask) {
    std::array<NonnegativeInterval, 3> intervals{};
    bool orthant_possible = true;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const std::uint8_t bit = static_cast<std::uint8_t>(
          std::uint32_t{1U} << static_cast<std::uint32_t>(axis));
      intervals[axis] = oriented_nonnegative_interval(
          displacement_lower[axis],
          displacement_upper[axis],
          (negative_mask & bit) != 0U);
      orthant_possible = orthant_possible && intervals[axis].nonempty;
    }
    for (std::size_t permutation_index = 0U;
         permutation_index < axis_permutations.size();
         ++permutation_index) {
      checked_accumulate(
          evaluation.test_count,
          UINT64_C(1),
          "the Morton/Yao48 cone-AABB test count overflows uint64");
      if (!orthant_possible ||
          !ordered_intervals_feasible(
              intervals, axis_permutations[permutation_index])) {
        continue;
      }
      const std::size_t cone_index =
          static_cast<std::size_t>(negative_mask) *
              axis_permutations.size() +
          permutation_index;
      evaluation.mask |= UINT64_C(1) << cone_index;
    }
  }
  return evaluation;
}

}  // namespace

MortonYao48PairFrontierContext::MortonYao48PairFrontierContext(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    MortonYao48PairFrontierConfig config)
    : config_(config) {
  if (config_.maximum_closed_rank < 2U ||
      config_.maximum_closed_rank >
          morton_yao48_pair_frontier_maximum_closed_rank) {
    throw std::out_of_range(
        "a Morton/Yao48 maximum closed rank must be in [2, 11]");
  }
  if (!traversal_lease.ready()) {
    throw std::invalid_argument(
        "Morton/Yao48 traversal requires a ready traversal lease");
  }
  if (traversal_lease.cuda_resident() ||
      !traversal_lease.audit().host_fake_lifecycle_exercised) {
    throw std::invalid_argument(
        "the current Morton/Yao48 executable specification is host/fake "
        "only");
  }

  const MortonLbvhDeviceTraversalLeaseAudit& lease_audit =
      traversal_lease.audit();
  const std::size_t expected_node_count =
      checked_full_binary_node_count(
          lease_audit.point_count,
          "the Morton/Yao48 lease point extent overflows size_t");
  if (lease_audit.certified_node_count != expected_node_count ||
      lease_audit.source_snapshot_epoch == 0U ||
      !lease_audit.source_snapshot_import_certified ||
      !lease_audit.active_morton_point_ids_retained ||
      !lease_audit.certified_device_nodes_retained ||
      lease_audit.second_host_snapshot_retained ||
      lease_audit.higher_order_delaunay_mosaic_materialized ||
      lease_audit.global_cell_or_coface_arena_materialized ||
      lease_audit.public_status_claimed) {
    throw std::invalid_argument(
        "the Morton/Yao48 traversal lease is not a closed Phase 14M "
        "authority");
  }

  point_count_ = lease_audit.point_count;
  certified_node_count_ = lease_audit.certified_node_count;
  unordered_pair_universe_count_ =
      checked_unordered_pair_count(point_count_);
  completed_anchor_count_ = 1U;
  if (point_count_ == 1U) {
    next_anchor_position_ = point_count_;
  } else {
    next_anchor_position_ = 1U;
    active_node_index_ = certified_node_count_ - 1U;
  }
  traversal_lease_.emplace(std::move(traversal_lease));
}

bool MortonYao48PairFrontierContext::ready() const noexcept {
  return traversal_lease_.has_value() && traversal_lease_->ready() &&
         !traversal_lease_->cuda_resident() &&
         traversal_lease_->audit().host_fake_lifecycle_exercised;
}

std::size_t MortonYao48PairFrontierContext::point_count() const noexcept {
  return ready() ? point_count_ : 0U;
}

std::size_t
MortonYao48PairFrontierContext::certified_node_count() const noexcept {
  return ready() ? certified_node_count_ : 0U;
}

void MortonYao48PairFrontierContext::bind_or_validate_source(
    const MortonLbvhDeviceBuildResult& certified_build,
    const spatial::CanonicalPointCloud& cloud) {
  if (!ready()) {
    throw std::logic_error(
        "a moved-from Morton/Yao48 frontier cannot advance");
  }
  if (!certified_build.complete_certified_build()) {
    throw std::invalid_argument(
        "Morton/Yao48 traversal requires a complete certified Phase 14M "
        "build");
  }
  const MortonLbvhDeviceBuildAudit& build_audit =
      certified_build.audit();
  const MortonLbvhDeviceTraversalLeaseAudit& lease_audit =
      traversal_lease_->audit();
  if (build_audit.point_count != point_count_ ||
      build_audit.required_node_count != certified_node_count_ ||
      build_audit.snapshot_buffer_epoch !=
          lease_audit.source_snapshot_epoch ||
      !build_audit.stable_morton_point_id_order_validated ||
      !build_audit.strict_find_split_postorder_requested ||
      !build_audit.snapshot_import_certified) {
    throw std::invalid_argument(
        "the Morton/Yao48 build and traversal lease authorities disagree");
  }

  const spatial::MortonLbvhIndex& index =
      certified_build.certified_index();
  if (!index.ready() || !index.validated_for(cloud) ||
      cloud.size() != point_count_ ||
      index.point_count_ != point_count_ ||
      index.leaves_.size() != point_count_ ||
      index.nodes_.size() != certified_node_count_ ||
      index.root_index_ != certified_node_count_ - 1U ||
      index.cloud_identity_.get() !=
          traversal_lease_->source_cloud_identity_.get() ||
      !index.identity_) {
    throw std::invalid_argument(
        "the Morton/Yao48 cloud and host index are not the traversal "
        "lease source");
  }

  if (source_index_identity_) {
    if (source_index_identity_.get() != index.identity_.get() ||
        source_cloud_identity_.get() != index.cloud_identity_.get()) {
      throw std::invalid_argument(
          "a resumed Morton/Yao48 frontier requires the same index and "
          "cloud identities");
    }
    return;
  }

  for (std::size_t position = 1U;
       position < index.leaves_.size();
       ++position) {
    if (!leaf_key_less(
            index.leaves_[position - 1U],
            index.leaves_[position])) {
      throw std::logic_error(
          "the certified Morton leaves are not strictly ordered by "
          "(MortonCode, PointId)");
    }
  }

  for (std::size_t node_index = 0U;
       node_index < index.nodes_.size();
       ++node_index) {
    const spatial::MortonLbvhIndex::Node& node =
        index.nodes_[node_index];
    if (node.leaf_begin >= node.leaf_end ||
        node.leaf_end > point_count_) {
      throw std::logic_error(
          "a certified Morton node has an invalid leaf interval");
    }
    const std::size_t width = node.leaf_end - node.leaf_begin;
    if (width == 1U) {
      if (!node.is_leaf() ||
          node.right_child !=
              spatial::MortonLbvhIndex::invalid_node_index) {
        throw std::logic_error(
            "a unit Morton interval is not a postorder leaf");
      }
      continue;
    }
    if (node.is_leaf() ||
        node.right_child ==
            spatial::MortonLbvhIndex::invalid_node_index ||
        node.left_child >= node_index ||
        node.right_child >= node_index ||
        node.right_child + 1U != node_index) {
      throw std::logic_error(
          "a certified Morton internal node breaks strict postorder");
    }
    const spatial::MortonLbvhIndex::Node& left =
        index.nodes_[node.left_child];
    const spatial::MortonLbvhIndex::Node& right =
        index.nodes_[node.right_child];
    if (left.leaf_begin != node.leaf_begin ||
        left.leaf_end != right.leaf_begin ||
        right.leaf_end != node.leaf_end) {
      throw std::logic_error(
          "a certified Morton internal interval is not partitioned by "
          "its children");
    }
    const std::size_t right_subtree_node_count =
        checked_full_binary_node_count(
            right.leaf_end - right.leaf_begin,
            "a Morton right subtree extent overflows size_t");
    if (right_subtree_node_count > node.right_child ||
        node.left_child !=
            node.right_child - right_subtree_node_count) {
      throw std::logic_error(
          "a certified Morton subtree is not contiguous in postorder");
    }
  }
  const spatial::MortonLbvhIndex::Node& root =
      index.nodes_[index.root_index_];
  if (root.leaf_begin != 0U || root.leaf_end != point_count_) {
    throw std::logic_error(
        "the certified Morton root does not cover every leaf");
  }

  source_index_identity_ = index.identity_;
  source_cloud_identity_ = index.cloud_identity_;
  source_build_authenticated_ = true;
  source_cloud_authenticated_ = true;
  morton_point_id_total_order_validated_ = true;
  strict_postorder_interval_tree_validated_ = true;
}

void MortonYao48PairFrontierContext::reset_active_witness_banks()
    noexcept {
  for (WitnessBank& bank : active_witness_banks_) {
    bank = WitnessBank{};
  }
  active_full_witness_bank_count_ = 0U;
  active_retained_witness_count_ = 0U;
}

void MortonYao48PairFrontierContext::complete_active_anchor() {
  const std::uint64_t expected_owned = checked_u64(
      next_anchor_position_,
      "a Morton/Yao48 anchor position does not fit uint64");
  const std::uint64_t actual_owned = checked_sum(
      active_anchor_candidate_pair_mass_,
      active_anchor_certified_pruned_pair_mass_,
      "a Morton/Yao48 active anchor mass overflows uint64");
  const std::uint64_t expected_suffix = checked_u64(
      point_count_ - next_anchor_position_,
      "a Morton/Yao48 suffix mass does not fit uint64");
  if (actual_owned != expected_owned ||
      active_anchor_suffix_leaf_mass_ != expected_suffix) {
    every_completed_anchor_partition_validated_ = false;
    throw std::logic_error(
        "a Morton/Yao48 anchor traversal did not partition its prefix "
        "and suffix exactly");
  }

  ++completed_anchor_count_;
  ++next_anchor_position_;
  active_anchor_candidate_pair_mass_ = 0U;
  active_anchor_certified_pruned_pair_mass_ = 0U;
  active_anchor_suffix_leaf_mass_ = 0U;
  reset_active_witness_banks();
  if (next_anchor_position_ == point_count_) {
    active_node_index_ =
        morton_yao48_pair_frontier_invalid_node_index;
    const std::uint64_t processed = checked_sum(
        cumulative_candidate_pair_mass_,
        cumulative_certified_pruned_pair_mass_,
        "the Morton/Yao48 global processed mass overflows uint64");
    if (completed_anchor_count_ != point_count_ ||
        processed != unordered_pair_universe_count_ ||
        cumulative_suffix_skipped_leaf_mass_ !=
            unordered_pair_universe_count_) {
      every_completed_anchor_partition_validated_ = false;
      throw std::logic_error(
          "the Morton/Yao48 global pair partition did not close");
    }
    return;
  }
  active_node_index_ = certified_node_count_ - 1U;
}

MortonYao48PairFrontierAudit
MortonYao48PairFrontierContext::audit() const noexcept {
  MortonYao48PairFrontierAudit value;
  value.point_count = point_count_;
  value.certified_node_count = certified_node_count_;
  value.maximum_closed_rank = config_.maximum_closed_rank;
  value.required_witness_count = config_.maximum_closed_rank - 1U;
  value.completed_anchor_count = completed_anchor_count_;
  value.next_anchor_position = next_anchor_position_;
  value.active_node_index = active_node_index_;
  value.active_full_witness_bank_count =
      active_full_witness_bank_count_;
  value.active_retained_witness_count =
      active_retained_witness_count_;
  value.peak_active_retained_witness_count =
      peak_active_retained_witness_count_;
  value.source_snapshot_epoch =
      traversal_lease_.has_value()
          ? traversal_lease_->audit().source_snapshot_epoch
          : UINT64_C(0);
  value.unordered_pair_universe_count =
      unordered_pair_universe_count_;
  value.active_anchor_candidate_pair_mass =
      active_anchor_candidate_pair_mass_;
  value.active_anchor_certified_pruned_pair_mass =
      active_anchor_certified_pruned_pair_mass_;
  value.active_anchor_suffix_leaf_mass =
      active_anchor_suffix_leaf_mass_;
  value.cumulative_candidate_pair_mass =
      cumulative_candidate_pair_mass_;
  value.cumulative_certified_pruned_pair_mass =
      cumulative_certified_pruned_pair_mass_;
  const bool mass_within_universe =
      cumulative_candidate_pair_mass_ <=
              unordered_pair_universe_count_ &&
      cumulative_certified_pruned_pair_mass_ <=
          unordered_pair_universe_count_ -
              cumulative_candidate_pair_mass_;
  const std::uint64_t processed =
      mass_within_universe
          ? cumulative_candidate_pair_mass_ +
                cumulative_certified_pruned_pair_mass_
          : unordered_pair_universe_count_;
  value.unresolved_pair_mass =
      unordered_pair_universe_count_ - processed;
  value.cumulative_node_visit_count = cumulative_node_visit_count_;
  value.cumulative_internal_node_descent_count =
      cumulative_internal_node_descent_count_;
  value.cumulative_boundary_node_descent_count =
      cumulative_boundary_node_descent_count_;
  value.cumulative_suffix_skip_node_count =
      cumulative_suffix_skip_node_count_;
  value.cumulative_suffix_skipped_leaf_mass =
      cumulative_suffix_skipped_leaf_mass_;
  value.cumulative_leaf_visit_count = cumulative_leaf_visit_count_;
  value.cumulative_exact_cone_classification_count =
      cumulative_exact_cone_classification_count_;
  value.cumulative_exact_witness_distance_evaluation_count =
      cumulative_exact_witness_distance_evaluation_count_;
  value.cumulative_frozen_witness_bank_count =
      cumulative_frozen_witness_bank_count_;
  value.cumulative_witness_bank_receipt_count =
      cumulative_witness_bank_receipt_count_;
  value.cumulative_directional_leaf_cutoff_evaluation_count =
      cumulative_directional_leaf_cutoff_evaluation_count_;
  value.cumulative_directional_leaf_prune_count =
      cumulative_directional_leaf_prune_count_;
  value.cumulative_closed_cone_aabb_test_count =
      cumulative_closed_cone_aabb_test_count_;
  value.cumulative_exact_aabb_lower_bound_evaluation_count =
      cumulative_exact_aabb_lower_bound_evaluation_count_;
  value.cumulative_radial_subtree_prune_count =
      cumulative_radial_subtree_prune_count_;
  value.cumulative_radial_subtree_pruned_pair_mass =
      cumulative_radial_subtree_pruned_pair_mass_;
  value.cumulative_certified_prune_region_count =
      cumulative_certified_prune_region_count_;
  value.source_build_authenticated = source_build_authenticated_;
  value.source_cloud_authenticated = source_cloud_authenticated_;
  value.morton_point_id_total_order_validated =
      morton_point_id_total_order_validated_;
  value.strict_postorder_interval_tree_validated =
      strict_postorder_interval_tree_validated_;
  value.every_completed_anchor_partition_validated =
      every_completed_anchor_partition_validated_;
  value.candidate_plus_certified_pruned_mass_validated =
      mass_within_universe;
  value.frontier_complete =
      source_build_authenticated_ && source_cloud_authenticated_ &&
      next_anchor_position_ == point_count_ &&
      completed_anchor_count_ == point_count_ &&
      mass_within_universe &&
      processed == unordered_pair_universe_count_ &&
      value.unresolved_pair_mass == 0U;
  value.traversal_lease_owner_retained = ready();
  value.host_fake_reference_execution = ready();
  value.active_storage_bounded_by_48_times_k =
      active_retained_witness_count_ <=
      hierarchy::exact_yao48_cone_count *
          (config_.maximum_closed_rank - 1U);
  return value;
}

MortonYao48PairFrontierAdvance
MortonYao48PairFrontierContext::advance(
    const MortonLbvhDeviceBuildResult& certified_build,
    const spatial::CanonicalPointCloud& cloud,
    MortonYao48PairFrontierAdvanceBudget budget) {
  bind_or_validate_source(certified_build, cloud);
  const spatial::MortonLbvhIndex& index =
      certified_build.certified_index();
  MortonYao48PairFrontierAdvance result;
  std::size_t advance_node_visit_count = 0U;
  const std::size_t required_witness_count =
      config_.maximum_closed_rank - 1U;

  const auto exhausted = [&](MortonYao48PairFrontierStopReason reason) {
    result.status = MortonYao48PairFrontierStatus::budget_exhausted;
    result.stop_reason = reason;
    result.audit = audit();
  };

  const auto finish_or_skip_subtree = [&](std::size_t width) {
    const std::size_t subtree_node_count =
        checked_full_binary_node_count(
            width,
            "a Morton/Yao48 terminal subtree extent overflows size_t");
    if (subtree_node_count > active_node_index_ + 1U) {
      throw std::logic_error(
          "a Morton/Yao48 postorder subtree jump crosses index zero");
    }
    if (subtree_node_count == active_node_index_ + 1U) {
      complete_active_anchor();
    } else {
      active_node_index_ -= subtree_node_count;
    }
  };

  while (next_anchor_position_ < point_count_) {
    if (advance_node_visit_count ==
        budget.maximum_node_visit_count) {
      exhausted(
          MortonYao48PairFrontierStopReason::node_visit_capacity);
      return result;
    }
    if (active_node_index_ >= index.nodes_.size()) {
      throw std::logic_error(
          "the Morton/Yao48 resume cursor is outside the certified tree");
    }

    const spatial::MortonLbvhIndex::Node& node =
        index.nodes_[active_node_index_];
    const std::size_t width = node.leaf_end - node.leaf_begin;
    const bool entirely_owned =
        node.leaf_end <= next_anchor_position_;
    const bool entirely_suffix =
        node.leaf_begin >= next_anchor_position_;

    if (!entirely_owned && !entirely_suffix) {
      if (node.is_leaf() ||
          !(node.leaf_begin < next_anchor_position_ &&
            next_anchor_position_ < node.leaf_end)) {
        throw std::logic_error(
            "a Morton/Yao48 boundary node cannot be descended exactly");
      }
      ++advance_node_visit_count;
      checked_accumulate(
          cumulative_node_visit_count_, UINT64_C(1),
          "the Morton/Yao48 node-visit count overflows uint64");
      checked_accumulate(
          cumulative_boundary_node_descent_count_, UINT64_C(1),
          "the Morton/Yao48 boundary descent count overflows uint64");
      active_node_index_ = node.right_child;
      continue;
    }

    if (entirely_suffix) {
      const std::uint64_t leaf_mass = checked_u64(
          width,
          "a Morton/Yao48 suffix leaf mass does not fit uint64");
      ++advance_node_visit_count;
      checked_accumulate(
          cumulative_node_visit_count_, UINT64_C(1),
          "the Morton/Yao48 node-visit count overflows uint64");
      checked_accumulate(
          cumulative_suffix_skip_node_count_, UINT64_C(1),
          "the Morton/Yao48 suffix skip count overflows uint64");
      checked_accumulate(
          active_anchor_suffix_leaf_mass_, leaf_mass,
          "a Morton/Yao48 active suffix mass overflows uint64");
      checked_accumulate(
          cumulative_suffix_skipped_leaf_mass_, leaf_mass,
          "the Morton/Yao48 global suffix mass overflows uint64");
      finish_or_skip_subtree(width);
      continue;
    }

    const spatial::PointId anchor_id =
        index.leaves_[next_anchor_position_].point_id;
    if (!node.is_leaf()) {
      const ClosedConeMaskEvaluation mask_evaluation =
          exact_closed_cone_aabb_mask(
              node.lower_point_ids,
              node.upper_point_ids,
              cloud,
              anchor_id);
      if (mask_evaluation.mask == UINT64_C(0)) {
        throw std::logic_error(
            "an owned nonempty AABB intersects no closed Yao48 chamber");
      }

      bool all_intersected_banks_full = true;
      std::optional<exact::ExactLevel> maximum_three_radius;
      for (std::size_t cone_index = 0U;
           cone_index < active_witness_banks_.size();
           ++cone_index) {
        if ((mask_evaluation.mask &
             (UINT64_C(1) << cone_index)) == UINT64_C(0)) {
          continue;
        }
        const WitnessBank& bank = active_witness_banks_[cone_index];
        if (bank.count != required_witness_count) {
          all_intersected_banks_full = false;
          break;
        }
        const exact::ExactLevel three_radius =
            triple_level(bank.squared_radius_upper_bound);
        if (!maximum_three_radius.has_value() ||
            *maximum_three_radius < three_radius) {
          maximum_three_radius = three_radius;
        }
      }

      std::optional<exact::ExactLevel> lower_bound;
      bool radial_prune = false;
      if (all_intersected_banks_full) {
        if (!maximum_three_radius.has_value()) {
          throw std::logic_error(
              "a nonempty Yao48 cone mask has no witness radius");
        }
        lower_bound = exact_aabb_lower_bound(
            node.lower_point_ids,
            node.upper_point_ids,
            cloud,
            anchor_id);
        radial_prune = *lower_bound >= *maximum_three_radius;
      }

      if (radial_prune &&
          result.certified_prune_regions.size() ==
              budget.maximum_certified_prune_region_count) {
        exhausted(MortonYao48PairFrontierStopReason::
                      certified_prune_region_capacity);
        return result;
      }

      if (radial_prune) {
        const std::uint64_t pair_mass = checked_u64(
            width,
            "a Morton/Yao48 radial prune mass does not fit uint64");
        MortonYao48CertifiedPruneRegion record;
        record.source_snapshot_epoch =
            traversal_lease_->audit().source_snapshot_epoch;
        record.anchor_position = checked_u64(
            next_anchor_position_,
            "a Morton/Yao48 anchor position does not fit uint64");
        record.anchor_point_id = anchor_id;
        record.node_index = checked_u64(
            active_node_index_,
            "a Morton/Yao48 node index does not fit uint64");
        record.leaf_begin = checked_u64(
            node.leaf_begin,
            "a Morton/Yao48 leaf begin does not fit uint64");
        record.leaf_end = checked_u64(
            node.leaf_end,
            "a Morton/Yao48 leaf end does not fit uint64");
        record.cone_mask = mask_evaluation.mask;
        record.certified_pair_mass = pair_mass;
        record.maximum_closed_rank = config_.maximum_closed_rank;
        record.kind = MortonYao48CertifiedPruneKind::radial_subtree;
        result.certified_prune_regions.push_back(std::move(record));

        ++advance_node_visit_count;
        checked_accumulate(
            cumulative_node_visit_count_, UINT64_C(1),
            "the Morton/Yao48 node-visit count overflows uint64");
        checked_accumulate(
            cumulative_closed_cone_aabb_test_count_,
            mask_evaluation.test_count,
            "the Morton/Yao48 cone-AABB count overflows uint64");
        checked_accumulate(
            cumulative_exact_aabb_lower_bound_evaluation_count_,
            UINT64_C(1),
            "the Morton/Yao48 AABB-bound count overflows uint64");
        checked_accumulate(
            cumulative_radial_subtree_prune_count_, UINT64_C(1),
            "the Morton/Yao48 radial prune count overflows uint64");
        checked_accumulate(
            cumulative_radial_subtree_pruned_pair_mass_, pair_mass,
            "the Morton/Yao48 radial prune mass overflows uint64");
        checked_accumulate(
            cumulative_certified_prune_region_count_, UINT64_C(1),
            "the Morton/Yao48 prune-region count overflows uint64");
        checked_accumulate(
            active_anchor_certified_pruned_pair_mass_, pair_mass,
            "the Morton/Yao48 active pruned mass overflows uint64");
        checked_accumulate(
            cumulative_certified_pruned_pair_mass_, pair_mass,
            "the Morton/Yao48 global pruned mass overflows uint64");
        finish_or_skip_subtree(width);
        continue;
      }

      ++advance_node_visit_count;
      checked_accumulate(
          cumulative_node_visit_count_, UINT64_C(1),
          "the Morton/Yao48 node-visit count overflows uint64");
      checked_accumulate(
          cumulative_closed_cone_aabb_test_count_,
          mask_evaluation.test_count,
          "the Morton/Yao48 cone-AABB count overflows uint64");
      if (lower_bound.has_value()) {
        checked_accumulate(
            cumulative_exact_aabb_lower_bound_evaluation_count_,
            UINT64_C(1),
            "the Morton/Yao48 AABB-bound count overflows uint64");
      }
      checked_accumulate(
          cumulative_internal_node_descent_count_, UINT64_C(1),
          "the Morton/Yao48 internal descent count overflows uint64");
      active_node_index_ = node.right_child;
      continue;
    }

    if (width != 1U || node.leaf_begin >= next_anchor_position_) {
      throw std::logic_error(
          "an owned Morton/Yao48 terminal is not one strict-prefix leaf");
    }
    const std::size_t partner_position = node.leaf_begin;
    const spatial::PointId partner_id =
        index.leaves_[partner_position].point_id;
    const hierarchy::ExactYao48ConeKey cone_key =
        hierarchy::classify_exact_yao48_cone(
            cloud.point(anchor_id), cloud.point(partner_id));
    if (cone_key.cone_index >= active_witness_banks_.size()) {
      throw std::logic_error(
          "a Morton/Yao48 leaf cone index exceeds 47");
    }
    WitnessBank& bank = active_witness_banks_[cone_key.cone_index];
    for (std::size_t witness_index = 0U;
         witness_index < bank.count;
         ++witness_index) {
      if (bank.point_ids[witness_index] == partner_id) {
        throw std::logic_error(
            "the current Morton/Yao48 target is already a witness");
      }
    }

    bool directional_prune = false;
    const bool directional_cutoff_evaluated =
        bank.count == required_witness_count;
    if (directional_cutoff_evaluated) {
      directional_prune =
          hierarchy::
              exact_yao48_directional_witness_radius_cutoff_certifies(
                  cloud.point(anchor_id),
                  cloud.point(partner_id),
                  bank.squared_radius_upper_bound,
                  hierarchy::ExactYao48DirectionalCutoffSemantics::
                      closed_ball);
    }

    if (directional_prune) {
      if (result.certified_prune_regions.size() ==
          budget.maximum_certified_prune_region_count) {
        exhausted(MortonYao48PairFrontierStopReason::
                      certified_prune_region_capacity);
        return result;
      }
      MortonYao48CertifiedPruneRegion record;
      record.source_snapshot_epoch =
          traversal_lease_->audit().source_snapshot_epoch;
      record.anchor_position = checked_u64(
          next_anchor_position_,
          "a Morton/Yao48 anchor position does not fit uint64");
      record.anchor_point_id = anchor_id;
      record.node_index = checked_u64(
          active_node_index_,
          "a Morton/Yao48 node index does not fit uint64");
      record.leaf_begin = checked_u64(
          partner_position,
          "a Morton/Yao48 leaf position does not fit uint64");
      record.leaf_end = checked_u64(
          partner_position + 1U,
          "a Morton/Yao48 leaf end does not fit uint64");
      record.cone_mask = UINT64_C(1) << cone_key.cone_index;
      record.certified_pair_mass = UINT64_C(1);
      record.maximum_closed_rank = config_.maximum_closed_rank;
      record.kind = MortonYao48CertifiedPruneKind::directional_leaf;
      result.certified_prune_regions.push_back(std::move(record));

      ++advance_node_visit_count;
      checked_accumulate(
          cumulative_node_visit_count_, UINT64_C(1),
          "the Morton/Yao48 node-visit count overflows uint64");
      checked_accumulate(
          cumulative_leaf_visit_count_, UINT64_C(1),
          "the Morton/Yao48 leaf-visit count overflows uint64");
      checked_accumulate(
          cumulative_exact_cone_classification_count_, UINT64_C(1),
          "the Morton/Yao48 cone classification count overflows uint64");
      checked_accumulate(
          cumulative_directional_leaf_cutoff_evaluation_count_,
          UINT64_C(1),
          "the Morton/Yao48 directional cutoff count overflows uint64");
      checked_accumulate(
          cumulative_directional_leaf_prune_count_, UINT64_C(1),
          "the Morton/Yao48 directional prune count overflows uint64");
      checked_accumulate(
          cumulative_certified_prune_region_count_, UINT64_C(1),
          "the Morton/Yao48 prune-region count overflows uint64");
      checked_accumulate(
          active_anchor_certified_pruned_pair_mass_, UINT64_C(1),
          "the Morton/Yao48 active pruned mass overflows uint64");
      checked_accumulate(
          cumulative_certified_pruned_pair_mass_, UINT64_C(1),
          "the Morton/Yao48 global pruned mass overflows uint64");
      finish_or_skip_subtree(1U);
      continue;
    }

    if (result.candidates.size() ==
        budget.maximum_candidate_count) {
      exhausted(MortonYao48PairFrontierStopReason::candidate_capacity);
      return result;
    }

    WitnessBank updated_bank = bank;
    bool bank_fills = false;
    if (bank.count < required_witness_count) {
      const exact::ExactLevel squared_distance =
          exact_squared_distance(cloud, anchor_id, partner_id);
      updated_bank.point_ids[updated_bank.count] = partner_id;
      ++updated_bank.count;
      if (updated_bank.count == 1U ||
          updated_bank.squared_radius_upper_bound < squared_distance) {
        updated_bank.squared_radius_upper_bound = squared_distance;
      }
      bank_fills = updated_bank.count == required_witness_count;
    }
    if (bank_fills &&
        result.witness_bank_receipts.size() ==
            budget.maximum_witness_bank_receipt_count) {
      exhausted(MortonYao48PairFrontierStopReason::
                    witness_bank_receipt_capacity);
      return result;
    }

    MortonYao48PairCandidate candidate;
    candidate.source_snapshot_epoch =
        traversal_lease_->audit().source_snapshot_epoch;
    candidate.support_ids =
        anchor_id < partner_id
            ? std::array<spatial::PointId, 2>{anchor_id, partner_id}
            : std::array<spatial::PointId, 2>{partner_id, anchor_id};
    candidate.morton_owner_position = checked_u64(
        next_anchor_position_,
        "a Morton/Yao48 anchor position does not fit uint64");
    candidate.morton_partner_position = checked_u64(
        partner_position,
        "a Morton/Yao48 partner position does not fit uint64");
    candidate.owner_cone_index = cone_key.cone_index;
    candidate.maximum_closed_rank = config_.maximum_closed_rank;
    result.candidates.push_back(std::move(candidate));

    if (bank_fills) {
      MortonYao48WitnessBankReceipt receipt;
      receipt.source_snapshot_epoch =
          traversal_lease_->audit().source_snapshot_epoch;
      receipt.anchor_position = checked_u64(
          next_anchor_position_,
          "a Morton/Yao48 anchor position does not fit uint64");
      receipt.anchor_point_id = anchor_id;
      receipt.cone_index = cone_key.cone_index;
      receipt.maximum_closed_rank = config_.maximum_closed_rank;
      receipt.witness_point_ids = updated_bank.point_ids;
      receipt.witness_count = updated_bank.count;
      receipt.witness_squared_radius_upper_bound =
          updated_bank.squared_radius_upper_bound;
      result.witness_bank_receipts.push_back(std::move(receipt));
    }

    ++advance_node_visit_count;
    checked_accumulate(
        cumulative_node_visit_count_, UINT64_C(1),
        "the Morton/Yao48 node-visit count overflows uint64");
    checked_accumulate(
        cumulative_leaf_visit_count_, UINT64_C(1),
        "the Morton/Yao48 leaf-visit count overflows uint64");
    checked_accumulate(
        cumulative_exact_cone_classification_count_, UINT64_C(1),
        "the Morton/Yao48 cone classification count overflows uint64");
    if (directional_cutoff_evaluated) {
      checked_accumulate(
          cumulative_directional_leaf_cutoff_evaluation_count_,
          UINT64_C(1),
          "the Morton/Yao48 directional cutoff count overflows uint64");
    }
    if (bank.count < required_witness_count) {
      checked_accumulate(
          cumulative_exact_witness_distance_evaluation_count_,
          UINT64_C(1),
          "the Morton/Yao48 witness-distance count overflows uint64");
      bank = std::move(updated_bank);
      ++active_retained_witness_count_;
      peak_active_retained_witness_count_ = std::max(
          peak_active_retained_witness_count_,
          active_retained_witness_count_);
      if (bank_fills) {
        ++active_full_witness_bank_count_;
        checked_accumulate(
            cumulative_frozen_witness_bank_count_, UINT64_C(1),
            "the Morton/Yao48 frozen-bank count overflows uint64");
        checked_accumulate(
            cumulative_witness_bank_receipt_count_, UINT64_C(1),
            "the Morton/Yao48 bank-receipt count overflows uint64");
      }
    }
    checked_accumulate(
        active_anchor_candidate_pair_mass_, UINT64_C(1),
        "the Morton/Yao48 active candidate mass overflows uint64");
    checked_accumulate(
        cumulative_candidate_pair_mass_, UINT64_C(1),
        "the Morton/Yao48 global candidate mass overflows uint64");
    finish_or_skip_subtree(1U);
  }

  result.status = MortonYao48PairFrontierStatus::complete;
  result.stop_reason = MortonYao48PairFrontierStopReason::none;
  result.audit = audit();
  if (!result.audit.frontier_complete) {
    throw std::logic_error(
        "a completed Morton/Yao48 traversal lacks its mass certificate");
  }
  return result;
}

}  // namespace morsehgp3d::gpu
