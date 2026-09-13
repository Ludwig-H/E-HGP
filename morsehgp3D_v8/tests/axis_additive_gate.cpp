#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
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

using mhgp8::AxisQ2Mode;
using mhgp8::AxisQ2Plan;
using mhgp8::CreditPlan;
using mhgp8::Lane;
using mhgp8::Point3;
using mhgp8::Range;
using mhgp8::RectangleInput;
using mhgp8::RectanglePtr;
using mhgp8::Strategy;
namespace oracle = mhgp8::oracle;
using Bits = std::vector<std::uint8_t>;

struct Gate {
  std::uint64_t checks{};
  std::uint64_t small_cases{};
  std::uint64_t checked_plans{};
  std::uint64_t census_pairs{};
  std::uint64_t rejected_pairs{};
  std::uint64_t retained_pairs{};
  std::uint64_t additive_prunes{};
  std::uint64_t intersection_prunes{};
  std::uint64_t whole_accepts{};
  std::uint64_t whole_rejects{};
  std::uint64_t query_nodes{};
  std::uint64_t axis_queries{};
  std::uint64_t restriction_queries{};
  std::uint64_t restriction_copies{};
  std::uint64_t core_partial{};
  std::uint64_t core_saturated{};
  std::uint64_t model_mutants{};
  std::uint64_t rejections{};
  std::uint64_t ownership_cases{};
  std::uint64_t large_points{};
  std::uint64_t large_candidates{};

  void require(bool condition, const char* cause) {
    ++checks;
    if (!condition) throw std::runtime_error(cause);
  }

  template <class Exception = std::invalid_argument, class Function>
  void rejects(Function&& function, const char* cause) {
    bool refused = false;
    try { function(); } catch (const Exception&) { refused = true; }
    require(refused, cause);
    ++rejections;
  }
};

// A deliberately scalar definition on actual original IDs. It neither sorts
// columns nor calls the product's axis/box predicates. Each witness is tested
// on all axes; distinct exact coordinate lines only meet at the excluded a.
std::array<unsigned, 3> direct_counts(Gate& gate, const RectangleInput& input,
                                      std::size_t a_id, std::size_t b_id) {
  std::array<unsigned, 3> counts{};
  const auto& a = input.points[a_id];
  const auto& b = input.points[b_id];
  for (std::size_t z_id = input.a.first; z_id < input.a.last; ++z_id) {
    if (z_id == a_id) continue;
    const auto& z = input.points[z_id];
    unsigned memberships = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      bool same_column = true;
      for (std::size_t other = 0; other < 3; ++other) {
        if (other != axis && z[other] != a[other]) same_column = false;
      }
      if (same_column && std::min(a[axis], b[axis]) < z[axis] &&
          z[axis] < std::max(a[axis], b[axis])) {
        ++counts[axis];
        ++memberships;
      }
    }
    gate.require(memberships <= 1, "the oracle counted one ID in two exact columns");
  }
  return counts;
}

std::size_t offset(const RectangleInput& input, std::size_t a, std::size_t b) {
  return (a - input.a.first) * input.b.size() + b - input.b.first;
}

