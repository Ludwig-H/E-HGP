// MorseHGP3D v3 — PREFILTRE COMBINE : COEUR + DOMINANCE PAR EXTREMITE.
//
// Specification : proposition de l'utilisateur du 15 aout 2026, formalisee ici.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_counter_only,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// L'IDEE, ET POURQUOI LA SOMME EST LICITE
//
// Un support positif d'arite `q` est possede par sa PAIRE DIAMETRALE `(a,b)`.
// Tout site strictement interieur a sa miniboule est dans le FUSEAU
// `W_q(a,b)`, intersection de toutes les boules admissibles de cette ancre a
// cette arite. Si `|P inter W_q(a,b)| >= h_q`, l'ancre ne porte aucun support
// pertinent : elle meurt. C'est le filtre d'ancre du contrat de source.
//
// Compter `|P inter W_q|` exactement coute une requete par paire. On le MINORE
// par trois comptes disjoints, calcules une fois par rectangle de la WSPD :
//
//   h_coeur  temoins universels sur TOUT le rectangle, hors `A` et hors `B` ;
//   h_a      temoins de `A` universels sur `{a} x B`, pour chaque `a` de `A` ;
//   h_b      temoins de `B` universels sur `A x {b}`, pour chaque `b` de `B`.
//
// DISJONCTION, ET ELLE EST DEUX FOIS ACQUISE. Par convention `h_coeur` ne
// compte que des sites hors `A union B` — mais c'est en fait automatique : pour
// `z` dans `A`, le choix `a = z` donne `H = 0`, donc `z` n'est JAMAIS certifie
// temoin universel du rectangle. Le mutant qui recomptait `A` et `B` dans le
// coeur s'est revele inatteignable pour cette raison, et a ete remplace par la
// faute reelle : majorer `Xi` aux coins. `h_coeur` ne compte donc que des sites
// hors `A union B` ; `h_a` ne compte que des sites de `A` ; `h_b` que des
// sites de `B` ; et `A inter B` est vide par construction de la partition
// Callahan--Kosaraju. Les trois ensembles sont donc deux a deux disjoints SANS
// hypothese geometrique, et
//
//     |P inter W_q(a,b)|  >=  h_coeur + h_a + h_b.
//
// La paire meurt donc des que cette somme atteint `h_q`. C'est un MINORANT :
// le filtre est fail-open, il ne ferme jamais a tort.
//
// C'est exactement ce que le contre-audit de la « gate a trois voies »
// reclamait et que le tableau refuse ne fournissait pas : une UNION COMMUNE,
// avec un ledger d'identites, au lieu de trois mesures juxtaposees.
//
// ---------------------------------------------------------------------------
// LA DECISION NE TOUCHE JAMAIS UNE PAIRE
//
// `h_coeur` ne depend que du rectangle, `h_a` que de `a`, `h_b` que de `b`.
// Le nombre de paires survivantes vaut donc
//
//     somme_{a dans A}  |{ b dans B : h_b < h_q - h_coeur - h_a }|,
//
// et comme tous les comptes sont ecretes a `h_q <= 10`, un histogramme de
// `h_b` sur onze cases suffit. Le cout par rectangle est `O(|A| + |B|)` APRES
// le calcul des `h`, jamais `O(|A| |B|)`. Aucune paire n'est materialisee.
//
// ---------------------------------------------------------------------------
// LES PREDICATS, TOUS ENTIERS ET TOUS SURS PAR EN DESSOUS
//
// Pour `z` temoin, `w = z-a`, `d = b-a` :
//
//     H(a,b,z) = (b-z).(z-a) = d.w - |w|^2
//     Xi(a,b,z) = |d x w|^2
//
//     q2 : H > 0            q3 : H > 0 et 3H^2 > Xi      q4 : H > 0 et 2H^2 > Xi
//
// Ces trois domaines sont les fuseaux `W_2 > W_3 > W_4` du contrat.
//
// UNIFORMITE SUR UNE BOITE. `a` et `b` parcourent des boites entieres. `H` est
// affine en `b` a `a,z` fixes, donc son minimum sur la boite se lit par
// SEPARATION D'AXE, exactement. Chaque composante de `d x w` est affine en
// `d`, donc son extremum par axe donne une MAJORATION exacte de `Xi`. On
// emploie `Hmin` et `Ximax` : minorer `H` et majorer `Xi` ne peut que perdre
// des temoins, jamais en inventer.
//
// LARGEURS, PROFIL u16. `|d.w| <= 3*65535^2 < 2^34`, `|w|^2 < 2^34`, donc
// `|H| < 2^35` tient en `i64`. `H^2 < 2^70` et les composantes de `d x w`
// valent au plus `2*65535^2`, donc `Xi < 2^69` : les deux sont formes en
// `i128`, jamais en `i64`. Le mutant `largeur-i64` doit mourir.
//
// ---------------------------------------------------------------------------
// CE QUE CE PROBE MESURE, ET CE QU'IL NE MESURE PAS
//
// Il compte, par lane, les ANCRES survivantes, puis estime le travail restant
// par ancre : porteurs de la lentille pour q3, couples de porteurs pour q4.
// Il ne produit AUCUN support, ne calcule aucun circumcentre, ne decide aucun
// rang et ne qualifie aucun SLO. C'est un compteur.
//
// LE CAP DE CELLULE, ET CE QU'IL FAUSSAIT. `h_a` et `h_b` se calculent par
// auto-jointure `O(|A|^2)`, d'ou un cap. Deux strategies :
//
//   `--cap=scission` (DEFAUT) : un rectangle trop gros est RAFFINE — on
//        redescend dans l'arbre jusqu'a ce que les deux extremites tiennent
//        sous le cap. Le recouvrement est preserve, chaque sous-rectangle est
//        re-teste pour la separation, et la recursion s'arrete au pire sur deux
//        feuilles, qui sont des points. `masse_non_decide` vaut donc ZERO par
//        construction, et la mesure ne majore plus le residuel.
//   `--cap=refus` : l'ancienne strategie, conservee pour rejouer les recus
//        anterieurs. Le rectangle n'est pas decide, toutes ses paires comptent
//        SURVIVANTES, et le rectangle est publie dans `non_decides`.
//
// Pourquoi ce n'est pas un detail : a `terrain, n=32000, s=8`, cinquante-deux
// rectangles capes sur 5,6 millions portaient SOIXANTE-QUINZE POUR CENT du
// residuel, et `99,052 %` du gain que j'avais attribue a `s=8` venait de la
// masse hors cap, pas des certificats.
//
// La separation N'EST PAS monotone sous raffinement, parce que la sphere
// circonscrite a une AABB ne l'est pas : la boite `[0,10]^2 x {0}` est incluse
// dans `[0,10]^3`, mais sa sphere (`rayon 5 sqrt(2)`, centre a `(5,5,0)`) sort
// de celle du parent (`rayon 5 sqrt(3)`, centre a `(5,5,5)`). Chaque
// sous-rectangle est donc RE-TESTE, jamais herite.
//
// CODES DE SORTIE : 1 desaccord du juge, 2 campagne refusee avant calcul,
// 3 plancher ou invariant viole, 4 mutant tue.
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/soc64_rect.hpp"
#include "prototype/spindle_core_ball.hpp"
#include "prototype/wspd_wavefront.hpp"

namespace {

using mhgp::i128;
using mhgp::i64;
using mhgp::P3;
using mhgp3v::CloudFamily;
using mhgp3v::WfNode;

// Seuils de mort par arite a `smax` : la lane `q` meurt au `smax-q+1`-ieme
// temoin. A `smax=11` cela donne 10, 9, 8 — un `h` par arite, decroissant.
inline int death_threshold(int smax, int q) { return smax - q + 1; }

// ---------------------------------------------------------------------------
// MUTANTS. Chacun casse UNE decision exacte et doit etre tue par une porte.
// ---------------------------------------------------------------------------
enum class Mutant {
  kNone,
  kIntervalXi,    // majore `Xi` par intervalles au lieu des sommets : sur, mais lache
  kNarrowI64,     // forme `H^2` et `Xi` en i64 : debordement silencieux
  kCoreCentreOnly,  // teste le centre de la boite au lieu de ses huit coins
  kThresholdOff,  // seuil a `h_q+1` : ferme des ancres vivantes
  kBulkSansMasque,  // LE BUG DU 15 AOUT : le credit en bloc ne retire pas la
                    // lane aux enfants, qui la recreditent par leurs feuilles.
  kDropB,         // oublie la contribution de `B` : minorant plus faible, sur
                  // mais il doit se voir au compteur
  kDualSansMasque,  // LE MEME DEFAUT QUE LE P0 q2, MAIS DANS LE DUAL-TREE : un
                    // bloc credite ne retire pas la lane a ses enfants, qui la
                    // recreditent. Les `h_a` cessent d'egaler la jointure.
  kCorner64Sept,  // n'evalue que sept des huit coins de `A` : la specialisation
                  // ponctuelle cesse d'etre le meme calcul que la reference et
                  // SUR-certifie. Il rend la porte d'egalite non vacue.
  kVivantSansExtinction,  // dans le balayage fusionne, une lane qui atteint son
                          // seuil n'est PAS eteinte, et son compteur continue
                          // de croitre : sans effet ici, puisque le verdict est
                          // `c < h_q`. C'est un mutant NEUTRE, et il sert a le
                          // montrer — la porte des deux balayages doit rester
                          // verte, sinon l'extinction n'etait pas anodine.
  kVivantLaneUnique,      // le balayage fusionne rend la lane q2 pour tout
                          // temoin, quel que soit l'angle : q3 et q4 comptent
                          // alors zero temoin et deviennent toutes vivantes.
                          // C'est LUI que la porte des deux balayages tue.
};

const char* mutant_name(Mutant m) {
  switch (m) {
    case Mutant::kNone: return "none";
    case Mutant::kIntervalXi: return "coeur-intervalle-xi";
    case Mutant::kNarrowI64: return "largeur-i64";
    case Mutant::kCoreCentreOnly: return "coeur-centre-seul";
    case Mutant::kThresholdOff: return "seuil-decale";
    case Mutant::kBulkSansMasque: return "bulk-sans-masque";
    case Mutant::kDropB: return "oublie-b";
    case Mutant::kDualSansMasque: return "dual-sans-masque";
    case Mutant::kCorner64Sept: return "corner64-sept-coins";
    case Mutant::kVivantSansExtinction: return "vivant-sans-extinction";
    case Mutant::kVivantLaneUnique: return "vivant-lane-unique";
  }
  return "?";
}

// ---------------------------------------------------------------------------
// BOITE ENTIERE ET PRIMITIVES EXACTES.
// ---------------------------------------------------------------------------
struct Box {
  i64 lo[3], hi[3];
};

inline Box box_of(const WfNode& n) {
  Box b;
  for (int i = 0; i < 3; ++i) { b.lo[i] = n.tlo[i]; b.hi[i] = n.thi[i]; }
  return b;
}
inline Box box_of_point(const P3& p) {
  Box b;
  b.lo[0] = b.hi[0] = p.x; b.lo[1] = b.hi[1] = p.y; b.lo[2] = b.hi[2] = p.z;
  return b;
}

// `Hmin` sur `b` dans la boite `B`, a `a` et `z` fixes.
//   H = d.w - |w|^2  avec d = b-a, w = z-a.
// `d.w = somme_i (b_i - a_i) w_i` est affine en `b` : son minimum se lit axe
// par axe, en prenant l'extremite qui minimise `w_i (b_i - a_i)`.
inline i64 h_min_over_box(const P3& a, const Box& B, const P3& z, bool narrow) {
  i64 w[3] = {z.x - a.x, z.y - a.y, z.z - a.z};
  i64 dot_min = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 aa = (i == 0) ? a.x : (i == 1) ? a.y : a.z;
    const i64 c1 = w[i] * (B.lo[i] - aa);
    const i64 c2 = w[i] * (B.hi[i] - aa);
    dot_min += (c1 < c2) ? c1 : c2;
  }
  const i64 w2 = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
  (void)narrow;  // `H` tient en i64 sous u16 ; c'est `H^2` que le mutant casse.
  return dot_min - w2;
}

// `Ximax` sur `b` dans la boite `B`. Chaque composante de `d x w` est affine
// en `d`, donc son extremum se lit par axe ; on prend le carre du plus grand
// module, ce qui MAJORE `Xi` exactement.
inline i128 xi_max_over_box(const P3& a, const Box& B, const P3& z, bool narrow) {
  const i64 w[3] = {z.x - a.x, z.y - a.y, z.z - a.z};
  const i64 alo[3] = {B.lo[0] - a.x, B.lo[1] - a.y, B.lo[2] - a.z};
  const i64 ahi[3] = {B.hi[0] - a.x, B.hi[1] - a.y, B.hi[2] - a.z};
  // (d x w)_0 = d1 w2 - d2 w1 ; (d x w)_1 = d2 w0 - d0 w2 ;
  // (d x w)_2 = d0 w1 - d1 w0.
  const int p[3][2] = {{1, 2}, {2, 0}, {0, 1}};
  i128 acc = 0;
  for (int k = 0; k < 3; ++k) {
    const int i = p[k][0], j = p[k][1];
    // terme = d_i w_j - d_j w_i, affine et separable en (d_i, d_j).
    const i64 t1lo = w[j] * alo[i], t1hi = w[j] * ahi[i];
    const i64 t2lo = -w[i] * alo[j], t2hi = -w[i] * ahi[j];
    const i64 lo = (t1lo < t1hi ? t1lo : t1hi) + (t2lo < t2hi ? t2lo : t2hi);
    const i64 hi = (t1lo > t1hi ? t1lo : t1hi) + (t2lo > t2hi ? t2lo : t2hi);
    const i64 m = (lo < 0 ? -lo : lo) > (hi < 0 ? -hi : hi) ? (lo < 0 ? -lo : lo)
                                                            : (hi < 0 ? -hi : hi);
    if (narrow) {
      const i64 sq = m * m;  // MUTANT : deborde des que m > 3e9.
      acc += (i128)sq;
    } else {
      acc += (i128)m * (i128)m;
    }
  }
  return acc;
}

// `z` est-il temoin universel de lane `q` pour TOUTE cible `b` de la boite ?
// `a` est un point. Fail-open : un `false` ne prouve rien.
// ---------------------------------------------------------------------------
// AUTORITE EXACTE A HUIT COINS POUR `h_a` ET `h_b`.
//
// A `a` et `z` FIXES, `t = b - z` est AFFINE en `b`, et l'ensemble admissible
// en `t` est le cone circulaire ouvert d'axe `e = z - a` et de demi-angle
// `theta'_q` — convexe. Les huit coins de `Box(B)` admissibles impliquent donc
// toute la boite, et l'implication ne se renverse pas : un echec reste
// `UNKNOWN`. C'est le meme argument que `soc64_rect.hpp`, degenere aux deux
// temoins ponctuels, donc HUIT evaluations et non 512.
//
// Ce que cela remplace : `universal_witness` majore `Xi` par
// `xi_max_over_box`, qui maximise SEPAREMENT le module de chaque composante du
// produit vectoriel puis somme les carres — un majorant sur, jamais le maximum.
// L'autorite aux coins est exacte sur l'enveloppe continue de la boite, donc
// elle domine, et le contre-audit du 15 aout demandait deja ce remplacement
// (P1.9). Elle ne peut que faire CROITRE `h_a`, donc la fermeture.
inline bool universal_corner8(const P3& a, const Box& B, const P3& z, int q) {
  const i64 e[3] = {z.x - a.x, z.y - a.y, z.z - a.z};
  const i64 e2 = e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
  if (e2 == 0) return false;  // `z == a` : jamais un temoin de sa propre ancre
  for (int c = 0; c < 8; ++c) {
    const i64 b[3] = {(c & 1) ? B.hi[0] : B.lo[0], (c & 2) ? B.hi[1] : B.lo[1],
                      (c & 4) ? B.hi[2] : B.lo[2]};
    const i64 t[3] = {b[0] - z.x, b[1] - z.y, b[2] - z.z};
    const i64 h = e[0] * t[0] + e[1] * t[1] + e[2] * t[2];
    if (h <= 0) return false;
    if (q == 2) continue;
    const i64 t2 = t[0] * t[0] + t[1] * t[1] + t[2] * t[2];
    const i128 hh = (i128)h * (i128)h;
    const i128 ex = (i128)e2 * (i128)t2;
    // q3 : 4H^2 > E T ; q4 : 3H^2 > E T. Emboitement `W4 < W3 < W2`.
    if (q == 3 ? !(4 * hh > ex) : !(3 * hh > ex)) return false;
  }
  return true;
}

// Meme autorite, rendue comme LANE minimale (0/2/3/4) : les trois fuseaux etant
// emboites, un seul parcours des huit coins decide les trois lanes.
// ---------------------------------------------------------------------------
// COINS DISTINCTS. Une AABB plate sur un axe n'a pas huit coins mais quatre ;
// un point n'en a qu'un. `corner512_all_lane` boucle `8x8x8` sans le voir, et
// evalue donc jusqu'a huit fois le meme triplet — c'est exactement la
// redondance qui rendait `corner512` sept fois trop cher pour le cœur. Le cas
// n'est pas theorique : `terrain` est quasi-surfacique, donc ses nœuds sont
// souvent plats sur un axe, et `4 x 8 x 4 = 128` remplace alors `512`.
inline int corners_distinct(const Box& B, i64 out[8][3]) {
  int nx = 0;
  i64 v[3][2];
  int cnt[3];
  for (int i = 0; i < 3; ++i) {
    v[i][0] = B.lo[i];
    v[i][1] = B.hi[i];
    cnt[i] = (B.lo[i] == B.hi[i]) ? 1 : 2;
  }
  for (int i = 0; i < cnt[0]; ++i)
    for (int j = 0; j < cnt[1]; ++j)
      for (int k = 0; k < cnt[2]; ++k) {
        out[nx][0] = v[0][i]; out[nx][1] = v[1][j]; out[nx][2] = v[2][k];
        ++nx;
      }
  return nx;
}

