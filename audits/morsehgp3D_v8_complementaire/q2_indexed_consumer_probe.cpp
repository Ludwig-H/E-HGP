#include "pipeline/axis_q2.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using Point = mhgp8::Point3;
using Box = mhgp8::Box3;
using i64 = std::int64_t;
using u64 = std::uint64_t;
using Clock = std::chrono::steady_clock;

void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}

struct Work {
  u64 construction_point_visits{}, nodes{}, max_depth{}, queries{};
  u64 visits{}, outside_nodes{}, inside_nodes{}, partial_nodes{}, credited{};
};

class Index {
 public:
  explicit Index(const mhgp8::PreparedRectangle& owner) : points_(owner.points()) {
    ids_.resize(points_.size());
    std::iota(ids_.begin(), ids_.end(), 0);
    nodes_.reserve(2 * points_.size() - 1);
    build(0, ids_.size(), 0);
  }

  unsigned count(Point a, Point b, unsigned limit, unsigned initial_count) {
    require(initial_count <= limit && limit > 0, "query threshold");
    ++work.queries;
    std::array<i64, 3> center2{};
    i64 radius4 = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
      center2[axis] = i64(a[axis]) + b[axis];
      const auto delta = i64(a[axis]) - b[axis];
      radius4 += delta * delta;
    }
    visit(0, center2, radius4, limit, initial_count);
    return initial_count;
  }

  Work work;

 private:
  struct Node {
    Box box;
    std::size_t first{}, last{}, left{}, right{};
  };
  std::span<const Point> points_;
  std::vector<std::size_t> ids_;
  std::vector<Node> nodes_;

  std::size_t build(std::size_t first, std::size_t last, u64 depth) {
    require(first < last, "empty index node");
    Box box{points_[ids_[first]], points_[ids_[first]]};
    for (std::size_t k = first; k < last; ++k) {
      const auto p = points_[ids_[k]];
      box.low = {std::min(box.low.x, p.x), std::min(box.low.y, p.y), std::min(box.low.z, p.z)};
      box.high = {std::max(box.high.x, p.x), std::max(box.high.y, p.y), std::max(box.high.z, p.z)};
      ++work.construction_point_visits;
    }
    const auto id = nodes_.size();
    nodes_.push_back({box, first, last, 0, 0});
    ++work.nodes;
    work.max_depth = std::max(work.max_depth, depth);
    if (last - first == 1) return id;
    unsigned axis = 0;
    for (unsigned candidate = 1; candidate < 3; ++candidate)
      if (box.high[candidate] - box.low[candidate] > box.high[axis] - box.low[axis])
        axis = candidate;
    const auto middle = (unsigned(box.low[axis]) + box.high[axis]) / 2;
    const auto cut = std::partition(ids_.begin() + static_cast<std::ptrdiff_t>(first),
                                    ids_.begin() + static_cast<std::ptrdiff_t>(last),
                                    [&](auto point_id) {
      ++work.construction_point_visits;
      return points_[point_id][axis] <= middle;
    });
    const auto split = static_cast<std::size_t>(cut - ids_.begin());
    require(first < split && split < last, "failed exact midpoint split");
    const auto left = build(first, split, depth + 1);
    const auto right = build(split, last, depth + 1);
    nodes_[id].left = left;
    nodes_[id].right = right;
    return id;
  }

  void visit(std::size_t id, const std::array<i64, 3>& center2, i64 radius4,
             unsigned limit, unsigned& count) {
    if (count == limit) return;
    ++work.visits;
    const auto& node = nodes_[id];
    i64 minimum = 0, maximum = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
      const i64 low = 2 * i64(node.box.low[axis]) - center2[axis];
      const i64 high = 2 * i64(node.box.high[axis]) - center2[axis];
      const i64 near = low > 0 ? low : high < 0 ? high : 0;
      minimum += near * near;
      maximum += std::max(low * low, high * high);
    }
    if (minimum >= radius4) {
      ++work.outside_nodes;
      return;
    }
    if (maximum < radius4) {
      ++work.inside_nodes;
      const auto contribution = std::min<std::size_t>(limit - count, node.last - node.first);
      count += static_cast<unsigned>(contribution);
      work.credited += contribution;
      return;
    }
    require(node.last - node.first > 1, "indecisive point node");
    ++work.partial_nodes;
    visit(node.left, center2, radius4, limit, count);
    visit(node.right, center2, radius4, limit, count);
  }
};

// Oracle independent of the midpoint/box machinery above.
unsigned depth(const mhgp8::RectangleInput& input, std::size_t a, std::size_t b) {
  unsigned result = 0;
  for (const auto z : input.points) {
    i64 h = 0;
    for (unsigned axis = 0; axis < 3; ++axis)
      h += (i64(z[axis]) - input.points[a][axis]) * (i64(input.points[b][axis]) - z[axis]);
    result += h > 0;
  }
  return result;
}

mhgp8::RectangleInput sheet(unsigned ny, unsigned nz) {
  mhgp8::RectangleInput input;
  for (unsigned side = 0; side < 2; ++side)
    for (unsigned y = 0; y < ny; ++y)
      for (unsigned z = 0; z < nz; ++z)
        input.points.push_back({static_cast<std::uint16_t>(1000 + 59000 * side),
                                static_cast<std::uint16_t>(1000 + y),
                                static_cast<std::uint16_t>(1000 + z)});
  const auto m = std::size_t(ny) * nz;
  input.a = {0, m};
  input.b = {m, 2 * m};
  return input;
}