// Check compressed ownership and disjoint ranges without expanding pairs.
// This routine also serves the large fixtures, where no n-by-M oracle runs.
void descriptors(Gate& gate, const AxisQ2Plan& plan, const RectangleInput& input,
                 const mhgp8::PreparedRectangle* owner) {
  gate.require(&plan.rectangle() == owner, "axis plan lost its immutable owner");
  gate.require(plan.total_pairs() == input.a.size() * input.b.size(),
               "axis plan changed its Cartesian rectangle size");
  std::vector<std::size_t> order(plan.b_order().begin(), plan.b_order().end());
  if (plan.need() == 0) {
    gate.require(order.empty() && plan.blocks().empty() && plan.candidate_pairs() == 0,
                 "a core-saturated plan retained unused residual storage");
    gate.require(plan.work().sort_passes == 0 && plan.work().tree_nodes == 0 &&
                     plan.work().query_nodes == 0,
                 "a core-saturated plan performed axis or index preparation");
  } else {
    std::sort(order.begin(), order.end());
    gate.require(order.size() == input.b.size(), "B order is not a full permutation");
    for (std::size_t index = 0; index < order.size(); ++index) {
      gate.require(order[index] == input.b.first + index, "B order changed original IDs");
    }
  }
  std::vector<std::vector<Range>> per_anchor(input.a.size());
  std::uint64_t mass = 0;
  for (const auto& block : plan.blocks()) {
    gate.require(input.a.first <= block.a_id && block.a_id < input.a.last &&
                     block.b.first < block.b.last && block.b.last <= plan.b_order().size(),
                 "invalid additive descriptor or owner-relative ID");
    mass += block.b.size();
    per_anchor[block.a_id - input.a.first].push_back(block.b);
  }
  for (auto& ranges : per_anchor) {
    std::sort(ranges.begin(), ranges.end(), [](Range left, Range right) {
      return std::pair{left.first, left.last} < std::pair{right.first, right.last};
    });
    for (std::size_t index = 1; index < ranges.size(); ++index) {
      gate.require(ranges[index - 1].last <= ranges[index].first,
                   "additive descriptors overlap for one anchor");
    }
  }
  gate.require(mass == plan.candidate_pairs() &&
                   plan.work().emitted_blocks == plan.blocks().size(),
               "descriptor mass or work differs from the published residual");
  gate.require(plan.work().contained_nodes + plan.work().whole_factor_accepts ==
                   plan.work().emitted_blocks + plan.work().coalesced_blocks,
               "accepted ranges disappeared from the descriptor/coalescing counters");
  gate.require(plan.work().max_tree_depth <= 48, "u16 index exceeded its proven depth");
  gate.whole_accepts += plan.work().whole_factor_accepts;
  gate.whole_rejects += plan.work().whole_factor_rejects;
  gate.query_nodes += plan.work().query_nodes;
  gate.axis_queries += plan.work().axis_bound_queries;
  gate.restriction_queries += plan.work().restriction_bound_queries;
  gate.restriction_copies += plan.work().restriction_credit_copies;
}

void check_plan(Gate& gate, const AxisQ2Plan& plan, const RectangleInput& input,
                const mhgp8::PreparedRectangle* owner, const Bits& expected,
                const std::vector<unsigned>& depths, unsigned kmax) {
  descriptors(gate, plan, input, owner);
  Bits emitted(expected.size(), 0);
  plan.for_each_candidate([&](std::size_t a, std::size_t b) {
    gate.require(input.a.first <= a && a < input.a.last &&
                     input.b.first <= b && b < input.b.last,
                 "additive expansion emitted an ID outside its factors");
    const auto index = offset(input, a, b);
    gate.require(emitted[index] == 0, "additive expansion duplicated a pair");
    emitted[index] = 1;
  });
  gate.require(emitted == expected, "additive expansion differs from the scalar ID oracle");
  for (std::size_t a = input.a.first; a < input.a.last; ++a) {
    for (std::size_t b = input.b.first; b < input.b.last; ++b) {
      const auto index = offset(input, a, b);
      gate.require(plan.keeps(a, b) == (expected[index] != 0),
                   "additive keeps differs from its descriptors or scalar oracle");
      if (expected[index] != 0) {
        ++gate.retained_pairs;
      } else {
        gate.require(depths[index] >= kmax,
                     "a rejected pair lacks enough distinct strict cpp_int witnesses");
        ++gate.rejected_pairs;
      }
    }
  }
  ++gate.checked_plans;
}

