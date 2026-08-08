// MorseHGP3D v2 — spheres definies par leur support minimal.
//
// Une sphere est portee par un support U (1 a 4 points affinement
// independants). Son centre s'ecrit  base + num / den  avec den > 0, et son
// niveau vaut beta = |num|^2 / den^2. Toutes les decisions sont exactes.
#pragma once

#include "mhgp/exact.hpp"

namespace mhgp {

struct Sphere {
  P3 base;          // premier point du support
  i128 nx, ny, nz;  // numerateurs du centre relatif
  i128 den;         // > 0
  int support;      // 1..4
};

// ---------------------------------------------------------------------------
// Construction. Les entrees sont des points quantifies ; les differences sont
// bornees par 2^(kCoordBits+1).
// ---------------------------------------------------------------------------
MHGP_HD inline Sphere sphere1(const P3& a) {
  return Sphere{a, 0, 0, 0, 1, 1};
}

MHGP_HD inline Sphere sphere2(const P3& a, const P3& b) {
  const P3 B = p3_sub(b, a);
  return Sphere{a, B.x, B.y, B.z, 2, 2};
}

// den = 2 |B1 x B2|^2  <= 2^72.6 ; |num| <= 2^89.6
MHGP_HD inline bool sphere3(const P3& a, const P3& b, const P3& c, Sphere* out) {
  const P3 B1 = p3_sub(b, a), B2 = p3_sub(c, a);
  const P3 X = p3_cross(B1, B2);
  const i128 x2 = p3_norm2(X);
  if (x2 == 0) return false;  // alignes
  const i128 l1 = p3_norm2(B1), l2 = p3_norm2(B2);
  // T = l1 * B2 - l2 * B1   (composantes < 2^53.6)
  const i128 tx = l1 * B2.x - l2 * B1.x;
  const i128 ty = l1 * B2.y - l2 * B1.y;
  const i128 tz = l1 * B2.z - l2 * B1.z;
  // num = T x X
  out->base = a;
  out->nx = ty * X.z - tz * X.y;
  out->ny = tz * X.x - tx * X.z;
  out->nz = tx * X.y - ty * X.x;
  out->den = 2 * x2;
  out->support = 3;
  return true;
}

// den = 2 det(B1,B2,B3) <= 2^54.6 ; |num| <= 2^72.2
MHGP_HD inline bool sphere4(const P3& a, const P3& b, const P3& c, const P3& d,
                            Sphere* out) {
  const P3 B1 = p3_sub(b, a), B2 = p3_sub(c, a), B3 = p3_sub(d, a);
  i128 dt = det3(B1, B2, B3);
  if (dt == 0) return false;  // coplanaires
  const i128 l1 = p3_norm2(B1), l2 = p3_norm2(B2), l3 = p3_norm2(B3);
  const P3 c23 = p3_cross(B2, B3), c31 = p3_cross(B3, B1), c12 = p3_cross(B1, B2);
  i128 nx = l1 * c23.x + l2 * c31.x + l3 * c12.x;
  i128 ny = l1 * c23.y + l2 * c31.y + l3 * c12.y;
  i128 nz = l1 * c23.z + l2 * c31.z + l3 * c12.z;
  i128 den = 2 * dt;
  if (den < 0) { den = -den; nx = -nx; ny = -ny; nz = -nz; }
  out->base = a;
  out->nx = nx; out->ny = ny; out->nz = nz;
  out->den = den;
  out->support = 4;
  return true;
}

// ---------------------------------------------------------------------------
// Position d'un point : < 0 dedans, = 0 dessus, > 0 dehors.
// Evalue |w|^2 * den - 2 <w, num> avec w = z - base ; borne 2^108.2 (i128).
// ---------------------------------------------------------------------------
MHGP_HD inline int sphere_side(const Sphere& s, const P3& z) {
  const P3 w = p3_sub(z, s.base);
  const i128 lhs = p3_norm2(w) * s.den
                 - 2 * (i128(w.x) * s.nx + i128(w.y) * s.ny + i128(w.z) * s.nz);
  return (lhs > 0) - (lhs < 0);
}

// ---------------------------------------------------------------------------
// Bon centrage : le centre est-il dans l'interieur relatif de conv(U) ?
// ---------------------------------------------------------------------------
MHGP_HD inline bool well_centered2() { return true; }

// Triangle strictement acutangle. Degre 2.
MHGP_HD inline bool well_centered3(const P3& a, const P3& b, const P3& c) {
  const P3 ab = p3_sub(b, a), ac = p3_sub(c, a), bc = p3_sub(c, b);
  const i128 la = p3_norm2(bc), lb = p3_norm2(ac), lc = p3_norm2(ab);
  return (lb + lc - la) > 0 && (la + lc - lb) > 0 && (la + lb - lc) > 0;
}

// Tetraedre : coordonnees barycentriques strictement positives.
// t_j = 2 det(B1,..,num,..,B3) / den^2 ; borne 2^108.8 (i128).
MHGP_HD inline bool well_centered4(const Sphere& s, const P3& a, const P3& b,
                                   const P3& c, const P3& d) {
  const P3 B1 = p3_sub(b, a), B2 = p3_sub(c, a), B3 = p3_sub(d, a);
  // det avec la colonne j remplacee par num : on developpe a la main
  // det(N, B2, B3), det(B1, N, B3), det(B1, B2, N) avec N = (nx, ny, nz)
  const i128 nx = s.nx, ny = s.ny, nz = s.nz;
  const i128 d1 = nx * (i128(B2.y) * B3.z - i128(B2.z) * B3.y)
                - ny * (i128(B2.x) * B3.z - i128(B2.z) * B3.x)
                + nz * (i128(B2.x) * B3.y - i128(B2.y) * B3.x);
  const i128 d2 = i128(B1.x) * (ny * B3.z - nz * B3.y)
                - i128(B1.y) * (nx * B3.z - nz * B3.x)
                + i128(B1.z) * (nx * B3.y - ny * B3.x);
  const i128 d3 = i128(B1.x) * (i128(B2.y) * nz - i128(B2.z) * ny)
                - i128(B1.y) * (i128(B2.x) * nz - i128(B2.z) * nx)
                + i128(B1.z) * (i128(B2.x) * ny - i128(B2.y) * nx);
  // sphere4 a normalise den > 0 en changeant au besoin le signe de num ; le
  // signe de det(B1,B2,B3) n'est donc plus lisible dans la sphere et doit etre
  // recalcule : t_j = 2 * sgn * d_j / den^2.
  const i128 dt = det3(B1, B2, B3);
  if (dt == 0) return false;
  const int sgn = (dt > 0) ? 1 : -1;
  if (sgn * d1 <= 0 || sgn * d2 <= 0 || sgn * d3 <= 0) return false;
  // sum t_j < 1  <=>  2 * sgn * (d1 + d2 + d3) < den^2
  const i128 lhs = 2 * sgn * (d1 + d2 + d3);
  const i128 rhs = s.den * s.den;
  return lhs < rhs;
}

// ---------------------------------------------------------------------------
// Niveau beta = |num|^2 / den^2 et comparaison exacte.
// |num|^2 <= 2^180.8 (BigInt<4>) ; produit croise <= 2^326 (BigInt<6>).
// ---------------------------------------------------------------------------
MHGP_HD inline BigInt<4> sphere_num2(const Sphere& s) {
  BigInt<4> r = mul128(s.nx, s.nx);
  r = big_add(r, mul128(s.ny, s.ny));
  r = big_add(r, mul128(s.nz, s.nz));
  return r;
}

MHGP_HD inline double sphere_beta(const Sphere& s) {
  const double d = static_cast<double>(s.den);
  return big_to_double(sphere_num2(s)) / (d * d);
}

MHGP_HD inline int sphere_cmp_beta(const Sphere& a, const Sphere& b) {
  const BigInt<4> na = sphere_num2(a);
  const BigInt<4> nb = sphere_num2(b);
  BigInt<6> lhs = big_mul_i128<6, 4>(na, b.den);
  lhs = big_mul_i128<6, 6>(lhs, b.den);
  BigInt<6> rhs = big_mul_i128<6, 4>(nb, a.den);
  rhs = big_mul_i128<6, 6>(rhs, a.den);
  return big_cmp(lhs, rhs);
}

}  // namespace mhgp
