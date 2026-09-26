// MorseHGP3D v9 — porte du recensement q2 cote q2 (levier q2_early_census,
// v28, 25 septembre 2026).
//
// Juge : sous q2_early_census, le recensement des cles q2 (representant q2 de
// plus petit support) tourne sur le fil de q2 pendant les appels q34 et la
// chaine reprend ses resultats apres la fusion. Tout ce que le recensement
// produit doit etre identique a la chaine sans le levier, octet pour octet et
// pour tout W : statut et raison, condenses (tour, catalogue, presentations),
// catalogue recoupe boule par boule dans l'ordre des groupes, statistiques
// (arites minimales, coquilles, Euler, noeuds et tests du census, refus de
// coquille), tower_work et resumes d'ordres.
//
//   --selftest [--n=1000] : familles uniform, terrain, clusters, une fixture
//     cospherique (coquilles q2 etendues de 10 sites, plusieurs paires
//     diametrales par cle) et une fixture a coquille q2 de 14 sites (refus
//     chain_shell_above_12 atteint cote q2) ; K1, K2, K3, K5, K10 ; W 1, 3
//     et 8 ; chaque bras compare a la reference sans levier a W1. Chronos :
//     l'attente cote q2 ne depasse pas le mur cote q2, la somme des etapes
//     (attentes comprises, murs recouverts exclus) ne depasse pas le total.
//     Planchers : cles recensees cote q2, coquilles etendues cote q2, refus
//     de coquille cote q2, cas compares. Levier sans q2_during_device refuse.
//   --priority : points de panne (cible de test) ; deux pannes de census, la
//     plus petite cle gagne (q2 contre q2, q2 contre q3/q4 dans les deux
//     sens, q3/q4 contre q3/q4) ; une panne de census cote q2 ne passe jamais
//     devant une panne de q2, de q34 ou de la fusion ; meme statut et meme
//     raison sur les deux bras a W 1, 4 et 8. Plancher : pannes vues cote q2.
//   --fallback : voie de repli du cote q2 (points de panne) ; une panne hors
//     du census cle par cle (apres l'index, apres le census de toutes ses
//     cles) : meme objet que sans le levier au meme W, aucune cle reprise du
//     cote q2, index reconstruit apres la fusion ; avec une panne de census
//     en plus, meme raison ; un echec de q34 annule le cote q2 (retenu
//     jusqu'a l'annulation), qui ne recense alors aucune cle. Planchers :
//     executions, replis, annulations, cles recensees avant la panne.
//   --frame=CHEMIN --k=K --tower=HEX --catalogue=HEX [--pairs=P] [--workers=W]
//     [--lever=0|1] : trame LiDAR sur le chemin par lots CPU (q2 pendant les
//     appels) ; P paires entrelacees levier coupe / actif, condenses epingles
//     sur les deux bras et meme objet (octets du recensement, tower_work) ;
//     avec --lever, ce seul bras (un processus par bras pour une serie A/B).
//     Chronos et pic de RSS publies (mesure indicative).
//
// Code 0 conforme, 1 desaccord (ligne `cause=`), 2 argument, 3 plancher.
#include <sys/resource.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "../../src/chain/tower_chain.hpp"
#include "../gen/front_fixtures.hpp"

#if !defined(MHGP9_TESTING)
#error This gate needs the MHGP9_TESTING failpoints
#endif

