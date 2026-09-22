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

using mhgp8::Point3;
using mhgp8::Q2CensusMode;
using mhgp8::Q2SiblingMode;
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
  u64 checks{}, clouds{}, oracle_pairs{}, oracle_sites{}, runs{}, supports{};
  u64 proposals{}, cardinality_skips{}, bound_tests{}, rejected_tasks{}, rejected_pairs{}, rejected_after_credit{};
  u64 transformed_clouds{}, permutations{}, default_comparisons{}, invalid_inputs{}, callback_failures{}, model_mutants{};
  u64 fragmentation_bound{}, fragmentation_disabled_tasks{}, fragmentation_enabled_tasks{};

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

void sort_output(Output& output) {
  std::sort(output.begin(), output.end(), [](const auto& a, const auto& b) { return a.pair < b.pair; });
}

// Exhaustive scalar test oracle only. No production geometry or selection.
// u16 differences are promoted first; the three dot-product terms fit i64.
Output oracle(Gate& gate, std::span<const Point3> points) {
  Output result;
  for (std::size_t a = 0; a < points.size(); ++a) {
    for (std::size_t b = a + 1; b < points.size(); ++b) {
      Payload support{{a, b}, {}, {}, {}};
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

Output accepted(const Output& all, unsigned kmax) {
  Output result;
  for (const auto& support : all) if (support.interior.size() < kmax) result.push_back(support);
  return result;
}

auto census_work(const mhgp8::Q2CensusWork& w) {
  return std::array{w.query_build_point_visits, w.query_build_nodes, w.query_build_max_depth,
      w.input_descriptors, w.query_cover_visits, w.query_tasks, w.query_splits, w.witness_splits,
      w.count_root_starts, w.shared_splits_after_credit, w.cursor_advances, w.cursor_reuses,
      w.count_node_visits, w.count_bound_tests, w.count_point_tests, w.uniform_credited_pairs,
      w.uniform_rejected_pairs, w.uniform_accepted_pairs, w.consumed_witness_sites,
      w.frontier_restarts, w.payload_node_visits, w.payload_bound_tests, w.payload_point_tests,
      w.payload_interior_sites, w.payload_shell_sites, w.payload_supports};
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
            unsigned kmax, unsigned separation, WspdFrontMode front_mode,
            Q2SiblingMode sibling_mode, bool use_default = false) {
  Capture capture;
  const auto n = index.cloud().points().size();
  u64 interiors = 0, shells = 0;
  const mhgp8::Q2CensusConsumer consumer = [&](const mhgp8::Q2Support& support) {
    gate.require(support.a_id < n && support.b_id < n && support.a_id != support.b_id,
                 "sibling census emitted invalid support IDs");
    Payload payload{{std::min(support.a_id, support.b_id), std::max(support.a_id, support.b_id)},
                     support.key, {support.interior.begin(), support.interior.end()},
                     {support.shell.begin(), support.shell.end()}};
    for (auto* ids : {&payload.interior, &payload.shell}) {
      for (const auto id : *ids) gate.require(id < n, "sibling payload ID escaped its cloud");
      std::sort(ids->begin(), ids->end());
      gate.require(std::adjacent_find(ids->begin(), ids->end()) == ids->end(),
                   "sibling payload duplicated a site");
    }
    interiors += payload.interior.size();
    shells += payload.shell.size();
    capture.output.push_back(std::move(payload));
  };
  if (use_default) {
    capture.result = mhgp8::run_wspd_q2_census(index, kmax, separation, front_mode,
                                              Q2CensusMode::SharedBlocks, consumer);
  } else {
    capture.result = mhgp8::run_wspd_q2_census(index, kmax, separation, front_mode,
                                              Q2CensusMode::SharedBlocks, consumer, sibling_mode);
  }
  sort_output(capture.output);
  gate.require(capture.output == expected, "sibling mode changed exact supports, keys, interiors or shells");
  const auto& r = capture.result;
  const auto& c = r.census;
  const auto& w = r.sibling_work;
  gate.require(r.front.active_lane_mask == 1 && c.candidate_pairs == r.front.work.residual_pair_mass[0] &&
                   c.accepted_pairs == expected.size() && c.accepted_pairs + c.rejected_pairs == c.candidate_pairs &&
                   c.candidate_pairs + r.front.work.rejected_pair_mass[0] == static_cast<u64>(n) * (n - 1) / 2,
               "sibling rejection changed the front/census pair partition");
  gate.require(c.work.payload_supports == expected.size() && c.work.payload_interior_sites == interiors &&
                   c.work.payload_shell_sites == shells && c.work.count_root_starts == r.anchor_queries &&
                   c.work.frontier_restarts == 0 && c.work.query_tasks == r.anchor_queries + 2 * c.work.query_splits &&
                   c.work.cursor_reuses == 2 * c.work.query_splits,
               "sibling payload or root/cursor/query-entry ledger failed");
  gate.require(c.work.query_build_nodes == 0 && c.work.query_build_point_visits == 0 &&
                   c.work.query_cover_visits == 0 && c.query_index_ms == 0,
               "sibling mode rebuilt or rescanned query factors");
  if (sibling_mode == Q2SiblingMode::Disabled || use_default) {
    gate.require(sibling_work(w) == std::array<u64, 6>{}, "disabled sibling mode paid sidecar work");
  } else {
    gate.require(w.proposals == 2 * c.work.query_splits && w.bound_tests + w.cardinality_skips == w.proposals &&
                     w.rejected_tasks <= w.bound_tests && w.rejected_tasks <= w.rejected_pairs &&
                     w.rejected_pairs <= c.rejected_pairs && w.rejected_after_credit <= w.rejected_tasks &&
                     w.rejected_after_credit <= 2 * c.work.shared_splits_after_credit,
                 "sibling sidecar proposal/bound/rejection ledger failed");
    gate.proposals += w.proposals;
    gate.cardinality_skips += w.cardinality_skips;
    gate.bound_tests += w.bound_tests;
    gate.rejected_tasks += w.rejected_tasks;
    gate.rejected_pairs += w.rejected_pairs;
    gate.rejected_after_credit += w.rejected_after_credit;
  }
  for (const double time : {r.total_ms, c.total_ms, c.count_ms, c.payload_ms}) {
    gate.require(std::isfinite(time) && time >= 0, "sibling census reported invalid timing");
  }
  gate.supports += c.accepted_pairs;
  ++gate.runs;
  return capture;
}

void unchanged(Gate& gate, const Capture& disabled, const Capture& other) {
  const auto& a = disabled.result;
  const auto& b = other.result;
  gate.require(disabled.output == other.output && a.anchor_queries == b.anchor_queries &&
                   a.input_rectangles == b.input_rectangles && a.census.candidate_pairs == b.census.candidate_pairs &&
                   a.front.work.residual_pair_mass == b.front.work.residual_pair_mass &&
                   a.front.work.rejected_pair_mass == b.front.work.rejected_pair_mass &&
                   a.front.work.product_visits == b.front.work.product_visits,
               "sibling switch changed upstream rectangles or output incidences");
}

std::vector<std::vector<Point3>> fixtures() {
  std::vector<std::vector<Point3>> result{
      {{7, 11, 13}}, {{0, 0, 0}, {65535, 65535, 65535}},
      {{0, 0, 0}, {10, 0, 0}, {5, 5, 0}},
      {{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}}};
  std::vector<Point3> cube;
  for (unsigned bits = 0; bits < 8; ++bits) {
    cube.push_back({static_cast<std::uint16_t>((bits & 1U) * 2),
                   static_cast<std::uint16_t>(((bits >> 1U) & 1U) * 2),
                   static_cast<std::uint16_t>(((bits >> 2U) & 1U) * 2)});
  }
  result.push_back(cube);
  std::vector<Point3> sheet;
  for (unsigned x = 0; x < 3; ++x) for (unsigned y = 0; y < 4; ++y)
    sheet.push_back({static_cast<std::uint16_t>(x * 43), static_cast<std::uint16_t>(y * 47), 17});
  result.push_back(sheet);
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
    for (unsigned transform = 0; transform < 3; ++transform) {
      auto points = base;
      if (transform != 0) {
        for (auto& point : points) {
          // Exact signed coordinate permutation: a rigid lattice isometry.
          point = transform == 1 ? Point3{static_cast<std::uint16_t>(65535U - point.x), point.y, point.z}
                                 : Point3{point.z, point.x, point.y};
        }
        ++gate.transformed_clouds;
        std::reverse(points.begin(), points.end());
        ++gate.permutations;
      }
      const auto all = oracle(gate, points);
      const auto cloud = mhgp8::prepare_cloud(points);
      const auto index = mhgp8::make_q2_cloud_index(cloud);
      ++gate.clouds;
      for (const unsigned kmax : {1U, 2U, 5U, 10U}) {
        const auto expected = accepted(all, kmax);
        for (const unsigned s : {8U, 10U, 12U}) {
          for (const auto front : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
            const auto disabled = run(gate, *index, expected, kmax, s, front, Q2SiblingMode::Disabled);
            const auto enabled = run(gate, *index, expected, kmax, s, front, Q2SiblingMode::Saturating);
            unchanged(gate, disabled, enabled);
            if (s == 8) {
              const auto implicit = run(gate, *index, expected, kmax, s, front, Q2SiblingMode::Disabled, true);
              unchanged(gate, disabled, implicit);
              gate.require(census_work(disabled.result.census.work) == census_work(implicit.result.census.work),
                           "default mode changed a generic census counter compared with Disabled");
              ++gate.default_comparisons;
            }
          }
        }
      }
    }
  }
}

std::size_t height(std::span<const mhgp8::Q2SpatialNode> nodes, std::size_t id) {
  if (nodes[id].left == mhgp8::Q2SpatialNode::absent) return 0;
  return 1 + std::max(height(nodes, nodes[id].left), height(nodes, nodes[id].right));
}

void fragmentation(Gate& gate) {
  std::vector<Point3> points;
  for (unsigned i = 0; i < 64; ++i) points.push_back({static_cast<std::uint16_t>(i), 0, 0});
  points.push_back({1000, 0, 0});
  const auto all = oracle(gate, points);
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const auto nodes = index->spatial_nodes();
  u64 bound = 0;
  static_cast<void>(mhgp8::run_wspd_front(*index, 1, 8, WspdFrontMode::Pure,
      [&](const mhgp8::WspdRectangle& rectangle) {
    auto a = rectangle.a_node, b = rectangle.b_node;
    if (nodes[a].range.size() > nodes[b].range.size()) std::swap(a, b);
    // In this collinear separated product, every split has a farther child
    // universally witnessed by its nearer sibling. At K=1 only one child
    // can continue: at most 1+2*height(B) task entries per anchor.
    bound += static_cast<u64>(nodes[a].range.size()) * (1 + 2 * height(nodes, b));
  }, 1));
  for (const unsigned kmax : {1U, 2U, 5U, 10U}) {
    const auto expected = accepted(all, kmax);
    for (const unsigned s : {8U, 10U, 12U}) {
      for (const auto front : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
        const auto disabled = run(gate, *index, expected, kmax, s, front, Q2SiblingMode::Disabled);
        const auto enabled = run(gate, *index, expected, kmax, s, front, Q2SiblingMode::Saturating);
        unchanged(gate, disabled, enabled);
        if (kmax == 1 && s == 8 && front == WspdFrontMode::Pure) {
          gate.require(enabled.result.sibling_work.rejected_tasks > 0 &&
                           enabled.result.census.work.query_tasks <= bound &&
                           enabled.result.census.work.query_tasks < disabled.result.census.work.query_tasks,
                       "integrated collinear fixture lost its independently bounded sibling fragmentation");
          gate.fragmentation_bound = bound;
          gate.fragmentation_disabled_tasks = disabled.result.census.work.query_tasks;
          gate.fragmentation_enabled_tasks = enabled.result.census.work.query_tasks;
        }
      }
    }
  }

  auto reflected = points;
  for (auto& point : reflected) point.x = static_cast<std::uint16_t>(65535U - point.x);
  std::reverse(reflected.begin(), reflected.end());
  const auto reflected_all = oracle(gate, reflected);
  const auto reflected_index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(reflected));
  for (const unsigned kmax : {1U, 2U, 5U, 10U}) {
    const auto expected = accepted(reflected_all, kmax);
    for (const unsigned s : {8U, 10U, 12U}) {
      for (const auto front : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
        const auto disabled = run(gate, *reflected_index, expected, kmax, s, front, Q2SiblingMode::Disabled);
        const auto enabled = run(gate, *reflected_index, expected, kmax, s, front, Q2SiblingMode::Saturating);
        unchanged(gate, disabled, enabled);
      }
    }
  }

  // A witness outside B is consumed before the B subtree. The subsequent
  // sibling certificate must reject autonomously even with acquired count
  // one, without adding to it or changing the shared continuation contract.
  std::vector<Point3> credited{{0, 0, 0}, {500, 0, 0}};
  for (unsigned i = 0; i < 64; ++i) credited.push_back({static_cast<std::uint16_t>(1000 + i), 0, 0});
  const auto credited_expected = accepted(oracle(gate, credited), 2);
  const auto credited_index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(credited));
  for (const auto front : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
    const auto disabled = run(gate, *credited_index, credited_expected, 2, 8, front, Q2SiblingMode::Disabled);
    const auto enabled = run(gate, *credited_index, credited_expected, 2, 8, front, Q2SiblingMode::Saturating);
    unchanged(gate, disabled, enabled);
    gate.require(enabled.result.sibling_work.rejected_after_credit > 0,
                 "sibling fixture lost its autonomous rejection after inherited credit");
  }
}

