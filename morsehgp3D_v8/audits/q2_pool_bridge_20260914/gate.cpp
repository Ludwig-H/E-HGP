// Independent bounded judge for the audit-only Pool/census bridge.
// No product qualification, timing, memory or parallel-scheduler claim.
#include "bridge.hpp"
#include "node_pool.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using mhgp8::Point3;
using Pair = std::pair<std::size_t, std::size_t>;
using Points = std::vector<Point3>;

struct Payload {
  std::array<std::uint32_t, 3> center{};
  std::uint64_t diameter{};
  std::vector<std::size_t> interior;
  std::vector<std::size_t> shell;
  bool operator==(const Payload&) const = default;
};
using Supports = std::map<Pair, Payload>;

struct Counts {
  std::uint64_t fixtures{}, pipeline_runs{}, scalar_site_tests{}, supports{};
  std::uint64_t shell_extra{}, oracle_rejected{}, node_plans{}, plan_pairs{};
  std::uint64_t rank_mutants{}, precredit_mutants{};
  std::uint64_t selected_rectangles{}, local_roots{}, reused_shared_covers{}, cutoff64_runs{};
  std::uint64_t rejected_inputs{};
} counts;

void require(bool ok, const std::string& message) {
  if (!ok) throw std::runtime_error(message);
}

template <class Error, class Operation>
void rejects(Operation operation) {
  try { operation(); }
  catch (const Error&) { ++counts.rejected_inputs; return; }
  throw std::runtime_error("invalid operation was not rejected with the promised exception type");
}

std::int64_t power(const Point3& a, const Point3& b, const Point3& z) {
  std::int64_t result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    result += (static_cast<std::int64_t>(z[axis]) - a[axis]) *
              (static_cast<std::int64_t>(b[axis]) - z[axis]);
  }
  return result;
}

// Direct scalar dot products, not product predicates, boxes, keys or census.
Payload oracle(const Points& points, std::size_t a, std::size_t b) {
  Payload result;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    result.center[axis] = static_cast<std::uint32_t>(points[a][axis]) + points[b][axis];
    const auto delta = static_cast<std::int64_t>(points[a][axis]) - points[b][axis];
    result.diameter += static_cast<std::uint64_t>(delta * delta);
  }
  for (std::size_t z = 0; z < points.size(); ++z) {
    const auto h = power(points[a], points[b], points[z]);
    ++counts.scalar_site_tests;
    if (h > 0) result.interior.push_back(z);
    if (h == 0) result.shell.push_back(z);
  }
  return result;
}

Supports all_pairs(const Points& points) {
  Supports result;
  for (std::size_t a = 0; a < points.size(); ++a)
    for (std::size_t b = a + 1; b < points.size(); ++b)
      result.emplace(Pair{a, b}, oracle(points, a, b));
  return result;
}

void canonical_ids(std::vector<std::size_t>& ids) {
  std::sort(ids.begin(), ids.end());
  require(std::adjacent_find(ids.begin(), ids.end()) == ids.end(), "duplicate payload ID");
}

