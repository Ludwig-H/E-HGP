#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "pipeline/wspd_q2_census.hpp"

namespace {

using mhgp9::gen::Point3;
using mhgp9::gen::Q2CensusMode;
using mhgp9::gen::Q2SiblingMode;
using mhgp9::gen::Q2WitnessOrder;
using mhgp9::gen::WspdFrontMode;
using mhgp9::gen::u64;
using Pair = std::pair<std::size_t, std::size_t>;

struct Payload {
  Pair pair;
  mhgp9::gen::Q2BallKey key;
  std::vector<std::size_t> interior;
  std::vector<std::size_t> shell;
  bool operator==(const Payload&) const = default;
};
using Output = std::vector<Payload>;

struct Gate {
  u64 checks{}, clouds{}, oracle_pairs{}, oracle_sites{}, runs{}, supports{}, shell_sites{};
  u64 structural_splits{}, deferred_skips{}, anchor_skips{}, phase_switches{};
  u64 default_comparisons{}, transformed_clouds{}, invalid_inputs{}, callback_failures{}, model_mutants{};
  u64 fixture_global_splits{}, fixture_complement_splits{}, fixture_universal_witnesses{};
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

// Same explicit scalar-oracle pattern as q2_sibling_gate, not a production
// predicate: three promoted integer products, independently of box bounds.
std::int64_t h(const Point3& a, const Point3& b, const Point3& z) {
  std::int64_t result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    result += (std::int64_t{z[axis]} - a[axis]) * (std::int64_t{b[axis]} - z[axis]);
  }
  return result;
}

u64 scalar_box_diagonal_squared(std::span<const Point3> points) {
  u64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    mhgp9::gen::Coordinate low = points.front()[axis], high = low;
    for (const auto& point : points) {
      low = std::min(low, point[axis]);
      high = std::max(high, point[axis]);
    }
    const auto delta = std::int64_t{high} - low;
    result += static_cast<u64>(delta * delta);
  }
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

auto work(const mhgp9::gen::Q2CensusWork& w) {
  return std::array{w.query_build_point_visits, w.query_build_nodes, w.query_build_max_depth,
      w.input_descriptors, w.query_cover_visits, w.query_tasks, w.query_splits, w.witness_splits,
      w.count_root_starts, w.shared_splits_after_credit, w.cursor_advances, w.cursor_reuses,
      w.count_node_visits, w.count_bound_tests, w.count_point_tests, w.uniform_credited_pairs,
      w.uniform_rejected_pairs, w.uniform_accepted_pairs, w.consumed_witness_sites,
      w.frontier_restarts, w.payload_node_visits, w.payload_bound_tests, w.payload_point_tests,
      w.payload_interior_sites, w.payload_shell_sites, w.payload_supports};
}

auto order_work(const mhgp9::gen::Q2OrderWork& w) {
  return std::array{w.structural_splits, w.deferred_skips, w.anchor_skips, w.phase_switches};
}

auto sibling_work(const mhgp9::gen::Q2SiblingWork& w) {
  return std::array{w.proposals, w.cardinality_skips, w.bound_tests, w.rejected_tasks,
                    w.rejected_pairs, w.rejected_after_credit};
}

struct Capture {
  Output output;
  mhgp9::gen::WspdQ2CensusResult result;
};

Capture run(Gate& gate, const mhgp9::gen::Q2CensusIndex& index, const Output& expected,
            unsigned kmax, unsigned separation, WspdFrontMode front_mode, Q2SiblingMode sibling_mode,
            Q2WitnessOrder order, bool implicit_order = false) {
  Capture capture;
  const auto n = index.cloud().points().size();
  const auto index_nodes = index.spatial_nodes().data();
  const auto index_order = index.spatial_order().data();
  const auto index_visits = index.work().point_visits;
  u64 interiors = 0, shells = 0;
  const mhgp9::gen::Q2CensusConsumer consume = [&](const mhgp9::gen::Q2Support& support) {
    gate.require(support.a_id < n && support.b_id < n && support.a_id != support.b_id,
                 "witness order emitted invalid original support IDs");
    Payload payload{{std::min(support.a_id, support.b_id), std::max(support.a_id, support.b_id)}, support.key,
                     {support.interior.begin(), support.interior.end()}, {support.shell.begin(), support.shell.end()}};
    for (auto* ids : {&payload.interior, &payload.shell}) {
      for (const auto id : *ids) gate.require(id < n, "witness order emitted an out-of-cloud payload site");
      std::sort(ids->begin(), ids->end());
      gate.require(std::adjacent_find(ids->begin(), ids->end()) == ids->end(), "witness order duplicated a payload site");
    }
    gate.require(std::binary_search(payload.shell.begin(), payload.shell.end(), support.a_id) &&
                     std::binary_search(payload.shell.begin(), payload.shell.end(), support.b_id),
                 "count-only anchor exclusion leaked into shell collection");
    interiors += payload.interior.size();
    shells += payload.shell.size();
    capture.output.push_back(std::move(payload));
  };
  capture.result = implicit_order
      ? mhgp9::gen::run_wspd_q2_census(index, kmax, separation, front_mode, Q2CensusMode::SharedBlocks, consume, sibling_mode)
      : mhgp9::gen::run_wspd_q2_census(index, kmax, separation, front_mode, Q2CensusMode::SharedBlocks, consume, sibling_mode, order);
  sort_output(capture.output);
  gate.require(capture.output == expected, "witness order changed exact supports, keys, interiors or complete shells");
  const auto& r = capture.result;
  const auto& c = r.census;
  const auto& o = r.order_work;
  gate.require(r.front.active_lane_mask == 1 && c.candidate_pairs == r.front.work.residual_pair_mass[0] &&
                   c.accepted_pairs == expected.size() && c.accepted_pairs + c.rejected_pairs == c.candidate_pairs &&
                   c.candidate_pairs + r.front.work.rejected_pair_mass[0] == static_cast<u64>(n) * (n - 1) / 2,
               "witness order broke the front/census pair partition");
  gate.require(c.work.payload_supports == expected.size() && c.work.payload_interior_sites == interiors &&
                   c.work.payload_shell_sites == shells && c.work.count_root_starts == r.anchor_queries &&
                   c.work.query_tasks == r.anchor_queries + 2 * c.work.query_splits &&
                   c.work.cursor_reuses == 2 * c.work.query_splits && c.work.frontier_restarts == 0,
               "witness order broke the query, count-continuation or payload ledger");
  gate.require(c.work.query_build_nodes == 0 && c.work.query_build_point_visits == 0 && c.work.query_cover_visits == 0 &&
                   c.query_index_ms == 0 && index_nodes == index.spatial_nodes().data() &&
                   index_order == index.spatial_order().data() && index_visits == index.work().point_visits,
               "witness order rebuilt a query factor or the shared index");
  if (order == Q2WitnessOrder::GlobalDfs || implicit_order) {
    gate.require(order_work(o) == std::array<u64, 4>{}, "GlobalDfs paid complement-order sidecar work");
  } else {
    gate.require(o.deferred_skips <= c.work.query_tasks && o.anchor_skips <= c.work.query_tasks &&
                     o.phase_switches <= c.work.query_tasks && o.structural_splits <= 2 * mhgp9::gen::max_index_depth * c.work.query_tasks,
                 "complement-order structural work exceeded its two-path/per-task envelope");
    gate.structural_splits += o.structural_splits;
    gate.deferred_skips += o.deferred_skips;
    gate.anchor_skips += o.anchor_skips;
    gate.phase_switches += o.phase_switches;
  }
  if (sibling_mode == Q2SiblingMode::Disabled) {
    gate.require(sibling_work(r.sibling_work) == std::array<u64, 6>{}, "Disabled siblings paid sidecar work");
  } else {
    gate.require(r.sibling_work.proposals == 2 * c.work.query_splits &&
                     r.sibling_work.bound_tests + r.sibling_work.cardinality_skips == r.sibling_work.proposals &&
                     r.sibling_work.rejected_pairs <= c.rejected_pairs,
                 "complement order broke the independent sibling certificate ledger");
  }
  for (const double time : {r.total_ms, c.total_ms, c.count_ms, c.payload_ms})
    gate.require(std::isfinite(time) && time >= 0, "witness order reported invalid timings");
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
               "witness order changed the upstream domain or output incidences");
}