// Lane `ALL` du produit relaxe `Box(U) x Box(B) x Box(Z)`, aux seuls coins
// DISTINCTS. Meme decision que `corner512_all_lane` — l'argument de convexite
// en trois temps ne depend pas de la multiplicite des coins enumeres — pour un
// cout divise par huit des qu'une des trois boites est plate, et par 512 quand
// les trois sont ponctuelles.
inline int block_lane(const Box& U, const Box& Bp, const Box& Z, long long* ev) {
  i64 cu[8][3], cb[8][3], cz[8][3];
  const int nu = corners_distinct(U, cu);
  const int nb2 = corners_distinct(Bp, cb);
  const int nz = corners_distinct(Z, cz);
  int best = 4;
  for (int ia = 0; ia < nu; ++ia)
    for (int ic = 0; ic < nz; ++ic) {
      const i64 e[3] = {cz[ic][0] - cu[ia][0], cz[ic][1] - cu[ia][1], cz[ic][2] - cu[ia][2]};
      const i64 e2 = e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
      if (e2 == 0) return 0;  // une ancre confondue avec un temoin
      for (int ib = 0; ib < nb2; ++ib) {
        if (ev) ++*ev;  // UNITE HOMOGENE : une evaluation du predicat (e,t)
        const i64 t[3] = {cb[ib][0] - cz[ic][0], cb[ib][1] - cz[ic][1], cb[ib][2] - cz[ic][2]};
        const i64 h = e[0] * t[0] + e[1] * t[1] + e[2] * t[2];
        if (h <= 0) return 0;
        const i64 t2 = t[0] * t[0] + t[1] * t[1] + t[2] * t[2];
        const i128 hh = (i128)h * (i128)h;
        const i128 ex = (i128)e2 * (i128)t2;
        int lane = 2;
        if (3 * hh > ex) lane = 4;
        else if (4 * hh > ex) lane = 3;
        if (lane < best) best = lane;
        if (best == 2 && lane == 2) { /* q2 seul ; seul `NONE` sort */ }
      }
    }
  return best;
}

inline int corner8_lane(const P3& a, const Box& B, const P3& z, long long* ev = nullptr) {
  const i64 e[3] = {z.x - a.x, z.y - a.y, z.z - a.z};
  const i64 e2 = e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
  if (e2 == 0) return 0;
  int best = 4;
  for (int c = 0; c < 8; ++c) {
    if (ev) ++*ev;
    const i64 b[3] = {(c & 1) ? B.hi[0] : B.lo[0], (c & 2) ? B.hi[1] : B.lo[1],
                      (c & 4) ? B.hi[2] : B.lo[2]};
    const i64 t[3] = {b[0] - z.x, b[1] - z.y, b[2] - z.z};
    const i64 h = e[0] * t[0] + e[1] * t[1] + e[2] * t[2];
    if (h <= 0) return 0;
    const i64 t2 = t[0] * t[0] + t[1] * t[1] + t[2] * t[2];
    const i128 hh = (i128)h * (i128)h;
    const i128 ex = (i128)e2 * (i128)t2;
    int lane = 2;
    if (3 * hh > ex) lane = 4;
    else if (4 * hh > ex) lane = 3;
    if (lane < best) best = lane;
  }
  return best;
}

// Meme predicat, aux TROIS temoins PONCTUELS. `corner8_lane(a, Box(b), z)`
// enumere huit coins qui sont tous le meme point quand `b` est ponctuel : sept
// evaluations sur huit sont litteralement identiques. Le re-audit du 15 aout le
// releve en section 6.4. La decision est la meme — l'enveloppe convexe d'un
// singleton est ce singleton — pour un huitieme du travail.
//
// Rendu : `0` si `z` n'est temoin d'aucune lane, sinon la LANE MAXIMALE dont il
// est temoin (2, 3 ou 4), l'emboitement `W_4 < W_3 < W_2` faisant le reste.
inline int pair_lane(const P3& a, const P3& b, const P3& z) {
  const i64 e[3] = {z.x - a.x, z.y - a.y, z.z - a.z};
  const i64 e2 = e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
  if (e2 == 0) return 0;  // `z == a` : jamais temoin de sa propre ancre
  const i64 t[3] = {b.x - z.x, b.y - z.y, b.z - z.z};
  const i64 h = e[0] * t[0] + e[1] * t[1] + e[2] * t[2];
  if (h <= 0) return 0;  // couvre aussi `z == b`, ou `t = 0` donne `h = 0`
  const i64 t2 = t[0] * t[0] + t[1] * t[1] + t[2] * t[2];
  const i128 hh = (i128)h * (i128)h;
  const i128 ex = (i128)e2 * (i128)t2;
  if (3 * hh > ex) return 4;
  if (4 * hh > ex) return 3;
  return 2;
}

// ---------------------------------------------------------------------------
// LE SEED AIGU, DECIDE PAR LE SIGNE DE `H` — ET C'EST LE MEME `H`.
//
// `(a,b)` etant l'arete maximale, un troisieme sommet `x` forme un SEED AIGU
// ssi le triangle `abx` est aigu. Les trois angles se reduisent a UN test :
//
//   x est un seed de (a,b)   <=>   x dans L(a,b)   ET   H(a,x,b) < 0
//
// avec `L(a,b) = {x : |ax| <= |ab| et |bx| <= |ab|}` la lentille et
// `H = (x-a).(b-x)` — exactement le `H` du fuseau, `e = x-a`, `t = b-x`.
//
// POURQUOI LES ANGLES EN `a` ET `b` SONT GRATUITS. Si l'angle en `a` valait
// `>= 90`, alors `|bx|^2 = |ax|^2 + |ab|^2 - 2 (x-a).(b-a) >= |ax|^2 + |ab|^2`,
// donc `|bx| > |ab|` des que `x != a` : `x` sortirait de la lentille. Dans la
// lentille, seul l'angle en `x` peut etre obtus, et son signe est `-H`.
//
// LA STRICTE N'EST PAS COSMETIQUE. `H = 0` est l'angle DROIT exact, donc un
// non-seed. Mon premier jet ecrivait `H <= 0` et faisait 185 ecarts sur 8005
// ancres, tous de ce cas. Verifie ensuite a 331 857 triplets sur quatre
// regimes — dont une grille `3^3` volontairement degeneree — sans un ecart.
//
// CE QUE CELA DONNE. `W_2(a,b) = {H > 0}` est la boule diametrale ouverte, donc
// son adherence est `{H >= 0}` et la lentille se PARTITIONNE :
//
//   L = (L inter {H >= 0})  disjoint  (L inter {H < 0})
//       \___ non-seeds ___/           \____ seeds ____/
//
// Un temoin q2 est donc exactement un non-seed : le meme parcours qui compte
// les temoins designe les candidats. Identite de comptage verifiee elle aussi
// sans ecart sur 8005 ancres :
//
//   #seeds(a,b) = |P inter L| - |P inter L inter {H >= 0}|.
inline bool est_seed(const P3& a, const P3& b, const P3& x) {
  const i64 ex = x.x - a.x, ey = x.y - a.y, ez = x.z - a.z;
  const i64 tx = b.x - x.x, ty = b.y - x.y, tz = b.z - x.z;
  const i64 ax2 = ex * ex + ey * ey + ez * ez;
  const i64 bx2 = tx * tx + ty * ty + tz * tz;
  if (ax2 == 0 || bx2 == 0) return false;  // `x` confondu avec une extremite
  const i64 dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
  const i64 ab2 = dx * dx + dy * dy + dz * dz;
  if (ax2 > ab2 || bx2 > ab2) return false;  // hors lentille : `(a,b)` non maximale
  return ex * tx + ey * ty + ez * tz < 0;    // STRICTE : `H = 0` est l'angle droit
}

// AUTORITE DE BLOC POUR L'ELAGAGE DES SEEDS. Certifie qu'AUCUN `x` de `Box X`
// n'est un seed d'AUCUNE ancre `(a,b)` de `Box A x Box B`, par le certificat
// suffisant « `H >= 0` partout ».
//
// Les sommets suffisent, et l'argument est en trois temps comme pour le fuseau.
// En developpant, `H = -|x|^2 + x.(a+b) - a.b` : CONCAVE en `x` — le terme
// quadratique est `-|x|^2` — donc son minimum sur une boite est atteint en un
// SOMMET ; et AFFINE en `a` comme en `b`. Le minimum sur le produit des trois
// boites est donc atteint en un triplet de sommets, et les enumerer decide
// exactement l'enveloppe continue.
//
// C'est le pendant exact du prefiltre, polarite retournee : la ou
// `universal_corner8` certifie « tout le bloc est TEMOIN » pour crediter,
// celui-ci certifie « tout le bloc est NON-SEED » pour elaguer. Meme descente,
// memes coins, meme arithmetique.
//
// Le certificat est SUFFISANT, pas necessaire : un bloc peut n'avoir aucun seed
// tout en contenant un point de `H < 0` hors lentille. L'elagage est donc
// conservatif — il ne rate jamais un seed, il en garde parfois trop.
// ---------------------------------------------------------------------------
// LE MEME CERTIFICAT EN `O(1)` — parce que `H` sur une boite EST une boule.
//
// En completant le carre, avec `m = (a+b)/2` et `R = |ab|/2` :
//
//   H(x) = -|x|^2 + x.(a+b) - a.b = R^2 - |x - m|^2.
//
// Donc « `H >= 0` sur toute la boite `X` » equivaut EXACTEMENT a « `X` est
// incluse dans la boule diametrale fermee ». Evident apres coup — `H > 0` EST
// l'interieur de cette boule — mais je ne l'avais pas vu, et j'ai d'abord
// enumere huit coins pour calculer un minimum dont le lieu est connu.
//
// Le maximum de `|x - m|` sur une AABB est atteint au coin le plus eloigne,
// choisi AXE PAR AXE : trois maxima independants, pas huit combinaisons. En
// coordonnees doublees — `M = a + b` entier, donc pas de demi-entier — le test
// s'ecrit `max_{x in X} |2x - M|^2 <= |b - a|^2`, tout en entiers.
//
// Coût : trois `max`, trois carres, une comparaison. Contre huit evaluations du
// produit scalaire. C'est ce facteur huit qui separait mon elagage refute
// (`gain = 0,33` a `0,59`, plus lent que le balayage complet) de sa version
// utilisable.
inline bool bloc_dans_boule_diametrale(const P3& a, const P3& b, const Box& X) {
  const i64 M[3] = {(i64)a.x + b.x, (i64)a.y + b.y, (i64)a.z + b.z};
  const i64 d[3] = {(i64)b.x - a.x, (i64)b.y - a.y, (i64)b.z - a.z};
  const i64 r2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
  const i64 lo[3] = {X.lo[0], X.lo[1], X.lo[2]};
  const i64 hi[3] = {X.hi[0], X.hi[1], X.hi[2]};
  i128 far = 0;
  for (int k = 0; k < 3; ++k) {
    const i64 u = 2 * lo[k] - M[k], v = 2 * hi[k] - M[k];
    const i64 au = u < 0 ? -u : u, av = v < 0 ? -v : v;
    const i64 w = au > av ? au : av;
    far += (i128)w * (i128)w;
  }
  return far <= (i128)r2;  // `H >= 0` partout : boule FERMEE, donc `<=`
}

// L'AUTRE DISJOINT, ET C'EST LUI QUI ELAGUE LE LOINTAIN.
//
// Mon premier elagage n'avait que « la boite est dans la boule diametrale ».
// Mesure : `travail_elag` a peine change, `657` visites de nœud par ancre sur
// un arbre de `799` nœuds — la descente parcourait tout. La cause est de
// polarite : la boule diametrale est PETITE devant l'emprise du nuage, donc
// « boite incluse » ne peut jamais elaguer pres de la racine.
//
// Or l'immense majorite des points ne sont pas des non-seeds parce que
// `H >= 0`, mais parce qu'ils sont HORS LENTILLE. La lentille est locale ; la
// boite lointaine se rejette donc en `O(1)` par disjonction avec `B(a,|ab|)`
// ou `B(b,|ab|)`. C'est ce test-la qui rend la descente logarithmique.
//
// Distance MINIMALE d'un point a une AABB, axe par axe : trois `max` a zero.
inline bool bloc_hors_lentille(const P3& a, const P3& b, const Box& X) {
  const i64 d[3] = {(i64)b.x - a.x, (i64)b.y - a.y, (i64)b.z - a.z};
  const i64 ab2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
  const i64 c[2][3] = {{a.x, a.y, a.z}, {b.x, b.y, b.z}};
  for (int s = 0; s < 2; ++s) {
    i128 near = 0;
    for (int k = 0; k < 3; ++k) {
      i64 e = 0;
      if (c[s][k] < X.lo[k]) e = X.lo[k] - c[s][k];
      else if (c[s][k] > X.hi[k]) e = c[s][k] - X.hi[k];
      near += (i128)e * (i128)e;
    }
    // `|cx| > |ab|` pour tout `x` de la boite : la boite est hors lentille.
    if (near > (i128)ab2) return true;
  }
  return false;
}

// Le certificat complet : une DISJONCTION de deux conditions, chacune exacte
// sur l'enveloppe continue. Certifier une disjonction par l'union de ses
// disjoints certifies est CONSERVATIF — une boite peut n'avoir aucun seed sans
// verifier ni l'un ni l'autre — donc l'elagage ne rate jamais un seed, il en
// garde parfois trop. C'est le sens qu'il faut.
inline bool bloc_sans_seed_boule(const P3& a, const P3& b, const Box& X, long long* ev) {
  if (ev) ++*ev;
  return bloc_hors_lentille(a, b, X) || bloc_dans_boule_diametrale(a, b, X);
}

// `ev` compte les EVALUATIONS DU PREDICAT `(e,t)`, la meme unite que le
// balayage de reference. Sans lui, comparer « tests de feuilles » a « tests de
// points » ignorerait le coût de la descente et fabriquerait un gain : c'est
// exactement l'erreur que le re-audit avait relevee sur le dual-tree.
//
// Cette version a huit coins est CONSERVEE comme juge de la version `O(1)` :
// les deux decident le meme predicat par deux chemins sans primitive commune,
// et la porte `mhgp3v_seed_deux_certificats` exige leur accord.
inline bool bloc_sans_seed(const Box& A, const Box& B, const Box& X, long long* ev) {
  i64 ca[8][3], cb[8][3], cx[8][3];
  const int na = corners_distinct(A, ca);
  const int nb = corners_distinct(B, cb);
  const int nx = corners_distinct(X, cx);
  for (int ix = 0; ix < nx; ++ix)
    for (int ia = 0; ia < na; ++ia) {
      const i64 e[3] = {cx[ix][0] - ca[ia][0], cx[ix][1] - ca[ia][1],
                        cx[ix][2] - ca[ia][2]};
      for (int ib = 0; ib < nb; ++ib) {
        if (ev) ++*ev;
        const i64 t[3] = {cb[ib][0] - cx[ix][0], cb[ib][1] - cx[ix][1],
                          cb[ib][2] - cx[ix][2]};
        if (e[0] * t[0] + e[1] * t[1] + e[2] * t[2] < 0) return false;
      }
    }
  return true;
}

inline bool universal_witness(const P3& a, const Box& B, const P3& z, int q, bool narrow) {
  const i64 h = h_min_over_box(a, B, z, narrow);
  if (h <= 0) return false;
  if (q == 2) return true;
  const i128 xi = xi_max_over_box(a, B, z, narrow);
  if (narrow) {
    const i64 hh = h * h;  // MUTANT : `H^2` deborde i64 des que H > 3e9.
    return (q == 3) ? ((i128)3 * hh > xi) : ((i128)2 * hh > xi);
  }
  const i128 hh = (i128)h * (i128)h;
  return (q == 3) ? (3 * hh > xi) : (2 * hh > xi);
}

// ---------------------------------------------------------------------------
// TEMOIN UNIVERSEL DU RECTANGLE ENTIER : `a` ET `b` PARCOURENT LEUR BOITE.
//
// On veut ici le `h` LE PLUS GRAND QUI RESTE RIGOUREUX, donc les bornes les
// plus serrees possibles — et surtout pas un test aux coins.
//
// POURQUOI LES SOMMETS SUFFISENT, ET C'EST LE POINT DELICAT. `H` est affine en
// `a` a `b` fixe : son minimum est a un sommet. Pour `Xi` la reponse est moins
// evidente, et une premiere lecture la donne fausse. En developpant,
//
//     (b-a) x (z-a) = b x z - b x a - a x z + a x a = b x z - b x a - a x z,
//
// le terme `a x a` etant nul : le produit vectoriel est donc AFFINE en `a`.
// Donc `Xi = |affine(a)|^2` est CONVEXE en `a`, et son maximum sur une boite
// est bien atteint a un SOMMET. Tester les huit sommets est exact, et c'est le
// majorant le plus serre possible — donc le `h` le plus grand qui reste
// rigoureux. Une enveloppe d'intervalles serait sure aussi, mais plus lache ;
// elle est conservee comme mutant `coeur-intervalle-xi` pour mesurer l'ecart.
//
// LA BORNE DE `H`, ET ELLE EST EXACTE.
//
//     H = somme_i [ z_i (a_i + b_i) - a_i b_i ]  -  |z|^2
//
// Le crochet est SEPARABLE PAR AXE et bilineaire en `(a_i,b_i)` : son minimum
// sur le rectangle plan `[alo_i,ahi_i] x [blo_i,bhi_i]` est atteint a l'un de
// ses QUATRE coins. Trois axes, quatre coins : douze evaluations entieres, et
// le resultat est le minimum EXACT sur le produit — on ne peut pas faire plus
// serre.
//
// LA BORNE DE `Xi` : ARITHMETIQUE D'INTERVALLES.
//
//     (d x w)_k = d_i w_j - d_j w_i,   d = b-a,  w = z-a.
//
// `d_i` vit dans `[blo_i - ahi_i, bhi_i - alo_i]`, `w_j` dans
// `[z_j - ahi_j, z_j - alo_j]`. Le produit de deux intervalles est l'enveloppe
// de ses quatre produits d'extremites, la difference est exacte, et on majore
// `|(d x w)_k|` par le plus grand module obtenu. C'est un sur-ensemble des
// valeurs reelles, donc un majorant SUR de `Xi`, et le plus serre qu'une
// arithmetique d'intervalles puisse rendre sans decouper la boite.
// ---------------------------------------------------------------------------
struct Iv {
  i64 lo, hi;
};

inline Iv iv_mul(const Iv& x, const Iv& y) {
  const i128 p[4] = {(i128)x.lo * y.lo, (i128)x.lo * y.hi, (i128)x.hi * y.lo,
                     (i128)x.hi * y.hi};
  i128 lo = p[0], hi = p[0];
  for (int i = 1; i < 4; ++i) { if (p[i] < lo) lo = p[i]; if (p[i] > hi) hi = p[i]; }
  return Iv{(i64)lo, (i64)hi};  // sous u16 chaque produit tient largement en i64
}

// Minimum EXACT de `H` sur `box(A) x box(B)`, `z` fixe.
inline i64 h_min_over_boxes(const Box& A, const Box& B, const P3& z) {
  const i64 zc[3] = {z.x, z.y, z.z};
  i64 acc = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 av[2] = {A.lo[i], A.hi[i]};
    const i64 bv[2] = {B.lo[i], B.hi[i]};
    i64 best = 0;
    bool first = true;
    for (int u = 0; u < 2; ++u)
      for (int v = 0; v < 2; ++v) {
        const i64 val = zc[i] * (av[u] + bv[v]) - av[u] * bv[v];
        if (first || val < best) { best = val; first = false; }
      }
    acc += best;
  }
  return acc - (z.x * z.x + z.y * z.y + z.z * z.z);
}