void check_pipeline(const std::string& label, const mhgp8::Q2CensusIndex& index,
                    const Supports& truth, unsigned k, unsigned s, std::size_t cutoff,
                    mhgp8::audit_pool::ResidualMode mode) {
  Supports expected;
  for (const auto& [pair, payload] : truth) {
    if (payload.interior.size() < k) expected.emplace(pair, payload);
    else ++counts.oracle_rejected;
  }
  Supports actual;
  const auto result = mhgp8::audit_pool::run_bridge(index, k, s, cutoff, mode,
      [&](const mhgp8::Q2Support& support) {
        require(support.a_id != support.b_id && support.a_id < index.cloud().points().size() &&
                    support.b_id < index.cloud().points().size(), "invalid support original IDs");
        const Pair pair = std::minmax(support.a_id, support.b_id);
        Payload payload{support.key.center_twice, support.key.diameter_squared,
                        {support.interior.begin(), support.interior.end()},
                        {support.shell.begin(), support.shell.end()}};
        canonical_ids(payload.interior);
        canonical_ids(payload.shell);
        require(actual.emplace(pair, std::move(payload)).second, "duplicate support incidence");
      });
  require(actual == expected, label + ": oracle supports/keys/interiors/shells differ");
  require(result.pipeline.census.accepted_pairs == actual.size(), label + ": accepted mass differs");
  require(result.pipeline.census.accepted_pairs <= result.pipeline.census.candidate_pairs &&
              result.pipeline.census.rejected_pairs == result.pipeline.census.candidate_pairs -
                  result.pipeline.census.accepted_pairs, label + ": residual mass is not partitioned");
  require(result.pipeline.census.work.frontier_restarts == 0, label + ": nonzero census restart");
  counts.selected_rectangles += result.pool.selected_rectangles;
  counts.local_roots += result.pool.local_roots;
  if (mode == mhgp8::audit_pool::ResidualMode::Shared &&
      result.pool.local_roots > result.pool.cover_nodes) ++counts.reused_shared_covers;
  if (cutoff == 64 && label.find("large_B") != std::string::npos) {
    require(result.pool.selected_rectangles > 0 && result.pool.filtered_pairs > 0 &&
                result.pool.local_roots > 0, label + ": large-factor branch was not exercised");
    if (mode == mhgp8::audit_pool::ResidualMode::Shared)
      require(result.pool.local_roots > result.pool.cover_nodes && result.pool.local_b_sites > 0,
              label + ": shared cover was not reused across anchors");
    ++counts.cutoff64_runs;
  }
  ++counts.pipeline_runs;
  counts.supports += actual.size();
  for (const auto& [pair, payload] : actual) {
    static_cast<void>(pair);
    counts.shell_extra += payload.shell.size() > 2 ? 1U : 0U;
  }
}

void check_fixture(const std::string& name, const Points& points) {
  require(points.size() >= 2 && points.size() <= 80, "fixture size outside bounded scope");
  const auto cloud = mhgp8::prepare_cloud(points);
  const auto index = mhgp8::make_q2_cloud_index(cloud);
  const auto truth = all_pairs(points);
  using mhgp8::audit_pool::ResidualMode;
  for (const auto k : {1U, 2U, 5U, 10U}) for (const auto s : {8U, 10U, 12U}) {
    const auto label = name + "/K" + std::to_string(k) + "/s" + std::to_string(s);
    check_pipeline(label + "/baseline", *index, truth, k, s, 0, ResidualMode::Shared);
    for (const auto cutoff : {std::size_t{1}, std::size_t{2}, std::size_t{64}})
      for (const auto mode : {ResidualMode::Pairwise, ResidualMode::Shared})
        check_pipeline(label + "/cut" + std::to_string(cutoff) +
                           (mode == ResidualMode::Shared ? "/shared" : "/pair"),
                       *index, truth, k, s, cutoff, mode);
  }
  ++counts.fixtures;
}

std::size_t node_with_ids(const mhgp8::Q2CensusIndex& index,
                          std::vector<std::size_t> wanted) {
  std::sort(wanted.begin(), wanted.end());
  const auto order = index.spatial_order();
  const auto nodes = index.spatial_nodes();
  for (std::size_t id = 0; id < nodes.size(); ++id) {
    const auto range = nodes[id].range;
    if (range.size() != wanted.size()) continue;
    std::vector<std::size_t> actual(order.begin() + range.first, order.begin() + range.last);
    std::sort(actual.begin(), actual.end());
    if (actual == wanted) return id;
  }
  throw std::runtime_error("fixture factor is not a global node");
}

