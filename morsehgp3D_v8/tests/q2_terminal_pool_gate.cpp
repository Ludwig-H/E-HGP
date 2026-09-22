#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "pipeline/wspd_q2_census.hpp"

namespace {
using mhgp8::Point3;
using mhgp8::u64;
using mhgp8::Q2AnchorMode;
using mhgp8::Q2CensusMode;
using mhgp8::Q2SiblingMode;
using mhgp8::Q2WitnessOrder;
using mhgp8::WspdFrontMode;
using Points = std::vector<Point3>;
using Pair = std::pair<std::size_t, std::size_t>;
struct Payload {
  mhgp8::Q2BallKey key;
  std::vector<std::size_t> interior, shell;
  bool operator==(const Payload&) const = default;
};
using Output = std::map<Pair, Payload>;
struct Gate {
  u64 checks{}, clouds{}, runs{}, oracle_pairs{}, oracle_sites{}, supports{}, extra_shells{}, max_shell{};
  u64 selected_rectangles{}, filtered_pairs{}, residual_pairs{}, factor_sites{}, bands{}, selected64{};
  u64 passthrough_rectangles{}, unchanged_census{}, reentrant_calls{};
  u64 default_comparisons{}, joint_runs{}, pairwise_runs{}, invalid_inputs{}, callback_failures{}, model_mutants{};
  void require(bool ok, const char* message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
  }
  template<class Function> void rejects(Function function) {
    bool caught = false;
    try { function(); } catch (const std::invalid_argument&) { caught = true; }
    require(caught, "terminal Pool accepted an invalid request");
    ++invalid_inputs;
  }
};

// Same explicit physical definition as earlier independent gates; no use
// of product keys, box predicates, local credits or census traversal.
std::int64_t h(const Point3& a, const Point3& b, const Point3& z) {
  std::int64_t result = 0;
  for (std::size_t d = 0; d < 3; ++d)
    result += (std::int64_t{z[d]} - a[d]) * (std::int64_t{b[d]} - z[d]);
  return result;
}

Output oracle(Gate& gate, std::span<const Point3> points) {
  gate.require(!points.empty() && points.size() <= 69, "exhaustive oracle escaped its bounded tiny-cloud scope");
  Output output;
  for (std::size_t a = 0; a < points.size(); ++a) for (std::size_t b = a + 1; b < points.size(); ++b) {
    Payload payload{};
    for (std::size_t d = 0; d < 3; ++d) {
      payload.key.center_twice[d] = static_cast<std::uint32_t>(points[a][d]) + points[b][d];
      const auto difference = std::int64_t{points[a][d]} - points[b][d];
      payload.key.diameter_squared += static_cast<u64>(difference * difference);
    }
    for (std::size_t z = 0; z < points.size(); ++z) {
      const auto value = h(points[a], points[b], points[z]);
      if (value > 0) payload.interior.push_back(z);
      if (value == 0) payload.shell.push_back(z);
      ++gate.oracle_sites;
    }
    output.emplace(Pair{a, b}, std::move(payload));
    ++gate.oracle_pairs;
  }
  return output;
}

Output admissible(const Output& all, unsigned k) {
  Output result;
  for (const auto& [pair, payload] : all) if (payload.interior.size() < k) result.emplace(pair, payload);
  return result;
}

auto pool_work(const mhgp8::Q2PoolWork& w) {
  return std::array{w.selected_rectangles, w.selected_pairs, w.residual_pairs, w.filtered_pairs,
      w.factor_sites, w.selection_tests, w.witness_attempts, w.universal_queries, w.q2_axis_terms,
      w.pool_selected, w.pool_insertions, w.pool_shifted_entries, w.prefix_class_visits,
      w.factor_read_visits, w.grouping_visits, w.bands, w.selected_anchors, w.pair_roots,
      w.plan_peak_bytes, w.original_selected_anchors, w.passthrough_rectangles,
      w.passthrough_pairs, w.passthrough_anchors};
}

auto census_work(const mhgp8::Q2CensusWork& w) {
  return std::array{w.query_build_point_visits, w.query_build_nodes, w.query_build_max_depth,
      w.input_descriptors, w.query_cover_visits, w.query_tasks, w.query_splits, w.witness_splits,
      w.count_root_starts, w.shared_splits_after_credit, w.cursor_advances, w.cursor_reuses,
      w.count_node_visits, w.count_bound_tests, w.count_point_tests, w.uniform_credited_pairs,
      w.uniform_rejected_pairs, w.uniform_accepted_pairs, w.consumed_witness_sites, w.frontier_restarts,
      w.payload_node_visits, w.payload_bound_tests, w.payload_point_tests, w.payload_interior_sites,
      w.payload_shell_sites, w.payload_supports};
}

struct Capture { Output output; mhgp8::WspdQ2CensusResult result; };

Capture run(Gate& gate, const mhgp8::Q2CensusIndex& index, const Output& expected,
            unsigned k, unsigned s, WspdFrontMode front, std::size_t cutoff,
            Q2AnchorMode anchors = Q2AnchorMode::Individual,
            Q2CensusMode mode = Q2CensusMode::SharedBlocks, bool implicit_default = false) {
  const auto sibling = mode == Q2CensusMode::SharedBlocks ? Q2SiblingMode::Saturating : Q2SiblingMode::Disabled;
  const auto witness = mode == Q2CensusMode::SharedBlocks ? Q2WitnessOrder::ComplementFirst : Q2WitnessOrder::GlobalDfs;
  const auto n = index.cloud().points().size();
  const auto* nodes = index.spatial_nodes().data();
  const auto* order = index.spatial_order().data();
  const auto* points = index.cloud().points().data();
  const auto visits = index.work().point_visits;
  Capture capture;
  u64 interiors = 0, shells = 0;
  const mhgp8::Q2CensusConsumer consume = [&](const mhgp8::Q2Support& support) {
    gate.require(support.a_id < n && support.b_id < n && support.a_id != support.b_id,
                 "terminal Pool emitted invalid original support IDs");
    Payload payload{support.key, {support.interior.begin(), support.interior.end()},
                    {support.shell.begin(), support.shell.end()}};
    for (auto* ids : {&payload.interior, &payload.shell}) {
      for (const auto id : *ids) gate.require(id < n, "terminal Pool payload contains an out-of-cloud ID");
      std::sort(ids->begin(), ids->end());
      gate.require(std::adjacent_find(ids->begin(), ids->end()) == ids->end(), "terminal Pool duplicated a payload ID");
    }
    gate.require(std::binary_search(payload.shell.begin(), payload.shell.end(), support.a_id) &&
                     std::binary_search(payload.shell.begin(), payload.shell.end(), support.b_id),
                 "Pool removed support endpoints from the global shell");
    interiors += payload.interior.size();
    shells += payload.shell.size();
    gate.extra_shells += payload.shell.size() > 2;
    gate.max_shell = std::max(gate.max_shell, static_cast<u64>(payload.shell.size()));
    gate.require(capture.output.emplace(std::minmax(support.a_id, support.b_id), std::move(payload)).second,
                 "terminal Pool emitted the same incidence twice");
  };
  capture.result = implicit_default
      ? mhgp8::run_wspd_q2_census(index, k, s, front, mode, consume, sibling, witness, anchors)
      : mhgp8::run_wspd_q2_census(index, k, s, front, mode, consume, sibling, witness, anchors, cutoff);
  gate.require(capture.output == expected, "terminal Pool changed exact supports, ball keys, interiors or full shells");
  const auto& r = capture.result;
  const auto& c = r.census;
  const auto& p = r.pool_work;
  gate.require(r.front.active_lane_mask == 1 && c.candidate_pairs + p.filtered_pairs == r.front.work.residual_pair_mass[0] &&
                   p.selected_pairs == p.filtered_pairs + p.residual_pairs && p.selected_pairs <= r.front.work.residual_pair_mass[0] &&
                   c.candidate_pairs == c.accepted_pairs + c.rejected_pairs && c.accepted_pairs == expected.size() &&
                   c.candidate_pairs + p.filtered_pairs + r.front.work.rejected_pair_mass[0] == n * (n - 1) / 2,
               "front, Pool and census no longer partition all unordered pairs");
  gate.require(p.pair_roots == p.residual_pairs - p.passthrough_pairs &&
                   p.passthrough_rectangles <= p.selected_rectangles && p.passthrough_pairs <= p.residual_pairs &&
                   p.passthrough_anchors <= p.original_selected_anchors &&
                   p.selected_anchors <= p.original_selected_anchors - p.passthrough_anchors &&
                   p.original_selected_anchors <= r.anchor_queries && p.selected_rectangles <= r.input_rectangles &&
                   p.residual_pairs >= p.selected_rectangles && p.bands <= k * (p.selected_rectangles - p.passthrough_rectangles) &&
                   p.selected_anchors <= p.pair_roots && p.bands <= p.selected_anchors,
               "terminal Pool expanded non-residual pairs or lost its local minimum cross pair");
  gate.require(p.factor_read_visits == 2 * p.factor_sites && p.grouping_visits == 2 * p.factor_sites &&
                   p.prefix_class_visits == (k + 1) * p.selected_rectangles &&
                   p.selection_tests <= (k + 1) * p.factor_sites && p.witness_attempts <= (k + 1) * p.factor_sites &&
                   p.universal_queries <= p.witness_attempts && p.q2_axis_terms <= 3 * p.universal_queries &&
                   p.pool_selected <= 2 * (k + 1) * p.selected_rectangles && p.pool_insertions <= p.factor_sites &&
                   p.pool_shifted_entries <= (k + 1) * p.pool_insertions,
               "terminal Pool preparation exceeded its O(KF+KR) ledger");
  gate.require(c.work.input_descriptors == r.input_rectangles && c.work.frontier_restarts == 0 &&
                   c.work.query_build_nodes == 0 && c.work.query_build_point_visits == 0 && c.work.query_cover_visits == 0 &&
                   c.work.query_build_max_depth == 0 && c.query_index_ms == 0 &&
                   c.work.payload_supports == expected.size() && c.work.payload_interior_sites == interiors &&
                   c.work.payload_shell_sites == shells && c.work.cursor_reuses == 2 * c.work.query_splits,
               "terminal Pool preloaded credits, rebuilt query factors or miscounted global payloads");
  if (mode == Q2CensusMode::Pairwise) {
    gate.require(c.work.count_root_starts == c.candidate_pairs && c.work.query_tasks == c.candidate_pairs &&
                     c.work.query_splits == 0, "pairwise Pool census did not start every survivor from zero");
    ++gate.pairwise_runs;
  } else if (anchors == Q2AnchorMode::Individual) {
    const auto roots = p.pair_roots + r.anchor_queries - p.original_selected_anchors + p.passthrough_anchors;
    gate.require(c.work.count_root_starts == roots && c.work.query_tasks == roots + 2 * c.work.query_splits &&
                     r.joint_work.root_products == 0, "mixed individual/Pool root ledger failed");
  } else {
    const auto roots = r.input_rectangles - p.selected_rectangles + p.passthrough_rectangles;
    const auto& j = r.joint_work;
    gate.require(j.root_products == roots && c.work.count_root_starts == roots + p.pair_roots &&
                     c.work.query_tasks == p.pair_roots + j.singleton_handoffs + 2 * c.work.query_splits &&
                     j.tasks == roots + 2 * (j.splits_a + j.splits_b) &&
                     j.rejected_pairs + j.accepted_pairs + j.handoff_pair_mass ==
                         c.candidate_pairs - p.residual_pairs + p.passthrough_pairs,
                 "mixed joint/Pool continuation or pair mass ledger failed");
    if (anchors == Q2AnchorMode::SharedAnchors)
      gate.require(j.splits_b == 0 && j.singleton_handoffs <=
                       r.anchor_queries - p.original_selected_anchors + p.passthrough_anchors,
                   "SharedAnchors duplicated non-Pool anchors");
    ++gate.joint_runs;
  }
  gate.require(r.sibling_work.proposals == 2 * c.work.query_splits &&
                   r.sibling_work.cardinality_skips + r.sibling_work.bound_tests == r.sibling_work.proposals,
               "Pool pair census incorrectly reused a grouped sibling certificate");
  if (cutoff == 0 || cutoff > n || r.input_rectangles == 0) {
    gate.require(pool_work(p) == std::array<u64, 23>{} && p.preparation_ms == 0 && p.selected_total_ms == 0,
                 "disabled/unselected Pool paid work or changed default counters");
  }
  if (cutoff == 1) {
    gate.require(p.selected_rectangles == r.input_rectangles && p.original_selected_anchors == r.anchor_queries,
                 "threshold1 failed to prepare every terminal rectangle");
  }
  if (n == 69 && cutoff == 64)
    gate.require(p.selected_rectangles == 1 && p.filtered_pairs > 0 && p.pair_roots > 0,
                 "large-factor threshold64 fixture stopped exercising filtering and residual census");
  for (const double time : {p.preparation_ms, p.selected_total_ms, r.total_ms, c.count_ms, c.payload_ms})
    gate.require(std::isfinite(time) && time >= 0, "terminal Pool emitted invalid timings");
  gate.require(p.preparation_ms <= p.selected_total_ms && p.selected_total_ms <= r.total_ms &&
                   nodes == index.spatial_nodes().data() && order == index.spatial_order().data() &&
                   points == index.cloud().points().data() && visits == index.work().point_visits,
               "Pool timing scope or immutable index sharing failed");
  gate.selected_rectangles += p.selected_rectangles;
  gate.filtered_pairs += p.filtered_pairs;
  gate.residual_pairs += p.residual_pairs;
  gate.factor_sites += p.factor_sites;
  gate.bands += p.bands;
  gate.passthrough_rectangles += p.passthrough_rectangles;
  if (cutoff == 64) gate.selected64 += p.selected_rectangles;
  gate.supports += expected.size();
  ++gate.runs;
  return capture;
}

void same_front(Gate& gate, const Capture& a, const Capture& b) {
  gate.require(a.output == b.output && a.result.input_rectangles == b.result.input_rectangles &&
                   a.result.anchor_queries == b.result.anchor_queries &&
                   a.result.front.work.product_visits == b.result.front.work.product_visits &&
                   a.result.front.work.residual_pair_mass == b.result.front.work.residual_pair_mass &&
                   a.result.front.work.rejected_pair_mass == b.result.front.work.rejected_pair_mass,
               "terminal policy changed the front or final support incidence domain");
}

std::vector<Points> fixtures() {
  std::vector<Points> result{
      {{7, 8, 9}}, {{0, 0, 0}, {1, 0, 0}, {99, 0, 0}, {100, 0, 0}},
      {{0, 0, 0}, {1, 0, 0}, {100, 0, 0}}, {{100, 0, 0}, {0, 1, 0}, {1, 0, 1}},
      {{1000, 0, 0}, {0, 1, 0}, {0, 0, 0}}, {{0, 0, 0}, {100, 0, 0}, {50, 0, 0}}};
  result.push_back({{0, 0, 0}, {0, 1, 0}, {1000, 0, 0}, {1000, 1, 0}});
  Points sphere, cube, large, random;
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y) for (int z = -5; z <= 5; ++z)
    if (x * x + y * y + z * z == 25)
      sphere.push_back({static_cast<mhgp8::Coordinate>(x + 8), static_cast<mhgp8::Coordinate>(y + 8),
                        static_cast<mhgp8::Coordinate>(z + 8)});
  for (unsigned bits = 0; bits < 8; ++bits)
    cube.push_back({static_cast<mhgp8::Coordinate>((bits & 1U) * 65535),
                   static_cast<mhgp8::Coordinate>(((bits >> 1U) & 1U) * 65535),
                   static_cast<mhgp8::Coordinate>(((bits >> 2U) & 1U) * 65535)});
  cube.push_back({32768, 32767, 32768});
  for (unsigned y = 0; y < 2; ++y) for (unsigned z = 0; z < 2; ++z)
    large.push_back({0, static_cast<mhgp8::Coordinate>(y), static_cast<mhgp8::Coordinate>(z)});
  for (unsigned i = 0; i < 65; ++i) large.push_back({static_cast<mhgp8::Coordinate>(1000 + i), 0, 0});
  std::uint32_t state = 971;
  for (unsigned i = 0; i < 13; ++i) {
    state = state * 1664525U + 1013904223U;
    const auto y = static_cast<mhgp8::Coordinate>(state >> 16U);
    state = state * 1664525U + 1013904223U;
    random.push_back({static_cast<mhgp8::Coordinate>(i * 251), y, static_cast<mhgp8::Coordinate>(state >> 16U)});
  }
  result.push_back(sphere); result.push_back(cube); result.push_back(large); result.push_back(random);
  return result;
}