std::vector<std::vector<Point3>> fixtures() {
  std::vector<std::vector<Point3>> result{
      {{7, 11, 13}}, {{0, 0, 0}, {65535, 65535, 65535}},
      {{1000, 0, 0}, {0, 1, 0}, {0, 0, 0}},
      {{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}},
      {{1, 0, 0}, {5, 0, 0}, {6, 0, 0}, {10, 0, 0}, {11, 0, 0}, {1000, 0, 0}}};
  std::vector<Point3> cube;
  for (unsigned bits = 0; bits < 8; ++bits)
    cube.push_back({static_cast<mhgp9::gen::Coordinate>((bits & 1U) * 2),
                   static_cast<mhgp9::gen::Coordinate>(((bits >> 1U) & 1U) * 2),
                   static_cast<mhgp9::gen::Coordinate>(((bits >> 2U) & 1U) * 2)});
  result.push_back(cube);
  for (std::uint32_t seed : {3U, 37U}) {
    auto state = seed;
    const auto next = [&]() { state = state * 1664525U + 1013904223U; return static_cast<mhgp9::gen::Coordinate>(state >> 16U); };
    std::vector<Point3> random;
    for (unsigned i = 0; i < 17; ++i) random.push_back({static_cast<mhgp9::gen::Coordinate>(i * 251 + seed), next(), next()});
    result.push_back(random);
  }
  return result;
}

