#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "pipeline/wspd_q2_census.hpp"

namespace {

using mhgp8::Point3;
using mhgp8::Q2BallKey;
using mhgp8::Q2CensusIndex;
using mhgp8::Q2CensusMode;
using mhgp8::Q2SpatialNode;
using mhgp8::WspdFrontMode;
using mhgp8::u64;
using Pair = std::pair<std::size_t, std::size_t>;

struct StoredSupport {
  Pair pair;
  Q2BallKey key;
  std::vector<std::size_t> interior;
  std::vector<std::size_t> shell;
  bool operator==(const StoredSupport&) const = default;
};

using Output = std::vector<StoredSupport>;

struct Gate {
  u64 checks{};
  u64 clouds{};
  u64 oracle_pairs{};
  u64 oracle_sites{};
  u64 front_reference_runs{};
  u64 integrated_runs{};
  u64 supports{};
  u64 interior_sites{};
  u64 shell_sites{};
  u64 front_rejected_pairs{};
  u64 census_rejected_pairs{};
  u64 empty_runs{};
  u64 permutations{};
  u64 nonidentity_orders{};
  u64 query_splits{};
  u64 split_after_credit{};
  u64 uniform_credit{};
  u64 uniform_reject{};
  u64 root_savings{};
  u64 invalid_inputs{};
  u64 callback_exceptions{};
  u64 model_mutants{};

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

Pair unordered_pair(std::size_t a, std::size_t b) {
  return {std::min(a, b), std::max(a, b)};
}

void sort_output(Output& output) {
  std::sort(output.begin(), output.end(), [](const auto& a, const auto& b) { return a.pair < b.pair; });
}

// Test-only all-pairs/all-sites oracle. It calls no production geometry,
// bounds, front, census, credit planner, or candidate-selection routine.
// Each factor is promoted before subtraction; |H| <= 3*65535^2 fits i64.
Output exhaustive(Gate& gate, std::span<const Point3> points) {
  Output result;
  for (std::size_t a = 0; a < points.size(); ++a) {
    for (std::size_t b = a + 1; b < points.size(); ++b) {
      StoredSupport support;
      support.pair = {a, b};
      for (std::size_t axis = 0; axis < 3; ++axis) {
        support.key.center_twice[axis] = static_cast<std::uint32_t>(points[a][axis]) + points[b][axis];
        const auto delta = std::int64_t{points[a][axis]} - points[b][axis];
        support.key.diameter_squared += static_cast<u64>(delta * delta);
      }
      for (std::size_t z = 0; z < points.size(); ++z) {
        std::int64_t h = 0;
        for (std::size_t axis = 0; axis < 3; ++axis) {
          h += (std::int64_t{points[z][axis]} - points[a][axis]) *
               (std::int64_t{points[b][axis]} - points[z][axis]);
        }
        if (h > 0) support.interior.push_back(z);
        if (h == 0) support.shell.push_back(z);
        ++gate.oracle_sites;
      }
      result.push_back(std::move(support));
      ++gate.oracle_pairs;
    }
  }
  return result;
}

Output accepted(const Output& oracle, unsigned kmax) {
  Output result;
  for (const auto& support : oracle) {
    if (support.interior.size() < kmax) result.push_back(support);
  }
  return result;
}

void validate_ids(Gate& gate, std::vector<std::size_t>& ids, std::size_t n) {
  for (const auto id : ids) gate.require(id < n, "census payload ID escaped the immutable cloud");
  std::sort(ids.begin(), ids.end());
  gate.require(std::adjacent_find(ids.begin(), ids.end()) == ids.end(),
               "census payload repeated an original site ID");
}

struct FrontReference {
  mhgp8::WspdFrontResult result;
  u64 rectangles{};
  u64 anchors{};
  u64 pairs{};
};

FrontReference reference_front(Gate& gate, const Q2CensusIndex& index,
                               unsigned kmax, unsigned separation, WspdFrontMode mode) {
  FrontReference result;
  const auto nodes = index.spatial_nodes();
  result.result = mhgp8::run_wspd_front(index, kmax, separation, mode,
      [&](const mhgp8::WspdRectangle& rectangle) {
    gate.require(rectangle.lane_mask == 1 && rectangle.a_node < nodes.size() && rectangle.b_node < nodes.size(),
                 "q2-only front emitted an invalid node or another geometric lane");
    const auto a = nodes[rectangle.a_node].range;
    const auto b = nodes[rectangle.b_node].range;
    gate.require(a.first < a.last && b.first < b.last &&
                     (a.last <= b.first || b.last <= a.first),
                 "q2-only front emitted empty or overlapping factors");
    ++result.rectangles;
    result.anchors += std::min(a.size(), b.size());
    result.pairs += static_cast<u64>(a.size()) * b.size();
  }, 1);
  gate.require(result.result.active_lane_mask == 1 && result.rectangles == result.result.work.emitted_rectangles &&
                   result.pairs == result.result.work.residual_pair_mass[0],
               "independently observed q2 front ledger failed");
  ++gate.front_reference_runs;
  return result;
}

struct Capture {
  Output output;
  mhgp8::WspdQ2CensusResult result;
};

Capture checked_run(Gate& gate, const Q2CensusIndex& index, const Output& expected,
                     const FrontReference& front, unsigned kmax, unsigned separation,
                     WspdFrontMode front_mode, Q2CensusMode census_mode) {
  Capture capture;
  const auto n = index.cloud().points().size();
  const auto nodes_before = index.spatial_nodes().data();
  const auto order_before = index.spatial_order().data();
  const auto index_work_before = index.work();
  const auto retained_before = index.retained_bytes();
  u64 interiors = 0;
  u64 shells = 0;
  capture.result = mhgp8::run_wspd_q2_census(index, kmax, separation, front_mode, census_mode,
      [&](const mhgp8::Q2Support& support) {
    gate.require(support.a_id < n && support.b_id < n && support.a_id != support.b_id,
                 "integrated census emitted invalid original support IDs");
    StoredSupport stored{unordered_pair(support.a_id, support.b_id), support.key,
                         {support.interior.begin(), support.interior.end()},
                         {support.shell.begin(), support.shell.end()}};
    validate_ids(gate, stored.interior, n);
    validate_ids(gate, stored.shell, n);
    gate.require(stored.interior.size() < kmax &&
                     std::binary_search(stored.shell.begin(), stored.shell.end(), support.a_id) &&
                     std::binary_search(stored.shell.begin(), stored.shell.end(), support.b_id),
                 "integrated census accepted a saturated support or lost an endpoint shell ID");
    interiors += stored.interior.size();
    shells += stored.shell.size();
    capture.output.push_back(std::move(stored));
  });
  sort_output(capture.output);
  gate.require(capture.output == expected,
               "integrated q2 support/key/interior/shell flux differs from all-pairs scalar oracle");
  const auto& result = capture.result;
  const auto& census = result.census;
  const auto& work = census.work;
  const auto& actual_front = result.front;
  const auto pairs = static_cast<u64>(n) * (n - 1) / 2;
  gate.require(actual_front.active_lane_mask == 1 && actual_front.total_unordered_pairs == pairs &&
                   actual_front.work.rejected_pair_mass[1] == 0 && actual_front.work.rejected_pair_mass[2] == 0 &&
                   actual_front.work.residual_pair_mass[1] == 0 && actual_front.work.residual_pair_mass[2] == 0 &&
                   actual_front.work.lane_rectangles[1] == 0 && actual_front.work.lane_rectangles[2] == 0 &&
                   actual_front.work.xi_bound_tests == 0,
               "integrated q2 processing silently ran q3/q4 or charged their pair mass");
  gate.require(actual_front.work.emitted_rectangles == front.rectangles &&
                   actual_front.work.residual_pair_mass == front.result.work.residual_pair_mass &&
                   actual_front.work.rejected_pair_mass == front.result.work.rejected_pair_mass &&
                   actual_front.work.product_visits == front.result.work.product_visits,
               "integrated callback changed the requested q2-only front traversal");
  gate.require(result.input_rectangles == front.rectangles && result.anchor_queries == front.anchors &&
                   census.candidate_pairs == front.pairs && census.candidate_pairs == actual_front.work.residual_pair_mass[0],
               "integrated census reconstructed a different rectangle/anchor/candidate domain");
  gate.require(census.accepted_pairs == capture.output.size() &&
                   census.accepted_pairs + census.rejected_pairs == census.candidate_pairs &&
                   actual_front.work.rejected_pair_mass[0] + census.candidate_pairs == pairs,
               "front and census do not partition all unordered pairs exactly once");
  gate.require(work.payload_supports == capture.output.size() && work.payload_interior_sites == interiors &&
                   work.payload_shell_sites == shells, "integrated census payload work does not match callback IDs");
  gate.require(work.query_build_nodes == 0 && work.query_build_point_visits == 0 &&
                   work.query_build_max_depth == 0 && work.query_cover_visits == 0 && census.query_index_ms == 0,
               "integrated census rebuilt a B query tree or covered a range already certified by a spatial node");
  const auto expected_roots = census_mode == Q2CensusMode::Pairwise ? front.pairs : front.anchors;
  gate.require(work.count_root_starts == expected_roots && work.frontier_restarts == 0,
               "integrated census changed root-start accounting or restarted after acquired credit");
  if (census_mode == Q2CensusMode::Pairwise) {
    gate.require(work.query_tasks == front.pairs && work.query_splits == 0 && work.cursor_reuses == 0 &&
                     work.shared_splits_after_credit == 0,
                 "Pairwise reference did not explicitly process every residual candidate once");
  } else {
    gate.require(work.query_tasks == front.anchors + 2 * work.query_splits &&
                     work.cursor_reuses == 2 * work.query_splits && front.anchors <= front.pairs,
                 "SharedBlocks did not preserve its exact query-child and cursor ledger");
    gate.root_savings += front.pairs - front.anchors;
  }
  const auto index_work_after = index.work();
  gate.require(nodes_before == index.spatial_nodes().data() && order_before == index.spatial_order().data() &&
                   retained_before == index.retained_bytes() && index_work_before.nodes == index_work_after.nodes &&
                   index_work_before.point_visits == index_work_after.point_visits &&
                   index_work_before.escape_links == index_work_after.escape_links &&
                   index_work_before.max_depth == index_work_after.max_depth,
               "integrated front/census mutated or rebuilt the shared index");
  for (const double time : {result.total_ms, census.total_ms, census.query_index_ms,
                            census.count_ms, census.payload_ms}) {
    gate.require(std::isfinite(time) && time >= 0, "integrated census emitted an invalid component timing");
  }
  gate.require(result.total_ms == census.total_ms,
               "integrated total and census total no longer name the same enclosing interval");
  gate.supports += census.accepted_pairs;
  gate.interior_sites += interiors;
  gate.shell_sites += shells;
  gate.front_rejected_pairs += actual_front.work.rejected_pair_mass[0];
  gate.census_rejected_pairs += census.rejected_pairs;
  gate.empty_runs += static_cast<u64>(pairs == 0);
  gate.query_splits += work.query_splits;
  gate.split_after_credit += work.shared_splits_after_credit;
  gate.uniform_credit += work.uniform_credited_pairs;
  gate.uniform_reject += work.uniform_rejected_pairs;
  ++gate.integrated_runs;
  return capture;
}

Output relabel(Output output, std::span<const std::size_t> original_ids) {
  for (auto& support : output) {
    support.pair = unordered_pair(original_ids[support.pair.first], original_ids[support.pair.second]);
    for (auto& id : support.interior) id = original_ids[id];
    for (auto& id : support.shell) id = original_ids[id];
    std::sort(support.interior.begin(), support.interior.end());
    std::sort(support.shell.begin(), support.shell.end());
  }
  sort_output(output);
  return output;
}

std::vector<Point3> cube(unsigned side) {
  std::vector<Point3> points;
  for (unsigned bits = 0; bits < 8; ++bits) {
    points.push_back({static_cast<std::uint16_t>((bits & 1U) != 0 ? side : 0),
                      static_cast<std::uint16_t>((bits & 2U) != 0 ? side : 0),
                      static_cast<std::uint16_t>((bits & 4U) != 0 ? side : 0)});
  }
  return points;
}

std::vector<std::vector<Point3>> fixtures() {
  std::vector<std::vector<Point3>> result{
      {{19, 23, 29}},
      {{0, 0, 0}, {65535, 65535, 65535}},
      {{0, 0, 0}, {10, 0, 0}, {5, 0, 0}},
      {{0, 0, 0}, {10, 0, 0}, {5, 5, 0}},
      {{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}},
      cube(2), cube(65535)};
  std::vector<Point3> line;
  for (unsigned i = 0; i < 12; ++i) line.push_back({static_cast<std::uint16_t>(i * 41), 13, 17});
  result.push_back(line);
  std::vector<Point3> sheet;
  for (unsigned x = 0; x < 3; ++x) {
    for (unsigned y = 0; y < 4; ++y) {
      sheet.push_back({static_cast<std::uint16_t>(x * 53), static_cast<std::uint16_t>(y * 47), 31});
    }
  }
  result.push_back(sheet);
  std::vector<Point3> clusters;
  for (unsigned group = 0; group < 2; ++group) {
    for (unsigned bits = 0; bits < 8; ++bits) {
      clusters.push_back({static_cast<std::uint16_t>(group * 40000 + (bits & 1U)),
                          static_cast<std::uint16_t>((bits >> 1U) & 1U),
                          static_cast<std::uint16_t>((bits >> 2U) & 1U)});
    }
  }
  result.push_back(clusters);
  for (std::uint32_t seed : {1U, 37U}) {
    auto state = seed;
    const auto next = [&]() {
      state = state * 1664525U + 1013904223U;
      return static_cast<std::uint16_t>(state >> 16U);
    };
    std::vector<Point3> random;
    for (unsigned i = 0; i < 19; ++i) {
      random.push_back({static_cast<std::uint16_t>(i * 271 + seed), next(), next()});
    }
    result.push_back(random);
  }
  std::vector<Point3> skew;
  for (unsigned i = 0; i < 20; ++i) {
    skew.push_back({static_cast<std::uint16_t>(i * 103),
                    static_cast<std::uint16_t>((i * i * 7) % 101),
                    static_cast<std::uint16_t>((i * 13) % 17)});
  }
  result.push_back(skew);
  return result;
}

void corpus(Gate& gate) {
  for (const auto& original : fixtures()) {
    std::array<Output, 4> references;
    for (unsigned permutation = 0; permutation < 2; ++permutation) {
      auto points = original;
      std::vector<std::size_t> original_ids(points.size());
      std::iota(original_ids.begin(), original_ids.end(), std::size_t{0});
      if (permutation != 0) {
        std::reverse(points.begin(), points.end());
        std::reverse(original_ids.begin(), original_ids.end());
        if (points.size() > 2) {
          std::rotate(points.begin(), points.begin() + 1, points.end());
          std::rotate(original_ids.begin(), original_ids.begin() + 1, original_ids.end());
        }
        ++gate.permutations;
      }
      const auto all_pairs = exhaustive(gate, points);
      const auto cloud = mhgp8::prepare_cloud(points);
      const auto index = mhgp8::make_q2_cloud_index(cloud);
      ++gate.clouds;
      bool nonidentity = false;
      for (std::size_t rank = 0; rank < points.size(); ++rank) {
        nonidentity = nonidentity || index->spatial_order()[rank] != rank;
      }
      gate.nonidentity_orders += static_cast<u64>(nonidentity);
      // No caller-owned coordinate alias may reach the retained cloud.
      std::fill(points.begin(), points.end(), Point3{65535, 65535, 65535});
      std::size_t k_slot = 0;
      for (const unsigned kmax : {1U, 2U, 5U, 10U}) {
        const auto expected = accepted(all_pairs, kmax);
        if (permutation == 0) references[k_slot] = relabel(expected, original_ids);
        for (const unsigned separation : {8U, 10U, 12U}) {
          for (const auto front_mode : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
            const auto front = reference_front(gate, *index, kmax, separation, front_mode);
            for (const auto census_mode : {Q2CensusMode::Pairwise, Q2CensusMode::SharedBlocks}) {
              const auto capture = checked_run(gate, *index, expected, front, kmax, separation,
                                                front_mode, census_mode);
              gate.require(relabel(capture.output, original_ids) == references[k_slot],
                           "input permutation, separation or execution mode changed exact q2 output");
            }
          }
        }
        ++k_slot;
      }
    }
  }
}

void targeted(Gate& gate) {
  const std::vector<Point3> line{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}};
  const auto all_line = exhaustive(gate, line);
  const auto expected_line = accepted(all_line, 2);
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(line));
  for (const auto mode : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
    const auto front = reference_front(gate, *index, 2, 8, mode);
    const auto run = checked_run(gate, *index, expected_line, front, 2, 8, mode, Q2CensusMode::SharedBlocks);
    gate.require(run.result.census.work.shared_splits_after_credit > 0 &&
                     run.result.census.work.cursor_reuses > 0 && run.result.anchor_queries < front.pairs,
                 "four-site fixture lost its split after common credit in the global B tree");
  }
  const auto interior_one = std::find_if(expected_line.begin(), expected_line.end(),
                                        [](const auto& support) { return support.pair == Pair{0, 2}; });
  gate.require(interior_one != expected_line.end() && interior_one->interior == std::vector<std::size_t>{1} &&
                   interior_one->shell == std::vector<std::size_t>({0, 2}) &&
                   std::none_of(expected_line.begin(), expected_line.end(),
                                [](const auto& support) { return support.pair == Pair{0, 3}; }),
               "four-site scalar fixture did not distinguish inherited count one from saturation two");

