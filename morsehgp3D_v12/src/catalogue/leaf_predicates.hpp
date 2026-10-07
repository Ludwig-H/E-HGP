// Predicats exacts de la feuille J3, en repere local, ecrits une fois pour les deux politiques de leaf_arith.hpp.
// Port explicite de microbancs/mes_m2_feuille/include/mhgp12/leaf/predicates.hpp (MES-M2), lui-meme port de
// src/catalogue/leaf_device_predicates.hpp de la v11 (ac081a06f) : memes formules, memes tests, meme ordre, memes
// issues. Ce qui change : (1) les coordonnees sont locales (site - coin minimal du repere de la feuille) et les
// bornes sont celles de l'etendue s, non du domaine B ; (2) les types sont ceux de la politique (Narrow : natifs,
// s <= 16, aucun certificat necessaire ; Exact : larges, toute etendue) ; (3) les certificats globaux de la v11 et le
// rejet << non certifie >> disparaissent de la voie etroite (palier etroit natif par construction) ; un predicat large
// ne signale qu'un depassement, impossible par les bornes (defense, rendue en invariant) ; (4) le test du milieu est
// en forme locale (CST-0114). Aucun flottant.
#pragma once

#include "catalogue/leaf_arith.hpp"

namespace mhgp12::catalogue_detail {

struct Vec {
  i64 v[3];
};
MHGP12_HD Vec diff(const u32* a, const u32* b) {
  return Vec{{i64(a[0]) - i64(b[0]), i64(a[1]) - i64(b[1]), i64(a[2]) - i64(b[2])}};
}
template <class A>
struct SVec {
  typename A::Small v[3];
};
template <class A>
MHGP12_HD typename A::Small dot(const Vec& a, const Vec& b) {
  using S = typename A::Small;
  return S(a.v[0]) * S(b.v[0]) + S(a.v[1]) * S(b.v[1]) + S(a.v[2]) * S(b.v[2]);
}
template <class A>
MHGP12_HD SVec<A> cross(const Vec& a, const Vec& b) {
  using S = typename A::Small;
  return SVec<A>{{S(a.v[1]) * S(b.v[2]) - S(a.v[2]) * S(b.v[1]), S(a.v[2]) * S(b.v[0]) - S(a.v[0]) * S(b.v[2]),
                  S(a.v[0]) * S(b.v[1]) - S(a.v[1]) * S(b.v[0])}};
}
template <class A>
MHGP12_HD typename A::Small sdot(const SVec<A>& a, const Vec& b) {
  using S = typename A::Small;
  return a.v[0] * S(b.v[0]) + a.v[1] * S(b.v[1]) + a.v[2] * S(b.v[2]);
}

// Centre a + N/D (D > 0), ancre = un site de la presentation, en coordonnees locales.
template <class A>
struct Center {
  typename A::Large n[3];
  typename A::Large d;
  i64 anchor[3];
  u32 arity;
};

// Issue d'un juge a valeurs larges : faute = depassement de la politique exacte (impossible par les bornes).
enum Verdict : u32 { kNo = 0, kYes = 1, kFault = 2 };

// Signe de D|z-a|^2 - 2N.(z-a) : 6s+8 bits (104 a s = 16). Faux si la valeur large deborde.
template <class A>
MHGP12_HD bool side(const Center<A>& c, const u32* z, int& out) {
  using S = typename A::Small;
  using L = typename A::Large;
  const S v0 = S(i64(z[0]) - c.anchor[0]), v1 = S(i64(z[1]) - c.anchor[1]), v2 = S(i64(z[2]) - c.anchor[2]);
  L total = c.d * L(v0 * v0 + v1 * v1 + v2 * v2);
  total = total + c.n[0] * L(S(-2) * v0);
  total = total + c.n[1] * L(S(-2) * v1);
  total = total + c.n[2] * L(S(-2) * v2);
  out = sign_of(total);
  return healthy(total);
}

// Cote q2 sans centre : presentation (a, b), D = 2, N = b - a, donc 2(|v|^2 - (b-a).v) avec v = z - a ; meme signe
// que side (facteur 2 > 0). |v|^2 et |(b-a).v| < 3 M^2.
template <class A>
MHGP12_HD int side_q2(const u32* a, const u32* b, const u32* z) {
  const Vec v = diff(z, a), n = diff(b, a);
  return sign_of(dot<A>(v, v) - dot<A>(n, v));
}

// Produit mixte de quatre points : < 6 M^3, exact en i128 a toute etendue.
template <class A>
MHGP12_HD int orientation4(const u32* a, const u32* b, const u32* c, const u32* d) {
  const SVec<A> normal = cross<A>(diff(b, a), diff(c, a));
  const Vec w = diff(d, a);
  return sign_of(i128(normal.v[0]) * w.v[0] + i128(normal.v[1]) * w.v[1] + i128(normal.v[2]) * w.v[2]);
}

// Orientation du centre par rapport au plan (a, b, c) : 7s+9 bits (121 a s = 16).
template <class A>
MHGP12_HD bool center_orientation(const u32* a, const u32* b, const u32* c, const Center<A>& s, int& out) {
  using L = typename A::Large;
  const SVec<A> normal = cross<A>(diff(b, a), diff(c, a));
  L total = L(i64{0});
  for (int j = 0; j < 3; ++j) {
    const L coordinate = s.n[j] + s.d * L(s.anchor[j] - i64(a[j]));
    total = total + coordinate * L(normal.v[j]);
  }
  out = sign_of(total);
  return healthy(total);
}

// Centre strictement interieur au tetraedre : orientations des quatre faces, sommet oppose contre centre, dans
// l'ordre de la v11. Faux seulement sur depassement.
template <class A>
MHGP12_HD bool center_inside(const Center<A>& s, const u32* const* p, bool& out) {
  for (int opposite = 0; opposite < 4; ++opposite) {
    const u32* face[3];
    int j = 0;
    for (int i = 0; i < 4; ++i)
      if (i != opposite) face[j++] = p[i];
    const int vertex = orientation4<A>(face[0], face[1], face[2], p[opposite]);
    if (vertex == 0) {
      out = false;
      return true;
    }
    int center = 0;
    if (!center_orientation<A>(face[0], face[1], face[2], s, center)) return false;
    if (center != vertex) {
      out = false;
      return true;
    }
  }
  out = true;
  return true;
}

// Test du milieu en forme locale (CST-0114) : 2(D a_j + N_j) == D (a_j + b_j), coordonnees locales.
template <class A>
MHGP12_HD Verdict midpoint(const Center<A>& s, const u32* a, const u32* b) {
  using L = typename A::Large;
  for (int j = 0; j < 3; ++j) {
    const L left = L(i64{2}) * (s.d * L(s.anchor[j]) + s.n[j]);
    const L right = s.d * L(i64(a[j]) + i64(b[j]));
    const L gap = left - right;
    if (!healthy(gap)) return kFault;
    if (sign_of(gap) != 0) return kNo;
  }
  return kYes;
}

// Triangle strictement aigu par trois produits scalaires en a.
template <class A>
MHGP12_HD bool acute_dots(typename A::Small uu, typename A::Small vv, typename A::Small uv) {
  return sign_of(uv) > 0 && sign_of(uu - uv) > 0 && sign_of(vv - uv) > 0;
}
template <class A>
MHGP12_HD bool strictly_acute(const u32* a, const u32* b, const u32* c) {
  const Vec u = diff(b, a), v = diff(c, a);
  return acute_dots<A>(dot<A>(u, u), dot<A>(v, v), dot<A>(u, v));
}

inline constexpr int kDegenerate = 0, kDisjoint = 1, kIntersects = 2;

// Lemme Z (LEM-ZONO) : droite des centres equidistants de (a, b, c) contre la fermeture de la boite ; memes tests,
// meme ordre, memes issues que la v11 (num/center_region.cpp). |f|,|g| < M, |p0|,|p1| < 3 M^2, gauche et droite
// < 6 M^3.
template <class A>
MHGP12_HD int center_line_meets(const u32* a, const u32* b, const u32* c, const i64* lo, const i64* hi) {
  using T = typename A::Tiny;
  using S = typename A::Small;
  T fu[3], gu[3];
  S p0 = 0, p1 = 0;
  for (int k = 0; k < 3; ++k) {
    const T ak = T(a[k]), bk = T(b[k]), ck = T(c[k]), box = T(lo[k] + hi[k]);
    fu[k] = ak - bk;
    gu[k] = ak - ck;
    p0 += S(fu[k]) * S(ak + bk - box);
    p1 += S(gu[k]) * S(ak + ck - box);
  }
  S cr[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  bool rank_two = false;
  for (int i = 0; i < 3; ++i)
    for (int j = i + 1; j < 3; ++j) {
      const S value = S(gu[i]) * S(fu[j]) - S(fu[i]) * S(gu[j]);
      cr[i][j] = cr[j][i] = value < 0 ? -value : value;
      rank_two = rank_two || value != 0;
    }
  if (!rank_two) return kDegenerate;
  for (int k = 0; k < 3; ++k) {
    if (fu[k] == 0 && gu[k] == 0) continue;
    const S signed_left = S(gu[k]) * p0 - S(fu[k]) * p1;
    const S left = signed_left < 0 ? -signed_left : signed_left;
    S right = 0;
    for (int j = 0; j < 3; ++j) right += S(hi[j] - lo[j]) * cr[k][j];
    if (left > right) return kDisjoint;
  }
  return kIntersects;
}

// Centre dans la boite demi-ouverte : lo <= a + N/D < hi par axe, N + D(a - borne).
template <class A>
MHGP12_HD Verdict center_in_box(const Center<A>& s, const i64* lo, const i64* hi) {
  using L = typename A::Large;
  for (int j = 0; j < 3; ++j) {
    const L lower = s.n[j] + s.d * L(s.anchor[j] - lo[j]);
    const L upper = s.n[j] + s.d * L(s.anchor[j] - hi[j]);
    if (!healthy(lower) || !healthy(upper)) return kFault;
    if (sign_of(lower) < 0 || sign_of(upper) >= 0) return kNo;
  }
  return kYes;
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
// Memes fabriques que Leaf::q2/q3/q4 de la v11 (leaf_device.hpp), sans census : kYes si la presentation est un
// candidat dont le centre est dans la boite (le juge suivant est le census).

// q2 : centre (a + b) / 2 ; la propriete se reduit a 2 lo <= a + b < 2 hi.
MHGP12_HD bool q2_in_box(const u32* a, const u32* b, const i64* lo, const i64* hi) {
  for (int j = 0; j < 3; ++j) {
    const i64 s = i64(a[j]) + i64(b[j]);
    if (s < 2 * lo[j] || s >= 2 * hi[j]) return false;
  }
  return true;
}
template <class A>
MHGP12_HD Center<A> q2_center(const u32* a, const u32* b) {
  using L = typename A::Large;
  Center<A> s;
  for (int j = 0; j < 3; ++j) {
    s.anchor[j] = a[j];
    s.n[j] = L(i64(b[j]) - i64(a[j]));
  }
  s.d = L(i64{2});
  s.arity = 2;
  return s;
}

// q3 strictement aigu ; enveloppe des milieux (lemme M3) ; centre ; proprietaire.
template <class A>
MHGP12_HD Verdict q3_candidate(const u32* a, const u32* b, const u32* cc, const i64* lo, const i64* hi, Center<A>& s) {
  using S = typename A::Small;
  using L = typename A::Large;
  const Vec u = diff(b, a), v = diff(cc, a);
  const S uu = dot<A>(u, u), vv = dot<A>(v, v);
  if (!acute_dots<A>(uu, vv, dot<A>(u, v))) return kNo;
  const i64 mid[3][3] = {{i64(a[0]) + b[0], i64(a[1]) + b[1], i64(a[2]) + b[2]},
                         {i64(b[0]) + cc[0], i64(b[1]) + cc[1], i64(b[2]) + cc[2]},
                         {i64(a[0]) + cc[0], i64(a[1]) + cc[1], i64(a[2]) + cc[2]}};
  if (!doubled_envelope_meets(mid, lo, hi)) return kNo;  // lemme M3
  const SVec<A> w = cross<A>(u, v);
  const L g = L(w.v[0]) * L(w.v[0]) + L(w.v[1]) * L(w.v[1]) + L(w.v[2]) * L(w.v[2]);
  if (sign_of(g) == 0) return kNo;
  S t[3];
  for (int j = 0; j < 3; ++j) t[j] = uu * S(v.v[j]) - vv * S(u.v[j]);
  for (int j = 0; j < 3; ++j) s.anchor[j] = a[j];
  s.n[0] = L(t[1]) * L(w.v[2]) - L(t[2]) * L(w.v[1]);
  s.n[1] = L(t[2]) * L(w.v[0]) - L(t[0]) * L(w.v[2]);
  s.n[2] = L(t[0]) * L(w.v[1]) - L(t[1]) * L(w.v[0]);
  s.d = L(i64{2}) * g;
  s.arity = 3;
  return center_in_box<A>(s, lo, hi);
}

// Poids de presentation strictement positifs du q4 (meme ordre que la v11) ; faute sur depassement.
template <class A>
MHGP12_HD Verdict q4_strict(const typename A::Large (&n)[3], const SVec<A>& vs, const SVec<A>& su,
                            const SVec<A>& uv, const typename A::Large& h) {
  using L = typename A::Large;
  const L w0 = h - (n[0] * L(vs.v[0] + su.v[0] + uv.v[0]) + n[1] * L(vs.v[1] + su.v[1] + uv.v[1]) +
                    n[2] * L(vs.v[2] + su.v[2] + uv.v[2]));
  if (!healthy(w0)) return kFault;
  if (sign_of(w0) <= 0) return kNo;
  const L w1 = n[0] * L(vs.v[0]) + n[1] * L(vs.v[1]) + n[2] * L(vs.v[2]);
  if (!healthy(w1)) return kFault;
  if (sign_of(w1) <= 0) return kNo;
  const L w2 = n[0] * L(su.v[0]) + n[1] * L(su.v[1]) + n[2] * L(su.v[2]);
  if (!healthy(w2)) return kFault;
  if (sign_of(w2) <= 0) return kNo;
  const L w3 = h - w0 - w1 - w2;
  if (!healthy(w3)) return kFault;
  return sign_of(w3) > 0 ? kYes : kNo;
}

// q4 : orientation non nulle (comptee par q4_counted) ; enveloppe des sommets (lemme E4) ; positivite stricte ;
// centre ; proprietaire.
template <class A>
MHGP12_HD Verdict q4_candidate(const u32* p0, const u32* p1, const u32* p2, const u32* p3, const i64* lo,
                               const i64* hi, Center<A>& s, bool& q4_counted) {
  using S = typename A::Small;
  using L = typename A::Large;
  q4_counted = false;
  const Vec u = diff(p1, p0), v = diff(p2, p0), sv = diff(p3, p0);
  const SVec<A> uv = cross<A>(u, v);
  const S det = sdot<A>(uv, sv);
  if (sign_of(det) == 0) return kNo;
  q4_counted = true;
  const i64 twice[4][3] = {{2 * i64(p0[0]), 2 * i64(p0[1]), 2 * i64(p0[2])},
                           {2 * i64(p1[0]), 2 * i64(p1[1]), 2 * i64(p1[2])},
                           {2 * i64(p2[0]), 2 * i64(p2[1]), 2 * i64(p2[2])},
                           {2 * i64(p3[0]), 2 * i64(p3[1]), 2 * i64(p3[2])}};
  if (!doubled_envelope_meets(twice, lo, hi)) return kNo;  // lemme E4
  const SVec<A> vs = cross<A>(v, sv), su = cross<A>(sv, u);
  const S uu = dot<A>(u, u), vv = dot<A>(v, v), ss = dot<A>(sv, sv);
  L n[3];
  for (int j = 0; j < 3; ++j) n[j] = L(uu) * L(vs.v[j]) + L(vv) * L(su.v[j]) + L(ss) * L(uv.v[j]);
  const L h = L(i64{2}) * (L(det) * L(det));
  const Verdict strict = q4_strict<A>(n, vs, su, uv, h);
  if (strict != kYes) return strict;
  for (int j = 0; j < 3; ++j) s.anchor[j] = p0[j];
  L d = L(i64{2}) * L(det);
  if (sign_of(d) < 0) {
    d = -d;
    for (int j = 0; j < 3; ++j) n[j] = -n[j];
  }
  for (int j = 0; j < 3; ++j) s.n[j] = n[j];
  s.d = d;
  s.arity = 4;
  return center_in_box<A>(s, lo, hi);
}

}  // namespace mhgp12::catalogue_detail
