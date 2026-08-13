// MorseHGP3D v3 — CREDITS CELLULAIRES : LE GROUPE SANS ENUMERATION DE TRIPLES.
//
// Specification : audits/AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md, section
// « Deblocage mathematique prioritaire : credits cellulaires sans triples ».
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// POURQUOI LES TRIPLES ETAIENT LE MAUVAIS OBJET
//
// Le theoreme de groupe vaut pour un ensemble fini `G` de taille QUELCONQUE :
// si la direction cible appartient a son cone positif et si chaque membre
// satisfait strictement la puissance (H2), alors `G` fournit un interieur a
// toute sphere admissible. Caratheodory borne a trois un sous-groupe pour une
// direction FIXEE ; il n'impose nullement d'enumerer ces sous-groupes pour
// couvrir une cellule entiere. Mon premier sujet formait `C(m,3)` triples : il
// payait une combinatoire que la geometrie ne demande pas.
//
// ---------------------------------------------------------------------------
// L'EVENEMENT D'ACTIVATION, EXACT ET ENTIER
//
// Soit une cellule simpliciale `C = cone(r_0,r_1,r_2)` dont les trois rayons
// sont sur la section de hauteur `T`. Pour un site relatif `s = z-a`, poser
//
//     m_C(s) = min_{0<=j<3} r_j . s.
//
// Toute direction `d` de `C` s'ecrit `d = somme beta_j r_j` avec `beta_j >= 0`,
// et la hauteur etant LINEAIRE sur la chambre, `tau(d) = T somme beta_j`. Donc
//
//     d . s = somme beta_j (r_j . s) >= (somme beta_j) m_C(s) = tau(d) m_C(s)/T.
//
// Si `m_C(s) > 0`, la condition (H2) `d.s > ||s||^2` est donc acquise sur TOUT
// le suffixe de hauteur des que
//
//     tau(d) m_C(s) > T ||s||^2,
//
// c'est-a-dire des que `tau(d) >= X_s = floor(T ||s||^2 / m_C(s)) + 1`.
//
// `X_s` est l'evenement d'activation du site pour la cellule. Il est exact,
// entier, et ne depend pas de la cible : c'est ce qui rend la decision
// DECIDABLE PAR INTERVALLE DE HAUTEUR, comme le cutoff de dominance.
//
// ---------------------------------------------------------------------------
// LE CREDIT
//
// Un credit de la cellule `C` est un sous-ensemble `G` du pool actif tel que
// `C` soit incluse dans `cone(G)`, ce qui equivaut a ce que les TROIS rayons
// `r_j` y appartiennent. Son seuil est `X_G = max_{s in G} X_s`. Pour toute
// cible `b` de `C` avec `tau(b-a) >= X_G`, les deux hypotheses du theoreme sont
// reunies, donc `G` fournit un interieur.
//
// Des credits DEUX A DEUX DISJOINTS donnent des interieurs distincts. Les
// trier par `X_G` croissant et lire le `h`-ieme donne le seuil de fermeture de
// la lane qui exige `h` temoins. Un echec du glouton reste `UNKNOWN` : aucun
// packing maximal n'est requis pour la surete.
//
// ---------------------------------------------------------------------------
// AUCUNE DIVISION DANS L'AUTORITE
//
// `r_j` appartient a `cone(G)` si et seulement si, dans la carte projective
// `w.u = 1` avec `w = r_0+r_1+r_2`, le projete de `r_j` est dans l'enveloppe
// convexe des projetes de `G`. Les denominateurs `w.s` y sont strictement
// positifs sur le pool actif, donc toute orientation projective a le SIGNE du
// determinant `det(s_i, s_j, s_k)`. L'enveloppe se calcule donc par marche de
// Jarvis avec ce seul predicat entier.
//
// Largeurs u16 : `|r_j| <= 3*sqrt(3) < 6`, `|s| <= 65535*sqrt(3)`, donc
// `|r_j . s| <= 3*3*65535 = 5,9e5`, `T ||s||^2 <= 3*3*65535^2 = 3,9e10`, et
// `det(s_i,s_j,s_k) <= 6*65535^3 = 1,7e15`. Tout tient dans `i64`.
#pragma once

#include <cstddef>
#include <cstdint>

#include "mhgp/mhgp.hpp"
#include "prototype/directional_dominance.hpp"

