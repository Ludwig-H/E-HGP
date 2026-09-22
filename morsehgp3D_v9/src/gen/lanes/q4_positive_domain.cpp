#include "lanes/q4_positive_domain.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace mhgp9::gen {
namespace {

struct DistanceBounds { i64 minimum{}, maximum{}; };

[[nodiscard]] i64 distance_squared(Point3 a, Point3 b) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = static_cast<i64>(a[axis]) - b[axis];
    result += delta * delta;
  }
  return result;
}

[[nodiscard]] DistanceBounds distance_bounds(const Box3& box, Point3 point) {
  DistanceBounds result;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 low = static_cast<i64>(box.low[axis]) - point[axis];
    const i64 high = static_cast<i64>(box.high[axis]) - point[axis];
    const i64 nearest = low > 0 ? low : (high < 0 ? high : 0);
    result.minimum += nearest * nearest;
    result.maximum += std::max(low * low, high * high);
  }
  return result;
}

[[nodiscard]] bool contains(const Box3& box, Point3 point) {
  return box.low.x <= point.x && point.x <= box.high.x &&
         box.low.y <= point.y && point.y <= box.high.y &&
         box.low.z <= point.z && point.z <= box.high.z;
}

[[nodiscard]] Box3 joined(const Box3& a, const Box3& b) {
  return {{std::min(a.low.x, b.low.x), std::min(a.low.y, b.low.y), std::min(a.low.z, b.low.z)},
          {std::max(a.high.x, b.high.x), std::max(a.high.y, b.high.y), std::max(a.high.z, b.high.z)}};
}

}  // namespace

Q4PositiveDomainPtr Q4PositiveDomain::make(Q34EdgeCoverPtr cover) {
  if (!cover) throw std::invalid_argument("mhgp9 gen positive domain requires an immutable edge cover");
  return Q4PositiveDomainPtr(new Q4PositiveDomain(std::move(cover)));
}

Q4PositiveDomain::Q4PositiveDomain(Q34EdgeCoverPtr cover) : cover_(std::move(cover)) {
  const auto points = cover_->index()->cloud().points();
  const auto ids = cover_->edge_ids();
  a_ = points[ids[0]];
  b_ = points[ids[1]];
  diameter_squared_ = distance_squared(a_, b_);
  // M=262143. Each coordinate difference is in [-M,M], and every distance
  // or extremal squared distance is <=3M^2<2^38. Promotion precedes every
  // subtraction/product; i64 is sufficient for this entire preparation.
  // A later projection can use |D*w_i-(w.v)*v_i|<=12M^3 and hull
  // orientations <=1152M^6<2^119, but MUST perform those products in i128.
  // No projected hull, dyadic denominator or depth bound is created here.
  build();
}

void Q4PositiveDomain::admit(const Box3& box, std::size_t population) {
  counter_add(work_.admitted_nodes);
  counter_add(work_.admitted_sites, static_cast<u64>(population));
  // Admitted populations are disjoint parts of the same index, so their
  // total is <= cloud size and addition cannot overflow size_t.
  population_ += population;
  if (box_) {
    *box_ = joined(*box_, box);
    counter_add(work_.box_merges);
  } else {
    box_ = box;
  }
}

void Q4PositiveDomain::reject(std::size_t population) {
  counter_add(work_.rejected_nodes);
  counter_add(work_.rejected_sites, static_cast<u64>(population));
}

void Q4PositiveDomain::build() {
  const auto& index = *cover_->index();
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  const auto points = index.cloud().points();
  const auto ids = cover_->edge_ids();
  std::size_t cursor = 0;
  while (cursor < nodes.size()) {
    const auto& node = nodes[cursor];
    counter_add(work_.node_visits);
    if (node.range.size() == 1) {
      counter_add(work_.endpoint_leaf_tests);
      const auto id = order[node.range.first];
      if (id == ids[0] || id == ids[1]) {
        counter_add(work_.excluded_endpoints);
      } else {
        counter_add(work_.point_tests);
        const auto point = points[id];
        const auto to_a = distance_squared(point, a_);
        const auto to_b = distance_squared(point, b_);
        if (to_a <= diameter_squared_ && to_b <= diameter_squared_)
          admit(singleton_box(point), 1);
        else
          reject(1);
      }
      cursor = node.escape;
      continue;
    }

    counter_add(work_.bound_tests);
    const auto to_a = distance_bounds(node.box, a_);
    const auto to_b = distance_bounds(node.box, b_);
    if (to_a.minimum > diameter_squared_ || to_b.minimum > diameter_squared_) {
      reject(node.range.size());
      cursor = node.escape;
      continue;
    }
    if (to_a.maximum <= diameter_squared_ && to_b.maximum <= diameter_squared_) {
      counter_add(work_.endpoint_box_tests, 2);
      const bool may_contain_a = contains(node.box, a_);
      const bool may_contain_b = contains(node.box, b_);
      if (!may_contain_a && !may_contain_b) {
        admit(node.box, node.range.size());
        cursor = node.escape;
        continue;
      }
      // Never include endpoint extrema in the AABB, nor subtract their
      // populations while retaining their box. Refine possible endpoint
      // nodes; false positives here only cost visits and cannot lose Z.
    }
    counter_add(work_.split_nodes);
    cursor = node.left;
  }
  // Global certified preorder/escape links partition all original sites:
  // admitted_sites + rejected_sites + excluded_endpoints == n, with two
  // excluded endpoints. Taking the union of the EXACT admitted node boxes
  // and singleton boxes therefore gives the exact AABB of Z. This visits
  // blocks, not every scalar point unless the geometry requires refinement.
}

}  // namespace mhgp9::gen
