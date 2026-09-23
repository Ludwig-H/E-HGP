#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <future>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "exact_ball_oracle.hpp"
#include "lanes/q34_seed.hpp"

namespace {
namespace oracle = mhgp9_gen_test::ball_oracle;
using oracle::Big;
using oracle::Coefficients;
using Points = std::vector<mhgp9::gen::Point3>;
using Ids = std::array<std::size_t, 3>;
using mhgp9::gen::Point3;
using mhgp9::gen::u64;

struct Gate {
  u64 checks{}, calls{}, oracle_completions{}, oracle_sites{}, candidates{}, q3{}, q4{};
  u64 owner_refusals{}, q3_rejected{}, q4_without_q3{}, late_valid{}, max_shell{};
  u64 canonical_refusals{}, positive_refusals{}, depth_refusals{}, exhaustive_clouds{};
  u64 exhaustive_balls{}, invalid_inputs{}, callback_failures{}, parallel_calls{}, judge_mutants{};
  u64 unexamined{}, depth_drop_runs{}, larger_than_minimal{};
  // 18-bit clouds (coordinate_limit = 262143); counted apart so that the
  // historical 16-bit pins keep their values.
  u64 wide_exhaustive{}, wide_random_clouds{};
  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
  template<class Exception = std::invalid_argument, class Function>
  void rejects(Function&& function, const char* message) {
    bool caught = false;
    try { function(); } catch (const Exception&) { caught = true; }
    require(caught, message);
    ++invalid_inputs;
  }
};

struct Candidate {
  unsigned arity{};
  Coefficients key;
  std::vector<std::size_t> support;
  std::size_t depth{};
  std::vector<std::size_t> shell;
  bool operator==(const Candidate&) const = default;
};
using Output = std::vector<Candidate>;

void normalize(Output& output) {
  std::sort(output.begin(), output.end(), [](const auto& a, const auto& b) {
    if (a.arity != b.arity) return a.arity < b.arity;
    if (a.key != b.key) return a.key < b.key;
    return a.support < b.support;
  });
}

auto owner(const Points& points, std::span<const std::size_t> ids) {
  struct Edge { Big length; std::pair<std::size_t, std::size_t> ids; };
  std::vector<Edge> edges;
  for (std::size_t i = 0; i != ids.size(); ++i)
    for (std::size_t j = i + 1; j != ids.size(); ++j) {
      const auto delta = oracle::difference(points[ids[i]], points[ids[j]]);
      edges.push_back({oracle::dot(delta, delta), std::minmax(ids[i], ids[j])});
    }
  std::sort(edges.begin(), edges.end(), [](const auto& a, const auto& b) {
    return a.length != b.length ? a.length > b.length : a.ids < b.ids;
  });
  return edges.front().ids;
}
bool owned(const Points& points, std::span<const std::size_t> ids) {
  const std::pair<std::size_t, std::size_t> proposed = std::minmax(ids[0], ids[1]);
  return owner(points, ids) == proposed;
}
Points select(const Points& points, std::span<const std::size_t> ids) {
  Points result;
  for (const auto id : ids) result.push_back(points[id]);
  return result;
}
Big side(const Points& points, Ids ids, std::size_t id) {
  const auto d = oracle::difference(points[ids[1]], points[ids[0]]);
  const auto u = oracle::difference(points[ids[2]], points[ids[0]]);
  const oracle::Vector normal{d[1] * u[2] - d[2] * u[1], d[2] * u[0] - d[0] * u[2],
                              d[0] * u[1] - d[1] * u[0]};
  return oracle::dot(normal, oracle::difference(points[id], points[ids[0]]));
}

Candidate census(Gate& gate, const Points& points, const oracle::Ball& ball,
                 std::span<const std::size_t> support) {
  Candidate result{static_cast<unsigned>(support.size()), ball.coefficients,
                   {support.begin(), support.end()}, 0, {}};
  std::sort(result.support.begin(), result.support.end());
  for (std::size_t id = 0; id != points.size(); ++id) {
    const auto p = ball.power(points[id]);
    if (p.numerator() < 0) ++result.depth;
    if (p.numerator() == 0) result.shell.push_back(id);
    ++gate.oracle_sites;
  }
  return result;
}

struct Expected {
  Output output;
  mhgp9::gen::Q34SeedWork work{};
};
Expected expected(Gate& gate, const Points& points, Ids ids, std::size_t kmax) {
  gate.require(points.size() <= 40, "q34 oracle exceeded bounded n<=40 domain");
  const auto face = oracle::make(select(points, ids));
  gate.require(face.ball.has_value(), "oracle called with nonpositive seed");
  Expected result;
  auto& work = result.work;
  if (!owned(points, ids)) {
    work.seed_owner_rejections = 1;
    return result;
  }
  if (kmax >= 2) {
    std::size_t depth = 0;
    for (const auto& point : points) {
      ++work.q3_point_tests;
      const auto power = face.ball->power(point);
      if (power.numerator() < 0 && ++depth == kmax - 1) break;
      if (power.numerator() == 0) ++work.q3_shell_ids;
    }
    if (depth >= kmax - 1) work.q3_depth_rejections = 1;
    else {
      result.output.push_back(census(gate, points, *face.ball, ids));
      work.q3_emitted = 1;
    }
  }
  if (kmax < 3) return result;
  struct Root {
    oracle::Ball ball;
    std::vector<std::size_t> members;
  };
  std::map<Coefficients, Root> roots;
  for (std::size_t id = 0; id != points.size(); ++id) {
    ++work.family.sites;
    const auto b = side(points, ids, id);
    if (b == 0) {
      const auto p = face.ball->power(points[id]);
      if (p.numerator() < 0) ++work.family.constant_inside;
      if (p.numerator() == 0) ++work.family.constant_on;
      if (p.numerator() > 0) ++work.family.constant_outside;
      continue;
    }
    if (b > 0) ++work.family.entries;
    else ++work.family.exits;
    ++work.family.event_count;
    const std::array<std::size_t, 4> support{ids[0], ids[1], ids[2], id};
    const auto sphere = oracle::make(select(points, support), false);
    gate.require(sphere.ball.has_value(), "noncoplanar completion had no rational sphere");
    auto [position, inserted] = roots.try_emplace(sphere.ball->coefficients, Root{*sphere.ball, {}});
    static_cast<void>(inserted);
    position->second.members.push_back(id);
    ++gate.oracle_completions;
  }
  for (const auto& [key, root] : roots) {
    static_cast<void>(key);
    ++work.family.groups;
    ++work.family.callbacks;
    work.family.max_group = std::max(work.family.max_group, static_cast<u64>(root.members.size()));
    const std::array<std::size_t, 4> any_support{ids[0], ids[1], ids[2], root.members.front()};
    auto candidate = census(gate, points, root.ball, any_support);
    if (candidate.depth >= kmax - 2) {
      ++work.q4_depth_rejected_groups;
      work.q4_depth_skipped_ids += root.members.size();
      continue;
    }
    bool emitted = false;
    for (std::size_t rank = 0; rank != root.members.size(); ++rank) {
      const auto id = root.members[rank];
      const std::array<std::size_t, 4> support{ids[0], ids[1], ids[2], id};
      ++work.q4_presentations;
      if (!owned(points, support)) { ++work.q4_owner_rejections; continue; }
      ++work.q4_positive_tests;
      const auto positive = oracle::make(select(points, support));
      if (!positive.ball) { ++work.q4_positive_rejections; continue; }
      ++work.q4_seed_tests;
      const std::array<std::size_t, 3> alternative{ids[0], ids[1], id};
      if (id < ids[2] && oracle::make(select(points, alternative)).ball) {
        ++work.q4_seed_rejections;
        continue;
      }
      candidate.support.assign(support.begin(), support.end());
      std::sort(candidate.support.begin(), candidate.support.end());
      result.output.push_back(candidate);
      ++work.q4_emitted;
      work.q4_unexamined_after_emit += root.members.size() - rank - 1;
      if (rank != 0) ++gate.late_valid;
      emitted = true;
      break;
    }
    if (!emitted) ++work.q4_groups_without_support;
  }
  normalize(result.output);
  return result;
}

Candidate copy(const mhgp9::gen::Q34SeedCandidate& value) {
  Candidate result;
  result.arity = value.arity;
  for (std::size_t i = 0; i != 5; ++i) result.key[i] = Big(value.ball.coefficients()[i]);
  result.support.assign(value.support_ids.begin(), value.support_ids.begin() + value.arity);
  result.depth = value.depth;
  result.shell.assign(value.shell_first.begin(), value.shell_first.end());
  result.shell.insert(result.shell.end(), value.shell_second.begin(), value.shell_second.end());
  std::sort(result.shell.begin(), result.shell.end());
  return result;
}
Output collect(const mhgp9::gen::CloudPtr& cloud, Ids ids, std::size_t kmax, mhgp9::gen::Q34SeedWork* work = nullptr) {
  Output result;
  const auto measured = mhgp9::gen::run_q34_seed_candidates(cloud, ids, kmax, [&](const auto& candidate) {
    if (candidate.arity != 3 && candidate.arity != 4) throw std::runtime_error("invalid emitted arity");
    result.push_back(copy(candidate));
  });
  if (work != nullptr) *work = measured;
  normalize(result);
  return result;
}

void check_work(Gate& gate, const mhgp9::gen::Q34SeedWork& got, const mhgp9::gen::Q34SeedWork& wanted) {
#define MHGP9G_COMPARE_Q34_FIELD(name) gate.require(got.name == wanted.name, "q34 ledger " #name)
  MHGP9G_COMPARE_Q34_FIELD(seed_owner_rejections);
  MHGP9G_COMPARE_Q34_FIELD(q3_point_tests);
  MHGP9G_COMPARE_Q34_FIELD(q3_shell_ids);
  MHGP9G_COMPARE_Q34_FIELD(q3_depth_rejections);
  MHGP9G_COMPARE_Q34_FIELD(q3_emitted);
  MHGP9G_COMPARE_Q34_FIELD(q4_depth_rejected_groups);
  MHGP9G_COMPARE_Q34_FIELD(q4_depth_skipped_ids);
  MHGP9G_COMPARE_Q34_FIELD(q4_presentations);
  MHGP9G_COMPARE_Q34_FIELD(q4_owner_rejections);
  MHGP9G_COMPARE_Q34_FIELD(q4_positive_tests);
  MHGP9G_COMPARE_Q34_FIELD(q4_positive_rejections);
  MHGP9G_COMPARE_Q34_FIELD(q4_seed_tests);
  MHGP9G_COMPARE_Q34_FIELD(q4_seed_rejections);
  MHGP9G_COMPARE_Q34_FIELD(q4_groups_without_support);
  MHGP9G_COMPARE_Q34_FIELD(q4_unexamined_after_emit);
  MHGP9G_COMPARE_Q34_FIELD(q4_emitted);
  MHGP9G_COMPARE_Q34_FIELD(family.sites);
  MHGP9G_COMPARE_Q34_FIELD(family.entries);
  MHGP9G_COMPARE_Q34_FIELD(family.exits);
  MHGP9G_COMPARE_Q34_FIELD(family.constant_inside);
  MHGP9G_COMPARE_Q34_FIELD(family.constant_on);
  MHGP9G_COMPARE_Q34_FIELD(family.constant_outside);
  MHGP9G_COMPARE_Q34_FIELD(family.event_count);
  MHGP9G_COMPARE_Q34_FIELD(family.groups);
  MHGP9G_COMPARE_Q34_FIELD(family.max_group);
  MHGP9G_COMPARE_Q34_FIELD(family.callbacks);
#undef MHGP9G_COMPARE_Q34_FIELD
  gate.require(got.seed_owner_tests > 0 && got.seed_owner_tests <= 2, "seed owner comparison ledger");
  gate.require(got.q4_owner_tests >= got.q4_presentations && got.q4_owner_tests <= 5 * got.q4_presentations,
               "tetrahedron owner comparison ledger");
  gate.require(got.q3_shell_capacity_bytes % sizeof(std::size_t) == 0 &&
               got.q3_shell_capacity_bytes >= got.q3_shell_ids * sizeof(std::size_t), "q3 shell capacity ledger");
  gate.require(got.family.group_comparisons == (got.family.event_count == 0 ? 0 : got.family.event_count - 1),
               "q4 root partition comparison ledger");
}

Output check_case(Gate& gate, const Points& points, Ids ids, std::size_t kmax) {
  const auto reference = expected(gate, points, ids, kmax);
  const auto cloud = mhgp9::gen::prepare_cloud(points);
  mhgp9::gen::Q34SeedWork work;
  const auto actual = collect(cloud, ids, kmax, &work);
  gate.require(actual == reference.output, "seed candidates differ from rational brute-force oracle");
  check_work(gate, work, reference.work);
  gate.require(Points(cloud->points().begin(), cloud->points().end()) == points, "seed consumer mutated cloud");
  for (const auto& candidate : actual) {
    gate.require(std::adjacent_find(candidate.shell.begin(), candidate.shell.end()) == candidate.shell.end(),
                 "emitted shell contains duplicate IDs");
    gate.max_shell = std::max(gate.max_shell, static_cast<u64>(candidate.shell.size()));
    if (candidate.arity == 3) ++gate.q3;
    else ++gate.q4;
  }
  gate.owner_refusals += work.seed_owner_rejections;
  gate.q3_rejected += work.q3_depth_rejections;
  if (work.q3_depth_rejections != 0 && work.q4_emitted != 0) ++gate.q4_without_q3;
  gate.canonical_refusals += work.q4_seed_rejections;
  gate.positive_refusals += work.q4_positive_rejections;
  gate.depth_refusals += work.q4_depth_rejected_groups;
  gate.unexamined += work.q4_unexamined_after_emit;
  if (work.q4_depth_rejected_groups != 0 && work.q4_emitted != 0) ++gate.depth_drop_runs;
  gate.candidates += actual.size();
  ++gate.calls;
  return actual;
}

void add_unique(Points& points, Point3 point) {
  if (std::find(points.begin(), points.end(), point) == points.end()) points.push_back(point);
}
Points shell_fixture() {
  Points points{{23, 24, 20}, {20, 17, 24}, {20, 17, 16}, {17, 24, 20}};
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y)
    for (int z = -5; z <= 5; ++z) if (x * x + y * y + z * z == 25)
      add_unique(points, {static_cast<mhgp9::gen::Coordinate>(x + 20), static_cast<mhgp9::gen::Coordinate>(y + 20),
                          static_cast<mhgp9::gen::Coordinate>(z + 20)});
  return points;
}

