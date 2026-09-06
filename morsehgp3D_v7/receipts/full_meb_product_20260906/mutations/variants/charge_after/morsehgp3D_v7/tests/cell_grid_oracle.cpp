// MorseHGP3D v6 — ORACLE BRUT de la grille de cellules (src/lanes/cell_grid.hpp,
// theoreme 10.5), PORTE de la v5 (tests/cell_grid_oracle.cpp, reponse V15) et
// GENERALISE au parametre G : les DEUX resolutions de production —
// CellGridT<8> (E3 historique) et CellGridT<16> (experimentation E3/G16 sur
// les ancres lourdes) — passent le MEME oracle (exigence du quatrieme tour
// d'audit : « porter un oracle v6 independant et le parametrer explicitement
// sur G=8 puis G=16 »). Deux autorites INDEPENDANTES, sans deux pointeurs ni
// flottant :
//   (A) COMPTEUR : pour chaque ancre (toutes les paires de trois petits
//       nuages, fixtures F9 (vallee), F10 (13 sites sur la frontiere des
//       sommets i' = 0), F11 (frontieres de la localisation), ancre u16
//       EXTREME) et chaque lane (rho² = D²/12 en q3, D²/8 en q4),
//       CellGridT<G>::build est compare CELLULE PAR CELLULE a l'evaluation
//       DIRECTE i128 des (2G+1)² sommets ; memes comparaisons pour
//       needed/dead/all_dead ; primitif count_site_t exerce site par site en
//       quatre instanciations, plus des sites SYNTHETIQUES ~2^100 (chemin
//       i128, egalites exactes construites).
//   (B) LOCALISATEUR RATIONNEL : coordonnees exactes en entiers signes 256
//       bits (produits 128x128 -> 192 de core/wide.hpp) ; contrat : la boite
//       consultee contient TOUTES les cellules fermees contenant le point
//       exact (une arete exacte consulte ses deux cellules).
// Planchers (code 3) PAR RESOLUTION : paires (site, cellule), signes de
// du/dv, egalites, primitifs, synthetiques, points localises, cordes, aretes
// F11, aucune grille non construite. Mutants tues (code 4) :
// cell-kill-nonstrict, cell-locate-eps-zero, cell-kill-h-minus-one.
// Codes : 0, 1 desaccord, 2 refus, 3 plancher, 4 mutant tue.
#include <algorithm>
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

using namespace mhgp7;

