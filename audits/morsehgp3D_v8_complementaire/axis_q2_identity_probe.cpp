// Independent bounded q2 judge: direct point powers and distinct witness IDs.
// No production predicate, spatial index or column sort is used by the oracle.
#include "pipeline/axis_q2.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using Point = mhgp8::Point3;
using Signature = std::set<std::array<std::uint16_t, 6>>;

void require(bool value, const char* cause) {
  if (!value) throw std::runtime_error(cause);
}

std::int64_t h(const Point& a, const Point& b, const Point& z) {
  std::int64_t result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto left = static_cast<std::int64_t>(z[axis]) - a[axis];
    const auto right = static_cast<std::int64_t>(b[axis]) - z[axis];
    result += left * right;
  }
  // Every term has magnitude <=65535^2; the signed sum fits int64.
  return result;
}

std::array<Point, 8> corners(const mhgp8::RectangleInput& input, mhgp8::Range range) {
  std::array<std::uint16_t, 3> low{65535, 65535, 65535}, high{};
  for (auto i = range.first; i < range.last; ++i) {
    for (std::size_t axis = 0; axis < 3; ++axis) {
      low[axis] = std::min(low[axis], input.points[i][axis]);
      high[axis] = std::max(high[axis], input.points[i][axis]);
    }
  }
  std::array<Point, 8> result{};
  for (unsigned i = 0; i < 8; ++i) {
    result[i] = {(i & 1U) != 0 ? high[0] : low[0],
                 (i & 2U) != 0 ? high[1] : low[1],
                 (i & 4U) != 0 ? high[2] : low[2]};
  }
  return result;
}

struct Checks {
  std::uint64_t plans{}, pairs{}, census_point_tests{}, accepted{}, rejected{};
  std::uint64_t additive_opportunities{}, index_plans{}, inactive_plans{}, core_positive_plans{};
  std::uint64_t boundary_witness_tests{}, permutation_pairs{};
};

