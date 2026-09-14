#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_census.hpp"

namespace {

using mhgp8::AxisQ2Mode;
using mhgp8::Box3;
using mhgp8::CloudPtr;
using mhgp8::Lane;
using mhgp8::Point3;
using mhgp8::Q2CensusMode;
using mhgp8::Range;
using mhgp8::RectangleSpec;
using mhgp8::Strategy;
using mhgp8::u64;
using Pair = std::pair<std::size_t, std::size_t>;

static_assert(!std::is_copy_constructible_v<mhgp8::PreparedCloud>);
static_assert(!std::is_move_constructible_v<mhgp8::PreparedCloud>);
static_assert(!std::is_copy_assignable_v<mhgp8::PreparedCloud>);
static_assert(!std::is_move_assignable_v<mhgp8::PreparedCloud>);
static_assert(std::is_same_v<decltype(std::declval<const mhgp8::PreparedCloud&>().points()),
                             std::span<const Point3>>);

struct Gate {
  u64 checks{};
  u64 clouds{};
  u64 range_queries{};
  u64 multi_node_queries{};
  u64 oracle_sites{};
  u64 census_runs{};
  u64 supports{};
  u64 invalid_inputs{};
  u64 model_mutants{};
  u64 lifetime_checks{};
  u64 max_range_visits{};
  u64 max_range_steps{};

  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }

  template <class Function>
  void rejects(Function&& function, const char* message) {
    bool caught = false;
    try { function(); } catch (const std::invalid_argument&) { caught = true; }
    require(caught, message);
    ++invalid_inputs;
  }
};

bool same_box(const Box3& a, const Box3& b) {
  return a.low == b.low && a.high == b.high;
}

// Deliberately independent of the product's range-tree reduction.
Box3 scan_bounds(std::span<const Point3> points, Range range) {
  std::array<std::uint16_t, 3> low{65535, 65535, 65535};
  std::array<std::uint16_t, 3> high{};
  for (std::size_t i = range.first; i < range.last; ++i) {
    for (std::size_t axis = 0; axis < 3; ++axis) {
      low[axis] = std::min(low[axis], points[i][axis]);
      high[axis] = std::max(high[axis], points[i][axis]);
    }
  }
  return {{low[0], low[1], low[2]}, {high[0], high[1], high[2]}};
}

struct ScalarPayload {
  std::vector<std::size_t> interior;
  std::vector<std::size_t> shell;
};

ScalarPayload scalar_payload(Gate& gate, std::span<const Point3> points, Pair pair) {
  ScalarPayload result;
  const auto a = points[pair.first];
  const auto b = points[pair.second];
  for (std::size_t id = 0; id < points.size(); ++id) {
    std::int64_t value = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      value += (std::int64_t{points[id][axis]} - a[axis]) *
               (std::int64_t{b[axis]} - points[id][axis]);
    }
    if (value > 0) result.interior.push_back(id);
    if (value == 0) result.shell.push_back(id);
    ++gate.oracle_sites;
  }
  return result;
}

