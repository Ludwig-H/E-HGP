// Auditeur C (v9), famille D4 : sonde statistique hors produit.
// Enumeration par site du <=L-niveau de l'arrangement des plans bissecteurs
// (equivalent a l'inversion de centre a) par balayage de pinceaux (a,x,y).
// ARITHMETIQUE : double, pour des COMPTES seulement (pas un moteur exact).
// Usage : d4_probe file.u32le Kmax samples rho_mm seed [brute]
//  - samples : nombre de sites tires ; rho_mm : rayon du voisinage N(a).
//  - un sommet (a;x,y,z) de rayon R n'est exact que si 2R <= rho ; au-dela il
//    est compte "incertain" et le parcours ne s'etend pas depuis lui.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct V3 { double x, y, z; };
static inline V3 sub(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline V3 add(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline V3 mul(V3 a, double s) { return {a.x * s, a.y * s, a.z * s}; }
static inline double dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }

static std::vector<std::array<int64_t, 3>> read_u32le(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::vector<std::array<int64_t, 3>> p(bytes.size() / 12);
  for (size_t i = 0; i < p.size(); ++i)
    for (int a = 0; a < 3; ++a) {
      uint32_t v = 0;
      for (int b = 0; b < 4; ++b) v |= uint32_t(bytes[(3 * i + a) * 4 + b]) << (8 * b);
      p[i][a] = v;
    }
  return p;
}

struct Grid {
  int64_t h;
  std::unordered_map<uint64_t, std::vector<int>> cells;
  static uint64_t key(int64_t i, int64_t j, int64_t k) { return (uint64_t(i) << 42) ^ (uint64_t(j) << 21) ^ uint64_t(k); }
  void build(const std::vector<std::array<int64_t, 3>>& p, int64_t hh) {
    h = hh;
    for (size_t i = 0; i < p.size(); ++i) cells[key(p[i][0] / h, p[i][1] / h, p[i][2] / h)].push_back((int)i);
  }
  template <class F> void ball(const std::array<int64_t, 3>& c, double r, F f) const {
    int64_t lo[3], hi[3];
    for (int a = 0; a < 3; ++a) { lo[a] = (int64_t)std::floor((c[a] - r) / h); hi[a] = (int64_t)std::floor((c[a] + r) / h); if (lo[a] < 0) lo[a] = 0; }
    for (int64_t i = lo[0]; i <= hi[0]; ++i)
      for (int64_t j = lo[1]; j <= hi[1]; ++j)
        for (int64_t k = lo[2]; k <= hi[2]; ++k) {
          auto it = cells.find(key(i, j, k));
          if (it == cells.end()) continue;
          for (int id : it->second) f(id);
        }
  }
};

struct SiteStats {
  uint64_t m = 0, pencils = 0, events = 0, cover = 0, active = 0;
  uint64_t verts_L = 0, verts_q4lvl = 0, crit_q4 = 0, crit_q3 = 0, uncertain = 0, q3_candidates_lvl = 0;
  double rmax_crit = 0;
  bool started = false;
};

int main(int argc, char** argv) {
  if (argc < 6) { std::fprintf(stderr, "usage: d4_probe file K samples rho_mm seed [brute]\n"); return 2; }
  const auto P = read_u32le(argv[1]);
  const int K = std::atoi(argv[2]);
  const int samples = std::atoi(argv[3]);
  const double rho = std::atof(argv[4]);
  const unsigned seed = (unsigned)std::atoi(argv[5]);
  const bool brute = argc > 6 && std::string(argv[6]) == "brute";
  const int L = K - 2;       // niveau de parcours (q3 : p <= K-2)
  const int Lq4 = K - 3;     // q4 : p <= K-3
  Grid g; g.build(P, 100);
  std::mt19937_64 rng(seed);
  std::vector<int> order(P.size());
  for (size_t i = 0; i < P.size(); ++i) order[i] = (int)i;
  std::shuffle(order.begin(), order.end(), rng);
  const int S = std::min<int>(samples, (int)P.size());
  std::vector<SiteStats> all;
  uint64_t brute_mismatch = 0, brute_checked = 0;
  const auto t0 = std::chrono::steady_clock::now();
  for (int si = 0; si < S; ++si) {
    const int ia = order[si];
    SiteStats st;
    std::vector<int> nb;
    std::vector<V3> w;  // positions relatives a a
    g.ball(P[ia], rho, [&](int id) {
      if (id == ia) return;
      double dx = double(P[id][0] - P[ia][0]), dy = double(P[id][1] - P[ia][1]), dz = double(P[id][2] - P[ia][2]);
      if (dx * dx + dy * dy + dz * dz <= rho * rho) { nb.push_back(id); w.push_back({dx, dy, dz}); }
    });
    const int m = (int)nb.size();
    st.m = (uint64_t)m;
    if (m < 4) { all.push_back(st); continue; }
    std::vector<double> n2(m);
    for (int i = 0; i < m; ++i) n2[i] = dot(w[i], w[i]);
    // ordre des voisins par distance (depart)
    std::vector<int> byd(m);
    for (int i = 0; i < m; ++i) byd[i] = i;
    std::sort(byd.begin(), byd.end(), [&](int x, int y) { return n2[x] < n2[y]; });

    std::unordered_set<uint64_t> seen_pencil, seen_vert_L, seen_vert_q4, seen_crit_q4, seen_uncertain;
    std::deque<std::pair<int, int>> queue;
    auto pkey = [&](int x, int y) { if (x > y) std::swap(x, y); return (uint64_t)x * (uint64_t)m + (uint64_t)y; };
    auto tkey = [&](int x, int y, int z) {
      int t[3] = {x, y, z}; std::sort(t, t + 3);
      return ((uint64_t)t[0] * (uint64_t)m + (uint64_t)t[1]) * (uint64_t)m + (uint64_t)t[2];
    };
    struct Ev { double tau; int z; int dir; };
    std::vector<Ev> ev;
    std::vector<std::array<int, 4>> found_q4;  // for brute check
    // Balayage d'un pinceau ; retourne vrai s'il contient un sommet de niveau <= L exact.
    auto sweep = [&](int x, int y, bool record) -> bool {
      const V3 u = w[x], v = w[y];
      const V3 n = cross(u, v);
      const double nn = dot(n, n);
      if (nn == 0) return false;
      const V3 o = mul(add(mul(cross(v, n), n2[x]), mul(cross(n, u), n2[y])), 1.0 / (2 * nn));
      ev.clear();
      int C0 = 0, below = 0;
      for (int z = 0; z < m; ++z) {
        if (z == x || z == y) continue;
        const double s = dot(n, w[z]);
        const double gz = n2[z] - 2 * dot(w[z], o);
        if (s == 0) { if (gz < 0) ++C0; continue; }
        ev.push_back({gz / (2 * s), z, s > 0 ? +1 : -1});
        if (s < 0) ++below;
      }
      if (record) { st.pencils++; st.events += (uint64_t)m; }
      std::sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& b) { return a.tau < b.tau; });
      int level = C0 + below;  // t -> -inf
      int lvl0 = -1;
      bool any = false;
      double tlo = 0, thi = 0; bool have = false;
      for (size_t e = 0; e < ev.size(); ++e) {
        if (lvl0 < 0 && ev[e].tau > 0) lvl0 = level;
        const int before = level;
        const int after = level + ev[e].dir;
        const int vl = std::min(before, after);
        level = after;
        if (vl > L) continue;
        const int z = ev[e].z;
        const V3 c = add(o, mul(n, ev[e].tau));
        const double R = std::sqrt(dot(c, c));
        const bool exact = 2 * R <= rho;
        if (!have) { tlo = ev[e].tau; have = true; }
        thi = ev[e].tau;
        if (!record) { if (exact) any = true; continue; }
        const uint64_t tk = tkey(x, y, z);
        if (!exact) { seen_uncertain.insert(tk); continue; }
        any = true;
        if (seen_vert_L.insert(tk).second) {
          st.verts_L++;
        }
        if (vl <= Lq4 && seen_vert_q4.insert(tk).second) {
          st.verts_q4lvl++;
          // centre dans le tetraedre (0,u,v,wz) ?
          const V3 wz = w[z];
          const double det = dot(u, cross(v, wz));
          if (det != 0) {
            const double al = dot(c, cross(v, wz)) / det, be = dot(u, cross(c, wz)) / det, ga = dot(u, cross(v, c)) / det;
            if (al > 0 && be > 0 && ga > 0 && al + be + ga < 1) {
              if (seen_crit_q4.insert(tk).second) { st.crit_q4++; st.rmax_crit = std::max(st.rmax_crit, R); found_q4.push_back({x, y, z, vl}); }
            }
          }
        }
        if (!seen_pencil.count(pkey(x, z))) queue.push_back({x, z});
        if (!seen_pencil.count(pkey(y, z))) queue.push_back({y, z});
      }
      if (lvl0 < 0) lvl0 = level;
      if (record) {
        // q3 : triangle aigu (o dans l'interieur du triangle 0,u,v) et niveau en t=0
        const double uu = n2[x], vv = n2[y], uv = dot(u, v), ou = dot(o, u), ov = dot(o, v);
        const double den = uu * vv - uv * uv;
        const double al = (ou * vv - ov * uv) / den, be = (ov * uu - ou * uv) / den;
        const double R0 = std::sqrt(dot(o, o));
        if (lvl0 <= L && 2 * R0 <= rho) {
          st.q3_candidates_lvl++;
          if (al > 0 && be > 0 && al + be < 1) { st.crit_q3++; st.rmax_crit = std::max(st.rmax_crit, R0); }
        }
        if (have) {
          uint64_t cov = 0, act = 0;
          for (int z = 0; z < m; ++z) {
            if (z == x || z == y) continue;
            const double s = dot(n, w[z]);
            const double gz = n2[z] - 2 * dot(w[z], o);
            const bool in_lo = 2 * tlo * s >= gz, in_hi = 2 * thi * s >= gz;
            if (in_lo || in_hi) ++cov;
            if (s != 0) { const double tau = gz / (2 * s); if (tau >= tlo && tau <= thi) ++act; }
          }
          st.cover += cov; st.active += act;
        }
      }
      return any;
    };
    // depart : pinceaux (a, nn1, c)
    bool started = false;
    for (int j = 1; j < std::min(m, 64) && !started; ++j) {
      if (sweep(byd[0], byd[j], false)) { queue.push_back({byd[0], byd[j]}); started = true; }
    }
    for (int i = 1; i < std::min(m, 16) && !started; ++i)
      for (int j = i + 1; j < std::min(m, 64) && !started; ++j)
        if (sweep(byd[i], byd[j], false)) { queue.push_back({byd[i], byd[j]}); started = true; }
    st.started = started;
    while (!queue.empty()) {
      auto [x, y] = queue.front(); queue.pop_front();
      if (!seen_pencil.insert(pkey(x, y)).second) continue;
      sweep(x, y, true);
    }
    st.uncertain = seen_uncertain.size();
    if (brute && m <= 90) {
      // controle exhaustif : tous les triplets, niveau exact dans N(a)
      std::unordered_set<uint64_t> bf;
      for (int x = 0; x < m; ++x)
        for (int y = x + 1; y < m; ++y)
          for (int z = y + 1; z < m; ++z) {
            const V3 u = w[x], v = w[y], wz = w[z];
            const double det = dot(u, cross(v, wz));
            if (det == 0) continue;
            // centre c : 2 c.w_i = |w_i|^2 pour i=x,y,z
            const V3 r0 = cross(v, wz), r1 = cross(wz, u), r2 = cross(u, v);
            const V3 c = mul(add(add(mul(r0, n2[x]), mul(r1, n2[y])), mul(r2, n2[z])), 1.0 / (2 * det));
            const double R = std::sqrt(dot(c, c));
            if (2 * R > rho) continue;
            int lvl = 0;
            for (int q = 0; q < m && lvl <= L; ++q) {
              if (q == x || q == y || q == z) continue;
              if (n2[q] - 2 * dot(w[q], c) < 0) ++lvl;
            }
            if (lvl <= L) bf.insert(tkey(x, y, z));
          }
      brute_checked++;
      if (bf.size() != seen_vert_L.size()) brute_mismatch++;
      else for (auto k : bf) if (!seen_vert_L.count(k)) { brute_mismatch++; break; }
    }
    all.push_back(st);
  }
  const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  // agregats
  auto mean = [&](auto f) { double s = 0; for (auto& x : all) s += (double)f(x); return s / (double)all.size(); };
  auto quant = [&](auto f, double q) { std::vector<double> v; for (auto& x : all) v.push_back((double)f(x)); std::sort(v.begin(), v.end()); return v[std::min(v.size() - 1, (size_t)(q * v.size()))]; };
  int nstart = 0; for (auto& x : all) nstart += x.started;
  std::printf("{\"file\":\"%s\",\"n\":%zu,\"K\":%d,\"samples\":%d,\"rho_mm\":%.0f,\"started\":%d,\"wall_s\":%.2f,", argv[1], P.size(), K, S, rho, nstart, wall);
  std::printf("\"m_mean\":%.1f,\"m_p50\":%.0f,\"m_p90\":%.0f,\"m_max\":%.0f,", mean([](auto& x) { return x.m; }), quant([](auto& x) { return x.m; }, 0.5), quant([](auto& x) { return x.m; }, 0.9), quant([](auto& x) { return x.m; }, 1.0));
  std::printf("\"pencils_mean\":%.1f,\"pencils_p90\":%.0f,\"verts_L_mean\":%.1f,\"verts_q4lvl_mean\":%.1f,", mean([](auto& x) { return x.pencils; }), quant([](auto& x) { return x.pencils; }, 0.9), mean([](auto& x) { return x.verts_L; }), mean([](auto& x) { return x.verts_q4lvl; }));
  std::printf("\"crit_q4_mean\":%.2f,\"crit_q3_mean\":%.2f,\"q3_cand_lvl_mean\":%.1f,\"uncertain_mean\":%.1f,\"uncertain_p90\":%.0f,", mean([](auto& x) { return x.crit_q4; }), mean([](auto& x) { return x.crit_q3; }), mean([](auto& x) { return x.q3_candidates_lvl; }), mean([](auto& x) { return x.uncertain; }), quant([](auto& x) { return x.uncertain; }, 0.9));
  std::printf("\"events_naive_mean\":%.0f,\"cover_mean\":%.0f,\"active_mean\":%.0f,\"cover_per_pencil\":%.1f,\"active_per_pencil\":%.1f,",
              mean([](auto& x) { return x.events; }), mean([](auto& x) { return x.cover; }), mean([](auto& x) { return x.active; }),
              mean([](auto& x) { return x.cover; }) / std::max(1e-9, mean([](auto& x) { return x.pencils; })),
              mean([](auto& x) { return x.active; }) / std::max(1e-9, mean([](auto& x) { return x.pencils; })));
  std::printf("\"brute_checked\":%llu,\"brute_mismatch\":%llu}\n", (unsigned long long)brute_checked, (unsigned long long)brute_mismatch);
  return 0;
}
