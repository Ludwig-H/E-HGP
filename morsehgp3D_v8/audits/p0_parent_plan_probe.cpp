// Independent audit adapter over current plans; no parallel product API.
#include "pipeline/local_credits.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace mhgp8;
using Pair = std::pair<std::size_t, std::size_t>;
using Pairs = std::vector<Pair>;

struct Checks {
  u64 matrix_plans{}, partition_runs{}, jobs{}, candidate_pairs{};
  u64 scalar_point_tests{}, geometric_pairs{}, rejected_mutants{};
} checks;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

Pairs canonical(Pairs pairs) {
  std::sort(pairs.begin(), pairs.end());
  require(std::adjacent_find(pairs.begin(), pairs.end()) == pairs.end(),
          "duplicate pair emission");
  return pairs;
}

unsigned depth(const PreparedRectangle& rectangle, Pair pair) {
  unsigned result = 0;
  const auto points = rectangle.points();
  const auto& a = points[pair.first];
  const auto& b = points[pair.second];
  for (const auto& z : points) {
    std::int64_t h = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      h += (static_cast<std::int64_t>(z[axis]) - a[axis]) *
           (static_cast<std::int64_t>(b[axis]) - z[axis]);
    }
    ++checks.scalar_point_tests;
    result += h > 0 ? 1U : 0U;
  }
  return result;
}

Pairs native(const CreditPlan& plan) {
  Pairs result;
  plan.for_each_candidate([&](std::size_t a, std::size_t b) {
    result.emplace_back(a, b);
  });
  require(result.size() == plan.candidate_pairs(), "native candidate count");
  return canonical(std::move(result));
}

struct Prefixes {
  const CreditPlan* parent;
  std::vector<std::size_t> ends;
};

Prefixes prepare_prefixes(const CreditPlan& plan) {
  require(plan.lane() == Lane::Q2, "prefix adapter requires q2");
  const auto need = static_cast<unsigned>(plan.threshold() - plan.core_credit());
  Prefixes result{&plan, std::vector<std::size_t>(need, 0)};
  const auto first = plan.rectangle().a_range().first;
  for (const auto& block : plan.blocks()) {
    const auto a = plan.a_order()[block.a.first];
    const auto credit = plan.a_credits()[a - first];
    require(credit < need, "saturated class emitted a block");
    result.ends[credit] = std::max(result.ends[credit], block.b.last);
  }
  return result;
}

struct Job { const CreditPlan* parent; Range ranks; };

std::vector<Job> partition(const CreditPlan& plan, std::size_t count) {
  const auto size = plan.a_order().size();
  require(count != 0 && count <= size, "invalid partition size");
  std::vector<Job> result;
  for (std::size_t i = 0; i < count; ++i) {
    result.push_back({&plan, {size * i / count, size * (i + 1) / count}});
  }
  return result;
}

Pairs dispatch(const Prefixes& prefixes, const std::vector<Job>& jobs,
               bool confuse_id_and_rank = false) {
  const auto& plan = *prefixes.parent;
  const auto order = plan.a_order();
  std::size_t next = 0;
  // Validate the complete partition before any candidate emission.
  for (const auto& job : jobs) {
    require(job.parent == &plan, "wrong parent plan identity");
    require(job.ranks.first == next && job.ranks.first < job.ranks.last &&
            job.ranks.last <= order.size(), "invalid job partition");
    next = job.ranks.last;
  }
  require(next == order.size(), "incomplete job partition");
  Pairs result;
  const auto first = plan.rectangle().a_range().first;
  for (const auto& job : jobs) {
    for (auto rank = job.ranks.first; rank < job.ranks.last; ++rank) {
      const auto a = order[rank];
      const auto credit = plan.a_credits()[a - first];
      if (credit >= prefixes.ends.size()) continue;
      for (std::size_t b = 0; b < prefixes.ends[credit]; ++b) {
        result.emplace_back(confuse_id_and_rank ? first + rank : a,
                            plan.b_order()[b]);
      }
    }
  }
  return canonical(std::move(result));
}

template <class Operation>
void reject(Operation operation, const char* expected) {
  try { operation(); }
  catch (const std::runtime_error& error) {
    require(std::string(error.what()) == expected, "unrelated mutant rejection");
    ++checks.rejected_mutants;
    return;
  }
  throw std::runtime_error("mutant survived");
}

CreditPlan pool(CloudPtr cloud, Range a, Range b, unsigned k, unsigned s,
                std::vector<std::size_t> core = {}) {
  return make_credit_plan(prepare_rectangle(std::move(cloud),
      RectangleSpec{a, b, std::move(core)}, k, s), Lane::Q2, Strategy::Pool);
}

Pairs admitted(const PreparedRectangle& rectangle, const Pairs& candidates) {
  Pairs result;
  for (const auto& pair : candidates) {
    if (depth(rectangle, pair) < rectangle.kmax()) result.push_back(pair);
  }
  return canonical(std::move(result));
}

