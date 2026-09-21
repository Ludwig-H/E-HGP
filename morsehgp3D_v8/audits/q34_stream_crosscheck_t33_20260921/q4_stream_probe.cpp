// Auditeur B, 21 septembre 2026 — énumération exhaustive indépendante des boules q4 propriétaires des paires
// résiduelles de la voie q4 du front WSPD (sources 4dbe3024, front inchangé depuis c5308651), pour comparaison
// avec le flux q4 (Local28) de `run_wspd_q34_parallel`. Aucun flottant, aucune brique q4 du moteur n'est appelée :
// seuls le front (appelé tel quel, masque 6, mode MidpointSamples) et l'index sont réutilisés.
//  Pour chaque paire (a,b) de chaque rectangle résiduel portant la voie q4 (toutes les paires, dans l'ordre) :
//   1. balayage complet du nuage : compte exact du citron L_4(a,b) = {H > 0 et 2H² > Ξ} (rejetable si ≥ K − 2) et
//      couverture fermée du constructeur |2z−a−b|² ≤ 4|b−a|² (qui contient tout intérieur et tout contact d'une
//      boule q4 propriétaire : (1/√2 + √(3/2))·D < 2D) ;
//   2. pour une paire conservée (ou une paire rejetable dans le budget du lemme) : tous les tétraèdres (a,b,x,y),
//      x < y dans la couverture, propriétaires de ab (les cinq autres arêtes ≤ |ab|², égalité admise sauf si la clé
//      d'IDs de l'arête égale est plus petite : règle `owned` de `q4_local.cpp`), strictement positifs (centre
//      strictement intérieur : δ = det(u,v,w) ≠ 0 et les quatre coordonnées barycentriques du centre > 0, en i128),
//      profondeur exacte (sites de la couverture strictement intérieurs, hors support, saturée à K − 2) ; une boule
//      de profondeur < K − 2 est écrite (clé entière réduite A|z|² + B·z + C, profondeur, support trié) ;
//   3. lemme du citron pour q4 : aucune boule d'une paire rejetable ne doit avoir une profondeur < K − 2.
//  Magnitudes (u16) : |u|² < 2^34, produits vectoriels < 2^33 par coordonnée, N < 2^69, δ < 2^51, déterminants
//  det(N,v,w) < 2^103, 2δ² < 2^103, tests d'intériorité < 2^87 : tout tient en i128 sans réduction préalable.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include "wspd/front.hpp"
#include "pipeline/q2_census.hpp"
#include "pipeline/prepared_cloud.hpp"
using namespace mhgp8;
namespace {
using V3 = std::array<i64, 3>;
V3 sub(const Point3& p, const Point3& q) { return {static_cast<i64>(p[0]) - q[0], static_cast<i64>(p[1]) - q[1], static_cast<i64>(p[2]) - q[2]}; }
i64 dot(const V3& p, const V3& q) { return p[0] * q[0] + p[1] * q[1] + p[2] * q[2]; }
V3 cross(const V3& p, const V3& q) { return {p[1] * q[2] - p[2] * q[1], p[2] * q[0] - p[0] * q[2], p[0] * q[1] - p[1] * q[0]}; }
i64 sq_dist(const Point3& p, const Point3& q) { const auto d = sub(p, q); return dot(d, d); }
// det(p, q, r) avec p en i128 et q, r en i64.
i128 det3(const std::array<i128, 3>& p, const V3& q, const V3& r) {
  const auto c = cross(q, r);
  return p[0] * c[0] + p[1] * c[1] + p[2] * c[2];
}
i128 det3(const V3& p, const std::array<i128, 3>& q, const V3& r) {
  // det(p,q,r) = q · (r × p)
  const auto c = cross(r, p);
  return q[0] * c[0] + q[1] * c[1] + q[2] * c[2];
}
i128 det3(const V3& p, const V3& q, const std::array<i128, 3>& r) {
  const auto c = cross(p, q);
  return r[0] * c[0] + r[1] * c[1] + r[2] * c[2];
}
bool lemon4(const Point3& a, const Point3& b, const Point3& z) {
  const auto u = sub(z, a), w = sub(b, z);
  const i64 h = dot(u, w);
  if (h <= 0) return false;
  const auto c = cross(u, w);
  i128 xi = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) xi += static_cast<i128>(c[axis]) * c[axis];
  return static_cast<i128>(2) * (static_cast<i128>(h) * h) > xi;
}
std::pair<std::size_t, std::size_t> edge_key(std::size_t a, std::size_t b) { return a < b ? std::pair{a, b} : std::pair{b, a}; }
// Boule d'un tétraèdre strictement positif : o = a + N/(2δ). Le signe s = signe(δ) rend f(z) = s(δ|z−a|² − (z−a)·N) < 0
// à l'intérieur strict. positive = centre strictement intérieur.
struct Tetra { i128 delta{}; std::array<i128, 3> N{}; int sign{}; bool positive{}; };
Tetra make_tetra(const Point3& a, const Point3& b, const Point3& c, const Point3& d) {
  Tetra t;
  const auto u = sub(b, a), v = sub(c, a), w = sub(d, a);
  const auto vw = cross(v, w), wu = cross(w, u), uv = cross(u, v);
  t.delta = static_cast<i128>(dot(u, vw));  // det(u,v,w)
  if (t.delta == 0) return t;
  const i64 uu = dot(u, u), vv = dot(v, v), ww = dot(w, w);
  for (std::size_t axis = 0; axis < 3; ++axis)
    t.N[axis] = static_cast<i128>(uu) * vw[axis] + static_cast<i128>(vv) * wu[axis] + static_cast<i128>(ww) * uv[axis];
  t.sign = t.delta > 0 ? 1 : -1;
  const i128 lb = det3(t.N, v, w), lc = det3(u, t.N, w), ld = det3(u, v, t.N);  // 2δ² · (λ_b, λ_c, λ_d)
  const i128 twice = static_cast<i128>(2) * t.delta * t.delta;
  t.positive = lb > 0 && lc > 0 && ld > 0 && twice > lb + lc + ld;
  return t;
}
// f(z) < 0 strictement intérieur, = 0 contact, > 0 extérieur.
int side(const Tetra& t, const Point3& a, const Point3& z) {
  const auto d = sub(z, a);
  i128 lhs = t.delta * static_cast<i128>(dot(d, d));
  i128 rhs = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) rhs += static_cast<i128>(d[axis]) * t.N[axis];
  const i128 f = static_cast<i128>(t.sign) * (lhs - rhs);
  return f < 0 ? -1 : (f > 0 ? 1 : 0);
}
__extension__ typedef unsigned __int128 u128;
u128 magnitude(i128 x) { return x < 0 ? static_cast<u128>(-(x + 1)) + 1 : static_cast<u128>(x); }
u128 gcd128(u128 a, u128 b) { while (b) { const u128 r = a % b; a = b; b = r; } return a; }
std::string to_string(i128 x) {
  if (x == 0) return "0";
  const bool neg = x < 0;
  u128 m = magnitude(x);
  std::string s;
  while (m) { s.insert(s.begin(), static_cast<char>('0' + static_cast<int>(m % 10))); m /= 10; }
  return neg ? "-" + s : s;
}
// Clé réduite [A, Bx, By, Bz, C] de f, A > 0, pgcd 1.
std::array<i128, 5> key(const Tetra& t, const Point3& a) {
  std::array<i128, 5> k{};
  k[0] = static_cast<i128>(t.sign) * t.delta;
  const i64 aa = static_cast<i64>(a[0]) * a[0] + static_cast<i64>(a[1]) * a[1] + static_cast<i64>(a[2]) * a[2];
  i128 na = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    k[1 + axis] = static_cast<i128>(t.sign) * (static_cast<i128>(-2) * t.delta * static_cast<i128>(a[axis]) - t.N[axis]);
    na += t.N[axis] * static_cast<i128>(a[axis]);
  }
  k[4] = static_cast<i128>(t.sign) * (t.delta * static_cast<i128>(aa) + na);
  u128 g = 0;
  for (const auto v : k) g = gcd128(g, magnitude(v));
  if (g > 1) for (auto& v : k) v /= static_cast<i128>(g);
  return k;
}
struct Rect { std::uint32_t a, b; };
}  // namespace
int main(int argc, char** argv) {
  if (argc < 4) { std::fprintf(stderr, "usage: file.u16le kmax s [balls_out] [lemma_budget=100]\n"); return 2; }
  const std::string file = argv[1];
  const unsigned kmax = static_cast<unsigned>(std::atoi(argv[2]));
  const unsigned s = static_cast<unsigned>(std::atoi(argv[3]));
  const std::string balls_out = argc > 4 ? argv[4] : "";
  const u64 lemma_budget = argc > 5 ? std::strtoull(argv[5], nullptr, 10) : 100;
  if (kmax < 4 || kmax > 10 || s < 8) { std::fprintf(stderr, "kmax in 4..10 and s >= 8 required\n"); return 2; }
  std::vector<Point3> points;
  {
    std::ifstream in(file, std::ios::binary);
    if (!in) { std::fprintf(stderr, "cannot open %s\n", file.c_str()); return 2; }
    std::vector<unsigned char> raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (raw.size() % 6 != 0) { std::fprintf(stderr, "u16le triples expected\n"); return 2; }
    for (std::size_t i = 0; i < raw.size(); i += 6)
      points.push_back({static_cast<std::uint16_t>(raw[i] | (raw[i + 1] << 8)), static_cast<std::uint16_t>(raw[i + 2] | (raw[i + 3] << 8)),
                        static_cast<std::uint16_t>(raw[i + 4] | (raw[i + 5] << 8))});
  }
  std::FILE* balls = balls_out.empty() ? nullptr : std::fopen(balls_out.c_str(), "w");
  if (!balls_out.empty() && !balls) { std::fprintf(stderr, "cannot write %s\n", balls_out.c_str()); return 2; }
  const std::size_t n = points.size();
  auto cloud = prepare_cloud(points);
  auto index = make_q2_cloud_index(cloud);
  const auto nodes = index->spatial_nodes(); const auto order = index->spatial_order(); const auto sites = index->cloud().points();
  const unsigned h4 = kmax - 2;
  std::vector<Rect> rects;
  const auto t0 = std::chrono::steady_clock::now();
  auto res = run_wspd_front(*index, kmax, s, WspdFrontMode::MidpointSamples,
                            [&](const WspdRectangle& r) { if (r.lane_mask & 4U) rects.push_back({static_cast<std::uint32_t>(r.a_node), static_cast<std::uint32_t>(r.b_node)}); }, 6);
  const double front_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  u64 pairs = 0, rejectable = 0, kept = 0, cover_rej = 0, cover_kept = 0, cover_kept_max = 0;
  u64 tetra_candidates = 0, owned_tetras = 0, positive_tetras = 0, emitted_tetras = 0, kept_with_emission = 0, census_tests = 0;
  u64 lemma_pairs = 0, lemma_tetras = 0, lemma_positive = 0, lemma_tests = 0, lemma_violations = 0;
  std::vector<std::size_t> cover_ranks;
  const auto t1 = std::chrono::steady_clock::now();
  for (const auto& rect : rects) {
    const auto& A = nodes[rect.a]; const auto& B = nodes[rect.b];
    for (auto rank_a = A.range.first; rank_a < A.range.last; ++rank_a) {
      for (auto rank_b = B.range.first; rank_b < B.range.last; ++rank_b) {
        ++pairs;
        const Point3 pa = sites[order[rank_a]], pb = sites[order[rank_b]];
        const auto ida = order[rank_a], idb = order[rank_b];
        const i64 L2 = sq_dist(pa, pb);
        u64 lemon = 0; cover_ranks.clear();
        for (std::size_t rank = 0; rank < n; ++rank) {
          if (rank == rank_a || rank == rank_b) continue;
          const Point3 pz = sites[order[rank]];
          i64 d2 = 0;
          for (std::size_t axis = 0; axis < 3; ++axis) { const i64 e = 2 * static_cast<i64>(pz[axis]) - pa[axis] - pb[axis]; d2 += e * e; }
          if (d2 <= 4 * L2) cover_ranks.push_back(rank);
          if (lemon4(pa, pb, pz)) ++lemon;
        }
        const bool rej = lemon >= h4;
        if (rej) { ++rejectable; cover_rej += cover_ranks.size(); } else { ++kept; cover_kept += cover_ranks.size(); cover_kept_max = std::max<u64>(cover_kept_max, cover_ranks.size()); }
        if (rej && lemma_pairs >= lemma_budget) continue;
        if (rej) ++lemma_pairs;
        const auto owner = edge_key(ida, idb);
        u64 emitted_here = 0;
        for (std::size_t i = 0; i < cover_ranks.size(); ++i) {
          const auto idx = order[cover_ranks[i]]; const Point3 px = sites[idx];
          const i64 ax = sq_dist(pa, px), bx = sq_dist(pb, px);
          if (ax > L2 || (ax == L2 && edge_key(ida, idx) < owner)) continue;
          if (bx > L2 || (bx == L2 && edge_key(idb, idx) < owner)) continue;
          for (std::size_t j = i + 1; j < cover_ranks.size(); ++j) {
            const auto idy = order[cover_ranks[j]]; const Point3 py = sites[idy];
            if (rej) ++lemma_tetras; else ++tetra_candidates;
            const i64 ay = sq_dist(pa, py), by = sq_dist(pb, py), xy = sq_dist(px, py);
            if (ay > L2 || (ay == L2 && edge_key(ida, idy) < owner)) continue;
            if (by > L2 || (by == L2 && edge_key(idb, idy) < owner)) continue;
            if (xy > L2 || (xy == L2 && edge_key(idx, idy) < owner)) continue;
            if (!rej) ++owned_tetras;
            const Tetra t = make_tetra(pa, pb, px, py);
            if (!t.positive) continue;
            if (rej) ++lemma_positive; else ++positive_tetras;
            u64 depth = 0;
            for (const auto rank_z : cover_ranks) {
              if (rank_z == cover_ranks[i] || rank_z == cover_ranks[j]) continue;
              if (rej) ++lemma_tests; else ++census_tests;
              if (side(t, pa, sites[order[rank_z]]) < 0 && ++depth >= h4) break;
            }
            if (depth >= h4) continue;
            if (rej) { ++lemma_violations; continue; }
            ++emitted_tetras; ++emitted_here;
            if (balls) {
              const auto k = key(t, pa);
              std::array<std::size_t, 4> ids{ida, idb, idx, idy};
              std::sort(ids.begin(), ids.end());
              std::fprintf(balls, "%s %s %s %s %s %llu %zu %zu %zu %zu\n", to_string(k[0]).c_str(), to_string(k[1]).c_str(), to_string(k[2]).c_str(),
                           to_string(k[3]).c_str(), to_string(k[4]).c_str(), (unsigned long long)depth, ids[0], ids[1], ids[2], ids[3]);
            }
          }
        }
        if (emitted_here) ++kept_with_emission;
      }
    }
  }
  const double enum_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t1).count();
  if (balls) std::fclose(balls);
  const auto& w = res.work;
  std::printf("{\"schema\":\"audit_b_q4_stream_probe_v1\",\"file\":\"%s\",\"n\":%zu,\"kmax\":%u,\"s\":%u,\"mask\":6,\"front_mode\":\"samples\",\"threshold\":%u,\"lemma_budget\":%llu",
              file.c_str(), n, kmax, s, h4, (unsigned long long)lemma_budget);
  std::printf(",\"front\":{\"lane_rectangles\":[%llu,%llu,%llu],\"residual_pair_mass\":[%llu,%llu,%llu],\"total_unordered_pairs\":%llu,\"ms\":%.1f}",
              (unsigned long long)w.lane_rectangles[0], (unsigned long long)w.lane_rectangles[1], (unsigned long long)w.lane_rectangles[2],
              (unsigned long long)w.residual_pair_mass[0], (unsigned long long)w.residual_pair_mass[1], (unsigned long long)w.residual_pair_mass[2],
              (unsigned long long)res.total_unordered_pairs, front_ms);
  std::printf(",\"q4\":{\"pairs\":%llu,\"rejectable\":%llu,\"kept\":%llu,\"cover_sites_rejectable\":%llu,\"cover_sites_kept\":%llu,\"cover_kept_max\":%llu,"
              "\"tetra_candidates\":%llu,\"owned_tetras\":%llu,\"positive_tetras\":%llu,\"emitted_tetras\":%llu,\"kept_with_emission\":%llu,\"census_tests\":%llu,"
              "\"lemma\":{\"pairs\":%llu,\"tetras\":%llu,\"positive\":%llu,\"tests\":%llu,\"violations\":%llu},\"ms\":%.1f}}\n",
              (unsigned long long)pairs, (unsigned long long)rejectable, (unsigned long long)kept, (unsigned long long)cover_rej, (unsigned long long)cover_kept,
              (unsigned long long)cover_kept_max, (unsigned long long)tetra_candidates, (unsigned long long)owned_tetras, (unsigned long long)positive_tetras,
              (unsigned long long)emitted_tetras, (unsigned long long)kept_with_emission, (unsigned long long)census_tests, (unsigned long long)lemma_pairs,
              (unsigned long long)lemma_tetras, (unsigned long long)lemma_positive, (unsigned long long)lemma_tests, (unsigned long long)lemma_violations, enum_ms);
  return 0;
}