// 18-bit twins (coordinate_limit = 262143) of the u16 corner fixtures: the
// diagonal pair and a full-extent cube, plus two random clouds drawn on 18
// bits (state >> 14). They pass through the same scalar oracle and the same
// runs as the u16 corpus, reflected by 262143 - x, and carry their own floors;
// the u16 fixtures and their pinned floors are untouched.
std::vector<std::vector<Point3>> fixtures_18bits() {
  constexpr mhgp9::gen::Coordinate limit = mhgp9::gen::coordinate_limit;
  std::vector<std::vector<Point3>> result{{{0, 0, 0}, {limit, limit, limit}}};
  std::vector<Point3> cube;
  for (unsigned bits = 0; bits < 8; ++bits)
    cube.push_back({(bits & 1U) != 0 ? limit : 0, (bits & 2U) != 0 ? limit : 0, (bits & 4U) != 0 ? limit : 0});
  result.push_back(cube);
  for (std::uint32_t seed : {3U, 37U}) {
    auto state = seed;
    const auto next = [&]() { state = state * 1664525U + 1013904223U; return static_cast<mhgp9::gen::Coordinate>(state >> 14U); };
    std::vector<Point3> random;
    for (unsigned i = 0; i < 17; ++i) random.push_back({static_cast<mhgp9::gen::Coordinate>(i * 15413 + seed), next(), next()});
    result.push_back(random);
  }
  return result;
}

void require_wide(Gate& gate, const std::vector<std::vector<Point3>>& fixtures) {
  for (const auto& fixture : fixtures) {
    mhgp9::gen::Coordinate widest = 0;
    for (const auto& point : fixture) widest = std::max({widest, point.x, point.y, point.z});
    gate.require(widest > 65535 && widest <= mhgp9::gen::coordinate_limit,
                 "18-bit fixture does not leave the historical u16 range or exceeds coordinate_limit");
  }
}

