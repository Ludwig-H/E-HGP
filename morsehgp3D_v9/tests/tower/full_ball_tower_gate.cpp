// Bounded independent Gram/Gamma judge of the ball-catalogue FULL producer.
// All subset enumeration, explicit facet membership and point sets live HERE,
// never in the product. Catalogues are not made by WSPD or product predicates.
#include <algorithm>
#include <bit>
#include <cstdio>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "../../oracle/tower/local_plateau_oracle.hpp"
#include "../../src/tower/forest/full_ball_tower.hpp"

#ifdef MHGP9_TESTING
#error This gate must use the unmodified product path
#endif

namespace {
using namespace mhgp9::tower;
namespace oracle = local_plateau_oracle;
using oracle::Int;
using oracle::Rat;
using Component = std::vector<u32>;
using Snapshot = std::map<u32, FullNodeId>;  // oracle facet -> product live root
struct Failure { const char* why; };
std::string context;
int static_threads = 0;
u64 paired_payload_checks = 0, deduplicated = 0, seeded_unique = 0;
u64 post_seed_queries = 0, post_seed_hits = 0, post_seed_terminals = 0;
u64 checks = 0, clouds = 0, orders = 0, cuts = 0, facets = 0, vertical = 0;
u64 births = 0, merges = 0, continuations = 0, growth = 0, rejections = 0;
u64 overlap_cuts = 0, duplicate_cover_cuts = 0, regular = 0, extra = 0;
u64 anchors = 0, representatives = 0, intruders = 0, same_radius = 0;

void need(bool good, const char* why) {
  ++checks;
  if (!good) throw Failure{why};
}

Int integer(i128 n) {
  const bool negative = n < 0;
  const u128 magnitude = negative ? static_cast<u128>(-(n + 1)) + 1 : static_cast<u128>(n);
  Int value = static_cast<u64>(magnitude >> 64);
  value <<= 64;
  value += static_cast<u64>(magnitude);
  return negative ? -value : value;
}

i128 narrow(const Int& value) {
  const bool negative = value < 0;
  const Int magnitude = negative ? -value : value;
  need(magnitude < (Int(1) << 127), "oracle.int128_fixture_bound");
  const Int low = magnitude & ((Int(1) << 64) - 1);
  const Int high = magnitude >> 64;
  const u128 packed = (static_cast<u128>(high.convert_to<u64>()) << 64) |
      low.convert_to<u64>();
  return negative ? -static_cast<i128>(packed) : static_cast<i128>(packed);
}

Int gcd(Int a, Int b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b != 0) { const Int remainder = a % b; a = b; b = remainder; }
  return a;
}

BallKey key(const oracle::Ball& ball) {
  std::array<Rat, 5> coefficients{Rat(1), -Rat(2) * ball.center[0],
      -Rat(2) * ball.center[1], -Rat(2) * ball.center[2], -ball.radius2};
  for (const auto& coordinate : ball.center) coefficients[4] += coordinate * coordinate;
  Int denominator = 1;
  for (const auto& c : coefficients)
    denominator = denominator / gcd(denominator, c.denominator()) * c.denominator();
  std::array<Int, 5> integral;
  Int divisor = 0;
  for (size_t j = 0; j < coefficients.size(); ++j) {
    integral[j] = coefficients[j].numerator() * (denominator / coefficients[j].denominator());
    divisor = gcd(divisor, integral[j]);
  }
  for (auto& c : integral) c /= divisor;
  return {narrow(integral[0]), {narrow(integral[1]), narrow(integral[2]),
      narrow(integral[3])}, narrow(integral[4])};
}

ExactLevel level(const Rat& value) {
  need(value >= Rat(0) && value.numerator() < (Int(1) << 192), "oracle.level_bound");
  ExactLevel result{};
  Int remaining = value.numerator();
  const Int mask = (Int(1) << 64) - 1;
  for (size_t j = 0; j < 3; ++j) {
    result.num[j] = (remaining & mask).convert_to<u64>();
    remaining >>= 64;
  }
  result.den = narrow(value.denominator());
  return result;
}