namespace {

using namespace mhgp9;
using Key5 = std::array<gen::i128, 5>;

int fail(const std::string& cause) {
  std::printf("cause=%s\n", cause.c_str());
  return 1;
}

Key5 key5(const tower::BallKey& k) { return Key5{k.a, k.b[0], k.b[1], k.b[2], k.c}; }

// Two spheres of radius 35, six axis points and four points of the great
// circle z = 0 each: q2 keys with five diametral pairs and 10-site shells.
std::vector<gen::Point3> sphere_fixture() {
  std::vector<gen::Point3> spheres;
  for (const std::int32_t cx : {1000, 1100}) {
    for (const auto& d : {std::array<int, 3>{35, 0, 0}, {-35, 0, 0}, {0, 35, 0}, {0, -35, 0}, {0, 0, 35},
                          {0, 0, -35}, {21, 28, 0}, {-21, 28, 0}, {21, -28, 0}, {-21, -28, 0}})
      spheres.push_back({cx + d[0], 1000 + d[1], 1000 + d[2]});
  }
  return spheres;
}

// Fourteen sites of one sphere of radius 35 (axes, z = 0 and x = 0 great
// circles) and a few sites around: the q2 ball of every antipodal pair has
// a 14-site shell, a domain refusal (chain_shell_above_12).
std::vector<gen::Point3> over_cap_fixture() {
  std::vector<gen::Point3> points;
  for (const auto& d : {std::array<int, 3>{35, 0, 0}, {-35, 0, 0}, {0, 35, 0}, {0, -35, 0}, {0, 0, 35},
                        {0, 0, -35}, {21, 28, 0}, {-21, 28, 0}, {21, -28, 0}, {-21, -28, 0}, {0, 21, 28},
                        {0, -21, 28}, {0, 21, -28}, {0, -21, -28}})
    points.push_back({2000 + d[0], 2000 + d[1], 2000 + d[2]});
  for (const auto& d : {std::array<int, 3>{90, 3, 7}, {-95, 11, 2}, {5, 97, -13}, {17, -88, 40}})
    points.push_back({2000 + d[0], 2000 + d[1], 2000 + d[2]});
  return points;
}

// The uniform family and four planted acute triangles inscribed in circles
// of radius 5 with integer centres (X, 30000, 30000): their q3 keys have the
// leading coefficient 1 of every q2 key and sort among the q2 keys (by
// centre), so the q2-represented groups are interleaved with the others.
std::vector<gen::Point3> planted_fixture(std::size_t n) {
  auto points = gen::bench::make_front_fixture(n, "uniform", 3).points;
  for (const std::int32_t x : {8000, 24000, 40000, 56000})
    for (const auto& d : {std::array<int, 3>{5, 0, 0}, {-3, 4, 0}, {-3, -4, 0}}) {
      const gen::Point3 p{x + d[0], 30000 + d[1], 30000 + d[2]};
      if (std::none_of(points.begin(), points.end(), [&](const gen::Point3& q) {
            return q.x == p.x && q.y == p.y && q.z == p.z;
          }))
        points.push_back(p);
    }
  return points;
}

ChainOptions base_options(unsigned kmax, std::size_t workers) {
  ChainOptions o;
  o.kmax = kmax;
  o.workers = workers;
  o.keep_catalogue = true;
  o.catalogue_digest = true;
  o.q34_batch_filter = true;
  o.q34_batch_certificates = true;
  o.q2_during_device = true;
  return o;
}

bool same_balls(const std::vector<tower::BallData>& a, const std::vector<tower::BallData>& b) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    const auto& x = a[i];
    const auto& y = b[i];
    if (!(x.key == y.key) || x.level != y.level || x.arity != y.arity || x.n_interior != y.n_interior ||
        x.n_shell != y.n_shell || !std::equal(std::begin(x.interior_ids), std::end(x.interior_ids), std::begin(y.interior_ids)) ||
        !std::equal(std::begin(x.shell_ids), std::end(x.shell_ids), std::begin(y.shell_ids)))
      return false;
  }
  return true;
}

// Everything the census produces and what follows it, lever off vs on;
// tower_work only on the same W (W1 resolves the tower on the temporal
// path, W > 1 on the static one: same tower, different work).
std::string compare(const ChainResult& a, const ChainResult& b, bool same_workers) {
  if (a.status != b.status) return "status";
  if (a.reason != b.reason) return "reason";
  if (a.tower_digest != b.tower_digest || a.catalogue_digest != b.catalogue_digest ||
      a.presentation_digest != b.presentation_digest)
    return "digest";
  const auto& x = a.catalogue;
  const auto& y = b.catalogue;
  if (x.q2_presentations != y.q2_presentations || x.q3_presentations != y.q3_presentations ||
      x.q4_presentations != y.q4_presentations || x.unique_keys != y.unique_keys || x.balls != y.balls ||
      x.extra_shell_balls != y.extra_shell_balls || x.shell_over_cap != y.shell_over_cap ||
      x.max_shell != y.max_shell || x.max_interior != y.max_interior || x.bytes != y.bytes)
    return "catalogue.counts";
  if (x.census_nodes != y.census_nodes || x.census_leaf_tests != y.census_leaf_tests) return "catalogue.census_work";
  if (x.balls_by_qmin != y.balls_by_qmin || x.balls_by_shell != y.balls_by_shell) return "catalogue.histograms";
  if (x.euler_by_k != y.euler_by_k || x.euler_checkable_max_k != y.euler_checkable_max_k ||
      x.euler_status != y.euler_status)
    return "catalogue.euler";
  if (!same_balls(a.catalogue_balls, b.catalogue_balls)) return "catalogue.balls";
  if (a.q2_accepted_pairs != b.q2_accepted_pairs || a.q3_emitted != b.q3_emitted || a.q4_emitted != b.q4_emitted)
    return "generator";
  const auto& s = a.tower_stats;
  const auto& t = b.tower_stats;
  if (same_workers && (s.records != t.records || s.extra_records != t.extra_records || s.representatives != t.representatives ||
      s.anchor_hits != t.anchor_hits || s.key_lookups != t.key_lookups || s.intruder_queries != t.intruder_queries ||
      s.intruder_nodes != t.intruder_nodes || s.births != t.births || s.merges != t.merges ||
      s.contributions != t.contributions || s.grouped_lots != t.grouped_lots))
    return "tower_work";
  if (a.orders.size() != b.orders.size()) return "orders";
  for (std::size_t i = 0; i < a.orders.size(); ++i) {
    const auto& o = a.orders[i];
    const auto& p = b.orders[i];
    if (o.k != p.k || o.nodes != p.nodes || o.births != p.births || o.merges != p.merges || o.parents != p.parents ||
        o.contributions != p.contributions)
      return "orders";
  }
  return {};
}