namespace {

struct Tally {
  u64 grids = 0, not_built = 0, pairs = 0, cnt_mism = 0, meta_mism = 0, sites = 0;
  u64 du_neg = 0, du_zero = 0, du_pos = 0, dv_neg = 0, dv_zero = 0, dv_pos = 0, ties = 0;
  u64 prim_sites = 0, prim_mism = 0, synth = 0, synth_mism = 0, synth_ties = 0;
  u64 located = 0, contract_viol = 0, on_edge = 0, out_domain = 0, overflow = 0, chords = 0;
  int max_log2_mag = -1, max_log2_rhs = -1;
};

int log2_i128(i128 v) {
  if (v < 0) v = -v;
  int b = -1;
  while (v > 0) {
    v >>= 1;
    ++b;
  }
  return b;
}

// Evaluation DIRECTE : ok[i'][j'] = (4(i'−G)·du + 4(j'−G)·dv > rhs) puis
// cellule ssi quatre sommets.
template <typename Grid>
void direct_site(i128 du, i128 dv, i128 rhs, bool nonstrict, bool cell[Grid::NC][Grid::NC], u64* ties) {
  constexpr int G = Grid::G, NV = Grid::NV, NC = Grid::NC;
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

// Primitif de production sur UN site.
template <typename Grid, typename Int, bool kNs>
u64 primitive_mismatch(Int du, Int dv, Int rhs, const bool cell[Grid::NC][Grid::NC]) {
  constexpr int NC = Grid::NC;
  u32 dlo[NC][NC + 1], dhi[NC][NC + 1], out[NC][NC];
  std::memset(dlo, 0, sizeof(dlo));
  std::memset(dhi, 0, sizeof(dhi));
  Grid::template count_site_t<Int, kNs>(du, dv, rhs, dlo, dhi);
  Grid::accumulate(dlo, dhi, out);
  u64 m = 0;
  for (int cj = 0; cj < NC; ++cj)
    for (int ci = 0; ci < NC; ++ci)
      if (out[cj][ci] != (cell[cj][ci] ? 1u : 0u)) ++m;
  return m;
}

// ENTIERS SIGNES 256 BITS (complement a deux) pour le localisateur rationnel.
struct W256 {
  u64 w[4];
};
W256 w256_zero() { return W256{{0, 0, 0, 0}}; }
W256 w256_from_u192(const U192& p) { return W256{{p.w[0], p.w[1], p.w[2], 0}}; }
W256 w256_neg(const W256& a) {
  W256 r;
  u128 carry = 1;
  for (int i = 0; i < 4; ++i) {
    const u128 s = (u128)(~a.w[i]) + carry;
    r.w[i] = (u64)s;
    carry = s >> 64;
  }
  return r;
}
W256 w256_add(const W256& a, const W256& b) {
  W256 r;
  u128 carry = 0;
  for (int i = 0; i < 4; ++i) {
    const u128 s = (u128)a.w[i] + b.w[i] + carry;
    r.w[i] = (u64)s;
    carry = s >> 64;
  }
  return r;
}
W256 w256_sub(const W256& a, const W256& b) { return w256_add(a, w256_neg(b)); }
bool w256_is_neg(const W256& a) { return (a.w[3] >> 63) != 0; }
int w256_cmp(const W256& a, const W256& b) {
  const bool na = w256_is_neg(a), nb = w256_is_neg(b);
  if (na != nb) return na ? -1 : 1;
  for (int i = 3; i >= 0; --i)
    if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1;
  return 0;
}
W256 w256_mul_i128(i128 a, i128 b) {
  const bool na = a < 0, nb = b < 0;
  const u128 ua = na ? (u128)(-a) : (u128)a, ub = nb ? (u128)(-b) : (u128)b;
  W256 r = w256_from_u192(mul_128x128_192(ua, ub));
  return (na != nb) ? w256_neg(r) : r;
}
// × G, G puissance de deux (8 ou 16) : decalage generique.
W256 w256_shl_pow2(const W256& a, int shift) {
  W256 r = a;
  for (int s = 0; s < shift; ++s) {
    u64 carry = 0;
    for (int i = 0; i < 4; ++i) {
      const u64 next = r.w[i] >> 63;
      r.w[i] = (r.w[i] << 1) | carry;
      carry = next;
    }
  }
  return r;
}
W256 w256_mul_small(const W256& a, i64 c) {  // |c| <= 4G + 1
  const bool neg = c < 0;
  const u64 m = (u64)(neg ? -c : c);
  W256 r = w256_zero();
  u128 carry = 0;
  const bool an = w256_is_neg(a);
  const W256 mag = an ? w256_neg(a) : a;
  for (int i = 0; i < 4; ++i) {
    const u128 p = (u128)mag.w[i] * m + carry;
    r.w[i] = (u64)p;
    carry = p >> 64;
  }
  return (an != neg) ? w256_neg(r) : r;
}

template <typename Grid>
void exact_coords(const Grid& g, i128 pu, i128 pv, i128 den, W256* na, W256* nb, W256* d) {
  const int shift = Grid::G == 8 ? 3 : 4;
  *na = w256_shl_pow2(w256_sub(w256_mul_i128(pu, g.vv_i), w256_mul_i128(pv, g.uv_i)), shift);
  *nb = w256_shl_pow2(w256_sub(w256_mul_i128(pv, g.uu_i), w256_mul_i128(pu, g.uv_i)), shift);
  *d = w256_mul_i128(den, g.det_i);
}
bool contains_closed(const W256& n, const W256& d, int c0, int c1, int G, bool* on_edge) {
  for (int k = -4 * G; k <= 4 * G; ++k)
    if (w256_cmp(n, w256_mul_small(d, k)) == 0) {
      *on_edge = true;
      break;
    }
  return w256_cmp(w256_mul_small(d, c0), n) < 0 && w256_cmp(n, w256_mul_small(d, (i64)c1 + 1)) < 0;
}

template <typename Grid>
void check_point(Tally& T, const Grid& g, i128 pu, i128 pv, i128 den, int r[4], bool boxed) {
  W256 na, nb, d;
  exact_coords(g, pu, pv, den, &na, &nb, &d);
  if (w256_is_neg(d) || w256_cmp(d, w256_zero()) == 0) {
    ++T.overflow;
    return;
  }
  ++T.located;
  if (!boxed) {
    ++T.out_domain;
    return;
  }
  bool edge = false;
  const bool ok = contains_closed(na, d, r[0], r[1], Grid::G, &edge) && contains_closed(nb, d, r[2], r[3], Grid::G, &edge);
  if (edge) ++T.on_edge;
  if (!ok) ++T.contract_viol;
}

template <typename Grid>
void oracle_anchor(Tally& T, const CloudIndex& ix, i32 ua, i32 ub, bool float_on) {
  constexpr int G = Grid::G, NC = Grid::NC, NV = Grid::NV;
  const P3& pa = ix.upos[(size_t)ua];
  const P3& pb = ix.upos[(size_t)ub];
  const i64 D2 = p3_norm2(p3_sub(pb, pa));
  if (D2 == 0) return;
  std::vector<CoverPoint> cover;
  cover_query(ix, pa, pb, D2, 4, &cover);
  const i64 d[3] = {pb.x - pa.x, pb.y - pa.y, pb.z - pa.z};
  const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
  for (int lane = 3; lane <= 4; ++lane) {
    const u64 h = lane_h(lane == 3 ? Lane::kQ3 : Lane::kQ4, 11);
    Grid g;
    ++T.grids;
    if (!g.build(cover, ix.upos, ua, ub, pa, pb, D2, lane == 3 ? 12 : 8, h, float_on)) {
      ++T.not_built;
      continue;
    }
    // (A) compteur direct.
    std::vector<u32> direct((size_t)NC * NC, 0);
    for (const CoverPoint& cz : cover) {
      if (cz.u == ua || cz.u == ub) continue;
      const P3& z = ix.upos[(size_t)cz.u];
      const i64 w0 = 2 * z.x - sx, w1 = 2 * z.y - sy, w2 = 2 * z.z - sz;
      const i128 n2w = (i128)w0 * w0 + (i128)w1 * w1 + (i128)w2 * w2;
      const i128 rhs = (i128)G * (n2w - (i128)D2);
      const i128 du = (i128)w0 * g.u[0] + (i128)w1 * g.u[1] + (i128)w2 * g.u[2];
      const i128 dv = (i128)w0 * g.v[0] + (i128)w1 * g.v[1] + (i128)w2 * g.v[2];
      ++T.sites;
      if (du < 0) ++T.du_neg;
      else if (du == 0) ++T.du_zero;
      else ++T.du_pos;
      if (dv < 0) ++T.dv_neg;
      else if (dv == 0) ++T.dv_zero;
      else ++T.dv_pos;
      const i128 mag = (i128)4 * G * ((du < 0 ? -du : du) + (dv < 0 ? -dv : dv));
      T.max_log2_mag = std::max(T.max_log2_mag, log2_i128(mag));
      T.max_log2_rhs = std::max(T.max_log2_rhs, log2_i128(rhs));
      bool cs[NC][NC], cn[NC][NC];
      u64 dummy = 0;
      direct_site<Grid>(du, dv, rhs, false, cs, &T.ties);
      direct_site<Grid>(du, dv, rhs, true, cn, &dummy);
      for (int cj = 0; cj < NC; ++cj)
        for (int ci = 0; ci < NC; ++ci) direct[(size_t)cj * NC + ci] += cs[cj][ci] ? 1u : 0u;
      ++T.prim_sites;
      if (mag < Grid::kFastLimit && (rhs < 0 ? -rhs : rhs) < Grid::kFastLimit) {
        T.prim_mism += primitive_mismatch<Grid, i64, false>((i64)du, (i64)dv, (i64)rhs, cs);
        T.prim_mism += primitive_mismatch<Grid, i64, true>((i64)du, (i64)dv, (i64)rhs, cn);
      }
      T.prim_mism += primitive_mismatch<Grid, i128, false>(du, dv, rhs, cs);
      T.prim_mism += primitive_mismatch<Grid, i128, true>(du, dv, rhs, cn);
    }
    u32 needed = 0, dead = 0;
    for (int j = -G; j < G; ++j)
      for (int i = -G; i < G; ++i) {
        ++T.pairs;
        if (g.cnt[j + G][i + G] != direct[(size_t)(j + G) * NC + (i + G)]) ++T.cnt_mism;
        if (!Grid::cell_needed(i, j)) continue;
        ++needed;
        if ((u64)direct[(size_t)(j + G) * NC + (i + G)] >= h) ++dead;
      }
    if (needed != g.needed_cells || dead != g.dead_cells || (dead == needed) != g.all_dead) ++T.meta_mism;
    // (B) localisateur rationnel.
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
        check_point(T, g, pu, pv, den, r, boxed);
      } else {
        const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
        i128 pu0, pv0, pu1, pv1, den;
        if (!generate_detail::seed_chord_coords(g, f3, d, nrm, D2, p3_norm2(p3_sub(px, pa)), p3_norm2(p3_sub(px, pb)), &pu0, &pv0,
                               &pu1, &pv1, &den))
          continue;
        ++T.chords;
        const bool boxed = g.segment_box(pu0, pv0, pu1, pv1, den, r);
        check_point(T, g, pu0, pv0, den, r, boxed);
        check_point(T, g, pu1, pv1, den, r, boxed);
      }
    }
  }
  (void)NV;
}