Rat rational(const ExactLevel& value) {
  Int numerator = 0;
  for (size_t j = 3; j-- > 0;) { numerator <<= 64; numerator += value.num[j]; }
  need(value.den > 0, "output.positive_denominator");
  return Rat(numerator, integer(value.den));
}

Rat distance2(const P3& p, const oracle::Ball& ball) {
  const std::array<i64, 3> coordinates{p.x, p.y, p.z};
  Rat result(0);
  for (size_t j = 0; j < 3; ++j) {
    const Rat delta = Rat(coordinates[j]) - ball.center[j];
    result += delta * delta;
  }
  return result;
}

struct Fixture { const char* name; std::vector<P3> points; unsigned kmax; };
std::vector<Fixture> fixtures() {
  return {
    {"pair", {{0,0,0},{2,0,0}}, 2},
    {"E5", {{0,0,7},{0,9,6},{1,4,0},{0,0,1},{4,1,2}}, 5},
    {"square", {{0,0,0},{2,0,0},{2,2,0},{0,2,0}}, 4},
    {"growth_ABCZ", {{1,8,0},{5,10,0},{9,8,0},{5,0,0}}, 4},
    {"growth_redundant", {{1,8,0},{5,10,0},{9,8,0},{5,0,0},{10,6,0},{9,1,0}}, 4},
    {"inert_ball", {{2,2,2},{2,0,0},{0,2,0},{0,0,2},{0,0,0}}, 5},
    {"shell7_window", {{10,5,0},{0,5,0},{5,10,0},{5,0,0},{8,9,0},{2,1,0},{9,8,0}}, 5},
    {"support3", {{0,0,0},{2,2,0},{2,0,2}}, 3},
    {"support4", {{0,0,0},{2,2,0},{2,0,2},{0,2,2}}, 4},
    {"u16_tetra", {{0,0,0},{65534,65534,0},{65534,0,65534},{0,65534,65534}}, 4},
    {"u18_tetra", {{0,0,0},{262142,262142,0},{262142,0,262142},{0,262142,262142}}, 4},
    {"u18_corners", {{262143,0,0},{0,262143,0},{0,0,262143},{262143,262143,262143},{131071,131072,1}}, 5},
    {"portal_equal", {{0,2,0},{2,4,0},{4,2,0},{2,0,0},{2,2,0},{2,1,0}}, 6},
    // The first four sites have a common circle and two positive triangles.
    // Its three global strict intruders make p+q_min=6 > smax=5: no anchor.
    // A future retained ball requests these four sites; removing the first
    // support vertex preserves the circle through the alternative triangle.
    {"actual_equal_radius_descent", {{35,100,0},{139,48,0},{152,139,0},{48,139,0},
                                    {100,36,0},{101,37,0},{99,37,0},{100,200,0}}, 4},
    // Independent audit 2026-09-10: the original growth/inert cases use
    // singleton lots after the fast-path split. These force GROUPED lots.
    {"growth_ABCZ_doubled_lot", {{1,8,0},{5,10,0},{9,8,0},{5,0,0},
                                {201,8,0},{205,10,0},{209,8,0},{205,0,0}}, 3},
    {"inert_ball_doubled_lot", {{2,2,2},{2,0,0},{0,2,0},{0,0,2},{0,0,0},
                               {100,100,100},{102,102,102}}, 3}
  };
}

std::vector<InputPoint> input(const Fixture& fixture, unsigned variant) {
  const PointId ids[] = {std::numeric_limits<PointId>::max(),17,0,902,2147483648u,3,65536,42};
  std::vector<InputPoint> result;
  for (size_t j = 0; j < fixture.points.size(); ++j)
    result.push_back({variant ? ids[j] : static_cast<PointId>(j), fixture.points[j]});
  if (variant) std::reverse(result.begin(), result.end());
  return result;
}