// Times of an arm: the early part overlaps q34 (out of the stage sum), its
// wait beyond q2's end is in the sum; zero without the lever.
std::string check_times(const ChainResult& r, bool early) {
  const auto& t = r.times;
  if (!early) {
    if (t.q2_census_ms != 0 || t.q2_census_index_ms != 0 || t.q2_census_wait_ms != 0) return "times.off_nonzero";
    return {};
  }
  if (t.q2_census_wait_ms > t.q2_census_ms + 0.05 || t.q2_census_index_ms > t.q2_census_ms + 0.05)
    return "times.wait_beyond_wall";
  if (r.catalogue.early_census_keys > 0 && t.tower_index_ms != 0) return "times.tower_index_twice";
  const double stages = t.prepare_ms + t.gen_index_ms + t.q2_wait_ms + t.q2_census_wait_ms + t.q34_ms + t.merge_ms +
                        t.tower_index_ms + t.census_ms + t.tower_ms;
  if (r.status == ChainStatus::kComplete && stages > t.total_ms + 0.1) return "times.stage_sum";
  return {};
}

struct SelftestFloors {
  unsigned long long cases = 0, early_keys = 0, early_extended = 0, early_over_cap = 0, complete = 0, interleaved = 0;
};

int selftest(std::size_t n) {
  SelftestFloors floors;
  const std::vector<std::string_view> families = {"uniform", "terrain", "clusters", "planted", "spheres", "over_cap"};
  for (const auto family : families) {
    const auto points = family == "spheres"    ? sphere_fixture()
                        : family == "over_cap" ? over_cap_fixture()
                        : family == "planted"  ? planted_fixture(n)
                                               : gen::bench::make_front_fixture(n, family, 3).points;
    for (const unsigned kmax : {1U, 2U, 3U, 5U, 10U}) {
      const std::string at = std::string(family) + "/K" + std::to_string(kmax);
      const auto reference = run_tower_chain(points, base_options(kmax, 1));
      if (family != "over_cap" && reference.status != ChainStatus::kComplete)
        return fail("reference_status " + at + " " + reference.reason);
      for (const std::size_t workers : {std::size_t{1}, std::size_t{3}, std::size_t{8}}) {
        const std::string where = at + "/W" + std::to_string(workers);
        auto off = base_options(kmax, workers);
        auto on = off;
        on.q2_early_census = true;
        const auto a = run_tower_chain(points, off);
        const auto b = run_tower_chain(points, on);
        for (const auto* r : {&a, &b}) {
          const auto diff = compare(reference, *r, workers == 1);
          if (!diff.empty())
            return fail("object." + diff + " " + where + (r == &b ? " on " : " off ") + r->reason);
        }
        if (const auto diff = compare(a, b, true); !diff.empty()) return fail("object." + diff + " " + where + " pair");
        if (const auto t = check_times(a, false); !t.empty()) return fail(t + " " + where);
        if (const auto t = check_times(b, true); !t.empty()) return fail(t + " " + where);
        if (a.catalogue.early_census_keys != 0 || a.catalogue.early_census_extra_shell_balls != 0)
          return fail("off_counts " + where);
        // Every q2 key is censused on the q2 side (K >= 2), and its minimal
        // arity is 2: on a complete chain, the q2 side's keys are by_qmin[2].
        const auto& c = b.catalogue;
        if (kmax < 2 && c.early_census_keys != 0) return fail("k1_early " + where);
        if (kmax >= 2 && b.status == ChainStatus::kComplete &&
            (c.early_census_keys != c.balls_by_qmin[2] || c.early_census_extra_shell_balls > c.extra_shell_balls))
          return fail("early_counts " + where);
        floors.early_keys += c.early_census_keys;
        floors.early_extended += c.early_census_extra_shell_balls;
        if (b.status == ChainStatus::kUnsupportedDegeneracy && b.reason == "chain_shell_above_12" &&
            c.early_census_keys > 0 && c.shell_over_cap > 0)
          ++floors.early_over_cap;
        if (b.status == ChainStatus::kComplete) ++floors.complete;
        // A q3/q4 key before a q2 key in group order: the q2 side's keys are
        // not a prefix of the groups (planted family).
        if (c.early_census_keys > 0) {
          bool other_seen = false;
          for (const auto& ball : b.catalogue_balls) {
            if (ball.arity != 2) other_seen = true;
            else if (other_seen) {
              ++floors.interleaved;
              break;
            }
          }
        }
        ++floors.cases;
      }
    }
  }
  // The lever without q2 during the device calls: an explicit refusal.
  {
    auto o = base_options(5, 2);
    o.q2_during_device = false;
    o.q2_early_census = true;
    const auto r = run_tower_chain(over_cap_fixture(), o);
    if (r.status != ChainStatus::kInvalidInput || r.reason != "chain_q2_early_census_requires_q2_during_device")
      return fail("refusal.requires_q2_during_device " + r.reason);
  }
  std::printf("selftest cases=%llu complete=%llu early_keys=%llu early_extended=%llu early_over_cap=%llu "
              "interleaved=%llu\n",
              floors.cases, floors.complete, floors.early_keys, floors.early_extended, floors.early_over_cap,
              floors.interleaved);
  // Observed on 25 September 2026: 90 cases, 75 complete (the 15 over_cap
  // cases refuse), 644 148 keys, 90 extended shells and 12 shell refusals
  // on the q2 side, 24 interleaved cases.
  if (floors.cases != 90 || floors.complete != 75 || floors.early_keys < 600000 || floors.early_extended < 90 ||
      floors.early_over_cap != 12 || floors.interleaved < 24) {
    std::printf("floor=selftest\n");
    return 3;
  }
  std::printf("chain_q2_early_census_gate selftest n=%zu cases=%llu\n", n, floors.cases);
  return 0;
}