void check_small(Gate& gate, const RectangleInput& input, unsigned kmax, unsigned s) {
  const auto owner = mhgp8::prepare_rectangle(input, kmax, s);
  const auto a_box = oracle::bounds(input.points, input.a.first, input.a.last);
  const auto b_box = oracle::bounds(input.points, input.b.first, input.b.last);
  const unsigned core = oracle::core_credit(input.points, input.core_candidates,
                                             a_box, b_box, Lane::Q2, kmax);
  gate.require(owner->core_credit(Lane::Q2) == core, "core certification differs from oracle");
  if (core > 0 && core < kmax) ++gate.core_partial;
  if (core == kmax) ++gate.core_saturated;
  Bits independent(input.a.size() * input.b.size(), 0);
  Bits additive(independent.size(), 0);
  std::vector<unsigned> depths(independent.size(), 0);
  for (std::size_t a = input.a.first; a < input.a.last; ++a) {
    for (std::size_t b = input.b.first; b < input.b.last; ++b) {
      const auto index = offset(input, a, b);
      const auto counts = direct_counts(gate, input, a, b);
      const unsigned sum = counts[0] + counts[1] + counts[2];
      const unsigned maximum = *std::max_element(counts.begin(), counts.end());
      independent[index] = static_cast<std::uint8_t>(core + maximum < kmax);
      additive[index] = static_cast<std::uint8_t>(core + sum < kmax);
      // All sites, including unproposed sites outside A union B. The cap is
      // above the possible depth, so this is an exact bounded cpp_int census.
      depths[index] = oracle::point_credit(input.points, a, b, Lane::Q2,
                                            static_cast<unsigned>(input.points.size()));
      gate.require(core + sum <= depths[index], "axis/core IDs overcounted real interiors");
      gate.require(additive[index] == 0 || independent[index] != 0,
                   "additive residual is not a subset of independent axes");
      if (independent[index] != 0 && additive[index] == 0) ++gate.additive_prunes;
      ++gate.census_pairs;
    }
  }
  const auto old_plan = mhgp8::make_axis_q2_plan(owner);
  const auto new_plan = mhgp8::make_axis_q2_plan(owner, AxisQ2Mode::Additive);
  gate.require(old_plan.need() == kmax - core && new_plan.need() == kmax - core,
               "axis mode changed the certified threshold");
  gate.require(old_plan.mode() == AxisQ2Mode::Independent &&
                   new_plan.mode() == AxisQ2Mode::Additive &&
                   !old_plan.has_restriction() && !new_plan.has_restriction(),
               "axis factory changed its default mode or invented a restriction");
  check_plan(gate, old_plan, input, owner.get(), independent, depths, kmax);
  check_plan(gate, new_plan, input, owner.get(), additive, depths, kmax);
  for (const auto strategy : {Strategy::Pool, Strategy::DualBlocks, Strategy::Tubes}) {
    const auto local = mhgp8::make_credit_plan(owner, Lane::Q2, strategy);
    for (const auto mode : {AxisQ2Mode::Independent, AxisQ2Mode::Additive}) {
      Bits expected = mode == AxisQ2Mode::Additive ? additive : independent;
      for (std::size_t a = input.a.first; a < input.a.last; ++a) {
        for (std::size_t b = input.b.first; b < input.b.last; ++b) {
          const auto index = offset(input, a, b);
          const bool local_keep = local.keeps(a, b);
          if (mode == AxisQ2Mode::Additive && expected[index] != 0 && !local_keep) {
            ++gate.intersection_prunes;
          }
          expected[index] = static_cast<std::uint8_t>(expected[index] != 0 && local_keep);
        }
      }
      const auto restricted = mhgp8::make_axis_q2_plan(owner, mode, &local);
      gate.require(restricted.mode() == mode && restricted.has_restriction(),
                   "intersection lost its mode or restriction identity");
      gate.require(restricted.work().restriction_credit_copies ==
                       (core == kmax ? 0 : input.a.size() + input.b.size()),
                   "restriction snapshot copies are not accounted exactly once");
      check_plan(gate, restricted, input, owner.get(), expected, depths, kmax);
    }
  }
  ++gate.small_cases;
}

RectangleInput grid(unsigned width, unsigned height, std::size_t count = 0) {
  if (count == 0) count = static_cast<std::size_t>(width) * height;
  RectangleInput input;
  input.a = {0, count};
  input.b = {count, 2 * count};
  input.points.reserve(2 * count);
  for (const std::uint16_t x : {std::uint16_t{1000}, std::uint16_t{60000}}) {
    for (std::size_t index = 0; index < count; ++index) {
      input.points.push_back({x, static_cast<std::uint16_t>(1000 + index % width),
                                static_cast<std::uint16_t>(1000 + index / width)});
    }
  }
  return input;
}

