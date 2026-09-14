// Explicit AUDIT adaptation of q2_pool_bridge_20260914/gate.cpp, SHA256
// 3d3a48926e66d0aafacc765a47d2a445fadc6748c26af0d8cad3edc5c9cb1a01.
// Retains its independent scalar oracle and bounded geometry fixtures.
// This gate varies ONLY the new singleton-root policy after Pool/pairs64;
// it does not qualify a product API, asynchronous execution or performance.
#include "small_bridge.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using mhgp8::Point3;
using Points = std::vector<Point3>;
using Pair = std::pair<std::size_t, std::size_t>;
using Policy = mhgp8::audit_pool::SmallRootMode;
using Result = mhgp8::audit_pool::BridgeResult;

struct Payload {
  std::array<std::uint32_t, 3> center{};
  std::uint64_t diameter{};
  std::vector<std::size_t> interior;
  std::vector<std::size_t> shell;
  bool operator==(const Payload&) const = default;
};
using Supports = std::map<Pair, Payload>;

struct Counts {
  std::uint64_t fixtures{}, configurations{}, pipeline_runs{}, scalar_site_tests{};
  std::uint64_t supports{}, extra_shell_supports{}, oracle_rejections{};
  std::uint64_t singleton_roots{}, selected_rectangles{}, filtered_pairs{};
  std::uint64_t global_work_comparisons{}, two_site_policy_checks{};
} counts;

void require(bool ok, const std::string& message) {
  if (!ok) throw std::runtime_error(message);
}

// Independent integer dot product. No product predicates, bounds or census.
std::int64_t power(const Point3& a, const Point3& b, const Point3& z) {
  std::int64_t result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis)
    result += (static_cast<std::int64_t>(z[axis]) - a[axis]) *
              (static_cast<std::int64_t>(b[axis]) - z[axis]);
  return result;
}

Supports all_pairs(const Points& points) {
  Supports result;
  for (std::size_t a = 0; a < points.size(); ++a) {
    for (std::size_t b = a + 1; b < points.size(); ++b) {
      Payload payload;
      for (std::size_t axis = 0; axis < 3; ++axis) {
        payload.center[axis] = static_cast<std::uint32_t>(points[a][axis]) + points[b][axis];
        const auto delta = static_cast<std::int64_t>(points[a][axis]) - points[b][axis];
        payload.diameter += static_cast<std::uint64_t>(delta * delta);
      }
      for (std::size_t z = 0; z < points.size(); ++z) {
        const auto h = power(points[a], points[b], points[z]);
        ++counts.scalar_site_tests;
        if (h > 0) payload.interior.push_back(z);
        if (h == 0) payload.shell.push_back(z);
      }
      result.emplace(Pair{a, b}, std::move(payload));
    }
  }
  return result;
}

void canonical_ids(std::vector<std::size_t>& ids) {
  std::sort(ids.begin(), ids.end());
  require(std::adjacent_find(ids.begin(), ids.end()) == ids.end(), "duplicate payload ID");
}