std::vector<Pair> checked_census(Gate& gate, const mhgp8::Q2CensusIndex& index,
                               const mhgp8::AxisQ2Plan& plan) {
  const auto& rectangle = plan.rectangle();
  const auto points = rectangle.points();
  std::vector<Pair> expected;
  for (std::size_t a = rectangle.a_range().first; a < rectangle.a_range().last; ++a) {
    for (std::size_t b = rectangle.b_range().first; b < rectangle.b_range().last; ++b) {
      if (scalar_payload(gate, points, {a, b}).interior.size() < rectangle.kmax()) {
        expected.emplace_back(a, b);
      }
    }
  }
  for (auto mode : {Q2CensusMode::Pairwise, Q2CensusMode::SharedBlocks}) {
    std::vector<Pair> actual;
    u64 interiors = 0;
    u64 shells = 0;
    const auto result = mhgp8::run_q2_census(index, plan, mode, [&](const mhgp8::Q2Support& support) {
      const Pair pair{support.a_id, support.b_id};
      gate.require(rectangle.a_range().first <= pair.first && pair.first < rectangle.a_range().last &&
                   rectangle.b_range().first <= pair.second && pair.second < rectangle.b_range().last,
                   "census emitted IDs outside its rectangle");
      const auto oracle = scalar_payload(gate, points, pair);
      auto interior = std::vector<std::size_t>(support.interior.begin(), support.interior.end());
      auto shell = std::vector<std::size_t>(support.shell.begin(), support.shell.end());
      std::sort(interior.begin(), interior.end());
      std::sort(shell.begin(), shell.end());
      gate.require(interior == oracle.interior && shell == oracle.shell,
                   "census payload differs from scalar all-cloud oracle");
      gate.require(interior.size() < rectangle.kmax(), "census ignored request threshold");
      u64 diameter = 0;
      for (std::size_t axis = 0; axis < 3; ++axis) {
        gate.require(support.key.center_twice[axis] ==
                       std::uint32_t{points[pair.first][axis]} + points[pair.second][axis],
                     "census changed original-ID ball center");
        const auto delta = std::int64_t{points[pair.first][axis]} - points[pair.second][axis];
        diameter += static_cast<u64>(delta * delta);
      }
      gate.require(support.key.diameter_squared == diameter, "census changed ball radius key");
      actual.push_back(pair);
      interiors += interior.size();
      shells += shell.size();
      ++gate.supports;
    });
    std::sort(actual.begin(), actual.end());
    gate.require(actual == expected, "shared cloud census is not complete on this bounded rectangle");
    gate.require(result.candidate_pairs == plan.candidate_pairs() &&
                 result.accepted_pairs == actual.size() &&
                 result.accepted_pairs + result.rejected_pairs == result.candidate_pairs,
                 "census pair accounting changed");
    gate.require(result.work.payload_interior_sites == interiors &&
                 result.work.payload_shell_sites == shells &&
                 result.work.payload_supports == actual.size(), "census payload ledger changed");
    ++gate.census_runs;
  }
  return expected;
}

