// MorseHGP3D v9 — porte de l'invariant d'Euler du catalogue sur la chaine reelle.
//
// Juge : la proposition de Morse-Euler par ordre (note C, preuve par le nerf de
// B, sans position generale) : pour K <= min(Kmax-2, n), n*[K=1] + somme des
// contributions des boules du catalogue vaut 1. La chaine publie les sommes et
// refuse (invariant_violated, chain_catalogue_euler_violated) un catalogue qui
// la viole. Fixtures d'egalite degenerees :
//   - carre plan (0,0,0),(2,0,0),(2,2,0),(0,2,0) : coquille etendue de quatre
//     sites, q_min = 2, une naissance a K3 (contre-exemple de B a la formule
//     generique de multifusion), Kmax = 3..10 ;
//   - deux sites, Kmax = 10 : ordres verifiables 1..2 (pas de faux echec) ;
//   - Kmax = 1 et 2 : statut not_checkable (jamais un succes vacant) ;
//   - cube (huit sommets cospheriques), octaedre et son centre, points
//     entiers d'une sphere ;
//   - nuages entiers aleatoires de 6 a 14 sites sur grilles minuscules et u18,
//     Kmax = 5 et 10, s = 8 et 10, un et quatre fils.
// Planchers : boules a coquille etendue rencontrees, ordres verifies a K10,
// statuts not_checkable vus.
//
//   mhgp9_chain_euler_gate --selftest
//
// Code 0 conforme, 1 desaccord (ligne `cause=`), 2 argument, 3 plancher.
// Compilee avec MHGP9_EULER_MUTANT_REGULAR_SHELLS_ONLY (coquille etendue
// traitee comme T = U seule) ou MHGP9_EULER_MUTANT_NO_SITE_TERM (terme n
// oublie), la chaine refuse un catalogue correct : code 1 attendu.
#include <algorithm>
#include <cstdio>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "../../src/chain/tower_chain.hpp"