std::vector<BallData> catalogue(const std::vector<P3>& points, const CloudIndex& ix,
    const oracle::Model& model, unsigned kmax) {
  std::map<BallKey, oracle::Ball> balls;
  const u32 domain = (u32{1} << points.size()) - 1;
  for (u32 mask = 1; mask <= domain; ++mask) {
    const auto ball = model.meb(mask);
    if (ball.radius2 != Rat(0)) balls.emplace(key(ball), ball);
  }
  std::vector<BallData> result;
  for (const auto& [ball_key, ball] : balls) {
    BallData row{};
    row.key = ball_key;
    row.level = level(ball.radius2);
    u32 shell = 0;
    for (size_t bit = 0; bit < points.size(); ++bit) {
      const Rat d = distance2(points[bit], ball);
      if (d > ball.radius2) continue;
      const auto found = std::find(ix.upos.begin(), ix.upos.end(), points[bit]);
      need(found != ix.upos.end(), "oracle.geometry_index");
      const i32 index = static_cast<i32>(found - ix.upos.begin());
      if (d == ball.radius2) {
        need(row.n_shell < kBallShellMax, "oracle.shell_fixture_bound");
        row.shell_ids[row.n_shell++] = index;
        shell |= u32{1} << bit;
      } else {
        need(row.n_interior < kBallInteriorMax, "oracle.interior_fixture_bound");
        row.interior_ids[row.n_interior++] = index;
      }
    }
    unsigned qmin = 5;
    for (u32 support = shell; support; support = (support - 1) & shell) {
      const unsigned q = std::popcount(support);
      if (q >= qmin || q > 4) continue;
      const auto candidate = oracle::detail::support_ball(points, support);
      if (candidate && candidate->center == ball.center && candidate->radius2 == ball.radius2)
        qmin = q;
    }
    need(qmin >= 2 && qmin <= 4, "oracle.positive_minimal_support");
    row.arity = static_cast<u8>(qmin);
    if (row.n_interior + qmin > std::min<size_t>(kmax + 1, points.size())) continue;
    std::sort(row.interior_ids, row.interior_ids + row.n_interior);
    std::sort(row.shell_ids, row.shell_ids + row.n_shell);
    result.push_back(row);
  }
  return result;
}

std::vector<PointId> cover(const Component& component, const std::vector<InputPoint>& in) {
  u32 mask = 0;
  for (u32 facet : component) mask |= facet;
  std::vector<PointId> result;
  for (size_t bit = 0; bit < in.size(); ++bit)
    if (mask & (u32{1} << bit)) result.push_back(in[bit].id);
  std::sort(result.begin(), result.end());
  return result;
}

std::vector<PointId> read(const FullCoverageCertificate& forest, FullNodeId root,
    const Rat& cut, bool closed) {
  auto result = full_coverage_at(forest, root, level(cut), closed);
  need(result.status == FullCertificateStatus::kOk, "coverage.reader_valid");
  return result.values;
}

std::vector<FullNodeId> parents(const FullCoverageCertificate& forest, FullNodeId node) {
  const auto& record = forest.nodes().at(node);
  need(record.first <= forest.parents().size() &&
      record.parent_count <= forest.parents().size() - record.first, "topology.parent_range");
  return {forest.parents().begin() + record.first,
      forest.parents().begin() + record.first + record.parent_count};
}

