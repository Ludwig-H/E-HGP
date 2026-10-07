// Predicats exacts de la feuille data-parallele (MES-M2, hors produit).
//
// PORT EXPLICITE de morsehgp3D_v11/src/catalogue/leaf_device_predicates.hpp (moteur ac081a06f, sha256
// c638996b661c6fc690a9dff30d61a0c5294b882001bb806009d72120f6168472) : memes formules, memes bornes, memes certificats,
// memes issues. Seul ajout : side_q2_narrow, voie i64 du cote q2 sous l'etendue de feuille <= 2^20 (lemme ci-dessous),
// qui rend le meme signe que side (meme valeur au facteur 2 > 0 pres). Aucun flottant ; un predicat non certifie rend
// faux et la feuille devient non resolue (contrat R7 de la v11).
#pragma once

#include "mhgp12/leaf/simt.hpp"

#ifndef MHGP12_COORD_BITS
#error "mhgp12 : MHGP12_COORD_BITS (21 ou 24) requis"
#endif

namespace mhgp12::leaf {

using simt::i128;
using simt::i32;
using simt::i64;
using simt::u32;
using simt::u64;
using simt::u8;

inline constexpr int kBits = MHGP12_COORD_BITS;
static_assert(kBits == 21 || kBits == 24, "mhgp12 : profil 21 ou 24 bits (u18 abandonne, decision D de la v12)");
inline constexpr u32 kMaxSites = 32;
// Levier C de la v11 : etendue de feuille (sites et fermeture de la boite) <= 2^20 sur chaque axe.
inline constexpr i64 kNarrowSpan = i64(1) << 20;

MHGP12_HD int sign(i128 v) { return (v > 0) - (v < 0); }

struct Vec {
  i64 v[3];
};
MHGP12_HD Vec diff(const u32* a, const u32* b) {
  return Vec{{i64(a[0]) - i64(b[0]), i64(a[1]) - i64(b[1]), i64(a[2]) - i64(b[2])}};
}
// Bornes de num/budgets.hpp (v11) : dot < 3M^2, cross < 2M^2, en i64.
MHGP12_HD i64 dot(const Vec& a, const Vec& b) { return a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2]; }
MHGP12_HD Vec cross(const Vec& a, const Vec& b) {
  return Vec{{a.v[1] * b.v[2] - a.v[2] * b.v[1], a.v[2] * b.v[0] - a.v[0] * b.v[2], a.v[0] * b.v[1] - a.v[1] * b.v[0]}};
}

// Centre a + N/D, D > 0, et certificats de sa fabrique (v11 : power_certificate, orientation_certificate).
struct Center {
  i128 n[3];
  i128 d;
  i64 anchor[3];
  u32 arity;
  u32 flags;  // bit 0 : certificat global de puissance q3 ; bit 1 : certificat global d'orientation
  MHGP12_HD bool q3_power() const { return (flags & 1u) != 0; }
  MHGP12_HD bool orient() const { return (flags & 2u) != 0; }
};

MHGP12_HD bool global_orientation(i128 d, const i128* n) {
  const i128 d_limit = i128(1) << (124 - 3 * kBits), n_limit = i128(1) << (124 - 2 * kBits);
  if (d <= 0 || d >= d_limit) return false;
  for (int j = 0; j < 3; ++j)
    if (n[j] <= -n_limit || n[j] >= n_limit) return false;
  return true;
}
MHGP12_HD bool q3_global_power(i128 d, const i128* n) {
  const i128 d_limit = i128(1) << (123 - 2 * kBits), n_limit = i128(1) << (124 - kBits);
  if (d <= 0 || d >= d_limit) return false;
  for (int j = 0; j < 3; ++j)
    if (n[j] <= -n_limit || n[j] >= n_limit) return false;
  return true;
}

// use_native_power de la v11 : 6B+8 <= 127, ou arite != 3, ou certificat q3.
MHGP12_HD bool native_power_ok(const Center& c) { return 6 * kBits + 8 <= 127 || c.arity != 3 || c.q3_power(); }

// Signe de D|z-a|^2 - 2N.(z-a) ; faux si la voie native n'est pas certifiee.
MHGP12_HD bool side(const Center& c, const u32* z, int& out) {
  if (!native_power_ok(c)) return false;
  const i64 v0 = i64(z[0]) - c.anchor[0], v1 = i64(z[1]) - c.anchor[1], v2 = i64(z[2]) - c.anchor[2];
  i128 total = c.d * i128(v0 * v0 + v1 * v1 + v2 * v2);
  total += c.n[0] * (-2 * i128(v0));
  total += c.n[1] * (-2 * i128(v1));
  total += c.n[2] * (-2 * i128(v2));
  out = sign(total);
  return true;
}

