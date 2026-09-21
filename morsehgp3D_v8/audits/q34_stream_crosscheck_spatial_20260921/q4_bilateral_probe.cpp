// Auditeur B, 21 septembre 2026 — contrôle bilatéral de la voie q4 à l'échelle d'une scène (protocole partiel) :
//  1. VALIDITÉ de chaque record q4 émis par le moteur (support de quatre IDs, profondeur, coquille complète) : arête
//     propriétaire = arête de longueur maximale du tétraèdre (égalité départagée par la plus petite clé d'IDs, règle
//     `owned` de q4_local.cpp), tétraèdre strictement positif (centre strictement intérieur, coordonnées barycentriques
//     en i128), profondeur exacte recalculée sur la couverture fermée |2z−a−b|² ≤ 4|b−a|² de l'arête propriétaire
//     (descente d'index), coquille complète recalculée (sites de puissance nulle, support compris) et comparée aux
//     IDs du record ; profondeur < K − 2 exigée. Chaque record est jugé indépendamment ; aucune brique q4 du moteur.
//  2. COMPLÉTUDE par échantillon : M paires tirées uniformément dans la masse résiduelle de la voie q4 du même front ;
//     pour chaque paire conservée par le citron exact (descente saturante), énumération de tous les tétraèdres
//     propriétaires positifs de profondeur < K − 2 (comme q4_stream_probe_v2) ; chaque boule trouvée (clé entière
//     réduite, profondeur) doit figurer parmi les records ; les manquantes sont comptées (exigées nulles).
//  Entrée des records : fichier texte, une ligne par record d'arité 4 : « depth id0 id1 id2 id3 | shell ids… » (IDs
//  originaux = indices dans le fichier u16le). Aucun flottant.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "wspd/front.hpp"
#include "pipeline/q2_census.hpp"
#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_joint_bounds.hpp"
#include "spindle/predicates.hpp"
using namespace mhgp8;
namespace {
using V3 = std::array<i64, 3>;
V3 sub(const Point3& p, const Point3& q) { return {static_cast<i64>(p[0]) - q[0], static_cast<i64>(p[1]) - q[1], static_cast<i64>(p[2]) - q[2]}; }
i64 dot(const V3& p, const V3& q) { return p[0] * q[0] + p[1] * q[1] + p[2] * q[2]; }
V3 cross(const V3& p, const V3& q) { return {p[1] * q[2] - p[2] * q[1], p[2] * q[0] - p[0] * q[2], p[0] * q[1] - p[1] * q[0]}; }
i64 sq_dist(const Point3& p, const Point3& q) { const auto d = sub(p, q); return dot(d, d); }
i128 det3(const std::array<i128, 3>& p, const V3& q, const V3& r) { const auto c = cross(q, r); return p[0] * c[0] + p[1] * c[1] + p[2] * c[2]; }
i128 det3(const V3& p, const std::array<i128, 3>& q, const V3& r) { const auto c = cross(r, p); return q[0] * c[0] + q[1] * c[1] + q[2] * c[2]; }
i128 det3(const V3& p, const V3& q, const std::array<i128, 3>& r) { const auto c = cross(p, q); return r[0] * c[0] + r[1] * c[1] + r[2] * c[2]; }
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
i64 min_cover_distance(const Box3& box, const Point3& a, const Point3& b) {
  i64 out = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 lo = 2 * static_cast<i64>(box.low[axis]) - a[axis] - b[axis], hi = 2 * static_cast<i64>(box.high[axis]) - a[axis] - b[axis];
    out += (lo <= 0 && hi >= 0) ? 0 : std::min(lo * lo, hi * hi);
  }
  return out;
}
i64 midpoint_distance4(const std::array<i64, 3>& center4, const Box3& box) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto delta = std::max<i64>({0, 4 * static_cast<i64>(box.low[axis]) - center4[axis], center4[axis] - 4 * static_cast<i64>(box.high[axis])});
    result += delta * delta;
  }
  return result;
}
struct Tetra { i128 delta{}; std::array<i128, 3> N{}; int sign{}; bool positive{}; };
Tetra make_tetra(const Point3& a, const Point3& b, const Point3& c, const Point3& d) {
  Tetra t;
  const auto u = sub(b, a), v = sub(c, a), w = sub(d, a);
  const auto vw = cross(v, w), wu = cross(w, u), uv = cross(u, v);
  t.delta = static_cast<i128>(dot(u, vw));
  if (t.delta == 0) return t;
  const i64 uu = dot(u, u), vv = dot(v, v), ww = dot(w, w);
  for (std::size_t axis = 0; axis < 3; ++axis) t.N[axis] = static_cast<i128>(uu) * vw[axis] + static_cast<i128>(vv) * wu[axis] + static_cast<i128>(ww) * uv[axis];
  t.sign = t.delta > 0 ? 1 : -1;
  const i128 lb = det3(t.N, v, w), lc = det3(u, t.N, w), ld = det3(u, v, t.N);
  const i128 twice = static_cast<i128>(2) * t.delta * t.delta;
  t.positive = lb > 0 && lc > 0 && ld > 0 && twice > lb + lc + ld;
  return t;
}
int side(const Tetra& t, const Point3& a, const Point3& z) {
  const auto d = sub(z, a);
  i128 lhs = t.delta * static_cast<i128>(dot(d, d)), rhs = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) rhs += static_cast<i128>(d[axis]) * t.N[axis];
  const i128 f = static_cast<i128>(t.sign) * (lhs - rhs);
  return f < 0 ? -1 : (f > 0 ? 1 : 0);
}
__extension__ typedef unsigned __int128 u128;
u128 magnitude(i128 x) { return x < 0 ? static_cast<u128>(-(x + 1)) + 1 : static_cast<u128>(x); }
u128 gcd128(u128 a, u128 b) { while (b) { const u128 r = a % b; a = b; b = r; } return a; }
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
using Ball = std::pair<std::array<i128, 5>, u64>;  // (clé réduite, profondeur)
struct Record { u64 depth; std::array<std::size_t, 4> ids; std::vector<std::size_t> shell; };
}  // namespace
int main(int argc, char** argv) {
  if (argc < 5) { std::fprintf(stderr, "usage: file.u16le kmax s records.txt [samples=2000] [seed=1]\n"); return 2; }
  const std::string file = argv[1];
  const unsigned kmax = static_cast<unsigned>(std::atoi(argv[2]));
  const unsigned s = static_cast<unsigned>(std::atoi(argv[3]));
  const std::string records_path = argv[4];
  const std::size_t samples = argc > 5 ? std::strtoull(argv[5], nullptr, 10) : 2000;
  const unsigned seed = argc > 6 ? static_cast<unsigned>(std::atoi(argv[6])) : 1;
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
  const std::size_t n = points.size();
  std::vector<Record> records;
  {
    std::ifstream in(records_path);
    if (!in) { std::fprintf(stderr, "cannot open %s\n", records_path.c_str()); return 2; }
    std::string line;
    while (std::getline(in, line)) {
      if (line.empty()) continue;
      std::istringstream ss(line);
      Record r{};
      std::string bar;
      if (!(ss >> r.depth >> r.ids[0] >> r.ids[1] >> r.ids[2] >> r.ids[3] >> bar) || bar != "|") { std::fprintf(stderr, "bad record line\n"); return 2; }
      std::size_t id;
      while (ss >> id) r.shell.push_back(id);
      for (const auto v : r.ids) if (v >= n) { std::fprintf(stderr, "record id out of range\n"); return 2; }
      records.push_back(std::move(r));
    }
  }
  auto cloud = prepare_cloud(points);
  auto index = make_q2_cloud_index(cloud);
  const auto nodes = index->spatial_nodes(); const auto order = index->spatial_order(); const auto sites = index->cloud().points();
  const unsigned h4 = kmax - 2;
  // rang de chaque site (id original -> rang dans l'ordre spatial)
  std::vector<std::size_t> rank_of(n, 0);
  for (std::size_t rank = 0; rank < n; ++rank) rank_of[order[rank]] = rank;
  // Couverture fermée de l'arête (a,b) par descente : rangs des sites, a et b exclus.
  std::vector<std::size_t> cover_ranks, st;
  auto cover_of = [&](const Point3& pa, const Point3& pb, std::size_t rank_a, std::size_t rank_b, i64 L2) {
    cover_ranks.clear(); st.assign(1, 0);
    while (!st.empty()) {
      const auto idx = st.back(); st.pop_back();
      const auto& z = nodes[idx];
      if (min_cover_distance(z.box, pa, pb) > 4 * L2) continue;
      if (z.left == Q2SpatialNode::absent) {
        const auto rank = z.range.first;
        if (rank == rank_a || rank == rank_b) continue;
        const Point3 pz = sites[order[rank]];
        i64 d2 = 0;
        for (std::size_t axis = 0; axis < 3; ++axis) { const i64 e = 2 * static_cast<i64>(pz[axis]) - pa[axis] - pb[axis]; d2 += e * e; }
        if (d2 <= 4 * L2) cover_ranks.push_back(rank);
        continue;
      }
      st.push_back(z.right); st.push_back(z.left);
    }
  };
  const auto t0 = std::chrono::steady_clock::now();
  // 1. Validité des records.
  u64 rec_total = records.size(), rec_owner_mismatch = 0, rec_not_positive = 0, rec_depth_mismatch = 0, rec_shell_mismatch = 0, rec_depth_over = 0, rec_ok = 0, rec_tests = 0;
  std::set<Ball> record_balls;
  std::vector<std::size_t> shell;
  for (const auto& r : records) {
    // arête propriétaire = plus longue, égalité départagée par la plus petite clé d'IDs
    std::size_t oa = r.ids[0], ob = r.ids[1]; i64 best = -1; std::pair<std::size_t, std::size_t> best_key{};
    for (std::size_t i = 0; i < 4; ++i) for (std::size_t j = i + 1; j < 4; ++j) {
      const i64 e = sq_dist(points[r.ids[i]], points[r.ids[j]]);
      const auto k = edge_key(r.ids[i], r.ids[j]);
      if (e > best || (e == best && k < best_key)) { best = e; best_key = k; oa = r.ids[i]; ob = r.ids[j]; }
    }
    std::array<std::size_t, 2> others{}; std::size_t m = 0;
    for (const auto v : r.ids) if (v != oa && v != ob) others[m++] = v;
    if (m != 2) { ++rec_owner_mismatch; continue; }
    const Point3 pa = points[oa], pb = points[ob], px = points[others[0]], py = points[others[1]];
    const Tetra t = make_tetra(pa, pb, px, py);
    if (!t.positive) { ++rec_not_positive; continue; }
    const i64 L2 = sq_dist(pa, pb);
    cover_of(pa, pb, rank_of[oa], rank_of[ob], L2);
    u64 depth = 0; shell.assign({oa, ob, others[0], others[1]});
    for (const auto rank_z : cover_ranks) {
      const auto id = order[rank_z];
      if (id == others[0] || id == others[1]) continue;
      ++rec_tests;
      const int sd = side(t, pa, sites[id]);
      if (sd < 0) ++depth; else if (sd == 0) shell.push_back(id);
    }
    std::sort(shell.begin(), shell.end());
    if (depth != r.depth) { ++rec_depth_mismatch; continue; }
    if (depth >= h4) { ++rec_depth_over; continue; }
    std::vector<std::size_t> rs = r.shell; std::sort(rs.begin(), rs.end());
    if (rs != shell) { ++rec_shell_mismatch; continue; }
    ++rec_ok;
    record_balls.insert({key(t, pa), depth});
  }
  const double validity_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  // 2. Complétude par échantillon sur la masse résiduelle q4 du front.
  std::vector<std::pair<std::uint32_t, std::uint32_t>> rects;
  auto res = run_wspd_front(*index, kmax, s, WspdFrontMode::MidpointSamples,
                            [&](const WspdRectangle& r) { if (r.lane_mask & 4U) rects.push_back({static_cast<std::uint32_t>(r.a_node), static_cast<std::uint32_t>(r.b_node)}); }, 6);
  std::vector<u64> prefix{0};
  for (const auto& rc : rects) prefix.push_back(prefix.back() + static_cast<u64>(nodes[rc.first].range.size()) * nodes[rc.second].range.size());
  std::mt19937_64 rng(seed);
  u64 sp_pairs = 0, sp_rejectable = 0, sp_kept = 0, sp_tetras = 0, sp_positive = 0, sp_balls = 0, sp_missing = 0, sp_tests = 0;
  const auto t1 = std::chrono::steady_clock::now();
  if (prefix.back() > 0) {
    std::uniform_int_distribution<u64> pick(0, prefix.back() - 1);
    for (std::size_t m = 0; m < samples; ++m) {
      const u64 t = pick(rng);
      const std::size_t slot = static_cast<std::size_t>(std::upper_bound(prefix.begin(), prefix.end(), t) - prefix.begin()) - 1;
      const auto& A = nodes[rects[slot].first]; const auto& B = nodes[rects[slot].second];
      const u64 offset = t - prefix[slot];
      const auto rank_a = A.range.first + static_cast<std::size_t>(offset / B.range.size());
      const auto rank_b = B.range.first + static_cast<std::size_t>(offset % B.range.size());
      const Point3 pa = sites[order[rank_a]], pb = sites[order[rank_b]];
      const auto ida = order[rank_a], idb = order[rank_b];
      const i64 L2 = sq_dist(pa, pb);
      ++sp_pairs;
      // citron exact par descente saturante
      u64 lemon = 0;
      {
        const Box3 box_a0 = singleton_box(pa), box_b0 = singleton_box(pb);
        const Q2JointPreparedBounds pb0(box_a0, box_b0);
        std::array<i64, 3> mid4{};
        for (std::size_t axis = 0; axis < 3; ++axis) mid4[axis] = 2 * (static_cast<i64>(pa[axis]) + pb[axis]);
        st.assign(1, 0);
        while (!st.empty() && lemon < h4) {
          const auto idx = st.back(); st.pop_back();
          const auto& z = nodes[idx];
          const auto bnd = pb0.bounds(z.box);
          if (bnd.maximum4 <= 0) continue;
          if (z.left == Q2SpatialNode::absent) {
            const auto rank = z.range.first;
            if (rank != rank_a && rank != rank_b && lemon4(pa, pb, sites[order[rank]])) ++lemon;
            continue;
          }
          if (bnd.minimum4 > 0) {
            const i128 xi16 = static_cast<i128>(16) * spindle_detail::xi_bounds(box_a0, box_b0, z.box).high;
            if (static_cast<i128>(2) * (static_cast<i128>(bnd.minimum4) * bnd.minimum4) > xi16) { lemon = std::min<u64>(h4, lemon + z.range.size()); continue; }
          }
          const bool lf = midpoint_distance4(mid4, nodes[z.left].box) <= midpoint_distance4(mid4, nodes[z.right].box);
          st.push_back(lf ? z.right : z.left); st.push_back(lf ? z.left : z.right);
        }
      }
      if (lemon >= h4) { ++sp_rejectable; continue; }
      ++sp_kept;
      cover_of(pa, pb, rank_a, rank_b, L2);
      const auto owner = edge_key(ida, idb);
      for (std::size_t i = 0; i < cover_ranks.size(); ++i) {
        const auto idx = order[cover_ranks[i]]; const Point3 px = sites[idx];
        const i64 ax = sq_dist(pa, px), bx = sq_dist(pb, px);
        if (ax > L2 || (ax == L2 && edge_key(ida, idx) < owner)) continue;
        if (bx > L2 || (bx == L2 && edge_key(idb, idx) < owner)) continue;
        for (std::size_t j = i + 1; j < cover_ranks.size(); ++j) {
          const auto idy = order[cover_ranks[j]]; const Point3 py = sites[idy];
          ++sp_tetras;
          const i64 ay = sq_dist(pa, py), by = sq_dist(pb, py), xy = sq_dist(px, py);
          if (ay > L2 || (ay == L2 && edge_key(ida, idy) < owner)) continue;
          if (by > L2 || (by == L2 && edge_key(idb, idy) < owner)) continue;
          if (xy > L2 || (xy == L2 && edge_key(idx, idy) < owner)) continue;
          const Tetra t = make_tetra(pa, pb, px, py);
          if (!t.positive) continue;
          ++sp_positive;
          u64 depth = 0;
          for (const auto rank_z : cover_ranks) {
            if (rank_z == cover_ranks[i] || rank_z == cover_ranks[j]) continue;
            ++sp_tests;
            if (side(t, pa, sites[order[rank_z]]) < 0 && ++depth >= h4) break;
          }
          if (depth >= h4) continue;
          ++sp_balls;
          if (!record_balls.count({key(t, pa), depth})) ++sp_missing;
        }
      }
    }
  }
  const double sample_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t1).count();
  const auto& w = res.work;
  std::printf("{\"schema\":\"audit_b_q4_bilateral_probe_v1\",\"file\":\"%s\",\"n\":%zu,\"kmax\":%u,\"s\":%u,\"threshold\":%u,\"samples\":%zu,\"seed\":%u",
              file.c_str(), n, kmax, s, h4, samples, seed);
  std::printf(",\"records\":{\"total\":%llu,\"valid\":%llu,\"owner_mismatch\":%llu,\"not_positive\":%llu,\"depth_mismatch\":%llu,\"depth_over_threshold\":%llu,\"shell_mismatch\":%llu,\"distinct_balls\":%zu,\"point_tests\":%llu,\"ms\":%.1f}",
              (unsigned long long)rec_total, (unsigned long long)rec_ok, (unsigned long long)rec_owner_mismatch, (unsigned long long)rec_not_positive, (unsigned long long)rec_depth_mismatch,
              (unsigned long long)rec_depth_over, (unsigned long long)rec_shell_mismatch, record_balls.size(), (unsigned long long)rec_tests, validity_ms);
  std::printf(",\"front\":{\"residual_pair_mass\":[%llu,%llu,%llu],\"total_unordered_pairs\":%llu}",
              (unsigned long long)w.residual_pair_mass[0], (unsigned long long)w.residual_pair_mass[1], (unsigned long long)w.residual_pair_mass[2], (unsigned long long)res.total_unordered_pairs);
  std::printf(",\"completeness\":{\"pairs\":%llu,\"rejectable\":%llu,\"kept\":%llu,\"tetra_candidates\":%llu,\"positive\":%llu,\"balls\":%llu,\"missing\":%llu,\"point_tests\":%llu,\"ms\":%.1f}}\n",
              (unsigned long long)sp_pairs, (unsigned long long)sp_rejectable, (unsigned long long)sp_kept, (unsigned long long)sp_tetras, (unsigned long long)sp_positive,
              (unsigned long long)sp_balls, (unsigned long long)sp_missing, (unsigned long long)sp_tests, sample_ms);
  return 0;
}
