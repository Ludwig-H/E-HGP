// MorseHGP3D v3 — LE FRONT DE RECTANGLES ET SON INTERVALLE EXACT.
//
// Objet : decider une lane pour un RECTANGLE `A x B` de noeuds d'arbre, et non
// pour une paire. La quantite portante est
//
//     H(a,b,z) = sum_i (z_i - a_i)(b_i - z_i),
//
// strictement positive si et seulement si `z` est interieur a la boule
// diametrale de `(a,b)`. Sur des BOITES PRODUIT elle admet un intervalle
// EXACT, coordonnee par coordonnee, et ses deux bornes portent les deux
// classifications d'un noeud temoin `C` :
//
//     Lambda_min(A,B,C) >  0  -> ALL   : tout point de `C` est temoin de toute
//                                        paire de `A x B` ; crediter |C|.
//     Lambda_max(A,B,C) <= 0  -> NONE  : aucun point de `C` n'est temoin
//                                        d'aucune paire ; retirer DEFINITIVEMENT.
//     sinon                      MIXED : raffiner.
//
// PREUVE (rectifiee par l'audit `AUDIT_REPONSE_WSPD_DESCENTE_JOINTE_96BE8E0`).
// Sur une coordonnee, `f(a,b,z) = (z-a)(b-z)` est AFFINE en `a` a `b` fixe, et
// AFFINE en `b` a `a` fixe. On peut donc remplacer successivement `a` puis `b`
// par une extremite de son intervalle sans perdre ni le minimum ni le maximum.
// Puis, `a` et `b` etant fixes, `f` est une parabole CONCAVE en `z` :
//   - son MINIMUM sur un intervalle est a une extremite ;
//   - son MAXIMUM sur le RESEAU ENTIER est atteint en un entier voisin de
//     `(a+b)/2`, ECRETE a `C`.
// Les trois axes sont independants et les AABB sont des produits cartesiens :
// les trois extrema scalaires s'additionnent.
//
// L'argument « minimum de bilineaires concave, maximum convexe » que j'avais
// d'abord ecrit est FAUX s'il est lu conjointement en `(a,b)` : a `z=0` la
// forme vaut `-ab`, dont la hessienne est indefinie. La conclusion etait bonne,
// la preuve ne l'etait pas. C'est l'affinite SEPAREE qui vaut.
//
// PORTEE EXACTE DE `Lambda_max` : c'est l'enveloppe du RESEAU ENTIER, nommee
// `integer_lattice_u16_aabb_envelope`, et NON l'enveloppe continue. Fixture :
// `A={0}`, `B={1}`, `C=[0,1]` donne un maximum entier NUL et un maximum continu
// egal a `1/4`. `Lambda_min`, lui, coincide avec le minimum continu, puisque le
// minimum en `z` reste aux extremites.
//
// N'evaluer que les extremites en `z` SOUS-ESTIME le maximum et fait prononcer
// NONE la ou des temoins existent : c'est le faux NONE que j'avais ecrit.
//
// ALL n'a besoin d'AUCUN test d'exclusion de `a` et `b` : un point strictement
// interieur a sa propre boule diametrale n'existe pas, donc `Lambda_min > 0`
// implique deja `z != a` et `z != b`.
//
// Lanes q3/q4 : l'identite de Lagrange donne
// `R = ||(b-a) x (z-a)||^2 = E2 X2 - H^2` avec `E2 = ||z-a||^2` et
// `X2 = ||b-z||^2`, donc la condition de temoin universel « `c H^2 > R` »
// devient « `(c+1) H^2 > E2 X2` », soit `4H^2 > E2X2` en q3 et `3H^2 > E2X2`
// en q4. `E2` et `X2` etant des sommes de carres par coordonnee, leurs maxima
// sur les boites sont separables. Le certificat de noeud q3/q4 est donc
// `Lambda_min > 0` et `c' Lambda_min^2 > E2max X2max`, quarante-huit produits
// entiers, SANS produit vectoriel ni enumeration de coins. C'est un MAJORANT
// (`E2` et `X2` peuvent culminer en des `z` differents) : fail-open, jamais un
// faux credit.
#pragma once

#include <algorithm>
#include <cstdint>

