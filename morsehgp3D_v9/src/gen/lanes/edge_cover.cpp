#include "lanes/edge_cover.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace mhgp9::gen {

Q34EdgeCoverPtr Q34EdgeCover::make(
    Q2CensusIndexPtr index, std::array<std::size_t, 2> edge_ids) {
  return make_ball(std::move(index), edge_ids, false);
}

Q34EdgeCoverPtr Q34EdgeCover::make_diametral(
    Q2CensusIndexPtr index, std::array<std::size_t, 2> edge_ids) {
  return make_ball(std::move(index), edge_ids, true);
}

void require_complete_q34_cover(const Q34EdgeCoverPtr& cover) {
  if (cover && !cover->complete())
    throw std::invalid_argument("mhgp9 gen consumer requires the complete edge cover, not its diametral core");
}

Q34EdgeCoverPtr Q34EdgeCover::make_ball(
    Q2CensusIndexPtr index, std::array<std::size_t, 2> edge_ids, bool diametral) {
  if (!index) throw std::invalid_argument("mhgp9 gen edge cover requires an immutable index");
  const auto size = index->cloud().points().size();
  if (edge_ids[0] >= size || edge_ids[1] >= size)
    throw std::out_of_range("mhgp9 gen edge cover ID outside cloud");
  if (edge_ids[0] == edge_ids[1])
    throw std::invalid_argument("mhgp9 gen edge cover requires distinct edge IDs");
  if (edge_ids[1] < edge_ids[0]) std::swap(edge_ids[0], edge_ids[1]);
  return Q34EdgeCoverPtr(new Q34EdgeCover(std::move(index), edge_ids, diametral));
}

Q34EdgeCover::Q34EdgeCover(Q2CensusIndexPtr index, std::array<std::size_t, 2> edge_ids, bool diametral)
    : index_(std::move(index)), edge_ids_(edge_ids), complete_(!diametral) {
  const auto points = index_->cloud().points();
  const auto a = points[edge_ids_[0]], b = points[edge_ids_[1]];
  for (std::size_t axis = 0; axis != 3; ++axis) {
    center_twice_[axis] = static_cast<i64>(a[axis]) + b[axis];
    const i64 delta = static_cast<i64>(b[axis]) - a[axis];
    radius_fourfold_ += (diametral ? 1 : 4) * delta * delta;
  }
  // For 18-bit coordinates, M=262143: each doubled displacement is in
  // [-2M,2M], so both its squared 3-norm and 4*|b-a|^2 are <=12M^2<2^40.
  // All operands are promoted before subtraction, doubling and products;
  // every bound, point predicate and intermediate therefore fits signed i64.
  build();
}

bool Q34EdgeCover::contains_point(Point3 point) const noexcept {
  i64 norm = 0;
  for (std::size_t axis = 0; axis != 3; ++axis) {
    const i64 delta = 2 * static_cast<i64>(point[axis]) - center_twice_[axis];
    norm += delta * delta;
  }
  return norm <= radius_fourfold_;
}

bool Q34EdgeCover::contains_id(std::size_t id) const {
  const auto points = index_->cloud().points();
  if (id >= points.size()) throw std::out_of_range("mhgp9 gen edge cover ID outside cloud");
  return contains_point(points[id]);
}

void Q34EdgeCover::admit(Range range) {
  counter_add(work_.admitted_nodes);
  counter_add(work_.admitted_sites, static_cast<u64>(range.size()));
  // Consumed node populations are disjoint portions of this cloud, so the
  // sum is <= points().size() and cannot overflow size_t.
  site_count_ += range.size();
  if (!ranges_.empty() && ranges_.back().last == range.first) {
    ranges_.back().last = range.last;
    counter_add(work_.merged_ranges);
  } else {
    ranges_.push_back(range);
    counter_add(work_.retained_ranges);
  }
}

void Q34EdgeCover::reject(Range range) {
  counter_add(work_.rejected_nodes);
  counter_add(work_.rejected_sites, static_cast<u64>(range.size()));
}

void Q34EdgeCover::build() {
  const auto nodes = index_->spatial_nodes();
  const auto order = index_->spatial_order();
  const auto points = index_->cloud().points();
  std::size_t cursor = 0;
  while (cursor < nodes.size()) {
    const auto& node = nodes[cursor];
    counter_add(work_.node_visits);
    if (node.range.size() == 1) {
      counter_add(work_.point_tests);
      if (contains_point(points[order[node.range.first]])) admit(node.range);
      else reject(node.range);
      cursor = node.escape;
      continue;
    }

    counter_add(work_.bound_tests);
    i64 minimum = 0, maximum = 0;
    for (std::size_t axis = 0; axis != 3; ++axis) {
      const i64 low = 2 * static_cast<i64>(node.box.low[axis]) - center_twice_[axis];
      const i64 high = 2 * static_cast<i64>(node.box.high[axis]) - center_twice_[axis];
      // Exact extrema of t^2 on a closed interval; summing the independent
      // coordinate extrema gives the exact squared-norm extrema of the box.
      const i64 nearest = low > 0 ? low : (high < 0 ? high : 0);
      minimum += nearest * nearest;
      maximum += std::max(low * low, high * high);
    }
    if (minimum > radius_fourfold_) {
      reject(node.range);
      cursor = node.escape;
    } else if (maximum <= radius_fourfold_) {
      admit(node.range);
      cursor = node.escape;
    } else {
      counter_add(work_.split_nodes);
      cursor = node.left;
    }
  }
  // The index certifies preorder children, disjoint child ranges and escape
  // links. Descending left or consuming a whole node therefore partitions
  // all ranks exactly once, in increasing spatial order, without a stack.
  // Work is O(visited nodes), at most O(n), memory O(retained ranges); these
  // bounds concern ONE edge only, not the sum over edges or seeds.
}

std::size_t Q34EdgeCover::retained_bytes() const {
  if (ranges_.capacity() > std::numeric_limits<std::size_t>::max() / sizeof(Range))
    throw std::overflow_error("mhgp9 gen edge cover storage exceeds size_t");
  return ranges_.capacity() * sizeof(Range);
}

}  // namespace mhgp9::gen