Signature check(const mhgp8::RectangleInput& input, unsigned kmax,
                unsigned separation, Checks& checks) {
  const auto owner = mhgp8::prepare_rectangle(input, kmax, separation);
  const auto plan = mhgp8::make_axis_q2_plan(owner);
  unsigned core = 0;
  for (const auto zid : input.core_candidates) {
    bool universal = true;
    for (const auto& a : corners(input, input.a)) {
      for (const auto& b : corners(input, input.b)) {
        universal = universal && h(a, b, input.points[zid]) > 0;
      }
    }
    core += universal ? 1U : 0U;
  }
  core = std::min(core, kmax);
  require(plan.need() == kmax - core, "wrong threshold/core transfer");
  ++checks.plans;
  checks.core_positive_plans += core > 0 ? 1U : 0U;
  checks.index_plans += plan.work().tree_nodes > 0 ? 1U : 0U;
  checks.inactive_plans += plan.need() == 0 ? 1U : 0U;
  std::vector<unsigned> expanded(input.a.size() * input.b.size(), 0);
  plan.for_each_candidate([&](std::size_t a, std::size_t b) {
    require(input.a.first <= a && a < input.a.last && input.b.first <= b && b < input.b.last,
            "expansion has wrong original IDs");
    auto& count = expanded[(a - input.a.first) * input.b.size() + b - input.b.first];
    require(++count == 1, "duplicate expanded pair");
  });
  Signature signature;
  std::uint64_t mass = 0;
  for (const auto& block : plan.blocks()) {
    require(input.a.first <= block.a_id && block.a_id < input.a.last &&
            block.b.first < block.b.last && block.b.last <= plan.b_order().size(),
            "invalid index descriptor");
    mass += block.b.size();
  }
  require(mass == plan.candidate_pairs(), "descriptor cardinality");
  for (auto aid = input.a.first; aid < input.a.last; ++aid) {
    const auto& a = input.points[aid];
    for (auto bid = input.b.first; bid < input.b.last; ++bid) {
      const auto& b = input.points[bid];
      std::array<unsigned, 3> axis_counts{};
      std::set<std::size_t> axial_ids;
      for (auto zid = input.a.first; zid < input.a.last; ++zid) {
        const auto& z = input.points[zid];
        for (std::size_t axis = 0; axis < 3; ++axis) {
          bool column = true;
          for (std::size_t other = 0; other < 3; ++other) {
            if (other != axis && a[other] != z[other]) column = false;
          }
          if (!column || zid == aid) continue;
          const auto value = h(a, b, z);
          checks.boundary_witness_tests += value == 0 ? 1U : 0U;
          if (value > 0) {
            ++axis_counts[axis];
            require(axial_ids.insert(zid).second, "two exact axes share a non-self witness");
          }
        }
      }
      const auto maximum = *std::max_element(axis_counts.begin(), axis_counts.end());
      const auto summed = axis_counts[0] + axis_counts[1] + axis_counts[2];
      require(summed == axial_ids.size(), "axis sum does not count distinct identities");
      const bool expected = core + maximum < kmax;
      require(plan.keeps(aid, bid) == expected &&
              (expanded[(aid - input.a.first) * input.b.size() + bid - input.b.first] != 0) == expected,
              "plan/index differs from direct halfspace witnesses");
      unsigned depth = 0;
      for (const auto& z : input.points) {
        depth += h(a, b, z) > 0 ? 1U : 0U;
        ++checks.census_point_tests;
      }
      require(depth >= core + summed, "core plus disjoint axial sum exceeds actual depth");
      if (!expected) require(depth >= kmax, "unsafe q2 rejection");
      checks.additive_opportunities += expected && core + summed >= kmax ? 1U : 0U;
      checks.accepted += expected ? 1U : 0U;
      checks.rejected += expected ? 0U : 1U;
      ++checks.pairs;
      if (expected) signature.insert({a.x, a.y, a.z, b.x, b.y, b.z});
    }
  }
  require(signature.size() == plan.candidate_pairs(), "physical signature cardinality");
  if (plan.need() == 0) {
    require(plan.blocks().empty() && plan.b_order().empty() && plan.work().sort_passes == 0 &&
            plan.work().tree_nodes == 0 && plan.work().query_nodes == 0,
            "inactive plan paid unused preparation");
  }
  return signature;
}

std::uint32_t next(std::uint32_t& state) {
  state = state * 1664525U + 1013904223U;
  return state;
}

Point transform(Point point, unsigned seed) {
  std::array<std::uint16_t, 3> values{point.x, point.y, point.z};
  if ((seed & 4U) != 0) {
    for (auto& value : values) value = static_cast<std::uint16_t>(65535 - value);
  }
  const auto shift = seed % 3;
  return {values[shift], values[(shift + 1) % 3], values[(shift + 2) % 3]};
}

mhgp8::RectangleInput varied(unsigned seed, bool reordered) {
  std::uint32_t state = 0x5A17U + seed;
  std::vector<Point> a, b, extra;
  constexpr std::array<unsigned, 5> coordinates{0, 1, 4, 10, 31};
  std::set<std::array<unsigned, 3>> used;
  const auto count_a = 8 + seed % 17;
  const auto count_b = 1 + (seed * 7) % 24;
  while (a.size() < count_a) {
    const auto x = seed % 3 == 0 ? next(state) % 3 : 0;
    const auto y = coordinates[next(state) % 5];
    const auto z = coordinates[next(state) % 5];
    if (used.insert({x, y, z}).second) {
      a.push_back({static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                   static_cast<std::uint16_t>(z)});
    }
  }
  while (b.size() < count_b) {
    const auto x = 60000 + next(state) % 3;
    const auto y = next(state) % 33;
    const auto z = next(state) % 33;
    if (used.insert({x, y, z}).second) {
      b.push_back({static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                   static_cast<std::uint16_t>(z)});
    }
  }
  for (unsigned i = 0; i < seed % 4; ++i) {
    extra.push_back({static_cast<std::uint16_t>(30000 + i), 16, 16});
  }
  extra.push_back({30000, 65535, 65535});  // Proposed but not universal.
  extra.push_back({29900, 17, 17});       // Real input, deliberately not proposed.
  mhgp8::RectangleInput result;
  const auto append = [&](const std::vector<Point>& points) {
    const auto first = result.points.size();
    for (const auto point : points) result.points.push_back(transform(point, seed));
    return mhgp8::Range{first, result.points.size()};
  };
  if (reordered) {
    std::reverse(a.begin(), a.end());
    std::reverse(b.begin(), b.end());
    result.b = append(b);
  }
  const auto external = append(extra);
  for (auto id = external.first; id + 1 < external.last; ++id) result.core_candidates.push_back(id);
  result.a = append(a);
  if (!reordered) result.b = append(b);
  return result;
}

