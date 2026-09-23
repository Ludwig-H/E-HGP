#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "oracle/q2_census_oracle.hpp"
#include "pipeline/q2_census.hpp"

namespace {

using mhgp9::gen::AxisQ2Mode;
using mhgp9::gen::Lane;
using mhgp9::gen::Point3;
using mhgp9::gen::Q2CensusMode;
using mhgp9::gen::RectangleInput;
using mhgp9::gen::Strategy;
namespace oracle = mhgp9::gen::q2_oracle;
using Pair = std::pair<std::size_t, std::size_t>;
using PointKey = std::array<mhgp9::gen::Coordinate, 3>;
using Geometry = std::pair<std::array<std::uint32_t, 3>, std::uint64_t>;

// Historical 16-bit border. Since the engine widened to 18 bits every fixture
// engraved at 65535 became an interior point; each one below keeps its oracle
// role and is doubled by an 18-bit twin at coordinate_limit. A support whose
// squared diameter exceeds 3 * 65535^2 cannot exist on a u16 cloud: counting
// those supports proves the twins exercise the widened range, not just the
// same arithmetic on larger literals.
constexpr mhgp9::gen::Coordinate u16_limit = 65535;
constexpr std::uint64_t u16_diameter_bound = 3 * std::uint64_t{u16_limit} * std::uint64_t{u16_limit};
static_assert(u16_limit < mhgp9::gen::coordinate_limit);

bool wide_input(const RectangleInput& input) {
  return std::any_of(input.points.begin(), input.points.end(), [](const Point3& point) {
    return point.x > u16_limit || point.y > u16_limit || point.z > u16_limit;
  });
}

struct Payload {
  mhgp9::gen::Q2BallKey key;
  std::vector<std::size_t> interior;
  std::vector<std::size_t> shell;
  bool operator==(const Payload&) const = default;
};
using Output = std::map<Pair, Payload>;

struct Gate {
  std::uint64_t checks{};
  std::uint64_t cases{};
  std::uint64_t runs{};
  std::uint64_t oracle_pairs{};
  std::uint64_t oracle_sites{};
  std::uint64_t prefilter_rejections{};
  std::uint64_t census_rejections{};
  std::uint64_t accepted{};
  std::uint64_t shell_sites{};
  std::uint64_t interior_sites{};
  std::uint64_t empty_runs{};
  std::uint64_t uniform_credit{};
  std::uint64_t uniform_accept{};
  std::uint64_t uniform_reject{};
  std::uint64_t query_splits{};
  std::uint64_t witness_splits{};
  std::uint64_t split_after_credit{};
  std::uint64_t cursor_reuses{};
  std::uint64_t cursor_advances{};
  std::uint64_t deep_indices{};
  std::uint64_t root_savings{};
  std::uint64_t model_mutants{};
  std::uint64_t permutations{};
  std::uint64_t rejections{};
  std::uint64_t callbacks_recovered{};
  // 18-bit twins, counted apart so that every u16 pin above stays untouched.
  std::uint64_t wide_cases{};
  std::uint64_t wide_permutations{};
  std::uint64_t wide_supports{};
  std::uint64_t wide_diameter{};

  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
  template <class Exception = std::invalid_argument, class Function>
  void rejects(Function&& function, const char* message) {
    bool refused = false;
    try { function(); } catch (const Exception&) { refused = true; }
    require(refused, message);
    ++rejections;
  }
};

PointKey point_key(const Point3& point) { return {point.x, point.y, point.z}; }

void validate_ids(Gate& gate, std::vector<std::size_t>& ids, std::size_t size) {
  for (const auto id : ids) gate.require(id < size, "payload ID escaped the global owner");
  std::sort(ids.begin(), ids.end());
  gate.require(std::adjacent_find(ids.begin(), ids.end()) == ids.end(),
               "payload repeated an original site ID");
}

void check_key(Gate& gate, const mhgp9::gen::Q2BallKey& actual, const oracle::BallKey& expected) {
  for (std::size_t axis = 0; axis < 3; ++axis) {
    gate.require(oracle::Integer(actual.center_twice[axis]) == expected.center_twice[axis],
                 "q2 key rounded, truncated or changed the doubled center");
  }
  gate.require(oracle::Integer(actual.diameter_squared) == expected.diameter_squared,
               "q2 key overflowed or changed its exact squared diameter");
}

struct Run {
  Output output;
  mhgp9::gen::Q2CensusResult result;
};

Run checked_run(Gate& gate, const mhgp9::gen::Q2CensusIndex& index,
                 const mhgp9::gen::AxisQ2Plan& plan, Q2CensusMode mode,
                 const RectangleInput& input, unsigned kmax,
                 const std::map<Pair, oracle::Census>& expected,
                 std::uint64_t candidate_count) {
  Output output;
  std::uint64_t payload_inside = 0;
  std::uint64_t payload_shell = 0;
  const auto result = mhgp9::gen::run_q2_census(index, plan, mode, [&](const mhgp9::gen::Q2Support& support) {
    const Pair pair{support.a_id, support.b_id};
    gate.require(input.a.first <= pair.first && pair.first < input.a.last &&
                     input.b.first <= pair.second && pair.second < input.b.last,
                 "census support lost its ordered-factor original IDs");
    const auto found = expected.find(pair);
    gate.require(found != expected.end(), "census emitted a pair outside the exact q2 window");
    gate.require(plan.keeps(pair.first, pair.second), "census reintroduced a prefiltered pair");
    Payload payload{support.key, {support.interior.begin(), support.interior.end()},
                                  {support.shell.begin(), support.shell.end()}};
    validate_ids(gate, payload.interior, input.points.size());
    validate_ids(gate, payload.shell, input.points.size());
    check_key(gate, payload.key, found->second.key);
    gate.require(payload.interior == found->second.interior && payload.shell == found->second.shell,
                 "census payload differs from the independent all-site cpp_int census");
    gate.require(payload.interior.size() < kmax &&
                     std::binary_search(payload.shell.begin(), payload.shell.end(), pair.first) &&
                     std::binary_search(payload.shell.begin(), payload.shell.end(), pair.second),
                 "accepted support lost its strict window or endpoint shell identities");
    payload_inside += payload.interior.size();
    payload_shell += payload.shell.size();
    if (payload.key.diameter_squared > u16_diameter_bound) ++gate.wide_supports;
    gate.wide_diameter = std::max(gate.wide_diameter, payload.key.diameter_squared);
    gate.require(output.emplace(pair, std::move(payload)).second,
                 "one support incidence was emitted twice");
  });
  gate.require(output.size() == expected.size(), "census lost a pair in the exact q2 window");
  gate.require(result.candidate_pairs == candidate_count &&
                   result.accepted_pairs == expected.size() &&
                   result.rejected_pairs == candidate_count - expected.size(),
               "census pair accounting changed the residual or support window");
  const auto& work = result.work;
  gate.require(work.input_descriptors == plan.blocks().size() &&
                   work.payload_supports == result.accepted_pairs &&
                   work.payload_interior_sites == payload_inside && work.payload_shell_sites == payload_shell,
               "census work omitted descriptors or materialized payload IDs");
  gate.require(work.frontier_restarts == 0,
               "shared census restarted an already credited prefix");
  gate.require(work.count_node_visits == work.count_bound_tests + work.count_point_tests &&
                   work.payload_node_visits == work.payload_bound_tests + work.payload_point_tests,
               "node visits omit exact bounds or point tests");
  gate.require(work.uniform_accepted_pairs <= result.accepted_pairs &&
                   work.uniform_rejected_pairs <= result.rejected_pairs &&
                   work.query_build_max_depth <= mhgp9::gen::max_index_depth,
               "uniform census decisions or query-index depth were miscounted");
  for (const double time : {result.query_index_ms, result.count_ms, result.payload_ms, result.total_ms}) {
    gate.require(std::isfinite(time) && time >= 0, "invalid census timing metadata");
  }
  if (candidate_count == 0) {
    gate.require(work.count_root_starts == 0 && work.count_node_visits == 0 &&
                     work.count_bound_tests == 0 && work.count_point_tests == 0 &&
                     work.query_build_nodes == 0 && work.query_tasks == 0 &&
                     work.cursor_advances == 0 && work.cursor_reuses == 0 &&
                     work.payload_node_visits == 0,
                 "empty residual performed unused census work");
    ++gate.empty_runs;
  } else {
    gate.require(work.count_root_starts > 0 && work.count_node_visits > 0,
                 "nonempty census did not enter its witness index");
  }
  if (mode == Q2CensusMode::Pairwise) {
    gate.require(work.count_root_starts == candidate_count &&
                     work.cursor_advances == 0 && work.cursor_reuses == 0,
                 "pairwise census did not start exactly once per residual pair");
  } else {
    if (candidate_count != 0) {
      std::size_t capacity = 1;
      std::uint64_t depth_bound = 0;
      while (capacity < input.b.size()) {
        capacity *= 2;
        ++depth_bound;
      }
      gate.require(work.query_build_nodes == 2 * input.b.size() - 1 &&
                       work.query_build_point_visits == input.b.size() &&
                       work.query_build_max_depth <= depth_bound,
                   "shared query index did not build its complete balanced factor once");
    }
    gate.require(work.cursor_reuses == 2 * work.query_splits &&
                     work.query_tasks == work.count_root_starts + 2 * work.query_splits &&
                     work.count_node_visits >= work.query_splits &&
                     work.cursor_advances == work.count_node_visits - work.query_splits,
                 "shared query splits did not inherit exactly the current witness suffix");
    if (work.count_root_starts < candidate_count) ++gate.root_savings;
  }
  gate.uniform_credit += work.uniform_credited_pairs;
  gate.uniform_accept += work.uniform_accepted_pairs;
  gate.uniform_reject += work.uniform_rejected_pairs;
  gate.query_splits += work.query_splits;
  gate.witness_splits += work.witness_splits;
  gate.split_after_credit += work.shared_splits_after_credit;
  gate.cursor_reuses += work.cursor_reuses;
  gate.cursor_advances += work.cursor_advances;
  gate.census_rejections += result.rejected_pairs;
  gate.accepted += result.accepted_pairs;
  gate.shell_sites += payload_shell;
  gate.interior_sites += payload_inside;
  ++gate.runs;
  return {std::move(output), result};
}

Output check_fixture(Gate& gate, const RectangleInput& input, unsigned kmax,
                      unsigned separation = 12, AxisQ2Mode axis_mode = AxisQ2Mode::Additive,
                      bool restrict_local = false, Strategy strategy = Strategy::Pool) {
  const auto owner = mhgp9::gen::prepare_rectangle(input, kmax, separation);
  const auto index = mhgp9::gen::make_q2_census_index(owner);
  const auto local = mhgp9::gen::make_credit_plan(owner, Lane::Q2, strategy);
  const auto plan = mhgp9::gen::make_axis_q2_plan(owner, axis_mode, restrict_local ? &local : nullptr);
  gate.require(&index->cloud() == &owner->cloud(), "global witness index changed its immutable cloud");
  const auto preparation = index->work();
  gate.require(preparation.point_visits >= input.points.size() && preparation.nodes > 0 &&
                   preparation.nodes == 2 * input.points.size() - 1 && preparation.max_depth <= mhgp9::gen::max_index_depth,
               "global witness index does not account for the complete input cloud");
  gate.require(preparation.escape_links == preparation.nodes &&
                   preparation.point_visits <= (2 * mhgp9::gen::max_index_depth + 1) * input.points.size(),
               "global index omitted escape links or exceeded its bbox-plus-partition work bound");
  std::map<Pair, oracle::Census> expected;
  std::uint64_t candidates = 0;
  for (std::size_t a = input.a.first; a < input.a.last; ++a) {
    for (std::size_t b = input.b.first; b < input.b.last; ++b) {
      auto exact = oracle::census(input.points, a, b);
      const bool keep = plan.keeps(a, b);
      if (keep) ++candidates;
      if (exact.interior.size() < kmax) {
        gate.require(keep, "prefilter discarded a support in the exact q2 census window");
        expected.emplace(Pair{a, b}, std::move(exact));
      } else if (!keep) {
        ++gate.prefilter_rejections;
      }
      ++gate.oracle_pairs;
      gate.oracle_sites += input.points.size();
    }
  }
  gate.require(candidates == plan.candidate_pairs(), "prefilter residual mass differs from pair coverage");
  const auto pairwise = checked_run(gate, *index, plan, Q2CensusMode::Pairwise,
                                     input, kmax, expected, candidates);
  const auto shared = checked_run(gate, *index, plan, Q2CensusMode::SharedBlocks,
                                   input, kmax, expected, candidates);
  gate.require(pairwise.output == shared.output, "pairwise and shared census payloads differ physically");
  gate.require(index->work().point_visits == preparation.point_visits &&
                   index->work().nodes == preparation.nodes &&
                   index->work().escape_links == preparation.escape_links &&
                   index->work().max_depth == preparation.max_depth,
               "query execution mutated or rebuilt the immutable global index");
  ++gate.cases;
  if (wide_input(input)) ++gate.wide_cases;
  return shared.output;
}

// The 18-bit twin keeps the u16 proportions (far plane at 240000 instead of
// 60000, universal interior site at 120000 instead of 30000) and moves the
// corner from 65535 to coordinate_limit.
RectangleInput sheet(unsigned side, bool extra, bool wide = false) {
  RectangleInput input;
  const std::size_t size = side * side;
  input.a = {0, size};
  input.b = {size, 2 * size};
  const mhgp9::gen::Coordinate far_plane = wide ? 240000 : 60000;
  for (const mhgp9::gen::Coordinate x : {mhgp9::gen::Coordinate{1000}, far_plane}) {
    for (unsigned y = 0; y < side; ++y) {
      for (unsigned z = 0; z < side; ++z) {
        input.points.push_back({x, static_cast<mhgp9::gen::Coordinate>(1000 + 10 * y),
                                  static_cast<mhgp9::gen::Coordinate>(1000 + 10 * z)});
      }
    }
  }
  if (extra) {
    const mhgp9::gen::Coordinate corner = wide ? mhgp9::gen::coordinate_limit : u16_limit;
    input.points.push_back({wide ? 120000 : 30000, 1000, 1000});
    input.points.push_back({0, 0, 0});
    input.points.push_back({corner, corner, corner});
  }
  return input;
}

RectangleInput reverse_ids(RectangleInput input) {
  const auto size = input.points.size();
  std::reverse(input.points.begin(), input.points.end());
  input.a = {size - input.a.last, size - input.a.first};
  input.b = {size - input.b.last, size - input.b.first};
  for (auto& id : input.core_candidates) id = size - 1 - id;
  return input;
}

using PhysicalPayload = std::pair<std::vector<PointKey>, std::vector<PointKey>>;
using PhysicalOutput = std::map<std::pair<PointKey, PointKey>, PhysicalPayload>;

PhysicalOutput physical(const Output& output, const RectangleInput& input) {
  PhysicalOutput result;
  for (const auto& [pair, payload] : output) {
    PhysicalPayload mapped;
    for (const auto id : payload.interior) mapped.first.push_back(point_key(input.points[id]));
    for (const auto id : payload.shell) mapped.second.push_back(point_key(input.points[id]));
    std::sort(mapped.first.begin(), mapped.first.end());
    std::sort(mapped.second.begin(), mapped.second.end());
    result.emplace(std::pair{point_key(input.points[pair.first]), point_key(input.points[pair.second])},
                     std::move(mapped));
  }
  return result;
}

void ordinary_fixtures(Gate& gate) {
  for (const unsigned side : {2U, 3U, 4U}) {
    for (const bool extra : {false, true}) {
      const auto input = sheet(side, extra);
      for (const unsigned k : {1U, 2U, 5U, 10U}) {
        const auto output = check_fixture(gate, input, k);
        const auto reversed = reverse_ids(input);
        const auto other = check_fixture(gate, reversed, k, 12, AxisQ2Mode::Independent);
        gate.require(physical(output, input) == physical(other, reversed),
                     "owner permutations or prefilter modes changed physical q2 supports");
        ++gate.permutations;
      }
    }
  }
  // 18-bit twin of the sheet family: same loops and checks, counted apart.
  for (const unsigned side : {2U, 3U, 4U}) {
    for (const bool extra : {false, true}) {
      const auto input = sheet(side, extra, true);
      for (const unsigned k : {1U, 2U, 5U, 10U}) {
        const auto output = check_fixture(gate, input, k);
        const auto reversed = reverse_ids(input);
        const auto other = check_fixture(gate, reversed, k, 12, AxisQ2Mode::Independent);
        gate.require(physical(output, input) == physical(other, reversed),
                     "owner permutations or prefilter modes changed physical 18-bit q2 supports");
        ++gate.wide_permutations;
      }
    }
  }
  for (const auto strategy : {Strategy::Pool, Strategy::DualBlocks, Strategy::Tubes}) {
    for (const unsigned k : {1U, 5U, 10U}) {
      static_cast<void>(check_fixture(gate, sheet(4, true), k, 12,
                                        AxisQ2Mode::Additive, true, strategy));
      static_cast<void>(check_fixture(gate, sheet(4, true, true), k, 12,
                                        AxisQ2Mode::Additive, true, strategy));
    }
  }
  const RectangleInput no_credit{
      {{1000, 1, 0}, {60000, 1, 0}, {60000, 3, 0}, {1000, 2, 0}},
      {0, 1}, {1, 3}, {}};
  static_cast<void>(check_fixture(gate, no_credit, 1));
  static_cast<void>(check_fixture(gate, no_credit, 2));
  // 18-bit twin: the x gap 239000 keeps the same +4/-4 fourfold powers on the
  // witness (read from the Boost oracle in model_counterexamples).
  const RectangleInput wide_no_credit{
      {{1000, 1, 0}, {240000, 1, 0}, {240000, 3, 0}, {1000, 2, 0}},
      {0, 1}, {1, 3}, {}};
  static_cast<void>(check_fixture(gate, wide_no_credit, 1));
  static_cast<void>(check_fixture(gate, wide_no_credit, 2));
  const RectangleInput inherited{
      {{1000, 0, 0}, {60000, 0, 0}, {60000, 2, 0}, {30000, 1, 0}, {60000, 1, 0}},
      {0, 1}, {1, 3}, {}};
  const RectangleInput wide_inherited{
      {{1000, 0, 0}, {240000, 0, 0}, {240000, 2, 0}, {120000, 1, 0}, {240000, 1, 0}},
      {0, 1}, {1, 3}, {}};
  const auto reflect = [](RectangleInput input, unsigned reflected, mhgp9::gen::Coordinate side) {
    for (auto& point : input.points) {
      point = {static_cast<mhgp9::gen::Coordinate>((reflected & 1U) != 0 ? side - point.x : point.x),
                 static_cast<mhgp9::gen::Coordinate>((reflected & 2U) != 0 ? side - point.y : point.y),
                 static_cast<mhgp9::gen::Coordinate>((reflected & 4U) != 0 ? side - point.z : point.z)};
    }
    return input;
  };
  for (unsigned reflected = 0; reflected < 8; ++reflected) {
    static_cast<void>(check_fixture(gate, reflect(inherited, reflected, u16_limit), 2));
    // 18-bit twins: the u16 fixture reflected through coordinate_limit - x
    // reaches the upper 18-bit border; the scaled fixture keeps the inherited
    // credit structure ({3} then {3, 4}, read from the Boost oracle below).
    static_cast<void>(check_fixture(gate, reflect(inherited, reflected, mhgp9::gen::coordinate_limit), 2));
    static_cast<void>(check_fixture(gate, reflect(wide_inherited, reflected, mhgp9::gen::coordinate_limit), 2));
  }
  const RectangleInput accepted_block{
      {{1000, 1000, 1000}, {60000, 999, 1000}, {60000, 1001, 1000},
       {60000, 998, 1000}, {60000, 1002, 1000}}, {0, 1}, {1, 3}, {}};
  static_cast<void>(check_fixture(gate, accepted_block, 1));
  static_cast<void>(check_fixture(gate, accepted_block, 5));
  const RectangleInput wide_accepted_block{
      {{1000, 1000, 1000}, {240000, 999, 1000}, {240000, 1001, 1000},
       {240000, 998, 1000}, {240000, 1002, 1000}}, {0, 1}, {1, 3}, {}};
  static_cast<void>(check_fixture(gate, wide_accepted_block, 1));
  static_cast<void>(check_fixture(gate, wide_accepted_block, 5));

  // A small work fixture, not a performance measurement: one universal
  // exterior-to-A/B site can reject an entire block of 64 residual queries.
  const auto shared_rejection = [](mhgp9::gen::Coordinate far_plane, mhgp9::gen::Coordinate universal) {
    RectangleInput input;
    input.points.push_back({1000, 1000, 1000});
    for (unsigned index = 0; index < 64; ++index) {
      input.points.push_back({far_plane, static_cast<mhgp9::gen::Coordinate>(1000 + index), 1000});
    }
    input.points.push_back({universal, 1000, 1000});
    input.a = {0, 1};
    input.b = {1, 65};
    return input;
  };
  for (const auto& input : {shared_rejection(60000, 30000), shared_rejection(240000, 120000)}) {
    static_cast<void>(check_fixture(gate, input, 1));
    static_cast<void>(check_fixture(gate, input, 5));
    static_cast<void>(check_fixture(gate, input, 10));
  }
}

void shell_and_key_fixtures(Gate& gate) {
  // Explicit port of P0_CENSUS_Q2_ET_COQUILLE.md, preserving its original IDs.
  const RectangleInput fractional{
      {{0, 1, 1}, {3, 2, 2}, {1, 0, 1}, {2, 3, 2}, {1, 1, 1}, {1, 1, 4}},
      {0, 1}, {1, 2}, {}};
  const auto reference = oracle::census(fractional.points, 0, 1);
  gate.require(reference.interior == std::vector<std::size_t>{4} &&
                   reference.shell == std::vector<std::size_t>({0, 1, 2, 3}) &&
                   reference.key.center_twice == std::array<oracle::Integer, 3>{3, 3, 3} &&
                   reference.key.diameter_squared == 11,
               "fractional-center audit fixture was not ported faithfully");
  std::array<unsigned, 3> axes{0, 1, 2};
  do {
    for (unsigned reflection = 0; reflection < 8; ++reflection) {
      // Each axis permutation and reflection runs twice: through the u16 side
      // 65535 (historical pin) and through coordinate_limit (18-bit twin).
      for (const mhgp9::gen::Coordinate side : {u16_limit, mhgp9::gen::coordinate_limit}) {
        auto input = fractional;
        for (auto& point : input.points) {
          const auto before = point;
          const auto coord = [&](std::size_t axis) {
            return static_cast<mhgp9::gen::Coordinate>((reflection & (1U << axis)) != 0
                ? side - before[axes[axis]] : before[axes[axis]]);
          };
          point = {coord(0), coord(1), coord(2)};
        }
        for (const unsigned k : {1U, 2U, 5U, 10U}) {
          static_cast<void>(check_fixture(gate, input, k, 8 + 2 * (reflection % 3)));
        }
        if (side == u16_limit) ++gate.permutations; else ++gate.wide_permutations;
      }
    }
  } while (std::next_permutation(axes.begin(), axes.end()));

  auto core = fractional;
  core.core_candidates = {4, 5};  // One actual universal interior and one exterior proposal.
  const auto core_output = check_fixture(gate, core, 2);
  gate.require(core_output.size() == 1 && core_output.begin()->second.interior.size() == 1,
               "census preloaded and recounted the core's only interior ID");
  static_cast<void>(check_fixture(gate, core, 1));  // Empty residual, no census work.
  auto empty_open_ball = fractional;
  empty_open_ball.points.erase(empty_open_ball.points.begin() + 4);
  const auto empty = check_fixture(gate, empty_open_ball, 1);
  gate.require(empty.size() == 1 && empty.begin()->second.interior.empty() &&
                   empty.begin()->second.shell.size() == 4,
               "an empty open ball lost its nontrivial shell at Kmax=1");

  // Reordered factors permit both diameters in one rectangle at s=1.
  // Four accepted support incidences represent three different geometries.
  const RectangleInput two_diameters{
      {{0, 1, 1}, {1, 0, 1}, {3, 2, 2}, {2, 3, 2}, {1, 1, 1}, {1, 1, 4}},
      {0, 2}, {2, 4}, {}};
  const auto same_ball = check_fixture(gate, two_diameters, 2, 1);
  gate.require(same_ball.size() == 4 && same_ball.at({0, 2}).key == same_ball.at({1, 3}).key &&
                   same_ball.at({0, 2}).interior == same_ball.at({1, 3}).interior &&
                   same_ball.at({0, 2}).shell == same_ball.at({1, 3}).shell,
               "equal ball keys erased a distinct diameter support incidence");
  std::set<Geometry> keys;
  for (const auto& [pair, payload] : same_ball) {
    static_cast<void>(pair);
    keys.emplace(payload.key.center_twice, payload.key.diameter_squared);
  }
  gate.require(keys.size() == 3, "pair count was presented as distinct-ball count");

  RectangleInput sphere;
  std::size_t a = 0;
  std::size_t b = 0;
  for (int x = -5; x <= 5; ++x) {
    for (int y = -5; y <= 5; ++y) {
      for (int z = -5; z <= 5; ++z) {
        if (x * x + y * y + z * z != 25) continue;
        if (x == -5) a = sphere.points.size();
        if (x == 5) b = sphere.points.size();
        sphere.points.push_back({static_cast<mhgp9::gen::Coordinate>(10 + x),
                                   static_cast<mhgp9::gen::Coordinate>(10 + y),
                                   static_cast<mhgp9::gen::Coordinate>(10 + z)});
      }
    }
  }
  sphere.a = {a, a + 1};
  sphere.b = {b, b + 1};
  gate.require(sphere.points.size() == 30, "large-shell fixture cardinal changed");
  const auto wide_shell = check_fixture(gate, sphere, 1);
  gate.require(wide_shell.at({a, b}).shell.size() == 30,
               "q2 payload imposed an artificial shell-size limit");
  sphere.core_candidates = {sphere.points.size()};
  sphere.points.push_back({10, 10, 10});
  static_cast<void>(check_fixture(gate, sphere, 2));

  RectangleInput extreme;
  for (unsigned bits = 0; bits < 8; ++bits) {
    extreme.points.push_back({static_cast<mhgp9::gen::Coordinate>((bits & 1U) != 0 ? u16_limit : 0),
                                static_cast<mhgp9::gen::Coordinate>((bits & 2U) != 0 ? u16_limit : 0),
                                static_cast<mhgp9::gen::Coordinate>((bits & 4U) != 0 ? u16_limit : 0)});
  }
  extreme.points.push_back({32767, 32767, 32767});
  extreme.a = {0, 1};
  extreme.b = {7, 8};
  const auto large_integer = check_fixture(gate, extreme, 2);
  gate.require(large_integer.at({0, 7}).key.diameter_squared == UINT64_C(12884508675) &&
                   large_integer.at({0, 7}).shell.size() == 8,
               "u16 extremes lost the wide squared diameter or cube-corner shell");
  static_cast<void>(check_fixture(gate, extreme, 1));

  // 18-bit twin of the corner cube: corners at coordinate_limit, near-centre
  // site at 131071. The squared diagonal 3 * 262143^2 exceeds any u16 pair;
  // its value was read from the Boost oracle (q2_oracle::census), not derived.
  RectangleInput wide_extreme;
  for (unsigned bits = 0; bits < 8; ++bits) {
    wide_extreme.points.push_back({static_cast<mhgp9::gen::Coordinate>((bits & 1U) != 0 ? mhgp9::gen::coordinate_limit : 0),
                                     static_cast<mhgp9::gen::Coordinate>((bits & 2U) != 0 ? mhgp9::gen::coordinate_limit : 0),
                                     static_cast<mhgp9::gen::Coordinate>((bits & 4U) != 0 ? mhgp9::gen::coordinate_limit : 0)});
  }
  wide_extreme.points.push_back({131071, 131071, 131071});
  wide_extreme.a = {0, 1};
  wide_extreme.b = {7, 8};
  const auto wide_integer = check_fixture(gate, wide_extreme, 2);
  gate.require(wide_integer.at({0, 7}).key.diameter_squared == UINT64_C(206156857347) &&
                   wide_integer.at({0, 7}).key.diameter_squared > u16_diameter_bound &&
                   wide_integer.at({0, 7}).shell.size() == 8 &&
                   wide_integer.at({0, 7}).interior == std::vector<std::size_t>{8},
               "u18 extremes lost the wide squared diameter, cube-corner shell or centre interior");
  static_cast<void>(check_fixture(gate, wide_extreme, 1));

  // The current midpoint index visits a long u16 path even though the
  // capped census can reject quickly. bbox reads AND partitions are paid.
  // One power of two per axis and bit up to the declared width: the index
  // path reaches its proven depth (54 at 18 bits) on 2 + 3 * 18 sites.
  RectangleInput deep{{{0, 0, 0}, {mhgp9::gen::coordinate_limit, mhgp9::gen::coordinate_limit, mhgp9::gen::coordinate_limit}}, {0, 1}, {1, 2}, {}};
  for (unsigned exponent = 0; exponent < mhgp9::gen::coordinate_bits; ++exponent) {
    const auto coordinate = static_cast<mhgp9::gen::Coordinate>(1U << exponent);
    deep.points.push_back({coordinate, 0, 0});
    deep.points.push_back({0, coordinate, 0});
    deep.points.push_back({0, 0, coordinate});
  }
  for (const unsigned separation : {8U, 10U, 12U}) {
    const auto deep_owner = mhgp9::gen::prepare_rectangle(deep, 10, separation);
    const auto deep_index = mhgp9::gen::make_q2_census_index(deep_owner);
    gate.require(deep.points.size() == 2 + 3 * mhgp9::gen::coordinate_bits && deep_index->work().max_depth >= mhgp9::gen::max_index_depth - 8 &&
                     deep_index->work().max_depth <= mhgp9::gen::max_index_depth &&
                     deep_index->work().point_visits > (mhgp9::gen::max_index_depth + 1) * deep.points.size() &&
                     deep_index->work().point_visits <= (2 * mhgp9::gen::max_index_depth + 1) * deep.points.size(),
                 "deep index fixture failed to expose the missing partition visits");
    static_cast<void>(check_fixture(gate, deep, 10, separation));
    ++gate.deep_indices;
  }
}

void model_counterexamples(Gate& gate) {
  // These execute counter-model decisions, not edited production mutants.
  // The same inputs reach both production modes through check_fixture.
  const RectangleInput corners{
      {{0, 2, 2}, {4, 2, 2}, {2, 0, 0}, {2, 4, 4}, {2, 2, 2}},
      {0, 1}, {1, 2}, {}};
  const auto exact = oracle::census(corners.points, 0, 1);
  for (unsigned bits = 0; bits < 8; ++bits) {
    const Point3 corner{2, static_cast<mhgp9::gen::Coordinate>((bits & 2U) != 0 ? 4 : 0),
                           static_cast<mhgp9::gen::Coordinate>((bits & 4U) != 0 ? 4 : 0)};
    gate.require(oracle::power_four(exact.key, corner) == 16,
                 "outside-corner counter-model lost its negative-H corners");
  }
  gate.require(oracle::power_four(exact.key, corners.points[4]) == -16 &&
                   exact.interior == std::vector<std::size_t>{4},
               "corner-only maximum failed to expose an interior site");
  static_cast<void>(check_fixture(gate, corners, 1));
  static_cast<void>(check_fixture(gate, corners, 2));
  ++gate.model_mutants;

  const Point3 anchor{1000, 1, 0};
  const Point3 witness{1000, 2, 0};
  gate.require(oracle::power_four(oracle::ball_key(anchor, {60000, 1, 0}), witness) == 4 &&
                   oracle::power_four(oracle::ball_key(anchor, {60000, 3, 0}), witness) == -4,
               "NoCredit was not refuted as a global-exterior certificate");
  gate.require(oracle::power_four(oracle::ball_key(anchor, {240000, 1, 0}), witness) == 4 &&
                   oracle::power_four(oracle::ball_key(anchor, {240000, 3, 0}), witness) == -4,
               "18-bit NoCredit twin was not refuted as a global-exterior certificate");
  ++gate.model_mutants;

  const std::vector<Point3> fractional{
      {0, 1, 1}, {3, 2, 2}, {1, 0, 1}, {2, 3, 2}, {1, 1, 1}, {1, 1, 4}};
  const auto ball = oracle::census(fractional, 0, 1);
  gate.require(ball.interior.size() < 2 && ball.interior.size() + ball.shell.size() >= 2,
               "counting shell IDs did not change the support-window decision");
  ++gate.model_mutants;
  gate.require(ball.shell.size() > 2 && oracle::power_four(ball.key, fractional[2]) == 0,
               "strict-interior pruning did not expose lost non-support shell IDs");
  ++gate.model_mutants;
  auto rounded = ball.key;
  for (auto& center : rounded.center_twice) center = 2 * (center / 2);
  gate.require(oracle::power_four(rounded, fractional[2]) < 0 &&
                   oracle::power_four(ball.key, fractional[2]) == 0,
               "rounded-center counter-model did not alter the shell classification");
  ++gate.model_mutants;
  gate.require(ball.interior.size() == 1 && 1 + ball.interior.size() >= 2,
               "preloaded-core counter-model did not double-count the only interior");
  ++gate.model_mutants;
  const auto same = oracle::census(fractional, 2, 3);
  gate.require(ball.key.center_twice == same.key.center_twice &&
                   ball.key.diameter_squared == same.key.diameter_squared,
               "pair-identity counter-model did not expose duplicate ball geometry");
  ++gate.model_mutants;

  const std::vector<Point3> inherited{
      {1000, 0, 0}, {60000, 0, 0}, {60000, 2, 0}, {30000, 1, 0}, {60000, 1, 0}};
  const auto left = oracle::census(inherited, 0, 1);
  const auto right = oracle::census(inherited, 0, 2);
  gate.require(left.interior == std::vector<std::size_t>{3} &&
                   right.interior == std::vector<std::size_t>({3, 4}) &&
                   1 + left.interior.size() >= 2,
               "restarting after a shared credit did not expose a false rejection");
  const std::vector<Point3> wide_inherited{
      {1000, 0, 0}, {240000, 0, 0}, {240000, 2, 0}, {120000, 1, 0}, {240000, 1, 0}};
  const auto wide_left = oracle::census(wide_inherited, 0, 1);
  const auto wide_right = oracle::census(wide_inherited, 0, 2);
  gate.require(wide_left.interior == std::vector<std::size_t>{3} &&
                   wide_right.interior == std::vector<std::size_t>({3, 4}),
               "18-bit inherited twin lost its shared-credit interior structure");
  ++gate.model_mutants;

  // Independent five-node preorder for the three sites x=0,2,4. Correct
  // escapes skip exactly a subtree; no implementation-private link is read.
  // A bad leaf escape can omit the middle sibling or revisit its own prefix.
  const std::array<std::size_t, 5> exits{5, 2, 5, 4, 5};
  const std::vector<Point3> line{{0, 0, 0}, {2, 0, 0}, {4, 0, 0}};
  const auto line_ball = oracle::census(line, 0, 2);
  const auto walk_model = [&](const std::array<std::size_t, 5>& candidate) {
    const std::array<int, 5> leaf_id{-1, 0, -1, 1, 2};
    std::array<bool, 5> visited{};
    std::vector<std::size_t> interior;
    std::size_t cursor = 0;
    while (cursor < candidate.size()) {
      // A revisit is a structural failure, not a timed-out test loop.
      if (visited[cursor]) return std::pair{false, interior};
      visited[cursor] = true;
      if (leaf_id[cursor] < 0) {
        ++cursor;
      } else {
        const auto id = static_cast<std::size_t>(leaf_id[cursor]);
        if (oracle::power_four(line_ball.key, line[id]) < 0) interior.push_back(id);
        cursor = candidate[cursor];
      }
    }
    return std::pair{cursor == candidate.size(), interior};
  };
  const auto valid_escape = [&](const std::array<std::size_t, 5>& candidate) {
    // root=[0,5), leaf0=[1,2), branch=[2,5), leaf2=[3,4), leaf4=[4,5).
    for (std::size_t node = 0; node < candidate.size(); ++node) {
      if (candidate[node] <= node || candidate[node] > candidate.size()) return false;
    }
    return candidate == exits;
  };
  gate.require(valid_escape(exits) && walk_model(exits).first &&
                   walk_model(exits).second == line_ball.interior,
               "preorder escape positive model is invalid");
  auto skipped_sibling = exits;
  skipped_sibling[1] = 5;
  gate.require(!valid_escape(skipped_sibling) && walk_model(skipped_sibling).first &&
                   walk_model(skipped_sibling).second != line_ball.interior,
               "escape skipped an unconsumed witness sibling");
  ++gate.model_mutants;
  auto revisited_prefix = exits;
  revisited_prefix[3] = 1;
  gate.require(!valid_escape(revisited_prefix) && !walk_model(revisited_prefix).first,
               "escape revisited an already consumed prefix");
  ++gate.model_mutants;
}

struct CallbackFailure final : std::runtime_error {
  CallbackFailure() : std::runtime_error("intentional census callback failure") {}
};

void ownership_and_rejections(Gate& gate) {
  auto input = sheet(2, true);
  const auto original = input;
  auto owner = mhgp9::gen::prepare_rectangle(std::move(input), 5, 12);
  auto index = mhgp9::gen::make_q2_census_index(owner);
  auto plan = mhgp9::gen::make_axis_q2_plan(owner, AxisQ2Mode::Additive);
  const auto* address = owner.get();
  const auto* cloud_address = &owner->cloud();
  gate.require(owner->points().data() != input.points.data(), "census owner inherited a mutable input alias");
  for (auto& point : input.points) point = {65535, 65535, 65535};
  owner.reset();
  auto moved_index = std::move(index);
  gate.require(!index && &moved_index->cloud() == cloud_address && &plan.rectangle() == address,
               "moving the index handle lost the shared immutable owner");
  std::map<Pair, oracle::Census> expected;
  for (std::size_t a = original.a.first; a < original.a.last; ++a) {
    for (std::size_t b = original.b.first; b < original.b.last; ++b) {
      auto census = oracle::census(original.points, a, b);
      if (census.interior.size() < 5) expected.emplace(Pair{a, b}, std::move(census));
    }
  }
  static_cast<void>(checked_run(gate, *moved_index, plan, Q2CensusMode::SharedBlocks,
                                  original, 5, expected, plan.candidate_pairs()));
  const mhgp9::gen::Q2CensusConsumer ignore = [](const mhgp9::gen::Q2Support&) {};
  gate.rejects([&] { static_cast<void>(mhgp9::gen::make_q2_census_index({})); },
                "census index factory accepted a null owner");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q2_census(*moved_index, plan,
                       static_cast<Q2CensusMode>(255), ignore)); }, "census accepted an invalid mode");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q2_census(*moved_index, plan,
                       Q2CensusMode::Pairwise, {})); }, "census accepted an empty callback");
  const auto foreign_owner = mhgp9::gen::prepare_rectangle(original, 5, 12);
  const auto foreign_plan = mhgp9::gen::make_axis_q2_plan(foreign_owner);
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q2_census(*moved_index, foreign_plan,
                       Q2CensusMode::SharedBlocks, ignore)); }, "census accepted another equal-looking owner");
  const auto destination = std::move(plan);
  gate.rejects<std::logic_error>([&] { static_cast<void>(mhgp9::gen::run_q2_census(*moved_index, plan,
                                        Q2CensusMode::SharedBlocks, ignore)); },
                                  "census accepted a moved-from prefilter");
  for (const auto mode : {Q2CensusMode::Pairwise, Q2CensusMode::SharedBlocks}) {
    gate.rejects<CallbackFailure>([&] { static_cast<void>(mhgp9::gen::run_q2_census(
                                     *moved_index, destination, mode,
                                     [](const mhgp9::gen::Q2Support&) { throw CallbackFailure(); })); },
                                 "census swallowed a failing output consumer");
    static_cast<void>(checked_run(gate, *moved_index, destination, mode,
                                    original, 5, expected, destination.candidate_pairs()));
    ++gate.callbacks_recovered;
  }
  auto saturated = original;
  saturated.core_candidates = {saturated.points.size()};
  saturated.points.push_back({30001, 1000, 1000});
  const auto saturated_owner = mhgp9::gen::prepare_rectangle(saturated, 1, 12);
  const auto saturated_index = mhgp9::gen::make_q2_census_index(saturated_owner);
  const auto empty = mhgp9::gen::make_axis_q2_plan(saturated_owner);
  gate.require(empty.candidate_pairs() == 0, "invalid-input empty-residual fixture was not saturated");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q2_census(*saturated_index, empty,
                       Q2CensusMode::SharedBlocks, {})); }, "empty residual bypassed callback validation");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q2_census(*saturated_index, empty,
                       static_cast<Q2CensusMode>(255), ignore)); }, "empty residual bypassed mode validation");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q2_census(*moved_index, empty,
                       Q2CensusMode::SharedBlocks, ignore)); }, "empty residual bypassed owner validation");
}

