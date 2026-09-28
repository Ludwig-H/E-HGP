// Index unique des sites : arbre binaire implicite sur l'ordre de Morton du Cloud, boites entieres.
//
// Requetes exactes en entiers (coordonnees <= 21 bits : distances carrees < 2^45, i64 suffit) :
// K-ieme distance ponderee D_K(q) (multiplicites comprises) et voisins dans une boule fermee.
#pragma once

#include <vector>

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

  const Cloud& cloud() const { return cloud_; }

 private:
  struct Node {
    u32 lo, hi;       // intervalle de sites [lo, hi)
    u32 left, right;  // enfants (kNone pour une feuille)
    i64 bmin[3], bmax[3];
  };
  u32 build(u32 lo, u32 hi);
  static u64 box_dist2(const Node& nd, i64 qx, i64 qy, i64 qz);

  const Cloud& cloud_;
  std::vector<Node> nodes_;
  u32 root_ = kNone;
};

}  // namespace mhgp10
