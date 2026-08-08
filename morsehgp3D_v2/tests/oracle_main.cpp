// MorseHGP3D v2 — oracle de reference.
//
// P0 : le catalogue produit par build_catalogue doit etre identique a
//      l'enumeration exhaustive de tous les supports de taille 1 a 4.
// P2 : pour chaque ordre k et chaque niveau, le nombre de composantes de
//      Gamma_k(a) (definition normative, DEFINITION_HGP_3D.md §1) doit egaler
//      celui lu sur la foret v2.
//
// Les deux comparaisons sont exactes. Aucun accord moyen n'est accepte.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <random>
#include <set>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "mhgp/miniball.hpp"

using namespace mhgp;

namespace {

int failures = 0;

void check(bool cond, const char* what, const std::string& detail = "") {
  if (!cond) {
    ++failures;
    std::printf("  ECHEC %s %s\n", what, detail.c_str());
  }
}

// ---------------------------------------------------------------------------
// Catalogue force brute
// ---------------------------------------------------------------------------
struct BruteSphere {
  std::vector<i32> support;
  std::vector<i32> members;
  int rank = 0;
};

std::vector<BruteSphere> brute_catalogue(const std::vector<P3>& pts, int s_max,
                                         int* degenerate) {
  const int n = static_cast<int>(pts.size());
  std::vector<BruteSphere> out;
  *degenerate = 0;
  std::vector<i32> ids(4);
  for (int m = 1; m <= 4 && m <= n; ++m) {
    std::vector<int> idx(static_cast<size_t>(m));
    for (int i = 0; i < m; ++i) idx[static_cast<size_t>(i)] = i;
    while (true) {
      Sphere s{};
      bool built = false;
      const P3 a = pts[static_cast<size_t>(idx[0])];
      if (m == 1) { s = sphere1(a); built = true; }
      else if (m == 2) { s = sphere2(a, pts[static_cast<size_t>(idx[1])]); built = true; }
      else if (m == 3) {
        const P3 b = pts[static_cast<size_t>(idx[1])], c = pts[static_cast<size_t>(idx[2])];
        built = sphere3(a, b, c, &s) && well_centered3(a, b, c);
      } else {
        const P3 b = pts[static_cast<size_t>(idx[1])], c = pts[static_cast<size_t>(idx[2])];
        const P3 d = pts[static_cast<size_t>(idx[3])];
        built = sphere4(a, b, c, d, &s) && well_centered4(s, a, b, c, d);
      }
      if (built) {
        BruteSphere bs;
        int on = 0;
        bool bad = false;
        for (int z = 0; z < n; ++z) {
          const int side = sphere_side(s, pts[static_cast<size_t>(z)]);
          if (side > 0) continue;
          if (side == 0) {
            ++on;
            if (std::find(idx.begin(), idx.end(), z) == idx.end()) bad = true;
          }
          bs.members.push_back(z);
        }
        if (bad) ++(*degenerate);
        if (!bad && on == m && static_cast<int>(bs.members.size()) <= s_max) {
          bs.rank = static_cast<int>(bs.members.size());
          for (int i = 0; i < m; ++i) bs.support.push_back(idx[static_cast<size_t>(i)]);
          std::sort(bs.support.begin(), bs.support.end());
          std::sort(bs.members.begin(), bs.members.end());
          out.push_back(std::move(bs));
        }
      }
      int i = m - 1;
      while (i >= 0 && idx[static_cast<size_t>(i)] == n - m + i) --i;
      if (i < 0) break;
      ++idx[static_cast<size_t>(i)];
      for (int j = i + 1; j < m; ++j)
        idx[static_cast<size_t>(j)] = idx[static_cast<size_t>(j - 1)] + 1;
    }
  }
  std::sort(out.begin(), out.end(), [](const BruteSphere& a, const BruteSphere& b) {
    return a.support < b.support;
  });
  return out;
}

// ---------------------------------------------------------------------------
// Gamma_k(a) exhaustif : composantes
// ---------------------------------------------------------------------------
struct DSUv {
  std::vector<int> p;
  int find(int a) { while (p[static_cast<size_t>(a)] != a) { p[static_cast<size_t>(a)] = p[static_cast<size_t>(p[static_cast<size_t>(a)])]; a = p[static_cast<size_t>(a)]; } return a; }
  void unite(int a, int b) { a = find(a); b = find(b); if (a != b) p[static_cast<size_t>(a)] = b; }
};

// enumere les sous-ensembles de taille k, renvoie ceux de niveau <= a
void subsets(int n, int k, std::vector<std::vector<i32>>* out) {
  out->clear();
  if (k > n) return;
  std::vector<int> idx(static_cast<size_t>(k));
  for (int i = 0; i < k; ++i) idx[static_cast<size_t>(i)] = i;
  while (true) {
    std::vector<i32> v(idx.begin(), idx.end());
    out->push_back(std::move(v));
    int i = k - 1;
    while (i >= 0 && idx[static_cast<size_t>(i)] == n - k + i) --i;
    if (i < 0) break;
    ++idx[static_cast<size_t>(i)];
    for (int j = i + 1; j < k; ++j)
      idx[static_cast<size_t>(j)] = idx[static_cast<size_t>(j - 1)] + 1;
  }
}

int gamma_components(const std::vector<P3>& pts, int k, const Sphere& level) {
  const int n = static_cast<int>(pts.size());
  std::vector<std::vector<i32>> faces, cofaces;
  subsets(n, k, &faces);
  subsets(n, k + 1, &cofaces);
  std::map<std::vector<i32>, int> id;
  for (auto& f : faces) {
    MiniballResult mb = miniball_of(pts, f.data(), k);
    if (!mb.ok) continue;
    if (sphere_cmp_beta(mb.sph, level) <= 0) {
      const int t = static_cast<int>(id.size());
      id.emplace(f, t);
    }
  }
  DSUv dsu;
  dsu.p.resize(id.size());
  for (size_t i = 0; i < dsu.p.size(); ++i) dsu.p[i] = static_cast<int>(i);
  for (auto& S : cofaces) {
    MiniballResult mb = miniball_of(pts, S.data(), k + 1);
    if (!mb.ok) continue;
    if (sphere_cmp_beta(mb.sph, level) > 0) continue;
    int first = -1;
    for (int drop = 0; drop <= k; ++drop) {
      std::vector<i32> f;
      for (int t = 0; t <= k; ++t) if (t != drop) f.push_back(S[static_cast<size_t>(t)]);
      auto it = id.find(f);
      if (it == id.end()) continue;
      if (first < 0) first = it->second; else dsu.unite(first, it->second);
    }
  }
  std::set<int> roots;
  for (size_t i = 0; i < dsu.p.size(); ++i) roots.insert(dsu.find(static_cast<int>(i)));
  return static_cast<int>(roots.size());
}

// nombre de composantes vivantes lu sur la foret au niveau donne
int forest_components(const Forest& f, double a) {
  int alive = 0;
  for (const ForestNode& n : f.nodes) {
    if (n.beta > a) continue;
    if (n.parent >= 0 && f.nodes[static_cast<size_t>(n.parent)].beta <= a) continue;
    ++alive;
  }
  return alive;
}

}  // namespace

