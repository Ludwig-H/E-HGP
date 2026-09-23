// Auditeur C, 23 septembre 2026 — sonde d'omission de la tour FULL (audit, hors produit).
//
// Question : la tour FULL refuse-t-elle un catalogue auquel manque une boule ?
// Enonce a tester (verification adverse de l'audit C, constat L4-02) : toute
// boule omise dont l'ordre haut p+u est <= Kmax est refusee par la tour
// (full_ball_*missing_weak_terminal ou controle vertical) ; l'angle mort est
// p+u > Kmax. Rien ici n'est une preuve : c'est un juge d'echantillon.
//
// Mode b13 : nuage de 13 sites du contre-exemple de B
// (CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md) ; chaine saine a K5 et K7 avec
// catalogue, puis tour reconstruite sans D, sans T, sans D et T.
// Mode family/file : chaine saine avec catalogue a Kmax, puis, par strate
// (ordre haut <= Kmax ou non, q, coquille reguliere ou etendue), jusqu'a S
// boules retirees UNE a une, a pas regulier ; tour reconstruite sur le reste.
// Classe Euler (coquilles regulieres) : une omission isolee change E_K pour un
// K <= Kmax-2 si et seulement si p <= Kmax-3 (coefficient de t^p dans
// t^p (t-1)^{q-1} = (-1)^{q-1}).
//
//   omission_tower_probe b13
//   omission_tower_probe family <uniform|terrain|clusters> <n> <Kmax> <S> <workers>
//   omission_tower_probe file <cut.u32le> <Kmax> <S> <workers>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "src/chain/tower_chain.hpp"
#include "src/tower/tree/cloud_index.hpp"
#include "tests/gen/front_fixtures.hpp"

using mhgp9::gen::Point3;
using mhgp9::tower::BallData;

namespace {

std::vector<Point3> read_u32le(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::invalid_argument("cannot open input");
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (bytes.size() % 12) throw std::invalid_argument("bad length");
  std::vector<Point3> pts(bytes.size() / 12);
  for (std::size_t i = 0; i < pts.size(); ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) {
      std::uint32_t v = 0;
      for (int b = 0; b < 4; ++b) v |= std::uint32_t(bytes[(3 * i + a) * 4 + b]) << (8 * b);
      if (v > 262143u) throw std::invalid_argument("coordinate outside 18 bits");
      c[a] = v;
    }
    pts[i] = Point3{(mhgp9::gen::Coordinate)c[0], (mhgp9::gen::Coordinate)c[1], (mhgp9::gen::Coordinate)c[2]};
  }
  return pts;
}

mhgp9::tower::CloudIndex index_of(const std::vector<Point3>& points) {
  std::vector<mhgp9::tower::InputPoint> input(points.size());
  for (std::size_t i = 0; i < points.size(); ++i)
    input[i] = mhgp9::tower::InputPoint{static_cast<mhgp9::tower::PointId>(i),
                                        mhgp9::tower::P3{points[i].x, points[i].y, points[i].z}};
  return mhgp9::tower::build_cloud_index(input);
}

mhgp9::ChainResult run_chain(const std::vector<Point3>& points, unsigned kmax, std::size_t workers) {
  mhgp9::ChainOptions o;
  o.kmax = kmax;
  o.workers = workers;
  o.tower_static_threads = static_cast<int>(workers);
  o.keep_catalogue = true;
  return mhgp9::run_tower_chain(points, o);
}

// Tour reconstruite sans les boules d'indices `drop` (tries).
mhgp9::tower::FullBallTowerResult tower_without(const mhgp9::tower::CloudIndex& ix, const std::vector<BallData>& cat,
                                                const std::vector<std::size_t>& drop, unsigned kmax, int threads) {
  std::vector<BallData> kept;
  kept.reserve(cat.size());
  std::size_t d = 0;
  for (std::size_t i = 0; i < cat.size(); ++i) {
    if (d < drop.size() && drop[d] == i) { ++d; continue; }
    kept.push_back(cat[i]);
  }
  return mhgp9::tower::build_full_ball_tower(ix, kept, kmax, threads);
}

