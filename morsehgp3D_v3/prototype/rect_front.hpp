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
#include <cmath>
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
  kCoeurTropGrand,   // rayon du coeur commun double : `2(d2 - 2 s2)` au lieu de
                     // `d2 - 2 s2`. C'est la faute que j'avais ecrite, et le
                     // juge du coeur l'a prise en flagrant delit.
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

// ---- COEUR COMMUN (audit `96be8e0`, section 9.1). Un seul test de sphere
// ENTIERE, au lieu d'une descente. Avec `S = r_A + r_B`, `d = ||c_B - c_A||` et
// `m_0 = (c_A + c_B)/2`, tout milieu de paire est a distance au plus `S/2` de
// `m_0` et toute longueur `||b-a||` vaut au moins `d - S`. Donc
//
//     d > 2S  =>  B(m_0, (d-2S)/2)  incluse dans TOUTE boule diametrale.
//
// Dix `PointId` distincts dans ce coeur ferment q2 sans qu'aucun noeud ne soit
// classe. Tout est entier : coordonnees quadruplees pour `m_0`, racines
// entieres ARRONDIES DANS LE SENS CONSERVATEUR pour les rayons — jamais un
// coeur trop grand, donc jamais un faux temoin.
inline long long rect_isqrt_floor(long long v) {
  if (v <= 0) return 0;
  long long r = (long long)std::sqrt((double)v);
  while (r > 0 && r * r > v) --r;
  while ((r + 1) * (r + 1) <= v) ++r;
  return r;
}
inline long long rect_isqrt_ceil(long long v) {
  const long long r = rect_isqrt_floor(v);
  return (r * r == v) ? r : r + 1;
}

struct RectCore {
  bool valid;
  long long m4[3];      // quadruple du centre : (A.lo+A.hi) + (B.lo+B.hi)
  long long r4;         // minorant entier de quatre fois le rayon du coeur
};

// `mult` = 2 pour q2 (boule diametrale), 3 pour q3/q4 (circumboule admissible,
// sous precondition d'arete maximale owner). Le rayon du coeur vaut alors
// `(d - mult*S)/2` pour q2 et `(d - 3S)/4` pour q3/q4.
//
// DEUX COEURS IMBRIQUES, tous deux universels — la restriction a q2 que j'avais
// posee etait fondee sur un FAUX JUGE et l'audit `a7f061b` la leve :
//
//   - coeur LARGE, q2 seul  : rayon `(d-2S)/2` sous `d > 2S` ;
//   - coeur ETROIT, q2/q3/q4: rayon `(d-3S)/4` sous `d > 3S`.
//
// Preuve du coeur etroit, sans aucun owner. Avec `m` le milieu de `(a,b)`,
// `u = z-m`, `A = ||b-a||^2` et `B = ||u||^2`, on a `H = A/4 - B` et
// `E2 X2 = (A/4 + B)^2 - (d.u)^2`. Si `B <= A/16`, alors meme en SUPPRIMANT le
// terme favorable `(d.u)^2`, la marge q4 au bord vaut
//
//     3 (1/4 - 1/16)^2 - (1/4 + 1/16)^2 = 27/256 - 25/256 = 1/128 > 0.
//
// Le coeur est donc STRICTEMENT dans le spindle q4, donc q3, donc q2. Et pour
// des noeuds, `||z-m0|| < (d-3S)/4` implique `||z-m|| < (d-S)/4 <= ||b-a||/4`.
//
// L'owner reste obligatoire pour la CONSOMMATION : neuf ou huit temoins de
// spindle eliminent une candidature q3 ou q4 seulement sous
// `owner = max_edge_canonical`, et l'issue s'appelle alors
// `PRUNED_MAX_EDGE_ANCHOR` — jamais « aucune sphere ne contient la paire ».
inline RectCore rect_common_core(const RectBox& a, const RectBox& b, int mult,
                                RectFrontInject inject = RectFrontInject::kNone) {
  RectCore c{};
  c.valid = false;
  long long wa2 = 0, wb2 = 0, d2x4 = 0;
  for (int i = 0; i < 3; ++i) {
    const long long da = a.hi[i] - a.lo[i], db = b.hi[i] - b.lo[i];
    wa2 += da * da;
    wb2 += db * db;
    const long long u = (b.lo[i] + b.hi[i]) - (a.lo[i] + a.hi[i]);   // 2*(c_B-c_A)
    d2x4 += u * u;
    c.m4[i] = (a.lo[i] + a.hi[i]) + (b.lo[i] + b.hi[i]);             // 4*m_0
  }
  const long long wa = rect_isqrt_ceil(wa2);   // majorant de 2 r_A
  const long long wb = rect_isqrt_ceil(wb2);
  const long long d2 = rect_isqrt_floor(d2x4); // minorant de 2 d
  // 2S <= wa + wb, donc `d > mult*S` est implique par `2d > mult*(wa+wb)`.
  const long long s2 = wa + wb;
  if (d2 <= mult * s2) return c;
  // q2 : `4 rho = 2d - 4S`. Avec `d2 <= 2d` et `2 s2 >= 4S`, le minorant est
  // `d2 - 2 s2` — et NON `2(d2 - 2 s2)`, qui etait deux fois trop grand et que
  // le juge du coeur a pris en flagrant delit (`30 862` points hors de la
  // region ALL exacte sur `terrain`).
  // q3/q4 : `4 rho = d - 3S >= (d2 - 3 s2)/2`.
  c.r4 = (mult == 2) ? (d2 - 2 * s2) : ((d2 - 3 * s2) / 2);
  if (inject == RectFrontInject::kCoeurTropGrand) c.r4 *= 2;
  if (c.r4 <= 0) return c;
  c.valid = true;
  return c;
}

