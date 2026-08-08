// MorseHGP3D v2 — oracle independant et structurel.
//
// Cet oracle ne partage AUCUNE primitive avec le chemin teste :
//   - arithmetique rationnelle explicite (numerateur / denominateur reduits),
//     et non des formules vectorielles a denominateur commun ;
//   - miniboule par resolution de Gauss du systeme 2<v,B_i> = |B_i|^2 dans le
//     sous-espace engendre, et non par les formules de Cramer de sphere.hpp ;
//   - appartenance par comparaison de deux rationnels, et non par le predicat
//     entier sphere_side.
//
// Il compare, pour chaque nuage :
//   O1  le catalogue : support, rang, membres, shell ET niveau exact ;
//   O2  la structure du merge tree : a chaque niveau critique, la PARTITION
//       des minima, puis le multi-ensemble des (niveau, arite) des fusions.
//       O2 detecte ce qu'un comptage de composantes ne peut pas voir : une
//       chaine de fusions binaires de meme niveau au lieu d'une multifusion.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"

using namespace mhgp;

namespace {

int failures = 0;
void fail(const std::string& what) { ++failures; std::printf("  ECHEC %s\n", what.c_str()); }

// ---------------------------------------------------------------------------
// Rationnels reduits sur i128, avec detection de debordement.
// ---------------------------------------------------------------------------
bool overflowed = false;

i128 gcd128(i128 a, i128 b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b) { const i128 t = a % b; a = b; b = t; }
  return a;
}

struct Rat {
  i128 n = 0, d = 1;
};

const i128 kGuard = (i128(1) << 100);

Rat mk(i128 n, i128 d) {
  if (d == 0) { overflowed = true; return Rat{0, 1}; }
  if (d < 0) { n = -n; d = -d; }
  const i128 g = gcd128(n, d);
  if (g > 1) { n /= g; d /= g; }
  if (n > kGuard || n < -kGuard || d > kGuard) overflowed = true;
  return Rat{n, d};
}

Rat radd(const Rat& a, const Rat& b) { return mk(a.n * b.d + b.n * a.d, a.d * b.d); }
Rat rsub(const Rat& a, const Rat& b) { return mk(a.n * b.d - b.n * a.d, a.d * b.d); }
Rat rmul(const Rat& a, const Rat& b) { return mk(a.n * b.n, a.d * b.d); }
Rat rdiv(const Rat& a, const Rat& b) { return mk(a.n * b.d, a.d * b.n); }
int rcmp(const Rat& a, const Rat& b) {
  const i128 l = a.n * b.d, r = b.n * a.d;
  if (l > kGuard || l < -kGuard || r > kGuard || r < -kGuard) overflowed = true;
  return (l > r) - (l < r);
}
Rat rint(i128 v) { return Rat{v, 1}; }

struct RVec { Rat x, y, z; };
RVec vsub(const RVec& a, const RVec& b) { return RVec{rsub(a.x, b.x), rsub(a.y, b.y), rsub(a.z, b.z)}; }
Rat vdot(const RVec& a, const RVec& b) {
  return radd(radd(rmul(a.x, b.x), rmul(a.y, b.y)), rmul(a.z, b.z));
}
RVec vscale(const RVec& a, const Rat& s) { return RVec{rmul(a.x, s), rmul(a.y, s), rmul(a.z, s)}; }
RVec vadd(const RVec& a, const RVec& b) { return RVec{radd(a.x, b.x), radd(a.y, b.y), radd(a.z, b.z)}; }
RVec of(const P3& p) { return RVec{rint(p.x), rint(p.y), rint(p.z)}; }

