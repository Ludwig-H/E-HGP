// Juge D1 (auditeur C) : oracle exhaustif exact en entiers (i128) du catalogue v9 sur des
// nuages moyens (n <= ~150, coordonnees <= 200), compare a la chaine v9 executee sur l'image
// similitude (off + f*z) du meme nuage en 18 bits. Une similitude preserve boules, coquilles,
// interieurs et q_min : la comparaison se fait par (coquille triee, p, q_min, u).
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>
#include "src/chain/tower_chain.hpp"
__extension__ typedef __int128 i128;
using V = std::array<long long, 3>;
struct Row { int qmin, p; std::vector<int> shell; };

static i128 gcd128(i128 a, i128 b) { if (a < 0) a = -a; if (b < 0) b = -b; while (b) { i128 t = a % b; a = b; b = t; } return a; }
static long long dot(const V& a, const V& b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }
static V sub(const V& a, const V& b) { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }
static V cross(const V& a, const V& b) { return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]}; }

struct Oracle {
  const std::vector<V>& s; unsigned kmax;
  std::map<std::array<i128, 5>, Row> balls; long long rejected_deep = 0;
  Oracle(const std::vector<V>& pts, unsigned k) : s(pts), kmax(k) {}
  void add(int q, i128 den, std::array<i128, 3> N, const V& anchor) {
    if (den < 0) { den = -den; for (auto& x : N) x = -x; }
    i128 g = gcd128(den, gcd128(N[0], gcd128(N[1], N[2])));
    den /= g; for (auto& x : N) x /= g;
    i128 R = 0; for (int t = 0; t < 3; ++t) { i128 d = den * anchor[t] - N[t]; R += d * d; }
    std::array<i128, 5> key{den, N[0], N[1], N[2], R};
    auto it = balls.find(key);
    if (it != balls.end()) { if (q < it->second.qmin) { std::puts("ORACLE q order bug"); std::exit(9); } return; }
    Row row{q, 0, {}};
    for (int i = 0; i < (int)s.size(); ++i) {
      i128 d2 = 0; for (int t = 0; t < 3; ++t) { i128 d = den * s[i][t] - N[t]; d2 += d * d; }
      if (d2 < R) { if (++row.p >= (int)kmax + 5) { row.p = 1000; break; } }
      else if (d2 == R) row.shell.push_back(i);
    }
    balls.emplace(key, row);
  }
  void run() {
    const int n = (int)s.size();
    for (int i = 0; i < n; ++i) for (int j = i + 1; j < n; ++j)
      add(2, 2, {(i128)s[i][0] + s[j][0], (i128)s[i][1] + s[j][1], (i128)s[i][2] + s[j][2]}, s[i]);
    for (int i = 0; i < n; ++i) for (int j = i + 1; j < n; ++j) for (int k = j + 1; k < n; ++k) {
      const V &a = s[i], &b = s[j], &c = s[k];
      if (dot(sub(b, a), sub(c, a)) <= 0 || dot(sub(a, b), sub(c, b)) <= 0 || dot(sub(a, c), sub(b, c)) <= 0) continue;
      V u = sub(b, a), v = sub(c, a), w = cross(u, v);
      i128 w2 = (i128)dot(w, w); if (!w2) continue;
      i128 uu = dot(u, u), vv = dot(v, v);
      std::array<i128, 3> m{uu * v[0] - vv * u[0], uu * v[1] - vv * u[1], uu * v[2] - vv * u[2]};
      std::array<i128, 3> num{m[1] * w[2] - m[2] * w[1], m[2] * w[0] - m[0] * w[2], m[0] * w[1] - m[1] * w[0]};
      i128 den = 2 * w2;
      add(3, den, {den * a[0] + num[0], den * a[1] + num[1], den * a[2] + num[2]}, a);
    }
    for (int i = 0; i < n; ++i) for (int j = i + 1; j < n; ++j) for (int k = j + 1; k < n; ++k) for (int l = k + 1; l < n; ++l) {
      const V& a = s[i]; V u = sub(s[j], a), v = sub(s[k], a), w = sub(s[l], a);
      V vw = cross(v, w); i128 D = (i128)dot(u, vw); if (!D) continue;
      i128 G[3][3] = {{dot(u,u),dot(u,v),dot(u,w)},{dot(v,u),dot(v,v),dot(v,w)},{dot(w,u),dot(w,v),dot(w,w)}};
      i128 h[3] = {G[0][0], G[1][1], G[2][2]};
      auto det3 = [](i128 m[3][3]) { return m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1]) - m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0]) + m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]); };
      i128 dG = det3(G); i128 sum = 0; bool ok = true;
      for (int c = 0; c < 3 && ok; ++c) {
        i128 Mx[3][3]; for (int r = 0; r < 3; ++r) for (int cc = 0; cc < 3; ++cc) Mx[r][cc] = cc == c ? h[r] : G[r][cc];
        i128 dc = det3(Mx); if (dc <= 0) ok = false; sum += dc;
      }
      if (!ok || 2 * dG - sum <= 0) continue;  // lambda_i = dc/(2 dG) > 0, lambda_0 > 0
      V wu = cross(w, u), uv = cross(u, v);
      i128 uu = G[0][0], vv = G[1][1], ww = G[2][2];
      std::array<i128, 3> num{uu*vw[0] + vv*wu[0] + ww*uv[0], uu*vw[1] + vv*wu[1] + ww*uv[1], uu*vw[2] + vv*wu[2] + ww*uv[2]};
      i128 den = 2 * D;
      add(4, den, {den * a[0] + num[0], den * a[1] + num[1], den * a[2] + num[2]}, a);
    }
  }
};

