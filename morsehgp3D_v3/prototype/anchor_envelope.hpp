// MorseHGP3D v3 — SOURCE PAR ANCRE MAXIMALE ET ENVELOPPE MOBILE.
//
// Specification : audits/NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_ENVELOPPE_20260812.md
//
// L'objet produit est Source S : tous les supports propres positifs pertinents
// d'arite 2, 3, 4 (miniboule portant tout le support sur sa frontiere, centre
// dans l'interieur relatif de l'enveloppe convexe, et p + q <= smax).
//
// Le principe est une reduction EXACTE, pas une heuristique de voisinage :
//   - tout support possede une arete de longueur maximale (Lemme A) ;
//   - le centre vit alors dans le disque de Jung de cette arete ;
//   - la boule de milieu B(m, D/sqrt(15)) est incluse dans TOUTE boule
//     admissible (Lemme B), donc huit PointId dedans tuent la lane q4 ;
//   - le cone de demi-angle 27,368 degres tronque a D/2 est inclus dans le
//     meme spindle (Lemme C), ce qui donne un rayon de coupure certifie par
//     (point, chambre) — jamais une borne de densite ;
//   - les deux carriers d'un q4 pertinent sont dans la lentille
//     B(a,D) inter B(b,D), et tous les interieurs sont a distance au plus
//     1,2248 D de a.
// L'emission se fait depuis la SEULE arete maximale canonique : chaque support
// est donc emis exactement une fois, sans RLE, sans lift et sans owner de
// cellule.
//
// TOUTE DECISION EST ENTIERE. Les flottants n'apparaissent que pour ordonner
// des taches, jamais pour decider une appartenance, une positivite ou un rang.
#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "mhgp/mhgp.hpp"

// MHGP_HD vient de mhgp/exact.hpp : host seul en C++, host+device sous nvcc.
// Tous les predicats ci-dessous sont donc compiles a l'identique des deux
// cotes, ce qui rend le differentiel hote/device une porte et non un espoir.