Result run(const std::string& label, const mhgp8::Q2CensusIndex& index,
           const Supports& truth, unsigned k, unsigned s, Policy policy) {
  Supports expected, actual;
  for (const auto& [pair, payload] : truth) {
    if (payload.interior.size() < k) expected.emplace(pair, payload);
    else ++counts.oracle_rejections;
  }
  auto result = mhgp8::audit_pool::run_bridge(index, k, s, 64,
      mhgp8::audit_pool::ResidualMode::Pairwise, [&](const mhgp8::Q2Support& support) {
        require(support.a_id != support.b_id && support.a_id < index.cloud().points().size() &&
                support.b_id < index.cloud().points().size(), "invalid support original IDs");
        const Pair pair = std::minmax(support.a_id, support.b_id);
        Payload payload{support.key.center_twice, support.key.diameter_squared,
                        {support.interior.begin(), support.interior.end()},
                        {support.shell.begin(), support.shell.end()}};
        canonical_ids(payload.interior);
        canonical_ids(payload.shell);
        require(actual.emplace(pair, std::move(payload)).second, "duplicate support incidence");
      }, mhgp8::WspdFrontMode::MidpointSamples, policy);
  require(actual == expected, label + ": oracle supports/keys/interiors/shells differ");
  const auto& census = result.pipeline.census;
  const auto& work = census.work;
  const auto& pool = result.pool;
  require(census.accepted_pairs == actual.size() && census.accepted_pairs <= census.candidate_pairs &&
          census.rejected_pairs == census.candidate_pairs - census.accepted_pairs,
          label + ": census mass differs");
  require(census.candidate_pairs + pool.filtered_pairs == result.pipeline.front.work.residual_pair_mass[0] &&
          pool.selected_pairs == pool.filtered_pairs + pool.residual_pairs,
          label + ": front/Pool mass differs");
  require(work.frontier_restarts == 0 && work.count_root_starts ==
          result.pipeline.anchor_queries - pool.original_selected_anchors + pool.local_roots,
          label + ": nonzero restart or doubled root_start");
  require(work.query_tasks == work.count_root_starts + 2 * work.query_splits &&
          work.count_node_visits == work.count_bound_tests + work.count_point_tests &&
          work.payload_supports == actual.size(), label + ": task/visit/payload ledger differs");
  require(pool.local_b_nodes == 0 && pool.local_b_sites == 0 && pool.cover_nodes == 0,
          label + ": Pool/pairs unexpectedly built local B queries");
  require(result.small_roots == result.pipeline.front.work.leaf_pair_rectangles,
          label + ": small-root policy escaped the initial singleton rectangles");
  ++counts.pipeline_runs;
  counts.supports += actual.size();
  counts.singleton_roots += result.small_roots;
  counts.selected_rectangles += pool.selected_rectangles;
  counts.filtered_pairs += pool.filtered_pairs;
  for (const auto& [pair, payload] : actual) {
    static_cast<void>(pair);
    counts.extra_shell_supports += payload.shell.size() > 2 ? 1U : 0U;
  }
  return result;
}

void same_global_work(const Result& iterative, const Result& recursive) {
  const auto& a = iterative.pipeline.census.work;
  const auto& b = recursive.pipeline.census.work;
#define SAME(field) require(a.field == b.field, "Global iterative/recursive differs: " #field)
  SAME(count_root_starts); SAME(query_tasks); SAME(query_splits); SAME(witness_splits);
  SAME(count_node_visits); SAME(count_bound_tests); SAME(count_point_tests);
  SAME(consumed_witness_sites); SAME(uniform_credited_pairs); SAME(uniform_rejected_pairs);
  SAME(uniform_accepted_pairs); SAME(payload_node_visits); SAME(payload_bound_tests);
  SAME(payload_point_tests); SAME(payload_supports); SAME(payload_interior_sites); SAME(payload_shell_sites);
#undef SAME
  const auto roots = iterative.small_roots;
  require(roots == recursive.small_roots, "Global policies changed the singleton-root domain");
  // Each changed iterative singleton root advances at least once; Pairwise
  // counts the same geometric nodes but has no preorder-cursor movements.
  require(a.cursor_advances >= b.cursor_advances && a.cursor_advances - b.cursor_advances >= roots,
          "Global iterative policy was not exercised at the singleton roots");
  ++counts.global_work_comparisons;
}

void check_fixture(const std::string& label, const Points& points) {
  require(points.size() >= 2 && points.size() <= 80, "fixture outside bounded scope");
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const auto truth = all_pairs(points);
  for (const auto k : {1U, 2U, 5U, 10U}) for (const auto s : {8U, 10U, 12U}) {
    const auto reference = run(label, *index, truth, k, s, Policy::Complement);
    const auto iterative = run(label, *index, truth, k, s, Policy::GlobalIterative);
    const auto recursive = run(label, *index, truth, k, s, Policy::GlobalPairwise);
    require(reference.pipeline.census.candidate_pairs == iterative.pipeline.census.candidate_pairs &&
            reference.pipeline.census.candidate_pairs == recursive.pipeline.census.candidate_pairs,
            "root policy changed candidate coverage");
    same_global_work(iterative, recursive);
    if (points.size() == 2) {
      require(reference.pipeline.front.work.leaf_pair_rectangles == 1 &&
              reference.pipeline.order_work.anchor_skips == 1 &&
              reference.pipeline.order_work.phase_switches == 1 &&
              iterative.pipeline.order_work.structural_splits == 0 &&
              iterative.pipeline.order_work.anchor_skips == 0 &&
              iterative.pipeline.order_work.phase_switches == 0 &&
              recursive.pipeline.order_work.structural_splits == 0 &&
              recursive.pipeline.order_work.anchor_skips == 0 &&
              recursive.pipeline.order_work.phase_switches == 0,
              "two-site root policy nonvacuity failed");
      ++counts.two_site_policy_checks;
    }
    ++counts.configurations;
  }
  ++counts.fixtures;
}