struct Scenario {
  std::vector<Key5> keys;  // census failpoints, reason failpoint_census_<rank in this list>
  bool q2 = false, q34 = false, merge = false;
  std::string expected;
  bool early_hit = false;  // the q2 side must meet a failpoint (lever on)
  bool no_early = false;   // the q2 side must not run (q2 failed)
  std::string name;
};

int priority(std::size_t n) {
  const auto points = planted_fixture(n);
  const auto reference = run_tower_chain(points, base_options(5, 1));
  if (reference.status != ChainStatus::kComplete) return fail("priority.reference " + reference.reason);
  // Keys in group (key) order: q2 keys have small leading coefficients and
  // come mostly first; picks at the boundaries between a q2 key and a q3/q4
  // key, in both directions, nearest to the middle of the catalogue.
  const auto& balls = reference.catalogue_balls;
  std::vector<std::size_t> up, down;  // i: (q2, other) at (i, i + 1), resp. (other, q2)
  std::size_t twos = 0;
  for (std::size_t i = 0; i < balls.size(); ++i) {
    twos += balls[i].arity == 2 ? 1 : 0;
    if (i + 1 == balls.size()) break;
    if (balls[i].arity == 2 && balls[i + 1].arity != 2) up.push_back(i);
    if (balls[i].arity != 2 && balls[i + 1].arity == 2) down.push_back(i);
  }
  std::printf("priority keys=%zu q2_keys=%zu boundaries=%zu/%zu\n", balls.size(), twos, up.size(), down.size());
  if (twos < 100 || balls.size() - twos < 100 || up.size() < 4 || down.size() < 4) {
    std::printf("floor=priority.keys\n");
    return 3;
  }
  const std::size_t i1 = up[up.size() / 2], i2 = down[(3 * down.size()) / 4];
  const Key5 a1 = key5(balls[i1].key), b_after = key5(balls[i1 + 1].key);
  const Key5 b_before = key5(balls[i2].key), a2 = key5(balls[i2 + 1].key);
  std::size_t j1 = balls.size(), j2 = 0;  // two other late keys, apart
  for (std::size_t i = balls.size() / 4; i < balls.size() && j1 == balls.size(); ++i)
    if (balls[i].arity != 2) j1 = i;
  for (std::size_t i = balls.size(); i-- > j1 + 1 && j2 == 0;)
    if (balls[i].arity != 2) j2 = i;
  if (j1 == balls.size() || j2 == 0) return fail("priority.picks_late");
  const Key5 b1 = key5(balls[j1].key), b2 = key5(balls[j2].key);
  if (!(a1 < b_after) || !(b_before < a2) || !(a1 < a2) || !(b1 < b2)) return fail("priority.picks");
  // Stage priorities first, then the key order between census failures
  // (each compiled mutant is killed on its own dimension).
  const std::vector<Scenario> scenarios = {
      {{a1}, false, false, false, "failpoint_census_0", true, false, "q2_key"},
      {{b1}, false, false, false, "failpoint_census_0", false, false, "late_key"},
      {{a1}, false, false, true, "failpoint_merge", true, false, "merge_before_census"},
      {{a1, b1}, false, false, true, "failpoint_merge", true, false, "merge_before_both"},
      {{a1}, false, true, false, "failpoint_q34", false, false, "q34_before_census"},
      {{a1}, true, false, false, "failpoint_q2", false, true, "q2_before_census"},
      {{a1}, true, true, false, "failpoint_q2", false, true, "q2_before_q34"},
      {{b_after, a1}, false, false, false, "failpoint_census_1", true, false, "q2_before_late"},
      {{a2, b_before}, false, false, false, "failpoint_census_1", true, false, "late_before_q2"},
      {{a2, a1}, false, false, false, "failpoint_census_1", true, false, "two_q2_keys"},
      {{b2, b1}, false, false, false, "failpoint_census_1", false, false, "two_late_keys"},
  };
  unsigned long long runs = 0, early_hits = 0, early_scenarios = 0;
  for (const auto& s : scenarios) {
    for (const std::size_t workers : {std::size_t{1}, std::size_t{4}, std::size_t{8}}) {
      for (const bool lever : {false, true}) {
        const std::string where = s.name + "/W" + std::to_string(workers) + (lever ? "/on" : "/off");
        auto o = base_options(5, workers);
        o.q2_early_census = lever;
        chain_testing::set_census_failpoints(s.keys);
        chain_testing::set_stage_failpoints(s.q2, s.q34, s.merge);
        const auto r = run_tower_chain(points, o);
        const auto hits = chain_testing::early_census_failpoint_hits();
        chain_testing::set_census_failpoints({});
        chain_testing::set_stage_failpoints(false, false, false);
        if (r.status != ChainStatus::kInvariantViolated || r.reason != s.expected)
          return fail("priority " + where + " reason=" + r.reason);
        if (r.tower_digest != 0 || r.catalogue_digest != 0 || !r.catalogue_balls.empty())
          return fail("priority.published " + where);
        if (!lever && hits != 0) return fail("priority.off_hit " + where);
        if (lever && s.no_early && (hits != 0 || r.times.q2_census_ms != 0)) return fail("priority.early_ran " + where);
        if (lever && s.early_hit) {
          if (hits == 0) return fail("priority.early_missed " + where);
          ++early_scenarios;
        }
        early_hits += hits;
        ++runs;
      }
    }
  }
  std::printf("priority runs=%llu early_scenarios=%llu early_hits=%llu\n", runs, early_scenarios, early_hits);
  if (runs != 66 || early_scenarios != 18 || early_hits < 18) {
    std::printf("floor=priority\n");
    return 3;
  }
  std::printf("chain_q2_early_census_gate priority n=%zu runs=%llu\n", n, runs);
  return 0;
}