// 18-bit twins (coordinate_limit = 262143) of the u16 corner fixture: the
// full-extent cube with its near-center site {131072, 131071, 131072}, a
// random cloud drawn on 18 bits (state >> 14), and the 4x65 large-factor
// fixture translated so its far row ends exactly at 262143 (squared axis
// gaps above 2^32, the Pool64 selection and filtering at 18-bit magnitude,
// judged by the same n==69 cutoff-64 check). They pass through the same
// oracle and run matrix as the u16 corpus, reflected by 262143 - x, with
// their own floors; the u16 fixtures and their pinned floors are untouched.
std::vector<Points> fixtures_18bits() {
  constexpr mhgp8::Coordinate limit = mhgp8::coordinate_limit;
  std::vector<Points> result;
  Points cube, random, large;
  for (unsigned bits = 0; bits < 8; ++bits)
    cube.push_back({(bits & 1U) != 0 ? limit : 0, (bits & 2U) != 0 ? limit : 0, (bits & 4U) != 0 ? limit : 0});
  cube.push_back({131072, 131071, 131072});
  std::uint32_t state = 971;
  for (unsigned i = 0; i < 13; ++i) {
    state = state * 1664525U + 1013904223U;
    const auto y = static_cast<mhgp8::Coordinate>(state >> 14U);
    state = state * 1664525U + 1013904223U;
    random.push_back({static_cast<mhgp8::Coordinate>(i * 20143), y, static_cast<mhgp8::Coordinate>(state >> 14U)});
  }
  for (unsigned y = 0; y < 2; ++y) for (unsigned z = 0; z < 2; ++z)
    large.push_back({0, static_cast<mhgp8::Coordinate>(y), static_cast<mhgp8::Coordinate>(z)});
  for (unsigned i = 0; i < 65; ++i) large.push_back({static_cast<mhgp8::Coordinate>(limit - 64 + i), 0, 0});
  result.push_back(cube); result.push_back(random); result.push_back(large);
  return result;
}

