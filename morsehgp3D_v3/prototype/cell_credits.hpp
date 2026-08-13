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
// ENVELOPPE PROJECTIVE EXACTE — ANDREW, PAS JARVIS.
//
// Ma premiere version employait une marche de Jarvis. Le contre-audit
// `AUDIT_WORKTREE_CREDITS_CELLULAIRES_20260813.md` l'a refutee sur quatre
// points, tous reproduits par des fixtures deterministes, et le plus grave
// produisait une FAUSSE FERMETURE DANS LE CHEMIN SAIN — pas une divergence de
// mutant :
//
//  1. son departage de colineaires employait `det(a x i, a, next)`, un predicat
//     de DEGRE QUATRE dans les coordonnees des sites. A `M=65535`, avec
//     `a=(M,M,M)`, `i=(M,-M,-M)`, `next=(M,0,0)`, les deux determinants
//     mathematiques valent environ `7,38e19` et le calcul `i64` deborde en
//     rendant `-4503496549203964` : le verdict est INVERSE sous le profil
//     contractuel ;
//  2. la branche `h==2` acceptait `r` sur la seule coplanarite. Dans `U00`, le
//     pool `G={(3,1,0),(3,2,0)}` accepte ainsi `r0=(3,0,0)`, alors que
//     l'unique ecriture plane est `r0 = 2 G0 - G1` : `r0` n'est pas dans le
//     cone positif ;
//  3. deux sites de MEME direction projective, comme `(1,0,0)` et `(2,0,0)`,
//     formaient une enveloppe de taille deux au lieu d'un unique sommet ;
//  4. le pivot n'etait extreme que sur une coordonnee, sans depart transversal.
//     La marche pouvait ne jamais revenir au pivot, et la garde `count>=m`
//     transformait alors le cycle en faux polygone. UNE GARDE DE CAPACITE NE
//     DOIT JAMAIS AUTHENTIFIER UN POLYGONE.
//
// La construction recue est Andrew exact. Choisir `e` orthogonal a `w`, puis
// `f = w x e`. Pour chaque site, stocker `W = w.s > 0`, `E = e.s` et `F = f.s`,
// puis trier les rationnels `(E/W, F/W)` par produits croises. Comme
// `det(e,f,w) = |e|^2 |w|^2 > 0`, le signe de l'orientation plane est celui de
// `det(s_i,s_j,s_k)` — un predicat de degre TROIS, couvert par la borne u16.
//
// Largeurs, `w` de composantes au plus `9` : `|e| <= 18`, `|f| <= 324`, donc
// `|E| <= 3,5e6`, `|F| <= 6,4e7`, `|W| <= 1,8e6`. Les produits croises du tri
// valent au plus `1,1e14`, et `|det(s_i,s_j,s_k)| <= 1,7e15`. Tout tient dans
// `i64`, et aucun produit de degre quatre n'est forme.
// ---------------------------------------------------------------------------

// Coordonnee projective d'un site, avec son identifiant.
struct Proj {
  i64 W = 0, E = 0, F = 0;
  int id = 0;
};

// Base entiere du plan orthogonal a `w`, orientee de sorte que `det(e,f,w)>0`.
inline void plane_basis(const i64 w[3], i64 e[3], i64 f[3]) {
  int axis = 0;
  for (int k = 1; k < 3; ++k)
    if ((w[k] < 0 ? -w[k] : w[k]) < (w[axis] < 0 ? -w[axis] : w[axis])) axis = k;
  i64 ax[3] = {0, 0, 0};
  ax[axis] = 1;
  e[0] = w[1] * ax[2] - w[2] * ax[1];
  e[1] = w[2] * ax[0] - w[0] * ax[2];
  e[2] = w[0] * ax[1] - w[1] * ax[0];
  f[0] = w[1] * e[2] - w[2] * e[1];
  f[1] = w[2] * e[0] - w[0] * e[2];
  f[2] = w[0] * e[1] - w[1] * e[0];
}

inline Proj project(const i64 s[3], const i64 w[3], const i64 e[3], const i64 f[3], int id) {
  Proj p;
  p.W = dot3(w, s);
  p.E = dot3(e, s);
  p.F = dot3(f, s);
  p.id = id;
  return p;
}