void range_tree(Gate& gate) {
  for (const std::size_t n : {1U, 2U, 3U, 5U, 7U, 8U, 9U, 15U, 16U, 17U, 23U, 31U, 32U, 33U}) {
    std::vector<Point3> points;
    for (std::size_t i = 0; i < n; ++i) {
      points.push_back({static_cast<std::uint16_t>(i * 31 + 7),
                        static_cast<std::uint16_t>((i * 1723 + 19) % 65536),
                        static_cast<std::uint16_t>((i * 7919 + 53) % 65536)});
    }
    points[0] = {0, 65535, 17};
    if (n > 1) points[1] = {65535, 0, 65535};
    const auto cloud = mhgp8::prepare_cloud(points);
    ++gate.clouds;
    const auto before = cloud->work();
    gate.require(before.coordinate_copies == n && before.validation_points == n,
                 "cloud did not account for one private copy and validation per site");
    gate.require(before.range_tree_leaf_visits == n && before.range_tree_nodes == 2 * n - 1 &&
                 before.range_tree_merges == n - 1, "range tree construction is not linear");
    gate.require(before.uniqueness_adjacent_tests == n - 1, "adjacent duplicate checks were not charged");
    gate.require(n == 1 || before.uniqueness_comparisons > 0, "duplicate validation was vacuous");
    gate.require(cloud->retained_bytes() >= n * sizeof(Point3) &&
                 cloud->retained_bytes() <= n * 128, "cloud retained arrays violate linear envelope");
    const auto logarithm = static_cast<u64>(std::bit_width(n - 1));
    for (std::size_t first = 0; first < n; ++first) {
      for (std::size_t last = first + 1; last <= n; ++last) {
        const auto actual = cloud->bounds({first, last});
        gate.require(same_box(actual.box, scan_bounds(points, {first, last})),
                     "range tree bounds differ from original-order scalar scan");
        gate.require(actual.node_visits > 0 && actual.node_visits <= 2 * logarithm + 2,
                     "range query exceeded logarithmic canonical-node envelope");
        gate.require(actual.steps > 0 && actual.steps <= logarithm + 1,
                     "range query exceeded logarithmic loop-step envelope");
        gate.max_range_visits = std::max(gate.max_range_visits, actual.node_visits);
        gate.max_range_steps = std::max(gate.max_range_steps, actual.steps);
        if (actual.node_visits > 1) ++gate.multi_node_queries;
        ++gate.range_queries;
      }
    }
    const auto after = cloud->work();
    gate.require(before.coordinate_copies == after.coordinate_copies &&
                 before.validation_points == after.validation_points &&
                 before.uniqueness_comparisons == after.uniqueness_comparisons &&
                 before.uniqueness_adjacent_tests == after.uniqueness_adjacent_tests &&
                 before.range_tree_leaf_visits == after.range_tree_leaf_visits &&
                 before.range_tree_nodes == after.range_tree_nodes &&
                 before.range_tree_merges == after.range_tree_merges,
                 "range queries mutated immutable cloud work");
    for (const Range invalid : {Range{0, 0}, Range{n, n}, Range{n, 0},
                                Range{0, n + 1}, Range{0, std::numeric_limits<std::size_t>::max()}}) {
      gate.rejects([&] { static_cast<void>(cloud->bounds(invalid)); }, "invalid original-ID range accepted");
    }
    if (n > 2) {
      const auto intended = scan_bounds(points, {2, n});
      const auto wrong = scan_bounds(points, {0, n});
      gate.require(!same_box(intended, wrong), "wrong full-cloud bounds model survived");
      ++gate.model_mutants;
    }
  }
  const std::vector<Point3> duplicates{{0, 0, 0}, {1, 2, 3}, {0, 0, 0}};
  gate.rejects([&] { static_cast<void>(mhgp8::prepare_cloud(duplicates)); },
               "cloud accepted duplicate coordinate IDs");
  gate.rejects([&] { static_cast<void>(mhgp8::prepare_cloud(std::span<const Point3>{})); },
               "cloud accepted an empty point array");
}

void ownership(Gate& gate) {
  CloudPtr retained;
  const std::vector<Point3> original{{0, 0, 0}, {100, 0, 0}, {50, 0, 0}};
  {
    auto source = original;
    auto* mutable_alias = source.data();
    retained = mhgp8::prepare_cloud(source);
    gate.require(retained->points().data() != mutable_alias, "cloud aliases mutable caller storage");
    mutable_alias[0] = {65535, 65535, 65535};
    source[1] = {1, 1, 1};
    gate.require(std::equal(retained->points().begin(), retained->points().end(), original.begin()),
                 "source alias mutation changed certified coordinates");
  }
  gate.require(std::equal(retained->points().begin(), retained->points().end(), original.begin()),
               "source destruction invalidated certified coordinates");
  ++gate.clouds;
  ++gate.lifetime_checks;

  std::weak_ptr<const mhgp8::PreparedCloud> weak_cloud;
  std::weak_ptr<const mhgp8::PreparedRectangle> weak_rectangle;
  {
    auto cloud = retained;
    retained.reset();
    weak_cloud = cloud;
    auto rectangle = mhgp8::prepare_rectangle(cloud, RectangleSpec{{0, 1}, {1, 2}, {}}, 2, 12);
    weak_rectangle = rectangle;
    gate.require(&rectangle->cloud() == cloud.get() && rectangle->cloud_ptr() == cloud &&
                 rectangle->points().data() == cloud->points().data(), "rectangle did not share cloud storage");
    const auto index = mhgp8::make_q2_cloud_index(cloud);
    const auto adapter_index = mhgp8::make_q2_census_index(rectangle);
    gate.require(&index->cloud() == cloud.get() && &adapter_index->cloud() == cloud.get(),
                 "index adapter changed cloud identity");
    gate.require(index->retained_bytes() >= original.size() * sizeof(std::size_t) &&
                 index->retained_bytes() <= original.size() * 256 &&
                 index->retained_bytes() == adapter_index->retained_bytes(),
                 "index arrays violate the declared cloud-excluding memory envelope");
    const auto plan = mhgp8::make_axis_q2_plan(rectangle);
    rectangle.reset();
    cloud.reset();
    gate.require(!weak_cloud.expired() && !weak_rectangle.expired(), "borrowed plan lifetime lost owner");
    checked_census(gate, *index, plan);
    checked_census(gate, *adapter_index, plan);
  }
  gate.require(weak_cloud.expired() && weak_rectangle.expired(), "cloud/index/context strong ownership cycle");
  ++gate.lifetime_checks;
}