// The q2 side's fallback: a failure outside its per-key census (after the
// index, or after the census of all its keys) leaves `ready` unset, and the
// chain rebuilds the index and runs the whole census after the merge; a q34
// failure cancels the q2 side (held until the cancel, so that it is seen).
int fallback(std::size_t n) {
  unsigned long long runs = 0, fallbacks = 0, thrown_keys = 0, cancels = 0;
  const auto run = [&](const std::vector<gen::Point3>& points, const ChainOptions& o, int early_throw, bool hold,
                       std::vector<Key5> keys, bool q34, chain_testing::EarlyFailpointCounts& counts,
                       std::uint64_t& hits) {
    chain_testing::set_census_failpoints(std::move(keys));
    chain_testing::set_stage_failpoints(false, q34, false);
    chain_testing::set_early_failpoints(early_throw, hold);
    auto r = run_tower_chain(points, o);
    counts = chain_testing::early_failpoint_counts();
    hits = chain_testing::early_census_failpoint_hits();
    chain_testing::set_census_failpoints({});
    chain_testing::set_stage_failpoints(false, false, false);
    chain_testing::set_early_failpoints(chain_testing::kEarlyNoThrow, false);
    ++runs;
    return r;
  };
  // A fallback arm: same object as the lever off at the same W, nothing
  // taken from the q2 side, the index rebuilt after the merge.
  const auto check = [](const ChainResult& off, const ChainResult& on,
                        const chain_testing::EarlyFailpointCounts& c) -> std::string {
    if (const auto diff = compare(off, on, true); !diff.empty()) return "object." + diff;
    if (on.catalogue.early_census_keys != 0 || on.catalogue.early_census_extra_shell_balls != 0) return "early_keys";
    if (c.fallbacks != 1 || c.cancels != 0 || c.hold_timeouts != 0) return "counts";
    if (on.times.q2_census_ms <= 0 || on.times.tower_index_ms <= 0) return "times";
    if (const auto t = check_times(on, true); !t.empty()) return t;
    return {};
  };
  const std::vector<std::pair<int, std::string_view>> stages = {{chain_testing::kEarlyAfterCensus, "after_census"},
                                                                {chain_testing::kEarlyAfterIndex, "after_index"}};
  chain_testing::EarlyFailpointCounts counts;
  std::uint64_t hits = 0;
  for (const std::string_view family : {"uniform", "planted", "spheres", "over_cap"}) {
    const auto points = family == "spheres"    ? sphere_fixture()
                        : family == "over_cap" ? over_cap_fixture()
                        : family == "planted"  ? planted_fixture(n)
                                               : gen::bench::make_front_fixture(n, family, 3).points;
    for (const unsigned kmax : {2U, 5U}) {
      for (const std::size_t workers : {std::size_t{1}, std::size_t{8}}) {
        const std::string at = std::string(family) + "/K" + std::to_string(kmax) + "/W" + std::to_string(workers);
        const auto off = run(points, base_options(kmax, workers), chain_testing::kEarlyNoThrow, false, {}, false,
                             counts, hits);
        if (family != "over_cap" && off.status != ChainStatus::kComplete) return fail("fallback.reference " + at);
        if (counts.fallbacks != 0) return fail("fallback.off_counts " + at);
        auto on_options = base_options(kmax, workers);
        on_options.q2_early_census = true;
        for (const auto& [early_throw, stage] : stages) {
          const std::string where = at + "/" + std::string(stage);
          const auto on = run(points, on_options, early_throw, false, {}, false, counts, hits);
          if (const auto c = check(off, on, counts); !c.empty()) return fail("fallback." + c + " " + where);
          ++fallbacks;
          thrown_keys += counts.thrown_keys;
        }
      }
    }
  }
  // A census failure met by the full census after the fallback: the same
  // reason as without the lever, whether or not the q2 side met it first.
  const auto points = planted_fixture(n);
  const auto reference = run_tower_chain(points, base_options(5, 1));
  if (reference.status != ChainStatus::kComplete) return fail("fallback.planted_reference " + reference.reason);
  const auto& balls = reference.catalogue_balls;
  std::size_t pick = balls.size();  // the last q2 key of the first half (q2 keys come mostly first)
  for (std::size_t i = balls.size() / 2; i-- > 0 && pick == balls.size();)
    if (balls[i].arity == 2) pick = i;
  if (pick == balls.size()) return fail("fallback.pick");
  const Key5 a1 = key5(balls[pick].key);
  for (const std::size_t workers : {std::size_t{1}, std::size_t{8}}) {
    const std::string at = "census_failure/W" + std::to_string(workers);
    const auto off = run(points, base_options(5, workers), chain_testing::kEarlyNoThrow, false, {a1}, false, counts,
                         hits);
    if (off.status != ChainStatus::kInvariantViolated || off.reason != "failpoint_census_0")
      return fail("fallback.census_off " + at + " " + off.reason);
    auto on_options = base_options(5, workers);
    on_options.q2_early_census = true;
    for (const auto& [early_throw, stage] : stages) {
      const std::string where = at + "/" + std::string(stage);
      const auto on = run(points, on_options, early_throw, false, {a1}, false, counts, hits);
      if (on.status != off.status || on.reason != off.reason) return fail("fallback.census " + where + " " + on.reason);
      if (on.tower_digest != 0 || on.catalogue_digest != 0 || !on.catalogue_balls.empty())
        return fail("fallback.census_published " + where);
      // After the index, the q2 side never reaches its census; after the
      // census, it met the failpoint once, then the full census again.
      const std::uint64_t early_hits = early_throw == chain_testing::kEarlyAfterIndex ? 0 : 1;
      if (counts.fallbacks != 1 || counts.cancels != 0 || hits != early_hits)
        return fail("fallback.census_counts " + where);
      ++fallbacks;
    }
  }
  // q34 fails: the q2 side, held until then, sees the cancel, censuses no
  // key (a failpoint on a q2 key stays unmet) and ends without results.
  for (const std::size_t workers : {std::size_t{1}, std::size_t{4}, std::size_t{8}}) {
    for (const bool lever : {false, true}) {
      const std::string where = "cancel/W" + std::to_string(workers) + (lever ? "/on" : "/off");
      auto o = base_options(5, workers);
      o.q2_early_census = lever;
      const auto r = run(points, o, chain_testing::kEarlyNoThrow, true, {a1}, true, counts, hits);
      if (r.status != ChainStatus::kInvariantViolated || r.reason != "failpoint_q34")
        return fail("fallback.cancel_reason " + where + " " + r.reason);
      if (r.tower_digest != 0 || r.catalogue_digest != 0 || !r.catalogue_balls.empty())
        return fail("fallback.cancel_published " + where);
      const std::uint64_t expected = lever ? 1 : 0;
      if (counts.fallbacks != expected || counts.cancels != expected || counts.hold_timeouts != 0 || hits != 0)
        return fail("fallback.cancel " + where);
      cancels += counts.cancels;
    }
  }
  std::printf("fallback runs=%llu fallbacks=%llu thrown_keys=%llu cancels=%llu\n", runs, fallbacks, thrown_keys,
              cancels);
  // 4 families x 2 K x 2 W (one reference, two stages), 2 W x 3 census
  // failure runs, 3 W x 2 cancel runs.
  if (runs != 60 || fallbacks != 36 || cancels != 3 || thrown_keys < 90000) {
    std::printf("floor=fallback\n");
    return 3;
  }
  std::printf("chain_q2_early_census_gate fallback n=%zu runs=%llu\n", n, runs);
  return 0;
}