namespace mhgp3v {
namespace anchor {

using mhgp::i128;
using mhgp::i64;
using mhgp::P3;

// ---------------------------------------------------------------------------
// Vecteur a composantes i128. Les numerateurs de circumcentre q3 atteignent
// 1,5e25 : ils ne tiennent pas en i64 (audit de bornes en tete de fichier).
// ---------------------------------------------------------------------------
struct V3 {
  i128 x = 0, y = 0, z = 0;
};

MHGP_HD inline i128 dot_i64(const P3& a, const P3& b) {
  return i128(a.x) * b.x + i128(a.y) * b.y + i128(a.z) * b.z;
}
MHGP_HD inline i128 norm2_i64(const P3& a) { return dot_i64(a, a); }
MHGP_HD inline P3 sub(const P3& a, const P3& b) { return P3{a.x - b.x, a.y - b.y, a.z - b.z}; }
MHGP_HD inline P3 cross_i64(const P3& a, const P3& b) {
  return P3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
MHGP_HD inline i128 dot_mixed(const P3& a, const V3& b) {
  return i128(a.x) * b.x + i128(a.y) * b.y + i128(a.z) * b.z;
}

// ---------------------------------------------------------------------------
// Boule support en forme exacte, relative a l'origine `a` du support :
//   centre = a + Y / Delta,  Delta > 0.
// Le signe de puissance d'un site z est
//   sigma(z) = Delta * |z-a|^2 - 2 (z-a) . Y
// negatif = strictement interieur, nul = shell, positif = exterieur.
// Bornes u16 : q3 -> Delta <= 4,5e20, |Y| <= 1,5e25, sigma <= 5,8e30 ;
//              q4 -> Delta <= 3,4e15, |Y| <= 3,4e20, sigma <= 1,4e26.
// Tout tient strictement dans i128 (1,7e38).
// ---------------------------------------------------------------------------
struct BallForm {
  V3 num{};        // Y
  i128 den = 0;    // Delta > 0
  bool valid = false;
};

MHGP_HD inline i128 power_sign_value(const BallForm& ball, const P3& origin, const P3& z) {
  const P3 r = sub(z, origin);
  return ball.den * norm2_i64(r) - 2 * dot_mixed(r, ball.num);
}

// Circumcentre du triangle (a,b,x), exprime relativement a `a`.
// c - a = ( |u|^2 (v x N) + |v|^2 (N x u) ) / (2 |N|^2), N = u x v.
MHGP_HD inline BallForm circum_q3(const P3& a, const P3& b, const P3& x) {
  BallForm out{};
  const P3 u = sub(b, a);
  const P3 v = sub(x, a);
  const P3 n = cross_i64(u, v);
  const i128 n2 = norm2_i64(n);
  if (n2 == 0) return out;  // colineaire : aucun cercle circonscrit
  const P3 vn = cross_i64(v, n);
  const P3 nu = cross_i64(n, u);
  const i128 u2 = norm2_i64(u);
  const i128 v2 = norm2_i64(v);
  out.num.x = u2 * vn.x + v2 * nu.x;
  out.num.y = u2 * vn.y + v2 * nu.y;
  out.num.z = u2 * vn.z + v2 * nu.z;
  out.den = 2 * n2;
  out.valid = true;
  return out;
}

// Circumcentre du tetraedre (a,b,x,y), relatif a `a`. Le denominateur est
// normalise strictement positif ; le centre est inchange.
MHGP_HD inline BallForm circum_q4(const P3& a, const P3& b, const P3& x, const P3& y) {
  BallForm out{};
  const P3 u = sub(b, a);
  const P3 v = sub(x, a);
  const P3 w = sub(y, a);
  const i128 det = dot_i64(u, cross_i64(v, w));
  if (det == 0) return out;  // coplanaire
  const P3 vw = cross_i64(v, w);
  const P3 wu = cross_i64(w, u);
  const P3 uv = cross_i64(u, v);
  const i128 u2 = norm2_i64(u);
  const i128 v2 = norm2_i64(v);
  const i128 w2 = norm2_i64(w);
  out.num.x = u2 * vw.x + v2 * wu.x + w2 * uv.x;
  out.num.y = u2 * vw.y + v2 * wu.y + w2 * uv.y;
  out.num.z = u2 * vw.z + v2 * wu.z + w2 * uv.z;
  out.den = 2 * det;
  if (out.den < 0) {
    out.den = -out.den;
    out.num.x = -out.num.x;
    out.num.y = -out.num.y;
    out.num.z = -out.num.z;
  }
  out.valid = true;
  return out;
}

// ---------------------------------------------------------------------------
// Positivite : le centre appartient a l'interieur relatif de conv(S).
// ---------------------------------------------------------------------------
// q2 : toujours vrai des que a != b (le centre est le milieu).
// q3 : le circumcentre est dans l'interieur du triangle ssi les trois angles
//      sont strictement aigus. Un angle droit place le centre sur une arete :
//      la miniboule est alors la boule diametrale, le support propre est q2 et
//      le troisieme point n'est qu'un membre de shell.
MHGP_HD inline bool positive_q3(const P3& a, const P3& b, const P3& x) {
  return dot_i64(sub(b, a), sub(x, a)) > 0 && dot_i64(sub(a, b), sub(x, b)) > 0 &&
         dot_i64(sub(a, x), sub(b, x)) > 0;
}

// q4 : le circumcentre est strictement du meme cote que le sommet oppose pour
// les quatre faces. Avec c = a + Y/Delta et Delta > 0, le test sur la face
// (p,q,r) de sommet oppose s s'ecrit sur Delta*(c - p) = Delta*(a - p) + Y.
MHGP_HD inline bool positive_q4(const P3& s0, const P3& s1, const P3& s2, const P3& s3,
                        const BallForm& ball, const P3& origin, bool loose = false) {
  const P3* v[4] = {&s0, &s1, &s2, &s3};
  for (int f = 0; f < 4; ++f) {
    const P3& p = *v[(f + 1) & 3];
    const P3& q = *v[(f + 2) & 3];
    const P3& r = *v[(f + 3) & 3];
    const P3& s = *v[f];
    const P3 e1 = sub(q, p);
    const P3 e2 = sub(r, p);
    const P3 nrm = cross_i64(e1, e2);  // |nrm| <= 8,6e9
    // cote du sommet oppose : signe de nrm . (s - p), borne 3*8,6e9*65535
    const i128 side_vertex = dot_i64(nrm, sub(s, p));
    if (side_vertex == 0) return false;  // degenere : tetraedre plat
    // cote du centre : Delta*(a-p) + Y, borne 4,5e20 ; produit borne 1,2e31
    const P3 ap = sub(origin, p);
    const i128 cx = ball.den * ap.x + ball.num.x;
    const i128 cy = ball.den * ap.y + ball.num.y;
    const i128 cz = ball.den * ap.z + ball.num.z;
    const i128 side_center = i128(nrm.x) * cx + i128(nrm.y) * cy + i128(nrm.z) * cz;
    if (side_center == 0) { if (!loose) return false; continue; }  // centre sur la face
    if ((side_center > 0) != (side_vertex > 0)) return false;  // mauvais cote
  }
  return true;
}

// ---------------------------------------------------------------------------
// Lanes de l'ancre : boules de milieu universelles (Lemme B).
// u = 2x - a - b ; D2 = |b-a|^2.
//   q2 : u.u < D2          (boule diametrale)
//   q3 : 3 (u.u) < D2      (rayon D/sqrt(12))
//   q4 : 15 (u.u) < 4 D2   (rayon D/sqrt(15))
// Bornes i64 : u.u <= 5,2e10 ; 4*D2*15 <= 7,8e11.
// ---------------------------------------------------------------------------
inline constexpr int kThresholdQ2 = 10;  // dix interieurs tuent la lane q2
inline constexpr int kThresholdQ3 = 9;
inline constexpr int kThresholdQ4 = 8;

struct LaneMask {
  bool q2 = false, q3 = false, q4 = false;
  bool any() const { return q2 || q3 || q4; }
};

// ---------------------------------------------------------------------------
// Support emis.
// ---------------------------------------------------------------------------
struct Support {
  int id[4] = {-1, -1, -1, -1};  // trie croissant, q premiers valides
  int q = 0;
  int p = 0;          // |I_B|
  int shell_extra = 0;  // |U_B| - q
};

MHGP_HD inline void sort_ids(int* v, int q) {
  for (int i = 1; i < q; ++i)
    for (int j = i; j > 0 && v[j] < v[j - 1]; --j) {
      const int t = v[j];
      v[j] = v[j - 1];
      v[j - 1] = t;
    }
}

// Cle canonique : quatre indices denses u16 tries croissants, du poids fort au
// poids faible, les slots manquants remplis par la sentinelle 0xFFFF. Comme
// n <= 65535, cette sentinelle n'est jamais un identifiant valide et l'arite se
// relit sans champ supplementaire. La cle n'est JAMAIS une autorite
// geometrique : elle sert au tri, a la deduplication et aux digests.
MHGP_HD inline unsigned long long support_key(const Support& s) {
  unsigned long long k = 0;
  for (int i = 0; i < 4; ++i) {
    const unsigned long long v = (i < s.q) ? (unsigned long long)(unsigned)s.id[i] : 0xFFFFULL;
    k |= v << (16 * (3 - i));
  }
  return k;
}

// ---------------------------------------------------------------------------
// Racine carree entiere exacte d'un i128 positif. L'estimation flottante n'est
// qu'un point de depart : les deux boucles de correction rendent le resultat
// exact, et c'est lui seul qui decide.
// ---------------------------------------------------------------------------
MHGP_HD inline i64 isqrt_i128(i128 v) {
  if (v <= 0) return 0;
#if defined(__CUDA_ARCH__)
  i64 r = (i64)sqrt((double)v);
#else
  i64 r = (i64)__builtin_sqrt((double)v);
#endif
  if (r < 0) r = 0;
  while (r > 0 && i128(r) * r > v) --r;
  while (i128(r + 1) * (r + 1) <= v) ++r;
  return r;
}

// ---------------------------------------------------------------------------
// ENVELOPPE MOBILE (Theoreme D de la note de solution).
//
// Pour l'ancre (a,b) : d = b-a, D2 = d.d ; pour un site z, U = 2z-a-b,
// g = D2 - U.U et la marge affine sur le plan mediateur est F(w) = g + 2 U.w.
// Sur le disque de Jung q4 (w.d = 0, |w|^2 <= D2/2), l'amplitude vaut
// exactement sqrt(2 Q) avec
//     Q = (U.U) * D2 - (U.d)^2  >= 0   (Cauchy-Schwarz).
// On borne donc L = g - sqrt(2Q) par en dessous et U* = g + sqrt(2Q) par au
// dessus avec la racine entiere, ce qui rend le filtre ENTIER et fail-open :
//     Llow = g - isqrt(2Q) - 1 <= L,     Uhigh = g + isqrt(2Q) + 1 >= U*.
// Un site est ecarte seulement si Uhigh < theta, ou theta est la neuvieme plus
// grande valeur de Llow parmi X \ {a,b}. Aucun interieur d'un support pertinent
// n'est alors ecarte (preuve dans la note).
// ---------------------------------------------------------------------------
struct SiteMargin {
  int id = 0;
  i64 d2a = 0;    // |z-a|^2
  i64 g = 0;      // D2 - |U|^2 : signe de l'appartenance a la boule diametrale
  i64 llow = 0;
  i64 uhigh = 0;
};

MHGP_HD inline void site_margin(const P3& pa, const P3& pb, const P3& pz, i64 d2, SiteMargin* out) {
  const P3 d = sub(pb, pa);
  const P3 u = P3{2 * pz.x - pa.x - pb.x, 2 * pz.y - pa.y - pb.y, 2 * pz.z - pa.z - pb.z};
  const i128 uu = norm2_i64(u);
  const i128 ud = dot_i64(u, d);
  out->g = (i64)(i128(d2) - uu);
  const i128 q = uu * i128(d2) - ud * ud;
  const i64 s = isqrt_i128(2 * q) + 1;
  out->llow = out->g - s;
  out->uhigh = out->g + s;
}

}  // namespace anchor
}  // namespace mhgp3v
