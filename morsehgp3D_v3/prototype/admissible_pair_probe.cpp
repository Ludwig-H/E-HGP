// MorseHGP3D v3 — LA SÉLECTIVITÉ DU LEMME DU DEMI-BOULE, mesurée.
//
// La spécification du générateur direct repose sur une condition nécessaire exacte,
// démontrée sans hypothèse de régularité ni de zéro-extra-shell :
//
//   Si p et u appartiennent à la coquille d'une sphère critique de rang au plus
//   s_max, alors il existe un demi-espace fermé H dont le plan contient la droite
//   (p,u) tel que |X inter D_pu inter H| <= s_max, où D_pu est la boule diamétrale.
//
// La preuve tient en trois lignes : le centre c est équidistant de p et u, donc
// (c-m) est perpendiculaire à (u-p) ; d'où R² = rho² + d² ; et pour tout y du
// demi-boule du côté du centre, ||y-c||² = ||y-m||² - 2(y-m).(c-m) + d² <= R².
//
// LA VERSION OUVERTE N'EST PAS UN MEILLEUR FILTRE : C'EST LE FILTRE D'UN AUTRE
// UNIVERS, ET LA DISTINCTION EST TOUT LE SUJET.
//
// La même preuve, avec la boule diamétrale OUVERTE, donne une inégalité stricte :
// tout y de D°_pu inter H est STRICTEMENT intérieur à la sphère critique, donc
// appartient à I. Avec
//
//   A(p,u) = 2 + min_H |(X \ {p,u}) inter D°_pu inter H|,
//
// on obtient A(p,u) <= 2 + |I| <= q + |I| <= s_max, où q est l'ARITÉ du support.
//
// Les deux lemmes ne bornent donc pas la même chose :
//
//   fermé   |X inter D_pu inter H|  <= |S| + |I|   le RANG FERMÉ
//   ouvert  2 + |X° inter D° inter H| <= q + |I|   l'arité plus l'intérieur
//
// Le catalogue de rang fermé exige |S| + |I| <= s_max ; la source Gabriel
// ouverte n'exige que q + |I| <= s_max et laisse l'extra-shell |S| - q libre.
// Sur le catalogue fermé, le lemme fermé est donc valide ET plus sélectif, car
// D° est inclus dans D donne A_ouvert <= A_ferme, donc {A_ouvert <= s_max}
// CONTIENT {A_ferme <= s_max} : l'ouvert admet PLUS de paires. Sur la source
// ouverte, en revanche, le lemme fermé n'est pas valide du tout — une sphère à
// grand extra-shell a |S| + |I| > s_max sans violer q + |I| <= s_max — et seul
// le lemme ouvert survit.
//
// Les deux sont mesurés ici, contre la vérité du catalogue FERMÉ, pour laquelle
// les deux sont nécessaires ; c'est la seule vérité dont ce programme dispose.
// Le nombre `ADMISES ouvert` ne dimensionne donc PAS la source ouverte, dont la
// vérité n'est pas énumérée ici.
//
// ---------------------------------------------------------------------------
// LES SUPPORTS SONT DES CLIQUES DU GRAPHE ADMISSIBLE, ET C'EST LÀ QUE SE JOUE
// LE COÛT DE LA SOURCE DIRECTE
// ---------------------------------------------------------------------------
//
// Le lemme s'applique à CHAQUE paire d'une coquille critique. Un support d'arité
// trois a donc ses trois paires admissibles, un support d'arité quatre ses six :
//
//   support d'arité 3  =>  TRIANGLE du graphe admissible G
//   support d'arité 4  =>  K4 du graphe admissible G
//
// C'est une réduction structurelle, pas une heuristique, et elle est mesurable.
// La source directe, elle, énumère aujourd'hui tous les $(q-1)$-sous-ensembles du
// voisinage du cover, dont le degré mesuré vaut environ 1 450 au profil produit :
// $C_4=\sum_p\binom{d^{+}}{3}$ y vaut $3{,}2\cdot10^{12}$ à 50 k, ce qui la
// réfute. Le degré de $G$, lui, est presque un ordre de grandeur plus petit.
//
// Ce programme compte donc les triangles et les K4 de $G$, et les compare aux
// vrais supports d'arité trois et quatre du catalogue. Le rapport est le facteur
// de travail gaspillé d'un générateur par cliques; sa croissance décide si cette
// voie tient à 50 k.
//
// ---------------------------------------------------------------------------
// LE LEMME DE TRIPLE, ET POURQUOI IL EST BEAUCOUP PLUS FORT QUE CELUI DE PAIRE
// ---------------------------------------------------------------------------
//
// Pour une PAIRE, le plan frontière peut tourner autour de la droite $(p,u)$ :
// le minimum porte sur une famille continue de demi-espaces, et il faut un
// balayage angulaire. Pour un TRIPLE non aligné, le plan est FIXÉ — c'est le
// plan de $T$ — et il ne reste que **deux** demi-espaces à tester.
//
// Soit $T$ un triple de la coquille d'une sphère critique de centre $c_0$ et de
// rayon $R$, avec $\lvert S\rvert+\lvert I\rvert\leq s_{\max}$. Soit $\pi$ le
// plan de $T$, $m$ le circumcentre de $T$ — qui est la projection orthogonale de
// $c_0$ sur $\pi$ —, $\rho$ son circumrayon et $d=\lVert m-c_0\rVert$. En
// coordonnées où $\pi=\{y_3=0\}$, $m=0$ et $c_0=(0,0,d)$ avec $d\geq0$ :
//
//   $y\in D_T$ donne $\lVert y\rVert^2\leq\rho^2$, et
//   $\lVert y-c_0\rVert^2=\lVert y\rVert^2-2y_3d+d^2\leq\rho^2+d^2=R^2$
//   dès que $y_3\geq0$.
//
// Donc tout point de la circumboule fermée $D_T$ situé **du côté du centre**
// appartient à la boule critique, et
//
//   $A_3(T)=\min\left(\lvert X\cap D_T\cap H^{+}\rvert,
//                      \lvert X\cap D_T\cap H^{-}\rvert\right)\leq s_{\max}$.
//
// Aucune hypothèse de bon centrage n'est requise : $m$ est le circumcentre, pas
// la miniboule. Le prédicat est entier et déjà écrit — `sphere_side` de la
// sphère de support $T$ pour l'appartenance, `orient3d` pour le côté —, et les
// points du plan comptent des deux côtés.
//
// ---------------------------------------------------------------------------
// CE QUE CE PROGRAMME MESURE, ET POURQUOI C'EST LUI QUI DÉCIDE
// ---------------------------------------------------------------------------
//
// Le budget du générateur direct dépend de deux quantités que la spécification
// déclare elle-même NON BORNÉES : la taille de l'univers admissible et celle des
// candidats d'arité trois. Si l'univers admissible est de l'ordre de n², la route
// meurt quel que soit le débit ; s'il est de l'ordre de la sortie, elle vit.
//
// On mesure donc trois nombres sur le même nuage :
//   * VRAI    : les paires qui apparaissent réellement dans une coquille critique,
//               lues sur le catalogue exact de référence ;
//   * ADMIS   : les paires que le lemme laisse passer ;
//   * TOTAL   : les C(n,2) paires.
// La sélectivité utile est ADMIS/VRAI — le facteur de travail gaspillé —, et
// ADMIS/n dit si l'univers est linéaire ou quadratique.
//
// ---------------------------------------------------------------------------
// LE MINIMUM SUR LES DEMI-PLANS FERMÉS N'EST PAS ATTEINT SUR UNE DIRECTION VIVE
// ---------------------------------------------------------------------------
//
// C'était le P0 de ce fichier, et il RENVERSAIT sa conclusion. La première
// version ne testait que les directions portées par les points eux-mêmes, puis
// comptait la frontière avec `cross >= 0`. En plaçant la frontière juste APRÈS
// un groupe colinéaire, ce groupe passe dans le demi-plan ouvert complémentaire
// alors que toute direction vive le remet sur la frontière et le compte. Le
// minimum rendu était donc TROP GRAND, le filtre rejetait des paires vraies, et
// les masses `ADMIS` publiées étaient des sous-estimations.
//
// La réparation est une identité, pas une heuristique. Un demi-plan fermé et le
// demi-plan ouvert opposé partitionnent le plan privé de l'origine :
//
//   min_H |P inter H_ferme| = |P| - max_H |P inter H_ouvert|.
//
// Le maximum sur les demi-plans OUVERTS, lui, est atteint sur un arc semi-ouvert
// [theta_i, theta_i + pi) porté par un point : si l'arc ouvert optimal contient
// des points, soit theta_i l'angle du premier d'entre eux ; tous les autres sont
// dans [theta_i, theta_i + pi), et cet arc semi-ouvert est réalisable par un
// demi-plan ouvert en reculant la frontière d'un epsilon sans point. Un balayage
// à deux pointeurs sur les angles triés le calcule exactement, sans jamais
// évaluer un angle : l'appartenance à [theta_i, theta_i + pi) s'écrit
// `cross(d,y) > 0 ou (cross(d,y) == 0 et dot(d,y) > 0)`.
//
// ---------------------------------------------------------------------------
// LA PROJECTION RESTE ÉTROITE, ET C'EST DÉMONTRABLE
// ---------------------------------------------------------------------------
//
// L'ancienne base entière du plan multipliait `d x e` par `d x (d x e)` et
// produisait des coordonnées jusqu'à 2^73 : le produit croisé de deux telles
// coordonnées atteint 2^147 et DÉBORDE `i128`. Ce n'était invisible que parce
// que l'emprise mesurée était minuscule devant la grille déclarée.
//
// On prend donc directement r = d x e, qui est déjà dans d^perp, avec
// r = 2 (u-p) x (z-p) donc |r_i| < 2^34 sur u16. `r = 0` caractérise exactement
// les points de la droite (p,u) : avec q la projection de e sur d^perp on a
// r = d x q et d x (d x q) = -||d||^2 q. On choisit ensuite l'axe canonique k
// le plus petit tel que d_k != 0 et on garde les deux autres composantes dans
// l'ordre : la restriction à d^perp de cette projection est un isomorphisme
// linéaire, donc elle envoie droites sur droites et demi-plans sur demi-plans,
// et le minimum cherché est inchangé. Les déterminants restent sous 2^69.
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>
#include <array>
#include <set>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/order_k_flats.hpp"