void fixtures(Gate& gate) {
  const Points late{{0, 0, 0}, {2, 2, 0}, {2, 2, 2}, {2, 0, 2}, {0, 2, 2}};
  const auto late_output = check_case(gate, late, {0, 1, 3}, 3);
  const auto kept = std::find_if(late_output.begin(), late_output.end(), [](const auto& value) { return value.arity == 4; });
  gate.require(kept != late_output.end() && kept->support == std::vector<std::size_t>{0, 1, 3, 4},
               "first invalid owner concealed the later valid presentation");
  for (const auto k : {1U, 2U, 3U, 5U, 10U}) {
    static_cast<void>(check_case(gate, late, {1, 0, 3}, k));
    static_cast<void>(check_case(gate, late, {0, 3, 1}, k));  // Wrong proposed owner.
    static_cast<void>(check_case(gate, late, {0, 1, 4}, k));  // Earlier acute face is canonical.
    static_cast<void>(check_case(gate, shell_fixture(), {0, 1, 2}, k));
  }
  const Points face_boundary{{15, 10, 10}, {7, 14, 10}, {7, 6, 10}, {10, 10, 15}};
  const auto boundary_output = check_case(gate, face_boundary, {0, 1, 2}, 10);
  gate.require(std::none_of(boundary_output.begin(), boundary_output.end(), [](const auto& c) { return c.arity == 4; }),
               "centre on seed face became a strictly positive tetrahedron");
  const Points decline{{57, 50, 51}, {45, 55, 50}, {45, 45, 50}, {57, 50, 49}, {57, 50, 48}};
  const auto declining = check_case(gate, decline, {0, 1, 2}, 3);
  gate.require(std::any_of(declining.begin(), declining.end(), [](const auto& c) {
    return c.arity == 4 && c.depth == 0 && c.support == std::vector<std::size_t>{0, 1, 2, 3};
  }), "deep early root suppressed a later shallow admissible root");
  const auto shell = shell_fixture();
  for (const auto& candidate : check_case(gate, shell, {0, 1, 2}, 10)) {
    if (candidate.arity != 4 || candidate.shell.size() != 30) continue;
    bool smaller = false;
    for (std::size_t i = 0; i != candidate.shell.size() && !smaller; ++i)
      for (std::size_t j = i + 1; j != candidate.shell.size() && !smaller; ++j) {
        const std::array<std::size_t, 2> pair{candidate.shell[i], candidate.shell[j]};
        const auto diameter = oracle::make(select(shell, pair));
        smaller = diameter.ball && diameter.ball->coefficients == candidate.key;
      }
    if (smaller) ++gate.larger_than_minimal;
  }
  for (const auto k : {5U, 10U}) {
    Points cloud{{900, 1000, 1000}, {1100, 1000, 1000},
                  {1000, 1120, 1040}, {1000, 1120, 960}};
    for (unsigned j = 0; j != k - 1; ++j)
      cloud.push_back({static_cast<mhgp9::gen::Coordinate>(1000 + j), 1020, 1105});
    const auto output = check_case(gate, cloud, {0, 1, 2}, k);
    gate.require(std::none_of(output.begin(), output.end(), [](const auto& c) { return c.arity == 3; }) &&
                 std::any_of(output.begin(), output.end(), [](const auto& c) { return c.arity == 4; }),
                 "q3 rejection incorrectly suppressed independent q4 route");
    auto reversed = cloud;
    std::reverse(reversed.begin(), reversed.end());
    static_cast<void>(check_case(gate, reversed, {cloud.size() - 1, cloud.size() - 2, cloud.size() - 3}, k));
  }
}