void rectangle_scope(Gate& gate) {
  const std::vector<Point3> points{{100, 0, 0}, {101, 0, 0}, {200, 0, 0}, {0, 0, 0}};
  const auto cloud = mhgp8::prepare_cloud(points);
  ++gate.clouds;
  const auto r1 = mhgp8::prepare_rectangle(cloud, RectangleSpec{{0, 2}, {2, 3}, {}}, 1, 12);
  const auto r2 = mhgp8::prepare_rectangle(cloud, RectangleSpec{{0, 2}, {3, 4}, {}}, 1, 12);
  const auto clone = mhgp8::prepare_rectangle(cloud, RectangleSpec{{0, 2}, {2, 3}, {}}, 1, 12);
  const auto index = mhgp8::make_q2_cloud_index(cloud);
  for (const auto& rectangle : {r1, r2, clone}) {
    gate.require(rectangle->preparation_work().validation_points == 0 &&
                 rectangle->preparation_work().uniqueness_comparisons == 0,
                 "shared rectangle revalidated whole cloud");
    gate.require(rectangle->factor_box_visits() > 0 && rectangle->factor_box_visits() <= 12,
                 "factor boxes have missing or nonlogarithmic work");
    gate.require(rectangle->factor_box_steps() > 0 && rectangle->factor_box_steps() <= 6,
                 "factor boxes have missing or nonlogarithmic loop work");
    gate.require(same_box(rectangle->a_box(), scan_bounds(points, rectangle->a_range())) &&
                 same_box(rectangle->b_box(), scan_bounds(points, rectangle->b_range())),
                 "rectangle factor boxes changed");
  }
  const auto legacy = mhgp8::prepare_rectangle(mhgp8::RectangleInput{points, {0, 2}, {2, 3}, {}}, 1, 12);
  gate.require(legacy->preparation_work().validation_points == points.size() &&
               legacy->preparation_work().uniqueness_comparisons > 0,
               "legacy adapter stopped charging global preparation");
  for (auto strategy : {Strategy::Pool, Strategy::DualBlocks, Strategy::Tubes}) {
    const auto c1 = mhgp8::make_credit_plan(r1, Lane::Q2, strategy);
    const auto c2 = mhgp8::make_credit_plan(r2, Lane::Q2, strategy);
    gate.require(c1.a_credits()[0] == 1 && c1.a_credits()[1] == 0 &&
                 c2.a_credits()[0] == 0 && c2.a_credits()[1] == 1,
                 "rectangle scope fixture no longer distinguishes local credits");
    const auto p1 = mhgp8::make_axis_q2_plan(r1, AxisQ2Mode::Additive, &c1);
    const auto p2 = mhgp8::make_axis_q2_plan(r2, AxisQ2Mode::Additive, &c2);
    gate.require(checked_census(gate, *index, p1) == std::vector<Pair>{{1, 2}} &&
                 checked_census(gate, *index, p2) == std::vector<Pair>{{0, 3}},
                 "one cloud index could not serve different rectangles");
    gate.rejects([&] { static_cast<void>(mhgp8::make_axis_q2_plan(r2, AxisQ2Mode::Additive, &c1)); },
                 "equal cloud allowed foreign rectangle restriction");
    gate.rejects([&] { static_cast<void>(mhgp8::make_axis_q2_plan(clone, AxisQ2Mode::Additive, &c1)); },
                 "equal rectangle values bypassed strict context identity");
    // False model: transfer the first rectangle's credits by local rank.
    gate.require(c1.a_credits()[0] + c1.b_credits()[0] >= r2->kmax() && c2.keeps(0, 3),
                 "false cloud-only restriction model survived");
    ++gate.model_mutants;
  }
  gate.rejects([&] { static_cast<void>(mhgp8::prepare_rectangle(CloudPtr{}, RectangleSpec{{0, 1}, {1, 2}, {}}, 1, 12)); },
               "rectangle accepted null cloud");
  gate.rejects([&] { static_cast<void>(mhgp8::make_q2_cloud_index(CloudPtr{})); }, "index accepted null cloud");
  gate.rejects([&] { static_cast<void>(mhgp8::prepare_rectangle(cloud, RectangleSpec{{0, 2}, {1, 3}, {}}, 1, 12)); },
               "rectangle accepted overlapping factors");
  gate.rejects([&] { static_cast<void>(mhgp8::prepare_rectangle(cloud, RectangleSpec{{0, 2}, {2, 3}, {3, 3}}, 1, 12)); },
               "rectangle accepted duplicate core IDs");
  gate.rejects([&] { static_cast<void>(mhgp8::prepare_rectangle(cloud, RectangleSpec{{0, 2}, {2, 3}, {0}}, 1, 12)); },
               "rectangle accepted a core site from a factor");
}

