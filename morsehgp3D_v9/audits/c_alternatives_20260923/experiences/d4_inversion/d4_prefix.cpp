// Auditeur C (v9), famille D4 : sonde de COUT hors produit (double, comptes).
// Parcours BFS par site du <=L-niveau des plans bissecteurs (= inversion de
// centre a), L = K-2, par balayage de pinceaux (a,x,y), chaque pinceau
// balaye sur le PREFIXE des voisins de a tries par distance, prefixe elargi
// jusqu'au certificat local : toute portion de niveau <= L a 2R <= rho_p.
// Usage : d4_prefix file.u32le K samples seed [rho_cap_mm]
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <fstream>
#include <iterator>
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
  uint64_t m = 0, pencils = 0, evals = 0, retries = 0, events_sorted = 0, capped = 0, union_cover = 0, union_unbounded = 0, active = 0;
  uint64_t verts_L = 0, verts_q4lvl = 0, crit_q4 = 0, crit_q3 = 0;
  double seconds = 0, rmax_vertex = 0;
  bool started = false;
};

int main(int argc, char** argv) {
  if (argc < 5) { std::fprintf(stderr, "usage: d4_prefix file K samples seed [rho_cap_mm]\n"); return 2; }
  const auto P = read_u32le(argv[1]);
  const int K = std::atoi(argv[2]);
  const int samples = std::atoi(argv[3]);
  const unsigned seed = (unsigned)std::atoi(argv[4]);
  const double rho_cap = argc > 5 ? std::atof(argv[5]) : 4000.0;
  const bool measure_union = !(argc > 6 && std::string(argv[6]) == "timing");
  const int L = K - 2, Lq4 = K - 3;
  Grid g; g.build(P, 250);
  std::mt19937_64 rng(seed);
  std::vector<int> order(P.size());
  for (size_t i = 0; i < P.size(); ++i) order[i] = (int)i;
  std::shuffle(order.begin(), order.end(), rng);
  const int S = std::min<int>(samples, (int)P.size());
  std::vector<SiteStats> all;
  std::vector<uint64_t> prefix_hist(8, 0);  // prefixe final par pinceau : <=16,32,64,128,256,512,1024,>1024
  const auto T0 = std::chrono::steady_clock::now();
  for (int si = 0; si < S; ++si) {
    const auto t0 = std::chrono::steady_clock::now();
    const int ia = order[si];
    SiteStats st;
    std::vector<std::pair<double, int>> tmp;
    auto visit = [&](auto fn) { if (rho_cap > 20000) { for (int id = 0; id < (int)P.size(); ++id) fn(id); } else g.ball(P[ia], rho_cap, fn); };
    visit([&](int id) {
      if (id == ia) return;
      double dx = double(P[id][0] - P[ia][0]), dy = double(P[id][1] - P[ia][1]), dz = double(P[id][2] - P[ia][2]);
      const double d2 = dx * dx + dy * dy + dz * dz;
      if (d2 <= rho_cap * rho_cap) tmp.push_back({d2, id});
    });
    std::sort(tmp.begin(), tmp.end());
    const int m = (int)tmp.size();
    st.m = (uint64_t)m;
    if (m < 4) { all.push_back(st); continue; }
    std::vector<V3> w(m);
    std::vector<double> n2(m), dist(m);
    for (int i = 0; i < m; ++i) {
      const int id = tmp[i].second;
      w[i] = {double(P[id][0] - P[ia][0]), double(P[id][1] - P[ia][1]), double(P[id][2] - P[ia][2])};
      n2[i] = tmp[i].first; dist[i] = std::sqrt(tmp[i].first);
    }
    auto prefix_len = [&](double r) { return (int)(std::upper_bound(dist.begin(), dist.end(), r) - dist.begin()); };
    std::unordered_set<uint64_t> seen_pencil, seen_vert_L, seen_vert_q4;
    seen_pencil.reserve(4096); seen_vert_L.reserve(8192); seen_vert_q4.reserve(8192);
    struct Item { int x, y; double r; };
    std::deque<Item> queue;
    auto pkey = [&](int x, int y) { if (x > y) std::swap(x, y); return (uint64_t)x * (uint64_t)m + (uint64_t)y; };
    auto tkey = [&](int x, int y, int z) {
      int t[3] = {x, y, z}; std::sort(t, t + 3);
      return ((uint64_t)t[0] * (uint64_t)m + (uint64_t)t[1]) * (uint64_t)m + (uint64_t)t[2];
    };
    struct Ev { double tau; int z; int dir; };
    std::vector<Ev> ev;
    // balayage certifie d'un pinceau ; record=false : test de depart seulement.
    auto sweep = [&](int x, int y, double rguess, bool record) -> bool {
      const V3 u = w[x], v = w[y];
      const V3 n = cross(u, v);
      const double nn = dot(n, n);
      if (nn == 0) return false;
      const V3 o = mul(add(mul(cross(v, n), n2[x]), mul(cross(n, u), n2[y])), 1.0 / (2 * nn));
      const double R0sq = dot(o, o);
      double rho_p = std::max(rguess, std::max(dist[x], dist[y]));
      bool capped = false;
      int lvl0 = -1, level = 0;
      double tlo = 0, thi = 0; bool have = false;
      for (int attempt = 0;; ++attempt) {
        if (rho_p >= rho_cap) { rho_p = rho_cap; capped = true; }
        const int pl = prefix_len(rho_p);
        if (record) { st.evals += (uint64_t)pl; if (attempt) st.retries++; }
        ev.clear();
        int C0 = 0, below = 0;
        for (int z = 0; z < pl; ++z) {
          if (z == x || z == y) continue;
          const double s = dot(n, w[z]);
          const double gz = n2[z] - 2 * dot(w[z], o);
          if (s == 0) { if (gz < 0) ++C0; continue; }
          ev.push_back({gz / (2 * s), z, s > 0 ? +1 : -1});
          if (s < 0) ++below;
        }
        std::sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& b) { return a.tau < b.tau; });
        level = C0 + below; lvl0 = -1; have = false;
        int lev = level;
        for (size_t e = 0; e < ev.size(); ++e) {
          if (lvl0 < 0 && ev[e].tau > 0) lvl0 = lev;
          const int vl = std::min(lev, lev + ev[e].dir);
          lev += ev[e].dir;
          if (vl <= L) { if (!have) tlo = ev[e].tau; have = true; thi = ev[e].tau; }
        }
        if (lvl0 < 0) lvl0 = lev;
        // portions non bornees de niveau <= L : non certifiables localement
        const bool unb = (level <= L) || (lev <= L);
        double Rend = 0;
        if (have) Rend = std::sqrt(R0sq + std::max(tlo * tlo, thi * thi) * nn);
        if (lvl0 <= L) Rend = std::max(Rend, std::sqrt(R0sq));
        if (capped || (!unb && 2 * Rend <= rho_p)) {
          if (record) {
            st.events_sorted += ev.size();
            int b = 0; while (b < 7 && pl > (16 << b)) ++b; prefix_hist[b]++;
            if (capped) st.capped++;
          }
          break;
        }
        rho_p = unb ? 2 * rho_p : std::max(2 * rho_p, 2 * Rend * 1.25);
      }
      if (!record) return have;
      st.pencils++;
      if (measure_union) {
        // couverture ideale : B(t_start*) U B(t_end*), bornes par L+1 sites permanents
        std::vector<double> up, dn;
        for (auto& e : ev) (e.dir > 0 ? up : dn).push_back(e.tau);
        double te = 0, ts = 0; bool ub = false;
        if ((int)up.size() >= L + 1) { std::nth_element(up.begin(), up.begin() + L, up.end()); te = up[L]; } else ub = true;
        if ((int)dn.size() >= L + 1) { std::nth_element(dn.begin(), dn.begin() + L, dn.end(), std::greater<double>()); ts = dn[L]; } else ub = true;
        if (ub) st.union_unbounded++;
        else {
          const int pl = prefix_len(rho_p);
          uint64_t cov = 0, act = 0;
          for (int z = 0; z < pl; ++z) {
            if (z == x || z == y) continue;
            const double s = dot(n, w[z]);
            const double gz = n2[z] - 2 * dot(w[z], o);
            if (2 * ts * s >= gz || 2 * te * s >= gz) ++cov;
            if (s != 0) { const double tau = gz / (2 * s); if (tau >= ts && tau <= te) ++act; }
          }
          st.union_cover += cov; st.active += act;
        }
      }
      int lev = level;
      for (size_t e = 0; e < ev.size(); ++e) {
        const int vl = std::min(lev, lev + ev[e].dir);
        lev += ev[e].dir;
        if (vl > L) continue;
        const int z = ev[e].z;
        const V3 c = add(o, mul(n, ev[e].tau));
        const double R = std::sqrt(dot(c, c));
        const uint64_t tk = tkey(x, y, z);
        if (seen_vert_L.insert(tk).second) { st.verts_L++; st.rmax_vertex = std::max(st.rmax_vertex, R); }
        if (vl <= Lq4 && seen_vert_q4.insert(tk).second) {
          st.verts_q4lvl++;
          const V3 wz = w[z];
          const double det = dot(u, cross(v, wz));
          if (det != 0) {
            const double al = dot(c, cross(v, wz)) / det, be = dot(u, cross(c, wz)) / det, ga = dot(u, cross(v, c)) / det;
            if (al > 0 && be > 0 && ga > 0 && al + be + ga < 1) st.crit_q4++;
          }
        }
        if (!seen_pencil.count(pkey(x, z))) queue.push_back({x, z, 2 * R * 1.25});
        if (!seen_pencil.count(pkey(y, z))) queue.push_back({y, z, 2 * R * 1.25});
      }
      if (lvl0 <= L) {
        const double uu = n2[x], vv = n2[y], uv = dot(u, v), ou = dot(o, u), ov = dot(o, v);
        const double den = uu * vv - uv * uv;
        const double al = (ou * vv - ov * uv) / den, be = (ov * uu - ou * uv) / den;
        if (lvl0 <= L && al > 0 && be > 0 && al + be < 1) st.crit_q3++;
      }
      return have;
    };
    bool started = false;
    for (int j = 1; j < std::min(m, 64) && !started; ++j)
      if (sweep(0, j, 2 * dist[j], false)) { queue.push_back({0, j, 2 * dist[j]}); started = true; }
    st.started = started;
    while (!queue.empty()) {
      Item it = queue.front(); queue.pop_front();
      if (!seen_pencil.insert(pkey(it.x, it.y)).second) continue;
      sweep(it.x, it.y, it.r, true);
    }
    st.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    all.push_back(st);
  }
  const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
  auto mean = [&](auto f) { double s = 0; for (auto& x : all) s += (double)f(x); return s / (double)all.size(); };
  auto quant = [&](auto f, double q) { std::vector<double> v; for (auto& x : all) v.push_back((double)f(x)); std::sort(v.begin(), v.end()); return v[std::min(v.size() - 1, (size_t)(q * v.size()))]; };
  int nstart = 0; for (auto& x : all) nstart += x.started;
  std::printf("{\"file\":\"%s\",\"n\":%zu,\"K\":%d,\"samples\":%d,\"rho_cap_mm\":%.0f,\"started\":%d,\"wall_s\":%.2f,", argv[1], P.size(), K, S, rho_cap, nstart, wall);
  std::printf("\"ms_per_site_mean\":%.3f,\"ms_per_site_p90\":%.3f,\"ms_per_site_max\":%.3f,", 1e3 * mean([](auto& x) { return x.seconds; }), 1e3 * quant([](auto& x) { return x.seconds; }, 0.9), 1e3 * quant([](auto& x) { return x.seconds; }, 1.0));
  std::printf("\"m_cap_mean\":%.0f,\"pencils_mean\":%.1f,\"pencils_p90\":%.0f,\"pencils_max\":%.0f,\"verts_L_mean\":%.1f,\"verts_q4lvl_mean\":%.1f,",
              mean([](auto& x) { return x.m; }), mean([](auto& x) { return x.pencils; }), quant([](auto& x) { return x.pencils; }, 0.9), quant([](auto& x) { return x.pencils; }, 1.0),
              mean([](auto& x) { return x.verts_L; }), mean([](auto& x) { return x.verts_q4lvl; }));
  std::printf("\"crit_q4_mean\":%.2f,\"crit_q3_mean\":%.2f,\"evals_mean\":%.0f,\"evals_p90\":%.0f,\"evals_max\":%.0f,\"evals_per_pencil\":%.1f,\"retries_per_pencil\":%.3f,\"events_sorted_per_pencil\":%.1f,\"capped_pencils_mean\":%.2f,\"sites_with_capped\":%.0f,\"rmax_vertex_p90_mm\":%.0f,",
              mean([](auto& x) { return x.crit_q4; }), mean([](auto& x) { return x.crit_q3; }), mean([](auto& x) { return x.evals; }), quant([](auto& x) { return x.evals; }, 0.9), quant([](auto& x) { return x.evals; }, 1.0),
              mean([](auto& x) { return x.evals; }) / std::max(1e-9, mean([](auto& x) { return x.pencils; })),
              mean([](auto& x) { return x.retries; }) / std::max(1e-9, mean([](auto& x) { return x.pencils; })),
              mean([](auto& x) { return x.events_sorted; }) / std::max(1e-9, mean([](auto& x) { return x.pencils; })),
              mean([](auto& x) { return x.capped; }), mean([](auto& x) { return x.capped > 0 ? 1.0 : 0.0; }) * all.size(),
              quant([](auto& x) { return x.rmax_vertex; }, 0.9));
  std::printf("\"union_cover_per_pencil\":%.1f,\"active_per_pencil\":%.1f,\"union_unbounded_frac\":%.4f,", mean([](auto& x) { return x.union_cover; }) / std::max(1e-9, mean([](auto& x) { return x.pencils - x.union_unbounded; })), mean([](auto& x) { return x.active; }) / std::max(1e-9, mean([](auto& x) { return x.pencils - x.union_unbounded; })), mean([](auto& x) { return x.union_unbounded; }) / std::max(1e-9, mean([](auto& x) { return x.pencils; })));
  std::printf("\"prefix_hist_le16_32_64_128_256_512_1024_gt\":[");
  for (int b = 0; b < 8; ++b) std::printf("%s%llu", b ? "," : "", (unsigned long long)prefix_hist[b]);
  std::printf("]}\n");
  return 0;
}
