// MorseHGP3D v5 — SONDE de mesure (jamais un claim) pour la lane par rectangle
// (docs/GPU.md, livraison 7) : par rectangle vivant de la lane q3 (ou q4), la
// taille du candidat de cover issu des handles (points des plages de handles :
// un sur-ensemble fail-open des covers d'ancres), |A|·|B|, ancres survivantes
// a l'histogramme, somme des covers exacts d'ancres, seeds, survivants —
// totaux, sommes et quantiles. Repond a la question de l'auditeur : le partage
// du candidat de rectangle entre ses ancres vaut-il la copie ?
// Usage : mhgp5_rect_probe --family=F --n=N [--lane=3|4] [--coord=C]
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/lanes/edge_cover.hpp"
#include "../src/lanes/sector_kill.hpp"
#include "../src/pipeline/generate.hpp"
#include "../src/spindle/spindle.hpp"

using namespace mhgp5;

namespace {
struct Q {
  std::vector<u64> v;
  void add(u64 x) { v.push_back(x); }
  u64 sum() const { u64 s = 0; for (u64 x : v) s += x; return s; }
  u64 q(double p) { if (v.empty()) return 0; std::sort(v.begin(), v.end()); return v[std::min(v.size() - 1, (size_t)(p * (double)v.size()))]; }
  u64 mx() const { u64 m = 0; for (u64 x : v) m = std::max(m, x); return m; }
};
}  // namespace