std::vector<gen::Point3> read_u32le(const std::string& path, bool& ok) {
  ok = false;
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (bytes.empty() || bytes.size() % 12 != 0) return {};
  std::vector<gen::Point3> points(bytes.size() / 12);
  for (std::size_t i = 0; i < points.size(); ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) {
      std::uint32_t v = 0;
      for (int b = 0; b < 4; ++b) v |= static_cast<std::uint32_t>(bytes[12 * i + 4 * a + b]) << (8 * b);
      if (v > 262143u) return {};
      c[a] = v;
    }
    points[i] = {static_cast<gen::Coordinate>(c[0]), static_cast<gen::Coordinate>(c[1]),
                 static_cast<gen::Coordinate>(c[2])};
  }
  ok = true;
  return points;
}

// Peak resident set of the process (kB), for the one-arm measurement.
long peak_rss_kb() {
  rusage usage{};
  return getrusage(RUSAGE_SELF, &usage) == 0 ? usage.ru_maxrss : -1;
}

// arm < 0: `pairs` interleaved pairs (off, on), objects compared; arm 0 or
// 1: that arm only, `pairs` runs (one process per arm for the A/B series).
int frame(const std::string& path, unsigned kmax, std::uint64_t tower_pin, std::uint64_t catalogue_pin,
          unsigned pairs, std::size_t workers, int arm) {
  bool ok = false;
  const auto points = read_u32le(path, ok);
  if (!ok) {
    std::fprintf(stderr, "frame refusal: %s\n", path.c_str());
    return 2;
  }
  ChainResult first_off, first_on;
  for (unsigned p = 0; p < pairs; ++p) {
    for (const bool lever : {false, true}) {
      if (arm >= 0 && lever != (arm == 1)) continue;
      auto o = base_options(kmax, workers);
      o.keep_catalogue = false;
      o.q2_early_census = lever;
      auto r = run_tower_chain(points, o);
      const auto& t = r.times;
      const double after_q34 = t.q2_wait_ms + t.q2_census_wait_ms + t.merge_ms + t.tower_index_ms + t.census_ms;
      std::printf("frame pair=%u lever=%d W=%zu status=%s total=%.1f q2=%.1f q2_wait=%.1f q2_census=%.1f "
                  "q2_census_index=%.1f q2_census_wait=%.1f q34=%.1f merge=%.1f tower_index=%.1f census=%.1f "
                  "tower=%.1f after_q34=%.1f cpu_s=%.2f early_keys=%llu peak_rss_kb=%ld\n",
                  p, lever ? 1 : 0, workers, chain_status_name(r.status), t.total_ms, t.q2_ms, t.q2_wait_ms,
                  t.q2_census_ms, t.q2_census_index_ms, t.q2_census_wait_ms, t.q34_ms, t.merge_ms, t.tower_index_ms,
                  t.census_ms, t.tower_ms, after_q34, t.cpu_s,
                  static_cast<unsigned long long>(r.catalogue.early_census_keys), peak_rss_kb());
      std::fflush(stdout);
      if (r.status != ChainStatus::kComplete) return fail("frame.status " + r.reason);
      if (r.tower_digest != tower_pin || r.catalogue_digest != catalogue_pin) return fail("frame.pinned_digests");
      if (const auto c = check_times(r, lever); !c.empty()) return fail("frame." + c);
      if (lever && (r.catalogue.early_census_keys == 0 ||
                    r.catalogue.early_census_keys != r.catalogue.balls_by_qmin[2]))
        return fail("frame.early_keys");
      if (p == 0) (lever ? first_on : first_off) = std::move(r);
    }
  }
  if (arm < 0) {
    if (const auto diff = compare(first_off, first_on, true); !diff.empty()) return fail("frame.object." + diff);
  }
  std::printf("chain_q2_early_census_gate frame n=%zu K=%u pairs=%u arm=%d\n", points.size(), kmax, pairs, arm);
  return 0;
}

