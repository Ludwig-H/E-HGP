// MorseHGP3D v9 — porte d'echelle : invariant d'Euler par ordre K sur le
// catalogue reel de la chaine, a la taille d'interet n = 8 000 (auditeur C,
// NOTE_C_INVARIANT_EULER_20260923 ; preuve sans position generale :
// CONTRELEC_EULER_PAR_NERF_20260923 de l'auditeur B).
//
// Pour K <= min(Kmax-2, n), toute boule dont la contribution a chi(L_K) est
// non nulle est admissible dans le catalogue ; si le catalogue est complet,
// n*[K=1] + somme des contributions = 1. C'est une condition NECESSAIRE : une
// somme egale a 1 ne certifie pas chaque cle (des omissions opposees se
// compensent). La contribution d'une boule (p interieurs, coquille U, centre c)
// est le coefficient de t^{K-1} dans t^p * somme_{T sous U, c dans conv(T)} (t-1)^{|T|-1}.
// Pour chaque boule, les T sont lus dans ShellTable::contains_center(), apres
// VALIDATION DES LISTES FOURNIES par la chaine sur un index reconstruit
// (puissance exacte nulle sur chaque site de coquille, negative sur chaque
// interieur) et verification de q_min, qui certifie la minimalite du support
// sans tour FULL. Ce n'est pas un recensement independant (contrelecture B) :
// un juge d'echantillon le complete, une boule sur 64 recensee par balayage
// brut de TOUS les sites (puissance exacte), ensembles d'interieurs et de
// coquille egaux a ceux de la chaine.
// La chaine tourne SANS tour : une omission du generateur est jugee par
// l'invariant, non par la detection partielle de la tour.
//
// Trois familles v8 epinglees (tests/gen/front_fixtures.hpp : uniform, terrain,
// clusters ; graine 3), chaine jusqu'au catalogue K5 (s = 8, deux fils), puis
// protocole Kmax+2 sur la premiere famille : chaine K7, Euler pour K <= 5, et
// egalite EXACTE, cle par cle (cle, p, q_min, u), du catalogue K5 avec la
// restriction p + q_min <= 6 du catalogue K7.
//
//   mhgp9_chain_euler_scale_gate --selftest [--n=8000]
//
// Code 0 conforme, 1 desaccord (ligne `cause=` sur stdout), 2 argument,
// 3 plancher (vacuite).
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "../../src/chain/tower_chain.hpp"
#include "../../src/tower/forest/local_plateau.hpp"
#include "../../src/tower/tree/cloud_index.hpp"
#include "../gen/front_fixtures.hpp"