int mode_b13() {
  // Coordonnees exactes de la note de B ; amas D (indices 0..5), amas T (6..12).
  const std::vector<Point3> pts = {{0, 0, 0},     {20, 0, 0},   {8, 1, 1},    {9, 2, 2},   {11, 1, 3},
                                   {12, 3, 1},    {100, 0, 0},  {120, 0, 0},  {110, 16, 0}, {108, 4, 1},
                                   {110, 4, 2},   {112, 5, 1},  {109, 6, 2}};
  const auto ix = index_of(pts);
  int failures = 0;
  for (const unsigned kmax : {5u, 7u}) {
    const auto r = run_chain(pts, kmax, 1);
    if (r.status != mhgp9::ChainStatus::kComplete) {
      std::printf("b13 kmax=%u chain_status=%s reason=%s\n", kmax, mhgp9::chain_status_name(r.status), r.reason.c_str());
      return 1;
    }
    const auto& cat = r.catalogue_balls;
    // Identification exacte par les coordonnees de la coquille (index de la
    // tour) et par l'effectif interieur p = 4 ; unicite exigee.
    const auto shell_is = [&](const BallData& b, std::vector<Point3> want) {
      if (b.n_shell != want.size() || b.n_interior != 4) return false;
      for (const auto site : b.shell()) {
        const auto& q = ix.upos[static_cast<std::size_t>(site)];
        const auto it = std::find_if(want.begin(), want.end(), [&](const Point3& w) {
          return w.x == q.x && w.y == q.y && w.z == q.z;
        });
        if (it == want.end()) return false;
        want.erase(it);
      }
      return want.empty();
    };
    std::size_t d_index = cat.size(), t_index = cat.size(), d_count = 0, t_count = 0;
    for (std::size_t i = 0; i < cat.size(); ++i) {
      if (shell_is(cat[i], {{0, 0, 0}, {20, 0, 0}})) { d_index = i; ++d_count; }
      if (shell_is(cat[i], {{100, 0, 0}, {120, 0, 0}, {110, 16, 0}})) { t_index = i; ++t_count; }
    }
    std::printf("b13 kmax=%u balls=%zu D=%s T=%s d_candidates=%zu t_candidates=%zu digest=%016llx\n", kmax,
                cat.size(), d_index < cat.size() ? "present" : "absent", t_index < cat.size() ? "present" : "absent",
                d_count, t_count, static_cast<unsigned long long>(r.tower_digest));
    if (d_count > 1 || t_count > 1) return 1;
    std::vector<std::pair<std::string, std::vector<std::size_t>>> cases;
    if (d_index < cat.size()) cases.push_back({"D", {d_index}});
    if (t_index < cat.size()) cases.push_back({"T", {t_index}});
    if (d_index < cat.size() && t_index < cat.size())
      cases.push_back({"D+T", {std::min(d_index, t_index), std::max(d_index, t_index)}});
    for (const auto& [name, drop] : cases) {
      for (const int threads : {0, 2}) {
        const auto t = tower_without(ix, cat, drop, kmax, threads);
        const bool refused = t.status != mhgp9::tower::FullBallStatus::kCompleteRelative;
        std::printf("b13 kmax=%u omit=%s static=%d refused=%d reason=%s\n", kmax, name.c_str(), threads,
                    refused ? 1 : 0, t.reason);
      }
    }
    if (d_index == cat.size()) ++failures;
    if (kmax == 7 && t_index == cat.size()) ++failures;
  }
  return failures ? 1 : 0;
}

struct Stratum {
  bool top_within = false, regular = false;
  unsigned q = 0;
  auto tie() const { return std::tuple(top_within, regular, q); }
  bool operator<(const Stratum& o) const { return tie() < o.tie(); }
};