using mhgp::P3;
using mhgp::i32;
using mhgp::i128;

namespace {

struct Planar {
  i128 x = 0, y = 0;
};

inline i128 cross2(const Planar& a, const Planar& b) { return a.x * b.y - a.y * b.x; }
inline i128 dot2(const Planar& a, const Planar& b) { return a.x * b.x + a.y * b.y; }

// L'ordre angulaire, sans angle : d'abord le demi-tour (y > 0, ou y == 0 et
// x > 0) contre l'autre, puis le signe du produit croisé. Deux points du même
// rayon sont égaux pour cet ordre — c'est exactement ce qu'exige le balayage.
inline int angular_half(const Planar& a) {
  if (a.y > 0) return 0;
  if (a.y < 0) return 1;
  return a.x > 0 ? 0 : 1;
}

inline bool angular_less(const Planar& a, const Planar& b) {
  const int ha = angular_half(a), hb = angular_half(b);
  if (ha != hb) return ha < hb;
  return cross2(a, b) > 0;
}

// `y` est-il dans l'arc semi-ouvert [angle(d), angle(d) + pi) ?
inline bool in_half_open_arc(const Planar& d, const Planar& y) {
  const i128 c = cross2(d, y);
  if (c > 0) return true;
  if (c < 0) return false;
  return dot2(d, y) > 0;                    // même rayon : oui ; antipode : non
}

// Le maximum, sur les demi-plans OUVERTS bordés par une droite passant par
// l'origine, du nombre de points. Balayage à deux pointeurs sur l'ordre
// angulaire ; les points doivent être non nuls.
int maximum_open_halfplane(std::vector<Planar>& points) {
  const int m = (int)points.size();
  if (m == 0) return 0;
  std::sort(points.begin(), points.end(), angular_less);
  int best = 0, j = 0;
  for (int i = 0; i < m; ++i) {
    if (j < i) j = i;
    while (j < i + m && in_half_open_arc(points[(std::size_t)i], points[(std::size_t)(j % m)]))
      ++j;
    best = std::max(best, j - i);
  }
  return best;
}

// Le minimum, sur les demi-plans FERMÉS, du nombre de points ; `always_inside`
// compte les points de la droite frontière, qui appartiennent à tous.
int minimum_closed_halfplane(std::vector<Planar>& points, int always_inside) {
  return always_inside + (int)points.size() - maximum_open_halfplane(points);
}

// LE MUTANT. C'est la version réfutée : seules les directions vives, frontière
// comptée. Elle n'est conservée que pour que la fixture puisse PROUVER qu'elle
// sépare encore les deux calculs. Un `min` sur un sur-ensemble de directions ne
// peut que baisser, donc `naive >= exact` toujours.
int minimum_closed_halfplane_live_directions(const std::vector<Planar>& points,
                                             int always_inside) {
  const int m = (int)points.size();
  if (m == 0) return always_inside;
  int best = m + always_inside;
  for (int i = 0; i < m; ++i)
    for (int flip = 0; flip < 2; ++flip) {
      Planar d = points[(std::size_t)i];
      if (flip) { d.x = -d.x; d.y = -d.y; }
      if (d.x == 0 && d.y == 0) continue;
      int count = 0;
      for (int j = 0; j < m; ++j)
        if (cross2(d, points[(std::size_t)j]) >= 0) ++count;
      best = std::min(best, count + always_inside);
    }
  return best;
}

// ---------------------------------------------------------------------------
// LA PROJECTION D'UNE PAIRE
// ---------------------------------------------------------------------------
//
// Rend le nombre minimal exact de points dans un demi-espace fermé bordé par un
// plan contenant la droite (p,u), pour la boule diamétrale fermée puis ouverte.
struct PairMinima {
  int closed_exact = 0;                 // 2 + points de D_pu fermee, minimises
  int closed_live = 0;                  // le mutant, sur les memes donnees
  int open_exact = 0;                   // 2 + points de D°_pu, minimises
  long long ball_points = 0;            // points visites dans la boule fermee
};

PairMinima pair_minima(const std::vector<P3>& pts, i32 a, i32 b, bool want_live) {
  PairMinima out;
  const P3& pa = pts[(std::size_t)a];
  const P3& pb = pts[(std::size_t)b];
  const i128 dx = (i128)pb.x - pa.x, dy = (i128)pb.y - pa.y, dz = (i128)pb.z - pa.z;
  const i128 diameter2 = dx * dx + dy * dy + dz * dz;
  // L'axe canonique : le plus petit indice ou d ne s'annule pas. La paire est
  // faite de deux points DISTINCTS, donc il existe.
  const int axis = (dx != 0) ? 0 : ((dy != 0) ? 1 : 2);
  std::vector<Planar> closed, open;
  int line_closed = 0, line_open = 0;
  const int n = (int)pts.size();
  for (i32 z = 0; z < n; ++z) {
    if (z == a || z == b) continue;
    const P3& pz = pts[(std::size_t)z];
    const i128 ex = 2 * (i128)pz.x - pa.x - pb.x;
    const i128 ey = 2 * (i128)pz.y - pa.y - pb.y;
    const i128 ez = 2 * (i128)pz.z - pa.z - pb.z;
    const i128 norm = ex * ex + ey * ey + ez * ez;
    if (norm > diameter2) continue;                       // hors de la boule fermee
    const bool strictly_inside = norm < diameter2;
    ++out.ball_points;
    const i128 rx = dy * ez - dz * ey;
    const i128 ry = dz * ex - dx * ez;
    const i128 rz = dx * ey - dy * ex;
    if (rx == 0 && ry == 0 && rz == 0) {                  // sur la droite (p,u)
      ++line_closed;
      if (strictly_inside) ++line_open;
      continue;
    }
    Planar q{};
    if (axis == 0) { q.x = ry; q.y = rz; }
    else if (axis == 1) { q.x = rx; q.y = rz; }
    else { q.x = rx; q.y = ry; }
    closed.push_back(q);
    if (strictly_inside) open.push_back(q);
  }
  out.ball_points += 2;
  if (want_live) out.closed_live = minimum_closed_halfplane_live_directions(closed, line_closed + 2);
  out.open_exact = minimum_closed_halfplane(open, line_open + 2);
  out.closed_exact = minimum_closed_halfplane(closed, line_closed + 2);
  return out;
}

// Le filtre de triple : deux demi-espaces, bornés par le plan de T.
bool triple_admissible(const std::vector<P3>& pts, i32 a, i32 b, i32 c, int smax,
                       long long* tests) {
  mhgp::Sphere sph{};
  if (!mhgp::sphere3(pts[(std::size_t)a], pts[(std::size_t)b], pts[(std::size_t)c], &sph))
    return true;                     // alignés : aucun plan, le lemme ne dit rien
  int plus = 0, minus = 0;
  const int n = (int)pts.size();
  for (i32 z = 0; z < n; ++z) {
    ++*tests;
    if (mhgp::sphere_side(sph, pts[(std::size_t)z]) > 0) continue;
    const i128 orient = mhgp3v::flats::orient3d_exact(
        pts[(std::size_t)a], pts[(std::size_t)b], pts[(std::size_t)c], pts[(std::size_t)z]);
    if (orient >= 0) ++plus;
    if (orient <= 0) ++minus;        // le plan compte des DEUX côtés
  }
  return (plus < minus ? plus : minus) <= smax;
}

P3 pt(int x, int y, int z) {
  P3 p{};
  p.x = (i32)x; p.y = (i32)y; p.z = (i32)z;
  return p;
}

// ---------------------------------------------------------------------------
// LES FIXTURES PERMANENTES DU BALAYAGE
// ---------------------------------------------------------------------------
//
// La première est le contre-exemple entier de l'audit : le minimum exact vaut 2,
// la version par directions vives rend 5. Sans elle, la réparation du balayage
// n'aurait aucun témoin, et une régression la rendrait silencieusement.
//
// La seconde sépare les TROIS contrats : boule ouverte, boule fermée exacte,
// boule fermée par directions vives, sur 3 / 4 / 5.
//
// Les trois suivantes couvrent les dégénérescences que le balayage doit traiter
// nommément : un groupe antipodal exact, des rayons confondus en nombre, et des
// points de la droite (qui sont dans tous les demi-plans).
int run_fixtures() {
  int faults = 0;

  {
    // centre=(100,100,100), rayon^2=194 ; les quatre supports sont a distance
    // carree 194 du centre, affinement independants et de moyenne nulle, donc le
    // centre est strictement interieur a leur convexe. Les trois extras sont
    // dans la boule diametrale de la paire {0,1}, hors de la sphere critique, et
    // portes par un MEME rayon projete : c'est ce qui piege les directions vives.
    const std::vector<P3> cloud{pt(113, 100, 95), pt(113, 100, 105), pt(87, 105, 100),
                                pt(87, 95, 100),  pt(114, 100, 100), pt(115, 100, 100),
                                pt(116, 100, 100)};
    const PairMinima m = pair_minima(cloud, 0, 1, true);
    if (m.closed_exact != 2 || m.closed_live != 5) {
      printf("[fixture demi-plan] contre-exemple de l'audit : exact=%d (attendu 2)"
             "  directions vives=%d (attendu 5)\n", m.closed_exact, m.closed_live);
      ++faults;
    } else {
      printf("[fixture demi-plan] contre-exemple de l'audit : exact=2, directions vives=5,"
             " la paire critique est conservee\n");
    }
  }

  {
    // p, u, un point de la droite au centre, et deux extra-shells orthogonaux
    // exactement sur la sphere diametrale.
    const std::vector<P3> cloud{pt(99, 100, 100), pt(101, 100, 100), pt(100, 100, 100),
                                pt(100, 101, 100), pt(100, 99, 100)};
    const PairMinima m = pair_minima(cloud, 0, 1, true);
    if (m.open_exact != 3 || m.closed_exact != 4 || m.closed_live != 5) {
      printf("[fixture demi-plan] trois contrats : ouvert=%d (3) ferme exact=%d (4)"
             " ferme vif=%d (5)\n", m.open_exact, m.closed_exact, m.closed_live);
      ++faults;
    } else {
      printf("[fixture demi-plan] trois contrats separes : ouvert=3, ferme exact=4,"
             " ferme par directions vives=5\n");
    }
  }

  {
    // ANTIPODES EXACTS. Deux points diametralement opposes dans la projection :
    // aucun demi-plan ferme ne peut les exclure tous les deux, mais un seul
    // suffit. Le maximum ouvert vaut 1, donc le minimum ferme vaut 2 + 1.
    const std::vector<P3> cloud{pt(0, 0, 0), pt(0, 0, 20), pt(4, 0, 10), pt(-4, 0, 10)};
    const PairMinima m = pair_minima(cloud, 0, 1, true);
    if (m.closed_exact != 3) {
      printf("[fixture demi-plan] antipodes : ferme exact=%d (attendu 3)\n", m.closed_exact);
      ++faults;
    } else {
      printf("[fixture demi-plan] antipodes exacts : un seul des deux est evitable\n");
    }
  }

  {
    // RAYONS CONFONDUS. Cinq points sur un meme rayon projete : la frontiere
    // placee juste apres le groupe les exclut TOUS. Le minimum ferme vaut donc
    // exactement 2, quel que soit le cardinal du groupe.
    std::vector<P3> cloud{pt(0, 0, 0), pt(0, 0, 40)};
    for (int i = 1; i <= 5; ++i) cloud.push_back(pt(i, 0, 20));
    const PairMinima m = pair_minima(cloud, 0, 1, true);
    if (m.closed_exact != 2 || m.closed_live != 7) {
      printf("[fixture demi-plan] rayons confondus : exact=%d (2) vifs=%d (7)\n",
             m.closed_exact, m.closed_live);
      ++faults;
    } else {
      printf("[fixture demi-plan] cinq rayons confondus : exact=2, vifs=7 — l'ecart croit"
             " avec le groupe\n");
    }
  }

  {
    // POINTS DE LA DROITE. Ils sont dans TOUS les demi-espaces bordes par un plan
    // qui contient cette droite : ils ne peuvent jamais etre evites.
    const std::vector<P3> cloud{pt(0, 0, 0), pt(0, 0, 60), pt(0, 0, 10), pt(0, 0, 20),
                                pt(0, 0, 50)};
    const PairMinima m = pair_minima(cloud, 0, 1, true);
    if (m.closed_exact != 5 || m.open_exact != 5) {
      printf("[fixture demi-plan] droite : ferme=%d ouvert=%d (attendu 5 et 5)\n",
             m.closed_exact, m.open_exact);
      ++faults;
    } else {
      printf("[fixture demi-plan] trois points de la droite : inevitables, ferme=ouvert=5\n");
    }
  }

  {
    // LE MUTANT DOIT ETRE DOMINE PARTOUT, JAMAIS EN DESSOUS. Un balayage qui
    // rendrait un minimum plus PETIT que le vrai admettrait des paires a tort et
    // ne serait pas non plus le bon filtre. On verifie l'inegalite sur un nuage
    // pseudo-aleatoire deterministe, et on exige au moins un ecart strict.
    std::mt19937 rng(20260810U);
    std::uniform_int_distribution<int> pick(0, 30);
    std::vector<P3> cloud;
    for (int i = 0; i < 24; ++i) cloud.push_back(pt(pick(rng), pick(rng), pick(rng)));
    std::sort(cloud.begin(), cloud.end(), [](const P3& x, const P3& y) {
      if (x.x != y.x) return x.x < y.x;
      if (x.y != y.y) return x.y < y.y;
      return x.z < y.z;
    });
    cloud.erase(std::unique(cloud.begin(), cloud.end(),
                            [](const P3& x, const P3& y) {
                              return x.x == y.x && x.y == y.y && x.z == y.z;
                            }),
                cloud.end());
    long long strict = 0, order_faults = 0, open_faults = 0;
    for (i32 a = 0; a + 1 < (i32)cloud.size(); ++a)
      for (i32 b = a + 1; b < (i32)cloud.size(); ++b) {
        const PairMinima m = pair_minima(cloud, a, b, true);
        if (m.closed_live < m.closed_exact) ++order_faults;
        if (m.open_exact > m.closed_exact) ++open_faults;
        if (m.closed_live > m.closed_exact) ++strict;
      }
    if (order_faults || open_faults || strict == 0) {
      printf("[fixture demi-plan] ordre des trois contrats : %lld inversions vif<exact,"
             " %lld inversions ouvert>ferme, %lld ecarts stricts\n",
             order_faults, open_faults, strict);
      ++faults;
    } else {
      printf("[fixture demi-plan] ordre verifie sur %zu points : ouvert <= ferme <= vif,"
             " %lld ecarts stricts\n", cloud.size(), strict);
    }
  }

  return faults;
}

}  // namespace