static bool power_checked(const mhgp9::tower::BallKey& k, const V& z, i128& out) {
  i128 n2 = (i128)z[0]*z[0] + (i128)z[1]*z[1] + (i128)z[2]*z[2], t, acc;
  if (__builtin_mul_overflow(k.a, n2, &acc)) return false;
  for (int i = 0; i < 3; ++i) { if (__builtin_mul_overflow(k.b[i], (i128)z[i], &t) || __builtin_add_overflow(acc, t, &acc)) return false; }
  if (__builtin_add_overflow(acc, k.c, &acc)) return false;
  out = acc; return true;
}

int main(int argc, char** argv) {
  // usage: med_judge kmax workers f offx offy offz < points(small)
  if (argc < 7) return 2;
  unsigned kmax = std::atoi(argv[1]); size_t workers = std::atoi(argv[2]); long long f = std::atoll(argv[3]);
  V off{std::atoll(argv[4]), std::atoll(argv[5]), std::atoll(argv[6])};
  std::vector<V> s; long long x, y, z;
  while (std::scanf("%lld %lld %lld", &x, &y, &z) == 3) s.push_back({x, y, z});
  const unsigned okmax = kmax + (std::getenv("ORACLE_KMAX_SHIFT") ? std::atoi(std::getenv("ORACLE_KMAX_SHIFT")) : 0);
  Oracle orc(s, okmax); orc.run();
  std::map<std::vector<int>, Row> expect; int maxshell = 0;
  for (auto& [k, r] : orc.balls) if (r.p + r.qmin <= (int)okmax + 1) { expect[r.shell] = r; maxshell = std::max(maxshell, (int)r.shell.size()); }
  std::vector<V> big(s.size()); std::vector<mhgp9::gen::Point3> pts;
  for (size_t i = 0; i < s.size(); ++i) {
    for (int t = 0; t < 3; ++t) big[i][t] = off[t] + f * s[i][t];
    pts.push_back({(mhgp9::gen::Coordinate)big[i][0], (mhgp9::gen::Coordinate)big[i][1], (mhgp9::gen::Coordinate)big[i][2]});
  }
  mhgp9::ChainOptions opt; opt.kmax = kmax; opt.workers = workers; opt.run_tower = true; opt.keep_catalogue = true;
  auto res = mhgp9::run_tower_chain(pts, opt);
  std::string st = mhgp9::chain_status_name(res.status);
  if (res.status != mhgp9::ChainStatus::kComplete) {
    bool ok = res.status == mhgp9::ChainStatus::kUnsupportedDegeneracy && maxshell > 12;
    std::printf("%s status=%s reason=%s expected=%zu maxshell=%d\n", ok ? "REFUSED_OK" : "BAD_STATUS", st.c_str(), res.reason.c_str(), expect.size(), maxshell);
    return ok ? 0 : 1;
  }
  if (maxshell > 12) { std::printf("BAD accepted maxshell=%d\n", maxshell); return 1; }
  std::map<std::vector<int>, Row> got; long long overflow = 0, self_mismatch = 0;
  for (auto& b : res.catalogue_balls) {
    Row r{(int)b.arity, 0, {}}; bool ovf = false;
    for (int i = 0; i < (int)big.size(); ++i) { i128 pw; if (!power_checked(b.key, big[i], pw)) { ovf = true; break; } if (pw < 0) ++r.p; else if (pw == 0) r.shell.push_back(i); }
    if (ovf) { ++overflow; continue; }
    if (r.p != b.n_interior || (int)r.shell.size() != b.n_shell) ++self_mismatch;
    got[r.shell] = r;
  }
  long long missing = 0, extra = 0, wrong = 0; std::string ex;
  for (auto& [sh, r] : expect) {
    auto it = got.find(sh);
    if (it == got.end()) { ++missing; if (ex.size() < 400) { ex += " MISS[q=" + std::to_string(r.qmin) + ",p=" + std::to_string(r.p) + ",u=" + std::to_string(sh.size()) + ":"; for (int i : sh) ex += std::to_string(i) + ","; ex += "]"; } }
    else if (it->second.p != r.p || it->second.qmin != r.qmin) { ++wrong; if (ex.size() < 400) ex += " WRONG[p " + std::to_string(r.p) + "/" + std::to_string(it->second.p) + " q " + std::to_string(r.qmin) + "/" + std::to_string(it->second.qmin) + "]"; }
  }
  for (auto& [sh, r] : got) if (!expect.count(sh)) { ++extra; if (ex.size() < 400) ex += " EXTRA[p=" + std::to_string(r.p) + ",q=" + std::to_string(r.qmin) + "]"; }
  bool ok = !missing && !extra && !wrong && !overflow && !self_mismatch;
  std::printf("%s n=%zu kmax=%u balls=%zu missing=%lld extra=%lld wrong=%lld overflow=%lld selfmis=%lld maxshell=%d digest=%016llx%s\n",
              ok ? "OK" : "MISMATCH", s.size(), kmax, expect.size(), missing, extra, wrong, overflow, self_mismatch, maxshell,
              (unsigned long long)res.tower_digest, ex.c_str());
  return ok ? 0 : 1;
}
