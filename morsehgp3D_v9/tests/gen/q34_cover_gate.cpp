#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <future>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "exact_ball_oracle.hpp"
#include "lanes/edge_cover.hpp"
#include "lanes/q34_cover.hpp"
#include "lanes/q34_seed.hpp"
#include "lanes/q34_pruning.hpp"
#include "lanes/q4_local.hpp"
#include "lanes/q4_local_partition.hpp"
#include "lanes/q4_positive_domain.hpp"
#include "lanes/q4_shallow.hpp"
#include "lanes/q4_window.hpp"

// Exhaustive only on small clouds. The cover is checked by the unexpanded
// rational inequality; positive balls and every retained payload are judged
// by Gram elimination over ALL sites, not by the covered family counts.
namespace {
namespace oracle = mhgp9_gen_test::ball_oracle;
using oracle::Big;
using oracle::Coefficients;
using Points = std::vector<mhgp9::gen::Point3>;
using Edge = std::array<std::size_t, 2>;
using Ids = std::array<std::size_t, 3>;
using mhgp9::gen::Point3;
using mhgp9::gen::u64;

static_assert(!std::is_default_constructible_v<mhgp9::gen::Q34EdgeCover>);
static_assert(!std::is_copy_constructible_v<mhgp9::gen::Q34EdgeCover>);
static_assert(!std::is_copy_assignable_v<mhgp9::gen::Q34EdgeCover>);

struct Gate {
  u64 checks{}, clouds{}, covers{}, edge_calls{}, seed_calls{}, reference_calls{};
  u64 oracle_completions{}, oracle_sites{}, candidates{}, q3{}, q4{}, max_shell{};
  u64 cover_boundary{}, cover_excluded{}, cover_admitted_nodes{}, cover_rejected_nodes{};
  u64 cover_split_nodes{}, reused_covers{}, q4_without_q3{}, invalid_root_depth_changed{};
  u64 diametral_cores{}, diametral_members{}, diametral_cover_only{}, diametral_refusals{};
  u64 exhaustive_edges{}, permutations{}, invalid_inputs{}, callback_failures{};
  u64 parallel_calls{}, judge_mutants{};
  u64 edge_seed_count{}, edge_pruned_sites{}, covered_site_reads{};
  // 18-bit twins (coordinate_limit = 262143); counted apart so that the
  // historical 16-bit pins above keep their values.
  u64 wide_clouds{}, wide_edge_calls{};
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

Big cover_power(const Points& points, Edge edge, Point3 point) {
  const auto d = oracle::difference(points[edge[1]], points[edge[0]]);
  Big value = -4 * oracle::dot(d, d);
  for (std::size_t axis = 0; axis != 3; ++axis) {
    const Big distance = 2 * Big(point[axis]) - points[edge[0]][axis] - points[edge[1]][axis];
    value += distance * distance;
  }
  return value;
}
Points select(const Points& points, std::span<const std::size_t> ids) {
  Points result;
  for (const auto id : ids) result.push_back(points[id]);
  return result;
}
Edge owner(const Points& points, std::span<const std::size_t> ids) {
  struct Item { Big length; Edge edge; };
  std::vector<Item> edges;
  for (std::size_t i = 0; i != ids.size(); ++i)
    for (std::size_t j = i + 1; j != ids.size(); ++j) {
      const auto difference = oracle::difference(points[ids[i]], points[ids[j]]);
      edges.push_back({oracle::dot(difference, difference),
                       {std::min(ids[i], ids[j]), std::max(ids[i], ids[j])}});
    }
  std::sort(edges.begin(), edges.end(), [](const auto& first, const auto& second) {
    return first.length != second.length ? first.length > second.length : first.edge < second.edge;
  });
  return edges.front().edge;
}

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
  std::sort(output.begin(), output.end(), [](const auto& first, const auto& second) {
    if (first.arity != second.arity) return first.arity < second.arity;
    if (first.key != second.key) return first.key < second.key;
    return first.support < second.support;
  });
}
Candidate census(Gate& gate, const Points& points, const oracle::Ball& ball,
                 std::span<const std::size_t> support) {
  Candidate result{static_cast<unsigned>(support.size()), ball.coefficients,
                   {support.begin(), support.end()}, 0, {}};
  std::sort(result.support.begin(), result.support.end());
  for (std::size_t id = 0; id != points.size(); ++id) {
    const auto power = ball.power(points[id]);
    if (power.numerator() < 0) ++result.depth;
    if (power.numerator() == 0) result.shell.push_back(id);
    ++gate.oracle_sites;
  }
  return result;
}
Candidate copy(const mhgp9::gen::Q34SeedCandidate& value) {
  if (value.arity != 3 && value.arity != 4) throw std::runtime_error("cover emitted invalid arity");
  if (!std::is_sorted(value.support_ids.begin(), value.support_ids.begin() + value.arity) ||
      !std::is_sorted(value.shell_first.begin(), value.shell_first.end()) ||
      !std::is_sorted(value.shell_second.begin(), value.shell_second.end()))
    throw std::runtime_error("cover emitted unsorted support or borrowed shell part");
  if (value.arity == 3 && (!value.shell_second.empty() ||
      value.support_ids[3] != std::numeric_limits<std::size_t>::max()))
    throw std::runtime_error("q3 candidate has a second shell part or invalid unused support ID");
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

Output oracle_seed(Gate& gate, const Points& points, Edge edge, std::size_t x, std::size_t kmax) {
  const Ids seed{edge[0], edge[1], x};
  const auto face = oracle::make(select(points, seed));
  if (!face.ball || owner(points, seed) != edge) return {};
  Output result;
  if (kmax >= 2) {
    auto candidate = census(gate, points, *face.ball, seed);
    if (candidate.depth < kmax - 1) result.push_back(std::move(candidate));
  }
  if (kmax < 3) return result;
  std::map<Coefficients, Candidate> roots;
  for (std::size_t y = 0; y != points.size(); ++y) {
    if (y == edge[0] || y == edge[1] || y == x) continue;
    const std::array<std::size_t, 4> support{edge[0], edge[1], x, y};
    const auto ball = oracle::make(select(points, support));
    ++gate.oracle_completions;
    if (!ball.ball || owner(points, support) != edge) continue;
    if (y < x && oracle::make(select(points, Ids{edge[0], edge[1], y})).ball) continue;
    auto candidate = census(gate, points, *ball.ball, support);
    if (candidate.depth < kmax - 2)
      roots.try_emplace(candidate.key, std::move(candidate));  // First valid original ID.
  }
  for (auto& [key, candidate] : roots) {
    static_cast<void>(key);
    result.push_back(std::move(candidate));
  }
  normalize(result);
  return result;
}

std::vector<std::size_t> check_cover(Gate& gate, const Points& points,
                                    const mhgp9::gen::Q34EdgeCoverPtr& cover, Edge edge) {
  std::sort(edge.begin(), edge.end());
  gate.require(cover && cover->edge_ids() == edge, "cover lost canonical original endpoint IDs");
  std::vector<std::size_t> wanted, actual;
  for (std::size_t id = 0; id != points.size(); ++id) {
    const auto power = cover_power(points, edge, points[id]);
    if (power <= 0) wanted.push_back(id);
    if (power == 0) ++gate.cover_boundary;
    if (power > 0) ++gate.cover_excluded;
    gate.require(cover->contains_id(id) == (power <= 0), "cover membership disagrees with closed rational ball");
  }
  std::size_t previous_end = 0;
  bool first = true;
  const auto order = cover->index()->spatial_order();
  for (const auto range : cover->ranges()) {
    gate.require(range.first < range.last && range.last <= order.size(), "invalid covered spatial range");
    if (!first) gate.require(previous_end < range.first, "covered ranges overlap or unmerged adjacent ranges remain");
    for (std::size_t rank = range.first; rank != range.last; ++rank) actual.push_back(order[rank]);
    previous_end = range.last;
    first = false;
  }
  std::sort(actual.begin(), actual.end());
  gate.require(actual == wanted, "range cover differs from exhaustive original-ID membership");
  gate.require(cover->site_count() == actual.size(), "cover population ledger");
  const auto& work = cover->work();
  gate.require(work.admitted_sites == actual.size() && work.admitted_sites + work.rejected_sites == points.size(),
               "cover admitted/rejected site partition ledger");
  gate.require(work.retained_ranges == cover->ranges().size(), "cover retained-range ledger");
  gate.require(work.node_visits > 0 && work.node_visits == work.point_tests + work.bound_tests &&
               work.node_visits == work.admitted_nodes + work.rejected_nodes + work.split_nodes &&
               work.retained_ranges + work.merged_ranges == work.admitted_nodes,
               "cover traversal ledger");
  gate.require(cover->retained_bytes() >= cover->ranges().size() * sizeof(mhgp9::gen::Range), "cover range capacity uncharged");
  gate.cover_admitted_nodes += work.admitted_nodes;
  gate.cover_rejected_nodes += work.rejected_nodes;
  gate.cover_split_nodes += work.split_nodes;
  ++gate.covers;
  return actual;
}

// v9 diametral core: the closed ball |2z-a-b|^2 <= |b-a|^2, judged against
// the same exhaustive rational membership, and a subset of the edge cover.
void check_diametral(Gate& gate, const Points& points, const mhgp9::gen::Q2CensusIndexPtr& index, Edge edge,
                     const std::vector<std::size_t>& cover_members) {
  const auto core = mhgp9::gen::Q34EdgeCover::make_diametral(index, edge);
  std::sort(edge.begin(), edge.end());
  gate.require(core->edge_ids() == edge && core->index().get() == index.get(), "diametral core lost its edge or index");
  std::vector<std::size_t> wanted, actual;
  const auto d = oracle::difference(points[edge[1]], points[edge[0]]);
  for (std::size_t id = 0; id != points.size(); ++id) {
    Big power = -oracle::dot(d, d);
    for (std::size_t axis = 0; axis != 3; ++axis) {
      const Big distance = 2 * Big(points[id][axis]) - points[edge[0]][axis] - points[edge[1]][axis];
      power += distance * distance;
    }
    if (power <= 0) wanted.push_back(id);
    gate.require(core->contains_id(id) == (power <= 0), "diametral membership disagrees with closed rational ball");
  }
  const auto order = index->spatial_order();
  for (const auto range : core->ranges())
    for (std::size_t rank = range.first; rank != range.last; ++rank) actual.push_back(order[rank]);
  std::sort(actual.begin(), actual.end());
  gate.require(actual == wanted && core->site_count() == actual.size(), "diametral ranges differ from exhaustive membership");
  gate.require(std::binary_search(actual.begin(), actual.end(), edge[0]) &&
               std::binary_search(actual.begin(), actual.end(), edge[1]) &&
               std::includes(cover_members.begin(), cover_members.end(), actual.begin(), actual.end()),
               "diametral core lost an endpoint or left the edge cover");
  // No consumer below the certificate accepts the core (census, atlas, seeds).
  if (gate.diametral_cores == 0) {
    const auto noop = [](const auto&) {};
    const auto refuses = [&](auto&& call, const char* message) {
      bool caught = false;
      try { call(); } catch (const std::invalid_argument&) { caught = true; }
      gate.require(caught, message);
      ++gate.diametral_refusals;
    };
    refuses([&] { static_cast<void>(mhgp9::gen::run_q34_edge_candidates(core, 5, noop)); },
                 "edge census accepted a diametral core");
    refuses([&] { static_cast<void>(mhgp9::gen::run_q34_cover_seed_candidates(core, edge[0] == 0 ? 1 : 0, 5, noop)); },
                 "seed census accepted a diametral core");
    refuses([&] { static_cast<void>(mhgp9::gen::Q4LocalAtlas::make(core, 5, mhgp9::gen::Q4LocalOptions{})); },
                 "local atlas accepted a diametral core");
    refuses([&] { static_cast<void>(mhgp9::gen::run_q4_window_edge_candidates(core, 5, noop)); },
                 "q4 window accepted a diametral core");
    refuses([&] { static_cast<void>(mhgp9::gen::Q4PositiveDomain::make(core)); },
                 "positive domain accepted a diametral core");
    refuses([&] { static_cast<void>(mhgp9::gen::Q34WitnessPool::make(core, 8)); },
                 "witness pool accepted a diametral core");
    refuses([&] { static_cast<void>(mhgp9::gen::run_q4_shallow_edge_candidates(core, 5, noop)); },
                 "q4 shallow accepted a diametral core");
    refuses([&] { static_cast<void>(mhgp9::gen::Q4LocalGeometry::make(core, mhgp9::gen::Q4CenterDomainMode::Disk)); },
                 "local geometry accepted a diametral core");
  }
  ++gate.diametral_cores;
  gate.diametral_members += actual.size();
  gate.diametral_cover_only += cover_members.size() - actual.size();
}

Output covered_seed(const mhgp9::gen::Q34EdgeCoverPtr& cover, std::size_t x, std::size_t kmax,
                    mhgp9::gen::Q34CoverSeedWork* work = nullptr) {
  Output result;
  const auto measured = mhgp9::gen::run_q34_cover_seed_candidates(cover, x, kmax, [&](const auto& item) {
    result.push_back(copy(item));
  });
  if (work != nullptr) *work = measured;
  normalize(result);
  return result;
}
Output covered_edge(const mhgp9::gen::Q34EdgeCoverPtr& cover, std::size_t kmax, mhgp9::gen::Q34EdgeWork* work = nullptr) {
  Output result;
  const auto measured = mhgp9::gen::run_q34_edge_candidates(cover, kmax, [&](const auto& item) {
    result.push_back(copy(item));
  });
  if (work != nullptr) *work = measured;
  normalize(result);
  return result;
}

Output check_edge(Gate& gate, const Points& points, Edge edge, std::size_t kmax,
                  const mhgp9::gen::Q2CensusIndexPtr& shared_index = {}) {
  gate.require(points.size() <= 40, "cover oracle exceeded bounded n<=40 domain");
  const auto cloud = mhgp9::gen::prepare_cloud(points);
  const auto index = shared_index ? shared_index : mhgp9::gen::make_q2_cloud_index(cloud);
  const auto cover = mhgp9::gen::Q34EdgeCover::make(index, edge);
  gate.require(cover->index().get() == index.get(), "cover rebuilt or copied the shared spatial index");
  const auto members = check_cover(gate, points, cover, edge);
  check_diametral(gate, points, index, edge, members);
  edge = cover->edge_ids();
  const auto before_ranges = std::vector<mhgp9::gen::Range>(cover->ranges().begin(), cover->ranges().end());
  const auto before_bytes = cover->retained_bytes();
  Output wanted;
  std::size_t seeds = 0;
  std::vector<mhgp9::gen::Q34CoverSeedWork> seed_work;
  for (std::size_t x = 0; x != points.size(); ++x) {
    if (x == edge[0] || x == edge[1]) continue;
    const Ids ids{edge[0], edge[1], x};
    if (!oracle::make(select(points, ids)).ball || owner(points, ids) != edge) continue;
    const auto expected = oracle_seed(gate, points, edge, x, kmax);
    mhgp9::gen::Q34CoverSeedWork measured;
    const auto partial = covered_seed(cover, x, kmax, &measured);
    gate.require(partial == expected, "covered seed lost a valid ball, depth, presentation or shell");
    gate.require(measured.site_reads == (kmax < 2 ? 0 : cover->site_count()) &&
                 measured.seed.q3_point_tests == measured.site_reads &&
                 measured.seed.family.sites == (kmax < 3 ? 0 : cover->site_count()),
                 "covered seed did not use one fused complete cover scan");
    seed_work.push_back(measured);
    Output global;
    static_cast<void>(mhgp9::gen::run_q34_seed_candidates(cloud, ids, kmax, [&](const auto& item) { global.push_back(copy(item)); }));
    normalize(global);
    gate.require(global == expected, "independent oracle disagrees with global reference on shared fixture");
    if (std::none_of(partial.begin(), partial.end(), [](const auto& item) { return item.arity == 3; }) &&
        std::any_of(partial.begin(), partial.end(), [](const auto& item) { return item.arity == 4; })) ++gate.q4_without_q3;
    wanted.insert(wanted.end(), expected.begin(), expected.end());
    ++gate.seed_calls;
    ++gate.reference_calls;
    ++seeds;
  }
  normalize(wanted);
  mhgp9::gen::Q34EdgeWork edge_work;
  const auto actual = covered_edge(cover, kmax, &edge_work);
  gate.require(actual == wanted, "edge generation differs from all canonical acute seeds");
  if (kmax < 2) {
    gate.require(edge_work.node_visits == 0 && edge_work.seeds == 0 && edge_work.covered.site_reads == 0,
                 "inactive q3/q4 lanes performed edge traversal");
  } else {
    gate.require(edge_work.seeds == seeds && edge_work.covered.site_reads == seeds * cover->site_count(),
                 "edge seed count or shared-cover scan ledger");
    gate.require(edge_work.point_tests + edge_work.rejected_sites == points.size() &&
                 edge_work.node_visits == edge_work.point_tests + edge_work.bound_tests &&
                 edge_work.bound_tests == edge_work.rejected_nodes + edge_work.split_nodes &&
                 edge_work.acute_seeds == edge_work.seeds + edge_work.owner_rejections &&
                 edge_work.owner_tests >= edge_work.acute_seeds && edge_work.owner_tests <= 2 * edge_work.acute_seeds,
                 "seed generator traversal/ownership ledger");
#define MHGP9G_COVER_SUM(field) do { \
      u64 total = 0; for (const auto& item : seed_work) total += item.field; \
      gate.require(edge_work.covered.field == total, "edge reduction sum " #field); \
    } while (false)
#define MHGP9G_COVER_MAX(field) do { \
      u64 maximum = 0; for (const auto& item : seed_work) maximum = std::max(maximum, item.field); \
      gate.require(edge_work.covered.field == maximum, "edge reduction maximum " #field); \
    } while (false)
    MHGP9G_COVER_SUM(site_reads);
    MHGP9G_COVER_SUM(q3_shell_sort_comparisons);
    MHGP9G_COVER_SUM(q4_shell_sort_comparisons);
    MHGP9G_COVER_MAX(peak_buffer_bytes);
    MHGP9G_COVER_SUM(seed.seed_owner_tests);
    MHGP9G_COVER_SUM(seed.seed_owner_rejections);
    MHGP9G_COVER_SUM(seed.q3_point_tests);
    MHGP9G_COVER_SUM(seed.q3_shell_ids);
    MHGP9G_COVER_SUM(seed.q3_depth_rejections);
    MHGP9G_COVER_SUM(seed.q3_emitted);
    MHGP9G_COVER_MAX(seed.q3_shell_capacity_bytes);
    MHGP9G_COVER_SUM(seed.q4_depth_rejected_groups);
    MHGP9G_COVER_SUM(seed.q4_depth_skipped_ids);
    MHGP9G_COVER_SUM(seed.q4_presentations);
    MHGP9G_COVER_SUM(seed.q4_owner_tests);
    MHGP9G_COVER_SUM(seed.q4_owner_rejections);
    MHGP9G_COVER_SUM(seed.q4_positive_tests);
    MHGP9G_COVER_SUM(seed.q4_positive_rejections);
    MHGP9G_COVER_SUM(seed.q4_seed_tests);
    MHGP9G_COVER_SUM(seed.q4_seed_rejections);
    MHGP9G_COVER_SUM(seed.q4_groups_without_support);
    MHGP9G_COVER_SUM(seed.q4_unexamined_after_emit);
    MHGP9G_COVER_SUM(seed.q4_emitted);
    MHGP9G_COVER_SUM(seed.family.sites);
    MHGP9G_COVER_SUM(seed.family.entries);
    MHGP9G_COVER_SUM(seed.family.exits);
    MHGP9G_COVER_SUM(seed.family.constant_inside);
    MHGP9G_COVER_SUM(seed.family.constant_on);
    MHGP9G_COVER_SUM(seed.family.constant_outside);
    MHGP9G_COVER_SUM(seed.family.sort_comparisons);
    MHGP9G_COVER_SUM(seed.family.group_comparisons);
    MHGP9G_COVER_SUM(seed.family.groups);
    MHGP9G_COVER_MAX(seed.family.max_group);
    MHGP9G_COVER_SUM(seed.family.callbacks);
    MHGP9G_COVER_SUM(seed.family.event_count);
    MHGP9G_COVER_MAX(seed.family.retained_capacity_bytes);
#undef MHGP9G_COVER_SUM
#undef MHGP9G_COVER_MAX
  }
  gate.edge_seed_count += edge_work.seeds;
  gate.edge_pruned_sites += edge_work.rejected_sites;
  gate.covered_site_reads += edge_work.covered.site_reads;
  gate.require(cover->retained_bytes() == before_bytes && cover->ranges().size() == before_ranges.size(),
               "seed calls changed immutable parent cover storage");
  for (std::size_t i = 0; i != before_ranges.size(); ++i)
    gate.require(cover->ranges()[i].first == before_ranges[i].first && cover->ranges()[i].last == before_ranges[i].last,
                 "seed call rewrote parent ranges");
  gate.require(Points(index->cloud().points().begin(), index->cloud().points().end()) == points,
               "covered processing changed original coordinates");
  for (const auto& item : actual) {
    if (item.arity == 3) ++gate.q3; else ++gate.q4;
    gate.max_shell = std::max(gate.max_shell, static_cast<u64>(item.shell.size()));
    gate.require(std::adjacent_find(item.shell.begin(), item.shell.end()) == item.shell.end(), "duplicate emitted shell ID");
  }
  gate.candidates += actual.size();
  if (seeds > 1) ++gate.reused_covers;
  ++gate.edge_calls;
  return actual;
}

void all_edges(Gate& gate, const Points& points, std::size_t kmax) {
  const auto cloud = mhgp9::gen::prepare_cloud(points);
  const auto index = mhgp9::gen::make_q2_cloud_index(cloud);
  using Key = std::pair<unsigned, Coefficients>;
  std::set<Key> got, wanted;
  for (std::size_t a = 0; a != points.size(); ++a)
    for (std::size_t b = a + 1; b != points.size(); ++b) {
      for (const auto& item : check_edge(gate, points, {a, b}, kmax, index)) got.emplace(item.arity, item.key);
      ++gate.exhaustive_edges;
      for (std::size_t x = b + 1; x != points.size(); ++x) {
        const Ids triangle{a, b, x};
        const auto face = oracle::make(select(points, triangle));
        if (face.ball && kmax >= 2) {
          const auto item = census(gate, points, *face.ball, triangle);
          if (item.depth < kmax - 1) wanted.emplace(3, item.key);
        }
        for (std::size_t y = x + 1; y != points.size(); ++y) {
          const std::array<std::size_t, 4> support{a, b, x, y};
          const auto ball = oracle::make(select(points, support));
          if (ball.ball && kmax >= 3) {
            const auto item = census(gate, points, *ball.ball, support);
            if (item.depth < kmax - 2) wanted.emplace(4, item.key);
          }
        }
      }
    }
  gate.require(got == wanted, "all supplied edges lost a brute-force positive ball");
  ++gate.clouds;
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

void special_cases(Gate& gate) {
  const Points boundary{{10, 20, 20}, {30, 20, 20}, {20, 40, 20}, {20, 0, 20},
                         {20, 41, 20}, {20, 35, 20}, {20, 25, 5}, {65535, 65535, 65535}};
  for (const auto k : {1U, 2U, 3U, 5U, 10U}) static_cast<void>(check_edge(gate, boundary, {1, 0}, k));
  // 18-bit twin: the far site sits at coordinate_limit (262143); the 65535
  // corner above is an interior site since the widening and keeps its role.
  const Points boundary18{{10, 20, 20}, {30, 20, 20}, {20, 40, 20}, {20, 0, 20},
                           {20, 41, 20}, {20, 35, 20}, {20, 25, 5}, {262143, 262143, 262143}};
  for (const auto k : {1U, 2U, 3U, 5U, 10U}) {
    static_cast<void>(check_edge(gate, boundary18, {1, 0}, k));
    ++gate.wide_edge_calls;
  }
  const Points partial{{10, 20, 20}, {30, 20, 20}, {20, 35, 20},
                        {20, 20, 21}, {20, 25, 0}, {20, 25, 5}};
  const std::array<std::size_t, 4> invalid{0, 1, 2, 3};
  const auto sphere = oracle::make(select(partial, invalid), false);
  gate.require(sphere.ball && !oracle::make(select(partial, invalid)).ball,
               "outside-cover fixture did not describe an invalid positive presentation");
  std::size_t local_depth = 0, global_depth = 0;
  for (const auto& point : partial) if (sphere.ball->power(point).numerator() < 0) {
    ++global_depth;
    if (cover_power(partial, {0, 1}, point) <= 0) ++local_depth;
  }
  gate.require(global_depth > local_depth && cover_power(partial, {0, 1}, partial[4]) > 0,
               "arbitrary root did not lose an exterior-cover interior witness");
  const auto kept = check_edge(gate, partial, {0, 1}, 5);
  gate.require(std::any_of(kept.begin(), kept.end(), [](const auto& item) {
    return item.arity == 4 && item.support == std::vector<std::size_t>{0, 1, 2, 5} && item.depth == 1;
  }), "partial invalid-root census changed the valid owner output");
  ++gate.invalid_root_depth_changed;
  for (const auto k : {3U, 5U, 10U}) static_cast<void>(check_edge(gate, shell_fixture(), {0, 1}, k));
  for (const auto k : {5U, 10U}) {
    Points points{{900, 1000, 1000}, {1100, 1000, 1000},
                  {1000, 1120, 1040}, {1000, 1120, 960}};
    for (unsigned j = 0; j != k - 1; ++j)
      points.push_back({static_cast<mhgp9::gen::Coordinate>(1000 + j), 1020, 1105});
    static_cast<void>(check_edge(gate, points, {0, 1}, k));
    auto reversed = points;
    std::reverse(reversed.begin(), reversed.end());
    static_cast<void>(check_edge(gate, reversed, {points.size() - 1, points.size() - 2}, k));
    ++gate.permutations;
  }
  Points rows;
  for (unsigned i = 0; i != 6; ++i) {
    rows.push_back({0, static_cast<mhgp9::gen::Coordinate>(4 * i), 0});
    rows.push_back({100, static_cast<mhgp9::gen::Coordinate>(4 * i), 0});
  }
  static_cast<void>(check_edge(gate, rows, {0, 11}, 10));
  auto rotated = rows;
  for (auto& point : rotated) point = {point.z, point.x, point.y};
  static_cast<void>(check_edge(gate, rotated, {11, 0}, 10));
  ++gate.permutations;
}

void lifecycle(Gate& gate) {
  const Points points{{0, 0, 0}, {2, 2, 0}, {2, 2, 2}, {2, 0, 2}, {0, 2, 2}, {65535, 65535, 65535}};
  auto cloud = mhgp9::gen::prepare_cloud(points);
  auto index = mhgp9::gen::make_q2_cloud_index(cloud);
  auto cover = mhgp9::gen::Q34EdgeCover::make(index, {0, 1});
  const auto wanted = covered_edge(cover, 10);
  gate.require(!wanted.empty(), "lifecycle fixture emitted nothing");
  const mhgp9::gen::Q34SeedConsumer consumer = [](const auto&) {};
  gate.rejects([&] { static_cast<void>(mhgp9::gen::Q34EdgeCover::make({}, {0, 1})); }, "null index accepted");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::Q34EdgeCover::make(index, {0, 0})); }, "repeated endpoint accepted");
  gate.rejects<std::out_of_range>([&] { static_cast<void>(mhgp9::gen::Q34EdgeCover::make(index, {0, points.size()})); },
                                 "invalid endpoint accepted");
  gate.rejects<std::out_of_range>([&] { static_cast<void>(cover->contains_id(points.size())); }, "invalid cover member ID accepted");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q34_edge_candidates({}, 10, consumer)); }, "null cover accepted");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q34_edge_candidates(cover, 0, consumer)); }, "zero K accepted");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q34_edge_candidates(cover, 10, {})); }, "empty callback accepted");
  gate.rejects<std::out_of_range>([&] {
    static_cast<void>(mhgp9::gen::run_q34_cover_seed_candidates(cover, points.size(), 10, consumer));
  }, "invalid seed ID accepted");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_q34_cover_seed_candidates(cover, 0, 10, consumer)); }, "endpoint as seed accepted");
  struct CallbackFailure final : std::exception {};
  bool caught = false;
  std::size_t calls = 0;
  try {
    static_cast<void>(mhgp9::gen::run_q34_edge_candidates(cover, 10, [&](const auto&) { ++calls; throw CallbackFailure{}; }));
  } catch (const CallbackFailure&) { caught = true; }
  gate.require(caught && calls == 1, "callback failure swallowed or processing continued");
  gate.require(covered_edge(cover, 10) == wanted, "failed edge call changed parent cover");
  ++gate.callback_failures;
  std::vector<std::future<Output>> futures;
  for (unsigned worker = 0; worker != 4; ++worker)
    futures.push_back(std::async(std::launch::async, [cover] { return covered_edge(cover, 10); }));
  for (auto& future : futures) {
    gate.require(future.get() == wanted, "parallel calls sharing immutable cover disagree");
    ++gate.parallel_calls;
  }
  calls = 0;
  static_cast<void>(mhgp9::gen::run_q34_edge_candidates(cover, 10, [&](const auto&) {
    ++calls;
    cover.reset(); index.reset(); cloud.reset();
  }));
  gate.require(!cover && !index && !cloud && calls == wanted.size(), "edge call lost its owned parent/index/cloud");
  auto wrong = wanted;
  wrong.pop_back();
  gate.require(wrong != wanted, "judge missed missing candidate");
  ++gate.judge_mutants;
  wrong = wanted;
  ++wrong.front().depth;
  gate.require(wrong != wanted, "judge missed partial depth exported as global");
  ++gate.judge_mutants;
  wrong = wanted;
  wrong.front().shell.pop_back();
  gate.require(wrong != wanted, "judge missed shell site");
  ++gate.judge_mutants;
}