bool parse_size(std::string_view s, std::size_t& out) {
  const auto [end, error] = std::from_chars(s.data(), s.data() + s.size(), out);
  return !s.empty() && error == std::errc{} && end == s.data() + s.size();
}

bool parse_hex(std::string_view s, std::uint64_t& out) {
  if (s.empty() || s.size() > 16) return false;
  out = 0;
  for (const char ch : s) {
    const int d = ch >= '0' && ch <= '9' ? ch - '0' : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 : -1;
    if (d < 0) return false;
    out = out * 16 + static_cast<std::uint64_t>(d);
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  std::string mode, path;
  std::size_t n = 1000, kmax = 5, pairs = 1, workers = 8;
  int arm = -1;
  std::uint64_t tower_pin = 0, catalogue_pin = 0;
  bool have_tower = false, have_catalogue = false;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg == "--selftest" || arg == "--priority" || arg == "--fallback") {
      mode = std::string(arg);
    } else if (arg.starts_with("--frame=")) {
      mode = "--frame";
      path = std::string(arg.substr(8));
    } else if (arg.starts_with("--n=")) {
      if (!parse_size(arg.substr(4), n)) return 2;
    } else if (arg.starts_with("--k=")) {
      if (!parse_size(arg.substr(4), kmax)) return 2;
    } else if (arg.starts_with("--pairs=")) {
      if (!parse_size(arg.substr(8), pairs)) return 2;
    } else if (arg == "--lever=0" || arg == "--lever=1") {
      arm = arg.back() == '1' ? 1 : 0;
    } else if (arg.starts_with("--workers=")) {
      if (!parse_size(arg.substr(10), workers)) return 2;
    } else if (arg.starts_with("--tower=")) {
      have_tower = parse_hex(arg.substr(8), tower_pin);
      if (!have_tower) return 2;
    } else if (arg.starts_with("--catalogue=")) {
      have_catalogue = parse_hex(arg.substr(12), catalogue_pin);
      if (!have_catalogue) return 2;
    } else {
      std::fprintf(stderr,
                   "usage: mhgp9_chain_q2_early_census_gate --selftest|--priority|--fallback [--n=1000] | "
                   "--frame=PATH --k=K --tower=HEX --catalogue=HEX [--pairs=1] [--workers=8] [--lever=0|1]\n");
      return 2;
    }
  }
  if (n < 64 || n > 65536) return 2;
  if (mode == "--selftest") return selftest(n);
  if (mode == "--priority") return priority(n);
  if (mode == "--fallback") return fallback(n);
  if (mode == "--frame") {
    if (!have_tower || !have_catalogue || kmax < 1 || kmax > 10 || pairs < 1 || pairs > 20 || workers < 1 ||
        workers > 256)
      return 2;
    return frame(path, static_cast<unsigned>(kmax), tower_pin, catalogue_pin, static_cast<unsigned>(pairs), workers,
                 arm);
  }
  std::fprintf(stderr, "usage: mhgp9_chain_q2_early_census_gate --selftest|--priority|--fallback|--frame=PATH ...\n");
  return 2;
}