RectangleInput longitudinal(unsigned count) {
  RectangleInput input;
  input.a = {0, count};
  input.b = {count, 2 * count};
  for (unsigned side = 0; side < 2; ++side) {
    for (unsigned index = 0; index < count; ++index) {
      input.points.push_back({static_cast<std::uint16_t>((side == 0 ? 1000 : 60000) + index),
                                1000, 1000});
    }
  }
  return input;
}

void fixtures(Gate& gate) {
  for (const unsigned k : {1U, 2U, 5U, 10U}) {
    for (const unsigned s : {8U, 10U, 12U}) {
      check_small(gate, grid(6, 5), k, s);
      check_small(gate, grid(6, 5, 27), k, s);
      check_small(gate, longitudinal(13), k, s);
    }
    check_small(gate, grid(1, 1), k, 12);
    auto core = grid(4, 3);
    for (unsigned index = 0; index < 12; ++index) {
      core.core_candidates.push_back(core.points.size());
      core.points.push_back({static_cast<std::uint16_t>(30000 + index), 1001, 1001});
    }
    // A proposal that is not a witness must not inflate the shared core.
    core.core_candidates.push_back(core.points.size());
    core.points.push_back({0, 65535, 65535});
    check_small(gate, core, k, 12);
    core.core_candidates.resize(1);
    check_small(gate, core, k, 12);
  }
  auto permuted = grid(5, 4, 18);
  for (unsigned axis = 0; axis < 3; ++axis) {
    for (unsigned reflection = 0; reflection < 2; ++reflection) {
      auto input = permuted;
      for (auto& point : input.points) {
        const std::array<std::uint16_t, 3> old{point.x, point.y, point.z};
        point = {old[axis], old[(axis + 1) % 3], old[(axis + 2) % 3]};
        if (reflection != 0) point = {static_cast<std::uint16_t>(65535 - point.x),
                                       static_cast<std::uint16_t>(65535 - point.y),
                                       static_cast<std::uint16_t>(65535 - point.z)};
      }
      std::reverse(input.points.begin(), input.points.begin() +
                     static_cast<std::ptrdiff_t>(input.a.last));
      std::rotate(input.points.begin() + static_cast<std::ptrdiff_t>(input.b.first),
                     input.points.begin() + static_cast<std::ptrdiff_t>(input.b.first + 3),
                     input.points.end());
      for (const unsigned k : {1U, 5U, 10U}) check_small(gate, input, k, 12);
    }
  }
  // Integer 3-4-5 rotation of a small grid, with a common scale of five.
  // On this width/height no two distinct rotated sites share y or z, hence
  // no axial credits; the census still sees the same scaled geometry.
  auto rotated = grid(4, 4);
  for (auto& point : rotated.points) {
    if (point.x == 60000) point.x = 10000;
  }
  for (auto& point : rotated.points) {
    const int y = static_cast<int>(point.y) - 1000;
    const int z = static_cast<int>(point.z) - 1000;
    point = {static_cast<std::uint16_t>(5 * point.x),
               static_cast<std::uint16_t>(2000 + 3 * y - 4 * z),
               static_cast<std::uint16_t>(2000 + 4 * y + 3 * z)};
  }
  for (const unsigned k : {1U, 5U, 10U}) check_small(gate, rotated, k, 12);
  const auto rotated_plan = mhgp8::make_axis_q2_plan(
      mhgp8::prepare_rectangle(rotated, 5, 12), AxisQ2Mode::Additive);
  gate.require(rotated_plan.candidate_pairs() == rotated_plan.total_pairs(),
               "rotated fixture unexpectedly acquired an exact coordinate column");

  RectangleInput generic;
  generic.a = {0, 11};
  generic.b = {11, 20};
  for (unsigned index = 0; index < 20; ++index) {
    generic.points.push_back({static_cast<std::uint16_t>((index < 11 ? 1000 : 60000) + index),
                               static_cast<std::uint16_t>(1000 + index * 7),
                               static_cast<std::uint16_t>(1000 + index * index)});
  }
  // An unproposed outside witness exercises the global small census.
  generic.points.push_back({30000, 1000, 1000});
  for (const unsigned k : {1U, 5U, 10U}) check_small(gate, generic, k, 8);
  auto offset_input = grid(3, 4);
  offset_input.points.insert(offset_input.points.begin(), {0, 65535, 65535});
  offset_input.a = {1, 13};
  offset_input.b = {13, 25};
  check_small(gate, offset_input, 2, 12);
  std::swap(offset_input.a, offset_input.b);
  check_small(gate, offset_input, 2, 12);
}

