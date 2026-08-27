// MorseHGP3D v5 — formes exactes q3/q4 et fuseaux sur DI128 (suffixe `_d`) :
// les MEMES formes que q3.hpp / q4.hpp / spindle.hpp, sans i128, compilables en
// device (MHGP5_HD). Fonctions DISTINCTES des formes de production : la porte
// `mhgp5_dint_gate` prouve l'egalite valeur par valeur sur tous les
// triangles/tetraedres de petits nuages et des fixtures u16 extremes ; le
// port GPU s'appuiera sur ces `_d`, jamais sur une copie locale.
//
// LARGEURS (profil u16, deltas de coordonnees |e| <= 65535 < 2^16 par axe) :
//   longueurs carrees, produits scalaires D, E, F, H         < 3·2^32 < 2^34 (i64)
//   composante d'un produit vectoriel                       < 2^33      (i64)
//   q3 : G = DE - F²                                        < 9·2^64 < 2^68
//        c1 = E(D-F), c2 = D(E-F) : |D-F| < 6·2^32          -> |c1|, |c2| < 2^69
//        W_i = c1·d_i + c2·u_i : 2^69 × 2^16, deux termes     -> |W_i| < 2^87
//        P(z) = G|v|² - W·v : 2^68 × 2^36 et 2^87 × 2^16 × 3 -> |P| < 2^106
//   q4 : lignes m = 2e < 2^17 ; cofacteurs (2 produits 17×17) < 2^35 (i64) ;
//        det = Σ m·cof : 3 × 2^52                            < 2^54
//        r_i = |e_i|² < 2^34 ; N'_j = Σ cof·r : 3 × 2^69     < 2^71
//        P4(z) = det|v|² - 2N'·v : 2^90 et 2^88 × 3          < 2^92
//        bien-centrage : rc_i = N'_i - det·dp_i (2^71 + 2^70) < 2^72 ;
//        mineurs de (e1, e2) < 2^33 ; det3 = Σ rc·mineur      < 2^107
//        niveau : |N'|² < 3·2^142 < 2^144 (U192) ; det² < 2^108 (DI128,
//        det < 2^54 tient en i64 : produit i64×i64 exact)
//   fuseaux : H = d·w - |w|² : |H| < 2^35 ; H² < 2^70 ; 3H² < 2^72 ;
//        Ξ = |d×w|² = 3 × 2^66                                < 2^68
// Chaque DI128×i64 de ce fichier a un vrai produit < 2^111 : la precondition
// de di_mul_di128_i64 (resultat dans [-2^127, 2^127)) est satisfaite avec une
// marge > 2^16. Les bornes de q3.hpp/q4.hpp (2^103, 2^105, 2^57, 2^72, 2^94)
// sont des majorants plus laches des memes quantites : compatibles.
//
// NON PORTE (exige U320 ou un rationnel large) : `cmp_2p2_jb2` (float_filter),
// `compare_exact_level` / `same_exact_level` (produits croises U320 des
// niveaux q4), `compare_rational`, la reduction pgcd (`rational_reduce`,
// `ball_key_reduce`) et `q3_ball_depth` (descente d'arbre, host). Le niveau
// q4 est fourni NON REDUIT (num U192, den DI128 = det²) : c'est la
// representation d'ExactLevel, l'ORDRE des niveaux reste au host.
#pragma once

#include "../core/dint.hpp"
#include "../core/types.hpp"

