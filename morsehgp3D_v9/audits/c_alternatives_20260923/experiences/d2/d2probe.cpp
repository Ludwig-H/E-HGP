// Auditeur C, piste D2 -- sonde de COMPTAGE de la generation par deletion locale
// (pavage rhomboidal borne au niveau Kmax-1) sur une fenetre de sites d'un nuage LiDAR.
// Hors produit, hors registre, public_status=not_claimed. OUTIL DE MESURE SEULEMENT :
// coordonnees x16 + gigue [0,15] (genericite pour Bowyer-Watson), jamais une voie exacte.
// Predicats orient/insphere exacts en i128 sur coordonnees gigue ; tests de boules minimales
// en long double (comptage).
//
// Usage : d2probe cloud.u32le Kmax windows window_sites ext_sites seed [M0]
// W = window_sites plus proches d'un germe aleatoire (proprietaires), W' = ext_sites plus proches
// (univers du BFS : J inclus dans W').
// Sortie : JSON sur stdout.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using i64 = long long;
using i128 = __int128;
using ld = long double;

struct P3 { i64 x, y, z; };

static std::vector<P3> pts;       // coordonnees gigue (x16 + [0,15])
static std::vector<std::array<ld, 3>> fpts;

static int sgn(i128 v) { return (v > 0) - (v < 0); }

static int orient(const P3& a, const P3& b, const P3& c, const P3& d) {
  const i128 bx = b.x - a.x, by = b.y - a.y, bz = b.z - a.z;
  const i128 cx = c.x - a.x, cy = c.y - a.y, cz = c.z - a.z;
  const i128 dx = d.x - a.x, dy = d.y - a.y, dz = d.z - a.z;
  const i128 det = bx * (cy * dz - cz * dy) - by * (cx * dz - cz * dx) + bz * (cx * dy - cy * dx);
  return sgn(det);
}

// > 0 si e strictement dans la sphere circonscrite de (a,b,c,d) positivement oriente.
static int insphere(const P3& a, const P3& b, const P3& c, const P3& d, const P3& e) {
  const P3* q[4] = {&a, &b, &c, &d};
  i128 m[4][4];
  for (int i = 0; i < 4; ++i) {
    const i128 x = q[i]->x - e.x, y = q[i]->y - e.y, z = q[i]->z - e.z;
    m[i][0] = x; m[i][1] = y; m[i][2] = z; m[i][3] = x * x + y * y + z * z;
  }
  auto det3 = [&](int r0, int r1, int r2, int c0, int c1, int c2) -> i128 {
    return m[r0][c0] * (m[r1][c1] * m[r2][c2] - m[r1][c2] * m[r2][c1])
         - m[r0][c1] * (m[r1][c0] * m[r2][c2] - m[r1][c2] * m[r2][c0])
         + m[r0][c2] * (m[r1][c0] * m[r2][c1] - m[r1][c1] * m[r2][c0]);
  };
  // developpement selon la derniere colonne (lift)
  const i128 dt = -m[0][3] * det3(1, 2, 3, 0, 1, 2) + m[1][3] * det3(0, 2, 3, 0, 1, 2)
                 - m[2][3] * det3(0, 1, 3, 0, 1, 2) + m[3][3] * det3(0, 1, 2, 0, 1, 2);
  return sgn(dt);
}
static int SIGN_IN = 0;

// ---------------------------------------------------------------- Bowyer-Watson local
struct Tet { int v[4]; bool alive; };  // v[3] == -1 : tetraedre infini (face v0 v1 v2)

struct LocalDel {
  const std::vector<int>* ids = nullptr;  // indices globaux
  std::vector<Tet> tets;
  long long predicate_calls = 0;
  bool degenerate = false;

  const P3& P(int l) const { return pts[(*ids)[l]]; }

  // conflit : +1 strictement dedans / au-dela de la face de bord
  int conflict(const Tet& t, const P3& e) {
    ++predicate_calls;
    if (t.v[3] < 0) {
      const int o = orient(P(t.v[0]), P(t.v[1]), P(t.v[2]), e);
      if (o == 0) degenerate = true;
      return o < 0 ? 1 : -1;  // cote fini : orient > 0
    }
    const int s = insphere(P(t.v[0]), P(t.v[1]), P(t.v[2]), P(t.v[3]), e) * SIGN_IN;
    if (s == 0) degenerate = true;
    return s;
  }

