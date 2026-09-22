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
#include "lanes/family_certificate.hpp"
#include "lanes/q34_cover.hpp"
#include "lanes/q34_pruning.hpp"

// Test-only rational Gram elimination supplies the centre and radius. In
// particular J is derived from the radius gap, not the product's E*X formula.
// Exhaustive completions and censuses below are restricted to small fixtures.
namespace {
namespace oracle = mhgp8_test::ball_oracle;
using oracle::Big;
using oracle::Rational;
using oracle::Coefficients;
using mhgp8::Point3;
using mhgp8::u64;
using Points = std::vector<Point3>;
using Edge = std::array<std::size_t, 2>;
using Ids = std::array<std::size_t, 3>;

static_assert(!std::is_default_constructible_v<mhgp8::Q34FamilyCertificate>);
static_assert(!std::is_copy_assignable_v<mhgp8::Q34FamilyCertificate>);
static_assert(!std::is_copy_constructible_v<mhgp8::Q34WitnessPool>);
static_assert(!std::is_move_constructible_v<mhgp8::Q34WitnessPool>);

struct Gate {
  u64 checks{}, primitive_calls{}, refused{}, certified_q3{}, certified_q4{};
  u64 positive_completions{}, universal_inside_checks{}, strict_boundary{};
  u64 nonintegral_radius_bounds{}, max_j_bits{}, max_product_bits{};
  u64 edge_calls{}, seed_calls{}, reference_calls{}, oracle_completions{}, oracle_sites{};
  u64 candidates{}, q3{}, q4{}, max_shell{}, q3_only_rejections{}, q4_only_rejections{};
  u64 both_rejections{}, neither_rejections{}, saved_site_reads{}, pool_calls{};
  u64 exhaustive_edges{}, permutations{}, invalid_inputs{}, callback_failures{}, parallel_calls{};
  // 18-bit twins (a seed coordinate above 65535): separate bit maxima whose
  // floors are provably unreachable by 16-bit seeds (see selftest()).
  u64 wide_primitive_calls{}, wide_max_j_bits{}, wide_max_product_bits{}, max_sqrt_iterations{};
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

Points select(const Points& points, std::span<const std::size_t> ids) {
  Points result;
  for (const auto id : ids) result.push_back(points[id]);
  return result;
}
Big distance_squared(Point3 a, Point3 b) {
  const auto delta = oracle::difference(a, b);
  return oracle::dot(delta, delta);
}
Edge owner(const Points& points, std::span<const std::size_t> ids) {
  Big largest = -1;
  Edge result{};
  for (std::size_t i = 0; i != ids.size(); ++i)
    for (std::size_t j = i + 1; j != ids.size(); ++j) {
      const auto length = distance_squared(points[ids[i]], points[ids[j]]);
      const Edge edge{std::min(ids[i], ids[j]), std::max(ids[i], ids[j])};
      if (length > largest || (length == largest && edge < result)) {
        largest = length;
        result = edge;
      }
    }
  return result;
}
bool wide_point(Point3 point) { return point.x > 65535 || point.y > 65535 || point.z > 65535; }
Big ceil_sqrt(const Big& value) {
  Big lower = 0, upper = 1;
  while (upper * upper < value) upper *= 2;
  while (lower < upper) {
    const Big middle = (lower + upper) / 2;
    if (middle * middle < value) lower = middle + 1;
    else upper = middle;
  }
  return lower;
}

void check_certificate(Gate& gate, const Points& points, Ids ids) {
  const auto a = points[ids[0]], b = points[ids[1]], x = points[ids[2]];
  const bool wide = wide_point(a) || wide_point(b) || wide_point(x);
  const auto face = oracle::make(select(points, ids));
  const auto diameter = distance_squared(a, b);
  const bool wanted = face.ball.has_value() && distance_squared(a, x) <= diameter &&
                      distance_squared(b, x) <= diameter;
  const auto certificate = mhgp8::Q34FamilyCertificate::make(a, b, x);
  ++gate.primitive_calls;
  gate.require(certificate.has_value() == wanted, "certificate domain differs from rational positive face and maximum edge");
  if (!certificate) { ++gate.refused; return; }
  if (wide) ++gate.wide_primitive_calls;
  const auto d = oracle::difference(b, a), u = oracle::difference(x, a);
  const oracle::Vector normal{d[1] * u[2] - d[2] * u[1],
                              d[2] * u[0] - d[0] * u[2],
                              d[0] * u[1] - d[1] * u[0]};
  const Big gram = oracle::dot(normal, normal);
  const Rational gap = Rational(3 * diameter, 8) - face.ball->radius_squared;
  const Rational j_rational = Rational(8 * gram) * gap;
  gate.require(j_rational.denominator() == 1 && j_rational.numerator() > 0,
               "positive owner face has invalid rational Jung chord");
  const Big j = j_rational.numerator();
  const Big limit = ceil_sqrt((j + 1) / 2);
  gate.require(Big(certificate->j_bound()) == j, "J differs from independent rational radius gap");
  gate.require(Big(certificate->radius_parameter_bound()) == limit,
               "integer Jung bound differs from multiprecision ceil sqrt");
  gate.require(2 * limit * limit >= j && 2 * (limit - 1) * (limit - 1) < j,
               "ceil sqrt is not minimal");
  // The bisection starts at high=2^ceil(bits(J/2)/2)<=2^58 at 18 bits (52 at
  // 16 bits: the 18-bit twins exceed the historical bound, widened 22 September 2026).
  gate.require(certificate->sqrt_iterations() > 0 && certificate->sqrt_iterations() <= 58,
               "integer sqrt iteration ledger");
  gate.max_sqrt_iterations = std::max(gate.max_sqrt_iterations, certificate->sqrt_iterations());
  gate.max_j_bits = std::max(gate.max_j_bits, oracle::bits(j));
  if (wide) gate.wide_max_j_bits = std::max(gate.wide_max_j_bits, oracle::bits(j));
  if (2 * limit * limit != j) ++gate.nonintegral_radius_bounds;
  std::vector<bool> universal;
  for (const auto z : points) {
    const auto v = oracle::difference(z, a);
    const Big side = oracle::dot(normal, v);
    const Rational p_rational = Rational(gram) * face.ball->power(z);
    gate.require(p_rational.denominator() == 1, "scaled rational face power is nonintegral");
    const Big p = p_rational.numerator();
    const Big margin = p + limit * oracle::absolute(side);
    const auto flags = certificate->witness(z);
    gate.require(flags.q3 == (p < 0) && flags.q4 == (margin < 0),
                 "witness flags differ from rational strict certificate");
    gate.require(Big(certificate->family().power(z)) == p &&
                 Big(certificate->family().side(z)) == side,
                 "certificate family changed exact rational power or orientation");
    if (flags.q3) { ++gate.certified_q3; gate.require(face.ball->power(z).numerator() < 0, "q3 certified shell or outside site"); }
    if (flags.q4) ++gate.certified_q4;
    if (margin == 0 && (p != 0 || side != 0)) {
      ++gate.strict_boundary;
      gate.require(!flags.q4, "closed chord tangence was credited as strict interior");
    }
    if (p == 0) gate.require(!flags.q3 && !flags.q4, "face shell credited as interior");
    gate.max_product_bits = std::max(gate.max_product_bits, oracle::bits(limit * oracle::absolute(side)));
    if (wide) gate.wide_max_product_bits = std::max(gate.wide_max_product_bits, oracle::bits(limit * oracle::absolute(side)));
    universal.push_back(flags.q4);
  }
  for (std::size_t y = 0; y != points.size(); ++y) {
    if (std::find(ids.begin(), ids.end(), y) != ids.end()) continue;
    const std::array<std::size_t, 4> support{ids[0], ids[1], ids[2], y};
    const auto ball = oracle::make(select(points, support));
    if (!ball.ball) continue;
    bool maximum = true;
    for (std::size_t i = 0; i != 4; ++i)
      for (std::size_t k = i + 1; k != 4; ++k)
        maximum = maximum && distance_squared(points[support[i]], points[support[k]]) <= diameter;
    if (!maximum) continue;
    ++gate.positive_completions;
    Rational mu(0);
    for (std::size_t axis = 0; axis != 3; ++axis)
      mu += Rational(2 * normal[axis]) * (ball.ball->center[axis] - face.ball->center[axis]);
    gate.require(Rational(2) * mu * mu <= Rational(j), "positive owner completion escaped rational Jung chord");
    for (std::size_t z = 0; z != points.size(); ++z) {
      if (!universal[z]) continue;
      gate.require(ball.ball->power(points[z]).numerator() < 0,
                   "universal q4 witness is not strictly inside a positive owner completion");
      ++gate.universal_inside_checks;
    }
  }
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
Candidate copy(const mhgp8::Q34SeedCandidate& value) {
  if ((value.arity != 3 && value.arity != 4) ||
      !std::is_sorted(value.support_ids.begin(), value.support_ids.begin() + value.arity) ||
      !std::is_sorted(value.shell_first.begin(), value.shell_first.end()) ||
      !std::is_sorted(value.shell_second.begin(), value.shell_second.end()))
    throw std::runtime_error("pruned candidate has invalid arity or unsorted support/shell part");
  if (value.arity == 3 && (!value.shell_second.empty() ||
      value.support_ids[3] != std::numeric_limits<std::size_t>::max()))
    throw std::runtime_error("pruned q3 candidate has invalid unused payload");
  Candidate result;
  result.arity = value.arity;
  for (std::size_t i = 0; i != 5; ++i) result.key[i] = Big(value.ball.coefficients()[i]);
  result.support.assign(value.support_ids.begin(), value.support_ids.begin() + value.arity);
  result.depth = value.depth;
  result.shell.assign(value.shell_first.begin(), value.shell_first.end());
  result.shell.insert(result.shell.end(), value.shell_second.begin(), value.shell_second.end());
  std::sort(result.shell.begin(), result.shell.end());
  if (std::adjacent_find(result.shell.begin(), result.shell.end()) != result.shell.end())
    throw std::runtime_error("pruned candidate duplicated a shell ID");
  return result;
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
    if (candidate.depth < kmax - 2) roots.try_emplace(candidate.key, std::move(candidate));
  }
  for (auto& [key, candidate] : roots) {
    static_cast<void>(key);
    result.push_back(std::move(candidate));
  }
  normalize(result);
  return result;
}

void primitive_fixtures(Gate& gate) {
  // The two 65535/32767 corner fixtures are followed by their 18-bit twins
  // (262143/131071 corners, and the spread fixture scaled by four).
  const std::array<Points, 9> fixtures{{
    {{0,0,0},{2,2,0},{2,0,2},{0,2,2},{1,1,1},{2,0,0}},
    {{0,0,0},{4,4,0},{4,0,3},{0,4,3},{2,2,2},{3,1,1}},
    {{0,0,0},{65535,65535,0},{65535,0,65535},{0,65535,65535},
     {32767,32767,32767},{65535,0,0}},
    {{0,0,0},{60000,65000,1000},{62000,500,64000},{2000,63000,62000},
     {32000,32000,32000},{65535,65535,65535}},
    {{0,0,0},{262143,262143,0},{262143,0,262143},{0,262143,262143},
     {131071,131071,131071},{262143,0,0}},
    {{0,0,0},{240000,260000,4000},{248000,2000,256000},{8000,252000,248000},
     {128000,128000,128000},{262143,262143,262143}},
    {{900,1000,1000},{1100,1000,1000},{1000,1120,1040},{1000,1120,960},
     {1000,1020,1105},{1001,1020,1105}},
    {{0,0,0},{2,0,0},{1,3,0},{0,1,0},{3,0,0}},
    {{20,20,20},{60,60,20},{60,20,60},{20,60,60},
     {40,40,40},{41,40,40},{40,41,40},{40,40,41}}
  }};
  for (const auto& points : fixtures) {
    for (std::size_t a = 0; a != 4; ++a)
      for (std::size_t b = 0; b != 4; ++b)
        for (std::size_t x = 0; x != 4; ++x) {
          if (a == b || a == x || b == x) continue;
          check_certificate(gate, points, {a,b,x});
          ++gate.permutations;
        }
    check_certificate(gate, points, {0,0,1});
  }
  check_certificate(gate, Points{{0,0,0},{2,0,0},{1,0,0}}, {0,1,2});
  check_certificate(gate, Points{{0,0,0},{2,0,0},{0,2,0}}, {0,1,2});
  check_certificate(gate, Points{{0,0,0},{2,0,0},{3,1,0}}, {0,1,2});
}

std::vector<u64> covered_signature(const mhgp8::Q34CoverSeedWork& w) {
  const auto& s = w.seed;
  const auto& f = s.family;
  return {w.site_reads, w.q3_shell_sort_comparisons, w.q4_shell_sort_comparisons,
    w.peak_buffer_bytes, s.seed_owner_tests, s.seed_owner_rejections,
    s.q3_point_tests, s.q3_shell_ids, s.q3_depth_rejections, s.q3_emitted,
    s.q3_shell_capacity_bytes, s.q4_depth_rejected_groups, s.q4_depth_skipped_ids,
    s.q4_presentations, s.q4_owner_tests, s.q4_owner_rejections, s.q4_positive_tests,
    s.q4_positive_rejections, s.q4_seed_tests, s.q4_seed_rejections,
    s.q4_groups_without_support, s.q4_unexamined_after_emit, s.q4_emitted,
    f.sites, f.entries, f.exits, f.constant_inside, f.constant_on, f.constant_outside,
    f.sort_comparisons, f.group_comparisons, f.groups, f.max_group, f.callbacks,
    f.event_count, f.retained_capacity_bytes};
}
std::vector<u64> generator_signature(const mhgp8::Q34EdgeWork& w) {
  return {w.node_visits, w.bound_tests, w.point_tests, w.rejected_nodes,
    w.split_nodes, w.rejected_sites, w.acute_seeds, w.owner_tests,
    w.owner_rejections, w.seeds};
}
std::vector<u64> pruning_signature(const mhgp8::Q34FamilyPruningWork& w) {
  return {w.seed_queries,w.certificate_builds,w.sqrt_iterations,w.proposed_sites,
    w.paired_predicate_tests,w.q3_credits,w.q4_credits,w.q3_rejected,w.q4_rejected,
    w.both_rejected,w.q3_only_survivors,w.q4_only_survivors,w.both_survivors};
}

struct RefCertificate {
  oracle::Ball face;
  oracle::Vector normal;
  Big gram, limit;
  Point3 anchor;
  std::pair<bool,bool> flags(Point3 point) const {
    const auto power = Rational(gram) * face.power(point);
    const Big side = oracle::dot(normal, oracle::difference(point, anchor));
    return {power.numerator() < 0,
            power + Rational(limit * oracle::absolute(side)) < Rational(0)};
  }
};
RefCertificate reference_certificate(const Points& points, Ids ids) {
  const auto face = oracle::make(select(points, ids));
  if (!face.ball) throw std::runtime_error("reference certificate requires an acute face");
  const auto d = oracle::difference(points[ids[1]], points[ids[0]]);
  const auto u = oracle::difference(points[ids[2]], points[ids[0]]);
  const oracle::Vector normal{d[1]*u[2]-d[2]*u[1],d[2]*u[0]-d[0]*u[2],d[0]*u[1]-d[1]*u[0]};
  const Big gram = oracle::dot(normal, normal);
  const Rational j = Rational(8 * gram) *
      (Rational(3 * distance_squared(points[ids[0]],points[ids[1]]),8) - face.ball->radius_squared);
  if (j.denominator() != 1 || j.numerator() <= 0) throw std::runtime_error("reference certificate domain");
  return {*face.ball, normal, gram, ceil_sqrt((j.numerator()+1)/2), points[ids[0]]};
}

void check_pool(Gate& gate, const mhgp8::Q34WitnessPoolPtr& pool, std::size_t budget) {
  const auto& cover = pool->cover();
  std::vector<std::size_t> all, expected;
  const auto order = cover->index()->spatial_order();
  for (const auto range : cover->ranges())
    for (std::size_t rank = range.first; rank != range.last; ++rank) all.push_back(order[rank]);
  const auto count = std::min(budget, all.size());
  for (std::size_t i = 0; i != count; ++i) {
    const Big rank = Big(i) * all.size() / count;
    expected.push_back(all[rank.convert_to<std::size_t>()]);
  }
  gate.require(std::vector<std::size_t>(pool->ids().begin(), pool->ids().end()) == expected,
               "pool proposals differ from evenly spaced covered spatial ranks");
  auto unique = expected;
  std::sort(unique.begin(), unique.end());
  gate.require(std::adjacent_find(unique.begin(), unique.end()) == unique.end(), "pool duplicates witness IDs");
  gate.require(pool->work().selected_sites == count && pool->retained_bytes() >= count * sizeof(std::size_t),
               "pool selection/capacity ledger");
  gate.require(pool->work().range_visits <= cover->ranges().size() &&
               ((count == 0) == (pool->work().range_visits == 0)), "pool range-visit ledger");
  ++gate.pool_calls;
}

Output check_edge(Gate& gate, const Points& points, Edge edge, std::size_t kmax, std::size_t budget) {
  gate.require(points.size() <= 40, "exhaustive pruning oracle exceeded bounded n<=40");
  const auto cloud = mhgp8::prepare_cloud(points);
  const auto cover = mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(cloud), edge);
  const auto pool = mhgp8::Q34WitnessPool::make(cover, budget);
  gate.require(pool->cover().get() == cover.get(), "pool lost shared immutable cover identity");
  check_pool(gate, pool, budget);
  edge = cover->edge_ids();
  Output expected_all;
  std::vector<u64> total_pruning(13);
  u64 reads = 0;
  for (std::size_t x = 0; x != points.size(); ++x) {
    if (x == edge[0] || x == edge[1]) continue;
    const Ids ids{edge[0],edge[1],x};
    if (!oracle::make(select(points,ids)).ball || owner(points,ids) != edge) continue;
    const auto expected = oracle_seed(gate,points,edge,x,kmax);
    Output actual, baseline;
    const auto work = mhgp8::run_q34_pruned_seed_candidates(pool,x,kmax,[&](const auto& item) { actual.push_back(copy(item)); });
    const auto old = mhgp8::run_q34_cover_seed_candidates(cover,x,kmax,[&](const auto& item) { baseline.push_back(copy(item)); });
    normalize(actual); normalize(baseline);
    gate.require(actual == expected, "pruned seed differs from rational complete census, depth or shell");
    gate.require(baseline == expected, "reference covered seed differs from rational oracle");
    if (budget == 0) gate.require(covered_signature(work.covered) == covered_signature(old),
                                  "zero-budget pruning changed reference seed work");
    gate.require(work.covered.site_reads <= old.site_reads, "pruning added a second cover scan");
    const auto& p = work.pruning;
    const bool active3 = kmax >= 2, active4 = kmax >= 3;
    std::size_t c3 = 0, c4 = 0, proposals = 0;
    if (active3 && !pool->ids().empty()) {
      const auto certificate = reference_certificate(points,ids);
      for (const auto id : pool->ids()) {
        if (c3 == kmax-1 && (!active4 || c4 == kmax-2)) break;
        const auto [inside3,inside4] = certificate.flags(points[id]);
        if (inside3 && c3 < kmax-1) ++c3;
        if (active4 && inside4 && c4 < kmax-2) ++c4;
        ++proposals;
      }
    }
    const bool reject3 = active3 && c3 == kmax-1;
    const bool reject4 = active4 && c4 == kmax-2;
    gate.require(p.proposed_sites == proposals && p.paired_predicate_tests == proposals &&
                 p.q3_credits == c3 && p.q4_credits == c4,
                 "pruning proposal or independently saturated distinct-credit ledger");
    gate.require(p.q3_rejected == static_cast<u64>(reject3) &&
                 p.q4_rejected == static_cast<u64>(reject4) &&
                 p.both_rejected == static_cast<u64>(reject3 && (!active4 || reject4)),
                 "q3/q4 rejection thresholds were coupled");
    const bool queried = active3 && !pool->ids().empty();
    gate.require(p.seed_queries == static_cast<u64>(queried) &&
                 p.certificate_builds == static_cast<u64>(queried) &&
                 (queried ? p.sqrt_iterations > 0 && p.sqrt_iterations <= 58 : p.sqrt_iterations == 0),
                 "pruning seed/certificate/sqrt ledger");
    gate.require(p.q3_only_survivors == static_cast<u64>(queried && !reject3 && (!active4 || reject4)) &&
                 p.q4_only_survivors == static_cast<u64>(queried && active4 && reject3 && !reject4) &&
                 p.both_survivors == static_cast<u64>(queried && active4 && !reject3 && !reject4),
                 "active surviving-lane partition ledger");
    if (active4) {
      if (reject3 && reject4) ++gate.both_rejections;
      else if (reject3) ++gate.q3_only_rejections;
      else if (reject4) ++gate.q4_only_rejections;
      else ++gate.neither_rejections;
    }
    if (reject3 && (!active4 || reject4))
      gate.require(work.covered.site_reads == 0 && work.covered.seed.family.sites == 0,
                   "all rejected active lanes still opened a family scan");
    if (reject4) gate.require(work.covered.seed.family.sites == 0 && work.covered.seed.family.event_count == 0,
                              "rejected q4 family still materialized events");
    if (reject3) gate.require(work.covered.seed.q3_point_tests == 0 && work.covered.seed.q3_emitted == 0,
                              "rejected q3 lane still ran its census");
    gate.saved_site_reads += old.site_reads-work.covered.site_reads;
    reads += work.covered.site_reads;
    const auto signature = pruning_signature(p);
    for (std::size_t i = 0; i != signature.size(); ++i) total_pruning[i] += signature[i];
    expected_all.insert(expected_all.end(),expected.begin(),expected.end());
    ++gate.seed_calls; ++gate.reference_calls;
  }
  normalize(expected_all);
  Output actual, baseline;
  const auto work = mhgp8::run_q34_pruned_edge_candidates(pool,kmax,[&](const auto& item) { actual.push_back(copy(item)); });
  const auto old = mhgp8::run_q34_edge_candidates(cover,kmax,[&](const auto& item) { baseline.push_back(copy(item)); });
  normalize(actual); normalize(baseline);
  gate.require(actual == expected_all && baseline == expected_all,
               "pruned edge differs from all canonical acute seeds or covered reference");
  gate.require(generator_signature(work.edge) == generator_signature(old), "pruning changed seed-generator work");
  if (kmax >= 2) {
    gate.require(pruning_signature(work.pruning) == total_pruning, "edge pruning counters are not seed sums");
    gate.require(work.edge.covered.site_reads == reads, "edge retained-site-read sum");
  } else gate.require(work.edge.seeds == 0 && work.edge.covered.site_reads == 0, "K1 opened edge families");
  if (budget == 0) gate.require(covered_signature(work.edge.covered) == covered_signature(old.covered),
                                "zero-budget pruning changed reference edge work");
  for (const auto& item : actual) {
    if (item.arity == 3) ++gate.q3; else ++gate.q4;
    gate.max_shell = std::max(gate.max_shell,static_cast<u64>(item.shell.size()));
  }
  gate.candidates += actual.size();
  gate.require(Points(cloud->points().begin(),cloud->points().end()) == points, "pruning modified the immutable cloud");
  ++gate.edge_calls; ++gate.reference_calls;
  return actual;
}

Points shell30() {
  Points points{{23,24,20},{20,17,24},{20,17,16},{17,24,20}};
  for (int x = -5; x <= 5; ++x)
    for (int y = -5; y <= 5; ++y)
      for (int z = -5; z <= 5; ++z) {
        if (x*x+y*y+z*z != 25) continue;
        const Point3 point{static_cast<mhgp8::Coordinate>(20+x),static_cast<mhgp8::Coordinate>(20+y),static_cast<mhgp8::Coordinate>(20+z)};
        if (std::find(points.begin(),points.end(),point) == points.end()) points.push_back(point);
      }
  return points;
}

void pipeline_fixtures(Gate& gate) {
  const Points q4_only{{0,0,0},{2,2,0},{2,0,2},{0,2,2},{1,1,1}};
  for (const auto k : {1U,2U,3U,5U,10U})
    for (const auto budget : {0U,1U,32U}) static_cast<void>(check_edge(gate,q4_only,{0,1},k,budget));
  const Points q3_only{{900,1000,1000},{1100,1000,1000},{1000,1120,1040},
    {1000,1120,960},{1000,1020,1105},{1001,1020,1105}};
  const Points both{{20,20,20},{60,60,20},{60,20,60},{20,60,60},
    {40,40,40},{41,40,40},{40,41,40},{40,40,41}};
  const Points late_valid{{0,0,0},{2,2,0},{2,2,2},{2,0,2},{0,2,2}};
  const Points extreme{{0,0,0},{65535,65535,0},{65535,0,65535},{0,65535,65535},
    {32767,32767,32767},{65535,0,0}};
  const Points extreme18{{0,0,0},{262143,262143,0},{262143,0,262143},{0,262143,262143},
    {131071,131071,131071},{262143,0,0},{65535,65535,65535}};
  for (const auto* points : {&q3_only,&both,&late_valid,&extreme,&extreme18})
    for (const auto k : {3U,5U})
      for (const auto budget : {0U,1U,32U}) static_cast<void>(check_edge(gate,*points,{0,1},k,budget));
  for (const auto budget : {0U,3U,32U}) static_cast<void>(check_edge(gate,shell30(),{0,1},5,budget));
  for (const auto* points : {&q4_only,&late_valid})
    for (std::size_t a = 0; a != points->size(); ++a)
      for (std::size_t b = a+1; b != points->size(); ++b) {
        static_cast<void>(check_edge(gate,*points,{a,b},3,3));
        ++gate.exhaustive_edges;
      }
  auto permuted = q3_only;
  std::reverse(permuted.begin(),permuted.end());
  static_cast<void>(check_edge(gate,permuted,{permuted.size()-1,permuted.size()-2},3,32));
  ++gate.permutations;
}

void lifecycle(Gate& gate) {
  const Points points{{0,0,0},{2,2,0},{2,0,2},{0,2,2},{1,1,1}};
  auto cloud = mhgp8::prepare_cloud(points);
  auto index = mhgp8::make_q2_cloud_index(cloud);
  auto cover = mhgp8::Q34EdgeCover::make(index,{0,1});
  auto pool = mhgp8::Q34WitnessPool::make(cover,32);
  const mhgp8::Q34SeedConsumer sink = [](const auto&) {};
  gate.rejects([&] { static_cast<void>(mhgp8::Q34WitnessPool::make({},32)); }, "null pool owner accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q34_pruned_edge_candidates({},3,sink)); }, "null pool accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q34_pruned_edge_candidates(pool,0,sink)); }, "K0 accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q34_pruned_edge_candidates(pool,3,{})); }, "null edge callback accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q34_pruned_seed_candidates(pool,2,0,sink)); }, "seed K0 accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q34_pruned_seed_candidates(pool,2,3,{})); }, "null seed callback accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q34_pruned_seed_candidates(pool,0,3,sink)); }, "repeated seed endpoint accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q34_pruned_seed_candidates(pool,4,3,sink)); }, "nonacute seed accepted");
  gate.rejects<std::out_of_range>([&] { static_cast<void>(mhgp8::run_q34_pruned_seed_candidates(pool,points.size(),3,sink)); }, "outside seed ID accepted");
  check_pool(gate,mhgp8::Q34WitnessPool::make(cover,std::numeric_limits<std::size_t>::max()),std::numeric_limits<std::size_t>::max());
  struct CallbackFailure {};
  bool threw = false;
  try {
    static_cast<void>(mhgp8::run_q34_pruned_seed_candidates(pool,2,3,[&](const auto&) { throw CallbackFailure{}; }));
  } catch (const CallbackFailure&) { threw = true; }
  gate.require(threw, "callback failure did not escape a surviving lane");
  ++gate.callback_failures;
  Output expected;
  {
    const auto execute = [pool] {
      Output output;
      static_cast<void>(mhgp8::run_q34_pruned_edge_candidates(pool,3,[&](const auto& item) { output.push_back(copy(item)); }));
      normalize(output);
      return output;
    };
    expected = execute();
    std::array<std::future<Output>,4> calls;
    for (auto& call : calls) call = std::async(std::launch::async,execute);
    for (auto& call : calls) { gate.require(call.get() == expected, "shared immutable pool leaked mutable state across calls"); ++gate.parallel_calls; }
  }
  Output reset_output;
  static_cast<void>(mhgp8::run_q34_pruned_edge_candidates(pool,3,[&](const auto& item) {
    pool.reset(); cover.reset(); index.reset(); cloud.reset();
    reset_output.push_back(copy(item));
  }));
  normalize(reset_output);
  gate.require(reset_output == expected, "callback reset invalidated the owned pool or borrowed payload");
}

