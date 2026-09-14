// Bounded independent scalar oracles for the isolated f7edd646 adaptation.
// The mutant predicates below are test models, not mutated product binaries.
#include "audit_sibling.hpp"
#include "spindle/q2_prepared_bounds.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>

namespace {
using namespace mhgp8;
using audit::SiblingMode;
using audit::SiblingWork;
u64 checks{}, runs{}, oracle_pairs{}, oracle_sites{}, mutants{}, reordered_queries{};
u64 autonomous_rejections{}, remaining_rejections{};

void require(bool condition, const char* message) {
  ++checks;
  if (!condition) throw std::runtime_error(message);
}

i64 h(const Point3& a, const Point3& b, const Point3& z) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    result += (static_cast<i64>(z[axis]) - a[axis]) *
              (static_cast<i64>(b[axis]) - z[axis]);
  }
  return result;
}

struct Stored {
  std::size_t b{};
  Q2BallKey key;
  std::vector<std::size_t> interior, shell;
  bool operator==(const Stored&) const = default;
};
using Output = std::vector<Stored>;

Output oracle(std::span<const Point3> points, std::size_t a,
              std::span<const std::size_t> ids, unsigned k) {
  Output result;
  for (const auto b : ids) {
    Stored row;
    row.b = b;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      row.key.center_twice[axis] = std::uint32_t{points[a][axis]} + points[b][axis];
      const i64 delta = static_cast<i64>(points[a][axis]) - points[b][axis];
      row.key.diameter_squared += static_cast<u64>(delta * delta);
    }
    for (std::size_t z = 0; z < points.size(); ++z) {
      const auto power = h(points[a], points[b], points[z]);
      if (power > 0) row.interior.push_back(z);
      if (power == 0) row.shell.push_back(z);
      ++oracle_sites;
    }
    ++oracle_pairs;
    if (row.interior.size() < k) result.push_back(std::move(row));
  }
  std::sort(result.begin(), result.end(), [](const auto& x, const auto& y) { return x.b < y.b; });
  return result;
}

struct Capture { Q2CensusResult result; SiblingWork work; Output output; };

Capture checked(const Q2CensusIndex& index, std::size_t a, std::span<const std::size_t> b,
                unsigned k, SiblingMode mode, const Output& expected) {
  Capture capture;
  const auto* nodes = index.spatial_nodes().data();
  const auto* order = index.spatial_order().data();
  capture.result = audit::sibling_group_fixture(index, a, b, k,
      [&](const Q2Support& support) {
        require(support.a_id == a, "group changed the anchor original ID");
        Stored row{support.b_id, support.key,
                   {support.interior.begin(), support.interior.end()},
                   {support.shell.begin(), support.shell.end()}};
        std::sort(row.interior.begin(), row.interior.end());
        std::sort(row.shell.begin(), row.shell.end());
        capture.output.push_back(std::move(row));
      }, mode, capture.work);
  std::sort(capture.output.begin(), capture.output.end(),
            [](const auto& x, const auto& y) { return x.b < y.b; });
  require(capture.output == expected, "group stream/key/interior/shell differs from scalar oracle");
  const auto& c = capture.result;
  const auto& w = capture.work;
  require(c.candidate_pairs == b.size() && c.accepted_pairs == expected.size() &&
          c.rejected_pairs == b.size() - expected.size(), "group pair ledger changed");
  require(c.work.query_tasks == 1 + 2 * c.work.query_splits &&
          c.work.cursor_reuses == 2 * c.work.query_splits && c.work.count_root_starts == 1 &&
          c.work.frontier_restarts == 0, "group lost fixed-prefix task accounting");
  require(w.population_eligible == w.bound_tests && w.bound_tests <= w.child_entries &&
          w.rejected_children <= w.bound_tests && w.rejected_children <= w.rejected_pairs &&
          w.rejected_pairs <= c.rejected_pairs, "sibling work ledger changed");
  if (mode == SiblingMode::Baseline) {
    require(w.child_entries == 0 && w.bound_tests == 0 && w.rejected_pairs == 0,
            "baseline incurred sibling instrumentation");
  } else {
    require(w.child_entries == 2 * c.work.query_splits, "sibling missed a child entry");
  }
  require(nodes == index.spatial_nodes().data() && order == index.spatial_order().data(),
          "query execution replaced the immutable global index");
  ++runs;
  return capture;
}

void lines() {
  for (unsigned m : {8U, 16U, 32U, 64U}) {
    std::vector<Point3> points{{1000, 0, 0}};
    for (unsigned i = 0; i < m; ++i) points.push_back({static_cast<std::uint16_t>(i), 0, 0});
    const auto index = make_q2_cloud_index(prepare_cloud(points));
    std::vector<std::size_t> ids(m);
    std::iota(ids.begin(), ids.end(), std::size_t{1});
    for (unsigned permutation = 0; permutation < 2; ++permutation) {
      if (permutation != 0) { std::reverse(ids.begin(), ids.end()); ++reordered_queries; }
      for (unsigned k : {1U, 2U, 5U, 10U}) {
        const auto expected = oracle(points, 0, ids, k);
        const auto baseline = checked(*index, 0, ids, k, SiblingMode::Baseline, expected);
        const auto autonomous = checked(*index, 0, ids, k, SiblingMode::Autonomous, expected);
        const auto remaining = checked(*index, 0, ids, k, SiblingMode::Remaining, expected);
        autonomous_rejections += autonomous.work.rejected_pairs;
        remaining_rejections += remaining.work.rejected_pairs;
        if (m == 64 && k == 10 && permutation == 0) {
          require(expected.size() == 10 && baseline.result.rejected_pairs == 54,
                  "a1000/B64 lost its exact depth m-1-i");
          require(autonomous.work.rejected_pairs == 48 && autonomous.work.rejected_children == 2,
                  "a1000/B64 autonomous certificate lost its 32+16 grouped rejection");
          require(autonomous.result.work.count_node_visits < baseline.result.work.count_node_visits,
                  "a1000/B64 sibling pruning saved no Z visits");
          std::cout << "{\"fixture\":\"a1000_B64_K10\",\"accepted\":10,\"baseline_tasks\":"
                    << baseline.result.work.query_tasks << ",\"baseline_nodes\":"
                    << baseline.result.work.count_node_visits << ",\"sibling_tasks\":"
                    << autonomous.result.work.query_tasks << ",\"sibling_nodes\":"
                    << autonomous.result.work.count_node_visits << ",\"sibling_group_rejected_pairs\":48}\n";
        }
      }
    }
  }
}