void counter_fixtures(Gate& gate) {
  // Explicit port of the independent auditor's section-8 four-site example.
  // The previous gate retains it under Independent; Additive must remove it.
  const RectangleInput four{
      {{1000, 1000, 1000}, {1000, 1001, 1000}, {1000, 1000, 1001},
       {60000, 1002, 1002}}, {0, 3}, {3, 4}, {}};
  check_small(gate, four, 2, 12);
  const auto owner = mhgp8::prepare_rectangle(four, 2, 12);
  const auto old_plan = mhgp8::make_axis_q2_plan(owner);
  const auto added = mhgp8::make_axis_q2_plan(owner, AxisQ2Mode::Additive);
  gate.require(oracle::metrics(four.points[0], four.points[3], four.points[1]).h == 1 &&
                   oracle::metrics(four.points[0], four.points[3], four.points[2]).h == 1 &&
                   old_plan.keeps(0, 3) && !added.keeps(0, 3),
               "the additive filter failed the disjoint two-column certificate");
  ++gate.model_mutants;

  const RectangleInput overlap{{{0, 0, 0}, {1, 0, 0}, {100, 0, 0}}, {0, 2}, {2, 3}, {}};
  check_small(gate, overlap, 2, 12);
  const auto overlap_owner = mhgp8::prepare_rectangle(overlap, 2, 12);
  const auto counts = direct_counts(gate, overlap, 0, 2);
  for (const auto strategy : {Strategy::Pool, Strategy::DualBlocks, Strategy::Tubes}) {
    const auto local = mhgp8::make_credit_plan(overlap_owner, Lane::Q2, strategy);
    const auto mixed = mhgp8::make_axis_q2_plan(overlap_owner, AxisQ2Mode::Additive, &local);
    const unsigned wrongly_added = counts[0] + counts[1] + counts[2] + local.a_credits()[0];
    gate.require(wrongly_added == 2 && oracle::point_credit(overlap.points, 0, 2,
                                                             Lane::Q2, 3) == 1 &&
                     local.keeps(0, 2) && mixed.keeps(0, 2),
                 "intersection added overlapping axial and local witness populations");
    ++gate.model_mutants;
  }
  const RectangleInput strict{
      {{0, 0, 0}, {0, 2, 0}, {0, 0, 2}, {100, 2, 3}, {100, 3, 3}},
      {0, 3}, {3, 5}, {}};
  check_small(gate, strict, 2, 12);
  const auto strict_plan = mhgp8::make_axis_q2_plan(
      mhgp8::prepare_rectangle(strict, 2, 12), AxisQ2Mode::Additive);
  gate.require(strict_plan.keeps(0, 3) && !strict_plan.keeps(0, 4) &&
                   oracle::point_credit(strict.points, 0, 3, Lane::Q2, 3) == 1 &&
                   oracle::metrics(strict.points[0], strict.points[3], strict.points[1]).h == 0,
               "a closed witness boundary replaced the strict axial count");
  ++gate.model_mutants;
  const RectangleInput approximate{
      {{0, 0, 0}, {0, 1, 1}, {100, 2, 0}}, {0, 2}, {2, 3}, {}};
  check_small(gate, approximate, 1, 12);
  const auto no_column = mhgp8::make_axis_q2_plan(
      mhgp8::prepare_rectangle(approximate, 1, 12), AxisQ2Mode::Additive);
  gate.require(no_column.keeps(0, 2) && oracle::point_credit(approximate.points, 0, 2,
                                                             Lane::Q2, 2) == 0,
               "approximate columns supplied false axial witnesses");
  ++gate.model_mutants;
  const RectangleInput irregular{
      {{0, 0, 0}, {0, 2, 0}, {0, 100, 0}, {60000, 99, 0}, {60000, 101, 0}},
      {0, 3}, {3, 5}, {}};
  check_small(gate, irregular, 2, 12);
  const auto exact_rank = mhgp8::make_axis_q2_plan(
      mhgp8::prepare_rectangle(irregular, 2, 12), AxisQ2Mode::Additive);
  gate.require(exact_rank.keeps(0, 3) && !exact_rank.keeps(0, 4),
               "rank offsets replaced the coordinates of actual axial neighbors");
  ++gate.model_mutants;

  const RectangleInput maximal{
      {{0, 65533, 65533}, {0, 65534, 65533}, {0, 65533, 65534},
       {65535, 65535, 65535}}, {0, 3}, {3, 4}, {}};
  check_small(gate, maximal, 2, 12);
  RectangleInput all_axes{
      {{1000, 1000, 1000}, {1001, 1000, 1000}, {1002, 1000, 1000},
       {1000, 1001, 1000}, {1000, 1002, 1000}, {1000, 1000, 1001},
       {1000, 1000, 1002}, {60000, 30000, 30000}}, {0, 7}, {7, 8}, {}};
  check_small(gate, all_axes, 5, 12);
  const auto three_counts = direct_counts(gate, all_axes, 0, 7);
  gate.require(three_counts == std::array<unsigned, 3>{2, 2, 2},
               "three-axis fixture did not exercise all disjoint populations");
}

