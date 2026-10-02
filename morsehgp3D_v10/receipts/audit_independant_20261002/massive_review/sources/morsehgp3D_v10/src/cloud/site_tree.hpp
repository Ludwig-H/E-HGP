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
  // L'elagage des boites se fait en double avec une marge prouvee large (erreur < 1e-3 en u18) ; toutes les
  // decisions sur les sites sont exactes.
  //
  // `count` sites de plus petite cle (departage par indice), ordre croissant ; out = (cle, site).
  void nearest(const geom::P3& anchor, const geom::Center& c, u32 count, std::vector<std::pair<i128, u32>>& out) const;
  // Sites de cle < 0 (interieur strict) et = 0 (coquille), tries par indice.
  void closed_ball(const geom::P3& anchor, const geom::Center& c, std::vector<u32>& interior, std::vector<u32>& shell) const;

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
};

}  // namespace mhgp10