// Majorant SUR de `Xi` sur `box(A) x box(B)`, `z` fixe.
inline i128 xi_max_over_boxes(const Box& A, const Box& B, const P3& z) {
  const i64 zc[3] = {z.x, z.y, z.z};
  Iv d[3], w[3];
  for (int i = 0; i < 3; ++i) {
    d[i] = Iv{B.lo[i] - A.hi[i], B.hi[i] - A.lo[i]};
    w[i] = Iv{zc[i] - A.hi[i], zc[i] - A.lo[i]};
  }
  const int p[3][2] = {{1, 2}, {2, 0}, {0, 1}};
  i128 acc = 0;
  for (int k = 0; k < 3; ++k) {
    const int i = p[k][0], j = p[k][1];
    const Iv t1 = iv_mul(d[i], w[j]);
    const Iv t2 = iv_mul(d[j], w[i]);
    const i64 lo = t1.lo - t2.hi, hi = t1.hi - t2.lo;
    const i64 al = lo < 0 ? -lo : lo, ah = hi < 0 ? -hi : hi;
    const i64 m = al > ah ? al : ah;
    acc += (i128)m * (i128)m;
  }
  return acc;
}

// Majorant de `H` sur `box(A) x box(B) x box(Z)` : sert a ELAGUER un noeud
// entier de l'octree pendant la recherche du coeur, sans jamais perdre un
// temoin. Par axe, `g = z_i(a_i+b_i) - a_i b_i - z_i^2` est concave en `z_i` ;
// son maximum est au sommet `(a_i+b_i)/2` rabattu dans la boite, evalue aux
// deux entiers encadrants et aux deux bords.
inline i64 h_max_over_boxes(const Box& A, const Box& B, const Box& Z) {
  i64 acc = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 av[2] = {A.lo[i], A.hi[i]};
    const i64 bv[2] = {B.lo[i], B.hi[i]};
    i64 best = 0;
    bool first = true;
    for (int u = 0; u < 2; ++u)
      for (int v = 0; v < 2; ++v) {
        const i64 s = av[u] + bv[v], pr = av[u] * bv[v];
        i64 cand[4];
        int nc = 0;
        cand[nc++] = Z.lo[i];
        cand[nc++] = Z.hi[i];
        const i64 mid = s / 2;
        for (i64 m = mid; m <= mid + 1; ++m)
          if (m > Z.lo[i] && m < Z.hi[i]) cand[nc++] = m;
        for (int c = 0; c < nc; ++c) {
          const i64 zi = cand[c];
          const i64 val = zi * s - pr - zi * zi;
          if (first || val > best) { best = val; first = false; }
        }
      }
    acc += best;
  }
  return acc;
}

// ---------------------------------------------------------------------------
// LES DEUX BORNES QUI RENDENT LA DESCENTE SOUS-QUADRATIQUE.
//
// Elaguer sur `max H` etait beaucoup trop lache : `{z : max_{a,b} H > 0}` est
// grosso modo l'UNION des boules diametrales du rectangle, une boule de rayon
// `~d/2`. La descente y visitait tout, soit `2154` evaluations par rectangle.
// Ce qu'il faut borner, c'est `min_{a,b} H`, la quantite qui decide vraiment.
//
//   `h_all_inside`  minore `H` sur `A x B x Z` tout entier. S'il est positif,
//                   TOUT point de `Z` est temoin q2 : on credite la population
//                   du noeud sans descendre.
//   `h_any_upper`   majore `max_{z dans Z} min_{a,b} H`. S'il est negatif ou
//                   nul, AUCUN point de `Z` n'est temoin : on elague.
//
// La seconde emploie l'inegalite minimax `max_z min_c f_c(z) <= min_c max_z
// f_c(z)`, valide sans hypothese, donc le majorant reste sur. Par axe,
// `f(z_i) = z_i(a_i+b_i) - a_i b_i - z_i^2` est concave : son maximum sur un
// intervalle est au sommet rabattu, son minimum a une extremite.
// ---------------------------------------------------------------------------
inline i64 h_all_inside(const Box& A, const Box& B, const Box& Z) {
  i64 acc = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 av[2] = {A.lo[i], A.hi[i]};
    const i64 bv[2] = {B.lo[i], B.hi[i]};
    const i64 zv[2] = {Z.lo[i], Z.hi[i]};
    i64 best = 0;
    bool first = true;
    for (int u = 0; u < 2; ++u)
      for (int v = 0; v < 2; ++v)
        for (int w = 0; w < 2; ++w) {  // concave en z : minimum a une extremite
          const i64 val = zv[w] * (av[u] + bv[v]) - av[u] * bv[v] - zv[w] * zv[w];
          if (first || val < best) { best = val; first = false; }
        }
    acc += best;
  }
  return acc;
}

inline i64 h_any_upper(const Box& A, const Box& B, const Box& Z) {
  i64 acc = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 av[2] = {A.lo[i], A.hi[i]};
    const i64 bv[2] = {B.lo[i], B.hi[i]};
    i64 best = 0;
    bool first = true;
    for (int u = 0; u < 2; ++u)
      for (int v = 0; v < 2; ++v) {
        const i64 s = av[u] + bv[v], pr = av[u] * bv[v];
        i64 top = 0;
        bool ft = true;
        i64 cand[4];
        int nc = 0;
        cand[nc++] = Z.lo[i];
        cand[nc++] = Z.hi[i];
        const i64 mid = s / 2;
        for (i64 m = mid; m <= mid + 1; ++m)
          if (m > Z.lo[i] && m < Z.hi[i]) cand[nc++] = m;
        for (int c = 0; c < nc; ++c) {
          const i64 zi = cand[c];
          const i64 val = zi * s - pr - zi * zi;
          if (ft || val > top) { top = val; ft = false; }
        }
        if (first || top < best) { best = top; first = false; }  // minimax
      }
    acc += best;
  }
  return acc;
}

inline bool universal_over_rect(const Box& A, const Box& B, const P3& z, int q,
                                bool narrow, bool centre_only, bool corners_xi) {
  if (centre_only) {  // MUTANT : un seul point au lieu du produit entier.
    const P3 c{(A.lo[0] + A.hi[0]) / 2, (A.lo[1] + A.hi[1]) / 2, (A.lo[2] + A.hi[2]) / 2};
    return universal_witness(c, B, z, q, narrow);
  }
  const i64 h = h_min_over_boxes(A, B, z);
  if (h <= 0) return false;
  if (q == 2) return true;
  // `Xi` PAR LES SOMMETS, ET C'EST EXACT. Voir la note ci-dessus : `d x w` est
  // AFFINE en `a`, donc `Xi = |affine|^2` est CONVEXE et son maximum sur la
  // boite est atteint a un sommet. Huit sommets pour `a`, et pour chacun le
  // maximum exact sur `b` par separation d'axe : c'est le majorant le plus
  // serre possible, donc le `h` le plus grand qui reste rigoureux.
  i128 xi = 0;
  if (corners_xi) {
    xi = xi_max_over_boxes(A, B, z);  // enveloppe d'intervalles : sure mais lache
  } else {
    for (int m = 0; m < 8; ++m) {
      const P3 c{(m & 1) ? A.hi[0] : A.lo[0], (m & 2) ? A.hi[1] : A.lo[1],
                 (m & 4) ? A.hi[2] : A.lo[2]};
      const i128 v = xi_max_over_box(c, B, z, false);
      if (v > xi) xi = v;
    }
  }
  if (narrow) {
    const i64 hh = h * h;  // MUTANT : deborde.
    return (q == 3) ? ((i128)3 * hh > xi) : ((i128)2 * hh > xi);
  }
  const i128 hh = (i128)h * (i128)h;
  return (q == 3) ? (3 * hh > xi) : (2 * hh > xi);
}

// ---------------------------------------------------------------------------
// CORNER64 : `corner512_all_lane` SPECIALISE AU TEMOIN PONCTUEL.
//
// Ce n'est PAS un predicat de plus. C'est exactement `corner512_all_lane` dont
// on retire deux redondances de calcul, sans toucher a ce qu'il decide :
//
//   1. La boite du temoin est ici un POINT (`CZ.lo == CZ.hi`). Ses huit coins
//      coincident donc, et la boucle `kc` de `soc64_rect.hpp` evalue huit fois
//      le meme couple `(e,t)`. Il reste 8 x 8 = 64 couples DISTINCTS.
//   2. Les seize coins de `A` et de `B` ne dependent pas du site : ils sont
//      constants sur toute la descente d'un rectangle, alors que la fonction
//      generale les recalcule par `box_corner` a chaque appel.
//
// L'egalite avec la reference n'est pas un raisonnement laisse au lecteur : le
// mode `--compare-corner512` confronte les deux valeurs site par site et
// `corner64_desaccords` doit rester nul, sous peine de code de sortie 3.
//
// Ce que la fonction rend garde le statut etabli par l'en-tete de
// `soc64_rect.hpp` : la lane `ALL` du produit relaxe `Ebox x Tbox`, EXACTE sur
// l'enveloppe continue, et seulement SUFFISANTE sur les points reellement
// stockes — jamais `NONE` sur eux. C'est tout ce dont `h_coeur` a besoin.
// ---------------------------------------------------------------------------
struct Corner16 {
  i64 a[8][3];
  i64 b[8][3];
};

inline Corner16 corners_of_rect(const Box& A, const Box& B) {
  Corner16 c{};
  for (int k = 0; k < 8; ++k) {
    for (int i = 0; i < 3; ++i) {
      c.a[k][i] = (k & (1 << i)) ? A.hi[i] : A.lo[i];
      c.b[k][i] = (k & (1 << i)) ? B.hi[i] : B.lo[i];
    }
  }
  return c;
}

// Rend la lane `ALL` du produit relaxe, dans le meme codage que
// `mhgp3v::cone::kLaneNone/kLaneQ2/kLaneQ3/kLaneQ4` (0/2/3/4).
inline int corner64_all_lane(const Corner16& c, const P3& z, bool sept = false) {
  const i64 zc[3] = {z.x, z.y, z.z};
  int best = 4;
  const int na = sept ? 7 : 8;  // MUTANT : un coin de `A` manquant sur-certifie.
  for (int ka = 0; ka < na; ++ka) {
    i64 e[3];
    for (int i = 0; i < 3; ++i) e[i] = zc[i] - c.a[ka][i];
    const i64 e2 = e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    for (int kb = 0; kb < 8; ++kb) {
      i64 t[3];
      for (int i = 0; i < 3; ++i) t[i] = c.b[kb][i] - zc[i];
      const i64 h = e[0] * t[0] + e[1] * t[1] + e[2] * t[2];
      if (h <= 0) return 0;  // kLaneNone : le minimum du treillis, definitif
      const i64 x2 = t[0] * t[0] + t[1] * t[1] + t[2] * t[2];
      const i128 hh = (i128)h * (i128)h;
      const i128 ex = (i128)e2 * (i128)x2;
      // q3 : 4 H^2 > E X ; q4 : 3 H^2 > E X. Emboitement `W4 < W3 < W2`.
      int lane = 2;
      if (3 * hh > ex) lane = 4;
      else if (4 * hh > ex) lane = 3;
      if (lane < best) best = lane;
    }
  }
  return best;
}

// ---------------------------------------------------------------------------
// SEPARATION WSPD, ENTIERE ET CONSERVATRICE.
// On minore `d` par racine entiere INFERIEURE et on majore les rayons par
// racine entiere SUPERIEURE : le test est donc plus strict que le reel, donc
// sur. Les coordonnees sont DOUBLEES pour eviter les demi-entiers.
// ---------------------------------------------------------------------------
inline i64 isqrt_floor(i128 v) {
  if (v <= 0) return 0;
  i64 r = (i64)__builtin_sqrt((double)v);
  if (r < 0) r = 0;
  while (r > 0 && (i128)r * r > v) --r;
  while ((i128)(r + 1) * (r + 1) <= v) ++r;
  return r;
}

struct Sphere {
  i64 c2[3];  // centre double
  i64 r2;     // rayon double, majore
};

inline Sphere sphere_of(const Box& b) {
  Sphere s;
  i128 acc = 0;
  for (int i = 0; i < 3; ++i) {
    s.c2[i] = b.lo[i] + b.hi[i];
    const i64 e = b.hi[i] - b.lo[i];
    acc += (i128)e * e;
  }
  // LE VRAI PLAFOND, PAS `floor + 1`. Sur un carre parfait — et un singleton en
  // est un, avec `acc = 0` — `floor + 1` majorait d'une unite entiere pour rien,
  // ce qui retrecit tous les cœurs gratuitement. L'auditeur le releve deux fois.
  s.r2 = isqrt_floor(acc);
  if (s.r2 * s.r2 < acc) ++s.r2;
  return s;
}

inline bool separated(const Sphere& A, const Sphere& B, i64 s) {
  i128 acc = 0;
  for (int i = 0; i < 3; ++i) {
    const i64 e = B.c2[i] - A.c2[i];
    acc += (i128)e * e;
  }
  const i64 d = isqrt_floor(acc);  // minore
  const i64 rmax = A.r2 > B.r2 ? A.r2 : B.r2;
  return (i128)d - A.r2 - B.r2 >= (i128)s * rmax;
}

// ---------------------------------------------------------------------------
// LEDGER
// ---------------------------------------------------------------------------
struct Ledger {
  long long rectangles = 0;
  long long non_decides = 0;
  long long masse_totale = 0;      // somme |A||B| : doit valoir C(n,2)
  long long masse_non_decide = 0;
  long long survivantes[3] = {0, 0, 0};  // par lane q2/q3/q4
  long long coeur_total[3] = {0, 0, 0};
  long long coeur_non_vide[3] = {0, 0, 0};
  long long ha_total[3] = {0, 0, 0};
  long long hb_total[3] = {0, 0, 0};
  long long recouvrements = 0;  // sites comptes deux fois : DOIT rester nul
  long long cellules_max = 0;
  long long travail_h = 0;      // evaluations de `universal_witness`
  long long coeur_verifies = 0; // temoins de coeur re-verifies contre les vraies paires
  long long coeur_faux = 0;     // ... et pris en defaut : DOIT rester nul
  // NON-VACUITE. Le contre-audit du 15 aout a montre que treize portes vertes
  // n'avaient pas vu un double credit, faute de prouver qu'elles exercaient ce
  // qu'elles testaient. Chaque compteur ci-dessous est un plancher : une porte
  // qui le trouve nul se REFUSE au lieu de passer.
  long long bulk_credits = 0;   // voies rapides q2 reellement declenchees
  long long oracle_paires = 0;  // paires confrontees a la force brute
  long long oracle_faux_morts = 0;  // fermees a tort : DOIT rester nul
  long long oracle_ids_doubles = 0; // PointId credite deux fois : DOIT rester nul
  long long oracle_couverture_ko = 0; // paires vues != 1 fois : DOIT rester nul
  // HARNAIS APPARIE `corner512` (question Q21/Q22 a l'auditeur).
  long long c512_sites = 0;      // sites confrontes aux deux predicats
  long long c512_gagne[3] = {0, 0, 0};  // corner512 certifie ou ma borne echoue
  long long c512_perd[3] = {0, 0, 0};   // l'inverse
  long long c512_faux[3] = {0, 0, 0};   // corner512 certifie, la force brute refute
  long long c64_desaccords = 0;  // corner64 != corner512 : DOIT rester nul
  long long c64_appels = 0;      // sites decides par la specialisation ponctuelle
  long long travail_ha = 0;      // travail des SEULS postes h_a et h_b
  long long bulk_boule = 0;      // sous-arbres credites en O(1) par la boule
  long long elague_ext = 0;      // sous-arbres coupes par la boule exterieure
  long long lentille_somme = 0;  // candidats d'instruction, cumules
  long long lentille_max = 0;    // le pire cas, qui est ce qui compte
  long long lentille_ancres = 0;
  long long dual_ecarts = 0;     // dual-tree != jointure : DOIT rester nul
  long long dual_verifies = 0;   // points confrontes
  // Le coût REEL du mode `--vrai-vivant`, publie et non affirme. Le re-audit
  // demandait un budget `n |S|` : ces deux compteurs le rendent verifiable —
  // `vivant_paires` doit rester du meme ordre que `max_q survivantes`, et
  // `vivant_travail` ne doit pas depasser `n x vivant_paires`.
  long long vivant_paires = 0;    // paires effectivement balayees
  long long vivant_travail = 0;   // VISITES de `z`, unite comparable entre modes
  long long vivant_evals = 0;     // EVALUATIONS du predicat `(e,t)`, par mode
  long long vivant_degenerees = 0;    // paires `D = 0` rencontrees, tous lanes
  long long vivant_degen_lane[3] = {0, 0, 0};  // et par lane, pour corriger `S_q`
  // ---- SEEDS AIGUS : reference contre elagage, et le travail des deux.
  long long seed_total_ref = 0;     // seeds comptes par balayage complet
  long long seed_total_elag = 0;    // seeds comptes par descente elaguee
  long long seed_ancres = 0;        // ancres q3 survivantes instruites
  long long seed_ancres_sans = 0;   // ... dont AUCUN seed : le cas `two_lines`
  long long seed_travail_ref = 0;   // visites de `x` du balayage complet
  long long seed_travail_elag = 0;  // visites de `x` de la descente elaguee
  long long seed_blocs_elagues = 0; // sous-arbres rejetes en bloc
  long long seed_points_elagues = 0;  // points evites par ces rejets
  long long seed_ecarts = 0;        // reference != elagage : DOIT rester nul
  long long seed_juges = 0;         // certificats confrontes aux deux chemins
  long long seed_desaccords_certif = 0;  // O(1) != huit coins : DOIT rester nul
};

struct Rect {
  int u, v;
};

}  // namespace