void large_grids(Gate& gate) {
  // For h=10 the sum of actual column counts is (|dy|-1)+ + (|dz|-1)+.
  // The 261 admissible offsets sum to this closed placement formula.
  // No pair expansion and no multiprecision census on these large inputs.
  const std::array<std::array<std::uint64_t, 3>, 3> cases{{
      {{50, 80, 918160}}, {{80, 100, 1912660}}, {{125, 128, 3928390}}}};
  for (const auto& dimensions : cases) {
    const auto width = dimensions[0];
    const auto height = dimensions[1];
    const auto input = grid(static_cast<unsigned>(width), static_cast<unsigned>(height));
    const auto owner = mhgp8::prepare_rectangle(input, 10, 12);
    const auto plan = mhgp8::make_axis_q2_plan(owner, AxisQ2Mode::Additive);
    const auto formula = 261 * width * height - 990 * (width + height) + 2860;
    gate.require(formula == dimensions[2] && plan.candidate_pairs() == formula,
                 "large complete-grid residual differs from the independent closed formula");
    gate.require(plan.candidate_pairs() < input.a.size() * 441 &&
                     plan.work().query_nodes > 0 && plan.blocks().size() < plan.candidate_pairs(),
                 "large additive fixture did not exercise compressed strict pruning");
    descriptors(gate, plan, input, owner.get());
    gate.large_points += input.points.size();
    gate.large_candidates += plan.candidate_pairs();
  }
}