void corpus(Gate& gate, const std::vector<std::vector<Point3>>& fixtures, mhgp9::gen::Coordinate reflect) {
  for (const auto& base : fixtures) {
    for (unsigned transform = 0; transform < 2; ++transform) {
      auto points = base;
      if (transform != 0) {
        for (auto& point : points) point = {point.z, static_cast<mhgp9::gen::Coordinate>(reflect - point.x), point.y};
        std::reverse(points.begin(), points.end());
        ++gate.transformed_clouds;
      }
      const auto all = oracle(gate, points);
      const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
      ++gate.clouds;
      for (const unsigned kmax : {1U, 2U, 5U, 10U}) {
        const auto expected = accepted(all, kmax);
        for (const unsigned s : {8U, 10U, 12U}) {
          for (const auto front : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
            for (const auto sibling : {Q2SiblingMode::Disabled, Q2SiblingMode::Saturating}) {
              const auto global = run(gate, *index, expected, kmax, s, front, sibling, Q2WitnessOrder::GlobalDfs);
              const auto complement = run(gate, *index, expected, kmax, s, front, sibling, Q2WitnessOrder::ComplementFirst);
              same_domain(gate, global, complement);
              if (s == 8) {
                const auto implicit = run(gate, *index, expected, kmax, s, front, sibling, Q2WitnessOrder::GlobalDfs, true);
                same_domain(gate, global, implicit);
                gate.require(work(global.result.census.work) == work(implicit.result.census.work) &&
                                 sibling_work(global.result.sibling_work) == sibling_work(implicit.result.sibling_work),
                             "default witness order changed existing generic or sibling counters");
                ++gate.default_comparisons;
              }
            }
          }
        }
      }
    }
  }
}

void spatial_counterexample(Gate& gate) {
  std::vector<Point3> points;
  for (unsigned x = 0; x < 4; ++x) for (unsigned y = 0; y < 4; ++y) for (unsigned z = 0; z < 4; ++z)
    points.push_back({static_cast<mhgp9::gen::Coordinate>(x), static_cast<mhgp9::gen::Coordinate>(y), static_cast<mhgp9::gen::Coordinate>(z)});
  const std::size_t anchor = points.size();
  points.push_back({1000, 1000, 1000});
  for (unsigned x = 997; x < 1000; ++x) for (unsigned y = 998; y < 1000; ++y) for (unsigned z = 998; z < 1000; ++z)
    points.push_back({static_cast<mhgp9::gen::Coordinate>(x), static_cast<mhgp9::gen::Coordinate>(y), static_cast<mhgp9::gen::Coordinate>(z)});
  gate.require(points.size() == 77 && anchor == 64, "3D deferral fixture lost its declared populations");
  for (std::size_t witness = anchor + 1; witness < points.size(); ++witness) {
    for (std::size_t b = 0; b < anchor; ++b)
      gate.require(h(points[anchor], points[b], points[witness]) > 0, "3D witness is not universal over the B sites");
    ++gate.fixture_universal_witnesses;
  }
  // Independent no-anchor-exclusion model: a itself forces Hmin=0 in
  // {a} union W, yet a real W point gives H>0. Its squared box diagonal
  // is 3^2+2^2+2^2=17, below B's 3^2+3^2+3^2=27. Deferral alone leaves
  // the existing diameter policy choosing a B split, not exposing W first.
  const auto population = std::span<const Point3>(points);
  const auto b_diagonal = scalar_box_diagonal_squared(population.first(anchor));
  const auto mixed_diagonal = scalar_box_diagonal_squared(population.subspan(anchor));
  gate.require(h(points[anchor], points[0], points[anchor]) == 0 &&
                   h(points[anchor], points[0], points[anchor + 1]) > 0 &&
                   mixed_diagonal == 17 && b_diagonal == 27 && mixed_diagonal < b_diagonal,
               "deferring B without structural anchor exclusion model became vacuous");
  ++gate.model_mutants;
  const auto expected = accepted(oracle(gate, points), 10);
  gate.require(std::none_of(expected.begin(), expected.end(), [&](const auto& payload) {
    return payload.pair.first < anchor && payload.pair.second == anchor;
  }), "universal 3D witnesses did not reject every a-times-B support in the scalar oracle");
  const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
  for (const auto sibling : {Q2SiblingMode::Disabled, Q2SiblingMode::Saturating}) {
    const auto global = run(gate, *index, expected, 10, 12, WspdFrontMode::Pure, sibling, Q2WitnessOrder::GlobalDfs);
    const auto complement = run(gate, *index, expected, 10, 12, WspdFrontMode::Pure, sibling, Q2WitnessOrder::ComplementFirst);
    same_domain(gate, global, complement);
    gate.require(complement.result.census.work.query_splits < global.result.census.work.query_splits &&
                     complement.result.order_work.structural_splits > 0 && complement.result.order_work.anchor_skips > 0,
                 "3D complemented traversal failed to reduce aggregate B fragmentation while excluding the anchor");
    gate.fixture_global_splits += global.result.census.work.query_splits;
    gate.fixture_complement_splits += complement.result.census.work.query_splits;
  }
}