void threshold_and_foreign_cloud(Gate& gate) {
  const std::vector<Point3> points{{0, 0, 0}, {100, 0, 0}, {50, 0, 0}};
  const auto cloud = mhgp8::prepare_cloud(points);
  const auto other_cloud = mhgp8::prepare_cloud(points);
  gate.clouds += 2;
  const auto index = mhgp8::make_q2_cloud_index(cloud);
  const auto r1 = mhgp8::prepare_rectangle(cloud, RectangleSpec{{0, 1}, {1, 2}, {}}, 1, 12);
  const auto r2 = mhgp8::prepare_rectangle(cloud, RectangleSpec{{0, 1}, {1, 2}, {}}, 2, 12);
  const auto p1 = mhgp8::make_axis_q2_plan(r1);
  const auto p2 = mhgp8::make_axis_q2_plan(r2);
  gate.require(p1.candidate_pairs() == 1 && p2.candidate_pairs() == 1, "threshold fixture prefiltered away");
  gate.require(checked_census(gate, *index, p1).empty() &&
               checked_census(gate, *index, p2) == std::vector<Pair>{{0, 1}},
               "index retained first request threshold");
  const auto depth = scalar_payload(gate, points, {0, 1}).interior.size();
  gate.require(depth >= r1->kmax() && depth < r2->kmax(), "false cached threshold model survived");
  ++gate.model_mutants;
  const auto restriction1 = mhgp8::make_credit_plan(r1, Lane::Q2, Strategy::Pool);
  gate.rejects([&] { static_cast<void>(mhgp8::make_axis_q2_plan(r2, AxisQ2Mode::Additive, &restriction1)); },
               "restriction with another threshold accepted on the same cloud");
  for (const bool empty : {false, true}) {
    const std::vector<std::size_t> core = empty ? std::vector<std::size_t>{2} : std::vector<std::size_t>{};
    const auto foreign = mhgp8::prepare_rectangle(other_cloud, RectangleSpec{{0, 1}, {1, 2}, core}, 1, 12);
    const auto foreign_plan = mhgp8::make_axis_q2_plan(foreign);
    gate.require(foreign_plan.candidate_pairs() == (empty ? 0U : 1U), "foreign-cloud fixture not in requested regime");
    for (auto mode : {Q2CensusMode::Pairwise, Q2CensusMode::SharedBlocks}) {
      u64 callbacks = 0;
      gate.rejects([&] { static_cast<void>(mhgp8::run_q2_census(*index, foreign_plan, mode,
                                [&](const auto&) { ++callbacks; })); },
                   "equal coordinates bypassed cloud identity before census");
      gate.require(callbacks == 0, "foreign cloud was rejected after emitting payloads");
    }
    if (empty) {
      gate.rejects([&] { static_cast<void>(mhgp8::make_axis_q2_plan(foreign, AxisQ2Mode::Additive, &restriction1)); },
                   "empty residual bypassed restriction context check");
    }
  }
}