// `z` (coordonnees simples) est-il STRICTEMENT dans le coeur ?
inline bool rect_core_contains(const RectCore& c, const long long z[3]) {
  __int128 s = 0;
  for (int i = 0; i < 3; ++i) {
    const __int128 u = (__int128)4 * z[i] - c.m4[i];
    s += u * u;
  }
  return s < (__int128)c.r4 * c.r4;
}

// La boite entiere est-elle DISJOINTE du coeur ? Sert a elaguer la descente.
inline bool rect_core_misses_box(const RectCore& c, const RectBox& p) {
  __int128 s = 0;
  for (int i = 0; i < 3; ++i) {
    const long long lo4 = 4 * p.lo[i], hi4 = 4 * p.hi[i];
    long long d = 0;
    if (c.m4[i] < lo4) d = lo4 - c.m4[i];
    else if (c.m4[i] > hi4) d = c.m4[i] - hi4;
    s += (__int128)d * d;
  }
  return s >= (__int128)c.r4 * c.r4;
}

enum class RectVerdict { kNone, kAll, kMixed };

// ENUM FERME. L'ABI n'accepte plus un `int` quelconque : toute valeur autre que
// zero, un ou deux etait auparavant traitee silencieusement comme q4.
enum class RectLane { kQ2 = 0, kQ3 = 1, kQ4 = 2 };

// ---- COEUR CENTRAL ENTIER, PARTAGE PAR LES TROIS LANES
// (audit `AUDIT_DEBLOCAGE_WSPD_PREFIX_CARRIERS`, section 5).
//
// Avec `d = b-a`, `v = 2z-a-b`, `D2 = ||d||^2`, `V2 = ||v||^2`, les identites
// exactes sont
//
//     4H = D2 - V2      et      16 E2 X2 = (D2+V2)^2 - 4 (d.v)^2.
//
// En SUPPRIMANT le terme negatif `-4(d.v)^2`, la condition universelle
// `c H^2 > E2 X2` se reduit a une comparaison du seul rapport `V2/D2` :
//
//     q2 : V2 < D2                 (c'est exactement `H > 0`)
//     q3 : 3 V2 < D2               (car apres SUPPRESSION du terme favorable,
//                                   4H^2 > E2X2 est IMPLIQUE par
//                                   2(D2-V2) > D2+V2 — une implication, pas
//                                   une equivalence)
//     q4 : 209 V2 <= 56 D2         (il faut V2/D2 < 2 - sqrt(3) = 0,2679491...,
//                                   et 56/209 = 0,2679425... est dessous)
//
// Le dernier test est donc STRICTEMENT interieur a la vraie frontiere : il perd
// une frange infime et ne peut jamais la franchir. Tous les bits sont imbriques
// et se calculent en UNE classification, sans produit vectoriel et sans
// `Lambda`, pour les trois lanes a la fois. Un echec reste `UNKNOWN` : ces
// tests ne rendent JAMAIS `NONE` ni un support positif.
//
// `Dlo` est le minimum exact de `||b-a||^2` entre les AABB `A` et `B`.
// `Vhi` est le maximum exact de `||2z-a-b||^2` : par axe, `2z-a-b` parcourt
// `[2 Clo - Ahi - Bhi, 2 Chi - Alo - Blo]`, et le carre y culmine a l'extremite
// de plus grande valeur absolue.
inline long long rect_v_max(const RectBox& a, const RectBox& b, const RectBox& c) {
  long long s = 0;
  for (int i = 0; i < 3; ++i) {
    const long long lo = 2 * c.lo[i] - a.hi[i] - b.hi[i];
    const long long hi = 2 * c.hi[i] - a.lo[i] - b.lo[i];
    const long long m = std::max(lo < 0 ? -lo : lo, hi < 0 ? -hi : hi);
    s += m * m;
  }
  return s;
}