namespace {
using Point = mhgp9::gen::Point3;
using Coordinate = mhgp9::gen::Coordinate;

unsigned long long runs = 0, checked_orders = 0, extended_balls = 0, not_checkable = 0, k10_orders = 0;

bool fail(const std::string& cause) {
  std::printf("cause=%s\n", cause.c_str());
  return false;
}

// One chain run; the published sums must equal 1 on every checkable order.
bool judge(const std::vector<Point>& points, unsigned kmax, unsigned s, std::size_t workers, const char* name) {
  mhgp9::ChainOptions options;
  options.kmax = kmax;
  options.separation_s = s;
  options.workers = workers;
  options.run_tower = true;
  const auto chain = mhgp9::run_tower_chain(points, options);
  ++runs;
  const auto& c = chain.catalogue;
  const unsigned expected_checkable =
      kmax >= 3 ? static_cast<unsigned>(std::min<std::size_t>(kmax - 2, points.size())) : 0;
  const std::string where = std::string(name) + " K" + std::to_string(kmax) + " s" + std::to_string(s) + " W" +
                            std::to_string(workers);
  if (chain.status != mhgp9::ChainStatus::kComplete) {
    // A refusal is an Euler refusal only with its status, reason and failed
    // sums together (contre-audit B): any other refusal is its own cause.
    if (chain.reason == "chain_catalogue_euler_violated" &&
        (chain.status != mhgp9::ChainStatus::kInvariantViolated || c.euler_status != mhgp9::EulerStatus::kFails))
      return fail("euler.refusal_inconsistent " + where);
    return fail("euler.chain_refused " + where + " reason=" + chain.reason);
  }
  if (c.euler_checkable_max_k != expected_checkable) return fail("euler.checkable_bound " + where);
  if (expected_checkable == 0) {
    if (c.euler_status != mhgp9::EulerStatus::kNotCheckable) return fail("euler.vacuous_status " + where);
    ++not_checkable;
    return true;
  }
  if (c.euler_status != mhgp9::EulerStatus::kHolds) return fail("euler.status " + where);
  for (unsigned k = 1; k <= expected_checkable; ++k) {
    if (c.euler_by_k[k] != 1) return fail("euler.sum " + where + " order " + std::to_string(k));
    ++checked_orders;
    if (kmax == 10) ++k10_orders;
  }
  extended_balls += c.extra_shell_balls;
  return true;
}

std::vector<Point> shifted(std::vector<std::array<long, 3>> raw) {
  std::vector<Point> points;
  for (const auto& p : raw)
    points.push_back({static_cast<Coordinate>(p[0] + 1000), static_cast<Coordinate>(p[1] + 1000),
                      static_cast<Coordinate>(p[2] + 1000)});
  return points;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::fprintf(stderr, "usage: mhgp9_chain_euler_gate --selftest\n");
    return 2;
  }
  // Degenerate square: one extended shell of four sites, a birth at K3.
  const auto square = shifted({{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}});
  for (unsigned kmax = 1; kmax <= 10; ++kmax)
    if (!judge(square, kmax, 8, 1, "square")) return 1;
  // Two sites, Kmax 10: orders 1..2 only (E_3.. are 0, never checked).
  if (!judge(shifted({{0, 0, 0}, {7, 3, 1}}), 10, 8, 1, "two_sites")) return 1;
  // Cospherical solids.
  const std::vector<std::pair<const char*, std::vector<Point>>> solids{
      {"cube", shifted({{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {0, 0, 2}, {2, 2, 0}, {2, 0, 2}, {0, 2, 2}, {2, 2, 2}})},
      {"octahedron_centre",
       shifted({{2, 1, 1}, {0, 1, 1}, {1, 2, 1}, {1, 0, 1}, {1, 1, 2}, {1, 1, 0}, {1, 1, 1}})}};
  for (const auto& [name, points] : solids)
    for (const unsigned kmax : {3u, 5u, 10u})
      if (!judge(points, kmax, 8, 1, name)) return 1;
  {
    std::vector<std::array<long, 3>> sphere;
    for (long x = -5; x <= 5; ++x)
      for (long y = -5; y <= 5; ++y)
        for (long z = -5; z <= 5; ++z)
          if (x * x + y * y + z * z == 9) sphere.push_back({x, y, z});
    sphere.resize(std::min<std::size_t>(sphere.size(), 12));
    if (!judge(shifted(sphere), 10, 8, 1, "sphere9")) return 1;
  }
  // Random integer clouds on tiny (degenerate) and wide grids.
  std::mt19937_64 random(20260923);
  unsigned cloud = 0;
  for (const long bound : {4L, 6L, 262143L})
    for (unsigned repetition = 0; repetition < 12; ++repetition) {
      const unsigned n = 6 + static_cast<unsigned>(random() % 9);
      std::vector<Point> points;
      unsigned guard = 0;
      while (points.size() < n && guard++ < 10000) {
        const Point p{static_cast<Coordinate>(random() % static_cast<unsigned long>(bound)),
                      static_cast<Coordinate>(random() % static_cast<unsigned long>(bound)),
                      static_cast<Coordinate>(random() % static_cast<unsigned long>(bound))};
        if (std::find(points.begin(), points.end(), p) == points.end()) points.push_back(p);
      }
      const std::string name = "random" + std::to_string(cloud++);
      for (const unsigned kmax : {5u, 10u})
        if (!judge(points, kmax, repetition % 2 ? 10 : 8, repetition % 3 ? 1 : 4, name.c_str())) return 1;
    }
  std::printf("chain_euler_gate runs=%llu checked_orders=%llu k10_orders=%llu extended_balls=%llu not_checkable=%llu\n",
              runs, checked_orders, k10_orders, extended_balls, not_checkable);
  if (extended_balls == 0 || k10_orders == 0 || not_checkable != 2) {
    std::printf("cause=floor.euler_coverage\n");
    return 3;
  }
  return 0;
}