static_assert(!std::is_copy_constructible_v<mhgp9::gen::Q2CensusIndex>);
static_assert(!std::is_move_constructible_v<mhgp9::gen::Q2CensusIndex>);
static_assert(!std::is_copy_assignable_v<mhgp9::gen::Q2CensusIndex>);
static_assert(!std::is_move_assignable_v<mhgp9::gen::Q2CensusIndex>);

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp9_gen_q2_census_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    ordinary_fixtures(gate);
    shell_and_key_fixtures(gate);
    model_counterexamples(gate);
    ownership_and_rejections(gate);
    gate.require(gate.cases >= 250 && gate.runs >= 500 && gate.oracle_pairs > 5000 &&
                     gate.oracle_sites > 100000 && gate.prefilter_rejections > 0 &&
                     gate.census_rejections > 0 && gate.accepted > 0 && gate.empty_runs >= 2 &&
                     gate.shell_sites > 1000 && gate.interior_sites > 100 &&
                     gate.uniform_credit > 0 && gate.uniform_accept > 0 && gate.uniform_reject > 0 &&
                     gate.query_splits > 0 && gate.witness_splits > 0 && gate.split_after_credit > 0 &&
                     gate.cursor_reuses > 0 && gate.cursor_advances > 0 && gate.root_savings > 0 &&
                     gate.deep_indices == 3 && gate.permutations == 72 && gate.model_mutants == 10 &&
                     gate.rejections == 10 && gate.callbacks_recovered == 2,
                 "q2 census gate non-vacuity failed");
    // 18-bit twins: separate floors, the u16 pins above are unchanged. At
    // least one accepted support must be impossible on a u16 cloud, and the
    // widest accepted diameter is the cube diagonal read from the oracle.
    gate.require(gate.wide_permutations == 72 && gate.wide_cases == 252 && gate.wide_supports > 0 &&
                     gate.wide_diameter > u16_diameter_bound && gate.wide_diameter == UINT64_C(206156857347),
                 "q2 census gate 18-bit twin non-vacuity failed");
    std::cout << "mhgp9_gen_q2_census_gate passed checks=" << gate.checks
              << " cases=" << gate.cases << " runs=" << gate.runs
              << " oracle_pairs=" << gate.oracle_pairs << " oracle_sites=" << gate.oracle_sites
              << " prefilter_rejections=" << gate.prefilter_rejections
              << " census_rejections=" << gate.census_rejections << " accepted=" << gate.accepted
              << " shell_sites=" << gate.shell_sites << " interior_sites=" << gate.interior_sites
              << " uniform_credit=" << gate.uniform_credit << " uniform_accept=" << gate.uniform_accept
              << " uniform_reject=" << gate.uniform_reject << " query_splits=" << gate.query_splits
              << " witness_splits=" << gate.witness_splits << " split_after_credit=" << gate.split_after_credit
              << " cursor_reuses=" << gate.cursor_reuses << " cursor_advances=" << gate.cursor_advances
              << " deep_indices=" << gate.deep_indices
              << " root_savings=" << gate.root_savings << " permutations=" << gate.permutations
              << " model_mutants=" << gate.model_mutants << " rejections=" << gate.rejections
              << " wide_cases=" << gate.wide_cases << " wide_permutations=" << gate.wide_permutations
              << " wide_supports=" << gate.wide_supports << " wide_diameter=" << gate.wide_diameter << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp9_gen_q2_census_gate failed: " << error.what() << '\n';
    return 1;
  }
}
