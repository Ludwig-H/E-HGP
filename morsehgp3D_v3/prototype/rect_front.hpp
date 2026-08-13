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

// ---- PORTEURS q3 : LA MOITIE QUE LE CERTIFICAT DE TEMOINS NE VOIT PAS.
//
// Les auditeurs l'etablissent (`AUDIT_DEBLOCAGE_WSPD_PREFIX_CARRIERS`, § 8) :
// compter des temoins ne prouve jamais l'ABSENCE d'un support. Un porteur q3
// propre, pour une paire owner `ab` de longueur carree `D2`, est un `x` tel que
//
//     ||x-a||^2 <= D2,   ||b-x||^2 <= D2,   D2 < ||x-a||^2 + ||b-x||^2.
//
// Les deux premieres sont FAIBLES — elles conservent les ties d'arete maximale
// —, la troisieme est STRICTE : c'est l'acuite du triangle. Or
// `D2 < E2 + X2` equivaut a `H < 0` : le porteur est donc STRICTEMENT HORS de
// la boule diametrale, la ou aucun temoin ne peut se trouver. Temoins et
// porteurs vivent dans des regions COMPLEMENTAIRES, et c'est pourquoi un
// certificat qui ne sait que fermer ne peut rien conclure sur l'existence.
//
// Releve au rectangle `A x B x C`, avec les bornes separables exactes :
//
//   ALL-porteur  : max||x-a||^2 <= min D2, max||b-x||^2 <= min D2,
//                  et max D2 < min(||x-a||^2 + ||b-x||^2) ;
//   NONE-porteur : min||x-a||^2 > max D2, OU min||b-x||^2 > max D2,
//                  OU min D2 >= max(||x-a||^2 + ||b-x||^2).
//
// `SOURCE_EMPTY` n'est autorise qu'apres partition EXHAUSTIVE du domaine
// temoin en nœuds `NONE-porteur` — jamais sur un echec de recherche.
enum class RectCarrier { kNone, kAll, kMixed };

// ---- LES MARGES, ET NON TROIS DISTANCES SEPAREES
// (audit `dba8961`, § 5). Avec `D = ||b-a||^2`, `E = ||x-a||^2`, `X = ||b-x||^2` :
//
//     M0 = E + X - D = -2H,    M1 = D - E,    M2 = D - X.
//
// Un porteur verifie EXACTEMENT `M0 > 0`, `M1 >= 0`, `M2 >= 0` — les deux
// inegalites de longueur faibles pour conserver les ties d'arete maximale, la
// premiere stricte parce que c'est l'acuite.
//
// L'apport est que `M1` est AFFINE en `a` et `M2` affine en `b` : leurs extrema
// sur les boites s'obtiennent donc en fixant l'extremite, puis en separant les
// variables restantes. Avec `near(u,I)` la distance carree de `u` a l'intervalle
// entier `I` — nulle si `u` y est — et `far(u,I)` la plus grande distance carree
// a ses deux extremites :
//
//     M1min_axe = min sur a dans {Alo,Ahi} de near(a,B) - far(a,C)
//     M1max_axe = max sur a dans {Alo,Ahi} de far(a,B)  - near(a,C)
//     M2min_axe = min sur b dans {Blo,Bhi} de near(b,A) - far(b,C)
//     M2max_axe = max sur b dans {Blo,Bhi} de far(b,A)  - near(b,C)
//
// Les extrema de `M0` sont `-2 Hmax` et `-2 Hmin`, deja disponibles.
//
// `ALL` est COMPLET sur le produit AABB ; `NONE` reste INCOMPLET, car deux
// contraintes peuvent echouer sur des points differents.
inline long long rect_near1(long long u, long long lo, long long hi) {
  const long long d = (u < lo) ? (lo - u) : ((u > hi) ? (u - hi) : 0);
  return d * d;
}
inline long long rect_far1(long long u, long long lo, long long hi) {
  const long long a = u - lo, b = u - hi;
  return std::max(a * a, b * b);
}

struct RectMargins { long long m0lo, m0hi, m1lo, m1hi, m2lo, m2hi; };

