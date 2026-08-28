// MorseHGP3D v5 — ORACLE BRUT de la grille de cellules (src/lanes/cell_grid.hpp,
// theoreme 10.5 ; reponse des auditeurs du 28 aout 2026, V15 points 3 et 4).
// Deux autorites INDEPENDANTES, sans deux pointeurs ni flottant :
//   (A) COMPTEUR : pour chaque ancre (a,b) — toutes les paires de trois petits
//       nuages, les fixtures F9 (vallee), F10 (13 sites sur la frontiere des
//       sommets i' = 0), F11 (frontieres de la localisation) et une ancre a
//       coordonnees u16 EXTREMES (0..65535) — et chaque lane (rho² = D²/12 en
//       q3, D²/8 en q4), CellGrid::build (deux pointeurs, tableau de
//       differences, prefixes/suffixes) est compare CELLULE PAR CELLULE (les
//       256) a l'evaluation DIRECTE i128 des 289 sommets pour chaque site :
//       un site compte pour la cellule ssi 4 w'·(i'u + j'v) > G(|w'|² − D²) a
//       ses quatre sommets (strict). Meme comparaison pour `needed_cells`,
//       `dead_cells`, `all_dead`. Le primitif `count_site_t` est de plus
//       exerce SITE PAR SITE dans ses quatre instanciations (i64/i128 ×
//       strict/non strict) contre l'evaluation directe strict/non strict —
//       le chemin i128 de `build` est INATTEIGNABLE sous le profil u16
//       (|4G(|du|+|dv|)| < 2^49 << 2^62, mesure ici : log2 maximal grave) ;
//       il est donc aussi exerce sur des sites SYNTHETIQUES de magnitude
//       ~2^100 (au-dela de l'i64), avec egalites exactes construites.
//   (B) LOCALISATEUR RATIONNEL : pour chaque seed aigu (q3 : centre v3 =
//       N/(2G3) ; q4 : les deux extremites de la corde (N ± μ̂ n)/(2G3)), les
//       coordonnees exactes αG = G(pu·vv − pv·uv)/(den·det), βG = G(pv·uu −
//       pu·uv)/(den·det) sont des RATIONNELS EXACTS (entiers signes 256 bits,
//       produits 128×128 -> 192 de core/wide.hpp : aucun point ignore) ;
//       contrat : la boite consultee par CellGrid::locate_box /
//       segment_box contient TOUTES les cellules fermees contenant le point
//       exact — [ceil(αG) − 1, floor(αG)] ⊆ [i0, i1] (idem β) : un centre
//       exactement sur une arete (F11 : αG = 1) consulte ses DEUX cellules.
// Planchers (code 3) : >= 10^5 paires (site, cellule) comparees ; sites a
// du < 0, du = 0, du > 0, dv < 0, dv = 0, dv > 0 ; >= 1 egalite exacte
// (site, sommet) ; >= 1000 sites par le primitif i128 et >= 1000 synthetiques ;
// >= 1000 points localises ; >= 1 point exactement sur une arete.
// Mutants tues (code 4) : `cell-kill-nonstrict` (le compteur non strict
// s'ecarte de l'autorite stricte sur les egalites de F10) ;
// `cell-locate-eps-zero` (marge nulle : le contrat de localisation est viole
// sur les centres exacts de F11) ; `cell-kill-h-minus-one` (dead_cells /
// all_dead s'ecartent de l'autorite a h). Codes : 0, 1 desaccord, 2 refus, 3 plancher, 4.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/wide.hpp"
#include "../src/lanes/cell_grid.hpp"
#include "../src/lanes/q3.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp5;

