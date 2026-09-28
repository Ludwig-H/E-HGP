// cble.cpp -- prototype d'audit (lentille L13), hors depot.
// Enumeration locale par BOITES DE CENTRES des boules bien centrees
// (centre dans relint conv(U_min)) avec |I| + q_min <= K+1, q_min in {2,3,4}.
// Certificat de completude : lemme des gardes. Pour toute boule admissible de
// centre c dans Q, la boule ouverte contient <= K-1 sites, donc parmi K sites
// distincts quelconques S0 il existe y avec |y-c| >= r >= |x-c| pour tout
// x du support, de la coquille et de l'interieur : x passe le test affine
// min_{c' in Q} (|x-c'|^2 - |y-c'|^2) <= 0. Aucun front WSPD, aucun filtre
// temoin, aucun atlas. Arithmetique exacte i64/i128 pour u16 et u18.
// Mode --oracle : enumeration brute globale (n petit) pour juger le prototype.
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

using i64 = long long;
using i128 = __int128;
struct Pt { i64 x, y, z; };

static int K = 5;
static int MMAX = 24;
static std::vector<Pt> P;

struct Stats {
  std::array<std::array<uint64_t, 16>, 5> by_q_p{};  // [q][p]
  uint64_t extra_shell = 0, max_shell = 0;
  uint64_t leaves = 0, sum_m = 0, max_m = 0, nodes = 0, skipped_bbox = 0;
  uint64_t pair_tests = 0, triple_tests = 0, line_hits = 0, quad_tests = 0, quad_center_in = 0;
  uint64_t guard_tests = 0;
  void add(const Stats& o) {
    for (int q = 0; q < 5; ++q) for (int p = 0; p < 16; ++p) by_q_p[q][p] += o.by_q_p[q][p];
    extra_shell += o.extra_shell; max_shell = std::max(max_shell, o.max_shell);
    leaves += o.leaves; sum_m += o.sum_m; max_m = std::max(max_m, o.max_m); nodes += o.nodes;
    skipped_bbox += o.skipped_bbox; pair_tests += o.pair_tests; triple_tests += o.triple_tests;
    line_hits += o.line_hits; quad_tests += o.quad_tests; quad_center_in += o.quad_center_in;
    guard_tests += o.guard_tests;
  }
};

struct Box { i64 lo[3], hi[3]; };  // demi-ouverte [lo,hi) pour la propriete des centres