// Cote q2 en i64 (ajout v12). Presentation q2 (a, b) : D = 2, N = b - a, donc D|v|^2 - 2N.v = 2(|v|^2 - (b-a).v) avec
// v = z - a. Sous l'etendue de feuille <= 2^20 (z, a, b sites de la feuille), |v_j|, |b_j - a_j| <= 2^20, donc
// |v|^2 <= 3*2^40 et |(b-a).v| <= 3*2^40 : la difference tient en i64, de meme signe que side (facteur 2 > 0).
MHGP12_HD int side_q2_narrow(const u32* a, const u32* b, const u32* z) {
  const i64 v0 = i64(z[0]) - i64(a[0]), v1 = i64(z[1]) - i64(a[1]), v2 = i64(z[2]) - i64(a[2]);
  const i64 n0 = i64(b[0]) - i64(a[0]), n1 = i64(b[1]) - i64(a[1]), n2 = i64(b[2]) - i64(a[2]);
  const i64 value = v0 * v0 + v1 * v1 + v2 * v2 - (n0 * v0 + n1 * v1 + n2 * v2);
  return (value > 0) - (value < 0);
}

// Produit mixte de quatre points : < 6M^3, exact en i128.
MHGP12_HD int orientation4(const u32* a, const u32* b, const u32* c, const u32* d) {
  const Vec normal = cross(diff(b, a), diff(c, a)), w = diff(d, a);
  return sign(i128(normal.v[0]) * w.v[0] + i128(normal.v[1]) * w.v[1] + i128(normal.v[2]) * w.v[2]);
}

// center_orientation, voie native certifiee seulement.
MHGP12_HD bool center_orientation(const u32* a, const u32* b, const u32* c, const Center& s, int& out) {
  if (!s.orient()) return false;
  const Vec normal = cross(diff(b, a), diff(c, a));
  i128 total = 0;
  for (int j = 0; j < 3; ++j) {
    const i128 coordinate = s.n[j] + s.d * i128(s.anchor[j] - i64(a[j]));
    total += coordinate * normal.v[j];
  }
  out = sign(total);
  return true;
}

// center_inside : orientations des quatre faces, sommet oppose contre centre, dans l'ordre de la v11.
MHGP12_HD bool center_inside(const Center& s, const u32* const* p, bool& out) {
  for (int opposite = 0; opposite < 4; ++opposite) {
    const u32* face[3];
    int j = 0;
    for (int i = 0; i < 4; ++i)
      if (i != opposite) face[j++] = p[i];
    const int vertex = orientation4(face[0], face[1], face[2], p[opposite]);
    if (vertex == 0) {
      out = false;
      return true;
    }
    int center = 0;
    if (!center_orientation(face[0], face[1], face[2], s, center)) return false;
    if (center != vertex) {
      out = false;
      return true;
    }
  }
  out = true;
  return true;
}

// center_midpoint : 2(D a_j + N_j) == D (a_j + b_j), < 2^(5B+7) <= 2^127.
MHGP12_HD bool midpoint(const Center& s, const u32* a, const u32* b) {
  for (int j = 0; j < 3; ++j)
    if (2 * (s.d * s.anchor[j] + s.n[j]) != s.d * (i128(a[j]) + i128(b[j]))) return false;
  return true;
}

// Triangle strictement aigu par trois produits scalaires en a.
MHGP12_HD bool acute_dots(i64 uu, i64 vv, i64 uv) { return uv > 0 && uu > uv && vv > uv; }
MHGP12_HD bool strictly_acute(const u32* a, const u32* b, const u32* c) {
  const Vec u = diff(b, a), v = diff(c, a);
  return acute_dots(dot(u, u), dot(v, v), dot(u, v));
}

inline constexpr int kDegenerate = 0, kDisjoint = 1, kIntersects = 2;