namespace mhgp3v {

struct RectBox {
  long long lo[3];
  long long hi[3];
};

// Injections destinees a etre TUEES par les portes. Elles ne sont pas des
// options de calcul : chacune modelise une faute reelle que j'ai ecrite.
enum class RectFrontInject {
  kNone,
  kMaxParCoins,   // le faux NONE : maximum evalue aux seules extremites en z
  kSommetNonEcrete,  // sommet utilise SANS ecretage a la boite de C : le
                     // maximum devient ((b-a)/2)^2 meme quand le sommet est
                     // hors de C, donc surestime — NONE ne se declenche plus.
};

// Intervalle EXACT de `H` sur `A x B x C`.
inline void rect_h_interval(const RectBox& a, const RectBox& b, const RectBox& c,
                            long long* out_min, long long* out_max,
                            RectFrontInject inject = RectFrontInject::kNone) {
  long long mn = 0, mx = 0;
  for (int i = 0; i < 3; ++i) {
    long long lo = 0, hi = 0;
    bool first = true;
    for (int ea = 0; ea < 2; ++ea) {
      for (int eb = 0; eb < 2; ++eb) {
        const long long av = ea ? a.hi[i] : a.lo[i];
        const long long bv = eb ? b.hi[i] : b.lo[i];
        for (int g = 0; g < 2; ++g) {
          const long long z = g ? c.hi[i] : c.lo[i];
          const long long v = (z - av) * (bv - z);
          if (first) { lo = hi = v; first = false; }
          else { lo = std::min(lo, v); hi = std::max(hi, v); }
        }
        if (inject == RectFrontInject::kMaxParCoins) continue;  // faute modelisee
        const long long s = av + bv;
        for (int t = 0; t < 2; ++t) {
          long long z = (s + t) / 2;
          if (inject != RectFrontInject::kSommetNonEcrete)
            z = std::max(c.lo[i], std::min(c.hi[i], z));
          hi = std::max(hi, (z - av) * (bv - z));
        }
      }
    }
    mn += lo;
    mx += hi;
  }
  *out_min = mn;
  *out_max = mx;
}

// Maximum EXACT de `||p - q||^2` sur deux boites : separable par coordonnee.
inline long long rect_maxsq(const RectBox& p, const RectBox& q) {
  long long s = 0;
  for (int i = 0; i < 3; ++i) {
    long long m = 0;
    for (int ep = 0; ep < 2; ++ep)
      for (int eq = 0; eq < 2; ++eq) {
        const long long d = (ep ? p.hi[i] : p.lo[i]) - (eq ? q.hi[i] : q.lo[i]);
        m = std::max(m, d * d);
      }
    s += m;
  }
  return s;
}

// Minimum EXACT de `||p - q||^2` sur deux boites : separable par coordonnee.
// Nul des que les projections se recouvrent sur l'axe.
inline long long rect_minsq(const RectBox& p, const RectBox& q) {
  long long s = 0;
  for (int i = 0; i < 3; ++i) {
    long long d = 0;
    if (p.lo[i] > q.hi[i]) d = p.lo[i] - q.hi[i];
    else if (q.lo[i] > p.hi[i]) d = q.lo[i] - p.hi[i];
    s += d * d;
  }
  return s;
}

enum class RectVerdict { kNone, kAll, kMixed };

// ENUM FERME. L'ABI n'accepte plus un `int` quelconque : toute valeur autre que
// zero, un ou deux etait auparavant traitee silencieusement comme q4.
enum class RectLane { kQ2 = 0, kQ3 = 1, kQ4 = 2 };

inline RectVerdict rect_classify(const RectBox& a, const RectBox& b, const RectBox& c,
                                 RectLane lane, long long* out_max,
                                 RectFrontInject inject = RectFrontInject::kNone) {
  long long mn = 0, mx = 0;
  rect_h_interval(a, b, c, &mn, &mx, inject);
  *out_max = mx;
  if (mx <= 0) return RectVerdict::kNone;   // NONE q2, donc NONE de toute lane
  if (lane != RectLane::kQ2) {
    // NONE SPECIFIQUE DE LANE (audit `96be8e0`, section 3). Avec `U = max(Hmax,0)`
    // et `LE`, `LX` les minima exacts des distances carrees `A-C` et `B-C`, la
    // condition universelle `c' H^2 > E2 X2` ne peut etre satisfaite nulle part
    // si `c' U^2 <= LE LX`. L'egalite rend bien NONE : les spindles sont ouverts.
    const __int128 u = (__int128)std::max(mx, 0LL);
    const __int128 le = (__int128)rect_minsq(a, c);
    const __int128 lx = (__int128)rect_minsq(b, c);
    const __int128 kn = (lane == RectLane::kQ3) ? 4 : 3;
    if (kn * u * u <= le * lx) return RectVerdict::kNone;
  }
  if (mn <= 0) return RectVerdict::kMixed;
  if (lane == RectLane::kQ2) return RectVerdict::kAll;
  const __int128 hh = (__int128)mn * mn;
  const __int128 ex = (__int128)rect_maxsq(c, a) * (__int128)rect_maxsq(b, c);
  const __int128 k = (lane == RectLane::kQ3) ? 4 : 3;
  return (k * hh > ex) ? RectVerdict::kAll : RectVerdict::kMixed;
}

}  // namespace mhgp3v