// TEST D'ANCRE PAR SECTEURS (mesure) : les centres des boules de l'ancre (a,b)
// vivent dans le plan bissecteur de ab, a distance <= rho de m (rho = D/(2
// sqrt 3) en q3 : angle en x >= 60 deg ; rho = D/(2 sqrt 2) en q4 : Jung). Un
// site z est interieur a TOUTE boule de centre v ssi 2 w.v > |w|^2 - D^2/4
// (w = z - m) : demi-plan de centres. Le disque des centres est recouvert par
// K triangles (0, p_i, p_{i+1}) d'un polygone circonscrit a sommets ENTIERS
// p_i dans le plan bissecteur ; le minimum d'une forme lineaire sur un
// triangle est en un sommet, donc z est universel sur le secteur i ssi
// |w|^2 < D^2/4 (sommet 0) et 2 w.p_i > |w|^2 - D^2/4 et 2 w.p_{i+1} > ...
// L'ancre est morte si CHAQUE secteur compte >= h temoins universels (tout
// seed a son centre dans un secteur, donc >= h sites interieurs). Sufficient,
// exact en entiers (4|w|^2 - D^2 < 0 et 8 w.p > 4|w|^2 - D^2), jamais
// necessaire. K = 4 : sommets +-A e1, +-B e2 avec e1 = d x axe, e2 = d x e1 ;
// le losange contient le disque ssi son rayon inscrit >= rho, verifie en
// entiers : (A^2|e1|^2)(B^2|e2|^2) >= rho^2 (A^2|e1|^2 + B^2|e2|^2), avec
// rho^2 = D^2/12 (q3) ou D^2/8 (q4) — on prend A, B les plus petits entiers
// tels que A^2|e1|^2 >= 2 rho^2 et B^2|e2|^2 >= 2 rho^2 (rayon inscrit du
// losange a demi-axes L1, L2 : L1 L2 / sqrt(L1^2+L2^2) >= rho si L1, L2 >= rho
// sqrt 2). K = 8 : on ajoute les sommets A e1 +- B e2 etc. (octogone).
struct SectorKill {
  u64 killed4 = 0, killed8 = 0, seeds_avoided4 = 0, seeds_avoided8 = 0, wrong = 0;
  u64 killed_prod = 0, seeds_avoided_prod = 0, killed_w = 0;  // test de production cumule (W_q puis secteurs)
};
inline i128 sq128(i64 x) { return (i128)x * x; }
// Rend le nombre minimal de temoins universels sur les secteurs (K=4 puis K=8).
inline void sector_witness_min(const CloudIndex& ix, const std::vector<CoverPoint>& cover, i32 ua, i32 ub, const P3& pa,
                               const P3& pb, i64 D2, int lane, u64* min4, u64* min8) {
  const i64 dx = pb.x - pa.x, dy = pb.y - pa.y, dz = pb.z - pa.z;
  // Variante PARALLELOGRAMME : u = A e1, v = A' e1' avec e1, e1' les deux plus
  // grands produits d x axe (tous deux dans le plan bissecteur, longueurs ~D,
  // non orthogonaux) ; le parallelogramme (+-u, +-v) contient le disque de
  // rayon rho ssi la distance de 0 a chacune de ses aretes >= rho :
  // |u x v|^2 >= rho^2 |u - v|^2 et |u x v|^2 >= rho^2 |u + v|^2 (entiers ;
  // rho^2 = D^2/12 en q3, D^2/8 en q4). Polygone bien moins allonge que le
  // losange (e1, d x e1) — test plus fort.
  i64 e1[3], e2[3];
  {
    const i64 cands[3][3] = {{0, dz, -dy}, {-dz, 0, dx}, {dy, -dx, 0}};
    i128 nn[3];
    for (int k = 0; k < 3; ++k) nn[k] = sq128(cands[k][0]) + sq128(cands[k][1]) + sq128(cands[k][2]);
    int b1 = 0;
    for (int k = 1; k < 3; ++k) if (nn[k] > nn[b1]) b1 = k;
    int b2 = (b1 + 1) % 3;
    for (int k = 0; k < 3; ++k) if (k != b1 && nn[k] > nn[b2]) b2 = k;
    for (int i = 0; i < 3; ++i) { e1[i] = cands[b1][i]; e2[i] = cands[b2][i]; }
  }
  const i128 rho2_num = D2, rho2_den = lane == 3 ? 12 : 8;  // rho^2 = D2 / den
  i64 A = 1, B = 1;
  for (;;) {
    const i64 u[3] = {A * e1[0], A * e1[1], A * e1[2]}, v[3] = {B * e2[0], B * e2[1], B * e2[2]};
    const i64 cx = u[1] * v[2] - u[2] * v[1], cy = u[2] * v[0] - u[0] * v[2], cz = u[0] * v[1] - u[1] * v[0];
    const i128 cross2 = sq128(cx) + sq128(cy) + sq128(cz);
    const i128 dm = sq128(u[0] - v[0]) + sq128(u[1] - v[1]) + sq128(u[2] - v[2]);
    const i128 dp = sq128(u[0] + v[0]) + sq128(u[1] + v[1]) + sq128(u[2] + v[2]);
    const bool ok = cross2 * rho2_den >= rho2_num * dm && cross2 * rho2_den >= rho2_num * dp;
    if (ok) break;
    if (A <= B) ++A; else ++B;
    if (A > 64 || B > 64) break;  // garde (jamais atteint : |e| ~ D)
  }
  i64 P[8][3];
  for (int i = 0; i < 3; ++i) {
    P[0][i] = A * e1[i]; P[2][i] = B * e2[i]; P[4][i] = -A * e1[i]; P[6][i] = -B * e2[i];
    P[1][i] = A * e1[i] + B * e2[i]; P[3][i] = -A * e1[i] + B * e2[i]; P[5][i] = -A * e1[i] - B * e2[i]; P[7][i] = A * e1[i] - B * e2[i];
  }
  u64 cnt4[4] = {0, 0, 0, 0}, cnt8[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
  for (const CoverPoint& cz : cover) {
    if (cz.u == ua || cz.u == ub) continue;
    const P3& z = ix.upos[(size_t)cz.u];
    // w2 := 2w = 2z - (a+b) ; condition sommet 0 : |w|^2 < D^2/4  <=>  |2w|^2 < D^2 ; sommet p : 2 w.p > |w|^2 - D^2/4  <=>  4 (2w).p > |2w|^2 - D^2.
    const i64 w0 = 2 * z.x - sx, w1 = 2 * z.y - sy, w2 = 2 * z.z - sz;
    const i128 n2w = sq128(w0) + sq128(w1) + sq128(w2);
    if (n2w >= (i128)D2) continue;
    const i128 rhs = n2w - (i128)D2;  // < 0
    bool ok[8];
    for (int k = 0; k < 8; ++k) {
      const i128 dot = (i128)w0 * P[k][0] + (i128)w1 * P[k][1] + (i128)w2 * P[k][2];
      ok[k] = 4 * dot > rhs;
    }
    // K=4 : secteurs (0, P0, P2), (0, P2, P4), (0, P4, P6), (0, P6, P0).
    if (ok[0] && ok[2]) ++cnt4[0];
    if (ok[2] && ok[4]) ++cnt4[1];
    if (ok[4] && ok[6]) ++cnt4[2];
    if (ok[6] && ok[0]) ++cnt4[3];
    for (int k = 0; k < 8; ++k) if (ok[k] && ok[(k + 1) % 8]) ++cnt8[k];
  }
  *min4 = std::min({cnt4[0], cnt4[1], cnt4[2], cnt4[3]});
  *min8 = cnt8[0];
  for (int k = 1; k < 8; ++k) *min8 = std::min(*min8, cnt8[k]);
}

#ifndef MHGP5_PROBE_PIN
#define MHGP5_PROBE_PIN "inconnu"
#endif

int main(int argc, char** argv) {
  std::printf("rect_probe pin_execution=%s (git rev-parse HEAD au moment de la compilation ; worktree : voir le recu)\n", MHGP5_PROBE_PIN);
  CloudFamily family = CloudFamily::kUniform;
  int n = 2000, coord = 0, lane = 3;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--family=", 0) == 0) { if (!parse_cloud_family(a.c_str() + 9, &family)) return 2; }
    else if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
    else if (a.rfind("--coord=", 0) == 0) coord = std::atoi(a.c_str() + 8);
    else if (a.rfind("--lane=", 0) == 0) lane = std::atoi(a.c_str() + 7);
    else return 2;
  }
  if (lane != 3 && lane != 4) return 2;
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, 3));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  const u64 smax = 11;
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax), lane_h(Lane::kQ3, smax), lane_h(Lane::kQ4, smax)};
  const int li = lane == 3 ? 1 : 2;
  const Lane L = lane == 3 ? Lane::kQ3 : Lane::kQ4;
  const i64 coef = 3;
  std::vector<AliveRect> alive;
  u64 visited = 0, workers = 0;
  generate_detail::alive_rectangles(ix, 8, h_of, li, 1, &alive, &visited, &workers);
  generate_detail::AnchorScratch sc;
  Q rect_points, rect_handles, rect_ab, rect_anchors_alive, rect_cover_sum, rect_seeds, rect_surv, anchor_cover, anchor_seeds;
  u64 anchors_total = 0, anchors_hist = 0, anchors_w4 = 0, anchors_alive_total = 0, seeds_total = 0, surv_total = 0;
  SectorKill sk;
  u64 t_prod_ns = 0, t_sect_ns = 0, t_prod_killed_ns = 0;
  const bool float_on = float_filter_runtime_enabled();
  for (const AliveRect& ar : alive) {
    generate_detail::corner_histograms(ix, L, ar.r, &sc.ha, &sc.hb);
    const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), coef, &sc.handles, &sc.cover_nodes);
    u64 pts = 0;
    for (const NodeRef h : sc.handles) { const NodeRange r = ix.range_of(h); pts += (u64)(r.last - r.first + 1); }
    rect_points.add(pts);
    rect_handles.add(sc.handles.size());
    const u64 nab = (u64)(ra.last - ra.first + 1) * (u64)(rb.last - rb.first + 1);
    rect_ab.add(nab);
    anchors_total += nab;
    const u64 need = h_of[li] - ar.core;
    u64 r_alive = 0, r_cover = 0, r_seeds = 0, r_surv = 0;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        if (sc.ha[(size_t)(ua - ra.first)] + sc.hb[(size_t)(ub - rb.first)] >= need) { ++anchors_hist; continue; }
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, coef, &sc.cover, &sc.visits, &sc.cover_tmp);
        if (lane == 4) {
          u64 n4 = 0;
          for (const CoverPoint& cz : sc.cover) {
            if (cz.u == ua || cz.u == ub) continue;
            if (in_spindle(Lane::kQ4, pa, pb, ix.upos[(size_t)cz.u]) && ++n4 >= h_of[2]) break;
          }
          if (n4 >= h_of[2]) { ++anchors_w4; continue; }
        }
        ++r_alive;
        r_cover += sc.cover.size();
        anchor_cover.add(sc.cover.size());
        std::vector<BallCandidate> lo;
        GenerateStats ls;
        const u64 s0 = lane == 3 ? ls.seeds[0] : ls.seeds[1];
        const auto tp0 = std::chrono::steady_clock::now();
        // CONTREFACTUEL : corps de production SANS les tests d'ancre (anchor_tests = false), sinon la mesure
        // de « seeds evites » et de « faux positifs » serait circulaire (la production applique deja les tests).
        if (lane == 3) generate_detail::scan_anchor_q3(ix, sc, ua, ub, pa, pb, D2, h_of[1], float_on, false, &lo, &ls, false);
        else generate_detail::process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h_of[2], float_on, false, false, false, &lo, &ls, false);
        t_prod_ns += (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - tp0).count();
        const u64 sd = (lane == 3 ? ls.seeds[0] : ls.seeds[1]) - s0;
        r_seeds += sd; r_surv += lo.size();
        anchor_seeds.add(sd);
        // Test par secteurs (mesure) : une ancre tuee par secteurs ne doit JAMAIS avoir de survivant (sinon le test est faux).
        u64 m4 = 0, m8 = 0;
        const auto ts0 = std::chrono::steady_clock::now();
        sector_witness_min(ix, sc.cover, ua, ub, pa, pb, D2, lane, &m4, &m8);
        // Test de PRODUCTION (W_q exact cumule avec les secteurs de sector_kill.hpp) : verdict et seeds evites.
        {
          const int k = anchor_kill_cumulated(sc.cover, ix.upos, ua, ub, pa, pb, D2, lane == 3 ? Lane::kQ3 : Lane::kQ4,
                                              lane == 3 ? 12 : 8, h_of[li]);
          if (k != 0) { ++sk.killed_prod; sk.seeds_avoided_prod += sd; if (!lo.empty()) ++sk.wrong; }
          if (k == 1) ++sk.killed_w;
        }
        t_sect_ns += (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - ts0).count();
        if (m4 >= h_of[li]) t_prod_killed_ns += (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - tp0).count() - (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - ts0).count() * 0;
        const u64 h = h_of[li];
        if (m4 >= h) { ++sk.killed4; sk.seeds_avoided4 += sd; if (!lo.empty()) ++sk.wrong; }
        if (m8 >= h) { ++sk.killed8; sk.seeds_avoided8 += sd; if (!lo.empty()) ++sk.wrong; }
      }
    rect_anchors_alive.add(r_alive); rect_cover_sum.add(r_cover); rect_seeds.add(r_seeds); rect_surv.add(r_surv);
    anchors_alive_total += r_alive; seeds_total += r_seeds; surv_total += r_surv;
  }
  const auto line = [](const char* name, Q& q) {
    std::printf("%-22s somme=%llu p50=%llu p90=%llu p99=%llu max=%llu\n", name, (unsigned long long)q.sum(), (unsigned long long)q.q(0.5),
                (unsigned long long)q.q(0.9), (unsigned long long)q.q(0.99), (unsigned long long)q.mx());
  };
  std::printf("rect_probe famille=%s n=%d lane=q%d rectangles=%zu ancres=%llu tuees_hist=%llu tuees_w4=%llu vivantes=%llu seeds=%llu survivants=%llu\n",
              cloud_family_name(family), n, lane, alive.size(), (unsigned long long)anchors_total, (unsigned long long)anchors_hist,
              (unsigned long long)anchors_w4, (unsigned long long)anchors_alive_total, (unsigned long long)seeds_total,
              (unsigned long long)surv_total);
  line("rect_points(handles)", rect_points);
  line("rect_handles", rect_handles);
  line("rect_|A|x|B|", rect_ab);
  line("rect_ancres_vivantes", rect_anchors_alive);
  line("rect_sum_covers", rect_cover_sum);
  line("rect_seeds", rect_seeds);
  line("rect_survivants", rect_surv);
  line("ancre_cover", anchor_cover);
  line("ancre_seeds", anchor_seeds);
  std::printf("secteurs : K=4 tue %llu ancres vivantes (%.1f %%), seeds evites %llu (%.1f %%) ; K=8 tue %llu (%.1f %%), seeds evites %llu (%.1f %%) ; FAUX POSITIFS (ancre tuee avec survivant) = %llu\n",
              (unsigned long long)sk.killed4, anchors_alive_total ? 100.0 * (double)sk.killed4 / (double)anchors_alive_total : 0.0,
              (unsigned long long)sk.seeds_avoided4, seeds_total ? 100.0 * (double)sk.seeds_avoided4 / (double)seeds_total : 0.0,
              (unsigned long long)sk.killed8, anchors_alive_total ? 100.0 * (double)sk.killed8 / (double)anchors_alive_total : 0.0,
              (unsigned long long)sk.seeds_avoided8, seeds_total ? 100.0 * (double)sk.seeds_avoided8 / (double)seeds_total : 0.0,
              (unsigned long long)sk.wrong);
  std::printf("production (W_q exact + secteurs cumules) : tue %llu ancres vivantes (%.1f %%), dont %llu par W_q, seeds evites %llu (%.1f %%)\n",
              (unsigned long long)sk.killed_prod, anchors_alive_total ? 100.0 * (double)sk.killed_prod / (double)anchors_alive_total : 0.0,
              (unsigned long long)sk.killed_w, (unsigned long long)sk.seeds_avoided_prod,
              seeds_total ? 100.0 * (double)sk.seeds_avoided_prod / (double)seeds_total : 0.0);
  std::printf("temps : corps de production par ancre (total) = %.1f ms ; test par secteurs (K=4 et 8 ensemble) = %.1f ms\n",
              (double)t_prod_ns / 1e6, (double)t_sect_ns / 1e6);
  (void)t_prod_killed_ns;
  std::printf("ratio sum_covers_ancres / points_rectangles = %.2f ; ancres vivantes par rectangle = %.2f ; seeds par survivant = %.1f\n",
              rect_points.sum() ? (double)rect_cover_sum.sum() / (double)rect_points.sum() : 0.0,
              alive.empty() ? 0.0 : (double)anchors_alive_total / (double)alive.size(),
              surv_total ? (double)seeds_total / (double)surv_total : 0.0);
  return 0;
}