// Resout M t = r par elimination de Gauss sur les rationnels (M symetrique
// definie positive de taille m <= 3). Renvoie false si singuliere.
bool gauss(std::vector<std::vector<Rat>> M, std::vector<Rat> r, std::vector<Rat>* out) {
  const size_t m = r.size();
  for (size_t c = 0; c < m; ++c) {
    size_t piv = m;
    for (size_t i = c; i < m; ++i) if (M[i][c].n != 0) { piv = i; break; }
    if (piv == m) return false;
    std::swap(M[c], M[piv]);
    std::swap(r[c], r[piv]);
    const Rat p = M[c][c];
    for (size_t k = c; k < m; ++k) M[c][k] = rdiv(M[c][k], p);
    r[c] = rdiv(r[c], p);
    for (size_t i = 0; i < m; ++i) {
      if (i == c || M[i][c].n == 0) continue;
      const Rat f = M[i][c];
      for (size_t k = c; k < m; ++k) M[i][k] = rsub(M[i][k], rmul(f, M[c][k]));
      r[i] = rsub(r[i], rmul(f, r[c]));
    }
  }
  *out = r;
  return true;
}

// Sphere passant par un sous-ensemble affinement independant : centre dans
// l'enveloppe affine, coefficients barycentriques renvoyes.
struct RSphere {
  RVec c;
  Rat r2;
  std::vector<Rat> lambda;  // coordonnees barycentriques du centre
  bool ok = false;
};

RSphere sphere_through(const std::vector<P3>& pts, const std::vector<i32>& U) {
  RSphere out;
  const size_t m = U.size();
  const RVec p0 = of(pts[static_cast<size_t>(U[0])]);
  if (m == 1) {
    out.c = p0;
    out.r2 = rint(0);
    out.lambda = {rint(1)};
    out.ok = true;
    return out;
  }
  std::vector<RVec> B;
  for (size_t i = 1; i < m; ++i) B.push_back(vsub(of(pts[static_cast<size_t>(U[i])]), p0));
  const size_t q = B.size();
  std::vector<std::vector<Rat>> M(q, std::vector<Rat>(q));
  std::vector<Rat> rhs(q);
  for (size_t i = 0; i < q; ++i) {
    for (size_t j = 0; j < q; ++j) M[i][j] = rmul(rint(2), vdot(B[i], B[j]));
    rhs[i] = vdot(B[i], B[i]);
  }
  std::vector<Rat> t;
  if (!gauss(M, rhs, &t)) return out;
  RVec v{rint(0), rint(0), rint(0)};
  for (size_t i = 0; i < q; ++i) v = vadd(v, vscale(B[i], t[i]));
  out.c = vadd(p0, v);
  out.r2 = vdot(v, v);
  Rat s = rint(0);
  for (size_t i = 0; i < q; ++i) s = radd(s, t[i]);
  out.lambda.push_back(rsub(rint(1), s));
  for (size_t i = 0; i < q; ++i) out.lambda.push_back(t[i]);
  out.ok = true;
  return out;
}

// -1 dedans, 0 dessus, +1 dehors
int side(const RSphere& s, const P3& z) {
  const RVec d = vsub(of(z), s.c);
  return rcmp(vdot(d, d), s.r2);
}

struct OracleSphere {
  std::vector<i32> support;
  std::vector<i32> members;
  Rat r2;
  int rank = 0;
};

// Miniboule exacte d'un ensemble : plus petit support couvrant, bien centre.
bool miniball_rat(const std::vector<P3>& pts, const std::vector<i32>& F, RSphere* out,
                  std::vector<i32>* support) {
  const int m = static_cast<int>(F.size());
  for (int sz = 1; sz <= 4 && sz <= m; ++sz) {
    std::vector<int> idx(static_cast<size_t>(sz));
    for (int i = 0; i < sz; ++i) idx[static_cast<size_t>(i)] = i;
    while (true) {
      std::vector<i32> U;
      for (int i = 0; i < sz; ++i) U.push_back(F[static_cast<size_t>(idx[static_cast<size_t>(i)])]);
      const RSphere s = sphere_through(pts, U);
      bool good = s.ok;
      if (good) for (const Rat& l : s.lambda) if (l.n <= 0) { good = false; break; }
      if (good) {
        for (i32 z : F) if (side(s, pts[static_cast<size_t>(z)]) > 0) { good = false; break; }
      }
      if (good) { *out = s; *support = U; return true; }
      int i = sz - 1;
      while (i >= 0 && idx[static_cast<size_t>(i)] == m - sz + i) --i;
      if (i < 0) break;
      ++idx[static_cast<size_t>(i)];
      for (int j = i + 1; j < sz; ++j)
        idx[static_cast<size_t>(j)] = idx[static_cast<size_t>(j - 1)] + 1;
    }
  }
  return false;
}