void run(const char* label, const mhgp8::RectangleInput& input, unsigned h,
         unsigned separation, bool exhaustive) {
  const auto started = Clock::now();
  const auto owner = mhgp8::prepare_rectangle(input, h, separation);
  const auto prepared = Clock::now();
  const auto plan = mhgp8::make_axis_q2_plan(owner);
  const auto filtered = Clock::now();
  Index index(*owner);
  const auto indexed = Clock::now();
  std::array<u64, 11> histogram{};
  u64 candidates = 0, oracle_tests = 0, survivors = 0;
  plan.for_each_candidate([&](std::size_t a, std::size_t b) {
    // All-site index: no preloading of core or local credits.
    const unsigned p = index.count(owner->points()[a], owner->points()[b], h, 0);
    ++candidates;
    ++histogram[p];
    survivors += p < h;
    if (exhaustive) {
      const auto truth = depth(input, a, b);
      oracle_tests += input.points.size();
      require(p == std::min(h, truth), "indexed depth differs from independent census");
    }
  });
  const auto consumed = Clock::now();
  require(candidates == plan.candidate_pairs(), "physical candidate count");
  if (exhaustive) {
    u64 full_survivors = 0;
    for (std::size_t a = input.a.first; a < input.a.last; ++a)
      for (std::size_t b = input.b.first; b < input.b.last; ++b) {
        const auto truth = depth(input, a, b);
        oracle_tests += input.points.size();
        if (truth < h) {
          ++full_survivors;
          require(plan.keeps(a, b), "prefilter lost q2 survivor");
        }
      }
    require(full_survivors == survivors, "survivor mass mismatch");
  }
  const auto ms = [](auto a, auto b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
  std::cout << "{\"family\":\"" << label << "\",\"n\":" << input.points.size()
            << ",\"h\":" << h << ",\"s\":" << separation
            << ",\"exhaustive_oracle\":" << (exhaustive ? "true" : "false")
            << ",\"total_pairs\":" << plan.total_pairs() << ",\"candidates\":" << candidates
            << ",\"q2_pairs_below_threshold\":" << survivors
            << ",\"oracle_point_tests\":" << oracle_tests
            << ",\"index_build_point_visits\":" << index.work.construction_point_visits
            << ",\"index_nodes\":" << index.work.nodes << ",\"index_max_depth\":" << index.work.max_depth
            << ",\"query_node_visits\":" << index.work.visits
            << ",\"outside_nodes\":" << index.work.outside_nodes
            << ",\"inside_nodes\":" << index.work.inside_nodes
            << ",\"partial_nodes\":" << index.work.partial_nodes
            << ",\"credited_sites_capped\":" << index.work.credited
            << ",\"timing_scope\":\"audit_prototype_single_run_not_qualification\""
            << ",\"prepare_ms\":" << ms(started, prepared)
            << ",\"axis_ms\":" << ms(prepared, filtered)
            << ",\"index_ms\":" << ms(filtered, indexed)
            << ",\"consume_ms\":" << ms(indexed, consumed)
            << ",\"total_ms\":" << ms(started, consumed) << ",\"histogram\":[";
  for (unsigned k = 0; k <= h; ++k) {
    if (k) std::cout << ',';
    std::cout << histogram[k];
  }
  std::cout << "]}\n";
  std::cout.flush();
}

int selftest() {
  run("boundary", {{{0,0,0},{4,0,0}}, {0,1}, {1,2}, {}}, 1, 12, true);
  run("external_unproposed", {{{0,0,0},{50,0,0},{100,0,0}}, {0,1}, {2,3}, {}}, 1, 12, true);
  run("core_must_not_be_preloaded", {{{0,0,0},{50,0,0},{100,0,0}}, {0,1}, {2,3}, {1}}, 2, 12, true);
  run("all_box_corners_outside", {{{0,2,2},{4,2,2},{2,0,0},{2,4,4},{2,2,2}}, {0,1}, {1,2}, {}}, 2, 12, true);
  run("half_integer_center", {{{0,1,1},{3,2,2},{1,0,1},{2,3,2},{1,1,1}}, {0,1}, {1,2}, {}}, 2, 12, true);
  run("second_diameter_same_ball", {{{0,1,1},{3,2,2},{1,0,1},{2,3,2},{1,1,1}}, {2,3}, {3,4}, {}}, 2, 12, true);
  run("u16_extremes", {{{0,0,0},{65535,65535,65535},{32767,32767,32767}}, {0,1}, {1,2}, {}}, 2, 12, true);
  for (const auto shape : {std::array{4U, 4U}, std::array{5U, 7U}, std::array{8U, 8U}})
    for (unsigned h : {1U, 5U, 10U}) run("small_sheet", sheet(shape[0], shape[1]), h, 12, true);
  return 0;
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string_view(argv[1]) == "--selftest") return selftest();
    if (argc == 2 && std::string_view(argv[1]) == "--large") {
      for (const auto shape : {std::array{50U, 80U}, std::array{100U, 80U}, std::array{125U, 128U}})
        for (unsigned h : {5U, 10U}) run("sheet_full", sheet(shape[0], shape[1]), h, 12, false);
      return 0;
    }
    std::cerr << "usage: q2_indexed_consumer_probe --selftest|--large\n";
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "indexed q2 consumer failed: " << error.what() << '\n';
    return 1;
  }
}