  bool build(const std::vector<int>& idv) {
    ids = &idv;
    tets.clear();
    const int m = (int)idv.size();
    if (m < 5) return false;
    // tetraedre initial
    int a = 0, b = 1, c = -1, d = -1;
    for (int i = 2; i < m && c < 0; ++i) {
      const P3 &pa = P(a), &pb = P(b), &pi = P(i);
      const i128 ux = pb.x - pa.x, uy = pb.y - pa.y, uz = pb.z - pa.z;
      const i128 vx = pi.x - pa.x, vy = pi.y - pa.y, vz = pi.z - pa.z;
      if (uy * vz - uz * vy != 0 || uz * vx - ux * vz != 0 || ux * vy - uy * vx != 0) c = i;
    }
    if (c < 0) return false;
    for (int i = 2; i < m && d < 0; ++i)
      if (i != c && orient(P(a), P(b), P(c), P(i)) != 0) d = i;
    if (d < 0) return false;
    if (orient(P(a), P(b), P(c), P(d)) < 0) std::swap(a, b);
    tets.push_back({{a, b, c, d}, true});
    // faces avec le sommet oppose du cote positif
    const int T[4] = {a, b, c, d};
    for (int k = 0; k < 4; ++k) {
      int f[3], n = 0;
      for (int j = 0; j < 4; ++j) if (j != k) f[n++] = T[j];
      if (orient(P(f[0]), P(f[1]), P(f[2]), P(T[k])) < 0) std::swap(f[0], f[1]);
      tets.push_back({{f[0], f[1], f[2], -1}, true});
    }
    std::vector<int> cavity;
    std::unordered_map<unsigned long long, std::pair<int, int>> faces;  // cle -> (tet, sommet oppose pos), compte
    for (int p = 0; p < m; ++p) {
      if (p == a || p == b || p == c || p == d) continue;
      const P3& e = P(p);
      cavity.clear();
      for (int t = 0; t < (int)tets.size(); ++t)
        if (tets[t].alive && conflict(tets[t], e) > 0) cavity.push_back(t);
      if (cavity.empty()) { degenerate = true; continue; }
      faces.clear();
      std::unordered_map<unsigned long long, int> count;
      std::vector<std::array<int, 3>> bnd;  // (tet, position remplacee, cle)
      std::vector<unsigned long long> keys;
      for (int t : cavity) {
        for (int k = 0; k < 4; ++k) {
          int f[3], n = 0;
          for (int j = 0; j < 4; ++j) if (j != k) f[n++] = tets[t].v[j];
          std::sort(f, f + 3);
          const unsigned long long key = ((unsigned long long)(f[0] + 1) << 42) | ((unsigned long long)(f[1] + 1) << 21) | (unsigned long long)(f[2] + 1);
          ++count[key];
          bnd.push_back({t, k, 0});
          keys.push_back(key);
        }
      }
      std::vector<Tet> fresh;
      for (std::size_t i = 0; i < bnd.size(); ++i) {
        if (count[keys[i]] != 1) continue;
        const Tet& old = tets[bnd[i][0]];
        const int k = bnd[i][1];
        Tet nt = old;
        nt.v[k] = p;
        if (old.v[3] < 0 && k == 3) {
          // face finie d'un tet infini : nouveau tet fini (a b c p)
          if (orient(P(nt.v[0]), P(nt.v[1]), P(nt.v[2]), P(nt.v[3])) < 0) std::swap(nt.v[0], nt.v[1]);
        } else if (old.v[3] < 0) {
          // face infinie : nouveau tet infini (f0 f1 p) ; sommet remplace old.v[k] cote fini
          int f[3], n = 0;
          for (int j = 0; j < 3; ++j) f[n++] = nt.v[j];
          const int o = orient(P(f[0]), P(f[1]), P(f[2]), P(old.v[k]));
          if (o == 0) degenerate = true;
          if (o < 0) std::swap(f[0], f[1]);
          nt.v[0] = f[0]; nt.v[1] = f[1]; nt.v[2] = f[2]; nt.v[3] = -1;
        } else {
          const int o = orient(P(nt.v[0]), P(nt.v[1]), P(nt.v[2]), P(nt.v[3]));
          if (o == 0) degenerate = true;
          if (o < 0) std::swap(nt.v[0], nt.v[1]);
        }
        fresh.push_back(nt);
      }
      for (int t : cavity) tets[t].alive = false;
      // compaction periodique
      for (auto& t : fresh) tets.push_back(t);
      if (tets.size() > 4096) {
        std::vector<Tet> keep;
        for (auto& t : tets) if (t.alive) keep.push_back(t);
        tets.swap(keep);
      }
    }
    std::vector<Tet> keep;
    for (auto& t : tets) if (t.alive) keep.push_back(t);
    tets.swap(keep);
    return true;
  }
};