static inline i128 dot3(const i128* a, const i128* b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

// Centre rationnel N/D (D>0) ; test d'appartenance a [lo,hi) exact.
static inline bool center_in_box(const i128 N[3], i128 D, const Box& Q) {
  for (int i = 0; i < 3; ++i) {
    if (N[i] < (i128)Q.lo[i] * D) return false;
    if (N[i] >= (i128)Q.hi[i] * D) return false;
  }
  return true;
}

// Cle canonique d'une boule : centre reduit + rayon carre au meme denominateur.
struct Key {
  i128 n[3], d, r;
  bool operator<(const Key& o) const {
    for (int i = 0; i < 3; ++i) if (n[i] != o.n[i]) return n[i] < o.n[i];
    if (d != o.d) return d < o.d;
    return r < o.r;
  }
};
static i128 gcd128(i128 a, i128 b) { if (a < 0) a = -a; if (b < 0) b = -b; while (b) { i128 t = a % b; a = b; b = t; } return a; }
static Key make_key(const i128 N[3], i128 D, const Pt& a) {
  i128 g = D;
  for (int i = 0; i < 3; ++i) g = gcd128(g, N[i]);
  Key k;
  for (int i = 0; i < 3; ++i) k.n[i] = N[i] / g;
  k.d = D / g;
  i128 dx = k.d * a.x - k.n[0], dy = k.d * a.y - k.n[1], dz = k.d * a.z - k.n[2];
  k.r = dx * dx + dy * dy + dz * dz;
  return k;
}

// Classement d'un site z par rapport a la boule de centre N/D passant par a :
// signe de D|z-a|^2 - 2 (N - D a).(z-a) : <0 interieur, 0 coquille.
static inline int side(const i128 Nrel[3], i128 D, const Pt& a, const Pt& z) {
  i128 zx = z.x - a.x, zy = z.y - a.y, zz = z.z - a.z;
  i128 lhs = D * (zx * zx + zy * zy + zz * zz);
  i128 rhs = 2 * (Nrel[0] * zx + Nrel[1] * zy + Nrel[2] * zz);
  return lhs < rhs ? -1 : (lhs == rhs ? 0 : 1);
}

static i128 orient(const Pt& p, const Pt& q, const Pt& r, const i128 X[3], i128 D) {
  // det[q-p, r-p, X/D - p] * D
  i128 a0 = q.x - p.x, a1 = q.y - p.y, a2 = q.z - p.z;
  i128 b0 = r.x - p.x, b1 = r.y - p.y, b2 = r.z - p.z;
  i128 c0 = X[0] - D * p.x, c1 = X[1] - D * p.y, c2 = X[2] - D * p.z;
  // (a x b) . c, a,b petits
  i128 w0 = a1 * b2 - a2 * b1, w1 = a2 * b0 - a0 * b2, w2 = a0 * b1 - a1 * b0;
  return w0 * c0 + w1 * c1 + w2 * c2;
}
static i128 orient_pt(const Pt& p, const Pt& q, const Pt& r, const Pt& s) {
  i128 X[3] = {s.x, s.y, s.z};
  return orient(p, q, r, X, 1);
}

// q_min d'une coquille etendue (rare) : plus petit sous-ensemble affinement
// independant dont l'interieur relatif contient le centre.
static int qmin_of_shell(const std::vector<int>& sh, const i128 N[3], i128 D) {
  int m = (int)sh.size();
  // q2 : paire antipodale (milieu == centre)
  for (int i = 0; i < m; ++i) for (int j = i + 1; j < m; ++j) {
    const Pt &a = P[sh[i]], &b = P[sh[j]];
    if ((i128)(a.x + b.x) * D == 2 * N[0] && (i128)(a.y + b.y) * D == 2 * N[1] && (i128)(a.z + b.z) * D == 2 * N[2]) return 2;
  }
  // q3 : triangle aigu dont le plan contient le centre (alors centre = circumcentre, coplanaire)
  for (int i = 0; i < m; ++i) for (int j = i + 1; j < m; ++j) for (int k = j + 1; k < m; ++k) {
    const Pt &a = P[sh[i]], &b = P[sh[j]], &c = P[sh[k]];
    i64 ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z, vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;
    i64 wx = uy * vz - uz * vy, wy = uz * vx - ux * vz, wz = ux * vy - uy * vx;
    if (wx == 0 && wy == 0 && wz == 0) continue;
    if (orient(a, b, c, N, D) != 0) continue;  // centre hors du plan
    // centre dans l'interieur relatif du triangle : aigu strict
    auto dt = [](const Pt& o, const Pt& p1, const Pt& p2) { return (p1.x - o.x) * (p2.x - o.x) + (p1.y - o.y) * (p2.y - o.y) + (p1.z - o.z) * (p2.z - o.z); };
    if (dt(a, b, c) > 0 && dt(b, a, c) > 0 && dt(c, a, b) > 0) return 3;
  }
  return 4;
}

struct Local {
  Stats st;
  std::set<Key> degenerate_seen;  // cle des boules a coquille etendue, par feuille
};

// Evalue une boule candidate (centre N/D, passe par a) sur la liste N(Q).
// qgen : arite de la presentation. Retourne apres enregistrement eventuel.
static void judge_ball(Local& L, const std::vector<int>& cand, const i128 N[3], i128 D, int ia, int qgen) {
  const Pt& a = P[ia];
  i128 Nrel[3] = {N[0] - D * a.x, N[1] - D * a.y, N[2] - D * a.z};
  int p = 0;
  int shell = 0;
  static thread_local std::vector<int> sh;
  sh.clear();
  for (int z : cand) {
    int s = side(Nrel, D, a, P[z]);
    if (s < 0) { if (++p >= K) return; }  // p >= K : rejet sur (lemme des gardes : compte exact si < K)
    else if (s == 0) { ++shell; sh.push_back(z); }
  }
  int q = qgen;
  if (shell > qgen) {
    q = qmin_of_shell(sh, N, D);
    if (p + q > K + 1) return;
    Key key = make_key(N, D, a);
    if (!L.degenerate_seen.insert(key).second) return;
    ++L.st.extra_shell;
  } else {
    if (p + q > K + 1) return;
  }
  L.st.max_shell = std::max<uint64_t>(L.st.max_shell, shell);
  L.st.by_q_p[q][p]++;
}

static bool g_prune = true;
static inline bool bisector_meets(const Pt& a, const Pt& b, const Box& Q) {
  // f(c) = |a|^2-|b|^2-2c.(a-b) : affine ; le plan bissecteur coupe la boite fermee ssi min<=0<=max
  i64 dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
  i128 base = (i128)(a.x * a.x + a.y * a.y + a.z * a.z) - (b.x * b.x + b.y * b.y + b.z * b.z);
  i128 smax = (i128)(dx > 0 ? Q.hi[0] : Q.lo[0]) * dx + (i128)(dy > 0 ? Q.hi[1] : Q.lo[1]) * dy + (i128)(dz > 0 ? Q.hi[2] : Q.lo[2]) * dz;
  i128 smin = (i128)(dx > 0 ? Q.lo[0] : Q.hi[0]) * dx + (i128)(dy > 0 ? Q.lo[1] : Q.hi[1]) * dy + (i128)(dz > 0 ? Q.lo[2] : Q.hi[2]) * dz;
  i128 fmin = base - 2 * smax, fmax = base - 2 * smin;
  return fmin <= 0 && 0 <= fmax;
}
static void enumerate_leaf(Local& L, const std::vector<int>& c, const Box& Q) {
  const int m = (int)c.size();
  Stats& st = L.st;
  static thread_local std::vector<unsigned char> meets;
  meets.assign((size_t)m * m, 1);
  if (g_prune)
    for (int i = 0; i < m; ++i) for (int j = i + 1; j < m; ++j) {
      bool b = bisector_meets(P[c[i]], P[c[j]], Q);
      meets[(size_t)i * m + j] = meets[(size_t)j * m + i] = b;
    }
  auto M_ = [&](int i, int j) { return meets[(size_t)i * m + j] != 0; };
  // ---- q2 ----
  for (int i = 0; i < m; ++i) {
    const Pt& a = P[c[i]];
    for (int j = i + 1; j < m; ++j) {
      const Pt& b = P[c[j]];
      if (!M_(i, j)) continue;
      ++st.pair_tests;
      i128 N[3] = {a.x + b.x, a.y + b.y, a.z + b.z};
      if (!center_in_box(N, 2, Q)) continue;
      judge_ball(L, c, N, 2, c[i], 2);
    }
  }
  // ---- q3 et q4 (droites des triplets) ----
  const double lo[3] = {(double)Q.lo[0], (double)Q.lo[1], (double)Q.lo[2]};
  const double hi[3] = {(double)Q.hi[0], (double)Q.hi[1], (double)Q.hi[2]};
  const double side_len = hi[0] - lo[0];
  const double marg = 1e-6 * side_len + 1e-3;
  for (int i = 0; i < m; ++i) {
    const Pt& a = P[c[i]];
    for (int j = i + 1; j < m; ++j) {
      if (!M_(i, j)) continue;
      const Pt& b = P[c[j]];
      i64 ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
      i64 uu = ux * ux + uy * uy + uz * uz;
      for (int k = j + 1; k < m; ++k) {
        if (!M_(i, k) || !M_(j, k)) continue;
        const Pt& cc = P[c[k]];
        ++st.triple_tests;
        i64 vx = cc.x - a.x, vy = cc.y - a.y, vz = cc.z - a.z;
        i64 wx = uy * vz - uz * vy, wy = uz * vx - ux * vz, wz = ux * vy - uy * vx;
        if (wx == 0 && wy == 0 && wz == 0) continue;
        i64 vv = vx * vx + vy * vy + vz * vz;
        // circumcentre (relatif a a) : ((uu v - vv u) x w) / (2|w|^2)
        i128 tx = (i128)uu * vx - (i128)vv * ux, ty = (i128)uu * vy - (i128)vv * uy, tz = (i128)uu * vz - (i128)vv * uz;
        i128 N3x = ty * wz - tz * wy, N3y = tz * wx - tx * wz, N3z = tx * wy - ty * wx;
        i128 D3 = 2 * ((i128)wx * wx + (i128)wy * wy + (i128)wz * wz);
        double cx = a.x + (double)N3x / (double)D3, cy = a.y + (double)N3y / (double)D3, cz = a.z + (double)N3z / (double)D3;
        // ---- q3 : triangle aigu strict, centre dans la boite ----
        {
          i64 d1 = ux * vx + uy * vy + uz * vz;  // angle en a
          i64 d2 = (a.x - b.x) * (cc.x - b.x) + (a.y - b.y) * (cc.y - b.y) + (a.z - b.z) * (cc.z - b.z);
          i64 d3 = (a.x - cc.x) * (b.x - cc.x) + (a.y - cc.y) * (b.y - cc.y) + (a.z - cc.z) * (b.z - cc.z);
          if (d1 > 0 && d2 > 0 && d3 > 0 && cx >= lo[0] - marg && cx < hi[0] + marg && cy >= lo[1] - marg &&
              cy < hi[1] + marg && cz >= lo[2] - marg && cz < hi[2] + marg) {
            i128 N[3] = {N3x + D3 * a.x, N3y + D3 * a.y, N3z + D3 * a.z};
            if (center_in_box(N, D3, Q)) judge_ball(L, c, N, D3, c[i], 3);
          }
        }
        // ---- q4 : la droite des centres equidistants coupe-t-elle Q ? ----
        if (k + 1 >= m) continue;
        {
          double dir[3] = {(double)wx, (double)wy, (double)wz};
          double o[3] = {cx, cy, cz};
          double t0 = -1e300, t1 = 1e300;
          bool ok = true;
          for (int ax = 0; ax < 3 && ok; ++ax) {
            double l = lo[ax] - marg, h = hi[ax] + marg;
            if (std::fabs(dir[ax]) < 1e-300) {
              if (o[ax] < l || o[ax] > h) ok = false;
            } else {
              double ta = (l - o[ax]) / dir[ax], tb = (h - o[ax]) / dir[ax];
              if (ta > tb) std::swap(ta, tb);
              t0 = std::max(t0, ta); t1 = std::min(t1, tb);
              if (t0 > t1) ok = false;
            }
          }
          // repli prudent si le triangle est presque degenere (centre flottant peu fiable)
          double wn = std::sqrt((double)wx * wx + (double)wy * wy + (double)wz * wz);
          if (!ok && wn > 1e-9 * (double)uu) continue;
        }
        ++st.line_hits;
        for (int l = k + 1; l < m; ++l) {
          if (!M_(i, l) || !M_(j, l) || !M_(k, l)) continue;
          const Pt& d = P[c[l]];
          ++st.quad_tests;
          i64 sx = d.x - a.x, sy = d.y - a.y, sz = d.z - a.z;
          i64 ss = sx * sx + sy * sy + sz * sz;
          // D4 = 2 det[u;v;s] ; N4 = uu (v x s) + vv (s x u) + ss (u x v)
          i128 det = (i128)ux * (vy * sz - vz * sy) - (i128)uy * (vx * sz - vz * sx) + (i128)uz * (vx * sy - vy * sx);
          if (det == 0) continue;
          i128 vsx = (i128)vy * sz - (i128)vz * sy, vsy = (i128)vz * sx - (i128)vx * sz, vsz = (i128)vx * sy - (i128)vy * sx;
          i128 sux = (i128)sy * uz - (i128)sz * uy, suy = (i128)sz * ux - (i128)sx * uz, suz = (i128)sx * uy - (i128)sy * ux;
          i128 N4x = uu * vsx + vv * sux + ss * (i128)wx;
          i128 N4y = uu * vsy + vv * suy + ss * (i128)wy;
          i128 N4z = uu * vsz + vv * suz + ss * (i128)wz;
          i128 D4 = 2 * det;
          if (D4 < 0) { D4 = -D4; N4x = -N4x; N4y = -N4y; N4z = -N4z; }
          double dd = (double)D4;
          double qx = a.x + (double)N4x / dd, qy = a.y + (double)N4y / dd, qz = a.z + (double)N4z / dd;
          if (qx < lo[0] - marg || qx >= hi[0] + marg || qy < lo[1] - marg || qy >= hi[1] + marg || qz < lo[2] - marg ||
              qz >= hi[2] + marg)
            continue;
          i128 N[3] = {N4x + D4 * a.x, N4y + D4 * a.y, N4z + D4 * a.z};
          if (!center_in_box(N, D4, Q)) continue;
          ++st.quad_center_in;
          // centre strictement interieur au tetraedre
          const Pt* T[4] = {&a, &b, &cc, &d};
          bool inside = true;
          for (int f = 0; f < 4 && inside; ++f) {
            const Pt &p0 = *T[(f + 1) % 4], &p1 = *T[(f + 2) % 4], &p2 = *T[(f + 3) % 4];
            i128 so = orient_pt(p0, p1, p2, *T[f]);
            i128 sc = orient(p0, p1, p2, N, D4);
            if (sc == 0 || ((so > 0) != (sc > 0))) inside = false;
          }
          if (!inside) continue;
          judge_ball(L, c, N, D4, c[i], 4);
        }
      }
    }
  }
}

static inline bool guard_pass(const Pt& x, const std::vector<int>& S0, const Box& Q, uint64_t& tests) {
  // exists y in S0 : min_{c in closed Q} |x|^2-|y|^2-2c.(x-y) <= 0
  i64 xx = x.x * x.x + x.y * x.y + x.z * x.z;
  for (int yi : S0) {
    ++tests;
    const Pt& y = P[yi];
    i64 dx = x.x - y.x, dy = x.y - y.y, dz = x.z - y.z;
    i64 cxv = dx > 0 ? Q.hi[0] : Q.lo[0];
    i64 cyv = dy > 0 ? Q.hi[1] : Q.lo[1];
    i64 czv = dz > 0 ? Q.hi[2] : Q.lo[2];
    i128 f = (i128)xx - (y.x * y.x + y.y * y.y + y.z * y.z) - 2 * ((i128)cxv * dx + (i128)cyv * dy + (i128)czv * dz);
    if (f <= 0) return true;
  }
  return false;
}

static int g_dom = 1;
static bool g_noenum = false;
// x est exclu si au moins K sites du pool sont strictement plus proches que x
// de TOUT centre c de la boite (test affine exact aux coins) : une boule
// admissible centree dans Q et contenant x aurait alors >= K interieurs.
static inline bool dominated(const Pt& x, const std::vector<int>& pool, const Box& Q, uint64_t& tests) {
  i64 xx = x.x * x.x + x.y * x.y + x.z * x.z;
  int cnt = 0, left = (int)pool.size();
  for (int yi : pool) {
    if (cnt + left < K) return false;
    --left;
    ++tests;
    const Pt& y = P[yi];
    i64 dx = y.x - x.x, dy = y.y - x.y, dz = y.z - x.z;
    i64 cxv = dx > 0 ? Q.lo[0] : Q.hi[0];
    i64 cyv = dy > 0 ? Q.lo[1] : Q.hi[1];
    i64 czv = dz > 0 ? Q.lo[2] : Q.hi[2];
    i128 g = (i128)(y.x * y.x + y.y * y.y + y.z * y.z) - xx - 2 * ((i128)cxv * dx + (i128)cyv * dy + (i128)czv * dz);
    if (g < 0 && ++cnt >= K) return true;
  }
  return false;
}
static void process(Local& L, const Box& Q, const std::vector<int>& parent) {
  ++L.st.nodes;
  // S0 : K sites les plus proches du centre de la boite, parmi le parent
  double cq[3] = {(Q.lo[0] + Q.hi[0]) * 0.5, (Q.lo[1] + Q.hi[1]) * 0.5, (Q.lo[2] + Q.hi[2]) * 0.5};
  std::vector<std::pair<double, int>> dd;
  dd.reserve(parent.size());
  for (int i : parent) {
    double dx = P[i].x - cq[0], dy = P[i].y - cq[1], dz = P[i].z - cq[2];
    dd.push_back({dx * dx + dy * dy + dz * dz, i});
  }
  if ((int)dd.size() < K) return;  // impossible si n >= K
  int pool_n = std::min<int>((int)dd.size(), g_dom * K);
  if (pool_n < K) pool_n = K;
  std::nth_element(dd.begin(), dd.begin() + (pool_n - 1), dd.end());
  std::sort(dd.begin(), dd.begin() + pool_n);
  std::vector<int> S0, pool;
  for (int i = 0; i < K; ++i) S0.push_back(dd[i].second);
  for (int i = 0; i < pool_n; ++i) pool.push_back(dd[i].second);
  std::vector<int> cand;
  cand.reserve(parent.size());
  i64 bl[3] = {INT64_MAX, INT64_MAX, INT64_MAX}, bh[3] = {INT64_MIN, INT64_MIN, INT64_MIN};
  for (int i : parent)
    if (guard_pass(P[i], S0, Q, L.st.guard_tests) && (g_dom <= 1 || !dominated(P[i], pool, Q, L.st.guard_tests))) {
      cand.push_back(i);
      bl[0] = std::min(bl[0], P[i].x); bl[1] = std::min(bl[1], P[i].y); bl[2] = std::min(bl[2], P[i].z);
      bh[0] = std::max(bh[0], P[i].x); bh[1] = std::max(bh[1], P[i].y); bh[2] = std::max(bh[2], P[i].z);
    }
  // un centre critique est dans conv(U) inclus dans bbox(cand)
  for (int ax = 0; ax < 3; ++ax)
    if (bh[ax] < Q.lo[ax] || bl[ax] >= Q.hi[ax]) { ++L.st.skipped_bbox; return; }
  bool can_split = (Q.hi[0] - Q.lo[0] > 1);
  if ((int)cand.size() > MMAX && can_split) {
    i64 mid[3] = {(Q.lo[0] + Q.hi[0]) / 2, (Q.lo[1] + Q.hi[1]) / 2, (Q.lo[2] + Q.hi[2]) / 2};
    for (int o = 0; o < 8; ++o) {
      Box C;
      for (int ax = 0; ax < 3; ++ax) {
        bool up = (o >> ax) & 1;
        C.lo[ax] = up ? mid[ax] : Q.lo[ax];
        C.hi[ax] = up ? Q.hi[ax] : mid[ax];
      }
      process(L, C, cand);
    }
    return;
  }
  ++L.st.leaves;
  if ((int)cand.size() > MMAX) { static std::atomic<unsigned long long> big{0}, c4{0}; unsigned long long m=cand.size(); ++big; c4 += m*(m-1)*(m-2)*(m-3)/24; if ((big.load() & (big.load()-1))==0) fprintf(stderr, "oversize_leaves=%llu sum_C(m,4)=%llu side=%lld m=%llu\n", big.load(), c4.load(), (long long)(Q.hi[0]-Q.lo[0]), m); }
  L.st.sum_m += cand.size();
  L.st.max_m = std::max<uint64_t>(L.st.max_m, cand.size());
  L.degenerate_seen.clear();
  if (!g_noenum) enumerate_leaf(L, cand, Q);
}

// ---------- oracle brut global (n petit) ----------
static void oracle(Stats& st) {
  int n = (int)P.size();
  std::vector<int> all(n);
  std::iota(all.begin(), all.end(), 0);
  Local L;
  std::set<Key> seen;
  // Parcours brut de tous les supports ; comptes globaux exacts ; dedup global.
  auto judge = [&](const i128 N[3], i128 D, int ia, int qgen) {
    const Pt& a = P[ia];
    i128 Nrel[3] = {N[0] - D * a.x, N[1] - D * a.y, N[2] - D * a.z};
    int p = 0; std::vector<int> sh;
    for (int z = 0; z < n; ++z) { int s = side(Nrel, D, a, P[z]); if (s < 0) ++p; else if (s == 0) sh.push_back(z); }
    int q = qgen;
    if ((int)sh.size() > qgen) q = qmin_of_shell(sh, N, D);
    if (p + q > K + 1) return;
    Key key = make_key(N, D, a);
    if (!seen.insert(key).second) return;
    if ((int)sh.size() > q) ++st.extra_shell;
    st.by_q_p[q][p]++;
  };
  for (int i = 0; i < n; ++i) for (int j = i + 1; j < n; ++j) {
    i128 N[3] = {P[i].x + P[j].x, P[i].y + P[j].y, P[i].z + P[j].z};
    judge(N, 2, i, 2);
  }
  for (int i = 0; i < n; ++i) for (int j = i + 1; j < n; ++j) for (int k = j + 1; k < n; ++k) {
    const Pt &a = P[i], &b = P[j], &c = P[k];
    i64 ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z, vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;
    i64 wx = uy * vz - uz * vy, wy = uz * vx - ux * vz, wz = ux * vy - uy * vx;
    if (!wx && !wy && !wz) continue;
    i64 d1 = ux * vx + uy * vy + uz * vz;
    i64 d2 = (a.x - b.x) * (c.x - b.x) + (a.y - b.y) * (c.y - b.y) + (a.z - b.z) * (c.z - b.z);
    i64 d3 = (a.x - c.x) * (b.x - c.x) + (a.y - c.y) * (b.y - c.y) + (a.z - c.z) * (b.z - c.z);
    if (!(d1 > 0 && d2 > 0 && d3 > 0)) continue;
    i64 uu = ux * ux + uy * uy + uz * uz, vv = vx * vx + vy * vy + vz * vz;
    i128 tx = (i128)uu * vx - (i128)vv * ux, ty = (i128)uu * vy - (i128)vv * uy, tz = (i128)uu * vz - (i128)vv * uz;
    i128 D3 = 2 * ((i128)wx * wx + (i128)wy * wy + (i128)wz * wz);
    i128 N[3] = {ty * wz - tz * wy + D3 * a.x, tz * wx - tx * wz + D3 * a.y, tx * wy - ty * wx + D3 * a.z};
    judge(N, D3, i, 3);
  }
  for (int i = 0; i < n; ++i) for (int j = i + 1; j < n; ++j) for (int k = j + 1; k < n; ++k) for (int l = k + 1; l < n; ++l) {
    const Pt &a = P[i], &b = P[j], &c = P[k], &d = P[l];
    i64 ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z, vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z, sx = d.x - a.x, sy = d.y - a.y, sz = d.z - a.z;
    i128 det = (i128)ux * (vy * sz - vz * sy) - (i128)uy * (vx * sz - vz * sx) + (i128)uz * (vx * sy - vy * sx);
    if (!det) continue;
    i64 uu = ux * ux + uy * uy + uz * uz, vv = vx * vx + vy * vy + vz * vz, ss = sx * sx + sy * sy + sz * sz;
    i64 wx = uy * vz - uz * vy, wy = uz * vx - ux * vz, wz = ux * vy - uy * vx;
    i128 N4x = uu * ((i128)vy * sz - (i128)vz * sy) + vv * ((i128)sy * uz - (i128)sz * uy) + ss * (i128)wx;
    i128 N4y = uu * ((i128)vz * sx - (i128)vx * sz) + vv * ((i128)sz * ux - (i128)sx * uz) + ss * (i128)wy;
    i128 N4z = uu * ((i128)vx * sy - (i128)vy * sx) + vv * ((i128)sx * uy - (i128)sy * ux) + ss * (i128)wz;
    i128 D4 = 2 * det;
    if (D4 < 0) { D4 = -D4; N4x = -N4x; N4y = -N4y; N4z = -N4z; }
    i128 N[3] = {N4x + D4 * a.x, N4y + D4 * a.y, N4z + D4 * a.z};
    const Pt* T[4] = {&a, &b, &c, &d};
    bool inside = true;
    for (int f = 0; f < 4 && inside; ++f) {
      const Pt &p0 = *T[(f + 1) % 4], &p1 = *T[(f + 2) % 4], &p2 = *T[(f + 3) % 4];
      i128 so = orient_pt(p0, p1, p2, *T[f]);
      i128 sc = orient(p0, p1, p2, N, D4);
      if (sc == 0 || ((so > 0) != (sc > 0))) inside = false;
    }
    if (!inside) continue;
    judge(N, D4, i, 4);
  }
}

int main(int argc, char** argv) {
  std::string file;
  int threads = 1;
  bool do_oracle = false;
  int gen_n = 0; int gen_seed = 1; int gen_range = 64; std::string gen_family = "uniform";
  for (int i = 1; i < argc; ++i) {
    std::string s = argv[i];
    if (s.rfind("--K=", 0) == 0) K = atoi(s.c_str() + 4);
    else if (s.rfind("--M=", 0) == 0) MMAX = atoi(s.c_str() + 4);
    else if (s.rfind("--threads=", 0) == 0) threads = atoi(s.c_str() + 10);
    else if (s == "--oracle") do_oracle = true;
    else if (s == "--noprune") g_prune = false;
    else if (s == "--noenum") g_noenum = true;
    else if (s.rfind("--dom=", 0) == 0) g_dom = atoi(s.c_str() + 6);
    else if (s.rfind("--gen=", 0) == 0) gen_n = atoi(s.c_str() + 6);
    else if (s.rfind("--seed=", 0) == 0) gen_seed = atoi(s.c_str() + 7);
    else if (s.rfind("--range=", 0) == 0) gen_range = atoi(s.c_str() + 8);
    else if (s.rfind("--family=", 0) == 0) gen_family = s.substr(9);
    else file = s;
  }
  if (gen_n > 0) {
    // petits nuages entiers (degeneres si range petit) pour l'oracle
    std::mt19937_64 rng(gen_seed);
    std::set<std::tuple<i64, i64, i64>> used;
    while ((int)P.size() < gen_n) {
      i64 x = rng() % gen_range, y = rng() % gen_range, z = rng() % gen_range;
      if (gen_family == "plane") z = (x + 2 * y) % 3;  // quasi-plan : degeneres
      if (gen_family == "grid") { x = rng() % 5; y = rng() % 5; z = rng() % 5; }
      if (used.insert({x, y, z}).second) P.push_back({x, y, z});
    }
  } else {
    FILE* f = fopen(file.c_str(), "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", file.c_str()); return 2; }
    uint32_t buf[3];
    while (fread(buf, 4, 3, f) == 3) P.push_back({(i64)buf[0], (i64)buf[1], (i64)buf[2]});
    fclose(f);
  }
  int n = (int)P.size();
  i64 mx = 0;
  for (auto& p : P) mx = std::max({mx, p.x, p.y, p.z});
  i64 side = 1;
  while (side <= mx) side <<= 1;
  auto t0 = std::chrono::steady_clock::now();
  Stats total;
  if (do_oracle) {
    oracle(total);
  } else {
    // taches : les boites du niveau 2 (64 boites), reparties sur les fils
    std::vector<int> all(n);
    std::iota(all.begin(), all.end(), 0);
    Box root{{0, 0, 0}, {side, side, side}};
    // developpe deux niveaux en sequentiel pour obtenir des taches
    struct Task { Box b; std::vector<int> cand; };
    std::vector<Task> tasks;
    std::vector<Task> cur{{root, all}};
    for (int lvl = 0; lvl < 2; ++lvl) {
      std::vector<Task> nxt;
      for (auto& t : cur) {
        i64 mid[3] = {(t.b.lo[0] + t.b.hi[0]) / 2, (t.b.lo[1] + t.b.hi[1]) / 2, (t.b.lo[2] + t.b.hi[2]) / 2};
        for (int o = 0; o < 8; ++o) {
          Box C;
          for (int ax = 0; ax < 3; ++ax) { bool up = (o >> ax) & 1; C.lo[ax] = up ? mid[ax] : t.b.lo[ax]; C.hi[ax] = up ? t.b.hi[ax] : mid[ax]; }
          nxt.push_back({C, t.cand});  // candidats du parent : filtres dans process
        }
      }
      cur.swap(nxt);
    }
    tasks = std::move(cur);
    std::atomic<size_t> next{0};
    std::mutex mu;
    std::vector<std::thread> th;
    for (int w = 0; w < threads; ++w)
      th.emplace_back([&] {
        Local L;
        for (;;) {
          size_t i = next++;
          if (i >= tasks.size()) break;
          process(L, tasks[i].b, tasks[i].cand);
        }
        std::lock_guard<std::mutex> g(mu);
        total.add(L.st);
      });
    for (auto& t : th) t.join();
  }
  double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  uint64_t byq[5] = {0, 0, 0, 0, 0}, tot = 0;
  for (int q = 2; q <= 4; ++q) for (int p = 0; p < 16; ++p) byq[q] += total.by_q_p[q][p];
  tot = byq[2] + byq[3] + byq[4];
  printf("{\"n\":%d,\"K\":%d,\"M\":%d,\"threads\":%d,\"oracle\":%s,\"seconds\":%.3f,\"balls\":%llu,\"by_qmin\":[%llu,%llu,%llu],"
         "\"per_site\":%.2f,\"extra_shell\":%llu,\"max_shell\":%llu,",
         n, K, MMAX, threads, do_oracle ? "true" : "false", sec, (unsigned long long)tot, (unsigned long long)byq[2],
         (unsigned long long)byq[3], (unsigned long long)byq[4], (double)tot / n, (unsigned long long)total.extra_shell,
         (unsigned long long)total.max_shell);
  printf("\"by_q_p\":{");
  for (int q = 2; q <= 4; ++q) {
    printf("\"q%d\":[", q);
    for (int p = 0; p <= K - 1; ++p) printf("%llu%s", (unsigned long long)total.by_q_p[q][p], p < K - 1 ? "," : "");
    printf("]%s", q < 4 ? "," : "");
  }
  printf("},\"nodes\":%llu,\"leaves\":%llu,\"skipped_bbox\":%llu,\"mean_m\":%.2f,\"max_m\":%llu,\"guard_tests\":%llu,"
         "\"pair_tests\":%llu,\"triple_tests\":%llu,\"line_hits\":%llu,\"quad_tests\":%llu,\"quad_center_in\":%llu,"
         "\"us_per_ball\":%.3f}\n",
         (unsigned long long)total.nodes, (unsigned long long)total.leaves, (unsigned long long)total.skipped_bbox,
         total.leaves ? (double)total.sum_m / total.leaves : 0.0, (unsigned long long)total.max_m,
         (unsigned long long)total.guard_tests, (unsigned long long)total.pair_tests,
         (unsigned long long)total.triple_tests, (unsigned long long)total.line_hits,
         (unsigned long long)total.quad_tests, (unsigned long long)total.quad_center_in,
         tot ? 1e6 * sec * threads / tot : 0.0);
  return 0;
}
