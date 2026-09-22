#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../oracle/p0_oracle.hpp"
#include "pipeline/axis_q2.hpp"

namespace {

using mhgp8::Box3;
using mhgp8::Lane;
using mhgp8::Point3;
using mhgp8::Range;
using mhgp8::RectangleInput;
namespace oracle = mhgp8::oracle;
using Key = std::array<mhgp8::Coordinate, 3>;

struct Gate {
  std::uint64_t checks{};
  std::uint64_t small_plans{};
  std::uint64_t exact_census_pairs{};
  std::uint64_t rejected_pairs{};
  std::uint64_t retained_pairs{};
  std::uint64_t permutations{};
  std::uint64_t model_mutants{};
  std::uint64_t conservative_cases{};
  std::uint64_t invalid_inputs{};
  std::uint64_t moved_rejections{};
  std::uint64_t positive_queries{};
  std::uint64_t whole_accepts{};
  std::uint64_t whole_rejects{};
  std::uint64_t large_grid_points{};
  std::uint64_t large_grid_candidates{};

  void require(bool condition, const std::string& cause) {
    ++checks;
    if (!condition) throw std::runtime_error(cause);
  }

  template <class Function>
  void rejects(Function&& function, const std::string& cause) {
    bool refused = false;
    try { function(); } catch (const std::invalid_argument&) { refused = true; }
    require(refused, cause);
    ++invalid_inputs;
  }
};

[[nodiscard]] Key key(const Point3& point) { return {point.x, point.y, point.z}; }

[[nodiscard]] Box3 direct_bounds(const RectangleInput& input, std::size_t aid,
                                  unsigned need) {
  const Point3& a = input.points[aid];
  std::array<mhgp8::Coordinate, 3> lower{0, 0, 0};
  std::array<mhgp8::Coordinate, 3> upper{65535, 65535, 65535};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    std::vector<std::uint16_t> below;
    std::vector<std::uint16_t> above;
    for (std::size_t zid = input.a.first; zid < input.a.last; ++zid) {
      const Point3& z = input.points[zid];
      bool same_column = true;
      for (std::size_t other = 0; other < 3; ++other) {
        if (other != axis && z[other] != a[other]) same_column = false;
      }
      if (!same_column) continue;
      if (z[axis] < a[axis]) below.push_back(z[axis]);
      if (z[axis] > a[axis]) above.push_back(z[axis]);
    }
    std::sort(below.begin(), below.end(), std::greater<>());
    std::sort(above.begin(), above.end());
    if (need > 0 && below.size() >= need) lower[axis] = below[need - 1];
    if (need > 0 && above.size() >= need) upper[axis] = above[need - 1];
  }
  return {{lower[0], lower[1], lower[2]}, {upper[0], upper[1], upper[2]}};
}

[[nodiscard]] bool contains(const Box3& box, const Point3& point) {
  for (std::size_t axis = 0; axis < 3; ++axis) {
    if (point[axis] < box.low[axis] || point[axis] > box.high[axis]) return false;
  }
  return true;
}

struct Signature {
  std::set<std::pair<Key, Key>> residual;
  std::set<std::pair<Key, Key>> final_q2;
  bool operator==(const Signature&) const = default;
};