// ---------------------------------------------------------------- grille kNN
struct Grid {
  i64 cell = 0;
  i64 ox = 0, oy = 0, oz = 0;
  std::unordered_map<unsigned long long, std::vector<int>> cells;
  static unsigned long long key(i64 x, i64 y, i64 z) {
    return ((unsigned long long)(x & 0x1FFFFF) << 42) | ((unsigned long long)(y & 0x1FFFFF) << 21) | (unsigned long long)(z & 0x1FFFFF);
  }
  void build(i64 c) {
    cell = c;
    for (int i = 0; i < (int)pts.size(); ++i) cells[key(pts[i].x / cell, pts[i].y / cell, pts[i].z / cell)].push_back(i);
  }
  // k plus proches de (fx,fy,fz) hors exclus ; renvoie aussi le rayon certifie (tous les points
  // strictement plus proches que rc sont dans la liste)
  std::vector<int> knn(ld fx, ld fy, ld fz, int k, const std::vector<int>& excl, ld& rc) const {
    const i64 cx = (i64)std::floor(fx / cell), cy = (i64)std::floor(fy / cell), cz = (i64)std::floor(fz / cell);
    std::vector<std::pair<ld, int>> cand;
    for (int r = 0;; ++r) {
      for (i64 x = cx - r; x <= cx + r; ++x)
        for (i64 y = cy - r; y <= cy + r; ++y)
          for (i64 z = cz - r; z <= cz + r; ++z) {
            if (std::max({std::llabs(x - cx), std::llabs(y - cy), std::llabs(z - cz)}) != r) continue;
            auto it = cells.find(key(x, y, z));
            if (it == cells.end()) continue;
            for (int i : it->second) {
              if (std::find(excl.begin(), excl.end(), i) != excl.end()) continue;
              const ld dx = fpts[i][0] - fx, dy = fpts[i][1] - fy, dz = fpts[i][2] - fz;
              cand.push_back({std::sqrt(dx * dx + dy * dy + dz * dz), i});
            }
          }
      // rayon garanti couvert par les anneaux 0..r : r * cell
      const ld covered = (ld)r * (ld)cell;
      if ((int)cand.size() >= k + 1) {
        std::nth_element(cand.begin(), cand.begin() + k, cand.end());
        if (cand[k].first <= covered) {
          std::sort(cand.begin(), cand.begin() + k + 1);
          rc = cand[k].first;
          std::vector<int> out;
          for (int i = 0; i < k; ++i) out.push_back(cand[i].second);
          return out;
        }
      }
      if (r > 4000) {
        std::sort(cand.begin(), cand.end());
        rc = 1e30L;
        std::vector<int> out;
        for (auto& c : cand) out.push_back(c.second);
        return out;
      }
    }
  }
};

// ---------------------------------------------------------------- geometrie flottante
struct Ball { ld c[3]; ld r2; bool ok; };

static Ball circum(const std::vector<int>& F) {
  // centre dans aff(F), barycentriques > 0 exige par l'appelant
  Ball b{};
  const auto& p0 = fpts[F[0]];
  const int k = (int)F.size() - 1;
  ld v[3][3];
  for (int i = 0; i < k; ++i) for (int t = 0; t < 3; ++t) v[i][t] = fpts[F[i + 1]][t] - p0[t];
  ld G[3][3], r[3];
  for (int i = 0; i < k; ++i) {
    for (int j = 0; j < k; ++j) G[i][j] = v[i][0] * v[j][0] + v[i][1] * v[j][1] + v[i][2] * v[j][2];
    r[i] = 0.5L * G[i][i];
  }
  // Gauss
  ld lam[3] = {0, 0, 0};
  ld A[3][4];
  for (int i = 0; i < k; ++i) { for (int j = 0; j < k; ++j) A[i][j] = G[i][j]; A[i][k] = r[i]; }
  for (int col = 0; col < k; ++col) {
    int piv = col;
    for (int i = col + 1; i < k; ++i) if (std::fabs(A[i][col]) > std::fabs(A[piv][col])) piv = i;
    if (std::fabs(A[piv][col]) < 1e-30L) { b.ok = false; return b; }
    for (int j = 0; j <= k; ++j) std::swap(A[col][j], A[piv][j]);
    for (int i = 0; i < k; ++i) if (i != col) {
      const ld f = A[i][col] / A[col][col];
      for (int j = col; j <= k; ++j) A[i][j] -= f * A[col][j];
    }
  }
  ld sum = 0;
  for (int i = 0; i < k; ++i) { lam[i] = A[i][k] / A[i][i]; sum += lam[i]; }
  b.ok = (1 - sum) > 0;
  for (int i = 0; i < k; ++i) b.ok = b.ok && lam[i] > 0;
  for (int t = 0; t < 3; ++t) { b.c[t] = p0[t]; for (int i = 0; i < k; ++i) b.c[t] += lam[i] * v[i][t]; }
  b.r2 = 0;
  for (int t = 0; t < 3; ++t) b.r2 += (b.c[t] - p0[t]) * (b.c[t] - p0[t]);
  return b;
}

