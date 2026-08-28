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
#include <cmath>
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
#ifndef MHGP5_PROBE_DIRTY
#define MHGP5_PROBE_DIRTY "inconnu"
#endif

int main(int argc, char** argv) {
  std::printf("rect_probe pin_configure=%s worktree_src_modifie=%s (HEAD et etat de src/ lus par CMake a la CONFIGURATION ; reconfigurer apres tout commit)\n",
              MHGP5_PROBE_PIN, MHGP5_PROBE_DIRTY);
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
  u64 t_prod_ns = 0, t_sect_ns = 0, t_prodtest_ns = 0;
  const bool float_on = float_filter_runtime_enabled();
  // PALMARES des rectangles les plus lourds (seeds) : D_max, |A|.|B|, ancres
  // vivantes, tuees par la production, seeds, survivants — repond a « le
  // travail lourd est-il inherent (survivants) ou gaspille (zero survivant) ? ».
  struct Heavy { u64 dmax2, nab, alive, killed, seeds, surv, cover, kw, ks; size_t idx; };
  std::vector<Heavy> heavy;
  u64 seeds_in_zero_surv = 0, seeds_in_killed_free = 0;
  size_t rect_idx = 0;
  for (const AliveRect& ar : alive) {
    const size_t my_idx = rect_idx++;
    u64 r_killed = 0, r_kw = 0, r_ks = 0;
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
        if (lane == 3) generate_detail::scan_anchor_q3(ix, sc, ua, ub, pa, pb, D2, h_of[1], float_on, false, &lo, &ls, generate_detail::AnchorPretests::kCounterfactual);
        else generate_detail::process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h_of[2], float_on, false, false, false, &lo, &ls, generate_detail::AnchorPretests::kCounterfactual);
        t_prod_ns += (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - tp0).count();
        const u64 sd = (lane == 3 ? ls.seeds[0] : ls.seeds[1]) - s0;
        r_seeds += sd; r_surv += lo.size();
        anchor_seeds.add(sd);
        // Test par secteurs (mesure) : une ancre tuee par secteurs ne doit JAMAIS avoir de survivant (sinon le test est faux).
        u64 m4 = 0, m8 = 0;
        const auto ts0 = std::chrono::steady_clock::now();
        sector_witness_min(ix, sc.cover, ua, ub, pa, pb, D2, lane, &m4, &m8);  // variante de sonde (K=4 et K=8, losange/parallelogramme)
        const auto ts1 = std::chrono::steady_clock::now();
        // Test de PRODUCTION (W_q exact cumule avec les secteurs de sector_kill.hpp) : verdict et seeds evites.
        {
          const int k = anchor_kill_cumulated(sc.cover, ix.upos, ua, ub, pa, pb, D2, lane == 3 ? Lane::kQ3 : Lane::kQ4,
                                              lane == 3 ? 12 : 8, h_of[li]);
          if (k != 0) { ++sk.killed_prod; sk.seeds_avoided_prod += sd; ++r_killed; if (!lo.empty()) ++sk.wrong; }
          if (k == 1) { ++sk.killed_w; ++r_kw; }
          if (k == 2) ++r_ks;
        }
        const auto ts2 = std::chrono::steady_clock::now();
        t_sect_ns += (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(ts1 - ts0).count();
        t_prodtest_ns += (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(ts2 - ts1).count();
        const u64 h = h_of[li];
        if (m4 >= h) { ++sk.killed4; sk.seeds_avoided4 += sd; if (!lo.empty()) ++sk.wrong; }
        if (m8 >= h) { ++sk.killed8; sk.seeds_avoided8 += sd; if (!lo.empty()) ++sk.wrong; }
      }
    rect_anchors_alive.add(r_alive); rect_cover_sum.add(r_cover); rect_seeds.add(r_seeds); rect_surv.add(r_surv);
    anchors_alive_total += r_alive; seeds_total += r_seeds; surv_total += r_surv;
    {
      const AxisBox ba = ix.box_of(ar.r.a), bb = ix.box_of(ar.r.b);
      u64 dmax2 = 0;
      for (int i = 0; i < 3; ++i) {
        const i64 w = std::max(std::llabs(ba.hi[i] - bb.lo[i]), std::llabs(bb.hi[i] - ba.lo[i]));
        dmax2 += (u64)(w * w);
      }
      heavy.push_back(Heavy{dmax2, (u64)nab, r_alive, r_killed, r_seeds, r_surv, r_cover, r_kw, r_ks, my_idx});
      if (r_surv == 0) seeds_in_zero_surv += r_seeds;
    }
  }
  std::sort(heavy.begin(), heavy.end(), [](const Heavy& x, const Heavy& y) { return x.seeds > y.seeds; });
  {
    u64 top_seeds = 0, top_surv = 0;
    const size_t ntop = std::min<size_t>(heavy.size(), heavy.size() / 100 + 1);
    for (size_t i = 0; i < ntop; ++i) { top_seeds += heavy[i].seeds; top_surv += heavy[i].surv; }
    std::printf("palmares : 1 %% des rectangles les plus lourds (%zu) = %.1f %% des seeds, %.1f %% des survivants ; seeds dans les rectangles SANS survivant = %.1f %%\n",
                ntop, seeds_total ? 100.0 * (double)top_seeds / (double)seeds_total : 0.0, surv_total ? 100.0 * (double)top_surv / (double)surv_total : 0.0,
                seeds_total ? 100.0 * (double)seeds_in_zero_surv / (double)seeds_total : 0.0);
    for (size_t i = 0; i < std::min<size_t>(heavy.size(), 12); ++i)
      std::printf("  rect#%zu Dmax=%.0f |A||B|=%llu vivantes=%llu tuees_prod=%llu seeds=%llu survivants=%llu covers=%llu\n", i, std::sqrt((double)heavy[i].dmax2),
                  (unsigned long long)heavy[i].nab, (unsigned long long)heavy[i].alive, (unsigned long long)heavy[i].killed, (unsigned long long)heavy[i].seeds,
                  (unsigned long long)heavy[i].surv, (unsigned long long)heavy[i].cover);
  }
  // CLASSES DE D_max (log2) : le palmares dit ou est le travail CONTREFACTUEL
  // (celui que les tests d'ancre evitent) ; ce tableau dit ou est le travail
  // QUI RESTE apres eux — le « regime intermediaire ». Par classe : nombre de
  // rectangles, paires, ancres vivantes, ancres tuees par la production,
  // seeds contrefactuels, seeds RESTANTS (contrefactuels des ancres NON tuees),
  // survivants. Sans cette colonne « restants », on optimise ce qui est deja
  // gratuit.
  {
    constexpr int kCls = 24;
    struct Cls { u64 rects, nab, alive, killed, seeds, surv, cover, kw, ks; } cls[kCls] = {};
    for (const Heavy& hh : heavy) {
      int c = 0;
      while (c + 1 < kCls && (hh.dmax2 >> (2 * (c + 1))) > 0) ++c;  // classe = floor(log2(Dmax))
      cls[c].rects += 1; cls[c].nab += hh.nab; cls[c].alive += hh.alive; cls[c].killed += hh.killed;
      cls[c].seeds += hh.seeds; cls[c].surv += hh.surv; cls[c].cover += hh.cover;
      cls[c].kw += hh.kw; cls[c].ks += hh.ks;
    }
    // Le bloc `plafond_test_rectangle` a ete RETIRE (audit du 28 aout) : W,
    // secteurs et grille sont des conditions SUFFISANTES et non necessaires,
    // un futur certificat peut tuer ce qu'elles laissent vivre, et ses
    // populations n'etaient pas alignees (`alive` post-histogramme en q3 mais
    // post-W4 en q4, `killed` omettant W4 et la grille). Ce n'etait donc pas
    // un plafond. Ce qui borne reellement le gain accessible au CERTIFICAT
    // UNIVERSEL est la masse de paires tuees par `k=1` (W_q) — ventilee
    // ci-dessous — car un rectangle dont les ancres ne meurent que par
    // secteurs ne sera jamais supprime par un raffinement de boites.
    // DESCENTE PROLONGEE (l'experience decisive) : `alive_rectangles` arrete
    // la descente ternaire des que le rectangle est SEPARE. Rien n'oblige a
    // s'y arreter pour le TRAVAIL : scinder un rectangle vivant ne change ni
    // les paires enumerees ni l'objet (la partition des paires est inchangee,
    // le critere terminal de la WSPD n'est pas touche), mais chaque scission
    // RESSERRE les boites, donc AUGMENTE le nombre de temoins universels — un
    // sous-rectangle peut mourir la ou son parent vivait.
    // On mesure ici, sans rien changer a la production : combien de paires la
    // descente prolongee elimine, et ce qu'elle coute en evaluations de cœur.
    // Le cout est borne : on ne descend jamais sous `kStop` paires, et la
    // descente d'un rectangle visite au plus 2*(nombre de feuilles) nœuds,
    // donc au plus 2*|A||B| — ce que l'enumeration des paires coutait deja.
    {
      constexpr u64 kStop = 4;      // en dessous, enumerer les paires est plus court que tester
      constexpr int kMaxDepth = 40;
      u64 pairs_in = 0, pairs_killed = 0, pairs_left = 0, core_evals = 0, subrects = 0, depth_max = 0;
      u64 seeds_killed_est = 0, rect_entier_tue = 0, rect_touche = 0;
      u64 core_nodes = 0, core_corners = 0, cover_evite_est = 0;
      struct Sub { NodeRef a, b; int d; };
      std::vector<Sub> st2;
      for (const Heavy& hh : heavy) {
        if (hh.alive == 0) continue;
        const AliveRect& ar = alive[hh.idx];
        const u64 nab0 = hh.nab;
        pairs_in += nab0;
        ++rect_touche;
        u64 killed_here = 0;
        st2.clear();
        st2.push_back(Sub{ar.r.a, ar.r.b, 0});
        while (!st2.empty()) {
          const Sub sb = st2.back();
          st2.pop_back();
          ++subrects;
          depth_max = std::max<u64>(depth_max, (u64)sb.d);
          const NodeRange qa = ix.range_of(sb.a), qb = ix.range_of(sb.b);
          const u64 npairs = (u64)(qa.last - qa.first + 1) * (u64)(qb.last - qb.first + 1);
          ++core_evals;
          const FusedCounts fc = count_universal_witnesses(ix, sb.a, sb.b, h_of, (u8)(1u << li), true);
          core_nodes += fc.nodes_visited;
          core_corners += fc.corner_evals;
          if (fc.c[li] >= h_of[li]) { killed_here += npairs; continue; }  // sous-rectangle MORT
          if (npairs <= kStop || sb.d >= kMaxDepth || (sb.a < 0 && sb.b < 0)) { pairs_left += npairs; continue; }
          const AxisBox va = ix.box_of(sb.a), vb = ix.box_of(sb.b);
          const i64 w2a = wspd_detail::box_w2(va), w2b = wspd_detail::box_w2(vb);
          const bool split_a = (sb.a >= 0) && (sb.b < 0 || w2a >= w2b);
          const NodeRef keep = split_a ? sb.b : sb.a;
          const RadixNode& nd = ix.nodes[(size_t)(split_a ? sb.a : sb.b)];
          st2.push_back(split_a ? Sub{nd.left, keep, sb.d + 1} : Sub{keep, nd.left, sb.d + 1});
          st2.push_back(split_a ? Sub{nd.right, keep, sb.d + 1} : Sub{keep, nd.right, sb.d + 1});
        }
        pairs_killed += killed_here;
        if (killed_here == nab0) { ++rect_entier_tue; seeds_killed_est += hh.seeds; cover_evite_est += hh.cover; }
        else if (nab0) {
          seeds_killed_est += (u64)((double)hh.seeds * (double)killed_here / (double)nab0);
          cover_evite_est += (u64)((double)hh.cover * (double)killed_here / (double)nab0);
        }
      }
      std::printf("descente_prolongee : %llu rectangles traites, %llu paires -> %llu tuees (%.1f %%), %llu restantes ; "
                  "%llu sous-rectangles, %llu evaluations de coeur (%llu nœuds visites, %llu evaluations de coin), profondeur max %llu ; "
                  "%llu rectangles ENTIEREMENT tues ; seeds evites (ESTIMATION AU PRORATA, non mesuree) %llu / %llu (%.1f %%) ; "
                  "sites de cover evites (meme estimation) %llu — a comparer aux %llu nœuds visites du test\n",
                  (unsigned long long)rect_touche, (unsigned long long)pairs_in, (unsigned long long)pairs_killed,
                  pairs_in ? 100.0 * (double)pairs_killed / (double)pairs_in : 0.0, (unsigned long long)pairs_left,
                  (unsigned long long)subrects, (unsigned long long)core_evals, (unsigned long long)core_nodes,
                  (unsigned long long)core_corners, (unsigned long long)depth_max,
                  (unsigned long long)rect_entier_tue, (unsigned long long)seeds_killed_est, (unsigned long long)seeds_total,
                  seeds_total ? 100.0 * (double)seeds_killed_est / (double)seeds_total : 0.0,
                  (unsigned long long)cover_evite_est, (unsigned long long)core_nodes);
    }
    {
      u64 tk1 = 0, tk2 = 0, talive = 0;
      for (const Heavy& hh : heavy) { tk1 += hh.kw; tk2 += hh.ks; talive += hh.alive; }
      std::printf("mortalite_par_cause : ancres vivantes=%llu, tuees par k=1 (W_q, certificat UNIVERSEL) = %llu (%.1f %%), "
                  "par k=2 (secteurs, ancre-specifique) = %llu (%.1f %%) — seule la masse k=1 borne le gain d'un raffinement de boites\n",
                  (unsigned long long)talive, (unsigned long long)tk1, talive ? 100.0 * (double)tk1 / (double)talive : 0.0,
                  (unsigned long long)tk2, talive ? 100.0 * (double)tk2 / (double)talive : 0.0);
    }
    std::printf("classes_dmax (Dmax dans [2^c, 2^(c+1)))\n");
    for (int c = 0; c < kCls; ++c) {
      if (!cls[c].rects) continue;
      const double part_ancres = cls[c].alive ? 100.0 * (double)cls[c].killed / (double)cls[c].alive : 0.0;
      std::printf("  c=%2d Dmax<2^%d rects=%llu paires=%llu vivantes=%llu tuees_prod=%llu (%.1f %%) dont k1_W=%llu k2_secteurs=%llu seeds_cf=%llu survivants=%llu covers=%llu\n",
                  c, c + 1, (unsigned long long)cls[c].rects, (unsigned long long)cls[c].nab, (unsigned long long)cls[c].alive,
                  (unsigned long long)cls[c].killed, part_ancres, (unsigned long long)cls[c].kw, (unsigned long long)cls[c].ks,
                  (unsigned long long)cls[c].seeds, (unsigned long long)cls[c].surv, (unsigned long long)cls[c].cover);
    }
  }
  (void)seeds_in_killed_free;
  // VIDAGE de l'ancre la plus lourde (diagnostic : ou sont les centres des
  // seeds d'une ancre sans survivant ? apex ou couronne ?).
  if (!heavy.empty() && heavy[0].seeds > 0) {
    const AliveRect& ar = alive[heavy[0].idx];
    const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), coef, &sc.handles, &sc.cover_nodes);
    i32 best_a = -1, best_b = -1;
    u64 best_seeds = 0;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, coef, &sc.cover, &sc.visits, &sc.cover_tmp);
        u64 ns = 0;
        for (const CoverPoint& cx : sc.cover) {
          if (cx.u == ua || cx.u == ub) continue;
          if (is_acute_seed(pa, pb, ix.upos[(size_t)cx.u], D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cx.u))) ++ns;
        }
        if (ns > best_seeds) { best_seeds = ns; best_a = ua; best_b = ub; }
      }
    if (best_a >= 0) {
      const P3 pa = ix.upos[(size_t)best_a], pb = ix.upos[(size_t)best_b];
      const i64 D2 = p3_norm2(p3_sub(pb, pa));
      anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, coef, &sc.cover, &sc.visits, &sc.cover_tmp);
      const double D = std::sqrt((double)D2);
      u64 diam = 0;
      double min_ratio = 1e300;
      for (const CoverPoint& cz : sc.cover) {
        if (cz.u == best_a || cz.u == best_b) continue;
        const double r = (double)cz.dist2q / (double)D2;
        min_ratio = std::min(min_ratio, r);
        if (cz.dist2q < D2) ++diam;
      }
      std::printf("VIDAGE ancre la plus lourde du rectangle #0 : a=(%lld,%lld,%lld) b=(%lld,%lld,%lld) D=%.1f cover=%zu sites_boule_diametrale=%llu min(|2w|²/D²)=%.3f seeds=%llu\n",
                  (long long)pa.x, (long long)pa.y, (long long)pa.z, (long long)pb.x, (long long)pb.y, (long long)pb.z, D, sc.cover.size(),
                  (unsigned long long)diam, min_ratio, (unsigned long long)best_seeds);
      struct SeedDiag { double v3_over_D, mu_over_D; u64 depth0, core; P3 x; };
      std::vector<SeedDiag> diag;
      const double dd[3] = {(double)(pb.x - pa.x), (double)(pb.y - pa.y), (double)(pb.z - pa.z)};
      for (const CoverPoint& cx : sc.cover) {
        if (cx.u == best_a || cx.u == best_b) continue;
        const P3 px = ix.upos[(size_t)cx.u];
        if (!is_acute_seed(pa, pb, px, D2, ix.point_id(best_a), ix.point_id(best_b), ix.point_id(cx.u))) continue;
        const Q3Form f3 = q3_form(pa, pb, px);
        const double G = (double)f3.g;
        const double v3[3] = {((double)f3.w[0] - G * dd[0]) / (2 * G), ((double)f3.w[1] - G * dd[1]) / (2 * G), ((double)f3.w[2] - G * dd[2]) / (2 * G)};
        const double v3n2 = v3[0] * v3[0] + v3[1] * v3[1] + v3[2] * v3[2];
        // corde : |mu| <= sqrt(J/2), J = G(D² − 8|v3|²) ; demi-longueur = mu* |n| / (2G)
        const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
        const double nn = std::sqrt((double)p3_norm2(nrm));
        const double J = G * ((double)D2 - 8.0 * v3n2);
        const double half = J > 0 ? std::sqrt(J / 2) * nn / (2 * G) : 0.0;
        // extremites de la corde (centres) et rayons
        const double c0[3] = {(double)pa.x + (double)f3.w[0] / (2 * G), (double)pa.y + (double)f3.w[1] / (2 * G), (double)pa.z + (double)f3.w[2] / (2 * G)};
        const double un[3] = {(double)nrm.x / nn, (double)nrm.y / nn, (double)nrm.z / nn};
        u64 depth0 = 0, core = 0;
        for (const CoverPoint& cz : sc.cover) {
          if (cz.u == best_a || cz.u == best_b || cz.u == cx.u) continue;
          const P3& z = ix.upos[(size_t)cz.u];
          if (q3_power(f3, z) < 0) ++depth0;
          bool in_all = true;
          for (int sgn = -1; sgn <= 1 && in_all; sgn += 2) {
            double c[3], r2 = (double)D2 / 4;
            for (int i = 0; i < 3; ++i) c[i] = c0[i] + sgn * half * un[i];
            for (int i = 0; i < 3; ++i) { const double t = v3[i] + sgn * half * un[i]; r2 += t * t; }
            const double dz = ((double)z.x - c[0]) * ((double)z.x - c[0]) + ((double)z.y - c[1]) * ((double)z.y - c[1]) + ((double)z.z - c[2]) * ((double)z.z - c[2]);
            if (!(dz < r2)) in_all = false;
          }
          if (in_all) ++core;
        }
        diag.push_back(SeedDiag{std::sqrt(v3n2) / D, half / D, depth0, core, px});
      }
      std::sort(diag.begin(), diag.end(), [](const SeedDiag& p1, const SeedDiag& p2) { return p1.v3_over_D < p2.v3_over_D; });
      for (size_t i = 0; i < diag.size(); ++i) {
        if (i >= 10 && i + 4 < diag.size() && i != diag.size() / 2) continue;
        std::printf("  seed x=(%lld,%lld,%lld) |v3|/D=%.3f demi-corde/D=%.3f interieurs(mu=0)=%llu temoins_coeur(extremites)=%llu\n", (long long)diag[i].x.x,
                    (long long)diag[i].x.y, (long long)diag[i].x.z, diag[i].v3_over_D, diag[i].mu_over_D, (unsigned long long)diag[i].depth0, (unsigned long long)diag[i].core);
      }
    }
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
  std::printf("temps (ratios dans ce run, 1 fil, machine partagee — jamais un temps citable) : corps de production contrefactuel = %.1f ms ; "
              "variante de sonde K=4+K=8 = %.1f ms ; test de production (W_q + secteurs) = %.1f ms\n",
              (double)t_prod_ns / 1e6, (double)t_sect_ns / 1e6, (double)t_prodtest_ns / 1e6);
  if (sk.wrong) {
    std::printf("FAUX POSITIF : une ancre tuee avait un survivant — theoreme contredit, code 1\n");
    return 1;
  }
  std::printf("ratio sum_covers_ancres / points_rectangles = %.2f ; ancres vivantes par rectangle = %.2f ; seeds par survivant = %.1f\n",
              rect_points.sum() ? (double)rect_cover_sum.sum() / (double)rect_points.sum() : 0.0,
              alive.empty() ? 0.0 : (double)anchors_alive_total / (double)alive.size(),
              surv_total ? (double)seeds_total / (double)surv_total : 0.0);
  return 0;
}