template <typename Grid>
void oracle_all_pairs(Tally& T, const CloudIndex& ix, bool float_on) {
  const i32 m = (i32)ix.upos.size();
  for (i32 ua = 0; ua < m; ++ua)
    for (i32 ub = ua + 1; ub < m; ++ub) oracle_anchor<Grid>(T, ix, ua, ub, float_on);
}
template <typename Grid>
void oracle_one(Tally& T, const CloudIndex& ix, const P3& pa, const P3& pb, bool float_on) {
  i32 ua = -1, ub = -1;
  for (i32 u = 0; u < (i32)ix.upos.size(); ++u) {
    const P3& p = ix.upos[(size_t)u];
    if (p.x == pa.x && p.y == pa.y && p.z == pa.z) ua = u;
    if (p.x == pb.x && p.y == pb.y && p.z == pb.z) ub = u;
  }
  if (ua < 0 || ub < 0) {
    std::printf("fixture : ancre introuvable\n");
    ++T.not_built;
    return;
  }
  oracle_anchor<Grid>(T, ix, ua, ub, float_on);
}

std::vector<InputPoint> points(const std::vector<P3>& v) {
  std::vector<InputPoint> in;
  for (const P3& p : v) {
    InputPoint q;
    q.id = (PointId)in.size();
    q.position = p;
    in.push_back(q);
  }
  return in;
}
// Fixtures F9, F10, F11 (portees de la v5 a l'identique).
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
std::vector<P3> fixture_extreme() {
  std::vector<P3> v{{0, 0, 0}, {65535, 65535, 65535}};
  u64 s = 0x9E3779B97F4A7C15ull;
  for (int i = 0; i < 100; ++i) {
    i64 c[3];
    for (int k = 0; k < 3; ++k) {
      s = s * 6364136223846793005ull + 1442695040888963407ull;
      c[k] = (i64)((s >> 33) % 65536);
    }
    v.push_back(P3{c[0], c[1], c[2]});
  }
  return v;
}