int selftest() {
  Gate gate;
  primitive_fixtures(gate);
  pipeline_fixtures(gate);
  lifecycle(gate);
  gate.require(gate.refused > 0 && gate.certified_q3 > 0 && gate.certified_q4 > 0 &&
    gate.positive_completions > 0 && gate.universal_inside_checks > 0 &&
    gate.strict_boundary > 0 && gate.nonintegral_radius_bounds > 0 &&
    gate.max_j_bits >= 96 && gate.max_product_bits >= 96,
    "primitive non-vacuity floors");
  // 16-bit seeds give J<=3*diam^3<=81*65535^6<2^102.4 and limit*|side|<2^101.6,
  // so bits above 103 and 102 can only come from the 18-bit twins.
  gate.require(gate.wide_primitive_calls > 0 && gate.wide_max_j_bits > 103 && gate.wide_max_product_bits > 102,
    "18-bit primitive non-vacuity floors");
  gate.require(gate.q3_only_rejections > 0 && gate.q4_only_rejections > 0 &&
    gate.both_rejections > 0 && gate.neither_rejections > 0 && gate.saved_site_reads > 0 &&
    gate.q3 > 0 && gate.q4 > 0 && gate.max_shell >= 30 && gate.exhaustive_edges > 0,
    "pipeline non-vacuity floors");
  std::cout << "{\"schema\":\"mhgp8_q34_family_pruning_gate_v1\",\"status\":\"passed\"";
#define MHGP8_PRUNING_FIELD(name) std::cout << ",\"" #name "\":" << gate.name
  MHGP8_PRUNING_FIELD(checks); MHGP8_PRUNING_FIELD(primitive_calls); MHGP8_PRUNING_FIELD(refused);
  MHGP8_PRUNING_FIELD(certified_q3); MHGP8_PRUNING_FIELD(certified_q4);
  MHGP8_PRUNING_FIELD(positive_completions); MHGP8_PRUNING_FIELD(universal_inside_checks);
  MHGP8_PRUNING_FIELD(strict_boundary); MHGP8_PRUNING_FIELD(nonintegral_radius_bounds);
  MHGP8_PRUNING_FIELD(max_j_bits); MHGP8_PRUNING_FIELD(max_product_bits);
  MHGP8_PRUNING_FIELD(edge_calls); MHGP8_PRUNING_FIELD(seed_calls); MHGP8_PRUNING_FIELD(reference_calls);
  MHGP8_PRUNING_FIELD(oracle_completions); MHGP8_PRUNING_FIELD(oracle_sites); MHGP8_PRUNING_FIELD(candidates);
  MHGP8_PRUNING_FIELD(q3); MHGP8_PRUNING_FIELD(q4); MHGP8_PRUNING_FIELD(max_shell);
  MHGP8_PRUNING_FIELD(q3_only_rejections); MHGP8_PRUNING_FIELD(q4_only_rejections);
  MHGP8_PRUNING_FIELD(both_rejections); MHGP8_PRUNING_FIELD(neither_rejections);
  MHGP8_PRUNING_FIELD(saved_site_reads); MHGP8_PRUNING_FIELD(pool_calls); MHGP8_PRUNING_FIELD(exhaustive_edges);
  MHGP8_PRUNING_FIELD(permutations); MHGP8_PRUNING_FIELD(invalid_inputs);
  MHGP8_PRUNING_FIELD(callback_failures); MHGP8_PRUNING_FIELD(parallel_calls);
  MHGP8_PRUNING_FIELD(wide_primitive_calls); MHGP8_PRUNING_FIELD(wide_max_j_bits); MHGP8_PRUNING_FIELD(wide_max_product_bits);
  MHGP8_PRUNING_FIELD(max_sqrt_iterations);
#undef MHGP8_PRUNING_FIELD
  std::cout << "}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try { return selftest(); }
  catch (const std::exception& error) {
    std::cerr << "q34 family pruning gate: " << error.what() << '\n';
    return 1;
  }
}