namespace mhgp5 {

namespace device_detail {
// Primitives P3 en i64 avec MHGP5_HD (celles de types.hpp sont constexpr sans
// MHGP5_HD : nvcc exigerait --expt-relaxed-constexpr pour les appeler).
MHGP5_HD inline constexpr P3 sub(const P3& a, const P3& b) { return P3{a.x - b.x, a.y - b.y, a.z - b.z}; }
MHGP5_HD inline constexpr i64 dot(const P3& a, const P3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
MHGP5_HD inline constexpr i64 norm2(const P3& a) { return dot(a, a); }
MHGP5_HD inline constexpr P3 cross(const P3& a, const P3& b) {
  return P3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// det3 [r0 ; r1 ; r2] avec r0, r1 en i64 (mineurs 2×2 < 2^33, i64) et r2 en
// DI128 : trois DI128×i64 (< 2^107) sommes.
MHGP5_HD inline MHGP5_DI_CONSTEXPR DI128 det3_d(const i64 r0[3], const i64 r1[3], const DI128 r2[3]) {
  const i64 m0 = r0[1] * r1[2] - r0[2] * r1[1];
  const i64 m1 = r0[0] * r1[2] - r0[2] * r1[0];
  const i64 m2 = r0[0] * r1[1] - r0[1] * r1[0];
  return di_add(di_sub(di_mul_di128_i64(r2[0], m0), di_mul_di128_i64(r2[1], m1)), di_mul_di128_i64(r2[2], m2));
}
}  // namespace device_detail

// ---- q3 -------------------------------------------------------------------------------

struct Q3FormD {
  DI128 g;     // G > 0 pour un triangle non degenere
  DI128 w[3];  // W
  P3 a;
};

// Forme de Gram (q3.hpp::q3_form) : G = DE - F², W = E(D-F) d + D(E-F) u.
MHGP5_HD inline MHGP5_DI_CONSTEXPR Q3FormD q3_form_d(const P3& a, const P3& b, const P3& x) {
  const P3 d = device_detail::sub(b, a);
  const P3 u = device_detail::sub(x, a);
  const i64 D = device_detail::norm2(d);
  const i64 E = device_detail::norm2(u);
  const i64 F = device_detail::dot(d, u);
  Q3FormD f;
  f.a = a;
  f.g = di_sub(di_mul_i64_i64(D, E), di_mul_i64_i64(F, F));
  const DI128 c1 = di_mul_i64_i64(E, D - F);
  const DI128 c2 = di_mul_i64_i64(D, E - F);
  f.w[0] = di_add(di_mul_di128_i64(c1, d.x), di_mul_di128_i64(c2, u.x));
  f.w[1] = di_add(di_mul_di128_i64(c1, d.y), di_mul_di128_i64(c2, u.y));
  f.w[2] = di_add(di_mul_di128_i64(c1, d.z), di_mul_di128_i64(c2, u.z));
  return f;
}

// P(z) = G|z-a|² - (z-a)·W (q3.hpp::q3_power) : < 0 interieur strict, = 0 coquille.
MHGP5_HD inline MHGP5_DI_CONSTEXPR DI128 q3_power_d(const Q3FormD& f, const P3& z) {
  const P3 v = device_detail::sub(z, f.a);
  const DI128 wv =
      di_add(di_add(di_mul_di128_i64(f.w[0], v.x), di_mul_di128_i64(f.w[1], v.y)), di_mul_di128_i64(f.w[2], v.z));
  return di_sub(di_mul_di128_i64(f.g, device_detail::norm2(v)), wv);
}

// ---- q4 -------------------------------------------------------------------------------

struct Q4FormD {
  DI128 det;    // > 0 apres canonisation ; 0 = coplanaire (refus)
  DI128 np[3];  // N' : c - a = N'/det
  P3 a;
};

// Cramer 3×3 relatif (q4.hpp::q4_form), cofacteurs en i64 (< 2^35), det et N'
// par produits i64×i64 exacts ; canonisation det > 0.
MHGP5_HD inline MHGP5_DI_CONSTEXPR Q4FormD q4_form_d(const P3& a, const P3& b, const P3& x, const P3& y) {
  const P3 e1 = device_detail::sub(b, a);
  const P3 e2 = device_detail::sub(x, a);
  const P3 e3 = device_detail::sub(y, a);
  const i64 m[3][3] = {{2 * e1.x, 2 * e1.y, 2 * e1.z}, {2 * e2.x, 2 * e2.y, 2 * e2.z}, {2 * e3.x, 2 * e3.y, 2 * e3.z}};
  const i64 r[3] = {device_detail::norm2(e1), device_detail::norm2(e2), device_detail::norm2(e3)};
  Q4FormD f;
  f.a = a;
  // cof(i0, i1, j0, j1) = m[i0][j0]·m[i1][j1] - m[i0][j1]·m[i1][j0], < 2^35.
  const i64 c00 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
  const i64 c01 = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]);
  const i64 c02 = m[1][0] * m[2][1] - m[1][1] * m[2][0];
  const i64 c10 = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]);
  const i64 c11 = m[0][0] * m[2][2] - m[0][2] * m[2][0];
  const i64 c12 = -(m[0][0] * m[2][1] - m[0][1] * m[2][0]);
  const i64 c20 = m[0][1] * m[1][2] - m[0][2] * m[1][1];
  const i64 c21 = -(m[0][0] * m[1][2] - m[0][2] * m[1][0]);
  const i64 c22 = m[0][0] * m[1][1] - m[0][1] * m[1][0];
  f.det = di_add(di_add(di_mul_i64_i64(m[0][0], c00), di_mul_i64_i64(m[0][1], c01)), di_mul_i64_i64(m[0][2], c02));
  f.np[0] = di_add(di_add(di_mul_i64_i64(c00, r[0]), di_mul_i64_i64(c10, r[1])), di_mul_i64_i64(c20, r[2]));
  f.np[1] = di_add(di_add(di_mul_i64_i64(c01, r[0]), di_mul_i64_i64(c11, r[1])), di_mul_i64_i64(c21, r[2]));
  f.np[2] = di_add(di_add(di_mul_i64_i64(c02, r[0]), di_mul_i64_i64(c12, r[1])), di_mul_i64_i64(c22, r[2]));
  if (di_is_neg(f.det)) {
    f.det = di_neg(f.det);
    for (int i = 0; i < 3; ++i) f.np[i] = di_neg(f.np[i]);
  }
  return f;
}