inline RectMargins rect_carrier_margins(const RectBox& a, const RectBox& b,
                                        const RectBox& c) {
  RectMargins r{};
  long long hmn = 0, hmx = 0;
  rect_h_interval(a, b, c, &hmn, &hmx);
  r.m0lo = -2 * hmx;
  r.m0hi = -2 * hmn;
  r.m1lo = 0; r.m1hi = 0; r.m2lo = 0; r.m2hi = 0;
  for (int i = 0; i < 3; ++i) {
    long long lo1 = 0, hi1 = 0, lo2 = 0, hi2 = 0;
    bool f1 = true, f2 = true;
    for (int e = 0; e < 2; ++e) {
      const long long av = e ? a.hi[i] : a.lo[i];
      const long long v1 = rect_near1(av, b.lo[i], b.hi[i]) - rect_far1(av, c.lo[i], c.hi[i]);
      const long long w1 = rect_far1(av, b.lo[i], b.hi[i]) - rect_near1(av, c.lo[i], c.hi[i]);
      if (f1) { lo1 = v1; hi1 = w1; f1 = false; }
      else { lo1 = std::min(lo1, v1); hi1 = std::max(hi1, w1); }
      const long long bv = e ? b.hi[i] : b.lo[i];
      const long long v2 = rect_near1(bv, a.lo[i], a.hi[i]) - rect_far1(bv, c.lo[i], c.hi[i]);
      const long long w2 = rect_far1(bv, a.lo[i], a.hi[i]) - rect_near1(bv, c.lo[i], c.hi[i]);
      if (f2) { lo2 = v2; hi2 = w2; f2 = false; }
      else { lo2 = std::min(lo2, v2); hi2 = std::max(hi2, w2); }
    }
    r.m1lo += lo1; r.m1hi += hi1; r.m2lo += lo2; r.m2hi += hi2;
  }
  return r;
}

inline RectCarrier rect_carrier_by_margins(const RectBox& a, const RectBox& b,
                                           const RectBox& c) {
  const RectMargins m = rect_carrier_margins(a, b, c);
  if (m.m0hi <= 0 || m.m1hi < 0 || m.m2hi < 0) return RectCarrier::kNone;
  if (m.m0lo > 0 && m.m1lo >= 0 && m.m2lo >= 0) return RectCarrier::kAll;
  return RectCarrier::kMixed;
}

inline RectCarrier rect_carrier_verdict(const RectBox& a, const RectBox& b,
                                        const RectBox& c) {
  const long long d2lo = rect_minsq(a, b), d2hi = rect_maxsq(a, b);
  const long long e2lo = rect_minsq(c, a), e2hi = rect_maxsq(c, a);
  const long long x2lo = rect_minsq(b, c), x2hi = rect_maxsq(b, c);
  // NONE : l'une des trois contraintes est impossible partout.
  if (e2lo > d2hi) return RectCarrier::kNone;
  if (x2lo > d2hi) return RectCarrier::kNone;
  if (d2lo >= e2hi + x2hi) return RectCarrier::kNone;   // aucun triangle aigu
  // ALL : les trois sont satisfaites partout.
  if (e2hi <= d2lo && x2hi <= d2lo && d2hi < e2lo + x2lo) return RectCarrier::kAll;
  return RectCarrier::kMixed;
}

enum class RectVerdict { kNone, kAll, kMixed };

