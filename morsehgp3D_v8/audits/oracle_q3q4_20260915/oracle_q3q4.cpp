// Auditeur B (15 sept. 2026) — oracle indépendant q3/q4 en arithmétique entière i128, sans aucun
// en-tête produit. Entrée : fichier texte « x y z » (u16) et Kmax ; sortie : toutes les présentations
// positives (circumcentre à poids barycentriques strictement positifs) de cardinal 3 (triangles aigus)
// et 4 (tétraèdres bien centrés) dont le nombre p de sites strictement intérieurs à la circumboule
// vérifie p + q <= Kmax + 1 (soit p < h_q = Kmax + 2 - q), avec intérieurs et coquille complète ;
// une coquille excédant le support est signalée (domaine générique violé) et non filtrée.
// Prédicats : q3 « z intérieur » = signe(2D|z-a|² - 2(z-a)·W) avec D = |u×v|², W = |v|²(u·u-u·v)u + |u|²(v·v-u·v)v
// (<= 2^106) ; q4 = puissance 2det|z-a|² - 2(z-a)·g avec g = 2det(c0-a) par Cramer (<= 2^89) ; acuité / bon centrage par déterminants entiers.
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
using i64 = long long; using i128 = __int128;
struct P { i64 x, y, z; };
static i64 dot(const std::array<i64,3>& a, const std::array<i64,3>& b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }
static std::array<i64,3> sub(const P& a, const P& b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
static std::array<i64,3> cross(const std::array<i64,3>& a, const std::array<i64,3>& b) { return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]}; }
static i128 det3(i128 a00,i128 a01,i128 a02,i128 a10,i128 a11,i128 a12,i128 a20,i128 a21,i128 a22) { return a00*(a11*a22-a12*a21) - a01*(a10*a22-a12*a20) + a02*(a10*a21-a11*a20); }
static int sgn(i128 v) { return v > 0 ? 1 : v < 0 ? -1 : 0; }
static std::string ids(const std::vector<int>& v) { std::string s; for (size_t i = 0; i < v.size(); ++i) { if (i) s += ','; s += std::to_string(v[i]); } return s.empty() ? "-" : s; }
int main(int argc, char** argv) {
  if (argc < 3) { std::fprintf(stderr, "usage: oracle_q3q4 points.txt kmax [q3only]\n"); return 2; }
  std::ifstream in(argv[1]); const int kmax = std::atoi(argv[2]); const bool q3only = argc > 3;
  std::vector<P> pts; { i64 x, y, z; while (in >> x >> y >> z) pts.push_back({x, y, z}); }
  const int n = (int)pts.size(); if (n < 3 || kmax < 1 || kmax > 10) { std::fprintf(stderr, "bad input\n"); return 2; }
  const int h3 = kmax - 1, h4 = kmax - 2;  // p < h_q  <=>  p + q <= kmax + 1
  long long q3_pos = 0, q3_kept = 0, q3_extra = 0, q4_pos = 0, q4_kept = 0, q4_extra = 0, q4_flat = 0;
  // ---- q3 : triangles aigus
  for (int a = 0; a < n; ++a) for (int b = a + 1; b < n; ++b) for (int c = b + 1; c < n; ++c) {
    const auto u = sub(pts[b], pts[a]), v = sub(pts[c], pts[a]), w = sub(pts[c], pts[b]);
    const i64 uu = dot(u,u), vv = dot(v,v), uv = dot(u,v);
    const auto nrm = cross(u, v); const i128 D = (i128)nrm[0]*nrm[0] + (i128)nrm[1]*nrm[1] + (i128)nrm[2]*nrm[2];
    if (D == 0) continue;  // colinéaire
    // aigu strict : angle en a : u·v > 0 ; en b : (a-b)·(c-b) = -u·w > 0 ; en c : (a-c)·(b-c) = v·(v-u)... = (−v)·(−w) = v·w > 0
    if (!(uv > 0 && dot(u, w) < 0 && dot(v, w) > 0)) continue;
    ++q3_pos;
    // W = |v|²(u·u - u·v) u + |u|²(v·v - u·v) v   (= 2D (c0 - a))
    const i128 ca = (i128)vv * (uu - uv), cb = (i128)uu * (vv - uv);
    std::array<i128,3> W{ca*u[0] + cb*v[0], ca*u[1] + cb*v[1], ca*u[2] + cb*v[2]};
    std::vector<int> interior, shell;
    for (int z = 0; z < n; ++z) {
      if (z == a || z == b || z == c) { shell.push_back(z); continue; }
      const auto za = sub(pts[z], pts[a]); const i128 zz = (i128)za[0]*za[0] + (i128)za[1]*za[1] + (i128)za[2]*za[2];
      const i128 power = (i128)2 * D * zz - (i128)2 * (za[0]*W[0] + za[1]*W[1] + za[2]*W[2]);
      if (power < 0) interior.push_back(z); else if (power == 0) shell.push_back(z);
    }
    const int p = (int)interior.size(); const bool extra = shell.size() > 3;
    if (extra) ++q3_extra;
    if (p < h3) { ++q3_kept; std::printf("q3 %d,%d,%d p=%d extra=%d interior=%s shell=%s\n", a, b, c, p, extra ? 1 : 0, ids(interior).c_str(), ids(shell).c_str()); }
  }
  if (!q3only) {
    // ---- q4 : tétraèdres bien centrés (circumcentre à poids barycentriques > 0)
    for (int a = 0; a < n; ++a) for (int b = a + 1; b < n; ++b) for (int c = b + 1; c < n; ++c) for (int d = c + 1; d < n; ++d) {
      const auto u = sub(pts[b], pts[a]), v = sub(pts[c], pts[a]), w = sub(pts[d], pts[a]);
      const i128 vol = det3(u[0],u[1],u[2], v[0],v[1],v[2], w[0],w[1],w[2]);
      if (vol == 0) { ++q4_flat; continue; }
      // circumcentre c0 = a + M^{-1} (|u|²/2, |v|²/2, |w|²/2) avec M lignes u,v,w ; poids barycentriques de a,b,c,d :
      // (1 - Σλ, λ_u, λ_v, λ_w) où λ = M^{-1} r ; par Cramer : λ_i = det(M avec colonne i remplacée par r)/det(M) ; on
      // travaille avec 2r = (|u|²,|v|²,|w|²) et le signe de det(M) : signe(λ_i) = signe(N_i)·signe(det), signe(1-Σλ) = signe(det - ΣN_i)·signe(det).
      const i128 ru = dot(u,u), rv = dot(v,v), rw = dot(w,w);
      // M en lignes : [u ; v ; w] ; système M λ = r  (λ colonne). Cramer sur les colonnes de M^T... utiliser M^T λ' ? Attention :
      // condition d'équidistance : 2 (c0-a)·u = |u|², idem v, w  =>  la matrice des lignes u,v,w multiplie (c0-a) : M (c0-a) = r.
      // Barycentriques : c0 - a = λ_u u + λ_v v + λ_w w  =>  M (M^T λ) ... on résout d'abord g = c0-a (Cramer sur M), puis λ = solution de [u v w] λ = g (colonnes).
      const i128 det = vol;
      const i128 gx = det3(ru,u[1],u[2], rv,v[1],v[2], rw,w[1],w[2]);
      const i128 gy = det3(u[0],ru,u[2], v[0],rv,v[2], w[0],rw,w[2]);
      const i128 gz = det3(u[0],u[1],ru, v[0],v[1],rv, w[0],w[1],rw);
      // 2 det · (c0 - a) = (gx, gy, gz).  Puis λ : colonnes u,v,w : [u v w] λ = (c0-a) ; Cramer avec matrice C = [u v w] (det C = det).
      // 2 det · det · λ_u = det([g v w]) etc. => signe(λ_u) = signe(det([g v w]))·signe(det) (facteur 2det² > 0)
      const i128 lu = det3(gx,v[0],w[0], gy,v[1],w[1], gz,v[2],w[2]);
      const i128 lv = det3(u[0],gx,w[0], u[1],gy,w[1], u[2],gz,w[2]);
      const i128 lw = det3(u[0],v[0],gx, u[1],v[1],gy, u[2],v[2],gz);
      const i128 la = (i128)2 * det * det - (lu + lv + lw);  // 2det² (1 - Σλ)
      const int sd = sgn(det);
      // λ_u = lu / (2 det²), etc. : le signe de chaque poids est celui de lu, lv, lw, la (2det² > 0).
      if (!(lu > 0 && lv > 0 && lw > 0 && la > 0)) continue;
      ++q4_pos;
      // Puissance exacte : 2det·(|z-a|² - 2 (z-a)·(c0-a)) = 2det|z-a|² - 2 (z-a)·g, avec g = 2det (c0-a) ;
      // le signe de la puissance est celui de cette quantité multiplié par signe(det) (facteur 2det > 0 après normalisation).
      // Bornes : det <= 2^51, |z-a|² <= 2^34, g <= 2^70, (z-a)·g <= 2^88 : i128 largement suffisant.
      std::vector<int> interior, shell;
      for (int z = 0; z < n; ++z) {
        if (z == a || z == b || z == c || z == d) { shell.push_back(z); continue; }
        const auto za = sub(pts[z], pts[a]); const i128 zz = (i128)za[0]*za[0] + (i128)za[1]*za[1] + (i128)za[2]*za[2];
        const i128 q = (i128)2 * det * zz - (i128)2 * ((i128)za[0]*gx + (i128)za[1]*gy + (i128)za[2]*gz);
        const int s = sgn(q) * sd;
        if (s < 0) interior.push_back(z); else if (s == 0) shell.push_back(z);
      }
      const int p = (int)interior.size(); const bool extra = shell.size() > 4;
      if (extra) ++q4_extra;
      if (p < h4) { ++q4_kept; std::printf("q4 %d,%d,%d,%d p=%d extra=%d interior=%s shell=%s\n", a, b, c, d, p, extra ? 1 : 0, ids(interior).c_str(), ids(shell).c_str()); }
    }
  }
  std::printf("SUMMARY n=%d kmax=%d q3_positive=%lld q3_kept=%lld q3_extra_shell=%lld q4_positive=%lld q4_kept=%lld q4_extra_shell=%lld q4_flat=%lld\n", n, kmax, q3_pos, q3_kept, q3_extra, q4_pos, q4_kept, q4_extra, q4_flat);
  return 0;
}