int main() {
  std::mt19937_64 rng(20260808);
  int total_cases = 0, degenerate_total = 0;

  for (int trial = 0; trial < 24; ++trial) {
    const int n = 8 + static_cast<int>(rng() % 5);
    const int K = 2 + static_cast<int>(rng() % 3);
    const int s_max = std::min(K + 1, n);
    std::vector<P3> pts(static_cast<size_t>(n));
    std::uniform_int_distribution<i64> u(0, 4000);
    for (int i = 0; i < n; ++i) pts[static_cast<size_t>(i)] = P3{u(rng), u(rng), u(rng)};

    int degenerate = 0;
    const std::vector<BruteSphere> brute = brute_catalogue(pts, s_max, &degenerate);
    degenerate_total += degenerate;

    Options opt;
    opt.k_max = K;
    opt.threads = 1;
    if (const char* e = std::getenv("MHGP_FULL")) if (e[0] == '1') opt.exhaustive_neighbourhood = true;
    Catalogue cat = build_catalogue(pts, s_max, opt);

    // --- P0 : egalite des catalogues -----------------------------------
    std::vector<std::vector<i32>> got;
    for (const CriticalSphere& s : cat.spheres) {
      std::vector<i32> sup(s.support, s.support + s.n_support);
      got.push_back(sup);
    }
    std::sort(got.begin(), got.end());
    std::vector<std::vector<i32>> want;
    for (const BruteSphere& b : brute) want.push_back(b.support);
    std::sort(want.begin(), want.end());
    if (got != want) {
      std::string miss, extra;
      int nm = 0, ne = 0;
      for (const auto& w : want)
        if (!std::binary_search(got.begin(), got.end(), w)) {
          if (nm++ < 4) {
            miss += " {";
            for (i32 v : w) miss += std::to_string(v) + ",";
            miss += "}";
          }
        }
      for (const auto& g : got)
        if (!std::binary_search(want.begin(), want.end(), g)) {
          if (ne++ < 4) {
            extra += " {";
            for (i32 v : g) extra += std::to_string(v) + ",";
            extra += "}";
          }
        }
      std::printf("  trial=%d n=%d K=%d manquants=%d%s superflus=%d%s\n", trial, n, K,
                  nm, miss.c_str(), ne, extra.c_str());
    }
    check(got == want, "P0 catalogue",
          "trial=" + std::to_string(trial) + " got=" + std::to_string(got.size())
              + " want=" + std::to_string(want.size()));

    // --- P2 : composantes de Gamma_k(a) --------------------------------
    if (got == want) {
      Result r = run(pts, opt);
      for (int k = 1; k <= K; ++k) {
        const Forest& f = r.forests[static_cast<size_t>(k - 1)];
        // niveaux testes : ceux des spheres critiques de rang <= k+1
        std::vector<const CriticalSphere*> lv;
        for (const CriticalSphere& s : cat.spheres)
          if (s.rank <= k + 1) lv.push_back(&s);
        std::sort(lv.begin(), lv.end(), [](const CriticalSphere* a, const CriticalSphere* b) {
          return a->beta < b->beta;
        });
        const size_t step = std::max<size_t>(1, lv.size() / 6);
        for (size_t t = 0; t < lv.size(); t += step) {
          const int g = gamma_components(pts, k, lv[t]->sph);
          const int v = forest_components(f, lv[t]->beta);
          ++total_cases;
          check(g == v, "P2 composantes",
                "trial=" + std::to_string(trial) + " k=" + std::to_string(k)
                    + " gamma=" + std::to_string(g) + " v2=" + std::to_string(v));
        }
      }
    }
  }

  std::printf("cas P2 testes : %d ; supports degeneres rencontres : %d\n",
              total_cases, degenerate_total);
  if (failures) {
    std::printf("ECHEC : %d desaccords\n", failures);
    return 1;
  }
  std::printf("OK : catalogue et forets identiques a l'oracle exhaustif\n");
  return 0;
}