int mode_scale(const std::string& label, const std::vector<Point3>& pts, unsigned kmax, std::size_t samples,
               std::size_t workers) {
  const auto r = run_chain(pts, kmax, workers);
  if (r.status != mhgp9::ChainStatus::kComplete) {
    std::printf("%s kmax=%u chain_status=%s reason=%s\n", label.c_str(), kmax, mhgp9::chain_status_name(r.status),
                r.reason.c_str());
    return 1;
  }
  const auto& cat = r.catalogue_balls;
  const auto ix = index_of(pts);
  // Population : strates, et classe jointe « tour » x « Euler » (regulieres).
  std::map<Stratum, std::vector<std::size_t>> strata;
  std::map<std::tuple<unsigned, unsigned>, std::uint64_t> joint_blind_by_pq;  // regulieres p+u>Kmax et p>Kmax-3
  std::uint64_t regular = 0, extended = 0, top_within = 0, euler_visible = 0, joint_blind = 0, ext_top_over = 0;
  for (std::size_t i = 0; i < cat.size(); ++i) {
    const auto& b = cat[i];
    const unsigned p = b.n_interior, u = b.n_shell, q = b.arity;
    const bool reg = u == q, within = p + u <= kmax;
    strata[Stratum{within, reg, q}].push_back(i);
    if (reg) {
      ++regular;
      const bool ev = kmax >= 3 && p + 3 <= kmax;
      if (ev) ++euler_visible;
      if (!within && !ev) { ++joint_blind; ++joint_blind_by_pq[{p, q}]; }
    } else {
      ++extended;
      if (!within) ++ext_top_over;
    }
    if (within) ++top_within;
  }
  std::printf("%s n=%zu kmax=%u balls=%zu regular=%llu extended=%llu top_within=%llu euler_visible_regular=%llu "
              "joint_blind_regular=%llu extended_top_over=%llu digest=%016llx\n",
              label.c_str(), pts.size(), kmax, cat.size(), (unsigned long long)regular, (unsigned long long)extended,
              (unsigned long long)top_within, (unsigned long long)euler_visible, (unsigned long long)joint_blind,
              (unsigned long long)ext_top_over, (unsigned long long)r.tower_digest);
  for (const auto& [pq, count] : joint_blind_by_pq)
    std::printf("%s joint_blind p=%u q=%u count=%llu\n", label.c_str(), std::get<0>(pq), std::get<1>(pq),
                (unsigned long long)count);
  int anomalies = 0;
  for (const auto& [s, members] : strata) {
    const std::size_t take = std::min(samples, members.size());
    std::map<std::string, std::uint64_t> reasons;
    std::uint64_t refused = 0;
    for (std::size_t j = 0; j < take; ++j) {
      const std::size_t idx = members[(j * members.size()) / take];
      const auto t = tower_without(ix, cat, {idx}, kmax, static_cast<int>(workers));
      const bool ref = t.status != mhgp9::tower::FullBallStatus::kCompleteRelative;
      if (ref) ++refused;
      ++reasons[ref ? t.reason : std::string("complete_relative")];
    }
    std::printf("%s stratum top_within=%d regular=%d q=%u population=%zu sampled=%zu refused=%llu", label.c_str(),
                s.top_within ? 1 : 0, s.regular ? 1 : 0, s.q, members.size(), take, (unsigned long long)refused);
    for (const auto& [reason, count] : reasons) std::printf(" %s=%llu", reason.c_str(), (unsigned long long)count);
    std::printf("\n");
    if (s.top_within && refused != take) ++anomalies;  // enonce L4-02 contredit
  }
  std::printf("%s claim_top_within_refused=%s\n", label.c_str(), anomalies ? "CONTRADICTED" : "holds_on_sample");
  return anomalies ? 4 : 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string_view(argv[1]) == "b13") return mode_b13();
    if (argc == 7 && std::string_view(argv[1]) == "family") {
      const auto fx = mhgp9::gen::bench::make_front_fixture(std::stoul(argv[3]), argv[2], 3);
      return mode_scale(std::string(argv[2]) + "_" + argv[3], fx.points, std::stoul(argv[4]), std::stoul(argv[5]),
                        std::stoul(argv[6]));
    }
    if (argc == 6 && std::string_view(argv[1]) == "file") {
      const std::string path = argv[2];
      const auto slash = path.find_last_of('/');
      return mode_scale(path.substr(slash == std::string::npos ? 0 : slash + 1), read_u32le(path),
                        std::stoul(argv[3]), std::stoul(argv[4]), std::stoul(argv[5]));
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 2;
  }
  std::fprintf(stderr, "usage: omission_tower_probe b13 | family <name> <n> <Kmax> <S> <workers> | "
                       "file <cut.u32le> <Kmax> <S> <workers>\n");
  return 2;
}