  const auto cube_points = cube(2);
  const auto cube_oracle = exhaustive(gate, cube_points);
  const auto expected_cube = accepted(cube_oracle, 1);
  gate.require(expected_cube.size() == 28, "cube fixture lost admissible q2 supports");
  const auto cube_index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(cube_points));
  const auto cube_front = reference_front(gate, *cube_index, 1, 8, WspdFrontMode::MidpointSamples);
  const auto cube_run = checked_run(gate, *cube_index, expected_cube, cube_front, 1, 8,
                                    WspdFrontMode::MidpointSamples, Q2CensusMode::SharedBlocks);
  const Q2BallKey common_ball{{2, 2, 2}, 12};
  u64 common_supports = 0;
  for (const auto& support : cube_run.output) {
    if (support.key == common_ball) {
      gate.require(support.interior.empty() && support.shell.size() == 8,
                   "cube diagonal lost non-support boundary sites");
      ++common_supports;
    }
  }
  gate.require(common_supports == 4, "equal ball keys were incorrectly deduplicated across supports");

  // Permanent negative models exercise the same full-output comparison used
  // above. They are test models, not injected mutations of production code.
  auto loss = expected_line;
  loss.erase(loss.begin());
  gate.require(loss != expected_line, "lost support model survived");
  ++gate.model_mutants;
  auto duplicate = expected_line;
  duplicate.push_back(expected_line.front());
  sort_output(duplicate);
  gate.require(duplicate != expected_line, "duplicate support model survived");
  ++gate.model_mutants;
  auto double_credit = expected_line;
  double_credit.erase(std::remove_if(double_credit.begin(), double_credit.end(),
                                     [](const auto& support) { return support.pair == Pair{0, 2}; }), double_credit.end());
  gate.require(double_credit != expected_line, "inherited count plus rechecked witness model survived");
  ++gate.model_mutants;
  auto wrong_key = expected_cube;
  ++wrong_key.front().key.center_twice[0];
  gate.require(wrong_key != expected_cube, "rounded or shifted center model survived");
  ++gate.model_mutants;
  auto lost_shell = expected_cube;
  const auto diagonal = std::find_if(lost_shell.begin(), lost_shell.end(),
                                     [&](const auto& support) { return support.key == common_ball; });
  gate.require(diagonal != lost_shell.end(), "cube shell mutant lost its target support");
  diagonal->shell.pop_back();
  gate.require(lost_shell != expected_cube, "endpoint-only or truncated shell model survived");
  ++gate.model_mutants;
  auto deduplicated = expected_cube;
  bool kept = false;
  deduplicated.erase(std::remove_if(deduplicated.begin(), deduplicated.end(), [&](const auto& support) {
    if (!(support.key == common_ball)) return false;
    if (!kept) { kept = true; return false; }
    return true;
  }), deduplicated.end());
  gate.require(deduplicated.size() == 25 && deduplicated != expected_cube,
               "ball-key deduplication model survived");
  ++gate.model_mutants;
  auto nonstrict = expected_cube;
  nonstrict.front().interior.push_back(nonstrict.front().pair.first);
  gate.require(nonstrict != expected_cube, "nonstrict endpoint/interior model survived");
  ++gate.model_mutants;

  const std::vector<Point3> shell_points{{0, 0, 0}, {10, 0, 0}, {5, 5, 0}};
  const auto shell_all = exhaustive(gate, shell_points);
  const auto shell_expected = accepted(shell_all, 1);
  const auto boundary = std::find_if(shell_expected.begin(), shell_expected.end(),
                                    [](const auto& support) { return support.pair == Pair{0, 1}; });
  gate.require(boundary != shell_expected.end() && boundary->interior.empty() &&
                   boundary->shell == std::vector<std::size_t>({0, 1, 2}),
               "nonstrict-boundary fixture lost its third shell site");
  auto invalid_boundary = shell_expected;
  invalid_boundary.erase(std::remove_if(invalid_boundary.begin(), invalid_boundary.end(),
                                       [](const auto& support) { return support.pair == Pair{0, 1}; }), invalid_boundary.end());
  gate.require(invalid_boundary != shell_expected, "nonstrict prefilter rejection model survived");
  ++gate.model_mutants;
}