[[nodiscard]] Signature check_small(Gate& gate, const RectangleInput& input,
                                     unsigned kmax, unsigned separation) {
  const auto owner = mhgp8::prepare_rectangle(input, kmax, separation);
  const auto plan = mhgp8::make_axis_q2_plan(owner);
  ++gate.small_plans;
  const auto a_box = oracle::bounds(input.points, input.a.first, input.a.last);
  const auto b_box = oracle::bounds(input.points, input.b.first, input.b.last);
  const unsigned core = oracle::core_credit(input.points, input.core_candidates,
                                              a_box, b_box, Lane::Q2, kmax);
  const unsigned need = kmax - core;
  gate.require(plan.need() == need && &plan.rectangle() == owner.get() &&
                   plan.total_pairs() == input.a.size() * input.b.size(),
                "axial plan changed the certified owner or q2 threshold");
  std::set<std::pair<std::size_t, std::size_t>> expanded;
  std::uint64_t descriptor_mass = 0;
  for (const auto& block : plan.blocks()) {
    gate.require(input.a.first <= block.a_id && block.a_id < input.a.last &&
                     block.b.first < block.b.last && block.b.last <= plan.b_order().size(),
                  "invalid axial residual descriptor");
    descriptor_mass += block.b.size();
  }
  plan.for_each_candidate([&](std::size_t a, std::size_t b) {
    gate.require(input.a.first <= a && a < input.a.last &&
                     input.b.first <= b && b < input.b.last,
                  "axial expansion changed original point IDs");
    gate.require(expanded.emplace(a, b).second, "axial descriptors overlap for one pair");
  });
  gate.require(expanded.size() == plan.candidate_pairs() &&
                   descriptor_mass == plan.candidate_pairs(),
                "axial descriptors disagree with their mass or expansion");
  Signature signature;
  std::set<std::pair<Key, Key>> filtered_final;
  for (std::size_t a = input.a.first; a < input.a.last; ++a) {
    const auto allowed = direct_bounds(input, a, need);
    for (std::size_t b = input.b.first; b < input.b.last; ++b) {
      const bool keep = need > 0 && contains(allowed, input.points[b]);
      gate.require(plan.keeps(a, b) == keep && expanded.contains({a, b}) == keep,
                    "axial keeps/expansion differs from direct exact-column model");
      // All sites, including unproposed exterior sites, enter this independent
      // cpp_int census. It is a small q2 judge, not a FULL producer.
      const unsigned depth = oracle::point_credit(input.points, a, b, Lane::Q2, kmax);
      ++gate.exact_census_pairs;
      const auto physical = std::pair{key(input.points[a]), key(input.points[b])};
      if (depth < kmax) signature.final_q2.insert(physical);
      if (keep) {
        ++gate.retained_pairs;
        signature.residual.insert(physical);
        if (depth < kmax) filtered_final.insert(physical);
      } else {
        gate.require(depth >= kmax, "axial rejection lacks kmax distinct exact q2 witnesses");
        ++gate.rejected_pairs;
      }
    }
  }
  gate.require(filtered_final == signature.final_q2,
                "axial filter lost a genuinely shallow q2 pair");
  const auto& work = plan.work();
  gate.require(work.emitted_blocks == plan.blocks().size(), "axial descriptor work not counted");
  if (need == 0) {
    gate.require(plan.candidate_pairs() == 0 && work.sort_passes == 0 &&
                     work.sorted_sites == 0 && work.tree_nodes == 0 && work.query_nodes == 0,
                  "core-saturated axial plan performed unused preparation");
  }
  gate.positive_queries += work.query_nodes;
  gate.whole_accepts += work.whole_factor_accepts;
  gate.whole_rejects += work.whole_factor_rejects;
  return signature;
}

[[nodiscard]] RectangleInput grid(unsigned columns, unsigned rows,
                                   std::size_t count = 0) {
  RectangleInput input;
  if (count == 0) count = static_cast<std::size_t>(columns) * rows;
  input.a = {0, count};
  input.b = {count, 2 * count};
  for (unsigned side = 0; side < 2; ++side) {
    for (std::size_t i = 0; i < count; ++i) {
      input.points.push_back({static_cast<std::uint16_t>(side == 0 ? 1000 : 60000),
                               static_cast<std::uint16_t>(1000 + i % columns),
                               static_cast<std::uint16_t>(1000 + i / columns)});
    }
  }
  return input;
}