void ownership_and_rejections(Gate& gate) {
  auto input = longitudinal(13);
  const auto original = input;
  auto owner = mhgp8::prepare_rectangle(std::move(input), 5, 12);
  gate.require(owner->points().data() != input.points.data() && !input.points.empty(),
               "prepared owner retained a mutable input-buffer alias");
  const auto original_owner = owner.get();
  auto local = mhgp8::make_credit_plan(owner, Lane::Q2, Strategy::DualBlocks);
  const CreditPlan snapshot(local);
  auto plan = mhgp8::make_axis_q2_plan(owner, AxisQ2Mode::Additive, &local);
  const CreditPlan moved_local(std::move(local));
  const auto replacement_owner = mhgp8::prepare_rectangle(grid(3, 3), 5, 12);
  local = mhgp8::make_credit_plan(replacement_owner, Lane::Q2, Strategy::Pool);
  for (std::size_t index = 0; index < input.points.size(); ++index) {
    input.points[index] = {static_cast<std::uint16_t>(index), 65535, 65535};
  }
  owner.reset();
  Bits expected(original.a.size() * original.b.size(), 0);
  std::vector<unsigned> depths(expected.size(), 0);
  for (std::size_t a = original.a.first; a < original.a.last; ++a) {
    for (std::size_t b = original.b.first; b < original.b.last; ++b) {
      const auto index = offset(original, a, b);
      const auto counts = direct_counts(gate, original, a, b);
      expected[index] = static_cast<std::uint8_t>(counts[0] + counts[1] + counts[2] < 5 &&
                                                  snapshot.keeps(a, b));
      depths[index] = oracle::point_credit(original.points, a, b, Lane::Q2, 30);
      gate.require(snapshot.keeps(a, b) == moved_local.keeps(a, b),
                   "copying or moving the restriction changed its original residual");
    }
  }
  check_plan(gate, plan, original, original_owner, expected, depths, 5);
  ++gate.ownership_cases;
  const auto scoped = [&] {
    auto transient = mhgp8::make_credit_plan(replacement_owner, Lane::Q2, Strategy::Pool);
    return mhgp8::make_axis_q2_plan(replacement_owner, AxisQ2Mode::Additive, &transient);
  }();
  gate.require(&scoped.rectangle() == replacement_owner.get(),
               "axis restriction borrowed a destroyed local plan");
  static_cast<void>(scoped.keeps(0, 9));
  ++gate.ownership_cases;

  const auto moved_axis = std::move(plan);
  gate.require(plan.mode() == AxisQ2Mode::Independent && !plan.has_restriction() &&
                   plan.need() == 0 && plan.total_pairs() == 0 &&
                   plan.candidate_pairs() == 0 && plan.b_order().empty() && plan.blocks().empty(),
               "moved axis source retained its old mode or ownership metadata");
  check_plan(gate, moved_axis, original, original_owner, expected, depths, 5);
  gate.rejects<std::logic_error>([&] { static_cast<void>(plan.rectangle()); },
                                  "moved additive plan exposed geometry");
  gate.rejects<std::logic_error>([&] { static_cast<void>(plan.keeps(0, 13)); },
                                  "moved additive plan accepted a pair query");
  gate.rejects<std::logic_error>([&] { plan.for_each_candidate([](auto, auto) {}); },
                                  "moved additive plan expanded stale descriptors");
  gate.rejects([&] { static_cast<void>(mhgp8::make_axis_q2_plan({}, AxisQ2Mode::Additive)); },
                "additive factory accepted a null owner");
  gate.rejects([&] { static_cast<void>(mhgp8::make_axis_q2_plan(
                       replacement_owner, static_cast<AxisQ2Mode>(255))); },
                "axis factory accepted an invalid mode");
  const auto foreign = mhgp8::make_credit_plan(
      mhgp8::prepare_rectangle(grid(3, 3), 5, 12), Lane::Q2, Strategy::Pool);
  gate.rejects([&] { static_cast<void>(mhgp8::make_axis_q2_plan(
                       replacement_owner, AxisQ2Mode::Additive, &foreign)); },
                "intersection accepted an equal-looking but different owner");
  const auto wrong_lane = mhgp8::make_credit_plan(replacement_owner, Lane::Q3, Strategy::Pool);
  gate.rejects([&] { static_cast<void>(mhgp8::make_axis_q2_plan(
                       replacement_owner, AxisQ2Mode::Additive, &wrong_lane)); },
                "q2 intersection accepted another geometric lane");
  auto abandoned = mhgp8::make_credit_plan(replacement_owner, Lane::Q2, Strategy::Pool);
  const auto survivor = std::move(abandoned);
  gate.rejects<std::logic_error>([&] { static_cast<void>(mhgp8::make_axis_q2_plan(
                                        replacement_owner, AxisQ2Mode::Additive, &abandoned)); },
                                  "intersection accepted a moved-from restriction");
  gate.require(&survivor.rectangle() == replacement_owner.get(),
               "restriction move did not preserve the destination owner");
  auto saturated_input = grid(1, 1);
  saturated_input.core_candidates = {2};
  saturated_input.points.push_back({30000, 1000, 1000});
  const auto saturated_owner = mhgp8::prepare_rectangle(saturated_input, 1, 12);
  const auto inactive_lane = mhgp8::make_credit_plan(saturated_owner, Lane::Q3, Strategy::Pool);
  gate.require(saturated_owner->core_credit(Lane::Q2) == 1,
               "saturated invalid-input fixture has no certified core");
  gate.rejects([&] { static_cast<void>(mhgp8::make_axis_q2_plan(
                       saturated_owner, static_cast<AxisQ2Mode>(255))); },
                "core saturation bypassed the mode check");
  gate.rejects([&] { static_cast<void>(mhgp8::make_axis_q2_plan(
                       saturated_owner, AxisQ2Mode::Additive, &inactive_lane)); },
                "core saturation bypassed the restriction lane check");
  gate.rejects([&] { static_cast<void>(mhgp8::make_axis_q2_plan(
                       saturated_owner, AxisQ2Mode::Additive, &foreign)); },
                "core saturation bypassed the restriction owner check");
  gate.rejects<std::logic_error>([&] { static_cast<void>(mhgp8::make_axis_q2_plan(
                                        saturated_owner, AxisQ2Mode::Additive, &abandoned)); },
                                  "core saturation bypassed the moved restriction check");
  for (const auto& ids : {std::pair<std::size_t, std::size_t>{13, 13}, {0, 0}, {0, 26}}) {
    gate.rejects([&] { static_cast<void>(moved_axis.keeps(ids.first, ids.second)); },
                  "additive keeps accepted IDs outside its ordered factors");
  }
}