// P4(z) = det|z-a|² - 2N'·(z-a) (q4.hpp::q4_power).
MHGP5_HD inline MHGP5_DI_CONSTEXPR DI128 q4_power_d(const Q4FormD& f, const P3& z) {
  const P3 v = device_detail::sub(z, f.a);
  const DI128 nv = di_add(di_add(di_mul_di128_i64(f.np[0], v.x), di_mul_di128_i64(f.np[1], v.y)),
                          di_mul_di128_i64(f.np[2], v.z));
  return di_sub(di_mul_di128_i64(f.det, device_detail::norm2(v)), di_shl1(nv));
}

// Bien-centrage STRICT (q4.hpp::q4_center_strictly_inside). Precondition det > 0.
// Preuve de parite reprise : V = det3(b-a, x-a, y-a) est calcule UNE fois ;
// pour la face opposee au sommet s (les trois autres dans l'ordre (a,b,x,y)
// prive de s), l'orientation du sommet oppose par rapport a cette face vaut
// (-V, +V, -V, +V) pour s = 0..3 : retirer le sommet s d'un determinant 4×4
// alterne le signe. Le centre c = a + N'/det est du meme cote ssi
// det3(e1, e2, det·(c - fp0)) = det3(e1, e2, N' - det·(fp0 - a)) a le signe de
// l'orientation (det > 0 ne change pas le signe). Un zero = centre sur la
// face = frontiere refusee (arite <= 3, autre lane).
MHGP5_HD inline MHGP5_DI_CONSTEXPR bool q4_center_strictly_inside_d(const Q4FormD& f, const P3& a, const P3& b,
                                                                    const P3& x, const P3& y) {
  const i64 ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
  const i64 px = x.x - a.x, py = x.y - a.y, pz = x.z - a.z;
  const i64 qx = y.x - a.x, qy = y.y - a.y, qz = y.z - a.z;
  const i64 vol = ux * (py * qz - pz * qy) - uy * (px * qz - pz * qx) + uz * (px * qy - py * qx);
  const P3* v[4] = {&a, &b, &x, &y};
  for (int s = 0; s < 4; ++s) {
    const P3* fp[3] = {nullptr, nullptr, nullptr};
    int t = 0;
    for (int i = 0; i < 4; ++i)
      if (i != s) fp[t++] = v[i];
    const P3 e1 = device_detail::sub(*fp[1], *fp[0]);
    const P3 e2 = device_detail::sub(*fp[2], *fp[0]);
    const i64 r0[3] = {e1.x, e1.y, e1.z};
    const i64 r1[3] = {e2.x, e2.y, e2.z};
    const P3 dp = device_detail::sub(*fp[0], f.a);
    const DI128 rc[3] = {di_sub(f.np[0], di_mul_di128_i64(f.det, dp.x)), di_sub(f.np[1], di_mul_di128_i64(f.det, dp.y)),
                         di_sub(f.np[2], di_mul_di128_i64(f.det, dp.z))};
    const bool even_face = (s % 2) == 0;
    const bool side_s_pos = even_face ? (vol < 0) : (vol > 0);
    const int side_c = di_sign(device_detail::det3_d(r0, r1, rc));
    if (side_c == 0) return false;
    if ((side_c > 0) != side_s_pos) return false;
  }
  return true;
}

// Niveau q4 NON REDUIT (q4.hpp::q4_level_raw) : num = |N'|² (U192), den = det²
// (DI128). Precondition : det tient en i64 (vrai sous u16 : det < 2^54) ;
// rend false sinon, sans calculer.
MHGP5_HD inline MHGP5_DI_CONSTEXPR bool q4_level_d(const Q4FormD& f, U192* num, DI128* den) {
  if (!di_fits_i64(f.det)) return false;
  const i64 d = di_to_i64_unchecked(f.det);
  *den = di_mul_i64_i64(d, d);
  *num = di_sum_of_three_squares_192(f.np[0], f.np[1], f.np[2]);
  return true;
}

// ---- Fuseaux ----------------------------------------------------------------------------

// z ∈ W_q(a,b) (spindle.hpp::in_spindle), arity ∈ {2, 3, 4} : H > 0 puis
// 3H² > Ξ (q3) / 2H² > Ξ (q4). Une arite hors {2,3,4} rend false.
MHGP5_HD inline MHGP5_DI_CONSTEXPR bool in_spindle_d(int arity, const P3& a, const P3& b, const P3& z) {
  const P3 w = device_detail::sub(z, a);
  const P3 d = device_detail::sub(b, a);
  const i64 h = device_detail::dot(d, w) - device_detail::norm2(w);
  if (h <= 0) return false;
  if (arity == 2) return true;
  if (arity != 3 && arity != 4) return false;
  const P3 c = device_detail::cross(d, w);
  const DI128 xi = di_add(di_add(di_mul_i64_i64(c.x, c.x), di_mul_i64_i64(c.y, c.y)), di_mul_i64_i64(c.z, c.z));
  const DI128 h2 = di_mul_i64_i64(h, h);
  const DI128 lhs = (arity == 3) ? di_add(h2, di_shl1(h2)) : di_shl1(h2);
  return di_cmp(lhs, xi) > 0;
}

}  // namespace mhgp5