void check_plan(const Points& points, const std::vector<std::size_t>& a_ids,
                const std::vector<std::size_t>& b_ids, unsigned k) {
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const auto a_node = node_with_ids(*index, a_ids);
  const auto b_node = node_with_ids(*index, b_ids);
  const mhgp8::audit::NodePoolPlan plan(*index, a_node, b_node, k);
  const auto a = index->spatial_nodes()[a_node].range;
  const auto b = index->spatial_nodes()[b_node].range;
  const auto order = index->spatial_order();
  std::vector<std::size_t> inverse(points.size());
  for (std::size_t rank = 0; rank < order.size(); ++rank) inverse[order[rank]] = rank;
  require(plan.a_credits().size() == a.size() && plan.b_credits().size() == b.size(),
          "plan credit domain size differs");
  std::vector<std::size_t> expected_a(a.size());
  std::iota(expected_a.begin(), expected_a.end(), a.first);
  std::stable_sort(expected_a.begin(), expected_a.end(), [&](auto left, auto right) {
    return plan.a_credits()[left - a.first] < plan.a_credits()[right - a.first];
  });
  std::vector<std::size_t> expected_b(order.begin() + b.first, order.begin() + b.last);
  std::stable_sort(expected_b.begin(), expected_b.end(), [&](auto left, auto right) {
    return plan.b_credits()[inverse[left] - b.first] < plan.b_credits()[inverse[right] - b.first];
  });
  require(std::vector<std::size_t>(plan.a_ranks().begin(), plan.a_ranks().end()) == expected_a &&
              std::vector<std::size_t>(plan.b_order().begin(), plan.b_order().end()) == expected_b,
          "plan confuses stable global A ranks and original B IDs");
  std::set<Pair> emitted, expected;
  std::size_t max_prefix = 0;
  for (const auto rank : expected_a) {
    const auto ca = plan.a_credits()[rank - a.first];
    require(ca <= k, "anchor credit exceeds saturation threshold");
    const auto prefix = plan.prefix_for_a_rank(rank);
    require(prefix <= b.size(), "residual prefix exceeds B");
    max_prefix = std::max(max_prefix, prefix);
    for (std::size_t position = 0; position < prefix; ++position)
      require(emitted.emplace(order[rank], plan.b_order()[position]).second, "duplicate plan pair");
    for (auto br = b.first; br < b.last; ++br) {
      const auto cb = plan.b_credits()[br - b.first];
      require(cb <= k, "opposite credit exceeds saturation threshold");
      const auto depth = oracle(points, order[rank], order[br]).interior.size();
      // The sum may exceed K after independent saturation, but it must
      // remain a valid lower bound because the two local witness sets are disjoint.
      require(static_cast<unsigned>(ca) + cb <= depth, "Pool minorant exceeds strict depth");
      if (static_cast<unsigned>(ca) + cb < k) expected.emplace(order[rank], order[br]);
      require(depth >= k || expected.contains(Pair{order[rank], order[br]}),
              "Pool loses an oracle-admissible pair");
      ++counts.plan_pairs;
    }
  }
  require(emitted == expected && plan.candidate_pairs() == emitted.size() &&
              plan.total_pairs() == a.size() * b.size() && plan.max_prefix() == max_prefix,
          "plan prefix union or pair mass differs");
  ++counts.node_plans;
}

Points large_factor() {
  Points points;
  for (const auto y : {0U, 1U}) for (const auto z : {0U, 1U})
    points.push_back({0, static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z)});
  for (unsigned j = 0; j < 65; ++j)
    points.push_back({static_cast<std::uint16_t>(1000 + j), 0, 0});
  return points;
}

void local_mutants() {
  const Points points{{100, 0, 0}, {0, 1, 0}, {1, 0, 1}};
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const auto a_node = node_with_ids(*index, {0});
  const auto b_node = node_with_ids(*index, {1, 2});
  const auto br = index->spatial_nodes()[b_node].range;
  const auto ar = index->spatial_nodes()[a_node].range.first;
  const mhgp8::audit::NodePoolPlan p1(*index, a_node, b_node, 1);
  require(p1.prefix_for_a_rank(ar) == 1 && p1.b_order()[0] == 2 &&
              index->spatial_order()[br.first] == 1, "real Pool permutation fixture missing");
  require(power(points[0], points[1], points[2]) == 98 &&
              power(points[0], points[2], points[1]) == -101, "closed dot products changed");
  require(oracle(points, 0, p1.b_order()[0]).interior.empty() &&
              oracle(points, 0, index->spatial_order()[br.first]).interior.size() == 1,
          "confusing permutation ranks did not lose the unique admission");
  ++counts.rank_mutants;
  const mhgp8::audit::NodePoolPlan p2(*index, a_node, b_node, 2);
  const auto depth = oracle(points, 0, 1).interior.size();
  const unsigned credit = p2.a_credits()[0] + p2.b_credits()[0];
  require(depth == 1 && credit == 1 && p2.prefix_for_a_rank(ar) == 2 &&
              depth < 2 && depth + credit >= 2, "preloaded credit mutant not exposed");
  ++counts.precredit_mutants;
  using mhgp8::audit::NodePoolPlan;
  for (const auto k : {0U, 11U}) rejects<std::invalid_argument>([&] {
    static_cast<void>(NodePoolPlan(*index, a_node, b_node, k));
  });
  rejects<std::invalid_argument>([&] {
    static_cast<void>(NodePoolPlan(*index, index->spatial_nodes().size(), b_node, 1));
  });
  rejects<std::invalid_argument>([&] { static_cast<void>(NodePoolPlan(*index, a_node, a_node, 1)); });
  rejects<std::invalid_argument>([&] { static_cast<void>(NodePoolPlan(*index, 0, b_node, 1)); });
  rejects<std::invalid_argument>([&] { static_cast<void>(p1.prefix_for_credit(2)); });
  rejects<std::invalid_argument>([&] { static_cast<void>(p1.prefix_for_a_rank(br.first)); });
  NodePoolPlan before_move(*index, a_node, b_node, 1);
  const NodePoolPlan after_move(std::move(before_move));
  require(after_move.candidate_pairs() == 1 && after_move.b_order()[0] == 2,
          "move changed the owned residual");
  rejects<std::logic_error>([&] { static_cast<void>(before_move.candidate_pairs()); });
}