namespace {
constexpr int G = CellGrid::G, NV = CellGrid::NV, NC = CellGrid::NC;

struct Tally {
  u64 grids = 0, not_built = 0, pairs = 0, cnt_mism = 0, meta_mism = 0, sites = 0;
  u64 du_neg = 0, du_zero = 0, du_pos = 0, dv_neg = 0, dv_zero = 0, dv_pos = 0, ties = 0;
  u64 prim_sites = 0, prim_mism = 0, synth = 0, synth_mism = 0, synth_ties = 0;
  u64 located = 0, contract_viol = 0, on_edge = 0, out_domain = 0, overflow = 0, chords = 0;
  int max_log2_mag = -1, max_log2_rhs = -1;
} T;

int log2_i128(i128 v) {
  if (v < 0) v = -v;
  int b = -1;
  while (v > 0) { v >>= 1; ++b; }
  return b;
}

// Evaluation DIRECTE : ok[i'][j'] = (4(i'−G)·du + 4(j'−G)·dv > rhs) puis cellule ssi quatre sommets.
void direct_site(i128 du, i128 dv, i128 rhs, bool nonstrict, bool cell[NC][NC], u64* ties) {
  bool ok[NV][NV];
  for (int ii = 0; ii < NV; ++ii)
    for (int jj = 0; jj < NV; ++jj) {
      const i128 lhs = (i128)4 * (ii - G) * du + (i128)4 * (jj - G) * dv;
      if (lhs == rhs) ++*ties;
      ok[ii][jj] = nonstrict ? (lhs >= rhs) : (lhs > rhs);
    }
  for (int cj = 0; cj < NC; ++cj)
    for (int ci = 0; ci < NC; ++ci) cell[cj][ci] = ok[ci][cj] && ok[ci + 1][cj] && ok[ci][cj + 1] && ok[ci + 1][cj + 1];
}

// Primitif de production sur UN site : count_site_t<Int, ns> puis accumulation -> comptes 0/1 par cellule.
template <typename Int, bool kNs>
u64 primitive_mismatch(Int du, Int dv, Int rhs, const bool cell[NC][NC]) {
  u32 dlo[NC][NC + 1], dhi[NC][NC + 1], out[NC][NC];
  std::memset(dlo, 0, sizeof(dlo));
  std::memset(dhi, 0, sizeof(dhi));
  CellGrid::count_site_t<Int, kNs>(du, dv, rhs, dlo, dhi);
  CellGrid::accumulate(dlo, dhi, out);
  u64 m = 0;
  for (int cj = 0; cj < NC; ++cj)
    for (int ci = 0; ci < NC; ++ci)
      if (out[cj][ci] != (cell[cj][ci] ? 1u : 0u)) ++m;
  return m;
}

// ENTIERS SIGNES 256 BITS (complement a deux, quatre mots) pour le localisateur
// rationnel : sous le profil u16, |pu|, |pv| < 2^118, Gram < 2^49, den < 2^70,
// det < 2^97 : numerateurs G(pu·vv − pv·uv) < 2^171, denominateur den·det <
// 2^167 — hors de l'i128, dans 256 bits. Produit 128×128 -> 192 de core/wide.hpp.
struct W256 {
  u64 w[4];
};
W256 w256_zero() { return W256{{0, 0, 0, 0}}; }
W256 w256_from_u192(const U192& p) { return W256{{p.w[0], p.w[1], p.w[2], 0}}; }
W256 w256_neg(const W256& a) {
  W256 r;
  u128 carry = 1;
  for (int i = 0; i < 4; ++i) { const u128 s = (u128)(~a.w[i]) + carry; r.w[i] = (u64)s; carry = s >> 64; }
  return r;
}
W256 w256_add(const W256& a, const W256& b) {
  W256 r;
  u128 carry = 0;
  for (int i = 0; i < 4; ++i) { const u128 s = (u128)a.w[i] + b.w[i] + carry; r.w[i] = (u64)s; carry = s >> 64; }
  return r;
}
W256 w256_sub(const W256& a, const W256& b) { return w256_add(a, w256_neg(b)); }
bool w256_is_neg(const W256& a) { return (a.w[3] >> 63) != 0; }
int w256_cmp(const W256& a, const W256& b) {  // signe
  const bool na = w256_is_neg(a), nb = w256_is_neg(b);
  if (na != nb) return na ? -1 : 1;
  for (int i = 3; i >= 0; --i)
    if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1;
  return 0;
}
W256 w256_mul_i128(i128 a, i128 b) {  // |a|, |b| < 2^127 ; |a·b| < 2^192
  const bool na = a < 0, nb = b < 0;
  const u128 ua = na ? (u128)(-a) : (u128)a, ub = nb ? (u128)(-b) : (u128)b;
  W256 r = w256_from_u192(mul_128x128_192(ua, ub));
  return (na != nb) ? w256_neg(r) : r;
}
W256 w256_shl3(const W256& a) {  // × 8 (= G)
  W256 r;
  u64 carry = 0;
  for (int i = 0; i < 4; ++i) { r.w[i] = (a.w[i] << 3) | carry; carry = a.w[i] >> 61; }
  return r;
}
W256 w256_mul_small(const W256& a, i64 c) {  // |c| <= 64 : par additions du signe-magnitude
  const bool neg = c < 0;
  const u64 m = (u64)(neg ? -c : c);
  W256 r = w256_zero();
  u128 carry = 0;
  const bool an = w256_is_neg(a);
  const W256 mag = an ? w256_neg(a) : a;
  for (int i = 0; i < 4; ++i) { const u128 p = (u128)mag.w[i] * m + carry; r.w[i] = (u64)p; carry = p >> 64; }
  return (an != neg) ? w256_neg(r) : r;
}

// Coordonnees exactes : numerateurs na = G(pu·vv − pv·uv), nb = G(pv·uu − pu·uv), denominateur d = den·det > 0.
void exact_coords(const CellGrid& g, i128 pu, i128 pv, i128 den, W256* na, W256* nb, W256* d) {
  *na = w256_shl3(w256_sub(w256_mul_i128(pu, g.vv_i), w256_mul_i128(pv, g.uv_i)));
  *nb = w256_shl3(w256_sub(w256_mul_i128(pv, g.uu_i), w256_mul_i128(pu, g.uv_i)));
  *d = w256_mul_i128(den, g.det_i);
}
// Contrat : [ceil(x) − 1, floor(x)] ⊆ [c0, c1]  <=>  c0 < x < c1 + 1  <=>  c0·d < n < (c1 + 1)·d (d > 0).
// Sur une arete ssi n = k·d pour un entier k (|k| <= 4G dans le domaine ; balayage k = −40..40).
bool contains_closed(const W256& n, const W256& d, int c0, int c1, bool* on_edge) {
  for (int k = -40; k <= 40; ++k)
    if (w256_cmp(n, w256_mul_small(d, k)) == 0) { *on_edge = true; break; }
  return w256_cmp(w256_mul_small(d, c0), n) < 0 && w256_cmp(n, w256_mul_small(d, (i64)c1 + 1)) < 0;
}

void check_point(const CellGrid& g, i128 pu, i128 pv, i128 den, int r[4], bool boxed) {
  W256 na, nb, d;
  exact_coords(g, pu, pv, den, &na, &nb, &d);
  if (w256_is_neg(d) || w256_cmp(d, w256_zero()) == 0) { ++T.overflow; return; }  // jamais : det > 0, den > 0
  ++T.located;
  if (!boxed) { ++T.out_domain; return; }
  bool edge = false;
  const bool ok = contains_closed(na, d, r[0], r[1], &edge) && contains_closed(nb, d, r[2], r[3], &edge);
  if (edge) ++T.on_edge;
  if (!ok) ++T.contract_viol;
}

void oracle_anchor(const CloudIndex& ix, i32 ua, i32 ub, bool float_on) {
  const P3& pa = ix.upos[(size_t)ua];
  const P3& pb = ix.upos[(size_t)ub];
  const i64 D2 = p3_norm2(p3_sub(pb, pa));
  if (D2 == 0) return;
  std::vector<CoverPoint> cover;
  cover_query(ix, pa, pb, D2, 3, &cover);
  const i64 d[3] = {pb.x - pa.x, pb.y - pa.y, pb.z - pa.z};
  const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
  for (int lane = 3; lane <= 4; ++lane) {
    const u64 h = lane_h(lane == 3 ? Lane::kQ3 : Lane::kQ4, 11);
    CellGrid g;
    ++T.grids;
    if (!g.build(cover, ix.upos, ua, ub, pa, pb, D2, lane == 3 ? 12 : 8, h, float_on)) { ++T.not_built; continue; }
    // (A) compteur direct.
    u32 direct[NC][NC];
    std::memset(direct, 0, sizeof(direct));
    for (const CoverPoint& cz : cover) {
      if (cz.u == ua || cz.u == ub) continue;
      const P3& z = ix.upos[(size_t)cz.u];
      const i64 w0 = 2 * z.x - sx, w1 = 2 * z.y - sy, w2 = 2 * z.z - sz;
      const i128 n2w = (i128)w0 * w0 + (i128)w1 * w1 + (i128)w2 * w2;
      const i128 rhs = (i128)G * (n2w - (i128)D2);
      const i128 du = (i128)w0 * g.u[0] + (i128)w1 * g.u[1] + (i128)w2 * g.u[2];
      const i128 dv = (i128)w0 * g.v[0] + (i128)w1 * g.v[1] + (i128)w2 * g.v[2];
      ++T.sites;
      if (du < 0) ++T.du_neg; else if (du == 0) ++T.du_zero; else ++T.du_pos;
      if (dv < 0) ++T.dv_neg; else if (dv == 0) ++T.dv_zero; else ++T.dv_pos;
      const i128 mag = (i128)4 * G * ((du < 0 ? -du : du) + (dv < 0 ? -dv : dv));
      T.max_log2_mag = std::max(T.max_log2_mag, log2_i128(mag));
      T.max_log2_rhs = std::max(T.max_log2_rhs, log2_i128(rhs));
      bool cs[NC][NC], cn[NC][NC];
      u64 dummy = 0;
      direct_site(du, dv, rhs, false, cs, &T.ties);
      direct_site(du, dv, rhs, true, cn, &dummy);
      for (int cj = 0; cj < NC; ++cj)
        for (int ci = 0; ci < NC; ++ci) direct[cj][ci] += cs[cj][ci] ? 1u : 0u;
      // Primitif site par site, quatre instanciations (le chemin i64 exige les bornes de build).
      ++T.prim_sites;
      if (mag < CellGrid::kFastLimit && (rhs < 0 ? -rhs : rhs) < CellGrid::kFastLimit) {
        T.prim_mism += primitive_mismatch<i64, false>((i64)du, (i64)dv, (i64)rhs, cs);
        T.prim_mism += primitive_mismatch<i64, true>((i64)du, (i64)dv, (i64)rhs, cn);
      }
      T.prim_mism += primitive_mismatch<i128, false>(du, dv, rhs, cs);
      T.prim_mism += primitive_mismatch<i128, true>(du, dv, rhs, cn);
    }
    u32 needed = 0, dead = 0;
    for (int j = -G; j < G; ++j)
      for (int i = -G; i < G; ++i) {
        ++T.pairs;
        if (g.cnt[j + G][i + G] != direct[j + G][i + G]) ++T.cnt_mism;
        if (!CellGrid::cell_needed(i, j)) continue;
        ++needed;
        if ((u64)direct[j + G][i + G] >= h) ++dead;
      }
    if (needed != g.needed_cells || dead != g.dead_cells || (dead == needed) != g.all_dead) ++T.meta_mism;
    // (B) localisateur rationnel : centres q3 / cordes q4 de chaque seed aigu.
    for (const CoverPoint& cx : cover) {
      if (cx.u == ua || cx.u == ub) continue;
      const P3& px = ix.upos[(size_t)cx.u];
      if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cx.u))) continue;
      const Q3Form f3 = q3_form(pa, pb, px);
      int r[4] = {0, 0, 0, 0};
      if (lane == 3) {
        i128 pu, pv, den;
        generate_detail::seed_center_coords(g, f3, d, &pu, &pv, &den);
        const bool boxed = g.locate_box(pu, pv, den, r);
        check_point(g, pu, pv, den, r, boxed);
      } else {
        const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
        i128 pu0, pv0, pu1, pv1, den;
        if (!generate_detail::seed_chord_coords(g, f3, d, nrm, D2, p3_norm2(p3_sub(px, pa)), p3_norm2(p3_sub(px, pb)), &pu0, &pv0, &pu1, &pv1, &den))
          continue;
        ++T.chords;
        const bool boxed = g.segment_box(pu0, pv0, pu1, pv1, den, r);
        check_point(g, pu0, pv0, den, r, boxed);
        check_point(g, pu1, pv1, den, r, boxed);
      }
    }
  }
}