void models_and_rejections(Gate& gate) {
  // Pure continuation model, not adoption of fabricated product handles.
  // Original B ranks are [2,6), anchor rank 1; complement prefix 0,6 was
  // already consumed. Resetting B to right child [4,6) at cursor 7 loses
  // original-B witnesses 2,3: they are neither revisited nor in phase B.
  const std::vector<Point3> points{{0, 0, 0}, {1, 0, 0}, {5, 0, 0}, {6, 0, 0},
                                  {10, 0, 0}, {11, 0, 0}, {15, 0, 0}, {16, 0, 0}, {17, 0, 0}};
  const std::vector<std::size_t> suffix{7, 8, 2, 3, 4, 5};
  const std::vector<std::size_t> reset_suffix{7, 8, 4, 5};
  const auto count = [&](const auto& ids) {
    unsigned result = 0;
    for (const auto id : ids) result += static_cast<unsigned>(h(points[1], points[4], points[id]) > 0);
    return result;
  };
  gate.require(count(suffix) == 2 && count(reset_suffix) == 0,
               "resetting the original deferred B on a query child model failed to lose strict witnesses");
  ++gate.model_mutants;
  const auto inherited = count(std::array<std::size_t, 1>{2});
  const auto remaining = count(std::array<std::size_t, 1>{3});
  gate.require(inherited == 1 && remaining == 1 && inherited + remaining >= 2 && remaining < 2,
               "discarded inherited count model became vacuous");
  ++gate.model_mutants;
  const auto all = oracle(gate, points);
  const auto expected = accepted(all, 2);
  auto missing_anchor = expected;
  gate.require(!missing_anchor.empty(), "shell exclusion mutant lost its support");
  auto& victim = missing_anchor.front();
  victim.shell.erase(std::remove(victim.shell.begin(), victim.shell.end(), victim.pair.first), victim.shell.end());
  gate.require(missing_anchor != expected, "anchor exclusion escaped counting and corrupted shell without detection");
  ++gate.model_mutants;
  auto duplicate = expected;
  duplicate.push_back(expected.front());
  sort_output(duplicate);
  gate.require(duplicate != expected, "restarted phase duplicated support without detection");
  ++gate.model_mutants;

  const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
  u64 emissions = 0;
  const mhgp9::gen::Q2CensusConsumer consumer = [&](const mhgp9::gen::Q2Support&) { ++emissions; };
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
      Q2CensusMode::Pairwise, consumer, Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst)); },
      "Pairwise accepted ComplementFirst");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
      Q2CensusMode::SharedBlocks, consumer, Q2SiblingMode::Disabled, static_cast<Q2WitnessOrder>(99))); },
      "invalid witness order was accepted");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
      Q2CensusMode::SharedBlocks, {}, Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst)); },
      "empty complement-order callback was accepted");
  gate.require(emissions == 0, "invalid witness order request emitted before rejection");
  struct CallbackFailure {};
  for (const auto sibling : {Q2SiblingMode::Disabled, Q2SiblingMode::Saturating}) {
    emissions = 0;
    bool caught = false;
    try {
      static_cast<void>(mhgp9::gen::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
          Q2CensusMode::SharedBlocks, [&](const mhgp9::gen::Q2Support&) {
            if (++emissions == 2) throw CallbackFailure{};
          }, sibling, Q2WitnessOrder::ComplementFirst));
    } catch (const CallbackFailure&) { caught = true; }
    gate.require(caught && emissions == 2, "complement-order callback exception was swallowed or traversal continued");
    static_cast<void>(run(gate, *index, expected, 2, 8, WspdFrontMode::Pure, sibling, Q2WitnessOrder::ComplementFirst));
    ++gate.callback_failures;
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp9_gen_q2_witness_order_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    corpus(gate, fixtures(), 65535);
    spatial_counterexample(gate);
    models_and_rejections(gate);
    gate.require(gate.clouds == 16 && gate.runs > 1700 && gate.oracle_pairs > 3500 && gate.oracle_sites > 200000 &&
                     gate.supports > 10000 && gate.shell_sites >= 2 * gate.supports && gate.structural_splits > 0 &&
                     gate.deferred_skips > 0 && gate.anchor_skips > 0 && gate.phase_switches > 0 &&
                     gate.default_comparisons == 256 && gate.transformed_clouds == 8 && gate.invalid_inputs == 3 &&
                     gate.callback_failures == 2 && gate.model_mutants == 5 && gate.fixture_universal_witnesses == 12 &&
                     gate.fixture_complement_splits < gate.fixture_global_splits,
                 "witness-order qualification lost a non-vacuity floor");
    // 18-bit twins: four fixtures, each doubled by the 262143 - x reflection,
    // through the same run matrix (112 runs and 16 default comparisons per
    // cloud). Structural counts are exact; the diagonal pair alone yields one
    // support per run (its only pair has an empty interior), hence >= 224.
    const Gate u16 = gate;
    const auto wide = fixtures_18bits();
    require_wide(gate, wide);
    corpus(gate, wide, mhgp9::gen::coordinate_limit);
    gate.require(wide.size() == 4 && gate.clouds - u16.clouds == 8 && gate.transformed_clouds - u16.transformed_clouds == 4 &&
                     gate.default_comparisons - u16.default_comparisons == 128 && gate.runs - u16.runs == 896 &&
                     gate.oracle_pairs - u16.oracle_pairs == 602 && gate.oracle_sites - u16.oracle_sites == 9700 &&
                     gate.supports - u16.supports >= 224 && gate.shell_sites - u16.shell_sites >= 2 * (gate.supports - u16.supports) &&
                     gate.structural_splits > u16.structural_splits && gate.anchor_skips > u16.anchor_skips &&
                     gate.invalid_inputs == u16.invalid_inputs && gate.model_mutants == u16.model_mutants,
                 "18-bit witness-order corpus lost a non-vacuity floor");
    std::cout << "mhgp9_gen_q2_witness_order_gate passed checks=" << gate.checks << " clouds=" << gate.clouds
              << " oracle_pairs=" << gate.oracle_pairs << " oracle_sites=" << gate.oracle_sites << " runs=" << gate.runs
              << " supports=" << gate.supports << " shell_sites=" << gate.shell_sites
              << " structural_splits=" << gate.structural_splits << " deferred_skips=" << gate.deferred_skips
              << " anchor_skips=" << gate.anchor_skips << " phase_switches=" << gate.phase_switches
              << " default_comparisons=" << gate.default_comparisons << " transformed_clouds=" << gate.transformed_clouds
              << " invalid_inputs=" << gate.invalid_inputs << " callback_failures=" << gate.callback_failures
              << " model_mutants=" << gate.model_mutants << " fixture_global_splits=" << gate.fixture_global_splits
              << " fixture_complement_splits=" << gate.fixture_complement_splits
              << " fixture_universal_witnesses=" << gate.fixture_universal_witnesses << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp9_gen_q2_witness_order_gate failed: " << error.what() << '\n';
    return 1;
  }
}