void run() {
  Checks checks;
  const mhgp8::RectangleInput crossing{
      {{0, 0, 0}, {0, 1, 0}, {0, 0, 1}, {100, 2, 2}}, {0, 3}, {3, 4}, {}};
  static_cast<void>(check(crossing, 2, 12, checks));
  require(checks.additive_opportunities == 1, "minimal additive improvement missing");
  const auto crossing_plan = mhgp8::make_axis_q2_plan(mhgp8::prepare_rectangle(crossing, 2, 12));
  require(crossing_plan.keeps(0, 3) && h(crossing.points[0], crossing.points[3], crossing.points[1]) == 1 &&
          h(crossing.points[0], crossing.points[3], crossing.points[2]) == 1,
          "minimal fixture changed");
  for (unsigned seed = 0; seed < 64; ++seed) {
    const unsigned kmax = 1 + seed % 5;
    const unsigned separation = std::array<unsigned, 3>{8, 10, 12}[seed % 3];
    require(check(varied(seed, false), kmax, separation, checks) ==
            check(varied(seed, true), kmax, separation, checks),
            "permuting points/range layout changed physical residual");
    ++checks.permutation_pairs;
  }
  const mhgp8::RectangleInput boundary{
      {{0, 0, 0}, {0, 1, 0}, {0, 4, 0}, {100, 1, 0}, {100, 2, 0}, {100, 4, 0}, {100, 5, 0}},
      {0, 3}, {3, 7}, {}};
  static_cast<void>(check(boundary, 1, 12, checks));
  static_cast<void>(check(boundary, 2, 12, checks));
  require(checks.plans == 131 && checks.permutation_pairs == 64 && checks.pairs > 10000 &&
          checks.accepted > 1000 && checks.rejected > 1000 && checks.index_plans > 0 &&
          checks.inactive_plans > 0 && checks.core_positive_plans > 0 &&
          checks.boundary_witness_tests > 0 && checks.additive_opportunities > 1,
          "campaign nonvacuity");
  std::cout << "{\"status\":\"passed\",\"scope\":\"independent_bounded_axis_q2_judge\""
            << ",\"plans\":" << checks.plans << ",\"pairs\":" << checks.pairs
            << ",\"census_point_tests\":" << checks.census_point_tests
            << ",\"accepted\":" << checks.accepted << ",\"rejected\":" << checks.rejected
            << ",\"additive_opportunities\":" << checks.additive_opportunities
            << ",\"index_plans\":" << checks.index_plans
            << ",\"inactive_plans\":" << checks.inactive_plans
            << ",\"core_positive_plans\":" << checks.core_positive_plans
            << ",\"boundary_witness_tests\":" << checks.boundary_witness_tests
            << ",\"permutation_pairs\":" << checks.permutation_pairs
            << ",\"minimal_current_candidates\":3,\"minimal_additive_candidates\":2}\n";
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try {
    run();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "axis q2 independent judge: " << error.what() << '\n';
    return 1;
  }
}