// Match oracle components by chronological identity, not by point overlap.
// A continuation retains exactly one old root; a new node has exactly the
// oracle's old roots as parents. Birth ambiguity is rejected, never guessed.
Snapshot check_cut(const FullCoverageCertificate& forest,
    const std::vector<Component>& expected, const Snapshot& before,
    const std::vector<InputPoint>& in, const Rat& cut, bool closed) {
  std::vector<FullNodeId> live;
  std::vector<std::vector<PointId>> actual_covers, expected_covers;
  for (FullNodeId node = 0; node < forest.nodes().size(); ++node)
    if (full_coverage_root_at(forest, node, level(cut), closed) == node) {
      live.push_back(node);
      actual_covers.push_back(read(forest, node, cut, closed));
    }
  for (const auto& component : expected) expected_covers.push_back(cover(component, in));
  auto sorted_actual = actual_covers;
  auto sorted_expected = expected_covers;
  std::sort(sorted_actual.begin(), sorted_actual.end());
  std::sort(sorted_expected.begin(), sorted_expected.end());
  need(sorted_actual == sorted_expected, "gamma.coverage_multiset_including_multiplicity");
  if (std::adjacent_find(sorted_expected.begin(), sorted_expected.end()) != sorted_expected.end())
    ++duplicate_cover_cuts;
  bool overlap = false;
  for (size_t a = 0; a < expected_covers.size(); ++a)
    for (size_t b = a + 1; b < expected_covers.size(); ++b)
      for (auto point : expected_covers[a])
        overlap = overlap || std::binary_search(expected_covers[b].begin(), expected_covers[b].end(), point);
  overlap_cuts += overlap;
  Snapshot after;
  std::set<FullNodeId> used;
  for (size_t c = 0; c < expected.size(); ++c) {
    std::set<FullNodeId> prior;
    for (u32 facet : expected[c])
      if (const auto found = before.find(facet); found != before.end()) prior.insert(found->second);
    const std::vector<FullNodeId> old(prior.begin(), prior.end());
    std::vector<FullNodeId> matches;
    for (size_t j = 0; j < live.size(); ++j) {
      const FullNodeId candidate = live[j];
      if (used.contains(candidate) || actual_covers[j] != expected_covers[c]) continue;
      if (old.size() == 1) {
        if (candidate == old.front()) matches.push_back(candidate);
      } else if (closed && rational(forest.nodes()[candidate].level) == cut &&
          parents(forest, candidate) == old) matches.push_back(candidate);
    }
    need(matches.size() == 1, "gamma.unique_history_mapping_birth_continuation_or_multifusion");
    const auto root = matches.front();
    used.insert(root);
    if (closed) {
      if (old.empty()) ++births;
      else if (old.size() > 1) ++merges;
      else {
        ++continuations;
        if (read(forest, root, cut, false) != expected_covers[c]) ++growth;
      }
    }
    for (u32 facet : expected[c]) { after.emplace(facet, root); ++facets; }
  }
  need(used.size() == live.size(), "gamma.all_live_roots_identified");
  ++cuts;
  return after;
}

void same_payload(const FullBallTowerResult& a, const FullBallTowerResult& b) {
  const auto exact = [](const ExactLevel& x, const ExactLevel& y) {
    return std::equal(std::begin(x.num), std::end(x.num), std::begin(y.num)) && x.den == y.den;
  };
  need(a.status == FullBallStatus::kCompleteRelative && a.status == b.status &&
       a.orders.size() == b.orders.size(), "paired.complete_towers");
  for (size_t k = 0; k < a.orders.size(); ++k) {
    const auto& x = a.orders[k].forest;
    const auto& y = b.orders[k].forest;
    need(x.order() == y.order() && x.nodes().size() == y.nodes().size() &&
         x.parents() == y.parents() && x.successors() == y.successors() &&
         a.orders[k].lower_nodes == b.orders[k].lower_nodes, "paired.exact_topology_vertical");
    need(x.populations()->domain() == y.populations()->domain() &&
         x.populations()->rows().size() == y.populations()->rows().size(), "paired.bank_shape");
    for (size_t p = 0; p < x.populations()->rows().size(); ++p)
      need(x.populations()->rows()[p].interior == y.populations()->rows()[p].interior &&
           x.populations()->rows()[p].shell == y.populations()->rows()[p].shell, "paired.bank_rows");
    for (size_t n = 0; n < x.nodes().size(); ++n)
      need(exact(x.nodes()[n].level, y.nodes()[n].level) && x.nodes()[n].first == y.nodes()[n].first &&
           x.nodes()[n].parent_count == y.nodes()[n].parent_count, "paired.exact_node");
    need(x.contributions().size() == y.contributions().size(), "paired.contribution_shape");
    for (size_t c = 0; c < x.contributions().size(); ++c) {
      const auto& u = x.contributions()[c]; const auto& v = y.contributions()[c];
      need(exact(u.level, v.level) && u.segment == v.segment && u.ref.population == v.ref.population &&
           u.ref.shell_mask == v.ref.shell_mask && u.ref.include_interior == v.ref.include_interior,
           "paired.exact_contribution");
    }
  }
}

