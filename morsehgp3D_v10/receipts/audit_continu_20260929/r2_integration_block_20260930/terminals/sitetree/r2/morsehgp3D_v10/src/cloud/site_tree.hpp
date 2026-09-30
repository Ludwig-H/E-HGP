// Index unique des sites : arbre k-d a coupe mediane sur l'axe le plus etendu, boites entieres serrees.
//
// Requetes exactes en entiers (coordonnees <= 21 bits : distances carrees < 2^45, i64 suffit) :
// K-ieme distance ponderee D_K(q) (multiplicites comprises) et voisins dans une boule fermee.
//
// Les sorties ne dependent pas de la forme de l'arbre : chaque requete rend un ensemble defini par les seules
// coordonnees (les `count` plus petites cles exactes departagees par indice, les sites d'une boule fermee
// tries par indice, D_K(q)). L'arbre ne decide que de l'elagage.
#pragma once

#include <vector>

#include "arith/geometry.hpp"
#include "cloud/cloud.hpp"

namespace mhgp10 {

class SiteTree {
 public:
  static constexpr u32 kLeaf = 16;

  explicit SiteTree(const Cloud& cloud);

  // D_K(q) : plus petit r2 tel que le poids des sites a distance carree <= r2 de q atteigne K.
  // q entier ; K >= 1 ; si le poids total est < K, rend ~0.
  u64 kth_distance(i64 qx, i64 qy, i64 qz, u64 k) const;

  // Sites (indices, ordre croissant) a distance carree <= r2 de q.
  void within(i64 qx, i64 qy, i64 qz, u64 r2, std::vector<u32>& out) const;

  // Requetes a centre rationnel c = anchor + N/D (anchor sur la sphere de rayon r^2 = |N|^2 / D^2).
  // Cle exacte s(z) = D |z - a|^2 - 2 N.(z - a) = D (|z - c|^2 - r^2) : meme ordre que la distance a c.
  //
  // Preconditions de l'appelant, non verifiees a l'execution :
  //  - representation (celle de geom::side_key) : la cle de chaque site et ses calculs intermediaires tiennent en
  //    i128. C'est le cas pour des sites et une ancre u18 avec un centre de l'une des formes construites par
  //    geom::center2/3/4 sur des points u18 (D < 2^82, |N_i| < 2^100 : |s| < 2^122), ou un centre sur un site
  //    (N = 0, D = 1) ; sur un nuage de 19 a 21 bits, seulement pour un centre sur un site ou un milieu (center2) ;
  //  - immuabilite : l'arbre emprunte le Cloud (reference) et fige a la construction ses coordonnees, ses boites et le
  //    drapeau de domaine des sites ; le Cloud doit survivre a l'arbre et ne pas etre modifie tant qu'il sert.
  // Sous ces preconditions, les sorties sont exactes pour TOUT centre, dans le nuage ou non, et sous tout mode
  // d'arrondi : filtre flottant a marge fixe (elagage des boites en double, decisions sur les sites par la cle exacte)
  // si filtered() est vrai, repli exact sinon (balayage de tous les sites par la cle exacte, O(n) par requete, memes
  // sorties). Hors du domaine du filtre, l'erreur des distances approchees croit comme 2^-53 |c|^2 et une marge fixe ne
  // vaut plus (constat G1 de l'audit independant du 29 septembre 2026). Les appels de la tour sont dans le domaine en
  // FE_TONEAREST : centre de MEB certifiee (dans conv(F), donc dans le cube) ou centre sur un site. Cout : le chemin
  // filtre n'a pas de borne sous-lineaire garantie (nearest peut collecter puis trier Theta(n) candidats, closed_ball
  // trie ses sorties).
  //
  // `count` sites de plus petite cle (departage par indice), ordre croissant ; out = (cle, site). count est plafonne
  // a 64.
  void nearest(const geom::P3& anchor, const geom::Center& c, u32 count, std::vector<std::pair<i128, u32>>& out) const;
  // Sites de cle < 0 (interieur strict) et = 0 (coquille), tries par indice.
  void closed_ball(const geom::P3& anchor, const geom::Center& c, std::vector<u32>& interior, std::vector<u32>& shell) const;
  // Chemin d'une requete : vrai si elle est servie par le filtre flottant, faux si elle passe par le repli exact.
  // Domaine complet du filtre (vrai si et seulement si les six conditions tiennent), L = 2^18 - 1 :
  //  (1) mode d'arrondi FE_TONEAREST dans le fil appelant, lu par std::fegetround a chaque appel (hypothese de la
  //      preuve de la marge ; doctrine v4 : hors de ce mode, le filtre est coupe). Le mode se change par
  //      std::fesetround ; un changement du seul registre MXCSR hors de <cfenv> (intrinseques SSE) n'est pas vu par
  //      glibc x86-64, qui lit le mot de controle x87, et sort du contrat ;
  //  (2) tous les sites dans le cube ferme [0, L]^3 (drapeau fige a la construction) ;
  //  (3) 0 < D <= 2^82 ;
  //  (4) ancre dans le cube : 0 <= a_i <= L ;
  //  (5) |N_i| <= 2^100 pour chaque axe ;
  //  (6) centre dans le cube : 0 <= D a_i + N_i <= L D pour chaque axe.
  // (3), (4) et (5) sont testees avant (6) : produits < 2^101, aucun debordement pour tous N, D i128 et ancre i64.
  // (5) ne decide jamais seule : sous (3), (4) et (6), |N_i| <= L D < 2^100.
  // filtered() == false n'est PAS un refus d'une requete forgee : nearest et closed_ball passent alors par le repli
  // exact, qui evalue geom::side_key sur tous les sites. Les preconditions de representation et d'immuabilite restent
  // celles de l'appelant ; hors de celles-ci, aucune sortie n'est garantie (debordement i128 possible).
  bool filtered(const geom::P3& anchor, const geom::Center& c) const;

  const Cloud& cloud() const { return cloud_; }

 private:
  struct Node {
    double bmin[3], bmax[3];  // boite entiere exacte (entiers < 2^21 representes exactement)
    u32 lo, hi;               // intervalle [lo, hi) de l'ordre de l'arbre
    u32 left, right;          // enfants (kNone pour une feuille)
  };
  u32 build(u32 lo, u32 hi);

  const Cloud& cloud_;
  std::vector<Node> nodes_;
  std::vector<u32> site_;               // ordre de l'arbre -> site
  std::vector<double> px_, py_, pz_;    // coordonnees dans l'ordre de l'arbre
  u32 root_ = kNone;
  bool sites_u18_ = false;              // tous les sites dans le cube u18 (condition du filtre flottant)
};

}  // namespace mhgp10