namespace mhgp3v {
namespace credits {

using mhgp::i64;
using mhgp::P3;

inline constexpr int kRayHeight = 3;  // les rayons (3,i,j) sont a hauteur trois

enum class CreditMutant {
  kNone,
  kActivationOff,    // oublie le `+1` : la frontiere d'activation entre a tort
  kOneRayOnly,       // ne fait passer qu'un rayon au lieu des trois
  kShareIds,         // autorise deux credits a partager un `PointId`
  kIgnorePositive,   // n'exige pas `m_C(s) > 0`
};

inline const char* credit_mutant_name(CreditMutant m) {
  switch (m) {
    case CreditMutant::kNone: return "none";
    case CreditMutant::kActivationOff: return "credit-activation-frontiere";
    case CreditMutant::kOneRayOnly: return "credit-un-seul-rayon";
    case CreditMutant::kShareIds: return "credit-ids-partages";
    case CreditMutant::kIgnorePositive: return "credit-sans-positivite";
  }
  return "?";
}

// Les trois rayons canoniques de chaque sous-cone, dans l'ordre des neuf
// identifiants. Ce sont les sommets des triangles `U_ij` et `D_ij`.
inline constexpr int kCanonicalRays[dominance::kSubcones][3][3] = {
    {{3, 0, 0}, {3, 1, 0}, {3, 1, 1}},  // U00
    {{3, 1, 0}, {3, 2, 0}, {3, 2, 1}},  // U10
    {{3, 1, 0}, {3, 1, 1}, {3, 2, 1}},  // D10
    {{3, 1, 1}, {3, 2, 1}, {3, 2, 2}},  // U11
    {{3, 2, 0}, {3, 3, 0}, {3, 3, 1}},  // U20
    {{3, 2, 0}, {3, 2, 1}, {3, 3, 1}},  // D20
    {{3, 2, 1}, {3, 3, 1}, {3, 3, 2}},  // U21
    {{3, 2, 1}, {3, 2, 2}, {3, 3, 2}},  // D21
    {{3, 2, 2}, {3, 3, 2}, {3, 3, 3}},  // U22
};

// Ramene les trois rayons d'une cellule dans le repere d'origine. La chambre
// vaut `pi*8 + sgn` : `pi` indexe la permutation qui trie les magnitudes en
// ordre decroissant, `sgn` porte un bit par composante negative. Un vecteur
// canonique `v` se relit donc `orig[q[k]] = +/- v[k]`.
inline void cell_rays(int cell, i64 out[3][3]) {
  const int chamber = cell / dominance::kSubcones;
  const int sub = cell % dominance::kSubcones;
  const int pi = chamber / 8, sgn = chamber % 8;
  const int* q = dominance::kPerm[pi];
  for (int j = 0; j < 3; ++j)
    for (int k = 0; k < 3; ++k) {
      const i64 v = kCanonicalRays[sub][j][k];
      out[j][q[k]] = ((sgn >> q[k]) & 1) ? -v : v;
    }
}

inline i64 dot3(const i64 a[3], const i64 b[3]) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline i64 det3(const i64 a[3], const i64 b[3], const i64 c[3]) {
  return a[0] * (b[1] * c[2] - b[2] * c[1]) - b[0] * (a[1] * c[2] - a[2] * c[1]) +
         c[0] * (a[1] * b[2] - a[2] * b[1]);
}

// `m_C(s)` : le minimum des trois produits scalaires avec les rayons.
inline i64 cell_margin(const i64 rays[3][3], const i64 s[3]) {
  i64 m = dot3(rays[0], s);
  for (int j = 1; j < 3; ++j) {
    const i64 v = dot3(rays[j], s);
    if (v < m) m = v;
  }
  return m;
}

// EVENEMENT D'ACTIVATION EXACT. Rend `-1` lorsque le site ne s'active jamais.
// Le mutant oublie le `+1` : a `tau(d) = floor(T||s||^2/m)` l'inegalite (H2)
// est une EGALITE au mieux, donc la puissance peut etre nulle et le site sur la
// sphere plutot que dedans.
inline i64 activation_height(const i64 rays[3][3], const i64 s[3],
                             CreditMutant mu = CreditMutant::kNone) {
  const i64 m = cell_margin(rays, s);
  if (m <= 0) return (mu == CreditMutant::kIgnorePositive) ? 1 : -1;
  const i64 num = (i64)kRayHeight * dot3(s, s);
  const i64 q = num / m;
  return (mu == CreditMutant::kActivationOff) ? q : q + 1;
}

// ---------------------------------------------------------------------------
// ENVELOPPE PROJECTIVE, SANS DIVISION NI FLOTTANT.
//
// Le test `r appartient a cone(G)` par enumeration de Caratheodory coute
// `O(m^3)` et interdit en pratique le pool de trente ou plus qu'il faut pour
// atteindre huit credits disjoints. La formulation projective l'evite.
//
// Tous les membres du pool verifient `m_C(s) > 0`, donc `w . s > 0` avec
// `w = r_0+r_1+r_2` : ils vivent tous dans le demi-espace ouvert `w > 0` et se
// projettent sur la carte `w . u = 1`. Dans cette carte, `r` appartient au cone
// du pool si et seulement si son projete est dans l'ENVELOPPE CONVEXE des
// projetes du pool.
//
// Les denominateurs `w . s` etant strictement positifs, l'orientation
// projective de trois vecteurs a le SIGNE de `det(a,b,c)`. La marche de Jarvis
// n'a besoin que de ce predicat entier — aucune division, aucune racine, aucun
// flottant — et coute `O(m h)`.
//
// Largeurs u16 : `|det(s_i,s_j,s_k)| <= 6*65535^3 = 1,7e15`, et le comparateur
// de pivot croise `(s.e)(w.s')` avec `|s.e| <= 3,2e7` et `|w.s| <= 1,8e6`, soit
// `5,7e13`. Tout tient dans `i64`.
// ---------------------------------------------------------------------------

// Enveloppe convexe projective du pool, rendue dans l'ordre cyclique. `hull`
// recoit des indices de `avail`. Rend la taille de l'enveloppe.
inline int projective_hull(const i64* pool /*3 par membre*/, const int* avail, int m,
                           const i64 w[3], int* hull, long long* orient_tests) {
  if (m <= 0) return 0;
  // Une direction du plan `w`, pour choisir un pivot extreme de facon exacte.
  int axis = 0;
  for (int k = 1; k < 3; ++k)
    if ((w[k] < 0 ? -w[k] : w[k]) < (w[axis] < 0 ? -w[axis] : w[axis])) axis = k;
  i64 ax[3] = {0, 0, 0};
  ax[axis] = 1;
  const i64 e[3] = {w[1] * ax[2] - w[2] * ax[1], w[2] * ax[0] - w[0] * ax[2],
                    w[0] * ax[1] - w[1] * ax[0]};

  auto sptr = [&](int i) { return pool + (std::size_t)avail[i] * 3; };
  int pivot = 0;
  for (int i = 1; i < m; ++i) {
    const i64* a = sptr(pivot);
    const i64* b = sptr(i);
    // Comparer `(a.e)/(w.a)` a `(b.e)/(w.b)`, denominateurs positifs.
    const i64 lhs = dot3(b, e) * dot3(w, a);
    const i64 rhs = dot3(a, e) * dot3(w, b);
    if (lhs < rhs) pivot = i;
  }

  int count = 0;
  int cur = pivot;
  for (;;) {
    hull[count++] = cur;
    int next = (cur + 1) % m;
    for (int i = 0; i < m; ++i) {
      if (i == cur) continue;
      ++*orient_tests;
      const i64 d = det3(sptr(cur), sptr(next), sptr(i));
      if (d < 0) {
        next = i;
      } else if (d == 0) {
        // COLINEAIRES DANS LA CARTE. Il faut garder le plus ELOIGNE de `cur` le
        // long de l'arete, sinon la marche saute un sommet et l'enveloppe
        // exclut une region : la porte d'equivalence l'a effectivement refute,
        // `enveloppe=0` contre `brute=1` sur un pool de cinq membres.
        //
        // Une premiere version comparait la coordonnee `e` de la carte, ce qui
        // n'est correct que si l'arete n'est pas parallele a l'autre axe. Le
        // test exact est CONIQUE et sans division : `i` est plus loin que
        // `next` si et seulement si `next` appartient au cone de `cur` et `i`.
        // Dans le plan de normale `n = cur x i`, cela s'ecrit avec deux
        // determinants.
        const i64* pc = sptr(cur);
        const i64* pi = sptr(i);
        const i64 nrm[3] = {pc[1] * pi[2] - pc[2] * pi[1], pc[2] * pi[0] - pc[0] * pi[2],
                            pc[0] * pi[1] - pc[1] * pi[0]};
        if (nrm[0] != 0 || nrm[1] != 0 || nrm[2] != 0) {
          const i64 b1 = det3(nrm, pc, sptr(next));
          const i64 b2 = det3(nrm, sptr(next), pi);
          if (b1 >= 0 && b2 >= 0) next = i;  // `next` est entre `cur` et `i`
        }
      }
    }
    cur = next;
    if (cur == pivot || count >= m) break;
  }
  return count;
}

// `r` est-il dans l'enveloppe ? Rend un carrier de rang un, deux ou trois.
inline bool ray_in_hull(const i64* pool, const int* avail, const int* hull, int h,
                        const i64 r[3], int* carrier, int* carrier_size,
                        long long* orient_tests) {
  auto sptr = [&](int i) { return pool + (std::size_t)avail[hull[i]] * 3; };
  *carrier_size = 0;
  if (h <= 0) return false;
  if (h == 1) {
    const i64* s = sptr(0);
    const i64 cx = s[1] * r[2] - s[2] * r[1];
    const i64 cy = s[2] * r[0] - s[0] * r[2];
    const i64 cz = s[0] * r[1] - s[1] * r[0];
    if (cx == 0 && cy == 0 && cz == 0 && dot3(s, r) > 0) {
      carrier[0] = avail[hull[0]];
      *carrier_size = 1;
      return true;
    }
    return false;
  }
  // CARRIER DE RANG UN D'ABORD, ET CE N'EST PAS UN DETAIL.
  //
  // Un credit consomme ses `PointId`, donc plus le carrier est petit, plus il
  // reste de sites pour les credits suivants — et c'est le NOMBRE de credits
  // disjoints qui ferme une lane. L'enveloppe seule rend systematiquement le
  // triangle de l'eventail, donc trois identifiants : la mesure montrait
  // `rang3` a 123 022 contre 824 en rang deux, la ou l'enumeration exhaustive
  // trouvait 7 885 carriers de rang deux. Ce test en `O(h)` recupere le rang un
  // sans rien couter.
  for (int i = 0; i < h; ++i) {
    const i64* s = sptr(i);
    const i64 cx = s[1] * r[2] - s[2] * r[1];
    const i64 cy = s[2] * r[0] - s[0] * r[2];
    const i64 cz = s[0] * r[1] - s[1] * r[0];
    if (cx == 0 && cy == 0 && cz == 0 && dot3(s, r) > 0) {
      carrier[0] = avail[hull[i]];
      *carrier_size = 1;
      return true;
    }
  }
  // Toutes les aretes orientees doivent laisser `r` a gauche ou dessus.
  for (int i = 0; i < h; ++i) {
    ++*orient_tests;
    if (det3(sptr(i), sptr((i + 1) % h), r) < 0) return false;
  }
  // Eventail depuis le premier sommet : le premier triangle qui contient `r`
  // donne le carrier. Le rang deux apparait quand un determinant s'annule, et
  // il est LEGITIME.
  for (int i = 1; i + 1 < h; ++i) {
    const i64* a = sptr(0);
    const i64* b = sptr(i);
    const i64* c = sptr(i + 1);
    ++*orient_tests;
    const i64 det = det3(a, b, c);
    if (det == 0) continue;
    const i64 d1 = det3(r, b, c), d2 = det3(a, r, c), d3 = det3(a, b, r);
    const bool ok = (det > 0) ? (d1 >= 0 && d2 >= 0 && d3 >= 0)
                              : (d1 <= 0 && d2 <= 0 && d3 <= 0);
    if (!ok) continue;
    // RANG DEUX QUAND UN POIDS EST NUL. Un `d_k` nul signifie que `r` est sur
    // une face du triangle : deux membres suffisent, et le troisieme reste
    // disponible pour un autre credit.
    if (d1 == 0) {
      carrier[0] = avail[hull[i]];
      carrier[1] = avail[hull[i + 1]];
      *carrier_size = 2;
      return true;
    }
    if (d2 == 0) {
      carrier[0] = avail[hull[0]];
      carrier[1] = avail[hull[i + 1]];
      *carrier_size = 2;
      return true;
    }
    if (d3 == 0) {
      carrier[0] = avail[hull[0]];
      carrier[1] = avail[hull[i]];
      *carrier_size = 2;
      return true;
    }
    carrier[0] = avail[hull[0]];
    carrier[1] = avail[hull[i]];
    carrier[2] = avail[hull[i + 1]];
    *carrier_size = 3;
    return true;
  }
  // Enveloppe degeneree en segment : carrier de rang deux.
  if (h == 2) {
    carrier[0] = avail[hull[0]];
    carrier[1] = avail[hull[1]];
    *carrier_size = 2;
    return true;
  }
  return false;
}

}  // namespace credits
}  // namespace mhgp3v
