// MorseHGP3D v9 — porte du catalogue scelle et de la positivite des supports
// reguliers cote chaine (R-29, auditeur C,
// audits/PROPOSITION_C_CATALOGUE_SCELLE_20260924.md).
//
// Cible de test : tower_chain.cpp recompile avec MHGP9_CHAIN_TEST_SEAM
// (presentations forgees a la fusion, alterations du catalogue entre la
// chaine et la tour). Sections, dans cet ordre :
//   0. le type du sceau : ni construit hors de ChainCatalogueSealer, ni
//      copie, ni deplace (static_assert) ;
//   1. refus directs de la tour, API publique (validation complete) : un
//      triangle obtus (q3) et un tetraedre a centre exterieur (q4), boules
//      regulieres a cle, niveau et recensement exacts, ajoutes au catalogue
//      garde d'une chaine saine, sont refuses full_ball_census_geometry, 0 et
//      4 fils ; le catalogue garde seul rend la tour de la chaine ;
//   2. presentations forgees dans la chaine (meme triangle a la profondeur
//      Kmax-2, angle mort d'Euler ; meme tetraedre) : refus
//      chain_nonpositive_regular_support, sous le sceau puis sans, 1 et 4
//      fils ; arite 5, arite 1 et ID hors du nuage : refus types ;
//   3. fautes de plomberie sur le nuage de la queue (K5, 4 fils) : le premier
//      interieur de chaque boule reguliere remplace (faute systematique) est
//      refuse par l'echantillon de la passe 1 sous le sceau
//      (tower: full_ball_census_power), comme sans sceau ; une seule boule
//      reguliere alteree hors de l'echantillon est refusee sans sceau et
//      publiee sous le sceau (residu declare, ligne residual=) ;
//   4. egalite scelle / non scelle : memes condenses (tour, catalogue,
//      presentations), meme tower_work, memes ordres, sur le nuage de la
//      queue (1 500 sites, six carres plantes) a K3, K5 et K10 et sur la
//      fixture a K5, 1 et 4 fils, et sur la voie CPU par lots avec voies
//      q3/q4 a K5 ; planchers : sceau pris (sealed_catalogues, echantillon
//      = ceil(n/64)), supports reguliers certifies q3 et q4 (somme = boules
//      regulieres), coquilles etendues, controles de la passe 1 reduits.
//
//   mhgp9_chain_sealed_catalogue_gate --selftest
//
// Code 0 conforme, 1 desaccord (ligne cause=), 2 argument, 3 plancher ou
// fixture. Mutants compiles (code 1 attendu) :
// MHGP9_TOWER_MUTANT_SUPPORT_NO_POSITIVITY (section 1),
// MHGP9_CHAIN_MUTANT_NO_POSITIVITY (section 2 : sous le sceau, le triangle
// obtus est publie ou pris par l'echantillon, jamais refuse par la chaine),
// MHGP9_TOWER_MUTANT_SEAL_NO_SAMPLE (section 3).
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../../src/chain/tower_chain.hpp"

#if !defined(MHGP9_CHAIN_TEST_SEAM)
#error "sealed_catalogue_gate needs MHGP9_CHAIN_TEST_SEAM (tower_chain.cpp recompiled in the test target)"
#endif

