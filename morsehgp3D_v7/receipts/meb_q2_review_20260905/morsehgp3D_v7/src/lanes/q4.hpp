// MorseHGP3D v6 — lane q4 : circumboule d'un tetraedre strictement bien centre.
//
// Circumcentre par Cramer 3×3 RELATIF : M = (2(b-a), 2(x-a), 2(y-a)),
// r = (|b-a|², |x-a|², |y-a|²), M(c-a) = r, c-a = N'/det ; canonisation
// d'orientation det > 0 ; det = 0 (coplanaires) : jamais un support q4.
// Puissance AFFINE : P4(z) = det|z-a|² - 2N'·(z-a) = det(|z-c|² - R²) ; sous
// det > 0 : < 0 interieur strict, = 0 coquille. Largeurs u16 : det < 2^57,
// |N'_i| < 2^72, P4 < 2^94 — i128.
// Arite 4 STRICTE : le centre est strictement interieur ssi, pour chaque face,
// le centre et le sommet oppose sont strictement du meme cote ; un zero =>
// centre sur une face => arite <= 3 (autre lane). Le volume V = det3(u,p,q)
// se calcule UNE fois : les orientations des sommets sont (-V,+V,-V,+V).
// Owner : ab maximale parmi les six longueurs carrees, ex aequo par EdgeKey.
// Exact-once du seed (lemme du prefixe ternaire, PROUVE par l'auditeur v4) :
// tout q4 bien centre d'arete maximale ab a AU MOINS une face abv strictement
// aigue ; on n'emet que si le carrier du seed est le plus petit PointId parmi
// les faces incidentes aigues du tetraedre forme.
// Niveau : R² = |N'|²/det², num < 2^146 (U192), den < 2^114 (i128), non
// reduits ; ordre par produits croises U320.
// BallForm : (det, -2(det a + N'), det|a|² + 2N'·a) ; |B_i| < 2^74, |C| < 2^90.
//
// PREFILTRES du bien-centrage (necessaires, exacts, en seules longueurs) :
//   etage i64 : 2 max(l_ay, l_by, l_xy) > D² et max(l_ax+l_ay, l_bx+l_by) > D² ;
//   puissance equatoriale de la face abx sur y : Pow_abx(y) > 0 ssi le centre
//   et y sont du meme cote — c'est le signe de q3_power(forme abx, y), deja
//   construite une fois par seed (contre-audit v4 7420355).
#pragma once

#include <array>

#include "../core/mutants.hpp"
#include "q3.hpp"