using BallSet = std::set<std::pair<unsigned, Coefficients>>;
void exhaustive(Gate& gate, const Points& points, std::size_t kmax) {
  BallSet wanted, actual;
  for (std::size_t a = 0; a != points.size(); ++a)
    for (std::size_t b = a + 1; b != points.size(); ++b)
      for (std::size_t x = b + 1; x != points.size(); ++x) {
        const Ids sorted{a, b, x};
        const auto triangle = oracle::make(select(points, sorted));
        if (triangle.ball && kmax >= 2) {
          const auto item = census(gate, points, *triangle.ball, sorted);
          if (item.depth < kmax - 1) wanted.emplace(3, item.key);
        }
        if (triangle.ball) {
          const auto edge = owner(points, sorted);
          const auto third = a != edge.first && a != edge.second ? a : b != edge.first && b != edge.second ? b : x;
          for (const auto& item : check_case(gate, points, {edge.first, edge.second, third}, kmax))
            actual.emplace(item.arity, item.key);
        }
        for (std::size_t y = x + 1; y != points.size(); ++y) {
          const std::array<std::size_t, 4> support{a, b, x, y};
          const auto tetra = oracle::make(select(points, support));
          if (!tetra.ball || kmax < 3) continue;
          const auto item = census(gate, points, *tetra.ball, support);
          if (item.depth < kmax - 2) wanted.emplace(4, item.key);
        }
      }
  gate.require(actual == wanted, "all canonical seeds missed a positive ball in exhaustive small cloud");
  gate.exhaustive_balls += wanted.size();
  ++gate.exhaustive_clouds;
}