int main(int argc, char** argv) {
  int n = 8000, smax = 11, sep = 8, cap = 512, judge = 0, min_rect = 0;
  bool cap_scission = true;     // raffiner plutot que refuser : voir l'en-tete
  bool refuse_doublons = false; // positions dupliquees : refus explicite
  bool seeds = false;           // contraction W-vivant -> seeds aigus
  bool seed_juge = false;       // confronter le certificat O(1) aux huit coins
  bool fixture_owner = false;   // le facteur deux des faces incidentes, refute
  long long seed = 3, coord = 0;
  CloudFamily family = CloudFamily::kUniform;
  Mutant mutant = Mutant::kNone;
  double min_ferme_q4 = -1.0;
  int oracle_n = 0;
  bool fixture = false;
  bool compare512 = false;
  bool core512 = false;
  bool ha_boule = false;
  bool ha_corner8 = false;
  // LA FUSION EST LE DEFAUT. C'est l'autorite AABB exacte — huit coins, donc le
  // maximum decidable depuis les boites — et le parcours le moins cher : un
  // seul appel par couple decide les trois lanes. Tout gain doit se mesurer
  // contre elle, jamais contre la version qui recalcule trois fois.
  bool ha_fusion = true;
  bool coeur_boule = false;
  bool ha_dual = false;
  long long dual_cutoff = 256;
  long long echantillon = 0;
  bool vrai_vivant = false;
  bool vivant_legacy = false;  // ablation : l'ancien balayage, trois passes
  bool cout_instruction = false;
  long long graine_ech = 0;
  bool ha_verifie = false;

  auto arg_ll = [](const char* s, long long* out) {
    const char* e = s + std::strlen(s);
    auto r = std::from_chars(s, e, *out);
    return r.ec == std::errc() && r.ptr == e;
  };

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    long long v = 0;
    auto eat = [&](const char* key, long long* dst) {
      const std::string p = std::string(key) + "=";
      if (a.rfind(p, 0) != 0) return false;
      if (!arg_ll(a.c_str() + p.size(), &v)) {
        std::fprintf(stderr, "REFUS : valeur non entiere pour %s\n", key);
        std::exit(2);
      }
      *dst = v;
      return true;
    };
    long long tmp = 0;
    // BORNER AVANT DE CASTER. Le re-audit a montre que `--points=4294967298` se
    // repliait sur `n=2` et que `--oracle=-1` etait accepte : la conversion
    // vers `int` avait lieu AVANT toute validation. On valide en `long long`,
    // puis seulement on caste.
    auto borne = [&](const char* key, long long lo, long long hi) {
      if (v < lo || v > hi) {
        std::fprintf(stderr, "REFUS : %s=%lld hors de [%lld,%lld]\n", key, v, lo, hi);
        std::exit(2);
      }
    };
    if (eat("--points", &tmp)) { borne("--points", 1, 2000000); n = (int)tmp; continue; }
    if (eat("--smax", &tmp)) { borne("--smax", 3, 32); smax = (int)tmp; continue; }
    if (eat("--separation", &tmp)) { borne("--separation", 1, 64); sep = (int)tmp; continue; }
    if (eat("--cap-cellule", &tmp)) { borne("--cap-cellule", 1, 1000000); cap = (int)tmp; continue; }
    if (a == "--seeds") { seeds = true; continue; }
    if (a == "--verifie-seed") { seeds = true; seed_juge = true; continue; }
    if (a == "--refuse-doublons") { refuse_doublons = true; continue; }
    if (a == "--cap=scission") { cap_scission = true; continue; }
    if (a == "--cap=refus") { cap_scission = false; continue; }
    if (eat("--seed", &seed)) continue;
    // PROFIL u16, ANNONCE DONC IMPOSE. Le re-audit a trouve qu'un
    // `--coord=2147483647` etait accepte, debordait sous UBSan et pouvait
    // fermer a tort. Le domaine annonce est desormais garde.
    if (eat("--coord", &coord)) { borne("--coord", 1, 65535); continue; }
    if (eat("--juge", &tmp)) { borne("--juge", 0, 200); judge = (int)tmp; continue; }
    if (eat("--oracle", &tmp)) { borne("--oracle", 0, 200); oracle_n = (int)tmp; continue; }
    if (a == "--fixture=coeur5") { fixture = true; continue; }
    if (a == "--fixture=owner-porteurs") { fixture_owner = true; continue; }
    if (a == "--compare-corner512") { compare512 = true; continue; }
    if (a == "--ha=boule") { ha_boule = true; ha_fusion = false; continue; }
    if (a == "--ha=corner8") { ha_corner8 = true; ha_fusion = false; continue; }
    if (a == "--ha=fusion") { ha_fusion = true; continue; }
    // Les trois autres chemins desactivent la fusion : ce sont des ablations.
    if (a == "--ha=jointure") { ha_fusion = false; continue; }
    if (a == "--coeur=boule") { coeur_boule = true; continue; }
    if (a == "--ha=dualtree") { ha_dual = true; ha_fusion = false; continue; }
    // PORTE METAMORPHIQUE : le dual-tree pretend rendre EXACTEMENT les memes
    // `h_a` que la jointure ponctuelle a huit coins. Ce mode calcule les deux et
    // les confronte point par point ; un seul ecart refuse la campagne.
    if (a == "--verifie-jointure") { ha_verifie = true; ha_dual = true; ha_fusion = false; continue; }
    if (a == "--vrai-vivant") { vrai_vivant = true; continue; }
    if (a == "--vivant=legacy") { vrai_vivant = true; vivant_legacy = true; continue; }
    if (a == "--vivant=fusion") { vrai_vivant = true; vivant_legacy = false; continue; }
    if (a == "--cout-instruction") { cout_instruction = true; vrai_vivant = true; continue; }
    if (eat("--graine-echantillon", &tmp)) { graine_ech = tmp; continue; }
    if (eat("--echantillon", &tmp)) { borne("--echantillon", 0, 200000); echantillon = tmp; continue; }
    if (eat("--dual-cutoff", &tmp)) { borne("--dual-cutoff", 0, 100000); dual_cutoff = tmp; continue; }
    // Les deux orthographes selectionnent le MEME predicat : `corner64` est
    // `corner512` prive de ses huit coins de temoin confondus. La porte
    // d'egalite du harnais apparie en est la preuve executable.
    if (a == "--coeur=corner512" || a == "--coeur=corner64") { core512 = true; continue; }
    if (eat("--min-rectangles", &tmp)) { min_rect = (int)tmp; continue; }
    if (a.rfind("--min-ferme-q4=", 0) == 0) {
      min_ferme_q4 = std::atof(a.c_str() + 15);
      continue;
    }
    if (a.rfind("--family=", 0) == 0) {
      const std::string f = a.substr(9);
      // LES TROIS FAMILLES QUE J'AVAIS OMISES. Le generateur les produit depuis
      // toujours ; c'est ce probe qui n'en acceptait que trois, et l'audit
      // positif du `00cf78c` a raison de dire que mes rampes ne prouvaient donc
      // rien sur les balayages ni sur la contre-famille. `two_lines` est
      // decisive : elle porte une masse universelle QUADRATIQUE avec ZERO
      // porteur aigu q3/q4, donc elle refute toute phrase du genre « le
      // `W`-vivant ne devient pas quadratique ».
      if (f == "uniform") family = CloudFamily::kUniform;
      else if (f == "eight_clusters") family = CloudFamily::kEightClusters;
      else if (f == "terrain") family = CloudFamily::kTerrain;
      else if (f == "scanline_single_pass") family = CloudFamily::kScanlineSinglePass;
      else if (f == "scanline_overlap_multiecho") family = CloudFamily::kScanlineOverlapMultiecho;
      else if (f == "two_lines") family = CloudFamily::kTwoLines;
      else { std::fprintf(stderr, "REFUS : famille inconnue %s\n", f.c_str()); return 2; }
      continue;
    }
    if (a.rfind("--inject=", 0) == 0) {
      const std::string m = a.substr(9);
      if (m == "coeur-intervalle-xi") mutant = Mutant::kIntervalXi;
      else if (m == "largeur-i64") mutant = Mutant::kNarrowI64;
      else if (m == "coeur-centre-seul") mutant = Mutant::kCoreCentreOnly;
      else if (m == "seuil-decale") mutant = Mutant::kThresholdOff;
      else if (m == "bulk-sans-masque") mutant = Mutant::kBulkSansMasque;
      else if (m == "oublie-b") mutant = Mutant::kDropB;
      else if (m == "corner64-sept-coins") mutant = Mutant::kCorner64Sept;
      else if (m == "dual-sans-masque") mutant = Mutant::kDualSansMasque;
      else if (m == "vivant-sans-extinction") mutant = Mutant::kVivantSansExtinction;
      else if (m == "vivant-lane-unique") mutant = Mutant::kVivantLaneUnique;
      else { std::fprintf(stderr, "REFUS : mutant inconnu %s\n", m.c_str()); return 2; }
      continue;
    }
    std::fprintf(stderr, "REFUS : argument inconnu %s\n", a.c_str());
    return 2;
  }

  if (n < 2 || n > 65535) { std::fprintf(stderr, "REFUS : n hors profil u16\n"); return 2; }
  if (smax < 4 || smax > 32) { std::fprintf(stderr, "REFUS : smax hors domaine\n"); return 2; }
  if (sep < 1 || sep > 64) { std::fprintf(stderr, "REFUS : separation hors domaine\n"); return 2; }
  if (judge > 400) { std::fprintf(stderr, "REFUS : juge non borne\n"); return 2; }
  if (oracle_n > 200) { std::fprintf(stderr, "REFUS : oracle non borne\n"); return 2; }
  // Les deux mutants `vivant-*` ne touchent QUE le balayage fusionne du
  // `W`-vivant, hors du chemin que le juge par force brute inspecte. Leur juge
  // est l'autre balayage : `--vivant=legacy` calcule le meme compte par un
  // chemin independant, et `audits/check_vivant_balayages.py` confronte les
  // deux. Exiger `--juge` ici refuserait le seul montage qui les tue.
  const bool mutant_vivant = (mutant == Mutant::kVivantSansExtinction ||
                              mutant == Mutant::kVivantLaneUnique);
  if (mutant_vivant && !vrai_vivant) {
    std::fprintf(stderr, "REFUS : mutant `vivant-*` sans --vrai-vivant\n");
    return 2;
  }
  if (mutant_vivant && vivant_legacy) {
    std::fprintf(stderr, "REFUS : mutant `vivant-*` injecte dans son propre juge\n");
    return 2;
  }
  if (mutant != Mutant::kNone && !mutant_vivant && judge <= 0) {
    std::fprintf(stderr, "REFUS : un mutant sans juge ne prouve rien\n");
    return 2;
  }

  // ---- FIXTURE PERMANENTE DU CONTRE-AUDIT (section 5).
  // `A` et `B` singletons ; un sous-arbre `Z` de CINQ PointId tous strictement
  // dans `W2` et groupes ; aucun autre site. A `s_max=11`, `h_2=10`. Sans le
  // masque de lanes, la voie rapide creditait cinq puis les feuilles cinq de
  // plus : `hcore2=10`, et l'ancre VIVANTE etait fermee. La fixture exige
  // `hcore2=5`, l'ancre survivante, et `bulk_credits>=1` — sans ce dernier
  // plancher elle serait verte meme si la voie rapide ne se declenchait jamais,
  // c'est-a-dire vacue comme celles que le contre-audit a prises en defaut.
  const int coord_used =
      coord > 0 ? (int)coord : mhgp3v::cloud_family_default_coord(family, n);
  // ---- LE FACTEUR DEUX DES FACES INCIDENTES, REFUTE PAR UN TETRAEDRE ENTIER.
  //
  // On aurait pu croire que les DEUX faces incidentes a l'arete owner d'un
  // tetraedre bien centre sont aigues, et en tirer `N4_event <= 2 r4 C4_carrier`
  // avec un facteur deux garanti. C'est faux, et le contre-exemple de
  // NOTE_AUDITEUR_ORDRE_EXECUTION_APRES_5CE2634 le montre en coordonnees
  // entieres — verifie ici, pas rapporte :
  //
  //   p0=(6,2,5)  p1=(0,3,3)  p2=(1,4,6)  p3=(5,3,1)
  //
  // Aretes au carre : 41, 30, 18, 11, 29, 42. L'owner `p2p3` vaut 42, atteint
  // UNE SEULE FOIS, donc il est unique sans tie-break. Le circumcentre est
  // rationnel exact `(83/26, 81/26, 97/26)`, equidistant a `7259/676` des
  // quatre sommets : c'est bien un tetraedre non degenere.
  //
  // Or des deux faces incidentes a `p2p3`, la face `(p0,p2,p3)` est aigue et la
  // face `(p1,p2,p3)` ne l'est PAS. Le vrai centre n'est donc propose que par
  // UN seul `Q4Seed3`. La borne correcte est `N4_event <= R4_bundle`, sans le
  // facteur deux.
  //
  // Le tetraedre REGULIER quantifie sert de contraste : ses quatre faces sont
  // aigues, donc les deux incidentes le sont. La fixture porte les deux, et
  // c'est le CONTRASTE qui fait qu'elle n'est pas vacue.
  if (fixture_owner) {
    struct Tetra { const char* nom; int p[4][3]; };
    const Tetra ts[2] = {
        {"auditeur", {{6, 2, 5}, {0, 3, 3}, {1, 4, 6}, {5, 3, 1}}},
        // Tetraedre regulier a coordonnees entieres : quatre sommets alternes
        // du cube `{0,2}^3`, aretes toutes egales a 8.
        {"regulier", {{0, 0, 0}, {2, 2, 0}, {2, 0, 2}, {0, 2, 2}}}};
    int total_aigues_incidentes = 0;
    for (const Tetra& t : ts) {
      auto D2 = [&](int i, int j) {
        i64 s = 0;
        for (int k = 0; k < 3; ++k) {
          const i64 d = (i64)t.p[i][k] - t.p[j][k];
          s += d * d;
        }
        return s;
      };
      i64 mx = -1;
      int oi = -1, oj = -1, mult = 0;
      for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j) {
          const i64 d = D2(i, j);
          if (d > mx) { mx = d; oi = i; oj = j; mult = 1; }
          else if (d == mx) ++mult;
        }
      auto aigue = [&](int i, int j, int k) {
        const int f[3][3] = {{i, j, k}, {j, i, k}, {k, i, j}};
        for (int s = 0; s < 3; ++s) {
          i64 h = 0;
          for (int c = 0; c < 3; ++c)
            h += ((i64)t.p[f[s][1]][c] - t.p[f[s][0]][c]) *
                 ((i64)t.p[f[s][2]][c] - t.p[f[s][0]][c]);
          if (h <= 0) return false;  // STRICTE : l'angle droit n'est pas aigu
        }
        return true;
      };
      int inc = 0;
      for (int k = 0; k < 4; ++k) {
        if (k == oi || k == oj) continue;
        if (aigue(oi, oj, k)) ++inc;
      }
      total_aigues_incidentes += inc;
      std::printf("owner tetra=%s arete_max=%lld owner=(%d,%d) multiplicite=%d "
                  "faces_incidentes_aigues=%d\n",
                  t.nom, (long long)mx, oi, oj, mult, inc);
    }
    // LE PLANCHER DE NON-VACUITE : `1` pour l'auditeur, `2` pour le regulier.
    // Un total de `3` atteste que la fixture voit bien les deux regimes ; un
    // total de `4` signifierait que le facteur deux est revenu.
    if (total_aigues_incidentes != 3) {
      std::fprintf(stderr,
                   "PLANCHER : faces incidentes aigues = %d, attendu 3 (1 + 2)\n",
                   total_aigues_incidentes);
      return 3;
    }
    std::printf("owner total_faces_incidentes_aigues=%d facteur_deux=REFUTE\n",
                total_aigues_incidentes);
    return 0;
  }
  std::vector<P3> pts;
  if (fixture) {
    pts = {P3{100, 500, 500}, P3{900, 500, 500},
           P3{490, 500, 500}, P3{495, 500, 500}, P3{500, 500, 500},
           P3{505, 500, 500}, P3{510, 500, 500}};
    n = (int)pts.size();
    smax = 11;
  } else {
    pts = mhgp3v::make_family_cloud(family, n, coord_used, seed);
  }
  if ((int)pts.size() != n) { std::fprintf(stderr, "REFUS : nuage non genere\n"); return 2; }
  // LE PROFIL EST VERIFIE SUR LES POINTS, PAS SEULEMENT SUR LA CLI. Une famille
  // pourrait produire hors domaine sans qu'aucun argument ne l'annonce ; les
  // largeurs prouvees de tout le fichier reposent sur `[0,65535]`.
  for (int i = 0; i < n; ++i) {
    const i64 c[3] = {pts[(size_t)i].x, pts[(size_t)i].y, pts[(size_t)i].z};
    for (int k = 0; k < 3; ++k)
      if (c[k] < 0 || c[k] > 65535) {
        std::fprintf(stderr, "REFUS : profil u16 viole, point %d composante %d = %lld\n",
                     i, k, (long long)c[k]);
        return 2;
      }
  }

  // ---- Octree comprime de Morton, partage par les trois lanes.
  std::vector<unsigned long long> keys(n);
  std::vector<int> order(n);
  for (int i = 0; i < n; ++i) {
    keys[i] = mhgp3v::wf_morton48(pts[i].x, pts[i].y, pts[i].z);
    order[i] = i;
  }
  std::sort(order.begin(), order.end(), [&](int i, int j) {
    return keys[i] != keys[j] ? keys[i] < keys[j] : i < j;
  });
  std::vector<unsigned long long> sorted_keys(n);
  std::vector<P3> sorted_pts(n);
  for (int i = 0; i < n; ++i) { sorted_keys[i] = keys[order[i]]; sorted_pts[i] = pts[order[i]]; }
  std::vector<WfNode> nodes = mhgp3v::wf_build(sorted_keys);
  {
    // `wf_tight_boxes` prend des triplets bruts ; on lui donne la meme vue.
    std::vector<std::array<long long, 3>> raw(n);
    for (int i = 0; i < n; ++i)
      raw[(size_t)i] = {sorted_pts[i].x, sorted_pts[i].y, sorted_pts[i].z};
    mhgp3v::wf_tight_boxes(&nodes, raw);
  }

  // ---- HANDLE. `h >= 0` designe le noeud interne `nodes[h]` ; `h < 0` designe
  // la FEUILLE d'indice trie `-1-h`, qui ne figure pas dans `nodes`. Toute la
  // recursion WSPD parle en handles, jamais en indices de tableau.
  auto h_first = [&](int h) { return h >= 0 ? nodes[h].first : -1 - h; };
  auto h_last = [&](int h) { return h >= 0 ? nodes[h].last : -1 - h; };
  auto h_pop = [&](int h) { return h_last(h) - h_first(h) + 1; };
  auto h_leaf = [&](int h) { return h < 0; };
  auto h_box = [&](int h) {
    return h >= 0 ? box_of(nodes[h]) : box_of_point(sorted_pts[-1 - h]);
  };
  auto h_sphere = [&](int h) { return sphere_of(h_box(h)); };

  std::vector<Rect> rects;
  {
    std::vector<Rect> stack;
    for (size_t i = 0; i < nodes.size(); ++i)
      stack.push_back({nodes[i].left, nodes[i].right});
    if (nodes.empty() && n == 1) { /* nuage a un point : aucune paire */ }
    while (!stack.empty()) {
      const Rect r = stack.back();
      stack.pop_back();
      // LE CAP EST UNE CONDITION D'ACCEPTATION, PAS UN REJET A POSTERIORI.
      // En mode `scission`, un rectangle separe mais trop gros continue de se
      // raffiner : le recouvrement est le meme, les sous-rectangles sont
      // re-testes pour la separation, et l'arret sur deux feuilles borne la
      // recursion — une feuille est un point, donc toujours sous le cap.
      const bool sous_cap = cap_scission
                                ? (h_pop(r.u) <= cap && h_pop(r.v) <= cap)
                                : true;
      if (sous_cap && separated(h_sphere(r.u), h_sphere(r.v), sep)) {
        rects.push_back(r);
        continue;
      }
      const bool u_int = !h_leaf(r.u), v_int = !h_leaf(r.v);
      if (!u_int && !v_int) { rects.push_back(r); continue; }  // deux feuilles
      const bool split_u = u_int && (!v_int || h_pop(r.u) >= h_pop(r.v));
      if (split_u) {
        stack.push_back({nodes[r.u].left, r.v});
        stack.push_back({nodes[r.u].right, r.v});
      } else {
        stack.push_back({r.u, nodes[r.v].left});
        stack.push_back({r.u, nodes[r.v].right});
      }
    }
  }

  Ledger L;
  L.rectangles = (long long)rects.size();
  const int h_q[3] = {death_threshold(smax, 2), death_threshold(smax, 3),
                      death_threshold(smax, 4)};
  const bool narrow = (mutant == Mutant::kNarrowI64);
  const bool centre_only = (mutant == Mutant::kCoreCentreOnly);
  const bool corners_xi = (mutant == Mutant::kIntervalXi);
  const int seuil_delta = (mutant == Mutant::kThresholdOff) ? 1 : 0;

  std::vector<int> ha(0), hb(0);
  // L'histogramme doit couvrir `h_2 = s_max - 1`, soit 31 a `s_max=32`. Seize
  // cases ne couvraient que le domaine effectif, plus etroit que le domaine
  // annonce par la CLI : le contre-audit avait raison de le relever.
  const int kHisto = smax + 2;
  std::vector<int> histo((size_t)kHisto, 0);
  // `--oracle=N` EST UNE BORNE, PAS UN INTERRUPTEUR. Le re-audit a montre que
  // `--points=201 --oracle=1` lancait l'oracle et sortait 0 alors que la limite
  // annoncee est 200. `N` borne desormais reellement `n`, et
  // `--compare-corner512`, qui active aussi l'oracle, recoit le meme cap.
  // Le coût du vrai-vivant est desormais `survivantes x n`, pas `C(n,2) x n` :
  // il suit le RESIDUEL, donc il tient jusqu'a des nuages ou l'ancienne
  // enumeration etait impensable. La borne reste large mais explicite.
  if (vrai_vivant && n > 40000) {
    std::fprintf(stderr, "REFUS : --vrai-vivant non borne (%d points)\n", n);
    return 2;
  }
  // Le balayage legacy ne connait qu'une lane a la fois : il ne peut pas
  // alimenter le compteur de lentille, qui n'a de sens que sur la lane q4. Le
  // combiner avec `--cout-instruction` rendrait un `lentille_ancres=0` muet.
  if (cout_instruction && vivant_legacy) {
    std::fprintf(stderr, "REFUS : --cout-instruction exige le balayage fusionne\n");
    return 2;
  }
  const bool oracle = (oracle_n > 0) || fixture || compare512;
  if (oracle_n > 0 && n > oracle_n) {
    std::fprintf(stderr, "REFUS : n=%d depasse la borne --oracle=%d\n", n, oracle_n);
    return 2;
  }
  if (oracle && n > 200) {
    std::fprintf(stderr, "REFUS : oracle borne a 200 points, n=%d\n", n);
    return 2;
  }
  std::vector<int> core_ids[3];  // LEDGER PAR LANE, exige avant tout bulk q3/q4
  long long vrai_vivantes[3] = {0, 0, 0};
  std::vector<unsigned char> vu((size_t)n, 0);
  const long long npairs = (long long)n * (n - 1) / 2;
  std::vector<unsigned char> ferme;      // par PairId et par lane
  std::vector<unsigned> couverture;
  if (oracle) {
    if (npairs > 40000000LL) { std::fprintf(stderr, "REFUS : oracle non borne\n"); return 2; }
    ferme.assign((size_t)npairs * 3, 0);
    couverture.assign((size_t)npairs, 0);
  }
  auto pair_idx = [&](int i, int j) {
    if (i > j) { const int k = i; i = j; j = k; }
    return (long long)i * (2LL * n - i - 1) / 2 + (j - i - 1);
  };

  for (const Rect& r : rects) {
    const int ua = h_first(r.u), ub = h_last(r.u);
    const int va = h_first(r.v), vb = h_last(r.v);
    const int na = ub - ua + 1, nb = vb - va + 1;
    L.masse_totale += (long long)na * nb;
    // LE COMPTEUR AVANT LE `continue`, ET C'ETAIT UN VRAI DEFAUT DE MESURE.
    // Il etait incremente APRES le rejet des rectangles capes, donc il ne
    // pouvait structurellement jamais depasser le cap. Le recu du 15 aout s'en
    // servait pour conclure « le cap n'est pas en cause : cellule_max = 482
    // reste sous 512 » — un raisonnement circulaire, et faux : a
    // `terrain, n=32000, s=8`, cinquante-deux rectangles capes sur 5,6 millions
    // portent SOIXANTE-QUINZE POUR CENT du residuel.
    if (na > L.cellules_max) L.cellules_max = na;
    if (nb > L.cellules_max) L.cellules_max = nb;
    if (na > cap || nb > cap) {
      // En mode `scission`, la construction garantit `na, nb <= cap`. Y arriver
      // signifierait que la condition d'acceptation et ce test divergent : ce
      // n'est pas un rectangle a compter, c'est un invariant casse.
      if (cap_scission) {
        std::fprintf(stderr,
                     "PLANCHER : rectangle %dx%d hors cap=%d en mode scission\n",
                     na, nb, cap);
        return 3;
      }
      ++L.non_decides;
      L.masse_non_decide += (long long)na * nb;
      for (int q = 0; q < 3; ++q) L.survivantes[q] += (long long)na * nb;
      continue;
    }

    const Box BA = h_box(r.u), BB = h_box(r.v);
    // Seize coins, une fois par rectangle : ils ne dependent d'aucun site.
    const Corner16 rect_corners = corners_of_rect(BA, BB);
    // La boule du cœur, en forme close, une fois par rectangle et par lane.
    const Sphere SphA = h_sphere(r.u), SphB = h_sphere(r.v);
    i64 Mb[3];
    i64 R4b[3] = {0, 0, 0};
    i64 R4ext = 0;
    {
      i128 acc = 0;
      for (int k = 0; k < 3; ++k) {
        Mb[k] = SphA.c2[k] + SphB.c2[k];
        const i64 e = SphB.c2[k] - SphA.c2[k];
        acc += (i128)e * (i128)e;
      }
      const i64 dd = isqrt_floor(acc);  // minorant, pour les cœurs
      for (int q = 0; q < 3; ++q)
        R4b[q] = mhgp3v::corebl::core_ball_radius4(dd, SphA.r2, SphB.r2, q + 2);
      // La boule EXTERIEURE veut le sens inverse : un MAJORANT de la distance.
      const i64 ddc = mhgp3v::corebl::isqrt_ceil_i128(acc);
      R4ext = mhgp3v::corebl::outer_ball_radius4(ddc, SphA.r2, SphB.r2);
    }

    // ---- h_coeur : temoins universels du rectangle, HORS `A` et HORS `B`.
    //
    // MASQUE DE LANES PAR FRAME. Le contre-audit du 15 aout a trouve ici un P0 :
    // un noeud credite EN BLOC pour q2 etait ensuite redescendu pour q3/q4, et
    // ses feuilles recreditaient q2 une seconde fois. Le meme `PointId` comptait
    // donc deux fois, `hcore[0]` sur-comptait, et des ancres VIVANTES etaient
    // fermees — 573 faux rejets au moins sur `uniform,n=160`.
    //
    // La pile transporte desormais `(noeud, masque)`. Un credit en bloc pour une
    // lane EFFACE cette lane du masque transmis aux enfants ; une feuille
    // n'incremente que les lanes encore actives. Les autres lanes continuent de
    // descendre normalement.
    //
    // L'elagage porte sur `max_z min_{a,b} H`, valide pour les trois lanes
    // puisque les fuseaux sont emboites `W_4 < W_3 < W_2`.
    int hcore[3] = {0, 0, 0};
    for (int q = 0; q < 3; ++q) core_ids[q].clear();
    {
      struct Frame { int node; int mask; };
      std::vector<Frame> st;
      if (!nodes.empty()) st.push_back({0, 7});
      else if (n == 1) st.push_back({-1, 7});
      while (!st.empty()) {
        const Frame f = st.back();
        st.pop_back();
        int m = f.mask;
        for (int q = 0; q < 3; ++q)
          if (hcore[q] >= h_q[q]) m &= ~(1 << q);  // lane saturee
        if (m == 0) continue;
        const Box Z = h_box(f.node);
        ++L.travail_h;
        // CERTIFICAT `NONE` EN O(1) : hors de la boule exterieure, aucune paire
        // du rectangle n'a de temoin ici, pour aucune lane.
        // LA BOULE EXTERIEURE EST DOMINEE, ET LE COMPTEUR LE PROUVE. Placee
        // AVANT `h_any_upper` elle semblait couper des millions de sous-arbres ;
        // placee APRES, elle en coupe EXACTEMENT ZERO. `elague_ext` mesure donc
        // le gain NET, et il est nul — voir l'en-tete de `spindle_core_ball.hpp`
        // pour la raison : les deux certificats ne repondent pas a la meme
        // question. Le test est conserve pour que la refutation reste
        // executable, jamais parce qu'il servirait.
        if (h_any_upper(BA, BB, Z) <= 0) continue;  // aucun temoin ici
        if (coeur_boule &&
            mhgp3v::corebl::ball_disjoint_box(Mb, R4ext, Z.lo, Z.hi)) {
          ++L.elague_ext;
          continue;
        }
        const bool leaf = h_leaf(f.node);
        int child = m;
        // ---- CREDIT EN BLOC PAR LA BOULE, POUR LES TROIS LANES (P1.8).
        //
        // Jusqu'ici seule q2 avait une voie rapide : q3 et q4 descendaient
        // jusqu'aux feuilles a chaque rectangle, et c'est la que se trouve
        // l'essentiel du travail. La boule du cœur donne le certificat `ALL`
        // manquant en `O(1)` par sous-arbre, avec le meme masque de lanes.
        //
        // Un echec du test boule est `UNKNOWN`, jamais `NONE` : la descente
        // continue, et `h_any_upper` reste le seul certificat `NONE`.
        if (!leaf && coeur_boule) {
          const int fi = nodes[f.node].first, la = nodes[f.node].last;
          const bool touche = !(la < ua || fi > ub) || !(la < va || fi > vb);
          if (!touche) {
            for (int q = 0; q < 3; ++q) {
              if (!(child & (1 << q)) || R4b[q] <= 0) continue;
              if (!mhgp3v::corebl::ball_contains_box(Mb, R4b[q], Z.lo, Z.hi)) continue;
              hcore[q] += (la - fi + 1);
              if (hcore[q] > h_q[q]) hcore[q] = h_q[q];
              ++L.bulk_boule;
              if (oracle) for (int k = fi; k <= la; ++k) core_ids[q].push_back(k);
              if (mutant != Mutant::kBulkSansMasque) child &= ~(1 << q);
            }
            if (child == 0) continue;  // les trois lanes creditees : rien a descendre
          }
        }
        if (!leaf && (m & 1) && (child & 1) && h_all_inside(BA, BB, Z) > 0) {
          // VOIE RAPIDE q2 : tout le noeud est temoin. On ne credite que s'il
          // ne chevauche ni `A` ni `B`, sans quoi la disjonction tomberait.
          const int fi = nodes[f.node].first, la = nodes[f.node].last;
          const bool touche = !(la < ua || fi > ub) || !(la < va || fi > vb);
          if (!touche) {
            hcore[0] += (la - fi + 1);
            if (hcore[0] > h_q[0]) hcore[0] = h_q[0];
            ++L.bulk_credits;
            if (oracle) for (int k = fi; k <= la; ++k) core_ids[0].push_back(k);
            // LA REPARATION : les enfants ne recreditent plus q2. Le mutant
            // `bulk-sans-masque` reproduit exactement le defaut du 15 aout.
            if (mutant != Mutant::kBulkSansMasque) child &= ~1;
          }
        }
        if (leaf) {
          const int i = -1 - f.node;
          if (i >= ua && i <= ub) continue;  // disjonction avec `A`
          if (i >= va && i <= vb) continue;  // disjonction avec `B`
          if (core512) {
            // MESURE, PAS SUBSTITUTION. `corner64_all_lane` rend la lane `ALL`
            // du produit relaxe, EXACTE sur l'enveloppe continue des deux
            // boites — donc la meilleure decision qu'aucune borne tiree des
            // seules AABB ne peut depasser. Ce mode sert a chiffrer l'effet sur
            // la fermeture, question Q22 posee a l'auditeur ; la substitution
            // en production attend la reponse a Q21.
            //
            // Les seize coins sont hisses HORS de la descente : ils ne
            // dependent que du rectangle.
            ++L.c64_appels;
            const int lane = corner64_all_lane(rect_corners, sorted_pts[i]);
            if (lane < 2) continue;
            if ((m & 1) && hcore[0] < h_q[0]) { ++hcore[0]; if (oracle) core_ids[0].push_back(i); }
            if ((m & 2) && hcore[1] < h_q[1] && lane >= 3) ++hcore[1];
            if ((m & 4) && hcore[2] < h_q[2] && lane >= 4) ++hcore[2];
            continue;
          }
          const i64 hh = h_min_over_boxes(BA, BB, sorted_pts[i]);
          if (hh <= 0) continue;
          if ((m & 1) && hcore[0] < h_q[0]) { ++hcore[0]; if (oracle) core_ids[0].push_back(i); }
          if ((m & 6) == 0) continue;
          i128 xi = 0;
          if (corners_xi) {
            xi = xi_max_over_boxes(BA, BB, sorted_pts[i]);
          } else {
            for (int c = 0; c < 8; ++c) {
              const P3 cp{(c & 1) ? BA.hi[0] : BA.lo[0], (c & 2) ? BA.hi[1] : BA.lo[1],
                          (c & 4) ? BA.hi[2] : BA.lo[2]};
              const i128 v = xi_max_over_box(cp, BB, sorted_pts[i], false);
              if (v > xi) xi = v;
            }
          }
          const i128 h2 = (i128)hh * (i128)hh;
          if ((m & 2) && hcore[1] < h_q[1] && 3 * h2 > xi) ++hcore[1];
          if ((m & 4) && hcore[2] < h_q[2] && 2 * h2 > xi) ++hcore[2];
          continue;
        }
        st.push_back({nodes[f.node].left, child});
        st.push_back({nodes[f.node].right, child});
      }
      for (int q = 0; q < 3; ++q) {
        L.coeur_total[q] += hcore[q];
        if (hcore[q] > 0) ++L.coeur_non_vide[q];
      }
    }

    // ---- h_a et h_b, ecretes eux aussi.
    //
    // DEUX CHEMINS, ET C'EST LA REPONSE A Q23. Le chemin par defaut est
    // l'auto-jointure ponctuelle `O(|A|^2)` que le contre-audit relevait. Le
    // chemin `--ha=boule` calcule la BOULE D'APEX du couple `(a, B)` — forme
    // close, voir `spindle_core_ball.hpp` — puis compte les points de `A`
    // qu'elle contient par une DESCENTE du seul sous-arbre `A`, avec credit en
    // bloc et elagage. Le comptage s'arrete des que le seuil `h_q` est atteint.
    //
    // La boule est INSCRITE dans le cone d'apex, donc elle en rate les points
    // proches des parois : `h_a` par boule MINORE `h_a` exact. Le filtre reste
    // donc fail-open, et l'ecart est mesure par `ha_manque`.
    ha.assign((size_t)na * 3, 0);
    hb.assign((size_t)nb * 3, 0);
    auto compte_boule = [&](const P3& p, const i64 udv[3], i64 rSelf2, i64 rAutre2,
                            int q, int need, int noeud, int lo_i, int hi_i) {
      const i64 pa[3] = {p.x, p.y, p.z};
      const auto sph = mhgp3v::corebl::apex_ball_of(pa, udv, rSelf2, rAutre2, q + 2);
      if (sph.vide) return 0;
      int c = 0;
      std::vector<int> st;
      st.push_back(noeud);
      while (!st.empty() && c < need) {
        const int h = st.back();
        st.pop_back();
        const Box bx = h_box(h);
        ++L.travail_h;
        ++L.travail_ha;
        if (mhgp3v::corebl::apex_disjoint_box(sph, bx.lo, bx.hi)) continue;
        const int fi = h_first(h), la = h_last(h);
        if (fi > hi_i || la < lo_i) continue;
        // Credit en bloc : le sous-arbre entier est dans la boule et ne
        // contient pas `p` lui-meme. Sa POPULATION suffit, en O(1).
        const bool contient_p = false;  // rempli ci-dessous pour les feuilles
        (void)contient_p;
        if (mhgp3v::corebl::apex_contains_box(sph, bx.lo, bx.hi)) {
          int pop = la - fi + 1;
          // `p` est exclu de son propre compte ; il ne peut etre que dans ce
          // sous-arbre si son indice y tombe.
          const int ip = &p - &sorted_pts[0];
          if (ip >= fi && ip <= la) --pop;
          c += pop;
          continue;
        }
        if (h_leaf(h)) {
          const int i = -1 - h;
          if (i == (int)(&p - &sorted_pts[0])) continue;
          const i64 zz[3] = {sorted_pts[i].x, sorted_pts[i].y, sorted_pts[i].z};
          if (mhgp3v::corebl::apex_contains_pt(sph, zz)) ++c;
          continue;
        }
        st.push_back(nodes[h].left);
        st.push_back(nodes[h].right);
      }
      return c > need ? need : c;
    };
    // ---- AUTO-JOINTURE DUAL-TREE (P1.10 du re-audit).
    //
    // Elle calcule EXACTEMENT les memes `h_a` que la jointure ponctuelle a huit
    // coins, et rien d'autre : la recursion ne s'arrete que sur un verdict
    // `ALL` — qui implique le verdict ponctuel pour chaque couple des deux
    // boites — ou sur un couple feuille-feuille, ou le test EST le test
    // ponctuel. Ce n'est donc pas un nouveau minorant, c'est la meme valeur
    // moins de travail. C'est la seconde branche de Q23, celle dont je disais
    // ne pas voir comment l'obtenir.
    //
    // TROIS PROPRIETES QUI LA RENDENT SURE.
    //
    // 1. La partition des couples ordonnes. Depuis `(U,U)` on descend en
    //    `(Ul,Ul) (Ul,Ur) (Ur,Ul) (Ur,Ur)`, qui partitionne `U x U` ; un couple
    //    de nœuds disjoints se scinde d'un seul cote. Chaque couple ordonne
    //    `(a,z)` est donc visite EXACTEMENT une fois, et la diagonale est
    //    ecartee par le seul cas `(feuille,meme feuille)`.
    // 2. Le range-add est un tableau de differences. Un nœud couvre un
    //    intervalle CONTIGU de l'ordre Morton, donc crediter tous ses ancres
    //    coute `O(1)` : `diff[first] += k ; diff[last+1] -= k`.
    // 3. Le masque de lanes, exactement comme la reparation q2. Un bloc
    //    credite pour la lane `q` retire ce bit avant de descendre, sinon ses
    //    sous-blocs recrediteraient les memes couples — c'est le defaut que le
    //    contre-audit avait trouve dans le cœur, et il se reproduirait ici.
    auto dual_tree = [&](int racine, const Box& Bpart, int lo_i, int hi_i,
                         std::vector<int>* diff) {
      const long long cutoff = dual_cutoff;
      const int m_pool = hi_i - lo_i + 1;
      struct F { int u, z, mask; };
      std::vector<F> st;
      st.push_back({racine, racine, 7});
      while (!st.empty()) {
        const F f = st.back();
        st.pop_back();
        const bool lu = h_leaf(f.u), lz = h_leaf(f.z);
        if (f.u == f.z) {
          if (lu) continue;  // diagonale `a == z`
          const int a1 = nodes[f.u].left, a2 = nodes[f.u].right;
          st.push_back({a1, a1, f.mask});
          st.push_back({a1, a2, f.mask});
          st.push_back({a2, a1, f.mask});
          st.push_back({a2, a2, f.mask});
          continue;
        }
        // CUTOFF. Un test de bloc coute jusqu'a `8^3` evaluations ; un couple
        // ponctuel en coute `8`. Tester un bloc qui couvre moins de `64`
        // couples ne peut donc pas etre rentable, et la descente y perd. En
        // dessous du seuil on paie directement les couples, ce qui rend la
        // meme valeur — c'est la MEME autorite ponctuelle.
        const int popu = h_pop(f.u), popz = h_pop(f.z);
        if ((long long)popu * popz <= cutoff) {
          const int fu = h_first(f.u), lau = h_last(f.u);
          const int fz = h_first(f.z), laz = h_last(f.z);
          for (int ia = fu; ia <= lau; ++ia)
            for (int iz = fz; iz <= laz; ++iz) {
              const int lp = corner8_lane(sorted_pts[ia], Bpart, sorted_pts[iz], &L.travail_ha);
              for (int q = 0; q < 3; ++q) {
                if (!(f.mask & (1 << q)) || q + 2 > lp) continue;
                std::vector<int>& d = diff[q];
                d[(size_t)(ia - lo_i)] += 1;
                d[(size_t)(ia - lo_i + 1)] -= 1;
              }
            }
          continue;
        }
        const int lane = block_lane(h_box(f.u), Bpart, h_box(f.z), &L.travail_ha);
        int reste = f.mask;
        if (lane >= 2) {
          const int fu = h_first(f.u), lau = h_last(f.u);
          const int pz = h_last(f.z) - h_first(f.z) + 1;
          for (int q = 0; q < 3; ++q) {
            if (!(reste & (1 << q)) || q + 2 > lane) continue;
            std::vector<int>& d = diff[q];
            d[(size_t)(fu - lo_i)] += pz;
            d[(size_t)(lau - lo_i + 1)] -= pz;
            // LE MASQUE : sans lui, les enfants recrediteraient les memes
            // couples. C'est litteralement le defaut du P0 q2, transpose.
            if (mutant != Mutant::kDualSansMasque) reste &= ~(1 << q);
          }
        }
        if (reste == 0) continue;
        if (lu && lz) continue;  // couple ponctuel deja decide exactement
        if (!lu && (lz || h_pop(f.u) >= h_pop(f.z))) {
          st.push_back({nodes[f.u].left, f.z, reste});
          st.push_back({nodes[f.u].right, f.z, reste});
        } else {
          st.push_back({f.u, nodes[f.z].left, reste});
          st.push_back({f.u, nodes[f.z].right, reste});
        }
      }
      (void)m_pool;
    };

    // `ud` doublee pour les deux sens, et rayons doubles majorants.
    const Sphere SA = h_sphere(r.u), SB = h_sphere(r.v);
    // Les crédits paresseux du dual-tree, puis leur propagation aux feuilles.
    std::vector<int> dA[3], dB[3];
    if (ha_dual) {
      for (int q = 0; q < 3; ++q) {
        dA[q].assign((size_t)na + 1, 0);
        dB[q].assign((size_t)nb + 1, 0);
      }
      dual_tree(r.u, BB, ua, ub, dA);
      if (mutant != Mutant::kDropB) dual_tree(r.v, BA, va, vb, dB);
      // Somme prefixe : le tableau de differences redevient un compte par point.
      for (int q = 0; q < 3; ++q) {
        int acc = 0;
        for (int i = 0; i < na; ++i) { acc += dA[q][(size_t)i]; dA[q][(size_t)i] = acc; }
        acc = 0;
        for (int i = 0; i < nb; ++i) { acc += dB[q][(size_t)i]; dB[q][(size_t)i] = acc; }
      }
    }
    // ---- BASELINE FUSIONNEE : UN SEUL PARCOURS POUR LES TROIS LANES.
    //
    // Le ré-audit a raison, et la faute est instructive : `corner8_lane` rend
    // deja la MEILLEURE lane en une passe, mais la boucle exterieure sur
    // `q = 2,3,4` la rappelait trois fois sur le meme couple. J'avais applique
    // exactement cette deduplication a `corner64` — huit coins de temoin
    // confondus — puis a `block_lane` — coins distincts d'une boite plate — et
    // je ne l'ai pas vue ici. Le gain que j'attribuais au dual-tree etait en
    // realite celui de cette fusion, que le dual-tree faisait par construction.
    //
    // C'est desormais la REFERENCE : c'est contre elle que tout gain doit se
    // mesurer, jamais contre la version qui recalcule trois fois.
    if (ha_fusion) {
      for (int i = 0; i < na; ++i) {
        const P3& a = sorted_pts[ua + i];
        int c[3] = {0, 0, 0};
        for (int j = 0; j < na; ++j) {
          if (j == i) continue;
          if (c[0] >= h_q[0] && c[1] >= h_q[1] && c[2] >= h_q[2]) break;
          const int lane = corner8_lane(a, BB, sorted_pts[ua + j], &L.travail_ha);
          ++L.travail_h;
          for (int q = 0; q < 3; ++q)
            if (lane >= q + 2 && c[q] < h_q[q]) ++c[q];
        }
        for (int q = 0; q < 3; ++q) ha[(size_t)i * 3 + q] = c[q];
      }
      if (mutant != Mutant::kDropB) {
        for (int i = 0; i < nb; ++i) {
          const P3& b = sorted_pts[va + i];
          int c[3] = {0, 0, 0};
          for (int j = 0; j < nb; ++j) {
            if (j == i) continue;
            if (c[0] >= h_q[0] && c[1] >= h_q[1] && c[2] >= h_q[2]) break;
            const int lane = corner8_lane(b, BA, sorted_pts[va + j], &L.travail_ha);
            ++L.travail_h;
            for (int q = 0; q < 3; ++q)
              if (lane >= q + 2 && c[q] < h_q[q]) ++c[q];
          }
          for (int q = 0; q < 3; ++q) hb[(size_t)i * 3 + q] = c[q];
        }
      }
      for (int q = 0; q < 3; ++q)
        for (int i = 0; i < na; ++i) L.ha_total[q] += ha[(size_t)i * 3 + q];
      for (int q = 0; q < 3; ++q)
        for (int i = 0; i < nb; ++i) L.hb_total[q] += hb[(size_t)i * 3 + q];
    }
    for (int q = 0; q < 3; ++q) {
      const int need = h_q[q];
      for (int i = 0; i < na; ++i) {
        if (ha_fusion) break;  // deja calcule, en une seule passe
        const P3& a = sorted_pts[ua + i];
        int c = 0;
        if (ha_dual) {
          c = dA[q][(size_t)i];
          if (c > need) c = need;  // ecretage APRES coup : min commute avec la somme
          if (ha_verifie) {
            int ref = 0;
            for (int j = 0; j < na && ref < need; ++j) {
              if (j == i) continue;
              if (corner8_lane(a, BB, sorted_pts[ua + j]) >= q + 2) ++ref;
            }
            ++L.dual_verifies;
            if (ref != c) ++L.dual_ecarts;
          }
        } else if (ha_boule) {
          const i64 ud[3] = {SB.c2[0] - 2 * a.x, SB.c2[1] - 2 * a.y, SB.c2[2] - 2 * a.z};
          c = compte_boule(a, ud, SA.r2, SB.r2, q, need, r.u, ua, ub);
        } else {
          for (int j = 0; j < na && c < need; ++j) {
            if (j == i) continue;
            ++L.travail_h;
            const P3& zz = sorted_pts[ua + j];
            bool w;
            if (ha_corner8) { w = corner8_lane(a, BB, zz, &L.travail_ha) >= q + 2; }
            else { L.travail_ha += 8; w = universal_witness(a, BB, zz, q + 2, narrow); }
            if (w) ++c;
          }
        }
        ha[(size_t)i * 3 + q] = c;
        L.ha_total[q] += c;
      }
      for (int i = 0; i < nb; ++i) {
        if (ha_fusion) break;  // deja calcule
        const P3& b = sorted_pts[va + i];
        int c = 0;
        if (mutant != Mutant::kDropB) {
          if (ha_dual) {
            c = dB[q][(size_t)i];
            if (c > need) c = need;
          } else if (ha_boule) {
            const i64 ud[3] = {SA.c2[0] - 2 * b.x, SA.c2[1] - 2 * b.y, SA.c2[2] - 2 * b.z};
            c = compte_boule(b, ud, SB.r2, SA.r2, q, need, r.v, va, vb);
          } else {
            for (int j = 0; j < nb && c < need; ++j) {
              if (j == i) continue;
              ++L.travail_h;
              const P3& zz = sorted_pts[va + j];
              bool w;
              if (ha_corner8) { w = corner8_lane(b, BA, zz, &L.travail_ha) >= q + 2; }
              else { L.travail_ha += 8; w = universal_witness(b, BA, zz, q + 2, narrow); }
              if (w) ++c;
            }
          }
        }
        hb[(size_t)i * 3 + q] = c;
        L.hb_total[q] += c;
      }
    }

    // ---- LE VRAI VIVANT, EXACTEMENT — et sans `O(n^3)`.
    //
    // Le prefiltre est fail-open : toute ancre vraiment vivante est PARMI les
    // survivantes. Il suffit donc de decider exactement les survivantes, qui
    // sont bien moins nombreuses que `C(n,2)`. Le coût est
    // `C(n,2)` tests de budget — trois additions — plus `survivantes x n`
    // evaluations avec sortie anticipee des que `h_q` temoins sont trouves.
    //
    // Ce n'est pas un estimateur : c'est le compte. J'avais d'abord retire
    // l'echantillonneur en invoquant une variance « inexpliquee » de trois a
    // douze ecarts-types — je comparais des ecarts RELATIFS a un ecart-type
    // ABSOLU en points de proportion. Les neuf ecarts tiennent en fait sous
    // `1,52` sigma : l'estimateur etait sain, et `--echantillon` reste offert
    // quand le scan exact deborde son budget. Le compte exact reste preferable
    // tant qu'il tient, parce qu'il ne demande aucun intervalle.
    //
    // ---- DEUX PASSES, ET UN SEUL BALAYAGE DE `z` POUR LES TROIS LANES.
    //
    // La premiere version bouclait `q` a l'exterieur : elle relisait donc trois
    // fois le meme `z` pour la meme paire, et appelait `corner8_lane` sur une
    // boite reduite a un point — huit coins identiques. Le re-audit du 15 aout
    // le releve (section 6.4). Ici :
    //
    //   passe 1, `O(1)` par paire : le MASQUE des lanes ou `(a,b)` survit,
    //            par test de budget ; masque vide, la paire est sautee ;
    //   passe 2, seulement si le masque est non vide : UN balayage de `z`,
    //            `pair_lane` une fois, trois compteurs alimentes, et chaque
    //            lane ETEINTE des qu'elle atteint son seuil `h_q`.
    //
    // Les paires `D = 0` sont exclues : `V_q` est defini sur `||a-b|| > 0`, et
    // un doublon quantifie n'est pas une ancre. Elles sont comptees a part.
    //
    // L'ancien balayage survit sous `--vivant=legacy`. Ce n'est pas de la
    // nostalgie : les deux doivent rendre EXACTEMENT les memes trois comptes,
    // et c'est le seul controle qui distingue « j'ai reecrit le balayage » de
    // « j'ai reecrit ce que le balayage compte ». La porte
    // `mhgp3v_vivant_deux_balayages` exige cette egalite.
    if (vrai_vivant && vivant_legacy) {
      for (int q = 0; q < 3; ++q) {
        const int need = h_q[q];
        for (int i = 0; i < na; ++i) {
          const int budget = need - hcore[q] - ha[(size_t)i * 3 + q];
          if (budget <= 0) continue;  // toute la ligne est morte
          const P3& a = sorted_pts[ua + i];
          for (int j = 0; j < nb; ++j) {
            if (hb[(size_t)j * 3 + q] >= budget) continue;  // paire fermee
            const int bi = va + j;
            if (ua + i == bi) continue;
            const P3& b = sorted_pts[bi];
            if (a.x == b.x && a.y == b.y && a.z == b.z) continue;  // `D = 0`
            const Box Bb = box_of_point(b);
            int c = 0;
            for (int z = 0; z < n && c < need; ++z) {
              if (z == ua + i || z == bi) continue;
              ++L.vivant_travail;
              if (corner8_lane(a, Bb, sorted_pts[z], &L.vivant_evals) >= q + 2) ++c;
            }
            ++L.vivant_paires;
            if (c < need) ++vrai_vivantes[q];
          }
        }
      }
    }
    if (vrai_vivant && !vivant_legacy) {
      for (int i = 0; i < na; ++i) {
        const int ai = ua + i;
        const P3& a = sorted_pts[ai];
        int budget[3];
        int ligne = 0;
        for (int q = 0; q < 3; ++q) {
          budget[q] = h_q[q] - hcore[q] - ha[(size_t)i * 3 + q];
          if (budget[q] > 0) ligne |= 1 << q;
        }
        if (ligne == 0) continue;  // ligne morte sur les trois lanes
        for (int j = 0; j < nb; ++j) {
          const int bi = va + j;
          if (ai == bi) continue;
          int masque = 0;
          for (int q = 0; q < 3; ++q)
            if ((ligne >> q & 1) && hb[(size_t)j * 3 + q] < budget[q]) masque |= 1 << q;
          if (masque == 0) continue;  // paire fermee sur les trois lanes
          const P3& b = sorted_pts[bi];
          if (a.x == b.x && a.y == b.y && a.z == b.z) {
            // `D = 0` : hors du domaine de `V_q`. Mais la paire EST comptee
            // dans `S_q`, qui indexe des paires d'identifiants. La retenir par
            // lane est le seul moyen de rendre le mou comparable : sinon le
            // numerateur compte des paires que le denominateur exclut.
            ++L.vivant_degenerees;
            for (int q = 0; q < 3; ++q)
              if (masque >> q & 1) ++L.vivant_degen_lane[q];
            continue;
          }
          ++L.vivant_paires;
          int c3[3] = {0, 0, 0};
          int actif = masque;
          for (int z = 0; z < n && actif; ++z) {
            if (z == ai || z == bi) continue;
            ++L.vivant_travail;
            ++L.vivant_evals;  // exactement une evaluation par visite
            int lane = pair_lane(a, b, sorted_pts[z]);
            if (mutant == Mutant::kVivantLaneUnique && lane > 2) lane = 2;
            if (lane == 0) continue;
            for (int q = 0; q < 3; ++q) {
              if (!(actif >> q & 1)) continue;
              if (lane >= q + 2 && ++c3[q] >= h_q[q] &&
                  mutant != Mutant::kVivantSansExtinction)
                actif &= ~(1 << q);
            }
          }
          for (int q = 0; q < 3; ++q) {
            if (!(masque >> q & 1) || c3[q] >= h_q[q]) continue;
            {
              ++vrai_vivantes[q];
              // ---- LE COUT D'INSTRUCTION, MESURE ET NON SUPPOSE.
              //
              // Une ancre vivante doit ensuite etre INSTRUITE : retrouver ses
              // supports. Pour q3, le troisieme sommet vit dans la LENTILLE
              // `{c : |ac| <= |ab| et |bc| <= |ab|}`, puisque `(a,b)` est
              // l'arete maximale. Son volume vaut `5 pi D^3/12`, soit
              // exactement `2,5` fois la boule diametrale `pi D^3/6`.
              //
              // Or une ancre VIVANTE a moins de `h_2` points dans sa boule
              // diametrale. Sous densite locale uniforme la lentille en
              // contiendrait donc `2,5 h_2`, soit vingt-cinq a `s_max=11` :
              // l'instruction serait en `O(h)` et non en `O(n)`.
              //
              // C'est exactement l'hypothese qui tombe sur un nuage groupe. On
              // la mesure au lieu de la supposer.
              if (cout_instruction && q == 2) {
                const i64 dab = (sorted_pts[ua + i].x - sorted_pts[bi].x) *
                                    (sorted_pts[ua + i].x - sorted_pts[bi].x) +
                                (sorted_pts[ua + i].y - sorted_pts[bi].y) *
                                    (sorted_pts[ua + i].y - sorted_pts[bi].y) +
                                (sorted_pts[ua + i].z - sorted_pts[bi].z) *
                                    (sorted_pts[ua + i].z - sorted_pts[bi].z);
                long long lent = 0;
                for (int c2 = 0; c2 < n; ++c2) {
                  if (c2 == ua + i || c2 == bi) continue;
                  const i64 da = (sorted_pts[c2].x - a.x) * (sorted_pts[c2].x - a.x) +
                                 (sorted_pts[c2].y - a.y) * (sorted_pts[c2].y - a.y) +
                                 (sorted_pts[c2].z - a.z) * (sorted_pts[c2].z - a.z);
                  if (da > dab) continue;
                  const i64 db = (sorted_pts[c2].x - sorted_pts[bi].x) *
                                     (sorted_pts[c2].x - sorted_pts[bi].x) +
                                 (sorted_pts[c2].y - sorted_pts[bi].y) *
                                     (sorted_pts[c2].y - sorted_pts[bi].y) +
                                 (sorted_pts[c2].z - sorted_pts[bi].z) *
                                     (sorted_pts[c2].z - sorted_pts[bi].z);
                  if (db <= dab) ++lent;
                }
                L.lentille_somme += lent;
                if (lent > L.lentille_max) L.lentille_max = lent;
                ++L.lentille_ancres;
              }
            }
          }
        }
      }
    }

    // ---- Comptage des survivantes SANS materialiser une seule paire.
    for (int q = 0; q < 3; ++q) {
      const int need = h_q[q] + seuil_delta;
      std::fill(histo.begin(), histo.end(), 0);
      for (int i = 0; i < nb; ++i) {
        int v = hb[(size_t)i * 3 + q];
        if (v > kHisto - 1) v = kHisto - 1;
        ++histo[v];
      }
      std::vector<int> cum((size_t)kHisto + 1, 0);
      for (int k = 0; k < kHisto; ++k) cum[k + 1] = cum[k] + histo[k];
      for (int i = 0; i < na; ++i) {
        const int budget = need - hcore[q] - ha[(size_t)i * 3 + q];
        if (budget <= 0) continue;                     // deja mort pour tout `b`
        const int k = budget > kHisto ? kHisto : budget;  // `h_b < budget`
        L.survivantes[q] += cum[k];
      }
    }

    // ---- LES SEEDS AIGUS, ET L'ELAGAGE PAR BLOC.
    //
    // C'est le maillon suivant de la contraction que l'audit reclame :
    // `W`-vivant -> seeds positifs -> supports -> fusions. Une ancre survivante
    // n'est pas un support ; il lui faut au moins un troisieme sommet formant
    // un triangle AIGU dont elle est l'arete maximale.
    //
    // Deux chemins, et le second doit rendre EXACTEMENT le compte du premier :
    //
    //   reference : balayage complet des `n` points par `est_seed`. C'est
    //               `O(n)` par ancre, et c'est ce qui coûte `Theta(n^3)` sur
    //               `two_lines` — `Theta(n^2)` ancres a lentille `n-2`.
    //   elagage   : descente sur l'arbre, un sous-arbre entier rejete des que
    //               `bloc_sans_seed(BA, BB, Box(X))` certifie `H >= 0` partout.
    //               Le certificat vaut pour TOUTES les ancres du rectangle a la
    //               fois, exactement comme `h_coeur`.
    //
    // Le compteur `seed_ancres_sans` est le chiffre qui compte : sur
    // `two_lines` il doit valoir la totalite, puisque la vraie source q3/q4 y
    // est vide. C'est la mesure de ce que la positivite retire et qu'aucun
    // certificat de temoins ne pouvait retirer.
    if (seeds) {
      const int q = 2;  // lane q4 : `C4_carrier` se compte sur les V4-vivantes
      const int need = h_q[q];
      for (int i = 0; i < na; ++i) {
        const int budget = need - hcore[q] - ha[(size_t)i * 3 + q];
        if (budget <= 0) continue;
        const int ai = ua + i;
        const P3& a = sorted_pts[ai];
        for (int j = 0; j < nb; ++j) {
          if (hb[(size_t)j * 3 + q] >= budget) continue;
          const int bi = va + j;
          if (ai == bi) continue;
          const P3& b = sorted_pts[bi];
          if (a.x == b.x && a.y == b.y && a.z == b.z) continue;  // `D = 0`
          ++L.seed_ancres;
          long long cref = 0;
          for (int x = 0; x < n; ++x) {
            if (x == ai || x == bi) continue;
            ++L.seed_travail_ref;
            if (est_seed(a, b, sorted_pts[x])) ++cref;
          }
          L.seed_total_ref += cref;
          if (cref == 0) ++L.seed_ancres_sans;

          // Descente elaguee, meme resultat par un autre chemin.
          long long celag = 0;
          std::vector<int> st;
          if (!nodes.empty()) st.push_back(0);
          else if (n == 1) st.push_back(-1);
          while (!st.empty()) {
            const int h = st.back();
            st.pop_back();
            const int pf = h_first(h), pl = h_last(h);
            // ---- LE CERTIFICAT PORTE SUR L'ANCRE, PAS SUR LE RECTANGLE.
            //
            // J'ai d'abord ecrit `bloc_sans_seed(BA, BB, Box(X))`, en me disant
            // que le certificat vaudrait pour toutes les ancres du rectangle a
            // la fois — la structure de `h_coeur`. C'est logiquement correct et
            // MESURABLEMENT INUTILE : `gain=1,005` sur `two_lines`,
            // `blocs_elagues=90 324` pour `points_elagues=99 210`, soit `1,1`
            // point par bloc. Il n'elaguait que des feuilles.
            //
            // La cause est le quantificateur : exiger `H >= 0` pour TOUT
            // `(a,b)` du produit `A x B` est bien plus fort que pour l'ancre
            // courante, et echoue des qu'une seule paire du rectangle rend un
            // `x` aigu. La mutualisation coûte ici plus qu'elle ne rapporte.
            //
            // Avec les boites PONCTUELLES de `a` et `b`, `corners_distinct` en
            // rend un seul chacune : huit evaluations par nœud, contre
            // `pop(X)` tests de points. Tout sous-arbre de plus de huit points
            // certifie est un gain net.
            const Box BX = h_box(h);
            const bool sans = bloc_sans_seed_boule(a, b, BX, &L.seed_travail_elag);
            // LE JUGE DU CERTIFICAT, sur le chemin et non a cote : les deux
            // implementations n'ont aucune primitive commune — l'une enumere
            // des coins et evalue `e.t`, l'autre calcule une distance maximale
            // a un centre — et doivent decider identiquement.
            // ---- LE JUGE PORTE SUR LA COMPOSANTE COMMUNE, ET PAS AUTRE CHOSE.
            //
            // J'ai d'abord confronte le certificat `O(1)` COMPLET aux huit
            // coins, et lu `365 234` desaccords en croyant a un defaut. Les
            // deux ne decident pas le meme predicat : le `O(1)` est la
            // DISJONCTION `hors_lentille OU dans_boule`, les huit coins ne
            // testent que `H >= 0`. Le desaccord etait le second disjoint qui
            // faisait son travail — un juge mal cadre, pas un bug.
            //
            // La comparaison qui a un sens est `bloc_dans_boule_diametrale`
            // contre `bloc_sans_seed` : meme predicat, deux chemins sans
            // primitive commune — distance maximale a un centre d'un cote,
            // enumeration de coins et produits scalaires de l'autre.
            if (seed_juge) {
              ++L.seed_juges;
              if (bloc_sans_seed(box_of_point(a), box_of_point(b), BX, nullptr) !=
                  bloc_dans_boule_diametrale(a, b, BX))
                ++L.seed_desaccords_certif;
            }
            if (sans) {
              ++L.seed_blocs_elagues;
              L.seed_points_elagues += pl - pf + 1;
              continue;
            }
            if (h_leaf(h)) {
              const int x = pf;
              if (x == ai || x == bi) continue;
              ++L.seed_travail_elag;
              if (est_seed(a, b, sorted_pts[x])) ++celag;
              continue;
            }
            st.push_back(nodes[h].left);
            st.push_back(nodes[h].right);
          }
          L.seed_total_elag += celag;
          if (celag != cref) ++L.seed_ecarts;
        }
      }
    }

    // ---- ORACLE (P0.3, P0.4, P0.5). Il materialise ce que le chemin normal
    // evite : la decision par `PairId`, l'identite des sites credites au coeur,
    // et la couverture reelle de la partition. Borne a petit `n`.
    if (oracle) {
      // P0.4 : chaque `PointId` credite au plus une fois, et jamais dans `A`
      // ou `B`. C'est ce controle, et non un compteur alimente par un mutant,
      // qui atteste la disjonction.
      // P0.4, DESORMAIS PAR LANE. Le contre-audit exigeait ce ledger avant tout
      // credit q3/q4 en bloc : sans lui, un double credit q3 ou q4 passerait
      // inapercu exactement comme le double credit q2 du 15 aout.
      for (int q = 0; q < 3; ++q) {
        for (int id : core_ids[q]) {
          if (vu[(size_t)id]) { ++L.oracle_ids_doubles; continue; }
          vu[(size_t)id] = 1;
          if ((id >= ua && id <= ub) || (id >= va && id <= vb)) ++L.oracle_ids_doubles;
        }
        for (int id : core_ids[q]) vu[(size_t)id] = 0;
      }
      // ---- HARNAIS APPARIE `corner512` (Q21/Q22).
      //
      // Il ne SUBSTITUE rien : il fait tourner les deux predicats sur les MEMES
      // sites et compte trois choses. `gagne` mesure ce que `corner512`
      // certifierait en plus — la reponse chiffree a Q22. `faux` mesure les cas
      // ou il certifie une lane que la force brute sur les VRAIES paires du
      // rectangle refute — la reponse a Q21 : s'il est non nul, le predicat
      // n'est pas un certificat `ALL` valide sur des boites, et la substitution
      // demandee en P1.6 serait une regression de surete.
      if (compare512) {
        mhgp3v::cone::Box CA, CB, CZ;
        for (int k = 0; k < 3; ++k) {
          CA.lo[k] = BA.lo[k]; CA.hi[k] = BA.hi[k];
          CB.lo[k] = BB.lo[k]; CB.hi[k] = BB.hi[k];
        }
        for (int i = 0; i < n; ++i) {
          if ((i >= ua && i <= ub) || (i >= va && i <= vb)) continue;
          for (int k = 0; k < 3; ++k) {
            CZ.lo[k] = CZ.hi[k] = (k == 0) ? sorted_pts[i].x
                                : (k == 1) ? sorted_pts[i].y : sorted_pts[i].z;
          }
          const int lane = mhgp3v::soc::corner512_all_lane(CA, CB, CZ);
          // PORTE D'EGALITE : la specialisation ponctuelle doit rendre la MEME
          // valeur que la reference, site par site. Un seul desaccord suffit a
          // refuser la campagne (code 3) : `corner64` n'est pas un predicat
          // distinct, c'est le meme calcul sans ses redondances.
          if (corner64_all_lane(rect_corners, sorted_pts[i],
                                mutant == Mutant::kCorner64Sept) != lane)
            ++L.c64_desaccords;
          ++L.c512_sites;
          for (int q = 0; q < 3; ++q) {
            const bool mine = universal_over_rect(BA, BB, sorted_pts[i], q + 2, narrow,
                                                  centre_only, corners_xi);
            const bool his = (lane >= q + 2);
            if (his && !mine) ++L.c512_gagne[q];
            if (mine && !his) ++L.c512_perd[q];
            if (!his) continue;
            // force brute sur les vraies paires du rectangle
            bool ok = true;
            for (int ai = ua; ai <= ub && ok; ++ai)
              for (int bi = va; bi <= vb && ok; ++bi) {
                const Box pb = box_of_point(sorted_pts[bi]);
                if (!universal_witness(sorted_pts[ai], pb, sorted_pts[i], q + 2, false))
                  ok = false;
              }
            if (!ok) ++L.c512_faux[q];
          }
        }
      }
      // P0.5 : couverture reelle, une occurrence par paire non ordonnee.
      // La somme `|A||B| = C(n,2)` peut masquer un doublon compense par un
      // manque ; ce compte-ci ne le peut pas.
      for (int i = ua; i <= ub; ++i)
        for (int j = va; j <= vb; ++j) ++couverture[(size_t)pair_idx(i, j)];
      // P0.3 : la decision, paire par paire.
      for (int q = 0; q < 3; ++q)
        for (int i = 0; i < na; ++i)
          for (int j = 0; j < nb; ++j) {
            const int lower = hcore[q] + ha[(size_t)i * 3 + q] + hb[(size_t)j * 3 + q];
            if (lower >= h_q[q] + seuil_delta)
              ferme[(size_t)pair_idx(ua + i, va + j) * 3 + q] = 1;
          }
    }
  }  // fin de la boucle sur les rectangles

  // ---- CONFRONTATION A LA FORCE BRUTE (P0.3). Une paire fermee par le
  // prefiltre DOIT etre vraiment morte. L'inverse n'est pas exige : le filtre
  // est un minorant, il a le droit de laisser vivre une paire morte.
  if (oracle) {
    for (long long p = 0; p < npairs; ++p)
      if (couverture[(size_t)p] != 1u) ++L.oracle_couverture_ko;
    for (int i = 0; i < n; ++i)
      for (int j = i + 1; j < n; ++j) {
        const long long idx = pair_idx(i, j);
        bool besoin = false;
        for (int q = 0; q < 3; ++q) if (ferme[(size_t)idx * 3 + q]) besoin = true;
        if (!besoin) continue;
        ++L.oracle_paires;
        const Box Bb = box_of_point(sorted_pts[j]);
        int vrai[3] = {0, 0, 0};
        for (int k = 0; k < n; ++k) {
          if (k == i || k == j) continue;
          for (int q = 0; q < 3; ++q)
            if (universal_witness(sorted_pts[i], Bb, sorted_pts[k], q + 2, false)) ++vrai[q];
        }
        for (int q = 0; q < 3; ++q)
          if (ferme[(size_t)idx * 3 + q] && vrai[q] < h_q[q]) ++L.oracle_faux_morts;
      }
  }

  // ---- JUGE DU COEUR. Le juge ponctuel ne voit pas une faute qui n'apparait
  // que sur des boites etendues : il faut donc re-verifier chaque temoin
  // certifie par `universal_over_rect` contre TOUTES les vraies paires `(a,b)`
  // du rectangle. C'est ce controle, et lui seul, qui tue `coeur-centre-seul`.
  if (judge > 0) {
    const size_t lim = rects.size() < 400 ? rects.size() : 400;
    for (size_t ri = 0; ri < lim; ++ri) {
      const Rect& r = rects[ri];
      const int ua2 = h_first(r.u), ub2 = h_last(r.u);
      const int va2 = h_first(r.v), vb2 = h_last(r.v);
      if (ub2 - ua2 + 1 > 24 || vb2 - va2 + 1 > 24) continue;
      const Box BA2 = h_box(r.u), BB2 = h_box(r.v);
      for (int q = 0; q < 3; ++q) {
        for (int i = 0; i < n; ++i) {
          if (i >= ua2 && i <= ub2) continue;
          if (i >= va2 && i <= vb2) continue;
          if (!universal_over_rect(BA2, BB2, sorted_pts[i], q + 2, narrow, centre_only, corners_xi))
            continue;
          ++L.coeur_verifies;
          bool ok = true;
          for (int ai = ua2; ai <= ub2 && ok; ++ai)
            for (int bi = va2; bi <= vb2 && ok; ++bi) {
              const Box pb = box_of_point(sorted_pts[bi]);
              if (!universal_witness(sorted_pts[ai], pb, sorted_pts[i], q + 2, false)) ok = false;
            }
          if (!ok) ++L.coeur_faux;
        }
      }
    }
  }

  // ---- JUGE PONCTUEL : a petite taille, aucune paire tuee ne doit etre vivante.
  long long desaccords = 0, juge_paires = 0, juge_vivantes = 0;
  if (judge > 0) {
    const int m = n < judge ? n : judge;
    for (int i = 0; i < m; ++i) {
      for (int j = i + 1; j < m; ++j) {
        ++juge_paires;
        const P3& a = sorted_pts[i];
        const P3& b = sorted_pts[j];
        const Box Bb = box_of_point(b);
        for (int q = 0; q < 3; ++q) {
          int vrai = 0;
          for (int k = 0; k < m; ++k) {
            if (k == i || k == j) continue;
            if (universal_witness(a, Bb, sorted_pts[k], q + 2, false)) ++vrai;
          }
          if (vrai < h_q[q]) ++juge_vivantes;
          // Le minorant du prefiltre, recalcule ici sur la paire ponctuelle,
          // ne doit JAMAIS depasser le compte vrai.
          const Box Ba = box_of_point(a);
          int minorant = 0;
          for (int k = 0; k < m; ++k) {
            if (k == i || k == j) continue;
            if (universal_over_rect(Ba, Bb, sorted_pts[k], q + 2, narrow, centre_only, corners_xi))
              ++minorant;
          }
          if (minorant > vrai) ++desaccords;
        }
      }
    }
  }

  // ---- ECHANTILLON : CONSERVE, MAIS HORS DU CHEMIN DE MESURE.
  //
  // Cette voie tire `K` paires et decide chacune exactement. Elle a servi a
  // amorcer la mesure, et elle est CONSERVEE parce qu'elle reste le seul recours
  // si le residuel devenait trop grand pour `--vrai-vivant`. Mais elle ne porte
  // plus aucun chiffre publie, et voici pourquoi.
  //
  // A `n=600`, contre le compte exact `45 913`, trois graines et trois tailles
  // donnent des ecarts de `+2,85 %`, `-3,63 %`, `-0,21 %`, `-1,42 %`,
  // `+1,84 %`, `+0,55 %` la ou l'ecart-type binomial vaut `0,62`, `0,31` et
  // `0,15 %`. Soit trois a douze ecarts-types. Le premier generateur —
  // xorshift64 reduit par `% n`, donc sur ses bits les plus faibles — en etait
  // une cause ; splitmix64 avec reduction par multiplication haute n'a PAS
  // suffi. La variance residuelle n'est pas expliquee.
  //
  // Extrapoler sur une variance qu'on ne comprend pas ne vaut rien. Le chemin
  // de mesure est donc `--vrai-vivant`, exact.
  long long ech_vivantes[3] = {0, 0, 0};
  // MODE EXACT, ET IL RESTE BORNE. Il enumere toutes les paires, donc il est en
  // `O(n^3)` et plafonne a quelques centaines de points : ce n'est PAS le
  // chemin de mesure. Son role est d'etre l'oracle de l'ORACLE — valider une
  // fois l'estimateur par echantillonnage, qui lui est en `O(K n)` et passe a
  // l'echelle. Accord mesure a `n=600` : `-0,81 %`, `-0,61 %`, `-0,34 %` sur
  // les trois familles, lane q4.
  //
  // Il porte aussi l'invariant central `survivantes >= vraiment vivantes` — le
  // prefiltre est fail-open — garde directement, sans passer par les `PairId`.
  if (echantillon > 0 && n >= 2) {
    // SPLITMIX64, ET LE TIRAGE SUR LES BITS DE POIDS FORT.
    //
    // La premiere version employait un xorshift64 et prenait `nxt() % n`, donc
    // les bits de POIDS FAIBLE — les plus mauvais d'un xorshift. Les ecarts
    // mesures a `n=600` valaient alors trois a cinq ecarts-types theoriques :
    // ce n'etait pas du bruit binomial, c'etait le generateur. Splitmix64 a un
    // pas d'avalanche complet, et la reduction se fait par multiplication haute
    // plutot que par modulo, ce qui evite en prime le biais de repliement.
    unsigned long long st = (unsigned long long)(graine_ech ? graine_ech : seed);
    auto nxt = [&]() {
      st += 0x9E3779B97F4A7C15ULL;
      unsigned long long z = st;
      z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
      z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
      return z ^ (z >> 31);
    };
    auto tire = [&](int borne) {
      return (int)(((unsigned __int128)nxt() * (unsigned __int128)(unsigned)borne) >> 64);
    };
    for (long long k = 0; k < echantillon; ++k) {
      int i = tire(n);
      int j = tire(n);
      if (i == j) { --k; continue; }
      const P3& a = sorted_pts[i];
      const Box Bb = box_of_point(sorted_pts[j]);
      int cnt[3] = {0, 0, 0};
      for (int z = 0; z < n; ++z) {
        if (z == i || z == j) continue;
        const int lane = corner8_lane(a, Bb, sorted_pts[z]);
        for (int q = 0; q < 3; ++q) if (lane >= q + 2) ++cnt[q];
      }
      for (int q = 0; q < 3; ++q) if (cnt[q] < h_q[q]) ++ech_vivantes[q];
    }
  }

  // ---- RECU
  std::printf("CombinedPrefilterReceipt-v1\n");
  std::printf(
      "cadre phase=exploration_v3_hors_registre backend=cpu_reference "
      "profile=quantized_u16_input_only mode=diagnostic_counter_only "
      "public_status=not_claimed\n");
  std::printf("cloud family=%s n=%d coord=%d seed=%lld smax=%d separation=%d cap=%d inject=%s\n",
              mhgp3v::cloud_family_name(family), n, coord_used, seed, smax, sep, cap,
              mutant_name(mutant));
  std::printf("seuils h_q2=%d h_q3=%d h_q4=%d ha_mode=%s coeur_mode=%s\n", h_q[0], h_q[1],
              h_q[2], ha_fusion ? "fusion" : (ha_dual ? "dualtree" : (ha_boule ? "boule" : (ha_corner8 ? "corner8" : "jointure"))), core512 ? "corner64" : "bornes");
  std::printf("wspd rectangles=%lld non_decides=%lld masse=%lld masse_non_decide=%lld "
              "cellule_max=%lld cap_mode=%s\n",
              L.rectangles, L.non_decides, L.masse_totale, L.masse_non_decide,
              L.cellules_max, cap_scission ? "scission" : "refus");
  // ---- L'UNIVERS DES ANCRES N'EST PAS `C(n,2)`.
  //
  // Deux identifiants aux memes coordonnees ne forment pas une ancre : `D = 0`
  // est hors du domaine. L'univers vaut donc `C(n,2) - somme_x C(m_x,2)`, ou
  // `m_x` est la multiplicite de la position `x`. Le ledger de recouvrement, lui,
  // reste sur `C(n,2)` : la WSPD couvre des paires d'INDICES, et confondre les
  // deux denominateurs rendrait l'ecart de recouvrement faux.
  long long paires_d0 = 0;
  long long positions_distinctes = 0;
  {
    std::vector<P3> tri = sorted_pts;
    std::sort(tri.begin(), tri.end(), [](const P3& p, const P3& q) {
      if (p.x != q.x) return p.x < q.x;
      if (p.y != q.y) return p.y < q.y;
      return p.z < q.z;
    });
    for (size_t i = 0; i < tri.size();) {
      size_t j = i + 1;
      while (j < tri.size() && tri[j].x == tri[i].x && tri[j].y == tri[i].y &&
             tri[j].z == tri[i].z)
        ++j;
      const long long m = (long long)(j - i);
      paires_d0 += m * (m - 1) / 2;
      ++positions_distinctes;
      i = j;
    }
  }
  const long long total = (long long)n * (n - 1) / 2;
  std::printf("univers paires_indices=%lld positions_distinctes=%lld paires_D0=%lld "
              "univers_ancres=%lld\n",
              total, positions_distinctes, paires_d0, total - paires_d0);
  // L'audit du `00cf78c` demande au minimum un refus explicite des doublons,
  // parce qu'ils deviennent artificiellement `W`-vivants. Il est optionnel : la
  // structure juste — une geometrie sur les positions distinctes, chacune
  // portant sa liste de `PointId` et sa multiplicite — n'est pas ecrite, et un
  // refus par defaut interdirait des nuages que le reste du probe traite bien.
  if (refuse_doublons && paires_d0 > 0) {
    std::fprintf(stderr, "REFUS : %lld paires de positions dupliquees\n", paires_d0);
    return 2;
  }
  std::printf("ledger masse_attendue=%lld ecart=%lld recouvrements=%lld travail_h=%lld "
              "travail_ha=%lld\n",
              total, L.masse_totale - total, L.recouvrements, L.travail_h, L.travail_ha);
  for (int q = 0; q < 3; ++q) {
    const double pct = total > 0 ? 100.0 * (double)(total - L.survivantes[q]) / (double)total : 0.0;
    std::printf("lane q%d survivantes=%lld fermees=%lld ferme_pct=%.3f coeur_somme=%lld "
                "coeur_non_vide=%lld ha_somme=%lld hb_somme=%lld\n",
                q + 2, L.survivantes[q], total - L.survivantes[q], pct, L.coeur_total[q],
                L.coeur_non_vide[q], L.ha_total[q], L.hb_total[q]);
  }
  if (compare512)
    std::printf("corner512 sites=%lld gagne=%lld/%lld/%lld perd=%lld/%lld/%lld "
                "faux=%lld/%lld/%lld corner64_desaccords=%lld\n",
                L.c512_sites, L.c512_gagne[0], L.c512_gagne[1], L.c512_gagne[2],
                L.c512_perd[0], L.c512_perd[1], L.c512_perd[2],
                L.c512_faux[0], L.c512_faux[1], L.c512_faux[2], L.c64_desaccords);
  if (core512) std::printf("corner64 appels=%lld\n", L.c64_appels);
  if (cout_instruction && L.lentille_ancres > 0)
    std::printf("instruction ancres=%lld lentille_moyenne=%.2f lentille_max=%lld\n",
                L.lentille_ancres,
                (double)L.lentille_somme / (double)L.lentille_ancres, L.lentille_max);
  // LE COMPTE EST FAUX DES QU'UNE MASSE EST HORS CAP, et l'auditeur l'a montre
  // par contre-rejeu : sur soixante points, cap 512 donne `q4_vivantes=1594`,
  // cap 1 donne `1201`, et les deux sortaient code zero en se disant exactes.
  // Les paires des rectangles capes sont ajoutees au residuel sans jamais etre
  // testees, donc le compte publie n'est qu'un MINORANT de `V_q` et le mou un
  // majorant sans garantie.
  if (vrai_vivant && L.masse_non_decide != 0) {
    std::fprintf(stderr,
                 "PLANCHER : --vrai-vivant avec %lld paires hors cap — le compte"
                 " serait un minorant, pas une mesure\n", L.masse_non_decide);
    return 3;
  }
  if (seeds) {
    // ---- LA CONTRACTION, PUBLIEE ET NON RACONTEE.
    //
    // `ancres` sont les q3 survivantes ; `sans_seed` celles qu'AUCUN triangle
    // aigu ne peut porter — elles meurent par POSITIVITE, ce qu'aucun
    // certificat de temoins ne pouvait faire. `travail_elag / travail_ref`
    // mesure ce que l'autorite de bloc retire reellement.
    const double contraction =
        L.seed_ancres > 0 ? (double)L.seed_ancres_sans / (double)L.seed_ancres : 0.0;
    const double gain = L.seed_travail_elag > 0
                            ? (double)L.seed_travail_ref / (double)L.seed_travail_elag
                            : 0.0;
    std::printf("etages V4_pair_walive=%lld ancres_sans_carrier=%lld contraction=%.4f "
                "C4_carrier=%lld C4_carrier_elag=%lld ecarts=%lld travail_ref=%lld "
                "travail_elag=%lld gain=%.3f blocs_elagues=%lld points_elagues=%lld "
                "certif_juges=%lld certif_desaccords=%lld\n",
                L.seed_ancres, L.seed_ancres_sans, contraction, L.seed_total_ref,
                L.seed_total_elag, L.seed_ecarts, L.seed_travail_ref,
                L.seed_travail_elag, gain, L.seed_blocs_elagues,
                L.seed_points_elagues, L.seed_juges, L.seed_desaccords_certif);
    // LES DEUX CERTIFICATS N'ONT AUCUNE PRIMITIVE COMMUNE : l'un enumere des
    // coins et evalue `e.t`, l'autre compare des distances a un centre. Un
    // desaccord est donc un desaccord de juge, pas un avertissement.
    if (L.seed_desaccords_certif > 0) {
      std::fprintf(stderr, "DESACCORD DU JUGE : %lld certificats de bloc divergent\n",
                   L.seed_desaccords_certif);
      return 1;
    }
    // L'ELAGAGE DOIT RENDRE LE COMPTE DE LA REFERENCE, A L'UNITE. Un ecart
    // signifie que `bloc_sans_seed` a certifie un bloc qui portait un seed :
    // c'est un desaccord de juge, donc le code 1, jamais un avertissement.
    if (L.seed_ecarts > 0) {
      std::fprintf(stderr,
                   "DESACCORD DU JUGE : %lld ancres ou l'elagage differe du"
                   " balayage complet\n", L.seed_ecarts);
      return 1;
    }
  }
  if (vrai_vivant) {
    // ---- LE MOU, ET SES DEUX DENOMINATEURS.
    //
    // `mu = S_q / V_q` est un RAPPORT, et l'imprimer a `0` quand `V_q` est nul
    // etait un mensonge de format : `0` se lit « le residuel est vide », alors
    // que `V_q = 0` avec `S_q > 0` est exactement l'inverse — le residuel est
    // ENTIEREMENT du mou. Trois cas, trois ecritures :
    //
    //   `V > 0`           `mu = S/V`
    //   `V = 0`, `S > 0`  `mu = inf`     tout le residuel est retirable
    //   `V = 0`, `S = 0`  `mu = NA`      il n'y a rien a mesurer
    //
    // Et deux quantites distinctes, qu'il faut nommer avec leur denominateur,
    // l'audit du `00cf78c` ayant raison de dire qu'aucune des deux n'est
    // fausse : `mu - 1` est le SURCOUT rapporte au plancher `V_q` ;
    // `1 - 1/mu` est la FRACTION DU RESIDUEL encore retirable. J'avais publie
    // la premiere en croyant publier la seconde.
    //
    // `S_q` est corrige des paires `D = 0` : elles sont dans `S_q`, qui indexe
    // des paires d'identifiants, mais hors de `V_q`, defini sur `||a-b|| > 0`.
    std::printf("vraivivant");
    for (int q = 0; q < 3; ++q) {
      const long long S = L.survivantes[q] - L.vivant_degen_lane[q];
      const long long V = vrai_vivantes[q];
      // `V%d_pair_walive` ET RIEN D'AUTRE. Le nom `q4_vivantes` faisait quatre
      // metiers a la fois — paires-ancrages, triples porteurs, tetraedres bien
      // centres, q4 de rang borne — et l'audit `eb42b57` a raison d'exiger la
      // separation. Ce compteur ne mesure QUE des PAIRES.
      std::printf(" V%d_pair_walive=%lld V%d_survivantes_D0exclu=%lld", q + 2, V, q + 2, S);
      if (V > 0) {
        const double mou = (double)S / (double)V;
        std::printf(" V%d_mou=%.3f V%d_surcout=%.3f V%d_retirable=%.3f", q + 2, mou,
                    q + 2, mou - 1.0, q + 2, 1.0 - 1.0 / mou);
      } else {
        std::printf(" V%d_mou=%s", q + 2, S > 0 ? "inf" : "NA");
      }
    }
    // LE BUDGET, PUBLIE ET NON AFFIRME. Le re-audit demandait un compte en
    // `n |S|` : ces trois champs le rendent verifiables. `paires` doit rester du
    // meme ordre que le plus grand des `survivantes` ; `travail / (paires n)`
    // mesure ce que la sortie anticipee gagne reellement ; `degenerees` dit
    // combien de doublons quantifies ont ete ecartes du domaine de `V_q`.
    std::printf(" paires=%lld travail=%lld evals=%lld degenerees=%lld",
                L.vivant_paires, L.vivant_travail, L.vivant_evals,
                L.vivant_degenerees);
    std::printf("\n");
  }
  if (echantillon > 0) {
    const double tot = (double)((long long)n * (n - 1) / 2);
    std::printf("echantillon paires=%lld", echantillon);
    for (int q = 0; q < 3; ++q) {
      const double frac = (double)ech_vivantes[q] / (double)echantillon;
      const double vrai = frac * tot;
      const double mou = vrai > 0 ? (double)L.survivantes[q] / vrai : 0.0;
      std::printf(" q%d_vivantes=%lld q%d_estime=%.0f q%d_mou=%.3f", q + 2,
                  ech_vivantes[q], q + 2, vrai, q + 2, mou);
    }
    std::printf("\n");
  }
  if (ha_dual)
    std::printf("dualtree verifies=%lld ecarts=%lld cutoff=%lld\n", L.dual_verifies,
                L.dual_ecarts, dual_cutoff);
  std::printf("nonvacuite bulk_boule=%lld elague_ext=%lld\n", L.bulk_boule, L.elague_ext);
  std::printf("nonvacuite bulk_credits=%lld oracle_paires=%lld oracle_faux_morts=%lld "
              "oracle_ids_doubles=%lld oracle_couverture_ko=%lld\n",
              L.bulk_credits, L.oracle_paires, L.oracle_faux_morts, L.oracle_ids_doubles,
              L.oracle_couverture_ko);
  if (judge > 0)
    std::printf("juge paires=%lld vivantes=%lld desaccords=%lld coeur_verifies=%lld coeur_faux=%lld\n",
                juge_paires, juge_vivantes, desaccords, L.coeur_verifies, L.coeur_faux);

  // ---- PORTES
  if (L.masse_totale != total) {
    std::fprintf(stderr, "PLANCHER : la WSPD ne partitionne pas (%lld contre %lld)\n",
                 L.masse_totale, total);
    return 3;
  }
  if (ha_verifie) {
    if (L.dual_verifies == 0) {
      std::fprintf(stderr, "PLANCHER : aucun point confronte, la porte est vacue\n");
      return 3;
    }
    if (L.dual_ecarts != 0) {
      std::fprintf(stderr, "PLANCHER : dual-tree != jointure sur %lld point(s)\n",
                   L.dual_ecarts);
      return 3;
    }
  }
  // CET INVARIANT EST CIRCULAIRE, ET CE N'EST PAS UNE PORTE DE SURETE.
  //
  // `vrai_vivantes` n'est incremente qu'APRES avoir etabli que la paire est
  // dans le residuel : il en resulte structurellement
  // `vrai_vivantes <= paires parcourues <= survivantes`. Une paire que le
  // prefiltre aurait fermee a tort n'est jamais examinee, donc jamais vue.
  // L'auditeur le demontre : avec `--fixture=coeur5 --inject=bulk-sans-masque`,
  // le mutant ferme une vraie paire q2 et ce mode imprime pourtant
  // `q2_vivantes=20 q2_mou=1.000` — il a simplement omis la vingt-et-unieme.
  // Seul l'oracle independant par `PairId` signale la fausse mort.
  //
  // Le test est conserve comme garde-fou d'implementation — une violation
  // signalerait un bug de comptage — jamais comme preuve de surete.
  if (vrai_vivant)
    for (int q = 0; q < 3; ++q)
      if (L.survivantes[q] < vrai_vivantes[q]) {
        std::fprintf(stderr,
                     "PLANCHER : lane q%d, %lld survivantes < %lld W-vivantes —"
                     " incoherence de comptage, PAS une preuve de fermeture a tort\n",
                     q + 2, L.survivantes[q], vrai_vivantes[q]);
        return 3;
      }
  if (mutant == Mutant::kNone && L.recouvrements != 0) {
    std::fprintf(stderr, "PLANCHER : recouvrement non nul sans mutant\n");
    return 3;
  }
  if (compare512) {
    if (L.c512_sites == 0) {
      std::fprintf(stderr, "PLANCHER : harnais apparie vide, aucun site confronte\n");
      return 3;
    }
    // Q21, cote surete : `corner512` ne doit JAMAIS certifier une lane que la
    // force brute sur les vraies paires du rectangle refute.
    for (int q = 0; q < 3; ++q)
      if (L.c512_faux[q] != 0) {
        std::fprintf(stderr, "PLANCHER : corner512 refute par la force brute, lane q%d (%lld)\n",
                     q + 2, L.c512_faux[q]);
        return 3;
      }
    if (L.c64_desaccords != 0) {
      std::fprintf(stderr, "PLANCHER : corner64 != corner512 sur %lld sites\n", L.c64_desaccords);
      return 3;
    }
  }
  if (min_rect > 0 && L.rectangles < min_rect) {
    std::fprintf(stderr, "PLANCHER : %lld rectangles, %d exiges\n", L.rectangles, min_rect);
    return 3;
  }
  if (min_ferme_q4 >= 0.0) {
    const double pct = 100.0 * (double)(total - L.survivantes[2]) / (double)total;
    if (pct < min_ferme_q4) {
      std::fprintf(stderr, "PLANCHER : fermeture q4 %.3f%% sous le plancher %.3f%%\n", pct,
                   min_ferme_q4);
      return 3;
    }
  }
  if (oracle) {
    if (L.oracle_couverture_ko > 0) {
      std::fprintf(stderr, "PLANCHER : %lld paires vues un nombre de fois != 1\n",
                   L.oracle_couverture_ko);
      return 3;
    }
    if (L.oracle_ids_doubles > 0) {
      std::fprintf(stderr, "DESACCORD DU JUGE : %lld PointId credites deux fois ou dans A/B\n",
                   L.oracle_ids_doubles);
      return 1;
    }
    if (L.oracle_faux_morts > 0) {
      std::fprintf(stderr, "DESACCORD DU JUGE : %lld ancres fermees a tort\n",
                   L.oracle_faux_morts);
      return 1;
    }
  }
  if (fixture) {
    // La voie rapide DOIT s'etre declenchee, sinon la fixture ne prouve rien.
    if (L.bulk_credits < 1) {
      std::fprintf(stderr, "PLANCHER : la voie rapide q2 ne s'est pas declenchee\n");
      return 3;
    }
    // L'ancre (a,b) de la fixture est le PairId des deux extremites ; elle a
    // cinq interieurs stricts, donc elle DOIT survivre a `h_2 = 10`.
    long long vivantes = 0;
    for (long long pp = 0; pp < npairs; ++pp)
      if (!ferme[(size_t)pp * 3]) ++vivantes;
    if (vivantes != npairs) {
      std::fprintf(stderr, "PLANCHER : %lld ancres q2 fermees sur la fixture, zero attendu\n",
                   npairs - vivantes);
      return 3;
    }
  }
  if (L.coeur_faux > 0) {
    if (mutant != Mutant::kNone) {
      std::fprintf(stderr, "MUTANT TUE : %s certifie %lld faux temoins de coeur\n",
                   mutant_name(mutant), L.coeur_faux);
      return 4;
    }
    std::fprintf(stderr, "DESACCORD DU JUGE : %lld faux temoins de coeur\n", L.coeur_faux);
    return 1;
  }
  if (desaccords > 0) {
    if (mutant != Mutant::kNone) {
      std::fprintf(stderr, "MUTANT TUE : %s produit %lld desaccords\n", mutant_name(mutant),
                   desaccords);
      return 4;
    }
    std::fprintf(stderr, "DESACCORD DU JUGE : %lld minorants depassent le compte vrai\n",
                 desaccords);
    return 1;
  }
  // Les mutants `vivant-*` sont hors du champ du juge par force brute : ils
  // n'alterent aucun certificat, seulement le compte de `V_q`. Leur mort est
  // constatee AILLEURS — par `audits/check_vivant_balayages.py`, qui confronte
  // le balayage fusionne mute au balayage legacy non mute. Les declarer
  // survivants ici masquerait ce montage derriere un code 3 sans rapport.
  if (mutant != Mutant::kNone && mutant != Mutant::kDropB &&
      mutant != Mutant::kIntervalXi && !mutant_vivant &&
      desaccords == 0 && L.coeur_faux == 0) {
    std::fprintf(stderr, "MUTANT SURVIVANT : %s n'a pas ete vu\n", mutant_name(mutant));
    return 3;
  }
  std::printf("OK : prefiltre combine mesure\n");
  return 0;
}