void ordinary_fixtures(Gate& gate) {
  for (unsigned orientation = 0; orientation < 6; ++orientation) {
    auto input = grid(5, 4);
    for (auto& point : input.points) {
      if (orientation % 3 == 1) std::swap(point.x, point.y);
      if (orientation % 3 == 2) std::swap(point.x, point.z);
      if (orientation >= 3) {
        point.x = static_cast<std::uint16_t>(65535 - point.x);
        point.y = static_cast<std::uint16_t>(65535 - point.y);
      }
    }
    auto permuted = input;
    std::reverse(permuted.points.begin(), permuted.points.begin() + 20);
    std::rotate(permuted.points.begin() + 20, permuted.points.begin() + 27,
                 permuted.points.end());
    for (const auto kmax : {1U, 2U, 5U, 10U}) {
      for (const auto separation : {8U, 10U, 12U}) {
        gate.require(check_small(gate, input, kmax, separation) ==
                         check_small(gate, permuted, kmax, separation),
                      "axial physical result changed under input permutation");
        ++gate.permutations;
      }
    }
  }
  auto with_core = grid(4, 3);
  for (unsigned i = 0; i < 10; ++i) {
    with_core.core_candidates.push_back(with_core.points.size());
    with_core.points.push_back({static_cast<std::uint16_t>(30000 + i), 1000, 1000});
  }
  for (const unsigned count : {0U, 1U, 3U, 10U}) {
    auto input = with_core;
    input.core_candidates.resize(count);
    static_cast<void>(check_small(gate, input, 5, 8));
  }
  // No two A sites share a complete axis column. No slabs are inferred;
  // the full B factor is accepted without a spatial index traversal.
  RectangleInput slanted;
  for (unsigned side = 0; side < 2; ++side) {
    for (unsigned i = 0; i < 8; ++i) {
      slanted.points.push_back({static_cast<std::uint16_t>(side == 0 ? 1000 : 60000),
                                  static_cast<std::uint16_t>(1000 + i),
                                  static_cast<std::uint16_t>(1000 + 2 * i)});
    }
  }
  slanted.a = {0, 8};
  slanted.b = {8, 16};
  static_cast<void>(check_small(gate, slanted, 1, 8));
  const auto unaligned = mhgp8::make_axis_q2_plan(mhgp8::prepare_rectangle(slanted, 1, 8));
  gate.require(unaligned.candidate_pairs() == 64 && unaligned.work().whole_factor_accepts == 8 &&
                   unaligned.work().tree_nodes == 0 && unaligned.work().query_nodes == 0,
                "unhelpful columns did not keep one compact whole-factor block per anchor");
  const RectangleInput singleton{{{0, 0, 0}, {65535, 0, 0}}, {0, 1}, {1, 2}, {}};
  static_cast<void>(check_small(gate, singleton, 1, 8));
  const RectangleInput unequal{
      {{0, 0, 0}, {0, 1, 0}, {0, 2, 0}, {0, 3, 0}, {0, 4, 0},
       {100, 10, 0}, {100, 11, 0}, {50, 500, 500}}, {0, 5}, {5, 7}, {7}};
  static_cast<void>(check_small(gate, unequal, 1, 8));
  auto offset = grid(3, 4);
  offset.points.insert(offset.points.begin(), {0, 65535, 65535});
  offset.a = {1, 13};
  offset.b = {13, 25};
  static_cast<void>(check_small(gate, offset, 2, 8));
  std::swap(offset.a, offset.b);
  static_cast<void>(check_small(gate, offset, 2, 8));
}

void counter_fixtures(Gate& gate) {
  const RectangleInput boundary{
      {{0, 0, 0}, {0, 1, 0}, {100, 1, 0}, {100, 2, 0}}, {0, 2}, {2, 4}, {}};
  static_cast<void>(check_small(gate, boundary, 1, 8));
  const auto exact = mhgp8::make_axis_q2_plan(mhgp8::prepare_rectangle(boundary, 1, 8));
  gate.require(exact.keeps(0, 2) && !exact.keeps(0, 3) &&
                   oracle::point_credit(boundary.points, 0, 2, Lane::Q2, 1) == 0,
                "strict tail boundary control failed");
  const bool closed_tail_mutant = boundary.points[2].y >= boundary.points[1].y;
  gate.require(closed_tail_mutant && exact.keeps(0, 2),
                "closed-tail mutation was not refuted by a real boundary pair");
  ++gate.model_mutants;

  const RectangleInput approximate{
      {{0, 0, 0}, {0, 1, 1}, {100, 2, 0}}, {0, 2}, {2, 3}, {}};
  static_cast<void>(check_small(gate, approximate, 1, 8));
  const auto no_fake_column = mhgp8::make_axis_q2_plan(
      mhgp8::prepare_rectangle(approximate, 1, 8));
  const bool ignored_other_coordinate = approximate.points[0].x == approximate.points[1].x &&
      approximate.points[2].y > approximate.points[1].y;
  gate.require(ignored_other_coordinate && no_fake_column.keeps(0, 2) &&
                   oracle::metrics(approximate.points[0], approximate.points[2],
                                     approximate.points[1]).h == 0,
                "approximate-column mutation was not refuted");
  ++gate.model_mutants;

  const RectangleInput irregular{
      {{1000, 0, 0}, {1000, 2, 0}, {1000, 100, 0},
       {60000, 99, 0}, {60000, 101, 0}}, {0, 3}, {3, 5}, {}};
  static_cast<void>(check_small(gate, irregular, 2, 8));
  const auto real_coordinate = mhgp8::make_axis_q2_plan(
      mhgp8::prepare_rectangle(irregular, 2, 8));
  gate.require(real_coordinate.keeps(0, 3) && !real_coordinate.keeps(0, 4) &&
                   irregular.points[3].y > 2 &&
                   oracle::point_credit(irregular.points, 0, 3, Lane::Q2, 2) == 1,
                "rank-instead-of-coordinate mutation was not refuted");
  ++gate.model_mutants;
  const RectangleInput too_short{
      {{0, 0, 0}, {0, 1, 0}, {100, 2, 0}}, {0, 2}, {2, 3}, {}};
  static_cast<void>(check_small(gate, too_short, 2, 8));
  const auto insufficient = mhgp8::make_axis_q2_plan(
      mhgp8::prepare_rectangle(too_short, 2, 8));
  gate.require(insufficient.keeps(0, 2) &&
                   oracle::point_credit(too_short.points, 0, 2, Lane::Q2, 2) == 1,
                "short-column witnesses were inflated to the requested threshold");
  ++gate.model_mutants;

  // Explicit port of the four-site counter-fixture in the independent audit
  // P0_SOUS_RECTANGLES_ET_GROUPES.md, section 8. Exact coordinate columns
  // meet only at the excluded anchor: their witnesses could be added safely.
  // This gate records the present isolated-axis filter's conservatism, not
  // a requirement for a future additive filter or a production-code mutant.
  const RectangleInput separate_axes{
      {{1000, 1000, 1000}, {1000, 1001, 1000}, {1000, 1000, 1001},
       {60000, 1002, 1002}}, {0, 3}, {3, 4}, {}};
  static_cast<void>(check_small(gate, separate_axes, 2, 12));
  const auto isolated = mhgp8::make_axis_q2_plan(
      mhgp8::prepare_rectangle(separate_axes, 2, 12));
  gate.require(oracle::metrics(separate_axes.points[0], separate_axes.points[3],
                                separate_axes.points[1]).h == 1 &&
                   oracle::metrics(separate_axes.points[0], separate_axes.points[3],
                                     separate_axes.points[2]).h == 1 &&
                   oracle::point_credit(separate_axes.points, 0, 3, Lane::Q2, 3) == 2,
                "disjoint exact columns no longer supply two strict q2 witnesses");
  gate.require(isolated.need() == 2 && isolated.keeps(0, 3) &&
                   isolated.candidate_pairs() == 3,
                "isolated-axis conservative fixture no longer retains the deep pair");
  ++gate.conservative_cases;
}