namespace {
namespace tw = mhgp9::tower;
using Point = mhgp9::gen::Point3;
using u64 = std::uint64_t;

// Section 0: a seal is issued by the chain alone, and never duplicated.
static_assert(!std::is_default_constructible_v<tw::SealedCatalogue>);
static_assert(!std::is_constructible_v<tw::SealedCatalogue, const tw::CloudIndex&, std::vector<tw::BallData>&&>);
static_assert(!std::is_copy_constructible_v<tw::SealedCatalogue>);
static_assert(!std::is_move_constructible_v<tw::SealedCatalogue>);

struct Floors {
  u64 direct_refusals = 0, forged_refusals = 0, typed_refusals = 0, fault_refusals = 0;
  u64 sealed_runs = 0, unsealed_runs = 0, sampled = 0, q3_regular = 0, q4_regular = 0, extended = 0;
  u64 lanes_sealed_runs = 0, reduced_pass1 = 0;
} floors;

bool fail(const std::string& cause) {
  std::printf("cause=%s\n", cause.c_str());
  return false;
}

tw::P3 p3(const Point& p) { return {p.x, p.y, p.z}; }

struct Fixture {
  std::vector<Point> points;
  std::array<std::uint32_t, 4> q3{};  // obtuse triangle, three sites strictly inside its ball
  std::array<std::uint32_t, 4> q4{};  // tetrahedron whose circumcentre lies outside it
};

// A far cluster (300 LCG sites), an obtuse triangle (angle at the third
// site) whose q3 ball holds exactly Kmax-2 = 3 sites, and a flat
// tetrahedron whose circumcentre lies below its base plane.
Fixture fixture() {
  Fixture f;
  std::uint64_t state = 7;
  for (int i = 0; i < 300; ++i) {
    std::int32_t c[3];
    for (int axis = 0; axis < 3; ++axis) {
      state = state * 6364136223846793005ull + 1442695040888963407ull;
      c[axis] = 60000 + static_cast<std::int32_t>((state >> 40) % 20000);
    }
    f.points.push_back({c[0], c[1], c[2]});
  }
  const auto add = [&f](std::int32_t x, std::int32_t y, std::int32_t z) {
    f.points.push_back({x, y, z});
    return static_cast<std::uint32_t>(f.points.size() - 1);
  };
  const auto a = add(1000, 5000, 1000), b = add(3000, 5000, 1000), d = add(2000, 5200, 1000);
  f.q3 = {a, b, d, 0};
  add(2000, 2600, 1000);
  add(2300, 2500, 1100);
  add(1800, 2700, 900);
  const auto e = add(1000, 1000, 20000), g = add(3000, 1000, 20000), h = add(1100, 3000, 20000),
             i = add(1500, 1500, 20100);
  f.q4 = {e, g, h, i};
  return f;
}

// The tower tail gate's cloud: 1 500 sites in three u18 clusters and six
// planted squares (extended shells, q_min 2, four sites).
std::vector<Point> tail_cloud() {
  std::vector<Point> points;
  std::uint64_t state = 3;
  for (int i = 0; i < 1500; ++i) {
    const std::int32_t centre = (i % 3) * 40000 + 20000;
    std::int32_t c[3];
    for (int axis = 0; axis < 3; ++axis) {
      state = state * 6364136223846793005ull + 1442695040888963407ull;
      c[axis] = centre + static_cast<std::int32_t>((state >> 40) % 9000);
    }
    points.push_back({c[0], c[1], c[2]});
  }
  for (int s = 0; s < 6; ++s) {
    const std::int32_t x = 150000 + 5000 * s, y = 150000 + 3000 * s, z = 200000;
    for (const auto& [dx, dy] : {std::pair{0, 0}, std::pair{2, 0}, std::pair{2, 2}, std::pair{0, 2}})
      points.push_back({x + dx, y + dy, z});
  }
  return points;
}

tw::CloudIndex index_of(const std::vector<Point>& points) {
  std::vector<tw::InputPoint> input(points.size());
  for (std::size_t i = 0; i < points.size(); ++i) input[i] = tw::InputPoint{static_cast<tw::PointId>(i), p3(points[i])};
  return tw::build_cloud_index(input);
}

// The regular ball of a support (key and level from the tower's formulas,
// census on the index), whatever its positivity.
tw::BallData ball_of(const tw::CloudIndex& ix, const std::vector<Point>& points, const std::array<std::uint32_t, 4>& s,
                     unsigned arity) {
  tw::BallData b;
  const tw::P3 a = p3(points[s[0]]), c = p3(points[s[1]]), d = p3(points[s[2]]);
  if (arity == 3) {
    b.key = tw::q3_ball_key(tw::q3_form(a, c, d));
    b.level = tw::promote_level(tw::q3_exact_level(a, c, d));
  } else {
    const auto form = tw::q4_form(a, c, d, p3(points[s[3]]));
    b.key = tw::ball_key_reduce(tw::q4_ball_form(form));
    b.level = tw::q4_level_raw(form);
  }
  std::vector<tw::i32> in, sh;
  tw::ball_census(ix, b.key, std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max(), &in,
                  &sh);
  std::sort(in.begin(), in.end());
  std::sort(sh.begin(), sh.end());
  b.arity = static_cast<tw::u8>(arity);
  b.n_interior = static_cast<tw::u8>(std::min<std::size_t>(in.size(), tw::kBallInteriorMax));
  b.n_shell = static_cast<tw::u8>(std::min<std::size_t>(sh.size(), tw::kBallShellMax));
  std::copy(in.begin(), in.begin() + b.n_interior, b.interior_ids);
  std::copy(sh.begin(), sh.begin() + b.n_shell, b.shell_ids);
  return b;
}

// Fixture validity: the triangle is obtuse, the tetrahedron's centre lies
// outside it, both balls are regular, the triangle's depth is Kmax-2 (Euler's
// blind spot) and the tetrahedron's within the rank window.
bool check_fixture(const Fixture& f, const tw::CloudIndex& ix, unsigned kmax) {
  const auto t = ball_of(ix, f.points, f.q3, 3);
  const tw::P3 a = p3(f.points[f.q3[0]]), b = p3(f.points[f.q3[1]]), d = p3(f.points[f.q3[2]]);
  const bool obtuse = tw::p3_dot(tw::p3_sub(a, d), tw::p3_sub(b, d)) < 0;
  if (!obtuse || t.n_shell != 3 || static_cast<unsigned>(t.n_interior) != kmax - 2) return fail("fixture.q3");
  const auto q = ball_of(ix, f.points, f.q4, 4);
  const tw::P3 e = p3(f.points[f.q4[0]]), g = p3(f.points[f.q4[1]]), h = p3(f.points[f.q4[2]]),
               i = p3(f.points[f.q4[3]]);
  const auto form = tw::q4_form(e, g, h, i);
  if (!(form.det > 0) || tw::q4_center_strictly_inside(form, e, g, h, i) || q.n_shell != 4 ||
      static_cast<unsigned>(q.n_interior) + 4 > kmax + 1)
    return fail("fixture.q4");
  return true;
}

mhgp9::ChainOptions options_for(unsigned kmax, std::size_t workers, bool sealed) {
  mhgp9::ChainOptions o;
  o.kmax = kmax;
  o.workers = workers;
  o.tower_sealed_catalogue = sealed;
  o.catalogue_digest = true;
  return o;
}

std::string outcome(const mhgp9::ChainResult& r) {
  return std::string("status=") + mhgp9::chain_status_name(r.status) + " reason=" + r.reason;
}

// Section 1.
bool direct_tower(const Fixture& f, const tw::CloudIndex& ix) {
  auto o = options_for(5, 1, false);
  o.keep_catalogue = true;
  const auto chain = mhgp9::run_tower_chain(f.points, o);
  if (chain.status != mhgp9::ChainStatus::kComplete) return fail("tower_direct.control_chain " + outcome(chain));
  const auto control = tw::build_full_ball_tower(ix, chain.catalogue_balls, 5, 0);
  if (control.status != tw::FullBallStatus::kCompleteRelative || mhgp9::tower_digest(control) != chain.tower_digest)
    return fail("tower_direct.control");
  const std::pair<const char*, unsigned> cases[] = {{"q3", 3}, {"q4", 4}};
  for (const auto& [name, arity] : cases) {
    auto balls = chain.catalogue_balls;
    balls.push_back(ball_of(ix, f.points, arity == 3 ? f.q3 : f.q4, arity));
    for (int threads : {0, 4}) {
      const auto t = tw::build_full_ball_tower(ix, balls, 5, threads);
      if (t.status != tw::FullBallStatus::kInvalidInput || std::string_view(t.reason) != "full_ball_census_geometry")
        return fail(std::string("tower_direct.") + name + " threads=" + std::to_string(threads) + " reason=" + t.reason);
      ++floors.direct_refusals;
    }
  }
  return true;
}

// Section 2.
bool forged(const Fixture& f) {
  const std::pair<const char*, unsigned> cases[] = {{"q3", 3}, {"q4", 4}};
  for (const auto& [name, arity] : cases)
    for (bool sealed : {true, false})
      for (std::size_t workers : {1u, 4u}) {
        mhgp9::chain_test::seam.forged = {{arity, arity == 3 ? f.q3 : f.q4}};
        const auto r = mhgp9::run_tower_chain(f.points, options_for(5, workers, sealed));
        mhgp9::chain_test::seam.forged.clear();
        if (r.status != mhgp9::ChainStatus::kInvariantViolated || r.reason != "chain_nonpositive_regular_support")
          return fail(std::string("forged.") + name + " sealed=" + (sealed ? "1" : "0") + " W=" +
                      std::to_string(workers) + " " + outcome(r));
        ++floors.forged_refusals;
      }
  const auto n = static_cast<std::uint32_t>(f.points.size());
  const std::pair<mhgp9::chain_test::ForgedSupport, const char*> typed[] = {
      {{5, {0, 1, 2, 3}}, "chain_presentation_arity"},
      {{1, {0, 0, 0, 0}}, "chain_presentation_arity"},
      {{3, {0, 1, n + 5, 0}}, "chain_support_id_outside_cloud"}};
  for (const auto& [support, reason] : typed) {
    mhgp9::chain_test::seam.forged = {support};
    const auto r = mhgp9::run_tower_chain(f.points, options_for(5, 4, true));
    mhgp9::chain_test::seam.forged.clear();
    if (r.status != mhgp9::ChainStatus::kInvariantViolated || r.reason != reason)
      return fail("forged.typed arity=" + std::to_string(support.arity) + " " + outcome(r));
    ++floors.typed_refusals;
  }
  return true;
}

// Section 3.
bool faults(const std::vector<Point>& points) {
  auto clean_options = options_for(5, 4, false);
  clean_options.keep_catalogue = true;
  const auto clean = mhgp9::run_tower_chain(points, clean_options);
  if (clean.status != mhgp9::ChainStatus::kComplete) return fail("fault.control " + outcome(clean));
  // The single fault: the first regular ball with interiors outside the sample.
  std::size_t single = clean.catalogue_balls.size();
  for (std::size_t j = 0; j < clean.catalogue_balls.size() && single == clean.catalogue_balls.size(); ++j) {
    const auto& b = clean.catalogue_balls[j];
    if (j % tw::kSealSampleStride != 0 && b.n_interior > 0 && b.n_shell == b.arity) single = j;
  }
  if (single == clean.catalogue_balls.size()) return fail("fault.no_single_candidate");
  auto& seam = mhgp9::chain_test::seam;
  seam.fault = mhgp9::chain_test::CatalogueFault::kSystematicInterior;
  for (bool sealed : {true, false}) {
    const auto r = mhgp9::run_tower_chain(points, options_for(5, 4, sealed));
    if (r.status != mhgp9::ChainStatus::kInvalidInput || r.reason != "tower: full_ball_census_power") {
      seam.fault = mhgp9::chain_test::CatalogueFault::kNone;
      return fail(std::string("fault.systematic sealed=") + (sealed ? "1" : "0") + " W=4 " + outcome(r));
    }
    ++floors.fault_refusals;
  }
  seam.fault = mhgp9::chain_test::CatalogueFault::kSingleInterior;
  seam.fault_ball = single;
  const auto unsealed = mhgp9::run_tower_chain(points, options_for(5, 4, false));
  const auto sealed = mhgp9::run_tower_chain(points, options_for(5, 4, true));
  seam.fault = mhgp9::chain_test::CatalogueFault::kNone;
  if (unsealed.status != mhgp9::ChainStatus::kInvalidInput || unsealed.reason != "tower: full_ball_census_power")
    return fail("fault.single sealed=0 W=4 " + outcome(unsealed));
  ++floors.fault_refusals;
  // Declared residual (R-29): an isolated fault outside the sample of a
  // regular ball is not caught under the seal.
  std::printf("residual=single_fault_sealed ball=%zu %s digest_differs=%d\n", single, outcome(sealed).c_str(),
              sealed.status == mhgp9::ChainStatus::kComplete && sealed.tower_digest != clean.tower_digest ? 1 : 0);
  return true;
}

bool same_work(const tw::FullBallStats& x, const tw::FullBallStats& y) {
  // The probe's tower_work fields (tower_tail_gate.cpp).
  return x.records == y.records && x.extra_records == y.extra_records && x.representatives == y.representatives &&
         x.anchor_hits == y.anchor_hits && x.key_lookups == y.key_lookups &&
         x.intruder_queries == y.intruder_queries && x.intruder_nodes == y.intruder_nodes &&
         x.resolve_work.calls == y.resolve_work.calls && x.resolve_work.power_tests == y.resolve_work.power_tests &&
         x.births == y.births && x.merges == y.merges && x.contributions == y.contributions &&
         x.grouped_lots == y.grouped_lots && x.resolver_cache_hits == y.resolver_cache_hits &&
         x.resolve_work.pair_distances == y.resolve_work.pair_distances &&
         x.resolve_work.materializations == y.resolve_work.materializations &&
         x.resolve_work.supports_by_size == y.resolve_work.supports_by_size &&
         x.resolve_work.proposals == y.resolve_work.proposals &&
         x.resolve_work.verified_proposals == y.resolve_work.verified_proposals;
}

bool same_orders(const mhgp9::ChainResult& x, const mhgp9::ChainResult& y) {
  if (x.orders.size() != y.orders.size()) return false;
  for (std::size_t k = 0; k < x.orders.size(); ++k) {
    const auto &a = x.orders[k], &b = y.orders[k];
    if (a.k != b.k || a.nodes != b.nodes || a.births != b.births || a.merges != b.merges ||
        a.contributions != b.contributions || a.parents != b.parents)
      return false;
  }
  return true;
}

// Section 4: one cloud at one K, sealed and unsealed at 1 and 4 workers
// (temporal and static resolution paths): digests, orders and catalogue
// against the unsealed run at 1 worker, tower_work against the unsealed
// twin of the same workers; the CPU lanes path against the engine path's
// digests (`engine`, when given).
bool equality(const std::vector<Point>& points, const char* name, unsigned kmax, bool lanes,
              std::array<u64, 3>* engine) {
  const mhgp9::ChainResult* reference = nullptr;
  std::vector<mhgp9::ChainResult> runs;
  runs.reserve(4);
  for (std::size_t workers : {1u, 4u})
    for (bool sealed : {false, true}) {
      auto o = options_for(kmax, workers, sealed);
      if (lanes) {
        o.q34_batch_filter = o.q34_batch_certificates = o.q34_batch_q3 = o.q34_batch_q4 = true;
      }
      runs.push_back(mhgp9::run_tower_chain(points, o));
      const auto& r = runs.back();
      const std::string where = std::string(name) + " K" + std::to_string(kmax) + " W" + std::to_string(workers) +
                                " sealed=" + (sealed ? "1" : "0") + (lanes ? " lanes" : "");
      const char* expected = sealed ? "complete_relative_to_cross_checked_catalogue_sealed_in_process_census"
                                    : "complete_relative_to_cross_checked_catalogue";
      if (r.status != mhgp9::ChainStatus::kComplete || r.reason != expected)
        return fail("equality.status " + where + " " + outcome(r));
      const auto& c = r.catalogue;
      const auto& s = r.tower_stats;
      if (s.sealed_catalogues != (sealed ? 1u : 0u) ||
          s.seal_sampled_balls != (sealed ? (c.balls + tw::kSealSampleStride - 1) / tw::kSealSampleStride : 0u))
        return fail("equality.seal_path " + where);
      u64 regular = 0;
      for (const auto v : c.regular_supports_by_arity) regular += v;
      if (regular != c.balls - c.extra_shell_balls || c.regular_supports_by_arity[0] != 0 ||
          c.regular_supports_by_arity[1] != 0)
        return fail("equality.regular_supports " + where);
      if (reference == nullptr) {
        reference = &runs.front();
        const std::array<u64, 3> digests{r.tower_digest, r.catalogue_digest, r.presentation_digest};
        if (engine != nullptr && !lanes) *engine = digests;
        if (engine != nullptr && lanes && digests != *engine) return fail("equality.lanes_vs_engine " + where);
      } else {
        const auto& x = *reference;
        if (r.tower_digest != x.tower_digest || r.catalogue_digest != x.catalogue_digest ||
            r.presentation_digest != x.presentation_digest || r.tower_digest == 0 || r.catalogue_digest == 0)
          return fail("equality.digests " + where);
        if (!same_orders(r, x) || c.balls != x.catalogue.balls || c.extra_shell_balls != x.catalogue.extra_shell_balls ||
            c.regular_supports_by_arity != x.catalogue.regular_supports_by_arity)
          return fail("equality.orders_catalogue " + where);
      }
      if (sealed) {
        // The seal changes neither the work nor the counts of the tower (the
        // unsealed twin: same workers, same resolution path).
        const auto& twin = runs[runs.size() - 2];
        if (!same_work(s, twin.tower_stats)) return fail("equality.tower_work " + where);
        ++floors.sealed_runs;
        floors.sampled += s.seal_sampled_balls;
        if (lanes) ++floors.lanes_sealed_runs;
        // Pass 1 checks at most the sample's regular balls under the seal.
        if (s.declared_support_checks < twin.tower_stats.declared_support_checks) ++floors.reduced_pass1;
      } else {
        ++floors.unsealed_runs;
      }
      floors.q3_regular += c.regular_supports_by_arity[3];
      floors.q4_regular += c.regular_supports_by_arity[4];
      floors.extended += c.extra_shell_balls;
    }
  return true;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::fprintf(stderr, "usage: mhgp9_chain_sealed_catalogue_gate --selftest\n");
    return 2;
  }
  const auto f = fixture();
  const auto ix = index_of(f.points);
  if (!check_fixture(f, ix, 5)) return 3;
  if (!direct_tower(f, ix)) return 1;
  if (!forged(f)) return 1;
  const auto tail = tail_cloud();
  if (!faults(tail)) return 1;
  std::array<u64, 3> engine_k5{};
  if (!equality(f.points, "fixture", 5, false, nullptr)) return 1;
  for (unsigned kmax : {3u, 5u, 10u})
    if (!equality(tail, "tail", kmax, false, kmax == 5 ? &engine_k5 : nullptr)) return 1;
  if (!equality(tail, "tail", 5, true, &engine_k5)) return 1;
  const bool floors_hold = floors.direct_refusals == 4 && floors.forged_refusals == 8 && floors.typed_refusals == 3 &&
                           floors.fault_refusals == 3 && floors.sealed_runs == 10 && floors.unsealed_runs == 10 &&
                           floors.lanes_sealed_runs == 2 && floors.reduced_pass1 == 10 && floors.sampled >= 100 &&
                           floors.q3_regular >= 1000 && floors.q4_regular >= 1000 && floors.extended >= 20;
  std::printf("sealed_catalogue_gate direct=%llu forged=%llu typed=%llu faults=%llu sealed_runs=%llu sampled=%llu "
              "q3_regular=%llu q4_regular=%llu extended=%llu reduced_pass1=%llu\n",
              static_cast<unsigned long long>(floors.direct_refusals),
              static_cast<unsigned long long>(floors.forged_refusals),
              static_cast<unsigned long long>(floors.typed_refusals),
              static_cast<unsigned long long>(floors.fault_refusals),
              static_cast<unsigned long long>(floors.sealed_runs), static_cast<unsigned long long>(floors.sampled),
              static_cast<unsigned long long>(floors.q3_regular), static_cast<unsigned long long>(floors.q4_regular),
              static_cast<unsigned long long>(floors.extended), static_cast<unsigned long long>(floors.reduced_pass1));
  if (!floors_hold) {
    std::printf("cause=floors\n");
    return 3;
  }
  return 0;
}
