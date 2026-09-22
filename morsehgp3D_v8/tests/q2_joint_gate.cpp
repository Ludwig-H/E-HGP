#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "pipeline/wspd_q2_census.hpp"

namespace {

using mhgp8::Point3;
using mhgp8::Q2AnchorMode;
using mhgp8::Q2CensusMode;
using mhgp8::Q2SiblingMode;
using mhgp8::Q2WitnessOrder;
using mhgp8::WspdFrontMode;
using mhgp8::u64;
using Pair = std::pair<std::size_t, std::size_t>;

struct Payload {
  Pair pair;
  mhgp8::Q2BallKey key;
  std::vector<std::size_t> interior;
  std::vector<std::size_t> shell;
  bool operator==(const Payload&) const = default;
};
using Output = std::vector<Payload>;

struct Gate {
  u64 checks{}, clouds{}, oracle_pairs{}, oracle_sites{}, runs{}, supports{}, shell_sites{};
  u64 root_products{}, joint_tasks{}, splits_a{}, splits_b{}, joint_tests{}, joint_credits{};
  u64 joint_rejected_pairs{}, joint_accepted_pairs{}, handoffs{}, handoffs_after_credit{}, handoff_pair_mass{};
  u64 structural_splits{}, phase_switches{}, shared_anchor_runs{}, default_comparisons{}, invalid_inputs{}, callback_failures{}, model_mutants{};
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

// Explicit scalar oracle pattern, independent of every production bound,
// planner and traversal. u16 coordinates are promoted before all products.
std::int64_t h(const Point3& a, const Point3& b, const Point3& z) {
  std::int64_t result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis)
    result += (std::int64_t{z[axis]} - a[axis]) * (std::int64_t{b[axis]} - z[axis]);
  return result;
}

void sort_output(Output& output) {
  std::sort(output.begin(), output.end(), [](const auto& a, const auto& b) { return a.pair < b.pair; });
}

Output oracle(Gate& gate, std::span<const Point3> points) {
  Output result;
  for (std::size_t a = 0; a < points.size(); ++a) {
    for (std::size_t b = a + 1; b < points.size(); ++b) {
      Payload payload{{a, b}, {}, {}, {}};
      for (std::size_t axis = 0; axis < 3; ++axis) {
        payload.key.center_twice[axis] = static_cast<std::uint32_t>(points[a][axis]) + points[b][axis];
        const auto difference = std::int64_t{points[a][axis]} - points[b][axis];
        payload.key.diameter_squared += static_cast<u64>(difference * difference);
      }
      for (std::size_t z = 0; z < points.size(); ++z) {
        const auto value = h(points[a], points[b], points[z]);
        if (value > 0) payload.interior.push_back(z);
        if (value == 0) payload.shell.push_back(z);
        ++gate.oracle_sites;
      }
      result.push_back(std::move(payload));
      ++gate.oracle_pairs;
    }
  }
  return result;
}

Output accepted(const Output& all, unsigned kmax) {
  Output result;
  for (const auto& payload : all) if (payload.interior.size() < kmax) result.push_back(payload);
  return result;
}

auto work(const mhgp8::Q2CensusWork& w) {
  return std::array{w.query_build_point_visits, w.query_build_nodes, w.query_build_max_depth,
      w.input_descriptors, w.query_cover_visits, w.query_tasks, w.query_splits, w.witness_splits,
      w.count_root_starts, w.shared_splits_after_credit, w.cursor_advances, w.cursor_reuses,
      w.count_node_visits, w.count_bound_tests, w.count_point_tests, w.uniform_credited_pairs,
      w.uniform_rejected_pairs, w.uniform_accepted_pairs, w.consumed_witness_sites,
      w.frontier_restarts, w.payload_node_visits, w.payload_bound_tests, w.payload_point_tests,
      w.payload_interior_sites, w.payload_shell_sites, w.payload_supports};
}

auto joint_work(const mhgp8::Q2JointWork& w) {
  return std::array{w.root_products, w.tasks, w.splits_a, w.splits_b, w.witness_splits, w.bound_tests,
      w.cursor_advances, w.structural_splits, w.deferred_skips, w.phase_switches, w.consumed_witness_sites,
      w.credit_events, w.credited_pair_mass, w.splits_after_credit, w.singleton_handoffs, w.handoffs_after_credit,
      w.handoff_pair_mass, w.rejected_pairs, w.accepted_pairs, w.max_depth};
}

auto order_work(const mhgp8::Q2OrderWork& w) {
  return std::array{w.structural_splits, w.deferred_skips, w.anchor_skips, w.phase_switches};
}

auto sibling_work(const mhgp8::Q2SiblingWork& w) {
  return std::array{w.proposals, w.cardinality_skips, w.bound_tests, w.rejected_tasks,
                    w.rejected_pairs, w.rejected_after_credit};
}

struct Capture {
  Output output;
  mhgp8::WspdQ2CensusResult result;
};

Capture run(Gate& gate, const mhgp8::Q2CensusIndex& index, const Output& expected,
            unsigned kmax, unsigned separation, WspdFrontMode front, Q2SiblingMode sibling,
            Q2WitnessOrder order, Q2AnchorMode anchors, bool use_default = false) {
  Capture capture;
  const auto n = index.cloud().points().size();
  const auto nodes_before = index.spatial_nodes().data();
  const auto order_before = index.spatial_order().data();
  const auto visits_before = index.work().point_visits;
  u64 interiors = 0, shells = 0;
  const mhgp8::Q2CensusConsumer consume = [&](const mhgp8::Q2Support& support) {
    gate.require(support.a_id < n && support.b_id < n && support.a_id != support.b_id,
                 "joint census emitted invalid support IDs");
    Payload payload{{std::min(support.a_id, support.b_id), std::max(support.a_id, support.b_id)}, support.key,
                     {support.interior.begin(), support.interior.end()}, {support.shell.begin(), support.shell.end()}};
    for (auto* ids : {&payload.interior, &payload.shell}) {
      for (const auto id : *ids) gate.require(id < n, "joint census emitted an out-of-cloud payload site");
      std::sort(ids->begin(), ids->end());
      gate.require(std::adjacent_find(ids->begin(), ids->end()) == ids->end(), "joint census repeated a payload ID");
    }
    gate.require(std::binary_search(payload.shell.begin(), payload.shell.end(), support.a_id) &&
                     std::binary_search(payload.shell.begin(), payload.shell.end(), support.b_id),
                 "joint counting excluded an endpoint from the shell");
    interiors += payload.interior.size();
    shells += payload.shell.size();
    capture.output.push_back(std::move(payload));
  };
  capture.result = use_default
      ? mhgp8::run_wspd_q2_census(index, kmax, separation, front, Q2CensusMode::SharedBlocks, consume, sibling, order)
      : mhgp8::run_wspd_q2_census(index, kmax, separation, front, Q2CensusMode::SharedBlocks, consume, sibling, order, anchors);
  sort_output(capture.output);
  gate.require(capture.output == expected, "joint census changed exact supports, keys, interiors or complete shells");
  const auto& r = capture.result;
  const auto& c = r.census;
  const auto& j = r.joint_work;
  gate.require(r.front.active_lane_mask == 1 && c.candidate_pairs == r.front.work.residual_pair_mass[0] &&
                   c.accepted_pairs == expected.size() && c.accepted_pairs + c.rejected_pairs == c.candidate_pairs &&
                   c.candidate_pairs + r.front.work.rejected_pair_mass[0] == static_cast<u64>(n) * (n - 1) / 2,
               "joint census broke the front/census pair partition");
  gate.require(c.work.payload_supports == expected.size() && c.work.payload_interior_sites == interiors &&
                   c.work.payload_shell_sites == shells && c.work.frontier_restarts == 0 &&
                   c.work.cursor_reuses == 2 * c.work.query_splits,
               "joint census payload or singleton-continuation ledger failed");
  gate.require(c.work.query_build_nodes == 0 && c.work.query_build_point_visits == 0 && c.work.query_cover_visits == 0 &&
                   c.query_index_ms == 0 && nodes_before == index.spatial_nodes().data() &&
                   order_before == index.spatial_order().data() && visits_before == index.work().point_visits,
               "joint census rebuilt factors, queries or the shared index");
  if (anchors == Q2AnchorMode::Individual || use_default) {
    gate.require(joint_work(j) == std::array<u64, 20>{} && c.work.count_root_starts == r.anchor_queries &&
                     c.work.query_tasks == r.anchor_queries + 2 * c.work.query_splits,
                 "Individual mode paid joint work or changed its root/query ledger");
  } else {
    gate.require(j.root_products == r.input_rectangles && c.work.count_root_starts == j.root_products &&
                     j.tasks == j.root_products + 2 * (j.splits_a + j.splits_b) &&
                     c.work.query_tasks == j.singleton_handoffs + 2 * c.work.query_splits,
                 "joint task or handoff silently restarted a root or lost a child");
    gate.require(j.rejected_pairs + j.accepted_pairs + j.handoff_pair_mass == c.candidate_pairs &&
                     j.rejected_pairs <= c.rejected_pairs && j.accepted_pairs <= c.accepted_pairs &&
                     j.singleton_handoffs <= j.handoff_pair_mass && j.handoffs_after_credit <= j.singleton_handoffs &&
                     j.splits_after_credit <= j.splits_a + j.splits_b && j.credit_events <= j.bound_tests && j.max_depth <= 2 * mhgp8::max_index_depth,
                 "joint rejected/accepted/handoff mass or credit ledger failed");
    if (anchors == Q2AnchorMode::SharedAnchors) {
      gate.require(j.splits_b == 0 && j.singleton_handoffs <= r.anchor_queries,
                   "SharedAnchors split B or handed off an original anchor more than once");
      ++gate.shared_anchor_runs;
    }
    if (order == Q2WitnessOrder::GlobalDfs) {
      gate.require(j.structural_splits == 0 && j.deferred_skips == 0 && j.phase_switches == 0,
                   "global joint traversal paid complemented-order work");
    } else {
      gate.require(j.deferred_skips <= j.tasks && j.phase_switches <= j.tasks && j.structural_splits <= mhgp8::max_index_depth * j.tasks,
                   "joint complemented-order structure exceeded its single deferred-path envelope");
    }
    gate.root_products += j.root_products;
    gate.joint_tasks += j.tasks;
    gate.splits_a += j.splits_a;
    gate.splits_b += j.splits_b;
    gate.joint_tests += j.bound_tests;
    gate.joint_credits += j.credit_events;
    gate.joint_rejected_pairs += j.rejected_pairs;
    gate.joint_accepted_pairs += j.accepted_pairs;
    gate.handoffs += j.singleton_handoffs;
    gate.handoffs_after_credit += j.handoffs_after_credit;
    gate.handoff_pair_mass += j.handoff_pair_mass;
    gate.structural_splits += j.structural_splits;
    gate.phase_switches += j.phase_switches;
  }
  if (sibling == Q2SiblingMode::Disabled) {
    gate.require(sibling_work(r.sibling_work) == std::array<u64, 6>{}, "Disabled sibling mode paid extra work");
  } else {
    gate.require(r.sibling_work.proposals == 2 * c.work.query_splits &&
                     r.sibling_work.bound_tests + r.sibling_work.cardinality_skips == r.sibling_work.proposals &&
                     r.sibling_work.rejected_pairs <= c.rejected_pairs,
                 "joint census incorrectly applied a sibling certificate before singleton handoff");
  }
  if (order == Q2WitnessOrder::GlobalDfs)
    gate.require(order_work(r.order_work) == std::array<u64, 4>{}, "GlobalDfs singleton path paid complemented work");
  for (const double time : {r.total_ms, c.total_ms, c.count_ms, c.payload_ms})
    gate.require(std::isfinite(time) && time >= 0, "joint census reported invalid timings");
  gate.supports += c.accepted_pairs;
  gate.shell_sites += shells;
  ++gate.runs;
  return capture;
}

void same_domain(Gate& gate, const Capture& a, const Capture& b) {
  gate.require(a.output == b.output && a.result.anchor_queries == b.result.anchor_queries &&
                   a.result.input_rectangles == b.result.input_rectangles &&
                   a.result.census.candidate_pairs == b.result.census.candidate_pairs &&
                   a.result.front.work.product_visits == b.result.front.work.product_visits &&
                   a.result.front.work.residual_pair_mass == b.result.front.work.residual_pair_mass &&
                   a.result.front.work.rejected_pair_mass == b.result.front.work.rejected_pair_mass,
               "anchor sharing changed the upstream domain or output incidences");
}

std::vector<Point3> six_sites() {
  return {{100, 0, 0}, {100, 4, 0}, {0, 1, 0}, {0, 2, 0}, {0, 3, 0}, {50, 2, 0}};
}

std::vector<std::vector<Point3>> fixtures() {
  std::vector<std::vector<Point3>> result{
      {{7, 11, 13}}, {{0, 0, 0}, {65535, 65535, 65535}},
      {{100, 0, 0}, {101, 0, 0}, {0, 0, 0}, {1, 0, 0}},
      {{100, 0, 0}, {100, 1, 0}, {0, 0, 0}, {0, 1, 0}, {0, 2, 0}, {0, 3, 0}},
      {{1000, 0, 0}, {1000, 1, 0}, {0, 0, 0}, {0, 1, 0}, {500, 500, 0}}, six_sites()};
  std::vector<Point3> cube;
  for (unsigned bits = 0; bits < 8; ++bits)
    cube.push_back({static_cast<std::uint16_t>((bits & 1U) * 2),
                   static_cast<std::uint16_t>(((bits >> 1U) & 1U) * 2),
                   static_cast<std::uint16_t>(((bits >> 2U) & 1U) * 2)});
  result.push_back(cube);
  for (std::uint32_t seed : {3U, 37U}) {
    auto state = seed;
    const auto next = [&]() { state = state * 1664525U + 1013904223U; return static_cast<std::uint16_t>(state >> 16U); };
    std::vector<Point3> random;
    for (unsigned i = 0; i < 17; ++i) random.push_back({static_cast<std::uint16_t>(i * 251 + seed), next(), next()});
    result.push_back(random);
  }
  return result;
}

void corpus(Gate& gate) {
  for (const auto& base : fixtures()) {
    for (unsigned transform = 0; transform < 2; ++transform) {
      auto points = base;
      if (transform != 0) {
        for (auto& point : points) point = {point.z, static_cast<std::uint16_t>(65535U - point.x), point.y};
        std::reverse(points.begin(), points.end());
      }
      const auto all = oracle(gate, points);
      const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
      ++gate.clouds;
      for (const unsigned kmax : {1U, 2U, 5U, 10U}) {
        const auto expected = accepted(all, kmax);
        for (const unsigned s : {8U, 10U, 12U}) {
          for (const auto front : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
            for (const auto sibling : {Q2SiblingMode::Disabled, Q2SiblingMode::Saturating}) {
              for (const auto order : {Q2WitnessOrder::GlobalDfs, Q2WitnessOrder::ComplementFirst}) {
                const auto individual = run(gate, *index, expected, kmax, s, front, sibling, order, Q2AnchorMode::Individual);
                for (const auto anchors : {Q2AnchorMode::SharedProduct, Q2AnchorMode::SharedAnchors}) {
                  const auto shared = run(gate, *index, expected, kmax, s, front, sibling, order, anchors);
                  same_domain(gate, individual, shared);
                }
                if (s == 8 && front == WspdFrontMode::Pure && sibling == Q2SiblingMode::Disabled) {
                  const auto implicit = run(gate, *index, expected, kmax, s, front, sibling, order,
                                             Q2AnchorMode::Individual, true);
                  same_domain(gate, individual, implicit);
                  gate.require(work(individual.result.census.work) == work(implicit.result.census.work) &&
                                   order_work(individual.result.order_work) == order_work(implicit.result.order_work) &&
                                   sibling_work(individual.result.sibling_work) == sibling_work(implicit.result.sibling_work),
                               "default anchor mode changed an existing work counter");
                  ++gate.default_comparisons;
                }
              }
            }
          }
        }
      }
    }
  }
}

void targeted(Gate& gate) {
  const auto points = six_sites();
  const auto all = oracle(gate, points);
  const std::array<std::size_t, 6> expected_depths{1, 2, 3, 3, 2, 1};
  std::size_t slot = 0;
  for (std::size_t a = 0; a < 2; ++a) for (std::size_t b = 2; b < 5; ++b) {
    const auto found = std::find_if(all.begin(), all.end(), [&](const auto& support) { return support.pair == Pair{a, b}; });
    gate.require(found != all.end() && found->interior.size() == expected_depths[slot++] &&
                     h(points[a], points[b], points[5]) >= 2498,
                 "six-site fixture lost its declared cross depths or common strict witness");
  }
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const auto expected = accepted(all, 2);
  for (const auto anchors : {Q2AnchorMode::SharedProduct, Q2AnchorMode::SharedAnchors}) {
    const auto k1 = run(gate, *index, accepted(all, 1), 1, 12, WspdFrontMode::Pure,
                        Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst, anchors);
    gate.require(k1.result.joint_work.rejected_pairs >= 6 && k1.result.joint_work.credit_events > 0,
                 "six-site fixture did not exercise a joint saturation certificate");
    const auto k2 = run(gate, *index, expected, 2, 12, WspdFrontMode::Pure,
                        Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst, anchors);
    gate.require(k2.result.joint_work.handoffs_after_credit > 0,
                 "six-site fixture lost its singleton continuation after joint credit");
    const auto cross_count = std::count_if(k2.output.begin(), k2.output.end(),
                                          [](const auto& p) { return p.pair.first < 2 && p.pair.second >= 2 && p.pair.second < 5; });
    gate.require(cross_count == 2, "six-site K2 fixture did not preserve exactly its two admissible cross supports");
  }
  auto restarted = expected;
  restarted.erase(std::remove_if(restarted.begin(), restarted.end(),
      [](const auto& p) { return p.pair == Pair{0, 2} || p.pair == Pair{1, 4}; }), restarted.end());
  gate.require(restarted != expected, "restarting Z with retained joint credit model survived");
  ++gate.model_mutants;

  const std::vector<Point3> line{{100, 0, 0}, {101, 0, 0}, {0, 0, 0}, {1, 0, 0}};
  gate.require(h(line[1], line[3], line[0]) > 0 && h(line[1], line[3], line[2]) < 0,
               "excluding all A lost its physical strict-witness counterexample");
  const auto line_all = oracle(gate, line);
  const auto forbidden = std::find_if(line_all.begin(), line_all.end(), [](const auto& p) { return p.pair == Pair{1, 3}; });
  gate.require(forbidden != line_all.end() && forbidden->interior == std::vector<std::size_t>{0},
               "excluding all A no longer changes the (101,1) depth");
  ++gate.model_mutants;

  const std::vector<Point3> boundary{{1000, 0, 0}, {1000, 1, 0}, {0, 0, 0}, {0, 1, 0}, {500, 500, 0}};
  gate.require(h(boundary[0], boundary[2], boundary[4]) == 0 && h(boundary[1], boundary[3], boundary[4]) > 0,
               "nonstrict joint-credit model lost its mixed boundary/interior fixture");
  const auto boundary_expected = accepted(oracle(gate, boundary), 1);
  gate.require(std::any_of(boundary_expected.begin(), boundary_expected.end(),
                          [](const auto& p) { return p.pair == Pair{0, 2}; }),
               "joint Hmin=0 fixture lost its admissible cross support");
  ++gate.model_mutants;
  auto missing_shell = expected;
  gate.require(!missing_shell.empty(), "shell mutant lost its support");
  missing_shell.front().shell.pop_back();
  gate.require(missing_shell != expected, "joint payload shell truncation model survived");
  ++gate.model_mutants;
  auto duplicate = expected;
  duplicate.push_back(expected.front());
  sort_output(duplicate);
  gate.require(duplicate != expected, "overlapping joint children duplicated a support without detection");
  ++gate.model_mutants;

  u64 emissions = 0;
  const mhgp8::Q2CensusConsumer consumer = [&](const mhgp8::Q2Support&) { ++emissions; };
  for (const auto anchors : {Q2AnchorMode::SharedProduct, Q2AnchorMode::SharedAnchors})
    gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 12, WspdFrontMode::Pure,
        Q2CensusMode::Pairwise, consumer, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, anchors)); },
        "Pairwise accepted a joint anchor mode");
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 12, WspdFrontMode::Pure,
      Q2CensusMode::SharedBlocks, consumer, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, static_cast<Q2AnchorMode>(99))); },
      "invalid anchor mode was accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 12, WspdFrontMode::Pure,
      Q2CensusMode::SharedBlocks, {}, Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst, Q2AnchorMode::SharedProduct)); },
      "joint census accepted an empty callback");
  gate.require(emissions == 0, "invalid joint request emitted before rejection");
  struct CallbackFailure {};
  for (const auto anchors : {Q2AnchorMode::SharedProduct, Q2AnchorMode::SharedAnchors}) {
    for (const auto order : {Q2WitnessOrder::GlobalDfs, Q2WitnessOrder::ComplementFirst}) {
      emissions = 0;
      bool caught = false;
      try {
        static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 12, WspdFrontMode::Pure,
            Q2CensusMode::SharedBlocks, [&](const mhgp8::Q2Support&) {
              if (++emissions == 2) throw CallbackFailure{};
            }, Q2SiblingMode::Saturating, order, anchors));
      } catch (const CallbackFailure&) { caught = true; }
      gate.require(caught && emissions == 2, "joint callback exception was swallowed or traversal continued");
      static_cast<void>(run(gate, *index, expected, 2, 12, WspdFrontMode::Pure,
                            Q2SiblingMode::Saturating, order, anchors));
      ++gate.callback_failures;
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_q2_joint_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    corpus(gate);
    targeted(gate);
    gate.require(gate.clouds == 18 && gate.runs > 5328 && gate.oracle_pairs > 700 && gate.oracle_sites > 10000 &&
                     gate.supports > 10000 && gate.shell_sites >= 2 * gate.supports && gate.root_products > 0 &&
                     gate.joint_tasks > gate.root_products && gate.splits_a > 0 && gate.splits_b > 0 && gate.joint_tests > 0 &&
                     gate.joint_credits > 0 && gate.joint_rejected_pairs > 0 && gate.handoffs > 0 &&
                     gate.handoffs_after_credit > 0 && gate.handoff_pair_mass >= gate.handoffs &&
                     gate.structural_splits > 0 && gate.shared_anchor_runs > 1728 && gate.default_comparisons == 144 &&
                     gate.invalid_inputs == 4 && gate.callback_failures == 4 && gate.model_mutants == 5,
                 "joint census qualification lost a non-vacuity floor");
    std::cout << "mhgp8_q2_joint_gate passed checks=" << gate.checks << " clouds=" << gate.clouds
              << " oracle_pairs=" << gate.oracle_pairs << " oracle_sites=" << gate.oracle_sites << " runs=" << gate.runs
              << " supports=" << gate.supports << " shell_sites=" << gate.shell_sites << " root_products=" << gate.root_products
              << " joint_tasks=" << gate.joint_tasks << " splits_a=" << gate.splits_a << " splits_b=" << gate.splits_b
              << " joint_tests=" << gate.joint_tests << " joint_credits=" << gate.joint_credits
              << " joint_rejected_pairs=" << gate.joint_rejected_pairs << " joint_accepted_pairs=" << gate.joint_accepted_pairs
              << " handoffs=" << gate.handoffs << " handoffs_after_credit=" << gate.handoffs_after_credit
              << " handoff_pair_mass=" << gate.handoff_pair_mass << " structural_splits=" << gate.structural_splits
              << " phase_switches=" << gate.phase_switches << " shared_anchor_runs=" << gate.shared_anchor_runs
              << " default_comparisons=" << gate.default_comparisons
              << " invalid_inputs=" << gate.invalid_inputs << " callback_failures=" << gate.callback_failures
              << " model_mutants=" << gate.model_mutants << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_q2_joint_gate failed: " << error.what() << '\n';
    return 1;
  }
}
