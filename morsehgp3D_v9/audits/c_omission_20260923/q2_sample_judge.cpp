// Auditeur C, 23 septembre 2026 — juge d'echantillon q2 independant du generateur (audit, hors produit).
//
// Pour un site a tire et TOUT autre site b, la boule diametrale de {a,b}
// (centre au milieu, donc toujours dans l'interieur relatif du support) est
// recensee par balayage brut de tous les sites, en entiers exacts :
// x interieur strict <=> (x-a).(x-b) < 0, x sur la sphere <=> = 0 (u18 : |.| < 2^38).
// Si elle a p <= Kmax-1 interieurs, elle est admise (p + q_min <= Kmax+1 avec
// q_min = 2) : le catalogue DOIT contenir une boule de p interieurs dont la
// coquille contient a et b et est portee par cette meme sphere, avec autant de
// sites de coquille que le balayage en trouve. Aucune structure du generateur
// (WSPD, temoins, Pool) n'est utilisee. Cout par site tire : O(n) candidats b,
// chacun un balayage O(n) arrete des que p depasse Kmax-1 : jamais O(n^3).
// Controle anti-vacuite : sur le premier site tire qui a une boule attendue, on
// retire une boule de la table et le juge doit la declarer manquante.
//
//   q2_sample_judge family <uniform|terrain|clusters> <n> <Kmax> <sites> <workers>
//   q2_sample_judge file <cut.u32le> <Kmax> <sites> <workers>
//
// Sortie : une ligne par cas ; code 0 conforme, 1 boule manquante, 3 vacuite.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "src/chain/tower_chain.hpp"
#include "src/tower/tree/cloud_index.hpp"
#include "tests/gen/front_fixtures.hpp"

using mhgp9::gen::Point3;
using mhgp9::tower::BallData;
using mhgp9::tower::P3;

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

inline std::int64_t power(const P3& x, const P3& a, const P3& b) {
  return (x.x - a.x) * (x.x - b.x) + (x.y - a.y) * (x.y - b.y) + (x.z - a.z) * (x.z - b.z);
}

struct Totals {
  std::uint64_t pairs = 0, expected = 0, found = 0, missing = 0, extended = 0, by_p[10] = {};
};

int run(const std::string& label, const std::vector<Point3>& points, unsigned kmax, std::size_t sites,
        std::size_t workers) {
  mhgp9::ChainOptions o;
  o.kmax = kmax;
  o.workers = workers;
  o.tower_static_threads = static_cast<int>(workers);
  o.keep_catalogue = true;
  o.run_tower = false;  // le juge porte sur le catalogue
  const auto r = mhgp9::run_tower_chain(points, o);
  if (r.status != mhgp9::ChainStatus::kComplete) {
    std::printf("%s kmax=%u chain_status=%s reason=%s\n", label.c_str(), kmax, mhgp9::chain_status_name(r.status),
                r.reason.c_str());
    return 2;
  }
  std::vector<mhgp9::tower::InputPoint> input(points.size());
  for (std::size_t i = 0; i < points.size(); ++i)
    input[i] = mhgp9::tower::InputPoint{static_cast<mhgp9::tower::PointId>(i),
                                        P3{points[i].x, points[i].y, points[i].z}};
  const auto ix = mhgp9::tower::build_cloud_index(input);
  const auto& pos = ix.upos;
  const std::size_t n = pos.size();
  const auto& cat = r.catalogue_balls;
  // Boules du catalogue par site de coquille.
  std::vector<std::vector<std::uint32_t>> by_site(n);
  for (std::size_t i = 0; i < cat.size(); ++i)
    for (const auto s : cat[i].shell()) by_site[static_cast<std::size_t>(s)].push_back(static_cast<std::uint32_t>(i));

  Totals t;
  bool mutant_done = false, mutant_killed = false;
  const std::size_t take = std::min(sites, n);
  std::vector<std::int32_t> shell;
  for (std::size_t j = 0; j < take; ++j) {
    const std::size_t a = (j * n) / take;
    for (std::size_t b = 0; b < n; ++b) {
      if (b == a) continue;
      ++t.pairs;
      unsigned p = 0;
      shell.clear();
      bool over = false;
      for (std::size_t x = 0; x < n; ++x) {
        const auto w = power(pos[x], pos[a], pos[b]);
        if (w < 0) {
          if (++p > kmax - 1) { over = true; break; }
        } else if (w == 0) {
          shell.push_back(static_cast<std::int32_t>(x));
        }
      }
      if (over) continue;
      ++t.expected;
      ++t.by_p[p];
      if (shell.size() > 2) ++t.extended;
      // Recherche d'une boule du catalogue portee par la meme sphere, en
      // excluant eventuellement un indice (controle anti-vacuite).
      const auto find = [&](std::int64_t exclude) -> std::int64_t {
        for (const auto bi : by_site[a]) {
          if (static_cast<std::int64_t>(bi) == exclude) continue;
          const auto& ball = cat[bi];
          if (ball.n_interior != p || ball.n_shell != shell.size()) continue;
          bool same = true, has_b = false;
          for (const auto s : ball.shell()) {
            if (power(pos[static_cast<std::size_t>(s)], pos[a], pos[b]) != 0) { same = false; break; }
            if (static_cast<std::size_t>(s) == b) has_b = true;
          }
          if (same && has_b) return static_cast<std::int64_t>(bi);
        }
        return -1;
      };
      const std::int64_t hit = find(-1);
      const bool ok = hit >= 0;
      // Anti-vacuite : la premiere boule attendue et trouvee est retiree de la
      // table ; le juge doit alors ne plus en trouver aucune.
      if (ok && !mutant_done) {
        mutant_done = true;
        mutant_killed = find(hit) < 0;
      }
      if (ok) ++t.found;
      else {
        ++t.missing;
        std::printf("%s MISSING a=%zu b=%zu p=%u shell=%zu\n", label.c_str(), a, b, p, shell.size());
      }
    }
  }
  std::printf("%s n=%zu kmax=%u balls=%zu sampled_sites=%zu pairs=%llu expected=%llu found=%llu missing=%llu "
              "extended=%llu mutant_killed=%d",
              label.c_str(), n, kmax, cat.size(), take, (unsigned long long)t.pairs, (unsigned long long)t.expected,
              (unsigned long long)t.found, (unsigned long long)t.missing, (unsigned long long)t.extended,
              mutant_killed ? 1 : 0);
  for (unsigned p = 0; p < kmax && p < 10; ++p) std::printf(" p%u=%llu", p, (unsigned long long)t.by_p[p]);
  std::printf("\n");
  if (t.missing) return 1;
  if (!mutant_killed || t.expected == 0) return 3;
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 7 && std::string_view(argv[1]) == "family") {
      const auto fx = mhgp9::gen::bench::make_front_fixture(std::stoul(argv[3]), argv[2], 3);
      return run(std::string(argv[2]) + "_" + argv[3], fx.points, std::stoul(argv[4]), std::stoul(argv[5]),
                 std::stoul(argv[6]));
    }
    if (argc == 6 && std::string_view(argv[1]) == "file") {
      const std::string path = argv[2];
      const auto slash = path.find_last_of('/');
      return run(path.substr(slash == std::string::npos ? 0 : slash + 1), read_u32le(path), std::stoul(argv[3]),
                 std::stoul(argv[4]), std::stoul(argv[5]));
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 2;
  }
  std::fprintf(stderr, "usage: q2_sample_judge family <name> <n> <Kmax> <sites> <workers> | "
                       "file <cut.u32le> <Kmax> <sites> <workers>\n");
  return 2;
}
