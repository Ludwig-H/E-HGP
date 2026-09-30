// Predicats geometriques exacts sur coordonnees entieres u18 (differences |.| < 2^19).
//
// Une sphere candidate est donnee par un site d'ancrage a et son centre rationnel c = a + N / D (D > 0),
// ou N et D sont des i128. Bornes (u18) :
//   q2 : N = b - a, D = 2                    ; q3 : |N| < 2^99.6, D < 2^81.6 ; q4 : |N| < 2^80.2, D < 2^61.6.
// Le cote d'un site z : signe de D|z-a|^2 - 2 N.(z-a) (< 0 interieur strict, 0 coquille) ; |.| < 2^121.
// Niveau r^2 = num / den : q2 |u|^2 / 4 ; q3 |u|^2 |v|^2 |u-v|^2 / (4 |u x v|^2) ; q4 |N|^2 / D^2.
// num < 2^162, den < 2^124 : comparaison par produits croises en 320 bits.
#pragma once

#include "arith/wide.hpp"
#include "core/types.hpp"

namespace mhgp10::geom {

struct P3 {
  i64 x, y, z;
};

inline P3 sub(const P3& a, const P3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline i64 dot(const P3& a, const P3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline P3 cross(const P3& a, const P3& b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

struct Center {
  i128 N[3];  // relatif au site d'ancrage
  i128 D;     // > 0
};

// Niveau exact (rayon carre) : num / den, den > 0.
struct Level {
  arith::I192 num;
  arith::I128w den;
  double approx() const;
};

// Comparaison exacte de deux niveaux : -1, 0, 1.
int compare(const Level& a, const Level& b);

// Centre du cercle circonscrit (dans le plan) de a, b, c relatif a a ; false si alignes.
inline bool center3(const P3& a, const P3& b, const P3& c, Center& out) {
  const P3 u = sub(b, a), v = sub(c, a);
  const P3 w = cross(u, v);
  if (w.x == 0 && w.y == 0 && w.z == 0) return false;
  const i128 uu = dot(u, u), vv = dot(v, v);
  const i128 tx = uu * v.x - vv * u.x, ty = uu * v.y - vv * u.y, tz = uu * v.z - vv * u.z;
  out.N[0] = ty * w.z - tz * w.y;
  out.N[1] = tz * w.x - tx * w.z;
  out.N[2] = tx * w.y - ty * w.x;
  out.D = 2 * (i128(w.x) * w.x + i128(w.y) * w.y + i128(w.z) * w.z);
  return true;
}

// Centre de la sphere circonscrite a a, b, c, d relatif a a ; false si coplanaires.
inline bool center4(const P3& a, const P3& b, const P3& c, const P3& d, Center& out) {
  const P3 u = sub(b, a), v = sub(c, a), s = sub(d, a);
  const i128 det = i128(u.x) * (i128(v.y) * s.z - i128(v.z) * s.y) - i128(u.y) * (i128(v.x) * s.z - i128(v.z) * s.x) +
                   i128(u.z) * (i128(v.x) * s.y - i128(v.y) * s.x);
  if (det == 0) return false;
  const i128 uu = dot(u, u), vv = dot(v, v), ss = dot(s, s);
  const i128 vsx = i128(v.y) * s.z - i128(v.z) * s.y, vsy = i128(v.z) * s.x - i128(v.x) * s.z,
             vsz = i128(v.x) * s.y - i128(v.y) * s.x;
  const i128 sux = i128(s.y) * u.z - i128(s.z) * u.y, suy = i128(s.z) * u.x - i128(s.x) * u.z,
             suz = i128(s.x) * u.y - i128(s.y) * u.x;
  const i128 uvx = i128(u.y) * v.z - i128(u.z) * v.y, uvy = i128(u.z) * v.x - i128(u.x) * v.z,
             uvz = i128(u.x) * v.y - i128(u.y) * v.x;
  i128 D = 2 * det;
  i128 N0 = uu * vsx + vv * sux + ss * uvx, N1 = uu * vsy + vv * suy + ss * uvy, N2 = uu * vsz + vv * suz + ss * uvz;
  if (D < 0) {
    D = -D;
    N0 = -N0;
    N1 = -N1;
    N2 = -N2;
  }
  out.N[0] = N0;
  out.N[1] = N1;
  out.N[2] = N2;
  out.D = D;
  return true;
}

inline void center2(const P3& a, const P3& b, Center& out) {
  out.N[0] = b.x - a.x;
  out.N[1] = b.y - a.y;
  out.N[2] = b.z - a.z;
  out.D = 2;
}

// Cle de puissance exacte s(z) = D |z - a|^2 - 2 N.(z - a) = D (|z - c|^2 - r^2) ; |s| < 2^122 en u18.
inline i128 side_key(const Center& c, const P3& a, const P3& z) {
  const i128 dx = z.x - a.x, dy = z.y - a.y, dz = z.z - a.z;
  return c.D * (dx * dx + dy * dy + dz * dz) - 2 * (c.N[0] * dx + c.N[1] * dy + c.N[2] * dz);
}

// Cote du site z par rapport a la sphere de centre a + N/D passant par a : -1 interieur, 0 coquille, 1 exterieur.
inline int side(const Center& c, const P3& a, const P3& z) {
  const i128 dx = z.x - a.x, dy = z.y - a.y, dz = z.z - a.z;
  const i128 lhs = c.D * (dx * dx + dy * dy + dz * dz);
  const i128 rhs = 2 * (c.N[0] * dx + c.N[1] * dy + c.N[2] * dz);
  return lhs < rhs ? -1 : (lhs == rhs ? 0 : 1);
}

// Orientation de s par rapport au plan (p, q, r) : signe de det[q-p, r-p, s-p].
inline int orient(const P3& p, const P3& q, const P3& r, const P3& s) {
  const P3 a = sub(q, p), b = sub(r, p), c = sub(s, p);
  const i128 v = i128(a.x) * (i128(b.y) * c.z - i128(b.z) * c.y) - i128(a.y) * (i128(b.x) * c.z - i128(b.z) * c.x) +
                 i128(a.z) * (i128(b.x) * c.y - i128(b.y) * c.x);
  return v > 0 ? 1 : (v < 0 ? -1 : 0);
}

// Orientation du centre rationnel (anchor + N/D) par rapport au plan (p, q, r). Borne i128 valable pour
// les centres de forme q4 (|N| < 2^80.2, D < 2^61.6) : |c_i| < 2^81, |w_i| < 2^39, somme < 2^122.
inline int orient_center(const P3& p, const P3& q, const P3& r, const P3& anchor, const Center& c) {
  const P3 a = sub(q, p), b = sub(r, p);
  const i128 w0 = i128(a.y) * b.z - i128(a.z) * b.y, w1 = i128(a.z) * b.x - i128(a.x) * b.z,
             w2 = i128(a.x) * b.y - i128(a.y) * b.x;
  const i128 c0 = c.N[0] + c.D * (anchor.x - p.x), c1 = c.N[1] + c.D * (anchor.y - p.y),
             c2 = c.N[2] + c.D * (anchor.z - p.z);
  const i128 v = w0 * c0 + w1 * c1 + w2 * c2;
  return v > 0 ? 1 : (v < 0 ? -1 : 0);
}

// Triangle strictement aigu (centre circonscrit strictement interieur).
inline bool acute(const P3& a, const P3& b, const P3& c) {
  return dot(sub(b, a), sub(c, a)) > 0 && dot(sub(a, b), sub(c, b)) > 0 && dot(sub(a, c), sub(b, c)) > 0;
}

// Centre (anchor + N/D) strictement interieur au tetraedre t[0..3].
bool strictly_inside_tetra(const P3* t[4], const P3& anchor, const Center& c);

// Orientation du centre en arithmetique large (tout centre u18, dont la forme q3 ou |N| atteint 2^100).
int orient_center_wide(const P3& p, const P3& q, const P3& r, const P3& anchor, const Center& c);

// Centre (anchor + N/D) dans le plan de a, b, c.
inline bool center_in_plane(const P3& a, const P3& b, const P3& c, const P3& anchor, const Center& ctr) {
  return orient_center_wide(a, b, c, anchor, ctr) == 0;
}

// Milieu de a, b egal au centre (anchor + N/D) : 2 (anchor D + N) = (a + b) D.
inline bool is_midpoint(const P3& a, const P3& b, const P3& anchor, const Center& c) {
  return 2 * (anchor.x * c.D + c.N[0]) == (i128(a.x) + b.x) * c.D &&
         2 * (anchor.y * c.D + c.N[1]) == (i128(a.y) + b.y) * c.D &&
         2 * (anchor.z * c.D + c.N[2]) == (i128(a.z) + b.z) * c.D;
}

// Niveaux exacts des trois formes.
Level level2(const P3& a, const P3& b);
Level level3(const P3& a, const P3& b, const P3& c);
Level level4(const Center& c);

}  // namespace mhgp10::geom
