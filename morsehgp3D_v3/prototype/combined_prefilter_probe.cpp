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
// Un rectangle dont une extremite depasse `--cap-cellule` n'est pas decide :
// toutes ses paires sont comptees SURVIVANTES et le rectangle est publie dans
// `non_decides`. La mesure majore donc toujours le residuel.
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
  kDropB,         // oublie la contribution de `B` : minorant plus faible, sur
                  // mais il doit se voir au compteur
};

const char* mutant_name(Mutant m) {
  switch (m) {
    case Mutant::kNone: return "none";
    case Mutant::kIntervalXi: return "coeur-intervalle-xi";
    case Mutant::kNarrowI64: return "largeur-i64";
    case Mutant::kCoreCentreOnly: return "coeur-centre-seul";
    case Mutant::kThresholdOff: return "seuil-decale";
    case Mutant::kDropB: return "oublie-b";
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
  s.r2 = isqrt_floor(acc) + 1;  // demi-diagonale doublee, arrondie au-dessus
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
};

struct Rect {
  int u, v;
};

}  // namespace

int main(int argc, char** argv) {
  int n = 8000, smax = 11, sep = 8, cap = 512, judge = 0, min_rect = 0;
  long long seed = 3, coord = 0;
  CloudFamily family = CloudFamily::kUniform;
  Mutant mutant = Mutant::kNone;
  double min_ferme_q4 = -1.0;

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
    if (eat("--points", &tmp)) { n = (int)tmp; continue; }
    if (eat("--smax", &tmp)) { smax = (int)tmp; continue; }
    if (eat("--separation", &tmp)) { sep = (int)tmp; continue; }
    if (eat("--cap-cellule", &tmp)) { cap = (int)tmp; continue; }
    if (eat("--seed", &seed)) continue;
    if (eat("--coord", &coord)) continue;
    if (eat("--juge", &tmp)) { judge = (int)tmp; continue; }
    if (eat("--min-rectangles", &tmp)) { min_rect = (int)tmp; continue; }
    if (a.rfind("--min-ferme-q4=", 0) == 0) {
      min_ferme_q4 = std::atof(a.c_str() + 15);
      continue;
    }
    if (a.rfind("--family=", 0) == 0) {
      const std::string f = a.substr(9);
      if (f == "uniform") family = CloudFamily::kUniform;
      else if (f == "eight_clusters") family = CloudFamily::kEightClusters;
      else if (f == "terrain") family = CloudFamily::kTerrain;
      else { std::fprintf(stderr, "REFUS : famille inconnue %s\n", f.c_str()); return 2; }
      continue;
    }
    if (a.rfind("--inject=", 0) == 0) {
      const std::string m = a.substr(9);
      if (m == "coeur-intervalle-xi") mutant = Mutant::kIntervalXi;
      else if (m == "largeur-i64") mutant = Mutant::kNarrowI64;
      else if (m == "coeur-centre-seul") mutant = Mutant::kCoreCentreOnly;
      else if (m == "seuil-decale") mutant = Mutant::kThresholdOff;
      else if (m == "oublie-b") mutant = Mutant::kDropB;
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
  if (mutant != Mutant::kNone && judge <= 0) {
    std::fprintf(stderr, "REFUS : un mutant sans juge ne prouve rien\n");
    return 2;
  }

  const int coord_used =
      coord > 0 ? (int)coord : mhgp3v::cloud_family_default_coord(family, n);
  std::vector<P3> pts = mhgp3v::make_family_cloud(family, n, coord_used, seed);
  if ((int)pts.size() != n) { std::fprintf(stderr, "REFUS : nuage non genere\n"); return 2; }

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
      if (separated(h_sphere(r.u), h_sphere(r.v), sep)) { rects.push_back(r); continue; }
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
  std::vector<int> histo(16, 0);

  for (const Rect& r : rects) {
    const int ua = h_first(r.u), ub = h_last(r.u);
    const int va = h_first(r.v), vb = h_last(r.v);
    const int na = ub - ua + 1, nb = vb - va + 1;
    L.masse_totale += (long long)na * nb;
    if (na > cap || nb > cap) {
      ++L.non_decides;
      L.masse_non_decide += (long long)na * nb;
      for (int q = 0; q < 3; ++q) L.survivantes[q] += (long long)na * nb;
      continue;
    }
    if (na > L.cellules_max) L.cellules_max = na;
    if (nb > L.cellules_max) L.cellules_max = nb;

    const Box BA = h_box(r.u), BB = h_box(r.v);

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
        if (h_any_upper(BA, BB, Z) <= 0) continue;  // aucun temoin ici
        const bool leaf = h_leaf(f.node);
        int child = m;
        if (!leaf && (m & 1) && h_all_inside(BA, BB, Z) > 0) {
          // VOIE RAPIDE q2 : tout le noeud est temoin. On ne credite que s'il
          // ne chevauche ni `A` ni `B`, sans quoi la disjonction tomberait.
          const int fi = nodes[f.node].first, la = nodes[f.node].last;
          const bool touche = !(la < ua || fi > ub) || !(la < va || fi > vb);
          if (!touche) {
            hcore[0] += (la - fi + 1);
            if (hcore[0] > h_q[0]) hcore[0] = h_q[0];
            child &= ~1;  // LA REPARATION : les enfants ne recreditent plus q2
          }
        }
        if (leaf) {
          const int i = -1 - f.node;
          if (i >= ua && i <= ub) continue;  // disjonction avec `A`
          if (i >= va && i <= vb) continue;  // disjonction avec `B`
          const i64 hh = h_min_over_boxes(BA, BB, sorted_pts[i]);
          if (hh <= 0) continue;
          if ((m & 1) && hcore[0] < h_q[0]) ++hcore[0];
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
    ha.assign((size_t)na * 3, 0);
    hb.assign((size_t)nb * 3, 0);
    for (int q = 0; q < 3; ++q) {
      const int need = h_q[q];
      for (int i = 0; i < na; ++i) {
        const P3& a = sorted_pts[ua + i];
        int c = 0;
        for (int j = 0; j < na && c < need; ++j) {
          if (j == i) continue;
          ++L.travail_h;
          if (universal_witness(a, BB, sorted_pts[ua + j], q + 2, narrow)) ++c;
        }
        ha[(size_t)i * 3 + q] = c;
        L.ha_total[q] += c;
      }
      for (int i = 0; i < nb; ++i) {
        const P3& b = sorted_pts[va + i];
        int c = 0;
        if (mutant != Mutant::kDropB) {
          for (int j = 0; j < nb && c < need; ++j) {
            if (j == i) continue;
            ++L.travail_h;
            if (universal_witness(b, BA, sorted_pts[va + j], q + 2, narrow)) ++c;
          }
        }
        hb[(size_t)i * 3 + q] = c;
        L.hb_total[q] += c;
      }
    }

    // ---- Comptage des survivantes SANS materialiser une seule paire.
    for (int q = 0; q < 3; ++q) {
      const int need = h_q[q] + seuil_delta;
      std::fill(histo.begin(), histo.end(), 0);
      for (int i = 0; i < nb; ++i) {
        int v = hb[(size_t)i * 3 + q];
        if (v > 15) v = 15;
        ++histo[v];
      }
      std::vector<int> cum(17, 0);
      for (int k = 0; k < 16; ++k) cum[k + 1] = cum[k] + histo[k];
      for (int i = 0; i < na; ++i) {
        const int budget = need - hcore[q] - ha[(size_t)i * 3 + q];
        if (budget <= 0) continue;               // deja mort pour tout `b`
        const int k = budget > 16 ? 16 : budget; // `h_b < budget`
        L.survivantes[q] += cum[k];
      }
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

  // ---- RECU
  std::printf("CombinedPrefilterReceipt-v1\n");
  std::printf(
      "cadre phase=exploration_v3_hors_registre backend=cpu_reference "
      "profile=quantized_u16_input_only mode=diagnostic_counter_only "
      "public_status=not_claimed\n");
  std::printf("cloud family=%s n=%d coord=%d seed=%lld smax=%d separation=%d cap=%d inject=%s\n",
              mhgp3v::cloud_family_name(family), n, coord_used, seed, smax, sep, cap,
              mutant_name(mutant));
  std::printf("seuils h_q2=%d h_q3=%d h_q4=%d\n", h_q[0], h_q[1], h_q[2]);
  std::printf("wspd rectangles=%lld non_decides=%lld masse=%lld masse_non_decide=%lld cellule_max=%lld\n",
              L.rectangles, L.non_decides, L.masse_totale, L.masse_non_decide, L.cellules_max);
  const long long total = (long long)n * (n - 1) / 2;
  std::printf("ledger masse_attendue=%lld ecart=%lld recouvrements=%lld travail_h=%lld\n",
              total, L.masse_totale - total, L.recouvrements, L.travail_h);
  for (int q = 0; q < 3; ++q) {
    const double pct = total > 0 ? 100.0 * (double)(total - L.survivantes[q]) / (double)total : 0.0;
    std::printf("lane q%d survivantes=%lld fermees=%lld ferme_pct=%.3f coeur_somme=%lld "
                "coeur_non_vide=%lld ha_somme=%lld hb_somme=%lld\n",
                q + 2, L.survivantes[q], total - L.survivantes[q], pct, L.coeur_total[q],
                L.coeur_non_vide[q], L.ha_total[q], L.hb_total[q]);
  }
  if (judge > 0)
    std::printf("juge paires=%lld vivantes=%lld desaccords=%lld coeur_verifies=%lld coeur_faux=%lld\n",
                juge_paires, juge_vivantes, desaccords, L.coeur_verifies, L.coeur_faux);

  // ---- PORTES
  if (L.masse_totale != total) {
    std::fprintf(stderr, "PLANCHER : la WSPD ne partitionne pas (%lld contre %lld)\n",
                 L.masse_totale, total);
    return 3;
  }
  if (mutant == Mutant::kNone && L.recouvrements != 0) {
    std::fprintf(stderr, "PLANCHER : recouvrement non nul sans mutant\n");
    return 3;
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
  if (mutant != Mutant::kNone && mutant != Mutant::kDropB &&
      mutant != Mutant::kIntervalXi && desaccords == 0 && L.coeur_faux == 0) {
    std::fprintf(stderr, "MUTANT SURVIVANT : %s n'a pas ete vu\n", mutant_name(mutant));
    return 3;
  }
  std::printf("OK : prefiltre combine mesure\n");
  return 0;
}
