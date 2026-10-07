// Fermeture d'une boite de centres T0 : hi peut valoir 2^B, contrairement aux extremites de num::Box.
// Le contact avec hi est conserve par ces rejets ; la propriete demi-ouverte reste au catalogue.
#pragma once

#include "num/geometry.hpp"

namespace mhgp12::num {

class CenterRegion {
 public:
  static Result<CenterRegion> make(std::array<i64, 3> lo, std::array<i64, 3> hi) noexcept;
  const std::array<i64, 3>& lo() const noexcept { return lo_; }
  const std::array<i64, 3>& hi() const noexcept { return hi_; }

 private:
  CenterRegion(std::array<i64, 3> lo, std::array<i64, 3> hi) noexcept : lo_(lo), hi_(hi) {}
  std::array<i64, 3> lo_, hi_;
};

enum class CenterLineRelation { degenerate, disjoint, intersects };

// Egalite des distances sur la fermeture : a==b donne le lieu entier, donc true.
bool bisector_meets(Point a, Point b, const CenterRegion& region) noexcept;
// Non-alignement requis pour une droite. Trois points alignes/doublons rendent degenerate, pas disjoint.
// Aucune condition d'angle : une face obtuse peut appartenir a un support q4 strictement positif.
CenterLineRelation center_line_meets(Point a, Point b, Point c, const CenterRegion& region) noexcept;

}  // namespace mhgp12::num