std::vector<OracleSphere> oracle_catalogue(const std::vector<P3>& pts, int s_max,
                                           int* degenerate) {
  const int n = static_cast<int>(pts.size());
  std::vector<OracleSphere> out;
  *degenerate = 0;
  for (int m = 1; m <= 4 && m <= n; ++m) {
    std::vector<int> idx(static_cast<size_t>(m));
    for (int i = 0; i < m; ++i) idx[static_cast<size_t>(i)] = i;
    while (true) {
      std::vector<i32> U;
      for (int i = 0; i < m; ++i) U.push_back(idx[static_cast<size_t>(i)]);
      const RSphere s = sphere_through(pts, U);
      bool good = s.ok;
      for (const Rat& l : s.lambda) if (l.n <= 0) good = false;
      if (good) {
        OracleSphere os;
        int on = 0;
        bool shell_extra = false;
        for (int z = 0; z < n; ++z) {
          const int sd = side(s, pts[static_cast<size_t>(z)]);
          if (sd > 0) continue;
          if (sd == 0) {
            ++on;
            if (std::find(U.begin(), U.end(), z) == U.end()) shell_extra = true;
          }
          os.members.push_back(z);
        }
        if (shell_extra) ++(*degenerate);
        if (!shell_extra && on == m && static_cast<int>(os.members.size()) <= s_max) {
          os.support = U;
          os.r2 = s.r2;
          os.rank = static_cast<int>(os.members.size());
          out.push_back(std::move(os));
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
  std::sort(out.begin(), out.end(), [](const OracleSphere& a, const OracleSphere& b) {
    return a.support < b.support;
  });
  return out;
}

// ---------------------------------------------------------------------------
// Merge tree de reference : partition de Gamma_k(a) restreinte aux minima
// ---------------------------------------------------------------------------
void subsets(int n, int k, std::vector<std::vector<i32>>* out) {
  out->clear();
  if (k > n || k <= 0) return;
  std::vector<int> idx(static_cast<size_t>(k));
  for (int i = 0; i < k; ++i) idx[static_cast<size_t>(i)] = i;
  while (true) {
    out->emplace_back(idx.begin(), idx.end());
    int i = k - 1;
    while (i >= 0 && idx[static_cast<size_t>(i)] == n - k + i) --i;
    if (i < 0) break;
    ++idx[static_cast<size_t>(i)];
    for (int j = i + 1; j < k; ++j)
      idx[static_cast<size_t>(j)] = idx[static_cast<size_t>(j - 1)] + 1;
  }
}

struct DSUv {
  std::vector<int> p;
  int find(int a) {
    while (p[static_cast<size_t>(a)] != a) {
      p[static_cast<size_t>(a)] = p[static_cast<size_t>(p[static_cast<size_t>(a)])];
      a = p[static_cast<size_t>(a)];
    }
    return a;
  }
  void unite(int a, int b) { a = find(a); b = find(b); if (a != b) p[static_cast<size_t>(a)] = b; }
};

// Partition, au niveau a, de la liste `minima` (ensembles de k points) telle
// qu'induite par Gamma_k(a). Renvoie un vecteur d'etiquettes canoniques.
std::vector<int> gamma_partition(const std::vector<P3>& pts, int k, const Rat& a,
                                 const std::vector<std::vector<i32>>& minima) {
  const int n = static_cast<int>(pts.size());
  std::vector<std::vector<i32>> faces, cofaces;
  subsets(n, k, &faces);
  subsets(n, k + 1, &cofaces);
  std::map<std::vector<i32>, int> id;
  for (auto& f : faces) {
    RSphere s;
    std::vector<i32> sup;
    if (!miniball_rat(pts, f, &s, &sup)) continue;
    if (rcmp(s.r2, a) <= 0) {
      const int t = static_cast<int>(id.size());
      id.emplace(f, t);
    }
  }
  DSUv dsu;
  dsu.p.resize(id.size());
  for (size_t i = 0; i < dsu.p.size(); ++i) dsu.p[i] = static_cast<int>(i);
  for (auto& S : cofaces) {
    RSphere s;
    std::vector<i32> sup;
    if (!miniball_rat(pts, S, &s, &sup)) continue;
    if (rcmp(s.r2, a) > 0) continue;
    int first = -1;
    for (int drop = 0; drop <= k; ++drop) {
      std::vector<i32> f;
      for (int t = 0; t <= k; ++t) if (t != drop) f.push_back(S[static_cast<size_t>(t)]);
      auto it = id.find(f);
      if (it == id.end()) continue;
      if (first < 0) first = it->second; else dsu.unite(first, it->second);
    }
  }
  std::vector<int> lab(minima.size(), -1);
  std::map<int, int> canon;
  for (size_t i = 0; i < minima.size(); ++i) {
    auto it = id.find(minima[i]);
    if (it == id.end()) continue;  // pas encore ne
    const int r = dsu.find(it->second);
    auto c = canon.emplace(r, static_cast<int>(canon.size()));
    lab[i] = c.first->second;
  }
  return lab;
}

// Niveau exact d'un noeud de la foret, relu en rationnel independant depuis les
// entiers de sa sphere source : beta = (nx^2 + ny^2 + nz^2) / den^2. Aucune
// primitive du chemin teste n'est employee, seulement ses donnees.
//
// Lire `beta` a la place serait faux : les deux flottants d'un meme rationnel
// (celui du noeud et celui du niveau teste) different d'un ULP, et cet ecart
// tombe exactement sur les niveaux critiques -- les seuls ou la comparaison
// decide quelque chose.
Rat node_level(const Catalogue& cat, const ForestNode& nd) {
  const Sphere& s = cat.spheres[static_cast<size_t>(nd.source)].sph;
  const i128 num2 = s.nx * s.nx + s.ny * s.ny + s.nz * s.nz;
  return mk(num2, s.den * s.den);
}

// Partition des minima lue sur la foret v2, au niveau exact a.
std::vector<int> forest_partition(const Forest& f, const Catalogue& cat,
                                  const std::vector<i32>& node_of_minimum, const Rat& a) {
  std::vector<int> lab(node_of_minimum.size(), -1);
  std::map<i32, int> canon;
  for (size_t i = 0; i < node_of_minimum.size(); ++i) {
    i32 nd = node_of_minimum[i];
    if (nd < 0 || rcmp(node_level(cat, f.nodes[static_cast<size_t>(nd)]), a) > 0) continue;
    while (true) {
      const i32 up = f.nodes[static_cast<size_t>(nd)].parent;
      if (up < 0 || rcmp(node_level(cat, f.nodes[static_cast<size_t>(up)]), a) > 0) break;
      nd = up;
    }
    auto c = canon.emplace(nd, static_cast<int>(canon.size()));
    lab[i] = c.first->second;
  }
  return lab;
}

bool same_partition(const std::vector<int>& a, const std::vector<int>& b) {
  if (a.size() != b.size()) return false;
  std::map<int, int> fwd, bwd;
  for (size_t i = 0; i < a.size(); ++i) {
    if ((a[i] < 0) != (b[i] < 0)) return false;
    if (a[i] < 0) continue;
    auto f = fwd.emplace(a[i], b[i]);
    if (!f.second && f.first->second != b[i]) return false;
    auto g = bwd.emplace(b[i], a[i]);
    if (!g.second && g.first->second != a[i]) return false;
  }
  return true;
}

}  // namespace

// Usage : mhgp_oracle2 [nuages] [graine]. Les valeurs par defaut sont la porte
// ; les arguments ne servent qu'a rejouer la meme porte plus longtemps.
int main(int argc, char** argv) {
  const int wanted = (argc > 1) ? std::atoi(argv[1]) : 40;
  const unsigned long long seed = (argc > 2) ? std::strtoull(argv[2], nullptr, 10) : 4242ULL;
  std::mt19937_64 rng(seed);
  int trials = 0, o2_cases = 0, degenerate_total = 0, censored = 0;

  for (int trial = 0; trial < wanted; ++trial) {
    const int n = 7 + static_cast<int>(rng() % 3);
    const int K = 2 + static_cast<int>(rng() % 2);
    const int s_max = std::min(K + 1, n);
    std::vector<P3> pts(static_cast<size_t>(n));
    std::uniform_int_distribution<i64> u(0, 120);
    for (int i = 0; i < n; ++i) pts[static_cast<size_t>(i)] = P3{u(rng), u(rng), u(rng)};
    overflowed = false;

    int degenerate = 0;
    const std::vector<OracleSphere> want = oracle_catalogue(pts, s_max, &degenerate);
    if (overflowed) continue;  // hors du domaine de l'oracle rationnel

    Options opt;
    opt.k_max = K;
    opt.threads = 1;
    const Catalogue cat = build_catalogue(pts, s_max, opt);

    // ---- domaine declare -------------------------------------------------
    // Une coquille cospherique sort du modele (DESIGN.md §6.4) : la sphere est
    // rejetee, donc la fusion qu'elle porte manque a la foret alors que Gamma
    // la voit. Ces nuages ne sont pas comparables -- mais le rejet doit etre
    // DECLARE par l'implementation, pas seulement constate par l'oracle, sinon
    // une hierarchie incomplete sort en se disant autoritaire.
    if (degenerate > 0 || cat.degenerate_shells > 0 || cat.shell_anomalies > 0) {
      ++degenerate_total;
      if (degenerate > 0 && cat.degenerate_shells == 0)
        fail("DOMAINE degenerescence vue par l'oracle et pas par v2 trial="
             + std::to_string(trial));
      const Result r = run(pts, opt);
      if (!r.out_of_declared_domain || !r.forests_suppressed || !r.forests.empty())
        fail("DOMAINE nuage degenere publie quand meme trial=" + std::to_string(trial));
      continue;
    }
    ++trials;

    // ---- O1 : support, rang, membres, shell et niveau exact --------------
    std::map<std::vector<i32>, const CriticalSphere*> got;
    for (const CriticalSphere& s : cat.spheres) {
      std::vector<i32> sup(s.support, s.support + s.n_support);
      got.emplace(sup, &s);
    }
    if (got.size() != want.size())
      fail("O1 cardinal trial=" + std::to_string(trial) + " got="
           + std::to_string(got.size()) + " want=" + std::to_string(want.size()));
    for (const OracleSphere& w : want) {
      auto it = got.find(w.support);
      if (it == got.end()) { fail("O1 support manquant trial=" + std::to_string(trial)); continue; }
      const CriticalSphere& g = *it->second;
      if (g.rank != w.rank) fail("O1 rang trial=" + std::to_string(trial));
      std::vector<i32> mem(cat.members.begin() + g.members_begin,
                           cat.members.begin() + g.members_begin + g.rank);
      std::sort(mem.begin(), mem.end());
      if (mem != w.members) fail("O1 membres trial=" + std::to_string(trial));
      // niveau : |num|^2 / den^2 doit egaler r2 de l'oracle
      const i128 num2 = g.sph.nx * g.sph.nx + g.sph.ny * g.sph.ny + g.sph.nz * g.sph.nz;
      const i128 den2 = g.sph.den * g.sph.den;
      if (num2 * w.r2.d != w.r2.n * den2) fail("O1 niveau trial=" + std::to_string(trial));
    }
    for (const auto& kv : got) {
      bool present = false;
      for (const OracleSphere& w : want) if (w.support == kv.first) { present = true; break; }
      if (!present) fail("O1 support superflu trial=" + std::to_string(trial));
    }

    // ---- O2 : structure du merge tree ------------------------------------
    if (failures) break;
    for (int k = 1; k <= K; ++k) {
      // minima = spheres critiques de rang exactement k, identifiees par leurs
      // membres (ensemble de k points)
      std::vector<std::vector<i32>> minima;
      std::vector<Rat> minima_level;
      for (const OracleSphere& w : want)
        if (w.rank == k) { minima.push_back(w.members); minima_level.push_back(w.r2); }
      if (minima.empty()) continue;

      const Forest f = build_forest(pts, cat, k);
      if (!f.authoritative) {
        // foret censuree : l'oracle verifie seulement que rien n'est publie
        ++censored;
        continue;
      }

      // Une foret publiee n'a aucun noeud sans sphere source : c'est ce qui
      // rend son niveau lisible exactement.
      for (const ForestNode& nd : f.nodes)
        if (nd.source < 0 || nd.source >= static_cast<i32>(cat.spheres.size()))
          fail("O2 noeud sans sphere source trial=" + std::to_string(trial));
      if (failures) break;

      // index minimum -> noeud de naissance, par la sphere source du noeud
      std::map<std::vector<i32>, i32> node_by_members;
      for (size_t t = 0; t < f.nodes.size(); ++t) {
        if (f.nodes[t].kind != 0 || f.nodes[t].source < 0) continue;
        const CriticalSphere& cs = cat.spheres[static_cast<size_t>(f.nodes[t].source)];
        std::vector<i32> mem(cat.members.begin() + cs.members_begin,
                             cat.members.begin() + cs.members_begin + cs.rank);
        std::sort(mem.begin(), mem.end());
        node_by_members[mem] = static_cast<i32>(t);
      }
      std::vector<i32> node_of(minima.size(), -1);
      for (size_t i = 0; i < minima.size(); ++i) {
        auto it = node_by_members.find(minima[i]);
        node_of[i] = (it == node_by_members.end()) ? -1 : it->second;
        if (node_of[i] < 0) fail("O2 minimum sans noeud de naissance");
      }

      // niveaux testes : chaque niveau critique, et un point juste apres
      std::set<std::pair<i128, i128>> lv;
      for (const OracleSphere& w : want)
        if (w.rank <= k + 1) lv.emplace(w.r2.n, w.r2.d);
      for (const auto& L : lv) {
        const Rat a{L.first, L.second};
        const std::vector<int> pa = gamma_partition(pts, k, a, minima);
        if (overflowed) break;
        const double ad = static_cast<double>(a.n) / static_cast<double>(a.d);
        const std::vector<int> pb = forest_partition(f, cat, node_of, a);
        ++o2_cases;
        if (!same_partition(pa, pb))
          fail("O2 partition trial=" + std::to_string(trial) + " k=" + std::to_string(k)
               + " a=" + std::to_string(ad));
      }
      // Monotonie exacte des niveaux le long des bras, et arite des fusions :
      // deux multifusions consecutives ne peuvent pas partager un niveau (ce
      // serait une chaine binaire au lieu d'une multifusion).
      for (const ForestNode& nd : f.nodes) {
        if (nd.parent < 0) continue;
        const ForestNode& par = f.nodes[static_cast<size_t>(nd.parent)];
        const int c = rcmp(node_level(cat, nd), node_level(cat, par));
        if (c > 0)
          fail("O2 niveau decroissant vers le parent trial=" + std::to_string(trial));
        if (nd.kind == 1 && par.kind == 1 && c == 0)
          fail("O2 chaine de fusions de meme niveau trial=" + std::to_string(trial));
      }
    }
    if (failures) break;
  }

  std::printf("nuages compares : %d ; cas O2 : %d ; nuages hors domaine (coquille "
              "cospherique, rejet certifie) : %d ; forets censurees : %d\n",
              trials, o2_cases, degenerate_total, censored);
  if (failures) { std::printf("ECHEC : %d\n", failures); return 1; }
  std::printf("OK : oracle rationnel independant (catalogue + structure du merge tree)\n");
  return 0;
}