[[nodiscard]] std::uint64_t band_mass(std::uint64_t size, std::uint64_t h) {
  return h >= size ? size * size : (2 * h + 1) * size - h * (h + 1);
}

void large_grids(Gate& gate) {
  constexpr unsigned h = 10;
  for (const auto& dimensions : {std::pair{50U, 80U}, std::pair{80U, 100U},
                                  std::pair{100U, 160U}}) {
    const auto input = grid(dimensions.first, dimensions.second);
    const auto owner = mhgp8::prepare_rectangle(input, h, 12);
    const auto plan = mhgp8::make_axis_q2_plan(owner);
    const auto expected = band_mass(dimensions.first, h) * band_mass(dimensions.second, h);
    std::uint64_t counted = 0;
    for (const auto& block : plan.blocks()) {
      gate.require(block.b.first < block.b.last && block.b.last <= plan.b_order().size(),
                    "large grid descriptor out of bounds");
      counted += block.b.size();
    }
    gate.require(plan.candidate_pairs() == expected && counted == expected &&
                     expected <= 441 * input.a.size() && expected < plan.total_pairs(),
                  "complete rectangular grid disagrees with its exact axial band formula");
    gate.require(plan.work().sort_passes == 3 && plan.work().sorted_sites == 3 * input.a.size() &&
                     plan.work().query_nodes > 0 && plan.work().tree_nodes > 0,
                  "large grid did not exercise shared axial sorts and spatial queries");
    // Deterministic sampled membership checks, no large Cartesian expansion.
    for (std::size_t sample = 0; sample < 128; ++sample) {
      const std::size_t a = (sample * 137) % input.a.size();
      const std::size_t b = input.b.first + (sample * 271 + 7) % input.b.size();
      const auto dy = static_cast<int>(input.points[a].y) - input.points[b].y;
      const auto dz = static_cast<int>(input.points[a].z) - input.points[b].z;
      const bool allowed = dy >= -10 && dy <= 10 && dz >= -10 && dz <= 10;
      gate.require(plan.keeps(a, b) == allowed, "large grid sample disagrees with coordinate band");
    }
    gate.large_grid_points += input.points.size();
    gate.large_grid_candidates += expected;
  }
  // The ceil(sqrt(m)) recipe has an incomplete last row. A missing h-th
  // successor leaves an open bound, so the COMPLETE-grid per-anchor bound
  // cannot be transferred. This is a measured descriptor sum, not a census.
  const auto truncated = grid(64, 63, 4000);
  const auto plan = mhgp8::make_axis_q2_plan(mhgp8::prepare_rectangle(truncated, h, 12));
  const std::size_t anchor = 62 * 64 + 22;
  std::uint64_t row_mass = 0;
  for (const auto& block : plan.blocks()) {
    if (block.a_id == anchor) row_mass += block.b.size();
  }
  gate.require(row_mass == 540 && row_mass > 441,
                "truncated-sheet fixture no longer refutes the complete-grid per-anchor bound");
  ++gate.model_mutants;
}