void minimal_fixtures() {
  const auto cloud = prepare_cloud(std::vector<Point3>{{0, 0, 0}, {1, 0, 0},
                                                      {100, 0, 0}});
  for (const auto s : {8U, 10U, 12U}) {
    const auto parent = pool(cloud, {0, 2}, {2, 3}, 1, s);
    const auto expected = native(parent);
    require(expected == Pairs{{1, 2}}, "parent lost the local witness");
    Pairs rebuilt;
    for (std::size_t a = 0; a < 2; ++a) {
      const auto child = pool(cloud, {a, a + 1}, {2, 3}, 1, s);
      const auto pairs = native(child);
      rebuilt.insert(rebuilt.end(), pairs.begin(), pairs.end());
    }
    require(canonical(rebuilt) == Pairs{{0, 2}, {1, 2}}, "child residual fixture");
    require(admitted(parent.rectangle(), rebuilt) == expected, "child scalar census");
    const auto prepared = prepare_prefixes(parent);
    const auto jobs = partition(parent, 2);
    require(dispatch(prepared, jobs) == expected, "parent jobs changed residual");
    auto overlap = jobs;
    overlap[1].ranks.first = 0;
    reject([&] { static_cast<void>(dispatch(prepared, overlap)); }, "invalid job partition");
    reject([&] {
      require(dispatch(prepared, jobs, true) == expected, "ID/rank changed pairs");
    }, "ID/rank changed pairs");
    const auto copied = parent;
    auto foreign = jobs;
    foreign[0].parent = &copied;
    reject([&] { static_cast<void>(dispatch(prepared, foreign)); }, "wrong parent plan identity");

    const auto p2 = pool(cloud, {0, 2}, {2, 3}, 2, s);
    const auto child = pool(cloud, {0, 1}, {2, 3}, 2, s, {1});
    const unsigned lp = p2.core_credit() + p2.a_credits()[0] + p2.b_credits()[0];
    const unsigned lc = child.core_credit() + child.a_credits()[0] + child.b_credits()[0];
    const auto actual = depth(p2.rectangle(), {0, 2});
    require(lp == 1 && lc == 1 && actual == 1, "overlapping witness fixture");
    require(p2.keeps(0, 2) && child.keeps(0, 2), "safe residual intersection");
    require(std::max(lp, lc) < 2 && lp + lc >= 2,
            "false addition did not eliminate an admissible pair");
  }
}

void matrix() {
  for (const auto na : {2U, 4U, 8U}) for (const auto nb : {2U, 4U, 8U}) {
    std::vector<Point3> points;
    for (unsigned a = 0; a < na; ++a) points.push_back({static_cast<std::uint16_t>(a), 0, 0});
    for (unsigned b = 0; b < nb; ++b) points.push_back({static_cast<std::uint16_t>(1000 + b), 0, 0});
    const auto cloud = prepare_cloud(points);
    for (const auto k : {1U, 2U, 5U, 10U}) for (const auto s : {8U, 10U, 12U}) {
      const auto plan = pool(cloud, {0, na}, {na, na + nb}, k, s);
      const auto expected = native(plan);
      ++checks.matrix_plans;
      for (std::size_t a = 0; a < na; ++a) for (std::size_t b = na; b < na + nb; ++b) {
        const auto actual = depth(plan.rectangle(), {a, b});
        const bool kept = std::binary_search(expected.begin(), expected.end(), Pair{a, b});
        const unsigned lower = plan.core_credit() + plan.a_credits()[a] + plan.b_credits()[b - na];
        require(lower <= actual, "parent credit exceeds scalar depth");
        require(kept == plan.keeps(a, b), "native expansion disagrees with keeps");
        require(actual >= k || kept, "parent filter loses admissible geometry");
        ++checks.geometric_pairs;
      }
      const auto prepared = prepare_prefixes(plan);
      for (const auto count : std::set<std::size_t>{1, 2, na}) {
        const auto jobs = partition(plan, count);
        require(dispatch(prepared, jobs) == expected, "matrix job residual mismatch");
        ++checks.partition_runs;
        checks.jobs += jobs.size();
        checks.candidate_pairs += expected.size();
      }
    }
  }
}
}  // namespace

int main() {
  try {
    minimal_fixtures();
    matrix();
    require(checks.matrix_plans == 108 && checks.partition_runs == 288 &&
            checks.jobs == 756 && checks.candidate_pairs > 1000 &&
            checks.geometric_pairs == 2352 && checks.rejected_mutants == 9,
            "non-vacuity floor failed");
    std::cout << "{\"status\":\"passed\",\"scope\":\"independent_parent_plan_job_adapter\","
              << "\"minimal_fixture_separations\":3,\"parent_residual\":1,\"rebuilt_child_residual\":2,"
              << "\"overlap_max\":1,\"overlap_false_sum\":2,\"matrix_plans\":" << checks.matrix_plans
              << ",\"partition_runs\":" << checks.partition_runs << ",\"jobs\":" << checks.jobs
              << ",\"candidate_pairs\":" << checks.candidate_pairs
              << ",\"geometric_pairs\":" << checks.geometric_pairs
              << ",\"scalar_point_tests\":" << checks.scalar_point_tests
              << ",\"rejected_mutants\":" << checks.rejected_mutants
              << ",\"product_parallel_api_tested\":false,\"timing_claim\":false}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "parent plan audit failed: " << error.what() << '\n';
    return 1;
  }
}