static ld dist2(const ld* c, int i) {
  ld s = 0;
  for (int t = 0; t < 3; ++t) s += (fpts[i][t] - c[t]) * (fpts[i][t] - c[t]);
  return s;
}

// ---------------------------------------------------------------- ensembles
struct VecHash {
  std::size_t operator()(const std::vector<int>& v) const {
    unsigned long long h = 1469598103934665603ull ^ v.size();
    for (int x : v) { h ^= (unsigned)x; h *= 1099511628211ull; }
    return (std::size_t)h;
  }
};
using SetOfSets = std::unordered_set<std::vector<int>, VecHash>;

static std::vector<int> read_u32le(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::invalid_argument("cannot open input");
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::vector<int> raw(bytes.size() / 4);
  for (std::size_t i = 0; i < raw.size(); ++i) {
    unsigned v = 0;
    for (int b = 0; b < 4; ++b) v |= unsigned(bytes[i * 4 + b]) << (8 * b);
    raw[i] = (int)v;
  }
  return raw;
}

int main(int argc, char** argv) {
  if (argc < 7) { std::fprintf(stderr, "usage: d2probe cloud Kmax windows window_sites ext_sites seed [M0]\n"); return 2; }
  const auto raw = read_u32le(argv[1]);
  const int kmax = std::stoi(argv[2]);
  const int nwin = std::stoi(argv[3]);
  const int wsites = std::stoi(argv[4]);
  const ld ext = std::stold(argv[5]) * 16.0L;
  const unsigned seed = (unsigned)std::stoul(argv[6]);
  const int M0 = argc > 7 ? std::stoi(argv[7]) : 40;
  const int pmax = kmax - 1;
  std::mt19937_64 rng(seed);
  const int n = (int)raw.size() / 3;
  pts.resize(n);
  fpts.resize(n);
  for (int i = 0; i < n; ++i) {
    pts[i] = {(i64)raw[3 * i] * 16 + (i64)(rng() & 15), (i64)raw[3 * i + 1] * 16 + (i64)(rng() & 15), (i64)raw[3 * i + 2] * 16 + (i64)(rng() & 15)};
    fpts[i] = {(ld)pts[i].x, (ld)pts[i].y, (ld)pts[i].z};
  }
  {
    P3 a{0, 0, 0}, b{64, 0, 0}, c{0, 64, 0}, d{0, 0, 64}, e{8, 8, 8};
    if (orient(a, b, c, d) < 0) std::swap(a, b);
    SIGN_IN = insphere(a, b, c, d, e);
    P3 f{200, 200, 200};
    if (SIGN_IN == 0 || insphere(a, b, c, d, f) * SIGN_IN >= 0) { std::fprintf(stderr, "calibration\n"); return 3; }
  }
  Grid grid;
  grid.build(16 * 100);  // cellules de 10 cm
  const auto t0 = std::chrono::steady_clock::now();

  // statistiques par niveau p = |J|
  std::vector<long long> nJ(pmax + 1, 0), ownedJ(pmax + 1, 0), cr(pmax + 1, 0), rt(pmax + 1, 0),
      faces(pmax + 1, 0), crit(pmax + 1, 0), ownedCrit(pmax + 1, 0), children(pmax + 1, 0),
      localPts(pmax + 1, 0), uncert(pmax + 1, 0), redo(pmax + 1, 0), ownedJwithCrit(pmax + 1, 0);
  std::vector<std::array<long long, 5>> ownedCritQ(pmax + 1, std::array<long long, 5>{});
  long long preds = 0, degenerate_events = 0, owned_sites = 0;
  std::vector<long long> histM(12, 0);
  LocalDel del;

  for (int w = 0; w < nwin; ++w) {
    const int seedSite = (int)(rng() % (unsigned long long)n);
    ld rc0;
    std::vector<int> none;
    const auto W = grid.knn(fpts[seedSite][0], fpts[seedSite][1], fpts[seedSite][2], wsites, none, rc0);
    std::unordered_set<int> inW(W.begin(), W.end());
    owned_sites += (long long)W.size();
    // W' = les ext sites les plus proches du germe (contient W)
    std::unordered_set<int> inWx;
    {
      ld rc;
      auto nb = grid.knn(fpts[seedSite][0], fpts[seedSite][1], fpts[seedSite][2], (int)(ext / 16), none, rc);
      for (int i : nb) inWx.insert(i);
      for (int s : W) inWx.insert(s);
    }
    // proprietaire : plus petit indice de J (J trie)
    auto owned = [&](const std::vector<int>& J) { return inW.count(J[0]) > 0; };
    std::vector<SetOfSets> level(pmax + 1);
    // niveau 0 : etoiles de Del(P) des sites de W' -> faces critiques p = 0 et enfants
    {
      SetOfSets seenF;
      for (int a : inWx) {
        std::vector<int> excl;
        int M = M0;
        for (int attempt = 0; attempt < 3; ++attempt, M *= 2) {
          ld rc;
          auto C = grid.knn(fpts[a][0], fpts[a][1], fpts[a][2], M, excl, rc);
          if (std::find(C.begin(), C.end(), a) == C.end()) C.push_back(a);
          if (!del.build(C)) break;
          if (del.degenerate) ++degenerate_events;
          const int la = (int)(std::find(C.begin(), C.end(), a) - C.begin());
          bool certified = true;
          std::vector<const Tet*> star;
          for (const auto& t : del.tets) {
            if (!(t.v[0] == la || t.v[1] == la || t.v[2] == la || t.v[3] == la)) continue;
            if (t.v[3] < 0) { certified = false; break; }
            std::vector<int> Fv = {C[t.v[0]], C[t.v[1]], C[t.v[2]], C[t.v[3]]};
            // sphere circonscrite
            const auto& p0 = fpts[Fv[0]];
            // centre par resolution 3x3
            ld A[3][4];
            for (int i = 0; i < 3; ++i) {
              for (int tt = 0; tt < 3; ++tt) A[i][tt] = fpts[Fv[i + 1]][tt] - p0[tt];
              A[i][3] = 0.5L * (A[i][0] * A[i][0] + A[i][1] * A[i][1] + A[i][2] * A[i][2]);
            }
            for (int col = 0; col < 3; ++col) {
              int piv = col;
              for (int i = col + 1; i < 3; ++i) if (std::fabs(A[i][col]) > std::fabs(A[piv][col])) piv = i;
              for (int j = 0; j < 4; ++j) std::swap(A[col][j], A[piv][j]);
              for (int i = 0; i < 3; ++i) if (i != col) { const ld f = A[i][col] / A[col][col]; for (int j = col; j < 4; ++j) A[i][j] -= f * A[col][j]; }
            }
            ld c[3];
            for (int tt = 0; tt < 3; ++tt) c[tt] = p0[tt] + A[tt][3] / A[tt][tt];
            const ld rr = std::sqrt(dist2(c, Fv[0]));
            const ld dc = std::sqrt((c[0] - fpts[a][0]) * (c[0] - fpts[a][0]) + (c[1] - fpts[a][1]) * (c[1] - fpts[a][1]) + (c[2] - fpts[a][2]) * (c[2] - fpts[a][2]));
            if (dc + rr >= rc) { certified = false; break; }
            star.push_back(&t);
          }
          if (!certified && attempt < 2) { ++redo[0]; continue; }
          if (!certified) ++uncert[0];
          ++histM[std::min(11, (int)std::log2((double)M))];
          for (const Tet* t : star) {
            std::vector<int> Tg = {C[t->v[0]], C[t->v[1]], C[t->v[2]], C[t->v[3]]};
            std::sort(Tg.begin(), Tg.end());
            // enfants : sous-ensembles contenant a
            for (int mask = 1; mask < 16; ++mask) {
              std::vector<int> J;
              bool hasA = false, inside = true;
              for (int j = 0; j < 4; ++j) if (mask >> j & 1) { J.push_back(Tg[j]); hasA |= Tg[j] == a; inside &= inWx.count(Tg[j]) > 0; }
              if (!hasA || !inside || (int)J.size() > pmax) continue;
              level[J.size()].insert(J);
            }
            // faces critiques p = 0
            for (int mask = 3; mask < 16; ++mask) {
              const int q = __builtin_popcount(mask);
              if (q < 2) continue;
              std::vector<int> Fv;
              for (int j = 0; j < 4; ++j) if (mask >> j & 1) Fv.push_back(Tg[j]);
              if (std::find(Fv.begin(), Fv.end(), a) == Fv.end()) continue;
              if (!seenF.insert(Fv).second) continue;
              ++faces[0];
              const Ball b = circum(Fv);
              if (!b.ok) continue;
              bool empty = true;
              for (int i : C) {
                if (std::find(Fv.begin(), Fv.end(), i) != Fv.end()) continue;
                if (dist2(b.c, i) < b.r2 * (1 - 1e-15L)) { empty = false; break; }
              }
              if (!empty) continue;
              if (q > kmax + 1) continue;
              ++crit[0];
              if (inW.count(Fv[0])) { ++ownedCrit[0]; ++ownedCritQ[0][q]; }
            }
          }
          break;
        }
      }
    }
    // niveaux 1..pmax
    for (int p = 1; p <= pmax; ++p) {
      std::vector<std::vector<int>> todo(level[p].begin(), level[p].end());
      std::sort(todo.begin(), todo.end());
      for (const auto& J : todo) {
        ++nJ[p];
        const bool own = owned(J);
        if (own) ++ownedJ[p];
        ld o[3] = {0, 0, 0};
        for (int j : J) for (int t = 0; t < 3; ++t) o[t] += fpts[j][t] / (ld)J.size();
        int M = M0 + 4 * p;
        for (int attempt = 0; attempt < 3; ++attempt, M *= 2) {
          ld rc;
          auto C = grid.knn(o[0], o[1], o[2], M, J, rc);
          if (!del.build(C)) break;
          if (del.degenerate) ++degenerate_events;
          preds += del.predicate_calls;
          del.predicate_calls = 0;
          // region de conflit
          std::vector<int> crIdx, rtIdx;
          for (int t = 0; t < (int)del.tets.size(); ++t) {
            int nin = 0;
            for (int j : J) if (del.conflict(del.tets[t], pts[j]) > 0) ++nin;
            if (nin > 0) crIdx.push_back(t);
            if (nin == (int)J.size()) rtIdx.push_back(t);
          }
          // certification : spheres des tets de conflit dans la boule certifiee
          bool certified = true;
          for (int t : crIdx) {
            const Tet& T = del.tets[t];
            if (T.v[3] < 0) { certified = false; break; }
            std::vector<int> Fv = {C[T.v[0]], C[T.v[1]], C[T.v[2]], C[T.v[3]]};
            const auto& p0 = fpts[Fv[0]];
            ld A[3][4];
            for (int i = 0; i < 3; ++i) {
              for (int tt = 0; tt < 3; ++tt) A[i][tt] = fpts[Fv[i + 1]][tt] - p0[tt];
              A[i][3] = 0.5L * (A[i][0] * A[i][0] + A[i][1] * A[i][1] + A[i][2] * A[i][2]);
            }
            for (int col = 0; col < 3; ++col) {
              int piv = col;
              for (int i = col + 1; i < 3; ++i) if (std::fabs(A[i][col]) > std::fabs(A[piv][col])) piv = i;
              for (int j = 0; j < 4; ++j) std::swap(A[col][j], A[piv][j]);
              for (int i = 0; i < 3; ++i) if (i != col) { const ld f = A[i][col] / A[col][col]; for (int j = col; j < 4; ++j) A[i][j] -= f * A[col][j]; }
            }
            ld c[3];
            for (int tt = 0; tt < 3; ++tt) c[tt] = p0[tt] + A[tt][3] / A[tt][tt];
            const ld rr = std::sqrt(dist2(c, Fv[0]));
            const ld dc = std::sqrt((c[0] - o[0]) * (c[0] - o[0]) + (c[1] - o[1]) * (c[1] - o[1]) + (c[2] - o[2]) * (c[2] - o[2]));
            if (dc + rr >= rc) { certified = false; break; }
          }
          if (!certified && attempt < 2) { ++redo[p]; continue; }
          if (!certified) ++uncert[p];
          ++histM[std::min(11, (int)std::log2((double)M))];
          localPts[p] += (long long)C.size();
          cr[p] += (long long)crIdx.size();
          rt[p] += (long long)rtIdx.size();
          // enfants
          for (int t : rtIdx) {
            const Tet& T = del.tets[t];
            std::vector<int> fv;
            for (int j = 0; j < 4; ++j) if (T.v[j] >= 0) fv.push_back(C[T.v[j]]);
            const int nf = (int)fv.size();
            for (int mask = 1; mask < (1 << nf); ++mask) {
              const int r = __builtin_popcount(mask);
              if (p + r > pmax) continue;
              std::vector<int> K = J;
              bool inside = true;
              for (int j = 0; j < nf; ++j) if (mask >> j & 1) { K.push_back(fv[j]); inside &= inWx.count(fv[j]) > 0; }
              if (!inside) continue;
              std::sort(K.begin(), K.end());
              ++children[p];
              level[p + r].insert(K);
            }
          }
          // faces critiques d'interieur exactement J
          SetOfSets seenF;
          bool anyCrit = false;
          for (int t : crIdx) {
            const Tet& T = del.tets[t];
            std::vector<int> fv;
            for (int j = 0; j < 4; ++j) if (T.v[j] >= 0) fv.push_back(C[T.v[j]]);
            std::sort(fv.begin(), fv.end());
            const int nf = (int)fv.size();
            for (int mask = 1; mask < (1 << nf); ++mask) {
              const int q = __builtin_popcount(mask);
              if (q < 2 || p + q > kmax + 1) continue;
              std::vector<int> Fv;
              for (int j = 0; j < nf; ++j) if (mask >> j & 1) Fv.push_back(fv[j]);
              if (!seenF.insert(Fv).second) continue;
              ++faces[p];
              const Ball b = circum(Fv);
              if (!b.ok) continue;
              bool allIn = true;
              for (int j : J) if (!(dist2(b.c, j) < b.r2 * (1 - 1e-15L))) { allIn = false; break; }
              if (!allIn) continue;
              bool empty = true;
              for (int i : C) {
                if (std::find(Fv.begin(), Fv.end(), i) != Fv.end()) continue;
                if (dist2(b.c, i) < b.r2 * (1 - 1e-15L)) { empty = false; break; }
              }
              if (!empty) continue;
              ++crit[p];
              anyCrit = true;
              if (own) { ++ownedCrit[p]; ++ownedCritQ[p][q]; }
            }
          }
          if (own && anyCrit) ++ownedJwithCrit[p];
          break;
        }
      }
    }
  }
  const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  auto arr = [&](const char* name, const std::vector<long long>& v, bool last = false) {
    std::printf("\"%s\":[", name);
    for (std::size_t i = 0; i < v.size(); ++i) std::printf("%s%lld", i ? "," : "", v[i]);
    std::printf("]%s", last ? "" : ",");
  };
  std::printf("{\"file\":\"%s\",\"n\":%d,\"kmax\":%d,\"windows\":%d,\"window_sites\":%d,\"ext_sites\":%.0Lf,\"seed\":%u,\"M0\":%d,\"owned_sites\":%lld,\"wall_s\":%.2f,",
              argv[1], n, kmax, nwin, wsites, ext / 16, seed, M0, owned_sites, wall);
  arr("processed_J", nJ); arr("owned_J", ownedJ); arr("owned_J_with_critical", ownedJwithCrit);
  arr("conflict_tets", cr); arr("rhomboid_tets", rt);
  arr("faces_tested", faces); arr("critical_all", crit); arr("critical_owned", ownedCrit);
  std::printf("\"critical_owned_by_q\":[");
  for (int p = 0; p <= pmax; ++p) std::printf("%s[%lld,%lld,%lld]", p ? "," : "", ownedCritQ[p][2], ownedCritQ[p][3], ownedCritQ[p][4]);
  std::printf("],");
  arr("children_generated", children); arr("local_points", localPts); arr("recertify_redo", redo); arr("uncertified", uncert);
  arr("hist_log2_M", histM);
  std::printf("\"insphere_calls\":%lld,\"degenerate_events\":%lld}\n", preds, degenerate_events);
  return 0;
}