void permutation_scope(Gate& gate) {
  const std::vector<Point3> points{{0, 0, 0}, {0, 1, 0}, {0, 0, 1}, {100, 2, 2}, {100, 2, 1}};
  const auto cloud = mhgp8::prepare_cloud(points);
  ++gate.clouds;
  const auto rectangle = mhgp8::prepare_rectangle(cloud, RectangleSpec{{0, 3}, {3, 5}, {}}, 2, 12);
  const auto index = mhgp8::make_q2_cloud_index(cloud);
  const auto independent = mhgp8::make_axis_q2_plan(rectangle);
  const auto additive = mhgp8::make_axis_q2_plan(rectangle, AxisQ2Mode::Additive);
  gate.require(independent.b_order().size() == 2 && additive.b_order().size() == 2 &&
               independent.b_order()[0] == 3 && independent.b_order()[1] == 4 &&
               additive.b_order()[0] == 4 && additive.b_order()[1] == 3,
               "permutation fixture lost its two different B orders");
  gate.require(independent.work().tree_nodes == 0 && additive.work().tree_nodes > 0,
               "permutation fixture no longer exercises query-tree reordering");
  const auto expected = checked_census(gate, *index, independent);
  gate.require(expected.size() == 5 && checked_census(gate, *index, additive) == expected,
               "different B permutations changed exact output supports");
  bool selected = false;
  for (const auto& block : additive.blocks()) {
    if (block.a_id == 0 && block.b.first == 0 && block.b.last == 1) selected = true;
  }
  gate.require(selected, "permutation fixture lost valid singleton rank zero");
  const auto intended = scalar_payload(gate, points, {0, additive.b_order()[0]}).interior.size();
  const auto wrong = scalar_payload(gate, points, {0, independent.b_order()[0]}).interior.size();
  gate.require(intended == 1 && wrong == 3 && intended < rectangle->kmax() && wrong >= rectangle->kmax(),
               "false permutation-free box-cache model survived");
  ++gate.model_mutants;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_cloud_owner_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    range_tree(gate);
    ownership(gate);
    rectangle_scope(gate);
    threshold_and_foreign_cloud(gate);
    permutation_scope(gate);
    gate.require(gate.clouds >= 19 && gate.range_queries > 2000 && gate.multi_node_queries > 1000 &&
                 gate.oracle_sites >= 290 && gate.census_runs == 24 && gate.supports == 38 &&
                 gate.invalid_inputs >= 80 && gate.model_mutants >= 15 && gate.lifetime_checks == 2,
                 "cloud owner qualification lost a non-vacuity floor");
    std::cout << "mhgp8_cloud_owner_gate passed checks=" << gate.checks
              << " clouds=" << gate.clouds << " range_queries=" << gate.range_queries
              << " multi_node_queries=" << gate.multi_node_queries
              << " max_range_visits=" << gate.max_range_visits
              << " max_range_steps=" << gate.max_range_steps
              << " oracle_sites=" << gate.oracle_sites << " census_runs=" << gate.census_runs
              << " supports=" << gate.supports << " invalid_inputs=" << gate.invalid_inputs
              << " model_mutants=" << gate.model_mutants << " lifetime_checks=" << gate.lifetime_checks << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_cloud_owner_gate failed: " << error.what() << '\n';
    return 1;
  }
}