void lifecycle(Gate& gate) {
  const Points points{{0, 0, 0}, {2, 2, 0}, {2, 2, 2}, {2, 0, 2}, {0, 2, 2}};
  auto cloud = mhgp9::gen::prepare_cloud(points);
  const auto expected_output = collect(cloud, {0, 1, 3}, 10);
  const mhgp9::gen::Q34SeedConsumer callback = [](const auto&) {};
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q34_seed_candidates({}, {0, 1, 3}, 10, callback)); }, "null cloud accepted");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q34_seed_candidates(cloud, {0, 1, 3}, 0, callback)); }, "zero K accepted");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q34_seed_candidates(cloud, {0, 1, 3}, 10, {})); }, "empty callback accepted");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q34_seed_candidates(cloud, {0, 0, 3}, 10, callback)); }, "repeated ID accepted");
  gate.rejects<std::out_of_range>([&] {
    static_cast<void>(mhgp9::gen::run_q34_seed_candidates(cloud, {0, 1, points.size()}, 10, callback));
  }, "invalid ID accepted");
  const auto right = mhgp9::gen::prepare_cloud(Points{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}});
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q34_seed_candidates(right, {0, 1, 2}, 10, callback)); }, "right seed accepted");
  struct CallbackFailure final : std::exception {};
  std::size_t calls = 0;
  bool caught = false;
  try {
    static_cast<void>(mhgp9::gen::run_q34_seed_candidates(cloud, {0, 1, 3}, 10, [&](const auto&) {
      ++calls;
      throw CallbackFailure{};
    }));
  } catch (const CallbackFailure&) { caught = true; }
  gate.require(caught && calls == 1, "callback failure was swallowed or followed by further emissions");
  gate.require(collect(cloud, {0, 1, 3}, 10) == expected_output, "failed call corrupted next call");
  ++gate.callback_failures;
  std::vector<std::future<Output>> futures;
  for (unsigned worker = 0; worker != 4; ++worker)
    futures.push_back(std::async(std::launch::async, [cloud] { return collect(cloud, {0, 1, 3}, 10); }));
  for (auto& future : futures) {
    gate.require(future.get() == expected_output, "concurrent independent seed calls disagree");
    ++gate.parallel_calls;
  }
  calls = 0;
  static_cast<void>(mhgp9::gen::run_q34_seed_candidates(cloud, {0, 1, 3}, 10, [&](const auto&) {
    ++calls;
    cloud.reset();
  }));
  gate.require(!cloud && calls == expected_output.size(), "seed did not own cloud across callback reset");
  auto wrong = expected_output;
  wrong.pop_back();
  gate.require(wrong != expected_output, "judge missed omitted q4 candidate");
  ++gate.judge_mutants;
  wrong = expected_output;
  ++wrong.front().depth;
  gate.require(wrong != expected_output, "judge missed non-strict interior count");
  ++gate.judge_mutants;
  wrong = expected_output;
  wrong.back().shell.pop_back();
  gate.require(wrong != expected_output, "judge missed extra-shell member");
  ++gate.judge_mutants;
}