template <typename Grid>
void synthetic_i128(Tally& T) {
  constexpr int G = Grid::G, NV = Grid::NV, NC = Grid::NC;
  u64 s = 0x2545F4914F6CDD1Dull;
  const auto rnd = [&]() {
    s = s * 6364136223846793005ull + 1442695040888963407ull;
    return s;
  };
  const auto big = [&]() {
    const i128 v = ((i128)(rnd() >> 4)) << 40 | (i128)(rnd() >> 24);
    return (rnd() >> 63) ? -v : v;
  };
  for (int it = 0; it < 4000; ++it) {
    i128 du = big(), dv = big(), rhs = big();
    const int mode = (int)(rnd() >> 62);
    if (mode == 1) du = 0;
    if (mode == 2) dv = 0;
    if (mode == 3) {
      const int ii = (int)((rnd() >> 40) % NV), jj = (int)((rnd() >> 40) % NV);
      rhs = (i128)4 * (ii - G) * du + (i128)4 * (jj - G) * dv;
    }
    bool cs[NC][NC], cn[NC][NC];
    u64 dummy = 0;
    direct_site<Grid>(du, dv, rhs, false, cs, &T.synth_ties);
    direct_site<Grid>(du, dv, rhs, true, cn, &dummy);
    ++T.synth;
    T.synth_mism += primitive_mismatch<Grid, i128, false>(du, dv, rhs, cs);
    T.synth_mism += primitive_mismatch<Grid, i128, true>(du, dv, rhs, cn);
  }
}