void remaining_positive() {
  const std::vector<Point3> points{{0, 0, 0}, {999, 0, 0}, {1000, 1, 0}, {500, 2, 1}};
  const std::array<std::size_t, 2> b{1, 2};
  const auto index = make_q2_cloud_index(prepare_cloud(points));
  const auto expected = oracle(points, 0, b, 2);
  require(expected.size() == 1 && expected[0].b == 1 && expected[0].interior.size() == 1,
          "remaining fixture lost its exact depths 1/2");
  const auto baseline = checked(*index, 0, b, 2, SiblingMode::Baseline, expected);
  const auto autonomous = checked(*index, 0, b, 2, SiblingMode::Autonomous, expected);
  const auto remaining = checked(*index, 0, b, 2, SiblingMode::Remaining, expected);
  require(baseline.result.work.shared_splits_after_credit > 0 &&
          autonomous.work.bound_tests == 0 && remaining.work.bound_tests > 0 &&
          remaining.work.rejected_pairs == 1, "remaining certificate did not use the disjoint prefix");
}

void mutant_models() {
  // Equality is shell, not strict interior: >=0 would wrongly reject depth0.
  const Point3 a{257, 0, 0}, c{0, 0, 0}, shell{1, 16, 0};
  const Q2PreparedBounds equality(a, singleton_box(c));
  const auto zero = equality.bounds(singleton_box(shell));
  require(h(a, c, shell) == 0 && zero.minimum4 == 0 &&
          !(zero.minimum4 > 0) && zero.minimum4 >= 0, "nonstrict mutant lost its shell target");
  const std::vector<Point3> equality_points{a, c, shell};
  const std::array<std::size_t, 1> only_c{1};
  require(oracle(equality_points, 0, only_c, 1).size() == 1,
          "nonstrict mutant would not lose an accepted support");
  ++mutants;
  // Taking the parent population2 as sibling population1 falsely saturates K2.
  const std::vector<Point3> population{{1000, 0, 0}, {0, 0, 0}, {1, 0, 0}};
  require(h(population[0], population[1], population[2]) == 999 &&
          oracle(population, 0, only_c, 2).size() == 1 && 1 < 2,
          "parent-population mutant lost its depth1 target");
  ++mutants;
  // A positive maximum does not certify every sibling site.
  const std::vector<Point3> mixed{{1000, 20, 0}, {0, 20, 0}, {1, 41, 0}, {1, 60, 0}};
  const Box3 box{mixed[2], mixed[3]};
  const auto bounds = Q2PreparedBounds(mixed[0], singleton_box(mixed[1])).bounds(box);
  require(h(mixed[0], mixed[1], mixed[2]) == 558 && h(mixed[0], mixed[1], mixed[3]) == -601 &&
          bounds.minimum4 == -2404 && bounds.maximum4 == 2232 &&
          oracle(mixed, 0, only_c, 2).size() == 1, "maximum mutant lost its depth1 target");
  ++mutants;
  // Also run the actual predicate on all three tiny complete B domains.
  for (const auto& points : {equality_points, population, mixed}) {
    const unsigned k = points == equality_points ? 1 : 2;
    const auto index = make_q2_cloud_index(prepare_cloud(points));
    std::vector<std::size_t> b(points.size() - 1);
    std::iota(b.begin(), b.end(), std::size_t{1});
    const auto expected = oracle(points, 0, b, k);
    for (const auto mode : {SiblingMode::Baseline, SiblingMode::Autonomous, SiblingMode::Remaining}) {
      static_cast<void>(checked(*index, 0, b, k, mode, expected));
    }
  }
}
}  // namespace

int main() {
  try {
    lines(); remaining_positive(); mutant_models();
    require(runs == 108 && mutants == 3 && reordered_queries == 4 &&
            autonomous_rejections > 0 && remaining_rejections > 0,
            "sibling fixture lost a non-vacuity floor");
    std::cout << "{\"status\":\"passed\",\"scope\":\"bounded_scalar_oracle_and_mutant_models\",\"checks\":"
              << checks << ",\"runs\":" << runs << ",\"oracle_pairs\":" << oracle_pairs
              << ",\"oracle_sites\":" << oracle_sites << ",\"mutant_models\":" << mutants
              << ",\"reordered_queries\":" << reordered_queries << ",\"autonomous_rejected_pairs\":"
              << autonomous_rejections << ",\"remaining_rejected_pairs\":" << remaining_rejections << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "sibling fixture failed: " << error.what() << '\n'; return 1;
  }
}