namespace mhgp7 {

struct Q4Form {
  i128 det = 0;            // > 0 apres canonisation ; 0 = coplanaire (refus)
  i128 np[3] = {0, 0, 0};  // N' : c - a = N'/det
  P3 a;
};

namespace q4_detail {
MHGP7_HD inline i128 det3_i128(const i64 r0[3], const i64 r1[3], const i128 r2[3]) {
  return r2[0] * ((i128)r0[1] * r1[2] - (i128)r0[2] * r1[1]) -
         r2[1] * ((i128)r0[0] * r1[2] - (i128)r0[2] * r1[0]) +
         r2[2] * ((i128)r0[0] * r1[1] - (i128)r0[1] * r1[0]);
}
}  // namespace q4_detail

MHGP7_HD inline Q4Form q4_form(const P3& a, const P3& b, const P3& x, const P3& y) {
  const P3 e1 = p3_sub(b, a);
  const P3 e2 = p3_sub(x, a);
  const P3 e3 = p3_sub(y, a);
  const i64 m[3][3] = {{2 * e1.x, 2 * e1.y, 2 * e1.z}, {2 * e2.x, 2 * e2.y, 2 * e2.z}, {2 * e3.x, 2 * e3.y, 2 * e3.z}};
  const i64 r[3] = {p3_norm2(e1), p3_norm2(e2), p3_norm2(e3)};
  Q4Form f;
  f.a = a;
  const auto cof = [&](int i0, int i1, int j0, int j1) {
    return (i128)m[i0][j0] * m[i1][j1] - (i128)m[i0][j1] * m[i1][j0];
  };
  const i128 c00 = cof(1, 2, 1, 2), c01 = -cof(1, 2, 0, 2), c02 = cof(1, 2, 0, 1);
  const i128 c10 = -cof(0, 2, 1, 2), c11 = cof(0, 2, 0, 2), c12 = -cof(0, 2, 0, 1);
  const i128 c20 = cof(0, 1, 1, 2), c21 = -cof(0, 1, 0, 2), c22 = cof(0, 1, 0, 1);
  f.det = m[0][0] * c00 + m[0][1] * c01 + m[0][2] * c02;
  f.np[0] = c00 * r[0] + c10 * r[1] + c20 * r[2];
  f.np[1] = c01 * r[0] + c11 * r[1] + c21 * r[2];
  f.np[2] = c02 * r[0] + c12 * r[1] + c22 * r[2];
  if (f.det < 0) {
    f.det = -f.det;
    for (int i = 0; i < 3; ++i) f.np[i] = -f.np[i];
  }
  return f;
}

MHGP7_HD inline i128 q4_power(const Q4Form& f, const P3& z) {
  const P3 v = p3_sub(z, f.a);
  return f.det * p3_norm2(v) - 2 * (f.np[0] * v.x + f.np[1] * v.y + f.np[2] * v.z);
}

// Precondition : det > 0.
inline bool q4_center_strictly_inside(const Q4Form& f, const P3& a, const P3& b, const P3& x, const P3& y) {
  const bool parity = MHGP7_MUTANT("q4-center-parity");
  const i64 ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
  const i64 px = x.x - a.x, py = x.y - a.y, pz = x.z - a.z;
  const i64 qx = y.x - a.x, qy = y.y - a.y, qz = y.z - a.z;
  const i64 vol = ux * (py * qz - pz * qy) - uy * (px * qz - pz * qx) + uz * (px * qy - py * qx);
  const P3* v[4] = {&a, &b, &x, &y};
  for (int s = 0; s < 4; ++s) {
    const P3* fp[3];
    int t = 0;
    for (int i = 0; i < 4; ++i)
      if (i != s) fp[t++] = v[i];
    const P3 e1 = p3_sub(*fp[1], *fp[0]);
    const P3 e2 = p3_sub(*fp[2], *fp[0]);
    const i64 r0[3] = {e1.x, e1.y, e1.z};
    const i64 r1[3] = {e2.x, e2.y, e2.z};
    const P3 dp = p3_sub(*fp[0], f.a);
    const i128 rc[3] = {f.np[0] - f.det * dp.x, f.np[1] - f.det * dp.y, f.np[2] - f.det * dp.z};
    const bool even_face = (s % 2) == 0;
    const bool side_s_pos = (even_face != parity) ? (vol < 0) : (vol > 0);
    const i128 side_c = q4_detail::det3_i128(r0, r1, rc);
    if (side_c == 0) return false;
    if ((side_c > 0) != side_s_pos) return false;
  }
  return true;
}

// Etage i64 du prefiltre : deux consequences necessaires du bien-centrage.
MHGP7_HD inline bool q4_i64_prefilter(i64 D2, i64 l_ax, i64 l_bx, i64 l_ay, i64 l_by, i64 l_xy) {
  const i64 m1 = MHGP7_MUTANT("q4-i64-drop-factor") ? std::max(l_ay, std::max(l_by, l_xy))
                                                      : 2 * std::max(l_ay, std::max(l_by, l_xy));
  if (!(m1 > D2)) return false;
  const i64 m2 = MHGP7_MUTANT("q4-i64-pair-min") ? std::min(l_ax + l_ay, l_bx + l_by)
                                                   : std::max(l_ax + l_ay, l_bx + l_by);
  return m2 > D2;
}

// Puissance equatoriale de la face abx (forme q3 du seed) sur y : le centre
// et y sont strictement du meme cote ssi q3_power(face, y) > 0.
MHGP7_HD inline bool q4_face_power_prefilter(const Q3Form& face, const P3& y) {
  const i128 p = q3_power(face, y);
  if (MHGP7_MUTANT("q4-eq-sign")) return p < 0;
  if (MHGP7_MUTANT("q4-eq-nonstrict")) return p >= 0;
  return p > 0;
}

MHGP7_HD inline bool tetra_owned_by(i64 l_ab, i64 l_ax, i64 l_ay, i64 l_bx, i64 l_by, i64 l_xy, PointId ida,
                                    PointId idb, PointId idx, PointId idy) {
  const EdgeKey e_ab = edge_key(ida, idb);
  const auto beats = [&](i64 l, PointId u, PointId w) { return l > l_ab || (l == l_ab && edge_key(u, w) < e_ab); };
  return !beats(l_ax, ida, idx) && !beats(l_ay, ida, idy) && !beats(l_bx, idb, idx) && !beats(l_by, idb, idy) &&
         !beats(l_xy, idx, idy);
}

MHGP7_HD inline BallForm q4_ball_form(const Q4Form& f) {
  BallForm r;
  r.a = f.det;
  const i64 ax[3] = {f.a.x, f.a.y, f.a.z};
  i128 na = 0;
  for (int i = 0; i < 3; ++i) {
    r.b[i] = -2 * (f.det * ax[i] + f.np[i]);
    na += f.np[i] * ax[i];
  }
  r.c = f.det * ((i128)ax[0] * ax[0] + (i128)ax[1] * ax[1] + (i128)ax[2] * ax[2]) + 2 * na;
  return r;
}

// Niveau q4 : |N'|² (U192) / det² (i128), non reduits.
MHGP7_HD inline ExactLevel q4_level_raw(const Q4Form& f) {
  ExactLevel l{{0, 0, 0}, f.det * f.det};
  const U192 n = sum_of_three_squares_192(uabs128(f.np[0]), uabs128(f.np[1]), uabs128(f.np[2]));
  l.num[0] = n.w[0];
  l.num[1] = n.w[1];
  l.num[2] = n.w[2];
  return l;
}

struct Q4Event {
  SupportKey4 support;
  EdgeKey owner;
  BallKey ball;
  ExactLevel level;
  u8 depth = 0;
  std::array<PointId, 8> interior{};
};

}  // namespace mhgp7