// Ordre lexicographique sur `(E/W, F/W)`, par produits croises. `W>0` partout.
inline int proj_cmp(const Proj& a, const Proj& b) {
  const i64 x = a.E * b.W - b.E * a.W;
  if (x != 0) return x < 0 ? -1 : 1;
  const i64 y = a.F * b.W - b.F * a.W;
  if (y != 0) return y < 0 ? -1 : 1;
  return 0;
}

// SEGMENT PROJECTIF, PREDICAT DE DEGRE DEUX. Une fois la coplanarite etablie,
// `r` est dans `[u,v]` lorsque `Delta(u,r)` et `Delta(r,v)` ont le meme signe
// FAIBLE que `Delta(u,v)`, pour `g = e` ou, s'il ne separe pas, `g = f`. Si
// aucun des deux ne separe, `u` et `v` sont la meme direction : rang un.
inline bool projective_between(const i64 u[3], const i64 v[3], const i64 r[3], const i64 w[3],
                               const i64 e[3], const i64 f[3]) {
  for (int pass = 0; pass < 2; ++pass) {
    const i64* g = (pass == 0) ? e : f;
    const i64 duv = dot3(g, v) * dot3(w, u) - dot3(g, u) * dot3(w, v);
    if (duv == 0) continue;  // `g` ne separe pas : essayer l'autre
    const i64 dur = dot3(g, r) * dot3(w, u) - dot3(g, u) * dot3(w, r);
    const i64 drv = dot3(g, v) * dot3(w, r) - dot3(g, r) * dot3(w, v);
    if (duv > 0) return dur >= 0 && drv >= 0;
    return dur <= 0 && drv <= 0;
  }
  return false;  // meme direction projective : le rang un a deja repondu
}

// ENVELOPPE PAR DEUX CHAINES MONOTONES. `pts` porte les projections DEDUPLIQUEES
// par direction ; `out` recoit les indices dans l'ordre cyclique. Rend la taille.
// Aucune garde de capacite n'authentifie ici quoi que ce soit : la construction
// est deterministe et se termine par construction.
inline int monotone_hull(const Proj* pts, const i64* sites, int m, int* out,
                         long long* orient_tests) {
  if (m <= 2) {
    for (int i = 0; i < m; ++i) out[i] = i;
    return m;
  }
  auto orient = [&](int a, int b, int c) {
    ++*orient_tests;
    return det3(sites + (std::size_t)pts[a].id * 3, sites + (std::size_t)pts[b].id * 3,
                sites + (std::size_t)pts[c].id * 3);
  };
  int k = 0;
  for (int i = 0; i < m; ++i) {
    while (k >= 2 && orient(out[k - 2], out[k - 1], i) <= 0) --k;
    out[k++] = i;
  }
  const int lower = k + 1;
  for (int i = m - 2; i >= 0; --i) {
    while (k >= lower && orient(out[k - 2], out[k - 1], i) <= 0) --k;
    out[k++] = i;
  }
  return k - 1;  // le dernier point repete le premier
}