static_assert(!std::is_copy_constructible_v<AxisQ2Plan>);
static_assert(!std::is_move_assignable_v<AxisQ2Plan>);
static_assert(std::is_nothrow_move_constructible_v<AxisQ2Plan>);

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_axis_additive_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    fixtures(gate);
    counter_fixtures(gate);
    large_grids(gate);
    ownership_and_rejections(gate);
    gate.require(gate.small_cases >= 70 && gate.checked_plans >= 560 &&
                     gate.census_pairs > 10000 && gate.additive_prunes > 100 &&
                     gate.intersection_prunes > 100 && gate.rejected_pairs > 1000 &&
                     gate.retained_pairs > 1000 && gate.whole_accepts > 0 &&
                     gate.whole_rejects > 0 && gate.query_nodes > 0 &&
                     gate.axis_queries > 0 && gate.restriction_queries > 0 &&
                     gate.restriction_copies > 0 &&
                     gate.core_partial >= 2 && gate.core_saturated >= 4 &&
                     gate.model_mutants == 7 && gate.rejections == 15 &&
                     gate.ownership_cases == 2 && gate.large_points == 56000 &&
                     gate.large_candidates == 6759210,
                 "additive/intersection gate non-vacuity failed");
    std::cout << "mhgp8_axis_additive_gate passed checks=" << gate.checks
              << " small_cases=" << gate.small_cases << " checked_plans=" << gate.checked_plans
              << " census_pairs=" << gate.census_pairs << " additive_prunes=" << gate.additive_prunes
              << " intersection_prunes=" << gate.intersection_prunes
              << " axis_queries=" << gate.axis_queries
              << " restriction_queries=" << gate.restriction_queries
              << " restriction_copies=" << gate.restriction_copies
              << " rejected_pairs=" << gate.rejected_pairs << " retained_pairs=" << gate.retained_pairs
              << " core_partial=" << gate.core_partial << " core_saturated=" << gate.core_saturated
              << " model_mutants=" << gate.model_mutants << " rejections=" << gate.rejections
              << " ownership_cases=" << gate.ownership_cases << " large_points=" << gate.large_points
              << " large_candidates=" << gate.large_candidates << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_axis_additive_gate failed: " << error.what() << '\n';
    return 1;
  }
}