// ---- `Central-VWave` : LE SCORE DU CERTIFICAT CENTRAL, ET SON INTERVALLE EXACT
// (audit `AUDIT_REPONSE_FOURCHE_SOURCE_CENTRAL_VWAVE_DBA8961`, § 4).
//
// Pour un terminal `R = A x B` et un point `z`, le score est
//
//     S_R(z) = somme_i max(|2z_i - Alo_i - Blo_i|, |2z_i - Ahi_i - Bhi_i|)^2,
//
// qui vaut EXACTEMENT `Vhi(A,B,{z})`. Le masque central s'ecrit alors
// `q2 : S < D`, `q3 : 3S < D`, `q4 : D > 0 et 209 S <= 56 D`, avec `D = Dlo(A,B)`.
//
// L'APPORT DECISIF est que ce score admet un intervalle EXACT sur un nœud
// temoin, donc un `NONE`. Le masque central seul n'en avait pas — il concluait
// `ALL` ou `UNKNOWN` — et c'est pourquoi il ne pouvait pas piloter une descente.
// En classifiant le score du certificat plutot que la geometrie sous-jacente, on
// recupere l'elagage.
//
// Par axe, avec `u = Alo + Blo`, `v = Ahi + Bhi` et `f(z) = max(|2z-u|,|2z-v|)^2` :
//   - `f` est un maximum de deux fonctions convexes, donc CONVEXE : son maximum
//     sur un intervalle est a une EXTREMITE ;
//   - son minimum est atteint la ou `|2z-u| = |2z-v|`, c'est-a-dire en
//     `z = (u+v)/4`, ECRETE a l'intervalle — donc en l'un des deux entiers
//     voisins.
// Les axes etant independants, les trois minima et maxima s'additionnent.
inline void rect_s_interval(const RectBox& a, const RectBox& b, const RectBox& c,
                            long long* out_min, long long* out_max) {
  long long smin = 0, smax = 0;
  for (int i = 0; i < 3; ++i) {
    const long long u = a.lo[i] + b.lo[i], v = a.hi[i] + b.hi[i];
    auto f = [&](long long z) {
      const long long p = 2 * z - u, q = 2 * z - v;
      const long long m = std::max(p < 0 ? -p : p, q < 0 ? -q : q);
      return m * m;
    };
    const long long fl = f(c.lo[i]), fh = f(c.hi[i]);
    smax += std::max(fl, fh);
    long long mn = std::min(fl, fh);
    // `4 z* = u + v`. La division entiere de C++ TRONQUE VERS ZERO, donc
    // `(u+v)/4` n'est pas le plancher des que la somme est negative — le juge
    // exhaustif a pris cette faute en flagrant delit, 848 desaccords sur 40 000.
    // On prend donc le vrai plancher, puis les DEUX entiers voisins.
    const long long s4 = u + v;
    const long long zf = (s4 >= 0) ? (s4 / 4) : -(((-s4) + 3) / 4);
    for (int t = 0; t < 2; ++t) {
      long long z = zf + t;
      z = std::max(c.lo[i], std::min(c.hi[i], z));
      mn = std::min(mn, f(z));
    }
    smin += mn;
  }
  *out_min = smin;
  *out_max = smax;
}

// Verdict de lane a partir de l'intervalle du score. `ALL` et `NONE` sont tous
// deux EXACTS pour le certificat central — c'est ce qui rend la vague possible.
inline RectVerdict rect_central_verdict(long long dlo, long long smin, long long smax,
                                        int lane) {
  const __int128 d = (__int128)dlo, lo = (__int128)smin, hi = (__int128)smax;
  if (lane == 0) {
    if (hi < d) return RectVerdict::kAll;
    if (lo >= d) return RectVerdict::kNone;
  } else if (lane == 1) {
    if (3 * hi < d) return RectVerdict::kAll;
    if (3 * lo >= d) return RectVerdict::kNone;
  } else {
    if (dlo <= 0) return RectVerdict::kNone;
    if (209 * hi <= 56 * d) return RectVerdict::kAll;
    if (209 * lo > 56 * d) return RectVerdict::kNone;
  }
  return RectVerdict::kMixed;
}

// ENUM FERME. L'ABI n'accepte plus un `int` quelconque : toute valeur autre que
// zero, un ou deux etait auparavant traitee silencieusement comme q4.
enum class RectLane { kQ2 = 0, kQ3 = 1, kQ4 = 2 };

inline long long rect_axis_near(long long z, long long lo, long long hi) {
  if (z < lo) return (lo - z) * (lo - z);
  if (z > hi) return (z - hi) * (z - hi);
  return 0;
}
inline long long rect_axis_far(long long z, long long lo, long long hi) {
  const long long a = (z - lo) * (z - lo), b = (z - hi) * (z - hi);
  return a > b ? a : b;
}

