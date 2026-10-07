// Port du lemme Z (R2 generator.cpp 4647e902 et v10 777406b82), reformule en T0, sans Sphere ni flottant.
// La provenance complete et les differences de contrat sont dans center_region.provenance.json.
// Repere local (v12, docs/CONTRAT_NUMERIQUE.md, paragraphe 3) : les deux tests sont invariants par translation ; ils
// se calculent sur les coordonnees moins le coin minimal du repere {region fermee, sites}, d'etendue s, et non plus
// sur les coordonnees absolues (2B+4 bits). Voie i64 pour les formes affines aux paliers etroit et moyen, i128 au
// palier large ; les tests du zonogone restent en i128 a toute etendue.
#include "num/center_region.hpp"

#include <algorithm>

namespace mhgp12::num {
namespace {

// M=2^s, coordonnees locales dans [0, M), 0<=lo<hi<=M. Les points sont deja certifies, les bornes possedees et validees.
// A=sum(a^2-b^2) : chaque somme partielle <3M^2 ; p=A-sum((lo+hi)u) : <9M^2.
// c_ij=u_i*v_j-u_j*v_i : <2M^2. Chaque produit du test SAT est elargi AVANT multiplication :
// |v_k*p0-u_k*p1|<18M^3 ; sum(j!=k) h_j*|c_kj|<4M^3, chaque terme et somme partielle compris.
using Test = i128;
static_assert(TierBudgets<Tier::medium>::dot + 2 <= 63 && 3 * kMaxSpan + 5 <= 127);
static_assert(2 * kMaxSpan + 4 <= 127);

using Local = std::array<i64, 3>;
template <class Affine>
struct Bisector {
  std::array<Affine, 3> u{};
  Affine constant = 0;
};

// Repere local des arguments : coin minimal et etendue de {lo, hi, sites}.
struct LocalFrame {
  Local origin{};
  Tier tier = Tier::narrow;
};
template <class... P>
LocalFrame local_frame(const CenterRegion& region, P... points) noexcept {
  Frame frame;
  (void)frame.add_box({static_cast<u64>(region.lo()[0]), static_cast<u64>(region.lo()[1]),
                       static_cast<u64>(region.lo()[2])},
                      {static_cast<u64>(region.hi()[0]), static_cast<u64>(region.hi()[1]),
                       static_cast<u64>(region.hi()[2])});
  (frame.add_site(points.x(), points.y(), points.z()), ...);
  return {{static_cast<i64>(frame.origin()[0]), static_cast<i64>(frame.origin()[1]),
           static_cast<i64>(frame.origin()[2])}, frame.tier()};
}
Local local(Point p, const LocalFrame& frame) noexcept {
  return {i64{p.x()} - frame.origin[0], i64{p.y()} - frame.origin[1], i64{p.z()} - frame.origin[2]};
}

template <class Affine>
Bisector<Affine> difference(const Local& x, const Local& y) noexcept {
  Bisector<Affine> result;
  for (int axis = 0; axis < 3; ++axis) {
    result.u[axis] = Affine{x[axis]} - y[axis];
    result.constant += Affine{x[axis]} * x[axis] - Affine{y[axis]} * y[axis];
  }
  return result;
}

template <class Affine>
Affine centered(const Bisector<Affine>& f, const Local& lo, const Local& hi) noexcept {
  Affine result = f.constant;
  for (int axis = 0; axis < 3; ++axis)
    result -= (Affine{lo[axis]} + hi[axis]) * f.u[axis];
  return result;
}

template <class Affine>
bool bisector_local(const Local& a, const Local& b, const Local& lo, const Local& hi) noexcept {
  const auto f = difference<Affine>(a, b);
  Affine lower = f.constant, upper = f.constant;
  for (int axis = 0; axis < 3; ++axis) {
    const Affine at_lo = 2 * f.u[axis] * lo[axis];
    const Affine at_hi = 2 * f.u[axis] * hi[axis];
    lower -= f.u[axis] >= 0 ? at_hi : at_lo;
    upper -= f.u[axis] >= 0 ? at_lo : at_hi;
  }
  return lower <= 0 && upper >= 0;  // Extremes exacts de |a-z|^2-|b-z|^2, contacts inclus.
}

template <class Affine>
CenterLineRelation line_local(const Local& a, const Local& b, const Local& c, const Local& lo,
                              const Local& hi) noexcept {
  const auto f = difference<Affine>(a, b), g = difference<Affine>(a, c);
  std::array<std::array<Affine, 3>, 3> cross{};
  bool rank_two = false;
  for (int i = 0; i < 3; ++i)
    for (int j = i + 1; j < 3; ++j) {
      const Affine value = g.u[i] * f.u[j] - f.u[i] * g.u[j];
      cross[i][j] = cross[j][i] = value < 0 ? -value : value;
      rank_two = rank_two || value != 0;
    }
  if (!rank_two) return CenterLineRelation::degenerate;
  const Affine p0 = centered(f, lo, hi), p1 = centered(g, lo, hi);
  // Image de la fermeture par les deux differences de distances : zonogone p+sum[-h,h]*(u,v).
  // Rang deux et h>0 : les normales (v_k,-u_k) a ses generateurs suffisent a tester l'origine.
  // Le facteur deux commun a p et aux rayons du lemme Z historique est divise exactement.
  for (int k = 0; k < 3; ++k) {
    if (f.u[k] == 0 && g.u[k] == 0) continue;
    const Test signed_left = Test{g.u[k]} * p0 - Test{f.u[k]} * p1;
    const Test left = signed_left < 0 ? -signed_left : signed_left;
    Test right = 0;
    for (int j = 0; j < 3; ++j)
      right += Test{hi[j] - lo[j]} * cross[k][j];
    if (left > right) return CenterLineRelation::disjoint;
  }
  return CenterLineRelation::intersects;
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
  const auto frame = local_frame(region, a, b);
  const Local lo{region.lo()[0] - frame.origin[0], region.lo()[1] - frame.origin[1], region.lo()[2] - frame.origin[2]};
  const Local hi{region.hi()[0] - frame.origin[0], region.hi()[1] - frame.origin[1], region.hi()[2] - frame.origin[2]};
  if (frame.tier != Tier::wide) return bisector_local<i64>(local(a, frame), local(b, frame), lo, hi);
  return bisector_local<i128>(local(a, frame), local(b, frame), lo, hi);
}

CenterLineRelation center_line_meets(Point a, Point b, Point c, const CenterRegion& region) noexcept {
  const auto frame = local_frame(region, a, b, c);
  const Local lo{region.lo()[0] - frame.origin[0], region.lo()[1] - frame.origin[1], region.lo()[2] - frame.origin[2]};
  const Local hi{region.hi()[0] - frame.origin[0], region.hi()[1] - frame.origin[1], region.hi()[2] - frame.origin[2]};
  if (frame.tier != Tier::wide) return line_local<i64>(local(a, frame), local(b, frame), local(c, frame), lo, hi);
  return line_local<i128>(local(a, frame), local(b, frame), local(c, frame), lo, hi);
}

}  // namespace mhgp12::num