void check_fixture(const Fixture& fixture, unsigned variant) {
  context = std::string(fixture.name) + "/variant" + std::to_string(variant);
  const auto in = input(fixture, variant);
  std::vector<P3> points;
  for (const auto& p : in) points.push_back(p.position);
  const oracle::Model model(points);
  const auto ix = build_cloud_index(in);
  auto balls = catalogue(points, ix, model, fixture.kmax);
  if (variant) std::reverse(balls.begin(), balls.end());
  if (std::string_view(fixture.name) == "post_seed_square_partial") {
    const auto circle = q2_ball_key(fixture.points[0], fixture.points[3]);
    const auto found = std::find_if(balls.begin(), balls.end(), [&](const BallData& b) { return b.key == circle; });
    need(found != balls.end() && found->n_interior == 4 && found->n_shell == 4,
         "post_seed.square_whole_population_eight");
    const std::array<P3,4> original{fixture.points[0],fixture.points[1],fixture.points[2],fixture.points[3]};
    const std::array<P3,4> next{fixture.points[1],fixture.points[2],fixture.points[3],fixture.points[4]};
    AnchorMebWork work;
    const auto a = anchor_meb(original, work), b = anchor_meb(next, work);
    need(a.status == AnchorMebStatus::kOk && b.status == AnchorMebStatus::kOk && a.key == circle &&
         b.key == circle && same_exact_level(a.level,b.level) && a.selected_shell_count == 4 &&
         b.selected_shell_count == 3, "post_seed.square_equal_radius_is_not_a_hit");
    std::array<i32,4> selected{};
    for (size_t j = 0; j < next.size(); ++j) {
      const auto at = std::find(ix.upos.begin(),ix.upos.end(),next[j]);
      need(at != ix.upos.end(), "post_seed.square_index_remap");
      selected[j] = static_cast<i32>(at-ix.upos.begin());
    }
    std::sort(selected.begin(),selected.end());
    const auto key = full_ball_detail::ResolverCache::key(selected);
    for (const auto& seed : balls) {
      if (static_cast<unsigned>(seed.n_interior)+seed.n_shell != selected.size()) continue;
      full_ball_detail::ResolverCache::Key entire{};
      size_t n = 0;
      for (i32 site : seed.interior()) entire[n++] = site;
      for (i32 site : seed.shell()) entire[n++] = site;
      std::sort(entire.begin(),entire.begin()+n);
      need(entire != key, "post_seed.square_partial_population_misses_lookup");
    }
  }
  auto result = build_full_ball_tower(ix, balls, fixture.kmax, static_threads);
  if (static_threads) {
    const auto baseline = build_full_ball_tower(ix, balls, fixture.kmax);
    const u64 before_checks = checks;
    same_payload(baseline, result);
    paired_payload_checks += checks - before_checks;
    for (unsigned k = 2; k <= fixture.kmax; ++k) {
      need(result.stats.static_unique[k] <= result.stats.static_requests[k] &&
           result.stats.static_seeded[k] <= result.stats.static_unique[k], "static.work_partition");
      deduplicated += result.stats.static_requests[k] - result.stats.static_unique[k];
      seeded_unique += result.stats.static_seeded[k];
      need(result.stats.static_post_seed_hits[k] == result.stats.static_post_seed_terminals[k] &&
           result.stats.static_post_seed_hits[k] <= result.stats.static_post_seed_queries[k] &&
           result.stats.static_post_seed_hits[k] <= result.stats.static_unique[k] - result.stats.static_seeded[k],
           "static.post_seed_per_order_bounds");
      post_seed_queries += result.stats.static_post_seed_queries[k];
      post_seed_hits += result.stats.static_post_seed_hits[k];
      post_seed_terminals += result.stats.static_post_seed_terminals[k];
    }
    need(result.stats.resolve_work.calls == result.stats.anchor_hits + result.stats.intruder_queries,
         "static.actual_meb_accounting");
    need(result.stats.resolver_cache_slots == 0, "static.no_temporal_token_cache");
    const auto sum = [](const auto& values) { return std::accumulate(values.begin(), values.end(), u64{0}); };
    need(sum(result.stats.static_post_seed_queries) == result.stats.descending_steps + result.stats.same_radius_steps,
         "static.every_exchange_lookup_is_paid");
    need(result.stats.anchor_hits + sum(result.stats.static_post_seed_terminals) ==
         sum(result.stats.static_unique) - sum(result.stats.static_seeded), "static.actual_terminal_partition");
    if (std::string_view(fixture.name) == "post_seed_ABEZW")
      need(sum(result.stats.static_post_seed_hits) > 0, "post_seed.five_point_nonvacuity");
  }
  if (result.status != FullBallStatus::kCompleteRelative)
    std::fprintf(stderr, "product refusal [%s]: %s\n", context.c_str(), result.reason);
  need(result.status == FullBallStatus::kCompleteRelative && result.orders.size() == fixture.kmax,
      "producer.complete_relative_orders");
  regular += result.stats.regular_blocks;
  extra += result.stats.extra_blocks;
  anchors += result.stats.anchor_hits;
  representatives += result.stats.representatives;
  intruders += result.stats.intruder_queries;
  same_radius += result.stats.same_radius_steps;
  const u32 domain = (u32{1} << in.size()) - 1;
  std::set<Rat> levels{Rat(0)};
  for (u32 mask = 1; mask <= domain; ++mask) levels.insert(model.meb(mask).radius2);
  levels.insert(*levels.rbegin() + Rat(1));
  std::vector<Rat> times(levels.begin(), levels.end());
  std::vector<std::vector<Snapshot>> history(fixture.kmax);
  for (unsigned k = 1; k <= fixture.kmax; ++k) {
    const auto& forest = result.orders[k - 1].forest;
    need(forest.order() == k, "producer.order_identity");
    need(result.orders[k - 1].lower_nodes.size() == forest.nodes().size(), "vertical.node_indexed");
    Snapshot previous;
    for (const auto& time : times) for (bool closed : {false, true}) {
      const auto expected = model.components(k, time, closed, domain);
      const auto current = check_cut(forest, expected, previous, in, time, closed);
      history[k - 1].push_back(current);
      if (closed) previous = current;
    }
    ++orders;
  }
  for (unsigned k = 1; k <= fixture.kmax; ++k) {
    size_t index = 0;
    for (const auto& time : times) for (bool closed : {false, true}) {
      const auto& current = history[k - 1][index];
      for (const auto& [facet, root] : current) {
        const auto actual = full_ball_vertical_root_at(result, k, root, level(time), closed);
        if (k == 1) need(actual == kFullCoverageAbsent, "vertical.K1_absent");
        else {
          const auto& lower = history[k - 2][index];
          for (u32 bits = facet; bits; bits &= bits - 1) {
            const u32 subfacet = facet & ~(u32{1} << std::countr_zero(bits));
            const auto found = lower.find(subfacet);
            need(found != lower.end() && actual == found->second,
                "vertical.all_subfacets_at_same_open_or_closed_cut");
            ++vertical;
          }
        }
      }
      ++index;
    }
  }
  if (std::string_view(fixture.name) == "square") {
    const auto& f = result.orders[2].forest;
    need(f.nodes().size() == 1 && read(f, 0, Rat(2), true).size() == 4,
        "square.birth_coverage_exceeds_K");
    const auto& k1 = result.orders[0].forest;
    need(k1.nodes().size() == 5 && k1.nodes().back().parent_count == 4,
        "square.same_level_four_balls_atomic_multifusion");
  }
  if (std::string_view(fixture.name) == "growth_ABCZ") {
    const auto& f = result.orders[2].forest;
    need(f.nodes().size() == 1 && read(f, 0, Rat(25), false).size() == 3 &&
        read(f, 0, Rat(25), true).size() == 4, "ABCZ.dated_growth_without_fake_topology_node");
  }
  if (std::string_view(fixture.name) == "shell7_window") {
    const auto& f = result.orders[4].forest;
    need(f.nodes().size() == 1 && read(f, 0, Rat(25), true).size() == 7,
        "shell7.population_above_smax_is_not_discarded");
  }
  if (std::string_view(fixture.name) == "actual_equal_radius_descent")
    need(result.stats.same_radius_steps > 0 && result.stats.intruder_queries > 0,
        "descent.same_radius_branch_exercised_by_actual_tower");
  if (std::string_view(fixture.name).ends_with("_doubled_lot"))
    need(result.stats.grouped_lots > 0 && result.stats.lot_dsu_slots > 0,
        "grouped.fixture_not_singleton_only");
  ++clouds;
}