// ---------------------------------------------------------------------------
// COUVERTURE D'UNE CELLULE PAR UN POOL — L'API QUE LE SUJET APPELLE.
//
// Rend vrai lorsque les TROIS rayons de la cellule appartiennent au cone
// positif du pool, et remplit alors `union_ids` avec au plus neuf identifiants
// — un carrier de rang un, deux ou trois par rayon.
//
// Deux gardes tiennent l'exactitude :
//
//  - les directions projectives DUPLIQUEES sont fusionnees avant la marche.
//    Deux sites de meme direction, comme `(1,0,0)` et `(2,0,0)`, ne forment pas
//    un segment : ils sont un unique sommet geometrique. La contre-fixture
//    `G={(1,0,0),(2,0,0)}` avec `r=(3,1,0)` etait acceptee a tort ;
//  - une enveloppe de dimension inferieure a deux rend UNKNOWN. Les trois
//    rayons d'une cellule sont affinement independants, donc aucun segment
//    projectif ne peut contenir leur triangle. C'est la garde qui tue la
//    branche `h==2` inconditionnelle, laquelle produisait une FAUSSE FERMETURE
//    dans le chemin sain.
// ---------------------------------------------------------------------------
// `rank_counts` recoit le rang du carrier de CHAQUE rayon — un, deux ou trois.
// Compter la taille de l'UNION serait un contresens : elle vaut presque toujours
// trois ou plus, et l'information utile est qu'un rayon a ete porte par un seul
// site, donc qu'il en reste pour les credits suivants.
inline bool cell_covered(const i64* sites, const int* avail, int m, const i64 rays[3][3],
                         int* union_ids, int* union_size, long long* orient_tests,
                         Proj* scratch, int* hull, long long* rank_counts = nullptr) {
  *union_size = 0;
  if (m < 3) return false;
  const i64 w[3] = {rays[0][0] + rays[1][0] + rays[2][0], rays[0][1] + rays[1][1] + rays[2][1],
                    rays[0][2] + rays[1][2] + rays[2][2]};
  i64 e[3], f[3];
  plane_basis(w, e, f);

  int cnt = 0;
  for (int i = 0; i < m; ++i) {
    const i64* s = sites + (std::size_t)avail[i] * 3;
    if (dot3(w, s) <= 0) continue;  // hors de la carte : le pool actif l'exclut
    scratch[cnt++] = project(s, w, e, f, avail[i]);
  }
  if (cnt < 3) return false;
  // Tri par coordonnee projective, puis FUSION des directions dupliquees. Le
  // representant canonique d'un sommet geometrique est le plus petit `PointId`.
  for (int i = 1; i < cnt; ++i) {
    const Proj key = scratch[i];
    int j = i - 1;
    while (j >= 0 && (proj_cmp(scratch[j], key) > 0 ||
                      (proj_cmp(scratch[j], key) == 0 && scratch[j].id > key.id))) {
      scratch[j + 1] = scratch[j];
      --j;
    }
    scratch[j + 1] = key;
  }
  int uniq = 0;
  for (int i = 0; i < cnt; ++i) {
    if (uniq > 0 && proj_cmp(scratch[uniq - 1], scratch[i]) == 0) continue;
    scratch[uniq++] = scratch[i];
  }
  if (uniq < 3) return false;  // dimension du hull < 2 : UNKNOWN

  const int h = monotone_hull(scratch, sites, uniq, hull, orient_tests);
  if (h < 3) return false;  // segment ou point : ne peut contenir un triangle

  auto sptr = [&](int i) { return sites + (std::size_t)scratch[hull[i]].id * 3; };
  auto add = [&](int id) {
    for (int k = 0; k < *union_size; ++k)
      if (union_ids[k] == id) return;
    union_ids[(*union_size)++] = id;
  };

  for (int j = 0; j < 3; ++j) {
    const i64* r = rays[j];
    // Rang un : un sommet geometrique colineaire positif au rayon.
    int found = -1;
    for (int i = 0; i < h && found < 0; ++i) {
      const i64* s = sptr(i);
      if (s[1] * r[2] - s[2] * r[1] == 0 && s[2] * r[0] - s[0] * r[2] == 0 &&
          s[0] * r[1] - s[1] * r[0] == 0 && dot3(s, r) > 0)
        found = i;
    }
    if (found >= 0) {
      add(scratch[hull[found]].id);
      if (rank_counts != nullptr) ++rank_counts[1];
      continue;
    }
    // Appartenance : toutes les aretes orientees laissent `r` a gauche ou dessus.
    bool inside = true;
    for (int i = 0; i < h && inside; ++i) {
      ++*orient_tests;
      if (det3(sptr(i), sptr((i + 1) % h), r) < 0) inside = false;
    }
    if (!inside) return false;
    // Carrier par l'eventail, en preferant le rang deux quand un poids de
    // Cramer s'annule : un credit CONSOMME ses identifiants.
    bool got = false;
    for (int i = 1; i + 1 < h && !got; ++i) {
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
      if (d1 == 0) {
        add(scratch[hull[i]].id); add(scratch[hull[i + 1]].id);
        if (rank_counts != nullptr) ++rank_counts[2];
      } else if (d2 == 0) {
        add(scratch[hull[0]].id); add(scratch[hull[i + 1]].id);
        if (rank_counts != nullptr) ++rank_counts[2];
      } else if (d3 == 0) {
        add(scratch[hull[0]].id); add(scratch[hull[i]].id);
        if (rank_counts != nullptr) ++rank_counts[2];
      } else {
        add(scratch[hull[0]].id); add(scratch[hull[i]].id); add(scratch[hull[i + 1]].id);
        if (rank_counts != nullptr) ++rank_counts[3];
      }
      got = true;
    }
    if (!got) return false;  // `r` est dans le hull mais aucun triangle ne le porte
  }
  return true;
}

}  // namespace credits
}  // namespace mhgp3v