void oracle_all_pairs(const CloudIndex& ix, bool float_on) {
  const i32 m = (i32)ix.upos.size();
  for (i32 ua = 0; ua < m; ++ua)
    for (i32 ub = ua + 1; ub < m; ++ub) oracle_anchor(ix, ua, ub, float_on);
}
// Une ancre designee par ses coordonnees (fixtures).
void oracle_one(const CloudIndex& ix, const P3& pa, const P3& pb, bool float_on) {
  i32 ua = -1, ub = -1;
  for (i32 u = 0; u < (i32)ix.upos.size(); ++u) {
    const P3& p = ix.upos[(size_t)u];
    if (p.x == pa.x && p.y == pa.y && p.z == pa.z) ua = u;
    if (p.x == pb.x && p.y == pb.y && p.z == pb.z) ub = u;
  }
  if (ua < 0 || ub < 0) { std::printf("fixture : ancre introuvable\n"); ++T.not_built; return; }
  oracle_anchor(ix, ua, ub, float_on);
}

std::vector<InputPoint> points(const std::vector<P3>& v) {
  std::vector<InputPoint> in;
  for (const P3& p : v) { InputPoint q; q.id = (PointId)in.size(); q.position = p; in.push_back(q); }
  return in;
}
// Fixtures F9, F10, F11 de tests/anchor_kill_fixture.cpp (coordonnees translatees de +1000 en y, z).
std::vector<P3> fixture_f9() {
  std::vector<P3> v{{800, 1000, 1000}, {2800, 1000, 1000}};
  for (i64 x = 0; x <= 3600; x += 40)
    for (i64 y = -600; y <= 600; y += 40) {
      const i64 ax = x >= 1800 ? x - 1800 : 1800 - x;
      const i64 h = ax <= 900 ? -600 : -600 + 6 * (ax - 900);
      if ((x == 800 || x == 2800) && y == 0) continue;
      v.push_back(P3{x, y + 1000, h + 1000});
    }
  return v;
}
std::vector<P3> fixture_f10() {
  std::vector<P3> v{{0, 1000, 1000}, {2000, 1000, 1000}};
  const i64 st[][2] = {{600, 800}, {-600, 800}, {800, 600}, {-800, 600}, {280, 960}, {-280, 960}, {960, 280},
                       {-960, 280}, {352, 936}, {-352, 936}, {936, 352}, {-936, 352}, {0, 1000}};
  for (const auto& q : st) v.push_back(P3{1000 + q[0], 1000, 1000 + q[1]});
  return v;
}
std::vector<P3> fixture_f11() {
  std::vector<P3> v{{0, 1000, 1000}, {2000, 1000, 1000}};
  for (i64 e = 0; e < 9; ++e) v.push_back(P3{1000 + e, 1000, 2200});
  v.push_back(P3{750, 1000, 2250});
  v.push_back(P3{1020, 184, 2088});
  v.push_back(P3{1000, 1000, 2400});
  v.push_back(P3{1000, 1300, 2400});
  return v;
}
// Ancre a coordonnees u16 EXTREMES : a = (0,0,0), b = (65535,65535,65535), 100 sites pseudo-aleatoires (LCG grave).
std::vector<P3> fixture_extreme() {
  std::vector<P3> v{{0, 0, 0}, {65535, 65535, 65535}};
  u64 s = 0x9E3779B97F4A7C15ull;
  for (int i = 0; i < 100; ++i) {
    i64 c[3];
    for (int k = 0; k < 3; ++k) { s = s * 6364136223846793005ull + 1442695040888963407ull; c[k] = (i64)((s >> 33) % 65536); }
    v.push_back(P3{c[0], c[1], c[2]});
  }
  return v;
}