void ownership_and_rejections(Gate& gate) {
  gate.rejects([] { static_cast<void>(mhgp8::make_axis_q2_plan({})); },
                 "axial plan accepted a null owner");
  auto source = grid(4, 3);
  auto owner = mhgp8::prepare_rectangle(source, 2, 8);
  std::weak_ptr<const mhgp8::PreparedRectangle> weak = owner;
  const auto plan = mhgp8::make_axis_q2_plan(owner);
  const auto original = owner->points()[0];
  owner.reset();
  source.points[0] = {65535, 65535, 65535};
  gate.require(!weak.expired() && plan.rectangle().points()[0] == original,
                "axial plan lost its immutable private owner");
  const std::array<std::pair<std::size_t, std::size_t>, 4> invalid_pairs{
      std::pair<std::size_t, std::size_t>{12, 12}, {0, 0}, {0, 24},
      {std::numeric_limits<std::size_t>::max(), 12}};
  for (const auto& pair : invalid_pairs) {
    gate.rejects([&] { static_cast<void>(plan.keeps(pair.first, pair.second)); },
                   "axial keeps accepted IDs outside its ordered factors");
  }
  gate.require(!std::is_default_constructible_v<mhgp8::AxisQ2Plan> &&
                   !std::is_copy_constructible_v<mhgp8::AxisQ2Plan> &&
                   !std::is_copy_assignable_v<mhgp8::AxisQ2Plan> &&
                   std::is_nothrow_move_constructible_v<mhgp8::AxisQ2Plan> &&
                   !std::is_move_assignable_v<mhgp8::AxisQ2Plan>,
                "axial plan exposes mutable ownership special members");
  auto movable = mhgp8::make_axis_q2_plan(mhgp8::prepare_rectangle(grid(4, 3), 2, 8));
  const auto mass = movable.candidate_pairs();
  const auto* const original_owner = &movable.rectangle();
  const auto moved = std::move(movable);
  gate.require(&moved.rectangle() == original_owner && moved.candidate_pairs() == mass &&
                   movable.need() == 0 && movable.total_pairs() == 0 &&
                   movable.candidate_pairs() == 0 && movable.blocks().empty() &&
                   movable.b_order().empty(),
                "axial move did not preserve destination and empty source state");
  const auto rejects_moved = [&](auto&& action) {
    bool refused = false;
    try { action(); } catch (const std::logic_error&) { refused = true; }
    gate.require(refused, "moved-from axial plan remained usable without an owner");
    ++gate.moved_rejections;
  };
  rejects_moved([&] { static_cast<void>(movable.rectangle()); });
  rejects_moved([&] { static_cast<void>(movable.keeps(0, 12)); });
  rejects_moved([&] { movable.for_each_candidate([](std::size_t, std::size_t) {}); });
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_axis_q2_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    ordinary_fixtures(gate);
    counter_fixtures(gate);
    large_grids(gate);
    ownership_and_rejections(gate);
    gate.require(gate.small_plans >= 150 && gate.exact_census_pairs > 10000 &&
                     gate.rejected_pairs > 1000 && gate.retained_pairs > 1000 &&
                     gate.permutations == 72 && gate.model_mutants == 5 &&
                     gate.conservative_cases == 1 &&
                     gate.invalid_inputs == 5 && gate.moved_rejections == 3 && gate.positive_queries > 0 &&
                     gate.whole_accepts > 0 && gate.whole_rejects > 0 &&
                     gate.large_grid_points == 56000,
                  "axial gate non-vacuity failed");
    std::cout << "mhgp8_axis_q2_gate passed checks=" << gate.checks
              << " small_plans=" << gate.small_plans
              << " exact_census_pairs=" << gate.exact_census_pairs
              << " rejected_pairs=" << gate.rejected_pairs
              << " retained_pairs=" << gate.retained_pairs
              << " permutations=" << gate.permutations
              << " model_mutants=" << gate.model_mutants
              << " conservative_cases=" << gate.conservative_cases
              << " invalid_inputs=" << gate.invalid_inputs
              << " moved_rejections=" << gate.moved_rejections
              << " large_grid_points=" << gate.large_grid_points
              << " large_grid_candidates=" << gate.large_grid_candidates << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_axis_q2_gate failed: " << error.what() << '\n';
    return 1;
  }
}