namespace {

using mhgp9::tower::BallData;

struct Totals {
  std::uint64_t balls = 0, degenerate = 0, checked_orders = 0, sampled = 0;
};
constexpr std::uint64_t kSampleStride = 64;

// Coefficient de t^{K-1} dans t^p (t-1)^{j-1}, accumule par ordre K = 1..kmax.
void add_polynomial(std::array<std::int64_t, 11>& e, unsigned p, unsigned j, std::int64_t count, unsigned kmax) {
  static constexpr std::int64_t binom[13][13] = {
      {1}, {1, 1}, {1, 2, 1}, {1, 3, 3, 1}, {1, 4, 6, 4, 1}, {1, 5, 10, 10, 5, 1}, {1, 6, 15, 20, 15, 6, 1},
      {1, 7, 21, 35, 35, 21, 7, 1}, {1, 8, 28, 56, 70, 56, 28, 8, 1}, {1, 9, 36, 84, 126, 126, 84, 36, 9, 1},
      {1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1}, {1, 11, 55, 165, 330, 462, 462, 330, 165, 55, 11, 1},
      {1, 12, 66, 220, 495, 792, 924, 792, 495, 220, 66, 12, 1}};
  for (unsigned i = 0; i < j && p + i + 1 <= kmax; ++i)
    e[p + i + 1] += (((j - 1 - i) % 2) ? -1 : 1) * binom[j - 1][i] * count;
}

// Bilan d'Euler recalcule depuis le catalogue ; `ok` faux sur incoherence de recensement.
bool euler_of(const std::vector<mhgp9::gen::Point3>& points, const std::vector<BallData>& balls, unsigned kmax,
              std::array<std::int64_t, 11>* out, Totals* totals, std::string* cause) {
  std::vector<mhgp9::tower::InputPoint> input(points.size());
  for (std::size_t i = 0; i < points.size(); ++i)
    input[i] = mhgp9::tower::InputPoint{static_cast<mhgp9::tower::PointId>(i),
                                        mhgp9::tower::P3{points[i].x, points[i].y, points[i].z}};
  const auto ix = mhgp9::tower::build_cloud_index(input);
  std::array<std::int64_t, 11> e{};
  std::vector<std::int32_t> inside, shell, listed;
  for (const auto& b : balls) {
    // Juge d'echantillon (pas deterministe) : recensement brut sur tous les sites.
    if (totals->balls % kSampleStride == 0) {
      inside.clear(); shell.clear();
      for (std::int32_t u = 0; u < ix.unique_count(); ++u) {
        const auto power = b.key.power(ix.upos[static_cast<std::size_t>(u)]);
        if (power < 0) inside.push_back(u);
        else if (power == 0) shell.push_back(u);
      }
      listed.assign(b.interior().begin(), b.interior().end());
      std::sort(listed.begin(), listed.end());
      if (listed != inside) { *cause = "sample.interior_set_differs"; return false; }
      listed.assign(b.shell().begin(), b.shell().end());
      std::sort(listed.begin(), listed.end());
      if (listed != shell) { *cause = "sample.shell_set_differs"; return false; }
      ++totals->sampled;
    }
    ++totals->balls;
    const unsigned p = b.n_interior, q = b.arity, u = b.n_shell;
    if (u != q) ++totals->degenerate;
    // Validation des listes fournies : puissance exacte negative sur chaque
    // interieur, nulle sur chaque site de coquille ; puis q_min recalcule par le
    // quotient local, qui certifie aussi la minimalite du support d'une coquille
    // reguliere (la chaine ne la verifie pas sans tour FULL).
    mhgp9::tower::local_plateau::LocalCensus local{b.key, {}, {}};
    for (const auto id : b.interior()) {
      const auto& pos = ix.upos[static_cast<std::size_t>(id)];
      if (b.key.power(pos) >= 0) { *cause = "census.interior_not_strict"; return false; }
      local.interior.push_back({ix.point_id(id), pos});
    }
    for (const auto id : b.shell()) {
      const auto& pos = ix.upos[static_cast<std::size_t>(id)];
      if (b.key.power(pos) != 0) { *cause = "census.shell_power_nonzero"; return false; }
      local.shell.push_back({ix.point_id(id), pos});
    }
    const auto table = mhgp9::tower::local_plateau::ShellTable::prepare(std::move(local));
    if (table.q_min() != q) { *cause = "census.qmin_differs"; return false; }
    std::array<std::int64_t, 13> by_size{};
    const auto& contains = table.contains_center();
    for (std::size_t mask = 1; mask < contains.size(); ++mask)
      if (contains[mask]) ++by_size[static_cast<std::size_t>(std::popcount(static_cast<unsigned>(mask)))];
    for (unsigned j = 2; j <= u; ++j)
      if (by_size[j]) add_polynomial(e, p, j, by_size[j], kmax);
  }
  e[1] += static_cast<std::int64_t>(points.size());
  *out = e;
  return true;
}

mhgp9::ChainResult run(const std::vector<mhgp9::gen::Point3>& points, unsigned kmax) {
  mhgp9::ChainOptions options;
  options.kmax = kmax;
  options.separation_s = 8;
  options.workers = 2;
  options.run_tower = false;      // porte independante de la tour : minimalite certifiee ci-dessus
  options.keep_catalogue = true;
  return mhgp9::run_tower_chain(points, options);
}

using Row = std::tuple<mhgp9::tower::BallKey, unsigned, unsigned, unsigned>;

std::vector<Row> rows_of(const std::vector<BallData>& balls, unsigned max_rank) {
  std::vector<Row> rows;
  for (const auto& b : balls)
    if (static_cast<unsigned>(b.n_interior) + b.arity <= max_rank) rows.emplace_back(b.key, b.n_interior, b.arity, b.n_shell);
  std::sort(rows.begin(), rows.end(), [](const Row& x, const Row& y) {
    if (std::get<0>(x) != std::get<0>(y)) return std::get<0>(x) < std::get<0>(y);
    return std::make_tuple(std::get<1>(x), std::get<2>(x), std::get<3>(x)) <
           std::make_tuple(std::get<1>(y), std::get<2>(y), std::get<3>(y));
  });
  return rows;
}

}  // namespace