inline void rect_t_axis(long long alo, long long ahi, long long blo, long long bhi,
                           long long clo, long long chi, long long* lo, long long* hi) {
  long long cand[6];
  int nc = 0;
  auto add = [&](long long z) {
    if (z < clo) z = clo;
    if (z > chi) z = chi;
    for (int i = 0; i < nc; ++i) if (cand[i] == z) return;
    cand[nc++] = z;
  };
  // MINIMUM : ruptures de `dist(.,A)` et milieu de `B`.
  add(clo); add(chi); add(alo); add(ahi);
  const long long mb = blo + bhi;                 // milieu double, sans division
  add(mb >= 0 ? mb / 2 : (mb - 1) / 2);
  add(mb >= 0 ? (mb + 1) / 2 : mb / 2);
  *lo = rect_axis_near(cand[0], alo, ahi) - rect_axis_far(cand[0], blo, bhi);
  for (int i = 1; i < nc; ++i) {
    const long long v = rect_axis_near(cand[i], alo, ahi) - rect_axis_far(cand[i], blo, bhi);
    if (v < *lo) *lo = v;
  }
  // MAXIMUM : le role de `A` et `B` s'echange.
  nc = 0;
  add(clo); add(chi); add(blo); add(bhi);
  const long long ma = alo + ahi;
  add(ma >= 0 ? ma / 2 : (ma - 1) / 2);
  add(ma >= 0 ? (ma + 1) / 2 : ma / 2);
  *hi = rect_axis_far(cand[0], alo, ahi) - rect_axis_near(cand[0], blo, bhi);
  for (int i = 1; i < nc; ++i) {
    const long long v = rect_axis_far(cand[i], alo, ahi) - rect_axis_near(cand[i], blo, bhi);
    if (v > *hi) *hi = v;
  }
}


// ---- `JungSpindleRect-v0` : LE CŒUR ANISOTROPE, SUR UN RECTANGLE.
//
// Le certificat central teste `209 V2 <= 56 D2`, qui est la BOULE INSCRITE dans
// le vrai cœur : la reduction supprime le terme favorable `-4 (d.v)^2`, maximal
// SUR L'AXE de l'arete. Mesure par paire : sur `eight_clusters`, le cœur exact
// porte `5,6` fois plus de temoins et fait tomber la fraction de cœur vide de
// `13,8 %` a `0,9 %`.
//
// Le passage au rectangle demande un MINORANT de `(d.v)^2`, donc l'intervalle de
// `T = d.v`. L'identite qui debloque tout est `T = ||z-a||^2 - ||z-b||^2`, donc
// T est SEPARABLE PAR AXE — `a`, `b` et `z` choisissent leurs coordonnees
// independamment dans des boites alignees. `rect_t_axis` rend l'intervalle exact
// par axe ; tester les seuls coins serait FAUX.
//
// Quand l'intervalle traverse zero, le minorant sur est zero et l'on retombe sur
// la boule inscrite : fail-open, donc sain, jamais optimiste.
inline long long rect_t_abs(const RectBox& a, const RectBox& b, const RectBox& c) {
  long long lo = 0, hi = 0;
  for (int i = 0; i < 3; ++i) {
    long long l = 0, h = 0;
    rect_t_axis(a.lo[i], a.hi[i], b.lo[i], b.hi[i], c.lo[i], c.hi[i], &l, &h);
    lo += l; hi += h;
  }
  if (lo <= 0 && hi >= 0) return 0;                 // l'intervalle enjambe zero
  const long long al = lo < 0 ? -lo : lo, ah = hi < 0 ? -hi : hi;
  return al < ah ? al : ah;
}

// `ALL` seulement, jamais `NONE` : ce certificat ne refuse rien.
inline bool rect_spindle_all(long long dlo, long long dhi, long long vhi,
                             long long tabs, int lane) {
  if (dlo <= vhi) return false;
  const __int128 g = (__int128)(dlo - vhi) * (dlo - vhi);
  const __int128 r = (__int128)dhi * vhi - (__int128)tabs * tabs;
  if (lane == 2) return g > 2 * r;                  // q4
  if (lane == 1) return 3 * g > 4 * r;              // q3
  return dhi < dlo || vhi < dlo;                    // q2 : deja exact
}

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
// `Dlo` NE DEPEND PAS DU TEMOIN : il se calcule une fois par rectangle, et non
// une fois par candidat. C'est la specification de l'audit, et c'est le seul
// terme couteux — `rect_minsq` sur deux boites contre trois soustractions par
// axe pour `Vhi`.
inline unsigned rect_central_mask_dlo(long long dlo, const RectBox& a, const RectBox& b,
                                      const RectBox& c) {
  if (dlo <= 0) return 0;
  const __int128 d = (__int128)dlo;
  const __int128 vhi = (__int128)rect_v_max(a, b, c);
  unsigned m = 0;
  if (vhi < d) m |= 1u;
  if (3 * vhi < d) m |= 2u;
  if (209 * vhi <= 56 * d) m |= 4u;
  return m;
}

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