// Une resolution complete de l'oracle ; rend le tally.
template <typename Grid>
Tally run_resolution(bool float_on, u64* on_edge_f11_out) {
  Tally T;
  const struct {
    CloudFamily f;
    int n;
  } clouds[] = {{CloudFamily::kUniform, 50}, {CloudFamily::kEightClusters, 60}, {CloudFamily::kTerrain, 50}};
  for (const auto& c : clouds) {
    const CloudIndex ix = build_cloud_index(make_family_input(c.f, c.n, cloud_family_default_coord(c.f, c.n), 3));
    oracle_all_pairs<Grid>(T, ix, float_on);
  }
  {
    const CloudIndex ix = build_cloud_index(points(fixture_f9()));
    oracle_one<Grid>(T, ix, P3{800, 1000, 1000}, P3{2800, 1000, 1000}, float_on);
  }
  {
    const CloudIndex ix = build_cloud_index(points(fixture_f10()));
    oracle_one<Grid>(T, ix, P3{0, 1000, 1000}, P3{2000, 1000, 1000}, float_on);
  }
  const u64 before = T.on_edge;
  {
    const CloudIndex ix = build_cloud_index(points(fixture_f11()));
    oracle_one<Grid>(T, ix, P3{0, 1000, 1000}, P3{2000, 1000, 1000}, float_on);
  }
  *on_edge_f11_out = T.on_edge - before;
  {
    const CloudIndex ix = build_cloud_index(points(fixture_extreme()));
    oracle_all_pairs<Grid>(T, ix, float_on);
  }
  synthetic_i128<Grid>(T);
  return T;
}