int main(int argc, char** argv) {
  int n = 200, smax = 11, repeats = 1, ranks = 1, mutant = 1;
  long long seed = 20260809;
  long long min_true = 0, min_admitted = 0;
  const double density = 1.0e-3;
  auto integer = [](const char* text, long long* value) {
    const char* first = text;
    const char* last = text + strlen(text);
    if (first == last) return false;
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(first, last, magnitude);
    if (r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 100000000ULL) return false;
    *value = (long long)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    long long value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    int* target = nullptr;
    long long* wide = nullptr;
    if (!strcmp(argv[i], "--points")) target = &n;
    else if (!strcmp(argv[i], "--smax")) target = &smax;
    else if (!strcmp(argv[i], "--repeats")) target = &repeats;
    else if (!strcmp(argv[i], "--ranks")) target = &ranks;
    else if (!strcmp(argv[i], "--mutant")) target = &mutant;
    else if (!strcmp(argv[i], "--min-true")) wide = &min_true;
    else if (!strcmp(argv[i], "--min-admitted")) wide = &min_admitted;
    else if (!strcmp(argv[i], "--seed")) {
      if (!has) { printf("ECHEC : --seed invalide\n"); return 2; }
      ++i; seed = value; continue;
    } else { printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    if (!has) { printf("ECHEC : valeur invalide pour %s\n", argv[i]); return 2; }
    ++i;
    if (wide != nullptr) *wide = value;
    else *target = (int)value;
  }
  if (n < 8 || n > 5000 || smax < 3 || smax > mhgp::kMaxRank || repeats < 1 || repeats > 20 ||
      ranks < 0 || ranks > 1 || mutant < 0 || mutant > 1) {
    printf("ECHEC : campagne absurde\n");
    return 2;
  }
  // La matrice des rangs est en O(n^2) : elle est explicitement plafonnee, et son
  // absence est DITE, jamais silencieuse.
  if (ranks == 1 && n > 2000) {
    printf("ECHEC : --ranks 1 au-dela de 2000 points demanderait une matrice n^2 ;"
           " passer --ranks 0\n");
    return 2;
  }

  const int fixture_faults = run_fixtures();
  if (fixture_faults != 0) {
    printf("ECHEC : %d fixture(s) du balayage en demi-plan\n", fixture_faults);
    return 1;
  }

  const double volume = (double)n / density;
  const int span = (int)std::llround(std::cbrt(volume));
  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, span - 1);

  long long sum_true = 0, sum_admitted_closed = 0, sum_admitted_open = 0, sum_admitted_live = 0;
  long long sum_total = 0, sum_missing_closed = 0, sum_missing_open = 0, sum_missing_live = 0;
  long long sum_ball_points = 0;
  // LE RANG k-NN D'UNE PAIRE. Si les paires admissibles etaient toujours proches
  // au sens du voisinage, une enumeration par k plus proches voisins les
  // trouverait toutes et le cout deviendrait O(nk). C'est cette question, et pas
  // le nombre de paires, qui decide du COUT de l'enumeration.
  //
  // LE RANG EST DE COMPETITION, pas d'insertion : rang(a,b) = 1 + le nombre de
  // points STRICTEMENT plus proches de a que b. Un tri stable avec depart par
  // PointId faisait dependre le rang de la numerotation des ex aequo.
  //
  // Et il est mesure sur les paires VRAIES independamment de l'admission : un
  // rang maximum calcule dans le seul ensemble admis ne dit rien d'un filtre qui
  // aurait deja supprime la paire genante.
  int rank_max_admitted = 0, rank_max_true = 0;
  std::vector<long long> rank_histogram(64, 0);
  long long rank_samples = 0;
  double sum_seconds = 0, sum_rank_seconds = 0, sum_clique_seconds = 0;
  int decided = 0;
  int refused_status = 0;
  long long sum_triangles = 0, sum_k4 = 0, sum_true_arity3 = 0, sum_true_arity4 = 0;
  long long sum_edges = 0, degree_g_max = 0, sum_triangle_probes = 0, sum_k4_probes = 0;
  long long sum_triples_kept = 0, sum_k4_kept = 0, sum_triple_tests = 0;
  long long sum_triple_missing = 0;

  for (int r = 0; r < repeats; ++r) {
    std::vector<P3> pts;
    {
      std::set<long long> keys;
      for (int guard = 0; (int)pts.size() < n && guard < 60 * n; ++guard) {
        P3 q{};
        q.x = (i32)pick(rng); q.y = (i32)pick(rng); q.z = (i32)pick(rng);
        const long long key = ((long long)q.x << 34) | ((long long)q.y << 17) | (long long)q.z;
        if (!keys.insert(key).second) continue;
        pts.push_back(q);
      }
    }
    if ((int)pts.size() < n) { printf("ECHEC : nuage non genere\n"); return 3; }
    // LE DIGEST DU NUAGE. Une graine ne suffit pas a un recu inter-machine :
    // `std::uniform_int_distribution` n'est pas un flux portable. Le digest, lui,
    // identifie l'entree effectivement mesuree.
    unsigned long long digest = 1469598103934665603ULL;
    for (const P3& q : pts)
      for (int c = 0; c < 3; ++c) {
        const unsigned long long v =
            (unsigned long long)(c == 0 ? q.x : (c == 1 ? q.y : q.z));
        for (int byte = 0; byte < 4; ++byte) {
          digest ^= (v >> (8 * byte)) & 0xFFULL;
          digest *= 1099511628211ULL;
        }
      }

    // LA VÉRITÉ : les paires qui apparaissent dans une coquille critique.
    mhgp3v::FlatStatistics st{};
    mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
    const mhgp::Catalogue cat = mhgp3v::flat_catalogue(pts, smax, &st, &status, false, true);
    if (status != mhgp3v::CloudStatus::kOk) {
      printf("  nuage %d digest=%016llx statut %s : REFUSE\n", r, digest,
             mhgp3v::cloud_status_name(status));
      ++refused_status;
      continue;
    }
    std::set<std::pair<i32, i32>> truth;
    long long true_arity3 = 0, true_arity4 = 0;
    for (const auto& sphere : cat.spheres) {
      std::vector<i32> shell;
      for (int i = 0; i < sphere.rank; ++i) {
        const i32 z = cat.members[(std::size_t)(sphere.members_begin + i)];
        if (mhgp::sphere_side(sphere.sph, pts[(std::size_t)z]) == 0) shell.push_back(z);
      }
      for (std::size_t a = 0; a < shell.size(); ++a)
        for (std::size_t b = a + 1; b < shell.size(); ++b)
          truth.insert({shell[a], shell[b]});
      if (sphere.n_support == 3) ++true_arity3;
      if (sphere.n_support == 4) ++true_arity4;
    }
    sum_true_arity3 += true_arity3;
    sum_true_arity4 += true_arity4;

    // Rang de competition de chaque point depuis chaque autre. Le cout de cette
    // construction est CHRONOMETRE A PART : c'est un diagnostic, il n'a pas le
    // droit de se cacher dans le temps du filtre.
    std::vector<int> rank_of;
    if (ranks == 1) {
      const auto rt0 = std::chrono::steady_clock::now();
      rank_of.assign((std::size_t)n * (std::size_t)n, 0);
      std::vector<std::pair<i128, i32>> order;
      for (i32 a = 0; a < n; ++a) {
        order.clear();
        for (i32 z = 0; z < n; ++z) {
          if (z == a) continue;
          const i128 ddx = (i128)pts[(std::size_t)z].x - pts[(std::size_t)a].x;
          const i128 ddy = (i128)pts[(std::size_t)z].y - pts[(std::size_t)a].y;
          const i128 ddz = (i128)pts[(std::size_t)z].z - pts[(std::size_t)a].z;
          order.push_back({ddx * ddx + ddy * ddy + ddz * ddz, z});
        }
        std::sort(order.begin(), order.end(),
                  [](const std::pair<i128, i32>& x, const std::pair<i128, i32>& y) {
                    return x.first < y.first;
                  });
        // Ex aequo GEOMETRIQUES groupes : meme distance carree, meme rang.
        std::size_t i = 0;
        while (i < order.size()) {
          std::size_t j = i;
          while (j < order.size() && order[j].first == order[i].first) ++j;
          for (std::size_t t = i; t < j; ++t)
            rank_of[(std::size_t)a * (std::size_t)n + (std::size_t)order[t].second] = (int)i + 1;
          i = j;
        }
      }
      sum_rank_seconds +=
          std::chrono::duration<double>(std::chrono::steady_clock::now() - rt0).count();
      for (const auto& pair : truth) {
        const int rank = std::min(rank_of[(std::size_t)pair.first * (std::size_t)n +
                                          (std::size_t)pair.second],
                                  rank_of[(std::size_t)pair.second * (std::size_t)n +
                                          (std::size_t)pair.first]);
        rank_max_true = std::max(rank_max_true, rank);
      }
    }

    const auto t0 = std::chrono::steady_clock::now();
    // LE GRAPHE ADMISSIBLE, en listes de voisins SUPÉRIEURS. Le filtre fermé est
    // le plus sélectif des deux sur le catalogue fermé, donc c'est lui qui sert
    // de générateur de candidats.
    std::vector<std::vector<i32>> forward_edges((std::size_t)n);
    long long admitted_closed = 0, admitted_open = 0, admitted_live = 0;
    long long missing_closed = 0, missing_open = 0, missing_live = 0, ball_points = 0;
    for (i32 a = 0; a < n; ++a)
      for (i32 b = a + 1; b < n; ++b) {
        const PairMinima m = pair_minima(pts, a, b, mutant == 1);
        ball_points += m.ball_points;
        const bool is_true = truth.count({a, b}) != 0;
        if (m.closed_exact <= smax) {
          ++admitted_closed;
          forward_edges[(std::size_t)a].push_back(b);
        } else if (is_true) ++missing_closed;
        if (m.open_exact <= smax) ++admitted_open; else if (is_true) ++missing_open;
        if (mutant == 1) {
          if (m.closed_live <= smax) ++admitted_live; else if (is_true) ++missing_live;
        }
        if (ranks == 1 && m.open_exact <= smax) {
          const int rank = std::min(rank_of[(std::size_t)a * (std::size_t)n + (std::size_t)b],
                                    rank_of[(std::size_t)b * (std::size_t)n + (std::size_t)a]);
          rank_max_admitted = std::max(rank_max_admitted, rank);
          int bucket = 0;
          while (bucket < 62 && (1 << (bucket + 1)) <= rank) ++bucket;
          ++rank_histogram[(std::size_t)bucket];
          ++rank_samples;
        }
      }
    const auto t1 = std::chrono::steady_clock::now();

    // LES CLIQUES. Un triangle se teste en cherchant l'arête (u,v) parmi les
    // voisins supérieurs de u; un K4 en intersectant les voisins supérieurs des
    // trois sommets d'un triangle. Les SONDES sont comptées à part du résultat :
    // c'est le travail, pas la sortie.
    const auto c0 = std::chrono::steady_clock::now();
    long long triangles = 0, k4 = 0, triangle_probes = 0, k4_probes = 0;
    long long triples_kept = 0, k4_kept = 0, triple_tests = 0, triple_missing = 0;
    // LA VÉRITÉ DES TRIPLES : les 3-sous-ensembles d'une coquille critique. Le
    // lemme de triple doit tous les conserver, sinon il est FAUX.
    std::set<std::array<i32, 3>> truth_triples;
    for (const auto& sphere : cat.spheres) {
      std::vector<i32> shell;
      for (int i = 0; i < sphere.rank; ++i) {
        const i32 z = cat.members[(std::size_t)(sphere.members_begin + i)];
        if (mhgp::sphere_side(sphere.sph, pts[(std::size_t)z]) == 0) shell.push_back(z);
      }
      std::sort(shell.begin(), shell.end());
      for (std::size_t x = 0; x < shell.size(); ++x)
        for (std::size_t y = x + 1; y < shell.size(); ++y)
          for (std::size_t w = y + 1; w < shell.size(); ++w)
            truth_triples.insert({shell[x], shell[y], shell[w]});
    }
    {
      std::vector<char> incident((std::size_t)n, 0);
      for (i32 p = 0; p < n; ++p) {
        const std::vector<i32>& up = forward_edges[(std::size_t)p];
        sum_edges += (long long)up.size();
        degree_g_max = std::max(degree_g_max, (long long)up.size());
        for (i32 z : up) incident[(std::size_t)z] = 1;
        for (std::size_t i = 0; i < up.size(); ++i) {
          const i32 u = up[i];
          for (i32 v : forward_edges[(std::size_t)u]) {
            ++triangle_probes;
            if (!incident[(std::size_t)v]) continue;
            ++triangles;
            const bool keep = triple_admissible(pts, p, u, v, smax, &triple_tests);
            if (keep) ++triples_kept;
            else if (truth_triples.count({p, u, v}) != 0) ++triple_missing;
            // K4 : un quatrième sommet w > v adjacent à p, u et v.
            for (i32 w : forward_edges[(std::size_t)v]) {
              ++k4_probes;
              if (!incident[(std::size_t)w]) continue;
              if (!std::binary_search(forward_edges[(std::size_t)u].begin(),
                                      forward_edges[(std::size_t)u].end(), w)) continue;
              ++k4;
              if (keep) ++k4_kept;
            }
          }
        }
        for (i32 z : up) incident[(std::size_t)z] = 0;
      }
    }
    sum_clique_seconds +=
        std::chrono::duration<double>(std::chrono::steady_clock::now() - c0).count();
    sum_triangles += triangles;
    sum_k4 += k4;
    sum_triples_kept += triples_kept;
    sum_k4_kept += k4_kept;
    sum_triple_tests += triple_tests;
    sum_triple_missing += triple_missing;
    sum_triangle_probes += triangle_probes;
    sum_k4_probes += k4_probes;

    printf("  nuage %d digest=%016llx statut ok  vraies=%zu admises ouvert=%lld ferme=%lld"
           "  vif=%lld  triangles=%lld K4=%lld contre arites 3/4 vraies=%lld/%lld\n", r, digest,
           truth.size(), admitted_open, admitted_closed, admitted_live, triangles, k4,
           true_arity3, true_arity4);
    ++decided;
    sum_true += (long long)truth.size();
    sum_admitted_closed += admitted_closed;
    sum_admitted_open += admitted_open;
    sum_admitted_live += admitted_live;
    sum_total += (long long)n * (n - 1) / 2;
    sum_missing_closed += missing_closed;
    sum_missing_open += missing_open;
    sum_missing_live += missing_live;
    sum_ball_points += ball_points;
    sum_seconds += std::chrono::duration<double>(t1 - t0).count();
  }

  if (decided == 0) { printf("ECHEC : aucun nuage decide\n"); return 3; }
  const double d = (double)decided;
  printf("n=%d smax=%d emprise=%d^3 graine=%lld nuages demandes=%d decides=%d refuses=%d\n",
         n, smax, span, seed, repeats, decided, refused_status);
  printf("  paires totales=%.0f  ADMISES ouvert=%.0f (%.3f%%)  ferme=%.0f (%.3f%%)"
         "  VRAIES=%.0f (%.3f%%)\n",
         sum_total / d, sum_admitted_open / d,
         100.0 * (double)sum_admitted_open / (double)sum_total, sum_admitted_closed / d,
         100.0 * (double)sum_admitted_closed / (double)sum_total, sum_true / d,
         100.0 * (double)sum_true / (double)sum_total);
  printf("  admises/vraies ouvert=%.2f ferme=%.2f   admises/point ouvert=%.1f ferme=%.1f"
         "   vraies/point=%.1f\n",
         sum_true ? (double)sum_admitted_open / (double)sum_true : 0.0,
         sum_true ? (double)sum_admitted_closed / (double)sum_true : 0.0,
         sum_admitted_open / d / n, sum_admitted_closed / d / n, sum_true / d / n);
  if (mutant == 1)
    printf("  le MUTANT par directions vives admet %.0f paires et en REFUTE %.0f de vraies\n",
           sum_admitted_live / d, sum_missing_live / d);
  else
    printf("  le mutant par directions vives n'est PAS evalue en campagne (--mutant 0) ;"
           " il reste juge par les fixtures\n");
  printf("  vraies REFUTEES : ouvert=%.0f  ferme=%.0f  (doivent etre ZERO : les deux"
         " lemmes sont necessaires)\n", sum_missing_open / d, sum_missing_closed / d);
  printf("  points de boule diametrale visites=%.0f  secondes filtre=%.2f"
         "  secondes matrice des rangs=%.2f  secondes cliques=%.2f\n",
         sum_ball_points / d, sum_seconds / d, sum_rank_seconds / d, sum_clique_seconds / d);
  printf("  GRAPHE ADMISSIBLE : aretes=%.0f  degre moyen=%.1f  degre max=%lld\n",
         sum_edges / d, 2.0 * (double)sum_edges / d / (double)n, degree_g_max);
  printf("  CLIQUES : triangles=%.0f pour %.0f supports d'arite 3 vrais (facteur %.1f)"
         "   K4=%.0f pour %.0f supports d'arite 4 vrais (facteur %.1f)\n",
         sum_triangles / d, sum_true_arity3 / d,
         sum_true_arity3 ? (double)sum_triangles / (double)sum_true_arity3 : 0.0,
         sum_k4 / d, sum_true_arity4 / d,
         sum_true_arity4 ? (double)sum_k4 / (double)sum_true_arity4 : 0.0);
  printf("  par point : triangles=%.1f  K4=%.1f  sondes triangle=%.1f  sondes K4=%.1f\n",
         sum_triangles / d / n, sum_k4 / d / n, sum_triangle_probes / d / n,
         sum_k4_probes / d / n);
  printf("  LEMME DE TRIPLE : %.0f triangles retenus sur %.0f (%.1f%%), %.0f pour un support"
         " d'arite 3 vrai (facteur %.1f)   K4 dont le triple survit=%.0f (facteur %.1f)\n",
         sum_triples_kept / d, sum_triangles / d,
         sum_triangles ? 100.0 * (double)sum_triples_kept / (double)sum_triangles : 0.0,
         sum_true_arity3 / d,
         sum_true_arity3 ? (double)sum_triples_kept / (double)sum_true_arity3 : 0.0,
         sum_k4_kept / d,
         sum_true_arity4 ? (double)sum_k4_kept / (double)sum_true_arity4 : 0.0);
  printf("  triples VRAIS refutes par le lemme=%lld  (doit etre ZERO)  tests=%.0f\n",
         sum_triple_missing, sum_triple_tests / d);
  if (ranks == 1) {
    printf("  rang k-NN maximum : admises=%d  vraies=%d  (sur %lld paires admises)\n",
           rank_max_admitted, rank_max_true, rank_samples);
    long long cumulative = 0;
    printf("  histogramme des rangs (classe = rangs de a a b : part cumulee) :");
    for (int i = 0; i < 20 && (1 << i) <= n; ++i) {
      cumulative += rank_histogram[(std::size_t)i];
      if (rank_samples > 0)
        printf(" [%d,%d]:%.3f", 1 << i, (1 << (i + 1)) - 1,
               (double)cumulative / (double)rank_samples);
    }
    printf("\n");
  } else {
    printf("  rangs k-NN NON mesures (--ranks 0)\n");
  }

  // LES PLANCHERS. Sans eux, un catalogue vide rendait `vraies=0, refutees=0, OK` :
  // la porte etait verte precisement quand le juge etait mort.
  if (sum_true < min_true) {
    printf("ECHEC : plancher de verite non atteint, %lld paires vraies pour %lld exigees\n",
           sum_true, min_true);
    return 3;
  }
  if (sum_admitted_open < min_admitted) {
    printf("ECHEC : plancher d'admission non atteint, %lld pour %lld exigees\n",
           sum_admitted_open, min_admitted);
    return 3;
  }
  if (refused_status != 0) {
    printf("ECHEC : %d nuage(s) au statut non kOk — une moyenne partielle n'est pas un recu\n",
           refused_status);
    return 3;
  }
  if (sum_triple_missing != 0) {
    printf("ECHEC : le lemme de triple a refute %lld triples reels — il est FAUX\n",
           sum_triple_missing);
    return 1;
  }
  if (sum_missing_open != 0 || sum_missing_closed != 0) {
    printf("ECHEC : le lemme du demi-boule a refute %lld (ouvert) et %lld (ferme) paires"
           " reelles — il est FAUX\n", sum_missing_open, sum_missing_closed);
    return 1;
  }
  printf("OK : aucune paire reelle refutee par le balayage exact\n");
  return 0;
}