void negative_models_and_requests(Gate& gate) {
  const std::vector<Point3> boundary{{1000, 0, 0}, {0, 1, 0}, {0, 0, 0}};
  const auto shell = accepted(oracle(gate, boundary), 1);
  const auto support = std::find_if(shell.begin(), shell.end(), [](const auto& p) { return p.pair == Pair{0, 1}; });
  gate.require(support != shell.end() && support->interior.empty() && support->shell.size() == 3,
               "nonstrict sibling-bound model lost its boundary fixture");
  // Unlike a compact triangle, this cloud emits {a} x B with |B|=2
  // under the real s=8 front. Both child proposals are tested at K=1;
  // z=(0,0,0) lies exactly on the shell of a--(0,1,0), so >=0 is unsafe.
  const auto boundary_index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(boundary));
  const auto boundary_disabled = run(gate, *boundary_index, shell, 1, 8,
                                     WspdFrontMode::Pure, Q2SiblingMode::Disabled);
  const auto boundary_enabled = run(gate, *boundary_index, shell, 1, 8,
                                    WspdFrontMode::Pure, Q2SiblingMode::Saturating);
  unchanged(gate, boundary_disabled, boundary_enabled);
  gate.require(sibling_work(boundary_enabled.result.sibling_work) == std::array<u64, 6>{2, 0, 2, 0, 0, 0},
               "real sibling-boundary fixture did not test and preserve both nonsaturating children");
  auto nonstrict = shell;
  nonstrict.erase(std::remove_if(nonstrict.begin(), nonstrict.end(), [](const auto& p) { return p.pair == Pair{0, 1}; }), nonstrict.end());
  gate.require(nonstrict != shell, "nonstrict autonomous rejection model survived");
  ++gate.model_mutants;
  const std::vector<Point3> line{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}};
  const auto expected = accepted(oracle(gate, line), 2);
  auto doubled = expected;
  const auto one = std::find_if(expected.begin(), expected.end(), [](const auto& p) { return p.pair == Pair{0, 2}; });
  gate.require(one != expected.end() && one->interior == std::vector<std::size_t>{1},
               "double-credit model lost its single-witness fixture");
  doubled.erase(std::remove_if(doubled.begin(), doubled.end(), [](const auto& p) { return p.pair == Pair{0, 2}; }), doubled.end());
  gate.require(doubled != expected, "generic repeated-witness credit model survived");
  ++gate.model_mutants;
  auto lost_shell = shell;
  lost_shell.front().shell.pop_back();
  gate.require(lost_shell != shell, "truncated shell model survived");
  ++gate.model_mutants;
  auto duplicate = expected;
  duplicate.push_back(expected.front());
  sort_output(duplicate);
  gate.require(duplicate != expected, "duplicate support model survived");
  ++gate.model_mutants;

  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(line));
  const auto below_threshold = run(gate, *index, expected, 2, 8, WspdFrontMode::Pure, Q2SiblingMode::Saturating);
  // The sole non-singleton query is {a=0} x {b=10,11}. z=5 gives
  // inherited count one; each sibling then contains one site, below K=2.
  // Both must be skipped. A count+|sibling| certificate could keep the same
  // final geometry here, but would violate these autonomous-mode counters.
  gate.require(below_threshold.result.census.work.query_splits == 1 &&
                   below_threshold.result.census.work.shared_splits_after_credit == 1 &&
                   sibling_work(below_threshold.result.sibling_work) == std::array<u64, 6>{2, 2, 0, 0, 0, 0},
               "subthreshold sibling was combined with inherited credit instead of skipped");
  ++gate.model_mutants;
  u64 emissions = 0;
  const mhgp8::Q2CensusConsumer consumer = [&](const mhgp8::Q2Support&) { ++emissions; };
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
      Q2CensusMode::Pairwise, consumer, Q2SiblingMode::Saturating)); }, "Pairwise accepted the sibling-only optimization");
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
      Q2CensusMode::SharedBlocks, consumer, static_cast<Q2SiblingMode>(99))); }, "invalid sibling mode was accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
      Q2CensusMode::SharedBlocks, {}, Q2SiblingMode::Saturating)); }, "empty sibling callback was accepted");
  gate.require(emissions == 0, "invalid sibling request emitted before rejection");
  struct CallbackFailure {};
  for (const auto mode : {Q2SiblingMode::Disabled, Q2SiblingMode::Saturating}) {
    emissions = 0;
    bool caught = false;
    try {
      static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
          Q2CensusMode::SharedBlocks, [&](const mhgp8::Q2Support&) {
            if (++emissions == 2) throw CallbackFailure{};
          }, mode));
    } catch (const CallbackFailure&) { caught = true; }
    gate.require(caught && emissions == 2, "sibling callback exception was swallowed or traversal continued");
    static_cast<void>(run(gate, *index, expected, 2, 8, WspdFrontMode::Pure, mode));
    ++gate.callback_failures;
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_q2_sibling_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    corpus(gate);
    fragmentation(gate);
    negative_models_and_requests(gate);
    gate.require(gate.clouds == 24 && gate.runs > 1300 && gate.oracle_pairs > 3000 && gate.oracle_sites > 130000 &&
                     gate.supports > 10000 && gate.proposals > 0 && gate.cardinality_skips > 0 && gate.bound_tests > 0 &&
                     gate.rejected_tasks > 0 && gate.rejected_pairs > 0 && gate.rejected_after_credit > 0 &&
                     gate.transformed_clouds == 16 &&
                     gate.permutations == 16 && gate.default_comparisons == 192 && gate.invalid_inputs == 3 &&
                     gate.callback_failures == 2 && gate.model_mutants == 5 && gate.fragmentation_bound > 0,
                 "sibling qualification lost a non-vacuity floor");
    std::cout << "mhgp8_q2_sibling_gate passed checks=" << gate.checks << " clouds=" << gate.clouds
              << " oracle_pairs=" << gate.oracle_pairs << " oracle_sites=" << gate.oracle_sites << " runs=" << gate.runs
              << " supports=" << gate.supports << " proposals=" << gate.proposals
              << " cardinality_skips=" << gate.cardinality_skips << " bound_tests=" << gate.bound_tests
              << " rejected_tasks=" << gate.rejected_tasks << " rejected_pairs=" << gate.rejected_pairs
              << " rejected_after_credit=" << gate.rejected_after_credit << " transformed_clouds=" << gate.transformed_clouds
              << " permutations=" << gate.permutations << " default_comparisons=" << gate.default_comparisons
              << " invalid_inputs=" << gate.invalid_inputs << " callback_failures=" << gate.callback_failures
              << " model_mutants=" << gate.model_mutants << " fragmentation_bound=" << gate.fragmentation_bound
              << " fragmentation_disabled_tasks=" << gate.fragmentation_disabled_tasks
              << " fragmentation_enabled_tasks=" << gate.fragmentation_enabled_tasks << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_q2_sibling_gate failed: " << error.what() << '\n';
    return 1;
  }
}