void rejection_fixtures() {
  context = "rejections";
  const Fixture fixture{"square", {{0,0,0},{2,0,0},{2,2,0},{0,2,0}}, 4};
  const auto in = input(fixture, 0);
  const auto ix = build_cloud_index(in);
  const oracle::Model model(fixture.points);
  const auto balls = catalogue(fixture.points, ix, model, fixture.kmax);
  const auto reject = [&](const CloudIndex& index, const std::vector<BallData>& catalogue,
      unsigned kmax, const char* why) {
    const auto result = build_full_ball_tower(index, catalogue, kmax, static_threads);
    need(result.status != FullBallStatus::kCompleteRelative && result.orders.empty(), why);
    ++rejections;
  };
  reject(ix, balls, 0, "reject.zero_order");
  reject(ix, balls, 11, "reject.order_above_representation");
  reject(build_cloud_index(std::vector<P3>{}), balls, 1, "reject.empty_cloud");
  reject(build_cloud_index(std::vector<P3>{{1,1,1},{1,1,1}}), balls, 1, "reject.duplicate_positions");
  auto invalid = balls;
  invalid.push_back(balls.front());
  reject(ix, invalid, 4, "reject.duplicate_ball_key");
  invalid = balls;
  invalid.front().level.den = 0;
  reject(ix, invalid, 4, "reject.zero_level_denominator");
  invalid = balls;
  invalid.front().level = level(Rat(999));
  reject(ix, invalid, 4, "reject.inconsistent_ball_level");
  // Remove every old connector needed by the square's diameter block. A
  // missing weak terminal MUST fail; relative authority does not promise
  // detection of every possible incomplete catalogue.
  invalid.clear();
  for (const auto& ball : balls) if (rational(ball.level) == Rat(2)) invalid.push_back(ball);
  need(invalid.size() == 1, "reject.nonvacuous_missing_anchors_fixture");
  reject(ix, invalid, 4, "reject.missing_required_weak_terminal_anchor");
}

