// Port du lemme Z (R2 generator.cpp 4647e902 et v10 777406b82), reformule en T0, sans Sphere ni flottant.
// La provenance complete et les differences de contrat sont dans center_region.provenance.json.
#include "num/center_region.hpp"

namespace mhgp11::num {
namespace {

// M=2^B, |u|<M, 0<=lo<hi<=M. Les points sont deja certifies, les bornes sont possedees et validees.
// A=sum(a^2-b^2) : chaque somme partielle <3M^2 ; p=A-sum((lo+hi)u) : <9M^2.
// c_ij=u_i*v_j-u_j*v_i : <2M^2. Chaque produit du test SAT est elargi AVANT multiplication :
// |v_k*p0-u_k*p1|<18M^3 ; sum(j!=k) h_j*|c_kj|<4M^3, chaque terme et somme partielle compris.
using Affine = Int<2 * kCoordBits + 4>;
using Test = Int<3 * kCoordBits + 5>;
static_assert(2 * kCoordBits + 4 <= 63 && 3 * kCoordBits + 5 <= 127);
static_assert(std::same_as<Affine, i64>);

struct Bisector {
  std::array<Affine, 3> u{};
  Affine constant = 0;
};

Bisector difference(Point a, Point b) noexcept {
  Bisector result;
  const auto x = a.coordinates(), y = b.coordinates();
  for (int axis = 0; axis < 3; ++axis) {
    result.u[axis] = Affine{x[axis]} - y[axis];
    result.constant += Affine{x[axis]} * x[axis] - Affine{y[axis]} * y[axis];
  }
  return result;
}

Affine centered(const Bisector& f, const CenterRegion& region) noexcept {
  Affine result = f.constant;
  for (int axis = 0; axis < 3; ++axis)
    result -= (region.lo()[axis] + region.hi()[axis]) * f.u[axis];
  return result;
}

}  // namespace

Result<CenterRegion> CenterRegion::make(std::array<i64, 3> lo, std::array<i64, 3> hi) noexcept {
  constexpr i64 maximum = i64{1} << kCoordBits;
  for (int axis = 0; axis < 3; ++axis)
    if (lo[axis] < 0 || lo[axis] >= hi[axis] || hi[axis] > maximum)
      return fail(Reason::parameter_out_of_range);
  return CenterRegion(lo, hi);
}

bool bisector_meets(Point a, Point b, const CenterRegion& region) noexcept {
  const auto f = difference(a, b);
  Affine lower = f.constant, upper = f.constant;
  for (int axis = 0; axis < 3; ++axis) {
    const Affine at_lo = 2 * f.u[axis] * region.lo()[axis];
    const Affine at_hi = 2 * f.u[axis] * region.hi()[axis];
    lower -= f.u[axis] >= 0 ? at_hi : at_lo;
    upper -= f.u[axis] >= 0 ? at_lo : at_hi;
  }
  return lower <= 0 && upper >= 0;  // Extremes exacts de |a-z|^2-|b-z|^2, contacts inclus.
}

CenterLineRelation center_line_meets(Point a, Point b, Point c, const CenterRegion& region) noexcept {
  const auto f = difference(a, b), g = difference(a, c);
  std::array<std::array<Affine, 3>, 3> cross{};
  bool rank_two = false;
  for (int i = 0; i < 3; ++i)
    for (int j = i + 1; j < 3; ++j) {
      const Affine value = g.u[i] * f.u[j] - f.u[i] * g.u[j];
      cross[i][j] = cross[j][i] = value < 0 ? -value : value;
      rank_two = rank_two || value != 0;
    }
  if (!rank_two) return CenterLineRelation::degenerate;
  const Affine p0 = centered(f, region), p1 = centered(g, region);
  // Image de la fermeture par les deux differences de distances : zonogone p+sum[-h,h]*(u,v).
  // Rang deux et h>0 : les normales (v_k,-u_k) a ses generateurs suffisent a tester l'origine.
  // Le facteur deux commun a p et aux rayons du lemme Z historique est divise exactement.
  for (int k = 0; k < 3; ++k) {
    if (f.u[k] == 0 && g.u[k] == 0) continue;
    const Test signed_left = Test{g.u[k]} * p0 - Test{f.u[k]} * p1;
    const Test left = signed_left < 0 ? -signed_left : signed_left;
    Test right = 0;
    for (int j = 0; j < 3; ++j)
      right += Test{region.hi()[j] - region.lo()[j]} * cross[k][j];
    if (left > right) return CenterLineRelation::disjoint;
  }
  return CenterLineRelation::intersects;
}

}  // namespace mhgp11::num