int main(int argc, char** argv) {
  std::size_t n = 8000;
  bool selftest = false;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg == "--selftest") selftest = true;
    else if (arg.starts_with("--n=")) {
      const auto digits = arg.substr(4);
      const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), n);
      if (digits.empty() || error != std::errc{} || end != digits.data() + digits.size()) {
        std::fprintf(stderr, "usage: mhgp9_chain_euler_scale_gate --selftest [--n=8000]\n");
        return 2;
      }
    }
    else { std::fprintf(stderr, "usage: mhgp9_chain_euler_scale_gate --selftest [--n=8000]\n"); return 2; }
  }
  if (!selftest || n < 64 || n > 65536) {
    std::fprintf(stderr, "usage: mhgp9_chain_euler_scale_gate --selftest [--n=8000]\n");
    return 2;
  }
  Totals totals;
  std::vector<BallData> first_k5;
  std::vector<mhgp9::gen::Point3> first_points;
  for (const std::string_view family : {"uniform", "terrain", "clusters"}) {
    const auto fixture = mhgp9::gen::bench::make_front_fixture(n, family, 3);
    const auto& points = fixture.points;
    const auto r = run(points, 5);
    if (r.status != mhgp9::ChainStatus::kComplete) {
      std::printf("cause=chain.status family=%s kmax=5 reason=%s\n", std::string(family).c_str(), r.reason.c_str());
      return 1;
    }
    std::array<std::int64_t, 11> e{};
    std::string cause;
    if (!euler_of(points, r.catalogue_balls, 5, &e, &totals, &cause)) {
      std::printf("cause=%s family=%s\n", cause.c_str(), std::string(family).c_str());
      return 1;
    }
    // Sonde v13 : la chaine publie ses propres sommes (et refuse une
    // violation) ; elles doivent egaler celles de ce calcul independant.
    for (unsigned k = 1; k <= 5; ++k)
      if (e[k] != r.catalogue.euler_by_k[k]) {
        std::printf("cause=euler.chain_sum_differs family=%s kmax=5 k=%u gate=%lld chain=%lld\n",
                    std::string(family).c_str(), k, static_cast<long long>(e[k]),
                    static_cast<long long>(r.catalogue.euler_by_k[k]));
        return 1;
      }
    const unsigned checkable = static_cast<unsigned>(std::min<std::size_t>(3, n));
    for (unsigned k = 1; k <= checkable; ++k) {
      ++totals.checked_orders;
      std::printf("euler family=%s kmax=5 k=%u value=%lld\n", std::string(family).c_str(), k, static_cast<long long>(e[k]));
      if (e[k] != 1) {
        std::printf("cause=euler.k%u family=%s value=%lld\n", k, std::string(family).c_str(), static_cast<long long>(e[k]));
        return 1;
      }
    }
    if (first_k5.empty()) { first_k5 = r.catalogue_balls; first_points = points; }
  }
  // Protocole Kmax+2 sur la premiere famille : K7, Euler K <= 5, restriction exacte.
  const auto r7 = run(first_points, 7);
  if (r7.status != mhgp9::ChainStatus::kComplete) {
    std::printf("cause=chain.status family=uniform kmax=7 reason=%s\n", r7.reason.c_str());
    return 1;
  }
  std::array<std::int64_t, 11> e7{};
  std::string cause;
  if (!euler_of(first_points, r7.catalogue_balls, 7, &e7, &totals, &cause)) {
    std::printf("cause=%s family=uniform kmax=7\n", cause.c_str());
    return 1;
  }
  for (unsigned k = 1; k <= 7; ++k)
    if (e7[k] != r7.catalogue.euler_by_k[k]) {
      std::printf("cause=euler.chain_sum_differs family=uniform kmax=7 k=%u gate=%lld chain=%lld\n", k,
                  static_cast<long long>(e7[k]), static_cast<long long>(r7.catalogue.euler_by_k[k]));
      return 1;
    }
  for (unsigned k = 1; k <= 5; ++k) {
    ++totals.checked_orders;
    std::printf("euler family=uniform kmax=7 k=%u value=%lld\n", k, static_cast<long long>(e7[k]));
    if (e7[k] != 1) {
      std::printf("cause=euler.kmax7.k%u family=uniform value=%lld\n", k, static_cast<long long>(e7[k]));
      return 1;
    }
  }
  const auto a = rows_of(first_k5, 6), b = rows_of(r7.catalogue_balls, 6);
  if (a != b) {
    std::printf("cause=restriction.kmax5_vs_kmax7 k5=%zu k7_restricted=%zu\n", a.size(), b.size());
    return 1;
  }
  // Planchers contre la vacuite : grandes populations et coquilles etendues exercees.
  if (totals.balls < 50 * n || totals.degenerate == 0 || totals.checked_orders != 14 ||
      totals.sampled * kSampleStride < totals.balls) {
    std::printf("cause=floor balls=%llu degenerate=%llu orders=%llu\n", static_cast<unsigned long long>(totals.balls),
                static_cast<unsigned long long>(totals.degenerate), static_cast<unsigned long long>(totals.checked_orders));
    return 3;
  }
  std::printf("euler_scale ok n=%zu families=3 balls=%llu degenerate=%llu restricted_rows=%zu sampled=%llu\n", n,
              static_cast<unsigned long long>(totals.balls), static_cast<unsigned long long>(totals.degenerate), a.size(),
              static_cast<unsigned long long>(totals.sampled));
  return 0;
}