void run(Gate& gate) {
  special_cases(gate);
  const Points regular{{0, 0, 0}, {2, 2, 0}, {2, 0, 2}, {0, 2, 2}};
  const Points extremes{{0, 0, 0}, {65535, 65535, 0}, {65535, 0, 65535}, {0, 65535, 65535},
                        {32767, 32768, 32767}, {65535, 65535, 65535}};
  for (const auto k : {1U, 2U, 3U, 5U, 10U}) all_edges(gate, regular, k);
  all_edges(gate, extremes, 5);
  // 18-bit twin of the corner cloud: 262143 = coordinate_limit, 131071/131072
  // the two middle values (the 65535/32767/32768 sites above are interior now).
  const Points extremes18{{0, 0, 0}, {262143, 262143, 0}, {262143, 0, 262143}, {0, 262143, 262143},
                          {131071, 131072, 131071}, {262143, 262143, 262143}};
  all_edges(gate, extremes18, 5);
  ++gate.wide_clouds;
  std::uint64_t state = 0x4c8a2761f01529b3ULL;
  auto next = [&]() {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    return state >> 32;
  };
  for (unsigned trial = 0; trial != 6; ++trial) {
    Points points;
    const unsigned side_length = trial % 2 == 0 ? 65536U : 13U;
    while (points.size() != 7)
      add_unique(points, {static_cast<mhgp9::gen::Coordinate>(next() % side_length),
                          static_cast<mhgp9::gen::Coordinate>(next() % side_length),
                          static_cast<mhgp9::gen::Coordinate>(next() % side_length)});
    all_edges(gate, points, trial % 2 == 0 ? 5 : 10);
  }
  // Separate 18-bit random clouds (own generator: the six clouds above stay
  // pinned). Each must leave the 16-bit range, otherwise it proves nothing new.
  std::uint64_t wide_state = 0x7d3e19a6c52b40f1ULL;
  auto wide_next = [&]() {
    wide_state = wide_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return wide_state >> 32;
  };
  for (unsigned trial = 0; trial != 2; ++trial) {
    Points points;
    while (points.size() != 7)
      add_unique(points, {static_cast<mhgp9::gen::Coordinate>(wide_next() % 262144U),
                          static_cast<mhgp9::gen::Coordinate>(wide_next() % 262144U),
                          static_cast<mhgp9::gen::Coordinate>(wide_next() % 262144U)});
    gate.require(std::any_of(points.begin(), points.end(), [](const Point3& point) {
      return point.x > 65535 || point.y > 65535 || point.z > 65535;
    }), "18-bit random cloud stayed inside the 16-bit range");
    all_edges(gate, points, trial == 0 ? 5 : 10);
    ++gate.wide_clouds;
  }
  lifecycle(gate);
  gate.require(gate.covers >= 150 && gate.edge_calls >= 150 && gate.seed_calls >= 100 &&
               gate.reference_calls == gate.seed_calls && gate.oracle_sites >= 1000 && gate.q3 >= 30 && gate.q4 >= 10,
               "cover correctness nonvacuity floor");
  gate.require(gate.diametral_cores > 0 && gate.diametral_refusals == 8 && gate.diametral_cover_only > 0 && gate.diametral_members > 2 * gate.diametral_cores,
               "diametral core never exercised (cores, cover-only sites, interior members)");
  gate.require(gate.cover_boundary >= 2 && gate.cover_excluded > 0 && gate.cover_admitted_nodes > 0 &&
               gate.cover_rejected_nodes > 0 && gate.cover_split_nodes > 0 && gate.reused_covers > 0 &&
               gate.max_shell >= 30 && gate.q4_without_q3 >= 2 && gate.invalid_root_depth_changed == 1 &&
               gate.edge_seed_count > 0 && gate.edge_pruned_sites > 0 && gate.covered_site_reads > 0,
               "cover adversarial nonvacuity floor");
  gate.require(gate.clouds == 12 + gate.wide_clouds && gate.exhaustive_edges >= 150 && gate.invalid_inputs == 9 &&
               gate.callback_failures == 1 && gate.parallel_calls == 4 && gate.judge_mutants == 3,
               "cover exhaustive/lifecycle nonvacuity floor");
  gate.require(gate.wide_clouds == 3 && gate.wide_edge_calls == 5, "18-bit twin fixture nonvacuity floor");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp9_gen_q34_cover_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    run(gate);
    std::cout << "{\"schema\":\"mhgp9_gen_q34_cover_gate_v1\",\"status\":\"passed\""
              << ",\"checks\":" << gate.checks << ",\"clouds\":" << gate.clouds
              << ",\"covers\":" << gate.covers << ",\"edge_calls\":" << gate.edge_calls
              << ",\"seed_calls\":" << gate.seed_calls << ",\"reference_calls\":" << gate.reference_calls
              << ",\"oracle_completions\":" << gate.oracle_completions << ",\"oracle_sites\":" << gate.oracle_sites
              << ",\"candidates\":" << gate.candidates << ",\"q3\":" << gate.q3 << ",\"q4\":" << gate.q4
              << ",\"max_shell\":" << gate.max_shell << ",\"cover_boundary\":" << gate.cover_boundary
              << ",\"diametral_cores\":" << gate.diametral_cores << ",\"diametral_members\":" << gate.diametral_members
              << ",\"diametral_cover_only\":" << gate.diametral_cover_only << ",\"diametral_refusals\":" << gate.diametral_refusals
              << ",\"cover_excluded\":" << gate.cover_excluded << ",\"cover_admitted_nodes\":" << gate.cover_admitted_nodes
              << ",\"cover_rejected_nodes\":" << gate.cover_rejected_nodes << ",\"cover_split_nodes\":" << gate.cover_split_nodes
              << ",\"reused_covers\":" << gate.reused_covers << ",\"q4_without_q3\":" << gate.q4_without_q3
              << ",\"invalid_root_depth_changed\":" << gate.invalid_root_depth_changed
              << ",\"exhaustive_edges\":" << gate.exhaustive_edges << ",\"permutations\":" << gate.permutations
              << ",\"invalid_inputs\":" << gate.invalid_inputs << ",\"callback_failures\":" << gate.callback_failures
              << ",\"parallel_calls\":" << gate.parallel_calls << ",\"judge_mutants\":" << gate.judge_mutants
              << ",\"edge_seed_count\":" << gate.edge_seed_count << ",\"edge_pruned_sites\":" << gate.edge_pruned_sites
              << ",\"covered_site_reads\":" << gate.covered_site_reads
              << ",\"wide_clouds\":" << gate.wide_clouds << ",\"wide_edge_calls\":" << gate.wide_edge_calls << "}\n";
  } catch (const std::exception& error) {
    std::cerr << "q34 cover gate: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