int floors_bad(const Tally& T, u64 on_edge_f11, int G) {
  return (T.overflow != 0 || T.pairs < 100000 || T.du_neg == 0 || T.du_zero == 0 || T.du_pos == 0 ||
          T.dv_neg == 0 || T.dv_zero == 0 || T.dv_pos == 0 || T.ties == 0 || T.prim_sites < 1000 ||
          T.synth < 1000 || T.synth_ties == 0 || T.located < 1000 || T.chords < 100 || on_edge_f11 < 2 ||
          T.not_built != 0 || T.max_log2_mag >= 62 || T.max_log2_rhs >= 62)
             ? G
             : 0;
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
  const bool m_ns = MHGP7_MUTANT("cell-kill-nonstrict");
  const bool m_eps0 = MHGP7_MUTANT("cell-locate-eps-zero");
  const bool m_h1 = MHGP7_MUTANT("cell-kill-h-minus-one");
  const bool float_on = float_filter_runtime_enabled();
  if (!float_on) {
    std::printf("REFUS : environnement flottant hors profil\n");
    return 2;
  }
  // Garde d'environnement : fail-open aux deux resolutions.
  {
    const CloudIndex ix = build_cloud_index(points(fixture_f10()));
    std::vector<CoverPoint> cover;
    const P3 pa{0, 1000, 1000}, pb{2000, 1000, 1000};
    cover_query(ix, pa, pb, 4000000, 4, &cover);
    CellGrid g8;
    CellGridFine g16;
    const bool b8 = g8.build(cover, ix.upos, 0, 1, pa, pb, 4000000, 12, 9, false);
    const bool b16 = g16.build(cover, ix.upos, 0, 1, pa, pb, 4000000, 12, 9, false);
    if (b8 || g8.built || b16 || g16.built) {
      std::printf("garde d'environnement : la grille devrait echouer ouverte (G=8 et G=16)\n");
      return 1;
    }
  }
  u64 edge8 = 0, edge16 = 0;
  const Tally T8 = run_resolution<CellGrid>(float_on, &edge8);
  const Tally T16 = run_resolution<CellGridFine>(float_on, &edge16);
  for (const auto* p : {&T8, &T16}) {
    const Tally& T = *p;
    const int G = (p == &T8) ? 8 : 16;
    std::printf(
        "cell_grid_oracle G=%d grilles=%llu sites=%llu paires=%llu desaccords_cnt=%llu desaccords_meta=%llu "
        "egalites=%llu log2max=%d/%d primitifs=%llu desaccords_prim=%llu synth=%llu desaccords_synth=%llu "
        "localises=%llu cordes=%llu sur_arete=%llu hors_domaine=%llu violations=%llu\n",
        G, (unsigned long long)T.grids, (unsigned long long)T.sites, (unsigned long long)T.pairs,
        (unsigned long long)T.cnt_mism, (unsigned long long)T.meta_mism, (unsigned long long)T.ties,
        T.max_log2_mag, T.max_log2_rhs, (unsigned long long)T.prim_sites, (unsigned long long)T.prim_mism,
        (unsigned long long)T.synth, (unsigned long long)T.synth_mism, (unsigned long long)T.located,
        (unsigned long long)T.chords, (unsigned long long)T.on_edge, (unsigned long long)T.out_domain,
        (unsigned long long)T.contract_viol);
  }
  if (floors_bad(T8, edge8, 8) || floors_bad(T16, edge16, 16)) {
    std::printf("PLANCHER (G=%d)\n", floors_bad(T8, edge8, 8) ? 8 : 16);
    return 3;
  }
  const u64 cnt_mism = T8.cnt_mism + T16.cnt_mism;
  const u64 meta_mism = T8.meta_mism + T16.meta_mism;
  const u64 prim_mism = T8.prim_mism + T16.prim_mism;
  const u64 synth_mism = T8.synth_mism + T16.synth_mism;
  const u64 contract_viol = T8.contract_viol + T16.contract_viol;
  if (m_ns) {
    if (cnt_mism && !prim_mism && !synth_mism && !contract_viol) return 4;
    std::printf("MUTANT NON TUE (cell-kill-nonstrict)\n");
    return 1;
  }
  if (m_eps0) {
    if (contract_viol && !cnt_mism && !prim_mism && !synth_mism) return 4;
    std::printf("MUTANT NON TUE (cell-locate-eps-zero)\n");
    return 1;
  }
  if (m_h1) {
    if (meta_mism && !cnt_mism && !prim_mism && !synth_mism && !contract_viol) return 4;
    std::printf("MUTANT NON TUE (cell-kill-h-minus-one)\n");
    return 1;
  }
  if (cnt_mism || meta_mism || prim_mism || synth_mism || contract_viol) return 1;
  std::printf("cell_grid_oracle OK (G=8 et G=16)\n");
  return 0;
}

