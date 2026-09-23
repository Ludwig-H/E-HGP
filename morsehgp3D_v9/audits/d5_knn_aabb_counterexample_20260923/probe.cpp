// Audit-only counterexample for the AABB node bound proposed by D5.
// Compile against the pinned v9 source tree; no product source is changed.
#include <cmath>
#include <cstdio>
#include <vector>

#include "morsehgp3D_v9/src/tower/pipeline/census.hpp"
#include "morsehgp3D_v9/src/tower/tree/cloud_index.hpp"

int main() {
  using namespace mhgp9::tower;
  constexpr i64 radius = 32044;
  constexpr i64 outer_radius = radius + 1;
  constexpr i64 cx = 100000, cy = 100000, cz = 100000;
  constexpr unsigned k = 5;
  std::vector<InputPoint> points;
  const auto add = [&](i64 x, i64 y, i64 z) {
    points.push_back({static_cast<PointId>(points.size()), {cx + x, cy + y, cz + z}});
  };

  // These two points force the MEB of the five-site facet to be this ball.
  add(-radius, 0, 0);
  add(radius, 0, 0);
  // Five strict interiors: p=5, q_min=2, so the window starts at K=6.
  add(0, radius - 1, 0);
  add(1, radius - 1, 0);
  add(-1, radius - 1, 0);
  add(0, radius - 1, 1);
  add(0, radius - 1, -1);
  const size_t core_size = points.size();

  // Exact lattice points on a slightly larger circle in the first quadrant.
  // Their coordinatewise minima lie well inside the smaller ball, although
  // every actual point lies outside. The radius has many representations as
  // a sum of two squares: 32045 = 5 * 13 * 17 * 29.
  for (i64 x = 1; x < outer_radius; ++x) {
    const i64 square = outer_radius * outer_radius - x * x;
    i64 y = static_cast<i64>(std::sqrt(static_cast<double>(square)));
    while ((y + 1) * (y + 1) <= square) ++y;
    while (y * y > square) --y;
    if (x > 5000 && y > 5000 && x * x + y * y == outer_radius * outer_radius) add(x, y, 0);
  }

  const BallKey ball{1, {-2 * cx, -2 * cy, -2 * cz}, 3 * cx * cx - radius * radius};
  const i128 threshold = (radius - 1) * (radius - 1) + 1 - radius * radius;
  // K=5 selects all five strict interiors; this is the final K-best power.
  // The initial seed includes both shell supports, so its threshold is 0.
  // Throughout any exact K-best query the live threshold is >= this final one.
  if (k != 5 || core_size != 7 || points.size() != 71 ||
      threshold != -64086 || ball.power(points[0].position) != 0 || ball.power(points[1].position) != 0)
    return 3;

  std::printf("{\"schema\":\"mhgp9_d5_knn_aabb_counterexample_v2\",\"u18_valid\":true,"
              "\"k\":5,\"p\":5,\"q_min\":2,\"window_lower_k\":6,\"cases\":[");
  bool first_case = true;
  for (const size_t exterior_count : {size_t{8}, size_t{16}, size_t{32}, size_t{64}}) {
    const std::vector<InputPoint> subset(points.begin(), points.begin() + core_size + exterior_count);
    const CloudIndex ix = build_cloud_index(subset);
    if (!ix.valid || ix.has_duplicate_positions()) return 2;
    size_t interior = 0, shell = 0, exterior = 0;
    for (const auto& point : subset) {
      const i128 power = ball.power(point.position);
      if (power < 0) ++interior;
      else if (power == 0) ++shell;
      else ++exterior;
    }
    if (interior != 5 || shell != 2 || exterior != exterior_count) return 3;

    const census_detail::AxisBounds bounds(ball);
    std::vector<NodeRef> stack{ix.root()};
    size_t visited_nodes = 0, visited_internal = 0, visited_leaves = 0, pruned_leaves = 0;
    bool all_internal_strict = true;
    while (!stack.empty()) {
      const NodeRef node = stack.back();
      stack.pop_back();
      ++visited_nodes;
      i128 lo = 0, hi = 0;
      bounds.bounds(ix.box_of(node), &lo, &hi);
      if (lo > threshold) {
        if (!is_leaf(node)) all_internal_strict = false;
        else ++pruned_leaves;
        continue;
      }
      if (is_leaf(node)) {
        ++visited_leaves;
      } else {
        ++visited_internal;
        if (lo >= threshold) all_internal_strict = false;
        stack.push_back(ix.nodes[static_cast<size_t>(node)].left);
        stack.push_back(ix.nodes[static_cast<size_t>(node)].right);
      }
    }
    // Every internal node survives even the *final*, strongest threshold.
    // The real near-first K-best query cannot avoid these node visits using
    // only AxisBounds and the stated threshold/tie pruning.
    if (!all_internal_strict || visited_nodes != 2 * subset.size() - 1 ||
        visited_internal != ix.nodes.size() || visited_leaves != 5 ||
        pruned_leaves != subset.size() - 5) return 4;
    std::printf("%s{\"points\":%zu,\"closed_sites\":%zu,\"exterior_sites\":%zu,"
                "\"threshold_power\":-64086,\"internal_nodes\":%zu,"
                "\"internal_surviving\":%zu,\"visited_nodes\":%zu,\"total_nodes\":%zu,"
                "\"visited_leaves\":%zu,\"pruned_leaves\":%zu}",
                first_case ? "" : ",", subset.size(), interior + shell, exterior, ix.nodes.size(),
                visited_internal, visited_nodes, ix.nodes.size() + subset.size(), visited_leaves, pruned_leaves);
    first_case = false;
  }
  std::printf("]}\n");
}