// Sites SYNTHETIQUES pour le chemin i128 du primitif : magnitudes ~2^100 (au-dela de l'i64) et egalites exactes.
void synthetic_i128() {
  u64 s = 0x2545F4914F6CDD1Dull;
  const auto rnd = [&]() { s = s * 6364136223846793005ull + 1442695040888963407ull; return s; };
  // Bits HAUTS du LCG seulement (les bits bas d'un LCG modulo 2^64 ont une periode courte).
  const auto big = [&]() { const i128 v = ((i128)(rnd() >> 4)) << 40 | (i128)(rnd() >> 24); return (rnd() >> 63) ? -v : v; };  // ~2^100
  for (int it = 0; it < 4000; ++it) {
    i128 du = big(), dv = big(), rhs = big();
    const int mode = (int)(rnd() >> 62);
    if (mode == 1) du = 0;
    if (mode == 2) dv = 0;
    if (mode == 3) {  // egalite exacte a un sommet tire au sort
      const int ii = (int)((rnd() >> 40) % NV), jj = (int)((rnd() >> 40) % NV);
      rhs = (i128)4 * (ii - G) * du + (i128)4 * (jj - G) * dv;
    }
    bool cs[NC][NC], cn[NC][NC];
    u64 dummy = 0;
    direct_site(du, dv, rhs, false, cs, &T.synth_ties);
    direct_site(du, dv, rhs, true, cn, &dummy);
    ++T.synth;
    T.synth_mism += primitive_mismatch<i128, false>(du, dv, rhs, cs);
    T.synth_mism += primitive_mismatch<i128, true>(du, dv, rhs, cn);
  }
}
}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool m_ns = MHGP5_MUTANT("cell-kill-nonstrict");
  const bool m_eps0 = MHGP5_MUTANT("cell-locate-eps-zero");
  const bool m_h1 = MHGP5_MUTANT("cell-kill-h-minus-one");
  const bool float_on = float_filter_runtime_enabled();
  if (!float_on) { std::printf("REFUS : environnement flottant hors profil (arrondi != au plus proche ou -ffast-math)\n"); return 2; }
  // Garde d'environnement : fail-open (aucune grille) quand le localisateur n'est pas certifiable.
  {
    const CloudIndex ix = build_cloud_index(points(fixture_f10()));
    std::vector<CoverPoint> cover;
    const P3 pa{0, 1000, 1000}, pb{2000, 1000, 1000};
    cover_query(ix, pa, pb, 4000000, 3, &cover);
    CellGrid g;
    const bool b = g.build(cover, ix.upos, 0, 1, pa, pb, 4000000, 12, 9, /*locate_env_ok=*/false);
    if (b || g.built || g.fail != CellGrid::Fail::kEnvironment) { std::printf("garde d'environnement : la grille devrait echouer ouverte\n"); return 1; }
  }
  const struct { CloudFamily f; int n; int coord; } clouds[] = {
      {CloudFamily::kUniform, 50, 0}, {CloudFamily::kEightClusters, 60, 0}, {CloudFamily::kTerrain, 50, 0}};
  for (const auto& c : clouds) {
    const int coord = c.coord > 0 ? c.coord : cloud_family_default_coord(c.f, c.n);
    const CloudIndex ix = build_cloud_index(make_family_input(c.f, c.n, coord, 3));
    if (!ix.valid || ix.has_duplicate_positions()) { std::printf("REFUS : nuage invalide\n"); return 2; }
    oracle_all_pairs(ix, float_on);
  }
  {
    const CloudIndex ix = build_cloud_index(points(fixture_f9()));
    if (!ix.valid) return 2;
    oracle_one(ix, P3{800, 1000, 1000}, P3{2800, 1000, 1000}, float_on);
  }
  {
    const CloudIndex ix = build_cloud_index(points(fixture_f10()));
    if (!ix.valid) return 2;
    oracle_one(ix, P3{0, 1000, 1000}, P3{2000, 1000, 1000}, float_on);
  }
  const u64 on_edge_before_f11 = T.on_edge;
  {
    const CloudIndex ix = build_cloud_index(points(fixture_f11()));
    if (!ix.valid) return 2;
    oracle_one(ix, P3{0, 1000, 1000}, P3{2000, 1000, 1000}, float_on);
  }
  const u64 on_edge_f11 = T.on_edge - on_edge_before_f11;
  {
    const CloudIndex ix = build_cloud_index(points(fixture_extreme()));
    if (!ix.valid || ix.has_duplicate_positions()) return 2;
    oracle_all_pairs(ix, float_on);  // toutes les paires (dont a-b extreme) : magnitudes maximales du profil
  }
  synthetic_i128();
  std::printf("cell_grid_oracle grilles=%llu non_construites=%llu sites=%llu paires=%llu desaccords_cnt=%llu desaccords_meta=%llu "
              "du(-/0/+)=%llu/%llu/%llu dv(-/0/+)=%llu/%llu/%llu egalites=%llu log2max(mag)=%d log2max(rhs)=%d "
              "primitif_sites=%llu desaccords_primitif=%llu synthetiques_i128=%llu (egalites %llu) desaccords_synth=%llu "
              "localises=%llu cordes=%llu sur_arete=%llu (F11 : %llu) hors_domaine=%llu denominateurs_invalides=%llu violations_contrat=%llu (mutants ns=%d eps0=%d)\n",
              (unsigned long long)T.grids, (unsigned long long)T.not_built, (unsigned long long)T.sites, (unsigned long long)T.pairs,
              (unsigned long long)T.cnt_mism, (unsigned long long)T.meta_mism, (unsigned long long)T.du_neg, (unsigned long long)T.du_zero,
              (unsigned long long)T.du_pos, (unsigned long long)T.dv_neg, (unsigned long long)T.dv_zero, (unsigned long long)T.dv_pos,
              (unsigned long long)T.ties, T.max_log2_mag, T.max_log2_rhs, (unsigned long long)T.prim_sites, (unsigned long long)T.prim_mism,
              (unsigned long long)T.synth, (unsigned long long)T.synth_ties, (unsigned long long)T.synth_mism, (unsigned long long)T.located,
              (unsigned long long)T.chords, (unsigned long long)T.on_edge, (unsigned long long)on_edge_f11, (unsigned long long)T.out_domain,
              (unsigned long long)T.overflow, (unsigned long long)T.contract_viol, m_ns ? 1 : 0, m_eps0 ? 1 : 0);
  // Planchers contre le vert-par-vacuite.
  if (T.overflow != 0 || T.pairs < 100000 || T.du_neg == 0 || T.du_zero == 0 || T.du_pos == 0 || T.dv_neg == 0 || T.dv_zero == 0 || T.dv_pos == 0 ||
      T.ties == 0 || T.prim_sites < 1000 || T.synth < 1000 || T.synth_ties == 0 || T.located < 1000 || T.chords < 100 || on_edge_f11 < 2 ||
      T.not_built != 0) {
    std::printf("PLANCHER\n");
    return 3;
  }
  // Le chemin i128 de build est inatteignable sous le profil u16 : grave (une valeur >= 62 contredirait la note de cell_grid.hpp).
  if (T.max_log2_mag >= 62 || T.max_log2_rhs >= 62) { std::printf("INVARIANT : le chemin i128 de build est atteint sous le profil u16\n"); return 3; }
  if (m_ns) {
    // Le compteur non strict compte les egalites de F10 : desaccord avec l'autorite stricte -> tue.
    if (T.cnt_mism && !T.prim_mism && !T.synth_mism && !T.contract_viol) return 4;
    std::printf("MUTANT NON TUE (cell-kill-nonstrict)\n");
    return 1;
  }
  if (m_eps0) {
    // Marge nulle : un centre exactement sur une arete (F11) ne consulte plus ses deux cellules fermees -> tue.
    if (T.contract_viol && !T.cnt_mism && !T.prim_mism && !T.synth_mism) return 4;
    std::printf("MUTANT NON TUE (cell-locate-eps-zero)\n");
    return 1;
  }
  if (m_h1) {
    // Cellule morte a h − 1 : les comptes sont exacts mais `dead_cells` / `all_dead` s'ecartent de l'autorite a h -> tue.
    if (T.meta_mism && !T.cnt_mism && !T.prim_mism && !T.synth_mism && !T.contract_viol) return 4;
    std::printf("MUTANT NON TUE (cell-kill-h-minus-one)\n");
    return 1;
  }
  if (T.cnt_mism || T.meta_mism || T.prim_mism || T.synth_mism || T.contract_viol) return 1;
  std::printf("cell_grid_oracle OK\n");
  return 0;
}