// LES TROIS LANES EN UN SEUL CALCUL. `Dlo` et `Vhi` ne dependent pas de la
// lane : les calculer une fois puis comparer trois seuils remplace trois appels
// au classifieur. Le contre-audit `33df59d` releve que `rect_classify` etait
// appele une fois par bit de lane ouvert, si bien que le nombre d'appels
// arithmetiques reels approchait le TRIPLE du compteur publie.
// Bit 0 = q2, bit 1 = q3, bit 2 = q4.
inline unsigned rect_central_mask(const RectBox& a, const RectBox& b, const RectBox& c) {
  const __int128 dlo = (__int128)rect_minsq(a, b);
  if (dlo <= 0) return 0;
  const __int128 vhi = (__int128)rect_v_max(a, b, c);
  unsigned m = 0;
  if (vhi < dlo) m |= 1u;
  if (3 * vhi < dlo) m |= 2u;
  if (209 * vhi <= 56 * dlo) m |= 4u;
  return m;
}

// `ALL` par le coeur central. Ce sont des IMPLICATIONS fail-open, jamais des
// equivalences : un echec rend `UNKNOWN`, jamais `NONE`. `56/209 < 2-sqrt(3)`
// est exact — `362^2 - 3 x 209^2 = 1` —, donc l'egalite rationnelle reste
// strictement a l'interieur de la vraie frontiere.
//
// LE GARDE `dlo > 0` EST OBLIGATOIRE EN q4 : sans lui, `A=B=C={0}` donne
// `209 x 0 <= 56 x 0` et le helper accepte un rectangle degenere.
inline bool rect_central_all(const RectBox& a, const RectBox& b, const RectBox& c,
                             RectLane lane) {
  const __int128 vhi = (__int128)rect_v_max(a, b, c);
  const __int128 dlo = (__int128)rect_minsq(a, b);
  if (dlo <= 0) return false;
  if (lane == RectLane::kQ2) return vhi < dlo;
  if (lane == RectLane::kQ3) return 3 * vhi < dlo;
  return 209 * vhi <= 56 * dlo;
}

// Coherence exigee : le masque et le helper par lane doivent coincider.
inline bool rect_central_mask_agrees(const RectBox& a, const RectBox& b, const RectBox& c) {
  const unsigned m = rect_central_mask(a, b, c);
  for (int q = 0; q < 3; ++q)
    if (((m >> q) & 1u) != (rect_central_all(a, b, c, (RectLane)q) ? 1u : 0u)) return false;
  return true;
}


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
  // COEUR CENTRAL D'ABORD : il decide les trois lanes en une comparaison de
  // rapport et ne demande ni `Lambda` ni produit vectoriel.
  if (rect_central_all(a, b, c, lane)) return RectVerdict::kAll;
  if (mn <= 0) return RectVerdict::kMixed;
  if (lane == RectLane::kQ2) return RectVerdict::kAll;
  // Repli : le certificat par `Hmin` et les deux maxima de distance. Il est
  // moins couvrant que le coeur central sur les rectangles centres, mais peut
  // mordre ailleurs ; les deux sont suffisants, jamais complets.
  const __int128 hh = (__int128)mn * mn;
  const __int128 ex = (__int128)rect_maxsq(c, a) * (__int128)rect_maxsq(b, c);
  const __int128 k = (lane == RectLane::kQ3) ? 4 : 3;
  return (k * hh > ex) ? RectVerdict::kAll : RectVerdict::kMixed;
}

}  // namespace mhgp3v