Points large_factor() {
  Points points;
  for (const auto y : {0U, 1U}) for (const auto z : {0U, 1U})
    points.push_back({0, static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z)});
  for (unsigned j = 0; j < 65; ++j)
    points.push_back({static_cast<std::uint16_t>(1000 + j), 0, 0});
  return points;
}

void selftest() {
  check_fixture("two_sites", {{0, 0, 0}, {65535, 65535, 65535}});
  check_fixture("permutation_and_precredit", {{100, 0, 0}, {0, 1, 0}, {1, 0, 1}});
  check_fixture("external_witness", {{100, 0, 0}, {100, 4, 0}, {0, 1, 0},
                                      {0, 2, 0}, {0, 3, 0}, {50, 2, 0}});
  check_fixture("full_dimension", {{0, 0, 0}, {1, 1, 0}, {100, 0, 1}, {101, 0, 0}});
  Points sphere;
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y)
    for (int z = -5; z <= 5; ++z) if (x*x + y*y + z*z == 25)
      sphere.push_back({static_cast<std::uint16_t>(x + 8), static_cast<std::uint16_t>(y + 8),
                        static_cast<std::uint16_t>(z + 8)});
  require(sphere.size() == 30, "sphere shell nonvacuity failed");
  check_fixture("shell30", sphere);
  Points extreme;
  for (const auto x : {0U, 65535U}) for (const auto y : {0U, 65535U})
    for (const auto z : {0U, 65535U})
      extreme.push_back({static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                         static_cast<std::uint16_t>(z)});
  extreme.push_back({32768, 32767, 32768});
  check_fixture("u16_extremes", extreme);
  // For (a,b), the global node {z,a} has H<=0 throughout its box;
  // Global may prune it, while Complement descends structurally to a.
  check_fixture("endpoint_box_zero", {{1, 1, 0}, {3, 1, 0}, {0, 0, 0}});
  std::uint32_t random_state = 0x31eab45U;
  for (unsigned sample = 0; sample < 1; ++sample) {
    Points random;
    for (unsigned i = 0; i < 7 + sample * 3; ++i) {
      random_state = random_state * 1664525U + 1013904223U;
      const auto y = static_cast<std::uint16_t>((random_state >> 10U) % 37U);
      random_state = random_state * 1664525U + 1013904223U;
      random.push_back({static_cast<std::uint16_t>(i * 3), y,
                        static_cast<std::uint16_t>((random_state >> 10U) % 41U)});
    }
    std::rotate(random.begin(), random.begin() + 2, random.end());
    check_fixture("small_random_" + std::to_string(sample), random);
  }
  auto large = large_factor();
  check_fixture("large_B", large);
  std::reverse(large.begin(), large.end());
  check_fixture("large_B_permuted_IDs", large);
  require(counts.fixtures == 10 && counts.configurations == 120 && counts.pipeline_runs == 360 &&
          counts.global_work_comparisons == 120 && counts.two_site_policy_checks == 12 &&
          counts.supports > 1000 && counts.extra_shell_supports > 100 && counts.oracle_rejections > 100 &&
          counts.singleton_roots > 0 && counts.selected_rectangles > 0 && counts.filtered_pairs > 0,
          "small-root gate nonvacuity failed");
}
}  // namespace

int main() {
  try {
    selftest();
    std::cout << "{\"status\":\"passed\",\"scope\":\"bounded_audit_singleton_roots\","
              << "\"fixtures\":" << counts.fixtures << ",\"configurations\":" << counts.configurations
              << ",\"pipeline_runs\":" << counts.pipeline_runs << ",\"scalar_site_tests\":" << counts.scalar_site_tests
              << ",\"supports\":" << counts.supports << ",\"extra_shell_supports\":" << counts.extra_shell_supports
              << ",\"oracle_rejections\":" << counts.oracle_rejections
              << ",\"singleton_roots\":" << counts.singleton_roots
              << ",\"selected_rectangles\":" << counts.selected_rectangles << ",\"filtered_pairs\":" << counts.filtered_pairs
              << ",\"global_work_comparisons\":" << counts.global_work_comparisons
              << ",\"two_site_policy_checks\":" << counts.two_site_policy_checks << "}\n";
    return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