// Lemme Z (num/center_region.cpp de la v11), arithmetique etroite du levier C sous etendue <= 2^20 : memes tests,
// meme ordre, memes issues ; bornes de la v11 (|f|, |g| <= D en i32, |p0|, |p1| <= 3D^2, left <= 6*2^60, right <= 2^62).
MHGP12_HD int center_line_meets(const u32* a, const u32* b, const u32* c, const i64* lo, const i64* hi) {
  i32 fu[3], gu[3];
  i64 p0 = 0, p1 = 0;
  for (int k = 0; k < 3; ++k) {
    const i32 ak = i32(a[k]), bk = i32(b[k]), ck = i32(c[k]), box = i32(lo[k] + hi[k]);
    fu[k] = ak - bk;
    gu[k] = ak - ck;
    p0 += i64(fu[k]) * (ak + bk - box);
    p1 += i64(gu[k]) * (ak + ck - box);
  }
  i64 cr[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  bool rank_two = false;
  for (int i = 0; i < 3; ++i)
    for (int j = i + 1; j < 3; ++j) {
      const i64 value = i64(gu[i]) * fu[j] - i64(fu[i]) * gu[j];
      cr[i][j] = cr[j][i] = value < 0 ? -value : value;
      rank_two = rank_two || value != 0;
    }
  if (!rank_two) return kDegenerate;
  for (int k = 0; k < 3; ++k) {
    if (fu[k] == 0 && gu[k] == 0) continue;
    const i64 signed_left = i64(gu[k]) * p0 - i64(fu[k]) * p1;
    const i64 left = signed_left < 0 ? -signed_left : signed_left;
    i64 right = 0;
    for (int j = 0; j < 3; ++j) right += (hi[j] - lo[j]) * cr[k][j];
    if (left > right) return kDisjoint;
  }
  return kIntersects;
}

// center_in_box de la v11 : lo <= a + N/D < hi par axe, N + D(a - borne) < 2^(5B+6).
MHGP12_HD bool center_in_box(const Center& s, const i64* lo, const i64* hi) {
  for (int j = 0; j < 3; ++j) {
    const i128 lower = s.n[j] + s.d * i128(s.anchor[j] - lo[j]);
    const i128 upper = s.n[j] + s.d * i128(s.anchor[j] - hi[j]);
    if (lower < 0 || upper >= 0) return false;
  }
  return true;
}

// Enveloppe fermee de points doubles contre la boite demi-ouverte doublee.
template <int N>
MHGP12_HD bool doubled_envelope_meets(const i64 (&pts)[N][3], const i64* lo, const i64* hi) {
  for (int axis = 0; axis < 3; ++axis) {
    i64 low = pts[0][axis], high = pts[0][axis];
    for (int i = 1; i < N; ++i) {
      low = pts[i][axis] < low ? pts[i][axis] : low;
      high = pts[i][axis] > high ? pts[i][axis] : high;
    }
    if (high < 2 * lo[axis] || low >= 2 * hi[axis]) return false;
  }
  return true;
}

// ----------------------------------------------------------------------------------------------- fabriques de centres
// Memes fabriques que Leaf::q2/q3/q4 de la v11 (leaf_device.hpp), sans census : rendent vrai si la presentation est
// un candidat dont le centre est dans la boite (le juge suivant est le census).

// q2 : centre (a + b) / 2. center_in_box(s) avec D = 2, N = b - a se reduit a 2lo <= a + b < 2hi (i64 exact).
MHGP12_HD bool q2_in_box(const u32* a, const u32* b, const i64* lo, const i64* hi) {
  for (int j = 0; j < 3; ++j) {
    const i64 s = i64(a[j]) + i64(b[j]);
    if (s < 2 * lo[j] || s >= 2 * hi[j]) return false;
  }
  return true;
}
MHGP12_HD Center q2_center(const u32* a, const u32* b) {
  Center s;
  for (int j = 0; j < 3; ++j) {
    s.anchor[j] = a[j];
    s.n[j] = i128(i64(b[j]) - i64(a[j]));
  }
  s.d = 2;
  s.arity = 2;
  s.flags = global_orientation(s.d, s.n) ? 2u : 0u;
  return s;
}

// q3 strictement aigu ; M3 ; centre ; proprietaire. Rend vrai si juge.
MHGP12_HD bool q3_candidate(const u32* a, const u32* b, const u32* cc, const i64* lo, const i64* hi, Center& s) {
  const Vec u = diff(b, a), v = diff(cc, a);
  const i64 uu = dot(u, u), vv = dot(v, v);
  if (!acute_dots(uu, vv, dot(u, v))) return false;
  const i64 mid[3][3] = {{i64(a[0]) + b[0], i64(a[1]) + b[1], i64(a[2]) + b[2]},
                         {i64(b[0]) + cc[0], i64(b[1]) + cc[1], i64(b[2]) + cc[2]},
                         {i64(a[0]) + cc[0], i64(a[1]) + cc[1], i64(a[2]) + cc[2]}};
  if (!doubled_envelope_meets(mid, lo, hi)) return false;  // lemme M3
  const Vec w = cross(u, v);
  const i128 g = i128(w.v[0]) * w.v[0] + i128(w.v[1]) * w.v[1] + i128(w.v[2]) * w.v[2];
  if (g == 0) return false;
  // Etendue D <= 2^20 : |t_j| <= 6 D^3 < 2^63 (i64 exact), comme la v11.
  i64 t[3];
  for (int j = 0; j < 3; ++j) t[j] = uu * v.v[j] - vv * u.v[j];
  for (int j = 0; j < 3; ++j) s.anchor[j] = a[j];
  s.n[0] = i128(t[1]) * w.v[2] - i128(t[2]) * w.v[1];
  s.n[1] = i128(t[2]) * w.v[0] - i128(t[0]) * w.v[2];
  s.n[2] = i128(t[0]) * w.v[1] - i128(t[1]) * w.v[0];
  s.d = 2 * g;
  s.arity = 3;
  s.flags = (q3_global_power(s.d, s.n) ? 1u : 0u) | (global_orientation(s.d, s.n) ? 2u : 0u);
  return center_in_box(s, lo, hi);
}

// q4 : orientation non nulle (comptee par q4_counted) ; E4 ; positivite stricte ; centre ; proprietaire.
MHGP12_HD bool q4_candidate(const u32* p0, const u32* p1, const u32* p2, const u32* p3, const i64* lo, const i64* hi,
                            Center& s, bool& q4_counted) {
  q4_counted = false;
  const Vec u = diff(p1, p0), v = diff(p2, p0), sv = diff(p3, p0);
  const Vec uv = cross(u, v);
  const i64 det = dot(uv, sv);  // etendue <= 2^20 : |det| <= 6 D^3 < 2^63
  if (det == 0) return false;
  q4_counted = true;
  const i64 twice[4][3] = {{2 * i64(p0[0]), 2 * i64(p0[1]), 2 * i64(p0[2])},
                           {2 * i64(p1[0]), 2 * i64(p1[1]), 2 * i64(p1[2])},
                           {2 * i64(p2[0]), 2 * i64(p2[1]), 2 * i64(p2[2])},
                           {2 * i64(p3[0]), 2 * i64(p3[1]), 2 * i64(p3[2])}};
  if (!doubled_envelope_meets(twice, lo, hi)) return false;  // lemme E4
  const Vec vs = cross(v, sv), su = cross(sv, u);
  const i64 uu = dot(u, u), vv = dot(v, v), ss = dot(sv, sv);
  i128 n[3];
  for (int j = 0; j < 3; ++j) n[j] = i128(uu) * vs.v[j] + i128(vv) * su.v[j] + i128(ss) * uv.v[j];
  const Vec face{{vs.v[0] + su.v[0] + uv.v[0], vs.v[1] + su.v[1] + uv.v[1], vs.v[2] + su.v[2] + uv.v[2]}};
  const i128 h = 2 * (i128(det) * det);
  bool strict = false;
  const i128 w0 = h - (n[0] * face.v[0] + n[1] * face.v[1] + n[2] * face.v[2]);
  if (w0 > 0) {
    const i128 w1 = n[0] * vs.v[0] + n[1] * vs.v[1] + n[2] * vs.v[2];
    if (w1 > 0) {
      const i128 w2 = n[0] * su.v[0] + n[1] * su.v[1] + n[2] * su.v[2];
      if (w2 > 0) strict = h - w0 - w1 - w2 > 0;
    }
  }
  if (!strict) return false;
  for (int j = 0; j < 3; ++j) s.anchor[j] = p0[j];
  i128 d = 2 * i128(det);
  if (d < 0) {
    d = -d;
    for (int j = 0; j < 3; ++j) n[j] = -n[j];
  }
  for (int j = 0; j < 3; ++j) s.n[j] = n[j];
  s.d = d;
  s.arity = 4;
  s.flags = global_orientation(s.d, s.n) ? 2u : 0u;
  return center_in_box(s, lo, hi);
}

}  // namespace mhgp12::leaf