void selftest() {
  const Points permutation{{100, 0, 0}, {0, 1, 0}, {1, 0, 1}};
  check_fixture("permutation_and_precredit", permutation);
  check_fixture("external_witness", {{100, 0, 0}, {100, 4, 0}, {0, 1, 0},
                                     {0, 2, 0}, {0, 3, 0}, {50, 2, 0}});
  check_fixture("full_dimension", {{0, 0, 0}, {1, 1, 0}, {100, 0, 1}, {101, 0, 0}});
  Points sphere;
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y)
    for (int z = -5; z <= 5; ++z) if (x * x + y * y + z * z == 25)
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
  std::uint32_t random_state = 0x31eab45U;
  for (unsigned sample = 0; sample < 3; ++sample) {
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
  check_fixture("large_B_shared_class", large);
  std::reverse(large.begin(), large.end());
  check_fixture("large_B_permuted_IDs", large);
  large = large_factor();
  std::vector<std::size_t> b(65);
  std::iota(b.begin(), b.end(), std::size_t{4});
  for (const auto k : {1U, 2U, 5U, 10U}) {
    check_plan(permutation, {0}, {1, 2}, k);
    check_plan(large, {0, 1, 2, 3}, b, k);
  }
  local_mutants();
  require(counts.fixtures == 10 && counts.pipeline_runs == 840 && counts.node_plans == 8 &&
              counts.supports > 1000 && counts.shell_extra > 100 && counts.oracle_rejected > 100 &&
              counts.plan_pairs > 1000 && counts.rank_mutants == 1 && counts.precredit_mutants == 1,
          "bounded gate nonvacuity failed");
  require(counts.selected_rectangles > 0 && counts.local_roots > 0 &&
              counts.reused_shared_covers > 0 && counts.cutoff64_runs == 48 && counts.rejected_inputs == 8,
          "sparse bridge branch nonvacuity failed");
}
}  // namespace

int main() {
  try {
    selftest();
    std::cout << "{\"status\":\"passed\",\"scope\":\"bounded_audit_pool_bridge\","
              << "\"fixtures\":" << counts.fixtures << ",\"pipeline_runs\":" << counts.pipeline_runs
              << ",\"scalar_site_tests\":" << counts.scalar_site_tests
              << ",\"supports\":" << counts.supports << ",\"extra_shell_supports\":" << counts.shell_extra
              << ",\"oracle_rejections\":" << counts.oracle_rejected
              << ",\"node_plans\":" << counts.node_plans << ",\"plan_pairs\":" << counts.plan_pairs
              << ",\"rank_mutants\":" << counts.rank_mutants
              << ",\"precredit_mutants\":" << counts.precredit_mutants
              << ",\"selected_rectangles\":" << counts.selected_rectangles
              << ",\"local_roots\":" << counts.local_roots
              << ",\"reused_shared_cover_runs\":" << counts.reused_shared_covers
              << ",\"cutoff64_runs\":" << counts.cutoff64_runs
              << ",\"rejected_inputs\":" << counts.rejected_inputs << "}\n";
  } catch (const std::exception& error) {
    std::cerr << "bounded Pool bridge gate failed: " << error.what() << '\n';
    return 1;
  }
}