int selftest() {
  try {
    auto cases = fixtures();
    // Extra static-admission test; keep the nominal shared fixture set stable.
    if (static_threads) {
      cases.push_back({"present_but_wrong_rank",
          {{35,100,0},{139,48,0},{152,139,0},{48,139,0},
           {100,36,0},{101,37,0},{99,37,0},{100,200,0}}, 6});
      cases.push_back({"post_seed_ABEZW", {{0,3,0},{8,3,0},{4,2,0},{6,0,0},{4,23,0}}, 5});
      cases.push_back({"post_seed_square_partial", {{0,0,0},{10,0,0},{0,10,0},{10,10,0},
           {4,4,0},{4,6,0},{6,4,0},{6,6,0}}, 8});
    }
    for (const auto& fixture : cases) for (unsigned variant = 0; variant < 2; ++variant)
      check_fixture(fixture, variant);
    rejection_fixtures();
    need(clouds == (static_threads ? 38u : 32u) && orders >= (static_threads ? 150u : 112u) &&
        cuts >= 500 && facets >= 1000 && vertical >= 1000,
        "nonvacuity.oracle_tower_and_vertical");
    need(births > 50 && merges > 10 && growth > 0 && overlap_cuts > 0 &&
        regular > 0 && extra > 0 && anchors > 0 && representatives > 0 && intruders > 0 && same_radius > 0,
        "nonvacuity.topology_growth_overlap_and_resolver");
    if (static_threads) {
      need(paired_payload_checks > 100 && deduplicated > 0 && seeded_unique > 0,
           "nonvacuity.static_dedup_seed_and_exact_payload");
      need(post_seed_hits >= 4 && post_seed_queries >= post_seed_hits && post_seed_terminals == post_seed_hits,
           "nonvacuity.post_exchange_complete_population_hits");
      std::fprintf(stderr, "static_threads=%d paired_payload_checks=%llu deduplicated=%llu seeded_unique=%llu\n",
          static_threads, static_cast<unsigned long long>(paired_payload_checks),
          static_cast<unsigned long long>(deduplicated), static_cast<unsigned long long>(seeded_unique));
      std::fprintf(stderr, "post_seed_queries=%llu post_seed_hits=%llu post_seed_terminals=%llu\n",
          static_cast<unsigned long long>(post_seed_queries), static_cast<unsigned long long>(post_seed_hits),
          static_cast<unsigned long long>(post_seed_terminals));
    }
    std::printf("{\"status\":\"passed\",\"authority\":\"bounded_independent_Gram_Gamma_not_WSPD_completeness\","
        "\"checks\":%llu,\"clouds\":%llu,\"orders\":%llu,\"cuts\":%llu,\"facets\":%llu,"
        "\"vertical_checks\":%llu,\"births\":%llu,\"merges\":%llu,\"continuation_snapshots\":%llu,"
        "\"growth_snapshots\":%llu,\"overlap_cuts\":%llu,\"duplicate_cover_cuts\":%llu,"
        "\"regular_blocks\":%llu,\"extra_blocks\":%llu,\"anchor_hits\":%llu,"
        "\"representatives\":%llu,\"intruder_queries\":%llu,\"same_radius_steps\":%llu,\"rejections\":%llu}\n",
        static_cast<unsigned long long>(checks), static_cast<unsigned long long>(clouds),
        static_cast<unsigned long long>(orders), static_cast<unsigned long long>(cuts),
        static_cast<unsigned long long>(facets), static_cast<unsigned long long>(vertical),
        static_cast<unsigned long long>(births), static_cast<unsigned long long>(merges),
        static_cast<unsigned long long>(continuations), static_cast<unsigned long long>(growth),
        static_cast<unsigned long long>(overlap_cuts), static_cast<unsigned long long>(duplicate_cover_cuts),
        static_cast<unsigned long long>(regular), static_cast<unsigned long long>(extra),
        static_cast<unsigned long long>(anchors), static_cast<unsigned long long>(representatives),
        static_cast<unsigned long long>(intruders), static_cast<unsigned long long>(same_radius),
        static_cast<unsigned long long>(rejections));
    return 0;
  } catch (const Failure& failure) {
    std::fprintf(stderr, "FAIL [%s] %s\n", context.c_str(), failure.why);
  } catch (const std::exception& error) {
    std::fprintf(stderr, "EXCEPTION [%s] %s\n", context.c_str(), error.what());
  }
  return 1;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--static-1") static_threads = 1;
  else if (argc == 2 && std::string_view(argv[1]) == "--static-4") static_threads = 4;
  else if (argc == 2 && std::string_view(argv[1]) == "--expect-launch-failure") {
    const auto fixture = fixtures()[1];
    const auto in = input(fixture, 0);
    const auto ix = build_cloud_index(in);
    const oracle::Model model(fixture.points);
    const auto balls = catalogue(fixture.points, ix, model, fixture.kmax);
    const auto result = build_full_ball_tower(ix, balls, fixture.kmax, 4);
    if (result.status != FullBallStatus::kResourceExhausted || !result.orders.empty() ||
        std::string_view(result.reason) != "full_ball_thread_launch_failed") return 1;
    std::puts("{\"status\":\"passed\",\"targeted_launch_failure\":true}");
    return 0;
  }
  else if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::fprintf(stderr, "usage: mhgp9_tower_full_ball_tower_gate --selftest\n");
    return 2;
  }
  return selftest();
}