void run(Gate& gate) {
  fixtures(gate);
  std::uint64_t state = 0x31c6e17058f29a44ULL;
  auto next = [&]() {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    return state >> 32;
  };
  for (unsigned trial = 0; trial != 24; ++trial) {
    Points points;
    const unsigned side_length = trial % 3 == 0 ? 65536U : 13U;
    while (points.size() != 8)
      add_unique(points, {static_cast<mhgp9::gen::Coordinate>(next() % side_length),
                          static_cast<mhgp9::gen::Coordinate>(next() % side_length),
                          static_cast<mhgp9::gen::Coordinate>(next() % side_length)});
    unsigned seeds = 0;
    for (std::size_t a = 0; a != points.size() && seeds != 3; ++a)
      for (std::size_t b = a + 1; b != points.size() && seeds != 3; ++b)
        for (std::size_t x = b + 1; x != points.size() && seeds != 3; ++x) {
          const Ids ids{a, b, x};
          if (!oracle::make(select(points, ids)).ball) continue;
          const auto edge = owner(points, ids);
          const auto third = a != edge.first && a != edge.second ? a : b != edge.first && b != edge.second ? b : x;
          for (const auto k : {1U, 2U, 3U, 5U, 10U})
            static_cast<void>(check_case(gate, points, {edge.first, edge.second, third}, k));
          ++seeds;
        }
    gate.require(seeds == 3, "random cloud lacked three positive seeds");
    if (trial < 4) for (const auto k : {3U, 5U, 10U}) exhaustive(gate, points, k);
  }
  const Points regular{{0, 0, 0}, {2, 2, 0}, {2, 0, 2}, {0, 2, 2}};
  exhaustive(gate, regular, 3);
  // 18-bit corner cloud: 262143 = coordinate_limit, 131071/131072 the two
  // middle values. This gate had no engraved 16-bit corner cloud (its widest
  // inputs were the side-65536 random trials above, which stay pinned).
  const Points corners18{{0, 0, 0}, {262143, 262143, 0}, {262143, 0, 262143}, {0, 262143, 262143},
                         {131071, 131072, 131071}, {262143, 262143, 262143}};
  for (const auto k : {3U, 5U, 10U}) {
    exhaustive(gate, corners18, k);
    ++gate.wide_exhaustive;
  }
  // Separate 18-bit random clouds (own generator; each must leave the 16-bit
  // range, otherwise it proves nothing that the u16 trials did not).
  std::uint64_t wide_state = 0x5a2f8c1d93e7b604ULL;
  auto wide_next = [&]() {
    wide_state = wide_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return wide_state >> 32;
  };
  for (unsigned trial = 0; trial != 3; ++trial) {
    Points points;
    while (points.size() != 8)
      add_unique(points, {static_cast<mhgp9::gen::Coordinate>(wide_next() % 262144U),
                          static_cast<mhgp9::gen::Coordinate>(wide_next() % 262144U),
                          static_cast<mhgp9::gen::Coordinate>(wide_next() % 262144U)});
    gate.require(std::any_of(points.begin(), points.end(), [](const Point3& point) {
      return point.x > 65535 || point.y > 65535 || point.z > 65535;
    }), "18-bit random cloud stayed inside the 16-bit range");
    unsigned seeds = 0;
    for (std::size_t a = 0; a != points.size() && seeds != 3; ++a)
      for (std::size_t b = a + 1; b != points.size() && seeds != 3; ++b)
        for (std::size_t x = b + 1; x != points.size() && seeds != 3; ++x) {
          const Ids ids{a, b, x};
          if (!oracle::make(select(points, ids)).ball) continue;
          const auto edge = owner(points, ids);
          const auto third = a != edge.first && a != edge.second ? a : b != edge.first && b != edge.second ? b : x;
          for (const auto k : {1U, 2U, 3U, 5U, 10U})
            static_cast<void>(check_case(gate, points, {edge.first, edge.second, third}, k));
          ++seeds;
        }
    gate.require(seeds == 3, "18-bit random cloud lacked three positive seeds");
    if (trial == 0) for (const auto k : {3U, 5U, 10U}) { exhaustive(gate, points, k); ++gate.wide_exhaustive; }
    ++gate.wide_random_clouds;
  }
  lifecycle(gate);
  gate.require(gate.calls >= 500 && gate.oracle_completions >= 1500 && gate.oracle_sites >= 10000 &&
               gate.candidates >= 100 && gate.q3 >= 100 && gate.q4 >= 20, "q34 correctness nonvacuity floor");
  gate.require(gate.owner_refusals >= 5 && gate.q3_rejected >= 10 && gate.q4_without_q3 >= 2 &&
               gate.late_valid > 0 && gate.max_shell >= 30 && gate.canonical_refusals > 0 &&
               gate.positive_refusals > 0 && gate.depth_refusals > 0 && gate.unexamined > 0 &&
               gate.depth_drop_runs > 0 && gate.larger_than_minimal > 0, "q34 adversarial nonvacuity floor");
  gate.require(gate.exhaustive_clouds == 13 + gate.wide_exhaustive && gate.exhaustive_balls > 20 && gate.invalid_inputs == 6 &&
               gate.callback_failures == 1 && gate.parallel_calls == 4 && gate.judge_mutants == 3,
               "q34 exhaustive/lifecycle nonvacuity floor");
  gate.require(gate.wide_exhaustive == 6 && gate.wide_random_clouds == 3, "q34 18-bit cloud nonvacuity floor");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp9_gen_q34_seed_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    run(gate);
    std::cout << "{\"schema\":\"mhgp9_gen_q34_seed_gate_v1\",\"status\":\"passed\""
              << ",\"checks\":" << gate.checks << ",\"calls\":" << gate.calls
              << ",\"oracle_completions\":" << gate.oracle_completions << ",\"oracle_sites\":" << gate.oracle_sites
              << ",\"candidates\":" << gate.candidates << ",\"q3\":" << gate.q3 << ",\"q4\":" << gate.q4
              << ",\"owner_refusals\":" << gate.owner_refusals << ",\"q3_rejected\":" << gate.q3_rejected
              << ",\"q4_without_q3\":" << gate.q4_without_q3 << ",\"late_valid\":" << gate.late_valid
              << ",\"max_shell\":" << gate.max_shell << ",\"canonical_refusals\":" << gate.canonical_refusals
              << ",\"positive_refusals\":" << gate.positive_refusals << ",\"depth_refusals\":" << gate.depth_refusals
              << ",\"exhaustive_clouds\":" << gate.exhaustive_clouds << ",\"exhaustive_balls\":" << gate.exhaustive_balls
              << ",\"invalid_inputs\":" << gate.invalid_inputs << ",\"callback_failures\":" << gate.callback_failures
              << ",\"parallel_calls\":" << gate.parallel_calls << ",\"judge_mutants\":" << gate.judge_mutants
              << ",\"unexamined\":" << gate.unexamined << ",\"depth_drop_runs\":" << gate.depth_drop_runs
              << ",\"larger_than_minimal\":" << gate.larger_than_minimal
              << ",\"wide_exhaustive\":" << gate.wide_exhaustive << ",\"wide_random_clouds\":" << gate.wide_random_clouds << "}\n";
  } catch (const std::exception& error) {
    std::cerr << "q34 seed gate: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