void invalid_and_exceptions(Gate& gate) {
  const std::vector<Point3> points{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}};
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  u64 emissions = 0;
  const mhgp8::Q2CensusConsumer consumer = [&](const mhgp8::Q2Support&) { ++emissions; };
  for (const unsigned kmax : {0U, 11U, std::numeric_limits<unsigned>::max()}) {
    gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, kmax, 8,
        WspdFrontMode::Pure, Q2CensusMode::Pairwise, consumer)); }, "integrated census accepted invalid Kmax");
  }
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 0,
      WspdFrontMode::Pure, Q2CensusMode::Pairwise, consumer)); }, "integrated census accepted zero separation");
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8,
      static_cast<WspdFrontMode>(99), Q2CensusMode::Pairwise, consumer)); }, "integrated census accepted invalid front mode");
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8,
      WspdFrontMode::Pure, static_cast<Q2CensusMode>(99), consumer)); }, "integrated census accepted invalid census mode");
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8,
      WspdFrontMode::Pure, Q2CensusMode::Pairwise, {})); }, "integrated census accepted an empty callback");
  gate.require(emissions == 0, "invalid integrated request emitted a payload before rejection");

  const mhgp8::WspdRectangleConsumer rectangle_consumer = [&](const mhgp8::WspdRectangle&) { ++emissions; };
  for (const std::uint8_t mask : {std::uint8_t{0}, std::uint8_t{8}, std::uint8_t{255}}) {
    gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_front(*index, 2, 8,
        WspdFrontMode::Pure, rectangle_consumer, mask)); }, "front accepted an empty or foreign-bit lane mask");
  }
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_front(*index, 1, 8,
      WspdFrontMode::Pure, rectangle_consumer, 6)); }, "front accepted a requested mask with no supported lane");
  gate.require(emissions == 0, "invalid lane request emitted a rectangle before rejection");
  const auto intersected = mhgp8::run_wspd_front(*index, 1, 8, WspdFrontMode::Pure,
      [&](const mhgp8::WspdRectangle& rectangle) {
    gate.require(rectangle.lane_mask == 1, "front failed to intersect requested and supported lanes");
  }, 3);
  gate.require(intersected.active_lane_mask == 1 && intersected.work.residual_pair_mass[0] == 6 &&
                   intersected.work.residual_pair_mass[1] == 0 && intersected.work.residual_pair_mass[2] == 0,
               "nonempty lane-mask intersection changed the q2 domain");

  const auto expected = accepted(exhaustive(gate, points), 2);
  struct CallbackFailure {};
  for (const auto front_mode : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
    for (const auto census_mode : {Q2CensusMode::Pairwise, Q2CensusMode::SharedBlocks}) {
      emissions = 0;
      std::vector<Pair> prefix;
      bool caught = false;
      try {
        static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8, front_mode, census_mode,
            [&](const mhgp8::Q2Support& support) {
          ++emissions;
          prefix.push_back(unordered_pair(support.a_id, support.b_id));
          if (emissions == 2) throw CallbackFailure{};
        }));
      } catch (const CallbackFailure&) { caught = true; }
      gate.require(caught && emissions == 2 && prefix.size() == 2,
                   "integrated census swallowed a callback exception or emitted after it");
      gate.require(prefix[0] != prefix[1], "failed call had already duplicated a support in its visible prefix");
      const auto front = reference_front(gate, *index, 2, 8, front_mode);
      static_cast<void>(checked_run(gate, *index, expected, front, 2, 8, front_mode, census_mode));
      ++gate.callback_exceptions;
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_wspd_q2_census_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    corpus(gate);
    targeted(gate);
    invalid_and_exceptions(gate);
    gate.require(gate.clouds == 26 && gate.integrated_runs >= 1248 && gate.front_reference_runs >= 624 &&
                     gate.oracle_pairs > 1000 && gate.oracle_sites > 20000 && gate.supports > 10000 &&
                     gate.interior_sites > 0 && gate.shell_sites > gate.supports * 2 &&
                     gate.front_rejected_pairs > 0 && gate.census_rejected_pairs > 0 && gate.empty_runs > 0 &&
                     gate.permutations == 13 && gate.nonidentity_orders > 0 && gate.query_splits > 0 &&
                     gate.split_after_credit > 0 && gate.uniform_credit > 0 && gate.uniform_reject > 0 &&
                     gate.root_savings > 0 && gate.invalid_inputs == 11 && gate.callback_exceptions == 4 &&
                     gate.model_mutants >= 8,
                 "integrated WSPD/q2 qualification lost a non-vacuity floor");
    std::cout << "mhgp8_wspd_q2_census_gate passed checks=" << gate.checks << " clouds=" << gate.clouds
              << " oracle_pairs=" << gate.oracle_pairs << " oracle_sites=" << gate.oracle_sites
              << " front_reference_runs=" << gate.front_reference_runs << " integrated_runs=" << gate.integrated_runs
              << " supports=" << gate.supports << " interior_sites=" << gate.interior_sites
              << " shell_sites=" << gate.shell_sites << " front_rejected_pairs=" << gate.front_rejected_pairs
              << " census_rejected_pairs=" << gate.census_rejected_pairs << " empty_runs=" << gate.empty_runs
              << " permutations=" << gate.permutations << " nonidentity_orders=" << gate.nonidentity_orders
              << " query_splits=" << gate.query_splits << " split_after_credit=" << gate.split_after_credit
              << " uniform_credit=" << gate.uniform_credit << " uniform_reject=" << gate.uniform_reject
              << " root_savings=" << gate.root_savings << " invalid_inputs=" << gate.invalid_inputs
              << " callback_exceptions=" << gate.callback_exceptions << " model_mutants=" << gate.model_mutants << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_wspd_q2_census_gate failed: " << error.what() << '\n';
    return 1;
  }
}