void require_wide(Gate& gate, const std::vector<Points>& fixtures) {
  for (const auto& fixture : fixtures) {
    mhgp8::Coordinate widest = 0;
    for (const auto& point : fixture) widest = std::max({widest, point.x, point.y, point.z});
    gate.require(widest > 65535 && widest <= mhgp8::coordinate_limit,
                 "18-bit fixture does not leave the historical u16 range or exceeds coordinate_limit");
  }
}

void corpus(Gate& gate, const std::vector<Points>& fixtures, mhgp8::Coordinate reflect) {
  for (const auto& base : fixtures) for (unsigned transform = 0; transform < 2; ++transform) {
    auto points = base;
    if (transform != 0) {
      for (auto& point : points) point = {point.z, static_cast<mhgp8::Coordinate>(reflect - point.x), point.y};
      std::reverse(points.begin(), points.end());
    }
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
    const auto all = oracle(gate, points);
    ++gate.clouds;
    for (const unsigned k : {1U, 2U, 5U, 10U}) for (const unsigned s : {8U, 10U, 12U})
      for (const auto front : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
        const auto expected = admissible(all, k);
        const auto baseline = run(gate, *index, expected, k, s, front, 0);
        for (const std::size_t cutoff : {std::size_t{1}, std::size_t{2}, std::size_t{64},
                                         std::numeric_limits<std::size_t>::max()}) {
          const auto selected = run(gate, *index, expected, k, s, front, cutoff);
          same_front(gate, baseline, selected);
          if (cutoff > points.size() || selected.result.pool_work.filtered_pairs == 0) {
            gate.require(census_work(baseline.result.census.work) == census_work(selected.result.census.work),
                         "unselected or zero-reduction Pool changed baseline census work");
            ++gate.unchanged_census;
          }
        }
        if (s == 8 && front == WspdFrontMode::Pure) {
          const auto implicit = run(gate, *index, expected, k, s, front, 0, Q2AnchorMode::Individual,
                                     Q2CensusMode::SharedBlocks, true);
          same_front(gate, baseline, implicit);
          gate.require(census_work(baseline.result.census.work) == census_work(implicit.result.census.work),
                       "omitted Pool argument changed its default work");
          ++gate.default_comparisons;
          if (k == 2 || k == 10)
            for (const auto anchors : {Q2AnchorMode::SharedProduct, Q2AnchorMode::SharedAnchors})
              for (const std::size_t cutoff : {std::size_t{0}, std::size_t{1}, std::size_t{2}, std::size_t{64}})
                same_front(gate, baseline, run(gate, *index, expected, k, s, front, cutoff, anchors));
        }
      }
  }
}

void targeted(Gate& gate) {
  const Points precredit{{0, 0, 0}, {1, 0, 0}, {100, 0, 0}};
  const auto all = oracle(gate, precredit);
  const auto expected = admissible(all, 2);
  gate.require(all.at({0, 2}).interior.size() == 1 && h(precredit[0], precredit[2], precredit[1]) == 99,
               "preloaded credit counterfixture lost its exact single witness");
  gate.require(all.at({0, 2}).interior.size() < 2 && all.at({0, 2}).interior.size() + 1 >= 2,
               "preload-and-recount model no longer kills an admissible support");
  ++gate.model_mutants;
  const Points perm{{100, 0, 0}, {0, 1, 0}, {1, 0, 1}};
  gate.require(h(perm[0], perm[1], perm[2]) == 98 && h(perm[0], perm[2], perm[1]) == -101,
               "rank/ID model lost its actual different strict populations");
  ++gate.model_mutants;
  const Points shell{{1000, 0, 0}, {0, 1, 0}, {0, 0, 0}};
  gate.require(h(shell[0], shell[1], shell[2]) == 0, "nonstrict-credit model lost the exact shell boundary");
  ++gate.model_mutants;
  const Points external{{0, 0, 0}, {100, 0, 0}, {50, 0, 0}};
  gate.require(h(external[0], external[1], external[2]) == 2500,
               "factor-only census model lost its external strict witness");
  ++gate.model_mutants;
  auto lost_shell = expected;
  lost_shell.begin()->second.shell.pop_back();
  gate.require(lost_shell != expected, "truncated shell payload model survived");
  ++gate.model_mutants;
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(precredit));
  for (const std::size_t cutoff : {std::size_t{0}, std::size_t{1}, std::size_t{2}, std::size_t{64}})
    static_cast<void>(run(gate, *index, expected, 2, 8, WspdFrontMode::Pure, cutoff,
                          Q2AnchorMode::Individual, Q2CensusMode::Pairwise));
  u64 emissions = 0;
  const mhgp8::Q2CensusConsumer consumer = [&](const mhgp8::Q2Support&) { ++emissions; };
  for (const auto anchors : {Q2AnchorMode::SharedProduct, Q2AnchorMode::SharedAnchors})
    gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
        Q2CensusMode::Pairwise, consumer, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, anchors, 1)); });
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
      Q2CensusMode::Pairwise, consumer, Q2SiblingMode::Saturating, Q2WitnessOrder::GlobalDfs, Q2AnchorMode::Individual, 1)); });
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
      Q2CensusMode::Pairwise, consumer, Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst, Q2AnchorMode::Individual, 1)); });
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 0, 8, WspdFrontMode::Pure,
      Q2CensusMode::SharedBlocks, consumer, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, Q2AnchorMode::Individual, 1)); });
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
      Q2CensusMode::SharedBlocks, {}, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, Q2AnchorMode::Individual, 1)); });
  gate.require(emissions == 0, "invalid terminal Pool request emitted before rejection");
  const Points addition{{0, 0, 0}, {1, 0, 0}, {99, 0, 0}, {100, 0, 0}};
  const auto addition_index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(addition));
  const auto addition_all = oracle(gate, addition);
  const auto addition_expected = admissible(addition_all, 1);
  gate.require(addition_all.at({0, 3}).interior == std::vector<std::size_t>{1, 2},
               "additive local credit fixture lost its two disjoint witnesses");
  const Pair selected_support{1, 2};
  // This support is the sole residual of the proper Pool rectangle
  // {0,1} x {99,100} at K1; it is not a no-reduction fallback callback.
  Output outer;
  const auto nested_expected = admissible(addition_all, 2);
  const auto reentrant = mhgp8::run_wspd_q2_census(*addition_index, 1, 8, WspdFrontMode::Pure,
      Q2CensusMode::SharedBlocks, [&](const mhgp8::Q2Support& support) {
        const Pair pair = std::minmax(support.a_id, support.b_id);
        if (pair == selected_support) {
          static_cast<void>(run(gate, *addition_index, nested_expected, 2, 8, WspdFrontMode::Pure, 1));
          ++gate.reentrant_calls;
        }
        Payload payload{support.key, {support.interior.begin(), support.interior.end()},
                        {support.shell.begin(), support.shell.end()}};
        std::sort(payload.interior.begin(), payload.interior.end());
        std::sort(payload.shell.begin(), payload.shell.end());
        gate.require(outer.emplace(pair, std::move(payload)).second, "reentrant callback repeated an outer support");
      }, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, Q2AnchorMode::Individual, 2);
  gate.require(gate.reentrant_calls == 1 && outer == addition_expected && reentrant.pool_work.filtered_pairs == 3 &&
                   reentrant.pool_work.pair_roots == 1,
               "reentrant Pool callback changed borrowed payloads or did not enter the selected branch");
  struct CallbackFailure {};
  for (const auto anchors : {Q2AnchorMode::Individual, Q2AnchorMode::SharedProduct, Q2AnchorMode::SharedAnchors}) {
    emissions = 0;
    bool caught = false;
    try {
      static_cast<void>(mhgp8::run_wspd_q2_census(*addition_index, 1, 8, WspdFrontMode::Pure, Q2CensusMode::SharedBlocks,
          [&](const mhgp8::Q2Support& support) {
            ++emissions;
            if (Pair{std::min(support.a_id, support.b_id), std::max(support.a_id, support.b_id)} == selected_support)
              throw CallbackFailure{};
          }, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, anchors, 2));
    } catch (const CallbackFailure&) { caught = true; }
    gate.require(caught && emissions > 0, "targeted Pool callback exception was swallowed or never exercised");
    static_cast<void>(run(gate, *addition_index, addition_expected, 1, 8, WspdFrontMode::Pure, 2, anchors));
    ++gate.callback_failures;
  }
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_q2_terminal_pool_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    corpus(gate, fixtures(), 65535);
    targeted(gate);
    gate.require(gate.clouds == 22 && gate.runs > 3000 && gate.oracle_pairs > 5000 && gate.oracle_sites > 300000 &&
                     gate.supports > 10000 && gate.max_shell == 30 && gate.extra_shells > 100 &&
                     gate.selected_rectangles > 0 && gate.filtered_pairs > 0 && gate.residual_pairs > 0 &&
                     gate.factor_sites > 0 && gate.bands > 0 && gate.selected64 > 0 &&
                     gate.passthrough_rectangles > 0 && gate.unchanged_census > 100 && gate.reentrant_calls == 1 &&
                     gate.default_comparisons == 88 && gate.joint_runs > 300 && gate.pairwise_runs == 4 &&
                     gate.invalid_inputs == 6 && gate.callback_failures == 3 && gate.model_mutants == 5,
                 "terminal Pool qualification lost a non-vacuity floor");
    // 18-bit twins: three fixtures, each doubled by the 262143 - x reflection,
    // through the same run matrix (140 runs and 4 default comparisons per
    // cloud; 36 + 78 + 2346 oracle pairs per transform). Structural counts are
    // exact; the u16 max_shell of 30 is unchanged (the new clouds have at most
    // 8 cospherical sites, the cube corners); the large twin must filter, band
    // and select at cutoff 64.
    const Gate u16 = gate;
    const auto wide = fixtures_18bits();
    require_wide(gate, wide);
    corpus(gate, wide, mhgp8::coordinate_limit);
    gate.require(wide.size() == 3 && gate.clouds - u16.clouds == 6 && gate.runs - u16.runs == 840 &&
                     gate.oracle_pairs - u16.oracle_pairs == 4920 && gate.oracle_sites - u16.oracle_sites == 326424 &&
                     gate.default_comparisons - u16.default_comparisons == 24 && gate.supports > u16.supports &&
                     gate.max_shell == 30 && gate.joint_runs > u16.joint_runs && gate.unchanged_census > u16.unchanged_census &&
                     gate.selected_rectangles > u16.selected_rectangles && gate.residual_pairs > u16.residual_pairs &&
                     gate.filtered_pairs > u16.filtered_pairs && gate.bands > u16.bands && gate.selected64 > u16.selected64 &&
                     gate.passthrough_rectangles > u16.passthrough_rectangles && gate.extra_shells > u16.extra_shells &&
                     gate.factor_sites > u16.factor_sites && gate.pairwise_runs == u16.pairwise_runs &&
                     gate.reentrant_calls == u16.reentrant_calls && gate.invalid_inputs == u16.invalid_inputs &&
                     gate.callback_failures == u16.callback_failures && gate.model_mutants == u16.model_mutants,
                 "18-bit terminal Pool corpus lost a non-vacuity floor");
    std::cout << "mhgp8_q2_terminal_pool_gate passed checks=" << gate.checks << " clouds=" << gate.clouds
              << " runs=" << gate.runs << " oracle_pairs=" << gate.oracle_pairs << " oracle_sites=" << gate.oracle_sites
              << " supports=" << gate.supports << " extra_shells=" << gate.extra_shells << " max_shell=" << gate.max_shell
              << " selected_rectangles=" << gate.selected_rectangles << " filtered_pairs=" << gate.filtered_pairs
              << " residual_pairs=" << gate.residual_pairs << " factor_sites=" << gate.factor_sites << " bands=" << gate.bands
              << " selected64=" << gate.selected64 << " default_comparisons=" << gate.default_comparisons
              << " passthrough_rectangles=" << gate.passthrough_rectangles << " unchanged_census=" << gate.unchanged_census
              << " reentrant_calls=" << gate.reentrant_calls
              << " joint_runs=" << gate.joint_runs << " pairwise_runs=" << gate.pairwise_runs
              << " invalid_inputs=" << gate.invalid_inputs << " callback_failures=" << gate.callback_failures
              << " model_mutants=" << gate.model_mutants << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_q2_terminal_pool_gate failed: " << error.what() << '\n';
    return 1;
  }
}
